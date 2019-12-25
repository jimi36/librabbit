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

#include "pump/transport/address.h"

namespace pump {
	namespace transport {

		address::address():
			is_v6_(false),
			addrlen_(0),
			port_(0)
		{
			memset(&addr_, 0, sizeof(addr_));
		}

		address::address(const std::string &ip, uint16 port)
		{
			if (net::string_to_address(ip, port, (struct sockaddr*)addr_, &addrlen_))
			{
				if (addrlen_ == sizeof(struct sockaddr_in6))
					is_v6_ = true;
				else
					is_v6_ = false;

				__update();
			}
		}

		address::address(const struct sockaddr *addr, int32 addr_len)
		{
			addrlen_ = addr_len;
			memcpy(&addr_, addr, addr_len);
			if (addrlen_ == sizeof(struct sockaddr_in6))
				is_v6_ = true;
			else
				is_v6_ = false;

			__update();
		}

		bool address::set(const std::string &ip, uint16 port)
		{
			if (net::string_to_address(ip, port, (struct sockaddr*)addr_, &addrlen_))
			{
				if (addrlen_ == sizeof(struct sockaddr_in6))
					is_v6_ = true;
				else
					is_v6_ = false;

				__update();
			}
			else
			{
				return false;
			}

			return true;
		}

		bool address::set(const struct sockaddr *addr, int32 addrlen)
		{
			if (addrlen == sizeof(struct sockaddr_in6))
				is_v6_ = true;
			else if (addrlen == sizeof(struct sockaddr_in))
				is_v6_ = false;
			else
				return false;

			addrlen_ = addrlen;
			memcpy(addr_, addr, addrlen);

			__update();

			return true;
		}

		std::string address::to_string() const
		{
			int8 tmp[126] = { 0 };
			snprintf(tmp, sizeof(tmp) - 1, "%s:%d", ip_.c_str(), port_);
			return std::move(std::string(tmp));
		}

		bool address::operator ==(const address& other)
		{
			if (is_v6_ == other.is_v6_ &&
				addrlen_ == other.addrlen_ &&
				memcpy(addr_, other.addr_, addrlen_) == 0)
				return true;

			return false;
		}

		bool address::operator <(const address& other)
		{
			if (addrlen_ < other.addrlen_)
				return true;
			else if (addrlen_ > other.addrlen_)
				return false;
			return memcmp(addr_, other.addr_, addrlen_) < 0;
		}

		void address::__update()
		{
			int8 host[128] = { 0 };
			if (is_v6_)
			{
				struct sockaddr_in6 *v6 = (struct sockaddr_in6*)addr_;
				if (::inet_ntop(AF_INET6, &(v6->sin6_addr), host, sizeof(host)) != NULL)
					ip_ = host;
				port_ = ntohs(v6->sin6_port);
			}
			else
			{
				struct sockaddr_in *v4 = (struct sockaddr_in*)addr_;
				if (::inet_ntop(AF_INET, &(v4->sin_addr), host, sizeof(host)) != NULL)
					ip_ = host;
				port_ = ntohs(v4->sin_port);
			}
		}

	}
}