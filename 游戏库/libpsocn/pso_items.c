/*
	�λ�֮���й� ���������� ��Ʒ����
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <f_logs.h>

#include "pso_items.h"
#include "pso_character.h"

/* ��ʼ����Ʒ���� */
void clear_inv_item(item_t* item) {
	item->datal[0] = 0;
	item->datal[1] = 0;
	item->datal[2] = 0;
	item->item_id = 0;
	item->data2l = 0;
}

void clear_bank_item(item_t* item) {
	item->datal[0] = 0;
	item->datal[1] = 0;
	item->datal[2] = 0;
	item->item_id = EMPTY_STRING;
	item->data2l = 0;
}

/* Ԥ����Ʒ���� */
bool create_tmp_item(const uint32_t* item, size_t item_size, item_t* tmp_item) {
	if (item_size < 4) {
		// �����������������ӡ������Ϣ�򷵻ش�����
		ERR_LOG("��ƷԤ��������� 4.");
		return false;
	}

	for (size_t i = 0; i < 3; i++) {
		if (item[i] != 0) {
			tmp_item->datal[i] = item[i];
		}
		else {
			// �����ֵ��������������ѡ����Ĭ��ֵ��ִ�������߼�
			tmp_item->datal[i] = 0;/* Ĭ��ֵ */;
		}
	}
	tmp_item->item_id = 0;
	tmp_item->data2l = item[3];

	return true;
}

/* ��ʼ����ұ������� */
void clear_iitem(iitem_t* iitem) {
	iitem->present = 0;
	iitem->extension_data1 = 0;
	iitem->extension_data2 = 0;
	iitem->flags = 0;
	clear_inv_item(&iitem->data);
}

/* ��ʼ�����б������� */
void clear_bitem(bitem_t* bitem) {
	clear_bank_item(&bitem->data);
	bitem->show_flags = 0;
	bitem->amount = 0;
}

size_t isUnique(size_t* numbers, size_t size, size_t num) {
	for (size_t i = 0; i < size; i++) {
		if (numbers[i] == num) {
			return 0;  // ���ֲ�Ψһ
		}
	}
	return 1;  // ����Ψһ
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

/* ��Ʒ���Ͷѵ������� */
size_t max_stack_size_for_item(uint8_t data0, uint8_t data1) {

	switch (data0) {
	case ITEM_TYPE_MESETA:
		return MAX_PLAYER_MESETA;

	case ITEM_TYPE_TOOL:
		switch (data1) {
		case ITEM_SUBTYPE_MATE:
		case ITEM_SUBTYPE_FLUID:
		case ITEM_SUBTYPE_SOL_ATOMIZER:
		case ITEM_SUBTYPE_MOON_ATOMIZER:
		case ITEM_SUBTYPE_STAR_ATOMIZER:
		case ITEM_SUBTYPE_ANTI_TOOL:
		case ITEM_SUBTYPE_TELEPIPE:
		case ITEM_SUBTYPE_TRAP_VISION:
			return 10;

		case ITEM_SUBTYPE_DISK:
		case ITEM_SUBTYPE_SCAPE_DOLL:
			return 1;

			/* ֧�ִ����ѵ� 99*/
		case ITEM_SUBTYPE_GRINDER:
		case ITEM_SUBTYPE_MATERIAL:
		case ITEM_SUBTYPE_MAG_CELL1:
		case ITEM_SUBTYPE_MONSTER_LIMBS:
		case ITEM_SUBTYPE_MAG_CELL2:
		case ITEM_SUBTYPE_ADD_SLOT:
		case ITEM_SUBTYPE_PHOTON:
		case ITEM_SUBTYPE_DISK_MUSIC:
			return 99;
		}

		break;
	}

	return 1;
}

/* �����ڷ�����Ʒ���� */
uint32_t get_item_amount(item_t* item, uint32_t amount) {
	if (is_stackable(item)) {
		if (item->datab[0] == ITEM_TYPE_MESETA) {
			item->data2l = amount > MAX_PLAYER_MESETA ? MAX_PLAYER_MESETA : amount;
			return item->data2l;
		}
		else if (item->datab[5] <= 0) {
			item->datab[5] = 1;
		}
		else if (amount > (uint8_t)max_stack_size(item)) {
			item->datab[5] = (uint8_t)max_stack_size(item);
		}
		else if (amount > 0){
			item->datab[5] = (uint8_t)amount;
		}
	}
	else if (item->datab[0] == 0x03 && item->datab[1] == 0x02) {
		item->datab[5] = 0x00;
	}

	return item->datab[5];
}

void clear_tool_item_if_invalid(item_t* item) {
	if ((item->datab[1] == 2) &&
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

	const char* unknow_name = "δʶ����Ʒ����";

	if (item->datal[0] == BBItem_Saber0)
		return "��";

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

	const char* unknow_name = "δʶ����Ʒ����";

	if (languageCheck && languageCheck < 10) {
		cur = bbitem_list_en;
	}

	if (item->datal[0] == BBItem_Saber0)
		return "��";

	/* Look through the list for the one we want */
	while (cur->code != BBItem_NoSuchItem) {
		if (cur->datab[0] == item->datab[0]) {
			if(item->datab[0] == ITEM_TYPE_TOOL && item->datab[1] == ITEM_SUBTYPE_DISK)
				if(cur->datab[2] == item->datab[4])
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

	//ERR_LOG("��ƷID������ %06X", cur->code);

	/* No item found... */
	return unknow_name;
}

/* ��ȡ��Ʒ���� */
const char* item_get_name(const item_t* item, int version, int languageCheck) {
//	uint32_t code = item->datab[0] | (item->datab[1] << 8) |
//		(item->datab[2] << 16);
//
//	/* ��ȡ�����Ʒ�����Ա� */
//	switch (item->datab[0]) {
//	case ITEM_TYPE_WEAPON:  /* ���� */
//		if (item->datab[5]) {
//			code = (item->datab[5] << 8);
//		}
//		break;
//
//	case ITEM_TYPE_GUARD:  /* װ�� */
//		if (item->datab[1] != ITEM_SUBTYPE_UNIT && item->datab[3]) {
//			code = code | (item->datab[3] << 16);
//		}
//		//printf("����1: %02X ����3: %02X\n", item->item_data1[1], item->item_data1[3]);
//		break;
//
//	case ITEM_TYPE_MAG:  /* ��� */
//		if (item->datab[1] == 0x00 && item->datab[2] >= 0xC9) {
//			code = 0x02 | (((item->datab[2] - 0xC9) + 0x2C) << 8);
//		}
//		break;
//
//	case ITEM_TYPE_TOOL: /* ��Ʒ */
//		if (code == 0x060D03 && item->datab[3]) {
//			code = 0x000E03 | ((item->datab[3] - 1) << 16);
//		}
//		break;
//
//	case ITEM_TYPE_MESETA: /* ������ */
//		break;
//
//#ifdef DEBUG
//	default:
//		ERR_LOG("δ�ҵ���Ʒ����");
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

/* ��ӡ��Ʒ���� */
void print_item_data(const item_t* item, int version) {
	ITEM_LOG("��Ʒ: %s", get_item_describe(item, version));
	ITEM_LOG("���: 0x%08X",
		item->item_id);
	ITEM_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
	ITEM_LOG("------------------------------------------------------------");
}

/* ��ӡ������Ʒ���� */
void print_iitem_data(const iitem_t* iitem, int item_index, int version) {
	ITEM_LOG("��Ʒ: %s", get_item_describe(&iitem->data, version));
	ITEM_LOG("���: 0x%08X", iitem->data.item_id);
	ITEM_LOG(""
		"��λ (%d) "
		"(%s) %04X "
		"���� %d %d"
		"(%s) "
		"Flags %08X",
		item_index,
		((iitem->present & LE32(0x0001)) ? "��ռ��λ" : "δռ��λ"),
		iitem->present,
		iitem->extension_data1,
		iitem->extension_data2,
		((iitem->flags & LE32(0x00000008)) ? "��װ��" : "δװ��"),
		iitem->flags
	);
	ITEM_LOG("��������: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		iitem->data.datab[0], iitem->data.datab[1], iitem->data.datab[2], iitem->data.datab[3],
		iitem->data.datab[4], iitem->data.datab[5], iitem->data.datab[6], iitem->data.datab[7],
		iitem->data.datab[8], iitem->data.datab[9], iitem->data.datab[10], iitem->data.datab[11],
		iitem->data.data2b[0], iitem->data.data2b[1], iitem->data.data2b[2], iitem->data.data2b[3]);
	ITEM_LOG("------------------------------------------------------------");
}

/* ��ӡ������Ʒ���� */
void print_bitem_data(const bitem_t* bitem, int item_index, int version) {
	ITEM_LOG("��Ʒ: %s", get_item_describe(&bitem->data, version));
	ITEM_LOG("���: 0x%08X", bitem->data.item_id);
	ITEM_LOG(""
		"��λ (%d) "
		"(%s) %04X "
		"(%s) "
		"Flags %04X",
		item_index,
		((max_stack_size_for_item(bitem->data.datab[0], bitem->data.datab[1]) > 1) ? "�ѵ�" : "����"),
		bitem->amount,
		((bitem->show_flags & LE32(0x0001)) ? "��ʾ" : "����"),
		bitem->show_flags
	);
	ITEM_LOG("��������: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		bitem->data.datab[0], bitem->data.datab[1], bitem->data.datab[2], bitem->data.datab[3],
		bitem->data.datab[4], bitem->data.datab[5], bitem->data.datab[6], bitem->data.datab[7],
		bitem->data.datab[8], bitem->data.datab[9], bitem->data.datab[10], bitem->data.datab[11],
		bitem->data.data2b[0], bitem->data.data2b[1], bitem->data.data2b[2], bitem->data.data2b[3]);
	ITEM_LOG("------------------------------------------------------------");
}

void print_biitem_data(void* data, int item_index, int version, int inv, int err) {
	char* inv_text = ((inv == 1) ? "����" : "����");
	char* err_text = ((err == 1) ? "����" : "���");

	if (data) {
		if (inv) {
			iitem_t* iitem = (iitem_t*)data;

			ITEM_LOG("%s X %s��Ʒ:(ID %d / %08X) %s", err_text, inv_text,
				iitem->data.item_id, iitem->data.item_id, item_get_name(&iitem->data, version, 0));

			ITEM_LOG(""
				"��λ (%d) "
				"(%s) %04X "
				"���� %d %d"
				"(%s) Flags %08X",
				item_index,
				((iitem->present == 0x0001) ? "��ռ��λ" : "δռ��λ"),
				iitem->present,
				iitem->extension_data1,
				iitem->extension_data2,
				((iitem->flags & LE32(0x00000008)) ? "��װ��" : "δװ��"),
				iitem->flags
			);

			ITEM_LOG("%s����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
				inv_text,
				iitem->data.datab[0], iitem->data.datab[1], iitem->data.datab[2], iitem->data.datab[3],
				iitem->data.datab[4], iitem->data.datab[5], iitem->data.datab[6], iitem->data.datab[7],
				iitem->data.datab[8], iitem->data.datab[9], iitem->data.datab[10], iitem->data.datab[11],
				iitem->data.data2b[0], iitem->data.data2b[1], iitem->data.data2b[2], iitem->data.data2b[3]);
		}
		else {
			bitem_t* bitem = (bitem_t*)data;

			ITEM_LOG("%s X %s��Ʒ:(ID %d / %08X) %s", err_text, inv_text,
				bitem->data.item_id, bitem->data.item_id, item_get_name(&bitem->data, version, 0));

			ITEM_LOG(""
				"��λ (%d) "
				"(%s) %04X "
				"(%s) Flags %04X",
				item_index,
				((bitem->amount == 0x0001) ? "�ѵ�" : "����"),
				bitem->amount,
				((bitem->show_flags == 0x0001) ? "��ʾ" : "����"),
				bitem->show_flags
			);

			ITEM_LOG("%s����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
				inv_text,
				bitem->data.datab[0], bitem->data.datab[1], bitem->data.datab[2], bitem->data.datab[3],
				bitem->data.datab[4], bitem->data.datab[5], bitem->data.datab[6], bitem->data.datab[7],
				bitem->data.datab[8], bitem->data.datab[9], bitem->data.datab[10], bitem->data.datab[11],
				bitem->data.data2b[0], bitem->data.data2b[1], bitem->data.data2b[2], bitem->data.data2b[3]);
		}
	}
}

const char* get_unit_bonus_describe(const item_t* item) {
	const char* unit_attrib[5][2] = {
		{ "\xFE", "--" },
		{ "\xFF", "-" },
		{ "\x00", "��" },
		{ "\x01", "+" },
		{ "\x02", "++" }
	};

	for (size_t x = 0; x < 5; x++) {
		if (item->datab[6] == *unit_attrib[x][0]) {
			return unit_attrib[x][1];
		}
	}

	return NULL; // ���û��ƥ��������NULL�������ʵ���Ĭ��ֵ
}

char* get_item_describe(const item_t* item, int version) {
	char* weapon_attrib[6] = { "��", "ԭ��", "����", "��е", "����", "����" };

	memset(item_des, 0, sizeof(item_des));

	/* ������Ʒ���� */
	switch (item->datab[0]) {
	case ITEM_TYPE_WEAPON: // ����
		sprintf(item_des, "%s +%d ����%d [%s%d/%s%d/%s%d]"
			, item_get_name(item, version, 0)
			, item->datab[3]
			, item->datab[4]
			, weapon_attrib[item->datab[6]], item->datab[7]
			, weapon_attrib[item->datab[8]], item->datab[9]
			, weapon_attrib[item->datab[10]], item->datab[11]
		);
		break;
	case ITEM_TYPE_GUARD: // װ��
		switch (item->datab[1]) {
		case ITEM_SUBTYPE_FRAME://����
			sprintf(item_des, "%s %d�� [%d+/%d+]"
				, item_get_name(item, version, 0)
				, item->datab[5]
				, item->datab[6]
				, item->datab[8]
			);
			break;
		case ITEM_SUBTYPE_BARRIER://����
			sprintf(item_des, "%s [%d+/%d+]"
				, item_get_name(item, version, 0)
				, item->datab[6]
				, item->datab[8]
			);
			break;

		case ITEM_SUBTYPE_UNIT://���
			const char* bonus_desc = get_unit_bonus_describe(item);
			if (bonus_desc == NULL) {
				sprintf(item_des, "%s", item_get_name(item, version, 0));
			}
			else {
				sprintf(item_des, "%s [%s]"
					, item_get_name(item, version, 0)
					, bonus_desc
				);
			}
			break;
		}
		break;

	case ITEM_TYPE_MAG:
		sprintf(item_des, "%s %s Lv%d ͬ�� %d ���� %d [%d/%d/%d/%d]"
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

	}

	return item_des;
}
