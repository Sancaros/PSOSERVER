/*
    梦幻之星中国 舰船服务器
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

#ifndef SHOPDATA_H
#define SHOPDATA_H

//#include <mtwist.h>
#include <SFMT.h>

/* Pull in the packet header types. */
#define PACKETS_H_HEADERS_ONLY
#include "packets.h"
#undef PACKETS_H_HEADERS_ONLY

#define BB_SHOP_SIZE               0x14

#define BB_SHOPTYPE_TOOL           0
#define BB_SHOPTYPE_WEAPON         1
#define BB_SHOPTYPE_ARMOR          2

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* 商店物品参数 */
typedef struct shop_item_params {
	sfmt_t* 随机因子;
	uint8_t 难度;
	uint8_t 玩家等级;
	uint8_t 物品类型;
	uint8_t 保留1;
	uint8_t 保留2;
	uint8_t 保留3;
} PACKED shop_item_params_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* 任务兑换限制 TODO 从数据库读取 */
//uint32_t* quest_allow = 0; // the "allow" list for the 0x60CA command...
//uint32_t quest_numallows;

/* 生成商店物品 */
item_t create_common_bb_shop_tool_item(uint8_t 难度, uint8_t index);
item_t create_common_bb_shop_item(uint8_t 难度, uint8_t 物品类型, sfmt_t* 随机因子);
item_t create_bb_shop_items(uint32_t player_level, uint8_t random_shop, uint32_t 商店类型, uint8_t 难度, uint8_t 物品索引, sfmt_t* 随机因子);
size_t price_for_item(const item_t* item);

#endif /* !SHOPDATA_H */
