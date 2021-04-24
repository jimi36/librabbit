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

#include "pump/utils.h"
#include "pump/protocol/http/utils.h"
#include "pump/protocol/http/request.h"

namespace pump {
namespace protocol {
namespace http {

    static const block_t *request_method_strings[] = {
        "UNKNOWN", 
        "GET", 
        "POST", 
        "HEAD", 
        "PUT", 
        "DELETE"
    };

    request::request(void *ctx) noexcept
      : pocket(PK_REQUEST), 
        ctx_(ctx), 
        method_(METHOD_UNKNOWN) {
    }

    request::request(
        const std::string &url,
        void *ctx) noexcept
      : pocket(PK_REQUEST), 
        ctx_(ctx), 
        method_(METHOD_UNKNOWN) {
        uri_.parse(url);
    }

    int32_t request::parse(
        const block_t *b, 
        int32_t size) {
        if (parse_status_ == PARSE_FINISHED) {
            return 0;
        }

        if (parse_status_ == PARSE_NONE) {
            parse_status_ = PARSE_LINE;
        }

        const block_t *pos = b;
        int32_t parse_size = 0;
        if (parse_status_ == PARSE_LINE) {
            parse_size = __parse_start_line(pos, size);
            if (parse_size <= 0) {
                return parse_size;
            }

            pos  += parse_size;
            size -= parse_size;

            parse_status_ = PARSE_HEADER;
        }

        if (parse_status_ == PARSE_HEADER) {
            parse_size = __parse_header(pos, size);
            if (parse_size < 0) {
                return -1;
            } else if (parse_size == 0) {
                return int32_t(pos - b);
            }

            pos  += parse_size;
            size -= parse_size;

            if (!__is_header_parsed()) {
                return int32_t(pos - b);
            }

            std::string host;
            if (get_head("Host", host)) {
                uri_.set_host(host);
            }

            parse_status_ = PARSE_CONTENT;
        }

        if (parse_status_ == PARSE_CONTENT) {
            body *rb = body_.get();
            if (rb == nullptr) {
                int32_t length = 0;
                if (get_head("Content-Length", length)) {
                    if (length > 0) {
                        if ((rb = object_create<body>()) == nullptr) {
                            return -1;
                        }
                        rb->set_length(length);
                        body_.reset(rb, object_delete<body>);
                    } 
                } else {
                    std::string transfer_encoding;
                    if (get_head("Transfer-Encoding", transfer_encoding) && transfer_encoding == "chunked") {
                        if ((rb = object_create<body>()) == nullptr) {
                            return -1;
                        }
                        rb->set_chunked();
                        body_.reset(rb, object_delete<body>);
                    }
                }
            }

            if (rb != nullptr) {
                parse_size = rb->parse(pos, size);
                if (parse_size < 0) {
                    return -1;
                } else if (parse_size == 0) {
                    return int32_t(pos - b);
                }

                pos  += parse_size;
                size -= parse_size;

                if (rb->is_parse_finished()) {
                    parse_status_ = PARSE_FINISHED;
                }
            } else {
                parse_status_ = PARSE_FINISHED;
            }
        }

        return int32_t(pos - b);
    }

    int32_t request::serialize(std::string &buffer) const {
        int32_t serialize_size = 0;

        int32_t size = __serialize_request_line(buffer);
        if (size < 0) {
            return -1;
        }
        serialize_size += size;

        size = __serialize_header(buffer);
        if (size < 0) {
            return -1;
        }
        serialize_size += size;

        if (body_) {
            size = body_->serialize(buffer);
            if (size < 0) {
                return -1;
            }
            serialize_size += size;
        }

        return serialize_size;
    }

    int32_t request::__parse_start_line(
        const block_t *b, 
        int32_t size) {
        const block_t *pos = b;

        // Find request line end
        const block_t *line_end = find_http_line_end(pos, size);
        if (line_end == nullptr) {
            return 0;
        }

        // Parse request method
        if (pos + 4 < line_end && memcmp(pos, "GET ", 4) == 0) {
            method_ = METHOD_GET, pos += 4;
        } else if (pos + 5 < line_end && memcmp(pos, "POST ", 5) == 0) {
            method_ = METHOD_POST, pos += 5;
        } else if (pos + 5 < line_end && memcmp(pos, "HEAD ", 5) == 0) {
            method_ = METHOD_HEAD, pos += 5;
        } else if (pos + 4 < line_end && memcmp(pos, "PUT ", 4) == 0) {
            method_ = METHOD_PUT, pos += 4;
        } else if (pos + 7 < line_end && memcmp(pos, "DELETE ", 7) == 0) {
            method_ = METHOD_DELETE, pos += 7;
        } else {
            return -1;
        }

        // Parse request path
        const block_t *old_pos = pos;
        while (pos < line_end && *pos != ' ' && *pos != '?') {
            ++pos;
        }
        if (pos == old_pos || pos == line_end) {
            return -1;
        }
        uri_.set_path(std::string(old_pos, pos));

        // Parse request params
        if (*pos == '?') {
            old_pos = ++pos;
            while (pos < line_end && *pos != ' ') {
                ++pos;
            }
            if (pos == old_pos || pos == line_end) {
                return -1;
            }

            std::string params;
            std::string raw_params(old_pos, pos);
            if (!url_decode(raw_params, params)) {
                return -1;
            }

            auto vals = split_string(params, "[=&]");
            uint32_t cnt = (uint32_t)vals.size();
            if (vals.empty() || cnt % 2 != 0) {
                return -1;
            }
            for (uint32_t i = 0; i < cnt; i += 2) {
                uri_.set_param(vals[i], vals[i + 1]);
            }
        }
        ++pos;

        // Parse request version
        if (memcmp(pos, "HTTP/1.0", 8) == 0) {
            version_ = VERSION_10;
        } else if (memcmp(pos, "HTTP/1.1", 8) == 0) {
            version_ = VERSION_11;
        } else if (memcmp(pos, "HTTP/2.0", 8) == 0) {
            version_ = VERSION_20;
        } else {
            return -1;
        }

        return int32_t(line_end - b);
    }

    int32_t request::__serialize_request_line(std::string &buf) const {
        block_t tmp[256] = {0};
        int32_t size = pump_snprintf(
                        tmp, 
                        sizeof(tmp) - 1,
                        "%s %s %s\r\n",
                        request_method_strings[method_],
                        uri_.get_path().c_str(),
                        get_http_version_string().c_str());
        buf.append(tmp);
        return size;
    }

}  // namespace http
}  // namespace protocol
}  // namespace pump