/*
    梦幻之星中国 补丁服务器
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

#ifndef PATCH_SERVER_H
#define PATCH_SERVER_H

#include <stdint.h>
#include <queue.h>
#include <WinSock_Defines.h>

#include <encryption.h>
#include <psoconfig.h>
#include <mtwist.h>

/* Define as a signed type of the same size as size_t. */
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

static const int hdr_sizes[] = {
    4, 4, 4, 4, 8, 8, 4, 4
};

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32 
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* The common packet header on top of all packets. */
typedef struct pkt_header {
    uint16_t pkt_len;
    uint16_t pkt_type;
} PACKED pkt_header_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* Patch server client structure. */
typedef struct patch_client {
    TAILQ_ENTRY(patch_client) qentry;

    int type;
    int sock;
    int version;
    int disconnected;
    int is_ipv6;

    struct sockaddr_storage ip_addr;
    CRYPT_SETUP client_cipher;
    CRYPT_SETUP server_cipher;

    unsigned char *recvbuf;
    int pkt_cur;
    int pkt_sz;

    unsigned char *sendbuf;
    int sendbuf_cur;
    int sendbuf_size;
    int sendbuf_start;

    struct cfile_queue files;
    int sending_data;
    int cur_chunk;
    int cur_pos;

    /* Random number generator state */
    struct mt19937_state rng;

} patch_client_t;

TAILQ_HEAD(client_queue, patch_client);
extern struct client_queue clients;

extern patch_config_t* cfg;
extern psocn_srvconfig_t* srvcfg;
extern psocn_srvsockets_t patch_sockets[PATCH_CLIENT_SOCKETS_TYPE_MAX];

/* Create a new connection, storing it in the list of clients. */
patch_client_t* create_connection(int sock, int type,
    struct sockaddr* ip, socklen_t size);

/* Destroy a connection, closing the socket and removing it from the list. */
void destroy_connection(patch_client_t* c);

/* Act on a list done packet from the client. */
int handle_list_done(patch_client_t* c);

///* Process one patch packet. */
//int process_patch_packet(patch_client_t* c, void* pkt);
//
///* Process one data packet. */
//int process_data_packet(patch_client_t* c, void* pkt);

/* Process data packet. */
int process_packet(patch_client_t* c, void* pkt);

#endif /* !PATCH_SERVER_H */
