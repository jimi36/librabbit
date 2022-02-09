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

#include "pump/debug.h"
#include "pump/proto/quic/sid.h"

namespace pump {
namespace proto {
namespace quic {

sid::sid() : id_(-1) {}

sid::sid(uint64_t id) : id_(id) {}

sid::sid(const sid &id) : id_(id.id_) {}

}  // namespace quic
}  // namespace proto
}  // namespace pump