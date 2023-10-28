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

#ifndef PTDATA_H
#define PTDATA_H

#include <stdint.h>

#include "lobby.h"
#include "subcmd.h"

#ifdef PACKED
#undef PACKED
#endif

/* 怪物掉落物品类型 其他则为无掉落 */
#define ENEMY_TYPE_WEAPON     0
#define ENEMY_TYPE_ARMOR      1
#define ENEMY_TYPE_SHIELD     2
#define ENEMY_TYPE_UNIT       3
#define ENEMY_TYPE_TOOL       4
#define ENEMY_TYPE_MESETA     5

/* 箱子掉落物品类型 6则为无掉落 */
#define BOX_TYPE_WEAPON     0
#define BOX_TYPE_ARMOR      1
#define BOX_TYPE_SHIELD     2
#define BOX_TYPE_UNIT       3
#define BOX_TYPE_TOOL       4
#define BOX_TYPE_MESETA     5
#define BOX_TYPE_NOTHING    6

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct rang_8bit {
    uint8_t min;
    uint8_t max;
} PACKED rang_8bit_t;

typedef struct rang_16bit {
    uint16_t min;
    uint16_t max;
} PACKED rang_16bit_t;

struct TableOffsets {
    /* 00 / 14884 */ uint32_t weapon_table; // -> [{count, offset -> [Weapon]}](0xED)
    /* 04 / 1478C */ uint32_t armor_table; // -> [{count, offset -> [ArmorOrShield]}](2; armors and shields)
    /* 08 / 1479C */ uint32_t unit_table; // -> {count, offset -> [Unit]} (last if out of range)
    /* 0C / 147AC */ uint32_t tool_table; // -> [{count, offset -> [Tool]}](0x1A) (last if out of range)
    /* 10 / 147A4 */ uint32_t mag_table; // -> {count, offset -> [Mag]}
    /* 14 / 0F4B8 */ uint32_t attack_animation_table; // -> [uint8_t](0xED)
    /* 18 / 0DE7C */ uint32_t photon_color_table; // -> [0x24-byte structs](0x20)
    /* 1C / 0E194 */ uint32_t weapon_range_table; // -> ???
    /* 20 / 0F5A8 */ uint32_t weapon_sale_divisor_table; // -> [float](0xED)
    /* 24 / 0F83C */ uint32_t sale_divisor_table; // -> NonWeaponSaleDivisors
    /* 28 / 1502C */ uint32_t mag_feed_table; // -> MagFeedResultsTable
    /* 2C / 0FB0C */ uint32_t star_value_table; // -> [uint8_t] (indexed by .id from weapon, armor, etc.)
    /* 30 / 0FE3C */ uint32_t special_data_table; // -> [Special]
    /* 34 / 0FEE0 */ uint32_t weapon_effect_table; // -> [16-byte structs]
    /* 38 / 1275C */ uint32_t stat_boost_table; // -> [StatBoost]
    /* 3C / 11C80 */ uint32_t shield_effect_table; // -> [8-byte structs]
    /* 40 / 12894 */ uint32_t max_tech_level_table; // -> MaxTechniqueLevels
    /* 44 / 14FF4 */ uint32_t combination_table; // -> {count, offset -> [ItemCombination]}
    /* 48 / 12754 */ uint32_t unknown_a1;
    /* 4C / 14278 */ uint32_t tech_boost_table; // -> [TechniqueBoost] (always 0x2C of them? from counts struct?)
    /* 50 / 15014 */ uint32_t unwrap_table; // -> {count, offset -> [{count, offset -> [EventItem]}]}
    /* 54 / 1501C */ uint32_t unsealable_table; // -> {count, offset -> [UnsealableItem]}
    /* 58 / 15024 */ uint32_t ranged_special_table; // -> {count, offset -> [4-byte structs]}
} PACKED;

/* Entry in one of the ItemPT files. This version corresponds to the files that
   were used in PSOv2. The names of the fields were taken from the above
   structure. In the file itself, each of these fields is stored in
   little-endian byte order. */
typedef struct fpt_v2_entry {
    int8_t weapon_ratio[12];                /* 0x0000 */
    int8_t weapon_minrank[12];              /* 0x000C */
    int8_t weapon_upgfloor[12];             /* 0x0018 */
    int8_t power_pattern[9][4];             /* 0x0024 */
    uint8_t percent_pattern[23][5];         /* 0x0048 */
    int8_t area_pattern[3][10];             /* 0x00BB */
    int8_t percent_attachment[6][10];       /* 0x00D9 */
    int8_t element_ranking[10];             /* 0x0115 */
    int8_t element_probability[10];         /* 0x011F */
    int8_t armor_ranking[5];                /* 0x0129 */
    int8_t slot_ranking[5];                 /* 0x012E */
    int8_t unit_level[10];                  /* 0x0133 */
    uint8_t padding;                        /* 0x013D */
    uint16_t tool_frequency[28][10];        /* 0x013E */
    uint8_t tech_frequency[19][10];         /* 0x036E */
    int8_t tech_levels[19][20];             /* 0x042C */
    int8_t enemy_dar[100];                  /* 0x05A8 */
    uint16_t enemy_meseta[100][2];          /* 0x060C */
    int8_t enemy_drop[100];                 /* 0x079C */
    uint16_t box_meseta[10][2];             /* 0x0800 */
    uint8_t box_drop[7][10];                /* 0x0828 */
    uint16_t padding2;                      /* 0x086E */
    /* 0870 */ uint32_t base_weapon_type_prob_table_offset;
    /* 0874 */ uint32_t subtype_base_table_offset;
    /* 0878 */ uint32_t subtype_area_length_table_offset;
    /* 087C */ uint32_t grind_prob_tables_offset;
    /* 0880 */ uint32_t armor_shield_type_index_prob_table_offset;
    /* 0884 */ uint32_t armor_slot_count_prob_table_offset;
    /* 0888 */ uint32_t enemy_meseta_ranges_offset;
    /* 088C */ uint32_t enemy_type_drop_probs_offset;
    /* 0890 */ uint32_t enemy_item_classes_offset;
    /* 0894 */ uint32_t box_meseta_ranges_offset;
    /* 0898 */ uint32_t bonus_value_prob_tables_offset;
    /* 089C */ uint32_t nonrare_bonus_prob_spec_offset;
    /* 08A0 */ uint32_t bonus_type_prob_tables_offset;
    /* 08A4 */ uint32_t special_mult_offset;
    /* 08A8 */ uint32_t special_percent_offset;
    /* 08AC */ uint32_t tool_class_prob_table_offset;
    /* 08B0 */ uint32_t technique_index_prob_table_offset;
    /* 08B4 */ uint32_t technique_level_ranges_offset;
    /* 08B8 */ uint8_t armor_level;
    /* 08B9 */ uint8_t unused2[3];
    /* 08BC */ uint32_t unit_maxes_offset;
    /* 08D0 */ uint32_t box_item_class_prob_tables_offset;
    /* 08D4 */ uint32_t unused_offset2;
    /* 08D8 */ uint32_t unused_offset3;
    /* 08DC */ uint32_t unused_offset4;
    /* 08E0 */ uint32_t unused_offset5;
    /* 08E4 */ uint32_t unused_offset6;
    /* 08E8 */ uint32_t unused_offset7;
    /* 08EC */ uint32_t unused_offset8;
    /* 08F0 */ uint16_t unknown_f1[0x20];
    /* 0920 */ uint32_t unknown_f1_offset;
    /* 0924 */ uint32_t unknown_f2[3];
    /* 0930 */ uint32_t offset_table_offset;
    /* 0934 */ uint32_t unknown_f3[3];
} PACKED fpt_v2_entry_t;

/* Entry in one of the ItemPT files. Mostly adapted from Tethealla... In the
   file itself, each of these fields is stored in big-endian byter order.
   Some of this data also comes from a post by Lee on the PSOBB Eden forums:
   http://edenserv.net/forum/viewtopic.php?p=19305#p19305 */
typedef struct fpt_v3_entry {
    uint8_t weapon_ratio[12];               /* 0x0000 */
    int8_t weapon_minrank[12];              /* 0x000C */
    uint8_t weapon_upgfloor[12];            /* 0x0018 */
    uint8_t power_pattern[9][4];            /* 0x0024 */
    uint16_t percent_pattern[23][6];        /* 0x0048 */
    uint8_t area_pattern[3][10];            /* 0x015C */
    uint8_t percent_attachment[6][10];      /* 0x017A */
    uint8_t element_ranking[10];            /* 0x01B6 */
    uint8_t element_probability[10];        /* 0x01C0 */
    uint8_t armor_ranking[5];               /* 0x01CA */
    uint8_t slot_ranking[5];                /* 0x01CF */
    uint8_t unit_level[10];                 /* 0x01D4 */
    uint16_t tool_frequency[28][10];        /* 0x01DE */
    uint8_t tech_frequency[19][10];         /* 0x040E */
    uint8_t tech_levels[19][20];            /* 0x04CC */
    uint8_t enemy_dar[100];                 /* 0x0648 */
    uint16_t enemy_meseta[100][2];          /* 0x06AC */
    uint8_t enemy_drop[100];                /* 0x083C */
    uint16_t box_meseta[10][2];             /* 0x08A0 */
    uint8_t box_drop[7][10];                /* 0x08C8 */
    uint16_t padding;                       /* 0x090E */
    /* 0910 */ uint32_t base_weapon_type_prob_table_offset;
    /* 0914 */ uint32_t subtype_base_table_offset;
    /* 0918 */ uint32_t subtype_area_length_table_offset;
    /* 091C */ uint32_t grind_prob_tables_offset;
    /* 0920 */ uint32_t armor_shield_type_index_prob_table_offset;
    /* 0924 */ uint32_t armor_slot_count_prob_table_offset;
    /* 0928 */ uint32_t enemy_meseta_ranges_offset;
    /* 092C */ uint32_t enemy_type_drop_probs_offset;
    /* 0930 */ uint32_t enemy_item_classes_offset;
    /* 0934 */ uint32_t box_meseta_ranges_offset;
    /* 0938 */ uint32_t bonus_value_prob_tables_offset;
    /* 093C */ uint32_t nonrare_bonus_prob_spec_offset;
    /* 0940 */ uint32_t bonus_type_prob_tables_offset;
    /* 0944 */ uint32_t special_mult_offset;
    /* 0948 */ uint32_t special_percent_offset;
    /* 094C */ uint32_t tool_class_prob_table_offset;
    /* 0950 */ uint32_t technique_index_prob_table_offset;
    /* 0954 */ uint32_t technique_level_ranges_offset;
    /* 0958 */ uint8_t armor_level;
    /* 0959 */ uint8_t unused2[3];
    /* 095C */ uint32_t unit_maxes_offset;
    /* 0960 */ uint32_t box_item_class_prob_tables_offset;
    /* 0964 */ uint32_t unused_offset2;
    /* 0968 */ uint32_t unused_offset3;
    /* 096C */ uint32_t unused_offset4;
    /* 0970 */ uint32_t unused_offset5;
    /* 0974 */ uint32_t unused_offset6;
    /* 0978 */ uint32_t unused_offset7;
    /* 097C */ uint32_t unused_offset8;
    /* 0980 */ uint16_t unknown_f1[0x20];
    /* 09C0 */ uint32_t unknown_f1_offset;
    /* 09C4 */ uint32_t unknown_f2[3];
    /* 09D0 */ uint32_t offset_table_offset;
    /* 09D4 */ uint32_t unknown_f3[3];
} PACKED fpt_v3_entry_t;

/* Entry in one of the ItemPT files. Mostly adapted from Tethealla... In the
   file itself, each of these fields is stored in big-endian byter order.
   Some of this data also comes from a post by Lee on the PSOBB Eden forums:
   http://edenserv.net/forum/viewtopic.php?p=19305#p19305
   补充：完整结构来自newserv
   */
typedef struct fpt_bb_entry {
    uint8_t weapon_ratio[12];               /* 0x0000 */
    int8_t weapon_minrank[12];              /* 0x000C */
    uint8_t weapon_upgfloor[12];            /* 0x0018 */
    uint8_t power_pattern[9][4];            /* 0x0024 */
    uint16_t percent_pattern[23][6];        /* 0x0048 */
    uint8_t area_pattern[3][10];            /* 0x015C */
    uint8_t percent_attachment[6][10];      /* 0x017A */
    uint8_t element_ranking[10];            /* 0x01B6 */
    uint8_t element_probability[10];        /* 0x01C0 */
    uint8_t armor_ranking[5];               /* 0x01CA */
    uint8_t slot_ranking[5];                /* 0x01CF */
    uint8_t unit_level[10];                 /* 0x01D4 */
    uint16_t tool_frequency[28][10];        /* 0x01DE */
    uint8_t tech_frequency[19][10];         /* 0x040E */
    uint8_t tech_levels[19][20];            /* 0x04CC */
    uint8_t enemy_dar[100];                 /* 0x0648 */
    uint16_t enemy_meseta[100][2];          /* 0x06AC */
    uint8_t enemy_drop[100];                /* 0x083C */
    uint16_t box_meseta[10][2];             /* 0x08A0 */
    uint8_t box_drop[7][10];                /* 0x08C8 */
    uint16_t padding;                       /* 0x090E */
    /* 0910 */ uint32_t base_weapon_type_prob_table_offset;
    /* 0914 */ uint32_t subtype_base_table_offset;
    /* 0918 */ uint32_t subtype_area_length_table_offset;
    /* 091C */ uint32_t grind_prob_tables_offset;
    /* 0920 */ uint32_t armor_shield_type_index_prob_table_offset;
    /* 0924 */ uint32_t armor_slot_count_prob_table_offset;
    /* 0928 */ uint32_t enemy_meseta_ranges_offset;
    /* 092C */ uint32_t enemy_type_drop_probs_offset;
    /* 0930 */ uint32_t enemy_item_classes_offset;
    /* 0934 */ uint32_t box_meseta_ranges_offset;
    /* 0938 */ uint32_t bonus_value_prob_tables_offset;
    /* 093C */ uint32_t nonrare_bonus_prob_spec_offset;
    /* 0940 */ uint32_t bonus_type_prob_tables_offset;
    /* 0944 */ uint32_t special_mult_offset;
    /* 0948 */ uint32_t special_percent_offset;
    /* 094C */ uint32_t tool_class_prob_table_offset;
    /* 0950 */ uint32_t technique_index_prob_table_offset;
    /* 0954 */ uint32_t technique_level_ranges_offset;
    /* 0958 */ uint8_t armor_level;
    /* 0959 */ uint8_t unused2[3];
    /* 095C */ uint32_t unit_maxes_offset;
    /* 0960 */ uint32_t box_item_class_prob_tables_offset;
    /* 0964 */ uint32_t unused_offset2;
    /* 0968 */ uint32_t unused_offset3;
    /* 096C */ uint32_t unused_offset4;
    /* 0970 */ uint32_t unused_offset5;
    /* 0974 */ uint32_t unused_offset6;
    /* 0978 */ uint32_t unused_offset7;
    /* 097C */ uint32_t unused_offset8;
    /* 0980 */ uint16_t unknown_f1[0x20];
    /* 09C0 */ uint32_t unknown_f1_offset;
    /* 09C4 */ uint32_t unknown_f2[3];
    /* 09D0 */ uint32_t offset_table_offset;
    /* 09D4 */ uint32_t unknown_f3[3];
} PACKED fpt_bb_entry_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* Clean (non-packed) version of the v2 ItemPT entry structure. */
typedef struct pt_v2_entry {
    int8_t weapon_ratio[12];                /* 0x0000 */
    int8_t weapon_minrank[12];              /* 0x000C */
    int8_t weapon_upgfloor[12];             /* 0x0018 */
    int8_t power_pattern[9][4];             /* 0x0024 */
    uint8_t percent_pattern[23][5];         /* 0x0048 */
    int8_t area_pattern[3][10];             /* 0x00BB */
    int8_t percent_attachment[6][10];       /* 0x00D9 */
    int8_t element_ranking[10];             /* 0x0115 */
    uint8_t element_probability[10];        /* 0x011F */
    int8_t armor_ranking[5];                /* 0x0129 */
    int8_t slot_ranking[5];                 /* 0x012E */
    int8_t unit_level[10];                  /* 0x0133 */
    uint16_t tool_frequency[28][10];        /* 0x013E */
    uint8_t tech_frequency[19][10];         /* 0x036E */
    rang_8bit_t tech_levels[19][10];            /* 0x042C */
    int8_t enemy_dar[100];                  /* 0x05A8 */
    uint16_t enemy_meseta[100][2];          /* 0x060C */
    int8_t enemy_drop[100];                 /* 0x079C */
    uint16_t box_meseta[10][2];             /* 0x0800 */
    uint8_t box_drop[7][10];                /* 0x0828 */
    uint16_t padding2;                      /* 0x086E */
    /* 0870 */ uint32_t base_weapon_type_prob_table_offset;
    /* 0874 */ uint32_t subtype_base_table_offset;
    /* 0878 */ uint32_t subtype_area_length_table_offset;
    /* 087C */ uint32_t grind_prob_tables_offset;
    /* 0880 */ uint32_t armor_shield_type_index_prob_table_offset;
    /* 0884 */ uint32_t armor_slot_count_prob_table_offset;
    /* 0888 */ uint32_t enemy_meseta_ranges_offset;
    /* 088C */ uint32_t enemy_type_drop_probs_offset;
    /* 0890 */ uint32_t enemy_item_classes_offset;
    /* 0894 */ uint32_t box_meseta_ranges_offset;
    /* 0898 */ uint32_t bonus_value_prob_tables_offset;
    /* 089C */ uint32_t nonrare_bonus_prob_spec_offset;
    /* 08A0 */ uint32_t bonus_type_prob_tables_offset;
    /* 08A4 */ uint32_t special_mult_offset;
    /* 08A8 */ uint32_t special_percent_offset;
    /* 08AC */ uint32_t tool_class_prob_table_offset;
    /* 08B0 */ uint32_t technique_index_prob_table_offset;
    /* 08B4 */ uint32_t technique_level_ranges_offset;
    /* 08B8 */ uint8_t armor_level;
    /* 08B9 */ uint8_t unused2[3];
    /* 08BC */ uint32_t unit_maxes_offset;
    /* 08D0 */ uint32_t box_item_class_prob_tables_offset;
    /* 08D4 */ uint32_t unused_offset2;
    /* 08D8 */ uint32_t unused_offset3;
    /* 08DC */ uint32_t unused_offset4;
    /* 08E0 */ uint32_t unused_offset5;
    /* 08E4 */ uint32_t unused_offset6;
    /* 08E8 */ uint32_t unused_offset7;
    /* 08EC */ uint32_t unused_offset8;
    /* 08F0 */ uint16_t unknown_f1[0x20];
    /* 0920 */ uint32_t unknown_f1_offset;
    /* 0924 */ uint32_t unknown_f2[3];
    /* 0930 */ uint32_t offset_table_offset;
    /* 0934 */ uint32_t unknown_f3[3];
} pt_v2_entry_t;

/* Clean (non-packed) version of the v3 ItemPT entry structure. */
typedef struct pt_v3_entry {
    uint8_t weapon_ratio[12];               /* 0x0000 */
    int8_t weapon_minrank[12];              /* 0x000C */
    uint8_t weapon_upgfloor[12];            /* 0x0018 */
    uint8_t power_pattern[9][4];            /* 0x0024 */
    uint16_t percent_pattern[23][6];        /* 0x0048 */
    uint8_t area_pattern[3][10];            /* 0x015C */
    uint8_t percent_attachment[6][10];      /* 0x017A */
    uint8_t element_ranking[10];            /* 0x01B6 */
    uint8_t element_probability[10];        /* 0x01C0 */
    uint8_t armor_ranking[5];               /* 0x01CA */
    uint8_t slot_ranking[5];                /* 0x01CF */
    uint8_t unit_level[10];                 /* 0x01D4 */
    uint16_t tool_frequency[28][10];        /* 0x01DE */
    uint8_t tech_frequency[19][10];         /* 0x040E */
    rang_8bit_t tech_levels[19][10];        /* 0x04CC */
    uint8_t enemy_dar[100];                 /* 0x0648 */
    uint16_t enemy_meseta[100][2];          /* 0x06AC */
    uint8_t enemy_drop[100];                /* 0x083C */
    uint16_t box_meseta[10][2];             /* 0x08A0 */
    uint8_t box_drop[7][10];                /* 0x08C8 */
    uint16_t padding;                       /* 0x090E */
    /* 0910 */ uint32_t base_weapon_type_prob_table_offset;
    /* 0914 */ uint32_t subtype_base_table_offset;
    /* 0918 */ uint32_t subtype_area_length_table_offset;
    /* 091C */ uint32_t grind_prob_tables_offset;
    /* 0920 */ uint32_t armor_shield_type_index_prob_table_offset;
    /* 0924 */ uint32_t armor_slot_count_prob_table_offset;
    /* 0928 */ uint32_t enemy_meseta_ranges_offset;
    /* 092C */ uint32_t enemy_type_drop_probs_offset;
    /* 0930 */ uint32_t enemy_item_classes_offset;
    /* 0934 */ uint32_t box_meseta_ranges_offset;
    /* 0938 */ uint32_t bonus_value_prob_tables_offset;
    /* 093C */ uint32_t nonrare_bonus_prob_spec_offset;
    /* 0940 */ uint32_t bonus_type_prob_tables_offset;
    /* 0944 */ uint32_t special_mult_offset;
    /* 0948 */ uint32_t special_percent_offset;
    /* 094C */ uint32_t tool_class_prob_table_offset;
    /* 0950 */ uint32_t technique_index_prob_table_offset;
    /* 0954 */ uint32_t technique_level_ranges_offset;
    /* 0958 */ uint8_t armor_level;
    /* 0959 */ uint8_t unused2[3];
    /* 095C */ uint32_t unit_maxes_offset;
    /* 0960 */ uint32_t box_item_class_prob_tables_offset;
    /* 0964 */ uint32_t unused_offset2;
    /* 0968 */ uint32_t unused_offset3;
    /* 096C */ uint32_t unused_offset4;
    /* 0970 */ uint32_t unused_offset5;
    /* 0974 */ uint32_t unused_offset6;
    /* 0978 */ uint32_t unused_offset7;
    /* 097C */ uint32_t unused_offset8;
    /* 0980 */ uint16_t unknown_f1[0x20];
    /* 09C0 */ uint32_t unknown_f1_offset;
    /* 09C4 */ uint32_t unknown_f2[3];
    /* 09D0 */ uint32_t offset_table_offset;
    /* 09D4 */ uint32_t unknown_f3[3];
} pt_v3_entry_t;

/* Clean (non-packed) version of the BB ItemPT entry structure. */
typedef struct pt_bb_entry {
    // 这个数据结构在多个地方使用了索引概率表。索引概率表是一个表格，每个条目包含该条目索引被使用的概率。
    // 例如，如果武器槽位数概率表包含 [77, 17, 5, 1, 0]，这表示没有槽位的概率为77%，1个槽位的概率为17%，2个槽位的概率为5%，3个槽位的概率为1%，没有4个槽位的概率为0%。
    // 索引概率表中的值不必加起来等于100；游戏会对它们求和，并选择一个小于该最大值的随机数。

    // 地区（楼层）编号也在许多地方使用。与普通的地区编号不同，普通地区编号从Pioneer 2开始，但是这个数据结构中的地区编号从Forest 1开始，而且首领区域被视为下一部分的第一个地区（所以De Rol Le有Mines 1的掉落，例如）。
    // 最后的首领区域被视为最后一个非首领区域（所以Dark Falz的箱子就像Ruins 3的箱子）。我们将这些调整过的地区编号称为（area - 1）。

    // 这个索引概率表确定非稀有武器的类型。这个表中的索引对应于非稀有武器类型01到0C（Saber到Wand）。
    uint8_t base_weapon_type_prob_table[12];             /* 0x0000 base_weapon_type_prob_table */
    // 这个表格指定了每个武器类型的基础子类型。这里的负值意味着该武器在前N个地区中找不到（所以-2，例如，意味着该武器根本不会出现在Forest 1或2中）。
    // 这里的非负值意味着该子类型在所有地区都可以找到，并指定基础子类型（通常在[0, 4]范围内）。实际出现的武器子类型取决于该值和下表中的一个值。
    int8_t subtype_base_table[12];              /* 0x000C subtype_base_table */
    // 这个表格指定了每个武器子类型出现在多少个地区。例如，如果剑（子类型02，在该表格和上表中的索引1）的基础子类型为-2，子类型区域长度为4，
    // 那么当地区-1为2，3，4或5（Cave 1到Mine 1）时可以找到剑物品，而Gigush（下一个剑子类型）可以在Mine 1到Ruins 3中找到。
    uint8_t subtype_area_length_table[12];           /* 0x0018 subtype_area_length_table */
    // 这个索引概率表指定了每个可能的强化值的概率。表格的索引为[强化值][子类型区域索引]，其中子类型区域索引是玩家超过可以找到该子类型的第一个区域的区域数（限制在[0, 3]范围内）。
    // 继续上面的例子，Cave 3中的子类型区域索引将为2，因为剑可以在Cave 1中早两个区域找到。
    // 例如，这个表格可以如下所示：
    //   [64 1E 19 14] // 获取+0强化值的概率
    //   [00 1E 17 0F] // 获取+1强化值的概率
    //   [00 14 14 0E] // 获取+2强化值的概率
    //    ...
    //    C1 C2 C3 M1  // （上述示例中的Episode 1区域值作为参考）
    uint8_t grind_prob_tables[9][4];               /* 0x0024 grind_prob_tables */
    // This array specifies the chance that a rare weapon will have each
    // possible bonus value. This is indexed as [(bonus_value - 10 / 5)][spec],
    // so the first row refers the probability of getting a -10% bonus, the next
    // row is the chance of getting -5%, etc., all the way up to +100%. For
    // non-rare items, spec is determined randomly based on the following field;
    // for rare items, spec is always 5.
    uint16_t bonus_value_prob_tables[23][6];        /* 0x0048 bonus_value_prob_tables */
    // This array specifies the value of spec to be used in the above lookup for
    // non-rare items. This is NOT an index probability table; this is a direct
    // lookup with indexes [bonus_index][area - 1]. A value of 0xFF in any byte
    // of this array prevents any weapon from having a bonus in that slot.
    // For example, the array might look like this:
    //   [00 00 00 01 01 01 01 02 02 02]
    //   [FF FF FF 00 00 00 01 01 01 01]
    //   [FF FF FF FF FF FF FF FF FF 00]
    //    F1 F2 C1 C2 C3 M1 M2 R1 R2 R3  // (Episode 1 areas, for reference)
    // In this example, spec is 0, 1, or 2 in all cases where a weapon can have
    // a bonus. In Forest 1 and 2 and Cave 1, weapons may have at most one
    // bonus; in all other areas except Ruins 3, they can have at most two
    // bonuses, and in Ruins 3, they can have up to three bonuses.
    uint8_t nonrare_bonus_prob_spec[3][10];            /* 0x015C nonrare_bonus_prob_spec */
    // This array specifies the chance that a weapon will have each bonus type.
    // The table is indexed as [bonus_type][area - 1] for non-rare items; for
    // rare items, a random value in the range [0, 9] is used instead of
    // (area - 1).
    // For example, the table might look like this:
    //   [46 46 3F 3E 3E 3D 3C 3C 3A 3A] // Chance of getting no bonus
    //   [14 14 0A 0A 09 02 02 04 05 05] // Chance of getting Native bonus
    //   [0A 0A 12 11 11 09 09 08 08 08] // Chance of getting A.Beast bonus
    //   [00 00 09 0A 0B 13 12 08 09 09] // Chance of getting Machine bonus
    //   [00 00 00 01 01 08 0A 13 13 13] // Chance of getting Dark bonus
    //   [00 00 00 00 00 01 01 01 01 01] // Chance of getting Hit bonus
    //    F1 F2 C1 C2 C3 M1 M2 R1 R2 R3  // (Episode 1 areas, for reference)
    uint8_t bonus_type_prob_tables[6][10];      /* 0x017A bonus_type_prob_tables */
    // This array (indexed by area - 1) specifies a multiplier of used in
    // special ability determination. It seems this uses the star values from
    // ItemPMT, but not yet clear exactly in what way.
    // TODO: Figure out exactly what this does. Anchor: 80106FEC
    uint8_t special_mult[10];            /* 0x01B6 special_mult */
    // This array (indexed by area - 1) specifies the probability that any
    // non-rare weapon will have a special ability.
    uint8_t special_percent[10];        /* 0x01C0 special_percent */
    // TODO: Figure out exactly how this table is used. Anchor: 80106D34
    uint8_t armor_shield_type_index_prob_table[5];               /* 0x01CA armor_shield_type_index_prob_table */
    // This index probability table specifies how common each possible slot
    // count is for armor drops.
    uint8_t armor_slot_count_prob_table[5];                /* 0x01CF armor_slot_count_prob_table */
    // These values specify maximum indexes into another array which is
    // generated at runtime. The values here are multiplied by a random float in
    // the range [0, n] to look up the value in the secondary array, which is
    // what ends up determining the unit type.
    // TODO: Figure out and document the exact logic here. Anchor: 80106364
    uint8_t unit_maxes[10];                 /* 0x01D4 unit_maxes */
    // This index probability table is indexed by [tool_class][area - 1]. The
    // tool class refers to an entry in ItemPMT, which links it to the actual
    // item code.
    uint16_t tool_class_prob_table[28][10];        /* 0x01DE tool_class_prob_table */
    // This index probability table determines how likely each technique is to
    // appear. The table is indexed as [technique_num][area - 1].
    uint8_t technique_index_prob_table[19][10];         /* 0x040E technique_index_prob_table */
    // This table specifies the ranges for technique disk levels. The table is
    // indexed as [technique_num][area - 1]. If either min or max in the range
    // is 0xFF, or if max < min, technique disks are not dropped for that
    // technique and area pair.
    rang_8bit_t technique_level_ranges[19][10];            /* 0x04CC technique_level_ranges */
    // Each byte in this table (indexed by enemy_type) represents the percent
    // chance that the enemy drops anything at all. (This check is done before
    // the rare drop check, so the chance of getting a rare item from an enemy
    // is essentially this probability multiplied by the rare drop rate.)
    uint8_t enemy_type_drop_probs[100];                 /* 0x0648 enemy_type_drop_probs */
    // This array (indexed by enemy_id) specifies the range of meseta values
    // that each enemy can drop.
    rang_16bit_t enemy_meseta_ranges[100];          /* 0x06AC enemy_meseta_ranges */
    // Each byte in this table (indexed by enemy_type) represents the class of
    // item that the enemy can drop. The values are:
    // 00 = weapon
    // 01 = armor
    // 02 = shield
    // 03 = unit
    // 04 = tool
    // 05 = meseta
    // Anything else = no item
    uint8_t enemy_item_classes[100];                /* 0x083C enemy_item_classes */
    // This table (indexed by area - 1) specifies the ranges of meseta values
    // that can drop from boxes.
    rang_16bit_t box_meseta_ranges[10];             /* 0x08A0 box_meseta_ranges */
    // This index probability table determines which type of items drop from
    // boxes. The table is indexed as [item_class][area - 1], with item_class as
    // the result value (that is, in the example below, the game looks at a
    // single column and sums the values going down, then the chosen item class
    // is one of the row indexes based on the weight values in the column.) The
    // resulting item_class value has the same meaning as in enemy_item_classes
    // above.
    // For example, this array might look like the following:
    //   [07 07 08 08 06 07 08 09 09 0A] // Chances per area of a weapon drop
    //   [02 02 02 02 03 02 02 02 03 03] // Chances per area of an armor drop
    //   [02 02 02 02 03 02 02 02 03 03] // Chances per area of a shield drop
    //   [00 00 03 03 03 04 03 04 05 05] // Chances per area of a unit drop
    //   [11 11 12 12 12 12 12 12 12 12] // Chances per area of a tool drop
    //   [32 32 32 32 32 32 32 32 32 32] // Chances per area of a meseta drop
    //   [16 16 11 11 11 11 11 0F 0C 0B] // Chances per area of an empty box
    //    F1 F2 C1 C2 C3 M1 M2 R1 R2 R3  // (Episode 1 areas, for reference)
    uint8_t box_drop[7][10];                /* 0x08C8 box_item_class_prob_tables */
    uint16_t padding;                       /* 0x090E */
    /* 0910 */ uint32_t base_weapon_type_prob_table_offset;
    /* 0914 */ uint32_t subtype_base_table_offset;
    /* 0918 */ uint32_t subtype_area_length_table_offset;
    /* 091C */ uint32_t grind_prob_tables_offset;
    /* 0920 */ uint32_t armor_shield_type_index_prob_table_offset;
    /* 0924 */ uint32_t armor_slot_count_prob_table_offset;
    /* 0928 */ uint32_t enemy_meseta_ranges_offset;
    /* 092C */ uint32_t enemy_type_drop_probs_offset;
    /* 0930 */ uint32_t enemy_item_classes_offset;
    /* 0934 */ uint32_t box_meseta_ranges_offset;
    /* 0938 */ uint32_t bonus_value_prob_tables_offset;
    /* 093C */ uint32_t nonrare_bonus_prob_spec_offset;
    /* 0940 */ uint32_t bonus_type_prob_tables_offset;
    /* 0944 */ uint32_t special_mult_offset;
    /* 0948 */ uint32_t special_percent_offset;
    /* 094C */ uint32_t tool_class_prob_table_offset;
    /* 0950 */ uint32_t technique_index_prob_table_offset;
    /* 0954 */ uint32_t technique_level_ranges_offset;
    /* 0958 */ uint8_t armor_level;
    /* 0959 */ uint8_t unused2[3];
    /* 095C */ uint32_t unit_maxes_offset;
    /* 0960 */ uint32_t box_item_class_prob_tables_offset;
    /* 0964 */ uint32_t unused_offset2;
    /* 0968 */ uint32_t unused_offset3;
    /* 096C */ uint32_t unused_offset4;
    /* 0970 */ uint32_t unused_offset5;
    /* 0974 */ uint32_t unused_offset6;
    /* 0978 */ uint32_t unused_offset7;
    /* 097C */ uint32_t unused_offset8;
    /* 0980 */ uint16_t unknown_f1[0x20];
    /* 09C0 */ uint32_t unknown_f1_offset;
    /* 09C4 */ uint32_t unknown_f2[3];
    /* 09D0 */ uint32_t offset_table_offset;
    /* 09D4 */ uint32_t unknown_f3[3];
} pt_bb_entry_t;

typedef struct {
    uint8_t override_area;
} ItemDropSub;

static int have_v2pt = 0;
static int have_gcpt = 0;
static int have_bbpt = 0;

static pt_v2_entry_t v2_ptdata[4][10];
static pt_v3_entry_t gc_ptdata[4][4][10];
//EP1 0 / EP2 1 /  CHALLENGE1 2/ CHALLENGE2 3/ EP4 4
static pt_bb_entry_t bb_ptdata[5][4][10];
static pt_bb_entry_t bb_dymnamic_ptdata[5][4][10];
static BattleRules_t* restrictions = NULL;
static ItemDropSub* item_drop_sub;

/* Read the ItemPT data from a v2-style (ItemPT.afs) file. */
int pt_read_v2(const char *fn);

/* Read the ItemPT data from a v3-style (ItemPT.gsl) file. */
int pt_read_v3(const char *fn);

/* Read the ItemPT data from a bb-style (ItemPT.gsl) file. */
int pt_read_bb(const char* fn);

/* Did we read in a v2 ItemPT? */
int pt_v2_enabled(void);

/* Did we read in a GC ItemPT? */
int pt_gc_enabled(void);

/* Did we read in a BB ItemPT? */
int pt_bb_enabled(void);

/* Generate an item drop from the PT data. This version uses the v2 PT data set,
   and thus is appropriate for any version before PSOGC. */
int pt_generate_v2_drop(ship_client_t *c, lobby_t *l, void *r);
int pt_generate_v2_boxdrop(ship_client_t *c, lobby_t *l, void *r);

/* Generate an item drop from the PT data. This version uses the v3 PT data set.
   This function only works for PSOGC. */
int pt_generate_gc_drop(ship_client_t *c, lobby_t *l, void *r);
int pt_generate_gc_boxdrop(ship_client_t *c, lobby_t *l, void *r);

/* Generate an item drop from the PT data. This version uses the v3 PT data set.
   This function only works for PSOBB. */
pt_bb_entry_t* get_pt_data_bb(uint8_t episode, uint8_t challenge, uint8_t difficulty, uint8_t section);
uint8_t get_pt_index(uint8_t episode, uint8_t pt_index);
uint8_t get_pt_data_area_bb(uint8_t episode, int cur_area);
int pt_generate_bb_drop(ship_client_t *c, lobby_t *l, void *r);
int pt_generate_bb_boxdrop(ship_client_t *c, lobby_t *l, void *r);

/* Generate an item drop from the PT data. This version uses the v3 PT data set.
   This function only works for PSOBB PSO2 style. */
int pt_generate_bb_pso2_drop(ship_client_t* src, lobby_t* l, void* r);
int pt_generate_bb_pso2_boxdrop(ship_client_t* src, lobby_t* l, uint8_t section, subcmd_bb_itemreq_t* req);

uint32_t rand_int(sfmt_t* rng, uint64_t max);
float rand_float_0_1_from_crypt(sfmt_t* rng);
void generate_common_weapon_bonuses(sfmt_t* rng, item_t* item, uint8_t area_norm, pt_bb_entry_t* ent);
void generate_seal_weapon_extra_bonuses(sfmt_t* rng, item_t* item, uint8_t area_norm, pt_bb_entry_t* ent);
void generate_common_item_variances(lobby_t* l, sfmt_t* rng, uint32_t norm_area, item_t* item, pt_bb_entry_t* ent);
item_t on_box_item_drop(lobby_t* l, sfmt_t* rng, uint8_t pkt_area, uint8_t area, uint8_t section_id);
item_t on_specialized_box_item_drop(lobby_t* l, sfmt_t* rng, uint8_t area, uint32_t def0, uint32_t def1, uint32_t def2);
item_t on_monster_item_drop(lobby_t* l, sfmt_t* rng, uint32_t enemy_type, uint8_t pkt_area, uint8_t area, uint8_t section_id);

uint16_t get_random_value(sfmt_t* rng, rang_16bit_t range);
uint8_t generate_weapon_special(pt_bb_entry_t* ent, uint8_t normarea, sfmt_t* rng);

#endif /* !PTDATA_H */