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

#ifndef librabbit_transport_flow_tls_dialer_h
#define librabbit_transport_flow_tls_dialer_h

#include "librabbit/transport/flow/flow_tcp_dialer.h"

namespace librabbit {
	namespace transport {
		namespace flow {

			typedef flow_tcp_dialer flow_tls_dialer;
			DEFINE_ALL_POINTER_TYPE(flow_tls_dialer);

		}
	}
}

#endif