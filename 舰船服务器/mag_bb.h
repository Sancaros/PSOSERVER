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

/* 计算玛古等级 */
uint16_t compute_mag_level(const item_t* item);

void ItemMagStats_init(magitemstat_t* stat, uint8_t color);
void assign_mag_stats(item_t* item, magitemstat_t* mag);

/* MAG 喂养函数 */
int player_feed_mag(ship_client_t* src, size_t mag_item_index, size_t fed_item_index);