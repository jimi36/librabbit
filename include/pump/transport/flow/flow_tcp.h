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

#ifndef pump_transport_flow_tcp_h
#define pump_transport_flow_tcp_h

#include "pump/transport/flow/flow.h"

namespace pump {
namespace transport {
    namespace flow {

        class flow_tcp
          : public flow_base {

          public:
            /*********************************************************************************
             * Constructor
             ********************************************************************************/
            flow_tcp() noexcept;

            /*********************************************************************************
             * Deconstructor
             ********************************************************************************/
            virtual ~flow_tcp();

            /*********************************************************************************
             * Init
             * Return results:
             *     FLOW_ERR_NO    => success
             *     FLOW_ERR_ABORT => error
             ********************************************************************************/
            flow_error init(poll::channel_sptr &&ch, int32_t fd);

#if defined(PUMP_HAVE_IOCP)
            /*********************************************************************************
             * Post read
             * If using IOCP this post an IOCP task for reading, else do nothing.
             * Return results:
             *     FLOW_ERR_NO    => success
             *     FLOW_ERR_ABORT => error
             ********************************************************************************/
            flow_error post_read(net::iocp_task_ptr iocp_task = nullptr);
#endif
            /*********************************************************************************
             * Read
             ********************************************************************************/
#if !defined(PUMP_HAVE_IOCP)
            PUMP_INLINE int32_t read(block_t *b, int32_t size) {
                return net::read(fd_, b, size);
            }
#endif

#if defined(PUMP_HAVE_IOCP)
            /*********************************************************************************
             * Post send
             * Return results:
             *     FLOW_ERR_NO    => success
             *     FLOW_ERR_ABORT => error
             ********************************************************************************/
            flow_error post_send(toolkit::io_buffer_ptr iob);
#else
             /*********************************************************************************
              * Want to send
              * If using IOCP this post an IOCP task for sending, else this try sending
              *data. Return results:
              *      FLOW_ERR_NO    => success
              *      FLOW_ERR_ABORT => error
              ********************************************************************************/
            flow_error want_to_send(toolkit::io_buffer_ptr iob);
#endif
            /*********************************************************************************
             * Send
             * Return results:
             *     FLOW_ERR_NO      => send completely
             *     FLOW_ERR_AGAIN   => try to send again
             *     FLOW_ERR_NO_DATA => no data to send
             *     FLOW_ERR_ABORT   => error
             ********************************************************************************/
#if defined(PUMP_HAVE_IOCP)
            flow_error send(net::iocp_task_ptr iocp_task);
#else
            flow_error send();
#endif
            /*********************************************************************************
             * Check there are data to send or not
             ********************************************************************************/
            PUMP_INLINE bool has_data_to_send() const {
                return (send_iob_ && send_iob_->data_size() > 0);
            }

          private:
            // Send buffer
            toolkit::io_buffer_ptr send_iob_;

#if defined(PUMP_HAVE_IOCP)
            // IOCP send task
            net::iocp_task_ptr send_task_;
#endif
        };
        DEFINE_ALL_POINTER_TYPE(flow_tcp);

    }  // namespace flow
}  // namespace transport
}  // namespace pump

#endif