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
#include "utils.h"
#include "clients.h"
#include "lobby.h"
#include "quest_functions.h"

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

static pthread_mutex_t script_mutex = PTHREAD_MUTEX_INITIALIZER;
static lua_State *lstate;
static int scripts_ref = 0;

static int script_ids[ScriptActionCount] = { 0 };
static int script_ids_gate[ScriptActionCount] = { 0 };

/* Text versions of the script actions. This must match the list in the
   script_action_t enum in scripts.h. */
static const xmlChar *script_action_text[] = {
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

/* Figure out what index a given script action sits at */
static inline script_action_t script_action_to_index(xmlChar *str) {
    int i;

    for(i = 0; i < ScriptActionCount; ++i) {
        if(!xmlStrcmp(script_action_text[i], str)) {
            return (script_action_t)i;
        }
    }

    return ScriptActionInvalid;
}

int script_add(script_action_t action, const char *filename) {
    char realfn[64];
    int len;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    /* Make the real filename we'll try to load from... */
    len = snprintf(realfn, 64, "Scripts/%s", filename);
    if(len >= 64) {
        SCRIPT_LOG("尝试添加具有长文件名的脚本");
        return -1;
    }

    pthread_mutex_lock(&script_mutex);

    /* Pull the scripts table out to the top of the stack. */
    lua_rawgeti(lstate, LUA_REGISTRYINDEX, scripts_ref);

    /* Attempt to read in the script. */
    if(luaL_loadfile(lstate, realfn) != LUA_OK) {
        SCRIPT_LOG("无法读取脚本 \"%s\"", filename);
        lua_pop(lstate, 1);
        pthread_mutex_unlock(&script_mutex);
        return -1;
    }

    /* Issue a warning if we're redefining something before doing it. */
    if(script_ids_gate[action]) {
        SCRIPT_LOG("重新定义脚本事件 %d", (int)action);
        luaL_unref(lstate, -2, script_ids_gate[action]);
    }

    /* Add the script to the Lua table. */
    script_ids_gate[action] = luaL_ref(lstate, -2);
    SCRIPT_LOG("Script for type %d added as ID %d", (int)action,
          script_ids_gate[action]);

    /* Pop off the scripts table and unlock the mutex to clean up. */
    lua_pop(lstate, 1);
    pthread_mutex_unlock(&script_mutex);

    return 0;
}

int script_add_lobby_locked(lobby_t *l, script_action_t action) {
    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    /* Pull the scripts table out to the top of the stack. */
    lua_rawgeti(lstate, LUA_REGISTRYINDEX, l->script_table);

    /* Issue a warning if we're redefining something before doing it. */
    if(l->script_ids[action]) {
        SCRIPT_LOG("Redefining lobby event %d for lobby %" PRIu32 "",
              (int)action, l->lobby_id);
        luaL_unref(lstate, -1, l->script_ids[action]);
    }

    /* Pull the function out to the top of the stack. */
    lua_pushvalue(lstate, -2);

    /* Add the script to the Lua table. */
    l->script_ids[action] = luaL_ref(lstate, -2);
    SCRIPT_LOG("Lobby %" PRIu32 " callback for type %d added as Lua ID "
          "%d", l->lobby_id, (int)action, l->script_ids[action]);

    /* Pop off the scripts table and the function to clean up. */
    lua_pop(lstate, 2);

    return 0;
}

int script_add_lobby_qfunc_locked(lobby_t *l, uint32_t id, int args, int rvs) {
    lobby_qfunc_t *i;
    int found = 0;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    /* Pull the scripts table out to the top of the stack. */
    lua_rawgeti(lstate, LUA_REGISTRYINDEX, l->script_table);

    /* Check if the entry is already in the list and issue a warning that we're
       going to redefine it. */
    SLIST_FOREACH(i, &l->qfunc_list, entry) {
        if(i->func_id == id) {
            SCRIPT_LOG("Redefining lobby quest function %" PRIu32
                  " for lobby %" PRIu32 "", id, l->lobby_id);
            luaL_unref(lstate, -1, i->script_id);
            found = 1;
        }
    }

    if(!found) {
        if(!(i = (lobby_qfunc_t *)malloc(sizeof(lobby_qfunc_t)))) {
            SCRIPT_LOG("Cannot allocate memory for lobby quest function: "
                  "%s", strerror(errno));
            return -1;
        }
    }

    /* Pull the function out to the top of the stack. */
    lua_pushvalue(lstate, -2);

    /* Fill in the structure and add the script reference to the Lua table. */
    if (i) {
        i->func_id = id;
        i->script_id = luaL_ref(lstate, -2);
        i->nargs = args;
        i->nretvals = rvs;

        /* Add to the list if it wasn't already there. */
        if (!found) {
            SLIST_INSERT_HEAD(&l->qfunc_list, i, entry);
        }

        SCRIPT_LOG("Lobby %" PRIu32 " callback for quest function %" PRIu32
            " added as Lua ID %d", l->lobby_id, id, i->script_id);
    }


    /* Pop off the scripts table and the function to clean up. */
    lua_pop(lstate, 2);

    return 0;
}

int script_remove(script_action_t action) {
    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    pthread_mutex_lock(&script_mutex);

    /* Make sure there's actually something registered. */
    if(!script_ids_gate[action]) {
        SCRIPT_LOG("Attempt to unregister script for event %d that does "
              "not exist.", (int)action);
        pthread_mutex_unlock(&script_mutex);
        return -1;
    }

    /* Pull the scripts table out to the top of the stack and remove the
       script reference from it. */
    lua_rawgeti(lstate, LUA_REGISTRYINDEX, scripts_ref);
    luaL_unref(lstate, -2, script_ids_gate[action]);

    /* Pop off the scripts table, clear out the id stored in the script_ids_gate
       array, and unlock the mutex to finish up. */
    lua_pop(lstate, 1);
    script_ids_gate[action] = 0;
    pthread_mutex_unlock(&script_mutex);

    return 0;
}

int script_remove_lobby_locked(lobby_t *l, script_action_t action) {
    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    /* Make sure there's actually something registered. */
    if(!l->script_ids[action]) {
        SCRIPT_LOG("Attempt to unregister lobby %" PRIu32 " script for "
              "event %d that does not exist.", l->lobby_id, (int)action);
        return -1;
    }

    /* Pull the scripts table out to the top of the stack and remove the
       script reference from it. */
    lua_rawgeti(lstate, LUA_REGISTRYINDEX, l->script_table);
    luaL_unref(lstate, -2, l->script_ids[action]);

    /* Pop off the scripts table and clear out the id stored in the lobby's
       script_ids array to finish up. */
    lua_pop(lstate, 1);
    l->script_ids[action] = 0;

    return 0;
}

int script_remove_lobby_qfunc_locked(lobby_t *l, uint32_t id) {
    lobby_qfunc_t *i;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    /* Look for the requested function. Note that we do not need the _SAFE
       variant here even though we're removing items from the list because the
       loop does not iterate after removing from the list. */
    SLIST_FOREACH(i, &l->qfunc_list, entry) {
        if(i->func_id == id) {
            /* Pull the scripts table out to the top of the stack and remove the
               script reference from it, then pop the script table. */
            lua_rawgeti(lstate, LUA_REGISTRYINDEX, l->script_table);
            luaL_unref(lstate, -2, i->script_id);
            lua_pop(lstate, 1);

            /* Now remove it from the list and clean up. */
            SLIST_REMOVE(&l->qfunc_list, i, lobby_qfunc, entry);
            free_safe(i);

            return 0;
        }
    }

    /* If we get here, there wasn't actually anything registered on this
       lobby. */
    SCRIPT_LOG("Attempt to unregister lobby %" PRIu32 " script for "
          "quest function %" PRIu32 " that does not exist.", l->lobby_id, id);
    return -1;
}

int script_cleanup_lobby_locked(lobby_t *l) {
    lobby_qfunc_t *j, *tmp;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    /* Unreference the script table for the lobby/team. This will cause all the
       elements in the table to be marked for collection. */
    luaL_unref(lstate, LUA_REGISTRYINDEX, l->script_table);

    /* Clean up the linked list of quest functions, if any were allocated. */
    j = SLIST_FIRST(&l->qfunc_list);
    while(j) {
        tmp = SLIST_NEXT(j, entry);
        SLIST_REMOVE_HEAD(&l->qfunc_list, entry);
        free_safe(j);
        j = tmp;
    }

    return 0;
}

int script_update_module(const char *filename) {
    char *script;
    size_t size;
    char *modname, *tmp;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    /* Chop off the extension of the filename. */
    if(!(modname = _strdup(filename)))
        return -1;

    tmp = strrchr(modname, '.');
    if(tmp)
        *tmp = '\0';

    size = strlen(modname);

    if(!(script = (char *)malloc(size + 100)))
        return -1;

    snprintf(script, size + 100, "package.loaded['%s'] = nil", modname);

    /* Set the module search path to include the scripts/modules dir. */
    pthread_mutex_lock(&script_mutex);
    (void)luaL_dostring(lstate, script);
    pthread_mutex_unlock(&script_mutex);
    free_safe(script);
    free_safe(modname);

    return 0;
}

/* Parse the XML for the script definitions */
int script_eventlist_read(const char *fn) {
    xmlParserCtxtPtr cxt;
    xmlDoc *doc;
    xmlNode *n;
    xmlChar *file, *event;
    int rv = 0;
    script_action_t idx;

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
        SCRIPT_LOG("空脚本列表文档");
        rv = -4;
        goto err_doc;
    }

    /* Make sure the list looks sane. */
    if(xmlStrcmp(n->name, XC"scripts")) {
        SCRIPT_LOG("脚本列表似乎不是正确的类型!");
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
        else if(xmlStrcmp(n->name, XC"script")) {
            SCRIPT_LOG("Invalid Tag %s on line %hu", n->name, n->line);
        }
        else {
            /* We've got the right tag, see if we have all the attributes... */
            event = xmlGetProp(n, XC"event");
            file = xmlGetProp(n, XC"file");

            if(!event || !file) {
                SCRIPT_LOG("第 %hu 行脚本输入不完整",
                      n->line);
                goto next;
            }

            /* Figure out the entry we're looking at */
            idx = script_action_to_index(event);

            if(idx == ScriptActionInvalid) {
                SCRIPT_LOG("Ignoring unknown event (%s) on line %hu",
                      (char *)event, n->line);
                goto next;
            }

            /* Issue a warning if we're redefining something */
            if(script_ids[idx]) {
                SCRIPT_LOG("Redefining event \"%s\" on line %hu",
                      (char *)event, n->line);
            }

            /* Attempt to read in the script. */
            if(luaL_loadfile(lstate, (const char *)file) != LUA_OK) {
                SCRIPT_LOG("无法读取脚本 \"%s\" 设置行 %hu",
                      (char *)file, n->line);
                lua_pop(lstate, 1);
                goto next;
            }

            /* Add the script to the Lua table. */
            script_ids[idx] = luaL_ref(lstate, -2);
            SCRIPT_LOG("Script for type %s added as ID %d", event,
                  script_ids[idx]);

next:
            /* Free the memory we allocated here... */
            xmlFree(event);
            xmlFree(file);
        }

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

void init_scripts(ship_t *s) {
    long size = MAX_PATH;//pathconf(".", _PC_PATH_MAX);
    char *path_str, *script;

    if(!(path_str = (char *)malloc(size))) {
        SCRIPT_LOG("内存不足, 程序退出!");
        return;
    }
    else if(!_getcwd(path_str, size)) {
        SCRIPT_LOG("无法保存路径,本地包无法工作!");
    }

    /* Not that this should happen, but just in case... */
    if(lstate) {
        SCRIPT_LOG("尝试初始化脚本两次!");
        return;
    }

    /* Initialize the Lua interpreter */
    SCRIPT_LOG("初始化Lua脚本支持...");
    if(!(lstate = luaL_newstate())) {
        ERR_LOG("无法初始化Lua脚本支持!");
        return;
    }

    /* Load up the standard libraries. */
    luaL_openlibs(lstate);

    /* Register various scripting libraries. */
    luaL_requiref(lstate, "ship", ship_register_lua, 1);
    lua_pop(lstate, 1);
    luaL_requiref(lstate, "client", client_register_lua, 1);
    lua_pop(lstate, 1);
    luaL_requiref(lstate, "lobby", lobby_register_lua, 1);
    lua_pop(lstate, 1);

    if(path_str) {
        size = strlen(path_str) + 100;

        if(!(script = (char *)malloc(size)))
            SCRIPT_LOG("无法在脚本中保存路径!");

        if(script) {
            snprintf(script, size, "package.path = package.path .. "
                     "\";%s/scripts/modules/?.lua\"", path_str);

            /* Set the module search path to include the scripts/modules dir. */
            (void)luaL_dostring(lstate, script);
            free_safe(script);
        }

        free_safe(path_str);
    }

    /* Read in the configuration into our script table */
    if(script_eventlist_read(s->cfg->scripts_file)) {
        SCRIPT_LOG("无法读取脚本设置!");

        /* Make a scripts table, in case the gate sends us some later. */
        lua_newtable(lstate);
        scripts_ref = luaL_ref(lstate, LUA_REGISTRYINDEX);
    }
    else {
        SCRIPT_LOG("读取脚本设置");
    }

    s->lstate = lstate;
}

void cleanup_scripts(ship_t *s) {
    int i;

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
            script_ids_gate[i] = 0;
        }

        s->lstate = NULL;
    }
}

static lua_Integer exec_pkt(int scr, script_action_t event, ship_client_t *c,
                            const void *pkt, uint16_t len) {
    lua_Integer rv = 0;
    int err;
    const char *errmsg;

    /* There is an script defined, grab it from the table. */
    lua_rawgeti(lstate, -1, scr);

    /* Now, push the arguments onto the stack. First up is a light userdata
       for the client object. */
    lua_pushlightuserdata(lstate, c);

    /* Next is a string of the packet itself. */
    lua_pushlstring(lstate, (const char *)pkt, (size_t)len);

    /* Done with that, call the function. */
    if((err = lua_pcall(lstate, 2, 1, 0)) != LUA_OK) {
        ERR_LOG("为事件 %d (%d) 运行Lua脚本时出错",
              (int)event, err);

        if((errmsg = lua_tostring(lstate, -1))) {
            ERR_LOG("脚本错误信息: %s", errmsg);
        }

        lua_pop(lstate, 1);
        goto out;
    }

    /* Grab the return value from the lua function (it should be of type
       integer). */
    rv = lua_tointegerx(lstate, -1, &err);
    if(!err) {
        ERR_LOG("事件脚本 %d 未返回一个数值", (int)event);
    }

    /* Pop off the return value. */
    lua_pop(lstate, 1);

out:
    return rv;
}

int script_execute_pkt(script_action_t event, ship_client_t *c, const void *pkt,
                       uint16_t len) {
    lua_Integer grv = 0, lrv = 0;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    pthread_mutex_lock(&script_mutex);

    /* Pull the scripts table out to the top of the stack. */
    lua_rawgeti(lstate, LUA_REGISTRYINDEX, scripts_ref);

    /* See if there's a script event defined by the shipgate. */
    if(script_ids_gate[event])
        grv = exec_pkt(script_ids_gate[event], event, c, pkt, len);

    /* See if there's a script event defined locally */
    if(script_ids[event])
        lrv = exec_pkt(script_ids[event], event, c, pkt, len);

    /* Pop off the table reference that we pushed up above. */
    lua_pop(lstate, 1);
    pthread_mutex_unlock(&script_mutex);

    /* Return success if either script ran and returned success. */
    return (int)(grv | lrv);
}

static lua_Integer push_args_and_exec(int scr, script_action_t event,
                                      va_list ap) {
    lua_Integer rv = 0;
    int err = 0, argtype, argcount = 0;
    const char *errmsg;

    /* Push the script that we're looking at onto the stack. */
    lua_rawgeti(lstate, -1, scr);

    /* Now, push the arguments onto the stack. */
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
                SCRIPT_LOG("脚本参数类型无效: %d", argtype);
                lua_pop(lstate, argcount);
                rv = 0;
                goto out;
        }

        ++argcount;
    }

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
    return rv;
}

int script_execute(script_action_t event, ship_client_t *c, ...) {
    lua_Integer llrv = 0, lrv = 0, grv = 0;
    va_list ap;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return 0;

    pthread_mutex_lock(&script_mutex);

    /* Pull the scripts table out to the top of the stack. */
    lua_rawgeti(lstate, LUA_REGISTRYINDEX, scripts_ref);

    /* See if there's a script event defined by the gate */
    if(script_ids_gate[event]) {
        va_start(ap, c);
        grv = push_args_and_exec(script_ids_gate[event], event, ap);
        va_end(ap);
    }

    /* See if there's a script event defined locally */
    if(script_ids[event]) {
        va_start(ap, c);
        lrv = push_args_and_exec(script_ids[event], event, ap);
        va_end(ap);
    }

    /* See if there is a team-defined event. */
    if(c && c->cur_lobby && c->cur_lobby->script_ids) {
        if(c->cur_lobby->script_ids[event]) {
            va_start(ap, c);
            llrv = push_args_and_exec(c->cur_lobby->script_ids[event], event,
                                      ap);
            va_end(ap);
        }
    }

    /* Pop off the table reference that we pushed up above. */
    lua_pop(lstate, 1);
    pthread_mutex_unlock(&script_mutex);
    return (int)(llrv | lrv | grv);
}

uint32_t script_execute_qfunc(ship_client_t *c, lobby_t *l) {
    lobby_qfunc_t *i;
    int j, err;
    lua_Integer rv;
    const char *errmsg;

    /* Can't do anything if we don't have any scripts loaded. */
    if(!scripts_ref)
        return QUEST_FUNC_RET_INVALID_FUNC;

    /* Look for the requested function. */
    SLIST_FOREACH(i, &l->qfunc_list, entry) {
        if(i->func_id == c->q_stack[0]) {
            /* Check that the argument count and return value count match */
            if(c->q_stack[1] != i->nargs)
                return QUEST_FUNC_RET_BAD_ARG_COUNT;

            if(c->q_stack[2] != i->nretvals)
                return QUEST_FUNC_RET_BAD_RET_COUNT;

            /* Check all return value registers for validity */
            for(j = 3 + i->nargs; j < 3 + i->nargs + i->nretvals; ++j) {
                if(c->q_stack[j] > 255)
                    return QUEST_FUNC_RET_INVALID_REGISTER;
            }

            /* We're gonna do a script if we get here, so... lock the mutex */
            pthread_mutex_lock(&script_mutex);

            /* Pull the scripts table out to the top of the stack. */
            lua_rawgeti(lstate, LUA_REGISTRYINDEX, l->script_table);

            /* Push the script that we're looking at onto the stack. */
            lua_rawgeti(lstate, -1, i->script_id);

            /* Push the client and lobby structures */
            lua_pushlightuserdata(lstate, c);
            lua_pushlightuserdata(lstate, l);

            /* Build a table for the arguments */
            lua_createtable(lstate, i->nargs, 0);

            for(j = 0; j < i->nargs; ++j) {
                lua_pushinteger(lstate, j + 1);
                lua_pushinteger(lstate, c->q_stack[j + 3]);
                lua_settable(lstate, -3);
            }

            /* Do the same for the returns */
            lua_createtable(lstate, i->nretvals, 0);

            for(j = 0; j < i->nretvals; ++j) {
                lua_pushinteger(lstate, j + 1);
                lua_pushinteger(lstate, c->q_stack[j + i->nargs + 3]);
                lua_settable(lstate, -3);
            }

            /* Done with that, call the function. */
            if((err = lua_pcall(lstate, 4, 1, 0)) != LUA_OK) {
                ERR_LOG("Error running Lua script for qfunc %" PRIu32
                      " (%d)", i->func_id, err);

                if((errmsg = lua_tostring(lstate, -1))) {
                    ERR_LOG("脚本错误信息:\n%s", errmsg);
                }

                lua_pop(lstate, 1);
                rv = QUEST_FUNC_RET_SCRIPT_ERROR;
            }
            else {
                /* Grab the return value from the lua function (it should be of
                   type integer). */
                rv = lua_tointegerx(lstate, -1, &err);
                if(!err) {
                    ERR_LOG("Script for qfunc %" PRIu32 " didn't "
                          "return an integer!", i->func_id);
                    rv = QUEST_FUNC_RET_SCRIPT_ERROR;
                }

                /* Pop off the return value. */
                lua_pop(lstate, 1);
            }

            /* Pop off the table reference that we pushed up above. */
            lua_pop(lstate, 1);
            pthread_mutex_unlock(&script_mutex);

            return (uint32_t)rv;
        }
    }

    /* If we get here, the function doesn't exist. */
    return QUEST_FUNC_RET_INVALID_FUNC;
}

int script_execute_file(const char *fn, lobby_t *l) {
    lua_Integer rv;
    int err;

    /* Can't do anything if we can't run scripts. */
    if(!scripts_ref)
        return -1;

    pthread_mutex_lock(&script_mutex);

    /* Attempt to read in the script. */
    if(luaL_loadfile(lstate, (const char *)fn) != LUA_OK) {
        SCRIPT_LOG("无法读取脚本 '%s'", fn);
        lua_pop(lstate, 1);
        return -1;
    }

    /* Push the lobby structure for the team to the stack. */
    lua_pushlightuserdata(lstate, l);

    /* Run the script. */
    if(lua_pcall(lstate, 1, 1, 0) != LUA_OK) {
        ERR_LOG("运行LUA脚本错误 '%s'", fn);
        lua_pop(lstate, 1);
        return -1;
    }

    /* Grab the return value from the lua function (it should be of type
       integer). */
    rv = lua_tointegerx(lstate, -1, &err);
    if(!err) {
        ERR_LOG("脚本 '%s' 未返回一个数值", fn);
    }

    /* Pop off the return value. */
    lua_pop(lstate, 1);

    return (int)rv;
}

#else

void init_scripts(ship_t *s) {
    (void)s;
}

void cleanup_scripts(ship_t *s) {
    (void)s;
}

int script_execute_pkt(script_action_t event, ship_client_t *c, const void *pkt,
                       uint16_t len) {
    (void)event;
    (void)c;
    (void)pkt;
    (void)len;
    return 0;
}

int script_execute(script_action_t event, ship_client_t *c, ...) {
    (void)event;
    (void)c;
    return 0;
}

uint32_t script_execute_qfunc(ship_client_t *c, lobby_t *l) {
    (void)c;
    (void)l;
    return QUEST_FUNC_RET_INVALID_FUNC;
}

int script_add(script_action_t event, const char *filename) {
    (void)event;
    (void)filename;
    return 0;
}

int script_remove(script_action_t event) {
    (void)event;
    return 0;
}

int script_update_module(const char *modname) {
    (void)modname;
    return 0;
}

int script_add_lobby_locked(lobby_t *l, script_action_t action) {
    (void)l;
    (void)action;
    return 0;
}

int script_add_lobby_qfunc_locked(lobby_t *l, uint32_t id, int args, int rvs) {
    (void)l;
    (void)id;
    (void)args;
    (void)rvs;
    return 0;
}

int script_remove_lobby_locked(lobby_t *l, script_action_t action) {
    (void)l;
    (void)action;
    return 0;
}

int script_remove_lobby_qfunc_locked(lobby_t *l, uint32_t id) {
    (void)l;
    (void)id;
    return 0;
}

int script_cleanup_lobby_locked(lobby_t *l) {
    (void)l;
    return 0;
}

int script_execute_file(const char *fn, lobby_t *l) {
    (void)fn;
    (void)l;
    return 0;
}

#endif /* ENABLE_LUA */
