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

#include "pump/time/timer.h"
#include "pump/time/manager.h"

namespace pump {
namespace time {

timer::timer(uint64_t timeout, const timer_callback &cb, bool repeated) noexcept :
    queue_(nullptr),
    status_(TIMER_INIT),
    cb_(cb),
    repeated_(repeated),
    timeout_(timeout),
    overtime_(0) {}

void timer::handle_timeout() {
    if (__set_state(TIMER_STARTED, TIMER_PENDING)) {
        cb_();

        if (PUMP_LIKELY(repeated_)) {
            if (__set_state(TIMER_PENDING, TIMER_STARTED)) {
                // Update overtime.
                overtime_ = get_clock_milliseconds() + timeout_;
                // Add to timer queue.
                queue_->restart_timer(shared_from_this());
            }
        } else {
            __set_state(TIMER_PENDING, TIMER_STOPPED);
        }
    }
}

bool timer::__start(manager *queue) {
    if (!__set_state(TIMER_INIT, TIMER_STARTED)) {
        return false;
    }

    // Save timer queue.
    queue_ = queue;
    // Update overtime.
    overtime_ = get_clock_milliseconds() + timeout_;

    return true;
}

}  // namespace time
}  // namespace pump
