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

#ifndef LOBBY_H
#define LOBBY_H

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <queue.h>

#include <quest.h>
#include <items.h>

#define PACKETS_H_HEADERS_ONLY
#include "packets.h"
#undef PACKETS_H_HEADERS_ONLY

#include "pso_player.h"
#include "mapdata.h"
#include "pso_struct_item.h"

#define LOBBY_MAX_CLIENTS   12
#define LOBBY_MAX_IN_TEAM   4

#define LOBBY_MAX_SAVED_ITEMS      3000

/* Forward declaration. */
struct ship_client;
struct block;

#ifndef SHIP_CLIENT_DEFINED
#define SHIP_CLIENT_DEFINED
typedef struct ship_client ship_client_t;
#endif

#ifndef BLOCK_DEFINED
#define BLOCK_DEFINED
typedef struct block block_t;
#endif

#ifndef SHIP_DEFINED
#define SHIP_DEFINED
typedef struct ship ship_t;
#endif

#ifndef QENEMY_DEFINED
#define QENEMY_DEFINED
typedef struct psocn_quest_enemy qenemy_t;
#endif

typedef struct lobby_pkt {
    STAILQ_ENTRY(lobby_pkt) qentry;

    ship_client_t *src;
    dc_pkt_hdr_t *pkt;
    bb_pkt_hdr_t *bb_pkt;
} lobby_pkt_t;

STAILQ_HEAD(lobby_pkt_queue, lobby_pkt);

/* 地面物品信息 */
typedef struct lobby_item {
    TAILQ_ENTRY(lobby_item) qentry;

    // lobby item info
    item_t item;
    float x;
    float z;
    uint8_t area;
    uint32_t amount;
} litem_t;

TAILQ_HEAD(lobby_item_queue, lobby_item);

typedef struct lobby_qfunc {
    SLIST_ENTRY(lobby_qfunc) entry;

    uint32_t func_id;
    int script_id;
    int nargs;
    int nretvals;
} lobby_qfunc_t;

typedef struct Map_Size {
    size_t map_nums;
    size_t map_vars;
} Map_Size_t;

SLIST_HEAD(lobby_qfunc_list, lobby_qfunc);

struct lobby {
    TAILQ_ENTRY(lobby) qentry;

    pthread_mutex_t mutex;

    uint32_t lobby_id;
    uint32_t type;
    uint32_t flags;

    int max_clients;
    int num_clients;
    int version;

    block_t *block;

    bool lobby_create;
    bool lobby_choice_version;
    bool lobby_choice_drop;

    uint8_t leader_id;
    uint8_t difficulty;
    uint8_t battle;
    uint8_t challenge;
    uint8_t oneperson;

    uint32_t govorlab;

    bool drop_pso2;
    bool drop_psocn;
    bool drops_disabled;
    bool questE0_done;

    uint32_t exp_mult;

    uint8_t v2;
    uint8_t section;
    uint8_t event;
    uint8_t episode; /* BB 1 2 3 */

    uint8_t max_chal;
    uint8_t legit_check_passed;
    uint8_t legit_check_done;
    uint8_t qlang;

    uint32_t min_level;
    uint32_t max_level;
    uint32_t rand_seed;
    uint32_t qid;

    char name[65];
    char passwd[65];
    uint32_t maps[0x20];
    //map_size_t lmaps[0x10]; 要改写的函数太多 暂时不启用 TODO

    uint32_t item_player_id[12]; //玩家背包物品id容器
    uint32_t bitem_player_id[12]; //玩家银行背包物品id容器
    uint32_t item_lobby_id; //大厅背包物品id容器
    uint32_t item_count; //大厅背包物品数量
    bool drop_rare;
    uint32_t monster_rare_drop_mult;/* 房间怪物掉率倍率 */
    uint32_t box_rare_drop_mult;/* 房间箱子掉率倍率 */
    /* 游戏房间物品链表 用于存储物品丢出 鉴定 */
    struct lobby_item_queue item_queue;

    ship_client_t *clients[LOBBY_MAX_CLIENTS]; // 角色数据所在位置 与 clients_slot 可以合并为加入房间的双重认证
    int clients_slot[LOBBY_MAX_CLIENTS]; // 记录角色所处格子 检测角色是否真实存在 角色在进入游戏后占用该格子

    struct lobby_pkt_queue pkt_queue;
    struct lobby_pkt_queue burst_queue;
    time_t create_time;

    game_enemies_t *map_enemies;
    game_objs_t *map_objs;
    uint32_t rare_enemy_count;
    uint16_t rare_enemy_data[0x10];
    uint32_t monster_rare_mult;
    bb_battle_param_t *bb_params;

    int num_mtypes;
    int num_mids;

    qenemy_t *mtypes;
    qenemy_t *mids;
    psocn_limits_t *limits_list;

    int (*dropfunc)(ship_client_t* src, struct lobby *l, void *req);
    int (*subcmd_handle)(ship_client_t* src, ship_client_t* dst, void* pkt);

    uint8_t q_flags;
    uint8_t q_shortflag_reg;
    uint8_t q_data_reg;
    uint8_t q_ctl_reg;

    int num_syncregs;
    uint16_t *syncregs; //大改动 同步注册ID
    uint32_t *regvals;

    uint8_t qpos_regs[LOBBY_MAX_IN_TEAM][LOBBY_MAX_IN_TEAM];

    uint8_t sdrops_ep;
    uint8_t sdrops_diff;
    uint8_t sdrops_section;

    int script_ref;
    int script_table;
    int *script_ids;
    FILE *logfp;

    struct lobby_qfunc_list qfunc_list;
};

#ifndef LOBBY_DEFINED
#define LOBBY_DEFINED
typedef struct lobby lobby_t;
#endif

TAILQ_HEAD(lobby_queue, lobby);

/* Possible values for the type parameter. */
#define LOBBY_TYPE_LOBBY              0x00000001
#define LOBBY_TYPE_GAME               0x00000002
#define LOBBY_TYPE_EP3_GAME           0x00000004
#define LOBBY_TYPE_PERSISTENT         0x00000008
// Flags used only for games
#define LOBBY_TYPE_CHEATS_ENABLED     0x00000100
//QUEST_IN_PROGRESS = 0x00000200,
//BATTLE_IN_PROGRESS = 0x00000400,
//JOINABLE_QUEST_IN_PROGRESS = 0x00000800,
//ITEM_TRACKING_ENABLED = 0x00001000,
//IS_SPECTATOR_TEAM = 0x00002000, // episode must be EP3 also
//SPECTATORS_FORBIDDEN = 0x00004000,
//START_BATTLE_PLAYER_IMMEDIATELY = 0x00008000,
//DROPS_ENABLED = 0x00010000, // Does not affect BB
//
//// Flags used only for lobbies
//PUBLIC = 0x01000000,
//DEFAULT = 0x02000000,
#define LOBBY_ITEM_TRACKING_ENABLED   0x00001000

#define LOBBY_EPISODE_NONE         0
#define LOBBY_EPISODE_1            1
#define LOBBY_EPISODE_2            2
#define LOBBY_EPISODE_3            3 //ep3 GAME
#define LOBBY_EPISODE_4            3

/* Possible values for the flags parameter. */
#define LOBBY_FLAG_BURSTING     0x00000001
#define LOBBY_FLAG_QUESTING     0x00000002
#define LOBBY_FLAG_QUESTSEL     0x00000004
#define LOBBY_FLAG_TEMP_UNAVAIL 0x00000008
#define LOBBY_FLAG_LEGIT_MODE   0x00000010
#define LOBBY_FLAG_ONLY_ONE     0x00000020
#define LOBBY_FLAG_DCONLY       0x00000040
#define LOBBY_FLAG_PCONLY       0x00000080
#define LOBBY_FLAG_V1ONLY       0x00000100
#define LOBBY_FLAG_GC_ALLOWED   0x00000200
#define LOBBY_FLAG_SINGLEPLAYER 0x00000400
#define LOBBY_FLAG_EP3          0x00000800
#define LOBBY_FLAG_SERVER_DROPS 0x00001000
#define LOBBY_FLAG_PSO2_DROPS   0x00001002
#define LOBBY_FLAG_PSOCN_DROPS  0x00001004
#define LOBBY_FLAG_DBG_SDROPS   0x00002000
#define LOBBY_FLAG_NTE          0x00004000
#define LOBBY_FLAG_HAS_NPC      0x00008000

/* 游戏房间错误参数定义 */
#define LOBBY_FLAG_ERROR_ADD_CLIENT               -1
#define LOBBY_FLAG_ERROR_REMOVE_CLIENT            -2
#define LOBBY_FLAG_ERROR_BURSTING                 -3
#define LOBBY_FLAG_ERROR_MIN_LEVEL                -4
#define LOBBY_FLAG_ERROR_MAX_LEVEL                -5
#define LOBBY_FLAG_ERROR_DCV2_ONLY                -6
#define LOBBY_FLAG_ERROR_QUESTING                 -7
#define LOBBY_FLAG_ERROR_QUESTSEL                 -8
#define LOBBY_FLAG_ERROR_LEGIT_MODE               -9
#define LOBBY_FLAG_ERROR_LEGIT_TEMP_UNAVAIL       -10
#define LOBBY_FLAG_ERROR_DC_ONLY                  -11
#define LOBBY_FLAG_ERROR_V1_ONLY                  -12
#define LOBBY_FLAG_ERROR_PC_ONLY                  -13
#define LOBBY_FLAG_ERROR_SINGLEPLAYER             -14
#define LOBBY_FLAG_ERROR_GAME_V1_CLASS            -15

/* Team log entry types. */
#define TLOG_BASIC              0x00000001
#define TLOG_DROPS              0x00000002
#define TLOG_DROPSV             0x00000004

/* Events that can be set on games */
#define GAME_EVENT_NONE         0
#define GAME_EVENT_CHRISTMAS    1
#define GAME_EVENT_21ST         2
#define GAME_EVENT_VALENTINES   3
#define GAME_EVENT_EASTER       4
#define GAME_EVENT_HALLOWEEN    5
#define GAME_EVENT_SONIC        6

/* Events that can be set on lobbies */
#define LOBBY_EVENT_NONE        0
#define LOBBY_EVENT_CHRISTMAS   1
#define LOBBY_EVENT_NONE2       2
/* 2 is just a normal (no event) lobby... */
#define LOBBY_EVENT_VALENTINES  3
#define LOBBY_EVENT_EASTER      4
#define LOBBY_EVENT_HALLOWEEN   5
#define LOBBY_EVENT_SONIC       6
#define LOBBY_EVENT_NEWYEARS    7
#define LOBBY_EVENT_SPRING      8
#define LOBBY_EVENT_WHITEDAY    9
#define LOBBY_EVENT_WEDDING     10
#define LOBBY_EVENT_AUTUMN      11
#define LOBBY_EVENT_FLAGS       12
#define LOBBY_EVENT_SPRINGFLAG  13
#define LOBBY_EVENT_ALT_NORMAL  14

/* 房间设置 */
#define LOBBY_SET_NONE          0
#define LOBBY_SET_DROP_RARE     1

#define LOBBY_QFLAG_SHORT       (1 << 0)
#define LOBBY_QFLAG_DATA        (1 << 1)
#define LOBBY_QFLAG_JOIN        (1 << 2)
#define LOBBY_QFLAG_SYNC_REGS   (1 << 3)

/* 用于打印大厅描述 */
static char lobby_des[512];

/* The required level for various difficulties. */
const static int game_required_level[4] = { 1, 20, 40, 80 };

static const uint32_t bb_game_required_level[3][4] = {
      {1, 20, 40, 80}, // episode 1
      {1, 30, 50, 90}, // episode 2
      {1, 40, 80, 110}  // episode 4
};

char* get_difficulty_describe(uint8_t difficulty);

lobby_t *lobby_create_default(block_t *block, uint32_t lobby_id, uint8_t ev);
lobby_t *lobby_create_game(block_t *block, char *name, char *passwd,
                           uint8_t difficulty, uint8_t battle, uint8_t chal,
                           uint8_t v2, int version, uint8_t section,
                           uint8_t event, uint8_t episode, ship_client_t *c,
                           uint8_t single_player, int bb);
lobby_t *lobby_create_ep3_game(block_t *block, char *name, char *passwd,
                               uint8_t view_battle, uint8_t section,
                               ship_client_t *c);
void lobby_destroy(lobby_t *l);
void lobby_destroy_noremove(lobby_t *l);

/* Add the client to any available lobby on the current block. */
int lobby_add_to_any(ship_client_t *c, lobby_t *req);

/* Send a packet to all people in a lobby. */
int lobby_send_pkt_dcnte(lobby_t *l, ship_client_t *c, void *h, void *h2,
                         int igcheck);
int lobby_send_pkt_dc(lobby_t *l, ship_client_t *c, void *hdr, int igcheck);
int lobby_send_pkt_bb(lobby_t *l, ship_client_t *c, void *hdr, int igcheck);

/* Send a packet to all Episode 3 clients in a lobby. */
int lobby_send_pkt_ep3(lobby_t *l, ship_client_t *c, void *h);

/* Move the client to the requested lobby, if possible. */
int lobby_change_lobby(ship_client_t *c, lobby_t *req);

/* Remove a player from a lobby without changing their lobby (for instance, if
   they disconnected). */
int lobby_remove_player(ship_client_t *c);
int lobby_remove_player_bb(ship_client_t* c);

/* Send an information reply packet with information about the lobby. */
int lobby_info_reply(ship_client_t *c, uint32_t lobby);

/* Check if a single player is legit enough for the lobby. */
int lobby_check_player_legit(lobby_t *l, ship_t *s, player_t *pl, uint32_t v);

/* Check if a single client is legit enough for the lobby. */
int lobby_check_client_legit(lobby_t *l, ship_t *s, ship_client_t *c);

/* Send out any queued packets when we get a done burst signal. */
int lobby_handle_done_burst(lobby_t *l, ship_client_t *c);
int lobby_resend_burst(lobby_t *l, ship_client_t *c);

/* Send out any queued packets when we get a done burst signal. */
int lobby_handle_done_burst_bb(lobby_t* l, ship_client_t* c);
int lobby_resend_burst_bb(lobby_t* l, ship_client_t* c);

/* Enqueue a packet for later sending (due to a player bursting) */
int lobby_enqueue_pkt(lobby_t *l, ship_client_t *c, dc_pkt_hdr_t *p);
int lobby_enqueue_burst(lobby_t *l, ship_client_t *c, dc_pkt_hdr_t *p);

/* Enqueue a packet for later sending (due to a player bursting) */
int lobby_enqueue_pkt_bb(lobby_t* l, ship_client_t* c, bb_pkt_hdr_t* p);
int lobby_enqueue_burst_bb(lobby_t* l, ship_client_t* c, bb_pkt_hdr_t* p);

/* Send the kill counts for all clients in the lobby that have kill tracking
   enabled. */
void lobby_send_kill_counts(lobby_t *l);

/* Set up lobby information for a quest being loaded. */
int lobby_setup_quest(lobby_t *l, ship_client_t *c, uint32_t qid, int lang);

int quick_set_quest(lobby_t* l, ship_client_t* c, uint32_t qid);

void lobby_print_info(lobby_t *l, FILE *fp);

void lobby_print_info2(ship_client_t* src);

const char* get_lobby_name(lobby_t* l);
char* get_lobby_describe(lobby_t* l);
char* get_lobby_describe_leader(lobby_t* l);

#ifdef ENABLE_LUA
#include <lua.h>
int lobby_register_lua(lua_State *l);
#endif

#endif /* !LOBBY_H */
