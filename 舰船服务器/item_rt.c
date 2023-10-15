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

#include <SFMT.h>

#include <AFS.h>
#include <GSL.h>
#include <PRS.h>

#include "f_logs.h"

#include "rtdata.h"
#include "ship_packets.h"
#include "lobby.h"

char abbreviation_for_difficulty(uint8_t difficulty) {
    char abbreviation = '?';

    switch (difficulty) {
    case 0:
        abbreviation = 'N';
        break;
    case 1:
        abbreviation = 'H';
        break;
    case 2:
        abbreviation = 'V';
        break;
    case 3:
        abbreviation = 'U';
        break;
    default:
        // 在给定的范围之外，默认返回 '?'
        abbreviation = '?';
        break;
    }

    return abbreviation;
}

int rt_read_v2(const char *fn) {
    FILE *fp;
    uint8_t buf[30];
    int rv = 0, i, j, k;
    uint32_t offsets[40] = { 0 }, tmp;
    rt_entry_t ent;

    have_v2rt = 0;

    /* Open up the file */
    if(!(fp = fopen(fn, "rb"))) {
        ERR_LOG("无法打开 %s 文件: %s", fn, strerror(errno));
        return -1;
    }

    /* Make sure that it looks like a sane AFS file. */
    if(fread(buf, 1, 4, fp) != 4) {
        ERR_LOG("读取文件错误: %s", strerror(errno));
        rv = -2;
        goto out;
    }

    if(buf[0] != 0x41 || buf[1] != 0x46 || buf[2] != 0x53 || buf[3] != 0x00) {
        ERR_LOG("%s is not an AFS archive!", fn);
        rv = -3;
        goto out;
    }

    /* Make sure there are exactly 40 entries */
    if(fread(buf, 1, 4, fp) != 4) {
        ERR_LOG("读取文件错误: %s", strerror(errno));
        rv = -2;
        goto out;
    }

    if(buf[0] != 40 || buf[1] != 0 || buf[2] != 0 || buf[3] != 0) {
        ERR_LOG("%s does not appear to be an ItemRT.afs file", fn);
        rv = -4;
        goto out;
    }

    /* Read in the offsets and lengths */
    for(i = 0; i < 40; ++i) {
        if(fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("读取文件错误: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        offsets[i] = (buf[0]) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

        if(fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("读取文件错误: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        if(buf[0] != 0x80 || buf[1] != 0x02 || buf[2] != 0 || buf[3] != 0) {
            ERR_LOG("Invalid sized entry in ItemRT.afs!");
            rv = 5;
            goto out;
        }
    }

    /* Now, parse each entry... */
    for(i = 0; i < 4; ++i) {
        for(j = 0; j < 10; ++j) {
            if(fseek(fp, (long)offsets[i * 10 + j], SEEK_SET)) {
                ERR_LOG("fseek错误: %s", strerror(errno));
                rv = -2;
                goto out;
            }

            /* Read in the enemy entries */
            for(k = 0; k < 0x65; ++k) {
                if(fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                   sizeof(rt_entry_t)) {
                    ERR_LOG("读取 ItemRT 文件错误: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                    (ent.item_data[2] << 16);
                v2_rtdata[i][j].enemy_rares[k].prob = expand_rate(ent.prob);
                v2_rtdata[i][j].enemy_rares[k].item_data = tmp;
                v2_rtdata[i][j].enemy_rares[k].area = 0; /* Unused */
            }

            /* Read in the box entries */
            if(fread(buf, 1, 30, fp) != 30) {
                ERR_LOG("读取 ItemRT 文件错误: %s", strerror(errno));
                rv = -2;
                goto out;
            }

            for(k = 0; k < 30; ++k) {
                if(fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                   sizeof(rt_entry_t)) {
                    ERR_LOG("读取 ItemRT 文件错误: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                    (ent.item_data[2] << 16);
                v2_rtdata[i][j].box_rares[k].prob = expand_rate(ent.prob);
                v2_rtdata[i][j].box_rares[k].item_data = tmp;
                v2_rtdata[i][j].box_rares[k].area = buf[k];
            }
        }
    }

    have_v2rt = 1;

out:
    fclose(fp);
    return rv;
}

int rt_read_gc(const char *fn) {
    FILE *fp;
    uint8_t buf[30];
    int rv = 0, i, j, k, l;
    uint32_t offsets[80] = { 0 }, tmp;
    rt_entry_t ent;

    have_gcrt = 0;

    /* Open up the file */
    if(!(fp = fopen(fn, "rb"))) {
        ERR_LOG("无法打开 ItemRT 文件 %s: %s", fn, strerror(errno));
        return -1;
    }

    /* Read in the offsets and lengths for the Episode I & II data. */
    for(i = 0; i < 80; ++i) {
        if(fseek(fp, 32, SEEK_CUR)) {
            ERR_LOG("fseek错误: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        if(fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("读取文件错误: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        /* The offsets are in 2048 byte blocks. */
        offsets[i] = (buf[3]) | (buf[2] << 8) | (buf[1] << 16) | (buf[0] << 24);
        offsets[i] <<= 11;

        if(fread(buf, 1, 4, fp) != 4) {
            ERR_LOG("读取文件错误: %s", strerror(errno));
            rv = -2;
            goto out;
        }

        if(buf[0] != 0 || buf[1] != 0 || buf[2] != 0x02 || buf[3] != 0x80) {
            ERR_LOG("ItemRT.gsl 中的条目大小无效!");
            rv = 5;
            goto out;
        }

        /* Skip over the padding. */
        if(fseek(fp, 8, SEEK_CUR)) {
            ERR_LOG("fseek错误: %s", strerror(errno));
            rv = -2;
            goto out;
        }
    }

    /* Now, parse each entry... */
    for(i = 0; i < 2; ++i) {
        for(j = 0; j < 4; ++j) {
            for(k = 0; k < 10; ++k) {
                if(fseek(fp, (long)offsets[i * 40 + j * 10 + k], SEEK_SET)) {
                    ERR_LOG("fseek错误: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                /* Read in the enemy entries */
                for(l = 0; l < 0x65; ++l) {
                    if(fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                       sizeof(rt_entry_t)) {
                        ERR_LOG("读取 ItemRT 文件错误: %s",
                              strerror(errno));
                        rv = -2;
                        goto out;
                    }

                    tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                        (ent.item_data[2] << 16);
                    gc_rtdata[i][j][k].enemy_rares[l].prob =
                        expand_rate(ent.prob);
                    gc_rtdata[i][j][k].enemy_rares[l].item_data = tmp;
                    gc_rtdata[i][j][k].enemy_rares[l].area = 0; /* Unused */
                }

                /* Read in the box entries */
                if(fread(buf, 1, 30, fp) != 30) {
                    ERR_LOG("读取 ItemRT 文件错误: %s", strerror(errno));
                    rv = -2;
                    goto out;
                }

                for(l = 0; l < 30; ++l) {
                    if(fread(&ent, 1, sizeof(rt_entry_t), fp) !=
                       sizeof(rt_entry_t)) {
                        ERR_LOG("读取 ItemRT 文件错误: %s",
                              strerror(errno));
                        rv = -2;
                        goto out;
                    }

                    tmp = ent.item_data[0] | (ent.item_data[1] << 8) |
                        (ent.item_data[2] << 16);
                    gc_rtdata[i][j][k].box_rares[l].prob =
                        expand_rate(ent.prob);
                    gc_rtdata[i][j][k].box_rares[l].item_data = tmp;
                    gc_rtdata[i][j][k].box_rares[l].area = buf[k];
                }
            }
        }
    }

    have_gcrt = 1;

out:
    fclose(fp);
    return rv;
}

int rt_read_bb(const char* fn) {
    pso_gsl_read_t* a;
    const char difficulties[4] = { 'n', 'h', 'v', 'u' };
    // 0  NULL / EP2 1  l /  CHALLENGE1 2 c / CHALLENGE2 3 cl / EP4 4 bb
    const char* game_ep_rt_index[4] = { "", "l" , "c", "bb" };
    char filename[32];
    uint32_t hnd;
    size_t sz = sizeof(rt_table_t);
    pso_error_t err;
    int rv = 0, 章节, 难度, 颜色/*, z*/;
    size_t i = 0, j = 0;
    rt_table_t* buf;
    rt_table_t* ent;

    if (!(buf = (rt_table_t*)malloc(sizeof(rt_table_t)))) {
        ERR_LOG("无法为 ItemRT 数据条目分配内存空间!");
        return -6;
    }

    /* Open up the file and make sure it looks sane enough... */
    if (!(a = pso_gsl_read_open(fn, 0, &err))) {
        ERR_LOG("无法读取 BB %s: %s", fn, pso_strerror(err));
        free_safe(buf);
        return -1;
    }

    /* Make sure the archive has the correct number of entries. */
    if (pso_gsl_file_count(a) != 0xA0) {
        ERR_LOG("%s 数据似乎不完整. 读取字节 0x%zX", fn, pso_gsl_file_count(a));
        rv = -2;
        goto out;
    }

    /* Now, parse each entry... */
    for (章节 = 0; 章节 < 4; ++章节) {
        for (难度 = 0; 难度 < 4; ++难度) {
            for (颜色 = 0; 颜色 < 10; ++颜色) {
                /* Figure out the name of the file in the archive that we're
                   looking for... */
                snprintf(filename, 32, "ItemRT%s%c%d.rel", game_ep_rt_index[章节],
                    tolower(abbreviation_for_difficulty(难度)), 颜色);

#ifdef DEBUG

                DBG_LOG("%s | 章节 %d 难度 %d 颜色 %d", filename, 章节, 难度, 颜色);

#endif // DEBUG

                /* Grab a handle to that file. */
                hnd = pso_gsl_file_lookup(a, filename);
                if (hnd == PSOARCHIVE_HND_INVALID) {
                    ERR_LOG("%s 缺少 %s 文件!", fn, filename);
                    rv = -3;
                    goto out;
                }

                /* Make sure the size is correct. */
                if ((err = pso_gsl_file_size(a, hnd)) != 0x280) {
                    ERR_LOG("%s 文件 %s 大小无效! 期待大小 0x%zX", fn,
                        filename, err);
                    rv = -4;
                    goto out;
                }

                if (pso_gsl_file_read(a, hnd, buf, sz) != sz) {
                    ERR_LOG("读取 %s 错误,路径 %s!",
                        filename, fn);
                    rv = -5;
                    goto out;
                }

                /* Dump it into our nicer (not packed) structure. */
                ent = &bb_rtdata[章节][难度][颜色];

                memcpy(ent, buf, sz);

                for (i = 0; i < 0x65; i++) {
                    ent->enemy_rares[i].probability = buf->enemy_rares[i].probability;
                    for (j = 0; j < 3; j++) {
                        ent->enemy_rares[i].item_code[j] = buf->enemy_rares[i].item_code[j];
                    }
                }

                for (i = 0; i < 0x1E; i++) {
                    ent->box_areas[i] = buf->box_areas[i];
                }

                for (i = 0; i < 0x1E; i++) {
                    ent->box_rares[i].probability = buf->box_rares[i].probability;
                    for (j = 0; j < 3; j++) {
                        ent->box_rares[i].item_code[j] = buf->box_rares[i].item_code[j];
                    }
                }

                for (i = 0; i < 2; i++) {
                    ent->unknown_a1[i] = buf->unknown_a1[i];
                }

                ent->enemy_rares_offset = buf->enemy_rares_offset;
                ent->box_count = buf->box_count;
                ent->box_areas_offset = buf->box_areas_offset;
                ent->box_rares_offset = buf->box_rares_offset;
                ent->unused_offset1 = buf->unused_offset1;

                for (i = 0; i < 0x10; i++) {
                    ent->unknown_a2[i] = buf->unknown_a2[i];
                }

                ent->unknown_a2_offset = buf->unknown_a2_offset;
                ent->unknown_a2_count = buf->unknown_a2_count;
                ent->unknown_a3 = buf->unknown_a3;
                ent->unknown_a3 = buf->unknown_a3;
                ent->unknown_a4 = buf->unknown_a4;
                ent->offset_table_offset = buf->offset_table_offset;

                for (i = 0; i < 3; i++) {
                    ent->unknown_a5[i] = buf->unknown_a5[i];
                }

#ifdef DEBUG

                if (章节 == 3) {

                    DBG_LOG("颜色 ID %d", 颜色);
                    DBG_LOG("enemy_rares_offset 0x%08X", ent->enemy_rares_offset);
                    DBG_LOG("box_count 0x%08X", ent->box_count);
                    DBG_LOG("box_areas_offset 0x%08X", ent->box_areas_offset);
                    DBG_LOG("box_rares_offset 0x%08X", ent->box_rares_offset);
                    for (z = 0; z < 0x10; z++) {
                        DBG_LOG("unknown_a2 x %d 0x%04X", z, ent->unknown_a2[z]);
                    }
                    DBG_LOG("unknown_a2_offset 0x%08X", ent->unknown_a2_offset);
                    DBG_LOG("unknown_a2_count 0x%08X", ent->unknown_a2_count);
                    DBG_LOG("unknown_a3 0x%08X", ent->unknown_a3);
                    DBG_LOG("unknown_a4 0x%08X", ent->unknown_a4);
                    DBG_LOG("offset_table_offset 0x%08X", ent->offset_table_offset);
                    for (z = 0; z < 3; z++) {
                        DBG_LOG("unknown_a5 x %d 0x%04X", z, ent->unknown_a5[z]);
                    }
                    //for (size_t x = 0; x < 0x65; x++) {
                    //    DBG_LOG("enemy_rares item_code0 0x%02X", ent->enemy_rares[x].item_code[0]);
                    //    DBG_LOG("enemy_rares item_code1 0x%02X", ent->enemy_rares[x].item_code[1]);
                    //    DBG_LOG("enemy_rares item_code2 0x%02X", ent->enemy_rares[x].item_code[2]);
                    //    DBG_LOG("enemy_rares probability %d %lf", ent->enemy_rares[x].probability, expand_rate(ent->enemy_rares[x].probability));
                    //    DBG_LOG("/////////////////");
                    //}

                    //for (size_t x = 0; x < 0x1E; x++) {
                    //    DBG_LOG("box_rares item_code0 0x%02X", ent->box_rares[x].item_code[0]);
                    //    DBG_LOG("box_rares item_code1 0x%02X", ent->box_rares[x].item_code[1]);
                    //    DBG_LOG("box_rares item_code2 0x%02X", ent->box_rares[x].item_code[2]);
                    //    DBG_LOG("box_rares probability %d %lf", ent->box_rares[x].probability, expand_rate(ent->box_rares[x].probability));
                    //    DBG_LOG("/////////////////");
                    //}
                    getchar();
                }

                DBG_LOG("颜色 ID %d", 颜色);

                for (size_t x = 0; x < 0x65; x++) {
                    DBG_LOG("enemy_rares item_code0 0x%02X", ent->enemy_rares[x].item_code[0]);
                    DBG_LOG("enemy_rares item_code1 0x%02X", ent->enemy_rares[x].item_code[1]);
                    DBG_LOG("enemy_rares item_code2 0x%02X", ent->enemy_rares[x].item_code[2]);
                    DBG_LOG("enemy_rares probability %d %lf", ent->enemy_rares[x].probability, expand_rate(ent->enemy_rares[x].probability));
                    DBG_LOG("/////////////////");
                }

                for (size_t x = 0; x < 0x1E; x++) {
                    DBG_LOG("box_rares item_code0 0x%02X", ent->box_rares[x].item_code[0]);
                    DBG_LOG("box_rares item_code1 0x%02X", ent->box_rares[x].item_code[1]);
                    DBG_LOG("box_rares item_code2 0x%02X", ent->box_rares[x].item_code[2]);
                    DBG_LOG("box_rares probability %d %lf", ent->box_rares[x].probability, expand_rate(ent->box_rares[x].probability));
                    DBG_LOG("/////////////////");
                }

#endif // DEBUG

            }
        }
    }

    have_bbrt = 1;

out:
    pso_gsl_read_close(a);
    free_safe(buf);
    return rv;
}

const char* episodenames[] = { "章节 I", "章节 II", "挑战模式", "章节 IV" };

rt_table_t* rt_dynamics_read_bb(const char* fn, int 章节, int 难度, int 颜色) {
    pso_gsl_read_t* a;
    const char difficulties[4] = { 'n', 'h', 'v', 'u' };
    //EP1 0  NULL / EP2 1  l / CHALLENGE 2 c  / EP4 3 bb
    const char* game_ep_rt_index[4] = { "", "l" , "c", "bb" };
    char filename[32];
    uint32_t hnd;
    size_t sz = sizeof(rt_table_t);
    pso_error_t err;
    int rv = 0;
    rt_table_t* buf;
    rt_table_t* ent;
    size_t i = 0, j = 0;

    if (!(buf = (rt_table_t*)malloc(sizeof(rt_table_t)))) {
        ERR_LOG("无法为 ItemRT 数据条目分配内存空间!");
        return NULL;
    }

    /* Open up the file and make sure it looks sane enough... */
    if (!(a = pso_gsl_read_open(fn, 0, &err))) {
        ERR_LOG("%d 无法读取 BB %s: %s", a, fn, pso_strerror(err));
        free_safe(buf);
        return NULL;
    }

    /* Make sure the archive has the correct number of entries. */
    if (pso_gsl_file_count(a) != 0xA0) {
        ERR_LOG("%s 数据似乎不完整. 读取字节 0x%zX", fn, pso_gsl_file_count(a));
        rv = -2;
        goto out;
    }

    /* 获取需要读取的文件名称 */
    snprintf(filename, 32, "ItemRT%s%c%d.rel", game_ep_rt_index[章节],
        tolower(abbreviation_for_difficulty(难度)), 颜色);

#ifdef DEBUG
    DBG_LOG("RT文件:%s | %s %s %s", filename, episodenames[章节], get_difficulty_describe(难度), get_section_describe(NULL, 颜色, true));
#endif // DEBUG

    /* Grab a handle to that file. */
    hnd = pso_gsl_file_lookup(a, filename);
    if (hnd == PSOARCHIVE_HND_INVALID) {
        ERR_LOG("%s 缺少 %s 文件!", fn, filename);
        rv = -3;
        goto out;
    }

    /* Make sure the size is correct. */
    if ((err = pso_gsl_file_size(a, hnd)) != 0x280) {
        ERR_LOG("%s 文件 %s 大小无效! 期待大小 0x%zX", fn,
            filename, err);
        rv = -4;
        goto out;
    }

    if (pso_gsl_file_read(a, hnd, buf, sz) != sz) {
        ERR_LOG("读取 %s 错误,路径 %s!",
            filename, fn);
        rv = -5;
        goto out;
    }

    /* Dump it into our nicer (not packed) structure. */
    ent = &bb_rtdata[章节][难度][颜色];

    /* 直接获取子表数据 后面再做解析 */
    memcpy(ent, buf, sz);

    for (i = 0; i < 0x65;i++) {
        ent->enemy_rares[i].probability = buf->enemy_rares[i].probability;
        for (j = 0; j < 3; j++) {
            ent->enemy_rares[i].item_code[j] = buf->enemy_rares[i].item_code[j];
        }
    }

    for (i = 0; i < 0x1E; i++) {
        ent->box_areas[i] = buf->box_areas[i];
    }

    for (i = 0; i < 0x1E; i++) {
        ent->box_rares[i].probability = buf->box_rares[i].probability;
        for (j = 0; j < 3; j++) {
            ent->box_rares[i].item_code[j] = buf->box_rares[i].item_code[j];
        }
    }

    for (i = 0; i < 2; i++) {
        ent->unknown_a1[i] = buf->unknown_a1[i];
    }

    ent->enemy_rares_offset = buf->enemy_rares_offset;
    ent->box_count = buf->box_count;
    ent->box_areas_offset = buf->box_areas_offset;
    ent->box_rares_offset = buf->box_rares_offset;
    ent->unused_offset1 = buf->unused_offset1;

    for (i = 0; i < 0x10; i++) {
        ent->unknown_a2[i] = buf->unknown_a2[i];
    }

    ent->unknown_a2_offset = buf->unknown_a2_offset;
    ent->unknown_a2_count = buf->unknown_a2_count;
    ent->unknown_a3 = buf->unknown_a3;
    ent->unknown_a3 = buf->unknown_a3;
    ent->unknown_a4 = buf->unknown_a4;
    ent->offset_table_offset = buf->offset_table_offset;

    for (i = 0; i < 3; i++) {
        ent->unknown_a5[i] = buf->unknown_a5[i];
    }

    have_bbrt = 1;
out:
    pso_gsl_read_close(a);
    free_safe(buf);
    return ent;
}

rt_table_t* rt_dynamics_read_v2(const char* fn, int 章节, int 难度, int 颜色) {
    const char* game_type[4] = { "", "l" , "c", "bb" };
    size_t sz = 0;
    int rv = 0;
    rt_table_t buf = { 0 };
    rt_table_t* ent;
    char filename[0x20];

    gsl_header* gsl = GSL_OpenArchive(fn);
    if (!gsl)
        return NULL;

    snprintf(filename, 32, "ItemRT%s%c%d.rel", game_type[章节],
        tolower(abbreviation_for_difficulty(难度)), 颜色);

    if ((sz = GSL_GetFileSize(gsl, filename)) > 0x800)
        return NULL;

    if (!GSL_ReadFile(gsl, filename, &buf))
        return NULL;

    ent = &bb_rtdata[章节][难度][颜色];

    memcpy(ent, &buf, sz);

    GSL_CloseArchive(gsl);

    have_bbrt = 1;

    return ent;
}

int rt_v2_enabled(void) {
    return have_v2rt;
}

int rt_gc_enabled(void) {
    return have_gcrt;
}

int rt_bb_enabled(void) {
    return have_bbrt;
}

size_t get_rt_index(uint8_t episode, size_t rt_index) {
    size_t ep4_rt_index_offset = 0x57;//87 Item_RT EP4 enemy_index 差值
    size_t new_rt_index = rt_index;
    return (episode == GAME_TYPE_EPISODE_3 ? (new_rt_index - ep4_rt_index_offset) : episode == GAME_TYPE_EPISODE_4 ? (new_rt_index - ep4_rt_index_offset) : new_rt_index);
}

uint32_t rt_generate_v2_rare(ship_client_t *src, lobby_t *l, int rt_index,
                             int area) {
    sfmt_t* rng = &src->cur_block->sfmt_rng;
    double rnd;
    rt_set_t *set;
    int i;
    uint8_t section = get_lobby_leader_section(l);

    /* Make sure we read in a rare table and we have a sane index */
    if(!have_v2rt)
        return 0;

    if(rt_index < -1 || rt_index > 100)
        return -1;

    /* Grab the rare set for the game */
    set = &v2_rtdata[l->difficulty][section];

    /* Are we doing a drop for an enemy or a box? */
    if(rt_index >= 0) {
        rnd = sfmt_genrand_real1(rng);

        if (rnd < set->enemy_rares[rt_index].prob)
            return set->enemy_rares[rt_index].item_data;
    }
    else {
        for (i = 0; i < 30; ++i) {
            if (set->box_rares[i].area == area) {
                rnd = sfmt_genrand_real1(rng);

                if(rnd < set->box_rares[i].prob)
                    return set->box_rares[i].item_data;
            }
        }
    }

    return 0;
}

uint32_t rt_generate_gc_rare(ship_client_t *src, lobby_t *l, int rt_index,
                             int area) {
    sfmt_t* rng = &src->cur_block->sfmt_rng;
    double rnd;
    rt_set_t *set;
    int i;
    uint8_t section = get_lobby_leader_section(l);

    /* Make sure we read in a rare table and we have a sane index */
    if(!have_gcrt)
        return 0;

    if(rt_index < -1 || rt_index > 100)
        return -1;

    /* Grab the rare set for the game */
    set = &gc_rtdata[l->episode - 1][l->difficulty][section];

    /* Are we doing a drop for an enemy or a box? */
    if(rt_index >= 0) {
        rnd = sfmt_genrand_real1(rng);

        if (rnd < set->enemy_rares[rt_index].prob)
            return set->enemy_rares[rt_index].item_data;
    }
    else {
        for (i = 0; i < 30; ++i) {
            if (set->box_rares[i].area == area) {
                rnd = sfmt_genrand_real1(rng);

                if(rnd < set->box_rares[i].prob)
                    return set->box_rares[i].item_data;
            }
        }
    }

    return 0;
}

rt_table_t* get_rt_table_bb(uint8_t episode, uint8_t challenge, uint8_t difficulty, uint8_t section) {
    uint8_t game_ep_rt_index = 0;//和游戏章节类型相关
    /* Grab the rare table for the game */
    //EP1 0  NULL / EP2 1  l /  CHALLENGE 2 c / EP4 3 bb
    if (challenge) {
        game_ep_rt_index = 2;
    }
    else {
        switch (episode)
        {
        case GAME_TYPE_NORMAL:
        case GAME_TYPE_EPISODE_1:
            game_ep_rt_index = 0;
            break;
        case GAME_TYPE_EPISODE_2:
            game_ep_rt_index = 1;
            break;
        case GAME_TYPE_EPISODE_3:
        case GAME_TYPE_EPISODE_4:
            game_ep_rt_index = 3;
            break;
        }
    }

    rt_table_t* tmp = rt_dynamics_read_bb(ship->cfg->bb_rtdata_file, game_ep_rt_index, difficulty, section);
    if(!tmp)
        return &bb_rtdata[game_ep_rt_index][difficulty][section];

    return tmp;
}

uint32_t rt_generate_bb_rare(ship_client_t* src, lobby_t* l, int rt_index,
    int area, uint8_t section) {
    sfmt_t* rng = &src->cur_block->sfmt_rng;
    double rnd;
    rt_table_t* set;
    int i;

    /* Make sure we read in a rare table and we have a sane index */
    if (!have_bbrt)
        return 0;

    if (rt_index < -1 || rt_index > 100)
        return -1;

#ifdef DEBUG

    DBG_LOG("rt_index %d episode %d challenge %d difficulty %d section %d", rt_index, l->episode, l->challenge, l->difficulty, section);

#endif // DEBUG

    set = get_rt_table_bb(l->episode, l->challenge, l->difficulty, section);

    /* Are we doing a drop for an enemy or a box? */
    if (rt_index >= 0) {
        rnd = sfmt_genrand_real1(rng);

        if ((rnd < expand_rate(set->enemy_rares[rt_index].probability)) || src->game_data->gm_drop_rare)
            return set->enemy_rares[rt_index].item_code[0] | (set->enemy_rares[rt_index].item_code[1] << 8) |
            (set->enemy_rares[rt_index].item_code[2] << 16);
    }
    else {
        for (i = 0; i < 30; ++i) {
            if (set->box_areas[i] == area) {
                rnd = sfmt_genrand_real1(rng);

                if ((rnd < expand_rate(set->box_rares[i].probability)) || src->game_data->gm_drop_rare)
                    return set->box_rares[i].item_code[0] | (set->box_rares[i].item_code[1] << 8) |
                    (set->box_rares[i].item_code[2] << 16);
            }
        }
    }

    return 0;
}

/* 用于随机掉落 独立掉落 */
uint32_t rt_generate_bb_rare_style(ship_client_t* src, lobby_t* l, int rt_index,
    int area, uint8_t section) {
    sfmt_t* rng = &src->sfmt_rng;
    double rnd;
    rt_table_t* set;
    int i;

    /* Make sure we read in a rare table and we have a sane index */
    if (!have_bbrt)
        return 0;

    if (rt_index < -1 || rt_index > 100)
        return -1;

#ifdef DEBUG

    DBG_LOG("rt_index %d episode %d challenge %d difficulty %d section %d", rt_index, l->episode, l->challenge, l->difficulty, section);

#endif // DEBUG

    set = get_rt_table_bb(l->episode, l->challenge, l->difficulty, section);

    /* Are we doing a drop for an enemy or a box? */
    if (rt_index >= 0) {
        rnd = sfmt_genrand_real1(rng);

        if ((rnd < expand_rate(set->enemy_rares[rt_index].probability)) || src->game_data->gm_drop_rare)
            return set->enemy_rares[rt_index].item_code[0] | (set->enemy_rares[rt_index].item_code[1] << 8) |
            (set->enemy_rares[rt_index].item_code[2] << 16);
    }
    else {
        for (i = 0; i < 30; ++i) {
            if (set->box_areas[i] == area) {
                rnd = sfmt_genrand_real1(rng);

                if ((rnd < expand_rate(set->box_rares[i].probability)) || src->game_data->gm_drop_rare)
                    return set->box_rares[i].item_code[0] | (set->box_rares[i].item_code[1] << 8) |
                    (set->box_rares[i].item_code[2] << 16);
            }
        }
    }

    return 0;
}