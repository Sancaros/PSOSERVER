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

#include "mageditdata.h"
#include "pso_items.h"

static int read_magedit_tbl(const uint8_t* pmt, uint32_t sz, magedit_table_offsets_t* ptrs) {
    uint32_t tmp;
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    uint32_t i;
#endif

    memcpy(&tmp, pmt + sz - 16, sizeof(uint32_t));
    tmp = LE32(tmp);

    if (tmp + sizeof(magedit_table_offsets_t) > sz - 16) {
        ERR_LOG("BB ItemPMT 中的指针表位置无效!");
        return -1;
    }

    memcpy(ptrs->ptr, pmt + tmp, sizeof(magedit_table_offsets_t));

#ifdef DEBUG

    DBG_LOG("evolution_number_offset offsets = 0x%08X", ptrs->evolution_number_offset);

    for (int i = 0; i < 6; ++i) {
        DBG_LOG("offsets = 0x%08X", ptrs->ptr[i]);
    }

#endif // DEBUG
    

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    for (int i = 0; i < 6; ++i) {
        ptrs.ptr[i] = LE32(ptrs.ptr[i]);
    }
#endif

    return 0;
}

static int read_mag_positions(const uint8_t* pmt, uint32_t sz,
    const magedit_table_offsets_t* ptrs) {
    size_t i = 0;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->position_table > sz) {
        ERR_LOG("ItemMagEdit.prs 文件的 position_table 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the data... */
    memcpy(&mag_positions_table, pmt + 0x0000001C, sizeof(mag_position_table_t));

#ifdef DEBUG

    for (i = 0; i < 0x53; ++i) {
        DBG_LOG("/////////////////////");
        DBG_LOG("第 %d 行", i);
        DBG_LOG("mag2_normal        %d", mag_positions_table.value[i].mag2_normal);
        DBG_LOG("mag2_unknow1       %d", mag_positions_table.value[i].mag2_unknow1);
        DBG_LOG("mag2_feed          %d", mag_positions_table.value[i].mag2_feed);
        DBG_LOG("mag2_activate      %d", mag_positions_table.value[i].mag2_activate);
        DBG_LOG("mag2_unknow2       %d", mag_positions_table.value[i].mag2_unknow2);
        DBG_LOG("mag2_unknow3       %d", mag_positions_table.value[i].mag2_unknow3);
        DBG_LOG("mag1_normal        %d", mag_positions_table.value[i].mag1_normal);
        DBG_LOG("mag1_unknow1       %d", mag_positions_table.value[i].mag1_unknow1);
        DBG_LOG("mag1_feed          %d", mag_positions_table.value[i].mag1_feed);
        DBG_LOG("mag1_activate      %d", mag_positions_table.value[i].mag1_activate);
        DBG_LOG("mag1_unknow2       %d", mag_positions_table.value[i].mag1_unknow2);
        DBG_LOG("mag1_unknow3       %d", mag_positions_table.value[i].mag1_unknow3);
        DBG_LOG("/////////////////////");

        if (i == 2)
            getchar();

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }
#endif // DEBUG

    return 0;
}

static int read_mag_movements(const uint8_t* pmt, uint32_t sz,
    const magedit_table_offsets_t* ptrs) {
    size_t i = 0;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->movement_table > sz) {
        ERR_LOG("ItemMagEdit.prs 文件的 movement_table 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the data... */
    memcpy(&mag_movements_table, pmt + ptrs->movement_table, sizeof(mag_movement_table_t));

#ifdef DEBUG

    for (i = 0; i < 0x53; ++i) {
        DBG_LOG("movement1      %d", mag_movements_table.value[i].movement1);
        DBG_LOG("movement2      %d", mag_movements_table.value[i].movement2);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }
    getchar();

#endif // DEBUG

    return 0;
}

static int read_mag_movement_configs(const uint8_t* pmt, uint32_t sz,
    const magedit_table_offsets_t* ptrs) {
    size_t i = 0;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->movement_config_table > sz) {
        ERR_LOG("ItemMagEdit.prs 文件的 movement_config_table 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the data... */
    memcpy(&mag_movement_configs_table, pmt + ptrs->movement_config_table, sizeof(mag_movement_config_table_t));

#ifdef DEBUG

    for (i = 0; i < 0x15; ++i) {
        DBG_LOG("mesh_id      %u", mag_movement_configs_table.value[i].mesh_id);
        DBG_LOG("speed        %u", mag_movement_configs_table.value[i].speed);
        DBG_LOG("start_angle  %u", mag_movement_configs_table.value[i].start_angle);
        DBG_LOG("point1_angle %u", mag_movement_configs_table.value[i].point1_angle);
        DBG_LOG("point2_angle %u", mag_movement_configs_table.value[i].point2_angle);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }
#endif // DEBUG

    return 0;
}

static int read_mag_mesh_to_colors(const uint8_t* pmt, uint32_t sz,
    const magedit_table_offsets_t* ptrs) {
    size_t i = 0;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->mesh_to_color_table > sz) {
        ERR_LOG("ItemMagEdit.prs 文件的 mesh_to_color_table 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the data... */
    memcpy(&mag_mesh_to_colors_table, pmt + ptrs->mesh_to_color_table, sizeof(mag_mesh_to_color_table_t));

#ifdef DEBUG

    for (i = 0; i < 83; ++i) {
        DBG_LOG("values %d", mag_mesh_to_colors_table.values[i]);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }

#endif // DEBUG

    return 0;
}

static int read_mag_colors(const uint8_t* pmt, uint32_t sz,
    const magedit_table_offsets_t* ptrs) {
    size_t i = 0;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->colors_table > sz) {
        ERR_LOG("ItemMagEdit.prs 文件的 colors_table 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the data... */
    memcpy(&mag_colors_table, pmt + ptrs->colors_table, sizeof(mag_color_table_t));

#ifdef DEBUG
    
    for (i = 0; i < 18; ++i) {
        DBG_LOG("alpha %f", mag_colors_table.value[i].alpha);
        DBG_LOG("red   %f", mag_colors_table.value[i].red);
        DBG_LOG("green %f", mag_colors_table.value[i].green);
        DBG_LOG("blue  %f", mag_colors_table.value[i].blue);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }
#endif // DEBUG

    return 0;
}

static int read_mag_evolution_numbers(const uint8_t* pmt, uint32_t sz,
    const magedit_table_offsets_t* ptrs) {
    size_t i = 0;

    /* Make sure the 指针无效 are sane... */
    if (ptrs->evolution_number_table > sz) {
        ERR_LOG("ItemMagEdit.prs 文件的 evolution_number_table 指针无效. "
            "请检查其有效性!");
        return -1;
    }

    /* Read the data... */
    memcpy(&mag_evolution_numbers_table, pmt + ptrs->evolution_number_table, sizeof(mag_evolution_number_table_t));

#ifdef DEBUG

    for (i = 0; i < 83; ++i) {
        DBG_LOG("values %d", mag_evolution_numbers_table.values[i]);
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
#endif
    }

#endif // DEBUG

    return 0;
}

int magedit_read_bb(const char* fn, int norestrict) {
    int ucsz;
    uint8_t* ucbuf;

    /* Read in the file and decompress it. */
    if ((ucsz = pso_prs_decompress_file(fn, &ucbuf)) < 0) {
        ERR_LOG("无法读取 MagEdit %s: %s", fn, strerror(-ucsz));
        return -1;
    }

    if (!(magedit_tb_offsets = (magedit_table_offsets_t*)malloc(sizeof(magedit_table_offsets_t)))) {
        ERR_LOG("分配动态内存给 MagEdit offsets: %s",
            strerror(errno));
        free_safe(magedit_tb_offsets);
        return -2;
    }

    /* Read in the 指针无效 table. */
    if (read_magedit_tbl(ucbuf, ucsz, magedit_tb_offsets)) {
        free_safe(ucbuf);
        return -3;
    }

    /* read_mag_positions */
    if (read_mag_positions(ucbuf, ucsz, magedit_tb_offsets)) {
        free_safe(ucbuf);
        return -4;
    }

    /* read_mag_movements */
    if (read_mag_movements(ucbuf, ucsz, magedit_tb_offsets)) {
        free_safe(ucbuf);
        return -5;
    }

    /* read_mag_movement_configs */
    if (read_mag_movement_configs(ucbuf, ucsz, magedit_tb_offsets)) {
        free_safe(ucbuf);
        return -6;
    }

    /* read_mag_mesh_to_color */
    if (read_mag_mesh_to_colors(ucbuf, ucsz, magedit_tb_offsets)) {
        free_safe(ucbuf);
        return -7;
    }

    /* read_mag_colors */
    if (read_mag_colors(ucbuf, ucsz, magedit_tb_offsets)) {
        free_safe(ucbuf);
        return -8;
    }

    /* read_mag_evolution_number */
    if (read_mag_evolution_numbers(ucbuf, ucsz, magedit_tb_offsets)) {
        free_safe(ucbuf);
        return -9;
    }

    /* We're done with the raw MagEdit data now, clean it up. */
    free_safe(ucbuf);

    have_bb_magedit = 1;

    return 0;
}

uint8_t magedit_lookup_mag_evolution_number(iitem_t* iitem) {

    /* Make sure we loaded the PMT stuff to start with and that there is a place
       to put the returned value */
    if (!have_bb_magedit || !iitem) {
        return -1;
    }

    /* 确保我们正在查找 圣诞物品 */
    if (iitem->data.datab[0] != ITEM_TYPE_MAG) {
        return -2;
    }

    /* 获取数据 */
    return mag_evolution_numbers_table.values[iitem->data.datab[1]];
}

