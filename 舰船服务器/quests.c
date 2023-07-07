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

#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <errno.h>

#include <sys/stat.h>

#include <f_logs.h>
#include <PRS.h>

#include "quests.h"
#include "clients.h"
#include "mapdata.h"
#include "ship.h"
#include "packets.h"
#include "pso_StringReader.h"

uint32_t quest_search_enemy_list(uint32_t id, qenemy_t *list, int len, int sd) {
    int i;
    uint32_t mask = sd ? PSOCN_QUEST_ENDROP_SDROPS :
        PSOCN_QUEST_ENDROP_CDROPS;

    for(i = 0; i < len; ++i) {
        if(list[i].key == id && (list[i].mask & mask))
            return list[i].value & 0xFF;
    }

    return 0xFFFFFFFF;
}

/* Find a quest by ID, if it exists */
quest_map_elem_t *quest_lookup(quest_map_t *map, uint32_t qid) {
    quest_map_elem_t *i;

    TAILQ_FOREACH(i, map, qentry) {
        if(qid == i->qid) {
            return i;
        }
    }

    return NULL;
}

/* Add a quest to the list */
quest_map_elem_t *quest_add(quest_map_t *map, uint32_t qid) {
    quest_map_elem_t *el;

    /* Create the element */
    el = (quest_map_elem_t *)malloc(sizeof(quest_map_elem_t));

    if(!el) {
        return NULL;
    }

    /* Clean it out, and fill in the id */
    memset(el, 0, sizeof(quest_map_elem_t));
    el->qid = qid;

    /* Add to the list */
    TAILQ_INSERT_TAIL(map, el, qentry);
    return el;
}

/* Clean the list out */
void quest_cleanup(quest_map_t *map) {
    quest_map_elem_t *tmp, *i;

    /* Remove all elements, freeing them as we go along */
    i = TAILQ_FIRST(map);
    while(i) {
        tmp = TAILQ_NEXT(i, qentry);

        free_safe(i);
        i = tmp;
    }

    /* Reinit the map, just in case we reuse it */
    TAILQ_INIT(map);
}

/* Process an entire list of quests read in for a version/language combo. */
int quest_map(quest_map_t *map, psocn_quest_list_t *list, int version,
              int language) {
    int i, j;
    psocn_quest_category_t *cat;
    psocn_quest_t *q;
    quest_map_elem_t *elem;

    if(version >= CLIENT_VERSION_ALL || language >= CLIENT_LANG_ALL) {
        return -1;
    }

    for(i = 0; i < list->cat_count; ++i) {
        cat = &list->cats[i];

        for(j = 0; j < cat->quest_count; ++j) {
            q = cat->quests[j];

            /* Find the quest if we have it */
            elem = quest_lookup(map, q->qid);

            if(elem) {
                elem->qptr[version][language] = q;
            }
            else {
                elem = quest_add(map, q->qid);

                if(!elem) {
                    return -2;
                }

                elem->qptr[version][language] = q;
            }

            q->user_data = elem;
        }
    }

    return 0;
}

static uint32_t quest_cat_type(ship_t *s, int ver, int lang,
                               psocn_quest_t *q) {
    int i, j;

    /* Look for it. */
    for(i = 0; i < s->qlist[ver][lang].cat_count; ++i) {
        for(j = 0; j < s->qlist[ver][lang].cats[i].quest_count; ++j) {
            if(q == s->qlist[ver][lang].cats[i].quests[j])
                return s->qlist[ver][lang].cats[i].type;
        }
    }

    return 0;
}

static int check_cache_age(const char *fn1, const char *fn2) {
    struct stat s1, s2;

    if(stat(fn1, &s1))
        return -1;

    if(stat(fn2, &s2))
        /* Assume it doesn't exist */
        return 1;

    if(s1.st_mtime >= s2.st_mtime)
        /* The quest file is newer than the cache, regenerate it. */
        return 1;

    /* The cache file is newer than the quest, so we should be safe to use what
       we already have. */
    return 0;
}

static uint8_t *decompress_dat(uint8_t *inbuf, uint32_t insz, uint32_t *osz) {
    uint8_t *rv;
    int sz;

    if((sz = pso_prs_decompress_buf(inbuf, &rv, (size_t)insz)) < 0) {
        QERR_LOG("无法解压数据: %s", pso_strerror(sz));
        return NULL;
    }

    *osz = (uint32_t)sz;
    return rv;
}

static uint8_t *read_and_dec_dat(const char *fn, uint32_t *osz) {
    FILE *fp;
    off_t sz;
    uint8_t *buf, *rv;

    /* Read the file in. */
    if(!(fp = fopen(fn, "rb"))) {
        QERR_LOG("无法打开任务文件 \"%s\": %s", fn,
              strerror(errno));
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(!(buf = (uint8_t *)malloc(sz))) {
        QERR_LOG("无法分配内存去读取 dat : %s",
              strerror(errno));
        fclose(fp);
        return NULL;
    }

    if(fread(buf, 1, sz, fp) != sz) {
        QERR_LOG("无法读取 dat\n错误信息: %s", strerror(errno));
        free_safe(buf);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    /* Return it decompressed. */
    rv = decompress_dat(buf, (uint32_t)sz, osz);
    free_safe(buf);
    return rv;
}

static uint32_t qst_dat_size(const uint8_t *buf, int ver) {
    const dc_quest_file_pkt *dchdr = (const dc_quest_file_pkt *)buf;
    const pc_quest_file_pkt *pchdr = (const pc_quest_file_pkt *)buf;
    const bb_quest_file_pkt *bbhdr = (const bb_quest_file_pkt *)buf;
    char fn[32];
    char *ptr;

    /* Figure out the size of the .dat portion. */
    switch(ver) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            /* Check the first file to see if it is the dat. */
            strncpy(fn, dchdr->filename, 16);
            fn[16] = 0;

            if((ptr = strrchr(fn, '.')) && !strcmp(ptr, ".dat"))
                return LE32(dchdr->length);

            /* Try the second file in the qst */
            dchdr = (const dc_quest_file_pkt *)(buf + 0x3C);
            strncpy(fn, dchdr->filename, 16);
            fn[16] = 0;

            if((ptr = strrchr(fn, '.')) && !strcmp(ptr, ".dat"))
                return LE32(dchdr->length);

            /* Didn't find it, punt. */
            return 0;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_PC:
            /* Check the first file to see if it is the dat. */
            strncpy(fn, pchdr->filename, 16);
            fn[16] = 0;

            if((ptr = strrchr(fn, '.')) && !strcmp(ptr, ".dat"))
                return LE32(pchdr->length);

            /* Try the second file in the qst */
            pchdr = (const pc_quest_file_pkt *)(buf + 0x3C);
            strncpy(fn, pchdr->filename, 16);
            fn[16] = 0;

            if((ptr = strrchr(fn, '.')) && !strcmp(ptr, ".dat"))
                return LE32(pchdr->length);

            /* Didn't find it, punt. */
            return 0;

        case CLIENT_VERSION_BB:
            /* Check the first file to see if it is the dat. */
            strncpy(fn, bbhdr->filename, 16);
            fn[16] = 0;

            if((ptr = strrchr(fn, '.')) && !strcmp(ptr, ".dat"))
                return LE32(bbhdr->length);

            /* Try the second file in the qst */
            bbhdr = (const bb_quest_file_pkt *)(buf + 0x58);
            strncpy(fn, bbhdr->filename, 16);
            fn[16] = 0;

            if((ptr = strrchr(fn, '.')) && !strcmp(ptr, ".dat"))
                return LE32(bbhdr->length);

            /* Didn't find it, punt. */
            return 0;
    }

    return 0;
}

static int copy_dc_qst_dat(const uint8_t *buf, uint8_t *rbuf, off_t sz,
                           uint32_t dsz) {
    const dc_quest_chunk_pkt *ck;
    uint32_t ptr = 120, optr = 0;
    char fn[32];
    char *cptr;
    uint32_t clen;

    while(ptr < (uint32_t)sz) {
        ck = (const dc_quest_chunk_pkt *)(buf + ptr);

        /* Check the chunk for validity. */
        if(ck->hdr.dc.pkt_type != QUEST_CHUNK_TYPE ||
           ck->hdr.dc.pkt_len != LE16(0x0418)) {
            QERR_LOG("未知或损坏的任务数据!");
            return -1;
        }

        /* Grab the vitals... */
        strncpy(fn, ck->filename, 16);
        fn[16] = 0;
        clen = LE32(ck->length);
        cptr = strrchr(fn, '.');

        /* 合理性检查... */
        if(clen > 1024 || !cptr) {
            QERR_LOG("损坏的任务数据!");
            return -1;
        }

        /* See if this is part of the .dat file */
        if(!strcmp(cptr, ".dat")) {
            if(optr + clen > dsz) {
                QERR_LOG("任务文件似乎已损坏!");
                return -1;
            }

            memcpy(rbuf + optr, ck->data, clen);
            optr += clen;
        }

        ptr += 0x0418;
    }

    if(optr != dsz) {
        QERR_LOG("任务文件似乎已损坏!");
        return -1;
    }

    return 0;
}

static int copy_pc_qst_dat(const uint8_t *buf, uint8_t *rbuf, off_t sz,
                           uint32_t dsz) {
    const dc_quest_chunk_pkt *ck;
    uint32_t ptr = 120, optr = 0;
    char fn[32];
    char *cptr;
    uint32_t clen;

    while(ptr < (uint32_t)sz) {
        ck = (const dc_quest_chunk_pkt *)(buf + ptr);

        /* Check the chunk for validity. */
        if(ck->hdr.pc.pkt_type != QUEST_CHUNK_TYPE ||
           ck->hdr.pc.pkt_len != LE16(0x0418)) {
            QERR_LOG("未知或损坏的任务数据!");
            return -1;
        }

        /* Grab the vitals... */
        strncpy(fn, ck->filename, 16);
        fn[16] = 0;
        clen = LE32(ck->length);
        cptr = strrchr(fn, '.');

        /* 合理性检查... */
        if(clen > 1024 || !cptr) {
            QERR_LOG("损坏的任务数据!");
            return -1;
        }

        /* See if this is part of the .dat file */
        if(!strcmp(cptr, ".dat")) {
            if(optr + clen > dsz) {
                QERR_LOG("任务文件似乎已损坏!");
                return -1;
            }

            memcpy(rbuf + optr, ck->data, clen);
            optr += clen;
        }

        ptr += 0x0418;
    }

    if(optr != dsz) {
        QERR_LOG("任务文件似乎已损坏!");
        return -1;
    }

    return 0;
}

static int copy_bb_qst_dat(const uint8_t *buf, uint8_t *rbuf, off_t sz,
                           uint32_t dsz) {
    const bb_quest_chunk_pkt *ck;
    uint32_t ptr = 176, optr = 0;
    char fn[32];
    char *cptr;
    uint32_t clen;

    while(ptr < (uint32_t)sz) {
        ck = (const bb_quest_chunk_pkt *)(buf + ptr);

        /* Check the chunk for validity. */
        if(ck->hdr.pkt_type != LE16(QUEST_CHUNK_TYPE) ||
           ck->hdr.pkt_len != LE16(0x041C)) {
            QERR_LOG("未知或损坏的任务数据!");
            return -1;
        }

        /* Grab the vitals... */
        strncpy(fn, ck->filename, 16);
        fn[16] = 0;
        clen = LE32(ck->length);
        cptr = strrchr(fn, '.');

        /* 合理性检查... */
        if(clen > 1024 || !cptr) {
            QERR_LOG("损坏的任务数据!");
            return -1;
        }

        /* See if this is part of the .dat file */
        if(!strcmp(cptr, ".dat")) {
            if(optr + clen > dsz) {
                QERR_LOG("任务文件似乎已损坏!");
                return -1;
            }

            memcpy(rbuf + optr, ck->data, clen);
            optr += clen;
        }

        ptr += 0x0420;
    }

    if(optr != dsz) {
        QERR_LOG("任务文件似乎已损坏!");
        return -1;
    }

    return 0;
}

static uint8_t *read_and_dec_qst(const char *fn, uint32_t *osz, int ver) {
    FILE *fp;
    off_t sz;
    uint8_t *buf, *buf2, *rv;
    uint32_t dsz;

    /* Read the file in. */
    if(!(fp = fopen(fn, "rb"))) {
        QERR_LOG("无法打开任务文件 \"%s\": %s", fn,
              strerror(errno));
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Make sure the file's size is sane. */
    if(sz < 120) {
        QERR_LOG("任务文件 \"%s\" 大小错误 %d < 120", fn, sz);
        fclose(fp);
        return NULL;
    }

    if(!(buf = (uint8_t *)malloc(sz))) {
        QERR_LOG("无法分配内存去读取 qst: %s",
              strerror(errno));
        fclose(fp);
        return NULL;
    }

    if(fread(buf, 1, sz, fp) != sz) {
        QERR_LOG("无法读取 qst: %s", strerror(errno));
        free_safe(buf);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    /* Figure out how big the .dat portion is. */
    if(!(dsz = qst_dat_size(buf, ver))) {
        QERR_LOG("无法从qst文件中获取dat大小 \"%s\"", fn);
        free_safe(buf);
        return NULL;
    }

    /* Allocate space for it. */
    if(!(buf2 = (uint8_t *)malloc(dsz))) {
        QERR_LOG("无法分配内存去解码 qst: %s",
              strerror(errno));
        free_safe(buf);
        return NULL;
    }

    /* Note, we'll never get PC quests in here, since we don't look at them. The
       primary thing this means is that PSOPC and DCv2 must have the same set of
       quests. */
    switch(ver) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
            if(copy_dc_qst_dat(buf, buf2, sz, dsz)) {
                QERR_LOG("解码 qst 文件 \"%s\" 错误, 详情看上文..", fn);
                free_safe(buf2);
                free_safe(buf);
                return NULL;
            }

            break;

        case CLIENT_VERSION_PC:
            if(copy_pc_qst_dat(buf, buf2, sz, dsz)) {
                QERR_LOG("解码 qst 文件 \"%s\" 错误, 详情看上文..", fn);
                free_safe(buf2);
                free_safe(buf);
                return NULL;
            }

            break;

        case CLIENT_VERSION_BB:
            if(copy_bb_qst_dat(buf, buf2, sz, dsz)) {
                QERR_LOG("解码 qst 文件 \"%s\" 错误, 详情看上文..", fn);
                free_safe(buf2);
                free_safe(buf);
                return NULL;
            }

            break;

        default:
            free_safe(buf2);
            free_safe(buf);
            return NULL;
    }

    /* We're done with the first buffer, so clean it up. */
    free_safe(buf);

    /* Return the dat decompressed. */
    rv = decompress_dat(buf2, (uint32_t)dsz, osz);
    free_safe(buf2);
    return rv;
}

/* Build/rebuild the quest enemy/object data cache. */
int quest_cache_maps(ship_t *s, quest_map_t *map, const char *dir) {
    quest_map_elem_t *i;
    size_t dlen = strlen(dir);
    char* mdir;
    char *fn1, *fn2;
    int j, k;
    psocn_quest_t *q;
    const static char exts[2][4] = { "dat", "qst" };
    uint8_t *dat;
    uint32_t dat_sz, tmp;

    mdir = (char*)malloc(sizeof(dlen) + 20);

    /* Make sure we have all the directories we'll need. */
    sprintf(mdir, "%s/.mapcache", dir);
    if(_mkdir(mdir) && errno != EEXIST) {
        QERR_LOG("创建地图缓存文件夹错误: %s",
              strerror(errno));
        free_safe(mdir);
        return -1;
    }


    for (j = 0; j < CLIENT_VERSION_ALL; ++j) {
        sprintf(mdir, "%s/.mapcache/%s", dir, client_type[j]->ver_name);
        if (_mkdir(mdir) && errno != EEXIST) {
            QERR_LOG("创建地图缓存文件夹错误: %s",
                strerror(errno));
            free_safe(mdir);
            return -1;
        }
    }

    //sprintf(mdir, "%s/.mapcache/v1", dir);
    //if(_mkdir(mdir) && errno != EEXIST) {
    //    QERR_LOG("创建地图缓存文件夹错误: %s",
    //          strerror(errno));
    //    free_safe(mdir);
    //    return -1;
    //}

    //sprintf(mdir, "%s/.mapcache/v2", dir);
    //if(_mkdir(mdir) && errno != EEXIST) {
    //    QERR_LOG("创建地图缓存文件夹错误: %s",
    //          strerror(errno));
    //    free_safe(mdir);
    //    return -1;
    //}

    //sprintf(mdir, "%s/.mapcache/gc", dir);
    //if(_mkdir(mdir) && errno != EEXIST) {
    //    QERR_LOG("创建地图缓存文件夹错误: %s",
    //          strerror(errno));
    //    free_safe(mdir);
    //    return -1;
    //}

    //sprintf(mdir, "%s/.mapcache/bb", dir);
    //if(_mkdir(mdir) && errno != EEXIST) {
    //    QERR_LOG("创建地图缓存文件夹错误: %s",
    //          strerror(errno));
    //    free_safe(mdir);
    //    return -1;
    //}

    TAILQ_FOREACH(i, map, qentry) {
        /* Process it. */
        for(j = 0; j < CLIENT_VERSION_ALL; ++j) {
            /* Skip PC, it is the same as v2. */
            if(j == CLIENT_VERSION_PC)
                continue;

            for(k = 0; k < CLIENT_LANG_ALL; ++k) {
                if((q = i->qptr[j][k])) {
                    /* Don't bother with battle or challenge quests. */
                    tmp = quest_cat_type(s, j, k, q);
                    if(tmp & (PSOCN_QUEST_BATTLE | PSOCN_QUEST_CHALLENGE))
                        break;

                    if(!(fn1 = (char *)malloc(dlen + 25 + strlen(q->prefix)))) {
                        QERR_LOG("分配内存错误: %s",
                              strerror(errno));
                        free_safe(mdir);
                        return -1;
                    }

                    if(!(fn2 = (char *)malloc(dlen + 35))) {
                        QERR_LOG("分配内存错误: %s",
                              strerror(errno));
                        free_safe(fn1);
                        free_safe(mdir);
                        return -1;
                    }

                    sprintf(fn1, "%s/%s/%s/%s_%s.%s", dir, client_type[j]->ver_name,
                            language_codes[k], q->prefix, language_codes[k], exts[q->format]);
                    sprintf(fn2, "%s/.mapcache/%s/%08x", dir, client_type[j]->ver_name,
                            q->qid);

                    if(check_cache_age(fn1, fn2)) {
                        QERR_LOG("任务缓存 %s 语言 %s %d 需要更新!",
                            client_type[j]->ver_name, language_codes[k], q->qid);
                    }

                    if(q->format == PSOCN_QUEST_BINDAT) {
                        if((dat = read_and_dec_dat(fn1, &dat_sz))) {
                            cache_quest_enemies(fn2, dat, dat_sz, q->episode);
                            free_safe(dat);
                        }
                    }
                    else {
                        if((dat = read_and_dec_qst(fn1, &dat_sz, j))) {
                            cache_quest_enemies(fn2, dat, dat_sz, q->episode);
                            free_safe(dat);
                        }
                    }

                    free_safe(fn2);
                    free_safe(fn1);
                    //free_safe(mdir);

                    break;
                }
            }
        }
    }

    return 0;
}
