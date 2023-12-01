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
#include "pso_item_list_bb.h"

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

#define MAX_LOBBY_SAVED_ITEMS						3000

#define ITEM_BASE_STAR_DEFAULT						0
#define ITEM_RARE_THRESHOLD							9

#define ITEM_ID_MESETA								0xFFFFFFFF

/* �湻����� ��ʱû���ҵ��취ͳ������ ֻ����д������ */
#define ITEM_TYPE_WEAPON_SPECIAL_NUM				0x29

/* Item buckets. Each item gets put into one of these buckets when in the list,
   in order to make searching the list slightly easier. These are all based on
   the least significant byte of the item code. */
#define ITEM_TYPE_WEAPON							0x00
#define ITEM_TYPE_GUARD								0x01
#define ITEM_TYPE_MAG								0x02
#define ITEM_TYPE_TOOL								0x03
#define ITEM_TYPE_MESETA							0x04

#define MESETA_IDENTIFIER							0x00040000

/* ITEM_TYPE_GUARD items are actually slightly more specialized, and fall into
   three subtypes of their own. These are the second least significant byte in
   the item code. */
#define ITEM_SUBTYPE_FRAME							0x01
#define ITEM_SUBTYPE_BARRIER						0x02
#define ITEM_SUBTYPE_UNIT							0x03

/* ITEM_TYPE_TOOL subtype */
#define ITEM_SUBTYPE_MATE							0x00 /* 99 �ѵ� */
#define ITEM_SUBTYPE_FLUID              			0x01 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK               			0x02 /* 1  �ѵ� */

#define ITEM_SUBTYPE_SOL_ATOMIZER       			0x03 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MOON_ATOMIZER      			0x04 /* 99 �ѵ� */
#define ITEM_SUBTYPE_STAR_ATOMIZER      			0x05 /* 99 �ѵ� */
#define ITEM_SUBTYPE_ANTI_TOOL          			0x06 /* 99 �ѵ� */
#define ITEM_SUBTYPE_TELEPIPE           			0x07 /* 99 �ѵ� */
#define ITEM_SUBTYPE_TRAP_VISION        			0x08 /* 99 �ѵ� */
#define ITEM_SUBTYPE_SCAPE_DOLL         			0x09 /* 1  �ѵ� */

#define ITEM_SUBTYPE_GRINDER            			0x0A /* 99 �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_MATERIAL           			0x0B /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_POWER     			0x00 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_MIND      			0x01 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_EVADE     			0x02 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_HP	    			0x03 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_TP	    			0x04 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_DEF	    			0x05 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MATERIAL_LUCK	    			0x06 /* 99 �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_MAG_CELL1						0x0C /* 99 �ѵ� */
#define ITEM_SUBTYPE_MAG_CELL1_502					0x00 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MAG_CELL1_213					0x01 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MAG_CELL1_ROBOCHAO				0x02 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MAG_CELL1_OPA_OPA				0x03 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MAG_CELL1_PIAN					0x04 /* 99 �ѵ� */
#define ITEM_SUBTYPE_MAG_CELL1_CHAO					0x05 /* 99 �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_MONSTER_LIMBS					0x0D /* 99 �ѵ� */
#define ITEM_SUBTYPE_MAG_CELL2						0x0E /* 99 �ѵ� */
#define ITEM_SUBTYPE_ADD_SLOT						0x0F /* 99 �ѵ� */
#define ITEM_SUBTYPE_PHOTON							0x10 /* 99 �ѵ� */
#define ITEM_SUBTYPE_BOOK							0x11 /* 1  �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_SERVER_ITEM1					0x12 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_BADGE_W_BRONZE	0x00 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_BADGE_W_SILVER	0x01 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_BADGE_W_GOLD		0x02 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_BADGE_W_CRYSTAL	0x03 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_BADGE_W_STEAL		0x04 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_BADGE_W_ALUMINUM	0x05 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_BADGE_W_LEATHER	0x06 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_BADGE_W_BONE		0x07 /* 99 �ѵ� *//*todo player_use_item*/
//���µĿ�����Ҫ��PMT�ϳɵĲ���
#define ITEM_SUBTYPE_SERVER_ITEM1_LETTER_OF_APPRE	0x08 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_ITEM_TICKET		0x09 /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_VALENTINES_CHOCO	0x0A /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_NEW_YEARS_CARD	0x0B /* 99 �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM1_CHRISTMAS_CARD	0x0C /* 99 �ѵ� *//*todo player_use_item*/
//SERVER_ITEM1 ���в���û��д�� ��δ����ҩ��ô��
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_PRESENT						0x13 /* 1 �ѵ� ��� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_SERVER_ITEM2					0x14 /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_CHOCOLATE			0x00 /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_CANDY				0x01 /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_CAKE				0x02 /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_W_SILVER			0x03 /* 1  �ѵ� */
#define ITEM_SUBTYPE_SERVER_ITEM2_W_GOLD			0x04 /* 1  �ѵ� */
#define ITEM_SUBTYPE_SERVER_ITEM2_W_CRYSTAL			0x05 /* 1  �ѵ� */
#define ITEM_SUBTYPE_SERVER_ITEM2_W_STEEL			0x06 /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_W_ALUMINUM		0x07 /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_W_LEATHER			0x08 /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_W_BONE			0x09 /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_BOUQUET			0x0A /* 1  �ѵ� *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_DECOCTION			0x0B /* 1  �ѵ� *//*todo player_use_item*/
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_PRESENT_EVENT      			0x15 /* 99 �ѵ� */
#define ITEM_SUBTYPE_PRESENT_EVENT_CHRISTMAS 		0x00 /* 99 �ѵ� */
#define ITEM_SUBTYPE_PRESENT_EVENT_EASTER	 		0x01 /* 99 �ѵ� */
#define ITEM_SUBTYPE_PRESENT_EVENT_LANTERN	 		0x03 /* 99 �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_DISK_MUSIC         			0x16 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_1         		0x00 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_2         		0x01 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_3         		0x02 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_4         		0x03 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_5         		0x04 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_6         		0x05 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_7         		0x06 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_8         		0x07 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_9         		0x08 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_10         		0x09 /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_11         		0x0A /* 99 �ѵ� */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_12         		0x0B /* 99 �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_HUNTER_REPORT      			0x17 /* 1  �ѵ� *//*todo player_use_item*/
/* ��5�����˱��� ��δ֪ʹ�ú���� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_PART_OF_MAG_CELL   			0x18 /* 99 �ѵ� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_GUILD_REWARD       			0x19 /* 99 �ѵ� *//*todo player_use_item*/
/* ��4��������� ��δ֪ʹ�ú���� */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_UNKNOW_ITEM        			0x1A

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
	5, 10, 15, 20, 25, 25, 20, 15, 10, 5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
};

static char* weapon_attrib[6] = { "��", "ԭ��", "����", "��е", "����", "����" };

typedef struct weapon_special {
	uint8_t id;
	const char* name;
	const char* cn_name;
} weapon_special_t;

/* Weapon Attributes -- Stored in byte #4 of weapons. */
typedef enum psocn_weapon_common_attr {
    Weapon_Attr_None     = 0x00,
    Weapon_Attr_Draw     = 0x01,
    Weapon_Attr_Drain    = 0x02,
    Weapon_Attr_Fill     = 0x03,
    Weapon_Attr_Gush     = 0x04,
    Weapon_Attr_Heart    = 0x05,
    Weapon_Attr_Mind     = 0x06,
    Weapon_Attr_Soul     = 0x07,
    Weapon_Attr_Geist    = 0x08,
    Weapon_Attr_Masters  = 0x09,
    Weapon_Attr_Lords    = 0x0A,
    Weapon_Attr_Kings    = 0x0B,
    Weapon_Attr_Charge   = 0x0C,
    Weapon_Attr_Spirit   = 0x0D,
    Weapon_Attr_Berserk  = 0x0E,
    Weapon_Attr_Ice      = 0x0F,
    Weapon_Attr_Frost    = 0x10,
    Weapon_Attr_Freeze   = 0x11,
    Weapon_Attr_Blizzard = 0x12,
    Weapon_Attr_Bind     = 0x13,
    Weapon_Attr_Hold     = 0x14,
    Weapon_Attr_Seize    = 0x15,
    Weapon_Attr_Arrest   = 0x16,
    Weapon_Attr_Heat     = 0x17,
    Weapon_Attr_Fire     = 0x18,
    Weapon_Attr_Flame    = 0x19,
    Weapon_Attr_Burning  = 0x1A,
    Weapon_Attr_Shock    = 0x1B,
    Weapon_Attr_Thunder  = 0x1C,
    Weapon_Attr_Storm    = 0x1D,
    Weapon_Attr_Tempest  = 0x1E,
    Weapon_Attr_Dim      = 0x1F,
    Weapon_Attr_Shadow   = 0x20,
    Weapon_Attr_Dark     = 0x21,
    Weapon_Attr_Hell     = 0x22,
    Weapon_Attr_Panic    = 0x23,
    Weapon_Attr_Riot     = 0x24,
    Weapon_Attr_Havoc    = 0x25,
    Weapon_Attr_Chaos    = 0x26,
    Weapon_Attr_Devils   = 0x27,
    Weapon_Attr_Demons   = 0x28,
    Weapon_Attr_MAX      = 0x29
} psocn_weapon_common_attr_t;

/* ������ͨ���⹥�� -- �洢����Ʒdatab[4]. */
static const weapon_special_t weapon_common_specials[] = {
	{Weapon_Attr_None,    "NULL",      "��EX"},
	{Weapon_Attr_Draw,    "Draw",      "��Ѫ"},
	{Weapon_Attr_Drain,   "Drain",     "��Ѫ"},
	{Weapon_Attr_Fill,    "Fill",      "��Ѫ"},
	{Weapon_Attr_Gush,    "Gush",      "��Ѫ"},
	{Weapon_Attr_Heart,   "Heart",     "��֮"},
	{Weapon_Attr_Mind,    "Mind",      "��֮"},
	{Weapon_Attr_Soul,    "Soul",      "��֮"},
	{Weapon_Attr_Geist,   "Geist",     "��֮"},
	{Weapon_Attr_Masters, "Master's",  "��֮"},
	{Weapon_Attr_Lords,   "Lord's",    "��֮"},
	{Weapon_Attr_Kings,   "King's",    "��֮"},
	{Weapon_Attr_Charge,  "Charge",    "����"},
	{Weapon_Attr_Spirit,  "Spirit",    "����"},
	{Weapon_Attr_Berserk, "Berserk",   "��Ѫ"},
	{Weapon_Attr_Ice,     "Ice",       "����"},
	{Weapon_Attr_Frost,   "Frost",     "��˪"},
	{Weapon_Attr_Freeze,  "Freeze",    "����"},
	{Weapon_Attr_Blizzard,"Blizzard",  "����"},
	{Weapon_Attr_Bind,    "Bind",      "С���"},
	{Weapon_Attr_Hold,    "Hold",      "�����"},
	{Weapon_Attr_Seize,   "Seize",     "ǿ���"},
	{Weapon_Attr_Arrest,  "Arrest",    "ȫ���"},
	{Weapon_Attr_Heat,    "Heat",      "����"},
	{Weapon_Attr_Fire,    "Fire",      "����"},
	{Weapon_Attr_Flame,   "Flame",     "����"},
	{Weapon_Attr_Burning, "Burning",   "����"},
	{Weapon_Attr_Shock,   "Shock",     "����"},
	{Weapon_Attr_Thunder, "Thunder",   "�׻�"},
	{Weapon_Attr_Storm,   "Storm",     "�ױ�"},
	{Weapon_Attr_Tempest, "Tempest",   "����"},
	{Weapon_Attr_Dim,     "Dim",       "����"},
	{Weapon_Attr_Shadow,  "Shadow",    "��Ӱ"},
	{Weapon_Attr_Dark,    "Dark",      "����"},
	{Weapon_Attr_Hell,    "Hell",      "����"},
	{Weapon_Attr_Panic,   "Panic",     "����"},
	{Weapon_Attr_Riot,    "Riot",      "����"},
	{Weapon_Attr_Havoc,   "Havoc",     "�ƽ�"},
	{Weapon_Attr_Chaos,   "Chaos",     "����"},
	{Weapon_Attr_Devils,  "Devil's",   "ħ���"},
	{Weapon_Attr_Demons,  "Demon's",   "��ħ��"},
};

typedef enum psocn_weapon_srank_attr {
	Weapon_Srank_Attr_None		= 0x00,
	Weapon_Srank_Attr_Jellen	= 0x01,
	Weapon_Srank_Attr_Zalure	= 0x02,
	Weapon_Srank_Attr_HP_Regen	= 0x03,
	Weapon_Srank_Attr_TP_Regen	= 0x04,
	Weapon_Srank_Attr_Burning	= 0x05,
	Weapon_Srank_Attr_Tempest	= 0x06,
	Weapon_Srank_Attr_Blizzard	= 0x07,
	Weapon_Srank_Attr_Arrest	= 0x08,
	Weapon_Srank_Attr_Chaos		= 0x09,
	Weapon_Srank_Attr_Hell		= 0x0A,
	Weapon_Srank_Attr_Spirit	= 0x0B,
	Weapon_Srank_Attr_Berserk	= 0x0C,
	Weapon_Srank_Attr_Demons	= 0x0D,
	Weapon_Srank_Attr_Gush		= 0x0E,
	Weapon_Srank_Attr_Geist		= 0x0F,
	Weapon_Srank_Attr_Kings		= 0x10
} psocn_weapon_srank_attr_t;

/* ����S�����⹥�� -- �洢����Ʒdatab[2]. */
static const weapon_special_t weapon_srank_specials[] = {
	{Weapon_Srank_Attr_None,		"NULL",				"��EX"},
    {Weapon_Srank_Attr_Jellen,		"Jellen",			"����"},
    {Weapon_Srank_Attr_Zalure,		"Zalure",			"����"},
	{Weapon_Srank_Attr_HP_Regen,	"HP Regeneration",	"HP����"},
	{Weapon_Srank_Attr_TP_Regen,	"TP Regeneration",	"TP����"},
    {Weapon_Srank_Attr_Burning,		"Burning",			"����"},
    {Weapon_Srank_Attr_Tempest,		"Tempest",			"����"},
    {Weapon_Srank_Attr_Blizzard,	"Blizzard",			"����"},
    {Weapon_Srank_Attr_Arrest,		"Arrest",			"ȫ���"},
    {Weapon_Srank_Attr_Chaos,		"Chaos",			"����"},
    {Weapon_Srank_Attr_Hell,		"Hell",				"����"},
    {Weapon_Srank_Attr_Spirit,		"Spirit",			"����"},
    {Weapon_Srank_Attr_Berserk,		"Berserk",			"��Ѫ"},
    {Weapon_Srank_Attr_Demons,		"Demon\'s",			"��ħ��"},
    {Weapon_Srank_Attr_Gush,		"Gush",				"��Ѫ"},
    {Weapon_Srank_Attr_Geist,		"Geist",			"��֮"},
    {Weapon_Srank_Attr_Kings,		"King\'s",			"��֮"},
};

/* Ԫ�����ԣ�������������
���ǻ��� Tethealla �еı��
��ܿ�����ĳ�������ļ��е����ݣ�����Ӧ�ô��Ǹ������ļ��ж�ȡ����������������Ϳ����ˡ� */
static const psocn_weapon_common_attr_t weapon_common_special_attr_list[4][12] = {
	{
		Weapon_Attr_Draw, Weapon_Attr_Heart, Weapon_Attr_Ice,
		Weapon_Attr_Bind, Weapon_Attr_Heat, Weapon_Attr_Shock,
		Weapon_Attr_Dim, Weapon_Attr_Panic, Weapon_Attr_None,
		Weapon_Attr_None, Weapon_Attr_None, Weapon_Attr_None
	},
	{
		Weapon_Attr_Drain, Weapon_Attr_Mind, Weapon_Attr_Frost,
		Weapon_Attr_Hold, Weapon_Attr_Fire, Weapon_Attr_Thunder,
		Weapon_Attr_Shadow, Weapon_Attr_Riot, Weapon_Attr_Masters,
		Weapon_Attr_Charge, Weapon_Attr_None, Weapon_Attr_None
	},
	{
		Weapon_Attr_Fill, Weapon_Attr_Soul, Weapon_Attr_Freeze,
		Weapon_Attr_Seize, Weapon_Attr_Flame, Weapon_Attr_Storm,
		Weapon_Attr_Dark, Weapon_Attr_Havoc, Weapon_Attr_Lords,
		Weapon_Attr_Charge, Weapon_Attr_Spirit, Weapon_Attr_Devils
	},
	{
		Weapon_Attr_Gush, Weapon_Attr_Geist, Weapon_Attr_Blizzard,
		Weapon_Attr_Arrest, Weapon_Attr_Burning, Weapon_Attr_Tempest,
		Weapon_Attr_Hell, Weapon_Attr_Chaos, Weapon_Attr_Kings,
		Weapon_Attr_Charge, Weapon_Attr_Berserk, Weapon_Attr_Demons
	}
};

static const int weapon_common_special_attr_count[4] = { 8, 10, 12, 12 };

// ÿ����λ�����Ͱ�����ֵ
static const uint8_t common_unit_subtypes[9][4] = {
	{0x00, 0x01, 0x02, 0x03}, //���� 1:1
	{0x04, 0x05, 0x06, 0x07}, //���� 1:1
	{0x08, 0x09, 0x0A, 0x0B}, //���� 1:1
	{0x0C, 0x0D, 0x0E, 0x0F}, //���� 1:1
	{0x10, 0x11, 0x12, 0x13}, //HP 1:1
	{0x14, 0x15, 0x16, 0x17}, //TP 1:1
	{0x18, 0x19, 0x1A, 0x1B}, //���� 1:1
	{0x1C, 0x1C, 0x1C, 0x1D}, //���� 1/4
	{0x1E, 0x1E, 0x1E, 0x1F}, //ȫ���� 1/4
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

// ���ϸ��
enum mag_cell_type {
	MAG_CELL_502,
	MAG_CELL_213,
	MAG_CELL_PARTS_OF_ROBOCHAO,
	MAG_CELL_HEART_OF_OPA_OPA,
	MAG_CELL_HEART_OF_PAIN,
	MAG_CELL_HEART_OF_CHAO,
	MAG_CELL_MAX,
};

// �����ɫ
enum mag_color_type {
	MagColor_Red,
	MagColor_Blue,
	MagColor_Yellow,
	MagColor_Green,
	MagColor_Purple,
	MagColor_Black,
	MagColor_White,
	MagColor_Cyan,
	MagColor_Brown,
	MagColor_Orange,
	MagColor_SlateBlue,
	MagColor_Olive,
	MagColor_Turquoise,
	MagColor_Fuschia,
	MagColor_Grey,
	MagColor_Ivory,
	MagColor_Pink,
	MagColor_DarkGreen,
	MagColor_Max,
};

// ��Ź�����װ��ɫ
static uint8_t costume_mag_color_type[12][25] = { // 12 * (9 + 9 + 7)
	// 0x00 HUmar
	{
		MagColor_Orange, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Black, MagColor_White, MagColor_Olive, MagColor_Black,
		MagColor_Red, MagColor_Cyan, MagColor_Olive, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Grey, MagColor_White,
	},
	// 0x01 HUnewearl
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Brown, MagColor_DarkGreen,
		MagColor_Fuschia, MagColor_Blue, MagColor_Yellow, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Pink, MagColor_Blue,
	},
	// 0x02 HUcast
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Grey, MagColor_White, MagColor_Blue, MagColor_Grey,
		MagColor_Orange, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Purple, MagColor_DarkGreen,
		MagColor_Fuschia, MagColor_Blue, MagColor_Olive, MagColor_DarkGreen, MagColor_Fuschia, MagColor_Black, MagColor_White,
	},
	// 0x03 RAmar
	{
		MagColor_Red, MagColor_Blue, MagColor_Olive, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Ivory, MagColor_Black,
		MagColor_Orange, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_Ivory, MagColor_White, MagColor_Brown,
	},
	// 0x04 RAcast
	{
		MagColor_Red, MagColor_Blue, MagColor_Olive, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Black, MagColor_White, MagColor_White, MagColor_Orange,
		MagColor_Orange, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Grey, MagColor_White, MagColor_Blue, MagColor_Purple,
		MagColor_Fuschia, MagColor_Cyan, MagColor_Blue, MagColor_Turquoise, MagColor_SlateBlue, MagColor_Black, MagColor_White,
	},
	// 0x05 RAcaseal
	{
		MagColor_Pink, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Black, MagColor_SlateBlue, MagColor_Red, MagColor_Cyan,
		MagColor_Red, MagColor_Blue, MagColor_Brown, MagColor_DarkGreen, MagColor_Purple, MagColor_Orange, MagColor_Ivory, MagColor_Fuschia, MagColor_Yellow,
		MagColor_SlateBlue, MagColor_Cyan, MagColor_Yellow, MagColor_Turquoise, MagColor_Purple, MagColor_Grey, MagColor_Grey,
	},
	// 0x06 FOmarl
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_Green, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Pink, MagColor_Blue,
		MagColor_Red, MagColor_Cyan, MagColor_Yellow, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Pink, MagColor_Olive,
	},
	// 0x07 FOnewm
	{
		MagColor_Fuschia, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Red, MagColor_DarkGreen,
		MagColor_Brown, MagColor_Blue, MagColor_Yellow, MagColor_Green, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Purple, MagColor_Orange,
	},
	// 0x08 FOnewearl
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_Green, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Pink, MagColor_Black,
		MagColor_Orange, MagColor_Blue, MagColor_Olive, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Grey, MagColor_Blue,
	},
	// 0x09 HUcaseal
	{
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_Ivory, MagColor_SlateBlue, MagColor_Purple,
		MagColor_Fuschia, MagColor_Blue, MagColor_Brown, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_Ivory, MagColor_Black, MagColor_Pink,
		MagColor_Pink, MagColor_Cyan, MagColor_Yellow, MagColor_Olive, MagColor_SlateBlue, MagColor_SlateBlue, MagColor_Ivory,
	},
	// 0x0a FOmar
	{
		MagColor_Red, MagColor_Blue, MagColor_Olive, MagColor_Turquoise, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Brown, MagColor_SlateBlue,
		MagColor_Fuschia, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_SlateBlue, MagColor_Black, MagColor_White, MagColor_Blue, MagColor_Olive,
	},
	// 0x0b RAmarl
	{
		MagColor_Red, MagColor_Cyan, MagColor_Yellow, MagColor_DarkGreen, MagColor_Purple, MagColor_Black, MagColor_White, MagColor_Orange, MagColor_Turquoise,
		MagColor_Red, MagColor_Blue, MagColor_Yellow, MagColor_DarkGreen, MagColor_Fuschia, MagColor_Black, MagColor_Pink, MagColor_Blue, MagColor_White,
	},
};

// ���PB����
enum pb_type {
	PB_Farlla,
	PB_Estlla,
	PB_Golla,
	PB_Pilla,
	PB_Leilla,
	PB_Mylla_Youlla,
	PB_MAX
};

// �������
enum mag_type {
	Mag_Mag,
	Mag_Varuna,
	Mag_Mitra,
	Mag_Surya,
	Mag_Vayu,
	Mag_Varaha,
	Mag_Kama,
	Mag_Ushasu,
	Mag_Apsaras,
	Mag_Kumara,
	Mag_Kaitabha,
	Mag_Tapas,
	Mag_Bhirava,
	Mag_Kalki,
	Mag_Rudra,
	Mag_Marutah,
	Mag_Yaksa,
	Mag_Sita,
	Mag_Garuda,
	Mag_Nandin,
	Mag_Ashvinau,
	Mag_Ribhava,
	Mag_Soma,
	Mag_Ila,
	Mag_Durga,
	Mag_Vritra,
	Mag_Namuci,
	Mag_Sumba,
	Mag_Naga,
	Mag_Pitri,
	Mag_Kabanda,
	Mag_Ravana,
	Mag_Marica,
	Mag_Soniti,
	Mag_Preta,
	Mag_Andhaka,
	Mag_Bana,
	Mag_Naraka,
	Mag_Madhu,
	Mag_Churel,
	Mag_RoboChao,
	Mag_OpaOpa,
	Mag_Pian,
	Mag_Chao,
	Mag_CHUCHU,
	Mag_KAPUKAPU,
	Mag_ANGELSWING,
	Mag_DEVILSWING,
	Mag_ELENOR,
	Mag_MARK3,
	Mag_MASTERSYSTEM,
	Mag_GENESIS,
	Mag_SEGASATURN,
	Mag_DREAMCAST,
	Mag_HAMBURGER,
	Mag_PANZERSTAIL,
	Mag_DEVILSTAIL,
	Mag_Deva,
	Mag_Rati,
	Mag_Savitri,
	Mag_Rukmin,
	Mag_Pushan,
	Mag_Dewari,
	Mag_Sato,
	Mag_Bhima,
	Mag_Nidra,
	Mag_Geungsi,
	Mag_BlankMag,
	Mag_Tellusis,
	Mag_StrikerUnit,
	Mag_Pioneer,
	Mag_Puyo,
	Mag_Muyo,
	Mag_Rappy,
	Mag_Yahoo,  // renamed
	Mag_Gaelgiel,
	Mag_Agastya,
	Mag_0503,    // They do exist lol
	Mag_0504,
	Mag_0505,
	Mag_0506,
	Mag_0507
};

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
uint8_t get_item_amount(item_t* item, uint32_t amount);

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
uint16_t get_sealed_item_kill_count(const item_t* item);
void set_sealed_item_kill_count(item_t* item, uint16_t v);
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

char* get_item_unsealable_describe(const item_t* item, int version);

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