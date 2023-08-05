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

#include <windows.h>
#include <WinSock_Defines.h>

#include <f_logs.h>
#include <mtwist.h>

#include <PRS.h>

#include "pmtdata.h"
#include "utils.h"
#include "packets.h"
#include "handle_player_items.h"

/* The parsing code in here is based on some code/information from Lee. Thanks
   again! */
static int read_v2ptr_tbl(const uint8_t *pmt, uint32_t sz, uint32_t ptrs[21]) {
    uint32_t tmp;
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    uint32_t i;
#endif

    memcpy(&tmp, pmt + sz - 16, sizeof(uint32_t));
    tmp = LE32(tmp);

    if(tmp + sizeof(uint32_t) * 21 > sz - 16) {
        ERR_LOG("v2 ItemPMT 中的指针表位置无效!");
        return -1;
    }

    memcpy(ptrs, pmt + tmp, sizeof(uint32_t) * 21);

#ifdef DEBUG

    for (int i = 0; i < 21; ++i) {
        DBG_LOG("v2 offset %02d 0x%08X", i, ptrs[i]);
    }

    getchar();

#endif // DEBUG

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    for(i = 0; i < 21; ++i) {
        ptrs[i] = LE32(ptrs[i]);
    }
#endif

    return 0;
}

static int read_gcptr_tbl(const uint8_t *pmt, uint32_t sz, pmt_table_offsets_v3_t* ptrs) {
    uint32_t tmp;
#if !defined(WORDS_BIGENDIAN) && !defined(__BIG_ENDIAN__)
    uint32_t i;
#endif

    memcpy(&tmp, pmt + sz - 16, sizeof(uint32_t));
    tmp = ntohl(tmp);

    if(tmp + sizeof(pmt_table_offsets_v3_t) > sz - 16) {
        ERR_LOG("GC ItemPMT 中的指针表位置无效!");
        return -1;
    }

    memcpy(ptrs->ptr, pmt + tmp, sizeof(pmt_table_offsets_v3_t));

#if !defined(WORDS_BIGENDIAN) && !defined(__BIG_ENDIAN__)
    for(i = 0; i < 23; ++i) {
        ptrs->ptr[i] = ntohl(ptrs->ptr[i]);
    }
#endif

    return 0;
}

static int read_bbptr_tbl(const uint8_t *pmt, uint32_t sz, pmt_table_offsets_v3_t* ptrs) {
    uint32_t tmp;
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    uint32_t i;
#endif

    memcpy(&tmp, pmt + sz - 16, sizeof(uint32_t));
    tmp = LE32(tmp);

    if(tmp + sizeof(pmt_table_offsets_v3_t) > sz - 16) {
        ERR_LOG("BB ItemPMT 中的指针表位置无效!");
        return -1;
    }

    memcpy(ptrs->ptr, pmt + tmp, sizeof(pmt_table_offsets_v3_t));

#ifdef DEBUG

    DBG_LOG("armor_table offsets = %u", ptrs->armor_table);
    DBG_LOG("ranged_special_table offsets = %u", ptrs->ranged_special_table);
    for (int i = 0; i < 23; ++i) {
        DBG_LOG("offsets = %u", ptrs->ptr[i]);
    }

#endif // DEBUG

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    for(int i = 0; i < 23; ++i) {
        ptrs.ptr[i] = LE32(ptrs.ptr[i]);
    }
#endif

    return 0;
}

static int read_weapons_v2(const uint8_t *pmt, uint32_t sz,
                           const uint32_t ptrs[21]) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the 指针无效 are sane... */
    if(ptrs[1] > sz || ptrs[1] > ptrs[11]) {
        ERR_LOG("ItemPMT.prs file for v2 的 weapon 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* 算出我们有多少张表... */
    num_weapon_types = cnt = (ptrs[11] - ptrs[1]) / 8;

    /* Allocate the stuff we need to allocate... */
    if(!(num_weapons = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("Cannot allocate space for v2 weapon count: %s",
              strerror(errno));
        num_weapon_types = 0;
        return -2;
    }

    if(!(weapons = (pmt_weapon_v2_t **)malloc(sizeof(pmt_weapon_v2_t *) *
                                              cnt))) {
        ERR_LOG("Cannot allocate space for v2 weapon list: %s",
              strerror(errno));
        free_safe(num_weapons);
        num_weapons = NULL;
        num_weapon_types = 0;
        return -3;
    }

    memset(weapons, 0, sizeof(pmt_weapon_v2_t *) * cnt);

    /* Read in each table... */
    for(i = 0; i < cnt; ++i) {
        /* Read the pointer and the size... */
        memcpy(values, pmt + ptrs[1] + (i << 3), sizeof(uint32_t) * 2);
        values[0] = LE32(values[0]);
        values[1] = LE32(values[1]);

        /* Make sure we have enough file... */
        if(values[1] + sizeof(pmt_weapon_v2_t) * values[0] > sz) {
            ERR_LOG("ItemPMT.prs file for v2 has weapon table outside "
                  "of file bounds! 请检查文件的有效性!");
            return -4;
        }

        num_weapons[i] = values[0];
        if(!(weapons[i] = (pmt_weapon_v2_t *)malloc(sizeof(pmt_weapon_v2_t) *
                                                    values[0]))) {
            ERR_LOG("Cannot allocate space for v2 weapons: %s",
                  strerror(errno));
            return -5;
        }

        memcpy(weapons[i], pmt + values[1],
               sizeof(pmt_weapon_v2_t) * values[0]);

        for(j = 0; j < values[0]; ++j) {
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
            weapons[i][j].index = LE32(weapons[i][j].index);
            weapons[i][j].atp_min = LE16(weapons[i][j].atp_min);
            weapons[i][j].atp_max = LE16(weapons[i][j].atp_max);
            weapons[i][j].atp_req = LE16(weapons[i][j].atp_req);
            weapons[i][j].mst_req = LE16(weapons[i][j].mst_req);
            weapons[i][j].ata_req = LE16(weapons[i][j].ata_req);
#endif

            if(weapons[i][j].index < weapon_lowest)
                weapon_lowest = weapons[i][j].index;
        }
    }

    return 0;
}

static int read_weapons_gc(const uint8_t *pmt, uint32_t sz,
                           const pmt_table_offsets_v3_t* ptrs) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the 指针无效 are sane... */
    if(ptrs->ptr[0] > sz || ptrs->ptr[0] > ptrs->ptr[17]) {
        ERR_LOG("ItemPMT.prs file for GC 的 weapon 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* 算出我们有多少张表... */
    num_weapon_types_gc = cnt = (ptrs->ptr[17] - ptrs->ptr[0]) / 8;

    /* Allocate the stuff we need to allocate... */
    if(!(num_weapons_gc = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("Cannot allocate space for GC weapon count: %s",
              strerror(errno));
        num_weapon_types_gc = 0;
        return -2;
    }

    if(!(weapons_gc = (pmt_weapon_gc_t **)malloc(sizeof(pmt_weapon_gc_t *) *
                                                 cnt))) {
        ERR_LOG("Cannot allocate space for GC weapon list: %s",
              strerror(errno));
        free_safe(num_weapons_gc);
        num_weapons_gc = NULL;
        num_weapon_types_gc = 0;
        return -3;
    }

    memset(weapons_gc, 0, sizeof(pmt_weapon_gc_t *) * cnt);

    /* Read in each table... */
    for(i = 0; i < cnt; ++i) {
        /* Read the pointer and the size... */
        memcpy(values, pmt + ptrs->ptr[0] + (i << 3), sizeof(uint32_t) * 2);
        values[0] = ntohl(values[0]);
        values[1] = ntohl(values[1]);

        /* Make sure we have enough file... */
        if(values[1] + sizeof(pmt_weapon_gc_t) * values[0] > sz) {
            ERR_LOG("ItemPMT.prs file for GC has weapon table outside "
                  "of file bounds! 请检查文件的有效性!");
            return -4;
        }

        num_weapons_gc[i] = values[0];
        if(!(weapons_gc[i] = (pmt_weapon_gc_t *)malloc(sizeof(pmt_weapon_gc_t) *
                                                       values[0]))) {
            ERR_LOG("Cannot allocate space for GC weapons: %s",
                  strerror(errno));
            return -5;
        }

        memcpy(weapons_gc[i], pmt + values[1],
               sizeof(pmt_weapon_gc_t) * values[0]);

        for(j = 0; j < values[0]; ++j) {
#if !defined(__BIG_ENDIAN__) && !defined(WORDS_BIGENDIAN)
            weapons_gc[i][j].index = ntohl(weapons_gc[i][j].index);
            weapons_gc[i][j].model = ntohs(weapons_gc[i][j].model);
            weapons_gc[i][j].skin = ntohs(weapons_gc[i][j].skin);
            weapons_gc[i][j].atp_min = ntohs(weapons_gc[i][j].atp_min);
            weapons_gc[i][j].atp_max = ntohs(weapons_gc[i][j].atp_max);
            weapons_gc[i][j].atp_req = ntohs(weapons_gc[i][j].atp_req);
            weapons_gc[i][j].mst_req = ntohs(weapons_gc[i][j].mst_req);
            weapons_gc[i][j].ata_req = ntohs(weapons_gc[i][j].ata_req);
            weapons_gc[i][j].mst = ntohs(weapons_gc[i][j].mst);
#endif

            if(weapons_gc[i][j].index < weapon_lowest_gc)
                weapon_lowest_gc = weapons_gc[i][j].index;
        }
    }

    return 0;
}

static int read_weapons_bb(const uint8_t *pmt, uint32_t sz,
                           const pmt_table_offsets_v3_t* ptrs) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the 指针无效 are sane... */
    if(ptrs->weapon_table > sz || ptrs->weapon_table > ptrs->combination_table) {
        ERR_LOG("ItemPMT.prs file for BB 的 weapon 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* 算出我们有多少张表... */
    num_weapon_types_bb = cnt = (ptrs->combination_table - ptrs->weapon_table) / 8;

    /* Allocate the stuff we need to allocate... */
    if(!(num_weapons_bb = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("Cannot allocate space for BB weapon count: %s",
              strerror(errno));
        num_weapon_types_bb = 0;
        return -2;
    }

    if(!(weapons_bb = (pmt_weapon_bb_t **)malloc(sizeof(pmt_weapon_bb_t *) *
                                                 cnt))) {
        ERR_LOG("Cannot allocate space for BB weapon list: %s",
              strerror(errno));
        free_safe(num_weapons_bb);
        num_weapons_bb = NULL;
        num_weapon_types_bb = 0;
        return -3;
    }

    memset(weapons_bb, 0, sizeof(pmt_weapon_bb_t *) * cnt);

    /* Read in each table... */
    for(i = 0; i < cnt; ++i) {
        /* Read the pointer and the size... */
        memcpy(values, pmt + ptrs->weapon_table + (i << 3), sizeof(uint32_t) * 2);
        values[0] = LE32(values[0]);
        values[1] = LE32(values[1]);

        /* Make sure we have enough file... */
        if(values[1] + sizeof(pmt_weapon_bb_t) * values[0] > sz) {
            ERR_LOG("ItemPMT.prs file for BB has weapon table outside "
                  "of file bounds! 请检查文件的有效性!");
            return -4;
        }

        num_weapons_bb[i] = values[0];
        if(!(weapons_bb[i] = (pmt_weapon_bb_t *)malloc(sizeof(pmt_weapon_bb_t) *
                                                       values[0]))) {
            ERR_LOG("Cannot allocate space for BB weapons: %s",
                  strerror(errno));
            return -5;
        }

        memcpy(weapons_bb[i], pmt + values[1],
               sizeof(pmt_weapon_bb_t) * values[0]);

        for(j = 0; j < values[0]; ++j) {
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
            weapons_bb[i][j].index = LE32(weapons_bb[i][j].index);
            weapons_bb[i][j].model = LE16(weapons_bb[i][j].model);
            weapons_bb[i][j].skin = LE16(weapons_bb[i][j].skin);
            weapons_bb[i][j].team_points = LE16(weapons_bb[i][j].team_points);
            weapons_bb[i][j].atp_min = LE16(weapons_bb[i][j].atp_min);
            weapons_bb[i][j].atp_max = LE16(weapons_bb[i][j].atp_max);
            weapons_bb[i][j].atp_req = LE16(weapons_bb[i][j].atp_req);
            weapons_bb[i][j].mst_req = LE16(weapons_bb[i][j].mst_req);
            weapons_bb[i][j].ata_req = LE16(weapons_bb[i][j].ata_req);
            weapons_bb[i][j].mst = LE16(weapons_bb[i][j].mst);
#endif

            if(weapons_bb[i][j].index < weapon_lowest_bb)
                weapon_lowest_bb = weapons_bb[i][j].index;
        }
    }

    return 0;
}

static int read_guards_v2(const uint8_t *pmt, uint32_t sz,
                          const uint32_t ptrs[21]) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the 指针无效 are sane... */
    if(ptrs[3] > sz || ptrs[2] > ptrs[3]) {
        ERR_LOG("ItemPMT.prs file for v2 的 guard 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* 算出我们有多少张表... */
    num_guard_types = cnt = (ptrs[3] - ptrs[2]) / 8;

    /* Make sure its sane... Should always be 2. */
    if(cnt != 2) {
        ERR_LOG("ItemPMT.prs file for v2 does not have two guard "
              "tables. 请检查其有效性!");
        num_guard_types = 0;
        return -2;
    }

    /* Allocate the stuff we need to allocate... */
    if(!(num_guards = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("Cannot allocate space for v2 guard count: %s",
              strerror(errno));
        num_guard_types = 0;
        return -3;
    }

    if(!(guards = (pmt_guard_v2_t **)malloc(sizeof(pmt_guard_v2_t *) * cnt))) {
        ERR_LOG("Cannot allocate space for v2 guards list: %s",
              strerror(errno));
        free_safe(num_guards);
        num_guards = NULL;
        num_guard_types = 0;
        return -4;
    }

    memset(guards, 0, sizeof(pmt_guard_v2_t *) * cnt);

    /* Read in each table... */
    for(i = 0; i < cnt; ++i) {
        /* Read the pointer and the size... */
        memcpy(values, pmt + ptrs[2] + (i << 3), sizeof(uint32_t) * 2);
        values[0] = LE32(values[0]);
        values[1] = LE32(values[1]);

        /* Make sure we have enough file... */
        if(values[1] + sizeof(pmt_guard_v2_t) * values[0] > sz) {
            ERR_LOG("ItemPMT.prs file for v2 has guard table outside "
                  "of file bounds! 请检查文件的有效性!");
            return -5;
        }

        num_guards[i] = values[0];
        if(!(guards[i] = (pmt_guard_v2_t *)malloc(sizeof(pmt_guard_v2_t) *
                                                  values[0]))) {
            ERR_LOG("Cannot allocate space for v2 guards: %s",
                  strerror(errno));
            return -6;
        }

        memcpy(guards[i], pmt + values[1], sizeof(pmt_guard_v2_t) * values[0]);

        for(j = 0; j < values[0]; ++j) {
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
            guards[i][j].index = LE32(guards[i][j].index);
            guards[i][j].base_dfp = LE16(guards[i][j].base_dfp);
            guards[i][j].base_evp = LE16(guards[i][j].base_evp);
#endif

            if(guards[i][j].index < guard_lowest)
                guard_lowest = guards[i][j].index;
        }
    }

    return 0;
}

static int read_guards_gc(const uint8_t *pmt, uint32_t sz,
                          const pmt_table_offsets_v3_t* ptrs) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the 指针无效 are sane... */
    if(ptrs->ptr[2] > sz || ptrs->ptr[1] > ptrs->ptr[2]) {
        ERR_LOG("ItemPMT.prs file for GC 的 guard 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* 算出我们有多少张表... */
    num_guard_types_gc = cnt = (ptrs->ptr[2] - ptrs->ptr[1]) / 8;

    /* Make sure its sane... Should always be 2. */
    if(cnt != 2) {
        ERR_LOG("ItemPMT.prs file for GC does not have two guard "
              "tables. 请检查其有效性!");
        num_guard_types_gc = 0;
        return -2;
    }

    /* Allocate the stuff we need to allocate... */
    if(!(num_guards_gc = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("Cannot allocate space for GC guard count: %s",
              strerror(errno));
        num_guard_types_gc = 0;
        return -3;
    }

    if(!(guards_gc = (pmt_guard_gc_t **)malloc(sizeof(pmt_guard_gc_t *) *
                                               cnt))) {
        ERR_LOG("Cannot allocate space for GC guards list: %s",
              strerror(errno));
        free_safe(num_guards_gc);
        num_guards_gc = NULL;
        num_guard_types_gc = 0;
        return -4;
    }

    memset(guards_gc, 0, sizeof(pmt_guard_gc_t *) * cnt);

    /* Read in each table... */
    for(i = 0; i < cnt; ++i) {
        /* Read the pointer and the size... */
        memcpy(values, pmt + ptrs->ptr[1] + (i << 3), sizeof(uint32_t) * 2);
        values[0] = ntohl(values[0]);
        values[1] = ntohl(values[1]);

        /* Make sure we have enough file... */
        if(values[1] + sizeof(pmt_guard_gc_t) * values[0] > sz) {
            ERR_LOG("ItemPMT.prs file for GC has guard table outside "
                  "of file bounds! 请检查文件的有效性!");
            return -5;
        }

        num_guards_gc[i] = values[0];
        if(!(guards_gc[i] = (pmt_guard_gc_t *)malloc(sizeof(pmt_guard_gc_t) *
                                                     values[0]))) {
            ERR_LOG("Cannot allocate space for GC guards: %s",
                  strerror(errno));
            return -6;
        }

        memcpy(guards_gc[i], pmt + values[1],
               sizeof(pmt_guard_gc_t) * values[0]);

        for(j = 0; j < values[0]; ++j) {
#if !defined(__BIG_ENDIAN__) && !defined(WORDS_BIGENDIAN)
            guards_gc[i][j].index = ntohl(guards_gc[i][j].index);
            guards_gc[i][j].model = ntohs(guards_gc[i][j].model);
            guards_gc[i][j].skin = ntohs(guards_gc[i][j].skin);
            guards_gc[i][j].base_dfp = ntohs(guards_gc[i][j].base_dfp);
            guards_gc[i][j].base_evp = ntohs(guards_gc[i][j].base_evp);
#endif

            if(guards_gc[i][j].index < guard_lowest_gc)
                guard_lowest_gc = guards_gc[i][j].index;
        }
    }

    return 0;
}

static int read_guards_bb(const uint8_t *pmt, uint32_t sz,
                          const pmt_table_offsets_v3_t* ptrs) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the 指针无效 are sane... */
    if(ptrs->unit_table > sz || ptrs->armor_table > ptrs->unit_table) {
        ERR_LOG("ItemPMT.prs file for BB 的 guard 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* 算出我们有多少张表... */
    num_guard_types_bb = cnt = (ptrs->unit_table - ptrs->armor_table) / 8;

    /* Make sure its sane... Should always be 2. */
    if(cnt != 2) {
        ERR_LOG("ItemPMT.prs file for BB does not have two guard "
              "tables. 请检查其有效性!");
        num_guard_types_bb = 0;
        return -2;
    }

    /* Allocate the stuff we need to allocate... */
    if(!(num_guards_bb = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("Cannot allocate space for BB guard count: %s",
              strerror(errno));
        num_guard_types_bb = 0;
        return -3;
    }

    if(!(guards_bb = (pmt_guard_bb_t **)malloc(sizeof(pmt_guard_bb_t *) *
                                               cnt))) {
        ERR_LOG("Cannot allocate space for BB guards list: %s",
              strerror(errno));
        free_safe(num_guards_bb);
        num_guards_bb = NULL;
        num_guard_types_bb = 0;
        return -4;
    }

    memset(guards_bb, 0, sizeof(pmt_guard_bb_t *) * cnt);

    /* Read in each table... */
    for(i = 0; i < cnt; ++i) {
        /* Read the pointer and the size... */
        memcpy(values, pmt + ptrs->armor_table + (i << 3), sizeof(uint32_t) * 2);
        values[0] = LE32(values[0]);
        values[1] = LE32(values[1]);

        /* Make sure we have enough file... */
        if(values[1] + sizeof(pmt_guard_bb_t) * values[0] > sz) {
            ERR_LOG("ItemPMT.prs file for BB has guard table outside "
                  "of file bounds! 请检查文件的有效性!");
            return -5;
        }

        num_guards_bb[i] = values[0];
        if(!(guards_bb[i] = (pmt_guard_bb_t *)malloc(sizeof(pmt_guard_bb_t) *
                                                     values[0]))) {
            ERR_LOG("Cannot allocate space for BB guards: %s",
                  strerror(errno));
            return -6;
        }

        memcpy(guards_bb[i], pmt + values[1],
               sizeof(pmt_guard_bb_t) * values[0]);

        for(j = 0; j < values[0]; ++j) {
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
            guards_bb[i][j].index = LE32(guards_bb[i][j].index);
            guards_bb[i][j].model = LE16(guards_bb[i][j].model);
            guards_bb[i][j].skin = LE16(guards_bb[i][j].skin);
            guards_bb[i][j].team_points = LE16(guards_bb[i][j].team_points);
            guards_bb[i][j].base_dfp = LE16(guards_bb[i][j].base_dfp);
            guards_bb[i][j].base_evp = LE16(guards_bb[i][j].base_evp);
#endif

            if(guards_bb[i][j].index < guard_lowest_bb)
                guard_lowest_bb = guards_bb[i][j].index;
        }
    }

    return 0;
}

static int read_units_v2(const uint8_t *pmt, uint32_t sz,
                         const uint32_t ptrs[21]) {
    uint32_t values[2], i;

    /* Make sure the 指针无效 are sane... */
    if(ptrs[3] > sz) {
        ERR_LOG("ItemPMT.prs file for v2 的 unit 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(values, pmt + ptrs[3], sizeof(uint32_t) * 2);
    values[0] = LE32(values[0]);
    values[1] = LE32(values[1]);

    /* Make sure we have enough file... */
    if(values[1] + sizeof(pmt_unit_v2_t) * values[0] > sz) {
        ERR_LOG("ItemPMT.prs file for v2 has unit table outside "
              "of file bounds! 请检查文件的有效性!");
        return -2;
    }

    num_units = values[0];
    if(!(units = (pmt_unit_v2_t *)malloc(sizeof(pmt_unit_v2_t) * values[0]))) {
        ERR_LOG("Cannot allocate space for v2 units: %s",
              strerror(errno));
        num_units = 0;
        return -3;
    }

    memcpy(units, pmt + values[1], sizeof(pmt_unit_v2_t) * values[0]);

    for(i = 0; i < values[0]; ++i) {
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
        units[i].index = LE32(units[i].index);
        units[i].stat = LE16(units[i].stat);
        units[i].amount = LE16(units[i].amount);
#endif

        if(units[i].index < unit_lowest)
            unit_lowest = units[i].index;
    }

    return 0;
}

static int read_units_gc(const uint8_t *pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    uint32_t values[2], i;

    /* Make sure the 指针无效 are sane... */
    if(ptrs->ptr[2] > sz) {
        ERR_LOG("ItemPMT.prs file for GC 的 unit 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(values, pmt + ptrs->ptr[2], sizeof(uint32_t) * 2);
    values[0] = ntohl(values[0]);
    values[1] = ntohl(values[1]);

    /* Make sure we have enough file... */
    if(values[1] + sizeof(pmt_unit_gc_t) * values[0] > sz) {
        ERR_LOG("ItemPMT.prs file for GC has unit table outside "
              "of file bounds! 请检查文件的有效性!");
        return -2;
    }

    num_units_gc = values[0];
    if(!(units_gc = (pmt_unit_gc_t *)malloc(sizeof(pmt_unit_gc_t) *
                                            values[0]))) {
        ERR_LOG("Cannot allocate space for GC units: %s",
              strerror(errno));
        num_units_gc = 0;
        return -3;
    }

    memcpy(units_gc, pmt + values[1], sizeof(pmt_unit_gc_t) * values[0]);

    for(i = 0; i < values[0]; ++i) {
#if !defined(__BIG_ENDIAN__) && !defined(WORDS_BIGENDIAN)
        units_gc[i].index = ntohl(units_gc[i].index);
        units_gc[i].model = ntohs(units_gc[i].model);
        units_gc[i].skin = ntohs(units_gc[i].skin);
        units_gc[i].stat = ntohs(units_gc[i].stat);
        units_gc[i].amount = ntohs(units_gc[i].amount);
#endif

        if(units_gc[i].index < unit_lowest_gc)
            unit_lowest_gc = units_gc[i].index;
    }

    return 0;
}

static int read_units_bb(const uint8_t *pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    uint32_t values[2], i;

    /* Make sure the 指针无效 are sane... */
    if(ptrs->unit_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB 的 unit 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(values, pmt + ptrs->unit_table, sizeof(uint32_t) * 2);
    values[0] = LE32(values[0]);
    values[1] = LE32(values[1]);

    /* Make sure we have enough file... */
    if(values[1] + sizeof(pmt_unit_bb_t) * values[0] > sz) {
        ERR_LOG("ItemPMT.prs file for BB has unit table outside "
              "of file bounds! 请检查文件的有效性!");
        return -2;
    }

    num_units_bb = values[0];
    if(!(units_bb = (pmt_unit_bb_t *)malloc(sizeof(pmt_unit_bb_t) *
                                            values[0]))) {
        ERR_LOG("Cannot allocate space for BB units: %s",
              strerror(errno));
        num_units_bb = 0;
        return -3;
    }

    memcpy(units_bb, pmt + values[1], sizeof(pmt_unit_bb_t) * values[0]);

    for(i = 0; i < values[0]; ++i) {
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
        units_bb[i].index = LE32(units_bb[i].index);
        units_bb[i].model = LE16(units_bb[i].model);
        units_bb[i].skin = LE16(units_bb[i].skin);
        units_bb[i].team_points = LE16(units_bb[i].team_points);
        units_bb[i].stat = LE16(units_bb[i].stat);
        units_bb[i].amount = LE16(units_bb[i].amount);
#endif

        if(units_bb[i].index < unit_lowest_bb)
            unit_lowest_bb = units_bb[i].index;
    }

    return 0;
}

static int read_stars_v2(const uint8_t *pmt, uint32_t sz,
                         const uint32_t ptrs[21]) {
    /* Make sure the 指针无效 are sane... */
    if(ptrs[12] > sz || ptrs[13] > sz || ptrs[13] < ptrs[12]) {
        ERR_LOG("ItemPMT.prs file for v2 的 star 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* Save how big it is, allocate the space, and copy it in */
    star_max = ptrs[13] - ptrs[12];

    if(star_max < unit_lowest + num_units - weapon_lowest) {
        ERR_LOG("Star table doesn't have enough entries!\n"
              "Expected at least %u, got %u",
              unit_lowest + num_units - weapon_lowest, star_max);
        star_max = 0;
        return -2;
    }

    if(!(star_table = (uint8_t *)malloc(star_max))) {
        ERR_LOG("Cannot allocate star table: %s", strerror(errno));
        star_max = 0;
        return -3;
    }

    memcpy(star_table, pmt + ptrs[12], star_max);
    return 0;
}

static int read_stars_gc(const uint8_t *pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    /* Make sure the 指针无效 are sane... */
    if(ptrs->ptr[11] > sz || ptrs->ptr[12] > sz || ptrs->ptr[12] < ptrs->ptr[11]) {
        ERR_LOG("ItemPMT.prs file for GC 的 star 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* Save how big it is, allocate the space, and copy it in */
    star_max_gc = ptrs->ptr[12] - ptrs->ptr[11];

    if(star_max_gc < unit_lowest_gc + num_units - weapon_lowest_gc) {
        ERR_LOG("Star table doesn't have enough entries!\n"
              "Expected at least %u, got %u",
              unit_lowest_gc + num_units_gc - weapon_lowest_gc, star_max_gc);
        star_max_gc = 0;
        return -2;
    }

    if(!(star_table_gc = (uint8_t *)malloc(star_max_gc))) {
        ERR_LOG("Cannot allocate star table: %s", strerror(errno));
        star_max_gc = 0;
        return -3;
    }

    memcpy(star_table_gc, pmt + ptrs->ptr[11], star_max_gc);
    return 0;
}

static int read_stars_bb(const uint8_t *pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    /* Make sure the 指针无效 are sane... */
    if(ptrs->star_value_table > sz || ptrs->special_data_table > sz || ptrs->special_data_table < ptrs->star_value_table) {
        ERR_LOG("ItemPMT.prs file for BB 的 star 指针无效. "
              "请检查其有效性!");
        return -1;
    }

    /* Save how big it is, allocate the space, and copy it in */
    star_max_bb = ptrs->special_data_table - ptrs->star_value_table;

    if(star_max_bb < unit_lowest_bb + num_units - weapon_lowest_bb) {
        ERR_LOG("Star table doesn't have enough entries!\n"
              "Expected at least %u, got %u",
              unit_lowest_bb + num_units_bb - weapon_lowest_bb, star_max_bb);
        star_max_bb = 0;
        return -2;
    }

    if(!(star_table_bb = (uint8_t *)malloc(star_max_bb))) {
        ERR_LOG("Cannot allocate star table: %s", strerror(errno));
        star_max_bb = 0;
        return -3;
    }

    memcpy(star_table_bb, pmt + ptrs->star_value_table, star_max_bb);
    return 0;
}

static int read_mags_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    uint32_t values[2], i;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->mag_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB 的 mag 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(values, pmt + ptrs->mag_table, sizeof(uint32_t) * 2);
    values[0] = LE32(values[0]);
    values[1] = LE32(values[1]);

    /* Make sure we have enough file... */
    if (values[1] + sizeof(pmt_mag_bb_t) * values[0] > sz) {
        ERR_LOG("ItemPMT.prs file for BB has mag table outside "
            "of file bounds! 请检查文件的有效性!");
        return -2;
    }

    num_mags_bb = values[0];
    if (!(mags_bb = (pmt_mag_bb_t*)malloc(sizeof(pmt_mag_bb_t) *
        values[0]))) {
        ERR_LOG("Cannot allocate space for BB mags: %s",
            strerror(errno));
        num_mags_bb = 0;
        return -3;
    }

    memcpy(mags_bb, pmt + values[1], sizeof(pmt_mag_bb_t) * values[0]);

    //DBG_LOG("num_mag_types_bb %d", num_mags_bb);

    for (i = 0; i < values[0]; ++i) {
        //DBG_LOG("index %d", mags_bb[i].index);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
        mags_bb[i].index = LE32(mags_bb[i].index);
        mags_bb[i].model = LE16(mags_bb[i].model);
        mags_bb[i].skin = LE16(mags_bb[i].skin);
        mags_bb[i].team_points = LE32(mags_bb[i].team_points);
        mags_bb[i].feed_table = LE16(mags_bb[i].feed_table);
#endif

        if (mags_bb[i].index < mag_lowest_bb)
            mag_lowest_bb = mags_bb[i].index;
    }

    return 0;
}

static int read_tools_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->tool_table > sz || ptrs->tool_table > ptrs->weapon_table) {
        ERR_LOG("ItemPMT.prs file for BB 的 tool 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* 算出我们有多少张表... */
    num_tool_types_bb = cnt = (ptrs->weapon_table - ptrs->tool_table) / 8;

    //DBG_LOG("num_tool_types_bb %d", num_tool_types_bb);

    /* Allocate the stuff we need to allocate... */
    if (!(num_tools_bb = (uint32_t*)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("Cannot allocate space for BB tool count: %s",
            strerror(errno));
        num_tool_types_bb = 0;
        return -2;
    }

    if (!(tools_bb = (pmt_tool_bb_t**)malloc(sizeof(pmt_tool_bb_t*) *
        cnt))) {
        ERR_LOG("Cannot allocate space for BB tool list: %s",
            strerror(errno));
        free_safe(num_tools_bb);
        num_tools_bb = NULL;
        num_tool_types_bb = 0;
        return -3;
    }

    memset(tools_bb, 0, sizeof(pmt_tool_bb_t*) * cnt);

    /* Read in each table... */
    for (i = 0; i < cnt; ++i) {
        /* Read the pointer and the size... */
        memcpy(values, pmt + ptrs->tool_table + (i << 3), sizeof(uint32_t) * 2);
        values[0] = LE32(values[0]);
        values[1] = LE32(values[1]);

        /* Make sure we have enough file... */
        if (values[1] + sizeof(pmt_tool_bb_t) * values[0] > sz) {
            ERR_LOG("ItemPMT.prs file for BB has weapon table outside "
                "of file bounds! 请检查文件的有效性!");
            return -4;
        }

        num_tools_bb[i] = values[0];

        //DBG_LOG("i %d num_tools_bb %d", i, num_tools_bb[i]);

        if (!(tools_bb[i] = (pmt_tool_bb_t*)malloc(sizeof(pmt_tool_bb_t) *
            values[0]))) {
            ERR_LOG("Cannot allocate space for BB weapons: %s",
                strerror(errno));
            return -5;
        }

        memcpy(tools_bb[i], pmt + values[1],
            sizeof(pmt_tool_bb_t) * values[0]);

        for (j = 0; j < values[0]; ++j) {
            //DBG_LOG("index %d", tools_bb[i][j].cost);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
            tools_bb[i][j].index = LE32(tools_bb[i][j].index);
            tools_bb[i][j].model = LE16(tools_bb[i][j].model);
            tools_bb[i][j].skin = LE16(tools_bb[i][j].skin);
            tools_bb[i][j].team_points = LE32(tools_bb[i][j].team_points);
            tools_bb[i][j].amount = LE16(tools_bb[i][j].amount);
            tools_bb[i][j].tech = LE16(tools_bb[i][j].tech);
            tools_bb[i][j].cost = LE32(tools_bb[i][j].cost);
#endif

            if (tools_bb[i][j].index < tool_lowest_bb)
                tool_lowest_bb = tools_bb[i][j].index;
        }
    }

    return 0;
}

static int read_itemcombinations_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    uint32_t values[2];

    /* Make sure the 指针无效 are sane... */
    if (ptrs->combination_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB 的 itemcombination 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(values, pmt + ptrs->combination_table, sizeof(uint32_t) * 2);
    values[0] = LE32(values[0]);
    values[1] = LE32(values[1]);

    /* Make sure we have enough file... */
    if (values[1] + sizeof(pmt_itemcombination_bb_t) * values[0] > sz) {
        ERR_LOG("ItemPMT.prs file for BB has itemcombination table outside "
            "of file bounds! 请检查文件的有效性!");
        return -2;
    }

    itemcombinations_max_bb = values[0];
    if (!(itemcombination_bb = (pmt_itemcombination_bb_t*)malloc(sizeof(pmt_itemcombination_bb_t) *
        values[0]))) {
        ERR_LOG("Cannot allocate space for BB itemcombination: %s",
            strerror(errno));
        itemcombinations_max_bb = 0;
        return -3;
    }

    memcpy(itemcombination_bb, pmt + values[1], sizeof(pmt_itemcombination_bb_t) * values[0]);

#ifdef DEBUG
    for (int i = 0; i < itemcombinations_max_bb; ++i) {

        DBG_LOG("i %d used_item 0x%02X", i, itemcombination_bb[i].used_item[0]);
        DBG_LOG("i %d used_item 0x%02X", i, itemcombination_bb[i].used_item[1]);
        DBG_LOG("i %d used_item 0x%02X", i, itemcombination_bb[i].used_item[2]);
        DBG_LOG("i %d equipped_item 0x%02X", i, itemcombination_bb[i].equipped_item[0]);
        DBG_LOG("i %d equipped_item 0x%02X", i, itemcombination_bb[i].equipped_item[1]);
        DBG_LOG("i %d equipped_item 0x%02X", i, itemcombination_bb[i].equipped_item[2]);
        DBG_LOG("i %d result_item 0x%02X", i, itemcombination_bb[i].result_item[0]);
        DBG_LOG("i %d result_item 0x%02X", i, itemcombination_bb[i].result_item[1]);
        DBG_LOG("i %d result_item 0x%02X", i, itemcombination_bb[i].result_item[2]);
        getchar();


#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }

#endif // DEBUG

    return 0;
}

static int read_eventitems_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    uint32_t values[2], i, j;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->unwrap_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB 的 unit 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(values, pmt + ptrs->unwrap_table, sizeof(uint32_t) * 2);
    values[0] = LE32(values[0]);
    values[1] = LE32(values[1]);

    /* Make sure we have enough file... */
    if (values[1] + sizeof(pmt_eventitem_bb_t) * values[0] > sz) {
        ERR_LOG("ItemPMT.prs file for BB has unit table outside "
            "of file bounds! 请检查文件的有效性!");
        return -2;
    }

    num_eventitem_list_bb = values[0];
    if (!(eventitem_bb = (pmt_eventitem_bb_t**)malloc(sizeof(pmt_eventitem_bb_t) *
        values[0]))) {
        ERR_LOG("Cannot allocate space for BB units: %s",
            strerror(errno));
        num_eventitem_list_bb = 0;
        return -3;
    }

    DBG_LOG("num_eventitem_list_bb %d", num_eventitem_list_bb);
//
//    /* Read in each table... */
//    for (i = 0; i < num_eventitem_list_bb; ++i) {
//        /* Read the pointer and the size... */
//        memcpy(values, pmt + ptrs->unwrap_table + (i << 3), sizeof(uint32_t) * 2);
//        values[0] = LE32(values[0]);
//        values[1] = LE32(values[1]);
//
//        /* Make sure we have enough file... */
//        if (values[1] + sizeof(pmt_eventitem_bb_t) * values[0] > sz) {
//            ERR_LOG("ItemPMT.prs file for BB has weapon table outside "
//                "of file bounds! 请检查文件的有效性!");
//            return -4;
//        }
//
//        num_eventitems_bb[i] = values[0];
//
//        //DBG_LOG("i %d num_eventitems_bb %d", i, num_eventitems_bb[i]);
//
//        if (!(eventitem_bb[i] = (pmt_eventitem_bb_t*)malloc(sizeof(pmt_eventitem_bb_t) *
//            values[0]))) {
//            ERR_LOG("Cannot allocate space for BB weapons: %s",
//                strerror(errno));
//            return -5;
//        }
//
//        memcpy(eventitem_bb[i], pmt + values[1],
//            sizeof(pmt_eventitem_bb_t) * values[0]);
//
//        for (j = 0; j < num_eventitem_list_bb; ++i) {
//
//            DBG_LOG("i %d j %d item 0x%02X", i, j, eventitem_bb[i][j].item[0]);
//            DBG_LOG("i %d j %d item 0x%02X", i, j, eventitem_bb[i][j].item[1]);
//            DBG_LOG("i %d j %d item 0x%02X", i, j, eventitem_bb[i][j].item[2]);
//
//
//#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
//#endif
//        }
//    }
//
//    getchar();
    return 0;
}

//static int read_eventitems_bb2(const uint8_t* pmt, uint32_t sz,
//                         const pmt_table_offsets_v3_t* ptrs) {
//    uint32_t cnt, i, values[2], j;
//
//    /* Make sure the 指针无效 are sane... */
//    if (ptrs->unwrap_table > sz || ptrs->unwrap_table > ptrs->ranged_special_table) {
//        ERR_LOG("ItemPMT.prs file for BB 的 eventitem 指针无效. "
//            "请检查其有效性!");
//        return -1;
//    }
//
//    /* 算出我们有多少张表... */
//    num_eventitem_list_bb = cnt = 3;
//
//    DBG_LOG("num_eventitem_list_bb %d", num_eventitem_list_bb);
//
//    //getchar();
//
//    /* Allocate the stuff we need to allocate... */
//    if (!(num_eventitems_bb = (uint32_t*)malloc(sizeof(uint32_t) * cnt))) {
//        ERR_LOG("Cannot allocate space for BB eventitem count: %s",
//            strerror(errno));
//        num_eventitem_list_bb = 0;
//        return -2;
//    }
//
//    if (!(eventitem_bb = (pmt_eventitem_bb_t**)malloc(sizeof(pmt_eventitem_bb_t*) *
//        cnt))) {
//        ERR_LOG("Cannot allocate space for BB eventitem list: %s",
//            strerror(errno));
//        free_safe(num_eventitems_bb);
//        num_eventitems_bb = NULL;
//        num_eventitem_list_bb = 0;
//        return -3;
//    }
//
//    memset(eventitem_bb, 0, sizeof(pmt_eventitem_bb_t*) * cnt);
//
//    /* Read in each table... */
//    for (i = 0; i < cnt; ++i) {
//        /* Read the pointer and the size... */
//        memcpy(values, pmt + ptrs->unwrap_table + (i << 3), sizeof(uint32_t) * 2);
//        values[0] = LE32(values[0]);
//        values[1] = LE32(values[1]);
//
//        /* Make sure we have enough file... */
//        if (values[1] + sizeof(pmt_eventitem_bb_t) * values[0] > sz) {
//            ERR_LOG("ItemPMT.prs file for BB has weapon table outside "
//                "of file bounds! 请检查文件的有效性!");
//            return -4;
//        }
//
//        num_eventitems_bb[i] = values[0];
//
//        //DBG_LOG("i %d num_eventitems_bb %d", i, num_eventitems_bb[i]);
//
//        if (!(eventitem_bb[i] = (pmt_eventitem_bb_t*)malloc(sizeof(pmt_eventitem_bb_t) *
//            values[0]))) {
//            ERR_LOG("Cannot allocate space for BB weapons: %s",
//                strerror(errno));
//            return -5;
//        }
//
//        memcpy(eventitem_bb[i], pmt + values[1],
//            sizeof(pmt_eventitem_bb_t) * values[0]);
//
//        for (j = 0; j < num_eventitem_list_bb; ++i) {
//
//            DBG_LOG("i %d j %d item 0x%02X", i, j, eventitem_bb[i][j].item[0]);
//            DBG_LOG("i %d j %d item 0x%02X", i, j, eventitem_bb[i][j].item[1]);
//            DBG_LOG("i %d j %d item 0x%02X", i, j, eventitem_bb[i][j].item[2]);
//
//
//#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
//#endif
//        }
//    }
//
//    getchar();
//    return 0;
//}

static int build_units_v2(int norestrict) {
    uint32_t i, j, k;
    uint8_t star;
    pmt_unit_v2_t *unit;
    int16_t pm;
    void *tmp;

    /* Figure out what the max number of stars for a unit is. */
    for(i = 0; i < num_units; ++i) {
        star = star_table[i + unit_lowest - weapon_lowest];
        if(star > unit_max_stars)
            unit_max_stars = star;
    }

    /* Always go one beyond, since we may have to deal with + and ++ on the last
       possible unit. */
    ++unit_max_stars;

    /* For now, punt and allocate space for every theoretically possible unit,
       even though some are actually disabled by the game. */
    if(!(units_by_stars = (uint64_t *)malloc((num_units * 5 + 1) *
                                             sizeof(uint64_t)))) {
        ERR_LOG("Cannot allocate unit table: %s", strerror(errno));
        return -1;
    }

    /* Allocate space for the "pointer" table */
    if(!(units_with_stars = (uint32_t *)malloc((unit_max_stars + 1) *
                                               sizeof(uint32_t)))) {
        ERR_LOG("Cannot allocate unit ptr table: %s",
              strerror(errno));
        free_safe(units_by_stars);
        units_by_stars = NULL;
        return -2;
    }

    /* Fill in the game's failsafe of a plain Knight/Power... */
    units_by_stars[0] = (uint64_t)Item_Knight_Power;
    units_with_stars[0] = 1;
    k = 1;

    /* Go through and fill in our tables... */
    for(i = 0; i <= unit_max_stars; ++i) {
        for(j = 0; j < num_units; ++j) {
            unit = units + j;
            star = star_table[j + unit_lowest - weapon_lowest];

            if(star - 1 == i && unit->pm_range &&
               (unit->stat <= 3 || norestrict)) {
                pm = -2 * unit->pm_range;
                units_by_stars[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
                pm = -1 * unit->pm_range;
                units_by_stars[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
            }
            else if(star == i) {
                units_by_stars[k++] = 0x00000301 | (j << 16);
            }
            else if(star + 1 == i && unit->pm_range &&
                    (unit->stat <= 3 || norestrict)) {
                pm = 2 * unit->pm_range;
                units_by_stars[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
                pm = unit->pm_range;
                units_by_stars[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
            }
        }

        units_with_stars[i] = k;
    }

    if((tmp = realloc(units_by_stars, k * sizeof(uint64_t)))) {
        units_by_stars = (uint64_t *)tmp;
    }
    else {
        SHIPS_LOG("Cannot resize units_by_stars table: %s",
              strerror(errno));
    }

    return 0;
}

static int build_units_gc(int norestrict) {
    uint32_t i, j, k;
    uint8_t star;
    pmt_unit_gc_t *unit;
    int16_t pm;
    void *tmp;

    /* Figure out what the max number of stars for a unit is. */
    for(i = 0; i < num_units_gc; ++i) {
        star = star_table_gc[i + unit_lowest_gc - weapon_lowest_gc];
        if(star > unit_max_stars_gc)
            unit_max_stars_gc = star;
    }

    /* Always go one beyond, since we may have to deal with + and ++ on the last
       possible unit. */
    ++unit_max_stars_gc;

    /* For now, punt and allocate space for every theoretically possible unit,
       even though some are actually disabled by the game. */
    if(!(units_by_stars_gc = (uint64_t *)malloc((num_units_gc * 5 + 1) *
                                                sizeof(uint64_t)))) {
        ERR_LOG("Cannot allocate unit table: %s", strerror(errno));
        return -1;
    }

    /* Allocate space for the "pointer" table */
    if(!(units_with_stars_gc = (uint32_t *)malloc((unit_max_stars_gc + 1) *
                                                  sizeof(uint32_t)))) {
        ERR_LOG("Cannot allocate unit ptr table: %s",
              strerror(errno));
        free_safe(units_by_stars_gc);
        units_by_stars_gc = NULL;
        return -2;
    }

    /* Fill in the game's failsafe of a plain Knight/Power... */
    units_by_stars_gc[0] = (uint64_t)Item_Knight_Power;
    units_with_stars_gc[0] = 1;
    k = 1;

    /* Go through and fill in our tables... */
    for(i = 0; i <= unit_max_stars_gc; ++i) {
        for(j = 0; j < num_units_gc; ++j) {
            unit = units_gc + j;
            star = star_table_gc[j + unit_lowest_gc - weapon_lowest_gc];

            if(star - 1 == i && unit->pm_range &&
               (unit->stat <= 3 || norestrict)) {
                pm = -2 * unit->pm_range;
                units_by_stars_gc[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
                pm = -1 * unit->pm_range;
                units_by_stars_gc[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
            }
            else if(star == i) {
                units_by_stars_gc[k++] = 0x00000301 | (j << 16);
            }
            else if(star + 1 == i && unit->pm_range &&
                    (unit->stat <= 3 || norestrict)) {
                pm = 2 * unit->pm_range;
                units_by_stars_gc[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
                pm = unit->pm_range;
                units_by_stars_gc[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
            }
        }

        units_with_stars_gc[i] = k;
    }

    if((tmp = realloc(units_by_stars_gc, k * sizeof(uint64_t)))) {
        units_by_stars_gc = (uint64_t *)tmp;
    }
    else {
        SHIPS_LOG("Cannot resize units_by_stars_gc table: %s",
              strerror(errno));
    }

    return 0;
}

static int build_units_bb(int norestrict) {
    uint32_t i, j, k;
    uint8_t star;
    pmt_unit_bb_t *unit;
    int16_t pm;
    void *tmp;

    /* Figure out what the max number of stars for a unit is. */
    for(i = 0; i < num_units_bb; ++i) {
        star = star_table_bb[i + unit_lowest_bb - weapon_lowest_bb];
        if(star > unit_max_stars_bb)
            unit_max_stars_bb = star;
    }

    /* Always go one beyond, since we may have to deal with + and ++ on the last
       possible unit. */
    ++unit_max_stars_bb;

    /* For now, punt and allocate space for every theoretically possible unit,
       even though some are actually disabled by the game. */
    if(!(units_by_stars_bb = (uint64_t *)malloc((num_units_bb * 5 + 1) *
                                                sizeof(uint64_t)))) {
        ERR_LOG("无法分配 unit 表格内存空间: %s", strerror(errno));
        return -1;
    }

    /* Allocate space for the "pointer" table */
    if(!(units_with_stars_bb = (uint32_t *)malloc((unit_max_stars_bb + 1) *
                                                  sizeof(uint32_t)))) {
        ERR_LOG("无法分配 unit ptr 表格内存空间: %s",
              strerror(errno));
        free_safe(units_by_stars_bb);
        units_by_stars_bb = NULL;
        return -2;
    }

    /* Fill in the game's failsafe of a plain Knight/Power... */
    units_by_stars_bb[0] = (uint64_t)Item_Knight_Power;
    units_with_stars_bb[0] = 1;
    k = 1;

    /* Go through and fill in our tables... */
    for(i = 0; i <= unit_max_stars_bb; ++i) {
        for(j = 0; j < num_units_bb; ++j) {
            unit = units_bb + j;
            star = star_table_bb[j + unit_lowest_bb - weapon_lowest_bb];

            if(star - 1 == i && unit->pm_range &&
               (unit->stat <= 3 || norestrict)) {
                pm = -2 * unit->pm_range;
                units_by_stars_bb[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
                pm = -1 * unit->pm_range;
                units_by_stars_bb[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
            }
            else if(star == i) {
                units_by_stars_bb[k++] = 0x00000301 | (j << 16);
            }
            else if(star + 1 == i && unit->pm_range &&
                    (unit->stat <= 3 || norestrict)) {
                pm = 2 * unit->pm_range;
                units_by_stars_bb[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
                pm = unit->pm_range;
                units_by_stars_bb[k++] = 0x00000301 | (j << 16) |
                    (((uint64_t)pm) << 48);
            }
        }

        units_with_stars_bb[i] = k;
    }

    if((tmp = realloc(units_by_stars_bb, k * sizeof(uint64_t)))) {
        units_by_stars_bb = (uint64_t *)tmp;
    }
    else {
        SHIPS_LOG("无法重写 units_by_stars_bb 表格大小: %s",
              strerror(errno));
    }

    return 0;
}

int pmt_read_v2(const char *fn, int norestrict) {
    int ucsz;
    uint8_t *ucbuf;
    uint32_t ptrs[21];

    /* Read in the file and decompress it. */
    if((ucsz = pso_prs_decompress_file(fn, &ucbuf)) < 0) {
        ERR_LOG("无法读取 v2 PMT %s: %s", fn, strerror(-ucsz));
        return -1;
    }

    /* Read in the 指针无效 table. */
    if(read_v2ptr_tbl(ucbuf, ucsz, ptrs)) {
        free_safe(ucbuf);
        return -9;
    }

    /* Let's start with weapons... */
    if(read_weapons_v2(ucbuf, ucsz, ptrs)) {
        free_safe(ucbuf);
        return -10;
    }

    /* Grab the guards... */
    if(read_guards_v2(ucbuf, ucsz, ptrs)) {
        free_safe(ucbuf);
        return -11;
    }

    /* Next, read in the units... */
    if(read_units_v2(ucbuf, ucsz, ptrs)) {
        free_safe(ucbuf);
        return -12;
    }

    /* Read in the star values... */
    if(read_stars_v2(ucbuf, ucsz, ptrs)) {
        free_safe(ucbuf);
        return -13;
    }

    /* We're done with the raw PMT data now, clean it up. */
    free_safe(ucbuf);

    /* Make the tables for generating random units */
    if(build_units_v2(norestrict)) {
        return -14;
    }

    have_v2_pmt = 1;

    return 0;
}

int pmt_read_gc(const char *fn, int norestrict) {
    int ucsz;
    uint8_t *ucbuf;
    //uint32_t ptrs[23];

    /* Read in the file and decompress it. */
    if((ucsz = pso_prs_decompress_file(fn, &ucbuf)) < 0) {
        ERR_LOG("无法读取 GameCube PMT %s: %s", fn, strerror(-ucsz));
        return -1;
    }

    if (!(pmt_tb_offsets_gc = (pmt_table_offsets_v3_t*)malloc(sizeof(pmt_table_offsets_v3_t)))) {
        ERR_LOG("Cannot allocate space for BB PMT offsets: %s",
            strerror(errno));
        free_safe(pmt_tb_offsets_gc);
        return -2;
    }

    /* Read in the 指针无效 table. */
    if(read_gcptr_tbl(ucbuf, ucsz, pmt_tb_offsets_gc)) {
        free_safe(ucbuf);
        return -9;
    }

    /* Let's start with weapons... */
    if(read_weapons_gc(ucbuf, ucsz, pmt_tb_offsets_gc)) {
        free_safe(ucbuf);
        return -10;
    }

    /* Grab the guards... */
    if(read_guards_gc(ucbuf, ucsz, pmt_tb_offsets_gc)) {
        free_safe(ucbuf);
        return -11;
    }

    /* Next, read in the units... */
    if(read_units_gc(ucbuf, ucsz, pmt_tb_offsets_gc)) {
        free_safe(ucbuf);
        return -12;
    }

    /* Read in the star values... */
    if(read_stars_gc(ucbuf, ucsz, pmt_tb_offsets_gc)) {
        free_safe(ucbuf);
        return -13;
    }

    /* We're done with the raw PMT data now, clean it up. */
    free_safe(ucbuf);

    /* Make the tables for generating random units */
    if(build_units_gc(norestrict)) {
        return -14;
    }

    have_gc_pmt = 1;

    return 0;
}

int pmt_read_bb(const char *fn, int norestrict) {
    int ucsz;
    uint8_t *ucbuf;

    /* Read in the file and decompress it. */
    if((ucsz = pso_prs_decompress_file(fn, &ucbuf)) < 0) {
        ERR_LOG("无法读取 Blue Burst PMT %s: %s", fn, strerror(-ucsz));
        return -1;
    }

    //display_packet(ucbuf, sizeof(pmt_table_offsets_v3_t));
    //getchar();

    if (!(pmt_tb_offsets_bb = (pmt_table_offsets_v3_t*)malloc(sizeof(pmt_table_offsets_v3_t)))) {
        ERR_LOG("Cannot allocate space for BB PMT offsets: %s",
            strerror(errno));
        free_safe(pmt_tb_offsets_bb);
        return -2;
    }

    /* Read in the 指针无效 table. */
    if(read_bbptr_tbl(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -9;
    }

    /* Let's start with weapons... */
    if(read_weapons_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -10;
    }

    /* Grab the guards... */
    if(read_guards_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -11;
    }

    /* Next, read in the units... */
    if(read_units_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -12;
    }

    /* Read in the star values... */
    if(read_stars_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -13;
    }

    /* Read in the mags values... */
    if (read_mags_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -14;
    }

    /* Read in the tools values... */
    if (read_tools_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -15;
    }

    /* Read in the itemcombinations values... */
    if (read_itemcombinations_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -16;
    }

    /* Read in the itemcombinations values... */
    if (read_eventitems_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -17;
    }

    /* We're done with the raw PMT data now, clean it up. */
    free_safe(ucbuf);

    /* Make the tables for generating random units */
    if(build_units_bb(norestrict)) {
        return -23;
    }

    have_bb_pmt = 1;

    return 0;
}

int pmt_enabled_v2(void) {
    return have_v2_pmt;
}

int pmt_enabled_gc(void) {
    return have_gc_pmt;
}

int pmt_enabled_bb(void) {
    return have_bb_pmt;
}

void pmt_cleanup_v2(void) {
    uint32_t i;

    for (i = 0; i < num_weapon_types; ++i) {
        free_safe(weapons[i]);
    }
    free_safe(weapons);
    free_safe(num_weapons);
    weapons = NULL;
    num_weapons = NULL;
    num_weapon_types = 0;
    weapon_lowest = EMPTY_STRING;

    for (i = 0; i < num_guard_types; ++i) {
        free_safe(guards[i]);
    }
    free_safe(guards);
    free_safe(num_guards);
    guards = NULL;
    num_guards = NULL;
    num_guard_types = 0;
    guard_lowest = EMPTY_STRING;

    free_safe(units);
    num_units = 0;
    units = NULL;
    unit_lowest = EMPTY_STRING;

    free_safe(star_table);
    star_table = NULL;
    star_max = 0;

    free_safe(units_by_stars);
    free_safe(units_with_stars);
    units_by_stars = NULL;
    units_with_stars = NULL;
    unit_max_stars = 0;

    have_v2_pmt = 0;

}

void pmt_cleanup_gc(void) {
    uint32_t i;

    free_safe(pmt_tb_offsets_gc);
    pmt_tb_offsets_gc = NULL;

    for (i = 0; i < num_weapon_types_gc; ++i) {
        free_safe(weapons_gc[i]);
    }
    free_safe(weapons_gc);
    free_safe(num_weapons_gc);
    weapons_gc = NULL;
    num_weapons_gc = NULL;
    num_weapon_types_gc = 0;
    weapon_lowest_gc = EMPTY_STRING;

    for (i = 0; i < num_guard_types_gc; ++i) {
        free_safe(guards_gc[i]);
    }
    free_safe(guards_gc);
    free_safe(num_guards_gc);
    guards_gc = NULL;
    num_guards_gc = NULL;
    num_guard_types_gc = 0;
    guard_lowest_gc = EMPTY_STRING;

    free_safe(units_gc);
    units_gc = NULL;
    num_units_gc = 0;
    unit_lowest_gc = EMPTY_STRING;

    free_safe(star_table_gc);
    star_table_gc = NULL;
    star_max_gc = 0;

    free_safe(units_by_stars_gc);
    free_safe(units_with_stars_gc);
    units_by_stars_gc = NULL;
    units_with_stars_gc = NULL;
    unit_max_stars_gc = 0;

    have_gc_pmt = 0;
}

void pmt_cleanup_bb(void) {
    uint32_t i;

    free_safe(pmt_tb_offsets_bb);
    pmt_tb_offsets_bb = NULL;

    for (i = 0; i < num_weapon_types_bb; ++i) {
        free_safe(weapons_bb[i]);
    }
    free_safe(weapons_bb);
    free_safe(num_weapons_bb);
    weapons_bb = NULL;
    num_weapons_bb = NULL;
    num_weapon_types_bb = 0;
    weapon_lowest_bb = EMPTY_STRING;

    for (i = 0; i < num_guard_types_bb; ++i) {
        free_safe(guards_bb[i]);
    }
    free_safe(guards_bb);
    free_safe(num_guards_bb);
    guards_bb = NULL;
    num_guards_bb = NULL;
    num_guard_types_bb = 0;
    guard_lowest_bb = EMPTY_STRING;

    free_safe(units_bb);
    units_bb = NULL;
    num_units_bb = 0;
    unit_lowest_bb = EMPTY_STRING;

    free_safe(star_table_bb);
    star_table_bb = NULL;
    star_max_bb = 0;

    free_safe(units_by_stars_bb);
    free_safe(units_with_stars_bb);
    units_by_stars_bb = NULL;
    units_with_stars_bb = NULL;
    unit_max_stars_bb = 0;

    free_safe(mags_bb);
    mags_bb = NULL;
    num_mags_bb = 0;
    mag_lowest_bb = EMPTY_STRING;

    for (i = 0; i < num_tool_types_bb; ++i) {
        free_safe(tools_bb[i]);
    }
    free_safe(tools_bb);
    free_safe(num_tools_bb);
    tools_bb = NULL;
    num_tools_bb = NULL;
    num_tool_types_bb = 0;
    tool_lowest_bb = EMPTY_STRING;

    free_safe(itemcombination_bb);
    itemcombination_bb = NULL;
    itemcombinations_max_bb = 0;




    have_bb_pmt = 0;
}

void pmt_cleanup(void) {

    /* 清理V2版本的 ItemPMT数据 */
    pmt_cleanup_v2();

    /* 清理GC版本的 ItemPMT数据 */
    pmt_cleanup_gc();

    /* 清理BB版本的 ItemPMT数据 */
    pmt_cleanup_bb();

}

int pmt_lookup_weapon_v2(uint32_t code, pmt_weapon_v2_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_v2_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 weapon */
    if(parts[0] != ITEM_TYPE_WEAPON) {
        return -2;
    }

    /* 确保我们在任何地方都不越界 */
    if(parts[1] > num_weapon_types) {
        return -3;
    }

    if(parts[2] >= num_weapons[parts[1]]) {
        return -4;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &weapons[parts[1]][parts[2]], sizeof(pmt_weapon_v2_t));
    return 0;
}

int pmt_lookup_guard_v2(uint32_t code, pmt_guard_v2_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_v2_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 guard item */
    if(parts[0] != ITEM_TYPE_GUARD) {
        return -2;
    }

    /* Make sure its not a unit */
    if(parts[1] == ITEM_SUBTYPE_UNIT) {
        return -3;
    }

    /* 确保我们在任何地方都不越界 */
    if(parts[1] > num_guard_types) {
        return -4;
    }

    if(parts[2] >= num_guards[parts[1] - 1]) {
        return -5;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &guards[parts[1] - 1][parts[2]], sizeof(pmt_guard_v2_t));
    return 0;
}

int pmt_lookup_unit_v2(uint32_t code, pmt_unit_v2_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_v2_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 unit */
    if(parts[0] != ITEM_TYPE_GUARD || parts[1] != ITEM_SUBTYPE_UNIT) {
        return -2;
    }

    if(parts[2] >= num_units) {
        return -3;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &units[parts[2]], sizeof(pmt_unit_v2_t));
    return 0;
}

uint8_t pmt_lookup_stars_v2(uint32_t code) {
    uint8_t parts[3] = { 0 };
    pmt_weapon_v2_t weap;
    pmt_guard_v2_t guard;
    pmt_unit_v2_t unit;

    /* Make sure we loaded the PMT stuff to start with. */
    if(!have_v2_pmt)
        return (uint8_t)-1;

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    switch(parts[0]) {
        case ITEM_TYPE_WEAPON:                          /* Weapons */
            if(pmt_lookup_weapon_v2(code, &weap))
                return (uint8_t)-1;

            if(weap.index - weapon_lowest > star_max)
                return (uint8_t)-1;

            return star_table[weap.index - weapon_lowest];

        case ITEM_TYPE_GUARD:                           /* Guards */
            switch(parts[1]) {
                case ITEM_SUBTYPE_FRAME:                /* Armors */
                case ITEM_SUBTYPE_BARRIER:              /* Shields */
                    if(pmt_lookup_guard_v2(code, &guard))
                        return (uint8_t)-1;

                    if(guard.index - weapon_lowest > star_max)
                        return (uint8_t)-1;

                    return star_table[guard.index - weapon_lowest];

                case ITEM_SUBTYPE_UNIT:                 /* Units */
                    if(pmt_lookup_unit_v2(code, &unit))
                        return (uint8_t)-1;

                    if(unit.index - weapon_lowest > star_max)
                        return (uint8_t)-1;

                    return star_table[unit.index - weapon_lowest];
            }
    }

    return (uint8_t)-1;
}

int pmt_lookup_weapon_gc(uint32_t code, pmt_weapon_gc_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_gc_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 weapon */
    if(parts[0] != ITEM_TYPE_WEAPON) {
        return -2;
    }

    /* 确保我们在任何地方都不越界 */
    if(parts[1] > num_weapon_types_gc) {
        return -3;
    }

    if(parts[2] >= num_weapons_gc[parts[1]]) {
        return -4;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &weapons_gc[parts[1]][parts[2]], sizeof(pmt_weapon_gc_t));
    return 0;
}

int pmt_lookup_guard_gc(uint32_t code, pmt_guard_gc_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_gc_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 guard item */
    if(parts[0] != ITEM_TYPE_GUARD) {
        return -2;
    }

    /* Make sure its not a unit */
    if(parts[1] == ITEM_SUBTYPE_UNIT) {
        return -3;
    }

    /* 确保我们在任何地方都不越界 */
    if(parts[1] > num_guard_types_gc) {
        return -4;
    }

    if(parts[2] >= num_guards_gc[parts[1] - 1]) {
        return -5;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &guards_gc[parts[1] - 1][parts[2]], sizeof(pmt_guard_gc_t));
    return 0;
}

int pmt_lookup_unit_gc(uint32_t code, pmt_unit_gc_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_gc_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 unit */
    if(parts[0] != ITEM_TYPE_GUARD || parts[1] != ITEM_SUBTYPE_UNIT) {
        return -2;
    }

    if(parts[2] >= num_units_gc) {
        return -3;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &units_gc[parts[2]], sizeof(pmt_unit_gc_t));
    return 0;
}

uint8_t pmt_lookup_stars_gc(uint32_t code) {
    uint8_t parts[3] = { 0 };
    pmt_weapon_gc_t weap;
    pmt_guard_gc_t guard;
    pmt_unit_gc_t unit;

    /* Make sure we loaded the PMT stuff to start with. */
    if(!have_gc_pmt)
        return (uint8_t)-1;

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    switch(parts[0]) {
        case ITEM_TYPE_WEAPON:                      /* Weapons */
            if(pmt_lookup_weapon_gc(code, &weap))
                return (uint8_t)-1;

            if(weap.index - weapon_lowest_gc > star_max_gc)
                return (uint8_t)-1;

            return star_table_gc[weap.index - weapon_lowest_gc];

        case ITEM_TYPE_GUARD:                        /* Guards */
            switch(parts[1]) {
                case ITEM_SUBTYPE_FRAME:             /* Armors */
                case ITEM_SUBTYPE_BARRIER:           /* Shields */
                    if(pmt_lookup_guard_gc(code, &guard))
                        return (uint8_t)-1;

                    if(guard.index - weapon_lowest_gc > star_max_gc)
                        return (uint8_t)-1;

                    return star_table_gc[guard.index - weapon_lowest_gc];

                case ITEM_SUBTYPE_UNIT:              /* Units */
                    if(pmt_lookup_unit_gc(code, &unit))
                        return (uint8_t)-1;

                    if(unit.index - weapon_lowest_gc > star_max_gc)
                        return (uint8_t)-1;

                    return star_table_gc[unit.index - weapon_lowest_gc];
            }
    }

    return (uint8_t)-1;
}

int pmt_lookup_weapon_bb(uint32_t code, pmt_weapon_bb_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 weapon */
    if(parts[0] != ITEM_TYPE_WEAPON) {
        return -2;
    }

    /* 确保我们在任何地方都不越界 */
    if(parts[1] > num_weapon_types_bb) {
        return -3;
    }

    if(parts[2] >= num_weapons_bb[parts[1]]) {
        return -4;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &weapons_bb[parts[1]][parts[2]], sizeof(pmt_weapon_bb_t));
    return 0;
}

int pmt_lookup_guard_bb(uint32_t code, pmt_guard_bb_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 guard item */
    if(parts[0] != ITEM_TYPE_GUARD) {
        return -2;
    }

    /* Make sure its not a unit */
    if(parts[1] == ITEM_SUBTYPE_UNIT) {
        return -3;
    }

    /* 确保我们在任何地方都不越界 */
    if(parts[1] > num_guard_types_bb) {
        return -4;
    }

    if(parts[2] >= num_guards_bb[parts[1] - 1]) {
        return -5;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &guards_bb[parts[1] - 1][parts[2]], sizeof(pmt_guard_bb_t));
    return 0;
}

int pmt_lookup_unit_bb(uint32_t code, pmt_unit_bb_t *rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if(!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 unit */
    if(parts[0] != ITEM_TYPE_GUARD || parts[1] != ITEM_SUBTYPE_UNIT) {
        return -2;
    }

    if(parts[2] >= num_units_bb) {
        return -3;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &units_bb[parts[2]], sizeof(pmt_unit_bb_t));
    return 0;
}

int pmt_lookup_mag_bb(uint32_t code, pmt_mag_bb_t* rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if (!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 guard item */
    if (parts[0] != ITEM_TYPE_MAG) {
        return -2;
    }

    /* 确保我们在任何地方都不越界 */
    if (parts[1] > num_mags_bb) {
        return -3;
    }

    /* 确保我们在任何地方都不越界 */
    if (parts[2] == 0x00) {
        return -4;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &mags_bb[parts[1]], sizeof(pmt_mag_bb_t));
    return 0;
}

int pmt_lookup_tools_bb(uint32_t code, pmt_tool_bb_t* rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if (!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* 确保我们正在查找 weapon */
    if (parts[0] != ITEM_TYPE_TOOL) {
        return -2;
    }

    /* 确保我们在任何地方都不越界 */
    if (parts[1] > num_tool_types_bb) {
        return -3;
    }

    if (parts[2] >= num_tools_bb[parts[1]]) {
        return -4;
    }

    /* 获取数据并将其复制出来 */
    memcpy(rv, &tools_bb[parts[1]][parts[2]], sizeof(pmt_tool_bb_t));
    return 0;
}

int pmt_lookup_itemcombination_bb(uint32_t code, uint32_t equip_code, pmt_itemcombination_bb_t* rv) {
    size_t i = 0;
    uint8_t parts[3] = { 0 }, eparts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if (!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    eparts[0] = (uint8_t)(equip_code & 0xFF);
    eparts[1] = (uint8_t)((equip_code >> 8) & 0xFF);
    eparts[2] = (uint8_t)((equip_code >> 16) & 0xFF);

#ifdef DEBUG
    DBG_LOG("物品1, used_item 0x%08X 0x%02X 0x%02X 0x%02X",
        code,
        parts[0],
        parts[1],
        parts[2]);

    DBG_LOG("物品2, equipped_item 0x%08X 0x%02X 0x%02X 0x%02X",
        equip_code,
        eparts[0],
        eparts[1],
        eparts[2]);
#endif // DEBUG

    ///* 确保我们正在查找 unit */
    //if (parts[0] != ITEM_TYPE_TOOL) {
    //    return -2;
    //}

    ///* 确保我们在任何地方都不越界 */
    //if (parts[1] > num_tool_types_bb) {
    //    return -3;
    //}

    //if (parts[2] >= num_tools_bb[parts[1]]) {
    //    return -4;
    //}

    for (i; i < itemcombinations_max_bb;i++) {
        if (parts[0] == itemcombination_bb[i].used_item[0] && 
            parts[1] == itemcombination_bb[i].used_item[1] &&
            parts[2] == itemcombination_bb[i].used_item[2]) {

            if (eparts[0] != itemcombination_bb[i].equipped_item[0] &&
                eparts[1] != itemcombination_bb[i].equipped_item[1] &&
                eparts[2] != itemcombination_bb[i].equipped_item[2]) {
                continue;
            }

            /* TODO 是否还要进一步的加强设置 或者合成失败？ */
            switch (itemcombination_bb[i].equipped_item[0]) {
            case ITEM_TYPE_WEAPON:
#ifdef DEBUG
                DBG_LOG("ITEM_TYPE_WEAPON");
#endif // DEBUG
                /* 获取数据并将其复制出来 */
                memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                break;

            case ITEM_TYPE_GUARD:
#ifdef DEBUG
                DBG_LOG("ITEM_TYPE_WEAPON");
#endif // DEBUG
                switch (itemcombination_bb[i].equipped_item[1]) {
                case ITEM_SUBTYPE_FRAME:
#ifdef DEBUG
                    DBG_LOG("ITEM_TYPE_WEAPON");
#endif // DEBUG
                    /* 获取数据并将其复制出来 */
                    memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                    break;

                case ITEM_SUBTYPE_BARRIER:
#ifdef DEBUG
                    DBG_LOG("ITEM_TYPE_WEAPON");
#endif // DEBUG
                    /* 获取数据并将其复制出来 */
                    memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                    break;
                }
                break;

            case ITEM_TYPE_MAG:
#ifdef DEBUG
                DBG_LOG("ITEM_TYPE_MAG");
#endif // DEBUG
                /* 获取数据并将其复制出来 */
                memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                break;
            }

            break;
        }
    }

    if (i >= itemcombinations_max_bb) {
#ifdef DEBUG
        ERR_LOG("结果有误 result_item 0x%02X 0x%02X 0x%02X",
            itemcombination_bb[i].result_item[0],
            itemcombination_bb[i].result_item[1],
            itemcombination_bb[i].result_item[2]);
#endif // DEBUG
        return -1;
    }
#ifdef DEBUG
    else
        DBG_LOG("结果, result_item 0x%02X 0x%02X 0x%02X",
            itemcombination_bb[i].result_item[0],
            itemcombination_bb[i].result_item[1],
            itemcombination_bb[i].result_item[2]);
#endif // DEBUG

    return 0;
}

uint8_t pmt_lookup_stars_bb(uint32_t code) {
    uint8_t parts[3] = { 0 };
    pmt_weapon_bb_t weap;
    pmt_guard_bb_t guard;
    pmt_unit_bb_t unit;

    /* Make sure we loaded the PMT stuff to start with. */
    if(!have_bb_pmt)
        return (uint8_t)-1;

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    switch(parts[0]) {
        case ITEM_TYPE_WEAPON:                        /* Weapons */
            if(pmt_lookup_weapon_bb(code, &weap))
                return (uint8_t)-1;

            if(weap.index - weapon_lowest_bb > star_max_bb)
                return (uint8_t)-1;

            return star_table_bb[weap.index - weapon_lowest_bb];

        case ITEM_TYPE_GUARD:                         /* Guards */
            switch(parts[1]) {
                case ITEM_SUBTYPE_FRAME:              /* Armors */
                case ITEM_SUBTYPE_BARRIER:            /* Shields */
                    if(pmt_lookup_guard_bb(code, &guard))
                        return (uint8_t)-1;

                    if(guard.index - weapon_lowest_bb > star_max_bb)
                        return (uint8_t)-1;

                    return star_table_bb[guard.index - weapon_lowest_bb];

                case ITEM_SUBTYPE_UNIT:               /* Units */
                    if(pmt_lookup_unit_bb(code, &unit))
                        return (uint8_t)-1;

                    if(unit.index - weapon_lowest_bb > star_max_bb)
                        return (uint8_t)-1;

                    return star_table_bb[unit.index - weapon_lowest_bb];
            }
    }

    return (uint8_t)-1;
}

/*
   Generate a random unit, based off of data for PSOv2. We precalculate most of
   the stuff needed for this up above in the build_v2_units() function.

   Random unit generation is actually pretty simple, compared to the other item
   types. Basically, the game builds up a table of units that have the amount of
   stars specified in the PT data's unit_level value for the floor or less
   and randomly selects from that table for each time its needed. Note that the
   game always has a fallback of a plain Knight/Power in case no other unit can
   be generated (which means that once you hit the star level for Knight/Power,
   it is actually 2x as likely to drop as any other unit). Note that the table
   also holds all the +/++/-/-- combinations for the units as well. Only one
   random number ever has to be generated to pick the unit, since the table
   contains all the boosted/lowered versions of the units.

   Some units (for some reason) have a +/- increment defined in the PMT, but the
   game disallows those units from dropping with + or - ever. As long as you
   don't configure it otherwise, this will work as it does in the game. The
   units affected by this are all the /HP, /TP, /Body, /Luck, /Ability, Resist/,
   /Resist, HP/, TP/, PB/, /Technique, /Battle, State/Maintenance, and
   Trap/Search (i.e, most of the units -- note everything there after /Resist
   is actually defined as a 0 increment anyway).
*/
int pmt_random_unit_v2(uint8_t max, uint32_t item[4],
                       struct mt19937_state*rng, lobby_t *l) {
    uint64_t unit;
    uint32_t rnd = mt19937_genrand_int32(rng);

#ifdef DEBUG
    if(l->flags & LOBBY_FLAG_DBG_SDROPS)
        SHIPS_LOG("pmt_random_unit_v2: max stars: %" PRIu8 "(%" PRIu8
              ")", max, unit_max_stars);
#endif

    if(max > unit_max_stars)
        max = unit_max_stars;

    /* Pick one of them, and return it. */
    unit = units_by_stars[rnd % units_with_stars[max]];
    item[0] = (uint32_t)unit;
    item[1] = (uint32_t)(unit >> 32);
    item[2] = item[3] = 0;

#ifdef DEBUG
    if(l->flags & LOBBY_FLAG_DBG_SDROPS)
        SHIPS_LOG("    RNG picked %" PRIu32 " max: %" PRIu32 " i: %"
              PRIu32 "", rnd, units_with_stars[max],
              rnd % units_with_stars[max]);
#endif

    return 0;
}

int pmt_random_unit_gc(uint8_t max, uint32_t item[4],
                       struct mt19937_state*rng, lobby_t *l) {
    uint64_t unit;
    uint32_t rnd = mt19937_genrand_int32(rng);

#ifdef DEBUG
    if(l->flags & LOBBY_FLAG_DBG_SDROPS)
        SHIPS_LOG("pmt_random_unit_gc: max stars: %" PRIu8 "(%" PRIu8
              ")", max, unit_max_stars_gc);
#endif

    if(max > unit_max_stars_gc)
        max = unit_max_stars_gc;

    /* Pick one of them, and return it. */
    unit = units_by_stars_gc[rnd % units_with_stars_gc[max]];
    item[0] = (uint32_t)unit;
    item[1] = (uint32_t)(unit >> 32);
    item[2] = item[3] = 0;

#ifdef DEBUG
    if(l->flags & LOBBY_FLAG_DBG_SDROPS)
        SHIPS_LOG("    RNG picked %" PRIu32 " max: %" PRIu32 " i: %"
              PRIu32 "", rnd, units_with_stars_gc[max],
              rnd % units_with_stars_gc[max]);
#endif

    return 0;
}

int pmt_random_unit_bb(uint8_t max, uint32_t item[4],
                       struct mt19937_state*rng, lobby_t *l) {
    uint64_t unit;
    uint32_t rnd = mt19937_genrand_int32(rng);

#ifdef DEBUG
    if(l->flags & LOBBY_FLAG_DBG_SDROPS)
        SHIPS_LOG("pmt_random_unit_bb: max stars: %" PRIu8 "(%" PRIu8
              ")", max, unit_max_stars_bb);
#endif

    if(max > unit_max_stars_bb)
        max = unit_max_stars_bb;

    /* Pick one of them, and return it. */
    unit = units_by_stars_bb[rnd % units_with_stars_bb[max]];
    item[0] = (uint32_t)unit;
    item[1] = (uint32_t)(unit >> 32);
    item[2] = item[3] = 0;

#ifdef DEBUG
    if(l->flags & LOBBY_FLAG_DBG_SDROPS)
        SHIPS_LOG("    RNG picked %" PRIu32 " max: %" PRIu32 " i: %"
              PRIu32 "", rnd, units_with_stars_bb[max],
              rnd % units_with_stars_bb[max]);
#endif

    return 0;
}
