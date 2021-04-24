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

#ifndef pump_protocol_websocket_connection_h
#define pump_protocol_websocket_connection_h

#include "pump/service.h"
#include "pump/toolkit/buffer.h"
#include "pump/transport/base_transport.h"

#include "pump/protocol/http/request.h"
#include "pump/protocol/http/response.h"
#include "pump/protocol/websocket/frame.h"

namespace pump {
namespace protocol {
namespace websocket {

    class connection;
    DEFINE_ALL_POINTER_TYPE(connection);

    struct connection_callbacks {
        // Frame callback
        pump_function<void(const block_t*, int32_t, bool)> frame_cb;
        // Error callback
        pump_function<void(const std::string &)> error_cb;
    };

    struct upgrade_callbacks {
        // Upgrade pocket callback
        pump_function<void(http::pocket_sptr)> pocket_cb;
        // Upgrade error callback
        pump_function<void(const std::string &)> error_cb;
    };

    class LIB_PUMP connection 
      : public std::enable_shared_from_this<connection> {

      protected:
        friend class client;
        friend class server;

      public:
        /*********************************************************************************
         * Constructor
         ********************************************************************************/
        connection(
            service *sv,
            transport::base_transport_sptr &transp,
            bool has_mask) noexcept;

        /*********************************************************************************
         * Deconstructor
         ********************************************************************************/
        virtual ~connection() = default;

        /*********************************************************************************
         * Start upgrade
         * User must not call this function!!!
         ********************************************************************************/
        bool start_upgrade(
            bool client,
            const upgrade_callbacks &ucbs);

        /*********************************************************************************
         * Start
         ********************************************************************************/
        bool start(const connection_callbacks &cbs);

        /*********************************************************************************
         * Stop
         ********************************************************************************/
        void stop();

        /*********************************************************************************
         * Send raw data
         * Send raw data without packing as frame.
         ********************************************************************************/
        bool send(
            const block_t *b,
            int32_t size);

        /*********************************************************************************
         * Send frame
         * Send data with packing as frame.
         ********************************************************************************/
        bool send_frame(
            const block_t *b,
            int32_t size);

        /*********************************************************************************
         * Check connection is valid or not
         ********************************************************************************/
        PUMP_INLINE bool is_valid() const {
            if (transp_ && transp_->is_started()) {
                return true;
            }
            return false;
        }

      protected:
        /*********************************************************************************
         * Read event callback
         ********************************************************************************/
        static void on_read(
            connection_wptr wptr,
            const block_t *b, 
            int32_t size);

        /*********************************************************************************
         * Disconnected event callback
         ********************************************************************************/
        static void on_disconnected(connection_wptr wptr);

        /*********************************************************************************
         * Stopped event callback
         ********************************************************************************/
        static void on_stopped(connection_wptr wptr);

      private:
        /*********************************************************************************
         * Handle frame
         ********************************************************************************/
        int32_t __handle_pocket(
            const block_t *b, 
            int32_t size);

        /*********************************************************************************
         * Handle frame
         ********************************************************************************/
        int32_t __handle_frame(
            const block_t *b, 
            int32_t size);

        /*********************************************************************************
         * Send ping frame
         ********************************************************************************/
        void __send_ping_frame();

        /*********************************************************************************
         * Send pong
         ********************************************************************************/
        void __send_pong_frame();

        /*********************************************************************************
         * Send close frame
         ********************************************************************************/
        void __send_close_frame();

      private:
        // Service
        service *sv_;

        // Transport
        transport::base_transport_sptr transp_;

        // Read category
        int32_t read_category_;

        // Read Cache
        toolkit::io_buffer *cahce_;

        // Pocket
        http::pocket_sptr pocket_;

        // Frame mask
        bool has_mask_;
        uint8_t mask_key_[4];
        // Frame closed
        std::atomic_flag closed_;

        // Frame decode info
        int32_t decode_phase_;
        frame_header decode_hdr_;

        // Upgrade callback
        upgrade_callbacks ucbs_;
        // Connection callbacks
        connection_callbacks cbs_;
    };
    DEFINE_ALL_POINTER_TYPE(connection);

}  // namespace websocket
}  // namespace protocol
}  // namespace pump

#endif