/*
	梦幻之星中国 舰船服务器 物品数据
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "pso_items.h"
#include "pso_character.h"

/* 初始化物品数据 */
void clear_inv_item(item_t* item) {
	item->datal[0] = 0;
	item->datal[1] = 0;
	item->datal[2] = 0;
	item->item_id = 0xFFFFFFFF;
	item->data2l = 0;
}

void clear_bank_item(item_t* item) {
	item->datal[0] = 0;
	item->datal[1] = 0;
	item->datal[2] = 0;
	item->item_id = 0xFFFFFFFF;
	item->data2l = 0;
}

/* 预设物品数据 */
bool create_tmp_item(const uint32_t* item, size_t item_size, item_t* tmp_item) {
	if (item_size < 4) {
		// 处理错误情况，例如打印错误消息或返回错误码
		ERR_LOG("物品预设参数少于 4.");
		return false;
	}

	for (size_t i = 0; i < 3; i++) {
		if (item[i] != 0) {
			tmp_item->datal[i] = item[i];
		}
		else {
			// 处理空值的情况，这里可以选择赋予默认值或执行其他逻辑
			tmp_item->datal[i] = 0;/* 默认值 */;
		}
	}
	tmp_item->item_id = 0;
	tmp_item->data2l = item[3];

	return true;
}

/* 初始化玩家背包数据 */
void clear_iitem(iitem_t* iitem) {
	iitem->present = 0;
	iitem->flags = 0;
	clear_inv_item(&iitem->data);
}

/* 初始化银行背包数据 */
void clear_bitem(bitem_t* bitem) {
	clear_bank_item(&bitem->data);
	bitem->show_flags = 0;
	bitem->amount = 0;
}

size_t isUnique(size_t* numbers, size_t size, size_t num) {
	for (size_t i = 0; i < size; i++) {
		if (numbers[i] == num) {
			return 0;  // 数字不唯一
		}
	}
	return 1;  // 数字唯一
}

size_t primary_code_identifier(const uint32_t code) {
	uint8_t parts[4] = { 0 };

	parts[0] = (uint8_t)(code & 0xFF);
	parts[1] = (uint8_t)((code >> 8) & 0xFF);
	parts[2] = (uint8_t)((code >> 16) & 0xFF);
	parts[3] = (uint8_t)((code >> 24) & 0xFF);

	// The game treats any item starting with 04 as Meseta, and ignores the rest
	// of data1 (the value is in data2)
	switch (parts[0]) {

	case ITEM_TYPE_MESETA:
		return 0x040000;

	case ITEM_TYPE_TOOL:
		if (parts[1] == ITEM_SUBTYPE_DISK) {
			return 0x030200;// Tech disk (data1[2] is level, so omit it)
		}
		break;

	case ITEM_TYPE_MAG:
		return 0x020000 | (parts[1] << 8); // Mag

	}

	return (parts[0] << 16) | (parts[1] << 8) | parts[2];
}

size_t primary_identifier(const item_t* item) {
	// The game treats any item starting with 04 as Meseta, and ignores the rest
	// of data1 (the value is in data2)
	switch (item->datab[0]) {

	case ITEM_TYPE_MESETA:
		return 0x040000;

	case ITEM_TYPE_TOOL:
		if (item->datab[1] == ITEM_SUBTYPE_DISK) {
			return 0x030200;// Tech disk (data1[2] is level, so omit it)
		}
		break;

	case ITEM_TYPE_MAG:
		return 0x020000 | (item->datab[1] << 8); // Mag

	}

	return (item->datab[0] << 16) | (item->datab[1] << 8) | item->datab[2];
}

bool is_stackable(const item_t* item) {
	return max_stack_size(item) > 1;
}

size_t stack_size(const item_t* item) {
	if (max_stack_size_for_item(item->datab[0], item->datab[1]) > 1) {
		return item->datab[5];
	}
	return 1;
}

size_t max_stack_size(const item_t* item) {
	return max_stack_size_for_item(item->datab[0], item->datab[1]);
}

/* 物品类型堆叠检测表函数 */
size_t max_stack_size_for_item(uint8_t data0, uint8_t data1) {
	/* 基本判断美赛塔 和 工具的堆叠 其他类型不适用 */
	switch (data0) {
	case ITEM_TYPE_MESETA:
		return MAX_PLAYER_MESETA;

	case ITEM_TYPE_TOOL:
		switch (data1) {
		case ITEM_SUBTYPE_DISK:
		case ITEM_SUBTYPE_SCAPE_DOLL:
		case ITEM_SUBTYPE_BOOK:
		case ITEM_SUBTYPE_PRESENT:
		case ITEM_SUBTYPE_SERVER_ITEM2:
		case ITEM_SUBTYPE_HUNTER_REPORT:
			return 1;

			/* 支持大量堆叠 99*/
		case ITEM_SUBTYPE_MATE:
		case ITEM_SUBTYPE_FLUID:
		case ITEM_SUBTYPE_TELEPIPE:
		case ITEM_SUBTYPE_TRAP_VISION:
		case ITEM_SUBTYPE_ANTI_TOOL:
		case ITEM_SUBTYPE_SOL_ATOMIZER:
		case ITEM_SUBTYPE_MOON_ATOMIZER:
		case ITEM_SUBTYPE_STAR_ATOMIZER:
		case ITEM_SUBTYPE_GRINDER:
		case ITEM_SUBTYPE_MATERIAL:
		case ITEM_SUBTYPE_MAG_CELL1:
		case ITEM_SUBTYPE_MONSTER_LIMBS:
		case ITEM_SUBTYPE_MAG_CELL2:
		case ITEM_SUBTYPE_ADD_SLOT:
		case ITEM_SUBTYPE_PHOTON:
		case ITEM_SUBTYPE_SERVER_ITEM1:
		case ITEM_SUBTYPE_PRESENT_EVENT:
		case ITEM_SUBTYPE_DISK_MUSIC:
		case ITEM_SUBTYPE_PART_OF_MAG_CELL:
		case ITEM_SUBTYPE_GUILD_REWARD:
			return 99;

		case ITEM_SUBTYPE_UNKNOW_ITEM:
			return 0;
		}

	default:
		return 1;
	}
}

/* 仅用于房间物品数量 */
uint8_t get_item_amount(item_t* item, uint32_t amount) {
	if (is_stackable(item)) {
		if (item->datab[0] == ITEM_TYPE_MESETA) {
			item->data2l = (amount > MAX_PLAYER_MESETA ? MAX_PLAYER_MESETA : amount);
			return 0;
		}
		else if (item->datab[5] <= 0) {
			item->datab[5] = 1;
		}
		else if (amount > (uint8_t)max_stack_size(item)) {
			item->datab[5] = (uint8_t)max_stack_size(item);
		}
		else if (amount > 0) {
			item->datab[5] = (uint8_t)amount;
		}
	}
	else if (item->datab[0] == ITEM_TYPE_TOOL && item->datab[1] == ITEM_SUBTYPE_DISK) {
		item->datab[5] = 0x00;
	}

	return item->datab[5];
}

void clear_tool_item_if_invalid(item_t* item) {
	if ((item->datab[1] == ITEM_SUBTYPE_DISK) &&
		((item->datab[2] > 0x1D) || (item->datab[4] > 0x12))) {
		clear_inv_item(item);
	}
}

bool is_common_consumable(uint32_t primary_identifier) {
	if (primary_identifier == 0x030200) {
		return false;
	}
	return (primary_identifier >= 0x030000) && (primary_identifier < 0x030A00);
}

void clear_after(item_t* this, size_t position, size_t count) {
	for (size_t x = position; x < count; x++) {
		this->datab[x] = 0;
	}
}

const char* item_get_name_by_code(const item_t* item) {
	item_map_t* cur = item_list;

	const char* unknow_name = "未识别物品名称";

	if (item->datal[0] == BBItem_Saber0)
		return "空";

	/* Look through the list for the one we want */
	while (cur->code != Item_NoSuchItem) {
		if (cur->datab[0] == item->datab[0]) {
			if (item->datab[0] == ITEM_TYPE_TOOL && item->datab[1] == ITEM_SUBTYPE_DISK)
				if (cur->datab[2] == item->datab[4])
					return cur->name;


			if (item->datab[0] == ITEM_TYPE_MAG) {
				if (cur->datab[1] == item->datab[1])
					return cur->name;
			}
			else if (cur->datab[1] == item->datab[1] && cur->datab[2] == item->datab[2]) {
				return cur->name;

			}
		}

		++cur;
	}

	/* No item found... */
	return unknow_name;
}

const char* bbitem_get_name_by_code(const item_t* item, int languageCheck) {
	bbitem_map_t* cur = bbitem_list_cn;

	const char* unknow_name = "未识别物品名称";

	if (languageCheck && languageCheck < 10) {
		cur = bbitem_list_en;
	}

	if (item->datal[0] == BBItem_Saber0)
		return "空";

	/* Look through the list for the one we want */
	while (cur->code != BBItem_NoSuchItem) {
		if (cur->datab[0] == item->datab[0]) {
			if (item->datab[0] == ITEM_TYPE_TOOL && item->datab[1] == ITEM_SUBTYPE_DISK && cur->datab[2] == item->datab[4]) {
				return cur->name;
			}
			else if (item->datab[0] == ITEM_TYPE_MAG) {
				if (cur->datab[1] == item->datab[1])
					return cur->name;
			}
			else if (cur->datab[1] == item->datab[1] && cur->datab[2] == item->datab[2]) {
				return cur->name;

			}
		}

		++cur;
	}

	//ERR_LOG("物品ID不存在 %06X", cur->code);

	/* No item found... */
	return unknow_name;
}

/* 获取物品名称 */
const char* item_get_name(const item_t* item, int version, int languageCheck) {
	//	uint32_t code = item->datab[0] | (item->datab[1] << 8) |
	//		(item->datab[2] << 16);
	//
	//	/* 获取相关物品参数对比 */
	//	switch (item->datab[0]) {
	//	case ITEM_TYPE_WEAPON:  /* 武器 */
	//		if (item->datab[5]) {
	//			code = (item->datab[5] << 8);
	//		}
	//		break;
	//
	//	case ITEM_TYPE_GUARD:  /* 装甲 */
	//		if (item->datab[1] != ITEM_SUBTYPE_UNIT && item->datab[3]) {
	//			code = code | (item->datab[3] << 16);
	//		}
	//		//printf("数据1: %02X 数据3: %02X\n", item->item_data1[1], item->item_data1[3]);
	//		break;
	//
	//	case ITEM_TYPE_MAG:  /* 玛古 */
	//		if (item->datab[1] == 0x00 && item->datab[2] >= 0xC9) {
	//			code = 0x02 | (((item->datab[2] - 0xC9) + 0x2C) << 8);
	//		}
	//		break;
	//
	//	case ITEM_TYPE_TOOL: /* 物品 */
	//		if (code == 0x060D03 && item->datab[3]) {
	//			code = 0x000E03 | ((item->datab[3] - 1) << 16);
	//		}
	//		break;
	//
	//	case ITEM_TYPE_MESETA: /* 美赛塔 */
	//		break;
	//
	//#ifdef DEBUG
	//	default:
	//		ERR_LOG("未找到物品类型");
	//		break;
	//
	//#endif // DEBUG
	//
	//	}

	if (version == 5)
		return bbitem_get_name_by_code(item, languageCheck);
	else
		return item_get_name_by_code(item);
}

bool is_item_empty(item_t* item) {
	return (item->datal[0] == 0) &&
		(item->datal[1] == 0) &&
		(item->datal[2] == 0) &&
		(item->data2l == 0);
}

void set_armor_or_shield_defense_bonus(item_t* item, int16_t bonus) {
	item->dataw[3] = bonus;
}

int16_t get_armor_or_shield_defense_bonus(const item_t* item) {
	return item->dataw[3];
}

void set_common_armor_evasion_bonus(item_t* item, int16_t bonus) {
	item->dataw[4] = bonus;
}

int16_t get_common_armor_evasion_bonus(const item_t* item) {
	return item->dataw[4];
}

void set_unit_bonus(item_t* item, int16_t bonus) {
	item->dataw[3] = bonus;
}

int16_t get_unit_bonus(const item_t* item) {
	return item->dataw[3];
}

int16_t get_sealed_item_kill_count(const item_t* item) {
	return ((item->datab[10] << 8) | item->datab[11]) & 0x7FFF;
}

void set_sealed_item_kill_count(item_t* item, int16_t v) {
	if (v > 0x7FFF) {
		item->dataw[5] = 0xFFFF;
	}
	else {
		item->datab[10] = (v >> 8) | 0x80;
		item->datab[11] = v;
	}
}

bool is_s_rank_weapon(const item_t* item) {
	if (item->datab[0] == 0) {
		if ((item->datab[1] > 0x6F) && (item->datab[1] < 0x89)) {
			return true;
		}
		if ((item->datab[1] > 0xA4) && (item->datab[1] < 0xAA)) {
			return true;
		}
	}
	return false;
}

bool compare_for_sort(item_t* itemDataA, item_t* itemDataB) {
	size_t z;
	for (z = 0; z < 12; z++) {
		if (itemDataA->datab[z] < itemDataB->datab[z]) {
			return false;
		}
		else if (itemDataA->datab[z] > itemDataB->datab[z]) {
			return true;
		}
	}

	for (z = 0; z < 4; z++) {
		if (itemDataA->data2b[z] < itemDataB->data2b[z]) {
			return false;
		}
		else if (itemDataA->data2b[z] > itemDataB->data2b[z]) {
			return true;
		}
	}

	return false;
}

const char* get_weapon_special_describe(uint8_t value, int lang) {
	if (value < 0x29 && value >= 0x00) {
		size_t len_weapon = ARRAYSIZE(weapon_common_specials);
		for (size_t i = 0; i < len_weapon; i++) {
			if (weapon_common_specials[i].id == value) {
				if (!lang)
					return weapon_common_specials[i].cn_name;
				else
					return weapon_common_specials[i].name;

			}
		}
	}
	return "无效EX";
}

const char* get_s_rank_special_describe(uint8_t value, int lang) {
	if (value < 0x11 && value >= 0x00) {
		size_t len_s_rank = ARRAYSIZE(weapon_srank_specials);
		for (size_t i = 0; i < len_s_rank; i++) {
			if (weapon_srank_specials[i].id == value) {
				if (!lang)
					return weapon_srank_specials[i].cn_name;
				else
					return weapon_srank_specials[i].name;

			}
		}
	}
	return "无效EX";
}

const char* get_unit_bonus_describe(const item_t* item) {
	size_t x = 0, attrib_val_len = ARRAYSIZE(unit_attrib_val);

	memset(item_attrib_des, 0, sizeof(item_attrib_des));

	for (x; x < attrib_val_len; x++) {
		if (item->datab[6] == unit_attrib_val[x]) {
			break;
		}
	}

	// 处理属性1
	if (x <= attrib_val_len)
		if (item->datab[10] & 0x80)
			sprintf(item_attrib_des, "%s 解封:%d", unit_attrib[x], get_sealed_item_kill_count(item));
		else
			sprintf(item_attrib_des, "%s", unit_attrib[x]);

	return item_attrib_des; // 如果没有匹配的项，返回NULL或其他适当的默认值
}

char* get_weapon_attrib_describe(const item_t* item) {
	size_t attrib_len = ARRAYSIZE(weapon_attrib);

	memset(item_attrib_des, 0, sizeof(item_attrib_des));

	char attrib1[50] = "空";
	char attrib2[50] = "空";
	char attrib3[50] = "空";

	// 处理属性1
	if (item->datab[6] < attrib_len)
		sprintf(attrib1, "%s%d", weapon_attrib[item->datab[6]], item->datab[7]);

	// 处理属性2
	if (item->datab[8] < attrib_len)
		sprintf(attrib2, "%s%d", weapon_attrib[item->datab[8]], item->datab[9]);

	// 处理属性3
	if (item->datab[10] < attrib_len)
		sprintf(attrib3, "%s%d", weapon_attrib[item->datab[10]], item->datab[11]);
	else if (item->datab[10] & 0x80) {
		sprintf(attrib3, "解封:%d", get_sealed_item_kill_count(item));
	}

	snprintf(item_attrib_des, sizeof(item_attrib_des), "[%s/%s/%s]", attrib1, attrib2, attrib3);

	return item_attrib_des;
}

char* get_item_describe(const item_t* item, int version) {

	memset(item_des, 0, sizeof(item_des));

	/* 检索物品类型 */
	switch (item->datab[0]) {
	case ITEM_TYPE_WEAPON: // 武器
		const char* special_describe;
		if (is_s_rank_weapon(item)) {
			special_describe = get_s_rank_special_describe(item->datab[2], 0);
		}
		else {
			special_describe = get_weapon_special_describe(item->datab[4], 0);
		}

		sprintf(item_des, "%s +%d %s EX(%s) %s"
			, item_get_name(item, version, 0)
			, item->datab[3]
			, item->datab[4] & 0x80 ? "未鉴定" : "已鉴定"
			, special_describe
			, is_s_rank_weapon(item) ? "" : get_weapon_attrib_describe(item)
		);
		break;
	case ITEM_TYPE_GUARD: // 装甲
		switch (item->datab[1]) {
		case ITEM_SUBTYPE_FRAME://护甲
			sprintf(item_des, "%s %d槽 [%d+/%d+]"
				, item_get_name(item, version, 0)
				, item->datab[5]
				, item->datab[6]
				, item->datab[8]
			);
			break;
		case ITEM_SUBTYPE_BARRIER://护盾
			sprintf(item_des, "%s [%d+/%d+]"
				, item_get_name(item, version, 0)
				, item->datab[6]
				, item->datab[8]
			);
			break;

		case ITEM_SUBTYPE_UNIT://插件
			sprintf(item_des, "%s [%s]"
				, item_get_name(item, version, 0)
				, get_unit_bonus_describe(item)
			);
			break;
		}
		break;

	case ITEM_TYPE_MAG:
		sprintf(item_des, "%s %s Lv%d 同步 %d 智商 %d [%d/%d/%d/%d]"
			, item_get_name(item, version, 0)
			, get_mag_color_name(item->data2b[3])
			, item->datab[2]
			, item->data2b[0]
			, item->data2b[1]
			, item->dataw[2] / 100
			, item->dataw[3] / 100
			, item->dataw[4] / 100
			, item->dataw[5] / 100
		);
		break;

	case ITEM_TYPE_TOOL:
		if (item->datab[1] == ITEM_SUBTYPE_DISK) {
			sprintf(item_des, "%s Lv%d"
				, get_technique_comment(item->datab[4])
				, item->datab[2] + 1
			);
		}
		else {
			sprintf(item_des, "%s x%d"
				, item_get_name(item, version, 0)
				, item->datab[5]
			);
		}
		break;

	case ITEM_TYPE_MESETA:
		sprintf(item_des, "%s x%d"
			, item_get_name(item, version, 0)
			, item->data2l
		);
		break;

	default:
		sprintf(item_des, "未解析物品 %s 0x%08X"
			, item_get_name(item, version, 0)
			, item->datal[0]
		);
		break;

	}

	return item_des;
}

const char* get_unit_unsealable_bonus_describe(const item_t* item) {
	size_t x = 0, attrib_val_len = ARRAYSIZE(unit_attrib_val);

	memset(item_attrib_des, 0, sizeof(item_attrib_des));

	for (x; x < attrib_val_len; x++) {
		if (item->datab[6] == unit_attrib_val[x]) {
			break;
		}
	}

	// 处理属性1
	if (x <= attrib_val_len)
		if (item->datab[10] & 0x80) {
			sprintf(item_attrib_des, "[解封:%d]", get_sealed_item_kill_count(item));
			return item_attrib_des;
		}

	return NULL; // 如果没有匹配的项，返回NULL或其他适当的默认值
}

char* get_weapon_unsealable_attrib_describe(const item_t* item) {

	memset(item_attrib_des, 0, sizeof(item_attrib_des));

	// 处理属性3
	//if (item->datab[10] & 0x80) {
	//}
	snprintf(item_attrib_des, sizeof(item_attrib_des), "[解封:%d]", get_sealed_item_kill_count(item));
	return item_attrib_des;
}

char* get_item_unsealable_describe(const item_t* item, int version) {

	memset(item_des, 0, sizeof(item_des));

	if (item->datab[0] == ITEM_TYPE_WEAPON) {
		sprintf(item_des, "%s %s"
			, item_get_name(item, version, 0)
			, is_s_rank_weapon(item) ? "" : get_weapon_unsealable_attrib_describe(item)
		);
	}
	else if ((item->datab[0] == ITEM_TYPE_GUARD) && (item->datab[1] == ITEM_SUBTYPE_UNIT)) {
		sprintf(item_des, "%s %s"
			, item_get_name(item, version, 0)
			, get_unit_unsealable_bonus_describe(item)
		);
	}
	else {
		sprintf(item_des, "未解析物品 %s 0x%08X"
			, item_get_name(item, version, 0)
			, item->datal[0]
		);
	}

	return item_des;
}