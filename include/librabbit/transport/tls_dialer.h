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

#ifndef librabbit_transport_tls_dialer_h
#define librabbit_transport_tls_dialer_h

#include "librabbit/transport/tls_handshaker.h"
#include "librabbit/transport/transport_notifier.h"
#include "librabbit/transport/flow/flow_tls_dialer.h"

namespace librabbit {
	namespace transport {

		class tls_dialer;
		DEFINE_ALL_POINTER_TYPE(tls_dialer);

		class LIB_EXPORT tls_dialer :
			public transport_base,
			public time::timeout_notifier,
			public tls_handshaked_notifier,
			public std::enable_shared_from_this<tls_dialer>
		{
		public:
			/*********************************************************************************
			 * Create instance
			 ********************************************************************************/
			static tls_dialer_sptr create_instance()
			{
				tls_dialer_sptr ins(new tls_dialer);
				return ins;
			}

			/*********************************************************************************
			 * Deconstructor
			 ********************************************************************************/
			virtual ~tls_dialer();

			/*********************************************************************************
			 * Start
			 ********************************************************************************/
			bool start(
				void_ptr tls_cert,
				service_ptr sv,
				int64 connect_timeout,
				int64 handshake_timeout,
				const address &bind_address,
				const address &connect_address,
				dialed_notifier_sptr &notifier
			);

			/*********************************************************************************
			 * Stop
			 ********************************************************************************/
			virtual void stop();

		protected:
			/*********************************************************************************
			 * Write event callback
			 ********************************************************************************/
			virtual void on_write_event(net::iocp_task_ptr itask);

			/*********************************************************************************
			 * Tracker event callback
			 ********************************************************************************/
			virtual void on_tracker_event(bool on);

			/*********************************************************************************
			 * Timeout event callback
			 ********************************************************************************/
			virtual void on_timer_timeout(void_ptr arg);

			/*********************************************************************************
			 * Tls handshake success callback
			 ********************************************************************************/
			virtual void on_handshaked_callback(transport_base_ptr handshaker, bool succ);

			/*********************************************************************************
			 * Tls handshake timeout callback
			 ********************************************************************************/
			virtual void on_handshaked_timeout(transport_base_ptr handshaker);

		private:
			/*********************************************************************************
			 * Constructor
			 ********************************************************************************/
			tls_dialer();

			/*********************************************************************************
			 * Set tls credentials
			 ********************************************************************************/
			void __set_tls_cert(void_ptr tls_cert);

			/*********************************************************************************
			 * Open flow
			 ********************************************************************************/
			bool __open_flow(const address &bind_address);

			/*********************************************************************************
			 * Close flow
			 ********************************************************************************/
			void __close_flow();

			/*********************************************************************************
			 * Start tracker
			 ********************************************************************************/
			bool __start_tracker();

			/*********************************************************************************
			 * Stop tracker
			 ********************************************************************************/
			void __stop_tracker();

			/*********************************************************************************
			 * Start timeout timer
			 ********************************************************************************/
			void __start_timer(int64 timeout);

			/*********************************************************************************
			 * Stop timeout timer
			 ********************************************************************************/
			void __stop_timer();

		private:
			// Bind address
			address bind_address_;

			// Remote address
			address connect_address_;

			// Gnutls credentials
			void_ptr tls_cert_;

			// Connect timeout timer
			time::timer_sptr timer_;

			// Channel tracker
			poll::channel_tracker_sptr tracker_;

			// Tls dialer flow layer
			flow::flow_tls_dialer_sptr flow_;

			// Tls handshaker
			int64 handshake_timeout_;
			tls_handshaker_sptr handshaker_;
		};

	}
}

#endif