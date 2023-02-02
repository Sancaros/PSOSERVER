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
static rt_set_t bb_rtdata[4][4][10];

int rt_read_v2(const char *fn);
int rt_read_gc(const char *fn);
int rt_read_bb(const char* fn);

int rt_v2_enabled(void);
int rt_gc_enabled(void);
int rt_bb_enabled(void);

uint32_t rt_generate_v2_rare(ship_client_t* c, lobby_t* l, int rt_index,
    int area);
uint32_t rt_generate_gc_rare(ship_client_t* c, lobby_t* l, int rt_index,
    int area);
uint32_t rt_generate_bb_rare(ship_client_t* c, lobby_t* l, int rt_index,
    int area);

#endif /* !RTDATA_H */
