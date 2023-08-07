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

#ifdef PACKED
#undef PACKED
#endif

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

struct Special {
    uint16_t type;
    uint16_t amount;
} PACKED;

struct StatBoost {
    uint8_t stat1;
    uint8_t stat2;
    uint16_t amount1;
    uint16_t amount2;
} PACKED;

struct TechniqueBoost {
    uint32_t tech1;
    float boost1;
    uint32_t tech2;
    float boost2;
    uint32_t tech3;
    float boost3;
} PACKED;

struct NonWeaponSaleDivisors {
    float armor_divisor;
    float shield_divisor;
    float unit_divisor;
    float mag_divisor;
} PACKED;

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
    int32_t armor_level;                    /* 0x08B8 */
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
    int32_t armor_level;                    /* 0x0958 */
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
    int32_t armor_level;                    /* 0x0958 */
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
    int8_t element_probability[10];         /* 0x011F */
    int8_t armor_ranking[5];                /* 0x0129 */
    int8_t slot_ranking[5];                 /* 0x012E */
    int8_t unit_level[10];                  /* 0x0133 */
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
    int32_t armor_level;                    /* 0x08B8 */
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
    int32_t armor_level;                    /* 0x0958 */
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
    int32_t armor_level;                    /* 0x0958 */
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

static int have_v2pt = 0;
static int have_gcpt = 0;
static int have_bbpt = 0;

static pt_v2_entry_t v2_ptdata[4][10];
static pt_v3_entry_t gc_ptdata[4][4][10];
//EP1 0 / EP2 1 /  CHALLENGE1 2/ CHALLENGE2 3/ EP4 4
static pt_bb_entry_t bb_ptdata[5][4][10];

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
int pt_generate_bb_drop(ship_client_t *c, lobby_t *l, void *r);
int pt_generate_bb_boxdrop(ship_client_t *c, lobby_t *l, void *r);

/* Generate an item drop from the PT data. This version uses the v3 PT data set.
   This function only works for PSOBB PSO2 style. */
int pt_generate_bb_pso2_drop(ship_client_t* src, lobby_t* l, void* r);
int pt_generate_bb_pso2_boxdrop(ship_client_t* src, lobby_t* l, int section, void* r);

#endif /* !PTDATA_H */
