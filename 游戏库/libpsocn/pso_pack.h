/*
    梦幻之星中国 压缩程序
    版权 (C) 2023 Sancaros

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

#ifndef PSO_PACK_H
#define PSO_PACK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>

#include "pso_memopt.h"

#define MAX_PACKET_SIZE 65535
#define DELAY_TIME_MS 5    // 延时时间（毫秒）


//Z_NO_COMPRESSION：无压缩，数据不会进行压缩。
//Z_BEST_SPEED：最快速度，但压缩比较低。
//Z_BEST_COMPRESSION：最佳压缩比，但速度较慢。
//Z_DEFAULT_COMPRESSION：默认压缩等级，提供一个平衡的选择。


typedef struct Packet {
    uint16_t length;
    uint8_t type;
    uint8_t data[MAX_PACKET_SIZE - sizeof(uint16_t) - sizeof(uint8_t)];
} Packet_t;

int compress_data(void* data, size_t data_size, Bytef** cmp_buf, uLong* cmp_sz, int compress_power);

//void pack(Packet_t* packet, uint8_t packet_type, const void* packet_data, size_t packet_length, int compression_level);
//
//void unpack(Packet_t* packet, void* output_data, size_t output_length);

int is_compressed_data_valid(const char* compressed_data, uLong compressed_length);

void delay(int milliseconds);

#endif // !PSO_PACK_H