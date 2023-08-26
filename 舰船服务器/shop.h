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
	struct mt19937_state* 随机因子;
	uint8_t 难度;
	uint8_t 玩家等级;
	uint8_t 物品类型;
	uint8_t 保留1;
	uint8_t 保留2;
	uint8_t 保留3;
} PACKED shop_item_params_t;

/* 商店结构 */
typedef struct shop_data {
    uint16_t pkt_len;
    uint16_t pkt_type;
    uint32_t flags;
    uint8_t type;
    uint8_t size;
    uint16_t reserved;
    uint8_t shop_type;
    uint8_t num_items;
    uint16_t reserved2;
	item_t item[0x18];
    uint8_t reserved4[16];
} PACKED shop_data_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* 获取物品价格 */
uint32_t get_bb_shop_price(iitem_t* ci);

/* 生成商店物品 */
item_t create_bb_shop_tool_common_item(uint8_t 难度, uint8_t 物品类型, uint8_t index);
item_t create_bb_shop_item(uint8_t 难度, uint8_t 物品类型/*, struct mt19937_state* 随机因子*/, sfmt_t* 随机因子);
size_t price_for_item(const item_t* item);

const static char* shop_files[] = {
    "System\\Shop\\shop.dat",
    "System\\Shop\\shop2.dat",
    "System\\Shop\\shop3.dat",
};

static shop_data_t shops[7000];
//static uint32_t shop_checksum; 未做数据检测
uint32_t shopidx[MAX_PLAYER_LEVEL];
uint32_t equip_prices[2][13][24][80];

int load_shop_data();

// Item list for Good Luck quest...祝你好运任务的物品清单
const static uint32_t good_luck[] = {
	0x00004C00,
	0x00004500,
	0x00078F00,
	0x00004400,
	0x00009A00,
	0x00006A00,
	0x002B0201,
	0x00310101,
	0x00300101,
	0x00320101,
	0x00003400,
	0x00001300,
	0x00003C00,
	0x00002700,
	0x00003E00,
	0x00003900,
	0x00004100,
	0x00290101,
	0x00290101,
	0x00290201,
	0x00350201,
	0x00330101,
	0x001C0101,
	0x00002000,
	0x00060600,
	0x00002C00,
	0x00060100,
	0x00070100,
	0x00210201,
	0x00070800,
	0x00070400,
	0x00010D00,
	0x00240201,
	0x001B0101,
	0x00060200,
	0x00001003
};

// Gallon's shop "Hopkins' Dad's" item list + Photon Drops required for each
const static uint32_t gallons_shop_hopkins[] = {
	0x000003, 1,
	0x000103, 1,
	0x000F03, 1,
	0x4A0201, 1,
	0x4B0201, 1,
	0x050E03, 20,
	0x060E03, 20,
	0x000E03, 20,
	0x4C0201, 20,
	0x4C0201, 20,
	0x4E0201, 20,
	0x020D00, 20,
	0x5900, 20,
	0x5200, 20,
	0x030E03, 50,
	0x070E03, 50,
	0x270E03, 50,
	0x011003, 99
};


// Gallon's shop "Hopkins' Dad's" amount of PDs required for S-Rank attribute
const static uint8_t gallon_srank_pds[] = {
	0x01, 60,
	0x02, 60,
	0x03, 20,
	0x04, 20,
	0x05, 30,
	0x06, 30,
	0x07, 30,
	0x08, 50,
	0x09, 40,
	0x0A, 50,
	0x0B, 40,
	0x0C, 40,
	0x0D, 50,
	0x0E, 40,
	0x0F, 40,
	0x10, 40
};

// Tekking Attribute arrays
const static uint8_t tekker_attributes[] = {
	0x00, 0x00, 0x00,
	0x01, 0x01, 0x02,
	0x02, 0x01, 0x02,
	0x03, 0x02, 0x04,
	0x04, 0x03, 0x04,
	0x05, 0x05, 0x06,
	0x06, 0x05, 0x07,
	0x07, 0x06, 0x08,
	0x08, 0x07, 0x08,
	0x09, 0x09, 0x0A,
	0x0A, 0x09, 0x0B,
	0x0B, 0x0A, 0x0B,
	0x0C, 0x0C, 0x0C,
	0x0D, 0x0D, 0x0D,
	0x0E, 0x0E, 0x0E,
	0x0F, 0x0F, 0x10,
	0x10, 0x0F, 0x11,
	0x11, 0x10, 0x12,
	0x12, 0x11, 0x12,
	0x13, 0x13, 0x14,
	0x14, 0x13, 0x15,
	0x15, 0x14, 0x15,
	0x16, 0x15, 0x16,
	0x17, 0x17, 0x18,
	0x18, 0x17, 0x19,
	0x19, 0x18, 0x1A,
	0x1A, 0x19, 0x1A,
	0x1B, 0x1B, 0x1C,
	0x1C, 0x1B, 0x1D,
	0x1D, 0x1C, 0x1E,
	0x1E, 0x1D, 0x1E,
	0x1F, 0x1F, 0x20,
	0x20, 0x1F, 0x21,
	0x21, 0x20, 0x22,
	0x22, 0x21, 0x22,
	0x23, 0x23, 0x24,
	0x24, 0x23, 0x25,
	0x25, 0x24, 0x26,
	0x26, 0x25, 0x26,
	0x27, 0x27, 0x28,
	0x28, 0x27, 0x28,
};

// Shop Sellback Price per Attribute
const static int32_t attrib[] = {
	0,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	500,
	1125,
	2000,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	125,
	500,
	1125,
	2000,
	1125,
	2000
};

// Shop Sellback Price per Common Armor

const static int32_t armor_prices[] = {
	24, 33, 41, 50, 59,
	65, 100, 135, 170, 205,
	123, 185, 246, 307, 368,
	201, 288, 376, 463, 551,
	305, 418, 532, 646, 760,
	415, 555, 695, 835, 975,
	556, 723, 889, 1055, 1221,
	708, 910, 1111, 1312, 1513,
	896, 1132, 1368, 1605, 1841,
	1081, 1352, 1623, 1895, 2166,
	1306, 1612, 1918, 2225, 2531,
	1523, 1865, 2206, 2547, 2888,
	1786, 2162, 2538, 2915, 3291,
	2036, 2448, 2859, 3270, 3681,
	2336, 2783, 3229, 3675, 4121,
	2620, 3101, 3582, 4063, 4545,
	2957, 3473, 3990, 4506, 5022,
	3273, 3825, 4376, 4927, 5478,
	3648, 4235, 4821, 5407, 5993,
	3997, 4618, 5240, 5861, 6482,
	4410, 5066, 5722, 6378, 7035,
	4783, 5465, 6148, 6830, 7513,
	5215, 5915, 6615, 7315, 8015,
	6503, 7247, 7991, 8735, 9478
};


// Shop Sellback Price per Common Unit

const static int32_t unit_prices[0x40] =
{
	250,
	375,
	625,
	10,
	250,
	375,
	625,
	10,
	250,
	375,
	625,
	10,
	250,
	375,
	625,
	10,
	250,
	500,
	750,
	10,
	250,
	500,
	750,
	10,
	250,
	375,
	625,
	10,
	1000,
	10,
	1000,
	10,
	10,
	250,
	375,
	625,
	250,
	375,
	625,
	250,
	375,
	625,
	375,
	500,
	750,
	375,
	500,
	625,
	875,
	1000,
	10,
	500,
	750,
	10,
	500,
	750,
	10,
	500,
	750,
	10,
	750,
	10,
	10,
	750
};


// Shop Sellback Price per Technique

const static int32_t tech_prices[] =
{
	1250,
	3750,
	5625,
	1250,
	3750,
	5625,
	1250,
	3750,
	5625,
	6250,
	1250,
	1250,
	1250,
	1250,
	62500,
	3750,
	1250,
	62500,
	6250,
};


// Shop Sellback Price per Tool

const static int32_t tool_prices[] =
{
	0x000003, 6,
	0x010003, 37,
	0x020003, 250,
	0x000103, 12,
	0x010103, 62,
	0x020103, 450,
	0x000303, 37,
	0x000403, 62,
	0x000503, 625,
	0x000603, 7,
	0x010603, 7,
	0x000703, 43,
	0x000803, 12,
	0x000903, 1250,
	0x000A03, 625,
	0x010A03, 1250,
	0x020A03, 1875,
	0x000B03, 300,
	0x010B03, 300,
	0x020B03, 300,
	0x030B03, 300,
	0x040B03, 300,
	0x050B03, 300,
	0x060B03, 300,
	0x001003, 1000,
	0x011003, 2000,
	0x021003, 3000
};

#endif /* !SHOPDATA_H */
