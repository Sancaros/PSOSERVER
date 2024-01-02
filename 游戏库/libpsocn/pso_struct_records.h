/*
    梦幻之星中国

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

#ifndef PSOCN_STRUCT_RECORDS_H
#define PSOCN_STRUCT_RECORDS_H

#include <stdint.h>

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct ChallengeAwardState {
#ifdef __BIG_ENDIAN__
    uint32_t rank_award_flags;
    uint32_t maximum_rank; // Encrypted; see decrypt_challenge_time
#else
    uint32_t maximum_rank; // Encrypted; see decrypt_challenge_time
    uint32_t rank_award_flags;
#endif
} PACKED ChallengeAwardState_t;

/* 24 */
typedef struct battle_records {
    // On Episode 3, place_counts[0] is win count and [1] is loss count
    uint16_t place_counts[4];//8
    uint16_t disconnect_count;//2
    union data {
        uint8_t unknown_a1[0x0E];//14
        struct MyStruct {
            uint8_t  data0;
            uint8_t  data1;
            uint8_t  data2;
            uint8_t  data3;
            uint8_t  data4;
            uint8_t  data5;
            uint8_t  data6;
            uint8_t  data7;
            uint8_t  data8;
            uint8_t  data9;
            uint8_t  data10;
            uint8_t  data11;
            uint8_t  data12;
            uint8_t  data13;
        }PACKED;
    }PACKED;
} PACKED battle_records_t;

#define PSOCN_STRUCT_BATTLE

/* 160 */
typedef struct dc_challenge_records {
    uint16_t title_color;
    uint16_t unknown_u0;
    union {
        struct {
            char title_tag;
            char title_tag2;
            char title_str[10];
        }PACKED;
        char rank_title[12]; //称号
    }PACKED;
    uint32_t times_ep1_online[9];
    uint8_t grave_stage_num;
    uint8_t grave_floor;
    uint16_t grave_deaths;
    // grave_time is encoded with the following bit fields:
//   YYYYMMMM DDDDDDDD HHHHHHHH mmmmmmmm
//   Y = year after 2000 (clamped to [0, 15])
//   M = month
//   D = day
//   H = hour
//   m = minute
    /* 38 */ uint32_t grave_time;
    /* 3C */ uint32_t grave_defeated_by_enemy_rt_index;
    /* 40 */ float grave_x;
    /* 44 */ float grave_y;
    /* 48 */ float grave_z;
    //uint32_t grave_coords_time[5];
    union {
        struct {
            char grave_team_tag;
            char grave_team_tag2;
            char grave_team_str[18];
        }PACKED;
        char grave_team[20]; //称号
    }PACKED;
    char grave_message[24];
    uint32_t times_ep1_offline[9];
    uint8_t battle[4];
} PACKED dc_challenge_records_t;

/* 188 V1 V2 */
typedef struct psocn_dc_records_data {
    uint32_t client_id;
    dc_challenge_records_t challenge;
    battle_records_t battle;
} PACKED psocn_dc_records_data_t;

/* 216 */
typedef struct pc_challenge_records {
    uint16_t title_color;
    uint16_t unknown_u0;
    union {
        struct {
            uint16_t title_tag;
            uint16_t title_tag2;
            uint16_t title_str[10];
        }PACKED;
        uint16_t rank_title[12]; //称号
    }PACKED;
    uint32_t times_ep1_online[9];
    uint8_t grave_stage_num;
    uint8_t grave_floor;
    uint16_t grave_deaths;
    // grave_time is encoded with the following bit fields:
//   YYYYMMMM DDDDDDDD HHHHHHHH mmmmmmmm
//   Y = year after 2000 (clamped to [0, 15])
//   M = month
//   D = day
//   H = hour
//   m = minute
    /* 38 */ uint32_t grave_time;
    /* 3C */ uint32_t grave_defeated_by_enemy_rt_index;
    /* 40 */ float grave_x;
    /* 44 */ float grave_y;
    /* 48 */ float grave_z;
    //uint32_t grave_coords_time[5];
    union {
        struct {
            uint16_t grave_team_tag;
            uint16_t grave_team_tag2;
            uint16_t grave_team_str[18];
        }PACKED;
        uint16_t grave_team[20]; //称号
    }PACKED;
    uint16_t grave_message[24];
    uint32_t times_ep1_offline[9];
    uint8_t battle[4];
} PACKED pc_challenge_records_t;

/* 244 PC */
typedef struct psocn_pc_records_data {
    uint32_t client_id;
    pc_challenge_records_t challenge;
    battle_records_t battle;
} PACKED psocn_pc_records_data_t;

/* 256 */
typedef struct v3_challenge_records {
    uint16_t title_color;
    uint16_t unknown_u0;
    uint32_t times_ep1_online[9];
    uint32_t times_ep2_online[5];
    uint32_t times_ep1_offline[9];
    uint8_t grave_is_ep2;
    uint8_t grave_stage_num;
    uint8_t grave_floor;
    uint8_t unknown_g0;
    uint16_t grave_deaths;
    uint16_t unknown_u4;
    // grave_time is encoded with the following bit fields:
//   YYYYMMMM DDDDDDDD HHHHHHHH mmmmmmmm
//   Y = year after 2000 (clamped to [0, 15])
//   M = month
//   D = day
//   H = hour
//   m = minute
    /* 38 */ uint32_t grave_time;
    /* 3C */ uint32_t grave_defeated_by_enemy_rt_index;
    /* 40 */ float grave_x;
    /* 44 */ float grave_y;
    /* 48 */ float grave_z;
    //uint32_t grave_coords_time[5];
    union {
        struct {
            char grave_team_tag;
            char grave_team_tag2;
            char grave_team_str[18];
        }PACKED;
        char grave_team[20]; //称号
    }PACKED;
    char grave_message[32];
    uint8_t unknown_m5[4];
    uint32_t unknown_t6[3];
    ChallengeAwardState_t ep1_online_award_state;
    ChallengeAwardState_t ep2_online_award_state;
    ChallengeAwardState_t ep1_offline_award_state;
    union {
        struct {
            char title_tag;
            char title_tag2;
            char title_str[10];
        }PACKED;
        char rank_title[12]; //称号
    }PACKED;
    uint32_t battle[7];
} PACKED v3_challenge_records_t;

/* 284 GC */
typedef struct psocn_v3_records_data {
    uint32_t client_id;
    v3_challenge_records_t challenge;
    battle_records_t battle;
} PACKED psocn_v3_records_data_t;

/* 320 */
typedef struct bb_challenge_records {
    uint16_t title_color;
    uint16_t unknown_u0;
    uint32_t times_ep1_online[9];
    uint32_t times_ep2_online[5];
    uint32_t times_ep1_offline[9];
    uint8_t grave_is_ep2;
    uint8_t grave_stage_num;
    uint8_t grave_floor;
    uint8_t unknown_g0;
    uint16_t grave_deaths;
    uint8_t unknown_u4[2];
    // grave_time is encoded with the following bit fields:
//   YYYYMMMM DDDDDDDD HHHHHHHH mmmmmmmm
//   Y = year after 2000 (clamped to [0, 15])
//   M = month
//   D = day
//   H = hour
//   m = minute
    /* 38 */ uint32_t grave_time;
    /* 3C */ uint32_t grave_defeated_by_enemy_rt_index;
    /* 40 */ float grave_x;
    /* 44 */ float grave_y;
    /* 48 */ float grave_z;
    //uint32_t grave_coords_time[5];
    union {
        struct {
            uint16_t grave_team_tag;
            uint16_t grave_team_tag2;
            uint16_t grave_team_str[18];
        }PACKED;
        uint16_t grave_team[20]; //称号
    }PACKED;
    uint16_t grave_message[32];
    uint8_t unk3[4];

    uint32_t unknown_t6[3];
    ChallengeAwardState_t ep1_online_award_state;
    ChallengeAwardState_t ep2_online_award_state;
    ChallengeAwardState_t ep1_offline_award_state;
    //uint16_t string[18];

    union {
        struct {
            uint16_t title_tag;
            uint16_t title_tag2;
            uint16_t title_str[10];
        }PACKED;
        uint16_t rank_title[12]; //称号
    }PACKED;
    uint32_t battle[7];
} PACKED bb_challenge_records_t;

/* 348 数据 4 + 320 + 24 */
typedef struct psocn_bb_records_data {
    uint32_t client_id;
    bb_challenge_records_t challenge;
    battle_records_t battle;
} PACKED psocn_bb_records_data_t;

/* 玩家挑战 对战数据的 联合结构体 */
typedef union record_data {
    psocn_dc_records_data_t v2;
    psocn_pc_records_data_t pc;
    psocn_v3_records_data_t v3;
    psocn_bb_records_data_t bb;
} record_data_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

#endif // !PSOCN_STRUCT_RECORDS_H
