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

#ifndef pump_proto_quic_varint_h
#define pump_proto_quic_varint_h

#include <string>

#include "pump/toolkit/buffer.h"

namespace pump {
namespace proto {
namespace quic {

    using toolkit::io_buffer;

    /*********************************************************************************
     * GEt varint length of value
     ********************************************************************************/
    int32_t varint_length(uint64_t val);

    /*********************************************************************************
     * Varint encode
     ********************************************************************************/
    bool varint_encode(uint64_t val, io_buffer *iob);

    /*********************************************************************************
     * Varint decode
     ********************************************************************************/
    bool varint_decode(io_buffer *iob, uint64_t *val);

    /*********************************************************************************
     * Varient decode ex
     ********************************************************************************/
    template <typename T>
    bool varint_decode_ex(io_buffer *iob, T *val){
        uint64_t u64 = 0;
        if (!varint_decode(iob, &u64)) {
            return false;
        }
        *val = T(u64);
        return true;
    }

}
}
}

#endif
