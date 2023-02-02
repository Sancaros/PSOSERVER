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

#ifndef MAPDATA_H
#define MAPDATA_H

#include <stdint.h>

#include <psoconfig.h>
#include <pso_cmd_code.h>
#include <pso_character.h>

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* Battle parameter entry (enemy type, basically) used by the server for Blue
   Burst. This is basically the same structure as newserv's BATTLE_PARAM or
   Tethealla's BATTLEPARAM, which makes sense considering that all of us are
   using basically the same data files (directly from the game itself). */
typedef struct bb_battle_param {
    uint16_t atp;           // 攻击强度
    //uint16_t psv;           // 掉落美塞塔
    uint16_t mst;           // 掉落美塞塔
    uint16_t evp;           // 闪避
    uint16_t hp;            // 血量
    uint16_t dfp;           // 防御
    uint16_t ata;           // 精准
    uint16_t lck;           // 幸运
    uint16_t esp;           // 逃跑几率 ？？？？？
    uint32_t dfp_range;     // 防御距离
    uint32_t evp_range;     // 闪避距离
    uint32_t hp2;           // 怪物的护盾值
    //uint8_t unk[12];
    uint32_t exp;           // 经验值
    uint32_t diff;          // 怪物的魔法值
} PACKED bb_battle_param_t;

/* Enemy data in the map files. This the same as the ENEMY_ENTRY struct from
   newserv. */
typedef struct map_enemy {
    uint32_t base;// 4 怪物的种类ID
    uint16_t reserved0;
    uint16_t num_clones;
    uint32_t reserved[11];
	float rareratio; // 4 怪物稀有率
    //uint32_t reserved12;
    uint32_t reserved13;
    uint32_t exp;// 4 该怪物可获取的经验值
    uint32_t skin;// 4 检测相同类型怪物是否有皮肤 0 1
    uint32_t rt_index;// 4 随机掉落索引地址
} PACKED map_enemy_t;

/* Object data in the map object files. */
typedef struct map_object {
    uint32_t skin;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t obj_id;
    float x;
    float y;
    float z;
    uint32_t rpl;
    uint32_t rotation;
    uint32_t unk3;
    uint32_t unk4;
    /* Everything beyond this point depends on the object type. */
    union {
        float sp[6];
        uint32_t dword[6];
    };
} PACKED map_object_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

struct PSOEnemy {
    uint64_t id;
    uint16_t source_type;
    uint8_t hit_flags;
    uint8_t last_hit;
    uint32_t experience;
    uint32_t rt_index;
};

/* Enemy data as used in the game. */
typedef struct game_enemy {
    uint32_t bp_entry;
    uint8_t rt_index;
    uint8_t clients_hit;
    uint8_t last_client;
    uint8_t drop_done;
    uint8_t area;
} game_enemy_t;

typedef struct game_enemies {
    uint32_t count;
    game_enemy_t* enemies;
} game_enemies_t;

typedef struct parsed_map {
    uint32_t map_count;
    uint32_t variation_count;
    game_enemies_t* data;
} parsed_map_t;

/* Object data as used in the game. */
typedef struct game_object {
    map_object_t data;
    uint32_t flags;
    uint8_t area;
} game_object_t;

typedef struct game_objects {
    uint32_t count;
    game_object_t* objs;
} game_objs_t;

typedef struct parsed_objects {
    uint32_t map_count;
    uint32_t variation_count;
    game_objs_t* data;
} parsed_objs_t;

#undef PACKED

/* Object types */
#define OBJ_SKIN_REG_BOX            0x0088
#define OBJ_SKIN_FIXED_BOX          0x0092
#define OBJ_SKIN_RUINS_REG_BOX      0x0161
#define OBJ_SKIN_RUINS_FIXED_BOX    0x0162
#define OBJ_SKIN_CCA_REG_BOX        0x0200
#define OBJ_SKIN_CCA_FIXED_BOX      0x0203

#ifndef LOBBY_DEFINED
#define LOBBY_DEFINED
struct lobby;
typedef struct lobby lobby_t;
#endif

/* Player levelup data */
extern bb_level_table_t bb_char_stats;
extern v2_level_table_t v2_char_stats;

int bb_read_params(psocn_ship_t* cfg);
void bb_free_params(void);

int v2_read_params(psocn_ship_t* cfg);
void v2_free_params(void);

int gc_read_params(psocn_ship_t* cfg);
void gc_free_params(void);

int bb_load_game_enemies(lobby_t* l);
int v2_load_game_enemies(lobby_t* l);
int gc_load_game_enemies(lobby_t* l);
void free_game_enemies(lobby_t* l);

int map_have_v2_maps(void);
int map_have_gc_maps(void);
int map_have_bb_maps(void);

int load_quest_enemies(lobby_t* l, uint32_t qid, int ver);
int cache_quest_enemies(const char* ofn, const uint8_t* dat, uint32_t sz,
    int episode);

#endif /* !MAPDATA_H */
