/*
    梦幻之星中国 认证服务器
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

#ifndef AUTH_H
#define AUTH_H

#include <queue.h>
#include <WinSock_Defines.h>

#include <psoconfig.h>
#include <encryption.h>
#include <database.h>
#include <mtwist.h>

#include <pso_player.h>

#include "patch.h"

/* Define as a signed type of the same size as size_t. */
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

#define AUTH_SERVER_VERSION VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

/* Determine if a client is in our LAN */
#define IN_NET(c, s, n) ((c & n) == (s & n))

#define PACKETS_H_HEADERS_ONLY
#include "packets.h"
#undef PACKETS_H_HEADERS_ONLY

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32 
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

typedef struct client_type {
    int ver_code;
    char ver_name[16];
    char ver_name_file[5];
    int hdr_size;
} client_type_t;

/* Login server client structure. */
typedef struct login_client {
    TAILQ_ENTRY(login_client) qentry;

    int type;
    int sock;
    int disconnected;
    int auth;
    int is_ipv6;
    int motd_wait;

    uint64_t hwinfo;
    uint32_t login_hwinfo[2];

    struct sockaddr_storage ip_addr;

    char username[0x30];
    uint32_t guildcard;
    uint32_t account_id;
    uint32_t guild_id;
    uint32_t flags;
    uint32_t menu_id;
    uint32_t preferred_lobby_id;
    int isgm;
    int islogged;
    uint32_t dress_flag;

    int language_code;
    uint32_t priv;
    uint16_t port;
    uint32_t ext_version;

    uint32_t client_key;
    uint32_t server_key;
    int got_first;
    int version;
    uint16_t bbversion;
    uint8_t bbversion_string[40];
    uint32_t det_version;

    CRYPT_SETUP client_cipher;
    CRYPT_SETUP server_cipher;

    unsigned char *recvbuf;
    int pkt_cur;
    int pkt_sz;

    unsigned char *sendbuf;
    int sendbuf_cur;
    int sendbuf_size;
    int sendbuf_start;

    bb_guildcard_data_t *gc_data;
    psocn_bb_db_guild_t *guild_data;
    bb_client_config_pkt sec_data;
    int hdr_read;

    /* Only used for the Dreamcast Network Trial Edition */
    char serial_number[16];
    char access_key[16];

    /* Random number generator state */
    struct mt19937_state rng;

} login_client_t;

/* Privilege levels */
#define CLIENT_PRIV_LOCAL_GM    0x01
#define CLIENT_PRIV_GLOBAL_GM   0x02
#define CLIENT_PRIV_LOCAL_ROOT  0x04
#define CLIENT_PRIV_GLOBAL_ROOT 0x08

#define IS_GLOBAL_GM(c)     (!!((c)->priv & CLIENT_PRIV_GLOBAL_GM))
#define IS_GLOBAL_ROOT(c)   (!!((c)->priv & CLIENT_PRIV_GLOBAL_ROOT))

/* Values for the type of the login_client_t */
#define CLIENT_AUTH_DC              0
#define CLIENT_AUTH_PC              1
#define CLIENT_AUTH_GC              2
#define CLIENT_AUTH_EP3             3
#define CLIENT_AUTH_BB_LOGIN        4
#define CLIENT_AUTH_BB_CHARACTER    5
#define CLIENT_AUTH_DCNTE           6
#define CLIENT_AUTH_XBOX            7

#define CLIENT_AUTH_VERSION_COUNT           8

/* 扩展版本代码位字段值 */
#define CLIENT_EXTVER_DC            (1 << 0)
#define CLIENT_EXTVER_PC            (1 << 1)
#define CLIENT_EXTVER_GC            (1 << 2)
#define CLIENT_EXTVER_BB            (1 << 3)

/* Subtypes for CLIENT_EXTVER_DC */
#define CLIENT_EXTVER_DCNTE         (1 << 4)
#define CLIENT_EXTVER_DCV1          (2 << 4)
#define CLIENT_EXTVER_DCV2          (3 << 4)    /* Both v1/nte bits = v2 */
#define CLIENT_EXTVER_DC_VER_MASK   (3 << 4)
#define CLIENT_EXTVER_DC50HZ        (1 << 7)
#define CLIENT_EXTVER_GC_TRIAL      (0 << 4)    /* No bits set = GC Trial */

/* Subtypes for CLIENT_EXTVER_PC */
#define CLIENT_EXTVER_PCNTE         (1 << 4)

/* Subtypes for CLIENT_EXTVER_GC */
#define CLIENT_EXTVER_GC_EP12       (1 << 4)
#define CLIENT_EXTVER_GC_EP3        (2 << 4)
#define CLIENT_EXTVER_GC_EP12PLUS   (3 << 4)
#define CLIENT_EXTVER_GC_REG_MASK   (3 << 6)    /* Values below... */
#define CLIENT_EXTVER_GC_REG_US     (0 << 6)
#define CLIENT_EXTVER_GC_REG_JP     (1 << 6)
#define CLIENT_EXTVER_GC_REG_PAL60  (2 << 6)
#define CLIENT_EXTVER_GC_REG_PAL50  (3 << 6)
#define CLIENT_EXTVER_GC_EP_MASK    (3 << 4)
#define CLIENT_EXTVER_GC_REG_MASK   (3 << 6)
#define CLIENT_EXTVER_GC_VER_MASK   (0xFF << 8)

/* 用于SET_FLAG */
#define CLIENT_AUTH_FLAG_CANCELED   0x00000000
#define CLIENT_AUTH_FLAG_RECREATE   0x00000001
#define CLIENT_AUTH_FLAG_DREESING   0x00000002

static client_type_t client_type[CLIENT_AUTH_VERSION_COUNT] = {
    {CLIENT_AUTH_DC,           "DreamCard"      ,  "dc", 4},
    {CLIENT_AUTH_PC,           "PC EP1&EP2"     ,  "pc", 4},
    {CLIENT_AUTH_GC,           "GameCube"       ,  "gc", 4},
    {CLIENT_AUTH_EP3,          "Episode III"    ,  "e3", 4},
    {CLIENT_AUTH_BB_LOGIN,     "Blue Burst"     ,  ""  , 8},
    {CLIENT_AUTH_BB_CHARACTER, "Blue Burst"     ,  ""  , 8},
    {CLIENT_AUTH_DCNTE,        "DreamCard NTE"  ,  ""  , 4},
    {CLIENT_AUTH_XBOX,         "XBOX"           ,  "xb", 4},
};

/* Language codes. */
enum CLIENT_LANG {
    CLIENT_LANG_CHINESE_S,
    CLIENT_LANG_CHINESE_T,
    CLIENT_LANG_JAPANESE,
    CLIENT_LANG_ENGLISH,
    CLIENT_LANG_GERMAN,
    CLIENT_LANG_FRENCH,
    CLIENT_LANG_SPANISH,
    CLIENT_LANG_KOREAN,
    CLIENT_LANG_ALL
};

/* The list of language codes for the quest directories. */
static const char language_codes[][3] = {
    "cn", "tc", "jp", "en", "de", "fr", "sp", "kr"
};

TAILQ_HEAD(client_queue, login_client);
extern struct client_queue clients;

extern psocn_dbconn_t conn;
extern psocn_config_t *cfg;
extern psocn_srvconfig_t* srvcfg;
extern patch_list_t *patches_v2;
extern patch_list_t *patches_gc;

extern psocn_srvsockets_t dc_sockets[NUM_AUTH_DC_SOCKS];
extern psocn_srvsockets_t pc_sockets[NUM_AUTH_PC_SOCKS];
extern psocn_srvsockets_t gc_sockets[NUM_AUTH_GC_SOCKS];
extern psocn_srvsockets_t ep3_sockets[NUM_AUTH_EP3_SOCKS];
extern psocn_srvsockets_t xb_sockets[NUM_AUTH_XB_SOCKS];
extern psocn_srvsockets_t web_sockets[NUM_AUTH_WEB_SOCKS];
extern psocn_srvsockets_t bb_sockets[NUM_AUTH_BB_SOCKS];

void print_packet(unsigned char* pkt, int len);

/* Check if an IP has been IP banned from the server. */
int is_ip_banned(login_client_t* c, time_t* until, char* reason);

/* Check if a user is banned by guildcard. */
int is_gc_banned(uint32_t gc, time_t* until, char* reason);

int send_ban_msg(login_client_t* c, time_t until, const char* reason);

int keycheck(char serial[8], char ak[8]);

login_client_t* create_connection(int sock, int type, struct sockaddr* ip,
    socklen_t size, uint16_t port);
void destroy_connection(login_client_t* c);

int read_from_client(login_client_t* c);

//void disconnect_from_ships(uint32_t gcn);

/* In auth_server.c */
int ship_transfer(login_client_t* c, uint32_t shipid);
void read_quests();

/* In dclogin.c */
int process_dclogin_packet(login_client_t* c, void* pkt);

void init_i18n(void);
void cleanup_i18n(void);

/* In bblogin.c */
int process_bblogin_packet(login_client_t* c, void* pkt);

/* In bbcharacter.c */
int process_bbcharacter_packet(login_client_t* c, void* pkt);
int load_param_data(void);
void cleanup_param_data(void);
int load_bb_char_data(void);

#ifdef HAVE_LIBMINI18N
#include <mini18n-multi.h>
#define __(c, s) mini18n_get(langs[c->language_code], s)
extern mini18n_t langs[CLIENT_LANG_ALL];
#else
#define __(c, s) s
#endif

#endif /* !AUTH_H */
