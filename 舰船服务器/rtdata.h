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

#ifndef RTDATA_H
#define RTDATA_H

#include <stdint.h>

#include "lobby.h"

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* Entry in one of the ItemRT files. The same format is used by all versions of
   PSO. */
typedef struct rt_entry {
    uint8_t prob;
    uint8_t item_data[3];
} PACKED rt_entry_t;

typedef struct PackedDrop {
    /* 读出来的数值 是 0-255 用expand_rate之后 则是浮点数 0.000000000*/
    uint8_t probability;
    uint8_t item_code[3];
} PACKED PackedDrop_t;

typedef struct rt_table {
    // 0x280 in size; describes one difficulty, section ID, and episode
    // TODO: It looks like this structure can actually vary. In PSOGC, these all
    // appear to be the same size/format, but that's probably not strictly
    // required to be the case.
    /* 0000 */ PackedDrop_t enemy_rares[0x65];
    /* 0194 */ uint8_t box_areas[0x1E];
    /* 01B2 */ PackedDrop_t box_rares[0x1E];
    /* 022A */ uint8_t unknown_a1[2];
    /* 022C */ uint32_t enemy_rares_offset; // == 0x00000000 大端序
    /* 0230 */ uint32_t box_count; // == 0x1E000000 大端序
    /* 0234 */ uint32_t box_areas_offset; // == 0x94010000 大端序
    /* 0238 */ uint32_t box_rares_offset; // == 0xB2010000 大端序
    /* 023C */ uint32_t unused_offset1;// 大端序
    /* 0240 */ uint16_t unknown_a2[0x10];
    /* 0260 */ uint32_t unknown_a2_offset;// == 0x40020000 大端序
    /* 0264 */ uint32_t unknown_a2_count;// == 0x03000000 大端序
    /* 0268 */ uint32_t unknown_a3;// == 0x01000000 大端序
    /* 026C */ uint32_t unknown_a4;
    /* 0270 */ uint32_t offset_table_offset; // == 0x2C020000 大端序
    /* 0274 */ uint32_t unknown_a5[3];
    /* 0280 */
} PACKED rt_table_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

typedef struct rt_data {
    double prob;
    uint32_t item_data;
    uint32_t area;                      /* 无法用在敌人上 */
} rt_data_t;

typedef struct rt_set {
    rt_data_t enemy_rares[101];
    rt_data_t box_rares[30];
} rt_set_t;

//章节 难度 数据条目
static int have_v2rt = 0;
static rt_set_t v2_rtdata[4][10];
static int have_gcrt = 0;
static rt_set_t gc_rtdata[2][4][10];
static int have_bbrt = 0;
//static rt_set_t bb_rtdata[4][4][10];
static rt_table_t bb_rtdata[4][4][10];
static rt_table_t bb_rtdata_rel[4][4][10];
static rt_table_t bb_dymnamic_rtdata[4][4][10];

/* 返回难度文字名称 */
char abbreviation_for_difficulty(uint8_t difficulty);

int rt_read_v2(const char *fn);
int rt_read_gc(const char *fn);
int rt_read_bb_rel(const char* fn);
int rt_read_bb(const char* fn);

int rt_v2_enabled(void);
int rt_gc_enabled(void);
int rt_bb_enabled(void);

size_t get_rt_index(uint8_t episode, size_t rt_index);

uint32_t rt_generate_v2_rare(ship_client_t* c, lobby_t* l, int rt_index,
    int area);
uint32_t rt_generate_gc_rare(ship_client_t* c, lobby_t* l, int rt_index,
    int area);
/* block 随机值*/
uint32_t rt_generate_bb_rare(ship_client_t* src, lobby_t* l, int rt_index,
    int area, uint8_t section);
/* src 随机值*/
uint32_t rt_generate_bb_rare_style(ship_client_t* src, lobby_t* l, int rt_index,
    int area, uint8_t section);

rt_table_t* get_rt_table_bb(uint8_t episode, uint8_t challenge, uint8_t difficulty, uint8_t section);

#endif /* !RTDATA_H */
