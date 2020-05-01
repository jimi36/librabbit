/*
 * Copyright (C) 2015-2018 ZhengHaiTao <ming8ren@163.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable
 law or agreed to in writing, software
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
			void_ptr cert,
			const address &local_address,
			const address &remote_address,
			int64 dial_timeout,
			int64 handshake_timeout
		) : base_dialer(TLS_DIALER, local_address, remote_address, dial_timeout),
			cert_(cert),
			handshake_timeout_(handshake_timeout)
		{
		}

		bool tls_dialer::start(service_ptr sv,const dialer_callbacks &cbs) 
		{
			if (!__set_status(TRANSPORT_INIT, TRANSPORT_STARTING))
				return false;

			PUMP_ASSERT_EXPR(sv, __set_service(sv));
			PUMP_ASSERT_EXPR(cbs.dialed_cb && cbs.stopped_cb && cbs.timeout_cb, cbs_ = cbs);

			utils::scoped_defer defer([&]() {
				__close_flow();
				__stop_tracker();
				__set_status(TRANSPORT_STARTING, TRANSPORT_ERROR);
			});

			if (!__open_flow())
				return false;

			if (!__start_tracker((poll::channel_sptr)shared_from_this()))
				return false;

			if (flow_->want_to_connect(remote_address_) != FLOW_ERR_NO)
				return false;

			if (!__start_connect_timer(function::bind(&tls_dialer::on_timeout, shared_from_this())))
				return false;

			if (!__set_status(TRANSPORT_STARTING, TRANSPORT_STARTED))
				PUMP_ASSERT(false);

			defer.clear();

			return true;
		}

		void tls_dialer::stop()
		{
			// When in started status at the moment, stopping can be done. Then tracker event callback
			// will be triggered, we can trigger stopped callabck at there.
			if (__set_status(TRANSPORT_STARTED, TRANSPORT_STOPPING))
			{
				__close_flow();
				__stop_tracker();
				__stop_connect_timer();
				return;
			}

			// If in timeout doing status or handshaking status at the moment, it means that dialer is  
			// timeout but hasn't triggered event callback yet. So we just set stopping status to 
			// dialer, and when event callback triggered, we will trigger stopped callabck at there.
			if (__set_status(TRANSPORT_HANDSHAKING, TRANSPORT_STOPPING) ||
				__set_status(TRANSPORT_TIMEOUT_DOING, TRANSPORT_STOPPING))
				return;
		}

		void tls_dialer::on_send_event(net::iocp_task_ptr itask)
		{
			PUMP_LOCK_SPOINTER_EXPR(flow, flow_, false, 
				return);

			__stop_connect_timer();

			address local_address, remote_address;
			if (flow->connect(itask, local_address, remote_address) != 0)
			{
				if (__set_status(TRANSPORT_STARTED, TRANSPORT_ERROR))
				{
					__close_flow();
					__stop_tracker();
				}
				return;
			}

			if (!__set_status(TRANSPORT_STARTED, TRANSPORT_HANDSHAKING))
				return;

			__close_flow();

			tls_handshaker::tls_handshaker_callbacks tls_cbs;
			tls_cbs.handshaked_cb = function::bind(&tls_dialer::on_handshaked_callback,
				shared_from_this(), _1, _2);
			tls_cbs.stopped_cb = function::bind(&tls_dialer::on_stopped_handshaking_callback,
				shared_from_this(), _1);

			// If handshaker is started error, handshaked callback will be triggered. So we do nothing
			// at here when started error. But if dialer stopped befere here, we shuold stop handshaking.
			handshaker_.reset(new tls_handshaker);
			if (!handshaker_->init(flow->unbind_fd(), true, cert_, local_address, remote_address))
				PUMP_ASSERT(false);

			poll::channel_tracker_sptr tracker(std::move(tracker_));
			if (handshaker_->start(get_service(), tracker, handshake_timeout_, tls_cbs))
			{
				if (__is_status(TRANSPORT_STOPPING))
					handshaker_->stop();
			}
		}

		void tls_dialer::on_timeout(tls_dialer_wptr wptr)
		{
			PUMP_LOCK_WPOINTER_EXPR(dialer, wptr, false,
				return);

			if (dialer->__set_status(TRANSPORT_STARTED, TRANSPORT_TIMEOUT_DOING))
			{
				dialer->__close_flow();
				dialer->__stop_tracker();
			}
		}

		void tls_dialer::on_handshaked_callback(
			tls_dialer_wptr wptr, 
			tls_handshaker_ptr handshaker,
			bool succ
		) {
			PUMP_LOCK_WPOINTER_EXPR(dialer, wptr, false,
				handshaker->stop(); return);

			if (dialer->__set_status(TRANSPORT_STOPPING, TRANSPORT_STOPPED))
			{
				dialer->cbs_.stopped_cb();
			}
			else if (dialer->__set_status(TRANSPORT_HANDSHAKING, TRANSPORT_FINISH))
			{
				tls_transport_sptr transp;
				if (succ)
				{
					auto flow = handshaker->unlock_flow();
					transp = tls_transport::create_instance();
					if (!transp->init(flow, handshaker->get_local_address(), handshaker->get_remote_address()))
						PUMP_ASSERT(false);
				}

				dialer->cbs_.dialed_cb(transp, succ);
			}

			dialer->handshaker_.reset();
		}

		void tls_dialer::on_stopped_handshaking_callback(
			tls_dialer_wptr wptr, 
			tls_handshaker_ptr handshaker
		) {
			PUMP_LOCK_WPOINTER_EXPR(dialer, wptr, false,
				return);

			if (dialer->__set_status(TRANSPORT_STOPPING, TRANSPORT_STOPPED))
				dialer->cbs_.stopped_cb();
		}

		bool tls_dialer::__open_flow()
		{
			// Setup flow
			PUMP_ASSERT(!flow_);
			flow_.reset(new flow::flow_tcp_dialer());
			poll::channel_sptr ch = shared_from_this();
			if (flow_->init(ch, local_address_) != FLOW_ERR_NO)
				return false;

			// Set channel FD
			poll::channel::__set_fd(flow_->get_fd());

			return true;
		}

		tls_sync_dialer::tls_sync_dialer()
		{
		}

		base_transport_sptr tls_sync_dialer::dial(
			void_ptr cert,
			service_ptr sv,
			const address &local_address,
			const address &remote_address,
			int64 connect_timeout,
			int64 handshake_timeout
		) {
			base_transport_sptr transp;

			if (dialer_)
				return transp;

			dialer_callbacks cbs;
			cbs.dialed_cb = function::bind(&tls_sync_dialer::on_dialed_callback,
				shared_from_this(), _1, _2);
			cbs.timeout_cb = function::bind(&tls_sync_dialer::on_timeout_callback,
				shared_from_this());
			cbs.stopped_cb = function::bind(&tls_sync_dialer::on_stopped_callback);

			dialer_ = tls_dialer::create_instance(cert, local_address, remote_address, connect_timeout, handshake_timeout);
			if (!dialer_->start(sv, cbs))
			{
				dialer_.reset();
				return transp;
			}

			auto future = dial_promise_.get_future();
			transp = future.get();

			return transp;
		}

		void tls_sync_dialer::on_dialed_callback(
			tls_sync_dialer_wptr wptr,
			base_transport_sptr transp,
			bool succ
		) {
			PUMP_LOCK_WPOINTER_EXPR(dialer, wptr, true,
				dialer->dial_promise_.set_value(transp));
		}

		void tls_sync_dialer::on_timeout_callback(tls_sync_dialer_wptr wptr) 
		{
			PUMP_LOCK_WPOINTER_EXPR(dialer, wptr, true,
				dialer->dial_promise_.set_value(base_transport_sptr()));
		}

		void tls_sync_dialer::on_stopped_callback()
		{
			PUMP_ASSERT(false);
		}

		void tls_sync_dialer::__reset()
		{
			dialer_.reset();
		}

	}
}
