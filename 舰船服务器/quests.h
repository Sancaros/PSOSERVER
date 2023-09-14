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

#ifndef QUESTS_H
#define QUESTS_H

#include <stdio.h>
#include <inttypes.h>
#include <queue.h>
#include <quest.h>

#define CLIENTS_H_COUNTS_ONLY
#include "clients.h"
#undef CLIENTS_H_COUNTS_ONLY

#ifndef SHIP_DEFINED
#define SHIP_DEFINED
struct ship;
typedef struct ship ship_t;
#endif

#ifndef QENEMY_DEFINED
#define QENEMY_DEFINED
typedef struct psocn_quest_enemy qenemy_t;
#endif

enum QuestScriptVersion {
    DC_NTE = 0,
    DC_V1 = 1,
    DC_V2 = 2,
    PC_V2 = 3,
    GC_NTE = 4,
    GC_V3 = 5,
    XB_V3 = 6,
    GC_EP3 = 7,
    BB_V4 = 8,
    UNKNOWN = 15,
};

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct psocn_quest_file_dc { // Same format for DC v1 and v2
    uint32_t code_offset;
    uint32_t function_table_offset;
    uint32_t size;
    uint32_t unused;
    uint8_t is_download;
    uint8_t unknown1;
    uint16_t quest_number; // 0xFFFF for challenge quests
    char name[0x20];
    char short_description[0x80];
    char long_description[0x120];
} PACKED psocn_quest_file_dc_t;

typedef struct psocn_quest_file_pc {
    uint32_t code_offset;
    uint32_t function_table_offset;
    uint32_t size;
    uint32_t unused;
    uint8_t is_download;
    uint8_t unknown1;
    uint16_t quest_number; // 0xFFFF for challenge quests
    char name[0x20];
    char short_description[0x80];
    char long_description[0x120];
} PACKED psocn_quest_file_pc_t;

// TODO: Is the XB quest header format the same as on GC? If not, make a
// separate struct; if so, rename this struct to V3.
typedef struct psocn_quest_file_gc {
    uint32_t code_offset;
    uint32_t function_table_offset;
    uint32_t size;
    uint32_t unused;
    uint8_t is_download;
    uint8_t unknown1;
    uint8_t quest_number;
    uint8_t episode; // 1 = Ep2. Apparently some quests have 0xFF here, which means ep1 (?)
    char name[0x20];
    char short_description[0x80];
    char long_description[0x120];
} PACKED psocn_quest_file_gc_t;

typedef struct psocn_quest_file_bb {
    uint32_t code_offset;
    uint32_t function_table_offset;
    uint32_t size;
    uint32_t unused;
    uint16_t quest_number; // 0xFFFF for challenge quests
    uint16_t unused2;
    uint8_t episode; // 0 = Ep1, 1 = Ep2, 2 = Ep4
    uint8_t max_players;
    uint8_t joinable_in_progress;
    uint8_t unknown;
    uint16_t name[0x20];
    uint16_t short_description[0x80];
    uint16_t long_description[0x120];
} PACKED psocn_quest_file_bb_t;

static int xdsa = sizeof(psocn_quest_file_bb_t);

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

typedef struct quest_map_elem {
    TAILQ_ENTRY(quest_map_elem) qentry;
    uint32_t qid;

    psocn_quest_t *qptr[CLIENT_VERSION_ALL][CLIENT_LANG_ALL];
} quest_map_elem_t;

TAILQ_HEAD(quest_map, quest_map_elem);
typedef struct quest_map quest_map_t;

/* 返回难度文字名称 */
char abbreviation_for_difficulty(uint8_t difficulty);

/* Find a quest by ID, if it exists */
quest_map_elem_t *quest_lookup(quest_map_t *map, uint32_t qid);

/* Add a quest to the list */
quest_map_elem_t *quest_add(quest_map_t *map, uint32_t qid);

/* Clean the list out */
void quest_cleanup(quest_map_t *map);

/* Process an entire list of quests read in for a version/language combo. */
int quest_map(quest_map_t *map, psocn_quest_list_t *list, int version,
              int language);

/* Build/rebuild the quest enemy/object data cache. */
int quest_cache_maps(ship_t *s, quest_map_t *map, const char *dir, int initial);

/* Search an enemy list from a quest for an entry. */
uint32_t quest_search_enemy_list(uint32_t id, qenemy_t *list, int len, int sd);

#endif /* !QUESTS_H */
