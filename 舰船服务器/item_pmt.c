/*
    �λ�֮���й� ItemPMT
    ��Ȩ (C) 2022, 2023 Sancaros

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
#include <SFMT.h>
#include <pso_StringReader.h>

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
        ERR_LOG("v2 ItemPMT �е�ָ���λ����Ч!");
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
        ERR_LOG("GC ItemPMT �е�ָ���λ����Ч!");
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

    memcpy(&tmp, pmt + sz - 0x10, sizeof(uint32_t));
    tmp = LE32(tmp);

    if(tmp + sizeof(pmt_table_offsets_v3_t) > sz - 0x10) {
        ERR_LOG("BB ItemPMT �е�ָ���λ����Ч!");
        return -1;
    }

    memcpy(ptrs->ptr, pmt + tmp, sizeof(pmt_table_offsets_v3_t));

#ifdef DEBUG

    for (int i = 0; i < 23; ++i) {
        DBG_LOG("offsets = 0x%08X", ptrs->ptr[i]);
    }

    DBG_LOG("armor_table offsets = %u", ptrs->armor_table);
    DBG_LOG("ranged_special_table offsets = %u", ptrs->ranged_special_table);

#endif // DEBUG

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    for (int i = 0; i < 23; ++i) {
        ptrs->ptr[i] = LE32(ptrs->ptr[i]);
    }
#endif

    return 0;
}

static int read_weapons_v2(const uint8_t *pmt, uint32_t sz,
                           const uint32_t ptrs[21]) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the ָ����Ч are sane... */
    if(ptrs[1] > sz || ptrs[1] > ptrs[11]) {
        ERR_LOG("ItemPMT.prs file for v2 �� weapon ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* ��������ж����ű�... */
    num_weapon_types = cnt = (ptrs[11] - ptrs[1]) / 8;

    /* Allocate the stuff we need to allocate... */
    if(!(num_weapons = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� v2 weapon count: %s",
              strerror(errno));
        num_weapon_types = 0;
        return -2;
    }

    if(!(weapons = (pmt_weapon_v2_t **)malloc(sizeof(pmt_weapon_v2_t *) *
                                              cnt))) {
        ERR_LOG("���䶯̬�ڴ�� v2 weapon list: %s",
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
                  "of file bounds! �����ļ�����Ч��!");
            return -4;
        }

        num_weapons[i] = values[0];
        if(!(weapons[i] = (pmt_weapon_v2_t *)malloc(sizeof(pmt_weapon_v2_t) *
                                                    values[0]))) {
            ERR_LOG("���䶯̬�ڴ�� v2 weapons: %s",
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

    /* Make sure the ָ����Ч are sane... */
    if(ptrs->ptr[0] > sz || ptrs->ptr[0] > ptrs->ptr[17]) {
        ERR_LOG("ItemPMT.prs file for GC �� weapon ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* ��������ж����ű�... */
    num_weapon_types_gc = cnt = (ptrs->ptr[17] - ptrs->ptr[0]) / 8;

    /* Allocate the stuff we need to allocate... */
    if(!(num_weapons_gc = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� GC weapon count: %s",
              strerror(errno));
        num_weapon_types_gc = 0;
        return -2;
    }

    if(!(weapons_gc = (pmt_weapon_gc_t **)malloc(sizeof(pmt_weapon_gc_t *) *
                                                 cnt))) {
        ERR_LOG("���䶯̬�ڴ�� GC weapon list: %s",
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
                  "of file bounds! �����ļ�����Ч��!");
            return -4;
        }

        num_weapons_gc[i] = values[0];
        if(!(weapons_gc[i] = (pmt_weapon_gc_t *)malloc(sizeof(pmt_weapon_gc_t) *
                                                       values[0]))) {
            ERR_LOG("���䶯̬�ڴ�� GC weapons: %s",
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
    uint32_t cnt, i, j;
    pmt_countandoffset_t values;

    /* Make sure the ָ����Ч are sane... */
    if(ptrs->weapon_table > sz || ptrs->weapon_table > ptrs->combination_table) {
        ERR_LOG("ItemPMT.prs file for BB �� weapon ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* ��������ж����ű�... */
    num_weapon_types_bb = cnt = (ptrs->combination_table - ptrs->weapon_table) / 8;

    /* Allocate the stuff we need to allocate... */
    if(!(num_weapons_bb = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB weapon count: %s",
              strerror(errno));
        num_weapon_types_bb = 0;
        return -2;
    }

    if(!(weapons_bb = (pmt_weapon_bb_t **)malloc(sizeof(pmt_weapon_bb_t *) *
                                                 cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB weapon list: %s",
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
        memcpy(&values, pmt + ptrs->weapon_table + (i << 3), sizeof(pmt_countandoffset_t));
        values.count = LE32(values.count);
        values.offset = LE32(values.offset);

        /* Make sure we have enough file... */
        if(values.offset + sizeof(pmt_weapon_bb_t) * values.count > sz) {
            ERR_LOG("ItemPMT.prs file for BB has weapon table outside "
                  "of file bounds! �����ļ�����Ч��!");
            return -4;
        }

        num_weapons_bb[i] = values.count;
        if(!(weapons_bb[i] = (pmt_weapon_bb_t *)malloc(sizeof(pmt_weapon_bb_t) *
                                                       values.count))) {
            ERR_LOG("���䶯̬�ڴ�� BB weapons: %s",
                  strerror(errno));
            return -5;
        }

        memcpy(weapons_bb[i], pmt + values.offset,
               sizeof(pmt_weapon_bb_t) * values.count);

        for(j = 0; j < values.count; ++j) {
#ifdef DEBUG
            DBG_LOG("/////////////// %d", i);
            DBG_LOG("base.index %u", weapons_bb[i][j].base.index);
            DBG_LOG("base.team_points %u", weapons_bb[i][j].base.team_points);
            DBG_LOG("equip_flag %u", weapons_bb[i][j].equip_flag);
#endif // DEBUG

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

            if(weapons_bb[i][j].base.index < weapon_lowest_bb)
                weapon_lowest_bb = weapons_bb[i][j].base.index;
        }
    }

    return 0;
}

static int read_guards_v2(const uint8_t *pmt, uint32_t sz,
                          const uint32_t ptrs[21]) {
    uint32_t cnt, i, values[2], j;

    /* Make sure the ָ����Ч are sane... */
    if(ptrs[3] > sz || ptrs[2] > ptrs[3]) {
        ERR_LOG("ItemPMT.prs file for v2 �� guard ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* ��������ж����ű�... */
    num_guard_types = cnt = (ptrs[3] - ptrs[2]) / 8;

    /* Make sure its sane... Should always be 2. */
    if(cnt != 2) {
        ERR_LOG("ItemPMT.prs file for v2 does not have two guard "
              "tables. ��������Ч��!");
        num_guard_types = 0;
        return -2;
    }

    /* Allocate the stuff we need to allocate... */
    if(!(num_guards = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� v2 guard count: %s",
              strerror(errno));
        num_guard_types = 0;
        return -3;
    }

    if(!(guards = (pmt_guard_v2_t **)malloc(sizeof(pmt_guard_v2_t *) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� v2 guards list: %s",
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
                  "of file bounds! �����ļ�����Ч��!");
            return -5;
        }

        num_guards[i] = values[0];
        if(!(guards[i] = (pmt_guard_v2_t *)malloc(sizeof(pmt_guard_v2_t) *
                                                  values[0]))) {
            ERR_LOG("���䶯̬�ڴ�� v2 guards: %s",
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

    /* Make sure the ָ����Ч are sane... */
    if(ptrs->ptr[2] > sz || ptrs->ptr[1] > ptrs->ptr[2]) {
        ERR_LOG("ItemPMT.prs file for GC �� guard ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* ��������ж����ű�... */
    num_guard_types_gc = cnt = (ptrs->ptr[2] - ptrs->ptr[1]) / 8;

    /* Make sure its sane... Should always be 2. */
    if(cnt != 2) {
        ERR_LOG("ItemPMT.prs file for GC does not have two guard "
              "tables. ��������Ч��!");
        num_guard_types_gc = 0;
        return -2;
    }

    /* Allocate the stuff we need to allocate... */
    if(!(num_guards_gc = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� GC guard count: %s",
              strerror(errno));
        num_guard_types_gc = 0;
        return -3;
    }

    if(!(guards_gc = (pmt_guard_gc_t **)malloc(sizeof(pmt_guard_gc_t *) *
                                               cnt))) {
        ERR_LOG("���䶯̬�ڴ�� GC guards list: %s",
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
                  "of file bounds! �����ļ�����Ч��!");
            return -5;
        }

        num_guards_gc[i] = values[0];
        if(!(guards_gc[i] = (pmt_guard_gc_t *)malloc(sizeof(pmt_guard_gc_t) *
                                                     values[0]))) {
            ERR_LOG("���䶯̬�ڴ�� GC guards: %s",
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
    uint32_t cnt, i, j;
    pmt_countandoffset_t values;

    /* Make sure the ָ����Ч are sane... */
    if(ptrs->unit_table > sz || ptrs->armor_table > ptrs->unit_table) {
        ERR_LOG("ItemPMT.prs file for BB �� guard ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* ��������ж����ű�... */
    num_guard_types_bb = cnt = (ptrs->unit_table - ptrs->armor_table) / 8;

    /* Make sure its sane... Should always be 2. */
    if(cnt != 2) {
        ERR_LOG("ItemPMT.prs file for BB does not have two guard "
              "tables. ��������Ч��!");
        num_guard_types_bb = 0;
        return -2;
    }

    /* Allocate the stuff we need to allocate... */
    if(!(num_guards_bb = (uint32_t *)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB guard count: %s",
              strerror(errno));
        num_guard_types_bb = 0;
        return -3;
    }

    if(!(guards_bb = (pmt_guard_bb_t **)malloc(sizeof(pmt_guard_bb_t *) *
                                               cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB guards list: %s",
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
        memcpy(&values, pmt + ptrs->armor_table + (i << 3), sizeof(pmt_countandoffset_t));
        values.count = LE32(values.count);
        values.offset = LE32(values.offset);

        /* Make sure we have enough file... */
        if(values.offset + sizeof(pmt_guard_bb_t) * values.count > sz) {
            ERR_LOG("ItemPMT.prs file for BB has guard table outside "
                  "of file bounds! �����ļ�����Ч��!");
            return -5;
        }

        num_guards_bb[i] = values.count;
        if(!(guards_bb[i] = (pmt_guard_bb_t *)malloc(sizeof(pmt_guard_bb_t) *
                                                     values.count))) {
            ERR_LOG("���䶯̬�ڴ�� BB guards: %s",
                  strerror(errno));
            return -6;
        }

        memcpy(guards_bb[i], pmt + values.offset,
               sizeof(pmt_guard_bb_t) * values.count);

        for(j = 0; j < values.count; ++j) {
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
            guards_bb[i][j].index = LE32(guards_bb[i][j].index);
            guards_bb[i][j].model = LE16(guards_bb[i][j].model);
            guards_bb[i][j].skin = LE16(guards_bb[i][j].skin);
            guards_bb[i][j].team_points = LE16(guards_bb[i][j].team_points);
            guards_bb[i][j].base_dfp = LE16(guards_bb[i][j].base_dfp);
            guards_bb[i][j].base_evp = LE16(guards_bb[i][j].base_evp);
#endif

            if(guards_bb[i][j].base.index < guard_lowest_bb)
                guard_lowest_bb = guards_bb[i][j].base.index;
        }
    }

    return 0;
}

static int read_units_v2(const uint8_t *pmt, uint32_t sz,
                         const uint32_t ptrs[21]) {
    uint32_t values[2], i;

    /* Make sure the ָ����Ч are sane... */
    if(ptrs[3] > sz) {
        ERR_LOG("ItemPMT.prs file for v2 �� unit ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(values, pmt + ptrs[3], sizeof(uint32_t) * 2);
    values[0] = LE32(values[0]);
    values[1] = LE32(values[1]);

    /* Make sure we have enough file... */
    if(values[1] + sizeof(pmt_unit_v2_t) * values[0] > sz) {
        ERR_LOG("ItemPMT.prs file for v2 has unit table outside "
              "of file bounds! �����ļ�����Ч��!");
        return -2;
    }

    num_units = values[0];
    if(!(units = (pmt_unit_v2_t *)malloc(sizeof(pmt_unit_v2_t) * values[0]))) {
        ERR_LOG("���䶯̬�ڴ�� v2 units: %s",
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

    /* Make sure the ָ����Ч are sane... */
    if(ptrs->ptr[2] > sz) {
        ERR_LOG("ItemPMT.prs file for GC �� unit ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(values, pmt + ptrs->ptr[2], sizeof(uint32_t) * 2);
    values[0] = ntohl(values[0]);
    values[1] = ntohl(values[1]);

    /* Make sure we have enough file... */
    if(values[1] + sizeof(pmt_unit_gc_t) * values[0] > sz) {
        ERR_LOG("ItemPMT.prs file for GC has unit table outside "
              "of file bounds! �����ļ�����Ч��!");
        return -2;
    }

    num_units_gc = values[0];
    if(!(units_gc = (pmt_unit_gc_t *)malloc(sizeof(pmt_unit_gc_t) *
                                            values[0]))) {
        ERR_LOG("���䶯̬�ڴ�� GC units: %s",
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
    pmt_countandoffset_t values;
    size_t i;

    /* Make sure the ָ����Ч are sane... */
    if(ptrs->unit_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB �� unit ָ����Ч. "
              "��������Ч��!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(&values, pmt + ptrs->unit_table, sizeof(pmt_countandoffset_t));
    values.count = LE32(values.count);
    values.offset = LE32(values.offset);

    /* Make sure we have enough file... */
    if(values.offset + sizeof(pmt_unit_bb_t) * values.count > sz) {
        ERR_LOG("ItemPMT.prs file for BB has unit table outside "
              "of file bounds! �����ļ�����Ч��!");
        return -2;
    }

    num_units_bb = values.count;
    if(!(units_bb = (pmt_unit_bb_t *)malloc(sizeof(pmt_unit_bb_t) *
                                            values.count))) {
        ERR_LOG("���䶯̬�ڴ�� BB units: %s",
              strerror(errno));
        num_units_bb = 0;
        return -3;
    }

    memcpy(units_bb, pmt + values.offset, sizeof(pmt_unit_bb_t) * values.count);

    for(i = 0; i < values.count; ++i) {

#ifdef DEBUG

        DBG_LOG("index %d base.index %d", i, units_bb[i].base.index);
        DBG_LOG("stat %d", units_bb[i].stat);
        DBG_LOG("amount %d", units_bb[i].amount);
        DBG_LOG("pm_range %d", units_bb[i].pm_range);

#endif // DEBUG
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
        units_bb[i].base.index = LE32(units_bb[i].base.index);
        units_bb[i].base.subtype = LE16(units_bb[i].subtype);
        units_bb[i].base.skin = LE16(units_bb[i].base.skin);
        units_bb[i].base.team_points = LE16(units_bb[i].base.team_points);
        units_bb[i].stat = LE16(units_bb[i].stat);
        units_bb[i].amount = LE16(units_bb[i].amount);
#endif

        if(units_bb[i].base.index < unit_lowest_bb)
            unit_lowest_bb = units_bb[i].base.index;
    }

    return 0;
}

static int read_stars_v2(const uint8_t *pmt, uint32_t sz,
                         const uint32_t ptrs[21]) {
    /* Make sure the ָ����Ч are sane... */
    if(ptrs[12] > sz || ptrs[13] > sz || ptrs[13] < ptrs[12]) {
        ERR_LOG("ItemPMT.prs file for v2 �� star ָ����Ч. "
              "��������Ч��!");
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
        ERR_LOG("���䶯̬�ڴ�� star table: %s", strerror(errno));
        star_max = 0;
        return -3;
    }

    memcpy(star_table, pmt + ptrs[12], star_max);
    return 0;
}

static int read_stars_gc(const uint8_t *pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    /* Make sure the ָ����Ч are sane... */
    if(ptrs->ptr[11] > sz || ptrs->ptr[12] > sz || ptrs->ptr[12] < ptrs->ptr[11]) {
        ERR_LOG("ItemPMT.prs file for GC �� star ָ����Ч. "
              "��������Ч��!");
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
        ERR_LOG("���䶯̬�ڴ�� star table: %s", strerror(errno));
        star_max_gc = 0;
        return -3;
    }

    memcpy(star_table_gc, pmt + ptrs->ptr[11], star_max_gc);
    return 0;
}

static int read_stars_bb(const uint8_t *pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    /* Make sure the ָ����Ч are sane... */
    if(ptrs->star_value_table > sz || ptrs->special_data_table > sz || ptrs->special_data_table < ptrs->star_value_table) {
        ERR_LOG("ItemPMT.prs file for BB �� star ָ����Ч. "
              "��������Ч��!");
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
        ERR_LOG("���䶯̬�ڴ�� star table: %s", strerror(errno));
        star_max_bb = 0;
        return -3;
    }

    /* Read the pointer and the size... */
    memcpy(star_table_bb, pmt + ptrs->star_value_table, star_max_bb);

#ifdef DEBUG
    for (size_t i = 0; i < star_max_bb; ++i) {
        DBG_LOG("i %d 0x%02X", i, star_table_bb[i]);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }
    getchar();
#endif // DEBUG
    return 0;
}

static int read_mags_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    pmt_countandoffset_t values;
    size_t i;

    /* Make sure the ָ����Ч are sane... */
    if (ptrs->mag_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB �� mag ָ����Ч. "
            "��������Ч��!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(&values, pmt + ptrs->mag_table, sizeof(pmt_countandoffset_t));
    values.count = LE32(values.count);
    values.offset = LE32(values.offset);

    /* Make sure we have enough file... */
    if (values.offset + sizeof(pmt_mag_bb_t) * values.count > sz) {
        ERR_LOG("ItemPMT.prs file for BB has mag table outside "
            "of file bounds! �����ļ�����Ч��!");
        return -2;
    }

    num_mags_bb = values.count;
    if (!(mags_bb = (pmt_mag_bb_t*)malloc(sizeof(pmt_mag_bb_t) *
        values.count))) {
        ERR_LOG("���䶯̬�ڴ�� BB mags: %s",
            strerror(errno));
        num_mags_bb = 0;
        return -3;
    }

    memcpy(mags_bb, pmt + values.offset, sizeof(pmt_mag_bb_t) * values.count);

    //DBG_LOG("num_mag_types_bb %d", num_mags_bb);

    for (i = 0; i < values.count; ++i) {
        //DBG_LOG("index %d", mags_bb[i].index);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
        mags_bb[i].index = LE32(mags_bb[i].index);
        mags_bb[i].model = LE16(mags_bb[i].model);
        mags_bb[i].skin = LE16(mags_bb[i].skin);
        mags_bb[i].team_points = LE32(mags_bb[i].team_points);
        mags_bb[i].feed_table = LE16(mags_bb[i].feed_table);
#endif

        if (mags_bb[i].base.index < mag_lowest_bb)
            mag_lowest_bb = mags_bb[i].base.index;
    }

    return 0;
}

static int read_tools_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    uint32_t cnt, i, j;
    pmt_countandoffset_t values;

    /* Make sure the ָ����Ч are sane... */
    if (ptrs->tool_table > sz || ptrs->tool_table > ptrs->weapon_table) {
        ERR_LOG("ItemPMT.prs file for BB �� tool ָ����Ч. "
            "��������Ч��!");
        return -1;
    }

    /* ��������ж����ű�... */
    num_tool_types_bb = cnt = (ptrs->weapon_table - ptrs->tool_table) / 8;

#ifdef DEBUG

    DBG_LOG("num_tool_types_bb %d", num_tool_types_bb);

    getchar();

#endif // DEBUG

    /* Allocate the stuff we need to allocate... */
    if (!(num_tools_bb = (uint32_t*)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB tool count: %s",
            strerror(errno));
        num_tool_types_bb = 0;
        return -2;
    }

    if (!(tools_bb = (pmt_tool_bb_t**)malloc(sizeof(pmt_tool_bb_t*) *
        cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB tool list: %s",
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
        memcpy(&values, pmt + ptrs->tool_table + (i << 3), sizeof(pmt_countandoffset_t));
        values.count = LE32(values.count);
        values.offset = LE32(values.offset);

        /* Make sure we have enough file... */
        if (values.offset + sizeof(pmt_tool_bb_t) * values.count > sz) {
            ERR_LOG("ItemPMT.prs file for BB has weapon table outside "
                "of file bounds! �����ļ�����Ч��!");
            return -4;
        }

        num_tools_bb[i] = values.count;

        //DBG_LOG("i %d num_tools_bb %d", i, num_tools_bb[i]);

        if (!(tools_bb[i] = (pmt_tool_bb_t*)malloc(sizeof(pmt_tool_bb_t) *
            values.count))) {
            ERR_LOG("���䶯̬�ڴ�� BB weapons: %s",
                strerror(errno));
            return -5;
        }

        memcpy(tools_bb[i], pmt + values.offset,
            sizeof(pmt_tool_bb_t) * values.count);

        for (j = 0; j < values.count; ++j) {
#ifdef DEBUG
            if (i == 2)
                DBG_LOG("id %d index %d cost %d", j, tools_bb[i][j].base.index, tools_bb[i][j].cost);
            DBG_LOG("id %d index %d cost %d", j, tools_bb[i][j].base.index, tools_bb[i][j].cost);
#endif // DEBUG

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
            tools_bb[i][j].index = LE32(tools_bb[i][j].index);
            tools_bb[i][j].model = LE16(tools_bb[i][j].model);
            tools_bb[i][j].skin = LE16(tools_bb[i][j].skin);
            tools_bb[i][j].team_points = LE32(tools_bb[i][j].team_points);
            tools_bb[i][j].amount = LE16(tools_bb[i][j].amount);
            tools_bb[i][j].tech = LE16(tools_bb[i][j].tech);
            tools_bb[i][j].cost = LE32(tools_bb[i][j].cost);
#endif

            if (tools_bb[i][j].base.index < tool_lowest_bb)
                tool_lowest_bb = tools_bb[i][j].base.index;
        }
    }

    return 0;
}

static int read_itemcombinations_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    pmt_countandoffset_t values;

    /* Make sure the ָ����Ч are sane... */
    if (ptrs->combination_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB �� itemcombination ָ����Ч. "
            "��������Ч��!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(&values, pmt + ptrs->combination_table, sizeof(pmt_countandoffset_t));
    values.count = LE32(values.count);
    values.offset = LE32(values.offset);

    /* Make sure we have enough file... */
    if (values.offset + sizeof(pmt_itemcombination_bb_t) * values.count > sz) {
        ERR_LOG("ItemPMT.prs file for BB has itemcombination table outside "
            "of file bounds! �����ļ�����Ч��!");
        return -2;
    }

    itemcombinations_max_bb = values.count;
    if (!(itemcombination_bb = (pmt_itemcombination_bb_t*)malloc(sizeof(pmt_itemcombination_bb_t) *
        values.count))) {
        ERR_LOG("���䶯̬�ڴ�� BB itemcombination: %s",
            strerror(errno));
        itemcombinations_max_bb = 0;
        return -3;
    }

    memcpy(itemcombination_bb, pmt + values.offset, sizeof(pmt_itemcombination_bb_t) * values.count);

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
    pmt_countandoffset_t values;
    size_t i = 0, j = 0, cnt;

    /* Make sure the ָ����Ч are sane... */
    if (ptrs->unwrap_table > sz || ptrs->unwrap_table > ptrs->mag_feed_table) {
        ERR_LOG("ItemPMT.prs file for BB �� eventitem ָ����Ч. "
            "��������Ч��!");
        return -1;
    }

    /* ��������ж����ű�... */
    num_eventitem_types_bb = cnt = (ptrs->mag_feed_table - ptrs->unwrap_table) / 8;

#ifdef DEBUG
    DBG_LOG("num_eventitem_types_bb %d", num_eventitem_types_bb);
#endif // DEBUG

    /* Allocate the stuff we need to allocate... */
    if (!(num_eventitems_bb = (uint32_t*)malloc(sizeof(uint32_t) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB eventitem count: %s",
            strerror(errno));
        num_eventitem_types_bb = 0;
        return -2;
    }

    if (!(eventitem_bb = (pmt_eventitem_bb_t**)malloc(sizeof(pmt_eventitem_bb_t*) *
        cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB eventitem list: %s",
            strerror(errno));
        free_safe(num_eventitems_bb);
        num_eventitems_bb = NULL;
        num_eventitem_types_bb = 0;
        return -3;
    }

    memset(eventitem_bb, 0, sizeof(pmt_eventitem_bb_t*) * cnt);

    /* Read in each table... */
    for (i = 0; i < cnt; ++i) {
        /* Read the pointer and the size... */
        memcpy(&values, pmt + 0x00015014 + (i << 3)/*ptrs->unwrap_table + (i << 3)*/, sizeof(pmt_countandoffset_t));
        values.count = LE32(values.count);
        values.offset = LE32(values.offset);

        /* Make sure we have enough file... */
        if (values.offset + sizeof(pmt_eventitem_bb_t) * values.count > sz) {
            ERR_LOG("ItemPMT.prs file for BB has eventitem table outside "
                "of file bounds! �����ļ�����Ч��!");
            return -4;
        }

        num_eventitems_bb[i] = values.count;

        //DBG_LOG("i %d num_eventitems_bb %d", i, num_eventitems_bb[i]);

        if (!(eventitem_bb[i] = (pmt_eventitem_bb_t*)malloc(sizeof(pmt_eventitem_bb_t) *
            values.count))) {
            ERR_LOG("���䶯̬�ڴ�� BB eventitem: %s",
                strerror(errno));
            return -5;
        }

        memcpy(eventitem_bb[i], pmt + values.offset,
            sizeof(pmt_eventitem_bb_t) * values.count);

#ifdef DEBUG

        for (j = 0; j < values.count; ++j) {
            DBG_LOG("i %d j %d 0x%02X", i, j, eventitem_bb[i][j].item[0]);
            DBG_LOG("i %d j %d 0x%02X", i, j, eventitem_bb[i][j].item[1]);
            DBG_LOG("i %d j %d 0x%02X", i, j, eventitem_bb[i][j].item[2]);
            DBG_LOG("i %d j %d 0x%02X", i, j, eventitem_bb[i][j].probability);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
        }
    getchar();
#endif // DEBUG

    }

    return 0;
}

static int read_unsealableitems_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    pmt_countandoffset_t values;
    size_t i = 0, j = 0;

    /* Make sure the ָ����Ч are sane... */
    if (ptrs->unsealable_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB �� unsealableitem ָ����Ч. "
            "��������Ч��!");
        return -1;
    }


    /* Read the pointer and the size... */
    memcpy(&values, pmt + ptrs->unsealable_table, sizeof(pmt_countandoffset_t));
    values.count = LE32(values.count);
    values.offset = LE32(values.offset);

    /* Make sure we have enough file... */
    if (values.offset + sizeof(pmt_unsealableitem_bb_t) * values.count > sz) {
        ERR_LOG("ItemPMT.prs file for BB has unsealableitem table outside "
            "of file bounds! �����ļ�����Ч��!");
        return -2;
    }

    unsealableitems_max_bb = values.count;

    //DBG_LOG("unsealableitems_max_bb %d", unsealableitems_max_bb);

    if (!(unsealableitems_bb = (pmt_unsealableitem_bb_t*)malloc(sizeof(pmt_unsealableitem_bb_t) *
        values.count))) {
        ERR_LOG("���䶯̬�ڴ�� BB unsealableitems: %s",
            strerror(errno));
        unsealableitems_max_bb = 0;
        return -3;
    }

    memcpy(unsealableitems_bb, pmt + values.offset, sizeof(pmt_unsealableitem_bb_t) * values.count);

#ifdef DEBUG

    for (i = 0; i < values.count; ++i) {
        DBG_LOG("item 0x%02X", unsealableitems_bb[i].item[0]);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }

#endif // DEBUG

    return 0;
}

static int read_nonweaponsaledivisors_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    size_t i = 0, j = 0;

    /* Make sure the ָ����Ч are sane... */
    if (ptrs->sale_divisor_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB �� nonweaponsaledivisors ָ����Ч. "
            "��������Ч��!");
        return -1;
    }

    /* Read the pointer and the size... */
    memcpy(&nonweaponsaledivisors_bb, pmt + ptrs->sale_divisor_table, sizeof(pmt_nonweaponsaledivisors_bb_t));

#ifdef DEBUG

    DBG_LOG("armor_divisor %f", nonweaponsaledivisors_bb.armor_divisor);
    DBG_LOG("shield_divisor %f", nonweaponsaledivisors_bb.shield_divisor);
    DBG_LOG("unit_divisor %f", nonweaponsaledivisors_bb.unit_divisor);
    DBG_LOG("mag_divisor %f", nonweaponsaledivisors_bb.mag_divisor);

    getchar();

#endif // DEBUG
    return 0;
}

static int read_weaponsaledivisors_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    size_t i = 0, j = 0, num_weaponsaledivisors = 0xA5;

    /* Make sure the ָ����Ч are sane... */
    if (ptrs->weapon_sale_divisor_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB �� weaponsaledivisors ָ����Ч. "
            "��������Ч��!");
        return -1;
    }

    if (!(weaponsaledivisors_bb = (float*)malloc(sizeof(float) * num_weaponsaledivisors))) {
        ERR_LOG("���䶯̬�ڴ�� weaponsaledivisors_bb table: %s", strerror(errno));
        star_max_bb = 0;
        return -2;
    }

    /* Read the pointer and the size... */
    memcpy(weaponsaledivisors_bb, pmt + ptrs->weapon_sale_divisor_table, sizeof(float) * num_weaponsaledivisors);

#ifdef DEBUG

    for (i = 0; i < num_weaponsaledivisors; ++i) {
        DBG_LOG("weapon_divisor %f", weaponsaledivisors_bb[i]);
    }

    getchar();

#endif // DEBUG

    return 0;
}

static int read_mag_feed_results_bb(const uint8_t* pmt, uint32_t sz,
                         const pmt_table_offsets_v3_t* ptrs) {
    size_t i = 0, j = 0, cnt = (sizeof(pmt_mag_feed_results_list_offsets_t) / 4);

    /* Make sure the ָ����Ч are sane... */
    if (ptrs->mag_feed_table > sz) {
        ERR_LOG("ItemPMT.prs file for BB �� mag_feed_table ָ����Ч. "
            "��������Ч��!");
        return -1;
    }

    if (!(mag_feed_results_list_offsets = (pmt_mag_feed_results_list_offsets_t*)malloc(sizeof(pmt_mag_feed_results_list_offsets_t)))) {
        ERR_LOG("���䶯̬�ڴ�� BB mag_feed_results_list_offsets: %s",
            strerror(errno));
        free_safe(mag_feed_results_list_offsets);
        return -2;
    }

    /* Read the pointer and the size... */
    memcpy(mag_feed_results_list_offsets, pmt + ptrs->mag_feed_table, sizeof(pmt_mag_feed_results_list_offsets_t));

    if (!(mag_feed_results_list = (pmt_mag_feed_results_list_t**)malloc(sizeof(pmt_mag_feed_results_list_t*) * cnt))) {
        ERR_LOG("���䶯̬�ڴ�� BB mag_feed_results list: %s",
            strerror(errno));
        return -3;
    }

    memset(mag_feed_results_list, 0, sizeof(pmt_mag_feed_results_list_t*) * cnt);

    for (i = 0; i < cnt; ++i) {

        if (!(mag_feed_results_list[i] = (pmt_mag_feed_results_list_t*)malloc(sizeof(pmt_mag_feed_results_list_t)))) {
            ERR_LOG("���䶯̬�ڴ�� BB mag_feed_results_list: %s",
                strerror(errno));
            return -5;
        }

        memcpy(mag_feed_results_list[i], pmt + mag_feed_results_list_offsets->offsets[i], sizeof(pmt_mag_feed_results_list_t));

#ifdef DEBUG

        for (j = 0; j < 11; j++) {
            DBG_LOG("item %d", mag_feed_results_list[i]->results[j].def);
            DBG_LOG("item %d", mag_feed_results_list[i]->results[j].pow);
            DBG_LOG("item %d", mag_feed_results_list[i]->results[j].dex);
            DBG_LOG("item %d", mag_feed_results_list[i]->results[j].mind);
            DBG_LOG("item %d", mag_feed_results_list[i]->results[j].iq);
            DBG_LOG("item %d", mag_feed_results_list[i]->results[j].synchro);
            DBG_LOG("item %d", mag_feed_results_list[i]->results[j].unused[0]);
            DBG_LOG("item %d", mag_feed_results_list[i]->results[j].unused[1]);
        }

        if (i == 2)
            getchar();
#endif // DEBUG

    }

    return 0;
}

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
        ERR_LOG("���䶯̬�ڴ�� unit table: %s", strerror(errno));
        return -1;
    }

    /* Allocate space for the "pointer" table */
    if(!(units_with_stars = (uint32_t *)malloc((unit_max_stars + 1) *
                                               sizeof(uint32_t)))) {
        ERR_LOG("���䶯̬�ڴ�� unit ptr table: %s",
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
        ERR_LOG("���䶯̬�ڴ�� unit table: %s", strerror(errno));
        return -1;
    }

    /* Allocate space for the "pointer" table */
    if(!(units_with_stars_gc = (uint32_t *)malloc((unit_max_stars_gc + 1) *
                                                  sizeof(uint32_t)))) {
        ERR_LOG("���䶯̬�ڴ�� unit ptr table: %s",
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
        ERR_LOG("�޷����� unit ����ڴ�ռ�: %s", strerror(errno));
        return -1;
    }

    /* Allocate space for the "pointer" table */
    if(!(units_with_stars_bb = (uint32_t *)malloc((unit_max_stars_bb + 1) *
                                                  sizeof(uint32_t)))) {
        ERR_LOG("�޷����� unit ptr ����ڴ�ռ�: %s",
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
        SHIPS_LOG("�޷���д units_by_stars_bb ����С: %s",
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
        ERR_LOG("�޷���ȡ v2 PMT %s: %s", fn, strerror(-ucsz));
        return -1;
    }

    /* Read in the ָ����Ч table. */
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
        ERR_LOG("�޷���ȡ GameCube PMT %s: %s", fn, strerror(-ucsz));
        return -1;
    }

    if (!(pmt_tb_offsets_gc = (pmt_table_offsets_v3_t*)malloc(sizeof(pmt_table_offsets_v3_t)))) {
        ERR_LOG("���䶯̬�ڴ�� BB PMT offsets: %s",
            strerror(errno));
        free_safe(pmt_tb_offsets_gc);
        return -2;
    }

    /* Read in the ָ����Ч table. */
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
        ERR_LOG("�޷���ȡ Blue Burst PMT %s: %s", fn, strerror(-ucsz));
        return -1;
    }

#ifdef DEBUG

    display_packet(&ucbuf, ucsz);
    DBG_LOG("%d", ucsz);
    getchar();

#endif // DEBUG

    if (!(pmt_tb_offsets_bb = (pmt_table_offsets_v3_t*)malloc(sizeof(pmt_table_offsets_v3_t)))) {
        ERR_LOG("���䶯̬�ڴ�� BB PMT offsets: %s",
            strerror(errno));
        free_safe(pmt_tb_offsets_bb);
        return -2;
    }

    /* Read in the ָ����Ч table. */
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

    /* Read in the eventitems values... */
    if (read_eventitems_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -17;
    }

    /* Read in the unsealableitems values... */
    if (read_unsealableitems_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -17;
    }

    /* Read in the mag_feed_results values... */
    if (read_mag_feed_results_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -18;
    }

    /* Read in the nonweaponsaledivisors values... */
    if (read_weaponsaledivisors_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -19;
    }

    /* Read in the nonweaponsaledivisors values... */
    if (read_nonweaponsaledivisors_bb(ucbuf, ucsz, pmt_tb_offsets_bb)) {
        free_safe(ucbuf);
        return -20;
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

static void pmt_cleanup_v2(void) {
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

static void pmt_cleanup_gc(void) {
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

static void pmt_cleanup_bb(void) {
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

    for (i = 0; i < num_eventitem_types_bb; ++i) {
        free_safe(eventitem_bb[i]);
    }
    free_safe(eventitem_bb);
    free_safe(num_eventitems_bb);
    eventitem_bb = NULL;
    num_eventitems_bb = NULL;
    num_eventitem_types_bb = 0;
    eventitem_lowest_bb = EMPTY_STRING;

    free_safe(unsealableitems_bb);
    unsealableitems_bb = NULL;
    unsealableitems_max_bb = 0;


    have_bb_pmt = 0;
}

void pmt_cleanup(void) {

    /* ����V2�汾�� ItemPMT���� */
    pmt_cleanup_v2();

    /* ����GC�汾�� ItemPMT���� */
    pmt_cleanup_gc();

    /* ����BB�汾�� ItemPMT���� */
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

    /* ȷ���������ڲ��� weapon */
    if(parts[0] != ITEM_TYPE_WEAPON) {
        return -2;
    }

    /* ȷ���������κεط�����Խ�� */
    if(parts[1] > num_weapon_types) {
        return -3;
    }

    if(parts[2] >= num_weapons[parts[1]]) {
        return -4;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� guard item */
    if(parts[0] != ITEM_TYPE_GUARD) {
        return -2;
    }

    /* Make sure its not a unit */
    if(parts[1] == ITEM_SUBTYPE_UNIT) {
        return -3;
    }

    /* ȷ���������κεط�����Խ�� */
    if(parts[1] > num_guard_types) {
        return -4;
    }

    if(parts[2] >= num_guards[parts[1] - 1]) {
        return -5;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� unit */
    if(parts[0] != ITEM_TYPE_GUARD || parts[1] != ITEM_SUBTYPE_UNIT) {
        return -2;
    }

    if(parts[2] >= num_units) {
        return -3;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� weapon */
    if(parts[0] != ITEM_TYPE_WEAPON) {
        return -2;
    }

    /* ȷ���������κεط�����Խ�� */
    if(parts[1] > num_weapon_types_gc) {
        return -3;
    }

    if(parts[2] >= num_weapons_gc[parts[1]]) {
        return -4;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� guard item */
    if(parts[0] != ITEM_TYPE_GUARD) {
        return -2;
    }

    /* Make sure its not a unit */
    if(parts[1] == ITEM_SUBTYPE_UNIT) {
        return -3;
    }

    /* ȷ���������κεط�����Խ�� */
    if(parts[1] > num_guard_types_gc) {
        return -4;
    }

    if(parts[2] >= num_guards_gc[parts[1] - 1]) {
        return -5;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� unit */
    if(parts[0] != ITEM_TYPE_GUARD || parts[1] != ITEM_SUBTYPE_UNIT) {
        return -2;
    }

    if(parts[2] >= num_units_gc) {
        return -3;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� weapon */
    if(parts[0] != ITEM_TYPE_WEAPON) {
        return -2;
    }

    /* ȷ���������κεط�����Խ�� */
    if(parts[1] > num_weapon_types_bb) {
        return -3;
    }

    if(parts[2] >= num_weapons_bb[parts[1]]) {
        return -4;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� guard item */
    if(parts[0] != ITEM_TYPE_GUARD) {
        return -2;
    }

    /* Make sure its not a unit */
    if(parts[1] == ITEM_SUBTYPE_UNIT) {
        return -3;
    }

    /* ȷ���������κεط�����Խ�� */
    if(parts[1] > num_guard_types_bb) {
        return -4;
    }

    if(parts[2] >= num_guards_bb[parts[1] - 1]) {
        return -5;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� unit */
    if(parts[0] != ITEM_TYPE_GUARD || parts[1] != ITEM_SUBTYPE_UNIT) {
        return -2;
    }

    if(parts[2] >= num_units_bb) {
        return -3;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
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

    /* ȷ���������ڲ��� guard item */
    if (parts[0] != ITEM_TYPE_MAG) {
        return -2;
    }

    /* ȷ���������κεط�����Խ�� */
    if (parts[1] > num_mags_bb) {
        return -3;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
    memcpy(rv, &mags_bb[parts[1]], sizeof(pmt_mag_bb_t));
    return 0;
}

int pmt_lookup_tools_bb(uint32_t code1, uint32_t code2, pmt_tool_bb_t* rv) {
    uint8_t parts[4] = { 0 };
    uint8_t parts2[4] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if (!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code1 & 0xFF);
    parts[1] = (uint8_t)((code1 >> 8) & 0xFF);
    parts[2] = (uint8_t)((code1 >> 16) & 0xFF);
    parts[3] = (uint8_t)((code1 >> 24) & 0xFF);

    parts2[0] = (uint8_t)(code2 & 0xFF);
    parts2[1] = (uint8_t)((code2 >> 8) & 0xFF);
    parts2[2] = (uint8_t)((code2 >> 16) & 0xFF);
    parts2[3] = (uint8_t)((code2 >> 24) & 0xFF);

#ifdef DEBUG
    DBG_LOG("��Ʒ, used_item 0x%08X 0x%02X 0x%02X 0x%02X",
        code,
        parts[0],
        parts[1],
        parts[2]);
#endif // DEBUG


    /* ȷ���������ڲ��� weapon */
    if (parts[0] != ITEM_TYPE_TOOL) {
        return -2;
    }

    /* ȷ���������κεط�����Խ�� */
    if (parts[1] > num_tool_types_bb) {
        return -3;
    }

    if (parts[1] == ITEM_SUBTYPE_DISK)
        parts[2] = parts2[0];

#ifdef DEBUG
    DBG_LOG("index 0x%08X", tools_bb[parts[1]][parts[2]].base.index);
    DBG_LOG("subtype 0x%04X", tools_bb[parts[1]][parts[2]].base.subtype);
    DBG_LOG("skin 0x%04X", tools_bb[parts[1]][parts[2]].base.skin);
    DBG_LOG("team_points 0x%08X", tools_bb[parts[1]][parts[2]].base.team_points);
#endif // DEBUG

    /* ��ȡ���ݲ����临�Ƴ��� */
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
    DBG_LOG("��Ʒ1, used_item 0x%08X 0x%02X 0x%02X 0x%02X",
        code,
        parts[0],
        parts[1],
        parts[2]);

    DBG_LOG("��Ʒ2, equipped_item 0x%08X 0x%02X 0x%02X 0x%02X",
        equip_code,
        eparts[0],
        eparts[1],
        eparts[2]);
#endif // DEBUG

    for (i; i < itemcombinations_max_bb;i++) {
        if (parts[0] == itemcombination_bb[i].used_item[0] && 
            parts[1] == itemcombination_bb[i].used_item[1]) {

            if (itemcombination_bb[i].used_item[2] != 0xFF)
                if (parts[2] != itemcombination_bb[i].used_item[2])
                    continue;

            if (eparts[0] != itemcombination_bb[i].equipped_item[0] &&
                eparts[1] != itemcombination_bb[i].equipped_item[1]) {
                continue;
            }

            /* TODO �Ƿ�Ҫ��һ���ļ�ǿ���� ���ߺϳ�ʧ�ܣ� */
            switch (eparts[0]) {
            case ITEM_TYPE_WEAPON:
#ifdef DEBUG
                DBG_LOG("ITEM_TYPE_WEAPON");
#endif // DEBUG
                if(eparts[2] == itemcombination_bb[i].equipped_item[2])
                    /* ��ȡ���ݲ����临�Ƴ��� */
                    memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                else
                    continue;

                break;

            case ITEM_TYPE_GUARD:

                if (eparts[2] == itemcombination_bb[i].equipped_item[2]) {

#ifdef DEBUG
                    DBG_LOG("ITEM_TYPE_WEAPON");
#endif // DEBUG
                    switch (eparts[1]) {
                    case ITEM_SUBTYPE_FRAME:
#ifdef DEBUG
                        DBG_LOG("ITEM_TYPE_WEAPON");
#endif // DEBUG
                        /* ��ȡ���ݲ����临�Ƴ��� */
                        memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                        break;

                    case ITEM_SUBTYPE_BARRIER:
#ifdef DEBUG
                        DBG_LOG("ITEM_TYPE_WEAPON");
#endif // DEBUG
                        /* ��ȡ���ݲ����临�Ƴ��� */
                        memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                        break;
                    }
                }
                else
                    continue;

                break;

            case ITEM_TYPE_MAG:
#ifdef DEBUG
                DBG_LOG("ITEM_TYPE_MAG");
#endif // DEBUG
                /* ��ȡ���ݲ����临�Ƴ��� */
                memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                break;
                
                /* TODO �������û�й�����Ʒ�ϳɵ���Ϣ */
            case ITEM_TYPE_TOOL:
#ifdef DEBUG
                DBG_LOG("ITEM_TYPE_TOOL");
#endif // DEBUG
                /* ��ȡ���ݲ����临�Ƴ��� */
                memcpy(rv, &itemcombination_bb[i], sizeof(pmt_itemcombination_bb_t));
                break;
            }

            break;
        }
    }

    if (i >= itemcombinations_max_bb) {
#ifdef DEBUG
        ERR_LOG("������� result_item 0x%02X 0x%02X 0x%02X",
            itemcombination_bb[i].result_item[0],
            itemcombination_bb[i].result_item[1],
            itemcombination_bb[i].result_item[2]);
#endif // DEBUG
        return -1;
    }
#ifdef DEBUG
    else
        DBG_LOG("���, result_item 0x%02X 0x%02X 0x%02X",
            itemcombination_bb[i].result_item[0],
            itemcombination_bb[i].result_item[1],
            itemcombination_bb[i].result_item[2]);
#endif // DEBUG

    return 0;
}

int pmt_lookup_eventitem_bb(uint32_t code, pmt_eventitem_bb_t* rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if (!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* ȷ���������ڲ��� ʥ����Ʒ */
    if (parts[0] != ITEM_TYPE_TOOL) {
        return -2;
    }

    /* ȷ���������κεط�����Խ�� */
    if (parts[1] != 0x15) {
        return -3;
    }

    /* ȷ���������κεط�����Խ�� */
    if (parts[2] > 0x02) {
        return -4;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
    memcpy(rv, &eventitem_bb[parts[2]], sizeof(pmt_eventitem_bb_t));
    return 0;
}

int pmt_lookup_mag_feed_table_bb(uint32_t code, uint32_t table_index, uint32_t item_index, pmt_mag_feed_result_t* rv) {
    uint8_t parts[3] = { 0 };

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if (!have_bb_pmt || !rv) {
        return -1;
    }

    parts[0] = (uint8_t)(code & 0xFF);
    parts[1] = (uint8_t)((code >> 8) & 0xFF);
    parts[2] = (uint8_t)((code >> 16) & 0xFF);

    /* ȷ���������ڲ��� ʥ����Ʒ */
    if (parts[0] != ITEM_TYPE_MAG) {
        return -2;
    }

    if (table_index >= 8) {
        return -3;
    }
    if (item_index >= 11) {
        return -4;
    }

    /* ��ȡ���ݲ����临�Ƴ��� */
    memcpy(rv, &mag_feed_results_list[table_index][item_index], sizeof(pmt_mag_feed_result_t));
    return 0;
}

float pmt_lookup_sale_divisor_bb(uint8_t code1, uint8_t code2) {

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if (!have_bb_pmt) {
        return 0;
    }

    switch (code1)
    {
    case ITEM_TYPE_WEAPON:
        if (code2 < 0xA5) {
            return weaponsaledivisors_bb[code2];
        }
        return 0.0f;

    case ITEM_TYPE_GUARD:
        switch (code2) {
        case ITEM_SUBTYPE_FRAME:
            return nonweaponsaledivisors_bb.armor_divisor;
        case ITEM_SUBTYPE_BARRIER:
            return nonweaponsaledivisors_bb.shield_divisor;
        case ITEM_SUBTYPE_UNIT:
            return nonweaponsaledivisors_bb.unit_divisor;
        }
        return 0.0f;

    case ITEM_TYPE_MAG:
        return nonweaponsaledivisors_bb.mag_divisor;
    }

    return 0.0f;
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

            if(weap.base.index - weapon_lowest_bb > star_max_bb)
                return (uint8_t)-1;

            return star_table_bb[weap.base.index - weapon_lowest_bb];

        case ITEM_TYPE_GUARD:                         /* Guards */
            switch(parts[1]) {
                case ITEM_SUBTYPE_FRAME:              /* Armors */
                case ITEM_SUBTYPE_BARRIER:            /* Shields */
                    if(pmt_lookup_guard_bb(code, &guard))
                        return (uint8_t)-1;

                    if(guard.base.index - weapon_lowest_bb > star_max_bb)
                        return (uint8_t)-1;

                    return star_table_bb[guard.base.index - weapon_lowest_bb];

                case ITEM_SUBTYPE_UNIT:               /* Units */
                    if(pmt_lookup_unit_bb(code, &unit))
                        return (uint8_t)-1;

                    if(unit.base.index - weapon_lowest_bb > star_max_bb)
                        return (uint8_t)-1;

                    return star_table_bb[unit.base.index - weapon_lowest_bb];
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
                       sfmt_t* rng, lobby_t *l) {
    uint64_t unit;
    uint32_t rnd = sfmt_genrand_uint32(rng);

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
                       sfmt_t* rng, lobby_t *l) {
    uint64_t unit;
    uint32_t rnd = sfmt_genrand_uint32(rng);

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
                       sfmt_t* rng, lobby_t *l) {
    uint64_t unit;
    uint32_t rnd = sfmt_genrand_uint32(rng);

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

pmt_item_base_t get_item_definition_bb(const uint32_t datal1, const uint32_t datal2) {
    errno_t err = 0;
    pmt_item_base_t item_base = { 0 };
    uint8_t parts[4] = { 0 };

    parts[0] = (uint8_t)(datal1 & 0xFF);
    parts[1] = (uint8_t)((datal1 >> 8) & 0xFF);
    parts[2] = (uint8_t)((datal1 >> 16) & 0xFF);
    parts[3] = (uint8_t)((datal1 >> 24) & 0xFF);

    switch (parts[0]) {
    case ITEM_TYPE_WEAPON:
        pmt_weapon_bb_t weapon = { 0 };
        if (err = pmt_lookup_weapon_bb(datal1, &weapon)) {
            ERR_LOG("pmt_lookup_weapon_bb ����������! ������ %d", err);
            return item_base;
        }
        memcpy(&item_base, &weapon.base, sizeof(pmt_item_base_t));
        return item_base;

    case ITEM_TYPE_GUARD:
        switch (parts[1]) {
        case ITEM_SUBTYPE_FRAME:
        case ITEM_SUBTYPE_BARRIER:
            pmt_guard_bb_t guard = { 0 };
            if (err = pmt_lookup_guard_bb(datal1, &guard)) {
                ERR_LOG("pmt_lookup_unit_bb ����������! ������ %d", err);
                return item_base;
            }
            memcpy(&item_base, &guard.base, sizeof(pmt_item_base_t));
            return item_base;

        case ITEM_SUBTYPE_UNIT:
            pmt_unit_bb_t unit = { 0 };
            if (err = pmt_lookup_unit_bb(datal1, &unit)) {
                ERR_LOG("pmt_lookup_unit_bb ����������! ������ %d", err);
                return item_base;
            }
            memcpy(&item_base, &unit.base, sizeof(pmt_item_base_t));
            return item_base;
        }
        ERR_LOG("��Ч��Ʒ 0x%08X", datal1);
        break;

    case ITEM_TYPE_MAG:
        pmt_mag_bb_t mag = { 0 };
        if (err = pmt_lookup_mag_bb(datal1, &mag)) {
            ERR_LOG("pmt_lookup_unit_bb ����������! ������ %d", err);
            return item_base;
        }
        memcpy(&item_base, &mag.base, sizeof(pmt_item_base_t));
        return item_base;

    case ITEM_TYPE_TOOL:
        pmt_tool_bb_t tool = { 0 };
        if (err = pmt_lookup_tools_bb(datal1, datal2, &tool)) {
            ERR_LOG("pmt_lookup_unit_bb ����������! ������ %d", err);
            return item_base;
        }

        if (!&tool) {
            ERR_LOG("�������� 0x%08X", datal1);
            break;
        }

        memcpy(&item_base, &tool.base, sizeof(pmt_item_base_t));
        return item_base;

    case ITEM_TYPE_MESETA:
        ERR_LOG("������֮�����Ʒû�ж���");
        break;

    default:
        ERR_LOG("��Ч��Ʒ 0x%08X", datal1);
        break;
    }

    return item_base;
}

bool item_not_identification(const item_t* item) {
    pmt_item_base_t item_base = get_item_definition_bb(item->datal[0], item->datal[1]);
    return (&item_base == NULL);
}

uint8_t get_item_stars(uint16_t index) {
    if ((index >= 0xB1) && (index <= 0x3E0)) {
#ifdef DEBUG
        uint8_t star = star_table_bb[index - 0xB1];
        DBG_LOG("%d", star);
#endif // DEBUG
        return star_table_bb[index - 0xB1];
    }
    return 0;
}

uint8_t get_item_base_stars(const item_t* item) {
    uint8_t star = 0;

    switch (item->datab[0]) {
    case ITEM_TYPE_WEAPON:
    case ITEM_TYPE_GUARD:
        pmt_item_base_t base = get_item_definition_bb(item->datal[0], item->datal[1]);
        if (!&base) {
            DBG_LOG("��Ʒ������Ϣ����");
            print_item_data(item, 5);
            return 0;
        }

        uint32_t id = base.index;
        star = get_item_stars(id);
        break;

    case ITEM_TYPE_MAG:
        star = (item->datab[1] > 0x27) ? 12 : 0;
        break;

    case ITEM_TYPE_TOOL:
        pmt_tool_bb_t def = { 0 };

        if (pmt_lookup_tools_bb(item->datal[0], item->datal[1], &def)) {
            ERR_LOG("��������Ʒ����! 0x%08X", item->datal[0]);
            return -1;
        }

        star = (def.itemFlag & 0x80) ? 12 : 0;
        break;
    }

    return star;

}

uint8_t get_special_stars(uint8_t det) {
    if (!(det & 0x3F) || (det & 0x80)) {
        return 0;
    }
    // Note: PSO GC uses 0x1CB here. 0x256 was chosen to point to the same data in
    // PSO BB's ItemPMT file.
    return get_item_stars(det + 0x0256);
}

uint8_t get_item_adjusted_stars(const item_t* item) {
    uint8_t ret = get_item_base_stars(item);
    if (item->datab[0] == ITEM_TYPE_WEAPON) {
        if (ret < 9) {
            if (!(item->datab[4] & 0x80)) {
                ret += get_special_stars(item->datab[4]);
            }
        }
        else if (item->datab[4] & 0x80) {
            ret = 0;
        }
    }
    else if (item->datab[0] == 1) {
        if (item->datab[1] == 3) {
            int16_t unit_bonus = get_unit_bonus(item);
            if (unit_bonus < 0) {
                ret--;
            }
            else if (unit_bonus > 0) {
                ret++;
            }
        }
    }
    return min(ret, 12);
}

bool is_item_rare(const item_t* item) {
    uint8_t item_base_star = get_item_base_stars(item);
    if (item_base_star == 0xFF)
        item_base_star = ITEM_BASE_STAR_DEFAULT;
#ifdef DEBUG
    DBG_LOG("item_base_star %d", item_base_star);
#endif // DEBUG
    bool is_rare = (item_base_star >= ITEM_RARE_THRESHOLD);
    return is_rare;
}



