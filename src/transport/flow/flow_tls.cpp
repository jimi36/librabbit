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

#include "pump/transport/flow/flow_tls.h"

#ifdef USE_GNUTLS
extern "C" {
#include <gnutls/gnutls.h>
}
#endif

namespace pump {
	namespace transport {
		namespace flow {

			struct tls_session
			{
#ifdef USE_GNUTLS
				tls_session() 
				{
					session = NULL;
				}
				gnutls_session_t session;
#endif
			};

			class ssl_net_layer
			{
#ifdef USE_GNUTLS
			public:
				static ssize_t data_pull(gnutls_transport_ptr_t ptr, void_ptr data, size_t maxlen)
				{
					flow_tls_ptr flow = (flow_tls_ptr)ptr;
					if (flow == nullptr)
						return 0;

					int32 size = flow->__read_from_net_recv_cache((block_ptr)data, (uint32)maxlen);
					if (size == 0)
						return -1;
					
					return size;
				}

				static ssize_t data_push(gnutls_transport_ptr_t ptr, c_void_ptr data, size_t len)
				{
					flow_tls_ptr flow = (flow_tls_ptr)ptr;
					if (flow == nullptr)
						return 0;

					flow->__write_to_net_send_cache((c_block_ptr)data, (uint32)len);

					return len;
				}

				static int get_error(gnutls_transport_ptr_t ptr)
				{
					return EAGAIN;
				}
#endif
			};

			flow_tls::flow_tls():
				is_handshaked_(false),
				session_(nullptr),
				recv_task_(nullptr)
			{
			}

			flow_tls::~flow_tls()
			{
#ifdef USE_GNUTLS
				if (session_)
				{
					if (session_->session)
						gnutls_deinit(session_->session);
					delete session_;
				}

				if (recv_task_)
					net::unlink_iocp_task(recv_task_);
				if (send_task_)
					net::unlink_iocp_task(send_task_);
#endif
			}

			int32 flow_tls::init(poll::channel_sptr &ch, int32 fd, void_ptr tls_cert, bool is_client)
			{
#ifdef USE_GNUTLS
				if (fd <= 0 && !ch)
					return FLOW_ERR_ABORT;

				ch_ = ch;
				fd_ = fd;

				session_ = new tls_session();
				if (is_client)
					gnutls_init(&session_->session, GNUTLS_CLIENT | GNUTLS_NONBLOCK);
				else 
					gnutls_init(&session_->session, GNUTLS_SERVER | GNUTLS_NONBLOCK);
				gnutls_set_default_priority(session_->session);
				// Set transport ptr
				gnutls_transport_set_ptr(session_->session, this);
				// Set GnuTLS session with credentials
				gnutls_credentials_set(session_->session, GNUTLS_CRD_CERTIFICATE, tls_cert);
				// Set GnuTLS handshake timeout time
				gnutls_handshake_set_timeout(session_->session, GNUTLS_INDEFINITE_TIMEOUT);
				//gnutls_handshake_set_timeout(session_->session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
				// Set the callback that allows GnuTLS to PUSH data TO the transport layer
				gnutls_transport_set_push_function(session_->session, ssl_net_layer::data_push);
				// Set the callback that allows GnuTls to PULL data FROM the tranport layer
				gnutls_transport_set_pull_function(session_->session, ssl_net_layer::data_pull);
				// Set the callback that allows GnuTls to Get error if PULL or PUSH function error
				gnutls_transport_set_errno_function(session_->session, ssl_net_layer::get_error);
				
				ssl_read_buffer_.resize(MAX_FLOW_BUFFER_SIZE);
				net_recv_cache_.resize(MAX_FLOW_BUFFER_SIZE);
				recv_task_ = net::new_iocp_task();
				net::set_iocp_task_fd(recv_task_, fd_);
				net::set_iocp_task_notifier(recv_task_, ch_);
				net::set_iocp_task_type(recv_task_, IOCP_TASK_RECV);
				net::set_iocp_task_buffer(recv_task_, (block_ptr)net_recv_cache_.data(), (uint32)net_recv_cache_.size());

				send_task_ = net::new_iocp_task();
				net::set_iocp_task_fd(send_task_, fd_);
				net::set_iocp_task_notifier(send_task_, ch_);
				net::set_iocp_task_type(send_task_, IOCP_TASK_SEND);

				return FLOW_ERR_NO;
#else
				return FLOW_ERR_ABORT;
#endif
			}

			void flow_tls::rebind_channel(poll::channel_sptr &ch)
			{
#ifdef USE_GNUTLS
				ch_ = ch;
				if (recv_task_)
					net::set_iocp_task_notifier(recv_task_, ch_);
				if (send_task_)
					net::set_iocp_task_notifier(send_task_, ch_);
#endif
			}

			int32 flow_tls::handshake()
			{
#ifdef USE_GNUTLS
				// Now we perform the actual SSL/TLS handshake.
				// If you wanted to, you could send some data over the tcp socket before
				// giving it to GnuTLS and performing the handshake. See the GnuTLS manual
				// on STARTTLS for more information.
				int32 ret = gnutls_handshake(session_->session);

				// GnuTLS manual says to keep trying until it returns zero (success) or
				// encounters a fatal error.
				if (ret != 0 && gnutls_error_is_fatal(ret) != 0)
					return FLOW_ERR_ABORT;

				// Flow handshakes success if ret is requal zero. 
				if (ret == 0)
					is_handshaked_ = true;

				return FLOW_ERR_NO;
#else
				return FLOW_ERR_ABORT;
#endif
			}

			int32 flow_tls::want_to_recv()
			{
#if defined(WIN32) && defined (USE_IOCP)
				net::link_iocp_task(recv_task_);
				net::reuse_iocp_task(recv_task_);
				if (!net::post_iocp_read(recv_task_))
				{
					net::unlink_iocp_task(recv_task_);
					return FLOW_ERR_ABORT;
				}
#endif
				return FLOW_ERR_NO;
			}

			int32 flow_tls::recv_from_net(net::iocp_task_ptr itask)
			{
#if defined(WIN32) && defined(USE_IOCP)
				assert(recv_task_ == itask);
				int32 size = net::get_iocp_task_processed_size(itask);
				block_ptr buf = net::get_iocp_task_processed_buffer(itask);
				net::unlink_iocp_task(itask);
				if (size <= 0)
					return FLOW_ERR_ABORT;
#else
				int32 size = net::recv(fd_, (int8*)net_recv_cache_.data(), (uint32)net_recv_cache_.size());
				block_ptr buf = (block_ptr)net_recv_cache_.data();
				if (size <= 0)
				{
					switch (net::last_errno())
					{
					case LANE_EINPROGRESS:
					case LANE_EWOULDBLOCK:
						return FLOW_ERR_AGAIN;
					default:
						return FLOW_ERR_ABORT;
					}
				}
#endif
				net_recv_buffer_.append(buf, size);

				return FLOW_ERR_AGAIN;
			}

			c_block_ptr flow_tls::read_from_ssl(int32 &size)
			{
#ifdef USE_GNUTLS
				size = (int32)gnutls_read(session_->session, (int8_ptr)ssl_read_buffer_.data(), ssl_read_buffer_.size());
				return ssl_read_buffer_.data();
#else
				size = -1;
				return nullptr;
#endif
			}

			uint32 flow_tls::__read_from_net_recv_cache(block_ptr buffer, uint32 maxlen)
			{
				// Get suitable size to read
				uint32 size = (uint32)net_recv_buffer_.size() > maxlen ? maxlen : (uint32)net_recv_buffer_.size();
				if (size > 0)
				{
					// Copy read cache to buffer.
					memcpy(buffer, net_recv_buffer_.data(), size);
					// Trim read cache. 
					net_recv_buffer_ = net_recv_buffer_.substr(size);
				}
				return size;
			}

			int32 flow_tls::write_to_ssl(buffer_ptr wb)
			{
#ifdef USE_GNUTLS
				int32 size = (int32)gnutls_write(session_->session, wb->data(), wb->data_size());
				if (size > 0)
					wb->shift(size);
				return size;
#else
				return -1;
#endif
			}

			int32 flow_tls::want_to_send()
			{
				if (!ssl_send_cache_.empty())
				{
					net_send_buffer_.append(ssl_send_cache_.data(), (uint32)ssl_send_cache_.size());
					ssl_send_cache_.clear();
				}
				if (net_send_buffer_.data_size() == 0)
					return FLOW_ERR_AGAIN;
				
#if defined(WIN32) && defined(USE_IOCP)
				net::link_iocp_task(send_task_);
				net::reuse_iocp_task(send_task_);
				net::set_iocp_task_buffer(send_task_, (int8_ptr)net_send_buffer_.data(), net_send_buffer_.data_size());
				if (!net::post_iocp_write(send_task_))
				{
					net::unlink_iocp_task(send_task_);
					return FLOW_ERR_ABORT;
				}
				return FLOW_ERR_AGAIN;
#else
				int32 size = net::send(fd_, net_send_buffer_.data(), net_send_buffer_.data_size());
				if (size <= 0)
				{
					switch (net::last_errno())
					{
					case LANE_EINPROGRESS:
					case LANE_EWOULDBLOCK:
						return FLOW_ERR_AGAIN;
					default:
						return FLOW_ERR_ABORT;
					}
				}

				if (!net_send_buffer_.shift(size))
					assert(false);

				return FLOW_ERR_AGAIN;
#endif
			}

			int32 flow_tls::send_to_net(net::iocp_task_ptr itask)
			{
				if (!ssl_send_cache_.empty())
				{
					net_send_buffer_.append(ssl_send_cache_.data(), (uint32)ssl_send_cache_.size());
					ssl_send_cache_.clear();
				}
				if (net_send_buffer_.data_size() == 0)
					return FLOW_ERR_NO_DATA;

#if defined(WIN32) && defined(USE_IOCP)
				assert(send_task_ == itask);
				int32 size = net::get_iocp_task_processed_size(itask);
				net::unlink_iocp_task(itask);
				if (size <= 0)
					return FLOW_ERR_ABORT;
#else
				int32 size = net::send(fd_, net_send_buffer_.data(), net_send_buffer_.data_size());
				if (size < 0)
				{
					switch (net::last_errno())
					{
					case LANE_EINPROGRESS:
					case LANE_EWOULDBLOCK:
						return FLOW_ERR_AGAIN;
					default:
						return FLOW_ERR_ABORT;
					}
				}
#endif
				if (!net_send_buffer_.shift(size))
					assert(false);

				if (net_send_buffer_.data_size() == 0)
					return FLOW_ERR_NO_DATA;

				return FLOW_ERR_AGAIN;
			}

			void flow_tls::__write_to_net_send_cache(c_block_ptr data, uint32 size)
			{
				ssl_send_cache_.append(data, size);
			}

		}
	}
}