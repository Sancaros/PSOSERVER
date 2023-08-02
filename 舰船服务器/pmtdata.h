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

#include <stdint.h>

#include <mtwist.h>

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
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint32_t team_points;
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

static const int pmtdsa = sizeof(pmt_weapon_bb_t);

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
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint32_t team_points;
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
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint32_t team_points;
    uint16_t stat;
    uint16_t amount;
    uint8_t pm_range;
    uint8_t unused2[3];
} PACKED pmt_unit_bb_t;

/* 工具参数结构 */
typedef struct pmt_tool_bb {
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint32_t team_points;
    uint16_t amount;
    uint16_t tech;
    uint32_t cost;
    uint8_t itemFlag;
    uint8_t reserved[3];
} PACKED pmt_tool_bb_t;

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

typedef struct pmt_mag_bb {
    uint32_t index;
    uint16_t model;
    uint16_t skin;
    uint32_t team_points;
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

typedef struct pmt_table_offsets_bb {
    union {
        struct {
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
        };
        uint32_t ptr[23];
    };
} PACKED pmt_table_offsets_bb_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* PSOv1/PSOv2 data. */
static pmt_weapon_v2_t** weapons = NULL;
static uint32_t* num_weapons = NULL;
static uint32_t num_weapon_types = 0;
static uint32_t weapon_lowest = EMPTY_STRING;

static pmt_guard_v2_t** guards = NULL;
static uint32_t* num_guards = NULL;
static uint32_t num_guard_types = 0;
static uint32_t guard_lowest = EMPTY_STRING;

static pmt_unit_v2_t* units = NULL;
static uint32_t num_units = 0;
static uint32_t unit_lowest = EMPTY_STRING;

static uint8_t* star_table = NULL;
static uint32_t star_max = 0;

/* These three are used in generating random units... */
static uint64_t* units_by_stars = NULL;
static uint32_t* units_with_stars = NULL;
static uint8_t unit_max_stars = 0;

/* PSOGC data. */
static pmt_weapon_gc_t** weapons_gc = NULL;
static uint32_t* num_weapons_gc = NULL;
static uint32_t num_weapon_types_gc = 0;
static uint32_t weapon_lowest_gc = EMPTY_STRING;

static pmt_guard_gc_t** guards_gc = NULL;
static uint32_t* num_guards_gc = NULL;
static uint32_t num_guard_types_gc = 0;
static uint32_t guard_lowest_gc = EMPTY_STRING;

static pmt_unit_gc_t* units_gc = NULL;
static uint32_t num_units_gc = 0;
static uint32_t unit_lowest_gc = EMPTY_STRING;

static uint8_t* star_table_gc = NULL;
static uint32_t star_max_gc = 0;

static uint64_t* units_by_stars_gc = NULL;
static uint32_t* units_with_stars_gc = NULL;
static uint8_t unit_max_stars_gc = 0;

/* PSOBB data. */
static pmt_table_offsets_bb_t* pmt_tb_offsets = NULL;


static pmt_mag_bb_t* pmt_mag_bb = NULL;

static pmt_tool_bb_t* pmt_tool_bb = NULL;

static pmt_weapon_bb_t** weapons_bb = NULL;
static uint32_t* num_weapons_bb = NULL;
static uint32_t num_weapon_types_bb = 0;
static uint32_t weapon_lowest_bb = 0xFFFFFFFF;

static pmt_guard_bb_t** guards_bb = NULL;
static uint32_t* num_guards_bb = NULL;
static uint32_t num_guard_types_bb = 0;
static uint32_t guard_lowest_bb = 0xFFFFFFFF;

static pmt_unit_bb_t* units_bb = NULL;
static uint32_t num_units_bb = 0;
static uint32_t unit_lowest_bb = 0xFFFFFFFF;

static uint8_t* star_table_bb = NULL;
static uint32_t star_max_bb = 0;

static uint64_t* units_by_stars_bb = NULL;
static uint32_t* units_with_stars_bb = NULL;
static uint8_t unit_max_stars_bb = 0;

static int have_v2_pmt = 0;
static int have_gc_pmt = 0;
static int have_bb_pmt = 0;

int pmt_read_v2(const char* fn, int norestrict);
int pmt_read_gc(const char* fn, int norestrict);
int pmt_read_bb(const char* fn, int norestrict);
int pmt_v2_enabled(void);
int pmt_gc_enabled(void);
int pmt_bb_enabled(void);

void pmt_cleanup(void);

int pmt_lookup_weapon_v2(uint32_t code, pmt_weapon_v2_t* rv);
int pmt_lookup_guard_v2(uint32_t code, pmt_guard_v2_t* rv);
int pmt_lookup_unit_v2(uint32_t code, pmt_unit_v2_t* rv);

uint8_t pmt_lookup_stars_v2(uint32_t code);
int pmt_random_unit_v2(uint8_t max, uint32_t item[4],
    struct mt19937_state* rng, lobby_t* l);

int pmt_lookup_weapon_gc(uint32_t code, pmt_weapon_gc_t* rv);
int pmt_lookup_guard_gc(uint32_t code, pmt_guard_gc_t* rv);
int pmt_lookup_unit_gc(uint32_t code, pmt_unit_gc_t* rv);

uint8_t pmt_lookup_stars_gc(uint32_t code);
int pmt_random_unit_gc(uint8_t max, uint32_t item[4],
    struct mt19937_state* rng, lobby_t* l);

int pmt_lookup_weapon_bb(uint32_t code, pmt_weapon_bb_t* rv);
int pmt_lookup_guard_bb(uint32_t code, pmt_guard_bb_t* rv);
int pmt_lookup_unit_bb(uint32_t code, pmt_unit_bb_t* rv);

int pmt_random_unit_bb(uint8_t max, uint32_t item[4],
    struct mt19937_state* rng, lobby_t* l);
uint8_t pmt_lookup_stars_bb(uint32_t code);

#endif /* !PMTDATA_H */
