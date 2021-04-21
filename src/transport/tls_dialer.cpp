/*
 * Copyright (C) 2015-2018 ZhengHaiTao <ming8ren@163.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pump/transport/tls_dialer.h"
#include "pump/transport/tls_transport.h"

namespace pump {
namespace transport {

    tls_dialer::tls_dialer(
        void *xcred,
        const address &local_address,
        const address &remote_address,
        int64_t dial_timeout,
        int64_t handshake_timeout) noexcept
      : base_dialer(TLS_DIALER, local_address, remote_address, dial_timeout),
        xcred_(xcred),
        xcred_owner_(false),
        handshake_timeout_(handshake_timeout) {
        if (!xcred_) {
            xcred_owner_ = true;
            xcred_ = ssl::create_tls_client_certificate();
        }
    }

    tls_dialer::~tls_dialer() {
        if (xcred_owner_) {
            ssl::destory_tls_certificate(xcred_);
        }
    }

    int32_t tls_dialer::start(
        service *sv, 
        const dialer_callbacks &cbs) {
        PUMP_DEBUG_FAILED_RUN(
            xcred_ == nullptr, 
            "tls_dialer: start failed for cert invalid",
            return ERROR_INVALID);

        PUMP_DEBUG_FAILED_RUN(
            !__set_state(TRANSPORT_INITED, TRANSPORT_STARTING), 
            "tcp_transport: start failed for transport state incorrect",
            return ERROR_INVALID);

        PUMP_DEBUG_FAILED_RUN(
            sv == nullptr,
            "tcp_transport: start failed for service invalid",
            return ERROR_INVALID);
        __set_service(sv);

        PUMP_DEBUG_FAILED_RUN(
            !cbs.dialed_cb || !cbs.stopped_cb || !cbs.timeouted_cb, 
            "tcp_transport: start failed for callbacks invalid",
            return ERROR_INVALID);
        cbs_ = cbs;

        toolkit::defer cleanup([&]() {
            __stop_dial_timer();
            __stop_dial_tracker();
            __close_dial_flow();
            __set_state(TRANSPORT_STARTING, TRANSPORT_ERROR);
        });

        if (!__open_dial_flow()) {
            PUMP_DEBUG_LOG("tls_dialer: start failed for opening flow failed");
            return ERROR_FAULT;
        }

        if (!__start_dial_timer(pump_bind(&tls_dialer::on_timeout, shared_from_this()))) {
            PUMP_DEBUG_LOG("tls_dialer: start failed for starting timer failed");
            return ERROR_FAULT;
        }

        if (flow_->post_connect(remote_address_) != flow::FLOW_ERR_NO) {
            PUMP_DEBUG_LOG("tls_dialer: start failed for posting connect failed");
            return ERROR_FAULT;
        }

        if (!__start_dial_tracker(shared_from_this())) {
            PUMP_WARN_LOG("tls_dialer: start failed for starting tracker failed");
            return ERROR_FAULT;
        }

        __set_state(TRANSPORT_STARTING, TRANSPORT_STARTED);

        cleanup.clear();

        return ERROR_OK;
    }

    void tls_dialer::stop() {
        // When stopping done, tracker event will trigger stopped callback.
        if (__set_state(TRANSPORT_STARTED, TRANSPORT_STOPPING)) {
            __stop_dial_timer();
            __close_dial_flow();
            __post_channel_event(shared_from_this(), 0);
            return;
        } else if (__set_state(TRANSPORT_HANDSHAKING, TRANSPORT_STOPPING)) {
            if(handshaker_) {
                handshaker_->stop();
            }
            return;
        }

        // If in timeouting status at the moment, it means that dialer is timeout
        // but hasn't triggered tracker event callback yet. So we just set it to
        // stopping status, then tracker event will trigger stopped callabck.
        if (__set_state(TRANSPORT_TIMEOUTING, TRANSPORT_STOPPING)) {
            return;
        }
    }

    void tls_dialer::on_send_event() {
        // Stop dial timer.
        __stop_dial_timer();
        // Stop dial tracker.
        __stop_dial_tracker();

        auto flow = flow_.get();
        address local_address, remote_address;
        bool success = (flow->connect(&local_address, &remote_address) == 0);
        auto next_status = success ? TRANSPORT_HANDSHAKING : TRANSPORT_ERROR;
        if (!__set_state(TRANSPORT_STARTING, next_status) &&
            !__set_state(TRANSPORT_STARTED, next_status)) {
            PUMP_DEBUG_LOG("tls_dialer: handle send event failed for not in started");
            __close_dial_flow();
            __trigger_interrupt_callbacks();
            return;
        }

        do {
            if (!success) {
                PUMP_DEBUG_LOG("tls_dialer: handle send event failed for dialing failed");
                __close_dial_flow();
                break;
            }

            // If handshaker is started error, handshaked callback will be triggered. So
            // we do nothing at here when started error. But if dialer stopped befere
            // here, we shuold stop handshaking.
            handshaker_.reset(
                object_create<tls_handshaker>(),
                object_delete<tls_handshaker>);
            if(!handshaker_) {
                PUMP_WARN_LOG(
                    "tls_dialer: handle send event failed for creating handshaker failed");
                if (__set_state(TRANSPORT_HANDSHAKING, TRANSPORT_ERROR)) {
                    break;
                }
                return;
            }
            handshaker_->init(
                flow->unbind(), 
                true, 
                xcred_, 
                local_address, 
                remote_address);

            tls_handshaker::tls_handshaker_callbacks tls_cbs;
            tls_cbs.handshaked_cb =
                pump_bind(&tls_dialer::on_handshaked, shared_from_this(), _1, _2);
            tls_cbs.stopped_cb =
                pump_bind(&tls_dialer::on_handshake_stopped, shared_from_this(), _1);
            if (handshaker_->start(get_service(), handshake_timeout_, tls_cbs)) {
                if (!__is_state(TRANSPORT_HANDSHAKING)) {
                    PUMP_DEBUG_LOG("tls_dialer: handle send event failed for not in started");
                    handshaker_->stop();
                    handshaker_.reset();
                    break;
                }
            } else if (__set_state(TRANSPORT_HANDSHAKING, TRANSPORT_ERROR)) {
                PUMP_DEBUG_LOG(
                    "tls_dialer: handle send event failed for handshaker start failed");
                handshaker_.reset();
                break;
            }
            return;
        } while(false);

        base_transport_sptr tls_transport;
        cbs_.dialed_cb(tls_transport, false);
    }

    void tls_dialer::on_timeout(tls_dialer_wptr wptr) {
        auto dialer = wptr.lock();
        if (dialer) {
            if (dialer->__set_state(TRANSPORT_STARTED, TRANSPORT_TIMEOUTING)) {
                PUMP_DEBUG_LOG("tls_dialer: handle dialing timeout");
                dialer->__stop_dial_tracker();
                dialer->__post_channel_event(dialer, 0);
            }
        }
    }

    void tls_dialer::on_handshaked(
        tls_dialer_wptr wptr,
        tls_handshaker *handshaker,
        bool succ) {
        auto dialer = wptr.lock();
        if (!dialer) {
            handshaker->stop();
            return;
        }

        if (dialer->__set_state(TRANSPORT_STOPPING, TRANSPORT_STOPPED)) {
            dialer->cbs_.stopped_cb();
        } else if (dialer->__set_state(TRANSPORT_HANDSHAKING, TRANSPORT_FINISHED)) {
            tls_transport_sptr tls_transport;
            if (PUMP_LIKELY(succ)) {
                auto flow = handshaker->unlock_flow();
                auto &local_address = handshaker->get_local_address();
                auto &remote_address = handshaker->get_remote_address();

                tls_transport = tls_transport::create();
                if (tls_transport) {
                    tls_transport->init(flow, local_address, remote_address);
                } else {
                    PUMP_WARN_LOG(
                        "tls_dialer: handle handshaked event failed for creating transport failed");
                    succ = false;
                }
            }

            base_transport_sptr transport = tls_transport;
            dialer->cbs_.dialed_cb(transport, succ);
        }

        dialer->handshaker_.reset();
    }

    void tls_dialer::on_handshake_stopped(
        tls_dialer_wptr wptr,
        tls_handshaker *handshaker) {
        auto dialer = wptr.lock();
        if (dialer) {
            dialer->handshaker_.reset();
            dialer->__trigger_interrupt_callbacks();
        }
    }

    bool tls_dialer::__open_dial_flow() {
        // Init tls dialer flow.
        flow_.reset(
            object_create<flow::flow_tcp_dialer>(),
            object_delete<flow::flow_tcp_dialer>);
        if (!flow_) { 
            PUMP_WARN_LOG("tls_dialer: open flow failed for creating flow failed");
            return false;
        }
        if (flow_->init(shared_from_this(), local_address_) != flow::FLOW_ERR_NO) {
            PUMP_DEBUG_LOG("tls_dialer: open flow failed for initing flow failed");
            return false;
        }

        // Set channel fd
        poll::channel::__set_fd(flow_->get_fd());

        return true;
    }

    base_transport_sptr tls_sync_dialer::dial(
        service *sv,
        const address &local_address,
        const address &remote_address,
        int64_t connect_timeout,
        int64_t handshake_timeout) {
        if (dialer_) {
            return base_transport_sptr();
        }

        dialer_callbacks cbs;
        cbs.dialed_cb = pump_bind(&tls_sync_dialer::on_dialed, shared_from_this(), _1, _2);
        cbs.timeouted_cb = pump_bind(&tls_sync_dialer::on_timeouted, shared_from_this());
        cbs.stopped_cb = pump_bind(&tls_sync_dialer::on_stopped);

        dialer_ = tls_dialer::create(
                    local_address, 
                    remote_address, 
                    connect_timeout, 
                    handshake_timeout);
        if (!dialer_ || dialer_->start(sv, cbs) != ERROR_OK) {
            return base_transport_sptr();
        }

        return dial_promise_.get_future().get();
    }

    void tls_sync_dialer::on_dialed(
        tls_sync_dialer_wptr wptr,
        base_transport_sptr &transp,
        bool succ) {
        auto dialer = wptr.lock();
        if (dialer) {
            dialer->dial_promise_.set_value(transp);
        }
    }

    void tls_sync_dialer::on_timeouted(tls_sync_dialer_wptr wptr) {
        auto dialer = wptr.lock();
        if (dialer) {
            dialer->dial_promise_.set_value(base_transport_sptr());
        }
    }

    void tls_sync_dialer::on_stopped() {
        PUMP_ASSERT(false);
    }

}  // namespace transport
}  // namespace pump
