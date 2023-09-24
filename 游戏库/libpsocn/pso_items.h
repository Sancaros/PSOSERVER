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

#include "pso_player.h"
#include "pso_item_list.h"

// item_equip_flags 职业装备标志 用于识别不同种族
#define EQUIP_FLAGS_NONE     1
#define EQUIP_FLAGS_OK       0
#define EQUIP_FLAGS_HUNTER   0x01   // Bit 1 猎人
#define EQUIP_FLAGS_RANGER   0x02   // Bit 2 枪手
#define EQUIP_FLAGS_FORCE    0x04   // Bit 3 法师
#define EQUIP_FLAGS_HUMAN    0x08   // Bit 4 人类
#define EQUIP_FLAGS_DROID    0x10   // Bit 5 机器人
#define EQUIP_FLAGS_NEWMAN   0x20   // Bit 6 新人类
#define EQUIP_FLAGS_MALE     0x40   // Bit 7 男人
#define EQUIP_FLAGS_FEMALE   0x80   // Bit 8 女人
#define EQUIP_FLAGS_MAX      8

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

/* ITEM_TYPE_TOOL subtype*/
#define ITEM_SUBTYPE_MATE               0x00 /* 10 堆叠 */
#define ITEM_SUBTYPE_FLUID              0x01 /* 10 堆叠 */
#define ITEM_SUBTYPE_DISK               0x02 /* 1  堆叠 */

#define ITEM_SUBTYPE_SOL_ATOMIZER       0x03 /* 10 堆叠 */
#define ITEM_SUBTYPE_MOON_ATOMIZER      0x04 /* 10 堆叠 */
#define ITEM_SUBTYPE_STAR_ATOMIZER      0x05 /* 10 堆叠 */
#define ITEM_SUBTYPE_ANTI_TOOL          0x06 /* 10 堆叠 */
#define ITEM_SUBTYPE_TELEPIPE           0x07 /* 10 堆叠 */
#define ITEM_SUBTYPE_TRAP_VISION        0x08 /* 10 堆叠 */
#define ITEM_SUBTYPE_SCAPE_DOLL         0x09 /* 1  堆叠 */

#define ITEM_SUBTYPE_GRINDER            0x0A /* 99 堆叠 */
#define ITEM_SUBTYPE_MATERIAL           0x0B /* 99 堆叠 */
#define ITEM_SUBTYPE_MAG_CELL1          0x0C /* 99 堆叠 */
#define ITEM_SUBTYPE_MONSTER_LIMBS      0x0D /* 99 堆叠 */
#define ITEM_SUBTYPE_MAG_CELL2          0x0E /* 99 堆叠 */
#define ITEM_SUBTYPE_ADD_SLOT           0x0F /* 99 堆叠 */
#define ITEM_SUBTYPE_PHOTON             0x10 /* 99 堆叠 */
#define ITEM_SUBTYPE_BOOK               0x11
#define ITEM_SUBTYPE_SERVER_ITEM1       0x12/*todo player_use_item*/
#define ITEM_SUBTYPE_PRESENT            0x13/*todo player_use_item*/
#define ITEM_SUBTYPE_SERVER_ITEM2       0x14/*todo player_use_item*/
#define ITEM_SUBTYPE_PRESENT_EVENT      0x15
#define ITEM_SUBTYPE_DISK_MUSIC         0x16 /* 99 堆叠 */
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

/* 游戏固定武器属性加成 */
static const int8_t weapon_bonus_values[20] = {
    -50, -45, -40, -35, -30, -25, -20, -15, -10, -5, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
};

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

static char item_des[512];

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
uint32_t get_item_amount(item_t* item, uint32_t amount);

void clear_tool_item_if_invalid(item_t* item);
bool is_common_consumable(uint32_t primary_identifier);

/* 获取物品名称 */
const char* item_get_name(const item_t* item, int version);
bool is_item_empty(item_t* item);
void set_armor_or_shield_defense_bonus(item_t* item, int16_t bonus);
int16_t get_armor_or_shield_defense_bonus(const item_t* item);
void set_common_armor_evasion_bonus(item_t* item, int16_t bonus);
int16_t get_common_armor_evasion_bonus(const item_t* item);
void set_unit_bonus(item_t* item, int16_t bonus);
int16_t get_unit_bonus(const item_t* item);
bool is_s_rank_weapon(const item_t* item);
bool compare_for_sort(item_t* itemDataA, item_t* itemDataB);

/* 打印物品数据 */
void print_item_data(const item_t* item, int version);

/* 打印背包物品数据 */
void print_iitem_data(const iitem_t* iitem, int item_index, int version);

/* 打印银行物品数据 */
void print_bitem_data(const bitem_t* iitem, int item_index, int version);

void print_biitem_data(void* data, int item_index, int version, int inv, int err);

char* get_item_describe(const item_t* item, int version);

#endif /* !PSO_ITEMS_H */