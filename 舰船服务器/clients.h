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

#ifndef CLIENTS_H_COUNTS
#define CLIENTS_H_COUNTS

#define CLIENT_VERSION_ALL    7
#define CLIENT_LANG_ALL       8

#endif /* !CLIENTS_H_COUNTS */

#ifndef CLIENTS_H_COUNTS_ONLY
#ifndef CLIENTS_H
#define CLIENTS_H

#include <time.h>
#include <inttypes.h>
#include <pthread.h>
#include <queue.h>
#include <WinSock_Defines.h>

#include <psoconfig.h>
#include <items.h>
#include <pso_version.h>

#include "ship.h"
#include "block.h"
#include <pso_player.h>
#include "shopdata.h"

/* Pull in the packet header types. */
#define PACKETS_H_HEADERS_ONLY
#include "packets.h"
#undef PACKETS_H_HEADERS_ONLY

#include <encryption.h>

/* Forward declarations. */
struct lobby;

#ifndef LOBBY_DEFINED
#define LOBBY_DEFINED
typedef struct lobby lobby_t;
#endif

#define CLIENT_IGNORE_LIST_SIZE     10
#define CLIENT_BLACKLIST_SIZE       30
#define CLIENT_MAX_QSTACK           32

#ifdef PACKED
#undef PACKED
#endif

typedef struct client_blocklist_entry {
    uint32_t gc;
    uint32_t flags;
} client_blocklist_t;

typedef struct client_trade_item {
    uint8_t other_client_id;
    bool confirmed; // true if client has sent a D2 command
    item_t *items;
} client_trade_item_t;

typedef struct client_game_data {
    uint32_t serial_number;
    client_trade_item_t *pending_item_trade;
    uint8_t bb_game_state;
    size_t bb_player_index;
    iitem_t identify_result;
    bool should_save;
    sitem_t shop_items[0x14];
} client_game_data_t;

typedef struct client_type {
    int ver_code;
    char ver_name[5];
    int hdr_size;
} client_type_t;

/* Ship server client structure. */
struct ship_client {
    TAILQ_ENTRY(ship_client) qentry;

    pthread_mutex_t mutex;
    pkt_header_t pkt;

    CRYPT_SETUP ckey;
    CRYPT_SETUP skey;

    int version;
    int sock;
    int hdr_size;
    int client_id;

    int language_code;
    int cur_area;
    int recvbuf_cur;
    int recvbuf_size;

    int sendbuf_cur;
    int sendbuf_size;
    int sendbuf_start;
    int item_count;

    int autoreply_len;
    int lobby_id;

    float x;
    float y;
    float z;
    float w;

    struct sockaddr_storage ip_addr;

    uint32_t guildcard;
    uint32_t menu_id;
    uint32_t preferred_lobby_id;
    uint32_t flags;
    uint32_t arrow;
    uint32_t blocklist_size;
    uint32_t option_flags;
    //uint32_t play_time;

    uint32_t next_item[4];
    uint32_t ignore_list[CLIENT_IGNORE_LIST_SIZE];
    uint32_t blacklist[CLIENT_BLACKLIST_SIZE];

    client_blocklist_t *blocklist;

    uint32_t last_info_req;

    float drop_x;
    float drop_z;
    uint32_t drop_area;
    uint32_t drop_item;
    uint32_t drop_amt;

    uint32_t privilege;
    uint8_t cc_char;
    uint8_t q_lang;
    uint8_t autoreply_on;

    //uint8_t infinite_hp; // cheats enabled
    //uint8_t infinite_tp; // cheats enabled
    uint8_t switch_assist; // cheats enabled
    //subcmd_bb_switch_changed_pkt_t last_switch_enabled_command;
    //uint8_t can_chat;

    uint8_t equip_flags;
    iitem_t iitems[30];

    int isvip;
    uint32_t doneshop[3];
    uint32_t donetrade[3];

    block_t *cur_block;
    lobby_t *cur_lobby;
    player_t *pl;
    int mode; // 通常为0 只有在挑战模式和对战模式才会发生改变

    unsigned char *recvbuf;
    unsigned char *sendbuf;
    void *autoreply;
    FILE *logfile;

    char *infoboard;                    /* Points into the player struct. */
    uint8_t *c_rank;                    /* Points into the player struct. */
    lobby_t *create_lobby;

    uint32_t *next_maps;
    uint32_t *enemy_kills;
    psocn_limits_t *limits;
    xbox_ip_t *xbl_ip;

    time_t last_message;
    time_t last_sent;
    time_t join_time;
    time_t login_time;

    bb_client_config_pkt sec_data;
    psocn_bb_db_char_t *bb_pl;
    psocn_bb_db_opts_t *bb_opts;
    psocn_bb_db_guild_t *bb_guild;

    client_game_data_t game_data;

    int script_ref;
    uint64_t aoe_timer;

    uint32_t q_stack[CLIENT_MAX_QSTACK];
    int q_stack_top;

    uint32_t p2_drops[30];
    uint32_t p2_drops_max;

    lobby_t *lobby_req;

#ifdef DEBUG
    uint8_t sdrops_ver;
    uint8_t sdrops_ep;
    uint8_t sdrops_diff;
    uint8_t sdrops_section;
#endif
};

#define CLIENT_PRIV_LOCAL_GM    0x00000001
#define CLIENT_PRIV_GLOBAL_GM   0x00000002
#define CLIENT_PRIV_LOCAL_ROOT  0x00000004
#define CLIENT_PRIV_GLOBAL_ROOT 0x00000008
#define CLIENT_PRIV_TESTER      0x80000000

/* Character classes */
enum client_classes {
    HUmar = 0,
    HUnewearl,
    HUcast,
    RAmar,
    RAcast,
    RAcaseal,
    FOmarl,
    FOnewm,
    FOnewearl,
    HUcaseal,
    FOmar,
    RAmarl,
    DCPCClassMax = FOnewearl
};

#ifndef SHIP_CLIENT_DEFINED
#define SHIP_CLIENT_DEFINED
typedef struct ship_client ship_client_t;
#endif

TAILQ_HEAD(client_queue, ship_client);

/* The key for accessing our thread-specific receive buffer. */
extern pthread_key_t recvbuf_key;

/* The key used for the thread-specific send buffer. */
extern pthread_key_t sendbuf_key;

/* Possible values for the type field of ship_client_t */
#define CLIENT_TYPE_SHIP        0
#define CLIENT_TYPE_BLOCK       1

/* Possible values for the version field of ship_client_t */
#define CLIENT_VERSION_DCV1     0
#define CLIENT_VERSION_DCV2     1
#define CLIENT_VERSION_PC       2
#define CLIENT_VERSION_GC       3
#define CLIENT_VERSION_EP3      4
#define CLIENT_VERSION_BB       5
#define CLIENT_VERSION_XBOX     6
//#define CLIENT_VERSION_ALL      7

#define CLIENT_FLAG_HDR_READ        0x00000001
#define CLIENT_FLAG_GOT_05          0x00000002
#define CLIENT_FLAG_INVULNERABLE    0x00000004
#define CLIENT_FLAG_INFINITE_TP     0x00000008
#define CLIENT_FLAG_DISCONNECTED    0x00000010
#define CLIENT_FLAG_TYPE_SHIP       0x00000020
#define CLIENT_FLAG_SENT_MOTD       0x00000040
#define CLIENT_FLAG_SHOW_DCPC_ON_GC 0x00000080
#define CLIENT_FLAG_LOGGED_IN       0x00000100
#define CLIENT_FLAG_STFU            0x00000200
#define CLIENT_FLAG_BURSTING        0x00000400
#define CLIENT_FLAG_OVERRIDE_GAME   0x00000800
#define CLIENT_FLAG_IPV6            0x00001000
#define CLIENT_FLAG_AUTO_BACKUP     0x00002000
#define CLIENT_FLAG_SERVER_DROPS    0x00004000
#define CLIENT_FLAG_GC_PROTECT      0x00008000
#define CLIENT_FLAG_IS_NTE          0x00010000
#define CLIENT_FLAG_TRACK_INVENTORY 0x00020000
#define CLIENT_FLAG_TRACK_KILLS     0x00040000
#define CLIENT_FLAG_QLOAD_DONE      0x00080000
#define CLIENT_FLAG_DBG_SDROPS      0x00100000
#define CLIENT_FLAG_LEGIT           0x00200000
#define CLIENT_FLAG_GC_MSG_BOXES    0x00400000
#define CLIENT_FLAG_CDATA_CHECK     0x00800000
#define CLIENT_FLAG_ALWAYS_LEGIT    0x01000000
#define CLIENT_FLAG_WAIT_QPING      0x02000000
#define CLIENT_FLAG_QSTACK_LOCK     0x04000000
#define CLIENT_FLAG_WORD_CENSOR     0x08000000
#define CLIENT_FLAG_SHOPPING        0x10000000

/* Technique numbers */
#define TECHNIQUE_FOIE              0
#define TECHNIQUE_GIFOIE            1
#define TECHNIQUE_RAFOIE            2
#define TECHNIQUE_BARTA             3
#define TECHNIQUE_GIBARTA           4
#define TECHNIQUE_RABARTA           5
#define TECHNIQUE_ZONDE             6
#define TECHNIQUE_GIZONDE           7
#define TECHNIQUE_RAZONDE           8
#define TECHNIQUE_GRANTS            9
#define TECHNIQUE_DEBAND            10
#define TECHNIQUE_JELLEN            11
#define TECHNIQUE_ZALURE            12
#define TECHNIQUE_SHIFTA            13
#define TECHNIQUE_RYUKER            14
#define TECHNIQUE_RESTA             15
#define TECHNIQUE_ANTI              16
#define TECHNIQUE_REVERSER          17
#define TECHNIQUE_MEGID             18

static client_type_t client_type[CLIENT_VERSION_ALL][3] = {
    {CLIENT_VERSION_DCV1,        "v1", 4},
    {CLIENT_VERSION_DCV2,        "v2", 4},
    {CLIENT_VERSION_PC,          "pc", 4},
    {CLIENT_VERSION_GC,          "gc", 4},
    {CLIENT_VERSION_EP3,         "e3", 4},
    {CLIENT_VERSION_BB,          "bb", 8},
    {CLIENT_VERSION_XBOX,        "xb", 4},
};

/* Language codes. */
enum CLIENT_LANG
{
    CLIENT_LANG_CHINESE_S,
    CLIENT_LANG_CHINESE_T,
    CLIENT_LANG_JAPANESE,
    CLIENT_LANG_ENGLISH,
    CLIENT_LANG_GERMAN,
    CLIENT_LANG_FRENCH,
    CLIENT_LANG_SPANISH,
    CLIENT_LANG_KOREAN,
};

/* The list of language codes for the quest directories. */
static const char language_codes[][3] = {
    "cn", "tc", "jp", "en", "de", "fr", "es", "kr"
};

///* The list of version codes for the quest directories. */
//static const char version_codes[][3] = {
//    "v1", "v2", "pc", "gc", "e3", "bb", "xb"
//};
//
///* Sizes of the headers sent on packets */
//static const int hdr_sizes[] = {
//    4, 4, 4, 4, 4, 8, 4
//};

/* Initialize the clients system, allocating any thread specific keys */
int client_init(psocn_ship_t *cfg);

/* Clean up the clients system. */
void client_shutdown(void);

/* Create a new connection, storing it in the list of clients. */
ship_client_t *client_create_connection(int sock, int version, int type,
                                        struct client_queue *clients,
                                        ship_t *ship, block_t *block,
                                        struct sockaddr *ip, socklen_t size);

/* Destroy a connection, closing the socket and removing it from the list. */
void client_destroy_connection(ship_client_t *c, struct client_queue *clients);

void client_send_bb_data(ship_client_t* c);

/* Read data from a client that is connected to any port. */
int client_process_pkt(ship_client_t *c);

/* Retrieve the thread-specific recvbuf for the current thread. */
uint8_t *get_recvbuf(void);

/* Set up a simple mail autoreply. */
int client_set_autoreply(ship_client_t *c, void *buf, uint16_t len);

/* Disable the user's simple mail autoreply (if set). */
int client_disable_autoreply(ship_client_t *c);

/* Check if a client has blacklisted someone. */
int client_has_blacklisted(ship_client_t *c, uint32_t gc);

/* Check if a client has /ignore'd someone. */
int client_has_ignored(ship_client_t *c, uint32_t gc);

/* Send a message to a client telling them that a friend has logged on/off */
void client_send_friendmsg(ship_client_t *c, int on, const char *fname,
                           const char *ship, uint32_t block, const char *nick);

/* Give a Blue Burst client some experience. */
int client_give_exp(ship_client_t *c, uint32_t exp);

/* Give a Blue Burst client some free level ups. */
int client_give_level(ship_client_t *c, uint32_t level_req);

/* Give a PSOv2 client some free level ups. */
int client_give_level_v2(ship_client_t *c, uint32_t level_req);

/* Check if a client's newly sent character data looks corrupted. */
int client_check_character(ship_client_t *c, void *pl, uint8_t ver);

/* Run a legit check on a given client. */
int client_legit_check(ship_client_t *c, psocn_limits_t *limits);

#ifdef ENABLE_LUA
#include <lua.h>

int client_register_lua(lua_State *l);
#endif

#endif /* !CLIENTS_H */
#endif /* !CLIENTS_H_COUNTS_ONLY */
