/*
    梦幻之星中国

    版权 (C) 2009, 2011, 2014, 2015, 2018, 2019, 2020, 2021,
                  2022 Sancaros

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

#ifndef QUEST_H
#define QUEST_H

#include <stdint.h>

#define PSOCN_QUEST_V1   (1 << 0)
#define PSOCN_QUEST_V2   (1 << 1)
#define PSOCN_QUEST_GC   (1 << 2)
#define PSOCN_QUEST_BB   (1 << 3)
#define PSOCN_QUEST_XBOX (1 << 4)

#define PSOCN_QUEST_NOPC (1 << 8)
#define PSOCN_QUEST_NODC (1 << 9)
#define PSOCN_QUEST_NOXB (1 << 10)
#define PSOCN_QUEST_NOGC (1 << 11)

#define PSOCN_QUEST_NORMAL          (1 << 0)
#define PSOCN_QUEST_BATTLE          (1 << 1)
#define PSOCN_QUEST_CHALLENGE       (1 << 2)
#define PSOCN_QUEST_TEAM            (1 << 3)
#define PSOCN_QUEST_ONEPERSON       (1 << 4)
#define PSOCN_QUEST_EXTERMINATION   (1 << 5)
#define PSOCN_QUEST_GOVERNMENT      (1 << 6)
#define PSOCN_QUEST_RETRIEVAL       (1 << 7)
#define PSOCN_QUEST_VR              (1 << 8)
#define PSOCN_QUEST_NET_CAFE        (1 << 9)
#define PSOCN_QUEST_SHOP            (1 << 10)
#define PSOCN_QUEST_LAB             (1 << 11)
#define PSOCN_QUEST_TOWER           (1 << 12)
#define PSOCN_QUEST_PRESENTS        (1 << 13)
#define PSOCN_QUEST_SIDESTORY       (1 << 14)
#define PSOCN_QUEST_DEBUG           (1 << 31)
#define PSOCN_QUEST_TYPE_MASK       0x000000FF

#define PSOCN_QUEST_JOINABLE    (1 << 0)
#define PSOCN_QUEST_SYNC_REGS   (1 << 1)
#define PSOCN_QUEST_SYNC_ALL    (1 << 2)
#define PSOCN_QUEST_FLAG16      (1 << 3)
#define PSOCN_QUEST_DATAFL      (1 << 5)
#define PSOCN_QUEST_HIDDEN      (1 << 6)

#define PSOCN_QUEST_BINDAT      0
#define PSOCN_QUEST_QST         1
#define PSOCN_QUEST_UBINDAT     2

#define PSOCN_QUEST_ENDROP_NONE     0
#define PSOCN_QUEST_ENDROP_NORARE   1
#define PSOCN_QUEST_ENDROP_PARTIAL  2
#define PSOCN_QUEST_ENDROP_FREE     3

#define PSOCN_QUEST_ENDROP_SDROPS   0x80000000
#define PSOCN_QUEST_ENDROP_CDROPS   0x40000000

#define PSOCN_QUEST_EP1             (1 << 0)
#define PSOCN_QUEST_EP2             (1 << 1)
#define PSOCN_QUEST_EP4             (1 << 2)

struct psocn_quest_enemy {
    uint32_t mask;
    uint32_t key;
    uint32_t value;
};

typedef struct psocn_quest {
    uint32_t qid;
    uint32_t versions;
    uint32_t flags;
    uint32_t privileges;

    char name[32];
    char desc[112];

    char *long_desc;
    char *prefix;
    void *user_data;

    uint64_t start_time;
    uint64_t end_time;

    int episode;
    int event;
    int format;
    int max_players;
    int min_players;

    /* Stuff related to server-side drops. */
    int num_monster_types;
    int num_monster_ids;
    struct psocn_quest_enemy *monster_types;
    struct psocn_quest_enemy *monster_ids;

    /* Stuff related to registers */
    int num_sync;
    uint8_t *synced_regs;
    uint8_t server_flag16_reg;
    uint8_t server_flag32_ctl;
    uint8_t server_flag32_dat;
    uint8_t server_data_reg;
    uint8_t server_ctl_reg;

    int sync;

    char *onload_script_file;
    char *beforeload_script_file;
} psocn_quest_t;

typedef struct psocn_qcat {
    char name[32];
    char desc[112];

    uint32_t type;
    uint32_t episodes;
    uint32_t privileges;
    int quest_count;
    psocn_quest_t **quests;
} psocn_quest_category_t;

typedef struct psocn_qlist {
    psocn_quest_category_t *cats;
    int cat_count;
} psocn_quest_list_t;

extern int psocn_quests_read(const char *filename,
                                 psocn_quest_list_t *rv);

extern void psocn_quests_destroy(psocn_quest_list_t *list);

#endif /* !QUEST_H */
