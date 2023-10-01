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
	uint32_t item_pid;
	size_t result_index;
} fed_item_t;

const fed_item_t result_index_for_fed_item[] = {
	{0x030000, 0},  // Monomate      小HP回复液
	{0x030001, 1},  // Dimate        中HP回复液
	{0x030002, 2},  // Trimate       大HP回复液
	{0x030100, 3},  // Monofluid     小TP回复液
	{0x030101, 4},  // Difluid       中TP回复液
	{0x030102, 5},  // Trifluid      大TP回复液
	{0x030600, 6},  // Antidote      解毒剂
	{0x030601, 7},  // Antiparalysis 解痉剂
	{0x030300, 8},  // Sol Atomizer  魂之粉
	{0x030400, 9},  // Moon Atomizer 月之粉
	{0x030500, 10}  // Star Atomizer 星之粉
};

size_t num_results = sizeof(result_index_for_fed_item) / sizeof(result_index_for_fed_item[0]);

// Function to find the result index for a given fed item
size_t find_result_index(uint32_t item_pid) {
	size_t i;
	for (i = 0; i < num_results; ++i) {
		if (result_index_for_fed_item[i].item_pid == item_pid) {
			return result_index_for_fed_item[i].result_index;
		}
	}
	return 0;  // Default result index if item_pid is not found
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

	uint16_t highest = max(dex, max(pow, mind));

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

void ItemMagStats_init(magitemstat_t* stat, uint8_t color) {
	stat->iq = 0;
	stat->synchro = 40;
	stat->def = 500;
	stat->pow = 0;
	stat->dex = 0;
	stat->mind = 0;
	stat->flags = 0;
	stat->photon_blasts = 0;
	stat->color = color;
}

void assign_mag_stats(item_t* item, magitemstat_t* stat) {
	item->datab[2] = (uint8_t)level(stat);
	item->datab[3] = stat->photon_blasts;
	item->dataw[2] = stat->def & 0x7FFE;
	item->dataw[3] = stat->pow & 0x7FFE;
	item->dataw[4] = stat->dex & 0x7FFE;
	item->dataw[5] = stat->mind & 0x7FFE;
	item->data2b[0] = (uint8_t)stat->synchro;
	item->data2b[1] = (uint8_t)stat->iq;
	item->data2b[2] = stat->flags;
	item->data2b[3] = stat->color;
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
		ERR_LOG("无法索引到空的光子爆发槽位.");
		return -1;
	}
	else {
		// Invalid slot index
		ERR_LOG("无效光子爆发槽索引.");
		return -2;
	}
}

int mag_has_photon_blast_in_any_slot(const item_t* item, uint8_t pb_num) {
	if (pb_num < PB_MAX) {
		for (uint8_t slot = 0; slot < 3; slot++) {
			if (mag_photon_blast_for_slot(item, slot) == pb_num) {
				return 1;
			}
		}
	}
	return 0;
}

int add_mag_photon_blast(item_t* item, uint8_t pb_num) {

	if (pb_num >= PB_MAX) {
		ERR_LOG("玛古 %s 光子槽索引超出界限 %d", get_item_describe(item, 5), pb_num);
		return -1;
	}

	if (mag_has_photon_blast_in_any_slot(item, pb_num)) {
		ERR_LOG("玛古 %s 光子槽 %d 已有技能", get_item_describe(item, 5), pb_num);
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
			*pb_nums |= (pb_num << 6);
			return -3;
		}
		*flags |= 4;
	}

	return 0;
}

int player_feed_mag(ship_client_t* src, size_t mag_item_id, size_t feed_item_id) {
	errno_t err = 0;
	bool should_delete_item = (src->version != CLIENT_VERSION_DCV2) && (src->version != CLIENT_VERSION_PC);

	psocn_bb_char_t* character = get_client_char_bb(src);

	int mag_item_index = -1;
	int feed_item_index = -1;
	if (!find_mag_and_feed_item(&character->inv, mag_item_id, feed_item_id, &mag_item_index, &feed_item_index)) {
		// 没有找到魔法装备和喂养物品
		ERR_LOG("%s 玛古或物品不存在!", get_player_describe(src));
		return -1;
	}

	// 找到了魔法装备和喂养物品
	// 可以使用mag_item_index和feed_item_index进行后续操作
	item_t* mag_item = &character->inv.iitems[mag_item_index].data;

	/* 搜索物品的结果索引 */
	size_t result_index = find_result_index(primary_identifier(&character->inv.iitems[feed_item_index].data));

	if (should_delete_item) {
		iitem_t delete_item = remove_iitem(src, feed_item_id, 1, src->version != CLIENT_VERSION_BB);
		if (item_not_identification_bb(delete_item.data.datal[0], delete_item.data.datal[1])) {
			ERR_LOG("%s 删除 ID 0x%08X 失败", get_player_describe(src), feed_item_id);
			err = -5;
		}
	}

	/* 玛古再次进行检索 */
	mag_item_index = find_iitem_index(&character->inv, mag_item_id);
	if (mag_item_index < 0) {
		ERR_LOG("%s 玛古不存在! 错误码 %d", get_player_describe(src), mag_item_index);
		return mag_item_index;
	}
	mag_item = &character->inv.iitems[mag_item_index].data;

	/* 查找该玛古的喂养表 */
	pmt_mag_bb_t mag_table = { 0 };
	if ((err = pmt_lookup_mag_bb(mag_item->datal[0], &mag_table))) {
		ERR_LOG("%s 喂养了不存在的玛古数据!错误码 %d",
			get_player_describe(src), err);
		return err;
	}

	pmt_mag_feed_result_t feed_result = { 0 };
	if ((err = pmt_lookup_mag_feed_table_bb(mag_item->datal[0], mag_table.feed_table, result_index, &feed_result))) {
		ERR_LOG("%s 喂养了不存在的玛古数据!错误码 %d",
			get_player_describe(src), err);
		return err;
	}

#ifdef DEBUG

	DBG_LOG("feed_table index %d result_index %d", mag_table.feed_table, result_index);

	print_item_data(mag_item, src->version);

	DBG_LOG("%d %d %d %d %d %d", feed_result.def, feed_result.pow, feed_result.dex, feed_result.mind, feed_result.synchro, feed_result.iq);

#endif // DEBUG

	update_stat(mag_item, 2, feed_result.def);
	update_stat(mag_item, 3, feed_result.pow);
	update_stat(mag_item, 4, feed_result.dex);
	update_stat(mag_item, 5, feed_result.mind);
	mag_item->data2b[0] = (uint8_t)clamp((ssize_t)mag_item->data2b[0] + feed_result.synchro, 0, 120);
	mag_item->data2b[1] = (uint8_t)clamp((ssize_t)mag_item->data2b[1] + feed_result.iq, 0, 200);

	uint8_t mag_level = (uint8_t)compute_mag_level(mag_item);
	mag_item->datab[2] = mag_level;
	uint8_t evolution_number = magedit_lookup_mag_evolution_number(mag_item);
	uint8_t mag_number = mag_item->datab[1];

	// Note: Sega really did just hardcode all these rules into the client. There
	// is no data file describing these evolutions, unfortunately.
	if (mag_level < 10) {
		// 啥也不做

	}
	else if (mag_level < 35) { // Level 10 evolution
		if (evolution_number < 1) {
			switch (character->dress_data.ch_class) {
			case CLASS_HUMAR: // HUmar
			case CLASS_HUNEWEARL: // HUnewearl
			case CLASS_HUCAST: // HUcast
			case CLASS_HUCASEAL: // HUcaseal
				mag_item->datab[1] = 0x01; // Varuna
				break;
			case CLASS_RAMAR: // RAmar
			case CLASS_RAMARL: // RAmarl
			case CLASS_RACAST: // RAcast
			case CLASS_RACASEAL: // RAcaseal
				mag_item->datab[1] = 0x0D; // Kalki
				break;
			case CLASS_FOMAR: // FOmar
			case CLASS_FOMARL: // FOmarl
			case CLASS_FONEWM: // FOnewm
			case CLASS_FONEWEARL: // FOnewearl
				mag_item->datab[1] = 0x19; // Vritra
				break;
			default:
				ERR_LOG("%s 无效角色职业 %d", get_player_describe(src), character->dress_data.ch_class);
				err = -2;
				return err;
			}
		}
	}
	else if (mag_level < 50) { // Level 35 evolution
		if (evolution_number < 2) {
			uint16_t flags = compute_mag_strength_flags(mag_item);
			if (mag_number == 0x0D) {
				if ((flags & 0x110) == 0) {
					mag_item->datab[1] = 0x02;
				} else if (flags & 8) {
					mag_item->datab[1] = 0x03;
				} else if (flags & 0x20) {
					mag_item->datab[1] = 0x0B;
				}
			}
			else if (mag_number == 1) {
				if (flags & 0x108) {
					mag_item->datab[1] = 0x0E;
				} else if (flags & 0x10) {
					mag_item->datab[1] = 0x0F;
				} else if (flags & 0x20) {
					mag_item->datab[1] = 0x04;
				}
			}
			else if (mag_number == 0x19) {
				if (flags & 0x120) {
					mag_item->datab[1] = 0x1A;
				} else if (flags & 8) {
					mag_item->datab[1] = 0x1B;
				} else if (flags & 0x10) {
					mag_item->datab[1] = 0x14;
				}
			}
		}
	}
	else if ((mag_level % 5) == 0) { // Level 50 (and beyond) evolutions
		if (evolution_number < 4) {

			if (mag_level >= 100) {
				uint8_t section_id_group = character->dress_data.section % 3;
				uint16_t def = mag_item->dataw[2] / 100;
				uint16_t pow = mag_item->dataw[3] / 100;
				uint16_t dex = mag_item->dataw[4] / 100;
				uint16_t mind = mag_item->dataw[5] / 100;
				bool is_male = char_class_is_male(src->equip_flags);
				size_t table_index = (is_male ? 0 : 1) + section_id_group * 2;

				bool is_hunter = char_class_is_hunter(src->equip_flags);
				bool is_ranger = char_class_is_ranger(src->equip_flags);
				bool is_force = char_class_is_force(src->equip_flags);
				if (is_force) {
					table_index += 12;
				}
				else if (is_ranger) {
					table_index += 6;
				}
				else if (!is_hunter) {
					ERR_LOG("%s 角色职业 %s 不在玛古进化范围中", get_player_describe(src), pso_class[character->dress_data.ch_class].cn_name);
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
					mag_item->datab[1] = result_table[table_index];
				}
			}

			// If a special evolution did not occur, do a normal level 50 evolution
			if (mag_number == mag_item->datab[1]) {
				uint16_t flags = compute_mag_strength_flags(mag_item);
				uint16_t def = mag_item->dataw[2] / 100;
				uint16_t pow = mag_item->dataw[3] / 100;
				uint16_t dex = mag_item->dataw[4] / 100;
				uint16_t mind = mag_item->dataw[5] / 100;

				bool is_hunter = char_class_is_hunter(src->equip_flags);
				bool is_ranger = char_class_is_ranger(src->equip_flags);
				bool is_force = char_class_is_force(src->equip_flags);
				if (!is_hunter && !is_ranger && !is_force) {
					ERR_LOG("%s 角色职业 %s 不在范围中", get_player_describe(src), pso_class[character->dress_data.ch_class].cn_name);
					err = -4;
					return err;
				}

				if (is_hunter) {
					if (flags & 0x108) {
						mag_item->datab[1] = (character->dress_data.section & 1)
							? ((dex < mind) ? 0x08 : 0x06)
							: ((dex < mind) ? 0x0C : 0x05);
					}
					else if (flags & 0x010) {
						mag_item->datab[1] = (character->dress_data.section & 1)
							? ((mind < pow) ? 0x12 : 0x10)
							: ((mind < pow) ? 0x17 : 0x13);
					}
					else if (flags & 0x020) {
						mag_item->datab[1] = (character->dress_data.section & 1)
							? ((pow < dex) ? 0x16 : 0x24)
							: ((pow < dex) ? 0x07 : 0x1E);
					}
				}
				else if (is_ranger) {
					if (flags & 0x110) {
						mag_item->datab[1] = (character->dress_data.section & 1)
							? ((mind < pow) ? 0x0A : 0x05)
							: ((mind < pow) ? 0x0C : 0x06);
					}
					else if (flags & 0x008) {
						mag_item->datab[1] = (character->dress_data.section & 1)
							? ((dex < mind) ? 0x0A : 0x26)
							: ((dex < mind) ? 0x0C : 0x06);
					}
					else if (flags & 0x020) {
						mag_item->datab[1] = (character->dress_data.section & 1)
							? ((pow < dex) ? 0x18 : 0x1E)
							: ((pow < dex) ? 0x08 : 0x05);
					}
				}
				else if (is_force) {
					if (flags & 0x120) {
						if (def < 45) {
							mag_item->datab[1] = (character->dress_data.section & 1)
								? ((pow < dex) ? 0x17 : 0x09)
								: ((pow < dex) ? 0x1E : 0x1C);
						}
						else {
							mag_item->datab[1] = 0x24;
						}
					}
					else if (flags & 0x008) {
						if (def < 45) {
							mag_item->datab[1] = (character->dress_data.section & 1)
								? ((dex < mind) ? 0x1C : 0x20)
								: ((dex < mind) ? 0x1F : 0x25);
						}
						else {
							mag_item->datab[1] = 0x23;
						}
					}
					else if (flags & 0x010) {
						if (def < 45) {
							mag_item->datab[1] = (character->dress_data.section & 1)
								? ((mind < pow) ? 0x12 : 0x0C)
								: ((mind < pow) ? 0x15 : 0x11);
						}
						else {
							mag_item->datab[1] = 0x24;
						}
					}
				}
			}
		}
	}

	 //如果玛古进化了,则增加光子爆发
	if (mag_number != mag_item->datab[1]) {
		pmt_mag_bb_t new_mag_def = { 0 };

		if (err = pmt_lookup_mag_bb(mag_item->datal[0], &new_mag_def)) {
			ERR_LOG("%s 的背包中有不存在的MAG数据!错误码 %d",
				get_player_describe(src), err);
			return err;
		}

		if (add_mag_photon_blast(mag_item, new_mag_def.photon_blast)) {
			ERR_LOG("%s 玛古新增PB %d 出错!", get_player_describe(src), new_mag_def.photon_blast);
		}
	}

#ifdef DEBUG

	print_item_data(mag_item, src->version);

#endif // DEBUG

	return err;
}
