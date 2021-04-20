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

#include "pump/transport/base_acceptor.h"

namespace pump {
namespace transport {

    base_acceptor::~base_acceptor() {
        __stop_accept_tracker();
        __close_accept_flow();
    }

    void base_acceptor::on_channel_event(int32_t ev) {
        __trigger_interrupt_callbacks();
    }

    bool base_acceptor::__start_accept_tracker(poll::channel_sptr &&ch) {
        if (tracker_) {
            return false;
        }

        tracker_.reset(
            object_create<poll::channel_tracker>(ch, poll::TRACK_READ),
            object_delete<poll::channel_tracker>);
        PUMP_DEBUG_COND_FAIL(
            !tracker_,
            return false
        );
        if (!get_service()->add_channel_tracker(tracker_, READ_POLLER_ID)) {
            PUMP_WARN_LOG("base_acceptor: start tracker failed");
            return false;
        }

        PUMP_DEBUG_LOG("base_acceptor: start tracker");
        return true;
    }

    bool base_acceptor::__resume_accept_tracker() {
        auto tracker = tracker_.get();
        PUMP_DEBUG_COND_FAIL(
            tracker == nullptr,
            return false);
        auto poller = tracker_->get_poller();
        PUMP_DEBUG_COND_FAIL(
            poller == nullptr,
            return false);
        return poller->resume_channel_tracker(tracker);
    }

    void base_acceptor::__stop_accept_tracker() {
        auto tracker = tracker_;
        if (!tracker) {
            return;
        }
        get_service()->remove_channel_tracker(tracker, READ_POLLER_ID);
        PUMP_DEBUG_LOG("base_acceptor: stop tracker");
    }

    void base_acceptor::__trigger_interrupt_callbacks() {
        if (__set_state(TRANSPORT_STOPPING, TRANSPORT_STOPPED)) {
            cbs_.stopped_cb();
        }
    }

}  // namespace transport
}  // namespace pump
