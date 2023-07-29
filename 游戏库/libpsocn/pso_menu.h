/*
    梦幻之星中国 菜单功能
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


#ifndef PSO_MENU_HAVE_MENU
#define PSO_MENU_HAVE_MENU

#include <stdio.h>
#include <stdint.h>

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct pso_menu {
    char* name;
    uint32_t menu_id;
    uint32_t item_id;
    uint16_t flag;
} PACKED pso_menu_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

#define MENU_ID_INITIAL         0x00000000
#define MENU_ID_BLOCK           0x00000001
#define MENU_ID_GAME            0x00000002
#define MENU_ID_PATCH           0x00000002
#define MENU_ID_QCATEGORY       0x00000003
#define MENU_ID_QUEST           0x00000004
#define MENU_ID_SHIP            0x00000005
#define MENU_ID_GAME_TYPE       0x00000006
#define MENU_ID_GM              0x00000007
#define MENU_ID_GAME_DROP       0x00000008
#define MENU_ID_NO_SHIP         0x000000EF
#define MENU_ID_INFODESK        0x000000FF
#define MENU_ID_DATABASE        0x00040000
#define MENU_ID_LAST_SHIP       0xDEADBEEF
#define MENU_ID_LOBBY           0xFFFFFFFF

/* Submenus of the GM menu. */
#define MENU_ID_GM_SHUTDOWN     0x00000407
#define MENU_ID_GM_RESTART      0x00000507
#define MENU_ID_GM_GAME_EVENT   0x00000607
#define MENU_ID_GM_LOBBY_EVENT  0x00000707
#define MENU_ID_GM_LOBBY_SET    0x00000807

#define ITEM_ID_EMPTY           0x00000000
#define ITEM_ID_INIT_SHIP       0x00000000
#define ITEM_ID_INIT_DOWNLOAD   0x00000001
#define ITEM_ID_INIT_INFO       0x00000002
#define ITEM_ID_INIT_GM         0x00000003
#define ITEM_ID_INIT_PATCH      0x00000004

#define ITEM_ID_DISCONNECT      0xFFFFFFFE
#define ITEM_ID_LAST            0xFFFFFFFF

/* GM Options Item IDs */
#define ITEM_ID_GM_REF_QUESTS   0x00000001
#define ITEM_ID_GM_REF_GMS      0x00000002
#define ITEM_ID_GM_REF_LIMITS   0x00000003
#define ITEM_ID_GM_SHUTDOWN     0x00000004
#define ITEM_ID_GM_RESTART      0x00000005
#define ITEM_ID_GM_GAME_EVENT   0x00000006
#define ITEM_ID_GM_LOBBY_EVENT  0x00000007
#define ITEM_ID_GM_LOBBY_SET    0x00000008

static pso_menu_t pso_initial_menu_auth_dc[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_INITIAL, 0x0004},
    {"SELECT SHIP", MENU_ID_INITIAL, ITEM_ID_INIT_SHIP, 0x0004},
    {"DOWNLOAD QUESTS", MENU_ID_INITIAL, ITEM_ID_INIT_DOWNLOAD, 0x0F04},
    {"PATCH", MENU_ID_INITIAL, ITEM_ID_INIT_PATCH, 0x0004},
    {"GM TOOLS", MENU_ID_INITIAL, ITEM_ID_INIT_GM, 0x0004},
    {"DISCONNECT", MENU_ID_INITIAL, ITEM_ID_DISCONNECT, 0x0004}
};

static pso_menu_t pso_initial_menu_auth_gc[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_INITIAL, 0x0004},
    {"SELECT SHIP", MENU_ID_INITIAL, ITEM_ID_INIT_SHIP, 0x0004},
    {"DOWNLOAD QUESTS", MENU_ID_INITIAL, ITEM_ID_INIT_DOWNLOAD, 0x0F04},
    {"SHIP INFO", MENU_ID_INITIAL, ITEM_ID_INIT_INFO, 0x0004},
    {"PATCH", MENU_ID_INITIAL, ITEM_ID_INIT_PATCH, 0x0004},
    {"GM TOOLS", MENU_ID_INITIAL, ITEM_ID_INIT_GM, 0x0004},
    {"DISCONNECT", MENU_ID_INITIAL, ITEM_ID_DISCONNECT, 0x0004}
};

static pso_menu_t pso_initial_menu_auth_pc[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_INITIAL, 0x0004},
    {"选择舰船", MENU_ID_INITIAL, ITEM_ID_INIT_SHIP, 0x0004},
    {"下载任务", MENU_ID_INITIAL, ITEM_ID_INIT_DOWNLOAD, 0x0F04},
    {"GM 工作台", MENU_ID_INITIAL, ITEM_ID_INIT_GM, 0x0004},
    {"断开连接", MENU_ID_INITIAL, ITEM_ID_DISCONNECT, 0x0004}
};

static pso_menu_t pso_initial_menu_auth_xbox[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_INITIAL, 0x0004},
    {"SELECT SHIP", MENU_ID_INITIAL, ITEM_ID_INIT_SHIP, 0x0004},
    {"DOWNLOAD QUESTS", MENU_ID_INITIAL, ITEM_ID_INIT_DOWNLOAD, 0x0F04},
    {"SHIP INFO", MENU_ID_INITIAL, ITEM_ID_INIT_INFO, 0x0004},
    {"GM TOOLS", MENU_ID_INITIAL, ITEM_ID_INIT_GM, 0x0004},
    {"DISCONNECT", MENU_ID_INITIAL, ITEM_ID_DISCONNECT, 0x0004}
};

static pso_menu_t pso_initial_menu_auth_bb[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_INITIAL, 0x0004},
    {"选择舰船", MENU_ID_INITIAL, ITEM_ID_INIT_SHIP, 0x0004},
    {"舰船信息", MENU_ID_INITIAL, ITEM_ID_INIT_INFO, 0x0004},
    {"GM 工作台", MENU_ID_INITIAL, ITEM_ID_INIT_GM, 0x0004},
    {"断开连接", MENU_ID_INITIAL, ITEM_ID_DISCONNECT, 0x0004}
};

static pso_menu_t pso_game_create_menu_pc[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_GAME_TYPE, 0x0000},
    {"允许 PSOv1", MENU_ID_GAME_TYPE, 0, 0x0000},
    {"PSOv2 独享", MENU_ID_GAME_TYPE, 1, 0x0000},
    {"PSOPC 独享", MENU_ID_GAME_TYPE, 2, 0x0000},
    //{"返回上一级", MENU_ID_GAME_TYPE, 0xFF, 0x0000},
};

static pso_menu_t pso_game_create_menu_bb[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_GAME_TYPE, 0x0000},
    {"PSOBB 独享", MENU_ID_GAME_TYPE, 0, 0x0000},
    {"允许所有版本", MENU_ID_GAME_TYPE, 1, 0x0000},
    {"允许 PSOv1", MENU_ID_GAME_TYPE, 2, 0x0000},
    {"允许 PSOv2", MENU_ID_GAME_TYPE, 3, 0x0000},
    {"允许 PSODC", MENU_ID_GAME_TYPE, 4, 0x0000},
    {"允许 PSOGC", MENU_ID_GAME_TYPE, 5, 0x0000},
    //{"返回上一级", MENU_ID_GAME_TYPE, 0xFF, 0x0000},
};

static pso_menu_t pso_game_drop_menu_bb[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_GAME_DROP, 0x0000},
    {"默认掉落模式", MENU_ID_GAME_DROP, 0, 0x0000},
    {"PSO2掉落模式", MENU_ID_GAME_DROP, 1, 0x0000},
    {"随机掉落模式", MENU_ID_GAME_DROP, 2, 0x0000},
    //{"返回上一级", MENU_ID_GAME_DROP, 0xFF, 0x0000},
};

static pso_menu_t pso_block_list_menu_last[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_BLOCK, 0x0000},
    {"返回选择舰船", MENU_ID_BLOCK, ITEM_ID_LAST, 0x0000},
    {"断开连接", MENU_ID_BLOCK, ITEM_ID_DISCONNECT, 0x0000},
};


static pso_menu_t pso_guild_rank_list_bb[][4] = {
    {"DATABASE/US", MENU_ID_DATABASE, MENU_ID_INITIAL, 0x0004},
    {"暂未完成", MENU_ID_INITIAL, ITEM_ID_INIT_SHIP, 0x0004},
    {"暂未完成", MENU_ID_INITIAL, ITEM_ID_INIT_INFO, 0x0004},
    {"暂未完成", MENU_ID_INITIAL, ITEM_ID_INIT_GM, 0x0004},
    {"暂未完成", MENU_ID_INITIAL, ITEM_ID_DISCONNECT, 0x0004}
};

#endif /* !PSO_MENU_HAVE_MENU */