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

#ifndef pump_codec_base64_h
#define pump_codec_base64_h

#include <string>

#include <pump/types.h>

namespace pump {
namespace codec {

/*********************************************************************************
 * Base64 encode string length
 ********************************************************************************/
pump_lib uint32_t base64_encode_length(const std::string &in);

/*********************************************************************************
 * Base64 encode
 ********************************************************************************/
pump_lib std::string base64_encode(const std::string &in);

/*********************************************************************************
 * Base64 decode string length
 ********************************************************************************/
pump_lib uint32_t base64_decode_length(const std::string &in);

/*********************************************************************************
 * Base64 decode
 ********************************************************************************/
pump_lib std::string base64_decode(const std::string &in);

}  // namespace codec
}  // namespace pump

#endif
