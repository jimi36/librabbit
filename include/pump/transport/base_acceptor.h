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

#ifndef pump_transport_acceptor_h
#define pump_transport_acceptor_h

#include <pump/transport/base_transport.h>

namespace pump {
namespace transport {

class pump_lib base_acceptor : public base_channel {
  public:
    /*********************************************************************************
     * Constructor
     ********************************************************************************/
    base_acceptor(
        int32_t type,
        const address &listen_address) noexcept
      : base_channel(type, nullptr, -1),
        listen_address_(listen_address) {
    }

    /*********************************************************************************
     * Deconstructor
     ********************************************************************************/
    virtual ~base_acceptor();

    /*********************************************************************************
     * Start
     ********************************************************************************/
    virtual error_code start(
        service *sv,
        const acceptor_callbacks &cbs) = 0;

    /*********************************************************************************
     * Stop
     ********************************************************************************/
    virtual void stop() = 0;

    /*********************************************************************************
     * Get local address
     ********************************************************************************/
    pump_inline const address &get_listen_address() const noexcept {
        return listen_address_;
    }

  protected:
    /*********************************************************************************
     * Channel event callback
     ********************************************************************************/
    virtual void on_channel_event(int32_t ev, void *arg) override;

  protected:
    /*********************************************************************************
     * Open accept flow
     ********************************************************************************/
    virtual bool __open_accept_flow() = 0;

    /*********************************************************************************
     * Close accept flow
     ********************************************************************************/
    virtual void __close_accept_flow() {}

  protected:
    /*********************************************************************************
     * Install accept tracker
     ********************************************************************************/
    bool __install_accept_tracker(poll::channel_sptr &&ch);

    /*********************************************************************************
     * Start accept tracker
     ********************************************************************************/
    bool __start_accept_tracker();

    /*********************************************************************************
     * Uninstall accept tracker
     ********************************************************************************/
    void __uninstall_accept_tracker();

    /*********************************************************************************
     * Trigger interrupt callbacks
     ********************************************************************************/
    void __trigger_interrupt_callbacks();

  protected:
    // Listen address
    address listen_address_;

    // Channel tracker
    poll::channel_tracker_sptr tracker_;

    // Acceptor callbacks
    acceptor_callbacks cbs_;
};
DEFINE_SMART_POINTERS(base_acceptor);

}  // namespace transport
}  // namespace pump

#endif
