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

#define US_PER_MS			1000
#define MS_PER_SECOND		US_PER_MS
#define US_PER_SECOND		US_PER_MS * US_PER_MS

		/*********************************************************************************
		 * Get microsecond, just for calculating time difference
		 ********************************************************************************/
		uint64 get_microsecond()
		{
#ifndef WIN32
			timeval t;
			gettimeofday(&t, 0);
			return t.tv_sec * US_PER_SECOND + t.tv_usec;
#else
			LARGE_INTEGER ccf, cc;
			//HANDLE hCurThread = ::GetCurrentThread(); 
			//DWORD_PTR dwOldMask = ::SetThreadAffinityMask(hCurThread, 1);
			QueryPerformanceFrequency(&ccf);
			QueryPerformanceCounter(&cc);
			//SetThreadAffinityMask(hCurThread, dwOldMask); 
			return (cc.QuadPart * US_PER_SECOND / ccf.QuadPart);
#endif
		}

		/*********************************************************************************
		 * Get time string
		 ********************************************************************************/
		std::string timestamp::to_string() const
		{
			struct tm tm_time;
			char buf[64] = { 0 };
			time_t seconds = static_cast<time_t>(time_ / MS_PER_SECOND);
			uint32 millisecond = static_cast<uint32>(time_ % MS_PER_SECOND);
#ifdef WIN32
			localtime_s(&tm_time, &seconds);
			snprintf(buf, sizeof(buf) - 1, "%4d-%d-%d %d:%d:%d:%d",
				tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
				tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, millisecond);
#else
			gmtime_r(&seconds, &tm_time);
			snprintf(buf, sizeof(buf) - 1, "%4d-%d-%d %d:%d:%d:%d",
				tm_time.tm_year + 1970, tm_time.tm_mon + 1, tm_time.tm_mday,
				tm_time.tm_hour - 8, tm_time.tm_min, tm_time.tm_sec, millisecond);
#endif

			return buf;
		}

		/*********************************************************************************
		 * Get time string
		 * YY as year
		 * MM as mouth
		 * DD as day
		 * hh as hour
		 * mm as minute
		 * ss as second
		 * ms as millsecond
		 ********************************************************************************/
		std::string timestamp::format(const std::string &fromat) const
		{
			struct tm tm_time;
			char buf[64] = { 0 };
			uint32 idx = 0, len = 0;
			time_t seconds = static_cast<time_t>(time_ / MS_PER_SECOND);
			uint32 millisecond = static_cast<uint32>(time_ % MS_PER_SECOND);

#ifdef WIN32
			localtime_s(&tm_time, &seconds);
			while (idx < fromat.size())
			{
				if (strncmp(fromat.c_str() + idx, "YY", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%4d", tm_time.tm_year + 1900);
				}
				else if (strncmp(fromat.c_str() + idx, "MM", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", tm_time.tm_mon + 1);
				}
				else if (strncmp(fromat.c_str() + idx, "DD", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", tm_time.tm_mday);
				}
				else if (strncmp(fromat.c_str() + idx, "hh", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", tm_time.tm_hour);
				}
				else if (strncmp(fromat.c_str() + idx, "mm", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", tm_time.tm_min);
				}
				else if (strncmp(fromat.c_str() + idx, "ss", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + idx, sizeof(buf) - len - 1, "%d", tm_time.tm_sec);
				}
				else if (strncmp(fromat.c_str() + idx, "ms", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", millisecond);
				}
				else
				{
					buf[len++] = fromat[idx++];
				}
			}
#else
			gmtime_r(&seconds, &tm_time);
			while (idx < fromat.size())
			{
				if (strncmp(fromat.c_str() + idx, "YY", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%4d", tm_time.tm_year + 1970);
				}
				else if (strncmp(fromat.c_str() + idx, "MM", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", tm_time.tm_mon + 1);
				}
				else if (strncmp(fromat.c_str() + idx, "DD", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", tm_time.tm_mday);
				}
				else if (strncmp(fromat.c_str() + idx, "hh", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", tm_time.tm_hour - 8);
				}
				else if (strncmp(fromat.c_str() + idx, "mm", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", tm_time.tm_min);
				}
				else if (strncmp(fromat.c_str() + idx, "ss", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + idx, sizeof(buf) - len - 1, "%d", tm_time.tm_sec);
				}
				else if (strncmp(fromat.c_str() + idx, "ms", 2) == 0)
				{
					idx += 2;
					len += snprintf(buf + len, sizeof(buf) - len - 1, "%d", millisecond);
				}
				else
				{
					buf[len++] = fromat[idx++];
				}
			}
#endif

			return buf;
		}

		/*********************************************************************************
		 * Get now microsecond
		 ********************************************************************************/
		uint64 timestamp::now_time()
		{
#ifdef WIN32
			FILETIME ft;
			GetSystemTimeAsFileTime(&ft);
			return (((uint64)(ft.dwHighDateTime) << 32) + (uint64)ft.dwLowDateTime - 116444736000000000) / 10000;
#else
			struct timeval tv;
			gettimeofday(&tv, NULL);
			return tv.tv_sec * MS_PER_SECOND + tv.tv_usec / US_PER_MS;
#endif
		}

	}
}