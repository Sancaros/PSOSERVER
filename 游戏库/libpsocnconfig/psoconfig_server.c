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

static int handle_server(xmlNode* n, psocn_srvconfig_t* cur) {
    xmlChar* addr4, * addr6, * ip4, * ip6, 
        * pc_patch_port, * pc_data_port, * web_port, * bb_patch_port, * bb_data_port;
    int rv;

    /* Grab the attributes of the tag. */
    addr4 = xmlGetProp(n, XC"addr4");
    addr6 = xmlGetProp(n, XC"addr6");
    ip4 = xmlGetProp(n, XC"ip4");
    ip6 = xmlGetProp(n, XC"ip6");
    pc_patch_port = xmlGetProp(n, XC"pc_patch_port");
    pc_data_port = xmlGetProp(n, XC"pc_data_port");
    web_port = xmlGetProp(n, XC"web_port");
    bb_patch_port = xmlGetProp(n, XC"bb_patch_port");
    bb_data_port = xmlGetProp(n, XC"bb_data_port");

    /* Make sure we have what we need... */
    if (!ip4 || !addr4) {
        ERR_LOG("服务器域名或IP未填写");
        rv = -1;
        goto err;
    }

    if (!pc_patch_port || !pc_data_port || !web_port || !bb_patch_port || !bb_data_port) {
        ERR_LOG("服务器端口未填写");
        rv = -1;
        goto err;
    }

    cur->host4 = (char*)addr4;

    //CONFIG_LOG("检测域名获取: %s", host4);

    if (check_ipaddr((char*)ip4)) {
        /* Parse the IP address out */
        rv = inet_pton(PF_INET, (char*)ip4, &cur->server_ip);
        //ERR_LOG("IP串码 %d rv = %d", &cur->server_ip, rv);
        if (rv < 1) {
            ERR_LOG("未获取到正确的 IP 地址: %s",
                (char*)ip4);
            rv = -2;
            goto err;
        }
    }

    /* See if we have a configured IPv6 address */
    if (ip6 || addr6) {

        cur->host6 = (char*)addr6;

        if (ip6) {
            rv = inet_pton(PF_INET6, (char*)ip6, cur->server_ip6);

            /* This isn't actually fatal, for now, anyway. */
            if (rv < 1) {
                //ERR_LOG("无效 IPv6 地址: %s", (char*)ip6);
            }
        }
    }

    /* Parse the port out */
    rv = (int)strtoul((char*)pc_patch_port, NULL, 0);

    if (rv == 0 || rv > 0xFFFF) {
        ERR_LOG("端口无效: %s", (char*)pc_patch_port);
        rv = -3;
        goto err;
    }

    cur->patch_port.ptports[0] = rv;

    /* Parse the port out */
    rv = (int)strtoul((char*)pc_data_port, NULL, 0);

    if (rv == 0 || rv > 0xFFFF) {
        ERR_LOG("端口无效: %s", (char*)pc_data_port);
        rv = -3;
        goto err;
    }

    cur->patch_port.ptports[1] = rv;

    /* Parse the port out */
    rv = (int)strtoul((char*)web_port, NULL, 0);

    if (rv == 0 || rv > 0xFFFF) {
        ERR_LOG("端口无效: %s", (char*)web_port);
        rv = -3;
        goto err;
    }

    cur->patch_port.ptports[2] = rv;

    /* Parse the port out */
    rv = (int)strtoul((char*)bb_patch_port, NULL, 0);

    if (rv == 0 || rv > 0xFFFF) {
        ERR_LOG("端口无效: %s", (char*)bb_patch_port);
        rv = -3;
        goto err;
    }

    cur->patch_port.ptports[3] = rv;

    /* Parse the port out */
    rv = (int)strtoul((char*)bb_data_port, NULL, 0);

    if (rv == 0 || rv > 0xFFFF) {
        ERR_LOG("端口无效: %s", (char*)bb_data_port);
        rv = -3;
        goto err;
    }

    cur->patch_port.ptports[4] = rv;

    DBG_LOG("%d", cur->patch_port.ptports[4]);

    rv = 0;

err:
    xmlFree(addr4);
    xmlFree(addr6);
    xmlFree(ip4);
    xmlFree(ip6);
    xmlFree(pc_patch_port);
    xmlFree(pc_data_port);
    xmlFree(web_port);
    xmlFree(bb_patch_port);
    xmlFree(bb_data_port);
    return rv;
}

void psocn_free_srv_config(psocn_srvconfig_t* cfg) {
    /* Make sure we actually have a valid configuration pointer. */
    if (cfg) {
        /* Clean up the pointers */
        xmlFree(cfg->host4);
        xmlFree(cfg->host6);

        free_safe(cfg->host4);
        free_safe(cfg->host6);

        /* Clean up the base structure. */
        free_safe(cfg);
    }
}

int psocn_read_srv_config(const char* f, psocn_srvconfig_t** cfg) {
    xmlParserCtxtPtr cxt;
    xmlDoc* doc;
    xmlNode* n;
    int irv = 0;
    psocn_srvconfig_t* rv;

    /* Allocate space for the base of the config. */
    rv = (psocn_srvconfig_t*)malloc(sizeof(psocn_srvconfig_t));

    if (!rv) {
        *cfg = NULL;
        ERR_LOG("无法为设置分配内存空间");
        perror("malloc");
        return -1;
    }

    /* Clear out the config. */
    memset(rv, 0, sizeof(psocn_srvconfig_t));

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
        doc = xmlReadFile(psocn_server_cfg, NULL, XML_PARSE_DTDVALID);
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
    if (xmlStrcmp(n->name, XC"psocn_server_config")) {
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
        else if (!xmlStrcmp(n->name, XC"server")) {
            if (handle_server(n, rv)) {
                irv = -7;
                goto err_doc;
            }
        }
        else {
            ERR_LOG("无效标签 %s 行 %hu", (char*)n->name,
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
        psocn_free_srv_config(rv);
        *cfg = NULL;
    }

    return irv;
}