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

#ifndef PATCH_PACKETS_H
#define PATCH_PACKETS_H

#include <inttypes.h>
#include <WinSock_Defines.h>

#include <encryption.h>

#include "patch_server.h"

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
#define LE16(x) (((x >> 8) & 0xFF) | ((x & 0xFF) << 8))
#define LE32(x) (((x >> 24) & 0x00FF) | \
                 ((x >>  8) & 0xFF00) | \
                 ((x & 0xFF00) <<  8) | \
                 ((x & 0x00FF) << 24))
#else
#define LE16(x) x
#define LE32(x) x
#endif

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32 
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* The Welcome packet for setting up encryption keys. */
typedef struct patch_welcome {
    pkt_header_t hdr;
    char copyright[44];      /* Copyright message, see below. */
    uint8_t padding[20];     /* All zeroes */
    uint32_t server_vector;
    uint32_t client_vector;
} PACKED patch_welcome_pkt;

/* The Login packet which contains the user's username/password. */
typedef struct patch_login {
    pkt_header_t hdr;
    uint8_t padding1[12];    /* All zeroes */
    char username[16];
    char password[16];
    uint8_t padding2[64];    /* All zeroes */
} PACKED patch_login_pkt;

/* The packet informing the client of the DATA portion's location. */
typedef struct patch_redirect {
    pkt_header_t hdr;
    uint32_t data_ip;        /* IP address of the DATA portion (big-endian) */
    uint16_t data_port;      /* Port of the DATA portion (big-endian) */
    uint16_t padding;        /* Zero */
} PACKED patch_redirect_pkt;

#ifdef PSOCN_ENABLE_IPV6

/* IPv6 version of the above */
typedef struct patch_redirect6 {
    pkt_header_t hdr;
    uint8_t data_ip[16];
    uint16_t data_port;
    uint16_t padding;
} PACKED patch_redirect6_pkt;

#endif

/* The Change Directory packet, which tells the client to switch directories. */
typedef struct patch_chdir {
    pkt_header_t hdr;
    char dir[64];
} PACKED patch_chdir_pkt;

/* The file info request packet, which tells the client to check a file. */
typedef struct patch_file_info {
    pkt_header_t hdr;
    uint32_t patch_id;
    char filename[32];
} PACKED patch_file_info_pkt;

/* A reply to a file info request. */
typedef struct patch_file_info_reply {
    pkt_header_t hdr;
    uint32_t patch_id;
    uint32_t checksum;
    uint32_t size;
} PACKED patch_file_info_reply;

/* A packet to let the client know that we're going to send files. */
typedef struct patch_send_info {
    pkt_header_t hdr;
    uint32_t total_length;
    uint32_t total_files;
} PACKED patch_send_info_pkt;

/* A packet to let the client know we're about to send a file. */
typedef struct patch_file_send {
    pkt_header_t hdr;
    uint32_t padding;
    uint32_t size;
    char filename[48];       /* Why is this longer than in the info pkt? */
} PACKED patch_file_send_pkt;

/* A packet to tell the client about the current chunk of the file. */
typedef struct patch_data_send {
    pkt_header_t hdr;
    uint32_t chunk_num;
    uint32_t checksum;
    uint32_t chunk_size;
} PACKED patch_data_send_pkt;

/* A packet to tell the client that we're done sending the current file. */
typedef struct patch_file_done {
    pkt_header_t hdr;
    uint32_t padding;
} PACKED patch_file_done_pkt;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* Parameters for the various packets. */
#define PATCH_WELCOME_TYPE              0x0002
#define PATCH_LOGIN_TYPE                0x0004
#define PATCH_FILE_SEND                 0x0006
#define PATCH_DATA_SEND                 0x0007
#define PATCH_FILE_DONE                 0x0008
#define PATCH_SET_DIRECTORY             0x0009
#define PATCH_ONE_DIR_UP                0x000A
#define PATCH_START_LIST                0x000B
#define PATCH_FILE_INFO                 0x000C
#define PATCH_INFO_FINISHED             0x000D
#define PATCH_FILE_INFO_REPLY           0x000F
#define PATCH_FILE_LIST_DONE            0x0010
#define PATCH_SEND_INFO                 0x0011
#define PATCH_SEND_DONE                 0x0012
#define PATCH_MESSAGE_TYPE              0x0013
#define PATCH_REDIRECT_TYPE             0x0014
#define PATCH_REDIRECT6_TYPE            0x0614
#define MSG_BOX_TYPE                    0x001A //大信息窗口文本代码
#define GC_MSG_BOX_TYPE                 0x00D5 //GC BB XB信息窗
#define GC_MSG_BOX_CLOSED_TYPE          0x00D6 //GC 信息窗关闭

#define PACKET_HEADER_LENGTH            0x0004
#define PATCH_WELCOME_LENGTH            0x004C
#define PATCH_REDIRECT_LENGTH           0x000C
#define PATCH_REDIRECT6_LENGTH          0x0018

#define PATCH_FILE_SEND_LENGTH          0x003C
#define PATCH_DATA_SEND_LENGTH          0x0010
#define PATCH_FILE_DONE_LENGTH          0x0008
#define PATCH_SET_DIRECTORY_LENGTH      0x0044
#define PATCH_FILE_INFO_LENGTH          0x0028
#define PATCH_FILE_INFO_REPLY_LENGTH    0x0010
#define PATCH_SEND_INFO_LENGTH          0x000C

/* This must be placed into the copyright field in the welcome packet. */
const static char patch_welcome_copyright[] =
    "Patch Server. Copyright SonicTeam, LTD. 2001";

/* Functions for sending the specified packets. */
/* Send a simple "header-only" packet to the given client. */
int send_simple(patch_client_t *c, uint16_t type);

/* Send a Welcome packet to the given client. */
int send_welcome(patch_client_t *c, uint32_t svect, uint32_t cvect);

/* Send the packet containing the textual welcome message to the client. */
int send_message(patch_client_t* c, uint16_t* msg, size_t size);

/* Send the data server redirect packet to the given client.
   IP and port MUST both be in network byte-order. */
int send_redirect(patch_client_t *c, in_addr_t ip, uint16_t port);

#ifdef PSOCN_ENABLE_IPV6

/* Send a IPv6 redirect packet to the given client.
   The port must be in network byte order. */
int send_redirect6(patch_client_t *c, const uint8_t ip[16], uint16_t port);

#endif

/* Send a change directory packet to the given client. */
int send_chdir(patch_client_t *c, const char dir[]);

/* Send a file information packet to the given client. */
int send_file_info(patch_client_t *c, uint32_t idx, const char fn[]);

/* Send a file-send information packet to the given client. */
int send_send_info(patch_client_t *c, uint32_t size, uint32_t files);

/* Send a file send packet to the given client. */
int send_file_send(patch_client_t *c, uint32_t size, const char fn[]);

/* Send a part of a file to the given client (dividing it into chunks). */
int send_file_chunk(patch_client_t *c, const char fn[], const char dir[]);

/* Send a file done packet to the given client. */
int send_file_done(patch_client_t *c);

#endif /* !PATCH_PACKETS_H */
