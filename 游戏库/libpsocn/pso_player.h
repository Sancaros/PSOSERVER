/*
    梦幻之星中国 玩家数据结构
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

#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

#include "pso_character.h"

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* The header attached before the player data when sending to a lobby client. */
typedef struct dc_player_hdr {
    uint32_t player_tag;
    uint32_t guildcard;
    uint32_t ip_addr;
    uint32_t client_id;
    char name[16];
} PACKED dc_player_hdr_t;

typedef struct pc_player_hdr {
    uint32_t player_tag;
    uint32_t guildcard;
    uint32_t ip_addr;
    uint32_t client_id;
    uint16_t name[16];
} PACKED pc_player_hdr_t;

typedef struct xbox_ip {
    uint32_t lan_ip;
    uint32_t wan_ip;
    uint16_t port;
    uint8_t mac_addr[6];
    uint32_t sg_addr;
    uint32_t sg_session_id;
    uint64_t xbox_account_id;
    uint32_t unused;
} PACKED xbox_ip_t;

typedef struct xbox_player_hdr {
    uint32_t player_tag;
    uint32_t guildcard;
    xbox_ip_t xbox_ip;
    uint32_t d1;
    uint32_t d2;
    uint32_t d3;
    uint32_t client_id;
    char name[16];
} PACKED xbox_player_hdr_t;

typedef struct bb_player_hdr {
    uint32_t player_tag;
    uint32_t guildcard;
    uint32_t ip_address; // Guess - the official builds didn't use this, but all other versions have it
    uint8_t unk1[0x10];
    uint32_t client_id;
    uint16_t name[0x10];
    uint32_t unk2;
} PACKED bb_player_hdr_t;

/* Alias some stuff from its "pso_character.h" versions to what was in
   here before... */

/* 玩家数据结构 用于认证服务器 0x0061 指令*/
typedef struct v1_player {
    psocn_v1v2v3pc_char_t character;
} PACKED v1_player_t;

/* 玩家数据结构 用于认证服务器 0x0061 指令*/
typedef struct v2_player {
    psocn_v1v2v3pc_char_t character;
    psocn_dc_records_data_t records;
    choice_search_config_t cs_config;
} PACKED v2_player_t;

/* 玩家数据结构 用于认证服务器 0x0061 指令*/
typedef struct pc_player {
    psocn_v1v2v3pc_char_t character;
    psocn_pc_records_data_t records;
    choice_search_config_t cs_config;
    uint32_t blacklist[30];
    uint32_t autoreply_enabled;
    uint16_t autoreply[];               /* Always at least 4 bytes! */
} PACKED pc_player_t;

/* 玩家数据结构 用于认证服务器 0x0061 指令*/
typedef struct v3_player {
    psocn_v1v2v3pc_char_t character;
    psocn_v3_records_data_t records;
    choice_search_config_t cs_config;
    char infoboard[0xAC];
    uint32_t blacklist[30];
    uint32_t autoreply_enabled;
    char autoreply[];                   /* Always at least 4 bytes! */
} PACKED v3_player_t;

/* BB玩家数据结构 用于认证服务器 0x0061 指令*/
typedef struct bb_player {
    psocn_bb_char_t character;
    psocn_bb_records_data_t records;
    choice_search_config_t cs_config;
    uint16_t infoboard[0x00AC];
    uint32_t blacklist[0x001E];
    uint32_t autoreply_enabled;
    uint16_t autoreply[];
} PACKED bb_player_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

typedef union {
    v1_player_t v1;
    v2_player_t v2;
    pc_player_t pc;
    v3_player_t v3;
    bb_player_t bb;
} player_t;

/*在这个枚举类型中，它定义了以下8个可枚举的常量值:

MATERIAL_HP 的值为 - 2
MATERIAL_TP 的值为 - 1
MATERIAL_POWER 的值为 0
MATERIAL_MIND 的值为 1
MATERIAL_EVADE 的值为 2
MATERIAL_DEF 的值为 3
MATERIAL_LUCK 的值为 4

因此，表达式 8 + which 可以得到以下情况的值 :

如果 which 的值为 - 2，则结果为 6
如果 which 的值为 - 1，则结果为 7
如果 which 的值为 0，则结果为 8
如果 which 的值为 1，则结果为 9
如果 which 的值为 2，则结果为 10
如果 which 的值为 3，则结果为 11
如果 which 的值为 4，则结果为 12

所以，在每个情况下，表达式的值增加了8。*/
typedef enum {
    MATERIAL_HP = -2,
    MATERIAL_TP = -1,
    MATERIAL_POWER = 0,
    MATERIAL_MIND = 1,
    MATERIAL_EVADE = 2,
    MATERIAL_DEF = 3,
    MATERIAL_LUCK = 4
} MaterialType;

/* 玩家皮肤支持列表描述 */
static const char* npcskin_desc[] = {
    "玩家",
    "神秘人",//0
    "莉可",//1
    "索尼克",//2
    "纳克鲁斯",//3
    "塔尔斯",//4
};

bool char_class_is_male(uint8_t equip_flags);

bool char_class_is_female(uint8_t equip_flags);

bool char_class_is_human(uint8_t equip_flags);

bool char_class_is_newman(uint8_t equip_flags);

bool char_class_is_android(uint8_t equip_flags);

bool char_class_is_hunter(uint8_t equip_flags);

bool char_class_is_ranger(uint8_t equip_flags);

bool char_class_is_force(uint8_t equip_flags);

static char player_name[24];

int player_bb_name_cpy(psocn_bb_char_name_t* dst, psocn_bb_char_name_t* src);

char* get_player_name(player_t* pl, int version, bool raw);

void set_technique_level(techniques_t* technique_levels_v1, inventory_t* inv, uint8_t which, uint8_t level);

uint8_t get_technique_level(techniques_t* technique_levels_v1, inventory_t* inv, uint8_t which);

uint8_t get_material_usage(inventory_t* inv, MaterialType which);

void set_material_usage(inventory_t* inv, MaterialType which, uint8_t usage);

#endif /* !PLAYER_H */