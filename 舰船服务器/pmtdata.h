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

#ifndef PMTDATA_H
#define PMTDATA_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

//#include <mtwist.h>
#include <SFMT.h>

#include "lobby.h"
#include "max_tech_level.h"

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct pmt_countandoffset {
    uint32_t count;
    uint32_t offset;
} PACKED pmt_countandoffset_t;

typedef struct pmt_item_base {
    uint32_t index;
    uint16_t subtype;
    uint16_t skin;
    uint32_t team_points;
} PACKED pmt_item_base_t;

typedef struct pmt_item_base_check {
    pmt_item_base_t base;
    int err;
} PACKED pmt_item_base_check_t;

typedef struct pmt_weapon_v2 {
    uint32_t index;
    uint8_t classes;
    uint8_t unused1;
    uint16_t atp_min;
    uint16_t atp_max;
    uint16_t atp_req;
    uint16_t mst_req;
    uint16_t ata_req;
    uint8_t max_grind;
    uint8_t photon;
    uint8_t special;
    uint8_t ata;
    uint8_t stat_boost;
    uint8_t unused2[3];
} PACKED pmt_weapon_v2_t;

typedef struct pmt_weapon_gc {
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint8_t unused1;
    uint8_t classes;
    uint16_t atp_min;
    uint16_t atp_max;
    uint16_t atp_req;
    uint16_t mst_req;
    uint16_t ata_req;
    uint16_t mst;
    uint8_t max_grind;
    uint8_t photon;
    uint8_t special;
    uint8_t ata;
    uint8_t stat_boost;
    uint8_t projectile;
    uint8_t ptrail_1_x;
    uint8_t ptrail_1_y;
    uint8_t ptrail_2_x;
    uint8_t ptrail_2_y;
    uint8_t ptype;
    uint8_t unk[3];
    uint8_t unused2[2];
    uint8_t tech_boost;
    uint8_t combo_type;
} PACKED pmt_weapon_gc_t;

typedef struct pmt_weapon_bb {
    pmt_item_base_t base;
    uint8_t equip_flag;
    uint8_t unused2;
    uint16_t atp_min;
    uint16_t atp_max;
    uint16_t atp_req;
    uint16_t mst_req;
    uint16_t ata_req;
    uint16_t mst;
    uint8_t max_grind;
    uint8_t photon_color;
    uint8_t special_type;
    uint8_t ata_add;
    uint8_t stat_boost;
    uint8_t projectile;
    uint8_t ptrail_1_x;
    uint8_t ptrail_1_y;
    uint8_t ptrail_2_x;
    uint8_t ptrail_2_y;
    uint8_t ptype;
    uint8_t unknown1;
    uint8_t unknown2;
    uint8_t unknown3;
    uint8_t unknown4;
    uint8_t unknown5;
    uint8_t tech_boost;
    uint8_t combo_type;
} PACKED pmt_weapon_bb_t;

typedef struct pmt_guard_v2 {
    uint32_t index;
    uint16_t base_dfp;
    uint16_t base_evp;
    uint16_t unused1;
    uint8_t equip_flag;
    uint8_t unused2;
    uint8_t level_req;
    uint8_t efr;
    uint8_t eth;
    uint8_t eic;
    uint8_t edk;
    uint8_t elt;
    uint8_t dfp_range;
    uint8_t evp_range;
    uint32_t unused3;
} PACKED pmt_guard_v2_t;

typedef struct pmt_guard_gc {
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint16_t base_dfp;
    uint16_t base_evp;
    uint16_t unused1;
    uint8_t unused2;
    uint8_t equip_flag;
    uint8_t level_req;
    uint8_t efr;
    uint8_t eth;
    uint8_t eic;
    uint8_t edk;
    uint8_t elt;
    uint8_t dfp_range;
    uint8_t evp_range;
    uint32_t unused3;
} PACKED pmt_guard_gc_t;

typedef struct pmt_guard_bb {
    pmt_item_base_t base;
    uint16_t base_dfp;
    uint16_t base_evp;
    uint8_t block_particle;
    uint8_t block_effect;
    uint8_t equip_flag;
    uint8_t unused3;
    uint8_t level_req;
    uint8_t efr;
    uint8_t eth;
    uint8_t eic;
    uint8_t edk;
    uint8_t elt;
    uint8_t dfp_range;
    uint8_t evp_range;
    uint8_t stat_boost;
    uint8_t tech_boost;
    uint8_t unknown1;
    uint8_t unknown2;
} PACKED pmt_guard_bb_t;

typedef struct pmt_unit_v2 {
    uint32_t index;
    uint16_t stat;
    uint16_t amount;
    uint8_t pm_range;
    uint8_t unused[3];
} PACKED pmt_unit_v2_t;

typedef struct pmt_unit_gc {
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint16_t stat;
    uint16_t amount;
    uint8_t pm_range;
    uint8_t unused[3];
} PACKED pmt_unit_gc_t;

typedef struct pmt_unit_bb {
    pmt_item_base_t base;
    uint16_t stat;
    uint16_t amount;
    uint8_t pm_range;
    uint8_t unused2[3];
} PACKED pmt_unit_bb_t;

typedef struct pmt_mag_v2 {
    uint32_t index;
    uint16_t feed_table;
    uint8_t photon_blast;
    uint8_t activation;
    uint8_t on_pb_full;
    uint8_t on_low_hp;
    uint8_t on_death;
    uint8_t on_boss;
    uint8_t pb_full_flag;
    uint8_t low_hp_flag;
    uint8_t death_flag;
    uint8_t boss_flag;
    uint8_t classes;
    uint8_t unused[3];
} PACKED pmt_mag_v2_t;

typedef struct pmt_mag_gc {
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint16_t feed_table;
    uint8_t photon_blast;
    uint8_t activation;
    uint8_t on_pb_full;
    uint8_t on_low_hp;
    uint8_t on_death;
    uint8_t on_boss;
    uint8_t pb_full_flag;
    uint8_t low_hp_flag;
    uint8_t death_flag;
    uint8_t boss_flag;
    uint8_t classes;
    uint8_t unused[3];
} PACKED pmt_mag_gc_t;

/* 玛古 ItemPmt参数结构 */
typedef struct pmt_mag_bb {
    pmt_item_base_t base;
    uint16_t feed_table;
    uint8_t photon_blast;
    uint8_t activation;
    uint8_t on_pb_full;
    uint8_t on_low_hp;
    uint8_t on_death;
    uint8_t on_boss;
    uint8_t pb_full_flag;
    uint8_t low_hp_flag;
    uint8_t death_flag;
    uint8_t boss_flag;
    uint8_t classes;
    uint8_t unused2[3];
} PACKED pmt_mag_bb_t;

/* 工具 ItemPmt参数结构 */
typedef struct pmt_tool_bb {
    pmt_item_base_t base;
    uint16_t amount;
    uint16_t tech;
    uint32_t cost;
    uint8_t itemFlag;
    uint8_t reserved[3];
} PACKED pmt_tool_bb_t;

typedef struct pmt_itemcombination_bb {
    uint8_t used_item[3];
    uint8_t equipped_item[3];
    uint8_t result_item[3];
    uint8_t mag_level;
    uint8_t grind;
    uint8_t level;
    uint8_t char_class;
    uint8_t unused[3];
} PACKED pmt_itemcombination_bb_t;

typedef struct pmt_techniqueboost_bb {
    uint32_t tech1;
    float boost1;
    uint32_t tech2;
    float boost2;
    uint32_t tech3;
    float boost3;
} PACKED pmt_techniqueboost_bb_t;

typedef struct pmt_eventitem_bb {
    uint8_t item[3];
    uint8_t probability;
} PACKED pmt_eventitem_bb_t;

typedef struct pmt_unsealableitem_bb {
    uint8_t item[3];
    uint8_t unused;
} PACKED pmt_unsealableitem_bb_t;

typedef struct pmt_nonweaponsaledivisors_bb {
    float armor_divisor;
    float shield_divisor;
    float unit_divisor;
    float mag_divisor;
} PACKED pmt_nonweaponsaledivisors_bb_t;

typedef struct pmt_weaponsaledivisors_bb {
    float sale_divisor[0xA5];
} PACKED pmt_weaponsaledivisors_bb_t;

typedef struct pmt_mag_feed_result {
    int8_t def;
    int8_t pow;
    int8_t dex;
    int8_t mind;
    int8_t iq;
    int8_t synchro;
    uint8_t unused[2];
} PACKED pmt_mag_feed_result_t;

typedef struct pmt_mag_feed_results_list {
    pmt_mag_feed_result_t results[11];
} PACKED pmt_mag_feed_results_list_t;

typedef struct pmt_mag_feed_results_list_offsets {
    uint32_t offsets[8]; // Offsets of MagFeedResultsList structs
} PACKED pmt_mag_feed_results_list_offsets_t;

typedef struct pmt_special_bb {
    uint16_t type;
    uint16_t amount;
} PACKED pmt_special_bb_t;

typedef struct pmt_max_tech_bb {
    // Indexed as [tech_num][char_class]
    uint8_t max_level[MAX_PLAYER_TECHNIQUES][MAX_PLAYER_CLASS_BB];
} PACKED pmt_max_tech_bb_t;

/* TODO V2 ItemPMT Offset Struct */
typedef struct pmt_table_offsets_v2 {
    union {
        struct {
            /*         GC    V2         */
            /* 00 00 / 14884 0x00000013 */ uint32_t weapon_table; // -> [{count, offset -> [Weapon]}](0xED)
            /* 01 04 / 1478C 0x00005AFC */ uint32_t armor_table; // -> [{count, offset -> [ArmorOrShield]}](2; armors and shields)
            /* 02 08 / 1479C 0x00005A5C */ uint32_t unit_table; // -> {count, offset -> [Unit]} (last if out of range)
            /* 03 0C / 147AC 0x00005A6C */ uint32_t tool_table; // -> [{count, offset -> [Tool]}](0x1A) (last if out of range)
            /* 04 10 / 147A4 0x00005A7C */ uint32_t mag_table; // -> {count, offset -> [Mag]}
            /* 05 14 / 0F4B8 0x00005A74 */ uint32_t attack_animation_table; // -> [uint8_t](0xED)
            /* 06 18 / 0DE7C 0x00003DF8 */ uint32_t photon_color_table; // -> [0x24-byte structs](0x20)
            /* 07 1C / 0E194 0x00002E4C */ uint32_t weapon_range_table; // -> ???
            /* 08 20 / 0F5A8 0x000032CC */ uint32_t weapon_sale_divisor_table; // -> [float](0xED)
            /* 09 24 / 0F83C 0x00003E84 */ uint32_t sale_divisor_table; // -> NonWeaponSaleDivisors
            /* 10 28 / 1502C 0x000040A8 */ uint32_t mag_feed_table; // -> MagFeedResultsTable
            /* 11 2C / 0FB0C 0x00005F4C */ uint32_t star_value_table; // -> [uint8_t] (indexed by .id from weapon, armor, etc.)
            /* 12 30 / 0FE3C 0x00004378 */ uint32_t special_data_table; // -> [Special]
            /* 13 34 / 0FEE0 0x00004540 */ uint32_t weapon_effect_table; // -> [16-byte structs]
            /* 14 38 / 1275C 0x000045E4 */ uint32_t stat_boost_table; // -> [StatBoost]
            /* 15 3C / 11C80 0x000058DC */ uint32_t shield_effect_table; // -> [8-byte structs]
            /* 16 40 / 12894 0x00005704 */ uint32_t max_tech_level_table; // -> MaxTechniqueLevels
            /* 17 44 / 14FF4 0x00000000 */ uint32_t combination_table; // -> {count, offset -> [ItemCombination]}
            /* 18 48 / 12754 0x00000000 */ uint32_t unknown_a1;
            /* 19 4C / 14278 0x00000000 */ uint32_t tech_boost_table; // -> [TechniqueBoost] (always 0x2C of them? from counts struct?)
            /* 20 50 / 15014 0x00000000 */ uint32_t unwrap_table; // -> {count, offset -> [{count, offset -> [EventItem]}]}
            /* 21 54 / 1501C 0x00000000 */ uint32_t unsealable_table; // -> {count, offset -> [UnsealableItem]}
            /* 22 58 / 15024 0x00000000 */ uint32_t ranged_special_table; // -> {count, offset -> [4-byte structs]}
        };
        uint32_t ptr[21];
    };
} PACKED pmt_table_offsets_v2_t;

typedef struct pmt_table_offsets_v3 {
    union {
        struct {
            /*         GC    BB         */
            /* 00 00 / 14884 0x0001489C */ uint32_t weapon_table; // -> [{count, offset -> [Weapon]}](0xED)
            /* 01 04 / 1478C 0x000147A4 */ uint32_t armor_table; // -> [{count, offset -> [ArmorOrShield]}](2; armors and shields)
            /* 02 08 / 1479C 0x000147B4 */ uint32_t unit_table; // -> {count, offset -> [Unit]} (last if out of range)
            /* 03 0C / 147AC 0x000147C4 */ uint32_t tool_table; // -> [{count, offset -> [Tool]}](0x1A) (last if out of range)
            /* 04 10 / 147A4 0x000147BC */ uint32_t mag_table; // -> {count, offset -> [Mag]}
            /* 05 14 / 0F4B8 0x0000F4CC */ uint32_t attack_animation_table; // -> [uint8_t](0xED)
            /* 06 18 / 0DE7C 0x0000DE7C */ uint32_t photon_color_table; // -> [0x24-byte structs](0x20)
            /* 07 1C / 0E194 0x0000E194 */ uint32_t weapon_range_table; // -> ???
            /* 08 20 / 0F5A8 0x0000F5BC */ uint32_t weapon_sale_divisor_table; // -> [float](0xED)
            /* 09 24 / 0F83C 0x0000F850 */ uint32_t sale_divisor_table; // -> NonWeaponSaleDivisors
            /* 10 28 / 1502C 0x00015044 */ uint32_t mag_feed_table; // -> MagFeedResultsTable
            /* 11 2C / 0FB0C 0x0000FB20 */ uint32_t star_value_table; // -> [uint8_t] (indexed by .id from weapon, armor, etc.)
            /* 12 30 / 0FE3C 0x0000FE50 */ uint32_t special_data_table; // -> [Special]
            /* 13 34 / 0FEE0 0x0000FEF4 */ uint32_t weapon_effect_table; // -> [16-byte structs]
            /* 14 38 / 1275C 0x00012770 */ uint32_t stat_boost_table; // -> [StatBoost]
            /* 15 3C / 11C80 0x00011C94 */ uint32_t shield_effect_table; // -> [8-byte structs]
            /* 16 40 / 12894 0x000128A8 */ uint32_t max_tech_level_table; // -> MaxTechniqueLevels
            /* 17 44 / 14FF4 0x0001500C */ uint32_t combination_table; // -> {count, offset -> [ItemCombination]}
            /* 18 48 / 12754 0x00012768 */ uint32_t unknown_a1;
            /* 19 4C / 14278 0x0001428C */ uint32_t tech_boost_table; // -> [TechniqueBoost] (always 0x2C of them? from counts struct?)
            /* 20 50 / 15014 0x0001502C */ uint32_t unwrap_table; // -> {count, offset -> [{count, offset -> [EventItem]}]}
            /* 21 54 / 1501C 0x00015034 */ uint32_t unsealable_table; // -> {count, offset -> [UnsealableItem]}
            /* 22 58 / 15024 0x0001503C */ uint32_t ranged_special_table; // -> {count, offset -> [4-byte structs]}
        };
        uint32_t ptr[23];
    };
} PACKED pmt_table_offsets_v3_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

int pmt_read_v2(const char* fn, int norestrict);
int pmt_read_gc(const char* fn, int norestrict);
int pmt_read_bb(const char* fn, int norestrict);
int pmt_enabled_v2(void);
int pmt_enabled_gc(void);
int pmt_enabled_bb(void);

void pmt_cleanup(void);

int pmt_lookup_weapon_v2(uint32_t code, pmt_weapon_v2_t* rv);
int pmt_lookup_guard_v2(uint32_t code, pmt_guard_v2_t* rv);
int pmt_lookup_unit_v2(uint32_t code, pmt_unit_v2_t* rv);

uint8_t pmt_lookup_stars_v2(uint32_t code);
int pmt_random_unit_v2(uint8_t max, uint32_t item[4],
                       sfmt_t* rng, lobby_t* l);

int pmt_lookup_weapon_gc(uint32_t code, pmt_weapon_gc_t* rv);
int pmt_lookup_guard_gc(uint32_t code, pmt_guard_gc_t* rv);
int pmt_lookup_unit_gc(uint32_t code, pmt_unit_gc_t* rv);

uint8_t pmt_lookup_stars_gc(uint32_t code);
int pmt_random_unit_gc(uint8_t max, uint32_t item[4],
                       sfmt_t* rng, lobby_t* l);

int pmt_lookup_weapon_bb(uint32_t code, pmt_weapon_bb_t* rv);
int pmt_lookup_guard_bb(uint32_t code, pmt_guard_bb_t* rv);
int pmt_lookup_unit_bb(uint32_t code, pmt_unit_bb_t* rv);
int pmt_lookup_mag_bb(uint32_t code, pmt_mag_bb_t* rv);
int pmt_lookup_tools_bb(uint32_t code1, uint32_t code2, pmt_tool_bb_t* rv);
int pmt_lookup_itemcombination_bb(uint32_t code, uint32_t equip_code, pmt_itemcombination_bb_t* rv);
int pmt_lookup_eventitem_bb(uint32_t code, uint32_t index, pmt_eventitem_bb_t* rv);
int pmt_lookup_mag_feed_table_bb(uint32_t code, uint32_t table_index, uint32_t item_index, pmt_mag_feed_result_t* rv);
float pmt_lookup_sale_divisor_bb(uint8_t code1, uint8_t code2);

int pmt_random_unit_bb(uint8_t max, uint32_t item[4],
                       sfmt_t* rng, lobby_t* l);
uint8_t pmt_lookup_stars_bb(uint32_t code);
uint8_t pmt_lookup_max_tech_level_bb(uint8_t tech_num, uint8_t char_class);
/* 表中数值均为16进制字符
, 是从0x00起始为1级 0xFF为未学习
,如果要获取实际显示等级 则需要 + 1
*/
uint8_t get_pmt_max_tech_level_bb(uint8_t tech_num, uint8_t char_class);

pmt_item_base_check_t get_item_definition_bb(const uint32_t datal1, const uint32_t datal2);
bool get_item_pmt_bb(const uint32_t datal1, const uint32_t datal2,
    pmt_weapon_bb_t* weapon, pmt_guard_bb_t* guard, pmt_unit_bb_t* unit, pmt_mag_bb_t* mag, pmt_tool_bb_t* tool);
pmt_item_base_t get_item_base_bb(const item_t* item);
int item_not_identification_bb(const uint32_t code1, const uint32_t code2);
uint32_t get_item_index(const item_t* item);

uint8_t get_item_stars(uint16_t slot);
uint8_t get_item_base_stars(const item_t* item);
uint8_t get_special_stars(uint8_t det);
uint8_t get_item_adjusted_stars(const item_t* item);

bool is_item_rare(const item_t* item);
uint8_t choose_weapon_special(uint8_t det);
int find_tool_by_class(uint8_t tool_class, uint8_t data[2]);
bool is_unsealable_item(const item_t* item);
void set_sealed_item_kill_count(item_t* item, int16_t v);
void set_item_kill_count_if_unsealable(item_t* item);
void set_item_identified_flag(bool is_mode, item_t* item);
void generate_unit_weights_tables(uint8_t unit_weights_table1[0x88], int8_t unit_weights_table2[0x0D]);
uint8_t get_unit_weights_table1(size_t det);
uint8_t get_unit_weights_table2(size_t det);
size_t get_unit_weights_table1_size();
size_t get_num_eventitems_bb(uint8_t datab2);

#endif /* !PMTDATA_H */