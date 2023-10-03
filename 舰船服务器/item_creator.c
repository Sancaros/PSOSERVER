/*
	梦幻之星中国 舰船服务器 物品制造器
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

#include <items.h>
#include <f_logs.h>
#include <debug.h>
#include <SFMT.h>

#include <AFS.h>
#include <GSL.h>

#include "ship.h"
#include "ptdata.h"
#include "pmtdata.h"
#include "rtdata.h"
#include "subcmd.h"
#include "subcmd_send.h"
#include "handle_player_items.h"
#include "utils.h"
#include "quests.h"
#include "mag_bb.h"
#include "itemrt_remap.h"

bool should_allow_meseta_drops(lobby_t* l) {
	/*需要完成一个开关或者是版本检测*/
	return l->challenge != 1;
}

bool are_rare_drops_allowed(lobby_t* l) {
	//注意：客户在这里有一个额外的检查，这似乎是一个微妙的
	//反作弊措施。客户端上有一个标志，最初为零
	//当某些与项目相关的意外事件发生时（对于
	//例如拥有等级高于200的mag的玩家）。当标志
	//如果设置了，此函数将返回false，这将阻止所有稀有项目的删除。
	//newserv有意不实现此标志。
	return l->challenge != 1;
}

void deduplicate_weapon_bonuses(item_t* item) {
	size_t x, y;

	for (x = 0; x < 6; x += 2) {
		for (y = 0; y < x; y += 2) {
			if (item->datab[y + 6] == 0x00) {
				item->datab[x + 6] = 0x00;
			}
			else if (item->datab[x + 6] == item->datab[y + 6]) {
				item->datab[x + 6] = 0x00;
			}
		}
		if (item->datab[x + 6] == 0x00) {
			item->datab[x + 7] = 0x00;
		}
	}
}

uint32_t rand_int(sfmt_t* rng, uint64_t max) {
	return sfmt_genrand_uint32(rng) % max;
}

float rand_float_0_1_from_crypt(sfmt_t* rng) {
	uint32_t next = sfmt_genrand_uint32(rng);
	next = next ^ (next << 11);
	next = next ^ (next >> 8);
	next = next ^ ((next << 19) & 0xffffffff);
	next = next ^ ((next >> 4) & 0xffffffff);
	return (float)(next >> 16) / 65536.0f;
}

// 从一维加权表中随机选择一个值
uint8_t get_rand_from_weighted_tables(sfmt_t* rng,
	uint8_t* data, size_t offset, size_t num_values, size_t stride) {

	uint64_t rand_max = 0;
	for (size_t x = 0; x < num_values; x++) {
		rand_max += data[x * stride + offset];  // 计算加权表的总权重
	}

	if (rand_max == 0) {
		ERR_LOG("权重表为空 offset 0x%08X num_values %zu stride %zu", offset, num_values, stride);
		return 0;
	}

	uint32_t x = rand_int(rng, rand_max);  // 生成一个 [0, rand_max) 范围内的随机数

	for (size_t z = 0; z < num_values; z++) {
		uint32_t table_value = data[z * stride + offset];

		if (x < table_value) {  // 当随机数 x 小于当前元素的权重时，返回对应的索引值
			return (uint8_t)z;
		}

		x -= table_value;  // 更新随机数 x
	}

	ERR_LOG("选择器不小于 rand_max");
	return 0;
}

// 从二维垂直排列的加权表中随机选择一个值
uint8_t get_rand_from_weighted_tables_2d_vertical(sfmt_t* rng,
	void* tables, size_t offset, size_t X, size_t Y) {
	uint8_t* table = (uint8_t*)tables;

	if (!table) {
		ERR_LOG("数据表为空");
		return 0;
	}

	return get_rand_from_weighted_tables(rng, table, offset, X, Y);
}

void generate_common_weapon_bonuses(sfmt_t* rng,
	item_t* item, uint8_t area_norm, pt_bb_entry_t* ent) {
	if (item->datab[0] == 0) {
		for (size_t row = 0; row < 3; row++) {
			uint8_t spec = ent->nonrare_bonus_prob_spec[row][area_norm];
			if (spec != 0xFF) {
				item->datab[(row * 2) + 6] = get_rand_from_weighted_tables_2d_vertical(rng,
					ent->bonus_type_prob_tables, area_norm, 6, 10);
				int16_t amount = get_rand_from_weighted_tables_2d_vertical(rng,
					ent->bonus_value_prob_tables, spec, 23, 6);
				item->datab[(row * 2) + 7] = amount * 5 - 10;
			}
			// Note: The original code has a special case here, which divides
			// item->datab[z + 7] by 5 and multiplies it by 5 again if bonus_type is 5
			// (Hit). Why this is done is unclear, because item->datab[z + 7] must
			// already be a multiple of 5.
		}
		deduplicate_weapon_bonuses(item);
	}
}

uint8_t get_rand_from_weighted_tables_1d(sfmt_t* rng,
	uint8_t* tables, size_t X) {
	if (!tables) {
		ERR_LOG("一维数据表为空");
		return 0;
	}

	return get_rand_from_weighted_tables(rng, tables, 0, X, 1);
}

void generate_common_weapon_grind(sfmt_t* rng,
	item_t* item, uint8_t offset_within_subtype_range, pt_bb_entry_t* ent) {
	if (item->datab[0] == 0) {
		uint8_t offset = (uint8_t)clamp(offset_within_subtype_range, 0, 3);
		item->datab[3] = get_rand_from_weighted_tables_2d_vertical(rng,
			ent->grind_prob_tables, offset, 9, 4);
	}
}

void generate_common_weapon_special(sfmt_t* rng,
	item_t* item, uint8_t area_norm, pt_bb_entry_t* ent) {
	if (item->datab[0] != 0) {
		return;
	}
	if (is_item_rare(item)) {
		return;
	}
	uint8_t special_mult = ent->special_mult[area_norm];
	if (special_mult == 0) {
		return;
	}
	if (rand_int(rng, 100) >= ent->special_percent[area_norm]) {
		return;
	}
	item->datab[4] = choose_weapon_special(
		special_mult * (uint8_t)rand_float_0_1_from_crypt(rng));
}

void generate_common_weapon_variances(lobby_t* l, sfmt_t* rng, uint8_t area_norm, item_t* item, pt_bb_entry_t* ent) {
	clear_inv_item(item);
	item->datab[0] = 0x00;

	uint8_t weapon_type_prob_table[0x0D] = { 0 };
	weapon_type_prob_table[0] = 0;
	memmove(weapon_type_prob_table + 1, ent->base_weapon_type_prob_table, 0x0C);

	for (size_t z = 1; z < 13; z++) {
		// Technically this should be `if (... < 0)`, but whatever
		if ((area_norm + ent->subtype_base_table[z - 1]) & 0x80) {
			weapon_type_prob_table[z] = 0;
		}
	}

	item->datab[1] = get_rand_from_weighted_tables_1d(rng, weapon_type_prob_table, 13);
	if (item->datab[1] == 0) {
		clear_inv_item(item);
	}
	else {
		int8_t subtype_base = ent->subtype_base_table[item->datab[1] - 1];
		uint8_t area_length = ent->subtype_area_length_table[item->datab[1] - 1];
		if (subtype_base < 0) {
			item->datab[2] = (area_norm + subtype_base) / area_length;
			generate_common_weapon_grind(rng,
				item, (area_norm + subtype_base) - (item->datab[2] * area_length), ent);
		}
		else {
			item->datab[2] = subtype_base + (area_norm / area_length);
			generate_common_weapon_grind(rng,
				item, area_norm - (area_norm / area_length) * area_length, ent);
		}
		generate_common_weapon_bonuses(rng, item, area_norm, ent);
		generate_common_weapon_special(rng, item, area_norm, ent);
		set_item_identified_flag(l->challenge, item);
	}
}

uint32_t choose_meseta_amount(sfmt_t* rng,
	const rang_16bit_t* ranges,
	size_t table_index) {
	uint16_t min = ranges[table_index].min;
	uint16_t max = ranges[table_index].max;

	// Note: The original code seems like it has a bug here: it compares to 0xFF
	// instead of 0xFFFF (and returns 0xFF if either limit matches 0xFF).
	if (((min == 0xFFFF) || (max == 0xFFFF)) || (max < min)) {
		return 0xFFFF;
	}
	else if (min != max) {
		return rand_int(rng, (uint64_t)((max - min) + 1)) + min;
	}
	return min;
}

void generate_common_armor_slot_count(sfmt_t* rng, item_t* item, pt_bb_entry_t* ent) {
	item->datab[5] = get_rand_from_weighted_tables_1d(rng, ent->armor_slot_count_prob_table, 5);
}

void generate_common_armor_slots_and_bonuses(sfmt_t* rng, item_t* item, pt_bb_entry_t* ent) {
	/* 检测物品是否属于盔甲 */
	if ((item->datab[0] != ITEM_TYPE_GUARD) || (item->datab[1] < ITEM_SUBTYPE_FRAME) || (item->datab[1] > ITEM_SUBTYPE_BARRIER)) {
		return;
	}

	if (item->datab[1] == ITEM_SUBTYPE_FRAME) {
		generate_common_armor_slot_count(rng, item, ent);
	}

	pmt_guard_bb_t def;
	errno_t err = 0;
	if (err = pmt_lookup_guard_bb(item->datal[0], &def)) {
		ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item->datal[0]);
		return;
	}
	set_armor_or_shield_defense_bonus(item, (int16_t)(def.dfp_range * rand_float_0_1_from_crypt(rng)));
	set_common_armor_evasion_bonus(item, (int16_t)(def.evp_range * rand_float_0_1_from_crypt(rng)));
}

void generate_common_armor_or_shield_type_and_variances(sfmt_t* rng,
	char area_norm, item_t* item, pt_bb_entry_t* ent) {
	generate_common_armor_slots_and_bonuses(rng, item, ent);

	uint8_t type = get_rand_from_weighted_tables_1d(rng,
		ent->armor_shield_type_index_prob_table, 5);
	item->datab[2] = area_norm + type + ent->armor_level;

	//DBG_LOG("0x%02X", item->datab[2]);
	////if (item->datab[2] < 3) {
	////	item->datab[2] = 0;
	////}
	////else {
	////	item->datab[2] -= 3;
	////}
	////DBG_LOG("0x%02X", item->datab[2]);
	//print_item_data(item, 5);
}

void generate_common_unit_variances(sfmt_t* rng, uint8_t det, item_t* item) {
	if (det >= 0x0D) {
		return;
	}
	clear_inv_item(item);
	item->datab[0] = ITEM_TYPE_GUARD;
	item->datab[1] = ITEM_SUBTYPE_UNIT;

	// Note: The original code calls generate_unit_weights_table1 here (which we
	// have inlined into generate_unit_weights_tables above). This call seems
	// unnecessary because the contents of the tables don't depend on anything
	// except what appears in ItemPMT, which is essentially constant, so we
	// don't bother regenerating the table here.

	if (get_unit_weights_table1(det) == 0) {
		return;
	}

	size_t which = rand_int(rng, get_unit_weights_table2(det));
	size_t current_index = 0;
	for (size_t z = 0; z < get_unit_weights_table1_size(); z++) {
		if (det != get_unit_weights_table1(z)) {
			continue;
		}
		if (current_index != which) {
			current_index++;
		}
		else {
			if (z > 0x4F) {
				if (det <= 0x87) {
					item->datab[2] = (uint8_t)(z + 0xC0);
				}
			}
			else {
				item->datab[2] = (uint8_t)(z / 5);
				pmt_unit_bb_t def;
				pmt_lookup_unit_bb(item->datab[2], &def);
				switch (z % 5) {
				case 0:
					set_unit_bonus(item, -(def.pm_range * 2));
					break;
				case 1:
					set_unit_bonus(item, -def.pm_range);
					break;
				case 2:
					break;
				case 3:
					set_unit_bonus(item, def.pm_range);
					break;
				case 4:
					set_unit_bonus(item, def.pm_range * 2);
					break;
				}
			}
			break;
		}
	}
}

void generate_common_mag_variances(item_t* item, uint8_t color) {
	if (item->datab[0] == ITEM_TYPE_MAG) {
		item->datab[1] = 0x00;
		magitemstat_t stats;
		ItemMagStats_init(&stats, color);
		assign_mag_stats(item, &stats);
	}
}

void generate_common_tool_type(uint8_t tool_class, item_t* item) {
	uint8_t data[2] = { 0 };
	if (find_tool_by_class(tool_class, data)) {
#ifdef DEBUG
		ERR_LOG("无效物品类别 %d", tool_class);
#endif // DEBUG
		return;
	}
	item->datab[0] = 0x03;
	item->datab[1] = data[0];
	item->datab[2] = data[1];
}

uint8_t generate_tech_disk_level(sfmt_t* rng, uint32_t tech_num, uint32_t area_norm, pt_bb_entry_t* ent) {
	uint8_t min = ent->technique_level_ranges[tech_num][area_norm];
	uint8_t max = ent->technique_level_ranges[tech_num][area_norm + 1];

	if (((min == 0xFF) || (max == 0xFF)) || (max < min)) {
		return 0xFF;
	}
	else if (min != max) {
		return rand_int(rng, (uint64_t)((max - min) + 1)) + min;
	}
	return min;
}

void generate_common_tool_variances(sfmt_t* rng, uint32_t area_norm, item_t* item, pt_bb_entry_t* ent) {
	clear_inv_item(item);

	uint8_t tool_class = get_rand_from_weighted_tables_2d_vertical(rng,
		ent->tool_class_prob_table, area_norm, 28, 10);

	if (tool_class == 0x1A) {
		/* 如果到底了 则让它掉落PD 这是PD的PMT索引ID */
		tool_class = 0x73;
	}

	generate_common_tool_type(tool_class, item);
	// 法术光盘
	if (item->datab[1] == ITEM_SUBTYPE_DISK) {
		item->datab[4] = get_rand_from_weighted_tables_2d_vertical(rng,
			ent->technique_index_prob_table, area_norm, 19, 10);
		item->datab[2] = generate_tech_disk_level(rng, item->datab[4], area_norm, ent);
		clear_tool_item_if_invalid(item);
	}
	get_item_amount(item, 1);
}

/* drop_sub TODO */
uint8_t normalize_area_number(lobby_t* l, uint8_t area) {
	if (!item_drop_sub || (area < 0x10) || (area > 0x11)) {
		uint8_t new_area = area;
		switch (l->episode) {
		case GAME_TYPE_EPISODE_1:
			switch (area) {
			case 11:
				new_area = 3; // Dragon -> Cave 1
				break;
			case 12:
				new_area = 6; // De Rol Le -> Mine 1
				break;
			case 13:
				new_area = 8; // Vol Opt -> Ruins 1
				break;
			case 14:
				new_area = 10; // Dark Falz -> Ruins 3
				break;
			default:
				new_area = area;
				break;
			}
			break;
		case GAME_TYPE_EPISODE_2: {
			static const uint8_t area_subs[] = {
				0x01, // 13 (VR Temple Alpha)
				0x02, // 14 (VR Temple Beta)
				0x03, // 15 (VR Spaceship Alpha)
				0x04, // 16 (VR Spaceship Beta)
				0x08, // 17 (Central Control Area)
				0x05, // 18 (Jungle North)
				0x06, // 19 (Jungle South)
				0x07, // 1A (Mountain)
				0x08, // 1B (Seaside)
				0x09, // 1C (Seabed Upper)
				0x0A, // 1D (Seabed Lower)
				0x09, // 1E (Gal Gryphon)
				0x0A, // 1F (Olga Flow)
				0x03, // 20 (Barba Ray)
				0x05, // 21 (Gol Dragon)
				0x08, // 22 (Seaside Night)
				0x0A, // 23 (Tower)
			};

			if ((area >= 0x13) && (area < 0x24)) {
				new_area = area_subs[area - 0x13];
			}else
				new_area = area;
			break;
		}
		case GAME_TYPE_EPISODE_3:
		case GAME_TYPE_EPISODE_4:
			// TODO: Figure out remaps for Episode 4 (if there are any)
			new_area = area + 1;
			break;
		default:
			ERR_LOG("%s 无效章节 %d 区域 %d", get_lobby_leader_describe(l), l->episode, area);
			new_area = 1;
			break;
		}

		if (area >= 10) {
			ERR_LOG("%s 章节 %d 区域数字大于10 %d", get_lobby_leader_describe(l), l->episode, area);
			new_area = 10;
		}

		return new_area;
	}
	else {
		return item_drop_sub[l->episode].override_area;
	}

	return 0;
}

void clear_item_if_restricted(lobby_t* l, item_t* item) {
	if (is_item_rare(item) && !are_rare_drops_allowed(l)) {
		ERR_LOG("Restricted: item is rare, but rares not allowed");
		clear_inv_item(item);
		return;
	}

	if (l->challenge) {
		// Forbid HP/TP-restoring units and meseta in challenge mode
		// Note: PSO GC doesn't check for 0x61 or 0x62 here since those items
		// (HP/Resurrection and TP/Resurrection) only exist on BB.
		if (item->datab[0] == 1) {
			if ((item->datab[1] == 3) && (((item->datab[2] >= 0x33) && (item->datab[2] <= 0x38)) || (item->datab[2] == 0x61) || (item->datab[2] == 0x62))) {
				ERR_LOG("Restricted: restore units not allowed in Challenge mode");
				clear_inv_item(item);
				return;
			}
		}
		else if (item->datab[0] == 4) {
			ERR_LOG("Restricted: meseta not allowed in Challenge mode");
			clear_inv_item(item);
			return;
		}
	}

	if (restrictions) {
		switch (item->datab[0]) {
		case ITEM_TYPE_WEAPON:
		case ITEM_TYPE_GUARD:
			switch (restrictions->weapon_and_armor_mode) {
			case WEAPON_AND_ARMOR_MODE_ALL_ON:
			case WEAPON_AND_ARMOR_MODE_ONLY_PICKING:
				break;
			case WEAPON_AND_ARMOR_MODE_NO_RARE:
				if (is_item_rare(item)) {
					ERR_LOG("Restricted: rare items not allowed");
					clear_inv_item(item);
				}
				break;
			case WEAPON_AND_ARMOR_MODE_ALL_OFF:
				ERR_LOG("Restricted: weapons and armors not allowed");
				clear_inv_item(item);
				break;
			default:
				ERR_LOG("invalid weapon and armor mode");
			}
			break;
		case ITEM_TYPE_MAG:
			if (restrictions->forbid_mags) {
				ERR_LOG("Restricted: mags not allowed");
				clear_inv_item(item);
			}
			break;
		case ITEM_TYPE_TOOL:
			if (restrictions->tool_mode == TOOL_MODE_ALL_OFF) {
				ERR_LOG("Restricted: tools not allowed");
				clear_inv_item(item);
			}
			else if (item->datab[1] == 2) {
				switch (restrictions->tech_disk_mode) {
				case TECH_DISK_MODE_ON:
					break;
				case TECH_DISK_MODE_OFF:
					ERR_LOG("Restricted: tech disks not allowed");
					clear_inv_item(item);
					break;
				case TECH_DISK_MODE_LIMIT_LEVEL:
					ERR_LOG("Restricted: tech disk level limited to %hhu",
						(uint8_t)(restrictions->max_tech_disk_level + 1));
					if (restrictions->max_tech_disk_level == 0) {
						item->datab[2] = 0;
					}
					else {
						item->datab[2] %= restrictions->max_tech_disk_level;
					}
					break;
				default:
					ERR_LOG("invalid tech disk mode");
				}
			}
			else if ((item->datab[1] == 9) && restrictions->forbid_scape_dolls) {
				ERR_LOG("Restricted: scape dolls not allowed");
				clear_inv_item(item);
			}
			break;
		case ITEM_TYPE_MESETA:
			if (restrictions->meseta_drop_mode == MESETA_DROP_MODE_OFF) {
				ERR_LOG("Restricted: meseta not allowed");
				clear_inv_item(item);
			}
			break;
		default:
			ERR_LOG("invalid item");
		}
	}
}

void generate_common_item_variances(lobby_t* l, sfmt_t* rng, uint32_t norm_area, item_t* item, pt_bb_entry_t* ent) {
	switch (item->datab[0]) {
	case ITEM_TYPE_WEAPON:
		generate_common_weapon_variances(l, rng, norm_area, item, ent);
		break;

	case ITEM_TYPE_GUARD:
		switch (item->datab[1]) {
		case ITEM_SUBTYPE_FRAME:
			generate_common_armor_or_shield_type_and_variances(rng, norm_area, item, ent);
			break;

		case ITEM_SUBTYPE_BARRIER:
			generate_common_armor_or_shield_type_and_variances(rng, norm_area, item, ent);
			break;

		case ITEM_SUBTYPE_UNIT:
			float f1 = (float)(1.0 + ent->unit_maxes[norm_area]);
			float f2 = rand_float_0_1_from_crypt(rng);
			generate_common_unit_variances(rng, (uint32_t)(f1 * f2) & 0xFF, item);
			if (item->datab[2] == 0xFF) {
				clear_inv_item(item);
			}
			break;
		}
		break;

	case ITEM_TYPE_MAG:
		generate_common_mag_variances(item, get_player_msg_color_set(l->clients[l->leader_id]));
		break;

	case ITEM_TYPE_TOOL:
		generate_common_tool_variances(rng, norm_area, item, ent);
		break;

	case ITEM_TYPE_MESETA:
		item->data2l = choose_meseta_amount(rng, ent->box_meseta_ranges, norm_area) & 0xFFFF;
		break;

	default:
		// Note: The original code does the following here:
		// clear_inv_item(&item);
		// item->datab[0] = 0x05;
		ERR_LOG("%s 掉落无效物品类型 0x%02X", get_lobby_leader_describe(l), item->datab[0]);
	}

	clear_item_if_restricted(l, item);
	set_item_kill_count_if_unsealable(item);
}

void generate_rare_weapon_bonuses(sfmt_t* rng, pt_bb_entry_t* ent, item_t* item, uint32_t random_sample) {
	if (item->datab[0] != 0) {
		return;
	}

	for (size_t z = 0; z < 6; z += 2) {
		uint8_t bonus_type = get_rand_from_weighted_tables_2d_vertical(rng,
			ent->bonus_type_prob_tables, random_sample, 6, 10);
		int16_t bonus_value = get_rand_from_weighted_tables_2d_vertical(rng,
			ent->bonus_value_prob_tables, 5, 23, 6);
		item->datab[z + 6] = bonus_type;
		item->datab[z + 7] = bonus_value * 5 - 10;
		// Note: The original code has a special case here, which divides
		// item->datab[z + 7] by 5 and multiplies it by 5 again if bonus_type is 5
		// (Hit). Why this is done is unclear, because item->datab[z + 7] must
		// already be a multiple of 5.
	}

	deduplicate_weapon_bonuses(item);
}

item_t check_rate_and_create_rare_item(lobby_t* l, pt_bb_entry_t* ent, sfmt_t* rng, PackedDrop_t drop) {
	item_t item = { 0 };
	double drop_rare = 0;
	if (drop.probability == 0) {
		return item;
	}

	// Note: The original code uses 0xFFFFFFFF as the maximum here. We use
	// 0x100000000 instead, which makes all rare items SLIGHTLY more rare.
	if ((drop_rare = sfmt_genrand_real1(rng)) >= expand_rate(drop.probability)) {
#ifdef DEBUG

		DBG_LOG("drop_rare %lf probability %lf ", drop_rare, expand_rate(drop.probability));

#endif // DEBUG
		return item;
	}
#ifdef DEBUG
	else
		DBG_LOG("drop_rare %lf probability %lf ", drop_rare, expand_rate(drop.probability));

#endif // DEBUG

	item.datab[0] = drop.item_code[0];
	item.datab[1] = drop.item_code[1];
	item.datab[2] = drop.item_code[2];
	switch (item.datab[0]) {
	case ITEM_TYPE_WEAPON:
		generate_rare_weapon_bonuses(rng, ent, &item, rand_int(rng, 10));
		set_item_identified_flag(l->challenge, &item);
		break;
	case ITEM_TYPE_GUARD:
		generate_common_armor_slots_and_bonuses(rng, &item, ent);
		break;
	case ITEM_TYPE_MAG:
		generate_common_mag_variances(&item, get_player_msg_color_set(l->clients[l->leader_id]));
		break;
	case ITEM_TYPE_TOOL:
		clear_tool_item_if_invalid(&item);
		get_item_amount(&item, 1);
		break;
	case ITEM_TYPE_MESETA:
		break;
	default:
		ERR_LOG("无效物品类型");
	}

	clear_item_if_restricted(l, &item);
	set_item_kill_count_if_unsealable(&item);
	return item;
}

item_t check_rare_specs_and_create_rare_box_item(lobby_t* l, pt_bb_entry_t* ent, sfmt_t* rng, uint8_t area_norm, uint8_t section_id) {
	item_t item = { 0 };
	if (!are_rare_drops_allowed(l)) {
		return item;
	}

	rt_table_t* rare_specs = get_rt_table_bb(l->episode, l->challenge, l->difficulty, section_id);

	for (size_t x = 0; x < rare_specs->box_count; x++) {
		if (rare_specs->box_areas[x] == area_norm) {
			PackedDrop_t spec = rare_specs->box_rares[x];

			if (spec.probability == 0) {
#ifdef DEBUG
				DBG_LOG("Area %d Box spec.probability %lf did not produce item %02hhX%02hhX%02hhX",
					area_norm, spec.probability, spec.item_code[0], spec.item_code[1], spec.item_code[2]);
#endif // DEBUG
				return item;
			}

			item = check_rate_and_create_rare_item(l, ent, rng, spec);
			if (!is_item_empty(&item)) {
#ifdef DEBUG
				DBG_LOG("Area %d Box spec.probability %lf produced item %02hhX%02hhX%02hhX",
					area_norm, spec.probability, spec.item_code[0], spec.item_code[1], spec.item_code[2]);

#endif // DEBUG
				return item;
			}
		}
	}
	return item;
}

item_t on_box_item_drop_with_norm_area(lobby_t* l, pt_bb_entry_t* ent, sfmt_t* rng, uint8_t area_norm, uint8_t section_id) {
	item_t item = check_rare_specs_and_create_rare_box_item(l, ent, rng, area_norm, section_id);

	if (is_item_empty(&item)) {
#ifdef DEBUG

		DBG_LOG("不是稀有");

#endif // DEBUG
		uint8_t item_class = get_rand_from_weighted_tables_2d_vertical(rng,
			ent->box_drop, area_norm, 7, 10);
		/* 二维表 原始表格 X轴 7列 Y轴 10行*/
		switch (item_class) {
		case BOX_TYPE_WEAPON: // 武器
			item.datab[0] = ITEM_TYPE_WEAPON;
			break;
		case BOX_TYPE_ARMOR: // 盔甲
			item.datab[0] = ITEM_TYPE_GUARD;
			item.datab[1] = ITEM_SUBTYPE_FRAME;
			break;
		case BOX_TYPE_SHIELD: // 盾牌
			item.datab[0] = ITEM_TYPE_GUARD;
			item.datab[1] = ITEM_SUBTYPE_BARRIER;
			break;
		case BOX_TYPE_UNIT: // 插件
			item.datab[0] = ITEM_TYPE_GUARD;
			item.datab[1] = ITEM_SUBTYPE_UNIT;
			break;
		case BOX_TYPE_TOOL: // 工具
			item.datab[0] = ITEM_TYPE_TOOL;
			break;
		case BOX_TYPE_MESETA: // 美赛塔
			item.datab[0] = ITEM_TYPE_MESETA;
			break;
		case BOX_TYPE_NOTHING: // 无掉落
			break;
		default:
			ERR_LOG("发生严重错误 item_class %d 超出界限", item_class);
			return item;
		}
		generate_common_item_variances(l, rng, area_norm, &item, ent);
	}

	/* 如果物品不为空 则对稀有物品添加未鉴定属性 */
	if (!is_item_empty(&item))
		set_item_identified_flag(l->challenge, &item);

	return item;
}

item_t on_box_item_drop(lobby_t* l, sfmt_t* rng, uint8_t area, uint8_t section_id) {
	item_t item = { 0 };
	uint8_t new_area = normalize_area_number(l, area) - 1;
#ifdef DEBUG
	DBG_LOG("new_area %d 新area %d", new_area, area);
#endif // DEBUG
	pt_bb_entry_t* ent = get_pt_data_bb(l->episode, l->challenge, l->difficulty, section_id);
	if (!ent) {
		ERR_LOG("%s Item_PT 不存在章节 %d 难度 %d 颜色 %d 的掉落", client_type[l->version].ver_name, l->episode, l->difficulty, section_id);
		return item;
	}
	item = on_box_item_drop_with_norm_area(l, ent, rng, area, section_id);
	return item;
}

item_t check_rare_spec_and_create_rare_enemy_item(lobby_t* l, pt_bb_entry_t* ent, sfmt_t* rng, uint32_t enemy_type, uint8_t section_id) {
	item_t item = { 0 };
	if (are_rare_drops_allowed(l) && (enemy_type > 0) && (enemy_type < 0x58)) {
		// Note: In the original implementation, enemies can only have one possible
		// rare drop. In our implementation, they can have multiple rare drops if
		// JSONRareItemSet is used (the other RareItemSet implementations never
		// return multiple drops for an enemy type).
		rt_table_t* rare_specs = get_rt_table_bb(l->episode, l->challenge, l->difficulty, section_id);

		PackedDrop_t spec = rare_specs->enemy_rares[enemy_type + 1];

		if (spec.probability == 0)
			return item;

		item = check_rate_and_create_rare_item(l, ent, rng, spec);
		if (!is_item_empty(&item)) {
#ifdef DEBUG
			DBG_LOG("Enemy %d spec.probability %lf produced item %02hhX%02hhX%02hhX",
				enemy_type, spec.probability, spec.item_code[0], spec.item_code[1], spec.item_code[2]);

#endif // DEBUG
			return item;
		}
#ifdef DEBUG
		DBG_LOG("Enemy spec.probability %lf did not produce item %02hhX%02hhX%02hhX",
			enemy_type, spec.probability, spec.item_code[0], spec.item_code[1], spec.item_code[2]);
		//for (size_t x = 0; x < 0x65; x++) {
		//}

#endif // DEBUG
	}
	return item;
}

item_t on_monster_item_drop_with_norm_area(lobby_t* l, pt_bb_entry_t* ent, sfmt_t* rng, uint32_t enemy_type, uint8_t norm_area, uint8_t section_id) {
	item_t item = { 0 };
	if (enemy_type > 0x58) {
		ERR_LOG("%s 无效(%d %d %d %d) Enemy Type: %d", get_lobby_leader_describe(l), l->episode, l->challenge, l->difficulty, section_id, enemy_type);
		return item;
	}
#ifdef DEBUG

	DBG_LOG("Enemy type: %" PRIX32, enemy_type);

#endif // DEBUG

	uint8_t type_drop_prob = ent->enemy_type_drop_probs[enemy_type];
	uint8_t drop_sample = rand_int(rng, 100);
	if (drop_sample >= type_drop_prob) {
#ifdef DEBUG
		DBG_LOG("enemy_type %d Drop not chosen (%hhu vs. %hhu)", enemy_type, drop_sample, type_drop_prob);
#endif // DEBUG
		return item;
	}

	item = check_rare_spec_and_create_rare_enemy_item(l, ent, rng, enemy_type, section_id);
	if (is_item_empty(&item)) {
		uint32_t item_class_determinant = should_allow_meseta_drops(l) ? rand_int(rng, 3) : (rand_int(rng, 2) + 1);

		uint32_t item_class;
		switch (item_class_determinant) {
		case 0:
			item_class = 5;
			break;
		case 1:
			item_class = 4;
			break;
		case 2:
			item_class = ent->enemy_item_classes[enemy_type];
			break;
		default:
			ERR_LOG("无效的物品类型列式");
		}

#ifdef DEBUG

		DBG_LOG("Rare drop not chosen; item class determinant is %" PRIu32 "; item class is %" PRIu32,
			item_class_determinant, item_class);

#endif // DEBUG

		switch (item_class) {
		case ENEMY_TYPE_WEAPON: // Weapon
			item.datab[0] = ITEM_TYPE_WEAPON;
			break;
		case ENEMY_TYPE_ARMOR: // Armor
			item.datab[0] = ITEM_TYPE_GUARD;
			item.datab[1] = ITEM_SUBTYPE_FRAME;
			break;
		case ENEMY_TYPE_SHIELD: // Shield
			item.datab[0] = ITEM_TYPE_GUARD;
			item.datab[1] = ITEM_SUBTYPE_BARRIER;
			break;
		case ENEMY_TYPE_UNIT: // Unit
			item.datab[0] = ITEM_TYPE_GUARD;
			item.datab[1] = ITEM_SUBTYPE_UNIT;
			break;
		case ENEMY_TYPE_TOOL: // Tool
			item.datab[0] = ITEM_TYPE_TOOL;
			break;
		case ENEMY_TYPE_MESETA: // Meseta
			item.datab[0] = ITEM_TYPE_MESETA;
			item.data2l = choose_meseta_amount(rng, ent->enemy_meseta_ranges, enemy_type) & 0xFFFF;
			break;
		default:
			return item;
		}

		if (item.datab[0] != ITEM_TYPE_MESETA) {
			generate_common_item_variances(l, rng, norm_area, &item, ent);
		}
	}

	return item;
}

item_t on_monster_item_drop(lobby_t* l, sfmt_t* rng, uint32_t enemy_type, uint8_t area, uint8_t section_id) {
	item_t item = { 0 };
	uint8_t new_area = normalize_area_number(l, area) - 1;
#ifdef DEBUG
	DBG_LOG("new_area %d 新area %d", new_area, area);
#endif // DEBUG
	pt_bb_entry_t* ent = get_pt_data_bb(l->episode, l->challenge, l->difficulty, section_id);
	if (!ent) {
		ERR_LOG("%s Item_PT 不存在章节 %d 难度 %d 颜色 %d 的掉落", client_type[l->version].ver_name, l->episode, l->difficulty, section_id);
		return item;
	}

	item = on_monster_item_drop_with_norm_area(l, ent, rng, get_pt_index(l->episode, enemy_type), area, section_id);
	return item;
}

item_t create_bb_box_item(lobby_t* l, pt_bb_entry_t* ent, sfmt_t* 随机因子, uint8_t normarea, uint8_t 颜色ID, uint8_t 物品类型) {
	static const uint8_t max_quantity[4] = { 1,  1,  1,  1 };
	static const uint8_t max_tech_lvl[4] = { 4,  7, 10, 15 };
	static const uint8_t max_anti_lvl[4] = { 2,  4,  6,  7 };
	item_t item = { 0 };
	uint8_t tmp_value = 0;
	item.datab[0] = 物品类型;
	errno_t err = 0;
	uint8_t 难度 = l->difficulty, 章节 = l->episode, 挑战 = l->challenge;

	/* 检索物品类型 */
	switch (item.datab[0]) {
	case ITEM_TYPE_WEAPON: // 武器
		item.datab[1] = rand_int(随机因子, 12) + 1; /* 01 - 0C 普通武器物品*/

		/* 9 以下都是 0/1 + 难度 9以上则 0-3（难度）类型ID*/
		if (item.datab[1] > 9) {
			item.datab[2] = 难度;
		}
		else
			item.datab[2] = rand_int(随机因子, 2) + 难度; /* 难度最大值 3 增加值 0 1 2 */

		/* 打磨值 0 - 10*/
		item.datab[3] = rand_int(随机因子, 11);

		/* 特殊攻击 0 - 10 配合难度 0 - 3*/
		//generate_common_weapon_special(随机因子, &item, normarea, ent);
		item.datab[4] = rand_int(随机因子, 11) + 难度;
		if (item.datab[4])
			item.datab[4] |= 0x80;

		/* datab[5] 在这里不涉及 礼物 未鉴定*/

		/* 生成属性*/
		size_t num_percentages = 0;
		while (num_percentages < 3) {
			/*0-5 涵盖所有属性*/
			for (size_t x = 0; x < 6; x++) {
				/* 后期设置调整 生成属性几率 TODO */
				if ((sfmt_genrand_uint32(随机因子) % 6) == 1) {
					/*+6 对应属性槽（结果分别为 6 8 10） +7对应数值（结果分别为 随机数1-20 1-35 1-45 1-50）*/
					item.datab[(num_percentages * 2) + 6] = (uint8_t)x;
					tmp_value = /*sfmt_genrand_uint32(随机因子) % 6 +*/ weapon_bonus_values[sfmt_genrand_uint32(随机因子) % 21];/* 0 - 5 % 0 - 19*/

					//if (tmp_value > 50)
					//	tmp_value = 50;

					//if (tmp_value < -50)
					//	tmp_value = -50;

					item.datab[(num_percentages * 2) + 7] = tmp_value;
					num_percentages++;
				}
			}
		}

		break;

	case ITEM_TYPE_GUARD: // 装甲
		pmt_guard_bb_t pmt_guard = { 0 };
		pmt_unit_bb_t pmt_unit = { 0 };
		item.datab[1] = 0;

		/* 必须是1或2 对应护甲或者护盾*/
		while (item.datab[1] == 0)
			item.datab[1] = sfmt_genrand_uint32(随机因子) & 3;

		switch (item.datab[1]) {
		case ITEM_SUBTYPE_FRAME://护甲
			/*护甲物品子类型*/
			item.datab[2] = (sfmt_genrand_uint32(随机因子) % 6) + (难度 * 6); /* 0x00 - 0x17 普通盔甲 随难度提升掉落物品类型 */
			if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
				ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
				break;
			}

			generate_common_armor_slot_count(随机因子, &item, ent);
			generate_common_armor_slots_and_bonuses(随机因子, &item, ent);
			/*随机槽位 0 - 4 */
			//item.datab[5] = sfmt_genrand_uint32(随机因子) % 5;
			///* DFP值 */
			//tmp_value = sfmt_genrand_uint32(随机因子) % pmt_guard.dfp_range + 1;
			//if (tmp_value < 0)
			//	tmp_value = 0;
			//item.datab[6] = tmp_value;

			///* EVP值 */
			//tmp_value = sfmt_genrand_uint32(随机因子) % pmt_guard.evp_range + 1;
			//if (tmp_value < 0)
			//	tmp_value = 0;
			//item.datab[8] = tmp_value;
			break;

		case ITEM_SUBTYPE_BARRIER://护盾 0 - 20 

			/*护盾物品子类型*/
			item.datab[2] = (sfmt_genrand_uint32(随机因子) % 6) + (难度 * 5); /* 0x00 - 0x14 普通护盾*/
			if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
				ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
				break;
			}

			generate_common_armor_slot_count(随机因子, &item, ent);
			generate_common_armor_slots_and_bonuses(随机因子, &item, ent);
			///* DFP值 */
			//tmp_value = sfmt_genrand_uint32(随机因子) % pmt_guard.dfp_range + 1;
			//if (tmp_value < 0)
			//	tmp_value = 0;
			//item.datab[6] = tmp_value;

			///* EVP值 */
			//tmp_value = sfmt_genrand_uint32(随机因子) % pmt_guard.evp_range + 1;
			//if (tmp_value < 0)
			//	tmp_value = 0;
			//item.datab[8] = tmp_value;
			break;

		case ITEM_SUBTYPE_UNIT://插件 不生成带属性的 省的麻烦 TODO 以后再做更详细的
			item.datab[2] = (sfmt_genrand_uint32(随机因子) % 2);
			if (err = pmt_lookup_unit_bb(item.datal[0], &pmt_unit)) {
				ERR_LOG("pmt_lookup_unit_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
				break;
			}

			float f1 = (float)(1.0 + ent->unit_maxes[normarea]);
			float f2 = rand_float_0_1_from_crypt(随机因子);
			generate_common_unit_variances(随机因子, (uint32_t)(f1 * f2) & 0xFF, &item);
			if (item.datab[2] == 0xFF) {
				clear_inv_item(&item);
			}
			//tmp_value = sfmt_genrand_uint32(随机因子) % 5;
			//item.datab[6] = unit_bonus_values[tmp_value][0];
			//item.datab[7] = unit_bonus_values[tmp_value][1];

			break;
		}
		break;

	case ITEM_TYPE_MAG:
		magitemstat_t stats;
		ItemMagStats_init(&stats, get_player_msg_color_set(l->clients[l->leader_id]));
		assign_mag_stats(&item, &stats);
		break;

	case ITEM_TYPE_TOOL: // 药品工具
		item.datab[1] = sfmt_genrand_uint32(随机因子) % 12;
		switch (item.datab[1]) {
		case ITEM_SUBTYPE_MATE:
		case ITEM_SUBTYPE_FLUID:
			switch (难度) {
			case GAME_TYPE_DIFFICULTY_NORMARL:
				item.datab[2] = 0;
				item.datab[5] = get_item_amount(&item, 1);
				break;

			case GAME_TYPE_DIFFICULTY_HARD:
				item.datab[2] = sfmt_genrand_uint32(随机因子) % 2;
				item.datab[5] = get_item_amount(&item, 1);
				break;

			case GAME_TYPE_DIFFICULTY_VERY_HARD:
				item.datab[2] = (sfmt_genrand_uint32(随机因子) % 2) + 1;
				item.datab[5] = get_item_amount(&item, 1);
				break;

			case GAME_TYPE_DIFFICULTY_ULTIMATE:
				item.datab[2] = 2;
				item.datab[5] = get_item_amount(&item, 1);
				break;
			}
			break;

		case ITEM_SUBTYPE_ANTI_TOOL:
			item.datab[2] = sfmt_genrand_uint32(随机因子) % 2;
			item.datab[5] = get_item_amount(&item, 1);
			break;

		case ITEM_SUBTYPE_GRINDER:
			item.datab[2] = sfmt_genrand_uint32(随机因子) % 3;
			item.datab[5] = get_item_amount(&item, 1);
			break;

			//case ITEM_SUBTYPE_MATERIAL:
			//    item.datab[2] = sfmt_genrand_uint32(随机因子) % 7;
			//    break;

		case ITEM_SUBTYPE_DISK:
			item.datab[4] = sfmt_genrand_uint32(随机因子) % 19;
			switch (item.datab[4]) {
			case TECHNIQUE_RYUKER:
			case TECHNIQUE_REVERSER:
				item.datab[2] = 0; // reverser & ryuker always level 1 这两个法术永远是1级
				break;
			case TECHNIQUE_ANTI:
				item.datab[2] = sfmt_genrand_uint32(随机因子) % max_anti_lvl[难度];
				break;
			case TECHNIQUE_FOIE:
			case TECHNIQUE_GIFOIE:
			case TECHNIQUE_RAFOIE:
			case TECHNIQUE_BARTA:
			case TECHNIQUE_GIBARTA:
			case TECHNIQUE_RABARTA:
			case TECHNIQUE_ZONDE:
			case TECHNIQUE_GIZONDE:
			case TECHNIQUE_RAZONDE:
			case TECHNIQUE_GRANTS:
			case TECHNIQUE_DEBAND:
			case TECHNIQUE_JELLEN:
			case TECHNIQUE_ZALURE:
			case TECHNIQUE_SHIFTA:
			case TECHNIQUE_RESTA:
			case TECHNIQUE_MEGID:
				item.datab[2] = sfmt_genrand_uint32(随机因子) % max_tech_lvl[难度];
				break;
			}
			break;
			//case ITEM_SUBTYPE_MATE:
			//case ITEM_SUBTYPE_FLUID:
		case ITEM_SUBTYPE_SOL_ATOMIZER:
		case ITEM_SUBTYPE_MOON_ATOMIZER:
		case ITEM_SUBTYPE_STAR_ATOMIZER:
			//case ITEM_SUBTYPE_ANTI_TOOL:
		case ITEM_SUBTYPE_TELEPIPE:
		case ITEM_SUBTYPE_TRAP_VISION:
		case ITEM_SUBTYPE_PHOTON:
			item.datab[5] = get_item_amount(&item, 1);
			break;
		}
	}

	return item;
}

item_t create_bb_box_waste_item(lobby_t* l, pt_bb_entry_t* ent, sfmt_t* 随机因子, uint32_t def0, uint32_t def1, uint32_t def2) {
	item_t item = { 0 };
	uint8_t tmp_value = 0;
	errno_t err = 0;
	uint8_t 难度 = l->difficulty, 章节 = l->episode, 挑战 = l->challenge;
	clear_inv_item(&item);

	item.datab[0] = (def0 >> 0x18) & 0x0F;
	item.datab[1] = (def0 >> 0x10) + ((item.datab[0] == ITEM_TYPE_WEAPON) || (item.datab[0] == ITEM_TYPE_GUARD));
	item.datab[2] = (def0 >> 8);

	switch (item.datab[0]) {
	case ITEM_TYPE_WEAPON:
		/* 打磨值 属性 EX */
		item.datab[3] = (def1 >> 0x18) & 0xFF;
		item.datab[4] = def0 & 0xFF;
		item.datab[6] = (def1 >> 8) & 0xFF;
		item.datab[7] = def1 & 0xFF;
		item.datab[8] = (def2 >> 0x18) & 0xFF;
		item.datab[9] = (def2 >> 0x10) & 0xFF;
		item.datab[10] = (def2 >> 8) & 0xFF;
		item.datab[11] = def2 & 0xFF;


		/* 打磨值 0 - 7*/
		if (sfmt_genrand_uint32(随机因子) % 2)
			item.datab[3] = sfmt_genrand_uint32(随机因子) % 8 + 1;

		/* 特殊攻击 0 - 10 配合难度 0 - 3*/
		if (sfmt_genrand_uint32(随机因子) % 2)
			item.datab[4] = sfmt_genrand_uint32(随机因子) % 11 + 难度;

		if (item.datab[4])
			item.datab[4] |= 0x80;
		/* datab[5] 在这里不涉及 礼物 未鉴定*/

		/* 生成属性*/
		size_t num_percentages = 0;
		while (num_percentages < 3) {
			/*0-5 涵盖所有属性*/
			for (size_t x = 0; x < 6; x++) {
				/* 后期设置调整 生成属性几率 TODO */
				if ((sfmt_genrand_uint32(随机因子) % 4) == 1) {
					/*+6 对应属性槽（结果分别为 6 8 10） +7对应数值（结果分别为 随机数1-20 1-35 1-45 1-50）*/
					item.datab[(num_percentages * 2) + 6] = (uint8_t)x;
					tmp_value = /*sfmt_genrand_uint32(随机因子) % 6 + */weapon_bonus_values[sfmt_genrand_uint32(随机因子) % 21];/* 0 - 5 % 0 - 19*/

					//if (tmp_value > 50)
					//	tmp_value = 50;

					//if (tmp_value < -50)
					//	tmp_value = -50;

					item.datab[(num_percentages * 2) + 7] = tmp_value;
					num_percentages++;
				}
			}
		}
		break;

	case ITEM_TYPE_GUARD:
		switch (item.datab[1]) {
		case ITEM_SUBTYPE_FRAME://护甲
		case ITEM_SUBTYPE_BARRIER://护盾 0 - 20 
			pmt_guard_bb_t pmt_guard = { 0 };
			if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
				ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
				break;
			}

			generate_common_armor_slot_count(随机因子, &item, ent);
			generate_common_armor_slots_and_bonuses(随机因子, &item, ent);
			break;

		case ITEM_SUBTYPE_UNIT://插件
			pmt_unit_bb_t pmt_unit = { 0 };
			if (err = pmt_lookup_unit_bb(item.datal[0], &pmt_unit)) {
				ERR_LOG("pmt_lookup_unit_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
				break;
			}

			if (sfmt_genrand_uint32(随机因子) % 2) {
				tmp_value = sfmt_genrand_uint32(随机因子) % 5;
				item.datab[6] = unit_bonus_values[tmp_value][0];
				item.datab[7] = unit_bonus_values[tmp_value][1];
			}
			break;
		}

		break;

	case ITEM_TYPE_MAG:
		magitemstat_t stats;
		ItemMagStats_init(&stats, get_player_msg_color_set(l->clients[l->leader_id]));
		assign_mag_stats(&item, &stats);
		break;
	case ITEM_TYPE_TOOL:
		if (item.datab[1] == ITEM_SUBTYPE_DISK) {
			item.datab[4] = def0 & 0xFF;
		}
		item.datab[5] = get_item_amount(&item, 1);
		break;
	case ITEM_TYPE_MESETA:
		item.data2l = ((def1 >> 0x10) & 0xFFFF) * 10;
		break;

	default:
		ERR_LOG("无效物品类型 0x%02X", item.datab[0]);
	}

	return item;
}

/* 特殊箱子掉落 */
item_t on_specialized_box_item_drop(lobby_t* l, sfmt_t* rng, uint8_t area, uint32_t def0, uint32_t def1, uint32_t def2) {
	item_t item = { 0 };
	/* 初始化物品数据 */
	clear_inv_item(&item);
	item.datab[0] = (def0 >> 0x18) & 0x0F;
	/* 随机选择掉率普通物品还是稀有物品 */
	uint32_t choice_rng = rand_int(rng, 2); /* 50% 几率选择掉落垃圾物品还是其他物品 */
	/* 获取房主玩家的颜色ID */
	uint8_t section_id = get_lobby_leader_section(l);

	pt_bb_entry_t* ent = get_pt_data_bb(l->episode, l->challenge, l->difficulty, section_id);
	if (!ent) {
		ERR_LOG("%s Item_PT 不存在章节 %d 难度 %d 颜色 %d 的掉落", client_type[l->version].ver_name, l->episode, l->difficulty, section_id);
		return item;
	}

	uint8_t normarea = get_pt_data_area_bb(l->episode, area);

	if (choice_rng)
		item = create_bb_box_item(l, ent, rng, normarea, section_id, item.datab[0]);
	else
		item = create_bb_box_waste_item(l, ent, rng, def0, def1, def2);
	
	return item;
}