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
#include <pso_opcodes_block.h>
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
    uint16_t psv;           // 毅力 (智商?)
    uint16_t evp;           // 闪避
    uint16_t hp;            // 血量
    uint16_t dfp;           // 防御
    uint16_t ata;           // 精准
    uint16_t lck;           // 幸运
    uint16_t esp;           // 逃跑几率 ？？？？？
    uint32_t dfp_range;     // 防御距离
    uint32_t evp_range;     // 闪避距离
    uint32_t shield;        // 怪物的护盾值
    uint32_t exp;           // 经验值
    uint32_t difficulty;    // 难度
} PACKED bb_battle_param_t;

typedef struct {
    bb_battle_param_t difficulty[4][0x60];
} bb_battle_param_table;

/* 地图文件中的敌人数据与newserv中的ENEMY_ENTRY结构相同. 72字节 */
typedef struct map_enemy {
    /* 00 */ uint16_t base_type;
    /* 02 */ uint16_t unknown_a0; // Overwritten by client at load time
    /* 04 */ uint16_t enemy_index; // Overwritten by client at load time
    /* 06 */ uint16_t num_children;
    /* 08 */ uint16_t area;
    /* 0A */ uint16_t entity_id; // == enemy_index + 0x1000
    /* 0C */ uint16_t section;
    /* 0E */ uint16_t wave_number;
    /* 10 */ uint32_t wave_number2;
    /* 14 */ float x;
    /* 18 */ float y;
    /* 1C */ float z;
    /* 20 */ uint32_t x_angle;
    /* 24 */ uint32_t y_angle;
    /* 28 */ uint32_t z_angle;
    /* 2C */ uint32_t rare_rate;
    /* 30 */ float rareratio; // 4 怪物稀有率
    /* 34 */ float reserved12;
    /* 38 */ uint32_t reserved13;
    /* 3C */ uint32_t exp;// 4 该怪物可获取的经验值
    /* 40 */ uint32_t skin;// 4 检测相同类型怪物是否有皮肤 0 1
    /* 44 */ uint32_t rt_index;// 4 稀有掉落索引地址
    /* 48 */
} PACKED map_enemy_t;

typedef struct EnemyEntry {
    /* 00 */ uint16_t base_type;
    /* 02 */ uint16_t unknown_a0; // Overwritten by client at load time
    /* 04 */ uint16_t enemy_index; // Overwritten by client at load time
    /* 06 */ uint16_t num_children;

    /* 08 */ uint16_t area;
    /* 0A */ uint16_t entity_id; // == enemy_index + 0x1000
    /* 0C */ uint16_t section;
    /* 0E */ uint16_t wave_number;
    /* 10 */ uint32_t wave_number2;
    /* 14 */ float x;
    /* 18 */ float y;
    /* 1C */ float z;
    /* 20 */ uint32_t x_angle;
    /* 24 */ uint32_t y_angle;
    /* 28 */ uint32_t z_angle;
    /* 2C */ uint32_t unknown_a3;

    /* 30 */ uint32_t unknown_a4;   //rareratio
    /* 34 */ uint32_t unknown_a5;  //reserved12
    /* 38 */ uint32_t unknown_a6; //reserved13
    /* 3C */ uint32_t exp;
    /* 40 */ uint32_t skin;
    /* 44 */ uint32_t rt_index;
    /* 48 */

    //string str() const {
    //    return string_printf("EnemyEntry(base_type=%hX, a0=%hX, enemy_index=%hX, num_children=%hX, area=%hX, entity_id=%hX, section=%hX, wave_number=%hX, wave_number2=%" PRIX32 ", x=%g, y=%g, z=%g, x_angle=%" PRIX32 ", y_angle=%" PRIX32 ", z_angle=%" PRIX32 ", a3=%" PRIX32 ", a4=%" PRIX32 ", a5=%" PRIX32 ", a6=%" PRIX32 ", a7=%" PRIX32 ", skin=%" PRIX32 ", a8=%" PRIX32 ")",
    //        this->base_type.load(), this->unknown_a0.load(), this->enemy_index.load(), this->num_children.load(), this->area.load(),
    //        this->entity_id.load(), this->section.load(), this->wave_number.load(),
    //        this->wave_number2.load(), this->x.load(), this->y.load(), this->z.load(), this->x_angle.load(),
    //        this->y_angle.load(), this->z_angle.load(), this->unknown_a3.load(), this->unknown_a4.load(),
    //        this->unknown_a5.load(), this->unknown_a6.load(), this->unknown_a7.load(), this->skin.load(),
    //        this->unknown_a8.load());
    //}
} PACKED EnemyEntry_t;

static int dsasadsada = sizeof(map_enemy_t);

/* 地图对象文件中的物体数据. 68字节 */
typedef struct map_object {
    uint32_t base_type;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t obj_id;
    float x;
    float y;
    float z;
    uint32_t x_angle;
    uint32_t y_angle;
    uint32_t z_angle;
    uint32_t unk4;
    /* Everything beyond this point depends on the object type. */
    union {
        float sp[6];
        uint32_t dword[6];
    };
} PACKED map_object_t;

typedef struct ObjectEntry {
    /* 00 */ uint16_t base_type;
    /* 02 */ uint16_t unknown_a1;
    /* 04 */ uint32_t unknown_a2;
    /* 08 */ uint16_t id;
    /* 0A */ uint16_t group;
    /* 0C */ uint16_t section;
    /* 0E */ uint16_t unknown_a3;
    /* 10 */ float x;
    /* 14 */ float y;
    /* 18 */ float z;
    /* 1C */ uint32_t x_angle;
    /* 20 */ uint32_t y_angle;
    /* 24 */ uint32_t z_angle;
    /* 28 */ uint32_t unknown_a4;
    /* 2C */ uint32_t unknown_a5;
    /* 30 */ uint32_t unknown_a6;
    /* 34 */ uint32_t unknown_a7;
    /* 38 */ uint32_t unknown_a8;
    /* 3C */ uint32_t unknown_a9;
    /* 40 */ uint32_t unknown_a10;
    /* 44 */

    //string str() const {
    //    return string_printf("ObjectEntry(base_type=%hX, a1=%hX, a2=%" PRIX32 ", id=%hX, group=%hX, section=%hX, a3=%hX, x=%g, y=%g, z=%g, x_angle=%" PRIX32 ", y_angle=%" PRIX32 ", z_angle=%" PRIX32 ", a3=%" PRIX32 ", a4=%" PRIX32 ", a5=%" PRIX32 ", a6=%" PRIX32 ", a7=%" PRIX32 ", a8=%" PRIX32 ", a9=%" PRIX32 ")",
    //        this->base_type.load(), this->unknown_a1.load(), this->unknown_a2.load(), this->id.load(), this->group.load(),
    //        this->section.load(), this->unknown_a3.load(), this->x.load(), this->y.load(), this->z.load(), this->x_angle.load(),
    //        this->y_angle.load(), this->z_angle.load(), this->unknown_a3.load(), this->unknown_a4.load(),
    //        this->unknown_a5.load(), this->unknown_a6.load(), this->unknown_a7.load(), this->unknown_a8.load(),
    //        this->unknown_a9.load());
    //}
} PACKED ObjectEntry_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

typedef struct PSOEnemy {
    uint64_t id;
    uint16_t source_type;
    uint8_t hit_flags;
    uint8_t last_hit;
    uint32_t experience;
    uint32_t rt_index;
    const char* type_name;
} PSOEnemy_t;

/* Enemy data as used in the game. */
typedef struct game_enemy {
    uint32_t bp_entry;
    uint8_t pt_index;
    uint8_t clients_hit;
    uint8_t last_client;
    uint8_t drop_done;
    uint8_t area;
} game_enemy_t;

/* Rare Enemy data as used in the game. */
typedef struct game_rare_enemy {
    uint32_t rare_enemy_count;
    uint16_t rare_enemy_data;
} game_rare_enemy_t;

typedef struct game_enemies {
    uint32_t enemy_count;
    game_enemy_t* enemies;
    uint32_t rare_enemy_count;
    uint16_t rare_enemy_data[0x10];
} game_enemies_t;

typedef struct parsed_map {
    uint32_t map_count;
    uint32_t variation_count;
    game_enemies_t* gen_data;
} parsed_map_t;

/* Object data as used in the game. */
typedef struct game_object {
    map_object_t mobj_data;
    uint32_t flags;
    uint8_t area;
} game_object_t;

typedef struct game_objects {
    uint32_t obj_count;
    game_object_t* objs;
} game_objs_t;

typedef struct parsed_objects {
    uint32_t map_count;
    uint32_t variation_count;
    game_objs_t* gobj_data;
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

static const char map_suffix[3][12] = { "_offe.dat", "e_s.dat", "e.dat" };
static const char obj_suffix[3][12] = { "_offo.dat", "_offo.dat", "o.dat" };

static const uint32_t dcnte_maps[0x20] =
{ 1,1,1,2,1,2,2,2,2,2,2,2,1,2,1,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1 };

static const int max_area[3] = { 0x0E, 0x0F, 0x09 };

typedef struct AreaMapFileIndex {
    const char* name_token;
    size_t map_nums;
    int map_num[3];
    size_t map_vars;
    int map_var[5];
} AreaMapFileIndex_t;

// 地图索引表 [episode][is_solo][area], 章节读取值 0-2 重写自newserv
static const AreaMapFileIndex_t map_file_info[3][2][16] = {
    {
        // 章节 I
        {
            // 多人
            {"city00", 1, {-1}, 1, {0}},
            {"forest01", 1, {-1}, 5, {0, 1, 2, 3, 4}},
            {"forest02", 1, {-1}, 5, {0, 1, 2, 3, 4}},
            {"cave01", 3, {0, 1, 2}, 2, {0, 1}},
            {"cave02", 3, {0, 1, 2}, 2, {0, 1}},
            {"cave03", 3, {0, 1, 2}, 2, {0, 1}},
            {"machine01", 3, {0, 1, 2}, 2, {0, 1}},
            {"machine02", 3, {0, 1, 2}, 2, {0, 1}},
            {"ancient01", 3, {0, 1, 2}, 2, {0, 1}},
            {"ancient02", 3, {0, 1, 2}, 2, {0, 1}},
            {"ancient03", 3, {0, 1, 2}, 2, {0, 1}},
            {"boss01", 1, {-1}, 1, {-1}},
            {"boss02", 1, {-1}, 1, {-1}},
            {"boss03", 1, {-1}, 1, {-1}},
            {"boss04", 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
        },
        {
            // 单人
            {"city00", 1, {-1}, 1, {0}},
            {"forest01", 1, {-1}, 3, {0, 2, 4}},
            {"forest02", 1, {-1}, 3, {0, 3, 4}},
            {"cave01", 3, {0, 1, 2}, 1, {0}},
            {"cave02", 3, {0, 1, 2}, 1, {0}},
            {"cave03", 3, {0, 1, 2}, 1, {0}},
            {"machine01", 3, {0, 1, 2}, 2, {0, 1}},
            {"machine02", 3, {0, 1, 2}, 2, {0, 1}},
            {"ancient01", 3, {0, 1, 2}, 2, {0, 1}},
            {"ancient02", 3, {0, 1, 2}, 2, {0, 1}},
            {"ancient03", 3, {0, 1, 2}, 2, {0, 1}},
            {"boss01", 1, {-1}, 1, {-1}},
            {"boss02", 1, {-1}, 1, {-1}},
            {"boss03", 1, {-1}, 1, {-1}},
            {"boss04", 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
        },
    },
    {
        // 章节 II
        {
            // 多人
            {"labo00", 1, {-1}, 1, {0}},
            {"ruins01", 2, {0, 1}, 1, {0}},
            {"ruins02", 2, {0, 1}, 1, {0}},
            {"space01", 2, {0, 1}, 1, {0}},
            {"space02", 2, {0, 1}, 1, {0}},
            {"jungle01", 1, {-1}, 3, {0, 1, 2}},
            {"jungle02", 1, {-1}, 3, {0, 1, 2}},
            {"jungle03", 1, {-1}, 3, {0, 1, 2}},
            {"jungle04", 2, {0, 1}, 2, {0, 1}},
            {"jungle05", 1, {-1}, 3, {0, 1, 2}},
            {"seabed01", 2, {0, 1}, 2, {0, 1}},
            {"seabed02", 2, {0, 1}, 2, {0, 1}},
            {"boss05", 1, {-1}, 1, {-1}},
            {"boss06", 1, {-1}, 1, {-1}},
            {"boss07", 1, {-1}, 1, {-1}},
            {"boss08", 1, {-1}, 1, {-1}},
        },
        {
            // 单人
            {"labo00", 1, {-1}, 1, {0}},
            {"ruins01", 2, {0, 1}, 1, {0}},
            {"ruins02", 2, {0, 1}, 1, {0}},
            {"space01", 2, {0, 1}, 1, {0}},
            {"space02", 2, {0, 1}, 1, {0}},
            {"jungle01", 1, {-1}, 3, {0, 1, 2}},
            {"jungle02", 1, {-1}, 3, {0, 1, 2}},
            {"jungle03", 1, {-1}, 3, {0, 1, 2}},
            {"jungle04", 2, {0, 1}, 2, {0, 1}},
            {"jungle05", 1, {-1}, 3, {0, 1, 2}},
            {"seabed01", 2, {0, 1}, 2, {0, 1}},
            {"seabed02", 2, {0, 1}, 2, {0, 1}},
            {"boss05", 1, {-1}, 1, {-1}},
            {"boss06", 1, {-1}, 1, {-1}},
            {"boss07", 1, {-1}, 1, {-1}},
            {"boss08", 1, {-1}, 1, {-1}},
        },
    },
    {
        // 章节 IV
        {
            // 多人
            {"city02", 1, {0}, 1, {0}},
            {"wilds01", 1, {0}, 3, {0, 1, 2}},
            {"wilds01", 1, {1}, 3, {0, 1, 2}},
            {"wilds01", 1, {2}, 3, {0, 1, 2}},
            {"wilds01", 1, {3}, 3, {0, 1, 2}},
            {"crater01", 1, {0}, 3, {0, 1, 2}},
            {"desert01", 3, {0, 1, 2}, 1, {0}},
            {"desert02", 1, {0}, 3, {0, 1, 2}},
            {"desert03", 3, {0, 1, 2}, 1, {0}},
            {"boss09", 1, {0}, 1, {0}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
        },
        {
            // 单人
            {"city02", 1, {0}, 1, {0}},
            {"wilds01", 1, {0}, 3, {0, 1, 2}},
            {"wilds01", 1, {1}, 3, {0, 1, 2}},
            {"wilds01", 1, {2}, 3, {0, 1, 2}},
            {"wilds01", 1, {3}, 3, {0, 1, 2}},
            {"crater01", 1, {0}, 3, {0, 1, 2}},
            {"desert01", 3, {0, 1, 2}, 1, {0}},
            {"desert02", 1, {0}, 3, {0, 1, 2}},
            {"desert03", 3, {0, 1, 2}, 1, {0}},
            {"boss09", 1, {0}, 1, {0}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
            {NULL, 1, {-1}, 1, {-1}},
        },
    },
};

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
