/*
	梦幻之星中国 舰船服务器 公会物品列表
	版权 (C) 2023 Sancaros

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

#ifndef PSO_GUILD_SPECIAL_ITEM_LIST_H
#define PSO_GUILD_SPECIAL_ITEM_LIST_H

#include <stdint.h>

typedef struct bb_guild_special_items {
    char item_name[64];
    char item_desc[128];
    uint32_t point_amount;
} bb_guild_special_items_t;

static const bb_guild_special_items_t guild_special_items[] = {
    {"测试专用", "\tE\tC6解锁公会的功能.", 0},
    {"公会标志", "\tE\tC6解锁公会的标志功能,只有会长可以更改公会标志.", 1000},
    {"更衣室", "\tE\tC6解锁公会的更衣室功能.", 1000},
    {"改名卡", "\tE\tC6解锁公会的改名功能.", 1000},
    {"公会人数 20", "\tE\tC6公会人数增加为20人,副会长人数限定为3人.", 1000},
    {"公会人数 40", "\tE\tC6公会人数增加为40人,副会长人数限定为5人.", 2000},
    {"公会人数 70", "\tE\tC6公会人数增加为70人,副会长人数限定为8人.", 5000},
    {"公会人数 100", "\tE\tC6公会人数增加为70人,副会长人数限定为8人.", 10000},
    {"会长指挥官大剑", "\tE\tC6购买会长指挥官大剑", 5000},
    {"公会盔甲", "\tE\tC6购买公会盔甲", 100},
    {"公会盾牌", "\tE\tC6购买公会盾牌", 100},
    {"公会点数 500", "\tE\tC6兑换公会点数 500", 500},
    {"公会点数 1000", "\tE\tC6兑换公会点数 1000", 1000},
    {"公会点数 5000", "\tE\tC6兑换公会点数 5000", 5000},
    {"公会点数 10000", "\tE\tC6兑换公会点数 10000", 10000},
    {"公会任务: 任务灾难中枢", "\tE\tC6解锁公会任务", 1500},
};

#endif // !PSO_GUILD_SPECIAL_ITEM_LIST_H
