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

#ifndef librabbit_poll_select_poller_h
#define librabbit_poll_select_poller_h

#include "librabbit/poll/poller.h"
#include "librabbit/utils/spin_mutex.h"

namespace librabbit {
	namespace poll {

		class LIB_EXPORT select_poller: public poller
		{
		public:
			/*********************************************************************************
			 * Constructor
			 ********************************************************************************/
			select_poller(bool pop_pending);

			/*********************************************************************************
			 * Deconstructor
			 ********************************************************************************/
			virtual ~select_poller();

		protected:
			/*********************************************************************************
			 * Poll
			 * actives is saving active channels
			 * timeout is polling timeout time, if set as -1, then no wait
			 ********************************************************************************/
			virtual void __poll(int32 timeout);

		private:
			/*********************************************************************************
			 * Dispatch pending event
			 ********************************************************************************/
			void __dispatch_pending_event(fd_set &rfds, fd_set &wfds);

		private:
			fd_set rfds_;
			fd_set wfds_;

			struct timeval tv_;

#ifdef WIN32
			std::mutex mx_;
			std::condition_variable cv_;
#endif
		};

	}
}

#endif
