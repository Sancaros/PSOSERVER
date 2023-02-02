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

#include "pso_timer.h"
#include "f_logs.h"

int time_check(time_t c_time, uint32_t check_value) {
	int v = 0;

	if ((srv_time - c_time) >= check_value)
	{
		c_time = srv_time;
		//DBG_LOG("c_time = %d", (uint32_t)c_time);
		v = 1;
	}

	return v;
}
