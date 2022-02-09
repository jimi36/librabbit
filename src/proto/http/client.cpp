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

#include <future>
#include <algorithm>

#include "pump/proto/http/uri.h"
#include "pump/proto/http/frame.h"
#include "pump/proto/http/client.h"
#include "pump/transport/tcp_dialer.h"
#include "pump/transport/tls_dialer.h"

namespace pump {
namespace proto {
namespace http {

client::client(service *sv) :
    sv_(sv), dial_timeout_(0), tls_handshake_timeout_(0), wait_for_response_(false) {}

client::~client() {}

response_sptr client::do_request(request_sptr &req) {
    response_sptr resp;

    do {
        std::unique_lock<std::mutex> lock(resp_mx_);

        const uri *u = req->get_uri();
        if (u->get_type() != URI_HTTP && u->get_type() != URI_HTTPS) {
            PUMP_WARN_LOG("request uri type is not http or https");
            break;
        }

        if (!__steup_connection_and_listen_response(u)) {
            PUMP_WARN_LOG("setup connection or listen response failed");
            break;
        }

        if (!conn_->send(std::static_pointer_cast<packet>(req))) {
            PUMP_WARN_LOG("send the http request failed");
            break;
        }

        wait_for_response_ = true;
        if (resp_cond_.wait_for(lock, std::chrono::seconds(5)) !=
            std::cv_status::timeout) {
            resp = std::move(resp_);
        } else {
            PUMP_WARN_LOG("wait http response timeout");
            __destroy_connection();
        }
        wait_for_response_ = false;
    } while (false);

    return resp;
}

connection_sptr client::open_websocket(const std::string &url) {
    connection_sptr conn;

    do {
        std::unique_lock<std::mutex> lock(resp_mx_);

        uri u(url);
        if (u.get_type() != URI_WS && u.get_type() != URI_WSS) {
            PUMP_WARN_LOG("request uri type is not ws or wss");
            break;
        }

        if (!__steup_connection_and_listen_response(&u)) {
            PUMP_WARN_LOG("setup connection or listen response failed");
            break;
        }

        std::map<std::string, std::string> headers;
        if (!__send_websocket_upgrade_request(url, headers)) {
            PUMP_WARN_LOG("send websocket upgrade request failed");
            break;
        }

        wait_for_response_ = true;
        if (resp_cond_.wait_for(lock, std::chrono::seconds(5)) ==
            std::cv_status::timeout) {
            PUMP_WARN_LOG("wait websocket upgrade response timeout");
            __destroy_connection();
            break;
        }
        wait_for_response_ = false;

        if (!resp_ || !__handle_websocket_upgrade_response(resp_)) {
            PUMP_WARN_LOG("handle websocket upgrade response failed");
            __destroy_connection();
            break;
        }

        conn_->__init_websocket_mask();

        conn = conn_;
        conn_.reset();
    } while (false);

    return conn;
}

bool client::__steup_connection_and_listen_response(const uri *u) {
    if (u->get_host() != last_uri_.get_host() || u->get_type() != last_uri_.get_type()) {
        __destroy_connection();
    }

    if (conn_ && conn_->is_valid()) {
        if (conn_->__read_next_http_packet()) {
            return true;
        } else {
            PUMP_WARN_LOG("http connection start to read response failed");
            conn_->stop();
            conn_.reset();
        }
    }

    PUMP_DEBUG_LOG("create new http connection %s", u->to_url().c_str());

    base_transport_sptr transp;
    address bind_address("0.0.0.0", 0);
    address peer_address = u->to_address();
    if (u->get_type() == URI_WSS || u->get_type() == URI_HTTPS) {
        auto dialer = transport::tls_sync_dialer::create();
        transp = dialer->dial(sv_,
                              bind_address,
                              peer_address,
                              dial_timeout_,
                              tls_handshake_timeout_);
    } else {
        auto dialer = transport::tcp_sync_dialer::create();
        transp = dialer->dial(sv_, bind_address, peer_address, dial_timeout_);
    }
    if (!transp) {
        PUMP_WARN_LOG("establish connection transport failed");
        return false;
    }

    conn_.reset(new connection(false, transp));
    if (!conn_) {
        PUMP_ERR_LOG("new connection object failed");
        return false;
    }

    http_callbacks cbs;
    client_wptr cli = shared_from_this();
    cbs.error_cb = pump_bind(&client::on_error, cli, conn_.get(), _1);
    cbs.packet_cb = pump_bind(&client::on_response, cli, conn_.get(), _1);
    if (!conn_->start_http(sv_, cbs)) {
        PUMP_WARN_LOG("start http connection failed");
        return false;
    }

    last_uri_ = *u;

    return true;
}

void client::__destroy_connection() {
    if (conn_) {
        conn_->stop();
        conn_.reset();
    }
}

bool client::__send_websocket_upgrade_request(
    const std::string &url,
    std::map<std::string, std::string> &headers) {
    // Create upgrade request.
    request req(nullptr, url);
    req.set_http_version(http::VERSION_11);
    req.set_method(http::METHOD_GET);
    for (auto &h : headers) {
        req.set_head(h.first, h.second);
    }
    auto u = req.get_uri();
    if (!req.has_head("Host")) {
        req.set_unique_head("Host", u->get_host());
    }
    req.set_unique_head("Connection", "Upgrade");
    req.set_unique_head("Upgrade", "websocket");
    req.set_unique_head("Sec-WebSocket-Version", "13");
    req.set_unique_head("Sec-WebSocket-Key", compute_sec_key());

    return conn_->send(&req);
}

bool client::__handle_websocket_upgrade_response(response_sptr &rsp) {
    if (rsp->get_status_code() != 101 || rsp->get_http_version() != http::VERSION_11) {
        return false;
    }

    std::string upgrade;
    if (!rsp->get_head("Upgrade", upgrade) || upgrade != "websocket") {
        return false;
    }

    std::vector<std::string> connection;
    if (!rsp->get_head("Connection", connection) ||
        std::find(connection.begin(), connection.end(), "Upgrade") == connection.end()) {
        return false;
    }

    std::string sec_accept;
    if (!rsp->get_head("Sec-WebSocket-Accept", sec_accept)) {
        return false;
    }

    return true;
}

void client::__notify_response(connection *conn, response_sptr &&resp) {
    std::unique_lock<std::mutex> lock(resp_mx_);
    if (wait_for_response_ && conn == conn_.get()) {
        resp_ = std::forward<response_sptr>(resp);
        resp_cond_.notify_one();
    }
}

void client::on_response(client_wptr wptr, connection *conn, packet_sptr &pk) {
    auto cli = wptr.lock();
    if (cli) {
        PUMP_DEBUG_LOG("connection of http client receive response");
        cli->__notify_response(conn, std::static_pointer_cast<response>(pk));
    }
}

void client::on_error(client_wptr wptr, connection *conn, const std::string &msg) {
    auto cli = wptr.lock();
    if (cli) {
        PUMP_DEBUG_LOG("connection of http client %s", msg.c_str());
        cli->__notify_response(conn, response_sptr());
    }
}

}  // namespace http
}  // namespace proto
}  // namespace pump
