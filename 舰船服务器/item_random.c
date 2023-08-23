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

#include <f_logs.h>
#include <pso_StringReader.h>

#include "item_random.h"

int load_ArmorRandomSet_data(const char* fn) {
    StringReader* r = StringReader_file(fn);

    if (!r->length) {
        ERR_LOG("读取的数据长度错误");
        StringReader_destroy(r);
        (void)getchar();
        return -1;
    }

    display_packet(r->data, r->length);

    uint32_t specs_offset_offset = 0;
    uint32_t specs_offset = 0;

    specs_offset_offset = pget_u32b(r, r->length - 0x10);

    ERR_LOG("读取的数据剩余长度 %d %d %d", StringReader_remaining(r), r->length, r->length - 0x10);

    specs_offset = pget_u32b(r, specs_offset_offset);

    DBG_LOG("specs_offset_offset 0x%08X", specs_offset_offset);
    DBG_LOG("specs_offset 0x%08X", specs_offset);

    //uint32_t specs_offset_offset = r.pget_u32b(data->size() - 0x10);
    //uint32_t specs_offset = this->r.pget_u32b(specs_offset_offset);








    (void)getchar();

    StringReader_destroy(r);
    return 0;
}