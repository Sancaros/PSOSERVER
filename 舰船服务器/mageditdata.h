/*
    梦幻之星中国 Mag Edit
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

#ifndef PSOCN_MAG_EDIT_DATA_H
#define PSOCN_MAG_EDIT_DATA_H

#include <stdint.h>

#include "pso_struct_item.h"

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct magedit_table_offsets {
    union {
        struct {
            /* 00 / 0400 */ uint32_t position_table; // -> [offset -> (0xC-byte struct)[0x53], offset -> (same as first offset)]
            /* 04 / 0408 */ uint32_t movement_table; // -> (2-byte struct, or single word)[0x53]
            /* 08 / 04AE */ uint32_t movement_config_table; // -> (0xA8 bytes; possibly (8-byte struct)[0x15])
            /* 0C / 0556 */ uint32_t mesh_to_color_table; // -> (int8_t)[0x53]
            /* 10 / 05AC */ uint32_t colors_table; // -> (float)[0x48]
            /* 14 / 06CC */ uint32_t evolution_number_table; // -> (uint8_t)[0x53]
        };
        uint32_t ptr[6];
    };
} PACKED magedit_table_offsets_t;

typedef struct mag_position_table {
    struct {
        int8_t mag2_normal;
        int8_t mag2_unknow1;
        int8_t mag2_feed;
        int8_t mag2_activate;
        int8_t mag2_unknow2;
        int8_t mag2_unknow3;
        int8_t mag1_normal;
        int8_t mag1_unknow1;
        int8_t mag1_feed;
        int8_t mag1_activate;
        int8_t mag1_unknow2;
        int8_t mag1_unknow3;
    } value[0x53];
} PACKED mag_position_table_t;

typedef struct mag_movement_table {
    struct {
        int8_t movement1;
        int8_t movement2;
    } value[0x53];
} PACKED mag_movement_table_t;

typedef struct mag_movement_config_table {
    struct {
        uint8_t mesh_id;
        uint8_t speed;
        uint16_t start_angle;
        uint16_t point1_angle;
        uint16_t point2_angle;
    } value[0x15];
} PACKED mag_movement_config_table_t;

typedef struct mag_mesh_to_color_table {
    int8_t values[0x53];
} PACKED mag_mesh_to_color_table_t;

typedef struct mag_color_table {
    struct {
        float alpha;
        float red;
        float green;
        float blue;
    } value[0x12];
} PACKED mag_color_table_t;

typedef struct mag_evolution_number_table {
    uint8_t values[0x53];
} PACKED mag_evolution_number_table_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

static magedit_table_offsets_t* magedit_tb_offsets;

static mag_position_table_t mag_positions_table;
static mag_movement_table_t mag_movements_table;
static mag_movement_config_table_t mag_movement_configs_table;
static mag_mesh_to_color_table_t mag_mesh_to_colors_table;
static mag_color_table_t mag_colors_table;
static mag_evolution_number_table_t mag_evolution_numbers_table;
static uint8_t mag_evolution_numbers_table_max_values;

int have_bb_magedit;

int magedit_read_bb(const char* fn, int norestrict);
uint8_t magedit_lookup_mag_evolution_number(item_t* item);

#endif // !PSOCN_MAG_EDIT_DATA_H
