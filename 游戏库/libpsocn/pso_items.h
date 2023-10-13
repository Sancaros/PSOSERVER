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

#ifndef PSO_ITEMS_H
#define PSO_ITEMS_H

#include <stdbool.h>
#include "f_logs.h"

#include "pso_player.h"
#include "pso_item_list.h"

// item_equip_flags ְҵװ����־ ����ʶ��ͬ����
#define PLAYER_EQUIP_FLAGS_NONE     false
#define PLAYER_EQUIP_FLAGS_OK       true
#define PLAYER_EQUIP_FLAGS_HUNTER   0x01   // Bit 1 ����
#define PLAYER_EQUIP_FLAGS_RANGER   0x02   // Bit 2 ǹ��
#define PLAYER_EQUIP_FLAGS_FORCE    0x04   // Bit 3 ��ʦ
#define PLAYER_EQUIP_FLAGS_HUMAN    0x08   // Bit 4 ����
#define PLAYER_EQUIP_FLAGS_DROID    0x10   // Bit 5 ������
#define PLAYER_EQUIP_FLAGS_NEWMAN   0x20   // Bit 6 ������
#define PLAYER_EQUIP_FLAGS_MALE     0x40   // Bit 7 ����
#define PLAYER_EQUIP_FLAGS_FEMALE   0x80   // Bit 8 Ů��
#define PLAYER_EQUIP_FLAGS_MAX      8

#define EQUIP_FLAGS          LE32(0x00000008)
#define UNEQUIP_FLAGS        LE32(0xFFFFFFF7)

/* ÿ��ְҵ��Ӧ��װ��FLAGS */
static uint8_t class_equip_flags[12] = {
    PLAYER_EQUIP_FLAGS_HUNTER  | PLAYER_EQUIP_FLAGS_HUMAN     | PLAYER_EQUIP_FLAGS_MALE,    // HUmar
    PLAYER_EQUIP_FLAGS_HUNTER  | PLAYER_EQUIP_FLAGS_NEWMAN    | PLAYER_EQUIP_FLAGS_FEMALE,  // HUnewearl
    PLAYER_EQUIP_FLAGS_HUNTER  | PLAYER_EQUIP_FLAGS_DROID     | PLAYER_EQUIP_FLAGS_MALE,    // HUcast
    PLAYER_EQUIP_FLAGS_RANGER  | PLAYER_EQUIP_FLAGS_HUMAN     | PLAYER_EQUIP_FLAGS_MALE,    // RAmar
    PLAYER_EQUIP_FLAGS_RANGER  | PLAYER_EQUIP_FLAGS_DROID     | PLAYER_EQUIP_FLAGS_MALE,    // RAcast
    PLAYER_EQUIP_FLAGS_RANGER  | PLAYER_EQUIP_FLAGS_DROID     | PLAYER_EQUIP_FLAGS_FEMALE,  // RAcaseal
    PLAYER_EQUIP_FLAGS_FORCE   | PLAYER_EQUIP_FLAGS_HUMAN     | PLAYER_EQUIP_FLAGS_FEMALE,  // FOmarl
    PLAYER_EQUIP_FLAGS_FORCE   | PLAYER_EQUIP_FLAGS_NEWMAN    | PLAYER_EQUIP_FLAGS_MALE,    // FOnewm
    PLAYER_EQUIP_FLAGS_FORCE   | PLAYER_EQUIP_FLAGS_NEWMAN    | PLAYER_EQUIP_FLAGS_FEMALE,  // FOnewearl
    PLAYER_EQUIP_FLAGS_HUNTER  | PLAYER_EQUIP_FLAGS_DROID     | PLAYER_EQUIP_FLAGS_FEMALE,  // HUcaseal
    PLAYER_EQUIP_FLAGS_FORCE   | PLAYER_EQUIP_FLAGS_HUMAN     | PLAYER_EQUIP_FLAGS_MALE,    // FOmar
    PLAYER_EQUIP_FLAGS_RANGER  | PLAYER_EQUIP_FLAGS_HUMAN     | PLAYER_EQUIP_FLAGS_FEMALE   // RAmarl
};

#define MAX_LOBBY_SAVED_ITEMS           3000

#define ITEM_BASE_STAR_DEFAULT          0
#define ITEM_RARE_THRESHOLD             9

#define ITEM_ID_MESETA                  0xFFFFFFFF

/* Item buckets. Each item gets put into one of these buckets when in the list,
   in order to make searching the list slightly easier. These are all based on
   the least significant byte of the item code. */
#define ITEM_TYPE_WEAPON                0x00
#define ITEM_TYPE_GUARD                 0x01
#define ITEM_TYPE_MAG                   0x02
#define ITEM_TYPE_TOOL                  0x03
#define ITEM_TYPE_MESETA                0x04

#define MESETA_IDENTIFIER               0x00040000

/* ITEM_TYPE_GUARD items are actually slightly more specialized, and fall into
   three subtypes of their own. These are the second least significant byte in
   the item code. */
#define ITEM_SUBTYPE_FRAME              0x01
#define ITEM_SUBTYPE_BARRIER            0x02
#define ITEM_SUBTYPE_UNIT               0x03

/* ITEM_TYPE_TOOL subtype */
#define ITEM_SUBTYPE_MATE               0x00 /* 10 �ѵ� */
#define ITEM_SUBTYPE_FLUID              0x01 /* 10 �ѵ� */
#define ITEM_SUBTYPE_DISK               0x02 /* 1  �ѵ� */

#define ITEM_SUBTYPE_SOL_ATOMIZER       0x03 /* 10 �ѵ� */
#define ITEM_SUBTYPE_MOON_ATOMIZER      0x04 /* 10 �ѵ� */
#define ITEM_SUBTYPE_STAR_ATOMIZER      0x05 /* 10 �ѵ� */
#define ITEM_SUBTYPE_ANTI_TOOL          0x06 /* 10 �ѵ� */
#define ITEM_SUBTYPE_TELEPIPE           0x07 /* 10 �ѵ� */
#define ITEM_SUBTYPE_TRAP_VISION        0x08 /* 10 �ѵ� */
#define ITEM_SUBTYPE_SCAPE_DOLL         0x09 /* 1  �ѵ� */

#define ITEM_SUBTYPE_GRINDER            0x0A /* 99 �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_MATERIAL           0x0B /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_POWER     0x00 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_MIND      0x01 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_EVADE     0x02 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_HP	    0x03 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_TP	    0x04 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_DEF	    0x05 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_LUCK	    0x06 /* 99 �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_MAG_CELL1          0x0C /* 99 �ѵ� */
#define ITEM_SUBTYPE_MONSTER_LIMBS      0x0D /* 99 �ѵ� */
#define ITEM_SUBTYPE_MAG_CELL2          0x0E /* 99 �ѵ� */
#define ITEM_SUBTYPE_ADD_SLOT           0x0F /* 99 �ѵ� */
#define ITEM_SUBTYPE_PHOTON             0x10 /* 99 �ѵ� */
#define ITEM_SUBTYPE_BOOK               0x11
#define ITEM_SUBTYPE_SERVER_ITEM1       0x12/*todo player_use_item*/
#define ITEM_SUBTYPE_PRESENT            0x13/*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2       0x14/*todo player_use_item*/
#define ITEM_SUBTYPE_PRESENT_EVENT      0x15
#define ITEM_SUBTYPE_DISK_MUSIC         0x16 /* 99 �ѵ� */
#define ITEM_SUBTYPE_HUNTER_REPORT      0x17/*todo player_use_item*/
#define ITEM_SUBTYPE_PART_OF_MAG_CELL   0x18
#define ITEM_SUBTYPE_GUILD_REWARD       0x19/*todo player_use_item*/
#define ITEM_SUBTYPE_UNKNOW_ITEM        0x1A

/* Default behaviors for the item lists. ITEM_DEFAULT_ALLOW means to accept any
   things NOT in the list read in by default, whereas ITEM_DEFAULT_REJECT causes
   unlisted items to be rejected. */
#define ITEM_DEFAULT_ALLOW      1
#define ITEM_DEFAULT_REJECT     0

   /* Version codes, as used in this part of the code. */
#define ITEM_VERSION_V1         0x01
#define ITEM_VERSION_V2         0x02
#define ITEM_VERSION_GC         0x04
#define ITEM_VERSION_XBOX       0x08
#define ITEM_VERSION_BB         0x10

/* ��Ϸ�̶��������Լӳ� */
static const int8_t weapon_bonus_values[21] = {
    50, 45, 40, 35, 30, 25, 20, 15, 10, 5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
};

static char* weapon_attrib[6] = { "��", "ԭ��", "����", "��е", "����", "����" };

typedef struct weapon_special {
    uint8_t id;
    const char* name;
    const char* cn_name;
} weapon_special_t;

static const weapon_special_t weapon_specials[] = {
	{0x00, "NULL", "��EX"},
    {0x01, "Draw", "��Ѫ"},
    {0x02, "Drain", "��Ѫ"},
    {0x03, "Fill", "��Ѫ"},
    {0x04, "Gush", "��Ѫ"},
    {0x05, "Heart", "��֮"},
    {0x06, "Mind", "��֮"},
    {0x07, "Soul", "��֮"},
    {0x08, "Geist", "��֮"},
    {0x09, "Master\'s", "��֮"},
    {0x0A, "Lord\'s", "��֮"},
    {0x0B, "King\'s", "��֮"},
    {0x0C, "Charge", "����"},
    {0x0D, "Spirit", "����"},
    {0x0E, "Berserk", "��Ѫ"},
    {0x0F, "Ice", "����"},
    {0x10, "Frost", "��˪"},
    {0x11, "Freeze", "����"},
    {0x12, "Blizzard", "����"},
    {0x13, "Bind", "С���"},
    {0x14, "Hold", "�����"},
    {0x15, "Seize", "ǿ���"},
    {0x16, "Arrest", "ȫ���"},
    {0x17, "Heat", "����"},
    {0x18, "Fire", "����"},
    {0x19, "Flame", "����"},
    {0x1A, "Burning", "����"},
    {0x1B, "Shock", "����"},
    {0x1C, "Thunder", "�׻�"},
    {0x1D, "Storm", "�ױ�"},
    {0x1E, "Tempest", "����"},
    {0x1F, "Dim", "����"},
    {0x20, "Shadow", "��Ӱ"},
    {0x21, "Dark", "����"},
    {0x22, "Hell", "����"},
    {0x23, "Panic", "����"},
    {0x24, "Riot", "����"},
    {0x25, "Havoc", "�ƽ�"},
    {0x26, "Chaos", "����"},
    {0x27, "Devil\'s", "ħ���"},
    {0x28, "Demon\'s", "��ħ��"},
};

typedef struct s_rank_special {
    uint8_t id;
    const char* name;
    const char* cn_name;
} s_rank_special_t;

static const s_rank_special_t s_rank_specials[] = {
	{0x00, "NULL", "��EX"},
    {0x01, "Jellen", "����"},
    {0x02, "Zalure", "����"},
	{0x03, "HP Regeneration", "HP����"},
	{0x04, "TP Regeneration", "TP����"},
    {0x05, "Burning", "����"},
    {0x06, "Tempest", "����"},
    {0x07, "Blizzard", "����"},
    {0x08, "Arrest", "ȫ���"},
    {0x09, "Chaos", "����"},
    {0x0A, "Hell", "����"},
    {0x0B, "Spirit", "����"},
    {0x0C, "Berserk", "��Ѫ"},
    {0x0D, "Demon\'s", "��ħ��"},
    {0x0E, "Gush", "��Ѫ"},
    {0x0F, "Geist", "��֮"},
    {0x10, "King\'s", "��֮"},
};

/* ��Ϸ�̶�������Լӳ� */
static const uint8_t unit_bonus_values[5][2] = {
    {0xFE, 0xFF},
    {0xFF, 0xFF},
    {0x00, 0x00},
    {0x01, 0x00},
    {0x02, 0x00}
};

static inline const char* get_mag_color_name(int color) {
	switch (color) {
	case 0:
		return "��ɫ";
	case 1:
		return "��ɫ";
	case 2:
		return "��ɫ";
	case 3:
		return "��ɫ";
	case 4:
		return "��ɫ";
	case 5:
		return "��ɫ";
	case 6:
		return "��ɫ";
	case 7:
		return "��ɫ";
	case 8:
		return "��ɫ";
	case 9:
		return "��ɫ";
	case 10:
		return "ʯ��ɫ";
	case 11:
		return "���ɫ";
	case 12:
		return "����ɫ";
	case 13:
		return "���ɫ";
	case 14:
		return "��ɫ";
	case 15:
		return "����ɫ";
	case 16:
		return "�ۺ�ɫ";
	case 17:
		return "����ɫ";
	default:
		return "δ֪��ɫ";
	}
}

static char item_des[4096] = { 0 };
static char item_attrib_des[4096] = { 0 };

static const uint8_t unit_attrib_val[5] = {
	0xFE,
	0xFF,
	0x00,
	0x01,
	0x02,
};

static const char* unit_attrib[5] = {
	"--",
	"-",
	"��",
	"+",
	"++"
};

/* ��ʼ����Ʒ���� */
void clear_inv_item(item_t* item);
void clear_bank_item(item_t* item);

bool create_tmp_item(const uint32_t* item, size_t item_size, item_t* tmp_item);

/* ��ʼ��������Ʒ���� */
void clear_iitem(iitem_t* iitem);

/* ��ʼ��������Ʒ���� */
void clear_bitem(bitem_t* bitem);

size_t isUnique(size_t* numbers, size_t size, size_t num);
size_t primary_code_identifier(const uint32_t code);
size_t primary_identifier(const item_t* i);

/* �ѵ����� */
bool is_stackable(const item_t* item);
size_t stack_size(const item_t* item);
size_t max_stack_size(const item_t* item);
size_t max_stack_size_for_item(uint8_t data0, uint8_t data1);
/* �����ڷ�����Ʒ���� */
uint32_t get_item_amount(item_t* item, uint32_t amount);

void clear_tool_item_if_invalid(item_t* item);
bool is_common_consumable(uint32_t primary_identifier);

/* ��ȡ��Ʒ���� */
const char* item_get_name(const item_t* item, int version, int languageCheck);
bool is_item_empty(item_t* item);
void set_armor_or_shield_defense_bonus(item_t* item, int16_t bonus);
int16_t get_armor_or_shield_defense_bonus(const item_t* item);
void set_common_armor_evasion_bonus(item_t* item, int16_t bonus);
int16_t get_common_armor_evasion_bonus(const item_t* item);
void set_unit_bonus(item_t* item, int16_t bonus);
int16_t get_unit_bonus(const item_t* item);
bool is_s_rank_weapon(const item_t* item);
bool compare_for_sort(item_t* itemDataA, item_t* itemDataB);

typedef void (*print_item)(const item_t* item, int version);

///* ��ӡ��Ʒ���� */
//void print_item_data(const item_t* item, int version);
//
///* ��ӡʰȡ��Ʒ���� */
//void print_pitem_data(const item_t* item, int version);
//
///* ��ӡ������Ʒ���� */
//void print_ditem_data(const item_t* item, int version);
//
///* ��ӡ������Ʒ���� */
//void print_titem_data(const item_t* item, int version);
//
///* ��ӡ������Ʒ���� */
//void print_iitem_data(const iitem_t* iitem, int item_index, int version);
//
///* ��ӡ������Ʒ���� */
//void print_bitem_data(const bitem_t* iitem, int item_index, int version);
//
//void print_biitem_data(void* data, int item_index, int version, int inv, int err);

char* get_item_describe(const item_t* item, int version);

/* ��ӡ��Ʒ���� */
inline void print_item_data(const item_t* item, int version) {
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

/* ��ӡ��Ʒʰȡ���� */
inline void print_pitem_data(const item_t* item, int version) {
	PICKS_LOG("��Ʒ: %s", get_item_describe(item, version));
	PICKS_LOG("���: 0x%08X",
		item->item_id);
	PICKS_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
	PICKS_LOG("------------------------------------------------------------");
}

/* ��ӡ��Ʒ�������� */
inline void print_ditem_data(const item_t* item, int version) {
	DROPS_LOG("��Ʒ: %s", get_item_describe(item, version));
	DROPS_LOG("���: 0x%08X",
		item->item_id);
	DROPS_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
	DROPS_LOG("------------------------------------------------------------");
}

/* ��ӡ��Ʒ�������� */
inline void print_titem_data(const item_t* item, int version) {
	TRADES_LOG("��Ʒ: %s", get_item_describe(item, version));
	TRADES_LOG("���: 0x%08X",
		item->item_id);
	TRADES_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
	TRADES_LOG("------------------------------------------------------------");
}

/* ��ӡ������Ʒ���� */
inline void print_iitem_data(const iitem_t* iitem, int item_index, int version) {
	ITEM_LOG("��Ʒ: %s", get_item_describe(&iitem->data, version));
	ITEM_LOG(""
		"��λ (%d) "
		"(%s) %04X "
		"(%s) "
		"Flags %08X",
		item_index,
		((iitem->present & LE32(0x0001)) ? "��ռ��λ" : "δռ��λ"),
		iitem->present,
		((iitem->flags & EQUIP_FLAGS) ? "��װ��" : "δװ��"),
		iitem->flags
	);
	ITEM_LOG("���: 0x%08X", iitem->data.item_id);
	ITEM_LOG("��������: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		iitem->data.datab[0], iitem->data.datab[1], iitem->data.datab[2], iitem->data.datab[3],
		iitem->data.datab[4], iitem->data.datab[5], iitem->data.datab[6], iitem->data.datab[7],
		iitem->data.datab[8], iitem->data.datab[9], iitem->data.datab[10], iitem->data.datab[11],
		iitem->data.data2b[0], iitem->data.data2b[1], iitem->data.data2b[2], iitem->data.data2b[3]);
	ITEM_LOG("------------------------------------------------------------");
}

/* ��ӡ������Ʒ���� */
inline void print_bitem_data(const bitem_t* bitem, int item_index, int version) {
	ITEM_LOG("��Ʒ: %s", get_item_describe(&bitem->data, version));
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
	ITEM_LOG("���: 0x%08X", bitem->data.item_id);
	ITEM_LOG("��������: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		bitem->data.datab[0], bitem->data.datab[1], bitem->data.datab[2], bitem->data.datab[3],
		bitem->data.datab[4], bitem->data.datab[5], bitem->data.datab[6], bitem->data.datab[7],
		bitem->data.datab[8], bitem->data.datab[9], bitem->data.datab[10], bitem->data.datab[11],
		bitem->data.data2b[0], bitem->data.data2b[1], bitem->data.data2b[2], bitem->data.data2b[3]);
	ITEM_LOG("------------------------------------------------------------");
}

/* ��ӡ���б�����Ʒ���� */
inline void print_biitem_data(void* data, int item_index, int version, int inv, int err) {
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
				"(%s) Flags %08X",
				item_index,
				((iitem->present == 0x0001) ? "��ռ��λ" : "δռ��λ"),
				iitem->present,
				((iitem->flags & EQUIP_FLAGS) ? "��װ��" : "δװ��"),
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

#endif /* !PSO_ITEMS_H */