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
typedef struct mag {
	//typedef struct st_mag
	uint8_t two; // "02" =P
	uint8_t mtype;
	uint8_t level;
	uint8_t blasts;
	int16_t defense;
	int16_t power;
	int16_t dex;
	int16_t mind;
	uint32_t itemid;
	int8_t synchro;
	uint8_t IQ;
	uint8_t PBflags;
	uint8_t color;
} PACKED mag_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

// 玛古喂养表
static uint16_t mag_feed_t[MAG_MAX_FEED_TABLE][11 * 6] =
{
	{3, 3, 5, 40, 5, 0,
   3, 3, 10, 45, 5, 0,
   4, 4, 15, 50, 10, 0,
   3, 3, 5, 0, 5, 40,
   3, 3, 10, 0, 5, 45,
   4, 4, 15, 0, 10, 50,
   3, 3, 5, 10, 40, 0,
   3, 3, 5, 0, 44, 10,
   4, 1, 15, 30, 15, 25,
   4, 1, 15, 25, 15, 30,
   6, 5, 25, 25, 25, 25},

	{0, 0, 5, 10, 0, -1,
   2, 1, 6, 15, 3, -3,
   3, 2, 12, 21, 4, -7,
   0, 0, 5, 0, 0, 8,
   2, 1, 7, 0, 3, 13,
   3, 2, 7, -7, 6, 19,
   0, 1, 0, 5, 15, 0,
   2, 0, -1, 0, 14, 5,
   -2, 2, 10, 11, 8, 0,
   3, -2, 9, 0, 9, 11,
   4, 3, 14, 9, 18, 11},

	{0, -1, 1, 9, 0, -5,
   3, 0, 1, 13, 0, -10,
   4, 1, 8, 16, 2, -15,
   0, -1, 0, -5, 0, 9,
   3, 0, 4, -10, 0, 13,
   3, 2, 6, -15, 5, 17,
   -1, 1, -5, 4, 12, -5,
   0, 0, -5, -6, 11, 4,
   4, -2, 0, 11, 3, -5,
   -1, 1, 4, -5, 0, 11,
   4, 2, 7, 8, 6, 9},

	{0, -1, 0, 3, 0, 0,
   2, 0, 5, 7, 0, -5,
   3, 1, 4, 14, 6, -10,
   0, 0, 0, 0, 0, 4,
   0, 1, 4, -5, 0, 8,
   2, 2, 4, -10, 3, 15,
   -3, 3, 0, 0, 7, 0,
   3, 0, -4, -5, 20, -5,
   3, -2, -10, 9, 6, 9,
   -2, 2, 8, 5, -8, 7,
   3, 2, 7, 7, 7, 7},

	{2, -1, -5, 9, -5, 0,
   2, 0, 0, 11, 0, -10,
   0, 1, 4, 14, 0, -15,
   2, -1, -5, 0, -6, 10,
   2, 0, 0, -10, 0, 11,
   0, 1, 4, -15, 0, 15,
   2, -1, -5, -5, 16, -5,
   -2, 3, 7, -3, 0, -3,
   4, -2, 5, 21, -5, -20,
   3, 0, -5, -20, 5, 21,
   3, 2, 4, 6, 8, 5},

	{2, -1, -4, 13, -5, -5,
   0, 1, 0, 16, 0, -15,
   2, 0, 3, 19, -2, -18,
   2, -1, -4, -5, -5, 13,
   0, 1, 0, -15, 0, 16,
   2, 0, 3, -20, 0, 19,
   0, 1, 5, -6, 6, -5,
   -1, 1, 0, -4, 14, -10,
   4, -1, 4, 17, -5, -15,
   2, 0, -10, -15, 5, 21,
   3, 2, 2, 8, 3, 6},

	{-1, 1, -3, 9, -3, -4,
   2, 0, 0, 11, 0, -10,
   2, 0, 2, 15, 0, -16,
   -1, 1, -3, -4, -3, 9,
   2, 0, 0, -10, 0, 11,
   2, 0, -2, -15, 0, 19,
   2, -1, 0, 6, 9, -15,
   -2, 3, 0, -15, 9, 6,
   3, -1, 9, -20, -5, 17,
   0, 2, -5, 20, 5, -20,
   3, 2, 0, 11, 0, 11},

	{-1, 0, -4, 21, -15, -5, // Fixed the 2 incorrect bytes in table 7 (was cell table)
   0, 1, -1, 27, -10, -16,
   2, 0, 5, 29, -7, -25,
   -1, 0, -10, -5, -10, 21,
   0, 1, -5, -16, -5, 25,
   2, 0, -7, -25, 6, 29,
   -1, 1, -10, -10, 28, -10,
   2, -1, 9, -18, 24, -15,
   2, 1, 19, 18, -15, -20,
   2, 1, -15, -20, 19, 18,
   4, 2, 3, 7, 3, 3}
};

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

extern int mag_bb_feed(ship_client_t* c, uint32_t item_id, uint32_t mag_id);
