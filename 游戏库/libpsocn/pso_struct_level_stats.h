/*
    梦幻之星中国 人物数值结构

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

#ifndef PSOCN_STRUCT_LEVEL_STATS_H
#define PSOCN_STRUCT_LEVEL_STATS_H

#include <stdint.h>

#define MAX_PLAYER_LEVEL                200
#define MAX_PLAYER_CLASS_DC             9
#define MAX_PLAYER_CLASS_BB             12

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* 14 字节 */
typedef struct psocn_pl_stats {
    uint16_t atp;//力量 初始3点 然后 1 : 1 增加
    uint16_t mst;//精神力 1 : 1
    uint16_t evp;//闪避力 1 : 1
    uint16_t hp;//初始血量 不知道如何计算的 按理说 应该是 1点+2血
    uint16_t dfp;//防御力 1 : 1
    uint16_t ata;//命中率 计算 = 数据库ata / 5
    uint16_t lck;//运气 1 : 1
} PACKED psocn_pl_stats_t;

typedef struct psocn_lvl_stats {
    uint8_t atp;
    uint8_t mst;
    uint8_t evp;
    uint8_t hp;
    uint8_t dfp;
    uint8_t ata;
    uint8_t lck;
    uint8_t tp;
    uint32_t exp;
} PACKED psocn_lvl_stats_t;

/* Level-up information table from PlyLevelTbl.prs */
typedef struct bb_level_table {
    psocn_pl_stats_t start_stats[MAX_PLAYER_CLASS_BB];
    uint32_t start_stats_index[MAX_PLAYER_CLASS_BB]; /* 应该是的 暂时不知 但是和索引有关系吧 */
    psocn_lvl_stats_t levels[MAX_PLAYER_CLASS_BB][MAX_PLAYER_LEVEL];
} PACKED bb_level_table_t;

/* PSOv2 level-up information table from PlayerTable.prs */
typedef struct v2_level_table {
    psocn_lvl_stats_t levels[MAX_PLAYER_CLASS_DC][MAX_PLAYER_LEVEL];
} PACKED v2_level_table_t;

/* Player levelup data */
extern bb_level_table_t bb_char_stats;
extern v2_level_table_t v2_char_stats;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

#endif // !PSOCN_STRUCT_LEVEL_STATS_H
