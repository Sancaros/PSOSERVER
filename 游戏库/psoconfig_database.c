/*
    梦幻之星中国 数据库
    版权 (C) 2022 Sancaros

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
#include "debug.h"
#include "f_logs.h"

#ifndef LIBXML_TREE_ENABLED
#error You must have libxml2 with tree support built-in.
#endif

#define XC (const xmlChar *)

static int handle_database(xmlNode* n, sylverant_dbconfig_t* cur) {
    xmlChar* type, * host, * user, * pass, * db, * port;
    int rv;
    unsigned long rv2;

    /* Grab the attributes of the tag. */
    type = xmlGetProp(n, XC"type");
    host = xmlGetProp(n, XC"host");
    user = xmlGetProp(n, XC"user");
    pass = xmlGetProp(n, XC"pass");
    db = xmlGetProp(n, XC"db");
    port = xmlGetProp(n, XC"port");

    /* Make sure we have all of them... */
    if (!type || !host || !user || !pass || !db || !port) {
        ERR_LOG("数据库设置不完整");
        rv = -1;
        goto err;
    }

    /* Copy out the strings */
    strncpy(cur->type, (char*)type, 255);
    strncpy(cur->host, (char*)host, 255);
    strncpy(cur->user, (char*)user, 255);
    strncpy(cur->pass, (char*)pass, 255);
    strncpy(cur->db, (char*)db, 255);

    cur->type[255] = '\0';
    cur->host[255] = '\0';
    cur->user[255] = '\0';
    cur->pass[255] = '\0';
    cur->db[255] = '\0';

    /* Parse the port out */
    rv2 = strtoul((char*)port, NULL, 0);

    if (rv2 == 0 || rv2 > 0xFFFF) {
        ERR_LOG("数据库端口无效: %s", (char*)port);
        rv = -3;
        goto err;
    }

    cur->port = (unsigned int)rv2;
    rv = 0;

err:
    xmlFree(type);
    xmlFree(host);
    xmlFree(user);
    xmlFree(pass);
    xmlFree(db);
    xmlFree(port);
    return rv;
}

int sylverant_read_dbconfig(const char* f, sylverant_dbconfig_t* cfg) {
    xmlParserCtxtPtr cxt;
    xmlDoc* doc;
    xmlNode* n;
    int irv = 0;

    /* Clear out the config. */
    memset(cfg, 0, sizeof(sylverant_config_t));

    /* Create an XML Parsing context */
    cxt = xmlNewParserCtxt();
    if (!cxt) {
        ERR_LOG("无法为配置创建分析上下文");
        irv = -1;
        goto err;
    }

    /* Open the configuration file for reading. */
    doc = xmlReadFile(sylverant_database_cfg, NULL, XML_PARSE_DTDVALID);

    if (!doc) {
        xmlParserError(cxt, "分析配置时出错\n");
        irv = -2;
        goto err_cxt;
    }

    /* Make sure the document validated properly. */
    if (!cxt->valid) {
        xmlParserValidityError(cxt, "分析配置有效性时出错\n");
        irv = -3;
        goto err_doc;
    }

    /* If we've gotten this far, we have a valid document, now go through and
       add in entries for everything... */
    n = xmlDocGetRootElement(doc);

    if (!n) {
        ERR_LOG("设置文档为空");
        irv = -4;
        goto err_doc;
    }

    /* Make sure the config looks sane. */
    if (xmlStrcmp(n->name, XC"psocn_database_config")) {
        ERR_LOG("配置似乎不是正确的类型");
        irv = -5;
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
            if (handle_database(n, cfg)) {
                irv = -6;
                goto err_doc;
            }
        }

        n = n->next;
    }

    /* Cleanup/error handling below... */
err_doc:
    xmlFreeDoc(doc);
err_cxt:
    xmlFreeParserCtxt(cxt);
err:
    return irv;
}
