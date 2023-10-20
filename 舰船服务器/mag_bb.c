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

	inventory_t* inv = get_client_inv(src);

	int mag_item_index = -1;
	int feed_item_index = -1;
	if (!find_mag_and_feed_item(inv, mag_item_id, feed_item_id, &mag_item_index, &feed_item_index)) {
		// 没有找到魔法装备和喂养物品
		ERR_LOG("%s 玛古或物品不存在!", get_player_describe(src));
		return -1;
	}

	// 找到了魔法装备和喂养物品
	// 可以使用mag_item_index和feed_item_index进行后续操作
	item_t* mag_item = &inv->iitems[mag_item_index].data;

	/* 搜索物品的结果索引 */
	size_t result_index = find_result_index(primary_identifier(&inv->iitems[feed_item_index].data));

	if (should_delete_item) {
		item_t delete_item = remove_invitem(src, feed_item_id, 1, src->version != CLIENT_VERSION_BB);
		if (item_not_identification_bb(delete_item.datal[0], delete_item.datal[1])) {
			ERR_LOG("%s 删除 ID 0x%08X 失败", get_player_describe(src), feed_item_id);
			err = -5;
		}
	}

	/* 玛古再次进行检索 */
	mag_item_index = find_iitem_index(inv, mag_item_id);
	if (mag_item_index < 0) {
		ERR_LOG("%s 玛古不存在! 错误码 %d", get_player_describe(src), mag_item_index);
		return mag_item_index;
	}
	mag_item = &inv->iitems[mag_item_index].data;

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
			switch (get_player_class(src)) {
			case CLASS_HUMAR: // HUmar
			case CLASS_HUNEWEARL: // HUnewearl
			case CLASS_HUCAST: // HUcast
			case CLASS_HUCASEAL: // HUcaseal
				mag_item->datab[1] = Mag_Varuna; // Varuna
				break;
			case CLASS_RAMAR: // RAmar
			case CLASS_RAMARL: // RAmarl
			case CLASS_RACAST: // RAcast
			case CLASS_RACASEAL: // RAcaseal
				mag_item->datab[1] = Mag_Kalki; // Kalki
				break;
			case CLASS_FOMAR: // FOmar
			case CLASS_FOMARL: // FOmarl
			case CLASS_FONEWM: // FOnewm
			case CLASS_FONEWEARL: // FOnewearl
				mag_item->datab[1] = Mag_Vritra; // Vritra
				break;
			default:
				ERR_LOG("%s 无效角色职业 %d", get_player_describe(src), get_player_class(src));
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
					mag_item->datab[1] = Mag_Mitra;
				} else if (flags & 8) {
					mag_item->datab[1] = Mag_Surya;
				} else if (flags & 0x20) {
					mag_item->datab[1] = Mag_Tapas;
				}
			}
			else if (mag_number == 1) {
				if (flags & 0x108) {
					mag_item->datab[1] = Mag_Rudra;
				} else if (flags & 0x10) {
					mag_item->datab[1] = Mag_Marutah;
				} else if (flags & 0x20) {
					mag_item->datab[1] = Mag_Vayu;
				}
			}
			else if (mag_number == 0x19) {
				if (flags & 0x120) {
					mag_item->datab[1] = Mag_Namuci;
				} else if (flags & 8) {
					mag_item->datab[1] = Mag_Sumba;
				} else if (flags & 0x10) {
					mag_item->datab[1] = Mag_Ashvinau;
				}
			}
		}
	}
	else if ((mag_level % 5) == 0) { // Level 50 (and beyond) evolutions
		if (evolution_number < 4) {
			if (mag_level >= 100) {
				uint8_t section_id_group = get_player_section(src) % 3;
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
					ERR_LOG("%s 角色职业 %s 不在玛古进化范围中", get_player_describe(src), pso_class[get_player_class(src)].cn_name);
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
					//   M0				F0				M1			F1				M2			F2
						Mag_Deva,	Mag_Savitri,	Mag_Rati,	Mag_Savitri,	Mag_Rati,		Mag_Savitri, // Hunter
						Mag_Pushan, Mag_Rukmin,		Mag_Pushan, Mag_Rukmin,		Mag_Pushan,		Mag_Dewari, // Ranger
						Mag_Nidra,	Mag_Sato,		Mag_Nidra,	Mag_Bhima,		Mag_Nidra,		Mag_Bhima, // Force
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
					ERR_LOG("%s 角色职业 %s 不在范围中", get_player_describe(src), pso_class[get_player_class(src)].cn_name);
					err = -4;
					return err;
				}

				if (is_hunter) {
					if (flags & 0x108) {
						mag_item->datab[1] = (get_player_section(src) & 1)
							? ((dex < mind) ? Mag_Apsaras : Mag_Kama)
							: ((dex < mind) ? Mag_Bhirava : Mag_Varaha);
					}
					else if (flags & 0x010) {
						mag_item->datab[1] = (get_player_section(src) & 1)
							? ((mind < pow) ? Mag_Garuda : Mag_Yaksa)
							: ((mind < pow) ? Mag_Ila : Mag_Nandin);
					}
					else if (flags & 0x020) {
						mag_item->datab[1] = (get_player_section(src) & 1)
							? ((pow < dex) ? Mag_Soma : Mag_Bana)
							: ((pow < dex) ? Mag_Ushasu : Mag_Kabanda);
					}
				}
				else if (is_ranger) {
					if (flags & 0x110) {
						mag_item->datab[1] = (get_player_section(src) & 1)
							? ((mind < pow) ? Mag_Kaitabha : Mag_Varaha)
							: ((mind < pow) ? Mag_Bhirava : Mag_Kama);
					}
					else if (flags & 0x008) {
						mag_item->datab[1] = (get_player_section(src) & 1)
							? ((dex < mind) ? Mag_Kaitabha : Mag_Madhu)
							: ((dex < mind) ? Mag_Bhirava : Mag_Kama);
					}
					else if (flags & 0x020) {
						mag_item->datab[1] = (get_player_section(src) & 1)
							? ((pow < dex) ? Mag_Durga : Mag_Kabanda)
							: ((pow < dex) ? Mag_Apsaras : Mag_Varaha);
					}
				}
				else if (is_force) {
					if (flags & 0x120) {
						if (def < 45) {
							mag_item->datab[1] = (get_player_section(src) & 1)
								? ((pow < dex) ? Mag_Ila : Mag_Kumara)
								: ((pow < dex) ? Mag_Kabanda : Mag_Naga);
						}
						else {
							mag_item->datab[1] = Mag_Bana;
						}
					}
					else if (flags & 0x008) {
						if (def < 45) {
							mag_item->datab[1] = (get_player_section(src) & 1)
								? ((dex < mind) ? Mag_Naga : Mag_Marica)
								: ((dex < mind) ? Mag_Ravana : Mag_Naraka);
						}
						else {
							mag_item->datab[1] = Mag_Andhaka;
						}
					}
					else if (flags & 0x010) {
						if (def < 45) {
							mag_item->datab[1] = (get_player_section(src) & 1)
								? ((mind < pow) ? Mag_Garuda : Mag_Bhirava)
								: ((mind < pow) ? Mag_Ribhava : Mag_Sita);
						}
						else {
							mag_item->datab[1] = Mag_Bana;
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

/////////////////////////////////////////////////////////////////////////////////////////////
//玛古增加光子爆发函数
void mag_add_photo_blast(uint8_t* flags, uint8_t* blasts, uint8_t pb) {
	int32_t pb_exists = 0;
	uint8_t pbv;
	uint32_t pb_slot;

	if ((*flags & 0x01) == 0x01) {
		if ((*blasts & 0x07) == pb)
			pb_exists = 1;
	}

	if ((*flags & 0x02) == 0x02) {
		if (((*blasts / 8) & 0x07) == pb)
			pb_exists = 1;
	}

	if ((*flags & 0x04) == 0x04)
		pb_exists = 1;

	if (!pb_exists) {
		if ((*flags & 0x01) == 0)
			pb_slot = 0;
		else
			if ((*flags & 0x02) == 0)
				pb_slot = 1;
			else
				pb_slot = 2;
		switch (pb_slot) {
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

int Mag_Alignment(item_t* mag) {
	int v1, v2, v3, v4, v5, v6;

	v4 = 0;
	v3 = mag->dataw[3];
	v2 = mag->dataw[4];
	v1 = mag->dataw[5];

	v4 |= (v2 < v3 && v1 < v3) ? 8 : 0;
	v4 |= (v3 < v2 && v1 < v2) ? 0x10 : 0;
	v4 |= (v2 < v1 && v3 < v1) ? 0x20 : 0;

	v6 = 0;
	v5 = (v3 <= v2) ? v2 : v3;
	v5 = (v5 <= v1) ? v1 : v5;

	v6 += (v5 == v3) ? 1 : 0;
	v6 += (v5 == v2) ? 1 : 0;
	v6 += (v5 == v1) ? 1 : 0;

	v4 |= (v6 >= 2) ? 0x100 : 0;

	return v4;

}

//void mag_add_photo_blast(uint8_t* flags, uint8_t* blasts, int pb) {
//	int already_has_pb = 0;
//	uint8_t pbv;
//	uint32_t pb_slot;
//
//	if (*flags & 0x01) {
//		if ((*blasts & 0x07) == pb)
//			already_has_pb = 1;
//	}
//
//	if (*flags & 0x02) {
//		if (((*blasts >> 3) & 0x07) == pb)
//			already_has_pb = 1;
//	}
//
//	if (*flags & 0x04)
//		already_has_pb = 1;
//
//	if (!already_has_pb) {
//		pb_slot = (*flags & 0x01) ? ((*flags & 0x02) ? 2 : 1) : 0;
//		switch (pb_slot) {
//		case 0:
//			*blasts &= 0xF8;
//			*flags |= 0x01;
//			break;
//
//		case 1:
//			*blasts = (*blasts & 0xC7) | (pb & 0x07);
//			*flags |= 0x02;
//			break;
//
//		case 2:
//			pbv = (uint8_t)pb;
//			if ((*blasts & 0x07) < pbv)
//				pbv--;
//			if (((*blasts >> 3) & 0x07) < pbv)
//				pbv--;
//			pb = pbv << 6;
//			*blasts &= 0x3F;
//			*flags |= 0x04;
//		}
//		*blasts |= pb;
//	}
//}

int32_t Mag_Special_Evolution(item_t* mag, uint8_t sectionID, uint8_t type, int32_t EvolutionClass) {
	uint8_t oldType;
	int16_t mDefense, mPower, mDex, mMind;

	oldType = mag->datab[1];

	if (mag->datab[2] >= 100) {
		mDefense = mag->dataw[2] / 100;//defense
		mPower = mag->dataw[3] / 100;//Power
		mDex = mag->dataw[4] / 100;//Dex
		mMind = mag->dataw[5] / 100;//Mind

		switch (sectionID)
		{
		case SID_Viridia:
		case SID_Bluefull:
		case SID_Redria:
		case SID_Whitill:
			if ((mDefense + mDex) == (mPower + mMind))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					mag->datab[1] = Mag_Deva;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					mag->datab[1] = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					mag->datab[1] = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					mag->datab[1] = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					mag->datab[1] = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					mag->datab[1] = Mag_Sato;
					break;
				default:
					break;
				}
			}
			break;
		case SID_Skyly:
		case SID_Pinkal:
		case SID_Yellowboze:
			if ((mDefense + mPower) == (mDex + mMind))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					mag->datab[1] = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					mag->datab[1] = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					mag->datab[1] = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					mag->datab[1] = Mag_Dewari;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					mag->datab[1] = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					mag->datab[1] = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		case SID_Greennill:
		case SID_Oran:
		case SID_Purplenum:
			if ((mDefense + mMind) == (mPower + mDex))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					mag->datab[1] = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					mag->datab[1] = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					mag->datab[1] = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					mag->datab[1] = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					mag->datab[1] = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					mag->datab[1] = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		}
	}
	return (int32_t)(oldType != mag->datab[1]);
}

void Mag_LV50_Evolution(item_t* mag, uint8_t sectionID, uint8_t type, int32_t EvolutionClass) {
	int32_t v10, v11, v12, v13;

	int32_t Alignment = Mag_Alignment(mag);

	if (EvolutionClass <= 3) {
		v10 = mag->dataw[3] / 100;
		v11 = mag->dataw[4] / 100;
		v12 = mag->dataw[5] / 100;
		v13 = mag->dataw[2] / 100;

		switch (type) {
		case CLASS_HUMAR:
		case CLASS_HUNEWEARL:
		case CLASS_HUCAST:
		case CLASS_HUCASEAL:
			if (Alignment & 0x108)
			{
				if (sectionID & 1)
				{
					if (v12 > v11)
					{
						mag->datab[1] = Mag_Apsaras;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
					}
					else
					{
						mag->datab[1] = Mag_Kama;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
					}
				}
				else
				{
					if (v12 > v11)
					{
						mag->datab[1] = Mag_Bhirava;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
					}
					else
					{
						mag->datab[1] = Mag_Varaha;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
					}
				}
			}
			else
			{
				if (Alignment & 0x10)
				{
					if (sectionID & 1)
					{
						if (v10 > v12)
						{
							mag->datab[1] = Mag_Garuda;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
						}
						else
						{
							mag->datab[1] = Mag_Yaksa;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
						}
					}
					else
					{
						if (v10 > v12)
						{
							mag->datab[1] = Mag_Ila;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
						}
						else
						{
							mag->datab[1] = Mag_Nandin;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
						}
					}
				}
				else
				{
					if (Alignment & 0x20)
					{
						if (sectionID & 1)
						{
							if (v11 > v10)
							{
								mag->datab[1] = Mag_Soma;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
							}
							else
							{
								mag->datab[1] = Mag_Bana;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
							}
						}
						else
						{
							if (v11 > v10)
							{
								mag->datab[1] = Mag_Ushasu;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
							}
							else
							{
								mag->datab[1] = Mag_Kabanda;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
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
				if (sectionID & 1)
				{
					if (v10 > v12)
					{
						mag->datab[1] = Mag_Kaitabha;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
					}
					else
					{
						mag->datab[1] = Mag_Varaha;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
					}
				}
				else
				{
					if (v10 > v12)
					{
						mag->datab[1] = Mag_Bhirava;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
					}
					else
					{
						mag->datab[1] = Mag_Kama;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
					}
				}
			}
			else
			{
				if (Alignment & 0x08)
				{
					if (sectionID & 1)
					{
						if (v12 > v11)
						{
							mag->datab[1] = Mag_Kaitabha;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
						}
						else
						{
							mag->datab[1] = Mag_Madhu;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
						}
					}
					else
					{
						if (v12 > v11)
						{
							mag->datab[1] = Mag_Bhirava;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
						}
						else
						{
							mag->datab[1] = Mag_Kama;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
						}
					}
				}
				else
				{
					if (Alignment & 0x20)
					{
						if (sectionID & 1)
						{
							if (v11 > v10)
							{
								mag->datab[1] = Mag_Durga;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
							}
							else
							{
								mag->datab[1] = Mag_Kabanda;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
							}
						}
						else
						{
							if (v11 > v10)
							{
								mag->datab[1] = Mag_Apsaras;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
							}
							else
							{
								mag->datab[1] = Mag_Varaha;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
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
					mag->datab[1] = Mag_Bana;
					mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
				}
				else
				{
					if (sectionID & 1)
					{
						if (v11 > v10)
						{
							mag->datab[1] = Mag_Ila;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
						}
						else
						{
							mag->datab[1] = Mag_Kumara;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							mag->datab[1] = Mag_Kabanda;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
						}
						else
						{
							mag->datab[1] = Mag_Naga;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
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
						mag->datab[1] = Mag_Andhaka;
						mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
					}
					else
					{
						if (sectionID & 1)
						{
							if (v12 > v11)
							{
								mag->datab[1] = Mag_Naga;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
							}
							else
							{
								mag->datab[1] = Mag_Marica;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
							}
						}
						else
						{
							if (v12 > v11)
							{
								mag->datab[1] = Mag_Ravana;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Farlla);
							}
							else
							{
								mag->datab[1] = Mag_Naraka;
								mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
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
							mag->datab[1] = Mag_Bana;
							mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Estlla);
						}
						else
						{
							if (sectionID & 1)
							{
								if (v10 > v12)
								{
									mag->datab[1] = Mag_Garuda;
									mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
								}
								else
								{
									mag->datab[1] = Mag_Bhirava;
									mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
								}
							}
							else
							{
								if (v10 > v12)
								{
									mag->datab[1] = Mag_Ribhava;
									mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Farlla);
								}
								else
								{
									mag->datab[1] = Mag_Sita;
									mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
								}
							}
						}
					}
				}
			}
			break;
		}
	}
}

void Mag_LV35_Evolution(item_t* mag, uint8_t sectionID, uint8_t type, int32_t EvolutionClass)
{
	int32_t Alignment = Mag_Alignment(mag);

	if (EvolutionClass > 3) // Don't bother to check if a special mag.
		return;

	switch (type) {
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108)
		{
			mag->datab[1] = Mag_Rudra;
			mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
			return;
		}
		else
		{
			if (Alignment & 0x10)
			{
				mag->datab[1] = Mag_Marutah;
				mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
				return;
			}
			else
			{
				if (Alignment & 0x20)
				{
					mag->datab[1] = Mag_Vayu;
					mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
					return;
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
			mag->datab[1] = Mag_Mitra;
			mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
			return;
		}
		else
		{
			if (Alignment & 0x08)
			{
				mag->datab[1] = Mag_Surya;
				mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
				return;
			}
			else
			{
				if (Alignment & 0x20)
				{
					mag->datab[1] = Mag_Tapas;
					mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
					return;
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
			mag->datab[1] = Mag_Namuci;
			mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Mylla_Youlla);
			return;
		}
		else
		{
			if (Alignment & 0x08)
			{
				mag->datab[1] = Mag_Sumba;
				mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Golla);
				return;
			}
			else
			{
				if (Alignment & 0x10)
				{
					mag->datab[1] = Mag_Ashvinau;
					mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], PB_Pilla);
					return;
				}
			}
		}
		break;
	}
}

void mag_evolution_lv10(item_t* mag, uint8_t sectionID, uint8_t type, int32_t EvolutionClass) {
	enum pb_type pb = 0xFF;

	switch (type) {
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		pb = PB_Farlla;
		break;

	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		pb = PB_Estlla;
		break;

	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		pb = PB_Leilla;
		break;
	}

	if (pb != 0xFF)
		mag_add_photo_blast(&mag->data2b[2], &mag->datab[3], pb);
}

void mag_check_evolution(item_t* mag, uint8_t sectionID, uint8_t type, int32_t EvolutionClass) {
	int datab2 = mag->datab[2];

	if ((datab2 < 10) || (datab2 >= 35)) {
		if ((datab2 < 35) || (datab2 >= 50)) {
			if (datab2 >= 50) {
				if (!(datab2 % 5)) { // Divisible by 5 with no remainder.
					if (EvolutionClass <= 3) {
						if (!Mag_Special_Evolution(mag, sectionID, type, EvolutionClass))
							Mag_LV50_Evolution(mag, sectionID, type, EvolutionClass);
					}
				}
			}
		}
		else {
			if (EvolutionClass < 2)
				Mag_LV35_Evolution(mag, sectionID, type, EvolutionClass);
		}
	}
	else {
		if (EvolutionClass <= 0)
			mag_evolution_lv10(mag, sectionID, type, EvolutionClass);
	}
}
