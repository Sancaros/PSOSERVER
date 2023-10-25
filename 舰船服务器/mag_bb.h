/*
	�λ�֮���й� ����������
	��Ȩ (C) 2022, 2023 Sancaros

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
#include <SFMT.h>
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

typedef struct special_mag_random_pb {
	uint8_t datab3;
	uint8_t data2b2;
	char* desc;
}special_mag_random_pb_t;

static const special_mag_random_pb_t smrpb[] = {
	{0xCD, 0x07, "Leilla/Mylla & Youlla/Estlla"},/* �� Leilla �� Mylla & Youlla �� Estlla */
	{0x9D, 0x07, "Golla/Mylla & Youlla/Pilla"},/* �� Golla �� Mylla & Youlla �� Pilla */
	{0x45, 0x07, "Golla/Mylla & Youlla/Farlla"},/* �� Golla �� Mylla & Youlla �� Farlla */
	{0x01, 0x07, "Golla/Estlla/Farlla"},/* �� Golla �� Estlla �� Farlla */
	{0xE5, 0x07, "Pilla/Mylla & Youlla/Leilla"},/* �� Pilla �� Mylla & Youlla �� Leilla */
};

/* ����Ƿ���������� */
bool is_special_mag(item_t* mag, uint8_t section, uint8_t ch_class);
bool check_mag_slot_has_pb(const item_t* item);
int add_reward_special_mag_pb(ship_client_t* src, sfmt_t* sfmt);

/* ������ŵȼ� */
uint16_t compute_mag_level(const item_t* item);

void ItemMagStats_init(magitemstat_t* stat, uint8_t color);
void assign_mag_stats(item_t* item, magitemstat_t* mag);

/* MAG ι������ */
int player_feed_mag(ship_client_t* src, size_t mag_item_index, size_t fed_item_index);