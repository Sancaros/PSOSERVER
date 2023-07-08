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

/* BB挑战模式数据结构 TODO 未完成 结构大小348 数据344*/
typedef struct psocn_bb_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0x0158];
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
} PACKED psocn_bb_c_rank_data_t;

static int bb_c_size = sizeof(psocn_bb_c_rank_data_t);

typedef struct psocn_v2_c_rank_data {
    uint32_t client_id;
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
} PACKED psocn_v2_c_rank_data_t;

static int v2_c_size = sizeof(psocn_v2_c_rank_data_t);

typedef struct psocn_pc_c_rank_data {
    uint32_t client_id;
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
} PACKED psocn_pc_c_rank_data_t;

static int pc_c_size = sizeof(psocn_pc_c_rank_data_t);

typedef struct psocn_v3_c_rank_data {
    uint32_t client_id;
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
} PACKED psocn_v3_c_rank_data_t;

static int v3_c_size = sizeof(psocn_v3_c_rank_data_t);


#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

#endif // !PSOCN_STRUCT_CHALLENGE_H
