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

#include <pso_character.h>
#include "clients.h"

#define MAG_MAX_FEED_TABLE 8

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* 玛古结构 */
typedef struct magitem {
	//typedef struct st_mag
	uint8_t item_type; // "02" =P
	uint8_t mtype;
	uint8_t level;
	uint8_t photon_blasts;
	int16_t def;
	int16_t pow;
	int16_t dex;
	int16_t mind;
	uint32_t itemid;
	int8_t synchro;
	uint8_t IQ;
	uint8_t PBflags;
	uint8_t color;
} PACKED magitem_t;

typedef struct magitemstat {
	uint16_t iq;
	uint16_t synchro;
	uint16_t def;
	uint16_t pow;
	uint16_t dex;
	uint16_t mind;
	uint8_t flags;
	uint8_t photon_blasts;
	uint8_t color;
} PACKED magitemstat_t;

typedef struct magfeedresult {
	int8_t def;
	int8_t pow;
	int8_t dex;
	int8_t mind;
	int8_t iq;
	int8_t synchro;
	uint8_t unused[2];
} PACKED magfeedresult_t;

typedef struct magfeedresultslist {
	magfeedresult_t results[11];
} PACKED magfeedresultslist_t;

typedef struct magfeedresultslistoffsets {
	uint32_t offsets[8]; // Offsets of MagFeedResultsList structs
} PACKED magfeedresultslistoffsets_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

// 玛古细胞
enum mag_cell_type {
	MAG_CELL_502,
	MAG_CELL_213,
	MAG_CELL_PARTS_OF_ROBOCHAO,
	MAG_CELL_HEART_OF_OPA_OPA,
	MAG_CELL_HEART_OF_PAIN,
	MAG_CELL_HEART_OF_CHAO,
	MAG_CELL_MAX,
};

// 玛古颜色
enum mag_color_type {
	MagColor_Red,
	MagColor_Blue,
	MagColor_Yellow,
	MagColor_Green,
	MagColor_Purple,
	MagColor_Black,
	MagColor_White,
	MagColor_Cyan,
	MagColor_Brown,
	MagColor_Orange,
	MagColor_SlateBlue,
	MagColor_Olive,
	MagColor_Turquoise,
	MagColor_Fuschia,
	MagColor_Grey,
	MagColor_Ivory,
	MagColor_Pink,
	MagColor_DarkGreen,
	MagColor_Max,
};

// 玛古关联服装颜色
static uint8_t costume_mag_color_type[12][25] = { // 12 * (9 + 9 + 7)
	// 0x00 HUmar
	{
		MagColor_Orange, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Black, MagColor_White, MagColor_Olive, MagColor_Black,
		MagColor_Red, MagColor_Cyan, MagColor_Olive, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Grey, MagColor_White,
	},
	// 0x01 HUnewearl
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Brown, MagColor_DarkGreen,
		MagColor_Fuschia, MagColor_Blue, MagColor_Yellow, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Pink, MagColor_Blue,
	},
	// 0x02 HUcast
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Grey, MagColor_White, MagColor_Blue, MagColor_Grey,
		MagColor_Orange, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Purple, MagColor_DarkGreen,
		MagColor_Fuschia, MagColor_Blue, MagColor_Olive, MagColor_DarkGreen, MagColor_Fuschia, MagColor_Black, MagColor_White,
	},
	// 0x03 RAmar
	{
		MagColor_Red, MagColor_Blue, MagColor_Olive, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Ivory, MagColor_Black,
		MagColor_Orange, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_Ivory, MagColor_White, MagColor_Brown,
	},
	// 0x04 RAcast
	{
		MagColor_Red, MagColor_Blue, MagColor_Olive, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Black, MagColor_White, MagColor_White, MagColor_Orange,
		MagColor_Orange, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Grey, MagColor_White, MagColor_Blue, MagColor_Purple,
		MagColor_Fuschia, MagColor_Cyan, MagColor_Blue, MagColor_Turquoise, MagColor_SlateBlue, MagColor_Black, MagColor_White,
	},
	// 0x05 RAcaseal
	{
		MagColor_Pink, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Black, MagColor_SlateBlue, MagColor_Red, MagColor_Cyan,
		MagColor_Red, MagColor_Blue, MagColor_Brown, MagColor_DarkGreen, MagColor_Purple, MagColor_Orange, MagColor_Ivory, MagColor_Fuschia, MagColor_Yellow,
		MagColor_SlateBlue, MagColor_Cyan, MagColor_Yellow, MagColor_Turquoise, MagColor_Purple, MagColor_Grey, MagColor_Grey,
	},
	// 0x06 FOmarl
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_Green, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Pink, MagColor_Blue,
		MagColor_Red, MagColor_Cyan, MagColor_Yellow, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Pink, MagColor_Olive,
	},
	// 0x07 FOnewm
	{
		MagColor_Fuschia, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Red, MagColor_DarkGreen,
		MagColor_Brown, MagColor_Blue, MagColor_Yellow, MagColor_Green, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Purple, MagColor_Orange,
	},
	// 0x08 FOnewearl
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_Green, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Pink, MagColor_Black,
		MagColor_Orange, MagColor_Blue, MagColor_Olive, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Grey, MagColor_Blue,
	},
	// 0x09 HUcaseal
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_Ivory, MagColor_SlateBlue, MagColor_Purple,
		MagColor_Fuschia, MagColor_Blue, MagColor_Brown, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_Ivory, MagColor_Black, MagColor_Pink,
		MagColor_Pink, MagColor_Cyan, MagColor_Yellow, MagColor_Olive, MagColor_SlateBlue, MagColor_SlateBlue, MagColor_Ivory,
	},
	// 0x0a FOmar
	{
		MagColor_Red, MagColor_Blue, MagColor_Olive, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Brown, MagColor_SlateBlue,
		MagColor_Fuschia, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Black, MagColor_White, MagColor_Blue, MagColor_Olive,
	},
	// 0x0b RAmarl
	{
		MagColor_Red, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Orange, MagColor_Turquoise,
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_Fuschia, MagColor_Black, MagColor_Pink, MagColor_Blue, MagColor_White,
	},
};

// 玛古PB类型
enum pb_type {
	PB_Farlla,
	PB_Estlla,
	PB_Golla,
	PB_Pilla,
	PB_Leilla,
	PB_Mylla_Youlla
};

// 玛古类型
enum mag_type {
	Mag_Mag,
	Mag_Varuna,
	Mag_Mitra,
	Mag_Surya,
	Mag_Vayu,
	Mag_Varaha,
	Mag_Kama,
	Mag_Ushasu,
	Mag_Apsaras,
	Mag_Kumara,
	Mag_Kaitabha,
	Mag_Tapas,
	Mag_Bhirava,
	Mag_Kalki,
	Mag_Rudra,
	Mag_Marutah,
	Mag_Yaksa,
	Mag_Sita,
	Mag_Garuda,
	Mag_Nandin,
	Mag_Ashvinau,
	Mag_Ribhava,
	Mag_Soma,
	Mag_Ila,
	Mag_Durga,
	Mag_Vritra,
	Mag_Namuci,
	Mag_Sumba,
	Mag_Naga,
	Mag_Pitri,
	Mag_Kabanda,
	Mag_Ravana,
	Mag_Marica,
	Mag_Soniti,
	Mag_Preta,
	Mag_Andhaka,
	Mag_Bana,
	Mag_Naraka,
	Mag_Madhu,
	Mag_Churel,
	Mag_RoboChao,
	Mag_OpaOpa,
	Mag_Pian,
	Mag_Chao,
	Mag_CHUCHU,
	Mag_KAPUKAPU,
	Mag_ANGELSWING,
	Mag_DEVILSWING,
	Mag_ELENOR,
	Mag_MARK3,
	Mag_MASTERSYSTEM,
	Mag_GENESIS,
	Mag_SEGASATURN,
	Mag_DREAMCAST,
	Mag_HAMBURGER,
	Mag_PANZERSTAIL,
	Mag_DEVILSTAIL,
	Mag_Deva,
	Mag_Rati,
	Mag_Savitri,
	Mag_Rukmin,
	Mag_Pushan,
	Mag_Dewari,
	Mag_Sato,
	Mag_Bhima,
	Mag_Nidra,
	Mag_Geungsi,
	Mag_BlankMag,
	Mag_Tellusis,
	Mag_StrikerUnit,
	Mag_Pioneer,
	Mag_Puyo,
	Mag_Muyo,
	Mag_Rappy,
	Mag_Yahoo,  // renamed
	Mag_Gaelgiel,
	Mag_Agastya,
	Mag_0503,    // They do exist lol
	Mag_0504,
	Mag_0505,
	Mag_0506,
	Mag_0507
};

/* 计算玛古等级 */
uint16_t compute_mag_level(const item_t* item);

void ItemMagStats_init(magitemstat_t* stat);
void assign_mag_stats(item_t* item, magitemstat_t* mag);

/* MAG 喂养函数 */
int player_feed_mag(ship_client_t* src, size_t mag_item_index, size_t fed_item_index);

/* MAG 喂养函数 */
//int mag_bb_feed(ship_client_t* src, uint32_t mag_id, uint32_t item_id);