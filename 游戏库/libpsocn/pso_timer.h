/*
	梦幻之星中国 时间检测
	版权 (C) 2022, 2023 Sancaros

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License version 3
	as published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PSO_TIMER
#define PSO_TIMER

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Windows.h>

time_t srv_time;

int time_check(time_t c_time, uint32_t check_value);

static void get_local_time(char* timestamp) {
	SYSTEMTIME raw_time;
	TIME_ZONE_INFORMATION tzi;

	GetSystemTime(&raw_time);

	GetTimeZoneInformation(&tzi);
	SystemTimeToTzSpecificLocalTime(&tzi, &raw_time, &raw_time);

	if (timestamp)
		sprintf(timestamp, "%u:%02u:%02u: %02u:%02u:%02u.%03u",
			raw_time.wYear, raw_time.wMonth, raw_time.wDay,
			raw_time.wHour, raw_time.wMinute, raw_time.wSecond,
			raw_time.wMilliseconds);
	else
		printf("get_local_time 为空\n");

}

#endif // !PSO_TIMER