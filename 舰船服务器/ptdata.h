/*
    梦幻之星中国 舰船服务器
    版权 (C) 2022 Sancaros

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

/* Clean (non-packed) version of the v3 ItemPT entry structure. */
typedef struct pt_v3_entry {
    int8_t weapon_ratio[12];                /* 0x0000 */
    int8_t weapon_minrank[12];              /* 0x000C */
    int8_t weapon_upgfloor[12];             /* 0x0018 */
    int8_t power_pattern[9][4];             /* 0x0024 */
    uint16_t percent_pattern[23][6];        /* 0x0048 */
    int8_t area_pattern[3][10];             /* 0x015C */
    int8_t percent_attachment[6][10];       /* 0x017A */
    int8_t element_ranking[10];             /* 0x01B6 */
    int8_t element_probability[10];         /* 0x01C0 */
    int8_t armor_ranking[5];                /* 0x01CA */
    int8_t slot_ranking[5];                 /* 0x01CF */
    int8_t unit_level[10];                  /* 0x01D4 */
    uint16_t tool_frequency[28][10];        /* 0x01DE */
    uint8_t tech_frequency[19][10];         /* 0x040E */
    int8_t tech_levels[19][20];             /* 0x04CC */
    int8_t enemy_dar[100];                  /* 0x0648 */
    uint16_t enemy_meseta[100][2];          /* 0x06AC */
    int8_t enemy_drop[100];                 /* 0x083C */
    uint16_t box_meseta[10][2];             /* 0x08A0 */
    uint8_t box_drop[7][10];                /* 0x08C8 */
    int32_t armor_level;                    /* 0x0958 */
} pt_v3_entry_t;

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
    int32_t armor_level;                    /* 0x08B8 */
} pt_v2_entry_t;

/* Clean (non-packed) version of the BB ItemPT entry structure. */
typedef struct pt_bb_entry {
    int8_t weapon_ratio[12];                /* 0x0000 */
    int8_t weapon_minrank[12];              /* 0x000C */
    int8_t weapon_upgfloor[12];             /* 0x0018 */
    int8_t power_pattern[9][4];             /* 0x0024 */
    uint16_t percent_pattern[23][6];        /* 0x0048 */
    int8_t area_pattern[3][10];             /* 0x015C */
    int8_t percent_attachment[6][10];       /* 0x017A */
    int8_t element_ranking[10];             /* 0x01B6 */
    int8_t element_probability[10];         /* 0x01C0 */
    int8_t armor_ranking[5];                /* 0x01CA */
    int8_t slot_ranking[5];                 /* 0x01CF */
    int8_t unit_level[10];                  /* 0x01D4 */
    uint16_t tool_frequency[28][10];        /* 0x01DE */
    uint8_t tech_frequency[19][10];         /* 0x040E */
    int8_t tech_levels[19][20];             /* 0x04CC */
    int8_t enemy_dar[100];                  /* 0x0648 */
    uint16_t enemy_meseta[100][2];          /* 0x06AC */
    int8_t enemy_drop[100];                 /* 0x083C */
    uint16_t box_meseta[10][2];             /* 0x08A0 */
    uint8_t box_drop[7][10];                /* 0x08C8 */
    int32_t armor_level;                    /* 0x0958 */
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
int pt_read_v3(const char *fn/*, int bb*/);

/* Read the ItemPT data from a bb-style (ItemPT.gsl) file. */
int pt_read_bb(const char* fn/*, int bb*/);

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

#endif /* !PTDATA_H */
