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

#include <f_logs.h>

#include "pso_items.h"
#include "pso_character.h"

/* 初始化物品数据 */
void clear_item(item_t* item) {
	item->datal[0] = 0;
	item->datal[1] = 0;
	item->datal[2] = 0;
	item->item_id = EMPTY_STRING;
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
	tmp_item->item_id = 0xFFFFFFFF;
	tmp_item->data2l = item[3];

	return true;
}

/* 初始化玩家背包数据 */
void clear_iitem(iitem_t* iitem) {
	iitem->present = LE16(0x0000);
	iitem->extension_data1 = 0;
	iitem->extension_data2 = 0;
	iitem->flags = 0;
	clear_item(&iitem->data);
}

/* 初始化银行背包数据 */
void clear_bitem(bitem_t* bitem) {
	clear_item(&bitem->data);
	bitem->show_flags = 0;
	bitem->amount = 0;
}

size_t primary_identifier(item_t* item) {
	// The game treats any item starting with 04 as Meseta, and ignores the rest
	// of data1 (the value is in data2)
	switch (item->datab[0])
	{
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

// TODO 需要客户端支持各种堆叠
size_t max_stack_size_for_item(uint8_t data0, uint8_t data1) {

	switch (data0) {
	case ITEM_TYPE_MESETA:
		return 999999;

	case ITEM_TYPE_TOOL:
		switch (data1) {
			/* 支持大量堆叠 99*/
		case ITEM_SUBTYPE_MATE:
		case ITEM_SUBTYPE_FLUID:
		case ITEM_SUBTYPE_SOL_ATOMIZER:
		case ITEM_SUBTYPE_MOON_ATOMIZER:
		case ITEM_SUBTYPE_STAR_ATOMIZER:
		case ITEM_SUBTYPE_ANTI:
		case ITEM_SUBTYPE_TELEPIPE:
		case ITEM_SUBTYPE_TRAP_VISION:
		case ITEM_SUBTYPE_GRINDER:
		case ITEM_SUBTYPE_MATERIAL:
		case ITEM_SUBTYPE_MAG_CELL1:
		case ITEM_SUBTYPE_MONSTER_LIMBS:
		case ITEM_SUBTYPE_MAG_CELL2:
		case ITEM_SUBTYPE_ADD_SLOT:
		case ITEM_SUBTYPE_PHOTON:
			return 99;

		case ITEM_SUBTYPE_DISK:
			return 1;


		default:
			return 99;
		}

		break;
	}

	return 1;
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

const char* item_get_name_by_code(item_code_t code, int version) {
	item_map_t* cur = item_list;
	//TODO 未完成多语言物品名称

	(void)version;

	const char* unknow_name = "未识别物品名称";

	/* Take care of mags so that we'll match them properly... */
	if ((code & 0xFF) == 0x02) {
		code &= 0xFFFF;
	}

	/* Look through the list for the one we want */
	while (cur->code != Item_NoSuchItem) {
		if (cur->code == code) {
			return cur->name;
		}

		++cur;
	}

	/* No item found... */
	return unknow_name;
}

const char* bbitem_get_name_by_code(bbitem_code_t code, int version) {
	bbitem_map_t* cur = bbitem_list_en;
	//TODO 未完成多语言物品名称
	(void)version;

	const char* unknow_name = "未识别物品名称";

	//TODO 未完成多语言物品名称
	int32_t languageCheck = 1;
	if (languageCheck) {
		cur = bbitem_list_cn;
	}

	/* Take care of mags so that we'll match them properly... */
	if ((code & 0xFF) == ITEM_TYPE_MAG) {
		code &= 0xFFFF;
	}

	/* Look through the list for the one we want */
	while (cur->code != Item_NoSuchItem) {
		if (cur->code == code) {
			return cur->name;
		}

		++cur;
	}

	//ERR_LOG("物品ID不存在 %06X", cur->code);

	/* No item found... */
	return unknow_name;
}

/* 获取物品名称 */
const char* item_get_name(item_t* item, int version) {
	uint32_t code = item->datab[0] | (item->datab[1] << 8) |
		(item->datab[2] << 16);

	/* 获取相关物品参数对比 */
	switch (item->datab[0]) {
	case ITEM_TYPE_WEAPON:  /* 武器 */
		if (item->datab[5]) {
			code = (item->datab[5] << 8);
		}
		break;

	case ITEM_TYPE_GUARD:  /* 装甲 */
		if (item->datab[1] != ITEM_SUBTYPE_UNIT && item->datab[3]) {
			code = code | (item->datab[3] << 16);
		}
		//printf("数据1: %02X 数据3: %02X\n", item->item_data1[1], item->item_data1[3]);
		break;

	case ITEM_TYPE_MAG:  /* 玛古 */
		if (item->datab[1] == 0x00 && item->datab[2] >= 0xC9) {
			code = 0x02 | (((item->datab[2] - 0xC9) + 0x2C) << 8);
		}
		break;

	case ITEM_TYPE_TOOL: /* 物品 */
		if (code == 0x060D03 && item->datab[3]) {
			code = 0x000E03 | ((item->datab[3] - 1) << 16);
		}
		break;

	case ITEM_TYPE_MESETA: /* 美赛塔 */
		break;

	default:
		ERR_LOG("未找到物品类型");
		break;
	}

	if (version == 5)
		return bbitem_get_name_by_code((bbitem_code_t)code, version);
	else
		return item_get_name_by_code((item_code_t)code, version);
}

int16_t get_armor_or_shield_defense_bonus(const item_t* item) {
	return item->dataw[3];
}

int16_t get_common_armor_evasion_bonus(const item_t* item) {
	return item->dataw[4];
}

int16_t get_unit_bonus(const item_t* item) {
	return item->dataw[3];
}

/* 打印物品数据 */
void print_item_data(item_t* item, int version) {
	ITEM_LOG("物品:(ID %d / %08X) %s",
		item->item_id, item->item_id, item_get_name(item, version));
	ITEM_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
}

/* 打印背包物品数据 */
void print_iitem_data(iitem_t* iitem, int item_index, int version) {
	ITEM_LOG("背包物品:(ID %d / %08X) %s",
		iitem->data.item_id, iitem->data.item_id, item_get_name(&iitem->data, version));
	ITEM_LOG(""
		"槽位 (%d) "
		"(%s) %04X "
		"鉴定 %d %d"
		"(%s) Flags %08X",
		item_index,
		((iitem->present & LE32(0x0001)) ? "已占槽位" : "未占槽位"),
		iitem->present,
		iitem->extension_data1,
		iitem->extension_data2,
		((iitem->flags & LE32(0x00000008)) ? "已装备" : "未装备"),
		iitem->flags
	);
	ITEM_LOG("背包数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		iitem->data.datab[0], iitem->data.datab[1], iitem->data.datab[2], iitem->data.datab[3],
		iitem->data.datab[4], iitem->data.datab[5], iitem->data.datab[6], iitem->data.datab[7],
		iitem->data.datab[8], iitem->data.datab[9], iitem->data.datab[10], iitem->data.datab[11],
		iitem->data.data2b[0], iitem->data.data2b[1], iitem->data.data2b[2], iitem->data.data2b[3]);
}

/* 打印银行物品数据 */
void print_bitem_data(bitem_t* bitem, int item_index, int version) {
	ITEM_LOG("银行物品:(ID %d / %08X) %s",
		bitem->data.item_id, bitem->data.item_id, item_get_name(&bitem->data, version));
	ITEM_LOG(""
		"槽位 (%d) "
		"(%s) %04X "
		"(%s) Flags %04X",
		item_index,
		((max_stack_size_for_item(bitem->data.datab[0], bitem->data.datab[1]) > 1) ? "堆叠" : "单独"),
		bitem->amount,
		((bitem->show_flags & LE32(0x0001)) ? "显示" : "隐藏"),
		bitem->show_flags
	);
	ITEM_LOG("银行数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		bitem->data.datab[0], bitem->data.datab[1], bitem->data.datab[2], bitem->data.datab[3],
		bitem->data.datab[4], bitem->data.datab[5], bitem->data.datab[6], bitem->data.datab[7],
		bitem->data.datab[8], bitem->data.datab[9], bitem->data.datab[10], bitem->data.datab[11],
		bitem->data.data2b[0], bitem->data.data2b[1], bitem->data.data2b[2], bitem->data.data2b[3]);
}

void print_biitem_data(void* data, int item_index, int version, int inv, int err) {
	char* inv_text = ((inv == 1) ? "背包" : "银行");
	char* err_text = ((err == 1) ? "错误" : "玩家");

	if (data) {
		if (inv) {
			iitem_t* iitem = (iitem_t*)data;

			ITEM_LOG("%s X %s物品:(ID %d / %08X) %s", err_text, inv_text,
				iitem->data.item_id, iitem->data.item_id, item_get_name(&iitem->data, version));

			ITEM_LOG(""
				"槽位 (%d) "
				"(%s) %04X "
				"鉴定 %d %d"
				"(%s) Flags %08X",
				item_index,
				((iitem->present == 0x0001) ? "已占槽位" : "未占槽位"),
				iitem->present,
				iitem->extension_data1,
				iitem->extension_data2,
				((iitem->flags & LE32(0x00000008)) ? "已装备" : "未装备"),
				iitem->flags
			);

			ITEM_LOG("%s数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
				inv_text,
				iitem->data.datab[0], iitem->data.datab[1], iitem->data.datab[2], iitem->data.datab[3],
				iitem->data.datab[4], iitem->data.datab[5], iitem->data.datab[6], iitem->data.datab[7],
				iitem->data.datab[8], iitem->data.datab[9], iitem->data.datab[10], iitem->data.datab[11],
				iitem->data.data2b[0], iitem->data.data2b[1], iitem->data.data2b[2], iitem->data.data2b[3]);
		}
		else {
			bitem_t* bitem = (bitem_t*)data;

			ITEM_LOG("%s X %s物品:(ID %d / %08X) %s", err_text, inv_text,
				bitem->data.item_id, bitem->data.item_id, item_get_name(&bitem->data, version));

			ITEM_LOG(""
				"槽位 (%d) "
				"(%s) %04X "
				"(%s) Flags %04X",
				item_index,
				((bitem->amount == 0x0001) ? "堆叠" : "单独"),
				bitem->amount,
				((bitem->show_flags == 0x0001) ? "显示" : "隐藏"),
				bitem->show_flags
			);

			ITEM_LOG("%s数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
				inv_text,
				bitem->data.datab[0], bitem->data.datab[1], bitem->data.datab[2], bitem->data.datab[3],
				bitem->data.datab[4], bitem->data.datab[5], bitem->data.datab[6], bitem->data.datab[7],
				bitem->data.datab[8], bitem->data.datab[9], bitem->data.datab[10], bitem->data.datab[11],
				bitem->data.data2b[0], bitem->data.data2b[1], bitem->data.data2b[2], bitem->data.data2b[3]);
		}
	}
}
