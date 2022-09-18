/*
    梦幻之星中国 舰船服务器
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
#include <inttypes.h>

#include <f_logs.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "gm.h"
#include "ship.h"
#include "clients.h"

#ifndef LIBXML_TREE_ENABLED
#error You must have libxml2 with tree support built-in.
#endif

#define XC (const xmlChar *)

int gm_list_read(const char *fn, ship_t *s) {
    xmlParserCtxtPtr cxt;
    xmlDoc *doc;
    xmlNode *n;
    xmlChar *guildcard, *root;
    local_gm_t *oldlist = NULL;
    int oldcount = 0, rv = 0;
    void *tmp;

    /* If we're reloading, save the old list. */
    if(s->gm_list) {
        oldlist = s->gm_list;
        oldcount = s->gm_count;
        s->gm_list = NULL;
        s->gm_count = 0;
    }

    /* Create an XML Parsing context */
    cxt = xmlNewParserCtxt();
    if(!cxt) {
        ERR_LOG("无法为 GM 列表创建XML分析上下文");
        rv = -1;
        goto err;
    }

    /* Open the GM list XML file for reading. */
    doc = xmlReadFile(fn, NULL, XML_PARSE_DTDVALID);
    if(!doc) {
        xmlParserError(cxt, "分析 GM 列表发生错误");
        rv = -2;
        goto err_cxt;
    }

    /* Make sure the document validated properly. */
    if(!cxt->valid) {
        xmlParserValidityError(cxt, "分析 GM 列表有效性失败");
        rv = -3;
        goto err_doc;
    }

    /* If we've gotten this far, we have a valid document, now go through and
       add in entries for everything... */
    n = xmlDocGetRootElement(doc);

    if(!n) {
        ERR_LOG("GM 列表文件为空");
        rv = -4;
        goto err_doc;
    }

    /* Make sure the list looks sane. */
    if(xmlStrcmp(n->name, XC"gms")) {
        ERR_LOG("GM 列表的类型似乎不正确");
        rv = -5;
        goto err_doc;
    }

    n = n->children;
    while(n) {
        if(n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if(xmlStrcmp(n->name, XC"gm")) {
            ERR_LOG("无效设置标签 %s 行 %hu", n->name, n->line);
        }
        else {
            /* We've got the right tag, see if we have all the attributes... */
            guildcard = xmlGetProp(n, XC"guildcard");
            root = xmlGetProp(n, XC"root");

            if(!guildcard) {
                ERR_LOG("GM 设置条目不完整 行 %hu", n->line);
                goto next;
            }
        
            /* We've got everything, make space for the new GM. */
            tmp = realloc(s->gm_list, (s->gm_count + 1) * sizeof(local_gm_t));

            if(!tmp) {
                ERR_LOG("无法为 GM 分配内存空间");
                perror("realloc");
                xmlFree(guildcard);
                xmlFree(root);
                rv = -6;
                free_safe(s->gm_list);
                goto err_doc;
            }

            s->gm_list = (local_gm_t *)tmp;

            /* Clear it */
            memset(&s->gm_list[s->gm_count], 0, sizeof(local_gm_t));
            s->gm_list[s->gm_count].flags = CLIENT_PRIV_LOCAL_GM;

            s->gm_list[s->gm_count].guildcard =
                (uint32_t)strtoul((char *)guildcard, NULL, 0);

            /* See if the user is a root user */
            if(root && !xmlStrcmp(root, XC"true")) {
                s->gm_list[s->gm_count].flags |= CLIENT_PRIV_LOCAL_ROOT;
            }

            ++s->gm_count;

next:
            /* Free the memory we allocated here... */
            xmlFree(guildcard);
            xmlFree(root);
        }

        n = n->next;
    }

    /* If we have an old list, we're clear to free it now... */
    if(oldlist) {
        free_safe(oldlist);
    }

    /* Cleanup/error handling below... */
err_doc:
    xmlFreeDoc(doc);
err_cxt:
    xmlFreeParserCtxt(cxt);
err:
    if(rv) {
        s->gm_list = oldlist;
        s->gm_count = oldcount;
    }

    return rv;
}

int is_gm(uint32_t guildcard, ship_t *s) {
    int i;

    /* Look through the list for this person. */
    for(i = 0; i < s->gm_count; ++i) {
        if(guildcard == s->gm_list[i].guildcard) {
            return s->gm_list[i].flags;
        }
    }

    /* Didn't find them, they're not a GM. */
    return 0;
}
