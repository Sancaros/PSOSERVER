/*
    梦幻之星中国 数据库
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

#include "WinSock_Defines.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "psoconfig.h"
#include "f_logs.h"
#include "debug.h"

#ifndef LIBXML_TREE_ENABLED
#error You must have libxml2 with tree support built-in.
#endif

#define XC (const xmlChar *)

static int handle_database(xmlNode* n, psocn_dbconfig_t* cur) {
    xmlChar* type, * host, * user, * pass, * db, * port, * unix_socket, * rec, * char_set, *showsetting;
    int rv;
    unsigned long rv2;

    /* Grab the attributes of the tag. */
    type = xmlGetProp(n, XC"type");
    host = xmlGetProp(n, XC"host");
    user = xmlGetProp(n, XC"user");
    pass = xmlGetProp(n, XC"pass");
    db = xmlGetProp(n, XC"db");
    port = xmlGetProp(n, XC"port");
    unix_socket = xmlGetProp(n, XC"unix_socket");
    rec = xmlGetProp(n, XC"auto_reconnect");
    char_set = xmlGetProp(n, XC"char_set");
    showsetting = xmlGetProp(n, XC"show_setting");

    /*printf("type = %s host = %s user = %s pass = %s db = %s port = %s rec = %s char_set = %s\n",
        type, host, user, pass, db, port, rec, char_set);*/

    /* Make sure we have all of them... */
    if (!type || !host || !user || !pass || !db || !port || !rec || !char_set || !showsetting) {
        ERR_LOG("数据库设置不完整");
        rv = -1;
        goto err;
    }

    /* Copy out the strings */

    /* Copy out the strings */
    cur->type = (char*)type;
    cur->host = (char*)host;
    cur->user = (char*)user;
    cur->pass = (char*)pass;
    cur->db = (char*)db;
    cur->unix_socket = (char*)unix_socket;
    cur->auto_reconnect = (char*)rec;
    cur->char_set = (char*)char_set;
    cur->show_setting = (char*)showsetting;


    /* 分析设置端口参数 */
    rv2 = strtoul((char*)port, NULL, 0);

    if (rv2 == 0 || rv2 > 0xFFFF) {
        ERR_LOG("数据库端口无效: %s", (char*)port);
        rv = -3;
        goto err;
    }

    cur->port = (uint16_t)rv2;
    rv = 0;

err:
    xmlFree(type);
    xmlFree(host);
    xmlFree(user);
    xmlFree(pass);
    xmlFree(db);
    xmlFree(port);
    xmlFree(unix_socket);
    xmlFree(rec);
    xmlFree(char_set);
    xmlFree(showsetting);
    return rv;
}

void psocn_free_db_config(psocn_dbconfig_t* cfg) {
    /* Make sure we actually have a valid configuration pointer. */
    if (cfg) {
        /* Clean up the pointers */
        xmlFree(cfg->type);
        xmlFree(cfg->host);
        xmlFree(cfg->user);
        xmlFree(cfg->pass);
        xmlFree(cfg->db);
        xmlFree(cfg->auto_reconnect);
        xmlFree(cfg->char_set);
        xmlFree(cfg->show_setting);

        /* Clean up the base structure. */
        free_safe(cfg);
    }
}

int psocn_read_db_config(const char* f, psocn_dbconfig_t** cfg) {
    xmlParserCtxtPtr cxt;
    xmlDoc* doc;
    xmlNode* n;
    int irv = 0;
    psocn_dbconfig_t* rv;

    /* Allocate space for the base of the config. */
    rv = (psocn_dbconfig_t*)malloc(sizeof(psocn_dbconfig_t));

    if (!rv) {
        *cfg = NULL;
        ERR_LOG("无法为设置分配内存空间");
        perror("malloc");
        return -1;
    }

    /* Clear out the config. */
    memset(rv, 0, sizeof(psocn_dbconfig_t));

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
        doc = xmlReadFile(psocn_database_cfg, NULL, XML_PARSE_DTDVALID);
    }

    if (!doc) {
        xmlParserError(cxt, "分析配置时出错\n");
        irv = -3;
        goto err_cxt;
    }

    /* Make sure the document validated properly. */
    if (!cxt->valid) {
        xmlParserValidityError(cxt, "分析配置有效性时出错\n");
        irv = -4;
        goto err_doc;
    }

    /* If we've gotten this far, we have a valid document, now go through and
       add in entries for everything... */
    n = xmlDocGetRootElement(doc);

    if (!n) {
        ERR_LOG("设置文档为空");
        irv = -5;
        goto err_doc;
    }

    /* Make sure the config looks sane. */
    if (xmlStrcmp(n->name, XC"psocn_database_config")) {
        ERR_LOG("配置似乎不是正确的类型");
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
        else if (!xmlStrcmp(n->name, XC"database")) {
            if (handle_database(n, rv)) {
                irv = -7;
                goto err_doc;
            }
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
        psocn_free_db_config(rv);
        *cfg = NULL;
    }

    return irv;
}
