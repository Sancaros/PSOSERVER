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

#ifndef ITEM_RANDOM_H
#define ITEM_RANDOM_H

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <f_logs.h>

#include <SFMT.h>

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct table_spec {
    uint32_t offset;
    uint8_t entries_per_table;
    uint8_t unused[3];
} PACKED table_spec_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

static table_spec_t armorrandomset[3];

uint32_t rand_int(sfmt_t* rng, uint64_t max);

float rand_float_0_1_from_crypt(sfmt_t* rng);

int load_ArmorRandomSet_data(const char* fn);


#endif // !ITEM_RANDOM_H
