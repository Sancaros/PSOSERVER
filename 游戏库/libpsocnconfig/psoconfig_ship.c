/*
    梦幻之星中国

    版权 (C) 2009, 2010, 2011, 2012, 2013, 2016, 2017, 2018, 2019,
                  2020 Sancaros

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
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#include "WinSock_Defines.h"

#include <libxml/xmlwriter.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>

#include "psoconfig.h"
#include "f_logs.h"
#include "f_iconv.h"
#include "debug.h"

#ifndef LIBXML_TREE_ENABLED
#error You must have libxml2 with tree support built-in.
#endif

#define XC (const xmlChar *)

/* The list of language codes */
#define LANGUAGE_CODE_COUNT     8

//static const char language_codes[LANGUAGE_CODE_COUNT][3] = {
//    "jp", "en", "de", "fr", "es", "cs", "ct", "kr"
//};

static const char language_codes[LANGUAGE_CODE_COUNT][3] = {
    "cn", "tc", "jp", "en", "de", "fr", "es", "kr"
};

static int handle_shipgate(xmlNode* n, psocn_ship_t* cfg) {
    xmlChar* proto_ver, * addr, * ip, * port, * key, * cert, * ca;
    int rv;
    unsigned long rv2;
    uint32_t ip4 = 0;
    uint8_t ip6[16] = { 0 };

    /* Grab the attributes of the tag. */
    proto_ver = xmlGetProp(n, XC"proto_ver");
    addr = xmlGetProp(n, XC"addr");
    ip = xmlGetProp(n, XC"ip");
    port = xmlGetProp(n, XC"port");
    key = xmlGetProp(n, XC"key");
    cert = xmlGetProp(n, XC"cert");
    ca = xmlGetProp(n, XC"ca-cert");

    //ERR_LOG("handle_shipgate %s %s %s %s", (char*)ip, (char*)port, (char*)ca, (char*)addr);

    /* Make sure we have all of them... */
    if (!port || !key || !cert || !ca) {
        ERR_LOG("必须为船闸设置端口和ca");
        rv = -1;
        goto err;
    }
    else if (!addr && !ip) {
        ERR_LOG("必须为船闸设置ip或addr");
        rv = -4;
        goto err;
    }

    /* Parse the address address out */
    if (addr) {
        cfg->shipgate_host = (char*)addr;
    }
    else if (ip) {
        rv = inet_pton(PF_INET, (char*)ip, &ip4);

        if (rv < 1) {
            rv = inet_pton(PF_INET6, (char*)ip, ip6);

            if (rv < 1) {
                ERR_LOG("船闸设置的IP无效: %s",
                    (char*)ip);
                rv = -2;
                goto err;
            }
        }

        cfg->shipgate_host = (char*)ip;
    }

    /* 分析设置端口参数 */
    rv2 = strtoul((char*)port, NULL, 0);

    if (rv2 == 0 || rv2 > 0xFFFF) {
        ERR_LOG("船闸设置的端口无效: %s", (char*)port);
        rv = -3;
        goto err;
    }

    cfg->shipgate_port = (uint16_t)rv2;
    cfg->shipgate_proto_ver = strtoul((char*)proto_ver, NULL, 0);


    cfg->ship_key = (char*)key;
    cfg->ship_cert = (char*)cert;
    cfg->shipgate_ca = (char*)ca;
    rv = 0;

err:
    xmlFree(proto_ver);
    xmlFree(port);

    if (rv) {
        xmlFree(addr);
        xmlFree(ip);
        xmlFree(key);
        xmlFree(cert);
        xmlFree(ca);
    }

    return rv;
}

static int handle_net(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* addr4, * addr6, * ip4, * ip6, * port;
    int rv;
    unsigned long rv2;
    //uint32_t tmpip4;
    //uint8_t tmpip6[16];

    /* Grab the attributes of the tag. */
    addr4 = xmlGetProp(n, XC"addr4");
    addr6 = xmlGetProp(n, XC"addr6");
    ip4 = xmlGetProp(n, XC"ip4");
    ip6 = xmlGetProp(n, XC"ip6");
    port = xmlGetProp(n, XC"port");

    /* Make sure we have both of the required ones... */
    if (!port) {
        ERR_LOG("舰船端口未设置");
        rv = -1;
        goto err;
    }
    else if (!ip4 && !addr4) {
        ERR_LOG("必须为舰船设置一个IP或域名");
        rv = -4;
        goto err;
    }
    else if (!ip6 && !addr6) {
        ERR_LOG("必须为舰船设置一个IPv6或域名6");
        rv = -5;
        goto err;
    }

    cur->ship_host4 = (char*)addr4;

    //CONFIG_LOG("检测域名获取: %s", host4);

    if (check_ipaddr((char*)ip4)) {
        /* Parse the IP address out */
        if (NULL != ip4) {
            rv = inet_pton(PF_INET, (char*)ip4, &cur->ship_ip4);

            if (rv < 1) {
                ERR_LOG("未获取到正确的 IP 地址: %s",
                    (char*)ip4);
                rv = -2;
                goto err;
            }
        }
    }

    /* 分析设置端口参数 */
    rv2 = strtoul((char*)port, NULL, 0);

    if (rv2 == 0 || rv2 > 0xFFFF) {
        ERR_LOG("无效舰船端口: %s", (char*)port);
        rv = -3;
        goto err;
    }

    cur->base_port = (uint16_t)rv2;

    /* See if we have a configured IPv6 address */
    if (ip6 || addr6) {

        cur->ship_host6 = (char*)addr6;

        if (ip6) {
            rv = inet_pton(PF_INET6, (char*)ip6, cur->ship_ip6);

            /* This isn't actually fatal, for now, anyway. */
            if (rv < 1) {
                //ERR_LOG("无效舰船 IPv6 地址: %s", (char*)ip6);
            }
        }
    }

    rv = 0;

err:
    xmlFree(port);

    if (rv) {
        xmlFree(ip4);
        xmlFree(ip6);
        xmlFree(addr4);
        xmlFree(addr6);
    }

    return rv;
}

static int handle_event_old(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* game, * lobby;
    int rv;
    long rv2;

    /* Make sure we don't already have an event setup done... */
    if (cur->event_count != 0) {
        ERR_LOG("xml行 %hu 忽略 <event> 设置标签", n->line);
        return 0;
    }

    /* Grab the attributes of the tag. */
    game = xmlGetProp(n, XC"game");
    lobby = xmlGetProp(n, XC"lobby");

    /* Make sure we have the data */
    if (!game || !lobby) {
        ERR_LOG("节日事件数字未填写");
        rv = -1;
        goto err;
    }

    /* Parse the game event out */
    errno = 0;
    rv2 = strtol((char*)game, NULL, 0);

    if (errno || rv2 > 6 || rv2 < 0) {
        ERR_LOG("舰船房间节日事件无效: %s",
            (char*)game);
        rv = -2;
        goto err;
    }

    cur->events[0].game_event = (uint8_t)rv2;

    /* Parse the lobby event out */
    rv2 = strtol((char*)lobby, NULL, 0);

    if (errno || rv2 > 14 || rv2 < 0) {
        ERR_LOG("舰船大厅节日事件无效: %s",
            (char*)lobby);
        rv = -3;
        goto err;
    }

    cur->events[0].lobby_event = (uint8_t)rv2;
    rv = 0;
    cur->event_count = 1;

err:
    xmlFree(game);
    xmlFree(lobby);
    return rv;
}

static int handle_event_date(xmlNode* n, uint8_t* m, uint8_t* d) {
    xmlChar* month, * day;
    int rv;

    /* Grab the attributes of the tag. */
    month = xmlGetProp(n, XC"month");
    day = xmlGetProp(n, XC"day");

    /* Make sure we have the data */
    if (!month || !day) {
        ERR_LOG("Event timeframe not specified properly");
        rv = -1;
        goto err;
    }

    /* Parse the game event out */
    errno = 0;
    *m = (uint8_t)strtoul((char*)month, NULL, 0);

    if (errno || *m > 12 || *m < 1) {
        ERR_LOG("Invalid month given: %s", (char*)month);
        rv = -3;
        goto err;
    }

    /* Parse the lobby event out */
    *d = (uint8_t)strtoul((char*)day, NULL, 0);

    if (errno || *d < 1) {
        ERR_LOG("Invalid day given: %s", (char*)day);
        rv = -4;
        goto err;
    }

    /* Check the day for validity */
    if (*m == 4 || *m == 6 || *m == 9 || *m == 11) {
        if (*d > 30) {
            ERR_LOG("Invalid day given (month = %d): %d", (int)*m,
                (int)*d);
            rv = -5;
            goto err;
        }
    }
    else if (*m == 2) {
        if (*d > 29) {
            ERR_LOG("Invalid day given (month = %d): %d", (int)*m,
                (int)*d);
            rv = -6;
            goto err;
        }
    }
    else {
        if (*d > 31) {
            ERR_LOG("Invalid day given (month = %d): %d", (int)*m,
                (int)*d);
            rv = -7;
            goto err;
        }
    }

    rv = 0;

err:
    xmlFree(day);
    xmlFree(month);
    return rv;
}

static int handle_event(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* game, * lobby;
    int rv;
    long rv2;
    int c = cur->event_count;
    void* tmp;
    xmlNode* n2;

    /* Make sure we already have an event setup started... */
    if (c < 1) {
        ERR_LOG("xml行 %hu 忽略 <event> 设置标签", n->line);
        return 0;
    }

    /* Grab the attributes of the tag. */
    game = xmlGetProp(n, XC"game");
    lobby = xmlGetProp(n, XC"lobby");

    /* Make sure we have the data */
    if (!game || !lobby) {
        ERR_LOG("节日事件数字未填写");
        rv = -1;
        goto err;
    }

    /* Allocate the space we need */
    tmp = realloc(cur->events, sizeof(psocn_event_t) * (c + 1));
    if (!tmp) {
        ERR_LOG("无法为节日事件分配内存空间: %s",
            strerror(errno));
        rv = -2;
        goto err;
    }

    cur->events = (psocn_event_t*)tmp;
    memset(&cur->events[c], 0, sizeof(psocn_event_t));

    /* Parse the game event out */
    errno = 0;
    rv2 = strtol((char*)game, NULL, 0);

    if (errno || rv2 > 6 || rv2 < -1) {
        ERR_LOG("舰船房间节日事件无效: %s",
            (char*)game);
        rv = -3;
        goto err;
    }

    cur->events[c].game_event = (uint8_t)rv2;

    /* Parse the lobby event out */
    rv2 = strtol((char*)lobby, NULL, 0);

    if (errno || rv2 > 14 || rv2 < -1) {
        ERR_LOG("舰船大厅节日事件无效: %s",
            (char*)lobby);
        rv = -4;
        goto err;
    }

    cur->events[c].lobby_event = (uint8_t)rv2;

    /* Parse out the children of the <event> tag. */
    n2 = n->children;
    while (n2) {
        if (n2->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n2 = n2->next;
            continue;
        }
        else if (!xmlStrcmp(n2->name, XC"start")) {
            if (handle_event_date(n2, &cur->events[c].start_month,
                &cur->events[c].start_day)) {
                rv = -5;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"end")) {
            if (handle_event_date(n2, &cur->events[c].end_month,
                &cur->events[c].end_day)) {
                rv = -6;
                goto err;
            }
        }
        else {
            ERR_LOG("xml行 %hu 无效设置标签 %s",
                n2->line, (char*)n2->name);
        }

        n2 = n2->next;
    }

    ++cur->event_count;
    rv = 0;

err:
    xmlFree(game);
    xmlFree(lobby);
    return rv;
}

static int handle_magnification(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* globla_exp_mult,* monster_rare_drop_mult,* box_rare_drop_mult;
    int rv = 0;
    long rv2;

    /* Grab the attributes of the tag. */
    globla_exp_mult = xmlGetProp(n, XC"globla_exp_mult");
    monster_rare_drop_mult = xmlGetProp(n, XC"monster_rare_drop_mult");
    box_rare_drop_mult = xmlGetProp(n, XC"box_rare_drop_mult");

    /* Make sure we have all of them... */
    if (!globla_exp_mult || !monster_rare_drop_mult || !box_rare_drop_mult) {
        ERR_LOG("倍率设置不完整");
        rv = -1;
        goto err;
    }

    /* Parse the game event out */
    errno = 0;
    rv2 = strtol((char*)globla_exp_mult, NULL, 0);

    if (errno || rv2 > 100000 || rv2 < 0) {
        ERR_LOG("经验倍率无效: %s",
            (char*)globla_exp_mult);
        rv = -2;
        goto err;
    }

    cur->globla_exp_mult = rv2;

    /* Parse the game event out */
    errno = 0;
    rv2 = strtol((char*)monster_rare_drop_mult, NULL, 0);

    if (errno || rv2 > 1000 || rv2 < 0) {
        ERR_LOG("怪物掉落倍率无效或超出限制: %s 最大:1000",
            (char*)monster_rare_drop_mult);
        rv = -2;
        goto err;
    }

    cur->monster_rare_drop_mult = rv2;

    /* Parse the game event out */
    errno = 0;
    rv2 = strtol((char*)box_rare_drop_mult, NULL, 0);

    if (errno || rv2 > 1000 || rv2 < 0) {
        ERR_LOG("箱子掉落倍率无效或超出限制: %s 最大:1000",
            (char*)box_rare_drop_mult);
        rv = -2;
        goto err;
    }

    cur->box_rare_drop_mult = rv2;

err:
    xmlFree(globla_exp_mult);
    xmlFree(monster_rare_drop_mult);
    xmlFree(box_rare_drop_mult);
    return rv;
}

static int handle_info(xmlNode* n, psocn_ship_t* cur, int is_motd) {
    xmlChar* fn, * desc, * v1, * v2, * pc, * gc, * bb, * lang;
    void* tmp;
    int rv = 0, count = cur->info_file_count, i, done = 0;
    char* lasts = { 0 }, * token = { 0 };

    /* Grab the attributes of the tag. */
    fn = xmlGetProp(n, XC"file");
    desc = xmlGetProp(n, XC"desc");
    v1 = xmlGetProp(n, XC"v1");
    v2 = xmlGetProp(n, XC"v2");
    pc = xmlGetProp(n, XC"pc");
    gc = xmlGetProp(n, XC"gc");
    bb = xmlGetProp(n, XC"bb");
    lang = xmlGetProp(n, XC"languages");

    /* Make sure we have all of them... */
    if (!fn || (!desc && !is_motd)) {
        ERR_LOG("info/motd 设置不完整");
        rv = -1;
        goto err;
    }

    if (is_motd && desc) {
        ERR_LOG("motd 无需填写描述!");
        rv = -3;
        goto err;
    }

    /* Allocate space for the new description. */
    tmp = realloc(cur->info_files, (count + 1) * sizeof(psocn_info_file_t));
    if (!tmp) {
        ERR_LOG("无法为 info/motd 文件分配内存");
        perror("realloc");
        rv = -2;
        goto err;
    }

    cur->info_files = (psocn_info_file_t*)tmp;

    /* Copy the data in */
    cur->info_files[count].versions = 0;
    cur->info_files[count].filename = (char*)fn;
    cur->info_files[count].desc = (char*)desc;

    /* Fill in the applicable versions */
    if (!v1 || !xmlStrcmp(v1, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_V1;

    if (!v2 || !xmlStrcmp(v2, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_V2;

    if (!pc || !xmlStrcmp(pc, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_PC;

    if (!gc || !xmlStrcmp(gc, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_GC;

    if (!bb || !xmlStrcmp(gc, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_BB;

    /* Parse the languages string, if given. */
    if (lang) {
        token = strtok_s((char*)lang, ", ", &lasts);

        while (token) {
            for (i = 0; i < LANGUAGE_CODE_COUNT && !done; ++i) {
                if (!strcmp(token, language_codes[i])) {
                    cur->info_files[count].languages |= (1 << i);
                    done = 1;
                }
            }

            if (!done) {
                ERR_LOG("xml行 %hu 忽略未知 info/motd 语言标签: %s", n->line, token);
            }

            done = 0;
            token = strtok_s(NULL, ", ", &lasts);
        }
    }
    else {
        cur->info_files[count].languages = 0xFFFFFFFF;
    }

    ++cur->info_file_count;

    xmlFree(lang);
    xmlFree(gc);
    xmlFree(pc);
    xmlFree(v2);
    xmlFree(v1);

    return 0;

err:
    xmlFree(lang);
    xmlFree(gc);
    xmlFree(pc);
    xmlFree(v2);
    xmlFree(v1);
    xmlFree(fn);
    xmlFree(desc);
    return rv;
}

static int handle_quests(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->quests_dir = (char*)fn;
        return 0;
    }

    /* If not, see if we have the file attribute */
    if ((fn = xmlGetProp(n, XC"file"))) {
        cur->quests_file = (char*)fn;
        return 0;
    }

    /* If we don't have either, report the error */
    ERR_LOG("任务标记格式错误,未提供文件或目录");
    return -1;
}

static int handle_limits(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn, * name, * def, * id;
    int enf = 0, i;
    void* tmp;
    uint32_t idn = 0x7FFFFFFF;

    /* Grab the attributes of the tag. */
    id = xmlGetProp(n, XC"id");
    fn = xmlGetProp(n, XC"file");
    name = xmlGetProp(n, XC"name");
    def = xmlGetProp(n, XC"default");

    /* Make sure we have the data */
    if (!fn) {
        ERR_LOG("Limits file not given");
        goto err;
    }

    if ((!id && name) || (id && !name)) {
        ERR_LOG("Must give both or none of id and name for limits.");
        goto err;
    }

    if (def) {
        if (!xmlStrcmp(def, XC"true")) {
            enf = 1;
        }
        else if (xmlStrcmp(def, XC"false")) {
            ERR_LOG("Invalid default value for limits file: %s",
                (char*)def);
            goto err;
        }
    }
    /* This is really !id && !name, but per the above, if one is not set, then
       the other must not be set either, so this works. */
    else if (!id) {
        enf = 1;
    }

    if (id) {
        /* Parse the id out */
        idn = (uint32_t)strtoul((char*)id, NULL, 0);

        if (idn == 0 || idn > 0x7FFFFFFF) {
            ERR_LOG("Invalid id given for limits: %s", (char*)id);
            goto err;
        }

        /* Check for duplicate IDs. */
        for (i = 0; i < cur->limits_count; ++i) {
            if (cur->limits[i].id == idn) {
                ERR_LOG("Duplicate id given for limits: %s",
                    (char*)id);
                goto err;
            }
        }
    }

    /* Make sure we don't already have an default limits file. */
    if (enf && cur->limits_default != -1) {
        ERR_LOG("Cannot have more than one default limits file!");
        goto err;
    }

    /* Allocate space for it in the array. */
    if (!(tmp = realloc(cur->limits, (cur->limits_count + 1) *
        sizeof(psocn_limit_config_t)))) {
        ERR_LOG("分配动态内存给 limits file: %s",
            strerror(errno));
        goto err;
    }

    /* Copy it over to the struct */
    cur->limits = (psocn_limit_config_t*)tmp;
    cur->limits[cur->limits_count].id = idn;
    cur->limits[cur->limits_count].name = (char*)name;
    cur->limits[cur->limits_count].filename = (char*)fn;
    cur->limits[cur->limits_count].enforce = enf;

    if (enf)
        cur->limits_default = cur->limits_count;

    ++cur->limits_count;

    xmlFree(def);
    xmlFree(id);

    return 0;

err:
    xmlFree(id);
    xmlFree(fn);
    xmlFree(name);
    xmlFree(def);
    return -1;
}

static int handle_bans(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the attributes of the tag. */
    fn = xmlGetProp(n, XC"file");

    /* Make sure we have the data */
    if (!fn) {
        ERR_LOG("本地封禁列表文件缺失");
        return -1;
    }

    /* Copy it over to the struct */
    cur->bans_file = (char*)fn;

    return 0;
}

static int handle_scripts(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the attributes of the tag. */
    fn = xmlGetProp(n, XC"file");

    /* Make sure we have the data */
    if (!fn) {
        ERR_LOG("Scripts 文件名称未填写");
        return -1;
    }

    /* Copy it over to the struct */
    cur->scripts_file = (char*)fn;

    return 0;
}

static int handle_versions(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* v1, * v2, * pc, * gc, * ep3, * bb, * dcnte, * xb, * pcnte;
    int rv = 0;

    /* Grab the attributes of the tag. */
    v1 = xmlGetProp(n, XC"v1");
    v2 = xmlGetProp(n, XC"v2");
    pc = xmlGetProp(n, XC"pc");
    gc = xmlGetProp(n, XC"gc");
    ep3 = xmlGetProp(n, XC"ep3");
    bb = xmlGetProp(n, XC"bb");
    dcnte = xmlGetProp(n, XC"dcnte");
    xb = xmlGetProp(n, XC"xbox");
    pcnte = xmlGetProp(n, XC"pcnte");

    /* Make sure we have the data */
    if (!v1 || !v2 || !pc || !gc || !ep3) {
        ERR_LOG("缺少所需版本");
        rv = -1;
        goto err;
    }

    /* Parse everything out */
    if (!xmlStrcmp(v1, XC"false"))
        cur->shipgate_flags |= SHIPGATE_FLAG_NOV1;

    if (!xmlStrcmp(v2, XC"false"))
        cur->shipgate_flags |= SHIPGATE_FLAG_NOV2;

    if (!xmlStrcmp(pc, XC"false"))
        cur->shipgate_flags |= SHIPGATE_FLAG_NOPC;

    if (!xmlStrcmp(gc, XC"false"))
        cur->shipgate_flags |= SHIPGATE_FLAG_NOEP12;

    if (!xmlStrcmp(ep3, XC"false"))
        cur->shipgate_flags |= SHIPGATE_FLAG_NOEP3;

    if (!bb || !xmlStrcmp(bb, XC"false"))
        cur->shipgate_flags |= SHIPGATE_FLAG_NOBB;

    if (!dcnte || !xmlStrcmp(dcnte, XC"false"))
        cur->shipgate_flags |= SHIPGATE_FLAG_NODCNTE;

    if (!pcnte || !xmlStrcmp(pcnte, XC"false"))
        cur->shipgate_flags |= SHIPGATE_FLAG_NOPCNTE;

    /* For the time being, default to xbox being disabled... */
    if (xb && !xmlStrcmp(xb, XC"true"))
        cur->shipgate_flags &= ~SHIPGATE_FLAG_NOPSOX;

err:
    xmlFree(v1);
    xmlFree(v2);
    xmlFree(pc);
    xmlFree(gc);
    xmlFree(ep3);
    xmlFree(bb);
    xmlFree(dcnte);
    xmlFree(xb);
    xmlFree(pcnte);
    return rv;
}

static int handle_events(xmlNode* n, psocn_ship_t* cur) {
    int rv;
    //unsigned long rv2;
    xmlNode* n2;

    /* Make sure we don't already have an event setup done... */
    if (cur->event_count != 0) {
        ERR_LOG("忽略 %hu 行 <events> 参数", n->line);
        return 0;
    }

    /* Parse out the children of the <events> tag. */
    n2 = n->children;
    while (n2) {
        if (n2->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n2 = n2->next;
            continue;
        }
        else if (!xmlStrcmp(n2->name, XC"defaults")) {
            /* Cheat a bit here, and reuse the old <event> tag parsing code.
               Since this has to be first, that should work fine... */
            if (handle_event_old(n2, cur)) {
                rv = -1;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"event")) {
            if (handle_event(n2, cur)) {
                rv = -2;
                goto err;
            }
        }
        else {
            ERR_LOG("xml行 %hu 无效设置标签 %s",
                n2->line, (char*)n2->name);
        }

        n2 = n2->next;
    }

    rv = 0;

err:
    return rv;
}

static int handle_bbparam(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->bb_param_dir = (char*)fn;
        return 0;
    }

    /* If we don't have it, report the error */
    ERR_LOG("bb param参数格式错误,未提供目录");
    return -1;
}

static int handle_v2param(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->v2_param_dir = (char*)fn;
        return 0;
    }

    /* If we don't have it, report the error */
    ERR_LOG("v2 param参数格式错误,未提供目录");
    return -1;
}

static int handle_bbmaps(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->bb_map_dir = (char*)fn;
        return 0;
    }

    /* If we don't have it, report the error */
    ERR_LOG("bb maps参数格式错误,未提供目录");
    return -1;
}

static int handle_itempt(xmlNode* n, psocn_ship_t* cur) {
    /* Grab the ptdata filenames */
    cur->v2_ptdata_file = (char*)xmlGetProp(n, XC"v2");
    cur->gc_ptdata_file = (char*)xmlGetProp(n, XC"gc");
    cur->bb_ptdata_file = (char*)xmlGetProp(n, XC"bb");

    return 0;
}

static int handle_v2maps(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->v2_map_dir = (char*)fn;
        return 0;
    }

    /* If we don't have it, report the error */
    ERR_LOG("v2 maps参数格式错误,未提供目录");
    return -1;
}

static int handle_gcmaps(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->gc_map_dir = (char*)fn;
        return 0;
    }

    /* If we don't have it, report the error */
    ERR_LOG("GC maps参数格式错误,未提供目录");
    return -1;
}

static int handle_rare_monster_mult_rate(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* hildeblue, * rappy, * nar_lily
        , * pouilly_slime, * merissa_aa,* pazuzu
        , * dorphon_eclair, * kondrieu
        ;
    int rv = 0;
    char* endptr;

    /* Grab the attributes of the tag. */
    hildeblue = xmlGetProp(n, XC"hildeblue");
    rappy = xmlGetProp(n, XC"rappy");
    nar_lily = xmlGetProp(n, XC"nar_lily");
    pouilly_slime = xmlGetProp(n, XC"pouilly_slime");
    merissa_aa = xmlGetProp(n, XC"merissa_aa");
    pazuzu = xmlGetProp(n, XC"pazuzu");
    dorphon_eclair = xmlGetProp(n, XC"dorphon_eclair");
    kondrieu = xmlGetProp(n, XC"kondrieu");

    if (!hildeblue || !rappy || !nar_lily || !pouilly_slime || !merissa_aa || !pazuzu || !dorphon_eclair || !kondrieu) {
        ERR_LOG("必须为舰船设置所有的怪物稀有出现概率");
        rv = -1;
        goto err;
    }

    /* 分析设置概率参数 */
    cur->rare_monster_mult_rates.hildeblue = (uint32_t)strtoul((char*)hildeblue, &endptr, 0);
    cur->rare_monster_mult_rates.rappy = (uint32_t)strtoul((char*)rappy, &endptr, 0);
    cur->rare_monster_mult_rates.nar_lily = (uint32_t)strtoul((char*)nar_lily, &endptr, 0);
    cur->rare_monster_mult_rates.pouilly_slime = (uint32_t)strtoul((char*)pouilly_slime, &endptr, 0);
    cur->rare_monster_mult_rates.merissa_aa = (uint32_t)strtoul((char*)merissa_aa, &endptr, 0);
    cur->rare_monster_mult_rates.pazuzu = (uint32_t)strtoul((char*)pazuzu, &endptr, 0);
    cur->rare_monster_mult_rates.dorphon_eclair = (uint32_t)strtoul((char*)dorphon_eclair, &endptr, 0);
    cur->rare_monster_mult_rates.kondrieu = (uint32_t)strtoul((char*)kondrieu, &endptr, 0);

    if (*endptr != '\0') {
        ERR_LOG("获取的稀有怪物出现概率错误, 改为获取默认值");
        xmlFree(hildeblue);
        xmlFree(rappy);
        xmlFree(nar_lily);
        xmlFree(pouilly_slime);
        xmlFree(merissa_aa);
        xmlFree(pazuzu);
        xmlFree(dorphon_eclair);
        xmlFree(kondrieu);
        cur->rare_monster_mult_rates.hildeblue = default_rare_monster_rates.hildeblue;
        cur->rare_monster_mult_rates.rappy = default_rare_monster_rates.rappy;
        cur->rare_monster_mult_rates.nar_lily = default_rare_monster_rates.nar_lily;
        cur->rare_monster_mult_rates.pouilly_slime = default_rare_monster_rates.pouilly_slime;
        cur->rare_monster_mult_rates.merissa_aa = default_rare_monster_rates.merissa_aa;
        cur->rare_monster_mult_rates.pazuzu = default_rare_monster_rates.pazuzu;
        cur->rare_monster_mult_rates.dorphon_eclair = default_rare_monster_rates.dorphon_eclair;
        cur->rare_monster_mult_rates.kondrieu = default_rare_monster_rates.kondrieu;
        goto err;
    }

#ifdef DEBUG

    for (size_t x = 0; x < _countof(default_rare_monster_rates.rate); x++) {
        DBG_LOG("%d 0x%08X", x, cur->rare_monster_mult_rates.rate[x]);
    }

#endif // DEBUG

err:
    if (rv) {
        xmlFree(hildeblue);
        xmlFree(rappy);
        xmlFree(nar_lily);
        xmlFree(pouilly_slime);
        xmlFree(merissa_aa);
        xmlFree(pazuzu);
        xmlFree(dorphon_eclair);
        xmlFree(kondrieu);
    }

    return rv;
}

static int handle_itempmt(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* limit;

    /* Grab the pmtdata filenames */
    cur->v2_pmtdata_file = (char*)xmlGetProp(n, XC"v2");
    cur->gc_pmtdata_file = (char*)xmlGetProp(n, XC"gc");
    cur->bb_pmtdata_file = (char*)xmlGetProp(n, XC"bb");

    /* See if we're supposed to cap unit +/- values like the client does... */
    limit = xmlGetProp(n, XC"limitv2units");
    if (!limit || !xmlStrcmp(limit, XC"true"))
        cur->local_flags |= PSOCN_SHIP_PMT_LIMITV2;

    xmlFree(limit);

    limit = xmlGetProp(n, XC"limitgcunits");
    if (!limit || !xmlStrcmp(limit, XC"true"))
        cur->local_flags |= PSOCN_SHIP_PMT_LIMITGC;

    xmlFree(limit);

    limit = xmlGetProp(n, XC"limitbbunits");
    if (!limit || !xmlStrcmp(limit, XC"true"))
        cur->local_flags |= PSOCN_SHIP_PMT_LIMITBB;

    xmlFree(limit);

    return 0;
}

static int handle_itemrt(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* quest;

    /* Grab the rtdata filenames */
    cur->v2_rtdata_file = (char*)xmlGetProp(n, XC"v2");
    cur->gc_rtdata_file = (char*)xmlGetProp(n, XC"gc");
    cur->bb_rtdata_file = (char*)xmlGetProp(n, XC"bb");

    quest = xmlGetProp(n, XC"questrares");

    /* See if we're supposed to disable quest rares globally. */
    if (quest) {
        if (!xmlStrcmp(quest, XC"true"))
            cur->local_flags |= PSOCN_SHIP_QUEST_RARES |
            PSOCN_SHIP_QUEST_SRARES;
        else if (!xmlStrcmp(quest, XC"partial"))
            cur->local_flags |= PSOCN_SHIP_QUEST_SRARES;
    }

    xmlFree(quest);

    return 0;
}

static int handle_smutdata(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"file"))) {
        cur->smutdata_file = (char*)fn;
        return 0;
    }

    /* If we don't have it, report the error */
    ERR_LOG("smutdata参数格式错误,未提供目录");
    return -1;
}

static int handle_mageditdata(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"file"))) {
        cur->mageditdata_file = (char*)fn;
        return 0;
    }

    /* If we don't have it, report the error */
    ERR_LOG("mageditdata参数格式错误,未提供目录");
    return -1;
}

static int handle_ship(xmlNode* n, psocn_ship_t* cur) {
    xmlChar* name, * cn_name, * blocks, * gms, * menu, * gmonly, * priv;
    int rv;
    unsigned long rv2;
    xmlNode* n2;
    char tmp_name[12] = { 0 };

    /* Grab the attributes of the <ship> tag. */
    name = xmlGetProp(n, XC"name");
    cn_name = xmlGetProp(n, XC"cn_name");
    blocks = xmlGetProp(n, XC"blocks");
    gms = xmlGetProp(n, XC"gms");
    gmonly = xmlGetProp(n, XC"gmonly");
    menu = xmlGetProp(n, XC"menu");
    priv = xmlGetProp(n, XC"privileges");

    if (!name || !blocks|| !gms || !gmonly || !menu) {
        ERR_LOG("缺少舰船的必需设置");
        rv = -1;
        goto err;
    }

    /* Copy out the strings out that we need */
    cur->ship_name = (char*)name;
    strncpy(tmp_name, convert_enc("utf-8", "gbk", (const char*)cn_name), 12);
    cur->ship_cn_name = (char*)malloc(sizeof(tmp_name));
    if (!cur->ship_cn_name) {
        ERR_LOG("舰船名称内存申请失败");
        rv = -1;
        goto err;
    }
    memcpy(cur->ship_cn_name, tmp_name, sizeof(tmp_name));

#ifdef DEBUG
    DBG_LOG("cur->ship_cn_name %s", cur->ship_cn_name);
#endif // DEBUG

    cur->gm_file = (char*)gms;

    /* Copy out the gmonly flag */
    if (!xmlStrcmp(gmonly, XC"true")) {
        cur->shipgate_flags |= SHIPGATE_FLAG_GMONLY;
    }

    /* Grab the menu code */
    rv = xmlStrlen(menu);

    //printf("rv = %d isalpha(menu[0]) = %d isalpha(menu[1]) = %d\n", rv, isalpha(menu[0]), isalpha(menu[1]));

    if (rv == 2 && isalpha(menu[0]) && isalpha(menu[1])) {
        cur->menu_code = ((uint16_t)menu[0]) | (((uint16_t)(menu[1])) << 8);
    }
    else if (rv) {
        ERR_LOG("无效 menu code");
        rv = -2;
        goto err;
    }

    /* Copy out the number of blocks */
    rv2 = strtoul((char*)blocks, NULL, 0);

    if (rv2 == 0 || rv2 > 16) {
        ERR_LOG("无效舰仓数量: %s", (char*)blocks);
        rv = -3;
        goto err;
    }

    cur->blocks = (int)rv2;

    /* Parse out the privilege level, if one is provided. */
    if (priv) {
        errno = 0;
        rv2 = strtoul((char*)priv, NULL, 0);

        if (errno) {
            ERR_LOG("设置的权限级别无效: %s",
                (char*)priv);
            rv = -22;
            goto err;
        }

        cur->privileges = (uint32_t)rv2;
    }

    /* Parse out the children of the <ship> tag. */
    n2 = n->children;
    while (n2) {
        if (n2->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n2 = n2->next;
            continue;
        }
        else if (!xmlStrcmp(n2->name, XC"net")) {
            if (handle_net(n2, cur)) {
                rv = -3;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"event")) {
            if (handle_event_old(n2, cur)) {
                rv = -4;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"magnification")) {
            if (handle_magnification(n2, cur)) {
                rv = -5;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"info")) {
            if (handle_info(n2, cur, 0)) {
                rv = -6;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"quests")) {
            if (handle_quests(n2, cur)) {
                rv = -7;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"limits")) {
            if (handle_limits(n2, cur)) {
                rv = -8;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"motd")) {
            if (handle_info(n2, cur, 1)) {
                rv = -9;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"versions")) {
            if (handle_versions(n2, cur)) {
                rv = -10;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"bans")) {
            if (handle_bans(n2, cur)) {
                rv = -11;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"scripts")) {
            if (handle_scripts(n2, cur)) {
                rv = -12;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"events")) {
            if (handle_events(n2, cur)) {
                rv = -13;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"bbparam")) {
            if (handle_bbparam(n2, cur)) {
                rv = -14;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"bbmaps")) {
            if (handle_bbmaps(n2, cur)) {
                rv = -15;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"itempt")) {
            if (handle_itempt(n2, cur)) {
                rv = -16;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"v2maps")) {
            if (handle_v2maps(n2, cur)) {
                rv = -17;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"itempmt")) {
            if (handle_itempmt(n2, cur)) {
                rv = -18;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"itemrt")) {
            if (handle_itemrt(n2, cur)) {
                rv = -19;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"gcmaps")) {
            if (handle_gcmaps(n2, cur)) {
                rv = -20;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"rare_monster_mult_rate")) {
            if (handle_rare_monster_mult_rate(n2, cur)) {
                rv = -21;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"v2param")) {
            if (handle_v2param(n2, cur)) {
                rv = -22;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"smutdata")) {
            if (handle_smutdata(n2, cur)) {
                rv = -23;
                goto err;
            }
        }
        else if (!xmlStrcmp(n2->name, XC"mageditdata")) {
            if (handle_mageditdata(n2, cur)) {
                rv = -24;
                goto err;
            }
        }
        else {
            ERR_LOG("xml行 %hu 无效设置标签 %s ",
                n2->line, (char*)n2->name);
        }

        n2 = n2->next;
    }

    rv = 0;

err:
    xmlFree(blocks);
    xmlFree(gmonly);
    xmlFree(menu);
    return rv;
}

int psocn_read_ship_config(const char* f, psocn_ship_t** cfg) {
    xmlParserCtxtPtr cxt;
    xmlDoc* doc;
    xmlNode* n;
    psocn_ship_t* rv;
    int irv = 0;
    static int have_initted = 0;

    if (!have_initted)
        xmlInitParser();

    /* Allocate space for the base of the config. */
    rv = (psocn_ship_t*)malloc(sizeof(psocn_ship_t));

    if (!rv) {
        *cfg = NULL;
        ERR_LOG("无法为舰船设置分配内存空间");
        perror("malloc");
        return -1;
    }

    /* Clear out the config. */
    memset(rv, 0, sizeof(psocn_ship_t));

    /* Allocate space for the default event. */
    rv->events = (psocn_event_t*)malloc(sizeof(psocn_event_t));
    if (!rv->events) {
        *cfg = NULL;
        free_safe(rv);
        ERR_LOG("无法为节日事件分配内存空间: %s",
            strerror(errno));
        return -1;
    }

    memset(rv->events, 0, sizeof(psocn_event_t));

    rv->limits_default = -1;
    rv->shipgate_flags |= SHIPGATE_FLAG_NOPSOX;

    /* Create an XML Parsing context */
    cxt = xmlNewParserCtxt();
    if (!cxt) {
        ERR_LOG("无法分析舰船设置");
        irv = -2;
        goto err;
    }

    /* Open the configuration file for reading. */
    if (f) {
        doc = xmlReadFile(f, NULL, XML_PARSE_DTDVALID);
    }
    else {
        doc = xmlReadFile(psocn_ship_cfg, NULL, XML_PARSE_DTDVALID);
    }

    if (!doc) {
        xmlParserError(cxt, "分析舰船配置文件错误");
        irv = -3;
        goto err_cxt;
    }

    /* Make sure the document validated properly. */
    if (!cxt->valid) {
        xmlParserValidityError(cxt, "无效舰船配置文件");
        irv = -4;
        goto err_doc;
    }

    /* If we've gotten this far, we have a valid document, now go through and
       add in entries for everything... */
    n = xmlDocGetRootElement(doc);

    if (!n) {
        ERR_LOG("舰船配置文件为空");
        irv = -5;
        goto err_doc;
    }

    /* Make sure the config looks sane. */
    if (xmlStrcmp(n->name, XC"psocn_ship_config")) {
        ERR_LOG("舰船配置文件类型错误");
        irv = -6;
        goto err_doc;
    }

    n = n->children;
    while (n) {
        if (n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if (!xmlStrcmp(n->name, XC"shipgate")) {
            if (handle_shipgate(n, rv)) {
                irv = -7;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"ship")) {
            if (handle_ship(n, rv)) {
                irv = -8;
                goto err_doc;
            }
        }
        else {
            ERR_LOG("%hu 行的设置 %s 无效"
                , n->line, (char*)n->name);
        }

        n = n->next;
    }

    /* Did we configure a set of events, or just the default? */
    if (rv->event_count == 0)
        rv->event_count = 1;

    /* Did we configure limits files, but no default? */
    if (rv->limits_count && rv->limits_default == -1)
        rv->limits_default = 0;

    *cfg = rv;

    /* Cleanup/error handling below... */
err_doc:
    xmlFreeDoc(doc);
err_cxt:
    xmlFreeParserCtxt(cxt);
    xmlCleanupParser();

err:
    if (irv && irv > -7) {
        free_safe(rv);
        *cfg = NULL;
    }
    else if (irv) {
        psocn_free_ship_config(rv);
        *cfg = NULL;
    }

    return irv;
}

void psocn_free_ship_config(psocn_ship_t* cfg) {
    int j;

    /* Make sure we actually have a valid configuration pointer. */
    if (cfg) {
        if (cfg->info_files) {
            for (j = 0; j < cfg->info_file_count; ++j) {
                xmlFree(cfg->info_files[j].desc);
                xmlFree(cfg->info_files[j].filename);
            }

            free_safe(cfg->info_files);
        }

        if (cfg->limits) {
            for (j = 0; j < cfg->limits_count; ++j) {
                xmlFree(cfg->limits[j].filename);
                xmlFree(cfg->limits[j].name);
            }

            free_safe(cfg->limits);
        }

        xmlFree(cfg->ship_name);
        xmlFree(cfg->ship_cert);
        xmlFree(cfg->ship_key);
        xmlFree(cfg->shipgate_ca);
        xmlFree(cfg->gm_file);
        xmlFree(cfg->quests_file);
        xmlFree(cfg->quests_dir);
        xmlFree(cfg->bans_file);
        xmlFree(cfg->scripts_file);
        xmlFree(cfg->bb_param_dir);
        xmlFree(cfg->v2_param_dir);
        xmlFree(cfg->bb_map_dir);
        xmlFree(cfg->v2_map_dir);
        xmlFree(cfg->gc_map_dir);
        xmlFree(cfg->shipgate_host);
        xmlFree(cfg->ship_host4);
        xmlFree(cfg->ship_host6);
        xmlFree(cfg->v2_ptdata_file);
        xmlFree(cfg->gc_ptdata_file);
        xmlFree(cfg->bb_ptdata_file);
        xmlFree(cfg->v2_pmtdata_file);
        xmlFree(cfg->gc_pmtdata_file);
        xmlFree(cfg->bb_pmtdata_file);
        xmlFree(cfg->v2_rtdata_file);
        xmlFree(cfg->gc_rtdata_file);
        xmlFree(cfg->bb_rtdata_file);
        xmlFree(cfg->smutdata_file);

        free_safe(cfg->events);

        /* Clean up the base structure. */
        free_safe(cfg);
    }
}