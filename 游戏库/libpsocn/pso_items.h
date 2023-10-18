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

#ifndef PSO_ITEMS_H
#define PSO_ITEMS_H

#include <stdbool.h>
#include "f_logs.h"

#include "pso_player.h"
#include "pso_item_list.h"
#include "pso_item_list_bb.h"

// item_equip_flags 职业装备标志 用于识别不同种族
#define PLAYER_EQUIP_FLAGS_NONE     false
#define PLAYER_EQUIP_FLAGS_OK       true
#define PLAYER_EQUIP_FLAGS_HUNTER   0x01   // Bit 1 猎人
#define PLAYER_EQUIP_FLAGS_RANGER   0x02   // Bit 2 枪手
#define PLAYER_EQUIP_FLAGS_FORCE    0x04   // Bit 3 法师
#define PLAYER_EQUIP_FLAGS_HUMAN    0x08   // Bit 4 人类
#define PLAYER_EQUIP_FLAGS_DROID    0x10   // Bit 5 机器人
#define PLAYER_EQUIP_FLAGS_NEWMAN   0x20   // Bit 6 新人类
#define PLAYER_EQUIP_FLAGS_MALE     0x40   // Bit 7 男人
#define PLAYER_EQUIP_FLAGS_FEMALE   0x80   // Bit 8 女人
#define PLAYER_EQUIP_FLAGS_MAX      8

#define EQUIP_FLAGS          LE32(0x00000008)
#define UNEQUIP_FLAGS        LE32(0xFFFFFFF7)

/* 每个职业对应的装备FLAGS */
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

#define MAX_LOBBY_SAVED_ITEMS					3000

#define ITEM_BASE_STAR_DEFAULT					0
#define ITEM_RARE_THRESHOLD						9

#define ITEM_ID_MESETA							0xFFFFFFFF

/* 真够讨厌的 暂时没有找到办法统计数量 只能先写常量了 */
#define ITEM_TYPE_WEAPON_SPECIAL_NUM            0x29

/* Item buckets. Each item gets put into one of these buckets when in the list,
   in order to make searching the list slightly easier. These are all based on
   the least significant byte of the item code. */
#define ITEM_TYPE_WEAPON						0x00
#define ITEM_TYPE_GUARD							0x01
#define ITEM_TYPE_MAG							0x02
#define ITEM_TYPE_TOOL							0x03
#define ITEM_TYPE_MESETA						0x04

#define MESETA_IDENTIFIER						0x00040000

/* ITEM_TYPE_GUARD items are actually slightly more specialized, and fall into
   three subtypes of their own. These are the second least significant byte in
   the item code. */
#define ITEM_SUBTYPE_FRAME						0x01
#define ITEM_SUBTYPE_BARRIER					0x02
#define ITEM_SUBTYPE_UNIT						0x03

/* ITEM_TYPE_TOOL subtype */
#define ITEM_SUBTYPE_MATE						0x00 /* 10 堆叠 */
#define ITEM_SUBTYPE_FLUID              		0x01 /* 10 堆叠 */
#define ITEM_SUBTYPE_DISK               		0x02 /* 1  堆叠 */

#define ITEM_SUBTYPE_SOL_ATOMIZER       		0x03 /* 10 堆叠 */
#define ITEM_SUBTYPE_MOON_ATOMIZER      		0x04 /* 10 堆叠 */
#define ITEM_SUBTYPE_STAR_ATOMIZER      		0x05 /* 10 堆叠 */
#define ITEM_SUBTYPE_ANTI_TOOL          		0x06 /* 10 堆叠 */
#define ITEM_SUBTYPE_TELEPIPE           		0x07 /* 10 堆叠 */
#define ITEM_SUBTYPE_TRAP_VISION        		0x08 /* 10 堆叠 */
#define ITEM_SUBTYPE_SCAPE_DOLL         		0x09 /* 1  堆叠 */

#define ITEM_SUBTYPE_GRINDER            		0x0A /* 99 堆叠 */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_MATERIAL           		0x0B /* 99 堆叠 */
#define ITEM_SUBTYPE_MATERIAL_POWER     		0x00 /* 99 堆叠 */
#define ITEM_SUBTYPE_MATERIAL_MIND      		0x01 /* 99 堆叠 */
#define ITEM_SUBTYPE_MATERIAL_EVADE     		0x02 /* 99 堆叠 */
#define ITEM_SUBTYPE_MATERIAL_HP	    		0x03 /* 99 堆叠 */
#define ITEM_SUBTYPE_MATERIAL_TP	    		0x04 /* 99 堆叠 */
#define ITEM_SUBTYPE_MATERIAL_DEF	    		0x05 /* 99 堆叠 */
#define ITEM_SUBTYPE_MATERIAL_LUCK	    		0x06 /* 99 堆叠 */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_MAG_CELL1					0x0C /* 99 堆叠 */
#define ITEM_SUBTYPE_MONSTER_LIMBS				0x0D /* 99 堆叠 */
#define ITEM_SUBTYPE_MAG_CELL2					0x0E /* 99 堆叠 */
#define ITEM_SUBTYPE_ADD_SLOT					0x0F /* 99 堆叠 */
#define ITEM_SUBTYPE_PHOTON						0x10 /* 99 堆叠 */
#define ITEM_SUBTYPE_BOOK						0x11 /* 1  堆叠 */
#define ITEM_SUBTYPE_SERVER_ITEM1				0x12 /* 99 堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_PRESENT					0x13 /* 1 堆叠 完成 */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_SERVER_ITEM2				0x14 /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_CHOCOLATE		0x00 /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_CANDY			0x01 /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_CAKE			0x02 /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_W_SILVER		0x03 /* 1  堆叠 */
#define ITEM_SUBTYPE_SERVER_ITEM2_W_GOLD		0x04 /* 1  堆叠 */
#define ITEM_SUBTYPE_SERVER_ITEM2_W_CRYSTAL		0x05 /* 1  堆叠 */
#define ITEM_SUBTYPE_SERVER_ITEM2_W_STEEL		0x06 /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_W_ALUMINUM	0x07 /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_W_LEATHER		0x08 /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_W_BONE		0x09 /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_BOUQUET		0x0A /* 1  堆叠 *//*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2_DECOCTION		0x0B /* 1  堆叠 *//*todo player_use_item*/
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_PRESENT_EVENT      		0x15 /* 99 堆叠 */
#define ITEM_SUBTYPE_PRESENT_EVENT_CHRISTMAS 	0x00 /* 99 堆叠 */
#define ITEM_SUBTYPE_PRESENT_EVENT_EASTER	 	0x01 /* 99 堆叠 */
#define ITEM_SUBTYPE_PRESENT_EVENT_LANTERN	 	0x03 /* 99 堆叠 */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_DISK_MUSIC         		0x16 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_1         	0x00 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_2         	0x01 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_3         	0x02 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_4         	0x03 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_5         	0x04 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_6         	0x05 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_7         	0x06 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_8         	0x07 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_9         	0x08 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_10         	0x09 /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_11         	0x0A /* 99 堆叠 */
#define ITEM_SUBTYPE_DISK_MUSIC_VOL_12         	0x0B /* 99 堆叠 */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_HUNTER_REPORT      		0x17 /* 1  堆叠 *//*todo player_use_item*/
/* 有5个猎人报告 暂未知使用后如何 */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_PART_OF_MAG_CELL   		0x18 /* 99 堆叠 */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_GUILD_REWARD       		0x19 /* 99 堆叠 *//*todo player_use_item*/
/* 有4个公会点数 暂未知使用后如何 */
/////////////////////////////////////////////////////////////////
#define ITEM_SUBTYPE_UNKNOW_ITEM        		0x1A

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

/* 游戏固定武器属性加成 */
static const int8_t weapon_bonus_values[21] = {
    -50, -45, -40, -35, -30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
};

static char* weapon_attrib[6] = { "无", "原生", "变异", "机械", "暗黑", "命中" };

/* Weapon Attributes -- Stored in byte #4 of weapons. */
typedef enum psocn_weapon_attr_e {
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
} psocn_weapon_attr_t;

typedef struct weapon_special {
	uint8_t id;
    const char* name;
    const char* cn_name;
} weapon_special_t;

static const weapon_special_t weapon_specials[] = {
	{Weapon_Attr_None,    "NULL",      "无EX"},
	{Weapon_Attr_Draw,    "Draw",      "吸血"},
	{Weapon_Attr_Drain,   "Drain",     "饮血"},
	{Weapon_Attr_Fill,    "Fill",      "噬血"},
	{Weapon_Attr_Gush,    "Gush",      "嗜血"},
	{Weapon_Attr_Heart,   "Heart",     "心之"},
	{Weapon_Attr_Mind,    "Mind",      "智之"},
	{Weapon_Attr_Soul,    "Soul",      "魂之"},
	{Weapon_Attr_Geist,   "Geist",     "魄之"},
	{Weapon_Attr_Masters, "Master's",  "将之"},
	{Weapon_Attr_Lords,   "Lord's",    "相之"},
	{Weapon_Attr_Kings,   "King's",    "王之"},
	{Weapon_Attr_Charge,  "Charge",    "销金"},
	{Weapon_Attr_Spirit,  "Spirit",    "销魂"},
	{Weapon_Attr_Berserk, "Berserk",   "销血"},
	{Weapon_Attr_Ice,     "Ice",       "冰晶"},
	{Weapon_Attr_Frost,   "Frost",     "冰霜"},
	{Weapon_Attr_Freeze,  "Freeze",    "冰冻"},
	{Weapon_Attr_Blizzard,"Blizzard",  "冰暴"},
	{Weapon_Attr_Bind,    "Bind",      "小麻痹"},
	{Weapon_Attr_Hold,    "Hold",      "中麻痹"},
	{Weapon_Attr_Seize,   "Seize",     "强麻痹"},
	{Weapon_Attr_Arrest,  "Arrest",    "全麻痹"},
	{Weapon_Attr_Heat,    "Heat",      "高热"},
	{Weapon_Attr_Fire,    "Fire",      "火热"},
	{Weapon_Attr_Flame,   "Flame",     "灼热"},
	{Weapon_Attr_Burning, "Burning",   "炽热"},
	{Weapon_Attr_Shock,   "Shock",     "雷鸣"},
	{Weapon_Attr_Thunder, "Thunder",   "雷击"},
	{Weapon_Attr_Storm,   "Storm",     "雷爆"},
	{Weapon_Attr_Tempest, "Tempest",   "雷霆"},
	{Weapon_Attr_Dim,     "Dim",       "暗淡"},
	{Weapon_Attr_Shadow,  "Shadow",    "暗影"},
	{Weapon_Attr_Dark,    "Dark",      "暗黑"},
	{Weapon_Attr_Hell,    "Hell",      "地狱"},
	{Weapon_Attr_Panic,   "Panic",     "混乱"},
	{Weapon_Attr_Riot,    "Riot",      "暴动"},
	{Weapon_Attr_Havoc,   "Havoc",     "浩劫"},
	{Weapon_Attr_Chaos,   "Chaos",     "混沌"},
	{Weapon_Attr_Devils,  "Devil's",   "魔鬼的"},
	{Weapon_Attr_Demons,  "Demon's",   "恶魔的"},
};

typedef struct s_rank_special {
    uint8_t id;
    const char* name;
    const char* cn_name;
} s_rank_special_t;

static const s_rank_special_t s_rank_specials[] = {
	{0x00, "NULL", "无EX"},
    {0x01, "Jellen", "降攻"},
    {0x02, "Zalure", "降防"},
	{0x03, "HP Regeneration", "HP再生"},
	{0x04, "TP Regeneration", "TP再生"},
    {0x05, "Burning", "炽热"},
    {0x06, "Tempest", "雷霆"},
    {0x07, "Blizzard", "冰暴"},
    {0x08, "Arrest", "全麻痹"},
    {0x09, "Chaos", "混沌"},
    {0x0A, "Hell", "地狱"},
    {0x0B, "Spirit", "销魂"},
    {0x0C, "Berserk", "销血"},
    {0x0D, "Demon\'s", "恶魔的"},
    {0x0E, "Gush", "嗜血"},
    {0x0F, "Geist", "魄之"},
    {0x10, "King\'s", "王之"},
};

/* Elemental attributes, sorted by their ranking. This is based on the table
   that is in Tethealla. This is probably in some data file somewhere, and we
   should probably read it from that data file, but this will work for now. */
static const psocn_weapon_attr_t attr_list[4][12] = {
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

static const int attr_count[4] = { 8, 10, 12, 12 };

/* 游戏固定插件属性加成 */
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
		return "红色";
	case 1:
		return "蓝色";
	case 2:
		return "黄色";
	case 3:
		return "绿色";
	case 4:
		return "紫色";
	case 5:
		return "黑色";
	case 6:
		return "白色";
	case 7:
		return "青色";
	case 8:
		return "棕色";
	case 9:
		return "橙色";
	case 10:
		return "石蓝色";
	case 11:
		return "橄榄色";
	case 12:
		return "青绿色";
	case 13:
		return "洋红色";
	case 14:
		return "灰色";
	case 15:
		return "象牙色";
	case 16:
		return "粉红色";
	case 17:
		return "深绿色";
	default:
		return "未知颜色";
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
	"无",
	"+",
	"++"
};

/* 初始化物品数据 */
void clear_inv_item(item_t* item);
void clear_bank_item(item_t* item);

bool create_tmp_item(const uint32_t* item, size_t item_size, item_t* tmp_item);

/* 初始化背包物品数据 */
void clear_iitem(iitem_t* iitem);

/* 初始化银行物品数据 */
void clear_bitem(bitem_t* bitem);

size_t isUnique(size_t* numbers, size_t size, size_t num);
size_t primary_code_identifier(const uint32_t code);
size_t primary_identifier(const item_t* i);

/* 堆叠物检测 */
bool is_stackable(const item_t* item);
size_t stack_size(const item_t* item);
size_t max_stack_size(const item_t* item);
size_t max_stack_size_for_item(uint8_t data0, uint8_t data1);
/* 仅用于房间物品数量 */
uint8_t get_item_amount(item_t* item, uint32_t amount);

void clear_tool_item_if_invalid(item_t* item);
bool is_common_consumable(uint32_t primary_identifier);

/* 获取物品名称 */
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

///* 打印物品数据 */
//void print_item_data(const item_t* item, int version);
//
///* 打印拾取物品数据 */
//void print_pitem_data(const item_t* item, int version);
//
///* 打印掉落物品数据 */
//void print_ditem_data(const item_t* item, int version);
//
///* 打印交易物品数据 */
//void print_titem_data(const item_t* item, int version);
//
///* 打印背包物品数据 */
//void print_iitem_data(const iitem_t* iitem, int item_index, int version);
//
///* 打印银行物品数据 */
//void print_bitem_data(const bitem_t* iitem, int item_index, int version);
//
//void print_biitem_data(void* data, int item_index, int version, int inv, int err);

char* get_item_describe(const item_t* item, int version);

char* get_item_unsealable_describe(const item_t* item, int version);

/* 打印物品数据 */
inline void print_item_data(const item_t* item, int version) {
	ITEM_LOG("物品: %s", get_item_describe(item, version));
	ITEM_LOG("编号: 0x%08X",
		item->item_id);
	ITEM_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
	ITEM_LOG("------------------------------------------------------------");
}

/* 打印物品拾取数据 */
inline void print_pitem_data(const item_t* item, int version) {
	PICKS_LOG("物品: %s", get_item_describe(item, version));
	PICKS_LOG("编号: 0x%08X",
		item->item_id);
	PICKS_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
	PICKS_LOG("------------------------------------------------------------");
}

/* 打印物品掉落数据 */
inline void print_ditem_data(const item_t* item, int version) {
	DROPS_LOG("物品: %s", get_item_describe(item, version));
	DROPS_LOG("编号: 0x%08X",
		item->item_id);
	DROPS_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
	DROPS_LOG("------------------------------------------------------------");
}

/* 打印物品交易数据 */
inline void print_titem_data(const item_t* item, int version) {
	TRADES_LOG("物品: %s", get_item_describe(item, version));
	TRADES_LOG("编号: 0x%08X",
		item->item_id);
	TRADES_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		item->datab[0], item->datab[1], item->datab[2], item->datab[3],
		item->datab[4], item->datab[5], item->datab[6], item->datab[7],
		item->datab[8], item->datab[9], item->datab[10], item->datab[11],
		item->data2b[0], item->data2b[1], item->data2b[2], item->data2b[3]);
	TRADES_LOG("------------------------------------------------------------");
}

/* 打印背包物品数据 */
inline void print_iitem_data(const iitem_t* iitem, int item_index, int version) {
	ITEM_LOG("物品: %s", get_item_describe(&iitem->data, version));
	ITEM_LOG(""
		"槽位 (%d) "
		"(%s) %04X "
		"(%s) "
		"Flags %08X",
		item_index,
		((iitem->present & LE32(0x0001)) ? "已占槽位" : "未占槽位"),
		iitem->present,
		((iitem->flags & EQUIP_FLAGS) ? "已装备" : "未装备"),
		iitem->flags
	);
	ITEM_LOG("编号: 0x%08X", iitem->data.item_id);
	ITEM_LOG("背包数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		iitem->data.datab[0], iitem->data.datab[1], iitem->data.datab[2], iitem->data.datab[3],
		iitem->data.datab[4], iitem->data.datab[5], iitem->data.datab[6], iitem->data.datab[7],
		iitem->data.datab[8], iitem->data.datab[9], iitem->data.datab[10], iitem->data.datab[11],
		iitem->data.data2b[0], iitem->data.data2b[1], iitem->data.data2b[2], iitem->data.data2b[3]);
	ITEM_LOG("------------------------------------------------------------");
}

/* 打印银行物品数据 */
inline void print_bitem_data(const bitem_t* bitem, int item_index, int version) {
	ITEM_LOG("物品: %s", get_item_describe(&bitem->data, version));
	ITEM_LOG(""
		"槽位 (%d) "
		"(%s) %04X "
		"(%s) "
		"Flags %04X",
		item_index,
		((max_stack_size_for_item(bitem->data.datab[0], bitem->data.datab[1]) > 1) ? "堆叠" : "单独"),
		bitem->amount,
		((bitem->show_flags & LE32(0x0001)) ? "显示" : "隐藏"),
		bitem->show_flags
	);
	ITEM_LOG("编号: 0x%08X", bitem->data.item_id);
	ITEM_LOG("银行数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
		bitem->data.datab[0], bitem->data.datab[1], bitem->data.datab[2], bitem->data.datab[3],
		bitem->data.datab[4], bitem->data.datab[5], bitem->data.datab[6], bitem->data.datab[7],
		bitem->data.datab[8], bitem->data.datab[9], bitem->data.datab[10], bitem->data.datab[11],
		bitem->data.data2b[0], bitem->data.data2b[1], bitem->data.data2b[2], bitem->data.data2b[3]);
	ITEM_LOG("------------------------------------------------------------");
}

/* 打印银行背包物品数据 */
inline void print_biitem_data(void* data, int item_index, int version, int inv, int err) {
	char* inv_text = ((inv == 1) ? "背包" : "银行");
	char* err_text = ((err == 1) ? "错误" : "玩家");

	if (data) {
		if (inv) {
			iitem_t* iitem = (iitem_t*)data;

			ITEM_LOG("%s X %s物品:(ID %d / %08X) %s", err_text, inv_text,
				iitem->data.item_id, iitem->data.item_id, item_get_name(&iitem->data, version, 0));

			ITEM_LOG(""
				"槽位 (%d) "
				"(%s) %04X "
				"(%s) Flags %08X",
				item_index,
				((iitem->present == 0x0001) ? "已占槽位" : "未占槽位"),
				iitem->present,
				((iitem->flags & EQUIP_FLAGS) ? "已装备" : "未装备"),
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
				bitem->data.item_id, bitem->data.item_id, item_get_name(&bitem->data, version, 0));

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

#endif /* !PSO_ITEMS_H */