/*
    梦幻之星中国

    版权 (C) 2010, 2011, 2018, 2019, 2020 Sancaros

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

#ifndef PSOCN_ITEMS_H
#define PSOCN_ITEMS_H

#include <stdint.h>
#include "queue.h"

#include <pso_character.h>

/* Item buckets. Each item gets put into one of these buckets when in the list,
   in order to make searching the list slightly easier. These are all based on
   the least significant byte of the item code. */
#define ITEM_TYPE_WEAPON                0x00
#define ITEM_TYPE_GUARD                 0x01
#define ITEM_TYPE_MAG                   0x02
#define ITEM_TYPE_TOOL                  0x03
#define ITEM_TYPE_MESETA                0x04

/* ITEM_TYPE_GUARD items are actually slightly more specialized, and fall into
   three subtypes of their own. These are the second least significant byte in
   the item code. */
#define ITEM_SUBTYPE_FRAME              0x01
#define ITEM_SUBTYPE_BARRIER            0x02
#define ITEM_SUBTYPE_UNIT               0x03

/* ITEM_TYPE_TOOL subtype*/
#define ITEM_SUBTYPE_MATE               0x00
#define ITEM_SUBTYPE_FLUID              0x01
#define ITEM_SUBTYPE_DISK               0x02
#define ITEM_SUBTYPE_SOL_ATOMIZER       0x03
#define ITEM_SUBTYPE_MOON_ATOMIZER      0x04
#define ITEM_SUBTYPE_STAR_ATOMIZER      0x05
#define ITEM_SUBTYPE_ANTI               0x06
#define ITEM_SUBTYPE_TELEPIPE           0x07
#define ITEM_SUBTYPE_TRAP_VISION        0x08
#define ITEM_SUBTYPE_GRINDER            0x0A
#define ITEM_SUBTYPE_MATERIAL           0x0B
#define ITEM_SUBTYPE_PHOTON             0x10

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

/* Base item structure. This is not generally used directly, but rather as a
   piece of the overall puzzle. */
typedef struct psocn_item {
    TAILQ_ENTRY(psocn_item) qentry;

    uint32_t item_code;
    uint32_t versions;
    int auto_reject;
    int reject_max;
} psocn_item_t;

TAILQ_HEAD(psocn_item_queue, psocn_item);

/* Weapon information structure. This is a "subclass" of the above item struct
   which holds information specific to weapons. */
typedef struct psocn_weapon {
    psocn_item_t base;

    int max_grind;
    int min_grind;
    int max_percents;
    int min_percents;
    int min_hit;
    int max_hit;
    uint64_t valid_attrs;
} psocn_weapon_t;

/* Frame information structure. */
typedef struct psocn_frame {
    psocn_item_t base;

    int max_slots;
    int min_slots;
    int max_dfp;
    int min_dfp;
    int max_evp;
    int min_evp;
} psocn_frame_t;

/* Barrier information structure. */
typedef struct psocn_barrier {
    psocn_item_t base;

    int max_dfp;
    int min_dfp;
    int max_evp;
    int min_evp;
} psocn_barrier_t;

/* Unit information structure. */
typedef struct psocn_unit {
    psocn_item_t base;

    int max_plus;
    int min_plus;
} psocn_unit_t;

/* Mag information structure. */
typedef struct psocn_mag {
    psocn_item_t base;

    int max_level;
    int min_level;
    int max_def;
    int min_def;
    int max_pow;
    int min_pow;
    int max_dex;
    int min_dex;
    int max_mind;
    int min_mind;
    int max_synchro;
    int min_synchro;
    int max_iq;
    int min_iq;
    uint8_t allowed_cpb;
    uint8_t allowed_rpb;
    uint8_t allowed_lpb;
    uint32_t allowed_colors;
} psocn_mag_t;

/* Tool information structure. */
typedef struct psocn_tool {
    psocn_item_t base;

    int max_stack;
    int min_stack;
} psocn_tool_t;

/* Overall list for reading in the configuration.*/
typedef struct psocn_limits {
    struct psocn_item_queue *weapons;
    struct psocn_item_queue *guards;
    struct psocn_item_queue *mags;
    struct psocn_item_queue *tools;

    int default_behavior;
    uint32_t default_colors;
    uint8_t check_srank_names;
    uint8_t check_pbs;
    uint8_t default_cpb;
    uint8_t default_rpb;
    uint8_t default_lpb;
    uint8_t check_wrap;
    int def_min_percent_v1;
    int def_max_percent_v1;
    int def_min_hit_v1;
    int def_max_hit_v1;
    int def_min_percent_v2;
    int def_max_percent_v2;
    int def_min_hit_v2;
    int def_max_hit_v2;
    int def_min_percent_gc;
    int def_max_percent_gc;
    int def_min_hit_gc;
    int def_max_hit_gc;
    int def_min_percent_xbox;
    int def_max_percent_xbox;
    int def_min_hit_xbox;
    int def_max_hit_xbox;
    int check_j_sword;

    char *name;
} psocn_limits_t;

/* Weapon Attributes -- Stored in byte #4 of weapons. */
typedef enum psocn_weapon_attr_e {
    Weapon_Attr_None        = 0x00,
    Weapon_Attr_Draw        = 0x01,
    Weapon_Attr_Drain       = 0x02,
    Weapon_Attr_Fill        = 0x03,
    Weapon_Attr_Gush        = 0x04,
    Weapon_Attr_Heart       = 0x05,
    Weapon_Attr_Mind        = 0x06,
    Weapon_Attr_Soul        = 0x07,
    Weapon_Attr_Geist       = 0x08,
    Weapon_Attr_Masters     = 0x09,
    Weapon_Attr_Lords       = 0x0A,
    Weapon_Attr_Kings       = 0x0B,
    Weapon_Attr_Charge      = 0x0C,
    Weapon_Attr_Spirit      = 0x0D,
    Weapon_Attr_Berserk     = 0x0E,
    Weapon_Attr_Ice         = 0x0F,
    Weapon_Attr_Frost       = 0x10,
    Weapon_Attr_Freeze      = 0x11,
    Weapon_Attr_Blizzard    = 0x12,
    Weapon_Attr_Bind        = 0x13,
    Weapon_Attr_Hold        = 0x14,
    Weapon_Attr_Seize       = 0x15,
    Weapon_Attr_Arrest      = 0x16,
    Weapon_Attr_Heat        = 0x17,
    Weapon_Attr_Fire        = 0x18,
    Weapon_Attr_Flame       = 0x19,
    Weapon_Attr_Burning     = 0x1A,
    Weapon_Attr_Shock       = 0x1B,
    Weapon_Attr_Thunder     = 0x1C,
    Weapon_Attr_Storm       = 0x1D,
    Weapon_Attr_Tempest     = 0x1E,
    Weapon_Attr_Dim         = 0x1F,
    Weapon_Attr_Shadow      = 0x20,
    Weapon_Attr_Dark        = 0x21,
    Weapon_Attr_Hell        = 0x22,
    Weapon_Attr_Panic       = 0x23,
    Weapon_Attr_Riot        = 0x24,
    Weapon_Attr_Havoc       = 0x25,
    Weapon_Attr_Chaos       = 0x26,
    Weapon_Attr_Devils      = 0x27,
    Weapon_Attr_Demons      = 0x28,
    Weapon_Attr_MAX         = 0x28
} psocn_weapon_attr_t;

/* Read the item limits data. You are responsible for calling the function to
   clean everything up when you're done. */
extern int psocn_read_limits(const char *f, psocn_limits_t **l);

/* Clean up the limits data. */
extern int psocn_free_limits(psocn_limits_t *l);

/* Find an item in the limits list, if its there, and check for legitness.
   Returns non-zero if the item is legit. */
extern int psocn_limits_check_item(psocn_limits_t *l,
                                       iitem_t *i, uint32_t version);

/* Retrieve the name of a given weapon attribute. */
extern const char *psocn_weapon_attr_name(psocn_weapon_attr_t num);

#endif /* !PSOCN_ITEMS_H */
