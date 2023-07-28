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

#include <stdint.h>

#include <pso_character.h>
#include <f_logs.h>
#include <items.h>

#include "mag_bb.h"
#include "clients.h"
#include "ship_packets.h"
#include "player_handle_iitem.h"
#include "pmtdata.h"

typedef struct {
	uint32_t item_id;
	size_t result_index;
} fed_item_t;

const fed_item_t result_index_for_fed_item[] = {
	{0x030000, 0},  // Monomate
	{0x030001, 1},  // Dimate
	{0x030002, 2},  // Trimate
	{0x030100, 3},  // Monofluid
	{0x030101, 4},  // Difluid
	{0x030102, 5},  // Trifluid
	{0x030600, 6},  // Antidote
	{0x030601, 7},  // Antiparalysis
	{0x030300, 8},  // Sol Atomizer
	{0x030400, 9},  // Moon Atomizer
	{0x030500, 10}  // Star Atomizer
};

size_t num_results = sizeof(result_index_for_fed_item) / sizeof(result_index_for_fed_item[0]);

// Function to find the result index for a given fed item
size_t find_result_index(uint32_t item_id) {
	size_t i;
	for (i = 0; i < num_results; ++i) {
		if (result_index_for_fed_item[i].item_id == item_id) {
			return result_index_for_fed_item[i].result_index;
		}
	}
	return 0;  // Default result index if item_id is not found
}

uint16_t def_level(magitemstat_t* this) {
	return this->def / 100;
}
uint16_t pow_level(magitemstat_t* this) {
	return this->pow / 100;
}
uint16_t dex_level(magitemstat_t* this) {
	return this->dex / 100;
}
uint16_t mind_level(magitemstat_t* this) {
	return this->mind / 100;
}
uint16_t level(magitemstat_t* this) {
	return def_level(this) + pow_level(this) + dex_level(this) + mind_level(this);
}

uint16_t compute_mag_level(const item_t* item) {
	return (item->dataw[2] / 100) + (item->dataw[3] / 100) + (item->dataw[4] / 100) + (item->dataw[5] / 100);
}

uint16_t compute_mag_strength_flags(const item_t* item) {
	uint16_t pow = item->dataw[3] / 100;
	uint16_t dex = item->dataw[4] / 100;
	uint16_t mind = item->dataw[5] / 100;
	uint16_t ret = 0;

	if ((dex < pow) && (mind < pow)) {
		ret = 0x008;
	}
	if ((pow < dex) && (mind < dex)) {
		ret |= 0x010;
	}
	if ((dex < mind) && (pow < mind)) {
		ret |= 0x020;
	}

	uint16_t highest = ((dex > pow) ? dex : pow);
	highest = (highest > mind) ? highest : mind;

	if ((pow == highest) + (dex == highest) + (mind == highest) > 1) {
		ret |= 0x100;
	}

	return ret;
}

void update_stat(item_t* data, size_t which, int8_t delta) {
	uint16_t existing_stat = data->dataw[which] % 100;

	if ((delta > 0) || ((delta < 0) && (-delta < existing_stat))) {
		uint16_t level = compute_mag_level(data);
		if (level > 200) {
			ERR_LOG("mag level is too high\n");
			return;
		}
		if ((level == 200) && ((99 - existing_stat) < delta)) {
			delta = 99 - existing_stat;
		}
		data->dataw[which] += delta;
	}
}

ssize_t clamp(ssize_t value, ssize_t min, ssize_t max) {
	if (value < min) {
		return min;
	}
	else if (value > max) {
		return max;
	}
	return value;
}

void assign_mag_stats(item_t* item, magitemstat_t* mag) {
	item->datab[2] = (uint8_t)level(mag);
	item->datab[3] = mag->photon_blasts;
	item->dataw[2] = mag->def & 0x7FFE;
	item->dataw[3] = mag->pow & 0x7FFE;
	item->dataw[4] = mag->dex & 0x7FFE;
	item->dataw[5] = mag->mind & 0x7FFE;
	item->data2b[0] = (uint8_t)mag->synchro;
	item->data2b[1] = (uint8_t)mag->iq;
	item->data2b[2] = mag->flags;
	item->data2b[3] = mag->color;
}

void clear_mag_stats(item_t* item) {
	if (item->datab[0] == 2) {
		item->datab[1] = '\0';
		assign_mag_stats(item, &(magitemstat_t) { 0 });
	}
}

uint8_t mag_photon_blast_for_slot(const item_t* item, uint8_t slot) {
	uint8_t flags = item->data2b[2];
	uint8_t pb_nums = item->datab[3];

	if (slot == 0) { // Center
		return (flags & 1) ? (pb_nums & 0x07) : 0xFF;
	}
	else if (slot == 1) { // Right
		return (flags & 2) ? ((pb_nums & 0x38) >> 3) : 0xFF;
	}
	else if (slot == 2) { // Left
		if (!(flags & 4)) {
			return 0xFF;
		}

		uint8_t used_pbs[6] = { 0, 0, 0, 0, 0, 0 };
		used_pbs[pb_nums & 0x07] = '\x01';
		used_pbs[(pb_nums & 0x38) >> 3] = '\x01';
		uint8_t left_pb_num = (pb_nums & 0xC0) >> 6;

		for (uint8_t z = 0; z < 6; z++) {
			if (!used_pbs[z]) {
				if (!left_pb_num) {
					return z;
				}
				left_pb_num--;
			}
		}
		// Failed to find unused photon blast number
		ERR_LOG("Failed to find unused photon blast number.\n");
		printf("按任意键停止程序...\n");
		getchar();
		exit(1);
	}
	else {
		// Invalid slot index
		ERR_LOG("Invalid slot index.\n");
		printf("按任意键停止程序...\n");
		getchar();
		exit(1);
	}
}

int mag_has_photon_blast_in_any_slot(const item_t* item, uint8_t pb_num) {
	if (pb_num < 6) {
		for (uint8_t slot = 0; slot < 3; slot++) {
			if (mag_photon_blast_for_slot(item, slot) == pb_num) {
				return 1;
			}
		}
	}
	return 0;
}

void add_mag_photon_blast(item_t* item, uint8_t pb_num) {
	if (pb_num >= 6) {
		return;
	}
	if (mag_has_photon_blast_in_any_slot(item, pb_num)) {
		return;
	}

	uint8_t* flags = &(item->data2b[2]);
	uint8_t* pb_nums = &(item->datab[3]);

	if (!(*flags & 1)) { // Center
		*pb_nums |= pb_num;
		*flags |= 1;
	}
	else if (!(*flags & 2)) { // Right
		*pb_nums |= (pb_num << 3);
		*flags |= 2;
	}
	else if (!(*flags & 4)) { // Left
		uint8_t orig_pb_num = pb_num;
		if (mag_photon_blast_for_slot(item, 0) < orig_pb_num) {
			pb_num--;
		}
		if (mag_photon_blast_for_slot(item, 1) < orig_pb_num) {
			pb_num--;
		}
		if (pb_num >= 4) {
			// Left photon blast number is too high
			ERR_LOG("Left photon blast number is too high.\n");
			printf("按任意键停止程序...\n");
			getchar();
			exit(1);
		}
		*pb_nums |= (pb_num << 6);
		*flags |= 4;
	}
}

size_t get_mag_feed_table_index(iitem_t* mag) {

	switch (mag->data.datab[1])
	{
	case Mag_Mag:
		return 0;

	case Mag_Varuna:
	case Mag_Vritra:
	case Mag_Kalki:
		return 1;

	case Mag_Ashvinau:
	case Mag_Sumba:
	case Mag_Namuci:
	case Mag_Marutah:
	case Mag_Rudra:
		return 2;

	case Mag_Surya:
	case Mag_Tapas:
	case Mag_Mitra:
		return 3;

	case Mag_Apsaras:
	case Mag_Vayu:
	case Mag_Varaha:
	case Mag_Ushasu:
	case Mag_Kama:
	case Mag_Kaitabha:
	case Mag_Kumara:
	case Mag_Bhirava:
		return 4;

	case Mag_Ila:
	case Mag_Garuda:
	case Mag_Sita:
	case Mag_Soma:
	case Mag_Durga:
	case Mag_Nandin:
	case Mag_Yaksa:
	case Mag_Ribhava:
		return 5;

	case Mag_Andhaka:
	case Mag_Kabanda:
	case Mag_Naga:
	case Mag_Naraka:
	case Mag_Bana:
	case Mag_Marica:
	case Mag_Madhu:
	case Mag_Ravana:
		return 6;

	case Mag_Deva:
	case Mag_Rukmin:
	case Mag_Sato:
		return 5;

	case Mag_Rati:
	case Mag_Pushan:
	case Mag_Bhima:
		return 6;

	default:
		return 7;
	}
}

int get_evolution_number(iitem_t* mag) {

	switch (mag->data.datab[1])
	{
	case Mag_Mag:
		return 0;

	case Mag_Varuna:
	case Mag_Vritra:
	case Mag_Kalki:
		return 1;

	case Mag_Ashvinau:
	case Mag_Sumba:
	case Mag_Namuci:
	case Mag_Marutah:
	case Mag_Rudra:
		return 2;

	case Mag_Surya:
	case Mag_Tapas:
	case Mag_Mitra:
		return 2;

	case Mag_Apsaras:
	case Mag_Vayu:
	case Mag_Varaha:
	case Mag_Ushasu:
	case Mag_Kama:
	case Mag_Kaitabha:
	case Mag_Kumara:
	case Mag_Bhirava:
		return 3;

	case Mag_Ila:
	case Mag_Garuda:
	case Mag_Sita:
	case Mag_Soma:
	case Mag_Durga:
	case Mag_Nandin:
	case Mag_Yaksa:
	case Mag_Ribhava:
		return 3;

	case Mag_Andhaka:
	case Mag_Kabanda:
	case Mag_Naga:
	case Mag_Naraka:
	case Mag_Bana:
	case Mag_Marica:
	case Mag_Madhu:
	case Mag_Ravana:
		return 3;
	case Mag_Deva:
	case Mag_Rukmin:
	case Mag_Sato:
		return 4;

	case Mag_Rati:
	case Mag_Pushan:
	case Mag_Bhima:
		return 4;

	default:
		return 4;
	}
}

// 玛古喂养表
magfeedresultslist_t mag_feed_table[MAG_MAX_FEED_TABLE][11 * 6] = {

	{
		3, 3, 5, 40, 5, 0,
		3, 3, 10, 45, 5, 0,
		4, 4, 15, 50, 10, 0,
		3, 3, 5, 0, 5, 40,
		3, 3, 10, 0, 5, 45,
		4, 4, 15, 0, 10, 50,
		3, 3, 5, 10, 40, 0,
		3, 3, 5, 0, 44, 10,
		4, 1, 15, 30, 15, 25,
		4, 1, 15, 25, 15, 30,
		6, 5, 25, 25, 25, 25
	},

	{
		0, 0, 5, 10, 0, -1,
		2, 1, 6, 15, 3, -3,
		3, 2, 12, 21, 4, -7,
		0, 0, 5, 0, 0, 8,
		2, 1, 7, 0, 3, 13,
		3, 2, 7, -7, 6, 19,
		0, 1, 0, 5, 15, 0,
		2, 0, -1, 0, 14, 5,
		-2, 2, 10, 11, 8, 0,
		3, -2, 9, 0, 9, 11,
		4, 3, 14, 9, 18, 11
	},

	{
		0, -1, 1, 9, 0, -5,
		3, 0, 1, 13, 0, -10,
		4, 1, 8, 16, 2, -15,
		0, -1, 0, -5, 0, 9,
		3, 0, 4, -10, 0, 13,
		3, 2, 6, -15, 5, 17,
		-1, 1, -5, 4, 12, -5,
		0, 0, -5, -6, 11, 4,
		4, -2, 0, 11, 3, -5,
		-1, 1, 4, -5, 0, 11,
		4, 2, 7, 8, 6, 9
	},

	{
		0, -1, 0, 3, 0, 0,
		2, 0, 5, 7, 0, -5,
		3, 1, 4, 14, 6, -10,
		0, 0, 0, 0, 0, 4,
		0, 1, 4, -5, 0, 8,
		2, 2, 4, -10, 3, 15,
		-3, 3, 0, 0, 7, 0,
		3, 0, -4, -5, 20, -5,
		3, -2, -10, 9, 6, 9,
		-2, 2, 8, 5, -8, 7,
		3, 2, 7, 7, 7, 7
	},

	{
		2, -1, -5, 9, -5, 0,
		2, 0, 0, 11, 0, -10,
		0, 1, 4, 14, 0, -15,
		2, -1, -5, 0, -6, 10,
		2, 0, 0, -10, 0, 11,
		0, 1, 4, -15, 0, 15,
		2, -1, -5, -5, 16, -5,
		-2, 3, 7, -3, 0, -3,
		4, -2, 5, 21, -5, -20,
		3, 0, -5, -20, 5, 21,
		3, 2, 4, 6, 8, 5
	},

	{
		2, -1, -4, 13, -5, -5,
		0, 1, 0, 16, 0, -15,
		2, 0, 3, 19, -2, -18,
		2, -1, -4, -5, -5, 13,
		0, 1, 0, -15, 0, 16,
		2, 0, 3, -20, 0, 19,
		0, 1, 5, -6, 6, -5,
		-1, 1, 0, -4, 14, -10,
		4, -1, 4, 17, -5, -15,
		2, 0, -10, -15, 5, 21,
		3, 2, 2, 8, 3, 6
	},

	{
		-1, 1, -3, 9, -3, -4,
		2, 0, 0, 11, 0, -10,
		2, 0, 2, 15, 0, -16,
		-1, 1, -3, -4, -3, 9,
		2, 0, 0, -10, 0, 11,
		2, 0, -2, -15, 0, 19,
		2, -1, 0, 6, 9, -15,
		-2, 3, 0, -15, 9, 6,
		3, -1, 9, -20, -5, 17,
		0, 2, -5, 20, 5, -20,
		3, 2, 0, 11, 0, 11
	},

	{
		-1, 0, -4, 21, -15, -5, // Fixed the 2 incorrect bytes in table 7 (was cell table)
		0, 1, -1, 27, -10, -16,
		2, 0, 5, 29, -7, -25,
		-1, 0, -10, -5, -10, 21,
		0, 1, -5, -16, -5, 25,
		2, 0, -7, -25, 6, 29,
		-1, 1, -10, -10, 28, -10,
		2, -1, 9, -18, 24, -15,
		2, 1, 19, 18, -15, -20,
		2, 1, -15, -20, 19, 18,
		4, 2, 3, 7, 3, 3
	}
};

magfeedresult_t get_mag_feed_result(size_t table_index, size_t item_index) {
	if (table_index >= 8) {
		ERR_LOG("invalid mag feed table index");
		printf("按任意键停止程序...\n");
		getchar();
		exit(1);
	}
	if (item_index >= 11) {
		ERR_LOG("invalid mag feed item index");
		printf("按任意键停止程序...\n");
		getchar();
		exit(1);
	}

	magfeedresultslist_t* result = mag_feed_table[table_index];
	return result->results[item_index];
}

#define MALE 0x01
#define HUMAN 0x02
#define NEWMAN 0x04
#define ANDROID 0x08
#define HUNTER 0x10
#define RANGER 0x20
#define FORCE 0x40

static uint8_t class_flags[12] = {
	HUNTER | HUMAN | MALE,         // HUmar
	HUNTER | NEWMAN,               // HUnewearl
	HUNTER | ANDROID | MALE,       // HUcast
	RANGER | HUMAN | MALE,         // RAmar
	RANGER | ANDROID | MALE,       // RAcast
	RANGER | ANDROID,              // RAcaseal
	FORCE | HUMAN,                 // FOmarl
	FORCE | NEWMAN | MALE,         // FOnewm
	FORCE | NEWMAN,                // FOnewearl
	HUNTER | ANDROID,              // HUcaseal
	FORCE | HUMAN | MALE,          // FOmar
	RANGER | HUMAN                 // RAmarl
};

bool char_class_is_male(uint8_t cls) {
	return class_flags[cls] & MALE;
}

bool char_class_is_human(uint8_t cls) {
	return class_flags[cls] & HUMAN;
}

bool char_class_is_newman(uint8_t cls) {
	return class_flags[cls] & NEWMAN;
}

bool char_class_is_android(uint8_t cls) {
	return class_flags[cls] & ANDROID;
}

bool char_class_is_hunter(uint8_t cls) {
	return class_flags[cls] & HUNTER;
}

bool char_class_is_ranger(uint8_t cls) {
	return class_flags[cls] & RANGER;
}

bool char_class_is_force(uint8_t cls) {
	return class_flags[cls] & FORCE;
}

void player_feed_mag(ship_client_t* src, size_t mag_item_index, size_t fed_item_index) {
	//if (src->version != CLIENT_VERSION_BB)
	//	return send_txt(src, __(src, "\tE\tC4不支持非BB版本"));

	psocn_bb_char_t* character = { 0 };

	if (src->mode)
		character = &src->mode_pl->bb;
	else
		character = &src->bb_pl->character;

	iitem_t fed_item = character->inv.iitems[fed_item_index];
	iitem_t mag_item = character->inv.iitems[mag_item_index];

	size_t result_index = find_result_index(primary_identifier(&fed_item.data));
	size_t feed_table_index = get_mag_feed_table_index(&mag_item);
	magfeedresult_t feed_result = get_mag_feed_result(feed_table_index, result_index);

	update_stat(&mag_item.data, 2, feed_result.def);
	update_stat(&mag_item.data, 3, feed_result.pow);
	update_stat(&mag_item.data, 4, feed_result.dex);
	update_stat(&mag_item.data, 5, feed_result.mind);
	mag_item.data.data2b[0] = (uint8_t)clamp((ssize_t)mag_item.data.data2b[0] + feed_result.synchro, 0, 120);
	mag_item.data.data2b[1] = (uint8_t)clamp((ssize_t)mag_item.data.data2b[1] + feed_result.iq, 0, 200);

	uint8_t mag_level = (uint8_t)compute_mag_level(&mag_item.data);
	mag_item.data.datab[2] = mag_level;
	uint8_t evolution_number = get_evolution_number(&mag_item);
	uint8_t mag_number = mag_item.data.datab[1];

	// Note: Sega really did just hardcode all these rules into the client. There
	// is no data file describing these evolutions, unfortunately.

	if (mag_level < 10) {
		// Nothing to do

	}
	else if (mag_level < 35) { // Level 10 evolution
		if (evolution_number < 1) {
			switch (character->dress_data.ch_class) {
			case 0: // HUmar
			case 1: // HUnewearl
			case 2: // HUcast
			case 9: // HUcaseal
				mag_item.data.datab[1] = 0x01; // Varuna
				break;
			case 3: // RAmar
			case 11: // RAmarl
			case 4: // RAcast
			case 5: // RAcaseal
				mag_item.data.datab[1] = 0x0D; // Kalki
				break;
			case 10: // FOmar
			case 6: // FOmarl
			case 7: // FOnewm
			case 8: // FOnewearl
				mag_item.data.datab[1] = 0x19; // Vritra
				break;
			default:
				ERR_LOG("invalid character class");
			}
		}

	}
	else if (mag_level < 50) { // Level 35 evolution
		if (evolution_number < 2) {
			uint16_t flags = compute_mag_strength_flags(&mag_item.data);
			if (mag_number == 0x0D) {
				if ((flags & 0x110) == 0) {
					mag_item.data.datab[1] = 0x02;
				}
				else if (flags & 8) {
					mag_item.data.datab[1] = 0x03;
				}
				else if (flags & 0x20) {
					mag_item.data.datab[1] = 0x0B;
				}
			}
			else if (mag_number == 1) {
				if (flags & 0x108) {
					mag_item.data.datab[1] = 0x0E;
				}
				else if (flags & 0x10) {
					mag_item.data.datab[1] = 0x0F;
				}
				else if (flags & 0x20) {
					mag_item.data.datab[1] = 0x04;
				}
			}
			else if (mag_number == 0x19) {
				if (flags & 0x120) {
					mag_item.data.datab[1] = 0x1A;
				}
				else if (flags & 8) {
					mag_item.data.datab[1] = 0x1B;
				}
				else if (flags & 0x10) {
					mag_item.data.datab[1] = 0x14;
				}
			}
		}

	}
	else if ((mag_level % 5) == 0) { // Level 50 (and beyond) evolutions
		if (evolution_number < 4) {

			if (mag_level >= 100) {
				uint8_t section_id_group = character->dress_data.section % 3;
				uint16_t def = mag_item.data.dataw[2] / 100;
				uint16_t pow = mag_item.data.dataw[3] / 100;
				uint16_t dex = mag_item.data.dataw[4] / 100;
				uint16_t mind = mag_item.data.dataw[5] / 100;
				bool is_male = char_class_is_male(character->dress_data.ch_class);
				size_t table_index = (is_male ? 0 : 1) + section_id_group * 2;

				bool is_hunter = char_class_is_hunter(character->dress_data.ch_class);
				bool is_ranger = char_class_is_ranger(character->dress_data.ch_class);
				bool is_force = char_class_is_force(character->dress_data.ch_class);
				if (is_force) {
					table_index += 12;
				}
				else if (is_ranger) {
					table_index += 6;
				}
				else if (!is_hunter) {
					ERR_LOG("char class is not any of the top-level classes");
				}

				// Note: The original code checks the class (hunter/ranger/force) again
				// here, and goes into 3 branches that each do these same checks.
				// However, the result of all 3 branches is exactly the same!
				if (((section_id_group == 0) && (pow + mind == def + dex)) ||
					((section_id_group == 1) && (pow + dex == mind + def)) ||
					((section_id_group == 2) && (pow + def == mind + dex))) {
					// clang-format off
					static const uint8_t result_table[] = {
						//   M0    F0    M1    F1    M2    F2
							0x39, 0x3B, 0x3A, 0x3B, 0x3A, 0x3B, // Hunter
							0x3D, 0x3C, 0x3D, 0x3C, 0x3D, 0x3E, // Ranger
							0x41, 0x3F, 0x41, 0x40, 0x41, 0x40, // Force
					};
					// clang-format on
					mag_item.data.datab[1] = result_table[table_index];
				}
			}

			// If a special evolution did not occur, do a normal level 50 evolution
			if (mag_number == mag_item.data.datab[1]) {
				uint16_t flags = compute_mag_strength_flags(&mag_item.data);
				uint16_t def = mag_item.data.dataw[2] / 100;
				uint16_t pow = mag_item.data.dataw[3] / 100;
				uint16_t dex = mag_item.data.dataw[4] / 100;
				uint16_t mind = mag_item.data.dataw[5] / 100;

				bool is_hunter = char_class_is_hunter(character->dress_data.ch_class);
				bool is_ranger = char_class_is_ranger(character->dress_data.ch_class);
				bool is_force = char_class_is_force(character->dress_data.ch_class);
				if (is_hunter + is_ranger + is_force != 1) {
					ERR_LOG("char class is not exactly one of the top-level classes");
				}

				if (is_hunter) {
					if (flags & 0x108) {
						mag_item.data.datab[1] = (character->dress_data.section & 1)
							? ((dex < mind) ? 0x08 : 0x06)
							: ((dex < mind) ? 0x0C : 0x05);
					}
					else if (flags & 0x010) {
						mag_item.data.datab[1] = (character->dress_data.section & 1)
							? ((mind < pow) ? 0x12 : 0x10)
							: ((mind < pow) ? 0x17 : 0x13);
					}
					else if (flags & 0x020) {
						mag_item.data.datab[1] = (character->dress_data.section & 1)
							? ((pow < dex) ? 0x16 : 0x24)
							: ((pow < dex) ? 0x07 : 0x1E);
					}
				}
				else if (is_ranger) {
					if (flags & 0x110) {
						mag_item.data.datab[1] = (character->dress_data.section & 1)
							? ((mind < pow) ? 0x0A : 0x05)
							: ((mind < pow) ? 0x0C : 0x06);
					}
					else if (flags & 0x008) {
						mag_item.data.datab[1] = (character->dress_data.section & 1)
							? ((dex < mind) ? 0x0A : 0x26)
							: ((dex < mind) ? 0x0C : 0x06);
					}
					else if (flags & 0x020) {
						mag_item.data.datab[1] = (character->dress_data.section & 1)
							? ((pow < dex) ? 0x18 : 0x1E)
							: ((pow < dex) ? 0x08 : 0x05);
					}
				}
				else if (is_force) {
					if (flags & 0x120) {
						if (def < 45) {
							mag_item.data.datab[1] = (character->dress_data.section & 1)
								? ((pow < dex) ? 0x17 : 0x09)
								: ((pow < dex) ? 0x1E : 0x1C);
						}
						else {
							mag_item.data.datab[1] = 0x24;
						}
					}
					else if (flags & 0x008) {
						if (def < 45) {
							mag_item.data.datab[1] = (character->dress_data.section & 1)
								? ((dex < mind) ? 0x1C : 0x20)
								: ((dex < mind) ? 0x1F : 0x25);
						}
						else {
							mag_item.data.datab[1] = 0x23;
						}
					}
					else if (flags & 0x010) {
						if (def < 45) {
							mag_item.data.datab[1] = (character->dress_data.section & 1)
								? ((mind < pow) ? 0x12 : 0x0C)
								: ((mind < pow) ? 0x15 : 0x11);
						}
						else {
							mag_item.data.datab[1] = 0x24;
						}
					}
				}
			}
		}
	}

	// If the mag has evolved, add its new photon blast
	// TODO 未完成PMT的完全解析
	//if (mag_number != mag_item.data.data_b[1]) {
	//	const auto& new_mag_def = s->item_parameter_table->get_mag(mag_item.data.data_b[1]);
	//	add_mag_photon_blast(&mag_item.data, new_mag_def.photon_blast);
	//}
}

/* MAG 增加光子爆发参数*/
void mag_bb_add_pb(uint8_t* flags, uint8_t* blasts, uint8_t pb) {
	int32_t pb_exists = 0;
	uint8_t pbv;
	uint32_t pb_slot;

	if ((*flags & 0x01) == 0x01)
	{
		if ((*blasts & 0x07) == pb)
			pb_exists = 1;
	}

	if ((*flags & 0x02) == 0x02)
	{
		if (((*blasts / 8) & 0x07) == pb)
			pb_exists = 1;
	}

	if ((*flags & 0x04) == 0x04)
		pb_exists = 1;

	if (!pb_exists)
	{
		if ((*flags & 0x01) == 0)
			pb_slot = 0;
		else
			if ((*flags & 0x02) == 0)
				pb_slot = 1;
			else
				pb_slot = 2;
		switch (pb_slot)
		{
		case 0x00:
			*blasts &= 0xF8;
			*flags |= 0x01;
			break;
		case 0x01:
			pb *= 8;
			*blasts &= 0xC7;
			*flags |= 0x02;
			break;
		case 0x02:
			pbv = pb;
			if ((*blasts & 0x07) < pb)
				pbv--;
			if (((*blasts / 8) & 0x07) < pb)
				pbv--;
			pb = pbv * 0x40;
			*blasts &= 0x3F;
			*flags |= 0x04;
		}
		*blasts |= pb;
	}
}

/* MAG 增加变量参数*/
int32_t mag_bb_alignment(magitem_t* m) {
	int32_t v1, v2, v3, v4, v5, v6;

	v4 = 0;
	v3 = m->pow;
	v2 = m->dex;
	v1 = m->mind;
	if (v2 < v3)
	{
		if (v1 < v3)
			v4 = 8;
	}
	if (v3 < v2)
	{
		if (v1 < v2)
			v4 |= 0x10u;
	}
	if (v2 < v1)
	{
		if (v3 < v1)
			v4 |= 0x20u;
	}
	v6 = 0;
	v5 = v3;
	if (v3 <= v2)
		v5 = v2;
	if (v5 <= v1)
		v5 = v1;
	if (v5 == v3)
		v6 = 1;
	if (v5 == v2)
		++v6;
	if (v5 == v1)
		++v6;
	if (v6 >= 2)
		v4 |= 0x100u;
	return v4;
}

/* MAG 特殊进化函数 */
int32_t mag_bb_special_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	uint8_t oldType;
	int16_t mDefense, mPower, mDex, mMind;

	oldType = m->mtype;

	if (m->level >= 100)
	{
		mDefense = m->def / 100;
		mPower = m->pow / 100;
		mDex = m->dex / 100;
		mMind = m->mind / 100;

		switch (section_id)
		{
		case ID_Viridia:
		case ID_Bluefull:
		case ID_Redria:
		case ID_Whitill:
			if ((mDefense + mDex) == (mPower + mMind))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Deva;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Sato;
					break;
				default:
					break;
				}
			}
			break;
		case ID_Skyly:
		case ID_Pinkal:
		case ID_Yellowboze:
			if ((mDefense + mPower) == (mDex + mMind))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Dewari;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		case ID_Greennill:
		case ID_Oran:
		case ID_Purplenum:
			if ((mDefense + mMind) == (mPower + mDex))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		}
	}
	return (int32_t)(oldType != m->mtype);
}

/* MAG 50级进化函数 */
void mag_bb_lv50_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	int32_t v10, v11, v12, v13;

	int32_t Alignment = mag_bb_alignment(m);

	if (evolution_class > 3) // Don't bother to check if a special mag.
		return;

	v10 = m->pow / 100;
	v11 = m->dex / 100;
	v12 = m->mind / 100;
	v13 = m->def / 100;

	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108)
		{
			if (section_id & 1)
			{
				if (v12 > v11)
				{
					m->mtype = Mag_Apsaras;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
				}
				else
				{
					m->mtype = Mag_Kama;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				}
			}
			else
			{
				if (v12 > v11)
				{
					m->mtype = Mag_Bhirava;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
				}
			}
		}
		else
		{
			if (Alignment & 0x10)
			{
				if (section_id & 1)
				{
					if (v10 > v12)
					{
						m->mtype = Mag_Garuda;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Yaksa;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
					}
				}
				else
				{
					if (v10 > v12)
					{
						m->mtype = Mag_Ila;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Nandin;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
					}
				}
			}
			else
			{
				if (Alignment & 0x20)
				{
					if (section_id & 1)
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Soma;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Bana;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Ushasu;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
						}
					}
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if (Alignment & 0x110)
		{
			if (section_id & 1)
			{
				if (v10 > v12)
				{
					m->mtype = Mag_Kaitabha;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
				}
			}
			else
			{
				if (v10 > v12)
				{
					m->mtype = Mag_Bhirava;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Kama;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				}
			}
		}
		else
		{
			if (Alignment & 0x08)
			{
				if (section_id & 1)
				{
					if (v12 > v11)
					{
						m->mtype = Mag_Kaitabha;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Madhu;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
				}
				else
				{
					if (v12 > v11)
					{
						m->mtype = Mag_Bhirava;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Kama;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
					}
				}
			}
			else
			{
				if (Alignment & 0x20)
				{
					if (section_id & 1)
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Durga;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Apsaras;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Varaha;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
						}
					}
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if (Alignment & 0x120)
		{
			if (v13 > 44)
			{
				m->mtype = Mag_Bana;
				mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
			}
			else
			{
				if (section_id & 1)
				{
					if (v11 > v10)
					{
						m->mtype = Mag_Ila;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Kumara;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
					}
				}
				else
				{
					if (v11 > v10)
					{
						m->mtype = Mag_Kabanda;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Naga;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
				}
			}
		}
		else
		{
			if (Alignment & 0x08)
			{
				if (v13 > 44)
				{
					m->mtype = Mag_Andhaka;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
				}
				else
				{
					if (section_id & 1)
					{
						if (v12 > v11)
						{
							m->mtype = Mag_Naga;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
						}
						else
						{
							m->mtype = Mag_Marica;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
						}
					}
					else
					{
						if (v12 > v11)
						{
							m->mtype = Mag_Ravana;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Farlla);
						}
						else
						{
							m->mtype = Mag_Naraka;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
						}
					}
				}
			}
			else
			{
				if (Alignment & 0x10)
				{
					if (v13 > 44)
					{
						m->mtype = Mag_Bana;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
					}
					else
					{
						if (section_id & 1)
						{
							if (v10 > v12)
							{
								m->mtype = Mag_Garuda;
								mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
							}
							else
							{
								m->mtype = Mag_Bhirava;
								mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
							}
						}
						else
						{
							if (v10 > v12)
							{
								m->mtype = Mag_Ribhava;
								mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Farlla);
							}
							else
							{
								m->mtype = Mag_Sita;
								mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
							}
						}
					}
				}
			}
		}
		break;
	}
}

/* MAG 35级进化函数 */
void mag_bb_lv35_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	int32_t Alignment = mag_bb_alignment(m);

	if (evolution_class > 3) // Don't bother to check if a special mag.
		return;

	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108) {
			m->mtype = Mag_Rudra;
			mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
			return;
		}
		else {
			if (Alignment & 0x10) {
				m->mtype = Mag_Marutah;
				mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				return;
			}
			else {
				if (Alignment & 0x20) {
					m->mtype = Mag_Vayu;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if (Alignment & 0x110) {
			m->mtype = Mag_Mitra;
			mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
			return;
		}
		else {
			if (Alignment & 0x08) {
				m->mtype = Mag_Surya;
				mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
				return;
			}
			else {
				if (Alignment & 0x20) {
					m->mtype = Mag_Tapas;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if (Alignment & 0x120) {
			m->mtype = Mag_Namuci;
			mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
			return;
		}
		else {
			if (Alignment & 0x08) {
				m->mtype = Mag_Sumba;
				mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
				return;
			}
			else {
				if (Alignment & 0x10) {
					m->mtype = Mag_Ashvinau;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
					return;
				}
			}
		}
		break;
	}
}

/* MAG 10级进化函数 */
void mag_bb_lv10_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		m->mtype = Mag_Varuna;
		mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Farlla);
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		m->mtype = Mag_Kalki;
		mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		m->mtype = Mag_Vritra;
		mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Leilla);
		break;
	}
}

/* MAG 进化检测函数 */
void mag_bb_check_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	if ((m->level < 10) || (m->level >= 35)) {
		if ((m->level < 35) || (m->level >= 50)) {
			if (m->level >= 50) {
				// Divisible by 5 with no remainder.
				if (!(m->level % 5)) {
					if (evolution_class <= 3) {
						if (!mag_bb_special_evolution(m, section_id, type, evolution_class))
							mag_bb_lv50_evolution(m, section_id, type, evolution_class);
					}
				}
			}
		}
		else {
			if (evolution_class < 2)
				mag_bb_lv35_evolution(m, section_id, type, evolution_class);
		}
	}
	else {
		if (evolution_class <= 0)
			mag_bb_lv10_evolution(m, section_id, type, evolution_class);
	}
}

/* MAG 喂养函数 */
int mag_bb_feed(ship_client_t* src, uint32_t mag_item_id, uint32_t fed_item_id) {
	lobby_t* l = src->cur_lobby;

	if (!l || l->type != LOBBY_TYPE_GAME)
		return -1;

	uint32_t i, mt_index = 0;
	int32_t evolution_class = 0;
	magitem_t* mag = { 0 };
	item_t* feed_item = { 0 };
	uint16_t* ft;
	int16_t mag_iq, mag_def, mag_pow, mag_dex, mag_mind;

	psocn_bb_char_t* character = &src->bb_pl->character;

	if (src->mode)
		character = &src->mode_pl->bb;

	if (fed_item_id != EMPTY_STRING) {
		i = find_iitem_index(&character->inv, fed_item_id);

		if (i == -1) {
			ERR_LOG("GC %" PRIu32 "无法找到需要喂养的物品ID %u",
				src->guildcard, fed_item_id);
			return -2;
		}

		feed_item = &character->inv.iitems[i].data;

		i = find_equipped_mag(&character->inv);

		if (i == -1) {
			ERR_LOG("GC %" PRIu32 "无法找到需要喂养的玛古ID %u",
				src->guildcard, mag_item_id);
			return -3;
		}

		mag = (magitem_t*)&character->inv.iitems[i].data;

		if ((feed_item->datab[0] == ITEM_TYPE_TOOL) &&
			(feed_item->datab[1] < ITEM_SUBTYPE_TELEPIPE) &&
			(feed_item->datab[1] != ITEM_SUBTYPE_DISK) &&
			(feed_item->datab[5] > 0x00))
		{
			switch (feed_item->datab[1])
			{
			case ITEM_SUBTYPE_MATE:
				mt_index = feed_item->datab[2];
				break;
			case ITEM_SUBTYPE_FLUID:
				mt_index = 3 + feed_item->datab[2];
				break;
			case ITEM_SUBTYPE_SOL_ATOMIZER:
			case ITEM_SUBTYPE_MOON_ATOMIZER:
			case ITEM_SUBTYPE_STAR_ATOMIZER:
				mt_index = 5 + feed_item->datab[1];
				break;
			case ITEM_SUBTYPE_ANTI:
				mt_index = 6 + feed_item->datab[2];
				break;
			}
		}

		remove_iitem(src, fed_item_id, 1, false);

		// 重新扫描以更新磁指针（如果由于清理而更改） 
		i = find_equipped_mag(&character->inv);

		if (i == -1) {
			ERR_LOG("GC %" PRIu32 "无法找到需要喂养的玛古ID %u",
				src->guildcard, mag_item_id);
			return -4;
		}

		mag = (magitem_t*)&character->inv.iitems[i].data;

		// Feed that mag (Updates to code by Lee from schtserv.com)
		switch (mag->mtype)
		{
		case Mag_Mag:
			ft = &mag_feed_t[0][0];
			evolution_class = 0;
			break;
		case Mag_Varuna:
		case Mag_Vritra:
		case Mag_Kalki:
			evolution_class = 1;
			ft = &mag_feed_t[1][0];
			break;
		case Mag_Ashvinau:
		case Mag_Sumba:
		case Mag_Namuci:
		case Mag_Marutah:
		case Mag_Rudra:
			ft = &mag_feed_t[2][0];
			evolution_class = 2;
			break;
		case Mag_Surya:
		case Mag_Tapas:
		case Mag_Mitra:
			ft = &mag_feed_t[3][0];
			evolution_class = 2;
			break;
		case Mag_Apsaras:
		case Mag_Vayu:
		case Mag_Varaha:
		case Mag_Ushasu:
		case Mag_Kama:
		case Mag_Kaitabha:
		case Mag_Kumara:
		case Mag_Bhirava:
			evolution_class = 3;
			ft = &mag_feed_t[4][0];
			break;
		case Mag_Ila:
		case Mag_Garuda:
		case Mag_Sita:
		case Mag_Soma:
		case Mag_Durga:
		case Mag_Nandin:
		case Mag_Yaksa:
		case Mag_Ribhava:
			evolution_class = 3;
			ft = &mag_feed_t[5][0];
			break;
		case Mag_Andhaka:
		case Mag_Kabanda:
		case Mag_Naga:
		case Mag_Naraka:
		case Mag_Bana:
		case Mag_Marica:
		case Mag_Madhu:
		case Mag_Ravana:
			evolution_class = 3;
			ft = &mag_feed_t[6][0];
			break;
		case Mag_Deva:
		case Mag_Rukmin:
		case Mag_Sato:
			ft = &mag_feed_t[5][0];
			evolution_class = 4;
			break;
		case Mag_Rati:
		case Mag_Pushan:
		case Mag_Bhima:
			ft = &mag_feed_t[6][0];
			evolution_class = 4;
			break;
		default:
			ft = &mag_feed_t[7][0];
			evolution_class = 4;
			break;
		}
		mt_index *= 6;
		mag->synchro += ft[mt_index];
		if (mag->synchro < 0)
			mag->synchro = 0;
		if (mag->synchro > 120)
			mag->synchro = 120;
		mag_iq = mag->IQ;

		mag_iq += ft[mt_index + 1];
		if (mag_iq < 0)
			mag_iq = 0;
		if (mag_iq > 200)
			mag_iq = 200;
		mag->IQ = (uint8_t)mag_iq;

		// Add Defense

		mag_def = mag->def % 100;
		mag_def += ft[mt_index + 2];

		if (mag_def < 0)
			mag_def = 0;

		if (mag_def >= 100) {
			if (mag->level == MAX_PLAYER_LEVEL)
				mag_def = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->def = ((mag->def / 100) * 100) + mag_def;
			mag_bb_check_evolution(mag, character->dress_data.section, character->dress_data.ch_class, evolution_class);
		}
		else
			mag->def = ((mag->def / 100) * 100) + mag_def;

		// Add Power

		mag_pow = mag->pow % 100;
		mag_pow += ft[mt_index + 3];

		if (mag_pow < 0)
			mag_pow = 0;

		if (mag_pow >= 100) {
			if (mag->level == MAX_PLAYER_LEVEL)
				mag_pow = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->pow = ((mag->pow / 100) * 100) + mag_pow;
			mag_bb_check_evolution(mag, character->dress_data.section, character->dress_data.ch_class, evolution_class);
		}
		else
			mag->pow = ((mag->pow / 100) * 100) + mag_pow;

		// Add Dex

		mag_dex = mag->dex % 100;
		mag_dex += ft[mt_index + 4];

		if (mag_dex < 0)
			mag_dex = 0;

		if (mag_dex >= 100)
		{
			if (mag->level == MAX_PLAYER_LEVEL)
				mag_dex = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->dex = ((mag->dex / 100) * 100) + mag_dex;
			mag_bb_check_evolution(mag, character->dress_data.section, character->dress_data.ch_class, evolution_class);
		}
		else
			mag->dex = ((mag->dex / 100) * 100) + mag_dex;

		// Add Mind

		mag_mind = mag->mind % 100;
		mag_mind += ft[mt_index + 5];

		if (mag_mind < 0)
			mag_mind = 0;

		if (mag_mind >= 100)
		{
			if (mag->level == MAX_PLAYER_LEVEL)
				mag_mind = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->mind = ((mag->mind / 100) * 100) + mag_mind;
			mag_bb_check_evolution(mag, character->dress_data.section, character->dress_data.ch_class, evolution_class);
		}
		else
			mag->mind = ((mag->mind / 100) * 100) + mag_mind;
	}

	return 0;
}
