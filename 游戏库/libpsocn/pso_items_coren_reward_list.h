/*
	梦幻之星中国 舰船服务器 科伦奖励物品列表
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

#ifndef COREN_REWARD_LISH_H
#define COREN_REWARD_LISH_H
#include <stdint.h>

typedef struct Coren_Reward_List {
	int week;

} Coren_Reward_List_t;

int weeklyRewards[7][3] = {
	{4, 8, 4},  // Day 1，第一列物品概率为4%，第二列物品概率为8%，第三列物品概率为4%
	{4, 8, 4},  // Day 2
	{4, 8, 4},  // Day 3
	{4, 8, 4},  // Day 4
	{4, 8, 4},  // Day 5
	{4, 8, 4},  // Day 6
	{4, 8, 4}   // Day 7
};

#endif // !COREN_REWARD_LISH_H
