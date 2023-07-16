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

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <f_logs.h>
#include <f_checksum.h>
#include <PRS.h>
#include <pso_StringReader.h>

#include "mapdata.h"
#include "lobby.h"
#include "clients.h"

/* Player levelup data */
extern bb_level_table_t bb_char_stats;
extern v2_level_table_t v2_char_stats;

void print_entry(const bb_battle_param_t* entry) {
    printf("atp: %u\n", entry->atp);
    printf("psv: %u\n", entry->psv);
    printf("evp: %u\n", entry->evp);
    printf("hp: %u\n", entry->hp);
    printf("dfp: %u\n", entry->dfp);
    printf("ata: %u\n", entry->ata);
    printf("lck: %u\n", entry->lck);
    printf("esp: %u\n", entry->esp);
    printf("esp: %u\n", entry->dfp_range);
    printf("esp: %u\n", entry->evp_range);
    printf("esp: %u\n", entry->shield);
    printf("experience: %u\n", entry->exp);
    printf("difficulty: %u\n", entry->difficulty);
}

void print_table(const bb_battle_param_table* table) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 0x60; j++) {
            printf("Difficulty: %d, Entry: %d\n", i, j);
            print_entry(&(table->difficulty[i][j]));
            printf("\n");
        }
    }
}

/* Enemy battle parameters. The array is organized in the following levels:
   multi/single player, episode, difficulty, entry.*/
typedef struct battle_param_checksum {
    int difficulty;
    int entry_num;
    size_t sum;
} battle_param_checksum_t;

const battle_param_checksum_t Battle_Param_Checksum[][3] = {
    {0, 374, 0x906b2d15},
    {1, 374, 0x845a960e},
    {2, 374, 0xd0934eb6},
    {3, 374, 0xbc5eb946},
    {0, 374, 0xca3abc5f},
    {1, 374, 0xc6baa768},
    {2, 374, 0x178df3c5},
    {3, 374, 0xd16feb80},
    {0, 332, 0xd53e105b},
    {1, 332, 0x6cc95d17},
    {2, 332, 0xd6046110},
    {3, 332, 0x9d5325d7},
    {0, 374, 0x67ecaee0},
    {1, 374, 0xdd8831ed},
    {2, 374, 0xa0ce7d8f},
    {3, 374, 0xf59c8220},
    {0, 374, 0xa9bfeea7},
    {1, 374, 0xdaff38de},
    {2, 374, 0x939fd4e7},
    {3, 374, 0x22687177},
    {0, 332, 0x0a3d38c5},
    {1, 332, 0x1ccf3789},
    {2, 332, 0x3c2f1e06},
    {3, 332, 0x2e836fd8}
};

#define NUM_BPEntry 6

typedef struct battle_param_entry_files {
    char* file;
    int is_solo;
    int episode;
    battle_param_checksum_t check_sum;
    //int difficulty;
    //int entry_num;
    //uint32_t check_sum;
} battle_param_entry_files_t;

const battle_param_entry_files_t battle_params_emtry_files[NUM_BPEntry][5] = {
    {"BattleParamEntry_on.dat"    , 0, 0, 0, 374, 0xB8A2D950},
    {"BattleParamEntry_lab_on.dat", 0, 1, 0, 374, 0x4D4059CF},
    {"BattleParamEntry_ep4_on.dat", 0, 2, 0, 332, 0x42BF9716},
    {"BattleParamEntry.dat"       , 1, 0, 0, 374, 0x8FEF1FFE},
    {"BattleParamEntry_lab.dat"   , 1, 1, 0, 374, 0x3DC217F5},
    {"BattleParamEntry_ep4.dat"   , 1, 2, 0, 332, 0x50841167}
};

// online/offline, episode, difficulty, entry_num
static bb_battle_param_t battle_params[2][3][4][0x60];

/* Parsed enemy data. Organized similarly to the battle parameters, except that
   the last level is the actual areas themselves (and there's no difficulty
   level in there)
   分析了敌人的数据。组织与作战参数类似，除了
   最后一个层次是实际区域本身（没有困难）
   在那里的水平）.是否单人/章节/16个变种 */
static parsed_map_t bb_parsed_maps[2][3][0x10];
static parsed_objs_t bb_parsed_objs[2][3][0x10];

/* Did we read in Blue Burst map data? */
static int have_bb_maps = 0;

/* V2 Parsed enemy data. This is much simpler, since there's no episodes nor
   single-player mode to worry about. */
static parsed_map_t v2_parsed_maps[0x10];
static parsed_objs_t v2_parsed_objs[0x10];

/* Did we read in v2 map data? */
static int have_v2_maps = 0;

/* GC Parsed enemy data. Midway point between v2 and BB. Two episodes, but no
   single player mode. */
static parsed_map_t gc_parsed_maps[2][0x10];
static parsed_objs_t gc_parsed_objs[2][0x10];

/* Did we read in gc map data? */
static int have_gc_maps = 0;

/* Header for sections of the .dat files for quests. */
typedef struct quest_dat_hdr {
    uint32_t obj_type;
    uint32_t next_hdr;
    uint32_t area;
    uint32_t size;
    uint8_t data[];
} quest_dat_hdr_t;

static int read_bb_battle_param_file(bb_battle_param_t dst[4][0x60], const char* fn, const char* file) {
    FILE* fp;
    const size_t sz = 0x60 * sizeof(bb_battle_param_t);
    char buf[128] = { 0 };
    sprintf(buf, "%s%s", fn, file);

    if (!(fp = fopen(buf, "rb"))) {
        ERR_LOG("无法打开 %s 并读取: %s", buf, strerror(errno));
        return 1;
    }

    /* Read each difficulty in... */
    for (int difficulty = 0; difficulty < 4; difficulty++) {
        if (fread(dst[difficulty], 1, sz, fp) != sz) {
            ERR_LOG("无法读取来自 %s 的数据: %s", buf, strerror(errno));
            return 1;
        }

        //if (battle_checksum != Battle_Param_Checksum[difficulty][i].sum)
        //{
        //    printf("0x%08X %s check_num %d difficulty %d 文件校对: 0x%08X\n", Battle_Param_Checksum[difficulty][i].sum, &buf[0], check_num, difficulty, battle_checksum);
        //    //printf("警告: 战斗参数文件 %s 已被修改.\n", &buf[0]);
        //}
        //i++;
    }

    fclose(fp);
    return 0;
}

static int read_v2_level_data(const char *fn) {
    uint8_t *buf;
    int decsize;

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    int i, j;
#endif

    /* Read in the file and decompress it. */
    if((decsize = pso_prs_decompress_file(fn, &buf)) < 0) {
        ERR_LOG("无法读取等级参数 %s: %s", fn, strerror(-decsize));
        return -1;
    }

    memcpy(&v2_char_stats, buf + 8, sizeof(v2_level_table_t));

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    /* Swap all the exp values */
    for(j = 0; j < MAX_PLAYER_CLASS_DC; ++j) {
        for(i = 0; i < MAX_PLAYER_LEVEL; ++i) {
            v2_char_stats.levels[j][i].exp =
                LE32(v2_char_stats.levels[j][i].exp);
        }
    }
#endif

    /* Clean up... */
    free_safe(buf);

    return 0;
}

typedef struct {
    uint32_t hildeblue;
    uint32_t rappy;
    uint32_t nar_lily;
    uint32_t pouilly_slime;
    uint32_t merissa_aa;
    uint32_t pazuzu;
    uint32_t dorphon_eclair;
    uint32_t kondrieu;
} RareEnemyRates;

RareEnemyRates default_rare_rates = {
    // All 1/512 except Kondrieu, which is 1/10
    .hildeblue = 0x00800000,
    .rappy = 0x00800000,
    .nar_lily = 0x00800000,
    .pouilly_slime = 0x00800000,
    .merissa_aa = 0x00800000,
    .pazuzu = 0x00800000,
    .dorphon_eclair = 0x00800000,
    .kondrieu = 0x1999999A
};

bool check_rare(bool default_is_rare, uint32_t rare_rate, int* rare_enemy_indexes, size_t enemy_size) {
    if (default_is_rare) {
        return true;
    }
    if ((*rare_enemy_indexes < 0x10) && ((uint32_t)rand() < rare_rate)) {
        rare_enemy_indexes[(*rare_enemy_indexes)++] = enemy_size;
        return true;
    }
    return false;
}

bool updateEnemyRareness(RareEnemyRates* rare_rates, int* rare_enemy_indexes, size_t enemy_size) {
    if (!rare_rates) {
        rare_rates = &default_rare_rates;
    }

    bool is_rare = false;
    is_rare = 
        check_rare(false, rare_rates->hildeblue, rare_enemy_indexes, enemy_size) || 
        check_rare(false, rare_rates->rappy, rare_enemy_indexes, enemy_size) ||
        check_rare(false, rare_rates->nar_lily, rare_enemy_indexes, enemy_size) ||
        check_rare(false, rare_rates->pouilly_slime, rare_enemy_indexes, enemy_size) ||
        check_rare(false, rare_rates->merissa_aa, rare_enemy_indexes, enemy_size) ||
        check_rare(false, rare_rates->pazuzu, rare_enemy_indexes, enemy_size) ||
        check_rare(false, rare_rates->dorphon_eclair, rare_enemy_indexes, enemy_size) ||
        check_rare(false, rare_rates->kondrieu, rare_enemy_indexes, enemy_size);

    return is_rare;
}

/* 3个章节 32个地图*/
static const uint32_t maps[3][0x20] = {
    {1,1,1,5,1,5,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1,1,1,1,1,1,1,1,1,1},
    {1,1,2,1,2,1,2,1,2,1,1,3,1,3,1,3,2,2,1,3,2,2,2,2,1,1,1,1,1,1,1,1},
    {1,1,1,3,1,3,1,3,1,3,1,3,3,1,1,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static const uint32_t sp_maps[3][0x20] = {
    {1,1,1,3,1,3,3,1,3,1,3,1,3,2,3,2,3,2,3,2,3,2,1,1,1,1,1,1,1,1,1,1},
    {1,1,2,1,2,1,2,1,2,1,1,3,1,3,1,3,2,2,1,3,2,1,2,1,1,1,1,1,1,1,1,1},
    {1,1,1,3,1,3,1,3,1,3,1,3,3,1,1,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

static const int max_area[3] = { 0x0E, 0x0F, 0x09 };

static int parse_map(map_enemy_t *en, int en_ct, game_enemies_t *game,
                     int ep, int alt, int area) {
    int i, j;
    game_enemy_t *gen;
    void *tmp;
    uint32_t count = 0;
    uint16_t n_clones;
    int acc;

    /* Allocate the space first */
    if(!(gen = (game_enemy_t *)malloc(sizeof(game_enemy_t) * 0xB50))) {
        ERR_LOG("无法分配敌人结构的内存: %s", strerror(errno));
        return -1;
    }

    /* Clear it */
    memset(gen, 0, sizeof(game_enemy_t) * 0xB50);

    /* Parse each enemy. */
    for(i = 0; i < en_ct; ++i) {
        n_clones = en[i].num_children;
        gen[count].area = area;

        switch(en[i].base_type) {
            case 0x0040:    /* Hildebear & Hildetorr */
                acc = en[i].skin & 0x01;
                gen[count].bp_entry = 0x49 + acc;
                gen[count].rt_index = 0x01 + acc;
                break;

            case 0x0041:    /* Rappies */
                acc = en[i].skin & 0x01;
                if(ep == LOBBY_EPISODE_4) {   /* Del Rappy & Sand Rappy */
                    if(alt) {
                        gen[count].bp_entry = 0x17 + acc;
                        gen[count].rt_index = 0x11 + acc;
                    }
                    else {
                        gen[count].bp_entry = 0x05 + acc;
                        gen[count].rt_index = 0x11 + acc;
                    }
                }
                else {
                    if(acc) { // Rag Rappy and Al Rappy (Love for Episode II)
                        gen[count].bp_entry = 0x19;

                        if(ep == LOBBY_EPISODE_1) {
                            gen[count].rt_index = 0x06;
                        }
                        else {
                            /* We need to fill this in when we make the lobby,
                               since it's dependent on the event. */
                            gen[count].rt_index = (uint8_t)-1;
                        }
                    }
                    else {
                        gen[count].bp_entry = 0x18;
                        gen[count].rt_index = 0x05;
                    }
                }
                break;

            case 0x0042:    /* Monest + 30 Mothmants */
                gen[count].bp_entry = 0x01;
                gen[count].rt_index = 0x04;

                for(j = 0; j < 30; ++j) {
                    ++count;
                    gen[count].bp_entry = 0x00;
                    gen[count].rt_index = 0x03;
                    gen[count].area = area;
                }
                break;

            case 0x0043:    /* Savage Wolf & Barbarous Wolf */
                acc = (en[i].reserved[10] & 0x800000) ? 1 : 0;
                gen[count].bp_entry = 0x02 + acc;
                gen[count].rt_index = 0x07 + acc;
                break;

            case 0x0044:    /* Booma family */
                acc = en[i].skin % 3;
                gen[count].bp_entry = 0x4B + acc;
                gen[count].rt_index = 0x09 + acc;
                break;

            case 0x0060:    /* Grass Assassin */
                gen[count].bp_entry = 0x4E;
                gen[count].rt_index = 0x0C;
                break;

            case 0x0061:    /* Del Lily, Poison Lily, Nar Lily */
                if(ep == 2 && alt) {
                    gen[count].bp_entry = 0x25;
                    gen[count].rt_index = 0x53;
                }
                else {
                    acc = (en[i].reserved[10] & 0x800000) ? 1 : 0;
                    gen[count].bp_entry = 0x04 + acc;
                    gen[count].rt_index = 0x0D + acc;
                }
                break;

            case 0x0062:    /* Nano Dragon */
                gen[count].bp_entry = 0x1A;
                gen[count].rt_index = 0x0F;
                break;

            case 0x0063:    /* Shark Family */
                acc = en[i].skin % 3;
                gen[count].bp_entry = 0x4F + acc;
                gen[count].rt_index = 0x10 + acc;
                break;

            case 0x0064:    /* Slime + 4 clones */
                acc = (en[i].reserved[10] & 0x800000) ? 1 : 0;
                gen[count].bp_entry = 0x30 - acc;
                gen[count].rt_index = 0x13 + acc;

                for(j = 0; j < 4; ++j) {
                    ++count;
                    gen[count].bp_entry = 0x30;
                    gen[count].rt_index = 0x13;
                    gen[count].area = area;
                }
                break;

            case 0x0065:    /* Pan Arms, Migium, Hidoom */
                for(j = 0; j < 3; ++j) {
                    gen[count + j].bp_entry = 0x31 + j;
                    gen[count + j].rt_index = 0x15 + j;
                    gen[count + j].area = area;
                }

                count += 2;
                break;

            case 0x0080:    /* Dubchic & Gilchic */
                acc = en[i].skin & 0x01;
                gen[count].bp_entry = 0x1B + acc;
                gen[count].rt_index = (0x18 + acc) << acc;
                break;

            case 0x0081:    /* Garanz */
                gen[count].bp_entry = 0x1D;
                gen[count].rt_index = 0x19;
                break;

            case 0x0082:    /* Sinow Beat & Sinow Gold */
                acc = (en[i].reserved[10] & 0x800000) ? 1 : 0;
                if(acc) {
                    gen[count].bp_entry = 0x13;
                    gen[count].rt_index = 0x1B;
                }
                else {
                    gen[count].bp_entry = 0x06;
                    gen[count].rt_index = 0x1A;
                }

                if(!n_clones)
                    n_clones = 4;
                break;

            case 0x0083:    /* Canadine */
                gen[count].bp_entry = 0x07;
                gen[count].rt_index = 0x1C;
                break;

            case 0x0084:    /* Canadine Group */
                gen[count].bp_entry = 0x09;
                gen[count].rt_index = 0x1D;

                for(j = 0; j < 8; ++j) {
                    ++count;
                    gen[count].bp_entry = 0x08;
                    gen[count].rt_index = 0x1C;
                    gen[count].area = area;
                }
                break;

            case 0x0085:    /* Dubwitch */
                break;

            case 0x00A0:    /* Delsaber */
                gen[count].bp_entry = 0x52;
                gen[count].rt_index = 0x1E;
                break;

            case 0x00A1:    /* Chaos Sorcerer */
                gen[count].bp_entry = 0x0A;
                gen[count].rt_index = 0x1F;

                /* Bee L */
                gen[count + 1].bp_entry = 0x0B;
                gen[count + 1].rt_index = 0x00;
                gen[count + 1].area = area;

                /* Bee R */
                gen[count + 2].bp_entry = 0x0C;
                gen[count + 2].rt_index = 0x00;
                gen[count + 2].area = area;
                count += 2;
                break;

            case 0x00A2:    /* Dark Gunner */
                gen[count].bp_entry = 0x1E;
                gen[count].rt_index = 0x22;
                break;

            case 0x00A3:    /* Death Gunner? */
                break;

            case 0x00A4:    /* Chaos Bringer */
                gen[count].bp_entry = 0x0D;
                gen[count].rt_index = 0x24;
                break;

            case 0x00A5:    /* Dark Belra */
                gen[count].bp_entry = 0x0E;
                gen[count].rt_index = 0x25;
                break;

            case 0x00A6:    /* Dimenian Family */
                acc = en[i].skin % 3;
                gen[count].bp_entry = 0x53 + acc;
                gen[count].rt_index = 0x29 + acc;
                break;

            case 0x00A7:    /* Bulclaw + 4 Claws */
                gen[count].bp_entry = 0x1F;
                gen[count].rt_index = 0x28;

                for(j = 0; j < 4; ++j) {
                    ++count;
                    gen[count].bp_entry = 0x20;
                    gen[count].rt_index = 0x26;
                    gen[count].area = area;
                }
                break;

            case 0x00A8:    /* Claw */
                gen[count].bp_entry = 0x20;
                gen[count].rt_index = 0x26;
                break;

            case 0x00C0:    /* Dragon or Gal Gryphon */
                if(ep == 1) {
                    gen[count].bp_entry = 0x12;
                    gen[count].rt_index = 0x2C;
                }
                else {
                    gen[count].bp_entry = 0x1E;
                    gen[count].rt_index = 0x4D;
                }
                break;

            case 0x00C1:    /* De Rol Le */
                gen[count].bp_entry = 0x0F;
                gen[count].rt_index = 0x2D;
                break;

            case 0x00C2:    /* Vol Opt (form 1) */
                break;

            case 0x00C5:    /* Vol Opt (form 2) */
                gen[count].bp_entry = 0x25;
                gen[count].rt_index = 0x2E;
                break;

            case 0x00C8:    /* Dark Falz (3 forms) + 510 Darvants */
                /* Deal with the Darvants first. */
                for(j = 0; j < 510; ++j) {
                    gen[count].bp_entry = 0x35;
                    gen[count].area = area;
                    gen[count++].rt_index = 0;
                }

                /* The forms are backwards in their ordering... */
                gen[count].bp_entry = 0x38;
                gen[count].area = area;
                gen[count++].rt_index = 0x2F;

                gen[count].bp_entry = 0x37;
                gen[count].area = area;
                gen[count++].rt_index = 0x2F;

                gen[count].bp_entry = 0x36;
                gen[count].area = area;
                gen[count].rt_index = 0x2F;
                break;

            case 0x00CA:    /* Olga Flow */
                gen[count].bp_entry = 0x2C;
                gen[count].rt_index = 0x4E;
                count += 512;
                break;

            case 0x00CB:    /* Barba Ray */
                gen[count].bp_entry = 0x0F;
                gen[count].rt_index = 0x49;
                count += 47;
                break;

            case 0x00CC:    /* Gol Dragon */
                gen[count].bp_entry = 0x12;
                gen[count].rt_index = 0x4C;
                count += 5;
                break;

            case 0x00D4:    /* Sinow Berill & Spigell */
                /* XXXX: How to do rare? Tethealla looks at skin, Newserv at the
                   reserved[10] value... */
                acc = en[i].skin >= 0x01 ? 1 : 0;
                if(acc) {
                    gen[count].bp_entry = 0x13;
                    gen[count].rt_index = 0x3F;
                }
                else {
                    gen[count].bp_entry = 0x06;
                    gen[count].rt_index = 0x3E;
                }

                count += 4; /* Add 4 clones which are never used... */
                break;

            case 0x00D5:    /* Merillia & Meriltas */
                acc = en[i].skin & 0x01;
                gen[count].bp_entry = 0x4B + acc;
                gen[count].rt_index = 0x34 + acc;
                break;

            case 0x00D6:    /* Mericus, Merikle, or Mericarol */
                acc = en[i].skin % 3;
                if(acc)
                    gen[count].bp_entry = 0x44 + acc;
                else
                    gen[count].bp_entry = 0x3A;

                gen[count].rt_index = 0x38 + acc;
                break;

            case 0x00D7:    /* Ul Gibbon & Zol Gibbon */
                acc = en[i].skin & 0x01;
                gen[count].bp_entry = 0x3B + acc;
                gen[count].rt_index = 0x3B + acc;
                break;

            case 0x00D8:    /* Gibbles */
                gen[count].bp_entry = 0x3D;
                gen[count].rt_index = 0x3D;
                break;

            case 0x00D9:    /* Gee */
                gen[count].bp_entry = 0x07;
                gen[count].rt_index = 0x36;
                break;

            case 0x00DA:    /* Gi Gue */
                gen[count].bp_entry = 0x1A;
                gen[count].rt_index = 0x37;
                break;

            case 0x00DB:    /* Deldepth */
                gen[count].bp_entry = 0x30;
                gen[count].rt_index = 0x47;
                break;

            case 0x00DC:    /* Delbiter */
                gen[count].bp_entry = 0x0D;
                gen[count].rt_index = 0x48;
                break;

            case 0x00DD:    /* Dolmolm & Dolmdarl */
                acc = en[i].skin & 0x01;
                gen[count].bp_entry = 0x4F + acc;
                gen[count].rt_index = 0x40 + acc;
                break;

            case 0x00DE:    /* Morfos */
                gen[count].bp_entry = 0x41;
                gen[count].rt_index = 0x42;
                break;

            case 0x00DF:    /* Recobox & Recons */
                gen[count].bp_entry = 0x41;
                gen[count].rt_index = 0x43;

                for(j = 1; j <= n_clones; ++j) {
                    ++count;
                    gen[count].bp_entry = 0x42;
                    gen[count].rt_index = 0x44;
                    gen[count].area = area;
                }

                /* Don't double count them. */
                n_clones = 0;
                break;

            case 0x00E0:    /* Epsilon, Sinow Zoa & Zele */
                if(ep == 2 && alt) {
                    gen[count].bp_entry = 0x23;
                    gen[count].rt_index = 0x54;
                    count += 4;
                }
                else {
                    acc = en[i].skin & 0x01;
                    gen[count].bp_entry = 0x43 + acc;
                    gen[count].rt_index = 0x45 + acc;
                }
                break;

            case 0x00E1:    /* Ill Gill */
                gen[count].bp_entry = 0x26;
                gen[count].rt_index = 0x52;
                break;

            case 0x0110:    /* Astark */
                gen[count].bp_entry = 0x09;
                gen[count].rt_index = 0x01;
                break;

            case 0x0111:    /* Satellite Lizard & Yowie */
                acc = (en[i].reserved[10] & 0x800000) ? 1 : 0;
                if(alt)
                    gen[count].bp_entry = 0x0D + acc + 0x10;
                else
                    gen[count].bp_entry = 0x0D + acc;

                gen[count].rt_index = 0x02 + acc;
                break;

            case 0x0112:    /* Merissa A/AA */
                acc = en[i].skin & 0x01;
                gen[count].bp_entry = 0x19 + acc;
                gen[count].rt_index = 0x04 + acc;
                break;

            case 0x0113:    /* Girtablulu */
                gen[count].bp_entry = 0x1F;
                gen[count].rt_index = 0x06;
                break;

            case 0x0114:    /* Zu & Pazuzu */
                acc = en[i].skin & 0x01;
                if(alt)
                    gen[count].bp_entry = 0x07 + acc + 0x14;
                else
                    gen[count].bp_entry = 0x07 + acc;

                gen[count].rt_index = 7 + acc;
                break;

            case 0x0115:    /* Boota Family */
                acc = en[i].skin % 3;
                gen[count].rt_index = 0x09 + acc;
                if(en[i].skin & 0x02)
                    gen[count].bp_entry = 0x03;
                else
                    gen[count].bp_entry = 0x00 + acc;
                break;

            case 0x0116:    /* Dorphon & Eclair */
                acc = en[i].skin & 0x01;
                gen[count].bp_entry = 0x0F + acc;
                gen[count].rt_index = 0x0C + acc;
                break;

            case 0x0117:    /* Goran Family */
                acc = en[i].skin % 3;
                gen[count].bp_entry = 0x11 + acc;
                if(en[i].skin & 0x02)
                    gen[count].rt_index = 0x0F;
                else if(en[i].skin & 0x01)
                    gen[count].rt_index = 0x10;
                else
                    gen[count].rt_index = 0x0E;
                break;

            case 0x119: /* Saint Million, Shambertin, & Kondrieu */
                acc = en[i].skin & 0x01;
                gen[count].bp_entry = 0x22;
                if(en[i].reserved[10] & 0x800000)
                    gen[count].rt_index = 0x15;
                else
                    gen[count].rt_index = 0x13 + acc;
                break;

            default:
#ifdef VERBOSE_DEBUGGING1
                ERR_LOG("未知敌人 ID: %04X RT_ID: %04X SKIN: %04X NUM_CLONES: %04X", en[i].base, en[i].rt_index, en[i].skin, en[i].num_clones);
#endif
                break;
        }

        /* Increment the counter, as needed */
        if(n_clones) {
            for(j = 0; j < n_clones; ++j, ++count) {
                gen[count + 1].rt_index = gen[count].rt_index;
                gen[count + 1].bp_entry = gen[count].bp_entry;
                gen[count + 1].area = area;
            }
        }
        ++count;
    }

    /* Resize, so as not to waste space */
    if(!(tmp = realloc(gen, sizeof(game_enemy_t) * count))) {
        ERR_LOG("无法重置 %d 个敌人结构的内存大小: %s", count, strerror(errno));
        tmp = gen;
    }

    game->enemies = (game_enemy_t *)tmp;
    game->count = count;

    return 0;
}

static int read_bb_map_set(int solo, int episode, int area, char* dir) {
    int srv;
    char fn[256];
    int k, l, nmaps, nvars, m;
    FILE *fp;
    long sz;
    map_enemy_t *en;
    map_object_t *obj;
    game_object_t *gobj;
    game_enemies_t *tmp;
    game_objs_t *tmp2;

    if(!solo) {
        /* 多人模式 */
        nmaps = maps[episode][area << 1];
        nvars = maps[episode][(area << 1) + 1];
        //printf("mult nmaps = %d (%d %d %d) nvars = %d\n", nmaps, episode, area << 1, (area << 1) + 1, nvars);
    }
    else {
        /* 单人模式 */
        nmaps = sp_maps[episode][area << 1];
        nvars = sp_maps[episode][(area << 1) + 1];
        //printf("solo nmaps = %d (%d %d %d) nvars = %d\n", nmaps, episode, area << 1, (area << 1) + 1, nvars);
    }

    bb_parsed_maps[solo][episode][area].map_count = nmaps;
    bb_parsed_maps[solo][episode][area].variation_count = nvars;

    bb_parsed_objs[solo][episode][area].map_count = nmaps;
    bb_parsed_objs[solo][episode][area].variation_count = nvars;

    if(!(tmp = (game_enemies_t *)malloc(sizeof(game_enemies_t) * nmaps *
                                        nvars))) {
        ERR_LOG("内存空间分配错误 bbmaps: %s",
              strerror(errno));
        return 10;
    }

    bb_parsed_maps[solo][episode][area].data = tmp;

    if(!(tmp2 = (game_objs_t *)malloc(sizeof(game_objs_t) * nmaps * nvars))) {
        ERR_LOG("内存空间分配错误 bbobjs: %s", strerror(errno));
        return 11;
    }

    bb_parsed_objs[solo][episode][area].data = tmp2;

    for(k = 0; k < nmaps; ++k) {                /* 地图编号 */
        for(l = 0; l < nvars; ++l) {            /* 变化 */
            tmp[k * nvars + l].count = 0;
            fp = NULL;

            /*  对于单人模式，请先尝试单人特定地图，  然后尝试多人游戏（因为有些地图是共享的） . 
            * 单人 章节 区域 K 
            s%d%X%d%d.dat
            */
            if(solo) {
                srv = snprintf(fn, 256, "%s\\map\\solo\\s%d%X%d%d.dat", dir, episode + 1, area, k, l);
                if(srv >= 256) {
                    return 1;
                }
                //printf("bbmapsolo %s \n", fn);

                fp = fopen(fn, "rb");
            }

            if(!fp) {
                srv = snprintf(fn, 256, "%s\\map\\mult\\m%d%X%d%d.dat", dir, episode + 1, area, k, l);
                if(srv >= 256) {
                    return 1;
                }

                //printf("bbmapmult %s \n", fn);

                if(!(fp = fopen(fn, "rb"))) {
                    ERR_LOG("无法读取 map 文件 \"%s\": %s", fn,
                          strerror(errno));
                    return 2;
                }
            }

            /* Figure out how long the file is, so we know what to read in... */
            if(fseek(fp, 0, SEEK_END) < 0) {
                ERR_LOG("无法查找字符终点: %s", strerror(errno));
                fclose(fp);
                return 3;
            }

            if((sz = ftell(fp)) < 0) {
                ERR_LOG("无法获取文件大小: %s", strerror(errno));
                fclose(fp);
                return 4;
            }

            if(fseek(fp, 0, SEEK_SET) < 0) {
                ERR_LOG("无法查找字符起点: %s", strerror(errno));
                fclose(fp);
                return 5;
            }

            /* Make sure the size is sane */
            if(sz % 0x48) {
                ERR_LOG("无效地图大小!");
                fclose(fp);
                return 6;
            }

            /* Allocate memory and read in the file. */
            if(!(en = (map_enemy_t*)malloc(sz))) {
                ERR_LOG("分配地图怪物内存错误: %s", strerror(errno));
                fclose(fp);
                return 7;
            }

            if(fread(en, 1, sz, fp) != (size_t)sz) {
                ERR_LOG("无法读取文件: %s", fn);
                free_safe(en);
                fclose(fp);
                return 8;
            }

            /* We're done with the file, so close it */
            fclose(fp);

            /* Parse */
            if(parse_map(en, sz / 0x48, &tmp[k * nvars + l], episode + 1,
                         0, area)) {
                free_safe(en);
                return 9;
            }

            /* Clean up, we're done with this for now... */
            free_safe(en);
            fp = NULL;

            /* 获取对象 */
            if(solo) {
                srv = snprintf(fn, 256, "%s\\objs\\solo\\s%d%X%d%d_o.dat", dir, episode + 1, area, k, l);
                if(srv >= 256) {
                    return 1;
                }
                //printf("bbobjsolo %s \n", fn);

                fp = fopen(fn, "rb");
            }

            if(!fp) {
                srv = snprintf(fn, 256, "%s\\objs\\mult\\m%d%X%d%d_o.dat", dir, episode + 1, area, k, l);
                if(srv >= 256) {
                    return 1;
                }
                //printf("bbobjmult %s \n", fn);

                if(!(fp = fopen(fn, "rb"))) {
                    ERR_LOG("无法读取 objects 文件 \"%s\": %s",
                          fn, strerror(errno));
                    return 2;
                }
            }

            /* Figure out how long the file is, so we know what to read in... */
            if(fseek(fp, 0, SEEK_END) < 0) {
                ERR_LOG("无法查找到文件终点: %s", strerror(errno));
                fclose(fp);
                return 3;
            }

            if((sz = ftell(fp)) < 0) {
                ERR_LOG("无法获取文件大小: %s", strerror(errno));
                fclose(fp);
                return 4;
            }

            if(fseek(fp, 0, SEEK_SET) < 0) {
                ERR_LOG("无法查找到文件起点: %s", strerror(errno));
                fclose(fp);
                return 5;
            }

            /* Make sure the size is sane */
            if(sz % 0x44) {
                ERR_LOG("无效!");
                fclose(fp);
                return 6;
            }

            /* Allocate memory and read in the file. */
            if(!(obj = (map_object_t *)malloc(sz))) {
                ERR_LOG("分配内存错误: %s", strerror(errno));
                fclose(fp);
                return 7;
            }

            if (fread(obj, 1, sz, fp) != (size_t)sz) {
                ERR_LOG("无法读取文件 %s", fn);
                free_safe(obj);
                fclose(fp);
                return 8;
            }

            /* We're done with the file, so close it */
            fclose(fp);

            /* Make space for the game object representation. */
            gobj = (game_object_t *)malloc((sz / 0x44) * sizeof(game_object_t));
            if(!gobj) {
                ERR_LOG("无法为游戏 objects 分配内存空间: %s",
                      strerror(errno));
                free_safe(obj);
                //fclose(fp);
                return 9;
            }

            /* Store what we'll actually use later... */
            for(m = 0; m < sz / 0x44; ++m) {
                gobj[m].data = obj[m];
                gobj[m].flags = 0;
                gobj[m].area = area;
            }

            /* Save it into the struct */
            tmp2[k * nvars + l].count = sz / 0x44;
            tmp2[k * nvars + l].objs = gobj;

            free_safe(obj);
        }
    }

    return 0;
}

static int read_v2_map_set(int j, int gcep, char* dir) {
    int srv, ep;
    char fn[256];
    int k, l, nmaps, nvars, i;
    FILE *fp;
    long sz;
    map_enemy_t *en;
    map_object_t *obj;
    game_object_t *gobj;
    game_enemies_t *tmp;
    game_objs_t *tmp2;

    if(!gcep) {
        nmaps = maps[0][j << 1];
        nvars = maps[0][(j << 1) + 1];
        ep = 1;
    }
    else {
        nmaps = maps[gcep - 1][j << 1];
        nvars = maps[gcep - 1][(j << 1) + 1];
        ep = gcep;
    }

    if(!gcep) {
        v2_parsed_maps[j].map_count = nmaps;
        v2_parsed_maps[j].variation_count = nvars;
        v2_parsed_objs[j].map_count = nmaps;
        v2_parsed_objs[j].variation_count = nvars;
    }
    else {
        gc_parsed_maps[gcep - 1][j].map_count = nmaps;
        gc_parsed_maps[gcep - 1][j].variation_count = nvars;
        gc_parsed_objs[gcep - 1][j].map_count = nmaps;
        gc_parsed_objs[gcep - 1][j].variation_count = nvars;
    }

    if(!(tmp = (game_enemies_t *)malloc(sizeof(game_enemies_t) * nmaps *
                                        nvars))) {
        ERR_LOG("内存空间分配错误 maps: %s", strerror(errno));
        return 10;
    }

    if(!gcep)
        v2_parsed_maps[j].data = tmp;
    else
        gc_parsed_maps[gcep - 1][j].data = tmp;

    if(!(tmp2 = (game_objs_t *)malloc(sizeof(game_objs_t) * nmaps * nvars))) {
        ERR_LOG("内存空间分配错误 objs: %s", strerror(errno));
        return 11;
    }

    if(!gcep)
        v2_parsed_objs[j].data = tmp2;
    else
        gc_parsed_objs[gcep - 1][j].data = tmp2;

    for(k = 0; k < nmaps; ++k) {                /* 地图编号 */
        for(l = 0; l < nvars; ++l) {            /* 变化 */
            tmp[k * nvars + l].count = 0;

            if (!gcep) {
                srv = snprintf(fn, 256, "%s\\map\\mult\\m%X%d%d.dat", dir, j, k, l);
                //printf("gcep = %d mult %s \n", gcep, fn);
            }
            else
                srv = snprintf(fn, 256, "%s\\map\\mult\\m%d%X%d%d.dat", dir, gcep, j, k, l);


            if(srv >= 256) {
                return 1;
            }

            if(!(fp = fopen(fn, "rb"))) {
                ERR_LOG("无法读取 map %s: %s", fn,
                      strerror(errno));
                return 2;
            }

            /* Figure out how long the file is, so we know what to read in... */
            if(fseek(fp, 0, SEEK_END) < 0) {
                ERR_LOG("无法查找到文件终点: %s", strerror(errno));
                fclose(fp);
                return 3;
            }

            if((sz = ftell(fp)) < 0) {
                ERR_LOG("无法获取文件大小: %s", strerror(errno));
                fclose(fp);
                return 4;
            }

            if(fseek(fp, 0, SEEK_SET) < 0) {
                ERR_LOG("无法查找到文件起点: %s", strerror(errno));
                fclose(fp);
                return 5;
            }

            /* Make sure the size is sane */
            if(sz % 0x48) {
                ERR_LOG("地图文件大小无效!");
                fclose(fp);
                return 6;
            }

            /* Allocate memory and read in the file. */
            if(!(en = (map_enemy_t *)malloc(sz))) {
                ERR_LOG("分配内存错误: %s", strerror(errno));
                fclose(fp);
                return 7;
            }

            if(fread(en, 1, sz, fp) != (size_t)sz) {
                ERR_LOG("无法读取 file!");
                free_safe(en);
                fclose(fp);
                return 8;
            }

            /* We're done with the file, so close it */
            fclose(fp);

            /* Parse */
            if(parse_map(en, sz / 0x48, &tmp[k * nvars + l], ep, 0, j)) {
                free_safe(en);
                return 9;
            }

            /* Clean up, we're done with this for now... */
            free_safe(en);

            /* Now, grab the objects */
            if (!gcep) {
                srv = snprintf(fn, 256, "%s\\objs\\mult\\m%X%d%d_o.dat", dir, j, k, l);
                //printf("gcep = %d objs %s \n", gcep, fn);
            }
            else
                srv = snprintf(fn, 256, "%s\\objs\\mult\\m%d%X%d%d_o.dat", dir, gcep, j, k, l);


            if(srv >= 256) {
                return 1;
            }

            if(!(fp = fopen(fn, "rb"))) {
                ERR_LOG("无法读取 objects: %s", strerror(errno));
                return 2;
            }

            /* Figure out how long the file is, so we know what to read in... */
            if(fseek(fp, 0, SEEK_END) < 0) {
                ERR_LOG("无法查找到文件终点: %s", strerror(errno));
                fclose(fp);
                return 3;
            }

            if((sz = ftell(fp)) < 0) {
                ERR_LOG("无法获取文件大小: %s", strerror(errno));
                fclose(fp);
                return 4;
            }

            if(fseek(fp, 0, SEEK_SET) < 0) {
                ERR_LOG("无法查找到文件起点: %s", strerror(errno));
                fclose(fp);
                return 5;
            }

            /* Make sure the size is sane */
            if(sz % 0x44) {
                ERR_LOG("地图文件大小无效!");
                fclose(fp);
                return 6;
            }

            /* Allocate memory and read in the file. */
            if(!(obj = (map_object_t *)malloc(sz))) {
                ERR_LOG("分配内存错误: %s", strerror(errno));
                fclose(fp);
                return 7;
            }

            if(fread(obj, 1, sz, fp) != (size_t)sz) {
                ERR_LOG("无法读取文件!");
                free_safe(obj);
                fclose(fp);
                return 8;
            }

            /* We're done with the file, so close it */
            fclose(fp);

            /* Make space for the game object representation. */
            gobj = (game_object_t *)malloc((sz / 0x44) * sizeof(game_object_t));
            if(!gobj) {
                ERR_LOG("无法为 game objects 分配内存: %s",
                      strerror(errno));
                free_safe(obj);
                //fclose(fp);
                return 9;
            }

            /* Store what we'll actually use later... */
            for(i = 0; i < sz / 0x44; ++i) {
                gobj[i].data = obj[i];
                gobj[i].flags = 0;
                gobj[i].area = j;
            }

            /* Save it into the struct */
            tmp2[k * nvars + l].count = sz / 0x44;
            tmp2[k * nvars + l].objs = gobj;

            free_safe(obj);
        }
    }

    return 0;
}

static int read_bb_map_files(char* fn) {
    int srv, i, j, k;
    for (i = 0; i < 2;++i) {
        //printf("k = %d \n", k);
        for (j = 0; j < 3; ++j) {                            /* 章节 */
            for (k = 0; k < 16 && k <= max_area[j]; ++k) {   /* 区域 */
                /* 读取多人和单人模式地图. */
                srv = read_bb_map_set(i, j, k, fn);
                /*if ((srv = read_bb_map_set(0, i, j, fn)))
                    return srv;
                if ((srv = read_bb_map_set(1, i, j, fn)))
                    return srv;*/
            }
        }
    }

    return srv;
}

static int read_v2_map_files(char* fn) {
    int srv, j;

    for(j = 0; j < 16 && j <= max_area[0]; ++j) {
        if((srv = read_v2_map_set(j, 0, fn)))
            return srv;
    }

    return 0;
}

static int read_gc_map_files(char* fn) {
    int srv, j;

    for(j = 0; j < 16 && j <= max_area[0]; ++j) {
        if((srv = read_v2_map_set(j, 1, fn)))
            return srv;
    }

    for(j = 0; j < 16 && j <= max_area[1]; ++j) {
        if((srv = read_v2_map_set(j, 2, fn)))
            return srv;
    }

    return 0;
}

int bb_read_params2(psocn_ship_t* cfg) {
    int rv = 0;
    long sz;
    char* buf, * path;

    /* Make sure we have a directory set... */
    if (!cfg->bb_param_dir || !cfg->bb_map_dir) {
        ERR_LOG("未找到 Blue Burst 参数文件 且/或 map 路径设置!"
            "关闭 Blue Burst 版本支持.");
        return 1;
    }

    /* Save the current working directory, so we can do this a bit easier. */
    sz = GetCurrentDirectory(0, NULL);
    if (!(buf = malloc(sz))) {
        ERR_LOG("Error allocating memory: %s\n", strerror(errno));
        return -1;
    }

    if (!(path = _getcwd(buf, (size_t)sz))) {
        ERR_LOG("Error getting current dir: %s\n", strerror(errno));
        free(buf);
        return -1;
    }

    if (_chdir(cfg->bb_param_dir)) {
        ERR_LOG("Error changing to Blue Burst param dir: %s\n",
            strerror(errno));
        free(buf);
        return 1;
    }

    /* Attempt to read all the files. */
    DBG_LOG("Loading Blue Burst battle parameter data...\n");
    //rv = read_param_file(battle_params[0][0], "BattleParamEntry_on.dat");
    //rv += read_param_file(battle_params[0][1], "BattleParamEntry_lab_on.dat");
    //rv += read_param_file(battle_params[0][2], "BattleParamEntry_ep4_on.dat");
    //rv += read_param_file(battle_params[1][0], "BattleParamEntry.dat");
    //rv += read_param_file(battle_params[1][1], "BattleParamEntry_lab.dat");
    //rv += read_param_file(battle_params[1][2], "BattleParamEntry_ep4.dat");
    int i = 0, j = 0, k = 0;
    for (i; i < NUM_BPEntry; i++) {
        //strcpy(&buf[i][0], path);
        //strcat(&buf[i][0], battle_params_emtry_files[i][0].file);
        printf("%s %s  %d %d\n", path, battle_params_emtry_files[i][0].file, j, k);
        rv += read_bb_battle_param_file(battle_params[j][k], path,
            battle_params_emtry_files[i][0].file/*,
            battle_params_emtry_files[i][0].check_sum.entry_num,
            battle_params_emtry_files[i][0].check_sum.sum*/);
        if (j < 1 && i >= 2)
            j++;
        if (k < 2)
            k++;
        if (i == 2)
            k = 0;
    }

    /* Try to read the levelup data */
    //DBG_LOG("Loading Blue Burst levelup table...\n");
    //rv += read_bb_level_data("PlyLevelTbl.prs");

    /* Change back to the original directory. */
    if (_chdir(path)) {
        ERR_LOG("Cannot change back to original dir: %s\n",
            strerror(errno));
        free(buf);
        return -1;
    }

    /* Bail out early, if appropriate. */
    if (rv) {
        goto bail;
    }

    /* Next, try to read the map data */
    if (_chdir(cfg->bb_map_dir)) {
        ERR_LOG("Error changing to Blue Burst param dir: %s\n",
            strerror(errno));
        free(buf);
        return 1;
    }

    DBG_LOG("Loading Blue Burst Map Enemy Data...\n");
    rv = read_bb_map_files(cfg->bb_map_dir);

    /* Change back to the original directory */
    if (_chdir(path)) {
        ERR_LOG("Cannot change back to original dir: %s\n",
            strerror(errno));
        free(buf);
        return -1;
    }

bail:
    if (rv) {
        ERR_LOG("Error reading Blue Burst data, disabling Blue Burst "
            "support!\n");
    }
    else {
        have_bb_maps = 1;
    }

    /* Clean up and return. */
    free(buf);
    return rv;
}

int bb_read_params(psocn_ship_t *cfg) {
    int rv = 0;
    CONFIG_LOG("读取 Blue Burst 参数文件");

    /* Make sure we have a directory set... */
    if(!cfg->bb_param_dir || !cfg->bb_map_dir) {
        ERR_LOG("未找到 Blue Burst 参数文件 且/或 map 路径设置!"
              "关闭 Blue Burst 版本支持.");
        return 1;
    }

    /* Attempt to read all the files. */
    CONFIG_LOG("读取 Blue Burst 战斗参数数据...");

    int i = 0, j= 0, k = 0;
    for (i; i < NUM_BPEntry;i++) {
#ifdef DEBUG
        printf("%s %s  %d %d\n", cfg->bb_param_dir, battle_params_emtry_files[i][0].file, j, k);
#endif // DEBUG
        rv += read_bb_battle_param_file(battle_params[j][k], cfg->bb_param_dir,
            battle_params_emtry_files[i][0].file/*,
            battle_params_emtry_files[i][0].check_sum.entry_num,
            battle_params_emtry_files[i][0].check_sum.sum*/);
        if (j < 1 && i >= 2)
            j++;
        if (k < 2)
            k++;
        if (i == 2)
            k = 0;
    }

    /* Bail out early, if appropriate. */
    if(rv) {
        goto bail;
    }

    CONFIG_LOG("读取 Blue Burst 地图敌人数据...");
    rv = read_bb_map_files(cfg->bb_map_dir);

bail:
    if(rv) {
        ERR_LOG("读取 Blue Burst 数据错误, 取消 Blue Burst "
              "版本支持!");
    }
    else {
        have_bb_maps = 1;
    }

    /* Clean up and return. */
    return rv;
}

int v2_read_params(psocn_ship_t *cfg) {
    int rv = 0;
    //long sz;
    //char buf[256] = { 0 }, * path;

    ///* Save the current working directory, so we can do this a bit easier. */
    //sz = MAX_PATH;//pathconf(".", _PC_PATH_MAX);
    //if(!(buf = malloc(sz))) {
    //    ERR_LOG("分配内存错误: %s", strerror(errno));
    //    return -1;
    //}

    //if(!(path = _getcwd(buf, (size_t)sz))) {
    //    ERR_LOG("获取当前目录时出错: %s", strerror(errno));
    //    free_safe(buf);
    //    return -1;
    //}

    if(cfg->v2_param_dir) {
        //if(_chdir(cfg->v2_param_dir)) {
        //    ERR_LOG("跳转至 v2 param 参数路径错误: %s",
        //          strerror(errno));
        //    free_safe(buf);
        //    return -1;
        //}
        //sprintf_s(&buf[0],sizeof(cfg->v2_param_dir),"%s\\PlayerTable.prs", cfg->v2_param_dir);
        /* Try to read the levelup data */
        CONFIG_LOG("读取 v2 升级数据表...");
        read_v2_level_data("System\\Battle\\v2\\PlayerTable.prs");

        ///* Change back to the original directory. */
        //if(_chdir(path)) {
        //    ERR_LOG("无法更改回原始目录: %s",
        //          strerror(errno));
        //    free_safe(buf);
        //    return -1;
        //}
    }

    /* Make sure we have a directory set... */
    if (!cfg->v2_map_dir) {
        CONFIG_LOG("未设置 v2 map 路径. 将取消服务器侧掉落支持.");
        return 1;
    }

    ///* Next, try to read the map data */
    //if(_chdir(cfg->v2_map_dir)) {
    //    ERR_LOG("跳转至 v2 地图路径错误: %s",
    //          strerror(errno));
    //    rv = 1;
    //    goto bail;
    //}

    CONFIG_LOG("读取 v2 地图敌人数据...");
    rv = read_v2_map_files(cfg->v2_map_dir);

    ///* Change back to the original directory */
    //if(_chdir(path)) {
    //    ERR_LOG("无法更改回原始目录: %s",
    //          strerror(errno));
    //    free_safe(buf);
    //    return -1;
    //}

//bail:
//    if(rv) {
//        ERR_LOG("读取 v2 数据错误, 将取消服务器侧 v1/v2 支持!");
//    }
//    else {
//        have_v2_maps = 1;
//    }

    /* Clean up and return. */
    //free_safe(buf);
    return rv;
}

int gc_read_params(psocn_ship_t *cfg) {
    int rv = 0;
    //long sz;
    //char *buf, *path;

    /* Make sure we have a directory set... */
    if(!cfg->gc_map_dir) {
        ERR_LOG("读取 GameCube 地图路径未设置, 将取消服务器侧掉落支持!");
        return 1;
    }

    /* Save the current working directory, so we can do this a bit easier. */
    //sz = MAX_PATH;//pathconf(".", _PC_PATH_MAX);
    //if(!(buf = malloc(sz))) {
    //    ERR_LOG("分配内存错误: %s", strerror(errno));
    //    return -1;
    //}

    //if(!(path = _getcwd(buf, (size_t)sz))) {
    //    ERR_LOG("获取当前目录时出错: %s", strerror(errno));
    //    free_safe(buf);
    //    return -1;
    //}

    ///* Next, try to read the map data */
    //if(_chdir(cfg->gc_map_dir)) {
    //    ERR_LOG("跳转至 GC 地图路径错误: %s",
    //          strerror(errno));
    //    rv = 1;
    //    goto bail;
    //}

    CONFIG_LOG("读取 GameCube 地图敌人数据...");
    rv = read_gc_map_files(cfg->gc_map_dir);

    ///* Change back to the original directory */
    //if(_chdir(path)) {
    //    ERR_LOG("无法更改回原始目录: %s",
    //          strerror(errno));
    //    free_safe(buf);
    //    return -1;
    //}

//bail:
    if(rv) {
        ERR_LOG("读取 GameCube 数据错误, 将取消服务器侧 PSO GameCube 掉落支持!");
    }
    else {
        have_gc_maps = 1;
    }

    ///* Clean up and return. */
    //free_safe(buf);
    return rv;
}

void bb_free_params(void) {
    int i, j, k;
    uint32_t l, nmaps;
    parsed_map_t* m = { 0 };
    parsed_objs_t *o = { 0 };

    for(i = 0; i < 2; ++i) {
        for(j = 0; j < 3; ++j) {
            for(k = 0; k < 0x10; ++k) {
                m = &bb_parsed_maps[i][j][k];
                o = &bb_parsed_objs[i][j][k];
                nmaps = m->map_count * m->variation_count;

                for(l = 0; l < nmaps; ++l) {
                    free_safe(m->data[l].enemies);
                    free_safe(o->data[l].objs);
                }

                free_safe(m->data);
                free_safe(o->data);
                m->data = NULL;
                m->map_count = m->variation_count = 0;
                o->data = NULL;
                o->map_count = o->variation_count = 0;
            }
        }
    }
}

void v2_free_params(void) {
    int k;
    uint32_t l, nmaps;
    parsed_map_t *m = { 0 };
    parsed_objs_t *o = { 0 };

    for(k = 0; k < 0x10; ++k) {
        m = &v2_parsed_maps[k];
        o = &v2_parsed_objs[k];
        nmaps = m->map_count * m->variation_count;

        for(l = 0; l < nmaps; ++l) {
            free_safe(m->data[l].enemies);
            free_safe(o->data[l].objs);
        }

        free_safe(m->data);
        free_safe(o->data);
        m->data = NULL;
        m->map_count = m->variation_count = 0;
        o->data = NULL;
        o->map_count = o->variation_count = 0;
    }
}

void gc_free_params(void) {
    int k, j;
    uint32_t l, nmaps;
    parsed_map_t *m = { 0 };
    parsed_objs_t *o = { 0 };

    for(j = 0; j < 2; ++j) {
        for(k = 0; k < 0x10; ++k) {
            m = &gc_parsed_maps[j][k];
            o = &gc_parsed_objs[j][k];
            nmaps = m->map_count * m->variation_count;

            for(l = 0; l < nmaps; ++l) {
                free_safe(m->data[l].enemies);
                free_safe(o->data[l].objs);
            }

            free_safe(m->data);
            free_safe(o->data);
            m->data = NULL;
            m->map_count = m->variation_count = 0;
            o->data = NULL;
            o->map_count = o->variation_count = 0;
        }
    }
}

int bb_load_game_enemies(lobby_t *l) {
    game_enemies_t *en;
    game_objs_t *ob;
    int solo = (l->flags & LOBBY_FLAG_SINGLEPLAYER) ? 1 : 0, i;
    uint32_t enemies = 0, index, objects = 0, index2;
    parsed_map_t *maps;
    parsed_objs_t *objs;
    game_enemies_t *sets[0x10] = { 0 };
    game_objs_t *osets[0x10] = { 0 };

    /* Figure out the parameter set that will be in use first... */
    l->bb_params = battle_params[solo][l->episode - 1][l->difficulty];

    /* Figure out the total number of enemies that the game will have... */
    for(i = 0; i < 0x20; i += 2) {
        maps = &bb_parsed_maps[solo][l->episode - 1][i >> 1];
        objs = &bb_parsed_objs[solo][l->episode - 1][i >> 1];

        /* If we hit zeroes, then we're done already... */
        if(maps->map_count == 0 && maps->variation_count == 0) {
            sets[i >> 1] = NULL;
            break;
        }
        //printf("%d > %d map_count l->episode - 1 = %d\n", l->maps[i], maps->map_count, l->episode - 1);
        //printf("%d > %d variation_count\n", l->maps[i + 1], maps->variation_count);
        /* Sanity Check! */
        if(l->maps[i] > maps->map_count ||
           l->maps[i + 1] > maps->variation_count) {
            ERR_LOG("创建地图等级 %d 无效(章节 %d): "
                  "(%d %d)", i, l->episode, l->maps[i], l->maps[i + 1]);
            return -1;
        }

        index = l->maps[i] * maps->variation_count + l->maps[i + 1];
        enemies += maps->data[index].count;
        objects += objs->data[index].count;
        sets[i >> 1] = &maps->data[index];
        osets[i >> 1] = &objs->data[index];
    }

    /* Allocate space for the enemy set and the enemies therein. */
    if(!(en = (game_enemies_t *)malloc(sizeof(game_enemies_t)))) {
        ERR_LOG("分配敌人设置内存错误: %s", strerror(errno));
        return -2;
    }

    if(!(en->enemies = (game_enemy_t *)malloc(sizeof(game_enemy_t) * enemies))) {
        ERR_LOG("分配敌人群体内存错误: %s", strerror(errno));
        free_safe(en);
        return -3;
    }

    /* Allocate space for the object set and the objects therein. */
    if(!(ob = (game_objs_t *)malloc(sizeof(game_objs_t)))) {
        ERR_LOG("分配实例设置内存错误: %s", strerror(errno));
        free_safe(en->enemies);
        free_safe(en);
        return -4;
    }

    if(!(ob->objs = (game_object_t *)malloc(sizeof(game_object_t) * objects))) {
        ERR_LOG("分配实例群体内存错误: %s", strerror(errno));
        free_safe(ob);
        free_safe(en->enemies);
        free_safe(en);
        return -5;
    }

    en->count = enemies;
    ob->count = objects;
    index = index2 = 0;

    /* Copy in the enemy data. */
    for(i = 0; i < 0x10; ++i) {
        if(!sets[i] || !osets[i])
            break;

        memcpy(&en->enemies[index], sets[i]->enemies,
               sizeof(game_enemy_t) * sets[i]->count);
        index += sets[i]->count;
        memcpy(&ob->objs[index2], osets[i]->objs,
               sizeof(game_object_t) * osets[i]->count);
        index2 += osets[i]->count;
    }

    /* Fixup Dark Falz' data for difficulties other than normal and the special
       Rappy data too...
       修正除正常数据和特殊Rappy数据之外的其他困难的Dark Falz数据*/
    for(i = 0; i < (int)en->count; ++i) {
        if(en->enemies[i].bp_entry == 0x37 && l->difficulty) {
            en->enemies[i].bp_entry = 0x38;
        }
        else if(en->enemies[i].rt_index == (uint8_t)-1) {
            switch(l->event) {
                case LOBBY_EVENT_CHRISTMAS:
                    en->enemies[i].rt_index = 79;
                    break;
                case LOBBY_EVENT_EASTER:
                    en->enemies[i].rt_index = 81;
                    break;
                case LOBBY_EVENT_HALLOWEEN:
                    en->enemies[i].rt_index = 80;
                    break;
                default:
                    en->enemies[i].rt_index = 51;
                    break;
            }
        }
    }

    /* Done! */
    l->map_enemies = en;
    l->map_objs = ob;
    return 0;
}

int v2_load_game_enemies(lobby_t *l) {
    game_enemies_t *en;
    game_objs_t *ob;
    int i;
    uint32_t enemies = 0, index, objects = 0, index2;
    parsed_map_t *maps;
    parsed_objs_t *objs;
    game_enemies_t *sets[0x10] = { 0 };
    game_objs_t *osets[0x10] = { 0 };

    /* Figure out the total number of enemies that the game will have... */
    for(i = 0; i < 0x20; i += 2) {
        maps = &v2_parsed_maps[i >> 1];
        objs = &v2_parsed_objs[i >> 1];

        /* If we hit zeroes, then we're done already... */
        if(maps->map_count == 0 && maps->variation_count == 0) {
            sets[i >> 1] = NULL;
            break;
        }

        /* Sanity Check! */
        if(l->maps[i] > maps->map_count ||
           l->maps[i + 1] > maps->variation_count) {
            ERR_LOG("创建地图等级 %d 无效(章节 %d): "
                  "(%d %d)", i, l->episode, l->maps[i], l->maps[i + 1]);
            return -1;
        }

        index = l->maps[i] * maps->variation_count + l->maps[i + 1];
        enemies += maps->data[index].count;
        objects += objs->data[index].count;
        sets[i >> 1] = &maps->data[index];
        osets[i >> 1] = &objs->data[index];
    }

    /* Allocate space for the enemy set and the enemies therein. */
    if (!(en = (game_enemies_t*)malloc(sizeof(game_enemies_t)))) {
        ERR_LOG("分配敌人设置内存错误: %s", strerror(errno));
        return -2;
    }

    if (!(en->enemies = (game_enemy_t*)malloc(sizeof(game_enemy_t) * enemies))) {
        ERR_LOG("分配敌人群体内存错误: %s", strerror(errno));
        free_safe(en);
        return -3;
    }

    /* Allocate space for the object set and the objects therein. */
    if (!(ob = (game_objs_t*)malloc(sizeof(game_objs_t)))) {
        ERR_LOG("分配实例设置内存错误: %s", strerror(errno));
        free_safe(en->enemies);
        free_safe(en);
        return -4;
    }

    if (!(ob->objs = (game_object_t*)malloc(sizeof(game_object_t) * objects))) {
        ERR_LOG("分配实例群体内存错误: %s", strerror(errno));
        free_safe(ob);
        free_safe(en->enemies);
        free_safe(en);
        return -5;
    }

    en->count = enemies;
    ob->count = objects;
    index = index2 = 0;

    /* Copy in the enemy data. */
    for(i = 0; i < 0x10; ++i) {
        if(!sets[i] || !osets[i])
            break;

        memcpy(&en->enemies[index], sets[i]->enemies,
               sizeof(game_enemy_t) * sets[i]->count);
        index += sets[i]->count;

        memcpy(&ob->objs[index2], osets[i]->objs,
               sizeof(game_object_t) * osets[i]->count);
        index2 += osets[i]->count;
    }

    /* Fixup Dark Falz' data for difficulties other than normal... */
    if(l->difficulty) {
        for(i = 0; i < (int)en->count; ++i) {
            if(en->enemies[i].bp_entry == 0x37) {
                en->enemies[i].bp_entry = 0x38;
            }
        }
    }

    /* Done! */
    l->map_enemies = en;
    l->map_objs = ob;
    return 0;
}

int gc_load_game_enemies(lobby_t *l) {
    game_enemies_t *en;
    game_objs_t *ob;
    int i;
    uint32_t enemies = 0, index, objects = 0, index2;
    parsed_map_t *maps;
    parsed_objs_t *objs;
    game_enemies_t *sets[0x10] = { 0 };
    game_objs_t *osets[0x10] = { 0 };

    /* Figure out the total number of enemies that the game will have... */
    for(i = 0; i < 0x20; i += 2) {
        maps = &gc_parsed_maps[l->episode - 1][i >> 1];
        objs = &gc_parsed_objs[l->episode - 1][i >> 1];

        /* If we hit zeroes, then we're done already... */
        if(maps->map_count == 0 && maps->variation_count == 0) {
            sets[i >> 1] = NULL;
            break;
        }

        /* Sanity Check! */
        if(l->maps[i] > maps->map_count ||
           l->maps[i + 1] > maps->variation_count) {
            ERR_LOG("创建地图等级 %d 无效(章节 %d): "
                  "(%d %d)", i, l->episode, l->maps[i], l->maps[i + 1]);
            return -1;
        }

        index = l->maps[i] * maps->variation_count + l->maps[i + 1];
        enemies += maps->data[index].count;
        objects += objs->data[index].count;
        sets[i >> 1] = &maps->data[index];
        osets[i >> 1] = &objs->data[index];
    }

    /* Allocate space for the enemy set and the enemies therein. */
    if (!(en = (game_enemies_t*)malloc(sizeof(game_enemies_t)))) {
        ERR_LOG("分配敌人设置内存错误: %s", strerror(errno));
        return -2;
    }

    if (!(en->enemies = (game_enemy_t*)malloc(sizeof(game_enemy_t) * enemies))) {
        ERR_LOG("分配敌人群体内存错误: %s", strerror(errno));
        free_safe(en);
        return -3;
    }

    /* Allocate space for the object set and the objects therein. */
    if (!(ob = (game_objs_t*)malloc(sizeof(game_objs_t)))) {
        ERR_LOG("分配实例设置内存错误: %s", strerror(errno));
        free_safe(en->enemies);
        free_safe(en);
        return -4;
    }

    if (!(ob->objs = (game_object_t*)malloc(sizeof(game_object_t) * objects))) {
        ERR_LOG("分配实例群体内存错误: %s", strerror(errno));
        free_safe(ob);
        free_safe(en->enemies);
        free_safe(en);
        return -5;
    }

    en->count = enemies;
    ob->count = objects;
    index = index2 = 0;

    /* Copy in the enemy data. */
    for(i = 0; i < 0x10; ++i) {
        if(!sets[i] || !osets[i])
            break;

        memcpy(&en->enemies[index], sets[i]->enemies,
               sizeof(game_enemy_t) * sets[i]->count);
        index += sets[i]->count;

        memcpy(&ob->objs[index2], osets[i]->objs,
               sizeof(game_object_t) * osets[i]->count);
        index2 += osets[i]->count;
    }

    /* Fixup Dark Falz' data for difficulties other than normal and the special
       Rappy data too... */
    for(i = 0; i < (int)en->count; ++i) {
        if(en->enemies[i].bp_entry == 0x37 && l->difficulty) {
            en->enemies[i].bp_entry = 0x38;
        }
        else if(en->enemies[i].rt_index == (uint8_t)-1) {
            switch(l->event) {
                case LOBBY_EVENT_CHRISTMAS:
                    en->enemies[i].rt_index = 79;
                    break;
                case LOBBY_EVENT_EASTER:
                    en->enemies[i].rt_index = 81;
                    break;
                case LOBBY_EVENT_HALLOWEEN:
                    en->enemies[i].rt_index = 80;
                    break;
                default:
                    en->enemies[i].rt_index = 51;
                    break;
            }
        }
    }

    /* Done! */
    l->map_enemies = en;
    l->map_objs = ob;
    return 0;
}

void free_game_enemies(lobby_t *l) {
    if(l->map_enemies) {
        free_safe(l->map_enemies->enemies);
        free_safe(l->map_enemies);
    }

    if(l->map_objs) {
        free_safe(l->map_objs->objs);
        free_safe(l->map_objs);
    }

    l->map_enemies = NULL;
    l->map_objs = NULL;
    l->bb_params = NULL;
}

int map_have_v2_maps(void) {
    return have_v2_maps;
}

int map_have_gc_maps(void) {
    return have_gc_maps;
}

int map_have_bb_maps(void) {
    return have_bb_maps;
}

static void parse_quest_objects(const uint8_t *data, uint32_t len,
                                uint32_t *obj_cnt,
                                const quest_dat_hdr_t *ptrs[4][17]) {
    const quest_dat_hdr_t *hdr = (const quest_dat_hdr_t *)data;
    uint32_t ptr = 0;
    uint32_t obj_count = 0;
    uint32_t enemy_count = 0;

    while(ptr < len) {

        //DBG_LOG("obj_type %d next_hdr %d", hdr->obj_type, hdr->next_hdr);

        switch(LE32(hdr->obj_type)) {
            case 0x01:                      /* 实例 */
                ptrs[0][LE32(hdr->area)] = hdr;
                obj_count += LE32(hdr->size) / sizeof(map_object_t);
                ptr += hdr->next_hdr;
                hdr = (const quest_dat_hdr_t *)(data + ptr);
                break;

            case 0x02:                      /* 敌人 */
                ptrs[1][LE32(hdr->area)] = hdr;
                enemy_count += LE32(hdr->size) / sizeof(map_enemy_t);
                ptr += hdr->next_hdr;
                hdr = (const quest_dat_hdr_t *)(data + ptr);
                break;

            case 0x03:                      /* TODO 此处需要修复 ??? - Skip */
                ptrs[2][LE32(hdr->area)] = hdr;
                ptr += hdr->next_hdr;
                hdr = (const quest_dat_hdr_t *)(data + ptr);
                break;

            case 0x04:                      /* TODO 此处需要修复 ??? - Skip */
                ptrs[3][LE32(hdr->area)] = hdr;
                ptr += hdr->next_hdr;
                hdr = (const quest_dat_hdr_t*)(data + ptr);
                break;

            default:
                /* 在文件末尾填充数据... */
                ptr = len;
                break;
        }
    }

    DBG_LOG("敌人 %d", enemy_count);

    *obj_cnt = obj_count;
}

int cache_quest_enemies(const char *ofn, const uint8_t *dat, uint32_t sz,
                        int episode) {
    int i, alt;
    uint32_t index, area, objects, j;
    const quest_dat_hdr_t *ptrs[4][17] = { { 0 } };
    game_enemies_t tmp_en;
    FILE *fp;
    const quest_dat_hdr_t *hdr;
    off_t offs = { 0 };

    /* 打开缓存文件... */
    if(!(fp = fopen(ofn, "wb"))) {
        ERR_LOG("无法打开缓存文件 \"%s\" 并写入: %s", ofn,
              strerror(errno));
        return -1;
    }

    /* 弄清楚该任务中敌人的总数... */
    parse_quest_objects(dat, sz, &objects, ptrs);
    //CONFIG_LOG("缓存文件: %s", ofn);
    //CONFIG_LOG("对象数量: %" PRIu32 "", objects);

    /* 按照稍后加载时需要的确切形式写出这些对象. */
    objects = LE32(objects);
    if(fwrite(&objects, 1, 4, fp) != 4) {
        ERR_LOG("无法写入缓存文件 \"%s\": %s", ofn,
              strerror(errno));
        fclose(fp);
        return -2;
    }

    /* 遍历每个区域并按顺序将它们编写出来. */
    for(i = 0; i < 17; ++i) {
        if((hdr = ptrs[0][i])) {
            sz = LE32(hdr->size);
            objects = sz / sizeof(map_object_t);

            for(j = 0; j < objects; ++j) {
                /* Write the object itself. */
                if(fwrite(hdr->data + j * sizeof(map_object_t), 1,
                          sizeof(map_object_t), fp) != sizeof(map_object_t)) {
                    ERR_LOG("无法写入缓存文件 \"%s\": %s",
                          ofn, strerror(errno));
                    fclose(fp);
                    return -3;
                }
            }
        }
    }

    /* 保存我们的位置, 因为我们事先不知道我们将有多少解析的敌人... */
    offs = (off_t)ftell(fp);
    fseek(fp, 4, SEEK_CUR);
    //CONFIG_LOG("敌人数据位置: %" PRIx64 "", (uint64_t)offs);
    index = 0;

    /* 复制敌人数据. */
    for(i = 0; i < 17; ++i) {
        if((hdr = ptrs[1][i])) {
            /* XXXX: Ugly! */
            sz = LE32(hdr->size);
            area = LE32(hdr->area);
            alt = 0;

            if((episode == 3 && area > 5) || (episode == 2 && area > 15))
                alt = 1;

            if(parse_map((map_enemy_t *)(hdr->data), sz / sizeof(map_enemy_t),
                         &tmp_en, episode, alt, area)) {
                ERR_LOG("Canot parse map for cache!");
                fclose(fp);
                return -4;
            }

            sz = tmp_en.count * sizeof(game_enemy_t);
            if(fwrite(tmp_en.enemies, 1, sz, fp) != sz) {
                ERR_LOG("无法写入缓存文件 \"%s\": %s", ofn,
                      strerror(errno));
                free_safe(tmp_en.enemies);
                fclose(fp);
                return -5;
            }

            index += tmp_en.count;
            free_safe(tmp_en.enemies);
        }
    }

    /* 返回并写下我们拥有的敌人数量. */
    fseek(fp, offs, SEEK_SET);
    //CONFIG_LOG("敌人数量: %" PRIu32 "", index);
    index = LE32(index);

    if(fwrite(&index, 1, 4, fp) != 4) {
        ERR_LOG("无法写入缓存文件 \"%s\": %s", ofn,
              strerror(errno));
        fclose(fp);
        return -6;
    }

    /* 现在我们已经完成了缓存文件的操作, 请进行清理并返回. */
    fclose(fp);
    return 0;
}

int load_quest_enemies(lobby_t *l, uint32_t qid, int ver) {
    FILE *fp;
    size_t dlen = strlen(ship->cfg->quests_dir);
    char* fn/*[dlen + 40]*/;
    //char fn[65535];
    void *tmp;
    uint32_t cnt, i;
    psocn_quest_t *q;
    quest_map_elem_t *el;
    uint32_t flags = l->flags;
    game_enemies_t *newen;
    game_objs_t *newob;
    ssize_t amt;

    /* Cowardly refuse to do this on challenge or battle mode. */
    if (l->challenge || l->battle) {

        //DBG_LOG("不处理挑战模式对战模式的怪物");
        return 0;
    }

    fn = (char*)malloc(sizeof(dlen) + 40);

    /* Unset this, in case something screws up. */
    l->flags &= ~LOBBY_FLAG_SERVER_DROPS;

    /* Map PC->DCv2. */
    if(ver == CLIENT_VERSION_PC)
        ver = CLIENT_VERSION_DCV2;

    /* Figure out where we're looking... */
    sprintf(fn, "%s/.mapcache/%s/%08x", ship->cfg->quests_dir,
        client_type[ver]->ver_name, qid);

    if(!(fp = fopen(fn, "rb"))) {
        ERR_LOG("无法打开文件 \"%s\": %s", fn, strerror(errno));
        free_safe(fn);
        return -1;
    }

    /* Start by reading in the objects array. */
    if(fread(&cnt, 1, 4, fp) != 4) {
        ERR_LOG("无法读取文件 \"%s\": %s", fn, strerror(errno));
        fclose(fp);
        free_safe(fn);
        return -2;
    }

    /* Allocate the storage for the new enemy/object arrays. */
    if(!(newen = (game_enemies_t *)malloc(sizeof(game_enemies_t)))) {
        ERR_LOG("无法为任务分配敌人内存: %s",
              strerror(errno));
        fclose(fp);
        free_safe(fn);
        return -10;
    }

    if(!(newob = (game_objs_t *)malloc(sizeof(game_objs_t)))) {
        ERR_LOG("无法为任务分配实例内存: %s",
              strerror(errno));
        free_safe(newen);
        fclose(fp);
        free_safe(fn);
        return -11;
    }

    memset(newen, 0, sizeof(game_enemies_t));
    memset(newob, 0, sizeof(game_objs_t));

    /* Allocate the objects array. */
    newob->count = cnt = LE32(cnt);
    if(!(tmp = malloc(cnt * sizeof(game_object_t)))) {
        ERR_LOG("无法为任务分配实例数组内存: %s",
              strerror(errno));
        ERR_LOG("任务 ID: %" PRIu32 " 版本: %d", qid, ver);
        ERR_LOG("对象数量: %" PRIu32 "", cnt);
        free_safe(newob);
        free_safe(newen);
        fclose(fp);
        free_safe(fn);
        return -3;
    }

    newob->objs = (game_object_t *)tmp;

    /* Read the objects in from the cache file. */
    for(i = 0; i < cnt; ++i) {
        if((amt = fread(&newob->objs[i].data, 1, sizeof(map_object_t),
                        fp)) != sizeof(map_object_t)) {
            if(amt < 0) {
                ERR_LOG("无法读取 objects 索引缓存 object id "
                      "%" PRIu32 ": %s", i, strerror(errno));
            }
            else {
                ERR_LOG("无法读取 objects 索引缓存 object id "
                      "%" PRIu32 ": 缺少 %zu, 并非 %zu", i,
                      sizeof(map_object_t), amt);
            }

            CONFIG_LOG("任务 ID: %" PRIu32 " 版本: %d", qid, ver);
            CONFIG_LOG("对象数量: %" PRIu32 "", cnt);
            free_safe(newob->objs);
            free_safe(newob);
            free_safe(newen);
            fclose(fp);
            free_safe(fn);
            return -4;
        }

        newob->objs[i].flags = 0;
    }

    if(fread(&cnt, 1, 4, fp) != 4) {
        ERR_LOG("无法读取 file \"%s\": %s", fn, strerror(errno));
        free_safe(newob->objs);
        free_safe(newob);
        free_safe(newen);
        fclose(fp);
        free_safe(fn);
        return -5;
    }

    /* Allocate the enemies array. */
    newen->count = cnt = LE32(cnt);
    if(!(tmp = malloc(cnt * sizeof(game_enemy_t)))) {
        ERR_LOG("无法为任务分配敌人数组内存: %s",
              strerror(errno));
        ERR_LOG("任务 ID: %" PRIu32 " 版本: %d", qid, ver);
        ERR_LOG("敌人数量: %" PRIu32 "", cnt);
        free_safe(newob->objs);
        free_safe(newob);
        free_safe(newen);
        fclose(fp);
        free_safe(fn);
        return -6;
    }

    newen->enemies = (game_enemy_t *)tmp;

    /* Read the enemies in from the cache file. */
    if(fread(newen->enemies, sizeof(game_enemy_t), cnt, fp) != cnt) {
        ERR_LOG("无法读取地图缓存: %s", strerror(errno));
        ERR_LOG("任务 ID: %" PRIu32 " 版本: %d", qid, ver);
        ERR_LOG("对象数量: %" PRIu32 "", newob->count);
        ERR_LOG("敌人数量: %" PRIu32 "", newen->count);
        free_safe(newen->enemies);
        free_safe(newob->objs);
        free_safe(newob);
        free_safe(newen);
        fclose(fp);
        free_safe(fn);
        return -7;
    }

    /* We're done with the file now, so close it. */
    fclose(fp);
    free_safe(fn);

    /* It should be safe to swap things out now, so do it. */
    free_safe(l->map_enemies->enemies);
    free_safe(l->map_objs->objs);
    free_safe(l->map_enemies);
    free_safe(l->map_objs);

    l->map_enemies = newen;
    l->map_objs = newob;

    /* Fixup Dark Falz' data for difficulties other than normal and the special
       Rappy data too... */
    for(i = 0; i < cnt; ++i) {
        if(l->map_enemies->enemies[i].bp_entry == 0x37 && l->difficulty) {
            l->map_enemies->enemies[i].bp_entry = 0x38;
        }
        else if(l->map_enemies->enemies[i].rt_index == (uint8_t)-1) {
            switch(l->event) {
                case LOBBY_EVENT_CHRISTMAS:
                    l->map_enemies->enemies[i].rt_index = 79;
                    break;
                case LOBBY_EVENT_EASTER:
                    l->map_enemies->enemies[i].rt_index = 81;
                    break;
                case LOBBY_EVENT_HALLOWEEN:
                    l->map_enemies->enemies[i].rt_index = 80;
                    break;
                default:
                    l->map_enemies->enemies[i].rt_index = 51;
                    break;
            }
        }
    }

    /* Find the quest since we need to check the enemies later for drops... */
    if(!(el = quest_lookup(&ship->qmap, qid))) {
        ERR_LOG("无法查找到任务?!");
        goto done;
    }

    /* Try to find a monster list associated with the quest. Basically, we look
       through each language of the quest we're loading for one that has the
       monster list set. Thus, you don't have to provide one for each and every
       language -- only one per version (other than PC) will do. */
    for(i = 0; i < CLIENT_LANG_ALL; ++i) {
        if(!(q = el->qptr[ver][i]))
            continue;

        if(q->num_monster_ids || q->num_monster_types)
            break;
    }

    /* If we never found a monster list, then we don't care about it at all. */
    if(!q || (!q->num_monster_ids && !q->num_monster_types))
        goto done;

    /* Make a copy of the monster data from the quest. */
    l->num_mtypes = q->num_monster_types;
    if(!(l->mtypes = (qenemy_t *)malloc(sizeof(qenemy_t) * l->num_mtypes))) {
        ERR_LOG("无法分配敌人类型内存: %s", strerror(errno));
        l->num_mtypes = 0;
        goto done;
    }

    l->num_mids = q->num_monster_ids;
    if(!(l->mids = (qenemy_t *)malloc(sizeof(qenemy_t) * l->num_mids))) {
        ERR_LOG("无法分配敌人索引内存: %s", strerror(errno));
        free_safe(l->mtypes);
        l->mtypes = NULL;
        l->num_mtypes = 0;
        l->num_mids = 0;
        goto done;
    }

    memcpy(l->mtypes, q->monster_types, sizeof(qenemy_t) * l->num_mtypes);
    memcpy(l->mids, q->monster_ids, sizeof(qenemy_t) * l->num_mids);

done:
    /* Re-set the server drops flag if it was set and clean up. */
    l->flags = flags;

    return 0;
}

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

struct DATSectionHeader {
    uint32_t type; // 1 = objects, 2 = enemies. There are other types too
    uint32_t section_size; // Includes this header
    uint32_t area;
    uint32_t data_size;
} PACKED;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

//void add_enemies_from_quest_data(
//    int episode,
//    uint8_t difficulty,
//    uint8_t event,
//    const void* data,
//    size_t size) {
//
//    StringReader r;
//    r.data = data;
//    r.size = size;
//    r.offset = 0;
//
//    while (r.offset < r.size) {
//
//        struct DATSectionHeader header;
//        memcpy(&header, (char*)r.data + r.offset, sizeof(header));
//        r.offset += sizeof(header);
//
//        if (header.type == 0 && header.section_size == 0) {
//            break;
//        }
//        if (header.section_size < sizeof(header)) {
//            printf("quest layout has invalid section header at offset 0x%zX\n", r.offset - sizeof(header));
//            break;
//        }
//        if (header.type == 2) {
//            if (header.data_size % sizeof(map_enemy_t)) {
//                printf("quest layout enemy section size is not a multiple of enemy entry size\n");
//                break;
//            }
//            add_enemies_from_map_data(episode, difficulty, event, (char*)r.data + r.offset, header.data_size);
//            r.offset += header.section_size - sizeof(header);
//        }
//        else {
//            r.offset += header.section_size - sizeof(header);
//        }
//    }
//}

const AreaMapFileIndex(*map_file_info_for_episode(uint32_t ep))[2][17]{
    switch (ep) {
        case GAME_TYPE_EPISODE_1:
            return &map_file_info[0];
        case GAME_TYPE_EPISODE_2:
            return &map_file_info[1];
        case GAME_TYPE_EPISODE_4:
            return &map_file_info[2];
    }

    ERR_LOG("无效章节");
    // 处理无效参数的其他错误处理措施
    return NULL;
}

void generate_variations(
    uint32_t variations[0x20],
    uint32_t episode,
    bool is_solo) {
    const AreaMapFileIndex(*ep_index)[2][17] = map_file_info_for_episode(episode);
    for (size_t z = 0; z < 0x10; z++) {
        const AreaMapFileIndex* a = NULL;
        if (is_solo) {
            a = &(*ep_index)[1][z];
        }
        if (!a || !a->name_token) {
            a = &(*ep_index)[0][z];
        }
        if (!a->name_token) {
            variations[z * 2 + 0] = 0;
            variations[z * 2 + 1] = 0;
        }
        else {
            variations[z * 2 + 0] = (ARRAY_SIZE(a->variation1_values) < 2) ? 0 : (rand() % ARRAY_SIZE(a->variation1_values));
            variations[z * 2 + 1] = (ARRAY_SIZE(a->variation2_values) < 2) ? 0 : (rand() % ARRAY_SIZE(a->variation2_values));
        }
    }
}

char* map_filenames_for_variation(
    uint32_t episode, int is_solo, uint8_t area, uint32_t var1, uint32_t var2) {
    const AreaMapFileIndex(*ep_index)[2][17] = map_file_info_for_episode(episode);
    const AreaMapFileIndex* a = NULL;
    char* ret = NULL;

    if (is_solo) {
        a = &((*ep_index)[1][area]);
    }
    if (!a || !a->name_token) {
        a = &((*ep_index)[0][area]);
    }
    if (!a->name_token) {
        return NULL;
    }

    char filename[256];
    sprintf(filename, "map_%s", a->name_token);

    if (a->variation1_values) {
        sprintf(filename + strlen(filename), "_%02X", a->variation1_values[var1]);
    }
    if (a->variation2_values) {
        sprintf(filename + strlen(filename), "_%02X", a->variation2_values[var2]);
    }

    if (is_solo) {
        char suffix1[10];
        char suffix2[10];
        sprintf(suffix1, "_offe.dat");
        sprintf(suffix2, "e_s.dat");

        ret = (char*)malloc(strlen(filename) + strlen(suffix1) + strlen(suffix2) + 1);

        if (ret)
            sprintf(ret, "%s%s,%s%s", filename, suffix1, filename, suffix2);
    }
    else {
        char suffix[10];
        sprintf(suffix, "e.dat");

        ret = (char*)malloc(strlen(filename) + strlen(suffix) + 1);
        if (ret)
            sprintf(ret, "%s%s", filename, suffix);
    }

    return ret;
}
