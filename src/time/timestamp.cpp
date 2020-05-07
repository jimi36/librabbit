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

#include "pump/time/timestamp.h"

namespace pump {
	namespace time {

#		define US_PER_MS			1000
#		define MS_PER_SECOND		US_PER_MS
#		define US_PER_SECOND		US_PER_MS * US_PER_MS

		uint64 get_clock_microsecond()
		{
			return std::chrono::time_point_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now()
				).time_since_epoch().count();
		}

		uint64 get_clock_milliseconds()
		{
			return std::chrono::time_point_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now()
				).time_since_epoch().count();
		}

		std::string timestamp::to_string() PUMP_CONST
		{
			struct tm tm_time;
			char date[64] = { 0 };
			uint64 ms = ms_.count();
			time_t seconds = static_cast<time_t>(ms / MS_PER_SECOND);
			uint32 milliseconds = static_cast<uint32>(ms % MS_PER_SECOND);
#if defined(WIN32)
			::localtime_s(&tm_time, &seconds);
			pump_snprintf(date, sizeof(date) - 1, "%4d-%d-%d %d:%d:%d:%d",
				tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
				tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, milliseconds);
#else
			::gmtime_r(&seconds, &tm_time);
			pump_snprintf(date, sizeof(date) - 1, "%4d-%d-%d %d:%d:%d:%d",
				tm_time.tm_year + 1970, tm_time.tm_mon + 1, tm_time.tm_mday,
				tm_time.tm_hour - 8, tm_time.tm_min, tm_time.tm_sec, milliseconds);
#endif

			return date;
		}

		std::string timestamp::format(PUMP_CONST std::string &format) PUMP_CONST
		{
			struct tm tm_time;
			char date[64] = { 0 };
			uint32 idx = 0, len = 0;
			uint64 ms = ms_.count();
			time_t seconds = static_cast<time_t>(ms / MS_PER_SECOND);
			uint32 millisecond = static_cast<uint32>(ms % MS_PER_SECOND);
#if defined(WIN32)
			localtime_s(&tm_time, &seconds);
			while (idx < format.size())
			{
				if (strncmp(format.c_str() + idx, "YY", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%4d", tm_time.tm_year + 1900);
				}
				else if (strncmp(format.c_str() + idx, "MM", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", tm_time.tm_mon + 1);
				}
				else if (strncmp(format.c_str() + idx, "DD", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", tm_time.tm_mday);
				}
				else if (strncmp(format.c_str() + idx, "hh", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", tm_time.tm_hour);
				}
				else if (strncmp(format.c_str() + idx, "mm", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", tm_time.tm_min);
				}
				else if (strncmp(format.c_str() + idx, "ss", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + idx, sizeof(date) - len - 1, "%d", tm_time.tm_sec);
				}
				else if (strncmp(format.c_str() + idx, "ms", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", millisecond);
				}
				else
				{
					date[len++] = format[idx++];
				}
			}
#else
			gmtime_r(&seconds, &tm_time);
			while (idx < format.size())
			{
				if (strncmp(format.c_str() + idx, "YY", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%4d", tm_time.tm_year + 1970);
				}
				else if (strncmp(format.c_str() + idx, "MM", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", tm_time.tm_mon + 1);
				}
				else if (strncmp(format.c_str() + idx, "DD", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", tm_time.tm_mday);
				}
				else if (strncmp(format.c_str() + idx, "hh", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", tm_time.tm_hour - 8);
				}
				else if (strncmp(format.c_str() + idx, "mm", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", tm_time.tm_min);
				}
				else if (strncmp(format.c_str() + idx, "ss", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + idx, sizeof(date) - len - 1, "%d", tm_time.tm_sec);
				}
				else if (strncmp(format.c_str() + idx, "ms", 2) == 0)
				{
					idx += 2;
					len += pump_snprintf(date + len, sizeof(date) - len - 1, "%d", millisecond);
				}
				else
				{
					date[len++] = format[idx++];
				}
			}
#endif
			return date;
		}

		uint64 timestamp::now_time()
		{
			auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now()
				);
			std::chrono::milliseconds ms(now.time_since_epoch().count());
			return ms.count();
		}

	}
}
