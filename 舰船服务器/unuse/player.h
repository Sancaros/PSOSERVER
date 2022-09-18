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

#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <characters.h>

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

/* Alias some stuff from its <sylverant/characters.h> versions to what was in
   here before... */
//typedef iitem_t iitem_t;
typedef psocn_inventory_t inventory_t;

/* Player data structures */
typedef struct v1_player {
    inventory_t inv;
    psocn_stats_t stats;
    uint8_t opt_flag[10];
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    char name[16];
    uint32_t unk3[2];
    uint8_t name_color_b; // 0x390; 912  1 整数
    uint8_t name_color_g; // 0x391; 913 1 整数
    uint8_t name_color_r; // 0x392; 914 1 整数
    uint8_t name_color_transparency; // 0x393; 915 1 整数
    //皮肤模型
    uint8_t model;
    uint8_t unused[15];
    uint32_t name_color_checksum;
    uint8_t section;
    uint8_t ch_class;
    uint8_t v2flags;
    uint8_t version;
    uint32_t v1flags;
    uint16_t costume;
    uint16_t skin;
    uint16_t face;
    uint16_t head;
    uint16_t hair;
    uint16_t hair_r;
    uint16_t hair_g;
    uint16_t hair_b;
    float prop_x;
    float prop_y;
    uint8_t config[0x48];
    uint8_t techniques[0x14];
} PACKED v1_player_t;

typedef struct v2_player {
    inventory_t inv;
    psocn_stats_t stats;
    uint8_t opt_flag[10];
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    char name[16];
    uint32_t unk3[2];
    uint8_t name_color_b; // 0x390; 912  1 整数
    uint8_t name_color_g; // 0x391; 913 1 整数
    uint8_t name_color_r; // 0x392; 914 1 整数
    uint8_t name_color_transparency; // 0x393; 915 1 整数
    //皮肤模型
    uint8_t model;
    uint8_t unused[15];
    uint32_t name_color_checksum;
    uint8_t section;
    uint8_t ch_class;
    uint8_t v2flags;
    uint8_t version;
    uint32_t v1flags;
    uint16_t costume;
    uint16_t skin;
    uint16_t face;
    uint16_t head;
    uint16_t hair;
    uint16_t hair_r;
    uint16_t hair_g;
    uint16_t hair_b;
    float prop_x;
    float prop_y;
    uint8_t config[0x48];
    uint8_t techniques[0x14];
    uint32_t padding;
    union {
        uint8_t all[0xB8];
        struct {
            uint32_t unk1;
            char string[0x0C];
            uint8_t unk2[0x24];
            uint16_t grave_unk4;
            uint16_t grave_deaths;
            uint32_t grave_coords_time[5];
            char grave_team[20];
            char grave_message[24];
            uint32_t times[9];
            uint32_t battle[7];
        } part;
    } c_rank;
    uint32_t unk4[6];
} PACKED v2_player_t;

typedef struct pc_player {
    inventory_t inv;
    psocn_stats_t stats;
    uint8_t opt_flag[10];
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    char guildcard_str[16];
    uint32_t unk3[2];
    uint8_t name_color_b; // 0x390; 912  1 整数
    uint8_t name_color_g; // 0x391; 913 1 整数
    uint8_t name_color_r; // 0x392; 914 1 整数
    uint8_t name_color_transparency; // 0x393; 915 1 整数
    //皮肤模型
    uint8_t model;
    uint8_t unused[15];
    uint32_t name_color_checksum;
    uint8_t section;
    uint8_t ch_class;
    uint8_t v2flags;
    uint8_t version;
    uint32_t v1flags;
    uint16_t costume;
    uint16_t skin;
    uint16_t face;
    uint16_t head;
    uint16_t hair;
    uint16_t hair_r;
    uint16_t hair_g;
    uint16_t hair_b;
    float prop_x;
    float prop_y;
    uint8_t config[0x48];
    uint8_t techniques[0x14];
    uint32_t padding;
    union {
        uint8_t all[0xF0];
        struct {
            uint32_t unk1;
            uint16_t string[0x0C];
            uint8_t unk2[0x24];
            uint16_t grave_unk4;
            uint16_t grave_deaths;
            uint32_t grave_coords_time[5];
            uint16_t grave_team[20];
            uint16_t grave_message[24];
            uint32_t times[9];
            uint32_t battle[7];
        } part;
    } c_rank;
    uint32_t unk4[6];
    uint32_t blacklist[30];
    uint32_t autoreply_enabled;
    uint16_t autoreply[];               /* Always at least 4 bytes! */
} PACKED pc_player_t;

typedef struct v3_player {
    inventory_t inv;
    psocn_stats_t stats;
    uint8_t opt_flag[10];
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    char guildcard_str[16];
    uint32_t unk3[2];
    uint8_t name_color_b; // 0x390; 912  1 整数
    uint8_t name_color_g; // 0x391; 913 1 整数
    uint8_t name_color_r; // 0x392; 914 1 整数
    uint8_t name_color_transparency; // 0x393; 915 1 整数
    //皮肤模型
    uint8_t model;
    uint8_t unused[15];
    uint32_t name_color_checksum;
    uint8_t section;
    uint8_t ch_class;
    uint8_t v2flags;
    uint8_t version;
    uint32_t v1flags;
    uint16_t costume;
    uint16_t skin;
    uint16_t face;
    uint16_t head;
    uint16_t hair;
    uint16_t hair_r;
    uint16_t hair_g;
    uint16_t hair_b;
    float prop_x;
    float prop_y;
    uint8_t config[0x48];
    uint8_t techniques[0x14];
    uint32_t padding;
    union {
        uint8_t all[0x0118];
        struct {
            uint32_t unk1;          /* Flip the words for dc/pc! */
            uint32_t times[9];
            uint32_t times_ep2[5];
            uint8_t unk2[0x24];     /* Probably corresponds to unk2 dc/pc */
            uint32_t grave_unk4;
            uint32_t grave_deaths;
            uint32_t grave_coords_time[5];
            char grave_team[20];
            char grave_message[48];
            uint8_t unk3[24];
            char string[12];
            uint8_t unk4[24];
            uint32_t battle[7];
        } part;
    } c_rank;
    uint32_t unk4[6];
    char infoboard[0xAC];
    uint32_t blacklist[30];
    uint32_t autoreply_enabled;
    char autoreply[];                   /* Always at least 4 bytes! */
} PACKED v3_player_t;

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

