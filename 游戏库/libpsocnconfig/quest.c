/*
    梦幻之星中国

    版权 (C) 2009, 2010, 2011, 2014, 2015, 2018, 2019, 2020, 2021,
                  2022 Sancaros

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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <time.h>
#include <Windows.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "quest.h"
#include "f_logs.h"
#include "f_iconv.h"
#include "psomemory.h"
#include "Strptime_win.h"

#ifndef LIBXML_TREE_ENABLED
#error You must have libxml2 with tree support built-in.
#endif

#define XC (const xmlChar *)

#ifndef HAVE_TIMEGM
#ifdef HAVE__MKGMTIME
#define timegm _mkgmtime
#else
//#warning Time values will not be in UTC time unless run in UTC timezone.
#define timegm mktime
#endif
#endif

static void quest_dtor(void *o);

static int handle_long(xmlNode *n, psocn_quest_t *q) {
    xmlChar *desc;

    /* Grab the long description from the node */
    if((desc = xmlNodeListGetString(n->doc, n->children, 1))) {
        q->long_desc = (char*)desc;
    }

    return 0;
}

static int handle_short(xmlNode *n, psocn_quest_t *q) {
    xmlChar *desc;

    /* Grab the short description from the node */
    if((desc = xmlNodeListGetString(n->doc, n->children, 1))) {
        strncpy(q->desc, convert_enc("utf-8", "gbk", (const char*)desc), 127);
        q->desc[127] = '\0';
        xmlFree(desc);
    }

    return 0;
}

static int handle_monster(xmlNode *n, psocn_quest_t *q, uint32_t def,
                          uint32_t mask) {
    xmlChar *id, *type, *drops;
    int rv = 0, count;
    void *tmp;
    uint32_t drop;
    uint32_t idnum, typenum;
    typedef struct psocn_quest_enemy mon;

    /* Grab the attributes we're expecting */
    type = xmlGetProp(n, XC"type");
    id = xmlGetProp(n, XC"id");
    drops = xmlGetProp(n, XC"drops");

    typenum = (uint32_t)strtoul((const char*)type, NULL, 0);
    idnum = (uint32_t)strtoul((const char*)id, NULL, 0);

    /* Make sure we have all of them... */
    if((!type && !id) || !drops) {
        ERR_LOG("One or more required monster attributes missing");
        rv = -1;
        goto err;
    }
    else if(typenum && idnum) {
        ERR_LOG("Cannot specify both id and type for monster");
        rv = -2;
        goto err;
    }

    /* Make sure the drops value is sane */
    if(!xmlStrcmp(drops, XC"none")) {
        drop = PSOCN_QUEST_ENDROP_NONE;
    }
    else if(!xmlStrcmp(drops, XC"norare")) {
        drop = PSOCN_QUEST_ENDROP_NORARE;
    }
    else if(!xmlStrcmp(drops, XC"partial")) {
        drop = PSOCN_QUEST_ENDROP_PARTIAL;
    }
    else if(!xmlStrcmp(drops, XC"free")) {
        drop = PSOCN_QUEST_ENDROP_FREE;
    }
    else if(!xmlStrcmp(drops, XC"default")) {
        drop = def;
    }
    else {
        ERR_LOG("无效怪物掉落: %s", (char *)drops);
        rv = -3;
        goto err;
    }

    /* Which definition did the user give us? */
    if(typenum) {
        /* Parse out the type number. */
        errno = 0;
        if(errno) {
            ERR_LOG("分析怪物类型出错 \"%s\": %s",
                  (const char *)type, strerror(errno));
            rv = -4;
            goto err;
        }

        /* Make space for this type of enemy. */
        count = q->num_monster_types + 1;

        if(!(tmp = realloc(q->monster_types, count * sizeof(mon)))) {
            ERR_LOG("怪物类型分配内存时出错: %s",
                  strerror(errno));
            rv = -5;
            goto err;
        }

        /* Save the new enemy type in the list. */
        q->monster_types = (mon *)tmp;
        q->monster_types[count - 1].key = typenum;
        q->monster_types[count - 1].value = drop;
        q->monster_types[count - 1].mask = mask;
        q->num_monster_types = count;
    }
    else {
        /* Parse out the ID number. */
        errno = 0;
        idnum = (uint32_t)strtoul((const char *)id, NULL, 0);
        if(errno) {
            ERR_LOG("分析怪物出错 id \"%s\": %s",
                  (const char *)id, strerror(errno));
            rv = -6;
            goto err;
        }

        /* Make space for this enemy. */
        count = q->num_monster_ids + 1;

        if(!(tmp = realloc(q->monster_ids, count * sizeof(mon)))) {
            ERR_LOG("怪物类型分配内存时出错 ids: %s",
                  strerror(errno));
            rv = -7;
            goto err;
        }

        /* Save the new enemy in the list. */
        q->monster_ids = (mon *)tmp;
        q->monster_ids[count - 1].key = idnum;
        q->monster_ids[count - 1].value = drop;
        q->monster_ids[count - 1].mask = mask;
        q->num_monster_ids = count;
    }

err:
    xmlFree(type);
    xmlFree(id);
    xmlFree(drops);
    return rv;
}

static int handle_drops(xmlNode *n, psocn_quest_t *q) {
    xmlChar *def, *typ;
    int rv = 0;
    uint32_t drop_def = PSOCN_QUEST_ENDROP_NORARE;
    uint32_t mask = PSOCN_QUEST_ENDROP_SDROPS;

    /* Grab the attribute we're expecting. */
    def = xmlGetProp(n, XC"default");
    typ = xmlGetProp(n, XC"type");

    if(!def) {
        ERR_LOG("<drops> 标签缺失默认属性.");
        return -1;
    }

    /* If the type attribute is set, parse it. Otherwise default to server. */
    if(typ) {
        if(!xmlStrcmp(typ, XC"client"))
            mask = PSOCN_QUEST_ENDROP_CDROPS;
        else if(!xmlStrcmp(typ, XC"both"))
            mask |= PSOCN_QUEST_ENDROP_CDROPS;
        else if(xmlStrcmp(typ, XC"server")) {
            ERR_LOG("无效类型属性 <drops>: '%s'",
                  (char *)typ);
            xmlFree(typ);
            xmlFree(def);
            return -1;
        }

        xmlFree(typ);
    }

    /* Make sure the default value is sane */
    if(!xmlStrcmp(def, XC"none")) {
        drop_def = PSOCN_QUEST_ENDROP_NONE;
    }
    else if(!xmlStrcmp(def, XC"norare")) {
        drop_def = PSOCN_QUEST_ENDROP_NORARE;
    }
    else if(!xmlStrcmp(def, XC"partial")) {
        drop_def = PSOCN_QUEST_ENDROP_PARTIAL;
    }
    else if(!xmlStrcmp(def, XC"free")) {
        drop_def = PSOCN_QUEST_ENDROP_FREE;
    }
    else {
        ERR_LOG("Invalid drops default: %s", (char *)def);
        rv = -2;
        goto err;
    }

    /* Now that we're done with that, deal with any children of the node */
    n = n->children;
    while(n) {
        if(n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if(!xmlStrcmp(n->name, XC"monster")) {
            if(handle_monster(n, q, drop_def, mask)) {
                rv = -3;
                goto err;
            }
        }
        else {
            ERR_LOG("无效 %s 标记在行 %hu", (char *)n->name,
                  n->line);
        }

        n = n->next;
    }

err:
    xmlFree(def);
    return rv;
}

static int handle_syncregs(xmlNode *n, psocn_quest_t *q) {
    xmlChar *def, *list;
    int rv = 0, cnt, ne;
    uint8_t *sr = NULL;
    char *tmp = NULL, *tok;
    unsigned long val;
    void *p;

    /* Grab the attributes we're expecting. */
    def = xmlGetProp(n, XC"default");
    list = xmlGetProp(n, XC"list");

    /* The default attribute is required. */
    if(!def) {
        ERR_LOG("syncregs requires a default attribute");
        rv = -1;
        goto err;
    }

    /* What we do next depends on the default setting... */
    if(!xmlStrcmp(def, (xmlChar*)"all")) {
        if(xmlStrcmp(list, (xmlChar*)"none")) {
            ERR_LOG("syncregs can't have default='all' and list");
            rv = -2;
            goto err;
        }

        q->flags |= PSOCN_QUEST_SYNC_REGS | PSOCN_QUEST_SYNC_ALL;
    }
    else if(!xmlStrcmp(def, (xmlChar*)"none")) {
        if(!list) {
            ERR_LOG("syncregs requires list with default='none'");
            rv = -3;
            goto err;
        }

        /* Parse the list... */
        if(!(sr = (uint8_t *)malloc(10))) {
            ERR_LOG("Malloc failed!");
            rv = -4;
            goto err;
        }

        cnt = 10;
        ne = 0;

        tok = strtok_s((char *)list, " ,;\t\n", &tmp);
        while(tok) {
            errno = 0;
            val = strtoul(tok, NULL, 0);
            if(errno) {
                ERR_LOG("Invalid element in syncregs list: %s", tok);
                rv = -5;
                goto err;
            }

            if(val > 255) {
                ERR_LOG("Invalid element in syncregs list: %d", val);
                rv = -6;
                goto err;
            }

            /* If we need more space, double it. */
            if(ne > cnt) {
                p = realloc(sr, cnt << 1);
                if(!p) {
                    ERR_LOG("重新分配内存失败!");
                    rv = -7;
                    goto err;
                }

                cnt <<= 1;
                sr = (uint8_t *)p;
            }

            /* Store the parsed value */
            sr[ne++] = (uint8_t)val;

            /* Parse the next one out */
            tok = strtok_s(NULL, " ,;\t\n", &tmp);
        }

        /* Resize the array down -- we don't fret if this fails, because we're
           just making it smaller anyway... */
        p = realloc(sr, ne);
        if(p)
            sr = (uint8_t *)p;

        /* We've parsed the whole list, store it. */
        q->flags |= PSOCN_QUEST_SYNC_REGS;
        q->num_sync = ne;
        q->synced_regs = sr;
    }

err:
    if(rv < 0 && sr)
        free_safe(sr);

    xmlFree(list);
    xmlFree(def);
    return rv;
}

static int handle_availability(xmlNode *n, psocn_quest_t *q) {
    xmlChar *start_str, *end_str;
    struct tm start_tm, end_tm;
    time_t start_val = 0, end_val = 0;
    int rv = 0;

    /* Grab the attributes we're expecting. */
    start_str = xmlGetProp(n, XC"start");
    end_str = xmlGetProp(n, XC"end");

    memset(&start_tm, 0, sizeof(struct tm));
    memset(&end_tm, 0, sizeof(struct tm));
    if(start_str) {
        if(NULL == strptime((char*)&start_str[0], "%Y-%m-%d %T", &start_tm)) {
            ERR_LOG("无法分析开始时间: '%s' 在xml行 %hu "
                  "设置中格式应该为 YYYY-MM-DD HH:MM:SS",
                  &start_str[0], n->line);
            rv = -1;
            goto err;
        }

        start_val = timegm(&start_tm);

        if(start_val < 0) {
            ERR_LOG("xml 第 %hu 行中指定的开始时间无效",
                  n->line);
            rv = -2;
            goto err;
        }
    }

    if(end_str) {
        if(NULL == strptime((char*)&end_str[0], "%Y-%m-%d %T", &end_tm)) {
            ERR_LOG("无法分析结束时间: '%s' 在xml行 %hu "
                  "设置中格式应该为 YYYY-MM-DD HH:MM:SS",
                  &end_str[0], n->line);
            rv = -3;
            goto err;
        }

        end_val = timegm(&end_tm);

        if(end_val < 0) {
            ERR_LOG("xml 第 %hu 行中指定的结束时间无效",
                  n->line);
            rv = -4;
            goto err;
        }

        if(end_val < start_val) {
            ERR_LOG("xml 结束时间早于第 %hu 行的开始时间",
                  n->line);
            rv = -5;
            goto err;
        }
    }

    q->start_time = (uint64_t)start_val;
    q->end_time = (uint64_t)end_val;

err:
    xmlFree(end_str);
    xmlFree(start_str);
    return rv;
}

static int handle_script(xmlNode *n, psocn_quest_t *q) {
    int rv = 0;
    xmlChar *fn;

    /* Grab the script filename */
    if((fn = xmlGetProp(n, XC"load"))) {
        /*if(-1 == _access((char *)fn, 06)) {
            ERR_LOG("Quest script file '%s' cannot be read",
                  (char *)fn);
            rv = -1;
            xmlFree(fn);
            goto err;
        }*/
        if (NULL != fn) {
            q->onload_script_file = (char*)fn;
        }
        else
        {
            rv = -1;
            xmlFree(fn);
            goto err;
        }
    }

    if((fn = xmlGetProp(n, XC"before_load"))) {
        /*if(-1 == _access((char *)fn, 06)) {
            ERR_LOG("Quest script file '%s' cannot be read",
                  (char *)fn);
            rv = -2;
            xmlFree(fn);
            goto err;
        }*/

        if (NULL != fn) {
            q->beforeload_script_file = (char*)fn;
        }
        else
        {
            rv = -2;
            xmlFree(fn);
            goto err;
        }
    }

err:
    return rv;
}

static int handle_quest(xmlNode *n, psocn_quest_category_t *c) {
    xmlChar *name, *prefix, *v1, *v2, *gc, *bb, *ep, *event, *fmt, *id, *sync;
    xmlChar *minpl, *maxpl, *join, *sflag, *sctl, *sdata, *priv, *hidden, *xb;
    xmlChar* pc;
    int rv = 0, format;
    void *tmp;
    unsigned long episode, id_num, min_pl = 1, max_pl = 4, sf = 0, lc = 0;
    unsigned long ld = 0, sd = 0, sc = 0, privs = 0, hidden_fl = 0;
    long event_num;
    psocn_quest_t *q;
    char *lasts = NULL, *token;
    int event_list = 0;

    /* Grab the attributes we're expecting */
    name = xmlGetProp(n, XC"name");
    prefix = xmlGetProp(n, XC"prefix");
    v1 = xmlGetProp(n, XC"v1");
    v2 = xmlGetProp(n, XC"v2");
    pc = xmlGetProp(n, XC"pc");
    gc = xmlGetProp(n, XC"gc");
    bb = xmlGetProp(n, XC"bb");
    xb = xmlGetProp(n, XC"xbox");
    ep = xmlGetProp(n, XC"episode");
    event = xmlGetProp(n, XC"event");
    fmt = xmlGetProp(n, XC"format");
    id = xmlGetProp(n, XC"id");
    minpl = xmlGetProp(n, XC"minpl");
    maxpl = xmlGetProp(n, XC"maxpl");
    sync = xmlGetProp(n, XC"sync");
    join = xmlGetProp(n, XC"joinable");
    sflag = xmlGetProp(n, XC"sflag");
    sdata = xmlGetProp(n, XC"datareg");
    sctl = xmlGetProp(n, XC"ctlreg");
    priv = xmlGetProp(n, XC"privileges");
    hidden = xmlGetProp(n, XC"hidden");

    /* Make sure we have all of them... */
    if(!name || !prefix || !v1 || !ep || !event || !fmt || !id) {
        ERR_LOG("One or more required quest attributes missing");
        rv = -1;
        goto err;
    }

    /* Make sure the episode is sane */
    episode = strtoul((const char *)ep, NULL, 0);

    if(episode < 1 || episode > 4) {
        ERR_LOG("无效章节: %s", ep);
        rv = -2;
        goto err;
    }

    /* Make sure the format is sane */
    if(!xmlStrcmp(fmt, XC"qst")) {
        format = PSOCN_QUEST_QST;
    }
    else if(!xmlStrcmp(fmt, XC"bindat")) {
        format = PSOCN_QUEST_BINDAT;
    }
    else if(!xmlStrcmp(fmt, XC"ubindat")) {
        format = PSOCN_QUEST_UBINDAT;
    }
    else {
        ERR_LOG("Invalid format given for quest: %s", fmt);
        rv = -3;
        goto err;
    }

    /* Make sure the event list is sane */
    token = strtok_s((char *)event, ", ", &lasts);

    if(!token) {
        ERR_LOG("Invalid event given for quest: %s", event);
        rv = -4;
        goto err;
    }

    while(token) {
        /* Parse the token */
        errno = 0;
        event_num = strtol(token, NULL, 0);

        if(errno || event_num < -1 || event_num > 14) {
            ERR_LOG("Invalid token in event: %s", token);
            rv = -9;
            goto err;
        }

        /* Make sure its valid now */
        if(event_list == -1) {
            ERR_LOG("Invalid event number specified after -1: %s",
                  token);
            rv = -10;
            goto err;
        }
        else if(event_list != 0 && event_num == -1) {
            ERR_LOG("Invalid to specify -1 after an event");
            rv = -11;
            goto err;
        }

        /* Set the bit for the specified event */
        if(event_num == -1) {
            event_list = -1;
        }
        else {
            event_list |= (1 << event_num);
        }

        /* Read the next event in, if any */
        token = strtok_s(NULL, ", ", &lasts);
    }

    /* Make sure the id is sane */
    errno = 0;
    id_num = strtoul((const char *)id, NULL, 0);

    if(errno) {
        ERR_LOG("Invalid ID given for quest: %s", (const char *)id);
        rv = -5;
        goto err;
    }

    /* Make sure the min/max players count is sane */
    if(minpl) {
        min_pl = strtoul((const char *)minpl, NULL, 0);

        if(min_pl < 1 || min_pl > 4) {
            ERR_LOG("Invalid minimum players given: %s",
                  (const char *)minpl);
            rv = -9;
            goto err;
        }
    }

    if(maxpl) {
        max_pl = strtoul((const char *)maxpl, NULL, 0);

        if(max_pl < 1 || max_pl > 4 || max_pl < min_pl) {
            ERR_LOG("Invalid maximum players given: %s",
                  (const char *)maxpl);
            rv = -10;
            goto err;
        }
    }

    /* If the flag stuff is set, parse it and check for sanity. */
    if(sflag) {
        errno = 0;
        sf = strtoul((const char *)sflag, NULL, 0);

        if(errno || sf > 255) {
            ERR_LOG("Invalid sflag given: %s", (const char *)sflag);
            rv = -11;
            goto err;
        }
    }

    /* Check if this quest uses server data function calls */
    if((sctl && !sdata) || (sdata && !sctl)) {
        ERR_LOG("Must give both of datareg/ctlreg (or neither)");
        rv = -12;
        goto err;
    }

    if(sctl) {
        errno = 0;
        sc = strtoul((const char *)sctl, NULL, 0);

        if(errno || sc > 255) {
            ERR_LOG("Invalid ctlreg given: %s",
                  (const char *)sctl);
            rv = -13;
            goto err;
        }

        sd = strtoul((const char *)sdata, NULL, 0);

        if(errno || sd > 255) {
            ERR_LOG("Invalid datareg given: %s",
                  (const char *)sdata);
            rv = -14;
            goto err;
        }
    }

    if(priv) {
        /* Make sure the privilege value is sane 确保特权值正常 */
        errno = 0;
        privs = strtoul((const char *)priv, NULL, 0);

        if(errno) {
            ERR_LOG("Invalid privilege value for quest: %s",
                  (const char *)priv);
            rv = -16;
            goto err;
        }
    }

    /* Allocate space for the quest */
    tmp = realloc(c->quests, (c->quest_count + 1) * sizeof(psocn_quest_t*));

    if(!tmp) {
        ERR_LOG("Couldn't allocate space for quest in array");
        perror("realloc");
        rv = -6;
        goto err;
    }

    c->quests = (psocn_quest_t **)tmp;

    q = (psocn_quest_t *)ref_alloc(sizeof(psocn_quest_t), &quest_dtor);
    if(!q) {
        ERR_LOG("Couldn't allocate space for quest");
        perror("ref_alloc");
        rv = -6;
        goto err;
    }

    c->quests[c->quest_count++] = q;

    /* Clear the quest out */
    memset(q, 0, sizeof(psocn_quest_t));

    /* Copy over what we have so far */
    q->qid = (uint32_t)id_num;
    q->privileges = privs;
    q->episode = (int)episode;
    q->event = (int)event_list;
    q->format = (int)format;

    strncpy(q->name, convert_enc("utf-8", "gbk", (const char*)name), 31);
    q->name[31] = '\0';
    q->prefix = (char *)prefix;

    /* Fill in the versions */
    if(!xmlStrcmp(v1, XC"true"))
        q->versions |= PSOCN_QUEST_V1;

    if(!xmlStrcmp(v2, XC"true"))
        q->versions |= PSOCN_QUEST_V2;
    else if (!xmlStrcmp(v2, XC"disable"))
        q->versions |= PSOCN_QUEST_NODC;

    if(!xmlStrcmp(gc, XC"true"))
        q->versions |= PSOCN_QUEST_GC;
    else if (!xmlStrcmp(gc, XC"disable"))
        q->versions |= PSOCN_QUEST_NOGC;

    if(!xmlStrcmp(bb, XC"true"))
        q->versions |= PSOCN_QUEST_BB;

    if (xb) {
        if (!xmlStrcmp(xb, XC"true"))
            q->versions |= PSOCN_QUEST_XBOX;
        else if (!xmlStrcmp(xb, XC"disable"))
            q->versions |= PSOCN_QUEST_NOXB;
    }

    if (pc) {
        if (!xmlStrcmp(pc, XC"true"))
            q->versions |= PSOCN_QUEST_V2;
        else if (!xmlStrcmp(pc, XC"disable"))
            q->versions |= PSOCN_QUEST_NOPC;
    }

    q->min_players = min_pl;
    q->max_players = max_pl;

    if(sync && !xmlStrcmp(sync, XC"true"))
        q->sync = 1;

    if(sflag) {
        q->flags |= PSOCN_QUEST_FLAG16;
        q->server_flag16_reg = (uint8_t)sf;
    }

    if(sdata) {
        q->flags |= PSOCN_QUEST_DATAFL;
        q->server_data_reg = (uint8_t)sd;
        q->server_ctl_reg = (uint8_t)sc;
    }

    if(join && !xmlStrcmp(join, XC"true"))
        q->flags |= PSOCN_QUEST_JOINABLE;

    if(hidden && !xmlStrcmp(hidden, XC"true"))
        q->flags |= PSOCN_QUEST_HIDDEN;

    /* Now that we're done with that, deal with any children of the node */
    n = n->children;
    while(n) {
        if(n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if(!xmlStrcmp(n->name, XC"long")) {
            if(handle_long(n, q)) {
                rv = -7;
                goto err;
            }
        }
        else if(!xmlStrcmp(n->name, XC"short")) {
            if(handle_short(n, q)) {
                rv = -8;
                goto err;
            }
        }
        else if(!xmlStrcmp(n->name, XC"drops")) {
            if(handle_drops(n, q)) {
                rv = -9;
                goto err;
            }
        }
        else if(!xmlStrcmp(n->name, XC"syncregs")) {
            if(handle_syncregs(n, q)) {
                rv = -15;
                goto err;
            }
        }
        else if(!xmlStrcmp(n->name, XC"availability")) {
            if(handle_availability(n, q)) {
                rv = -17;
                goto err;
            }
        }
        else if(!xmlStrcmp(n->name, XC"script")) {
            if(handle_script(n, q)) {
                rv = -18;
                goto err;
            }
        }
        else {
            ERR_LOG("无效 %s 标记在行 %hu", (char *)n->name,
                  n->line);
        }

        n = n->next;
    }

err:
    xmlFree(name);
    xmlFree(v1);
    xmlFree(v2);
    xmlFree(pc);
    xmlFree(gc);
    xmlFree(bb);
    xmlFree(xb);
    xmlFree(ep);
    xmlFree(event);
    xmlFree(fmt);
    xmlFree(id);
    xmlFree(minpl);
    xmlFree(maxpl);
    xmlFree(sync);
    xmlFree(join);
    xmlFree(sflag);
    xmlFree(sctl);
    xmlFree(sdata);
    xmlFree(priv);
    xmlFree(hidden);
    return rv;
}

static int handle_description(xmlNode *n, psocn_quest_category_t *c) {
    xmlChar *desc;

    /* Grab the description from the node */
    if((desc = xmlNodeListGetString(n->doc, n->children, 1))) {
        strncpy(c->desc, convert_enc("utf-8", "gbk", (const char*)desc), 111);
        c->desc[111] = '\0';
        xmlFree(desc);
    }

    return 0;
}

static int handle_category(xmlNode *n, psocn_quest_list_t *l) {
    xmlChar *name, *type, *eps, *priv;
    char *token, *lasts = NULL;
    int rv = 0;
    uint32_t type_num;
    uint32_t episodes = PSOCN_QUEST_EP1 | PSOCN_QUEST_EP2 |
        PSOCN_QUEST_EP4;
    uint32_t privs = 0;
    void *tmp;
    psocn_quest_category_t *cat;
    int epnum;

    /* Grab the attributes we're expecting */
    name = xmlGetProp(n, XC"name");
    type = xmlGetProp(n, XC"type");
    eps = xmlGetProp(n, XC"episodes");
    priv = xmlGetProp(n, XC"privileges");

    /* Make sure we have both of them... */
    if(!name || !type) {
        ERR_LOG("清单中未包含名称或者类型");
        rv = -1;
        goto err;
    }

    /* Make sure the type is sane */
    if (!xmlStrcmp(type, XC"Normal")) {
        type_num = PSOCN_QUEST_NORMAL;
    }
    else if (!xmlStrcmp(type, XC"Battle")) {
        type_num = PSOCN_QUEST_BATTLE;
    }
    else if (!xmlStrcmp(type, XC"Challenge")) {
        type_num = PSOCN_QUEST_CHALLENGE;
    }
    else if (!xmlStrcmp(type, XC"Oneperson")) {
        type_num = PSOCN_QUEST_ONEPERSON;
    }
    else if (!xmlStrcmp(type, XC"Government")) {
        type_num = PSOCN_QUEST_GOVERNMENT;
    }
    else if (!xmlStrcmp(type, XC"Unused1")) {
        type_num = PSOCN_QUEST_UNUSED1;
    }
    else if (!xmlStrcmp(type, XC"Unused2")) {
        type_num = PSOCN_QUEST_UNUSED2;
    }
    else if(!xmlStrcmp(type, XC"Debug")) {
        type_num = PSOCN_QUEST_NORMAL | PSOCN_QUEST_DEBUG;
    }
    else {
        ERR_LOG("无效任务列表类型: %s", (char *)type);
        rv = -2;
        goto err;
    }

    if(priv) {
        /* Make sure the privilege value is sane */
        errno = 0;
        privs = strtoul((const char *)priv, NULL, 0);

        if(errno) {
            ERR_LOG("无效任务列表权限值: %s",
                  (const char *)priv);
            rv = -8;
            goto err;
        }
    }

    /* Is there an episode list specified? */
    if(eps) {
        /* Parse it. */
        token = strtok_s((char *)eps, ", ", &lasts);
        episodes = 0;

        if(!token) {
            ERR_LOG("任务列表中填写的章节错误: %s",
                  (char *)eps);
            rv = -6;
            goto err;
        }

        while(token) {
            /* Parse the token */
            errno = 0;
            epnum = (int)strtol(token, NULL, 0);

            if(errno || (epnum != 1 && epnum != 2 && epnum != 4)) {
                ERR_LOG("无效章节数: %s", token);
                rv = -7;
                goto err;
            }

            /* Set the bit for the specified episode */
            episodes |= epnum;

            /* Read the next number in, if any */
            token = strtok_s(NULL, ", ", &lasts);
        }
    }

    /* Allocate space for the category */
    tmp = realloc(l->cats, (l->cat_count + 1) *
                  sizeof(psocn_quest_category_t));

    if(!tmp) {
        ERR_LOG("Couldn't allocate space for category");
        perror("realloc");
        rv = -3;
        goto err;
    }

    l->cats = (psocn_quest_category_t *)tmp;
    cat = l->cats + l->cat_count++;

    /* Clear the category out */
    memset(cat, 0, sizeof(psocn_quest_category_t));

    /* Copy over what we have so far */
    cat->type = type_num;
    cat->episodes = episodes;
    cat->privileges = privs;

    //convert_enc("utf-8", "gb2312", (const char*)name);

    /* 任务菜单中文 */
    strncpy(cat->name, convert_enc("utf-8", "gbk", (const char*)name), 31);
    cat->name[31] = '\0';

    /* Now that we're done with that, deal with any children of the node */
    n = n->children;
    while(n) {
        if(n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if(!xmlStrcmp(n->name, XC"description")) {
            if(handle_description(n, cat)) {
                rv = -4;
                goto err;
            }
        }
        else if(!xmlStrcmp(n->name, XC"quest")) {
            if(handle_quest(n, cat)) {
                rv = -5;
                goto err;
            }
        }
        else {
            ERR_LOG("无效 %s 标记在行 %hu", (char *)n->name,
                  n->line);
        }

        n = n->next;
    }

err:
    xmlFree(priv);
    xmlFree(name);
    xmlFree(type);
    xmlFree(eps);
    return rv;
}

int psocn_quests_read(const char *filename, psocn_quest_list_t *rv) {
    xmlParserCtxtPtr cxt;
    xmlDoc *doc;
    xmlNode *n;
    int irv = 0;

    /* Clear out the config. */
    memset(rv, 0, sizeof(psocn_quest_list_t));

    /* Make sure the file exists and can be read, otherwise quietly bail out */
    //if(-1 == _access(filename, 06)) {
    //    return -1;
    //}

    /* Create an XML Parsing context */
    cxt = xmlNewParserCtxt();
    if(!cxt) {
        ERR_LOG("无法为配置创建分析上下文");
        irv = -2;
        goto err;
    }

    /* Open the configuration file for reading. */
    doc = xmlReadFile(filename, NULL, XML_PARSE_DTDVALID);

    if(!doc) {
        xmlParserError(cxt, "分析配置时出错");
        irv = -3;
        goto err_cxt;
    }

    /* Make sure the document validated properly. */
    if(!cxt->valid) {
        xmlParserValidityError(cxt, "分析配置有效性时出错");
        irv = -4;
        goto err_doc;
    }

    /* If we've gotten this far, we have a valid document, go through and read
       everything contained within... */
    n = xmlDocGetRootElement(doc);

    if(!n) {
        ERR_LOG("设置文档为空");
        irv = -5;
        goto err_doc;
    }

    /* Make sure the config looks sane. */
    if(xmlStrcmp(n->name, XC"quests")) {
        ERR_LOG("任务列表清单设置类型不正确");
        irv = -6;
        goto err_doc;
    }

    n = n->children;
    while(n) {
        if(n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if(!xmlStrcmp(n->name, XC"category")) {
            if(handle_category(n, rv)) {
                irv = -7;
                goto err_clean;
            }
        }
        else {
            ERR_LOG("无效标签 %s 设置行 %hu", (char *)n->name,
                  n->line);
        }

        n = n->next;
    }

    /* Cleanup/error handling below... */
err_clean:
    if(irv < 0) {
        psocn_quests_destroy(rv);
    }

err_doc:
    xmlFreeDoc(doc);
err_cxt:
    xmlFreeParserCtxt(cxt);
err:
    return irv;
}

static void quest_dtor(void *o) {
    psocn_quest_t *q = (psocn_quest_t *)o;

    if(!q)
        return;

    xmlFree(q->long_desc);
    xmlFree(q->prefix);
    xmlFree(q->onload_script_file);
    xmlFree(q->beforeload_script_file);
    free_safe(q->monster_ids);
    free_safe(q->monster_types);
    free_safe(q->synced_regs);
}

void psocn_quests_destroy(psocn_quest_list_t *list) {
    int i, j;
    psocn_quest_category_t *cat;

    for(i = 0; i < list->cat_count; ++i) {
        cat = &list->cats[i];

        for(j = 0; j < cat->quest_count; ++j) {
            if (cat->quests[j] != NULL) {
                ref_release(cat->quests[j]);
            }
        }

        /* Free the list of quests. */
        if(cat->quests)
            free_safe(cat->quests);
    }

    /* Free the list of categories, and we're done. */
    if (list->cats)
        free_safe(list->cats);
    list->cats = NULL;
    list->cat_count = 0;
}
