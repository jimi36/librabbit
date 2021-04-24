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

// Import random functions
#include <random>

// Import std::find function
#include <algorithm>

#include "pump/codec/sha1.h"
#include "pump/codec/base64.h"
#include "pump/protocol/websocket/utils.h"

namespace pump {
namespace protocol {
namespace websocket {

    uint8_t random_uint8() {
        static std::default_random_engine e((uint32_t)::time(0));
        static std::uniform_int_distribution<uint16_t> u(0, 255);
        return (uint8_t)u(e);
    }

    std::string compute_sec_key() {
        std::string tmp(16, 0);
        for (int32_t i = 0; i < 8; i++) {
            tmp[i * 2] = random_uint8();
        }

        return codec::base64_encode(tmp);
    }

    std::string compute_sec_accept_key(const std::string &sec_key) {
        std::string hash(20, 0);
        std::string tmp = sec_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        codec::sha1(tmp.c_str(), (uint32_t)tmp.size(), (uint8_t*)hash.c_str());

        return codec::base64_encode(hash);
    }

    std::string match_protocol(
        const std::vector<std::string> &srcs,
        const std::string &des) {
        std::string protocol;
        if (std::find(srcs.begin(), srcs.end(), des) != srcs.end()) {
            protocol = des;
        }
        return protocol;
    }

    void send_http_error_response(
        connection *conn,
        int32_t status_code,
        const std::string &reason) {
        http::response resp;
        resp.set_http_version(http::VERSION_11);
        resp.set_status_code(status_code);

        if (!reason.empty()) {
            resp.set_head("Content-Length", (int32_t)reason.size());

            http::body_sptr hb(new http::body);
            hb->append(reason);

            resp.set_body(hb);
        }

        std::string data;
        resp.serialize(data);
        conn->send(data.c_str(), (uint32_t)data.size());
    }

}  // namespace websocket
}  // namespace protocol
}  // namespace pump
