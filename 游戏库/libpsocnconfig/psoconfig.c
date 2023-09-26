/*
    梦幻之星中国 设置文件.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <direct.h>
#include <urlmon.h>
#include <time.h>
#include <locale.h>

#include "WinSock_Defines.h"

#include <libxml/xmlwriter.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>

#include "psoconfig.h"
#include "f_logs.h"
#include "debug.h"

#ifndef LIBXML_TREE_ENABLED
#error You must have libxml2 with tree support built-in.
#endif

#define XC (const xmlChar *)

char ipbuf[32] = { 0 };

/* The list of language codes */
#define LANGUAGE_CODE_COUNT     8

static const char language_codes[LANGUAGE_CODE_COUNT][3] = {
    "cn", "tc", "jp", "en", "de", "fr", "es", "kr"
};

//static const char language_codes[LANGUAGE_CODE_COUNT][3] = {
//    "jp", "en", "de", "fr", "es", "cs", "ct", "kr"
//};

char* int2ipstr(const int ip)
{
    sprintf(ipbuf, "%u.%u.%u.%u",
        (char)*((char*)&ip + 0),
        (char)*((char*)&ip + 1),
        (char)*((char*)&ip + 2),
        (char)*((char*)&ip + 3));
    return ipbuf;
};

/*
* 	地址为IP返回true 其他返回false
*/
int32_t server_is_ip(char* server)
{
    char ip[4][4] = { 0 };
    int32_t ip_i = 0;
    char* ptr = server;
    int32_t ptr_i = 0;
    int32_t ptr_len = strlen(server);

    if (NULL == server || 15 < ptr_len)
        return 0;

    //检查IP范围
    for (int i = 0; i < 4; i++)
    {
        ip_i = 0;
        for (int j = 0; j < 4; j++)
        {
            //1.字符为0-9的数字
            if (3 != j && isdigit(ptr[ptr_i]))
            {
                ip[i][ip_i++] = ptr[ptr_i++];
            }
            //2.字符为'.'分隔符
            else if (3 != i && 0 != j && '.' == ptr[ptr_i])
            {
                ip[i][ip_i++] = '\0';
                ptr_i++;
                break;
            }
            else
            {
                return 0;
            }
            //3.结束
            if (ptr_len <= ptr_i)
            {
                if (3 == i)
                    break;
                else
                    return 0;
            }
        }
    }
    //检查IP格式
    for (int z = 0; z < 4; z++)
        if (atoi(ip[z]) > 255)
            return 0;
    return 1;
}

#define False 0
#define True 1

int checkIPV4(char* ip)
{
    int sepNum;
    if (ip[0] == '0' || ip[0] == '.' || strlen(ip) > 3 || strlen(ip) == 0)
        return False;
    sepNum = atoi(ip);
    if (sepNum == 0 || sepNum > 255)
        return False;
    return True;
}

int checkIPV6(char* ip)
{
    size_t i = 0;
    if (ip[0] == ':' || strlen(ip) > 4 || strlen(ip) == 0)
        return False;
    for (i = 0; i < strlen(ip); i++)
    {
        if ((ip[i] >= '0' && ip[i] <= '9') || (ip[i] >= 'a' && ip[i] <= 'f') || (ip[i] >= 'A' && ip[i] <= 'F'))
            continue;
        else
            return False;
    }
    return True;
}

int check_ipaddr(char* IP)
{
    if (IP == NULL)
        return 0;
    char* cur, * pre;
    char ip[10] = { 0 };
    char len = 0;
    if (strchr(IP, '.'))
    {
        pre = IP;
        cur = strchr(IP, '.');    //找到'.'的位置
        while (cur)
        {
            memcpy(ip, pre, cur - pre);    //取出两个'.'之间的字符
            if (checkIPV4(ip) == False)
                return 0;
            pre = cur + 1;
            cur = strchr(pre, '.');
            memset(ip, 0, 10);
            len++;
            if (len > 3)        //判断分组是否符合ipv4要求
                return 0;
        }
        if (len != 3)
            return 0;
        memcpy(ip, pre, strlen(pre));
        if (checkIPV4(ip) == False)
            return 0;
        return 1;
    }
    else if (strchr(IP, ':'))
    {
        pre = IP;
        cur = strchr(IP, ':');
        while (cur)
        {
            memcpy(ip, pre, cur - pre);
            if (checkIPV6(ip) == False)
                return 0;
            pre = cur + 1;
            cur = strchr(pre, ':');
            memset(ip, 0, 10);
            len++;
            if (len > 7)
                return 0;
        }
        if (len != 7)
            return 0;
        memcpy(ip, pre, strlen(pre));
        if (checkIPV6(ip) == False)
            return 0;
        return 1;
    }

    return 0;
}

//static int handle_server(xmlNode* n, psocn_config_t* cur) {
//    xmlChar* addr4, * addr6, * ip4, * ip6;
//    int rv;
//
//    /* Grab the attributes of the tag. */
//    addr4 = xmlGetProp(n, XC"addr4");
//    addr6 = xmlGetProp(n, XC"addr6");
//    ip4 = xmlGetProp(n, XC"ip4");
//    ip6 = xmlGetProp(n, XC"ip6");
//
//    /* Make sure we have what we need... */
//    if (!ip4 || !addr4) {
//        ERR_LOG("服务器域名或IP未填写");
//        rv = -1;
//        goto err;
//    }
//
//    cur->srvcfg.host4 = (char*)addr4;
//
//    //CONFIG_LOG("检测域名获取: %s", host4);
//
//    if (check_ipaddr((char*)ip4)) {
//        /* Parse the IP address out */
//        rv = inet_pton(PF_INET, (char*)ip4, &cur->srvcfg.server_ip);
//        //ERR_LOG("IP串码 %d rv = %d", &cur->server_ip, rv);
//        if (rv < 1) {
//            ERR_LOG("未获取到正确的 IP 地址: %s",
//                (char*)ip4);
//            rv = -2;
//            goto err;
//        }
//    }
//
//    /* See if we have a configured IPv6 address */
//    if (ip6 || addr6) {
//
//        cur->srvcfg.host6 = (char*)addr6;
//
//        if (ip6) {
//            rv = inet_pton(PF_INET6, (char*)ip6, cur->srvcfg.server_ip6);
//
//            /* This isn't actually fatal, for now, anyway. */
//            if (rv < 1) {
//                //ERR_LOG("无效 IPv6 地址: %s", (char*)ip6);
//            }
//        }
//    }
//
//    rv = 0;
//
//err:
//    xmlFree(addr4);
//    xmlFree(addr6);
//    xmlFree(ip4);
//    xmlFree(ip6);
//    return rv;
//}
//
//static int handle_database(xmlNode* n, psocn_config_t* cur) {
//    xmlChar* type, * host, * user, * pass, * db, * port, * rec, * char_set, * showsetting;
//    int rv;
//    unsigned long rv2;
//
//    /* Grab the attributes of the tag. */
//    type = xmlGetProp(n, XC"type");
//    host = xmlGetProp(n, XC"host");
//    user = xmlGetProp(n, XC"user");
//    pass = xmlGetProp(n, XC"pass");
//    db = xmlGetProp(n, XC"db");
//    port = xmlGetProp(n, XC"port");
//    rec = xmlGetProp(n, XC"auto_reconnect");
//    char_set = xmlGetProp(n, XC"char_set");
//    showsetting = xmlGetProp(n, XC"show_setting");
//
//    /*printf("type = %s host = %s user = %s pass = %s db = %s port = %s rec = %s char_set = %s\n",
//        type, host, user, pass, db, port, rec, char_set);*/
//
//    /* Make sure we have all of them... */
//    if (!type || !host || !user || !pass || !db || !port || !rec || !char_set || !showsetting) {
//        ERR_LOG("数据库设置不完整");
//        rv = -1;
//        goto err;
//    }
//
//    /* Copy out the strings */
//    cur->dbcfg.type = (char*)type;
//    cur->dbcfg.host = (char*)host;
//    cur->dbcfg.user = (char*)user;
//    cur->dbcfg.pass = (char*)pass;
//    cur->dbcfg.db = (char*)db;
//    cur->dbcfg.auto_reconnect = (char*)rec;
//    cur->dbcfg.char_set = (char*)char_set;
//    cur->dbcfg.show_setting = (char*)showsetting;
//
//
//    /* 分析设置端口参数 */
//    rv2 = strtoul((char*)port, NULL, 0);
//
//    //ERR_LOG("数据库端口: %u", rv2);
//
//    if (rv2 == 0 || rv2 > 0xFFFF) {
//        ERR_LOG("无效数据库端口: %s", (char*)port);
//        rv = -3;
//        goto err;
//    }
//
//    cur->dbcfg.port = (uint16_t)rv2;
//    rv = 0;
//
//err:
//    xmlFree(type);
//    xmlFree(host);
//    xmlFree(user);
//    xmlFree(pass);
//    xmlFree(db);
//    xmlFree(port);
//    xmlFree(rec);
//    xmlFree(char_set);
//    xmlFree(showsetting);
//    return rv;
//}

static int handle_web_server(xmlNode* n, psocn_config_t* cur) {
    xmlChar* web_host, * web_port, * web_patch_motd, * web_login_motd;
    int rv;
    unsigned long rv2;

    /* Grab the attributes of the tag. */
    web_host = xmlGetProp(n, XC"web_host");
    web_port = xmlGetProp(n, XC"web_port");
    web_patch_motd = xmlGetProp(n, XC"web_patch_motd");
    web_login_motd = xmlGetProp(n, XC"web_login_motd");

    /* Make sure we have what we need... */
    if (web_host) {
        cur->w_motd.web_host = (char*)web_host;

        /* 分析设置端口参数 */
        rv2 = strtoul((char*)web_port, NULL, 0);

        if (rv2 == 0 || rv2 > 0xFFFF) {
            ERR_LOG("无效网络服务器端口: %s", (char*)web_port);
            rv = -3;
        }

        cur->w_motd.web_port = (uint16_t)rv2;

        cur->w_motd.patch_welcom_file = (char*)web_patch_motd;
        cur->w_motd.login_welcom_file = (char*)web_login_motd;

        if (0 == psocn_web_server_getfile(cur->w_motd.web_host, cur->w_motd.web_port, cur->w_motd.login_welcom_file, Welcome_Files[1])) {
            CONFIG_LOG("获取登录欢迎信息成功");
            psocn_web_server_loadfile(Welcome_Files[1], &cur->w_motd.login_welcom[0]);
        }
        else {
            ERR_LOG("获取登录欢迎信息失败");
        }

        if (0 == psocn_web_server_getfile(cur->w_motd.web_host, cur->w_motd.web_port, cur->w_motd.patch_welcom_file, Welcome_Files[0])) {
            CONFIG_LOG("获取补丁欢迎信息成功");
            psocn_web_server_loadfile(Welcome_Files[0], &cur->w_motd.patch_welcom[0]);
        }
        else {
            ERR_LOG("获取补丁欢迎信息失败");
        }

    }
    else {
        ERR_LOG("未开启网络服务器");
        rv = 0;
    }

    rv = 0;

    return rv;
}

static int handle_shipgate(xmlNode* n, psocn_config_t* cur) {
    xmlChar* port, * cert, * key, * ca;
    int rv = 0;
    unsigned long rv2;

    /* Grab the attributes of the tag. */
    port = xmlGetProp(n, XC"port");
    cert = xmlGetProp(n, XC"cert");
    key = xmlGetProp(n, XC"key");
    ca = xmlGetProp(n, XC"ca-cert");

    /* Make sure we have what we need... */
    if (!port || !cert || !key || !ca) {
        ERR_LOG("船闸设置参数不完整");
        rv = -1;
        goto err;
    }

    /* 分析设置端口参数 */
    rv2 = strtoul((char*)port, NULL, 0);

    if (rv2 == 0 || rv2 > 0xFFFF) {
        ERR_LOG("船闸端口无效: %s", (char*)port);
        rv = -3;
        goto err;
    }

    cur->sgcfg.shipgate_port = (uint16_t)rv2;

    /* Grab the certificate file */
    cur->sgcfg.shipgate_cert = (char*)cert;
    cur->sgcfg.shipgate_key = (char*)key;
    cur->sgcfg.shipgate_ca = (char*)ca;

err:
    if (rv < 0) {
        xmlFree(ca);
        xmlFree(key);
        xmlFree(cert);
    }

    xmlFree(port);
    return rv;
}

static int handle_quests(xmlNode* n, psocn_config_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->quests_dir = (char*)fn;
        return 0;
    }

    /* If we don't have either, report the error */
    ERR_LOG("Malformed quest tag, no dir given");
    return -1;
}

static int handle_limits(xmlNode* n, psocn_config_t* cur) {
    xmlChar* fn, * name, * enforce, * id;
    int enf = 0, i;
    void* tmp;
    unsigned long idn = 0xFFFFFFFF;

    /* Grab the attributes of the tag. */
    id = xmlGetProp(n, XC"id");
    fn = xmlGetProp(n, XC"file");
    name = xmlGetProp(n, XC"name");
    enforce = xmlGetProp(n, XC"enforce");

    /* Make sure we have the data */
    if (!fn) {
        ERR_LOG("Limits file not given in limits tag.");
        goto err;
    }

    if ((!id && name) || (id && !name)) {
        ERR_LOG("Must give both or none of id and name for limits.");
        goto err;
    }

    if (enforce) {
        if (!xmlStrcmp(enforce, XC"true")) {
            enf = 1;
        }
        else if (xmlStrcmp(enforce, XC"false")) {
            ERR_LOG("Invalid enforce value for limits file: %s",
                (char*)enforce);
            goto err;
        }
    }
    /* This is really !id && !name, but per the above, if one is not set, then
       the other must not be set either, so this works. */
    else if (!id) {
        enf = 1;
    }

    /* Make sure we don't already have an enforced limits file. */
    if (enf && cur->limits_enforced != -1) {
        ERR_LOG("Cannot have more than one enforced limits file!");
        goto err;
    }

    if (id) {
        /* Parse the id out */
        idn = (uint32_t)strtoul((char*)id, NULL, 0);

        if (idn < 0x80000000 || idn > 0xFFFFFFFF) {
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
        cur->limits_enforced = cur->limits_count;

    ++cur->limits_count;

    xmlFree(enforce);
    xmlFree(id);

    return 0;

err:
    xmlFree(id);
    xmlFree(fn);
    xmlFree(name);
    xmlFree(enforce);
    return -1;
}

static int handle_info(xmlNode* n, psocn_config_t* cur, int is_motd) {
    xmlChar* fn, * desc, * gc, * ep3, * bb, * lang, * xb;
    void* tmp;
    int rv = 0, count = cur->info_file_count, i, done = 0;
    char* lasts = { 0 }, * token;

    /* Grab the attributes of the tag. */
    fn = xmlGetProp(n, XC"file");
    desc = xmlGetProp(n, XC"desc");
    gc = xmlGetProp(n, XC"gc");
    ep3 = xmlGetProp(n, XC"ep3");
    bb = xmlGetProp(n, XC"bb");
    xb = xmlGetProp(n, XC"xbox");
    lang = xmlGetProp(n, XC"languages");

    /* Make sure we have all of them... */
    if (!fn || !gc || !ep3 || !bb) {
        ERR_LOG("标签设置不完整");
        rv = -1;
        goto err;
    }

    if (!desc && !is_motd) {
        ERR_LOG("标签设置不完整");
        rv = -1;
        goto err;
    }
    else if (desc && is_motd) {
        ERR_LOG("MOTD 无需拥有描述! 忽略.");
    }

    /* Allocate space for the new description. */
    tmp = realloc(cur->info_files, (count + 1) * sizeof(psocn_info_file_t));
    if (!tmp) {
        ERR_LOG("Couldn't allocate space for info file");
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
    if (!xmlStrcmp(gc, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_GC;

    if (!xmlStrcmp(ep3, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_EP3;

    if (!xmlStrcmp(bb, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_BB;

    if (xb && !xmlStrcmp(xb, XC"true"))
        cur->info_files[count].versions |= PSOCN_INFO_XBOX;

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
                ERR_LOG("Ignoring unknown language in info/motd tag on "
                    "line %hu: %s", n->line, token);
            }

            done = 0;
            token = strtok_s(NULL, ", ", &lasts);
        }
    }
    else {
        cur->info_files[count].languages = 0xFFFFFFFF;
    }

    ++cur->info_file_count;

    xmlFree(xb);
    xmlFree(lang);
    xmlFree(bb);
    xmlFree(ep3);
    xmlFree(gc);

    return 0;

err:
    xmlFree(xb);
    xmlFree(lang);
    xmlFree(bb);
    xmlFree(ep3);
    xmlFree(gc);
    xmlFree(fn);
    xmlFree(desc);
    return rv;
}

static int handle_log(xmlNode* n, psocn_config_t* cur) {
    xmlChar* dir, * pfx;

    /* Grab the data from the tag */
    dir = xmlGetProp(n, XC"dir");
    pfx = xmlGetProp(n, XC"prefix");

    /* If we don't have the directory, report an error */
    if (!dir) {
        ERR_LOG("Malformed log tag, no dir given");
        goto err;
    }

    /* If no prefix was given, then blank it out. */
    if (!pfx) {
        pfx = (xmlChar*)xmlMalloc(1);
        pfx[0] = 0;
    }

    /* Save the data to the configuration struct. */
    cur->log_dir = (char*)dir;
    cur->log_prefix = (char*)pfx;
    return 0;

err:
    xmlFree(dir);
    xmlFree(pfx);
    return -1;
}

static int handle_patch(xmlNode* n, psocn_config_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->patch_dir = (char*)fn;
        return 0;
    }

    /* If we don't have the directory, report the error */
    ERR_LOG("Malformed patch tag, no dir given");
    return -1;
}

static int handle_scripts(xmlNode* n, psocn_config_t* cur) {
    xmlChar* fn;

    /* Grab the files, if given */
    if ((fn = xmlGetProp(n, XC"shipgate")))
        cur->sg_scripts_file = (char*)fn;
    if ((fn = xmlGetProp(n, XC"auth")))
        cur->auth_scripts_file = (char*)fn;

    return 0;
}

static int handle_socket(xmlNode* n, psocn_config_t* cur) {
    xmlChar* fn;

    /* Grab the directory, if given */
    if ((fn = xmlGetProp(n, XC"dir"))) {
        cur->socket_dir = (char*)fn;
        return 0;
    }

    ERR_LOG("socket 套接字设置格式错误,未提供目录");
    return -1;
}

static int handle_registration(xmlNode* n, psocn_config_t* cur) {
    xmlChar* t;

    /* Grab the attributes, if given. */
    if ((t = xmlGetProp(n, XC"dc"))) {
        if (!xmlStrcmp(t, XC"true")) {
            cur->registration_required |= PSOCN_REG_DC;
        }
        else if (xmlStrcmp(t, XC"false")) {
            xmlFree(t);
            return -1;
        }

        xmlFree(t);
    }

    if ((t = xmlGetProp(n, XC"dcnte"))) {
        if (!xmlStrcmp(t, XC"false")) {
            cur->registration_required &= ~PSOCN_REG_DCNTE;
        }
        else if (xmlStrcmp(t, XC"true")) {
            xmlFree(t);
            return -1;
        }

        xmlFree(t);
    }

    if ((t = xmlGetProp(n, XC"pc"))) {
        if (!xmlStrcmp(t, XC"false")) {
            cur->registration_required &= ~PSOCN_REG_PC;
        }
        else if (xmlStrcmp(t, XC"true")) {
            xmlFree(t);
            return -1;
        }

        xmlFree(t);
    }

    if ((t = xmlGetProp(n, XC"gc"))) {
        if (!xmlStrcmp(t, XC"false")) {
            cur->registration_required &= ~PSOCN_REG_GC;
        }
        else if (xmlStrcmp(t, XC"true")) {
            xmlFree(t);
            return -1;
        }

        xmlFree(t);
    }

    if ((t = xmlGetProp(n, XC"xbox"))) {
        if (!xmlStrcmp(t, XC"false")) {
            cur->registration_required &= ~PSOCN_REG_XBOX;
        }
        else if (xmlStrcmp(t, XC"true")) {
            xmlFree(t);
            return -1;
        }

        xmlFree(t);
    }

    if ((t = xmlGetProp(n, XC"bb"))) {
        if (!xmlStrcmp(t, XC"false")) {
            cur->registration_required &= ~PSOCN_REG_BB;
        }
        else if (xmlStrcmp(t, XC"true")) {
            xmlFree(t);
            return -1;
        }

        xmlFree(t);
    }

    return 0;
}

void psocn_free_config(psocn_config_t* cfg) {
    int i;

    /* Make sure we actually have a valid configuration pointer. */
    if (cfg) {
        for (i = 0; i < cfg->info_file_count; ++i) {
            xmlFree(cfg->info_files[i].filename);
            xmlFree(cfg->info_files[i].desc);
        }

        for (i = 0; i < cfg->limits_count; ++i) {
            xmlFree(cfg->limits[i].filename);
            xmlFree(cfg->limits[i].name);
        }

        /* Clean up the pointers */
        xmlFree(cfg->dbcfg.type);
        xmlFree(cfg->dbcfg.host);
        xmlFree(cfg->dbcfg.user);
        xmlFree(cfg->dbcfg.pass);
        xmlFree(cfg->dbcfg.db);
        xmlFree(cfg->dbcfg.auto_reconnect);
        xmlFree(cfg->dbcfg.char_set);
        xmlFree(cfg->dbcfg.show_setting);
        xmlFree(cfg->sgcfg.shipgate_cert);
        xmlFree(cfg->sgcfg.shipgate_key);
        xmlFree(cfg->sgcfg.shipgate_ca);
        xmlFree(cfg->quests_dir);
        xmlFree(cfg->log_dir);
        xmlFree(cfg->log_prefix);
        xmlFree(cfg->patch_dir);
        xmlFree(cfg->sg_scripts_file);
        xmlFree(cfg->auth_scripts_file);
        xmlFree(cfg->socket_dir);

        free_safe(cfg->info_files);
        free_safe(cfg->limits);

        /* Clean up the base structure. */
        free_safe(cfg);
    }
}

int psocn_read_config(const char* f, psocn_config_t** cfg) {
    xmlParserCtxtPtr cxt;
    xmlDoc* doc;
    xmlNode* n;
    int irv = 0;
    psocn_config_t* rv;

    /* Allocate space for the base of the config. */
    rv = (psocn_config_t*)malloc(sizeof(psocn_config_t));

    if (!rv) {
        *cfg = NULL;
        ERR_LOG("无法为设置分配内存空间");
        perror("malloc");
        return -1;
    }

    /* Clear out the config. */
    memset(rv, 0, sizeof(psocn_config_t));
    rv->limits_enforced = -1;
    rv->registration_required = PSOCN_REG_DCNTE | PSOCN_REG_PC |
        PSOCN_REG_GC | PSOCN_REG_XBOX | PSOCN_REG_BB;

    /* Create an XML Parsing context */
    cxt = xmlNewParserCtxt();
    if (!cxt) {
        ERR_LOG("无法为配置创建分析上下文");
        irv = -2;
        goto err;
    }

    /* Open the configuration file for reading. */
    if (f) {
        doc = xmlReadFile(f, NULL, XML_PARSE_DTDVALID);
    }
    else {
        doc = xmlReadFile(psocn_global_cfg, NULL, XML_PARSE_DTDVALID);
    }

    if (!doc) {
        xmlParserError(cxt, "分析配置文件出错");
        irv = -3;
        goto err_cxt;
    }

    /* Make sure the document validated properly. */
    if (!cxt->valid) {
        xmlParserValidityError(cxt, "分析配置文件有效性错误");
        irv = -4;
        goto err_doc;
    }

    /* If we've gotten this far, we have a valid document, now go through and
       add in entries for everything... */
    n = xmlDocGetRootElement(doc);

    if (!n) {
        ERR_LOG("配置文件为空");
        irv = -5;
        goto err_doc;
    }

    /* Make sure the config looks sane. */
    if (xmlStrcmp(n->name, XC"psocn_global_config")) {
        ERR_LOG("配置类型错误");
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
        /*else if (!xmlStrcmp(n->name, XC"server")) {
            if (handle_server(n, rv)) {
                irv = -7;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"database")) {
            if (handle_database(n, rv)) {
                irv = -8;
                goto err_doc;
            }
        }*/
        else if (!xmlStrcmp(n->name, XC"quests")) {
            if (handle_quests(n, rv)) {
                irv = -9;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"limits")) {
            if (handle_limits(n, rv)) {
                irv = -10;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"info")) {
            if (handle_info(n, rv, 0)) {
                irv = -11;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"motd")) {
            if (handle_info(n, rv, 1)) {
                irv = -12;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"shipgate")) {
            if (handle_shipgate(n, rv)) {
                irv = -13;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"log")) {
            if (handle_log(n, rv)) {
                irv = -14;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"patch")) {
            if (handle_patch(n, rv)) {
                irv = -15;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"scripts")) {
            if (handle_scripts(n, rv)) {
                irv = -16;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"socket")) {
            if (handle_socket(n, rv)) {
                irv = -17;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"registration_required")) {
            if (handle_registration(n, rv)) {
                irv = -18;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"web_server")) {
            if (handle_web_server(n, rv)) {
                irv = -19;
                goto err_doc;
            }
        }
        else {
            ERR_LOG("无效设置标签 %s 设置行 %hu", (char*)n->name,
                n->line);
        }

        n = n->next;
    }

    *cfg = rv;

    /* Cleanup/error handling below... */
err_doc:
    xmlFreeDoc(doc);
err_cxt:
    xmlFreeParserCtxt(cxt);
err:
    if (irv && irv > -7) {
        free_safe(rv);
        *cfg = NULL;
    }
    else if (irv) {
        psocn_free_config(rv);
        *cfg = NULL;
    }

    return irv;
}

int psocn_web_server_loadfile(const char* onlinefile, char* dest) {
    int32_t filesize, ch2;
    uint16_t* w;
    uint16_t* w2;
    FILE* fp = { 0 };

    uint8_t msgdata[2048] = { 0 };

    memset(dest, 0, 2048);

    size_t len = 0;
    errno_t err = fopen_s(&fp, onlinefile, "rb");
    if (err)
        ERR_EXIT("文件缺失了.请确保它在 %s 文件夹中", onlinefile);
    else {
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (filesize > 2048)
            filesize = 2048;
        fread(&msgdata[0], 1, filesize, fp);
        msgdata[2047] = '\0';
        fclose(fp);
        w = (uint16_t*)&msgdata[0];
        w2 = (uint16_t*)&dest[0];
        for (ch2 = 0; ch2 < filesize; ch2 += 2)
        {
            if (*w == 0x0024)
                *w = 0x0009;
            if (*w != 0x000D)
            {
                *(w2++) = *w;
                len += 2;
            }
            w++;
        }

        *w2 = 0x0000;

        return len;
    }
}

int psocn_web_server_loadfile2(const char* onlinefile, uint16_t* dest) {
    int32_t filesize, ch2;
    uint16_t* w;
    uint16_t* w2;
    FILE* fp = { 0 };

    uint8_t msgdata[4096] = { 0 };

    memset(dest, 0, 2048);

    size_t len = 0;
    errno_t err = fopen_s(&fp, onlinefile, "rb");
    if (err)
        ERR_EXIT("文件缺失了.请确保它在 %s 文件夹中", onlinefile);
    else {
        fseek(fp, 0, SEEK_END);
        filesize = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (filesize > 4096)
            filesize = 4096;
        fread(&msgdata[0], 1, filesize, fp);
        msgdata[4095] = '\0';
        fclose(fp);
        w = (uint16_t*)&msgdata[0];
        w2 = dest;
        for (ch2 = 0; ch2 < (filesize / 2); ch2++)
        {
            if (*w == 0x0024)
                *w = 0x0009;
            if (*w != 0x000D)
            {
                *(w2++) = *w;
                len += 2;
            }
            w++;
        }

        *w2 = 0x0000;

        return len;
    }
}

int psocn_web_server_getfile(void* HostName, int32_t port, char* file, const char* onlinefile) 
{
    struct hostent* hptr = gethostbyname((char*)HostName);

    //用域名获取对方主机名
    if (hptr == NULL) {
        return -1; //ERR_LOG("网络服务器连接失败,采用离线文件读取");
    }
    else
    {
        //printf(hptr->h_addr);

        //只支持IPV4
        if (hptr->h_addrtype != PF_INET) {
            //ERR_LOG("网络地址类型不是IPV4");
            return -1;
        }

        struct in_addr ina;
        //解析IP
        memmove(&ina, hptr->h_addr, 4);
        LPSTR ipstr = inet_ntoa(ina);

        //Socket封装
        struct sockaddr_in si = { 0 };
        si.sin_family = PF_INET;
        si.sin_port = htons(port);
        si.sin_addr.S_un.S_addr = inet_addr(ipstr);
        int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        connect(sock, (SOCKADDR*)&si, sizeof(si));
        if (sock == SOCKET_ERROR || sock == -2) {
            PATCH_LOG("网络串口有误");
            return -1;
        }
        _wsetlocale(0, L"chs");
        char buf[255] = { 0 };
        char WebServer[255] = { 0 };
        int nRand = (uint32_t)time(NULL);
        snprintf(WebServer, sizeof(WebServer), "http://%s:%d%s?rand=%d", (char*)HostName, port, file, nRand);
        //printf("网络路径: %s\n", WebServer);
        if (_getcwd(buf, sizeof(buf))) {
            strcat(buf, "\\");
            strcat(buf, onlinefile);
            //printf("本地路径: %s\n", buf);
            URLDownloadToFileA(0, WebServer, buf, 0, NULL);
            return 0;
        }
        else
        {
            ERR_LOG("获取文件路径失败, 文件保存失败.");
            return -1;
        }

        closesocket(sock);
        WSACleanup();
    }
}

// 回调函数，用于写入接收到的数据
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    FILE* file = (FILE*)userp;
    size_t written = fwrite(contents, size, nmemb, file);
    return written;
}

int download_txt_from_network(const char* url, const char* filename) {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in serverAddr = { 0 };
    char request[1024] = { 0 };
    char response[8192] = { 0 };
    int responseSize, recvSize, fileSize;
    FILE* file;

    // 初始化 WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        fprintf(stderr, "Failed to initialize WinSock\n");
        return -1;
    }

    // 创建套接字
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "Failed to create socket\n");
        WSACleanup();
        return -1;
    }

    // 解析服务器 IP 地址和端口号
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);
    serverAddr.sin_addr.s_addr = inet_addr(url);

    // 连接服务器
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr))) {
        fprintf(stderr, "Failed to connect to server\n");
        WSACleanup();
        closesocket(sock);
        return -1;
    }

    // 构造 HTTP 请求报文
    sprintf(request, "GET /%s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n",
        filename, url);

    // 发送请求
    if (send(sock, request, strlen(request), 0) == SOCKET_ERROR) {
        fprintf(stderr, "Failed to send request\n");
        WSACleanup();
        closesocket(sock);
        return -1;
    }

    // 接收响应
    responseSize = 0;
    fileSize = -1;
    while ((recvSize = recv(sock, response + responseSize, sizeof(response) - responseSize - 1, 0)) > 0) {
        responseSize += recvSize;
        response[responseSize] = '\0';
        if (fileSize == -1) {
            char* headEnd = strstr(response, "\r\n\r\n");
            if (headEnd) {
                fileSize = responseSize - (headEnd - response) - 4;
                file = fopen(filename, "wb");
                if (!file) {
                    fprintf(stderr, "Failed to open file for writing\n");
                    WSACleanup();
                    closesocket(sock);
                    return -1;
                }
                fwrite(headEnd + 4, fileSize, 1, file);
            }
        }
        else {
            fwrite(response + responseSize - recvSize, recvSize, 1, file);
        }
    }

    // 关闭套接字和文件
    fclose(file);
    WSACleanup();
    closesocket(sock);

    if (fileSize > 0) {
        printf("File downloaded successfully\n");
        return 0;
    }
    else {
        printf("Failed to download file\n");
        return -1;
    }
}