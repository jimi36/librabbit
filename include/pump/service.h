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

#ifndef pump_service_h
#define pump_service_h

#include "pump/poll/poller.h"
#include "pump/time/timer_queue.h"
#include "pump/toolkit/freelock_multi_queue.h"
#include "pump/toolkit/freelock_single_queue.h"
#include "pump/toolkit/freelock_block_queue.h"

namespace pump {

    typedef int32_t poller_id;
    const poller_id READ_POLLER_ID = 0;
    const poller_id SEND_POLLER_ID = 1;

    class LIB_PUMP service 
      : public toolkit::noncopyable {

      public:
        /*********************************************************************************
         * Constructor
         ********************************************************************************/
        service(bool enable_poller = true);

        /*********************************************************************************
         * Deconstructor
         ********************************************************************************/
        ~service();

        /*********************************************************************************
         * Start
         ********************************************************************************/
        bool start();

        /*********************************************************************************
         * Stop
         ********************************************************************************/
        void stop();

        /*********************************************************************************
         * Wait stopping
         ********************************************************************************/
        void wait_stopped();

        /*********************************************************************************
         * Add channel checker
         ********************************************************************************/
        PUMP_INLINE bool add_channel_tracker(
            poll::channel_tracker_sptr &tracker, 
            poller_id pid) {
            PUMP_ASSERT(pid <= SEND_POLLER_ID);
            if (pollers_[pid]) {
                return pollers_[pid]->add_channel_tracker(tracker);
            }
            return false;
        }

        /*********************************************************************************
         * Delete channel
         ********************************************************************************/
        PUMP_INLINE void remove_channel_tracker(
            poll::channel_tracker_sptr &tracker, 
            poller_id pid) {
            PUMP_ASSERT(pid <= SEND_POLLER_ID);
            if (pollers_[pid]) {
                return pollers_[pid]->remove_channel_tracker(tracker);
            }
        }

        /*********************************************************************************
         * Resume channel
         ********************************************************************************/
        PUMP_INLINE bool resume_channel_tracker(
            poll::channel_tracker *tracker, 
            poller_id pid) {
            PUMP_ASSERT(pid <= SEND_POLLER_ID);
            if (pollers_[pid]) {
                return pollers_[pid]->resume_channel_tracker(tracker);
            }
            return false;
        }

        /*********************************************************************************
         * Post channel event
         ********************************************************************************/
        PUMP_INLINE bool post_channel_event(
            poll::channel_sptr &ch, 
            int32_t event) {
            if (PUMP_LIKELY(!!pollers_[SEND_POLLER_ID])) {
                return pollers_[SEND_POLLER_ID]->push_channel_event(ch, event);
            }
            return false;
        }

        /*********************************************************************************
         * Post callback task
         ********************************************************************************/
        template <typename PostedTaskType>
        PUMP_INLINE void post(PostedTaskType &&task) {
            posted_tasks_.enqueue(std::forward<PostedTaskType>(task));
        }

        /*********************************************************************************
         * Start timer
         ********************************************************************************/
        PUMP_INLINE bool start_timer(time::timer_sptr &timer) {
            auto queue = timers_;
            if (PUMP_LIKELY(!!queue)) {
                return queue->start_timer(timer);
            }
            return false;
        }

      private:
        /*********************************************************************************
        * Post pending timer
        ********************************************************************************/
        PUMP_INLINE void __post_pending_timer(time::timer_wptr &&timer) {
            pending_timers_.enqueue(std::move(timer));
        }

        /*********************************************************************************
         * Start posted task worker
         ********************************************************************************/
        void __start_posted_task_worker();

        /*********************************************************************************
         * Start timeout timer worker
         ********************************************************************************/
        void __start_timeout_timer_worker();

      private:
        // Running status
        bool running_;

        // Pollers
        poll::poller *pollers_[2];

        // Task worker
        std::shared_ptr<std::thread> task_worker_;
        typedef pump_function<void()> posted_task_type;
        typedef toolkit::freelock_multi_queue<posted_task_type> task_impl_queue;
        toolkit::freelock_block_queue<task_impl_queue> posted_tasks_;

        // Timers
        time::timer_queue_sptr timers_;

        // Timer worker
        std::shared_ptr<std::thread> timer_worker_;
        typedef toolkit::freelock_single_queue<time::timer_wptr> timer_impl_queue;
        toolkit::freelock_block_queue<timer_impl_queue> pending_timers_;
    };
    DEFINE_ALL_POINTER_TYPE(service);

    class LIB_PUMP service_getter {
      public:
        /*********************************************************************************
         * Constructor
         ********************************************************************************/
        service_getter(service *sv) noexcept
          : service_(sv) {
        }

        /*********************************************************************************
         * Deconstructor
         ********************************************************************************/
        ~service_getter() = default;

        /*********************************************************************************
         * Get service
         ********************************************************************************/
        PUMP_INLINE service* get_service() {
            return service_;
        }

      protected:
        /*********************************************************************************
         * Set service
         ********************************************************************************/
        PUMP_INLINE void __set_service(service *sv) {
            service_ = sv;
        }

      private:
        service *service_;
    };

}  // namespace pump

#endif
