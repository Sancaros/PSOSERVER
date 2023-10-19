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
#include <pso_items.h>

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
extern const char *psocn_weapon_attr_name(psocn_weapon_common_attr_t num);

#endif /* !PSOCN_ITEMS_H */
