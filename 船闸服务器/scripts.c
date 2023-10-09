/*
    梦幻之星中国 船闸服务器
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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <direct.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <errno.h>

#include <queue.h>

#include <debug.h>
#include <f_logs.h>
#include <f_checksum.h>
#include <psoconfig.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "scripts.h"

#ifdef ENABLE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

#ifndef LIBXML_TREE_ENABLED
#error You must have libxml2 with tree support built-in.
#endif

#define XC (const xmlChar *)

#ifdef ENABLE_LUA

extern psocn_config_t *cfg;
extern psocn_srvconfig_t* srvcfg;

static lua_State *lstate;
static int scripts_ref = 0;
static int script_ids[ScriptActionCount] = { 0 };

uint32_t script_count;
ship_script_t *scripts;

/* This should be kept in sync with the same list in ship_server... */
static const xmlChar *ship_script_action_text[] = {
    XC"STARTUP",
    XC"SHUTDOWN",
    XC"SHIP_LOGIN",
    XC"SHIP_LOGOUT",
    XC"BLOCK_LOGIN",
    XC"BLOCK_LOGOUT",
    XC"UNK_SHIP_PKT",
    XC"UNK_BLOCK_PKT",
    XC"UNK_EP3_PKT",
    XC"TEAM_CREATE",
    XC"TEAM_DESTROY",
    XC"TEAM_JOIN",
    XC"TEAM_LEAVE",
    XC"ENEMY_KILL",
    XC"ENEMY_HIT",
    XC"BOX_BREAK",
    XC"UNK_COMMAND",
    XC"SDATA",
    XC"UNK_MENU",
    XC"BANK_ACTION",
    XC"CHANGE_AREA",
    XC"QUEST_SYNCREG",
    XC"QUEST_LOAD",
    XC"BEFORE_QUEST_LOAD",
    NULL
};

/* Text versions of the script actions. This must match the list in the
   script_action_t enum in scripts.h . */
static const xmlChar *script_action_text[] = {
    XC"STARTUP",
    XC"SHUTDOWN",
    XC"SDATA",
};

/* Figure out what index a given script action sits at */
static inline int ship_script_action_to_index(xmlChar *str) {
    int i;

    for(i = 0; ship_script_action_text[i]; ++i) {
        if(!xmlStrcmp(ship_script_action_text[i], str)) {
            return i;
        }
    }

    return -1;
}

static inline script_action_t script_action_to_index(xmlChar *str) {
    int i;

    for(i = 0; i < ScriptActionCount; ++i) {
        if(!xmlStrcmp(script_action_text[i], str)) {
            return (script_action_t)i;
        }
    }

    return ScriptActionInvalid;
}

static int ship_script_add(xmlChar *file, xmlChar *remote, int mod,
                           int action, uint32_t *alloc, int deleted) {
    void *tmp;
    FILE *fp;
    long len = 0;
    uint32_t crc = 0;
    char sfile[48] = { 0 };

    /* Do we have space for this new script? */
    if(!*alloc) {
        /* This will probably be enough... At least for now. */
        scripts = (ship_script_t *)malloc(sizeof(ship_script_t) * 10);
        if(!scripts) {
            ERR_LOG("分配脚本数组内存错误");
            return -1;
        }

        *alloc = 10;
    }
    else if(*alloc == script_count) {
        tmp = realloc(scripts, sizeof(ship_script_t) * *alloc * 2);
        if(!tmp) {
            ERR_LOG("重分配脚本数组内存错误");
            return -1;
        }

        scripts = (ship_script_t *)tmp;
        *alloc *= 2;
    }

    strcpy(sfile, "Scripts/");
    SAFE_STRCAT(sfile, file);

    /* Is the script deleted? */
    if(!deleted) {
        if(!(fp = fopen(sfile, "rb"))) {
            SGATE_LOG("无法打开脚本文件 '%s'", file);
            return -2;
        }

        /* Figure out how long it is */
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if(len > 32768) {
            ERR_LOG("脚本文件 '%s' 写那么长干嘛,不会换个文件吗!", file);
            fclose(fp);
            return -3;
        }

        if(!(tmp = malloc(len))) {
            ERR_LOG("分配脚本内存错误");
            fclose(fp);
            return -4;
        }

        if(fread(tmp, 1, len, fp) != (size_t)len) {
            ERR_LOG("无法读取脚本 '%s'", file);
            free(tmp);
            fclose(fp);
            return -4;
        }

        fclose(fp);

        crc = psocn_crc32((uint8_t *)tmp, len);
        free(tmp);
    }

    scripts[script_count].local_fn = (char *)file;
    scripts[script_count].remote_fn = (char *)remote;
    scripts[script_count].module = mod;
    scripts[script_count].event = action;
    scripts[script_count].len = (uint32_t)len;
    scripts[script_count].crc = crc;
    scripts[script_count].deleted = deleted;
    ++script_count;

    return 0;
}

/* Parse the XML for the script definitions */
int script_list_read(const char *fn) {
    xmlParserCtxtPtr cxt;
    xmlDoc *doc;
    xmlNode *n;
    xmlChar *dir, *file, *event, *remote, *deleted;
    char luafile[32] = { 0 };
    int rv = 0;
    script_action_t idx;
    int sidx;
    uint32_t num_alloc = 0;
    int is_del = 0;

    /* If we're reloading, kill the old list. */
    if(scripts_ref) {
        luaL_unref(lstate, LUA_REGISTRYINDEX, scripts_ref);
    }

    /* Create an XML Parsing context */
    cxt = xmlNewParserCtxt();
    if(!cxt) {
        ERR_LOG("无法为Lua脚本创建XML分析上下文");
        rv = -1;
        goto err;
    }

    /* Open the script list XML file for reading. */
    doc = xmlReadFile(fn, NULL, 0);
    if(!doc) {
        xmlParserError(cxt, "分析脚本列表时出错\n");
        rv = -2;
        goto err_cxt;
    }

    /* Make sure the document validated properly. */
    if(!cxt->valid) {
        xmlParserValidityError(cxt, "分析脚本列表有效性错误\n");
        rv = -3;
        goto err_doc;
    }

    /* If we've gotten this far, we have a valid document, now go through and
       add in entries for everything... */
    n = xmlDocGetRootElement(doc);

    if(!n) {
        ERR_LOG("空脚本列表文档");
        rv = -4;
        goto err_doc;
    }

    /* Make sure the list looks sane. */
    if(xmlStrcmp(n->name, XC"scripts")) {
        ERR_LOG("脚本列表似乎不是正确的类型");
        rv = -5;
        goto err_doc;
    }

    /* Create a table for storing our pre-parsed scripts in... */
    lua_newtable(lstate);

    n = n->children;
    while(n) {
        if(n->type != XML_ELEMENT_NODE) {
            /* Ignore non-elements. */
            n = n->next;
            continue;
        }
        if(!xmlStrcmp(n->name, XC"script")) {
            /* See if we have all <script> elements... */
            event = xmlGetProp(n, XC"event");
            dir = xmlGetProp(n, XC"dir");
            file = xmlGetProp(n, XC"file");

            if(!event || !file || !dir) {
                ERR_LOG("第 %hu 行的脚本设置不完整",
                      n->line);
                goto next;
            }

            /* Figure out the entry we're looking at */
            idx = script_action_to_index(event);

            if(idx == ScriptActionInvalid) {
                ERR_LOG("忽略未知脚本事件 (%s) 行 %hu",
                      (char *)event, n->line);
                goto next;
            }

            /* Issue a warning if we're redefining something */
            if(script_ids[idx]) {
                ERR_LOG("重定向脚本事件 \"%s\" 设置行 %hu",
                      (char *)event, n->line);
            }

            strcpy(luafile, dir);
            SAFE_STRCAT(luafile, file);

            /* Attempt to read in the script. */
            if(luaL_loadfile(lstate, luafile) != LUA_OK) {
                ERR_LOG("无法读取 \"%s\" 脚本 设置行 %hu",
                      (char *)file, n->line);
                goto next;
            }

            /* Add the script to the Lua table. */
            script_ids[idx] = luaL_ref(lstate, -2);
            SGATE_LOG("新增脚本类型 %s ID %d 文件: %s", event,
                  script_ids[idx], file);

        next:
            /* Free the memory we allocated here... */
            xmlFree(event);
            xmlFree(file);
            xmlFree(dir);
        }
        else if(!xmlStrcmp(n->name, XC"module")) {
            /* See if we have all <module> elements... */
            dir = xmlGetProp(n, XC"dir");
            file = xmlGetProp(n, XC"file");
            remote = xmlGetProp(n, XC"remote_file");
            deleted = xmlGetProp(n, XC"deleted");

            if(deleted) {
                if(!xmlStrcmp(deleted, XC"true")) {
                    is_del = 1;
                }
                else if(xmlStrcmp(deleted, XC"false")) {
                    ERR_LOG("忽略未知 deleted 值 (%s) "
                          "设置行 %hu, 默认 false", (char *)deleted,
                          n->line);
                }
            }

            /* We don't need this anymore... */
            xmlFree(deleted);

            if((!is_del && !file) || !remote) {
                ERR_LOG("脚本模组实例不完整 设置行 %hu",
                      n->line);
                xmlFree(remote);
                xmlFree(file);
                xmlFree(dir);
                goto next_ent;
            }

            strcpy(luafile, dir);
            SAFE_STRCAT(luafile, file);

            /* Add it to the list. */
            if(ship_script_add(file, remote, 1, 0, &num_alloc, is_del)) {
                xmlFree(remote);
                xmlFree(file);
                xmlFree(dir);
            }
            else {
                if(!is_del) {
                    SGATE_LOG("新增脚本模组 '%s'",
                          scripts[script_count - 1].local_fn);
                }
                else {
                    SGATE_LOG("新增删除的模组 '%s'",
                          scripts[script_count - 1].remote_fn);
                }
            }
        }
        else if(!xmlStrcmp(n->name, XC"ship")) {
            /* See if we have all <module> elements... */
            dir = xmlGetProp(n, XC"dir");
            file = xmlGetProp(n, XC"file");
            remote = xmlGetProp(n, XC"remote_file");
            event = xmlGetProp(n, XC"event");
            deleted = xmlGetProp(n, XC"deleted");

            if(deleted) {
                if(!xmlStrcmp(deleted, XC"true")) {
                    is_del = 1;
                }
                else if(xmlStrcmp(deleted, XC"false")) {
                    ERR_LOG("忽略未知 deleted 值 (%s) "
                          "设置行 %hu, 默认 false", (char *)deleted,
                          n->line);
                }
            }

            /* We don't need this anymore... */
            xmlFree(deleted);

            if((!is_del && !file) || !remote) {
                ERR_LOG("舰船脚本实例不完整 设置行 %hu",
                      n->line);
                xmlFree(event);
                xmlFree(remote);
                xmlFree(file);
                xmlFree(dir);
                goto next_ent;
            }

            /* If an event was provided, mark it */
            if(event) {
                /* Figure out the entry we're looking at */
                sidx = ship_script_action_to_index(event);
                if(sidx == -1) {
                    ERR_LOG("忽略未知脚本事件 (%s) 设置行 %hu",
                          (char *)event, n->line);
                    xmlFree(event);
                    xmlFree(remote);
                    xmlFree(file);
                    xmlFree(dir);
                    goto next_ent;
                }
            }
            else {
                sidx = -1;
            }

            /* We're done with this now... */
            xmlFree(event);

            strcpy(luafile, dir);
            SAFE_STRCAT(luafile, file);

            /* Add it to the list. */
            if(ship_script_add(file, remote, 0, sidx, &num_alloc, is_del)) {
                xmlFree(remote);
                xmlFree(file);
                xmlFree(dir);
            }
            else {
                if(!is_del) {
                    SGATE_LOG("添加了舰船脚本: '%s'",
                          scripts[script_count - 1].local_fn);
                }
                else {
                    SGATE_LOG("新增删除舰船脚本: '%s'",
                          scripts[script_count - 1].remote_fn);
                }
            }
        }

    next_ent:
        n = n->next;
    }

    /* Store the table of scripts to the registry for later use. */
    scripts_ref = luaL_ref(lstate, LUA_REGISTRYINDEX);

    /* Cleanup/error handling below... */
err_doc:
    xmlFreeDoc(doc);
err_cxt:
    xmlFreeParserCtxt(cxt);
err:

    return rv;
}

extern int ship_register_lua(lua_State *l);

void init_scripts(void) {
    /* Not that this should happen, but just in case... */
    if(lstate) {
        ERR_LOG("尝试初始化脚本两次!");
        return;
    }

    /* Initialize the Lua interpreter */
    SGATE_LOG("初始化Lua脚本支持...");
    if(!(lstate = luaL_newstate())) {
        ERR_LOG("无法初始化Lua脚本支持!");
        return;
    }

    /* Load up the standard libraries. */
    luaL_openlibs(lstate);

    luaL_requiref(lstate, "shipgate", ship_register_lua, 1);
    lua_pop(lstate, 1);

    /* Set the module search path to include the scripts/modules dir. */
    (void)luaL_dostring(lstate, "package.path = package.path .. "
                        "';scripts/modules/?.lua'");

    /* Read in the configuration into our script table */
    if(cfg->sg_scripts_file) {
        if(script_list_read(cfg->sg_scripts_file)) {
            ERR_LOG("无法加载Lua脚本配置!");
        }
        else {
            SGATE_LOG("读取脚本配置");
        }
    }
    else {
        ERR_LOG("未配置Lua脚本");
    }
}

void cleanup_scripts(void) {
    uint32_t i;

    if(lstate) {
        /* For good measure, remove the scripts table from the registry. This
           should garbage collect everything in it, I hope. */
        if(scripts_ref)
            luaL_unref(lstate, LUA_REGISTRYINDEX, scripts_ref);

        lua_close(lstate);

        /* Clean everything back to a sensible state. */
        lstate = NULL;
        scripts_ref = 0;
        for(i = 0; i < ScriptActionCount; ++i) {
            script_ids[i] = 0;
        }

        /* Free the ship scripts */
        for(i = 0; i < script_count; ++i) {
            xmlFree(scripts[i].local_fn);
            xmlFree(scripts[i].remote_fn);
        }

        free(scripts);
        scripts = NULL;
        script_count = 0;
    }
}

int script_execute(script_action_t event, ...) {
    lua_Integer rv = 0;
    int err = 0, argtype, argcount = 0;
    va_list ap;
    const char *errmsg;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    /* Pull the scripts table out to the top of the stack. */
    lua_rawgeti(lstate, LUA_REGISTRYINDEX, scripts_ref);

    /* See if there's a script event defined */
    if(!script_ids[event])
        goto out;

    /* There is an script defined, grab it from the table. */
    lua_rawgeti(lstate, -1, script_ids[event]);

    /* Now, push the arguments onto the stack. */
    va_start(ap, event);
    while((argtype = va_arg(ap, int))) {
        switch(argtype) {
            case SCRIPT_ARG_INT:
            {
                int arg = va_arg(ap, int);
                lua_Integer larg = (lua_Integer)arg;
                lua_pushinteger(lstate, larg);
                break;
            }

            case SCRIPT_ARG_UINT8:
            {
                uint8_t arg = (uint8_t)va_arg(ap, int);
                lua_Integer larg = (lua_Integer)arg;
                lua_pushinteger(lstate, larg);
                break;
            }

            case SCRIPT_ARG_UINT16:
            {
                uint16_t arg = (uint16_t)va_arg(ap, int);
                lua_Integer larg = (lua_Integer)arg;
                lua_pushinteger(lstate, larg);
                break;
            }

            case SCRIPT_ARG_UINT32:
            {
                uint32_t arg = va_arg(ap, uint32_t);
                lua_Integer larg = (lua_Integer)arg;
                lua_pushinteger(lstate, larg);
                break;
            }

            case SCRIPT_ARG_FLOAT:
            {
                double arg = va_arg(ap, double);
                lua_Number larg = (lua_Number)arg;
                lua_pushnumber(lstate, larg);
                break;
            }

            case SCRIPT_ARG_PTR:
            {
                void *arg = va_arg(ap, void *);
                lua_pushlightuserdata(lstate, arg);
                break;
            }

            case SCRIPT_ARG_STRING:
            {
                size_t len = va_arg(ap, size_t);
                char *str = va_arg(ap, char *);
                lua_pushlstring(lstate, str, len);
                break;
            }

            case SCRIPT_ARG_CSTRING:
            {
                char *str = va_arg(ap, char *);
                lua_pushstring(lstate, str);
                break;
            }

            default:
                /* Fix the stack and stop trying to parse now... */
                ERR_LOG("脚本参数类型无效: %d", argtype);
                lua_pop(lstate, argcount);
                rv = 0;
                goto out;
        }

        ++argcount;
    }
    va_end(ap);

    /* Done with that, call the function. */
    if((err = lua_pcall(lstate, argcount, 1, 0)) != LUA_OK) {
        ERR_LOG("为事件 %d 运行Lua脚本时出错 (%d)",
              (int)event, err);

        if((errmsg = lua_tostring(lstate, -1))) {
            ERR_LOG("Lua脚本错误信息:\n%s", errmsg);
        }

        lua_pop(lstate, 1);
        goto out;
    }

    /* Grab the return value from the lua function (it should be of type
       integer). */
    rv = lua_tointegerx(lstate, -1, &err);
    if(!err) {
        ERR_LOG("事件Lua脚本 %d 未返回一个int数值",(int)event);
    }

    /* Pop off the return value. */
    lua_pop(lstate, 1);

out:
    /* Pop off the table reference that we pushed up above. */
    lua_pop(lstate, 1);
    return (int)rv;
}

#else

void init_scripts(void) {
}

void cleanup_scripts(void) {
}

int script_execute(script_action_t event, ...) {
    return 0;
}

#endif
