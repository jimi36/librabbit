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

#include "pump/transport/tcp_acceptor.h"

namespace pump {
	namespace transport {

		tcp_acceptor::tcp_acceptor() :
			transport_base(TCP_ACCEPTOR, nullptr, -1)
		{
		}

		tcp_acceptor::~tcp_acceptor()
		{
		}

		bool tcp_acceptor::start(
			service_ptr sv,
			const address &listen_address,
			accepted_notifier_sptr &notifier
		) {
			if (!__set_status(TRANSPORT_INIT, TRANSPORT_STARTING))
				return false;

			assert(sv);
			__set_service(sv);

			assert(notifier);
			__set_notifier(notifier);

			{
				utils::scoped_defer defer([&]() {
					__close_flow();
					__stop_tracker();
					__set_status(TRANSPORT_STARTING, TRANSPORT_ERROR);
				});

				if (!__open_flow(listen_address))
					return false;

				if (!__start_tracker())
					return false;

				if (flow_->want_to_accept() != FLOW_ERR_NO)
					return false;

				if (!__set_status(TRANSPORT_STARTING, TRANSPORT_STARTED))
					assert(false);

				defer.clear();
			}

			return true;
		}

		void tcp_acceptor::stop()
		{
			if (__set_status(TRANSPORT_STARTED, TRANSPORT_STOPPING))
			{
				__close_flow();
				__stop_tracker();
			}
		}

		void tcp_acceptor::on_read_event(net::iocp_task_ptr itask)
		{
			auto flow_locker = flow_;
			auto flow = flow_locker.get();
			if (flow == nullptr)
			{
				flow::free_iocp_task(itask);
				return;
			}

			address local_address, remote_address;
			int32 fd = flow->accept(itask, &local_address, &remote_address);
			if (fd > 0)
			{
				auto notifier_locker = __get_notifier<accepted_notifier>();
				auto notifier = notifier_locker.get();
				assert(notifier);
				
				auto conn = tcp_transport::create_instance();
				conn->init(fd, local_address, remote_address);

				notifier->on_accepted_callback(get_context(), conn);
			}

			if (flow->want_to_accept() != FLOW_ERR_NO && is_started())
				assert(false);
		}

		void tcp_acceptor::on_tracker_event(bool on)
		{
			if (on)
				return;

			tracker_cnt_.fetch_sub(1);

			if (tracker_cnt_ == 0)
			{
				auto notifier_locker = __get_notifier<accepted_notifier>();
				auto notifier = notifier_locker.get();
				assert(notifier);

				if (__set_status(TRANSPORT_STOPPING, TRANSPORT_STOPPED))
					notifier->on_stopped_accepting_callback(get_context());
			}
		}

		bool tcp_acceptor::__open_flow(const address &listen_address)
		{
			if (flow_)
				return false;

			// Create and init flow.
			poll::channel_sptr ch = shared_from_this();
			flow_.reset(new flow::flow_tcp_acceptor());
			if (flow_->init(ch, get_service()->get_iocp_handler(), listen_address) != FLOW_ERR_NO)
				return false;

			// Set channel fd.
			poll::channel::__set_fd(flow_->get_fd());

			// Save listen address.
			listen_address_ = listen_address;

			return true;
		}

		void tcp_acceptor::__close_flow()
		{
			flow_.reset();
		}

		bool tcp_acceptor::__start_tracker()
		{
			if (tracker_)
				return false;

			poll::channel_sptr ch = shared_from_this();
			tracker_.reset(new poll::channel_tracker(ch, TRACK_READ, TRACK_MODE_KEPPING));
			tracker_->track(true);

			if (!get_service()->add_channel_tracker(tracker_))
				return false;

			tracker_cnt_.fetch_add(1);

			return true;
		}

		void tcp_acceptor::__stop_tracker()
		{
			if (!tracker_)
				return;

			poll::channel_tracker_sptr tmp;
			tmp.swap(tracker_);

			get_service()->remove_channel_tracker(tmp);
		}

	}
}