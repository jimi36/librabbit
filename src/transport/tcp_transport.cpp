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

#include "pump/transport/tcp_transport.h"

namespace pump {
namespace transport {

    tcp_transport::tcp_transport() noexcept
      : base_transport(TCP_TRANSPORT, nullptr, -1),
        last_send_iob_size_(0),
        last_send_iob_(nullptr),
        pending_send_cnt_(0),
        sendlist_(32) {
    }

    tcp_transport::~tcp_transport() {
        __clear_sendlist();
    }

    void tcp_transport::init(int32_t fd,
                             const address &local_address,
                             const address &remote_address) {
        local_address_ = local_address;
        remote_address_ = remote_address;

        // Set channel fd
        poll::channel::__set_fd(fd);
    }

    transport_error tcp_transport::start(
        service_ptr sv,
        const transport_callbacks &cbs) {
        if (flow_) {
            PUMP_ERR_LOG("tcp_transport::start: flow exists");
            return ERROR_INVALID;
        }

        if (!sv) {
            PUMP_ERR_LOG("tcp_transport::start: service invalid");
            return ERROR_INVALID;
        }

        if (!cbs.read_cb || !cbs.disconnected_cb || !cbs.stopped_cb) {
            PUMP_ERR_LOG("tcp_transport::start: callbacks invalid");
            return ERROR_INVALID;
        }

        if (!__set_status(TRANSPORT_INITED, TRANSPORT_STARTING)) {
            PUMP_ERR_LOG("tcp_transport::start: transport has started");
            return ERROR_INVALID;
        }

        // Set callbacks
        cbs_ = cbs;

        // Set service
        __set_service(sv);

        if (!__open_transport_flow()) {
            PUMP_ERR_LOG("tcp_transport::start: open flow failed");
            return ERROR_FAULT;
        }

        __set_status(TRANSPORT_STARTING, TRANSPORT_STARTED);

        return ERROR_OK;
    }

    void tcp_transport::stop() {
        while (__is_status(TRANSPORT_STARTED)) {
            // When in started status at the moment, stopping can be done, Then
            // tracker event callback will be triggered, we can trigger stopped
            // callabck at there.
            if (__set_status(TRANSPORT_STARTED, TRANSPORT_STOPPING)) {
                // Wait pending send count reduce to zero.
                while (pending_send_cnt_.load(std::memory_order_relaxed) != 0);
                // Shutdown transport flow.
                __shutdown_transport_flow();
                // If there is data to send, waiting sending finished.
                if (pending_send_size_.load(std::memory_order_acquire) == 0) {
                    __post_channel_event(shared_from_this(), 0);
                }
                return;
            }
        }

        // If in disconnecting status at the moment, it means transport is
        // disconnected but hasn't triggered tracker event callback yet. So we just
        // set stopping status to transport, and when tracker event callback
        // triggered, we will trigger stopped callabck at there.
        if (__set_status(TRANSPORT_DISCONNECTING, TRANSPORT_STOPPING)) {
            return;
        }
    }

    void tcp_transport::force_stop() {
        while (__is_status(TRANSPORT_STARTED)) {
            // When in started status at the moment, stopping can be done, Then
            // tracker event callback will be triggered, we can trigger stopped
            // callabck at there.
            if (__set_status(TRANSPORT_STARTED, TRANSPORT_STOPPING)) {
                __close_transport_flow();
                __post_channel_event(shared_from_this(), 0);
                return;
            }
        }

        // If in disconnecting status at the moment, it means transport is
        // disconnected but hasn't triggered tracker event callback yet. So we just
        // set stopping status to transport, and when tracker event callback
        // triggered, we will trigger stopped callabck at there.
        if (__set_status(TRANSPORT_DISCONNECTING, TRANSPORT_STOPPING)) {
            return;
        }
    }

    transport_error tcp_transport::read_for_once() {
        while (__is_status(TRANSPORT_STARTED)) {
            transport_error err = __async_read(READ_ONCE);
            if (err != ERROR_AGAIN) {
                return err;
            }
        }
        return ERROR_UNSTART;
    }

    transport_error tcp_transport::read_for_loop() {
        while (__is_status(TRANSPORT_STARTED)) {
            transport_error err = __async_read(READ_LOOP);
            if (err != ERROR_AGAIN) {
                return err;
            }
        }
        return ERROR_UNSTART;
    }

    transport_error tcp_transport::send(const block_t *b, int32_t size) {
        if (!b || size == 0) {
            PUMP_ERR_LOG("tcp_transport::send: buffer invalid ");
            return ERROR_INVALID;
        }

        // Add pending send count.
        pending_send_cnt_.fetch_add(1);

        auto ec = ERROR_OK;
        if (PUMP_UNLIKELY(!__is_status(TRANSPORT_STARTED))) {
            PUMP_ERR_LOG("tcp_transport::send: transport not started");
            ec = ERROR_UNSTART;
            goto end;
        }

        auto iob = toolkit::io_buffer::create();
        if (PUMP_UNLIKELY(!iob || !iob->append(b, size))) {
            PUMP_WARN_LOG("tcp_transport::send: new buffer failed");
            if (iob) {
                iob->sub_ref();
            }
            ec = ERROR_FAULT;
            goto end;
        }

        if (!__async_send(iob)) {
            PUMP_WARN_LOG("tcp_transport::send: async send failed");
            ec = ERROR_FAULT;
            goto end;
        }

    end:
        // Resuce pending send count.
        pending_send_cnt_.fetch_sub(1);

        return ec;
    }

    transport_error tcp_transport::send(toolkit::io_buffer_ptr iob) {
        if (!iob || iob->data_size() == 0) {
            PUMP_ERR_LOG("tcp_transport::send: io buffer invalid ");
            return ERROR_INVALID;
        }

        // Add pending send count.
        pending_send_cnt_.fetch_add(1);

        auto ec = ERROR_OK;
        if (PUMP_UNLIKELY(!__is_status(TRANSPORT_STARTED))) {
            PUMP_ERR_LOG("tcp_transport::send: transport not started");
            ec = ERROR_UNSTART;
            goto end;
        }

        iob->add_ref();

        if (!__async_send(iob)) {
            PUMP_WARN_LOG("tcp_transport::send: async send failed");
            ec = ERROR_FAULT;
            goto end;
        }
        
    end:
        // Resuce pending send count.
        pending_send_cnt_.fetch_sub(1);

        return ERROR_OK;
    }

#if defined(PUMP_HAVE_IOCP)
    void tcp_transport::on_read_event(net::iocp_task_ptr iocp_task) {
#else
    void tcp_transport::on_read_event() {
#endif
        auto flow = flow_.get();

        // Get and change read state to READ_PENDING.
        uint32_t pending_state = READ_PENDING;
        uint32_t old_state = read_state_.exchange(pending_state);

#if defined(PUMP_HAVE_IOCP)
        int32_t size = 0;
        const block_t *b = iocp_task->get_processed_data(&size);
#else
        block_t b[MAX_TRANSPORT_BUFFER_SIZE];
        int32_t size = flow->read(b, MAX_TRANSPORT_BUFFER_SIZE);
#endif
        if (PUMP_LIKELY(size > 0)) {
            // Read callback
            cbs_.read_cb(b, size);

            // If transport is not in started state and no buffer waiting send, then
            // interrupt this transport.
            if (!__is_status(TRANSPORT_STARTED) &&
                pending_send_size_.load(std::memory_order_acquire) == 0) {
                __interrupt_and_trigger_callbacks();
                return;
            }

            // If old read state is READ_ONCE, then change it to READ_NONE.
            if (old_state == READ_ONCE &&
                read_state_.compare_exchange_strong(pending_state, READ_NONE)) {
                return;
            }

#if defined(PUMP_HAVE_IOCP)
            if (flow->post_read(iocp_task) == flow::FLOW_ERR_ABORT) {
                PUMP_WARN_LOG("tcp_transport::on_read_event: want to read failed");
                __try_doing_disconnected_process();
            }
#else
            PUMP_DEBUG_CHECK(r_tracker_->set_tracked(true));
#endif
        } else {
            PUMP_WARN_LOG("tcp_transport::on_read_event: flow read failed");
            __try_doing_disconnected_process();
        }
    }

#if defined(PUMP_HAVE_IOCP)
    void tcp_transport::on_send_event(net::iocp_task_ptr iocp_task) {
#else
    void tcp_transport::on_send_event() {
#endif
        auto flow = flow_.get();

#if defined(PUMP_HAVE_IOCP)
        auto ret = flow->send(iocp_task);
#else
        auto ret = flow->send();
#endif
        if (ret == flow::FLOW_ERR_AGAIN) {
#if !defined(PUMP_HAVE_IOCP)
            PUMP_DEBUG_CHECK(s_tracker_->set_tracked(true));
#endif
            return;
        } else if (ret == flow::FLOW_ERR_ABORT) {
            PUMP_DEBUG_LOG("tcp_transport::on_send_event: send failed");
            __try_doing_disconnected_process();
            return;
        }

        // Free last send buffer.
        last_send_iob_->sub_ref();
        last_send_iob_ = nullptr;

        // If there are more buffers to send, we should send next one immediately.
        if (pending_send_size_.fetch_sub(last_send_iob_size_) > last_send_iob_size_) {
            if (!__send_once(flow, nullptr, false)) {
                PUMP_DEBUG_LOG("tcp_transport::on_send_event: send once failed");
                __try_doing_disconnected_process();
            }
            return;
        }

        if (__is_status(TRANSPORT_STOPPING)) {
            PUMP_DEBUG_LOG("tcp_transport::on_send_event: interrupt and callback");
            __interrupt_and_trigger_callbacks();
        }
    }

    bool tcp_transport::__open_transport_flow() {
        // Init tcp transport flow.
        PUMP_ASSERT(!flow_);
        flow_.reset(object_create<flow::flow_tcp>(), object_delete<flow::flow_tcp>);
        if (flow_->init(shared_from_this(), get_fd()) != flow::FLOW_ERR_NO) {
            PUMP_ERR_LOG("tcp_transport::__open_flow: flow init failed");
            return false;
        }

        return true;
    }

    transport_error tcp_transport::__async_read(uint32_t state) {
        uint32_t old_state = __change_read_state(state);
        if (old_state == READ_INVALID) {
            return ERROR_AGAIN;
        } else if (old_state >= READ_ONCE) {
            return ERROR_OK;
        }

#if defined(PUMP_HAVE_IOCP)
        if (flow_->post_read() == flow::FLOW_ERR_ABORT) {
            PUMP_ERR_LOG("tcp_transport::__async_read: want to read fialed");
            return ERROR_FAULT;
        }
#else
        if (!__start_read_tracker(shared_from_this())) {
            PUMP_ERR_LOG("tcp_transport::__async_read: start tracker fialed");
            return ERROR_FAULT;
        }
#endif
        return ERROR_OK;
    }

    bool tcp_transport::__async_send(toolkit::io_buffer_ptr iob) {
        // If there are no more buffers, we should try to get next send chance.
        if (pending_send_size_.fetch_add(iob->data_size()) > 0) {
            PUMP_DEBUG_CHECK(sendlist_.push(iob));
            return true;
        }

        if (!__send_once(flow_.get(), iob, true)) {
            if (__set_status(TRANSPORT_STARTED, TRANSPORT_STOPPING)) {
                __close_transport_flow();
                __post_channel_event(shared_from_this(), 0);
            }
            return false;
        }

        return true;
    }

    bool tcp_transport::__send_once(flow::flow_tcp_ptr flow, toolkit::io_buffer_ptr iob, bool resume) {
        // Get a buffer from sendlist to send.
        PUMP_ASSERT(!last_send_iob_);
        if (iob) {
            last_send_iob_ = iob;
        } else {
            while(!sendlist_.pop(last_send_iob_));
        }

        // Save last send buffer data size.
        last_send_iob_size_ = last_send_iob_->data_size();

#if defined(PUMP_HAVE_IOCP)
        if (flow->post_send(last_send_iob_) == flow::FLOW_ERR_ABORT) {
            PUMP_ERR_LOG("tcp_transport::__send_once: post send task failed");
            return false;
        }
#else
        // Try to send the buffer.
        auto ret = flow->want_to_send(last_send_iob_);
        if (PUMP_UNLIKELY(ret == flow::FLOW_ERR_ABORT)) {
            PUMP_ERR_LOG("tcp_transport::__send_once: want to send failed");
            return false;
        }

        if (!resume) {
            PUMP_DEBUG_CHECK(s_tracker_->set_tracked(true));
            return true;
        }

        if (PUMP_LIKELY(ret == flow::FLOW_ERR_NO)) {
            // Free last send buffer.
            last_send_iob_->sub_ref();
            last_send_iob_ = nullptr;
            // Reduce pending send size.
            if (pending_send_size_.fetch_sub(last_send_iob_size_) == last_send_iob_size_) {
                return true;
            }
        }

        if (!__start_send_tracker(shared_from_this())) {
            PUMP_ERR_LOG("tcp_transport::__send_once: start tracker failed");
            return false;
        }
#endif
        return true;
    }

    void tcp_transport::__try_doing_disconnected_process() {
        // Change transport state from TRANSPORT_STARTED to TRANSPORT_DISCONNECTING.
        __set_status(TRANSPORT_STARTED, TRANSPORT_DISCONNECTING);
        // Interrupt tranport
        __interrupt_and_trigger_callbacks();
    }

    void tcp_transport::__clear_sendlist() {
        if (last_send_iob_) {
            last_send_iob_->sub_ref();
        }

        toolkit::io_buffer_ptr iob;
        while (sendlist_.pop(iob)) {
            iob->sub_ref();
        }
    }

}  // namespace transport
}  // namespace pump
