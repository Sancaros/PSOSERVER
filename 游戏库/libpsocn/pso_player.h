/*
    梦幻之星中国 玩家数据结构
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

#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
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
    uint32_t tag;
    uint32_t guildcard;
    uint32_t ip_addr;
    uint32_t client_id;
    char name[16];
} PACKED dc_player_hdr_t;

typedef struct pc_player_hdr {
    uint32_t tag;
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
    uint32_t tag;
    uint32_t guildcard;
    xbox_ip_t xbox_ip;
    uint32_t d1;
    uint32_t d2;
    uint32_t d3;
    uint32_t client_id;
    char name[16];
} PACKED xbox_player_hdr_t;

typedef struct bb_player_hdr {
    uint32_t tag;
    uint32_t guildcard;
    uint32_t ip_address; // Guess - the official builds didn't use this, but all other versions have it
    uint8_t unk1[0x10];
    uint32_t client_id;
    uint16_t name[0x10];
    uint32_t unk2;
} PACKED bb_player_hdr_t;

/* Alias some stuff from its "pso_character.h" versions to what was in
   here before... */

/* Player data structures */
typedef struct v1_player {
    inventory_t inv;
    psocn_v1v2v3pc_char_t character;
} PACKED v1_player_t;

typedef struct v2_player {
    inventory_t inv;
    psocn_v1v2v3pc_char_t character;
    psocn_v2_c_rank_data_t chal_data;
    uint32_t unk4[6];
} PACKED v2_player_t;

typedef struct pc_player {
    inventory_t inv;
    psocn_v1v2v3pc_char_t character;
    psocn_pc_c_rank_data_t chal_data;
    uint32_t unk4[6];
    uint32_t blacklist[30];
    uint32_t autoreply_enabled;
    uint16_t autoreply[];               /* Always at least 4 bytes! */
} PACKED pc_player_t;

typedef struct v3_player {
    inventory_t inv;
    psocn_v1v2v3pc_char_t character;
    psocn_v3_c_rank_data_t chal_data;
    uint32_t unk4[6];
    char infoboard[0xAC];
    uint32_t blacklist[30];
    uint32_t autoreply_enabled;
    char autoreply[];                   /* Always at least 4 bytes! */
} PACKED v3_player_t;

/* BB玩家数据结构 用于认证服务器 0x0061 指令*/
typedef struct bb_player {
    inventory_t inv;
    psocn_bb_char_t character;
    psocn_bb_c_rank_data_t c_rank;
    uint32_t unk4[6];
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

#define PLAYER_T_DEFINED

#endif /* !PLAYER_H */

