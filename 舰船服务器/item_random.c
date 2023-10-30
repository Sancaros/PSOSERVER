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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <pthread.h>
#include <GSL.h>

#include <pso_StringReader.h>

#include "item_random.h"

uint32_t rand_int(sfmt_t* rng, uint64_t max) {
    if (max != 0)
        return sfmt_genrand_uint32(rng) % max;
    else
        return 0;
}

float rand_float_0_1_from_crypt(sfmt_t* rng) {
    uint32_t next = sfmt_genrand_uint32(rng);
    next = next ^ (next << 11);
    next = next ^ (next >> 8);
    next = next ^ ((next << 19) & 0xffffffff);
    next = next ^ ((next >> 4) & 0xffffffff);
    return (float)(next >> 16) / 65536.0f;
}

int load_ArmorRandomSet_data(const char* fn) {
    StringReader* r = StringReader_file(fn);
    uint32_t tmp, tmp2 = 0;

    if (!r) {
        ERR_LOG("读取的数据长度错误");
        (void)getchar();
        return -1;
    }

    if (!r->length) {
        ERR_LOG("读取的数据长度错误");
        StringReader_destroy(r);
        (void)getchar();
        return -1;
    }
//( 00000000 )   00 21 01 21 02 0F 03 0A   04 05 05 00 06 04 07 00  .!.!............
//( 00000010 )   08 00 09 00 0A 00 0B 00   0C 00 0D 00 0E 00 0F 00  ................
//( 00000020 )   10 00 11 00 12 00 13 00   14 00 00 05 01 05 02 0A  ................
//( 00000030 )   03 0F 04 13 05 13 06 0A   07 0A 08 05 09 00 0A 00  ................
//( 00000040 )   0B 02 0C 00 0D 00 0E 00   0F 00 10 00 11 00 12 00  ................
//( 00000050 )   13 00 14 00 00 00 01 00   02 00 03 00 04 05 05 05  ................
//( 00000060 )   06 0A 07 0F 08 13 09 13   0A 0A 0B 0A 0C 05 0D 00  ................
//( 00000070 )   0E 00 0F 02 10 00 11 00   12 00 13 00 14 00 00 00  ................
//( 00000080 )   01 00 02 00 03 00 04 00   05 00 06 02 07 04 08 05  ................
//( 00000090 )   09 05 0A 0A 0B 0C 0C 0E   0D 0E 0E 0A 0F 09 10 05  ................
//( 000000A0 )   11 05 12 03 13 02 14 00   00 00 01 00 02 00 03 00  ................
//( 000000B0 )   04 00 05 00 06 00 07 02   08 05 09 05 0A 05 0B 06  ................
//( 000000C0 )   0C 0A 0D 0D 0E 0E 0F 0E   10 0E 11 07 12 03 13 01  ................
//( 000000D0 )   14 01 00 22 01 22 02 0F   03 0F 04 00 05 00 06 02  ..."."..........
//( 000000E0 )   07 00 08 00 09 00 0A 00   0B 00 0C 00 0D 00 0E 00  ................
//( 000000F0 )   0F 00 10 00 11 00 00 05   01 0A 02 0E 03 12 04 12  ................
//( 00000100 )   05 0E 06 0A 07 05 08 02   09 02 0A 02 0B 00 0C 00  ................
//( 00000110 )   0D 00 0E 00 0F 00 10 00   11 00 00 00 01 00 02 00  ................
//( 00000120 )   03 06 04 0A 05 0F 06 13   07 12 08 0E 09 09 0A 04  ................
//( 00000130 )   0B 03 0C 00 0D 02 0E 00   0F 00 10 00 11 00 00 00  ................
//( 00000140 )   01 00 02 00 03 00 04 05   05 05 06 05 07 0A 08 0E  ................
//( 00000150 )   09 0F 0A 0E 0B 0B 0C 09   0D 06 0E 04 0F 00 10 02  ................
//( 00000160 )   11 00 00 00 01 00 02 00   03 00 04 00 05 04 06 05  ................
//( 00000170 )   07 05 08 08 09 0B 0A 0D   0B 0D 0C 0E 0D 0E 0E 09  ................
//( 00000180 )   0F 02 10 01 11 01 00 00   04 00 08 00 0C 00 10 00  ................
//( 00000190 )   18 00 21 00 24 00 27 00   2A 00 2D 00 01 00 05 00  ..!.$.'.*.-.....
//( 000001A0 )   09 00 0D 00 11 00 19 00   22 00 25 00 28 00 2B 00  ........".%.(.+.
//( 000001B0 )   2E 00 33 00 36 00 39 00   43 00 00 0A 04 0A 08 0A  ..3.6.9.C.......
//( 000001C0 )   0C 0A 10 0A 18 0A 21 0A   24 0A 27 0A 2A 05 2D 05  ......!.$.'.*.-.
//( 000001D0 )   01 00 05 00 09 00 0D 00   11 00 19 00 22 00 25 00  ............".%.
//( 000001E0 )   28 00 2B 00 2E 00 33 00   36 00 39 00 43 00 00 0A  (.+...3.6.9.C...
//( 000001F0 )   04 0A 08 0A 0C 0A 10 0A   18 0A 21 0A 24 0A 27 0A  ..........!.$.'.
//( 00000200 )   2A 05 2D 05 01 00 05 00   09 00 0D 00 11 00 19 00  *.-.............
//( 00000210 )   22 00 25 00 28 00 2B 00   2E 00 33 00 36 00 39 00  ".%.(.+...3.6.9.
//( 00000220 )   43 00 00 04 04 04 08 04   0C 04 10 04 18 04 21 04  C.............!.
//( 00000230 )   24 04 27 04 2A 05 2D 05   01 05 05 05 09 05 0D 05  $.'.*.-.........
//( 00000240 )   11 05 19 05 22 05 25 05   28 05 2B 03 2E 03 33 01  ....".%.(.+...3.
//( 00000250 )   36 01 39 01 43 00 00 03   04 03 08 03 0C 03 10 03  6.9.C...........
//( 00000260 )   18 03 21 04 24 04 27 04   2A 04 2D 05 01 05 05 05  ..!.$.'.*.-.....
//( 00000270 )   09 05 0D 05 11 05 19 05   22 05 25 05 28 05 2B 05  ........".%.(.+.
//( 00000280 )   2E 05 33 02 36 02 39 02   43 00 00 00 00 00 00 00  ..3.6.9.C.......
//( 000002A0 )   1A 00 00 00 00 00 02 8C   00 00 00 00 00 00 00 00  .......?.......
//( 000002B0 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 000002C0 )   00 A3 00 02 00 02 00 02   00 00 00 00 00 00 00 00  .?.............
//( 000002D0 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 000002E0 )   00 00 02 C0 00 00 00 04   00 00 00 01 00 00 00 00  ...?...........
//( 000002F0 )   00 00 02 A4 00 00 00 00   00 00 00 00 00 00 00 00  ...?...........
    PRINT_HEX_LOG(DBG_LOG, r->data, r->length);

    tmp = pget_u32b(r, r->length - 0x10);

    tmp2 = pget_u32b(r, letoh32(tmp));

    //memcpy(&tmp, r->data + r->length - 0x10, sizeof(uint32_t));
    //tmp = LE32(tmp);

    //memcpy(&tmp2, r->data + letoh32(tmp), sizeof(uint32_t));
    //tmp2 = LE32(tmp2);

    ERR_LOG("读取的数据剩余长度 %d %d %d", StringReader_remaining(r), r->length, r->length - 0x10);

    DBG_LOG("tmp 0x%08X tmp2 0x%08X", letoh32(tmp), letoh32(tmp2));




    (void)getchar();

    StringReader_destroy(r);
    return 0;
}