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
#include <iconv.h>
#include <errno.h>

#include "WinSock_Defines.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include "debug.h"
#include "f_logs.h"

#include "psoconfig.h"

#ifndef LIBXML_TREE_ENABLED
#error You must have libxml2 with tree support built-in.
#endif

#define XC (const xmlChar *)

static void patch_free_entry(patch_file_t* e) {
    patch_file_entry_t* ent, * tmp;

    ent = e->entries;
    while (ent) {
        tmp = ent->next;
        xmlFree(ent->filename);
        free_safe(ent);
        ent = tmp;
    }

    xmlFree(e->filename);
    free_safe(e);
}

static int handle_versions(xmlNode* n, patch_config_t* cfg) {
    xmlChar* pc, * bb;
    int rv = 0;

    /* Grab the attributes of the tag. */
    pc = xmlGetProp(n, XC"pc");
    bb = xmlGetProp(n, XC"bb");

    /* Make sure we have the data */
    if (!pc || !bb) {
        ERR_LOG("认证版本未填写");
        rv = -1;
        goto err;
    }

    /* Parse everything out */
    if (!xmlStrcmp(pc, XC"false")) {
        cfg->disallow_pc = 1;
    }

    if (!xmlStrcmp(bb, XC"false")) {
        cfg->disallow_bb = 1;
    }

err:
    xmlFree(bb);
    xmlFree(pc);
    return rv;
}

static int handle_web_server(xmlNode* n, patch_config_t* cur) {
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

        /* Parse the port out */
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

static int handle_welcome(xmlNode* n, patch_config_t* cfg) {
    xmlChar* ver = NULL, * msg = NULL;
    int rv = 0;
    int ispc = 0;
    iconv_t ic;
    size_t in, out;
    char* inptr;
    char* outptr;
    char buf[4096];
    char buf2[4096] = { 0 };
    uint16_t* tmp;
    size_t i;

    /* Make the converting context */
    ic = iconv_open(UTF16LE, UTF8);
    if (ic == (iconv_t)-1) {
        ERR_LOG("无法创建iconv上下文");
        return -1;
    }

    /* Clear the buffer */
    memset(buf, 0, 4096);

    /* Grab the attributes first... */
    ver = xmlGetProp(n, XC"version");

    /* Make sure we got it and it is valid */
    if (!ver) {
        ERR_LOG("未为欢迎消息指定版本");
        rv = -2;
        goto err;
    }

    if (!xmlStrcmp(ver, XC"PC")) {
        ispc = 1;
    }
    else if (!xmlStrcmp(ver, XC"BB")) {
        ispc = 0;
    }
    else {
        ERR_LOG("无效版本设置(BB或PC): %s", (char*)ver);
        rv = -3;
        goto err;
    }

    /* Grab the message from the node */
    if ((msg = xmlNodeListGetString(n->doc, n->children, 1))) {
        /* Convert the message to UTF-16LE */
        in = (size_t)xmlStrlen(msg) + 1;
        inptr = (char*)msg;

        if (ispc) {
            outptr = buf2;

            for (i = 0; i < in && i < 2048; ++i) {
                if (msg[i] == '\n' && (!i || msg[i - 1] != '\r')) {
                    *outptr++ = '\r';
                }

                *outptr++ = msg[i];
            }

            in = (size_t)strlen(buf2) + 1;
            inptr = (char*)buf2;
        }

        out = 4094;
        outptr = buf;
        iconv(ic, &inptr, &in, &outptr, &out);

        /* Allocate space for the string to stay in, and copy it there */
        tmp = (uint16_t*)malloc(4096 - out);
        if (!tmp) {
            ERR_LOG("无法为欢迎信息分配内存空间 "
                "%s", strerror(errno));
            rv = -4;
            goto err;
        }

        memcpy(tmp, buf, 4096 - out);

        /* Put it where it belongs */
        if (ispc) {
            cfg->pc_welcome = tmp;
            cfg->pc_welcome_size = (uint16_t)(4096 - out);
        }
        else {
            cfg->bb_welcome = tmp;
            cfg->bb_welcome_size = (uint16_t)(4096 - out);
        }
    }
    else {
        ERR_LOG("未设置正确欢迎信息");
        rv = 0;
        //goto err;
    }

err:
    iconv_close(ic);
    xmlFree(msg);
    xmlFree(ver);

    return rv;
}

static int handle_clientfile(xmlNode* n, patch_file_t* f) {
    xmlChar* fn;

    /* Grab the long description from the node */
    if ((fn = xmlNodeListGetString(n->doc, n->children, 1))) {
        f->filename = (char*)fn;
    }

    return 0;
}

static int handle_file(xmlNode* n, patch_file_entry_t* f) {
    xmlChar* fn;

    /* Grab the long description from the node */
    if ((fn = xmlNodeListGetString(n->doc, n->children, 1))) {
        f->filename = (char*)fn;
    }

    return 0;
}

static int handle_checksum(xmlNode* n, patch_file_entry_t* f) {
    xmlChar* csum;
    int rv = 0;

    /* Grab the long description from the node */
    if ((csum = xmlNodeListGetString(n->doc, n->children, 1))) {
        errno = 0;
        f->checksum = (uint32_t)strtoul((char*)csum, NULL, 16);

        if (errno) {
            ERR_LOG("在线修补程序的校验无效 行 %hu: %s",
                n->line, (char*)csum);
            rv = -1;
        }
    }

    xmlFree(csum);
    return rv;
}

static int handle_size(xmlNode* n, patch_file_entry_t* f) {
    xmlChar* size;
    int rv = 0;

    /* Grab the long description from the node */
    if ((size = xmlNodeListGetString(n->doc, n->children, 1))) {
        errno = 0;
        f->size = (uint32_t)strtoul((char*)size, NULL, 0);

        if (errno) {
            ERR_LOG("在线修补程序的大小无效 行 %hu: %s",
                n->line, (char*)size);
            rv = -1;
        }
    }

    xmlFree(size);
    return rv;
}

static int handle_entry(xmlNode* n, patch_file_entry_t* ent, int cksum) {
    int have_file = 0, have_checksum = 0, have_size = 0;
    xmlChar* checksum;

    if (cksum) {
        checksum = xmlGetProp(n, XC"checksum");

        if (!checksum) {
            ERR_LOG("行 %hu 需要校验", n->line);
            return -1;
        }

        errno = 0;
        ent->client_checksum = (uint32_t)strtoul((char*)checksum, NULL, 16);

        if (errno) {
            ERR_LOG("在线修补程序的校验和无效 行 %hu: %s",
                n->line, (char*)checksum);
            xmlFree(checksum);
            return -2;
        }

        xmlFree(checksum);
    }

    /* Now that we're done with that, deal with any children of the node */
    n = n->children;
    while (n) {
        if (n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if (!xmlStrcmp(n->name, XC"file")) {
            if (have_file) {
                ERR_LOG("补丁程序的服务器文件名重复"
                    "%hu", n->line);
                return -3;
            }

            if (handle_file(n, ent)) {
                return -4;
            }

            have_file = 1;
        }
        else if (!xmlStrcmp(n->name, XC"checksum")) {
            if (have_checksum) {
                ERR_LOG("补丁重复校验在行 %hu",
                    n->line);
                return -5;
            }

            if (handle_checksum(n, ent)) {
                return -6;
            }

            have_checksum = 1;
        }
        else if (!xmlStrcmp(n->name, XC"size")) {
            if (have_size) {
                ERR_LOG("Duplicate size for patch on line %hu",
                    n->line);
                return -7;
            }

            if (handle_size(n, ent)) {
                return -8;
            }

            have_size = 1;
        }
        else {
            ERR_LOG("无效 %s 标记在行 %hu", (char*)n->name,
                n->line);
        }

        n = n->next;
    }

    /* If we got this far, make sure we got all the required attributes */
    if (!have_file || !have_checksum || !have_size) {
        ERR_LOG("One or more required attributes not set for patch");
        return -9;
    }

    return 0;
}

static int handle_patch(xmlNode* n, struct file_queue* q) {
    xmlChar* enabled;
    int rv;
    patch_file_t* file = NULL;
    patch_file_entry_t* ent, * ent2;
    int have_cfile = 0, have_file = 0, have_checksum = 0, have_size = 0;

    /* Grab the attributes we're expecting */
    enabled = xmlGetProp(n, XC"enabled");

    /* Make sure we have all of them... */
    if (!enabled) {
        ERR_LOG("缺少必需的修补程序属性");
        rv = -1;
        goto err;
    }

    /* If the patch isn't enabled, we can ignore it. */
    if (!xmlStrcmp(enabled, XC"false")) {
        rv = 0;
        goto err;
    }

    /* Allocate space for the file */
    file = (patch_file_t*)malloc(sizeof(patch_file_t));
    if (!file) {
        ERR_LOG("Cannot allocate space for patch info\n"
            "%s", strerror(errno));
        rv = -2;
        goto err;
    }

    memset(file, 0, sizeof(patch_file_t));

    /* Allocate space for the first entry... */
    ent = (patch_file_entry_t*)malloc(sizeof(patch_file_entry_t));
    if (!ent) {
        ERR_LOG("Cannot allocate space for patch entry",
            "%s", strerror(errno));
        rv = -11;
        goto err;
    }

    memset(ent, 0, sizeof(patch_file_entry_t));

    file->entries = ent;

    /* Now that we're done with that, deal with any children of the node */
    n = n->children;
    while (n) {
        if (n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if (!xmlStrcmp(n->name, XC"clientfile")) {
            if (have_cfile) {
                ERR_LOG("Duplicate client filename for patch on line"
                    "%hu", n->line);
                rv = -3;
                goto err;
            }

            if (handle_clientfile(n, file)) {
                rv = -4;
                goto err;
            }

            have_cfile = 1;
        }
        else if (!xmlStrcmp(n->name, XC"file")) {
            if ((file->flags & PATCH_FLAG_HAS_IF)) {
                ERR_LOG("Invalid file tag on line %hu", n->line);
                rv = -18;
                goto err;
            }

            file->flags |= PATCH_FLAG_NO_IF;

            if (have_file) {
                ERR_LOG("Duplicate server filename for patch on line"
                    "%hu", n->line);
                rv = -5;
                goto err;
            }

            if (handle_file(n, ent)) {
                rv = -6;
                goto err;
            }

            have_file = 1;
        }
        else if (!xmlStrcmp(n->name, XC"checksum")) {
            if ((file->flags & PATCH_FLAG_HAS_IF)) {
                ERR_LOG("Invalid checksum tag on line %hu", n->line);
                rv = -17;
                goto err;
            }

            file->flags |= PATCH_FLAG_NO_IF;

            if (have_checksum) {
                ERR_LOG("Duplicate checksum for patch on line %hu",
                    n->line);
                rv = -7;
                goto err;
            }

            if (handle_checksum(n, ent)) {
                rv = -8;
                goto err;
            }

            have_checksum = 1;
        }
        else if (!xmlStrcmp(n->name, XC"size")) {
            if ((file->flags & PATCH_FLAG_HAS_IF)) {
                ERR_LOG("Invalid size tag on line %hu", n->line);
                rv = -16;
                goto err;
            }

            file->flags |= PATCH_FLAG_NO_IF;

            if (have_size) {
                ERR_LOG("Duplicate size for patch on line %hu",
                    n->line);
                rv = -9;
                goto err;
            }

            if (handle_size(n, ent)) {
                rv = -10;
                goto err;
            }

            have_size = 1;
        }
        else if (!xmlStrcmp(n->name, XC"if")) {
            if ((file->flags & PATCH_FLAG_HAS_ELSE) ||
                (file->flags & PATCH_FLAG_NO_IF)) {
                ERR_LOG("Invalid if tag on line %hu", n->line);
                rv = -14;
                goto err;
            }

            /* Set the flag to say we've seen an if tag */
            file->flags |= PATCH_FLAG_HAS_IF;

            /* Set up the new entry */
            if (have_size) {
                ent2 = (patch_file_entry_t*)malloc(sizeof(patch_file_entry_t));
                if (!ent2) {
                    ERR_LOG("Cannot make space for entry on line %hu\n"
                        "%s", n->line, strerror(errno));
                    rv = -15;
                    goto err;
                }

                memset(ent2, 0, sizeof(patch_file_entry_t));
                ent->next = ent2;
                ent = ent2;
            }

            if (handle_entry(n, ent, 1)) {
                rv = -20;
                goto err;
            }

            /* Set these so we don't trip later... */
            have_size = have_checksum = have_file = 1;
        }
        else if (!xmlStrcmp(n->name, XC"else")) {
            if ((file->flags & PATCH_FLAG_HAS_ELSE) ||
                !(file->flags & PATCH_FLAG_HAS_IF)) {
                ERR_LOG("Invalid else tag on line %hu", n->line);
                rv = -12;
                goto err;
            }

            /* Set the required flags and set up the entry... */
            file->flags |= PATCH_FLAG_HAS_ELSE;

            ent2 = (patch_file_entry_t*)malloc(sizeof(patch_file_entry_t));
            if (!ent2) {
                ERR_LOG("Cannot make space for entry on line %hu\n"
                    "%s", n->line, strerror(errno));
                rv = -13;
                goto err;
            }

            memset(ent2, 0, sizeof(patch_file_entry_t));
            ent->next = ent2;
            ent = ent2;

            if (handle_entry(n, ent, 0)) {
                rv = -19;
                goto err;
            }
        }
        else {
            ERR_LOG("无效 %s 标记在行 %hu", (char*)n->name,
                n->line);
        }

        n = n->next;
    }

    /* If we got this far, make sure we got all the required attributes */
    if (!have_cfile || !have_file || !have_checksum || !have_size) {
        ERR_LOG("One or more required attributes not set for patch");
        rv = -11;
        goto err;
    }

    /* We've got everything, store it */
    TAILQ_INSERT_TAIL(q, file, qentry);
    rv = 0;

err:
    if (rv && file) {
        patch_free_entry(file);
    }

    xmlFree(enabled);

    return rv;
}

static int handle_patches(xmlNode* n, patch_config_t* cfg) {
    xmlChar* ver, * dir;
    int rv = 0;
    struct file_queue* q;

    /* Grab the attributes we're expecting */
    ver = xmlGetProp(n, XC"version");
    dir = xmlGetProp(n, XC"dir");

    /* Make sure we have all of them... */
    if (!ver || !dir) {
        ERR_LOG("One or more required patches attributes missing");
        rv = -1;
        goto err;
    }

    /* Make sure the version is sane */
    if (!xmlStrcmp(ver, XC"PC")) {
        q = &cfg->pc_files;
        cfg->pc_dir = (char*)dir;
    }
    else if (!xmlStrcmp(ver, XC"BB")) {
        q = &cfg->bb_files;
        cfg->bb_dir = (char*)dir;
    }
    else {
        ERR_LOG("Invalid version for patches tag: %s", (char*)ver);
        rv = -2;
        goto err;
    }

    /* Now that we're done with that, deal with any children of the node */
    n = n->children;
    while (n) {
        if (n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        else if (!xmlStrcmp(n->name, XC"patch")) {
            if (handle_patch(n, q)) {
                rv = -3;
                goto err;
            }
        }
        else {
            ERR_LOG("无效 %s 标记在行 %hu", (char*)n->name,
                n->line);
        }

        n = n->next;
    }

err:
    xmlFree(ver);

    if (rv < 0) {
        xmlFree(dir);
    }

    return rv;
}

int patch_read_config(const char* fn, patch_config_t** cfg) {
    xmlParserCtxtPtr cxt;
    xmlDoc* doc;
    xmlNode* n;
    int irv = 0;
    patch_config_t* rv;

    //printf("%s\n", fn);

    /* Allocate space for the base of the config. */
    rv = (patch_config_t*)malloc(sizeof(patch_config_t));

    if (!rv) {
        *cfg = NULL;
        ERR_LOG("无法为设置分配内存空间");
        perror("malloc");
        return -1;
    }

    /* Clear out the config. */
    memset(rv, 0, sizeof(patch_config_t));
    TAILQ_INIT(&rv->pc_files);
    TAILQ_INIT(&rv->bb_files);

    /* Create an XML Parsing context */
    cxt = xmlNewParserCtxt();
    if (!cxt) {
        ERR_LOG("无法为配置创建分析上下文");
        irv = -2;
        goto err;
    }

    /* Open the configuration file for reading. */

    if (fn) {
        doc = xmlReadFile(fn, NULL, XML_PARSE_DTDVALID);
    }
    else {
        doc = xmlReadFile(psocn_patch_cfg, NULL, XML_PARSE_DTDVALID);
    }

    if (!doc) {
        xmlParserError(cxt, "分析配置文件出错\n");
        irv = -3;
        goto err_cxt;
    }

    /* Make sure the document validated properly. */
    if (!cxt->valid) {
        xmlParserValidityError(cxt, "分析配置文件有效性错误\n");
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
    if (xmlStrcmp(n->name, XC"psocn_patch_config")) {
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
        else if (!xmlStrcmp(n->name, XC"versions")) {
            if (handle_versions(n, rv)) {
                irv = -7;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"welcome")) {
            if (handle_welcome(n, rv)) {
                irv = -8;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"patches")) {
            if (handle_patches(n, rv)) {
                irv = -9;
                goto err_doc;
            }
        }
        else if (!xmlStrcmp(n->name, XC"web_server")) {
            if (handle_web_server(n, rv)) {
                irv = -10;
                goto err_doc;
            }
        }
        else {
            ERR_LOG("无效 %s 标记在行 %hu", (char*)n->name,
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
        patch_free_config(rv);
        *cfg = NULL;
    }

    return irv;
}

void patch_free_config(patch_config_t* cfg) {
    patch_file_t* i, * tmp;

    /* Make sure we actually have a valid configuration pointer. */
    if (cfg) {
        /* Clean up any stored pointers */
        free_safe(cfg->pc_welcome);
        free_safe(cfg->bb_welcome);
        xmlFree(cfg->pc_dir);
        xmlFree(cfg->bb_dir);

        /* Clean up the file queues */
        i = TAILQ_FIRST(&cfg->pc_files);
        while (i) {
            tmp = TAILQ_NEXT(i, qentry);

            TAILQ_REMOVE(&cfg->pc_files, i, qentry);
            patch_free_entry(i);
            i = tmp;
        }

        i = TAILQ_FIRST(&cfg->bb_files);
        while (i) {
            tmp = TAILQ_NEXT(i, qentry);

            TAILQ_REMOVE(&cfg->bb_files, i, qentry);
            patch_free_entry(i);
            i = tmp;
        }

        /* Clean up the base structure. */
        free_safe(cfg);
    }
}
