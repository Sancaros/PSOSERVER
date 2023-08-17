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
#include "handle_player_items.h"
#include "pmtdata.h"
#include "mageditdata.h"

typedef struct {
	uint32_t item_type;
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
size_t find_result_index(uint32_t item_type) {
	size_t i;
	for (i = 0; i < num_results; ++i) {
		if (result_index_for_fed_item[i].item_type == item_type) {
			return result_index_for_fed_item[i].result_index;
		}
	}
	return 0;  // Default result index if item_type is not found
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

	uint16_t highest = MAX(dex, MAX(pow, mind));

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
			ERR_LOG("玛古等级已超上限值");
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
		ERR_LOG("Failed to find unused photon blast number.");
		return -1;
	}
	else {
		// Invalid slot index
		ERR_LOG("Invalid slot index.");
		return -2;
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

int add_mag_photon_blast(item_t* item, uint8_t pb_num) {

	if (pb_num >= 6) {
		return -1;
	}

	if (mag_has_photon_blast_in_any_slot(item, pb_num)) {
		return -2;
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
			ERR_LOG("Left photon blast number is too high.");
			return -3;
		}
		*pb_nums |= (pb_num << 6);
		*flags |= 4;
	}

	return 0;
}

int player_feed_mag(ship_client_t* src, size_t mag_item_id, size_t feed_item_id) {
	errno_t err = 0;
	bool should_delete_item = (src->version != CLIENT_VERSION_DCV2) && (src->version != CLIENT_VERSION_PC);

	psocn_bb_char_t* character = get_client_char_bb(src);

	iitem_t* mag_item = &character->inv.iitems[find_iitem_index(&character->inv, mag_item_id)];
	iitem_t* fed_item = &character->inv.iitems[find_iitem_index(&character->inv, feed_item_id)];
	size_t result_index = find_result_index(primary_identifier(&fed_item->data));
	pmt_mag_bb_t mag_table = { 0 };
	if ((err = pmt_lookup_mag_bb(mag_item->data.datal[0], &mag_table))) {
		ERR_LOG("GC %" PRIu32 " 喂养了不存在的玛古数据!错误码 %d",
			src->guildcard, err);
		return err;
	}
	pmt_mag_feed_result_t feed_result = { 0 };
	if ((err = pmt_lookup_mag_feed_table_bb(mag_item->data.datal[0], mag_table.feed_table, result_index, &feed_result))) {
		ERR_LOG("GC %" PRIu32 " 喂养了不存在的玛古数据!错误码 %d",
			src->guildcard, err);
		return err;
	}

	update_stat(&mag_item->data, 2, feed_result.def);
	update_stat(&mag_item->data, 3, feed_result.pow);
	update_stat(&mag_item->data, 4, feed_result.dex);
	update_stat(&mag_item->data, 5, feed_result.mind);
	mag_item->data.data2b[0] = (uint8_t)clamp((ssize_t)mag_item->data.data2b[0] + feed_result.synchro, 0, 120);
	mag_item->data.data2b[1] = (uint8_t)clamp((ssize_t)mag_item->data.data2b[1] + feed_result.iq, 0, 200);

	uint8_t mag_level = (uint8_t)compute_mag_level(&mag_item->data);
	mag_item->data.datab[2] = mag_level;
	uint8_t evolution_number = magedit_lookup_mag_evolution_number(mag_item);
	uint8_t mag_number = mag_item->data.datab[1];
	// Note: Sega really did just hardcode all these rules into the client. There
	// is no data file describing these evolutions, unfortunately.
	if (mag_level < 10) {
		// Nothing to do

	}
	else if (mag_level < 35) { // Level 10 evolution
		if (evolution_number < 1) {
			switch (character->dress_data.ch_class) {
			case CLASS_HUMAR: // HUmar
			case CLASS_HUNEWEARL: // HUnewearl
			case CLASS_HUCAST: // HUcast
			case CLASS_HUCASEAL: // HUcaseal
				mag_item->data.datab[1] = 0x01; // Varuna
				break;
			case CLASS_RAMAR: // RAmar
			case CLASS_RAMARL: // RAmarl
			case CLASS_RACAST: // RAcast
			case CLASS_RACASEAL: // RAcaseal
				mag_item->data.datab[1] = 0x0D; // Kalki
				break;
			case CLASS_FOMAR: // FOmar
			case CLASS_FOMARL: // FOmarl
			case CLASS_FONEWM: // FOnewm
			case CLASS_FONEWEARL: // FOnewearl
				mag_item->data.datab[1] = 0x19; // Vritra
				break;
			default:
				ERR_LOG("无效角色职业");
				err = -2;
				return err;
			}
		}
	}
	else if (mag_level < 50) { // Level 35 evolution
		if (evolution_number < 2) {
			uint16_t flags = compute_mag_strength_flags(&mag_item->data);
			if (mag_number == 0x0D) {
				if ((flags & 0x110) == 0) {
					mag_item->data.datab[1] = 0x02;
				} else if (flags & 8) {
					mag_item->data.datab[1] = 0x03;
				} else if (flags & 0x20) {
					mag_item->data.datab[1] = 0x0B;
				}
			}
			else if (mag_number == 1) {
				if (flags & 0x108) {
					mag_item->data.datab[1] = 0x0E;
				} else if (flags & 0x10) {
					mag_item->data.datab[1] = 0x0F;
				} else if (flags & 0x20) {
					mag_item->data.datab[1] = 0x04;
				}
			}
			else if (mag_number == 0x19) {
				if (flags & 0x120) {
					mag_item->data.datab[1] = 0x1A;
				} else if (flags & 8) {
					mag_item->data.datab[1] = 0x1B;
				} else if (flags & 0x10) {
					mag_item->data.datab[1] = 0x14;
				}
			}
		}
	}
	else if ((mag_level % 5) == 0) { // Level 50 (and beyond) evolutions
		if (evolution_number < 4) {

			if (mag_level >= 100) {
				uint8_t section_id_group = character->dress_data.section % 3;
				uint16_t def = mag_item->data.dataw[2] / 100;
				uint16_t pow = mag_item->data.dataw[3] / 100;
				uint16_t dex = mag_item->data.dataw[4] / 100;
				uint16_t mind = mag_item->data.dataw[5] / 100;
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
					err = -3;
					return err;
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
					mag_item->data.datab[1] = result_table[table_index];
				}
			}

			// If a special evolution did not occur, do a normal level 50 evolution
			if (mag_number == mag_item->data.datab[1]) {
				uint16_t flags = compute_mag_strength_flags(&mag_item->data);
				uint16_t def = mag_item->data.dataw[2] / 100;
				uint16_t pow = mag_item->data.dataw[3] / 100;
				uint16_t dex = mag_item->data.dataw[4] / 100;
				uint16_t mind = mag_item->data.dataw[5] / 100;

				bool is_hunter = char_class_is_hunter(character->dress_data.ch_class);
				bool is_ranger = char_class_is_ranger(character->dress_data.ch_class);
				bool is_force = char_class_is_force(character->dress_data.ch_class);
				if (is_hunter + is_ranger + is_force != 1) {
					ERR_LOG("char class is not exactly one of the top-level classes");
					err = -4;
					return err;
				}

				if (is_hunter) {
					if (flags & 0x108) {
						mag_item->data.datab[1] = (character->dress_data.section & 1)
							? ((dex < mind) ? 0x08 : 0x06)
							: ((dex < mind) ? 0x0C : 0x05);
					}
					else if (flags & 0x010) {
						mag_item->data.datab[1] = (character->dress_data.section & 1)
							? ((mind < pow) ? 0x12 : 0x10)
							: ((mind < pow) ? 0x17 : 0x13);
					}
					else if (flags & 0x020) {
						mag_item->data.datab[1] = (character->dress_data.section & 1)
							? ((pow < dex) ? 0x16 : 0x24)
							: ((pow < dex) ? 0x07 : 0x1E);
					}
				}
				else if (is_ranger) {
					if (flags & 0x110) {
						mag_item->data.datab[1] = (character->dress_data.section & 1)
							? ((mind < pow) ? 0x0A : 0x05)
							: ((mind < pow) ? 0x0C : 0x06);
					}
					else if (flags & 0x008) {
						mag_item->data.datab[1] = (character->dress_data.section & 1)
							? ((dex < mind) ? 0x0A : 0x26)
							: ((dex < mind) ? 0x0C : 0x06);
					}
					else if (flags & 0x020) {
						mag_item->data.datab[1] = (character->dress_data.section & 1)
							? ((pow < dex) ? 0x18 : 0x1E)
							: ((pow < dex) ? 0x08 : 0x05);
					}
				}
				else if (is_force) {
					if (flags & 0x120) {
						if (def < 45) {
							mag_item->data.datab[1] = (character->dress_data.section & 1)
								? ((pow < dex) ? 0x17 : 0x09)
								: ((pow < dex) ? 0x1E : 0x1C);
						}
						else {
							mag_item->data.datab[1] = 0x24;
						}
					}
					else if (flags & 0x008) {
						if (def < 45) {
							mag_item->data.datab[1] = (character->dress_data.section & 1)
								? ((dex < mind) ? 0x1C : 0x20)
								: ((dex < mind) ? 0x1F : 0x25);
						}
						else {
							mag_item->data.datab[1] = 0x23;
						}
					}
					else if (flags & 0x010) {
						if (def < 45) {
							mag_item->data.datab[1] = (character->dress_data.section & 1)
								? ((mind < pow) ? 0x12 : 0x0C)
								: ((mind < pow) ? 0x15 : 0x11);
						}
						else {
							mag_item->data.datab[1] = 0x24;
						}
					}
				}
			}
		}
	}

	 //If the mag has evolved, add its new photon blast
	if (mag_number != mag_item->data.datab[1]) {
		pmt_mag_bb_t new_mag_def = { 0 };

		if (err = pmt_lookup_mag_bb(mag_item->data.datal[0], &new_mag_def)) {
			ERR_LOG("GC %" PRIu32 " 的背包中有不存在的MAG数据!错误码 %d",
				src->guildcard, err);
			return err;
		}

		err = add_mag_photon_blast(&mag_item->data, new_mag_def.photon_blast);
	}

	if (should_delete_item) {
		// Allow overdrafting meseta if the client is not BB, since the server isn't
		// informed when meseta is added or removed from the bank.
		remove_iitem(src, fed_item->data.item_id, 1, src->version != CLIENT_VERSION_BB);
		fix_client_inv(&character->inv);

	}

	return err;
}
