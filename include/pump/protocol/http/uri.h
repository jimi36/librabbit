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

#ifndef pump_protocol_http_uri_h
#define pump_protocol_http_uri_h

#include <map>

#include "pump/protocol/http/utils.h"

namespace pump {
namespace protocol {
namespace http {

    typedef int32_t uri_protocol;
    const static uri_protocol UIR_UNKNOWN  = 0;
    const static uri_protocol URI_HTTP     = 1;
    const static uri_protocol URI_HTTPS    = 2;
    const static uri_protocol URI_WS       = 3;
    const static uri_protocol URI_WSS      = 4;
    const static uri_protocol URI_END      = 5;

    class LIB_PUMP uri {

      public:
        /*********************************************************************************
         * Constructor
         ********************************************************************************/
        uri() noexcept;
        uri(const std::string &url) noexcept;

        /*********************************************************************************
         * Deconstructor
         ********************************************************************************/
        ~uri() = default;

        /*********************************************************************************
         * Reset uri
         ********************************************************************************/
        void reset();

        /*********************************************************************************
         * Parse url
         ********************************************************************************/
        bool parse(const std::string &url);

        /*********************************************************************************
         * Set uri type
         ********************************************************************************/
        PUMP_INLINE void set_protocol(uri_protocol proto) {
            proto_ = proto;
        }

        /*********************************************************************************
         * Get uri type
         ********************************************************************************/
        PUMP_INLINE uri_protocol get_type() const {
            return proto_;
        }

        /*********************************************************************************
         * Set host
         ********************************************************************************/
        PUMP_INLINE void set_host(const std::string &host) {
            host_ = host;
        }

        /*********************************************************************************
         * Get host
         ********************************************************************************/
        PUMP_INLINE const std::string &get_host() const {
            return host_;
        }

        /*********************************************************************************
         * Set path
         ********************************************************************************/
        PUMP_INLINE void set_path(const std::string &path) {
            path_ = path;
        }

        /*********************************************************************************
         * Get path
         ********************************************************************************/
        PUMP_INLINE const std::string& get_path() const {
            return path_;
        }

        /*********************************************************************************
         * Set param
         ********************************************************************************/
        PUMP_INLINE void set_param(
            const std::string &key, 
            const std::string &value) {
            params_[key] = value;
        }

        /*********************************************************************************
         * Get param
         ********************************************************************************/
        bool get_param(
            const std::string &key, 
            std::string &value) const;

        /*********************************************************************************
         * To url string
         ********************************************************************************/
        std::string to_url() const;

      private:
        // Uri protocol
        uri_protocol proto_;
        // Uri host
        std::string host_;
        // Uri path
        std::string path_;
        // Uri params
        std::map<std::string, std::string> params_;
    };
    DEFINE_ALL_POINTER_TYPE(uri);

    /*********************************************************************************
     * Get uri protocol name
     ********************************************************************************/
    LIB_PUMP const std::string& get_uri_protocol_name(int32_t proto);

    /*********************************************************************************
     * Parse url
     ********************************************************************************/
    LIB_PUMP bool parse_url(
        const std::string &url,
        uri_protocol &proto,
        std::string &host,
        std::string &path,
        std::map<std::string, std::string> &params);

}  // namespace http
}  // namespace protocol
}  // namespace pump

#endif