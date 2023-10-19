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

#ifndef SHIP_H
#define SHIP_H

#include <time.h>
#include <inttypes.h>
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/queue.h>
#else
#include <WinSock_Defines.h>
#include <queue.h>
#endif

#include <sg_packets.h>
#include <pso_packet_length.h>

#include <gnutls/gnutls.h>

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32 
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct event_monster {
    uint16_t monster;
    uint8_t episode;
    uint8_t reserved;
} PACKED event_monster_t;

typedef struct monster_event {
    uint32_t event_id;
    char *event_title;
    uint32_t start_time;
    uint32_t end_time;
    uint8_t difficulties;
    uint8_t versions;
    uint8_t allow_quests;
    uint32_t monster_count;
    event_monster_t *monsters;
} monster_event_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

typedef struct ship {
    TAILQ_ENTRY(ship) qentry;

    pthread_mutex_t pkt_mutex;
    pthread_rwlock_t rwlock;
    pthread_t read_tid;
    pthread_t write_tid;
    //pthread_t thd;

    int sock;
    int disconnected;
    int has_key;
    int err_inconnect;
    uint32_t flags;
    uint32_t menu;

    char remote_host6[128];
    struct in6_addr remote_addr6;
    struct sockaddr_storage conn_addr;

    char remote_host4[32];
    in_addr_t remote_addr4;
    uint32_t proto_ver;
    uint32_t privileges;

    uint16_t port;
    uint16_t key_idx;
    uint16_t clients;
    uint16_t games;
    uint16_t menu_code;

    int ship_number;
    uint8_t ship_nonce[4];
    uint8_t gate_nonce[4];

    time_t last_message;
    time_t last_ping;

    unsigned char *recvbuf;
    int recvbuf_cur;
    int recvbuf_size;
    shipgate_hdr_t pkt;
    int hdr_read;

    unsigned char *sendbuf;
    int sendbuf_cur;
    int sendbuf_size;
    int sendbuf_start;

    gnutls_session_t session;

    char ship_name[13];
    char ship_cn_name[13];
} ship_t;

TAILQ_HEAD(ship_queue, ship);
extern struct ship_queue ships;

static char tmp_ship_desc[128] = { 0 };

/* The key for accessing our thread-specific receive buffer. */
extern pthread_key_t recvbuf_key;

/* The key used for the thread-specific send buffer. */
extern pthread_key_t sendbuf_key;

/* Create a new connection, storing it in the list of ships. */
ship_t *create_connection_tls(int sock, struct sockaddr *addr, socklen_t size);

/* Destroy a connection, closing the socket and removing it from the list. */
void destroy_connection_err(ship_t* s);
void destroy_connection(ship_t *c);

/* 更新ipv4 */
int update_ship_ipv4(ship_t* c);

char* get_ship_describe(ship_t* s);

/* Handle incoming data to the shipgate. */
int handle_pkt(ship_t *s);

/* 加载初始公会默认标志数据 */
int load_guild_default_flag(char* file);

uint8_t get_bb_max_tech_level(uint8_t ch_class, int tech);

void fix_player_max_tech_level(psocn_bb_char_t* character);

/* IDs for the ship_metadata table */
#define SHIP_METADATA_VER_VERSION       1
#define SHIP_METADATA_VER_FLAGS         2
#define SHIP_METADATA_VER_CMT_HASH      3
#define SHIP_METADATA_VER_CMT_TIME      4
#define SHIP_METADATA_VER_CMT_REF       5
#define SHIP_METADATA_UNAME_NAME        6
#define SHIP_METADATA_UNAME_NODE        7
#define SHIP_METADATA_UNAME_RELEASE     8
#define SHIP_METADATA_UNAME_VERSION     9
#define SHIP_METADATA_UNAME_MACHINE     10

#endif /* !SHIP_H */
