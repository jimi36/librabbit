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

#include "pump/transport/flow/flow_tcp_dialer.h"

namespace pump {
	namespace transport {
		namespace flow {

			flow_tcp_dialer::flow_tcp_dialer() PUMP_NOEXCEPT : 
				is_ipv6_(false),
				dial_task_(nullptr)
			{
			}

			flow_tcp_dialer::~flow_tcp_dialer()
			{
#if defined(WIN32) && defined(USE_IOCP)
				if (dial_task_)
					net::unlink_iocp_task(dial_task_);
#endif
			}

			int32 flow_tcp_dialer::init(poll::channel_sptr &ch, PUMP_CONST address &bind_address)
			{
				PUMP_ASSERT_EXPR(ch, ch_ = ch);
	
				is_ipv6_ = bind_address.is_ipv6();
				int32 domain = is_ipv6_ ? AF_INET6 : AF_INET;

#if defined(WIN32) && defined(USE_IOCP)
				fd_ = net::create_iocp_socket(domain, SOCK_STREAM, net::get_iocp_handler());
				if (fd_ == -1)
					return FLOW_ERR_ABORT;

				ext_ = net::new_net_extension(fd_);
				if (!ext_)
					return FLOW_ERR_ABORT;

				dial_task_ = net::new_iocp_task();
				net::set_iocp_task_fd(dial_task_, fd_);
				net::set_iocp_task_notifier(dial_task_, ch_);
				net::set_iocp_task_type(dial_task_, IOCP_TASK_CONNECT);
#else
				if ((fd_ = net::create_socket(domain, SOCK_STREAM)) == -1)
					return FLOW_ERR_ABORT;
#endif
				if (!net::set_reuse(fd_, 1) ||
					!net::set_noblock(fd_, 1) ||
					!net::set_nodelay(fd_, 1) ||
					//!net::set_keeplive(fd_, 3, 3) ||
					!net::bind(fd_, (sockaddr*)bind_address.get(), bind_address.len()))
					return FLOW_ERR_ABORT;

				return FLOW_ERR_NO;
			}

			int32 flow_tcp_dialer::want_to_connect(PUMP_CONST address &remote_address)
			{
#if defined(WIN32) && defined(USE_IOCP)
				if (!net::post_iocp_connect(ext_, dial_task_, remote_address.get(), remote_address.len()))
					return FLOW_ERR_ABORT;
#else
				if (!net::connect(fd_, (sockaddr*)remote_address.get(), remote_address.len()))
					return FLOW_ERR_ABORT;
#endif
				return FLOW_ERR_NO;
			}

			int32 flow_tcp_dialer::connect(
				net::iocp_task_ptr itask, 
				address_ptr local_address, 
				address_ptr remote_address
			) {
#if defined(WIN32) && defined(USE_IOCP)
				PUMP_ASSERT(itask);
				int32 ec = net::get_iocp_task_ec(itask);
#else
				int32 ec = net::get_socket_error(fd_);
#endif
				if (ec != 0)
					return ec;

				if (!net::update_connect_context(fd_))
				{
					ec = net::get_socket_error(fd_);
					return ec;
				}

				int32 addrlen = 0;
				block addr[ADDRESS_MAX_LEN];

				addrlen = ADDRESS_MAX_LEN;
				net::local_address(fd_, (sockaddr*)addr, &addrlen);
				local_address->set((sockaddr*)addr, addrlen);

				addrlen = ADDRESS_MAX_LEN;
				net::remote_address(fd_, (sockaddr*)addr, &addrlen);
				remote_address->set((sockaddr*)addr, addrlen);

				return ec;
			}

		}
	}
}