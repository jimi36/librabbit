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

#ifndef pump_transport_udp_transport_h
#define pump_transport_udp_transport_h

#include "pump/transport/flow/flow_udp.h"
#include "pump/transport/base_transport.h"

namespace pump {
namespace transport {

    class udp_transport;
    DEFINE_ALL_POINTER_TYPE(udp_transport);

    class LIB_PUMP udp_transport
      : public base_transport {

      public:
        /*********************************************************************************
         * Create instance
         ********************************************************************************/
        PUMP_INLINE static udp_transport_sptr create(const address &bind_address) {
            INLINE_OBJECT_CREATE(obj, udp_transport, (bind_address));
            return udp_transport_sptr(obj, object_delete<udp_transport>);
        }

        /*********************************************************************************
         * Deconstructor
         ********************************************************************************/
        virtual ~udp_transport();

        /*********************************************************************************
         * Start
         * max_pending_send_size is ignore on udp transport.
         ********************************************************************************/
        virtual error_code start(
            service *sv, 
            read_mode mode,
            const transport_callbacks &cbs) override;

        /*********************************************************************************
         * Stop
         ********************************************************************************/
        virtual void stop() override;

        /*********************************************************************************
         * Force stop
         ********************************************************************************/
        virtual void force_stop() override {
            stop();
        }

        /*********************************************************************************
         * Continue read for read once mode
         ********************************************************************************/
        virtual error_code continue_read() override;

        /*********************************************************************************
         * Send
         ********************************************************************************/
        virtual error_code send(
            const block_t *b,
            int32_t size,
            const address &address) override;

      protected:
        /*********************************************************************************
         * Read event callback
         ********************************************************************************/
        virtual void on_read_event() override;

      private:
        /*********************************************************************************
         * Constructor
         ********************************************************************************/
        udp_transport(const address &bind_address) noexcept;

        /*********************************************************************************
         * Open transport flow
         ********************************************************************************/
        bool __open_transport_flow();

        /*********************************************************************************
         * Shutdown transport flow
         ********************************************************************************/
        virtual void __shutdown_transport_flow(int32_t how) override;

        /*********************************************************************************
         * Close transport flow
         ********************************************************************************/
        virtual void __close_transport_flow() override;

      private:
        // Udp flow
        flow::flow_udp_sptr flow_;
    };

}  // namespace transport
}  // namespace pump

#endif
