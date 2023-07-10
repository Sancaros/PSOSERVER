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

#ifndef PSOCN_STRUCT_CHALLENGE_H
#define PSOCN_STRUCT_CHALLENGE_H

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

/* 256 */
typedef struct {
    uint16_t title_color;
    uint8_t unknown_u0[2];
    uint32_t times_ep1_online[9];
    uint32_t times_ep2_online[5];
    uint32_t times_ep1_offline[9];
    uint8_t unknown_g3[4];
    uint16_t grave_deaths;
    uint8_t unknown_u4[2];
    uint32_t grave_coords_time[5];
    char grave_team[0x14];
    char grave_message[0x20];
    uint8_t unknown_m5[4];
    uint32_t unknown_t6[9];
    char rank_title[0x0C];
    uint8_t unknown_l7[0x1C];
} PlayerRecordsV3_Challenge_t;

static int dsadasda4 = sizeof(PlayerRecordsV3_Challenge_t);

typedef struct PlayerRecords_Battle {
    uint16_t place_counts[4];
    uint16_t disconnect_count;
    uint8_t unknown_a1[0x0E];
} PlayerRecords_Battle_t;

static int dsadasda6 = sizeof(PlayerRecords_Battle_t);

/* 320 */
typedef struct {
    uint16_t title_color;
    uint8_t unknown_u0[2];
    uint32_t times_ep1_online[9];
    uint32_t times_ep2_online[5];
    uint32_t times_ep1_offline[9];
    uint32_t grave_unk4;
    uint16_t grave_deaths;
    uint8_t unknown_u4[2];
    uint32_t grave_coords_time[5];
    uint16_t grave_team[20];
    uint16_t grave_message[32];
    uint8_t unk3[4];
    uint16_t string[18];
    uint16_t rank_title[12];
    uint32_t battle[7];
} PlayerRecordsBB_Challenge_t;

static int dsadasda5 = sizeof(PlayerRecordsBB_Challenge_t);

/* BB挑战模式数据结构 TODO 未完成 结构大小348 数据344*/
typedef struct psocn_bb_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0x0158];
        PlayerRecordsBB_Challenge_t part;
    } c_rank;
} PACKED psocn_bb_c_rank_data_t;

static int bb_c_size = sizeof(psocn_bb_c_rank_data_t);

/* 160 */
typedef struct {
    uint16_t title_color;
    uint8_t unknown_u0[2];
    char rank_title[0x0C];
    uint32_t times_ep1_online[9];
    uint16_t unknown_g3;
    uint16_t grave_deaths;
    uint32_t grave_coords_time[5];
    char grave_team[0x14];
    char grave_message[0x18];
    uint32_t times_ep1_offline[9];
    uint8_t unknown_l4[4];
} PlayerRecordsDC_Challenge_t;

static int dsadasda2 = sizeof(PlayerRecordsDC_Challenge_t);

typedef struct psocn_v2_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0xB8];
        struct {
            uint32_t unk1;
            char string[0x0C];
            uint8_t times_ep1_online[0x24];
            uint16_t grave_unk4;
            uint16_t grave_deaths;
            uint32_t grave_coords_time[5];
            char grave_team[20];
            char grave_message[24];
            uint32_t times_ep1_offline[9];
            uint32_t battle[7];
        } part;
    } c_rank;
} PACKED psocn_v2_c_rank_data_t;

static int v2_c_size = sizeof(psocn_v2_c_rank_data_t);

/* 216 */
typedef struct {
    uint16_t title_color;
    uint8_t unknown_u0[2];
    uint16_t rank_title[0x0C];
    uint32_t times_ep1_online[9];
    uint16_t unknown_g3;
    uint16_t grave_deaths;
    uint32_t grave_coords_time[5];
    uint16_t grave_team[0x14];
    uint16_t grave_message[0x18];
    uint32_t times_ep1_offline[9];
    uint8_t unknown_l4[4];
} PlayerRecordsPC_Challenge_t;

static int dsadasda3 = sizeof(PlayerRecordsPC_Challenge_t);

typedef struct psocn_pc_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0xF0];
        struct {
            uint32_t unk1;
            uint16_t string[0x0C];
            uint8_t times_ep1_online[0x24];
            uint16_t grave_unk4;
            uint16_t grave_deaths;
            uint32_t grave_coords_time[5];
            uint16_t grave_team[20];
            uint16_t grave_message[24];
            uint32_t times_ep1_offline[9];
            uint32_t battle[7];
        } part;
    } c_rank;
} PACKED psocn_pc_c_rank_data_t;

static int pc_c_size = sizeof(psocn_pc_c_rank_data_t);

typedef struct psocn_v3_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0x0118];
        struct {
            uint32_t unk1;          /* Flip the words for dc/pc! */
            uint32_t times_ep1_offline[9];
            uint32_t times_ep2_offline[5];
            uint8_t times_ep1_online[0x24];     /* Probably corresponds to unk2 dc/pc */
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
} PACKED psocn_v3_c_rank_data_t;

static int v3_c_size = sizeof(psocn_v3_c_rank_data_t);


#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

#endif // !PSOCN_STRUCT_CHALLENGE_H
