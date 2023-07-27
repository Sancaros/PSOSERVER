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

/* 24 */
typedef struct battle_records {
    uint16_t place_counts[4];//8
    uint16_t disconnect_count;//2
    union data
    {
        uint8_t unknown_a1[0x0E];//14
        struct MyStruct
        {
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
        };

    };
} PACKED battle_records_t;

#define PSOCN_STRUCT_BATTLE

/* 160 */
typedef struct dc_challenge_records {
    uint16_t title_color;
    uint16_t unknown_u0;
    char rank_title[0x0C];
    uint32_t times_ep1_online[9];
    uint16_t grave_unk4;
    uint16_t grave_deaths;
    uint32_t grave_coords_time[5];
    char grave_team[0x14];
    char grave_message[0x18];
    uint32_t times_ep1_offline[9];
    uint8_t battle[4];
} PACKED dc_challenge_records_t;

/* 188 V1 V2 */
typedef struct psocn_dc_records_data {
    uint32_t client_id;
    dc_challenge_records_t challenge;
    battle_records_t battle;
} PACKED psocn_dc_records_data_t;

//typedef struct psocn_v2_c_rank_data {
//    uint32_t client_id;
//    union {
//        uint8_t data[184];
//        struct {
//            uint32_t unk1;
//            char string[0x0C];
//            uint8_t times_ep1_online[0x24];
//            uint16_t grave_unk4;
//            uint16_t grave_deaths;
//            uint32_t grave_coords_time[5];
//            char grave_team[20];
//            char grave_message[24];
//            uint32_t times_ep1_offline[9];
//            uint32_t battle[7];
//        } part;
//    } c_rank;
//} PACKED psocn_v2_c_rank_data_t;

/* 216 */
typedef struct pc_challenge_records {
    uint16_t title_color;
    uint16_t unknown_u0;
    uint16_t rank_title[0x0C];
    uint32_t times_ep1_online[9];
    uint16_t grave_unk4;
    uint16_t grave_deaths;
    uint32_t grave_coords_time[5];
    uint16_t grave_team[0x14];
    uint16_t grave_message[0x18];
    uint32_t times_ep1_offline[9];
    uint8_t battle[4];
} PACKED pc_challenge_records_t;

/* 244 PC */
typedef struct psocn_pc_records_data {
    uint32_t client_id;
    pc_challenge_records_t challenge;
    battle_records_t battle;
} PACKED psocn_pc_records_data_t;

//typedef struct psocn_pc_c_rank_data {
//    uint32_t client_id;
//    union {
//        uint8_t data[0xF0];
//        struct {
//            uint32_t unk1;
//            uint16_t string[0x0C];
//            uint8_t times_ep1_online[0x24];
//            uint16_t grave_unk4;
//            uint16_t grave_deaths;
//            uint32_t grave_coords_time[5];
//            uint16_t grave_team[20];
//            uint16_t grave_message[24];
//            uint32_t times_ep1_offline[9];
//            uint32_t battle[7];
//        } part;
//    } c_rank;
//} PACKED psocn_pc_c_rank_data_t;

/* 256 */
typedef struct v3_challenge_records {
    uint16_t title_color;
    uint16_t unknown_u0;
    uint32_t times_ep1_online[9];
    uint32_t times_ep2_online[5];
    uint32_t times_ep1_offline[9];
    uint8_t unknown_g3[4];
    uint16_t grave_deaths;
    uint16_t unknown_u4;
    uint32_t grave_coords_time[5];
    char grave_team[0x14];
    char grave_message[0x20];
    uint8_t unknown_m5[4];
    uint32_t unknown_t6[9];
    char rank_title[12];
    uint32_t battle[7];
} PACKED v3_challenge_records_t;

/* 284 GC */
typedef struct psocn_v3_records_data {
    uint32_t client_id;
    v3_challenge_records_t challenge;
    battle_records_t battle;
} PACKED psocn_v3_records_data_t;

//typedef struct psocn_v3_c_rank_data {
//    uint32_t client_id;
//    union {
//        uint8_t data[0x0118];
//        struct {
//            uint32_t unk1;          /* Flip the words for dc/pc! */
//            uint32_t times_ep1_offline[9];
//            uint32_t times_ep2_offline[5];
//            uint8_t times_ep1_online[0x24];     /* Probably corresponds to unk2 dc/pc */
//            uint32_t grave_unk4;
//            uint32_t grave_deaths;
//            uint32_t grave_coords_time[5];
//            char grave_team[20];
//            char grave_message[48];
//            uint8_t unk3[24];
//            char string[12];
//            uint8_t unk4[24];
//            uint32_t battle[7];
//        } part;
//    } c_rank;
//} PACKED psocn_v3_c_rank_data_t;
/* 320 */
typedef struct bb_challenge_records {
    uint16_t title_color;
    uint16_t unknown_u0;
    uint32_t times_ep1_online[9];
    uint32_t times_ep2_online[5];
    uint32_t times_ep1_offline[9];
    uint32_t grave_unk4;
    uint16_t grave_deaths;
    uint16_t unknown_u4;
    uint32_t grave_coords_time[5];
    uint16_t grave_team[20];
    uint16_t grave_message[32];
    uint8_t unk3[4];
    uint16_t string[18];
    uint16_t rank_title[12]; //称号
    uint32_t battle[7];
} PACKED bb_challenge_records_t;

/* 348 数据 4 + 320 + 24 */
typedef struct psocn_bb_records_data {
    uint32_t client_id;
    bb_challenge_records_t challenge;
    battle_records_t battle;
} PACKED psocn_bb_records_data_t;

/* 玩家挑战 对战数据的 联合结构体 */
typedef union record_data{
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
