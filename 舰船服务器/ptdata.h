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
    // This data structure uses index probability tables in multiple places. An
    // index probability table is a table where each entry holds the probability
    // that that entry's index is used. For example, if the armor slot count
    // probability table contains [77, 17, 5, 1, 0], this means there is a 77%
    // chance of no slots, 17% chance of 1 slot, 5% chance of 2 slots, 1% chance
    // of 3 slots, and no chance of 4 slots. The values in index probability
    // tables do not have to add up to 100; the game sums all of them and
    // chooses a random number less than that maximum.

    // The area (floor) number is used in many places as well. Unlike the normal
    // area numbers, which start with Pioneer 2, the area numbers in this
    // structure start with Forest 1, and boss areas are treated as the first
    // area of the next section (so De Rol Le has Mines 1 drops, for example).
    // Final boss areas are treated as the last non-boss area (so Dark Falz
    // boxes are like Ruins 3 boxes). We refer to these adjusted area numbers as
    // (area - 1).

    // This index probability table determines the types of non-rare weapons.
    // The indexes in this table correspond to the non-rare weapon types 01
    // through 0C (Saber through Wand).
    uint8_t base_weapon_type_prob_table[12];               /* 0x0000 base_weapon_type_prob_table */
    // This table specifies the base subtype for each weapon type. Negative
    // values here mean that the weapon cannot be found in the first N areas (so
    // -2, for example, means that the weapon never appears in Forest 1 or 2 at
    // all). Nonnegative values here mean the subtype can be found in all areas,
    // and specify the base subtype (usually in the range [0, 4]). The subtype
    // of weapon that actually appears depends on this value and a value from
    // the following table.
    int8_t subtype_base_table[12];              /* 0x000C subtype_base_table */
    // This table specifies how many areas each weapon subtype appears in. For
    // example, if Sword (subtype 02, which is index 1 in this table and the
    // table above) has a subtype base of -2 and a subtype area lneght of 4,
    // then Sword items can be found when area - 1 is 2, 3, 4, or 5 (Cave 1
    // through Mine 1), and Gigush (the next sword subtype) can be found in Mine
    // 1 through Ruins 3.
    uint8_t subtype_area_length_table[12];            /* 0x0018 subtype_area_length_table */
    // This index probability table specifies how likely each possible grind
    // value is. The table is indexed as [grind][subtype_area_index], where the
    // subtype area index is how many areas the player is beyond the first area
    // in which the subtype can first be found (clamped to [0, 3]). To continue
    // the example above, in Cave 3, subtype_area_index would be 2, since Swords
    // can first be found two areas earlier in Cave 1.
    // For example, this table could look like this:
    //   [64 1E 19 14] // Chance of getting a grind +0
    //   [00 1E 17 0F] // Chance of getting a grind +1
    //   [00 14 14 0E] // Chance of getting a grind +2
    //    ...
    //    C1 C2 C3 M1  // (Episode 1 area values from the example for reference)
    uint8_t grind_prob_tables[9][4];            /* 0x0024 grind_prob_tables */
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
    uint8_t technique_level_ranges[19][20];            /* 0x04CC technique_level_ranges */
    // Each byte in this table (indexed by enemy_type) represents the percent
    // chance that the enemy drops anything at all. (This check is done after
    // the rare drop check, so it only applies to non-rare items.)
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
    int32_t armor_level;                    /* 0x0958 armor_or_shield_type_bias */
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
