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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <WinSock_Defines.h>
#include <iconv.h>
#include <limits.h>
#include <stdarg.h>
#include <ctype.h>

#include <encryption.h>
#include <database.h>
#include <f_logs.h>
#include <f_iconv.h>
#include <pso_menu.h>

#include "ship_packets.h"
#include "utils.h"
#include "subcmd.h"
#include "quests.h"
#include "admin.h"
#include "items.h"

extern char ship_host4[32];
extern char ship_host6[128];
extern uint32_t ship_ip4;
extern uint8_t ship_ip6[16];

/* Options for choice search. */
typedef struct cs_opt {
    uint16_t menu_id;
    uint16_t item_id;
    char text[0x1C];
} cs_opt_t;

static cs_opt_t cs_options[] = {
    { 0x0000, 0x0001, "Hunter's Level" },
    { 0x0001, 0x0000, "Any" },
    { 0x0001, 0x0001, "Own Level +/- 5" },
    { 0x0001, 0x0002, "Level 1-10" },
    { 0x0001, 0x0003, "Level 11-20" },
    { 0x0001, 0x0004, "Level 21-40" },
    { 0x0001, 0x0005, "Level 41-60" },
    { 0x0001, 0x0006, "Level 61-80" },
    { 0x0001, 0x0007, "Level 81-100" },
    { 0x0001, 0x0008, "Level 101-120" },
    { 0x0001, 0x0009, "Level 121-160" },
    { 0x0001, 0x000A, "Level 161-200" },
    { 0x0000, 0x0002, "Hunter's Class" },
    { 0x0002, 0x0000, "Any" },
    { 0x0002, 0x0001, "HUmar" },
    { 0x0002, 0x0002, "HUnewearl" },
    { 0x0002, 0x0003, "HUcast" },
    { 0x0002, 0x0004, "RAmar" },
    { 0x0002, 0x0005, "RAcast" },
    { 0x0002, 0x0006, "RAcaseal" },
    { 0x0002, 0x0007, "FOmarl" },
    { 0x0002, 0x0008, "FOnewm" },
    { 0x0002, 0x0009, "FOnewearl" }
};

#define CS_OPTIONS_COUNT 23

/* Definition of the options in each menu of the GM menu */
typedef struct gm_opt {
    uint32_t menu_id;
    uint32_t item_id;
    uint32_t lobby_type;
    uint8_t privilege;
    char text[0x10];
} gm_opt_t;

/* GM Menu options */
static gm_opt_t gm_opts[] = {
    {   MENU_ID_GM              , ITEM_ID_GM_REF_QUESTS , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Refresh Quests"  },
    {   MENU_ID_GM              , ITEM_ID_GM_REF_GMS    , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "Refresh GMs"     },
    {   MENU_ID_GM              , ITEM_ID_GM_REF_LIMITS , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Refresh Limits"  },
    {   MENU_ID_GM              , ITEM_ID_GM_SHUTDOWN   , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "Shutdown"        },
    {   MENU_ID_GM              , ITEM_ID_GM_RESTART    , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "Restart"         },
    {   MENU_ID_GM              , ITEM_ID_GM_GAME_EVENT , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Game Event"      },
    {   MENU_ID_GM              , ITEM_ID_GM_LOBBY_EVENT, 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Lobby Event"     },
    {   MENU_ID_GM_SHUTDOWN     , 1                     , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "1 Minute"        },
    {   MENU_ID_GM_SHUTDOWN     , 5                     , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "5 Minutes"       },
    {   MENU_ID_GM_SHUTDOWN     , 15                    , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "15 Minutes"      },
    {   MENU_ID_GM_SHUTDOWN     , 30                    , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "30 Minutes"      },
    {   MENU_ID_GM_SHUTDOWN     , 60                    , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "60 Minutes"      },
    {   MENU_ID_GM_RESTART      , 1                     , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "1 Minute"        },
    {   MENU_ID_GM_RESTART      , 5                     , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "5 Minutes"       },
    {   MENU_ID_GM_RESTART      , 15                    , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "15 Minutes"      },
    {   MENU_ID_GM_RESTART      , 30                    , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "30 Minutes"      },
    {   MENU_ID_GM_RESTART      , 60                    , 0x07,
        CLIENT_PRIV_LOCAL_ROOT  , "60 Minutes"      },
    {   MENU_ID_GM_GAME_EVENT   , GAME_EVENT_NONE       , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "None"            },
    {   MENU_ID_GM_GAME_EVENT   , GAME_EVENT_CHRISTMAS  , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Christmas"       },
    {   MENU_ID_GM_GAME_EVENT   , GAME_EVENT_21ST       , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "21st Century"    },
    {   MENU_ID_GM_GAME_EVENT   , GAME_EVENT_VALENTINES , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Valentine's Day" },
    {   MENU_ID_GM_GAME_EVENT   , GAME_EVENT_EASTER     , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Easter"          },
    {   MENU_ID_GM_GAME_EVENT   , GAME_EVENT_HALLOWEEN  , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Halloween"       },
    {   MENU_ID_GM_GAME_EVENT   , GAME_EVENT_SONIC      , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Sonic"           },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_NONE      , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "None"            },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_CHRISTMAS , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Christmas"       },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_VALENTINES, 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Valentine's Day" },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_EASTER    , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Easter"          },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_HALLOWEEN , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Halloween"       },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_SONIC     , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Sonic"           },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_NEWYEARS  , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "New Year's"      },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_SPRING    , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Spring"          },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_WHITEDAY  , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "White Day"       },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_WEDDING   , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Wedding"         },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_AUTUMN    , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Autumn"          },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_FLAGS     , 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Flags"           },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_SPRINGFLAG, 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Spring (flags)"  },
    {   MENU_ID_GM_LOBBY_EVENT  , LOBBY_EVENT_ALT_NORMAL, 0x07,
        CLIENT_PRIV_LOCAL_GM    , "Normal (Alt)"    },
    /* End of list marker -- Do not remove */
    {   0                       , 0                     , 0x00,
        0                       , ""                }
};

/* Forward declaration. */
static int send_dc_lobby_arrows(lobby_t *l, ship_client_t *c);
static int send_bb_lobby_arrows(lobby_t *l, ship_client_t *c);

/* Send a raw packet away. */
static int send_raw(ship_client_t *c, int len, uint8_t *sendbuf) {
    ssize_t rv, total = 0;
    void *tmp;

    /* Keep trying until the whole thing's sent. */
    if(!c->sendbuf_cur) {
        while(total < len) {
            rv = send(c->sock, sendbuf + total, len - total, 0);

            //ERR_LOG("舰船数据端口 %d 发送数据 = %d 字节 版本识别 = %d", c->sock, rv, c->version);

            if(rv == SOCKET_ERROR && errno != EAGAIN) {
                return SOCKET_ERROR;
            }
            else if(rv == SOCKET_ERROR) {
                break;
            }

            total += rv;
        }
    }

    rv = len - total;

    if(rv) {
        /* Move out any already transferred data. */
        if(c->sendbuf_start) {
            memmove(c->sendbuf, c->sendbuf + c->sendbuf_start,
                    c->sendbuf_cur - c->sendbuf_start);
            c->sendbuf_cur -= c->sendbuf_start;
            c->sendbuf_start = 0;
        }

        /* See if we need to reallocate the buffer. */
        if(c->sendbuf_cur + rv > c->sendbuf_size) {
            tmp = realloc(c->sendbuf, c->sendbuf_cur + rv);

            /* If we can't allocate the space, bail. */
            if(tmp == NULL) {
                return -1;
            }

            c->sendbuf_size = c->sendbuf_cur + rv;
            c->sendbuf = (unsigned char *)tmp;
        }

        /* Copy what's left of the packet into the output buffer. */
        memcpy(c->sendbuf + c->sendbuf_cur, sendbuf + total, rv);
        c->sendbuf_cur += rv;
    }

    return 0;
}

/* 加密并发送一个数据包. */
int crypt_send(ship_client_t *c, int len, uint8_t *sendbuf) {
    /* Expand it to be a multiple of 8/4 bytes long */
    while(len & (c->hdr_size - 1))
        sendbuf[len++] = 0;

    /* If we're logging the client, write into the log */
    if(c->logfile) {
        fprint_packet(c->logfile, sendbuf, len, 0);
    }

#ifdef DEBUG
    DBG_LOG("舰船：发送数据 指令 = 0x%04X %s 长度 = %d 字节 GC = %u",
        sendbuf[0x02], c_cmd_name(sendbuf[0x02], 0), len, c->guildcard);
    display_packet(sendbuf, len);
#endif // DEBUG

    /* Encrypt the packet */
    CRYPT_CryptData(&c->skey, sendbuf, len, 1);

    return send_raw(c, len, sendbuf);
}

/* 获取当前线程的 sendbuf 线程特定数据. */
uint8_t *get_sendbuf() {
    uint8_t *sendbuf = (uint8_t *)pthread_getspecific(sendbuf_key);

    /* If we haven't initialized the sendbuf pointer yet for this thread, then
       we need to do that now. */
    if(!sendbuf) {
        sendbuf = (uint8_t *)malloc(65536);

        if(!sendbuf) {
            perror("malloc");
            return NULL;
        }

        if(pthread_setspecific(sendbuf_key, sendbuf)) {
            perror("pthread_setspecific");
            free_safe(sendbuf);
            return NULL;
        }

        memset(sendbuf, 0, 65536);
    }

    return sendbuf;
}

/* Send a Dreamcast welcome packet to the given client. */
int send_dc_welcome(ship_client_t *c, uint32_t svect, uint32_t cvect) {
    uint8_t *sendbuf = get_sendbuf();
    dc_welcome_pkt *pkt = (dc_welcome_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(dc_welcome_pkt));

    /* 填充数据头 */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_len = LE16(DC_WELCOME_LENGTH);
        pkt->hdr.dc.pkt_type = WELCOME_TYPE;
    }
    else {
        pkt->hdr.pc.pkt_len = LE16(DC_WELCOME_LENGTH);
        pkt->hdr.pc.pkt_type = WELCOME_TYPE;
    }

    /* Fill in the required message */
    memcpy(pkt->copyright, dc_welcome_copyright, 56);

    /* Fill in the anti message */
    memcpy(pkt->after_message, anti_copyright, 188);

    /* Fill in the encryption vectors */
    pkt->svect = LE32(svect);
    pkt->cvect = LE32(cvect);

    if(c->logfile) {
        fprint_packet(c->logfile, sendbuf, DC_WELCOME_LENGTH, 0);
    }

    /* 将数据包发送出去 */
    return send_raw(c, DC_WELCOME_LENGTH, sendbuf);
}

/* Send a Blue Burst Welcome packet to the given client. */
int send_bb_welcome(ship_client_t *c, const uint8_t svect[48],
                    const uint8_t cvect[48]) {
    uint8_t *sendbuf = get_sendbuf();
    bb_welcome_pkt *pkt = (bb_welcome_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(bb_welcome_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(BB_WELCOME_LENGTH);
    pkt->hdr.pkt_type = LE16(BB_WELCOME_TYPE);

    /* Fill in the required message */
    memcpy(pkt->copyright, bb_welcome_copyright, 75);

    /* Fill in the anti message */
    memcpy(pkt->after_message, anti_copyright, 188);

    /* Fill in the encryption vectors */
    memcpy(pkt->server_key, svect, 0x30);
    memcpy(pkt->client_key, cvect, 0x30);

    if(c->logfile) {
        fprint_packet(c->logfile, sendbuf, BB_WELCOME_LENGTH, 0);
    }

    /* 将数据包发送出去 */
    return send_raw(c, BB_WELCOME_LENGTH, sendbuf);
}

/* Send the Dreamcast security packet to the given client. */
int send_dc_security(ship_client_t *c, uint32_t gc, uint8_t *data,
                     int data_len) {
    uint8_t *sendbuf = get_sendbuf();
    dc_security_pkt *pkt = (dc_security_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Wipe the packet */
    memset(pkt, 0, sizeof(dc_security_pkt));

    /* 填充数据头 */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = SECURITY_TYPE;
        pkt->hdr.dc.pkt_len = LE16((0x0C + data_len));
    }
    else {
        pkt->hdr.pc.pkt_type = SECURITY_TYPE;
        pkt->hdr.pc.pkt_len = LE16((0x0C + data_len));
    }

    /* Fill in the guildcard/tag */
    pkt->tag = LE32(0x00010000);
    pkt->guildcard = LE32(gc);

    /* Copy over any security data */
    if(data_len)
        memcpy(pkt->security_data, data, data_len);

    /* 将数据包发送出去 */
    return crypt_send(c, 0x0C + data_len, sendbuf);
}

/* Send a Blue Burst security packet to the given client. */
int send_bb_security(ship_client_t *c, uint32_t gc, uint32_t err,
                     uint32_t guild_id, const void *data, int data_len) {
    uint8_t *sendbuf = get_sendbuf();
    bb_security_pkt *pkt = (bb_security_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    //DBG_LOG("send_bb_security %d", data_len);

    /* Make sure the data is sane */
    if(data_len > 40 || data_len < 0) {
        return -1;
    }

    /* Wipe the packet */
    memset(pkt, 0, sizeof(bb_security_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(BB_SECURITY_LENGTH);
    pkt->hdr.pkt_type = LE16(BB_SECURITY_TYPE);

    /* Fill in the information */
    pkt->err_code = LE32(err);
    pkt->player_tag = LE32(0x00010000);
    pkt->guildcard = LE32(gc);
    pkt->guild_id = LE32(guild_id);
    pkt->caps = LE32(0x00000102);   /* ??? - newserv sets it this way */

    /* Copy over any security data */
    if(data_len)
        memcpy(&pkt->security_data, data, data_len);

    /* 将数据包发送出去 */
    return crypt_send(c, BB_SECURITY_LENGTH, sendbuf);
}

/* Send a redirect packet to the given client. */
static int send_dc_redirect(ship_client_t *c, in_addr_t ip, uint16_t port) {
    uint8_t *sendbuf = get_sendbuf();
    dc_redirect_pkt *pkt = (dc_redirect_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Wipe the packet */
    memset(pkt, 0, DC_REDIRECT_LENGTH);

    /* 填充数据头 */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = REDIRECT_TYPE;
        pkt->hdr.dc.pkt_len = LE16(DC_REDIRECT_LENGTH);
    }
    else {
        pkt->hdr.pc.pkt_type = REDIRECT_TYPE;
        pkt->hdr.pc.pkt_len = LE16(DC_REDIRECT_LENGTH);
    }

    /* Fill in the IP and port */
    pkt->ip_addr = ip;
    pkt->port = LE16(port);

    /* 将数据包发送出去 */
    return crypt_send(c, DC_REDIRECT_LENGTH, sendbuf);
}

static int send_bb_redirect(ship_client_t *c, in_addr_t ip, uint16_t port) {
    uint8_t *sendbuf = get_sendbuf();
    bb_redirect_pkt *pkt = (bb_redirect_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Wipe the packet */
    memset(pkt, 0, BB_REDIRECT_LENGTH);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(REDIRECT_TYPE);
    pkt->hdr.pkt_len = LE16(BB_REDIRECT_LENGTH);

    /* Fill in the IP and port */
    pkt->ip_addr = ip;
    pkt->port = LE16(port);

    /* 将数据包发送出去 */
    return crypt_send(c, BB_REDIRECT_LENGTH, sendbuf);
}

int send_redirect(ship_client_t *c, char* host4, in_addr_t ip, uint16_t port) {
    struct addrinfo hints;
    struct addrinfo* server, * j;
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in* addr4;
    struct sockaddr_in6* addr6;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    /* If we don't have a separate IPv6 host set, we're done. */
    if (!host4) {
        return 0;
    }

    if (getaddrinfo(host4, "9000", &hints, &server)) {
        ERR_LOG("无效舰船地址: %s", host4);
        return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET) {
            addr4 = (struct sockaddr_in*)j->ai_addr;
            inet_ntop(j->ai_family, &addr4->sin_addr, ipstr, INET6_ADDRSTRLEN);
            //SHIPS_LOG("    获取到 IPv4 地址: %s", ipstr);
            ship_ip4 = addr4->sin_addr.s_addr;
        }
        else if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //SHIPS_LOG("    获取到 IPv6 地址: %s", ipstr);
            memcpy(ship_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    /* Make sure we found at least an IPv4 address */
    if (!ship_ip4) {
        ERR_LOG("无法获取到IPv4地址!");
        return -1;
    }

    if (ship_ip4 != ip)
        ip = ship_ip4;

    /* Call the appropriate function */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_redirect(c, ip, port);

        case CLIENT_VERSION_BB:
            return send_bb_redirect(c, ip, port);
    }

    return -1;
}

#ifdef PSOCN_ENABLE_IPV6
/* Send a redirect packet (IPv6) to the given client. */
static int send_redirect6_dc(ship_client_t *c, const uint8_t ip[16],
                             uint16_t port) {
    uint8_t *sendbuf = get_sendbuf();
    dc_redirect6_pkt *pkt = (dc_redirect6_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Wipe the packet */
    memset(pkt, 0, DC_REDIRECT6_LENGTH);

    /* 填充数据头 */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = REDIRECT_TYPE;
        pkt->hdr.dc.pkt_len = LE16(DC_REDIRECT6_LENGTH);
        pkt->hdr.dc.flags = 6;
    }
    else {
        pkt->hdr.pc.pkt_type = REDIRECT_TYPE;
        pkt->hdr.pc.pkt_len = LE16(DC_REDIRECT6_LENGTH);
        pkt->hdr.pc.flags = 6;
    }

    /* Fill in the IP and port */
    memcpy(pkt->ip_addr, ip, 16);
    pkt->port = LE16(port);

    /* 将数据包发送出去 */
    return crypt_send(c, DC_REDIRECT6_LENGTH, sendbuf);
}

static int send_redirect6_bb(ship_client_t *c, const uint8_t ip[16],
                             uint16_t port) {
    uint8_t *sendbuf = get_sendbuf();
    bb_redirect6_pkt *pkt = (bb_redirect6_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Wipe the packet */
    memset(pkt, 0, BB_REDIRECT6_LENGTH);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(REDIRECT_TYPE);
    pkt->hdr.pkt_len = LE16(BB_REDIRECT6_LENGTH);
    pkt->hdr.flags = LE32(6);

    /* Fill in the IP and port */
    memcpy(pkt->ip_addr, ip, 16);
    pkt->port = LE16(port);

    /* 将数据包发送出去 */
    return crypt_send(c, BB_REDIRECT6_LENGTH, sendbuf);
}

int send_redirect6(ship_client_t *c, char* host6, const uint8_t ip[16], uint16_t port) {

    struct addrinfo hints;
    struct addrinfo* server, * j;
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in6* addr6;

    /* If we don't have a separate IPv6 host set, we're done. */
    if (!host6) {
        return 0;
    }

    /* Now try with IPv6 only */
    memset(ship_ip6, 0, 16);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host6, "9000", &hints, &server)) {
        ERR_LOG("无效舰船地址 (v6): %s", host6);
        //return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //SHIPS_LOG("    获取到 IPv6 地址: %s", ipstr);
            memcpy(ship_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    if (!ship_ip6[0]) {
        ERR_LOG("无法获取到IPv6地址 (但设置了IPv6域名)!");
        return -1;
    }

    if (ship_ip6 != ip)
        ip = ship_ip6;

    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_redirect6_dc(c, ip, port);

        case CLIENT_VERSION_BB:
            return send_redirect6_bb(c, ip, port);
    }

    return -1;
}
#endif

/* Send a timestamp packet to the given client. */
static int send_dc_timestamp(ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    dc_timestamp_pkt *pkt = (dc_timestamp_pkt *)sendbuf;
    //struct timeval rawtime;
    //struct tm cooked;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Wipe the packet */
    memset(pkt, 0, DC_TIMESTAMP_LENGTH);

    /* 填充数据头 */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = TIMESTAMP_TYPE;
        pkt->hdr.dc.pkt_len = LE16(DC_TIMESTAMP_LENGTH);
    }
    else {
        pkt->hdr.pc.pkt_type = TIMESTAMP_TYPE;
        pkt->hdr.pc.pkt_len = LE16(DC_TIMESTAMP_LENGTH);
    }

    /* Get the timestamp */
    //gettimeofday(&rawtime, NULL);

    /* Get UTC */
    //gmtime_r(&rawtime.tv_sec, &cooked);

    /* Fill in the timestamp */
    sprintf(pkt->timestamp, "%u:%02u:%02u: %02u:%02u:%02u.%03u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
            rawtime.wMilliseconds);

    /* 将数据包发送出去 */
    return crypt_send(c, DC_TIMESTAMP_LENGTH, sendbuf);
}

int send_timestamp(ship_client_t *c) {
    /* Call the appropriate function */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_timestamp(c);
    }

    return -1;
}

/* Send the list of blocks to the client. */
static int send_dc_block_list(ship_client_t *c, ship_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    dc_block_list_pkt *pkt = (dc_block_list_pkt *)sendbuf;
    int len = 0x20, entries = 1;
    uint32_t len2 = 0x1C, len3 = 0x10;
    uint32_t i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the base packet */
    memset(pkt, 0, sizeof(dc_block_list_pkt));

    /* Fill in the ship name entry */
    memset(&pkt->entries[entries], 0, len2);
    pkt->entries[entries].menu_id = LE32(pso_block_list_menu_last[entries]->menu_id);
    pkt->entries[entries].item_id = LE32(pso_block_list_menu_last[entries]->item_id);
    pkt->entries[entries].flags = LE16(pso_block_list_menu_last[entries]->flag);
    istrncpy(ic_gbk_to_8859, (char*)pkt->entries[entries].name, s->cfg->name, len3);
    ++entries;

    /* Add each block to the list. */
    for(i = 1; i <= s->cfg->blocks; ++i) {
        if(s->blocks[i - 1] && s->blocks[i - 1]->run) {
            /* Clear out the block information */
            memset(&pkt->entries[entries], 0, len2);

            /* Fill in what we have */
            pkt->entries[entries].menu_id = LE32(MENU_ID_BLOCK);
            pkt->entries[entries].item_id = LE32(i);
            pkt->entries[entries].flags = LE16(0x0000);

            /* Create the name string */
            char tmp[17];
            sprintf_s(tmp, sizeof(tmp), "选择舰仓%01X%01X", (i / 10), (i % 10));

            /* Create the name string */
            istrncpy(ic_gbk_to_8859, (char*)pkt->entries[entries].name, tmp,
                len3);

            len += len2;
            ++entries;
        }
    }

    /* 填充菜单实例 */
    for (i = 1; i < _countof(pso_block_list_menu_last); ++i) {
        memset(&pkt->entries[entries], 0, len2);
        pkt->entries[entries].menu_id = LE32(pso_block_list_menu_last[i]->menu_id);
        pkt->entries[entries].item_id = LE32(pso_block_list_menu_last[i]->item_id);
        pkt->entries[entries].flags = LE16(pso_block_list_menu_last[i]->flag);
        istrncpy(ic_gbk_to_8859, (char*)pkt->entries[entries].name, pso_block_list_menu_last[i]->name, len3);
        ++entries;
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = entries - 1;

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_block_list(ship_client_t *c, ship_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    pc_block_list_pkt *pkt = (pc_block_list_pkt *)sendbuf;
    int len = 0x30, entries = 0;
    uint32_t len2 = 0x2C, len3 = 0x20;
    uint32_t i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the base packet */
    memset(pkt, 0, sizeof(pc_block_list_pkt));

    /* Fill in the ship name entry */
    memset(&pkt->entries[entries], 0, len2);
    pkt->entries[entries].menu_id = LE32(pso_block_list_menu_last[entries]->menu_id);
    pkt->entries[entries].item_id = LE32(pso_block_list_menu_last[entries]->item_id);
    pkt->entries[entries].flags = LE16(pso_block_list_menu_last[entries]->flag);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, s->cfg->name, len3);
    ++entries;

    /* Add each block to the list. */
    for(i = 1; i <= s->cfg->blocks; ++i) {
        if(s->blocks[i - 1] && s->blocks[i - 1]->run) {
            /* Clear out the block information */
            memset(&pkt->entries[entries], 0, len2);

            /* Fill in what we have */
            pkt->entries[entries].menu_id = LE32(MENU_ID_BLOCK);
            pkt->entries[entries].item_id = LE32(i);
            pkt->entries[entries].flags = LE16(0x0000);

            /* Create the name string */
            char tmp[17];
            sprintf_s(tmp, sizeof(tmp), "选择舰仓%01X%01X", (i / 10), (i % 10));

            /* Create the name string */
            istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, tmp,
                len3);

            len += len2;
            ++entries;
        }
    }
    
    /* 填充菜单实例 */
    for (i = 1; i < _countof(pso_block_list_menu_last); ++i) {
        memset(&pkt->entries[entries], 0, len2);
        pkt->entries[entries].menu_id = LE32(pso_block_list_menu_last[i]->menu_id);
        pkt->entries[entries].item_id = LE32(pso_block_list_menu_last[i]->item_id);
        pkt->entries[entries].flags = LE16(pso_block_list_menu_last[i]->flag);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, pso_block_list_menu_last[i]->name, len3);
        ++entries;
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = entries - 1;

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_block_list(ship_client_t *c, ship_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    bb_block_list_pkt *pkt = (bb_block_list_pkt *)sendbuf;
    int len = 0x34, entries = 0;
    uint32_t len2 = 0x2C, len3 = 0x20;
    uint32_t i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the base packet */
    memset(pkt, 0, sizeof(bb_block_list_pkt));

    /* Fill in the ship name entry */
    memset(&pkt->entries[entries], 0, len2);
    pkt->entries[entries].menu_id = LE32(pso_block_list_menu_last[entries]->menu_id);
    pkt->entries[entries].item_id = LE32(pso_block_list_menu_last[entries]->item_id);
    pkt->entries[entries].flags = LE16(pso_block_list_menu_last[entries]->flag);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, s->cfg->name, len3);
    ++entries;

    /* Add each block to the list. */
    for(i = 1; i <= s->cfg->blocks; ++i) {
        if(s->blocks[i - 1] && s->blocks[i - 1]->run) {
            /* Clear out the block information */
            memset(&pkt->entries[entries], 0, len2);

            /* Fill in what we have */
            pkt->entries[entries].menu_id = LE32(MENU_ID_BLOCK);
            pkt->entries[entries].item_id = LE32(i);
            pkt->entries[entries].flags = LE16(0x0000);
            char tmp[17];
            sprintf_s(tmp, sizeof(tmp), "选择舰仓%01X%01X", (i / 10), (i % 10));

            /* Create the name string */
            istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, tmp,
                len3);
            /* We don't need to terminate this string because we did the memset
               above to clear the entry to all zero bytes. */
            len += len2;
            ++entries;
        }
    }

    /* 填充菜单实例 */
    for (i = 1; i < _countof(pso_block_list_menu_last); ++i) {
        memset(&pkt->entries[entries], 0, 0x2C);
        pkt->entries[entries].menu_id = LE32(pso_block_list_menu_last[i]->menu_id);
        pkt->entries[entries].item_id = LE32(pso_block_list_menu_last[i]->item_id);
        pkt->entries[entries].flags = LE16(pso_block_list_menu_last[i]->flag);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, pso_block_list_menu_last[i]->name, len3);
        ++entries;
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(BLOCK_LIST_TYPE);
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = LE32(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

int send_block_list(ship_client_t *c, ship_t *s) {
    /* Call the appropriate function */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_block_list(c, s);

        case CLIENT_VERSION_PC:
            return send_pc_block_list(c, s);

        case CLIENT_VERSION_BB:
            return send_bb_block_list(c, s);
    }

    return -1;
}

/* Send a block/ship information reply packet to the client. */
static int send_dc_info_reply(ship_client_t *c, const char *msg) {
    uint8_t *sendbuf = get_sendbuf();
    dc_info_reply_pkt *pkt = (dc_info_reply_pkt *)sendbuf;
    iconv_t ic;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        if(msg[1] == 'J') {
            ic = ic_gbk_to_sjis;
        }
        else {
            ic = ic_gbk_to_8859;
        }
    }
    else {
        ic = ic_gbk_to_utf16;
    }

    /* Convert the message to the appropriate encoding. */
    in = strlen(msg) + 1;
    out = 65524;
    inptr = (char *)msg;
    outptr = pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    out = 65524 - out + 0x0C;

    /* Fill in the oddities of the packet. */
    pkt->odd[0] = LE32(0x00200000);
    pkt->odd[1] = LE32(0x00200020);

    /* Pad to a length that's at divisible by 4. */
    while(out & 0x03) {
        sendbuf[out++] = 0;
    }

    /* 填充数据头 */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = INFO_REPLY_TYPE;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16((uint16_t)out);
    }
    else {
        pkt->hdr.pc.pkt_type = INFO_REPLY_TYPE;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16((uint16_t)out);
    }

    /* 将数据包发送出去 */
    return crypt_send(c, out, sendbuf);
}

static int send_bb_info_reply(ship_client_t *c, const char *msg) {
    uint8_t *sendbuf = get_sendbuf();
    bb_info_reply_pkt *pkt = (bb_info_reply_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Convert the message to the appropriate encoding. */
    in = strlen(msg) + 1;
    out = 65520;
    inptr = (char *)msg;
    outptr = (char *)pkt->msg;
    iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    out = 65520 - out + 0x10;

    /* Fill in the unused fields of the packet. */
    pkt->unused[0] = pkt->unused[1] = 0;

    /* Pad to a length that is divisible by 8. */
    while(out & 0x07) {
        sendbuf[out++] = 0;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(INFO_REPLY_TYPE);
    pkt->hdr.flags = 0;
    pkt->hdr.pkt_len = LE16((uint16_t)out);

    /* 将数据包发送出去 */
    return crypt_send(c, out, sendbuf);
}

int send_info_reply(ship_client_t *c, const char *msg) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_info_reply(c, msg);

        case CLIENT_VERSION_BB:
            return send_bb_info_reply(c, msg);
    }

    return -1;
}

/* Send a simple (header-only) packet to the client */
static int send_dc_simple(ship_client_t *c, int type, int flags) {
    uint8_t *sendbuf = get_sendbuf();
    dc_pkt_hdr_t *pkt = (dc_pkt_hdr_t *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* 填充数据头 */
    pkt->pkt_type = (uint8_t)type;
    pkt->flags = (uint8_t)flags;
    pkt->pkt_len = LE16(4);

    /* 将数据包发送出去 */
    return crypt_send(c, 4, sendbuf);
}

static int send_pc_simple(ship_client_t *c, int type, int flags) {
    uint8_t *sendbuf = get_sendbuf();
    pc_pkt_hdr_t *pkt = (pc_pkt_hdr_t *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* 填充数据头 */
    pkt->pkt_type = (uint8_t)type;
    pkt->flags = (uint8_t)flags;
    pkt->pkt_len = LE16(4);

    /* 将数据包发送出去 */
    return crypt_send(c, 4, sendbuf);
}

static int send_bb_simple(ship_client_t *c, int type, int flags) {
    uint8_t *sendbuf = get_sendbuf();
    bb_pkt_hdr_t *pkt = (bb_pkt_hdr_t *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* 填充数据头 */
    pkt->pkt_type = LE16(((uint16_t)type));
    pkt->flags = LE32(((uint32_t)flags));
    pkt->pkt_len = LE16(8);

    /* 将数据包发送出去 */
    return crypt_send(c, 8, sendbuf);
}

int send_simple(ship_client_t *c, int type, int flags) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_simple(c, type, flags);

        case CLIENT_VERSION_PC:
            return send_pc_simple(c, type, flags);

        case CLIENT_VERSION_BB:
            return send_bb_simple(c, type, flags);
    }

    return -1;
}

/* Send the lobby list packet to the client. */
static int send_dc_lobby_list(ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    dc_lobby_list_pkt *pkt = (dc_lobby_list_pkt *)sendbuf;
    uint32_t i, max = 15;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* 填充数据头 */
    if(c->version == CLIENT_VERSION_DCV1) {
        pkt->hdr.dc.pkt_type = LOBBY_LIST_TYPE;
        pkt->hdr.dc.flags = 0x0A;                               /* 10 lobbies */
        pkt->hdr.dc.pkt_len = LE16(DC_LOBBY_LIST_LENGTH - 60);
        max = 10;
    }
    else if(c->version == CLIENT_VERSION_DCV2 ||
            c->version == CLIENT_VERSION_GC ||
            c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = LOBBY_LIST_TYPE;
        pkt->hdr.dc.flags = 0x0F;                               /* 15 lobbies */
        pkt->hdr.dc.pkt_len = LE16(DC_LOBBY_LIST_LENGTH);
    }
    else if(c->version == CLIENT_VERSION_EP3) {
        pkt->hdr.dc.pkt_type = LOBBY_LIST_TYPE;
        pkt->hdr.dc.flags = 0x14;                               /* 20 lobbies */
        pkt->hdr.dc.pkt_len = LE16(EP3_LOBBY_LIST_LENGTH);
        max = 20;
    }
    else {
        pkt->hdr.pc.pkt_type = LOBBY_LIST_TYPE;
        pkt->hdr.pc.flags = 0x0F;                               /* 15 lobbies */
        pkt->hdr.pc.pkt_len = LE16(DC_LOBBY_LIST_LENGTH);
    }

    /* Fill in the lobbies. */
    for(i = 0; i < max; ++i) {
        pkt->entries[i].menu_id = MENU_ID_LOBBY;
        pkt->entries[i].item_id = LE32((i + 1));
        pkt->entries[i].padding = 0;
    }

    /* There's padding at the end -- enough for one more lobby. */
    pkt->entries[max].menu_id = 0;
    pkt->entries[max].item_id = 0;
    pkt->entries[max].padding = 0;

    if(c->version == CLIENT_VERSION_DCV1) {
        return crypt_send(c, DC_LOBBY_LIST_LENGTH - 60, sendbuf);
    }
    else if(c->version != CLIENT_VERSION_EP3) {
        return crypt_send(c, DC_LOBBY_LIST_LENGTH, sendbuf);
    }
    else {
        return crypt_send(c, EP3_LOBBY_LIST_LENGTH, sendbuf);
    }
}

static int send_bb_lobby_list(ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    bb_lobby_list_pkt *pkt = (bb_lobby_list_pkt *)sendbuf;
    uint32_t i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(LOBBY_LIST_TYPE);
    pkt->hdr.flags = LE32(0x0F);
    pkt->hdr.pkt_len = LE16(BB_LOBBY_LIST_LENGTH);

    /* Fill in the lobbies. */
    for(i = 0; i < 15; ++i) {
        pkt->entries[i].menu_id = MENU_ID_LOBBY;
        pkt->entries[i].item_id = LE32((i + 1));
        pkt->entries[i].padding = 0;
    }

    /* There's padding at the end -- enough for one more lobby. */
    pkt->entries[15].menu_id = 0;
    pkt->entries[15].item_id = 0;
    pkt->entries[15].padding = 0;

    return crypt_send(c, BB_LOBBY_LIST_LENGTH, sendbuf);
}

int send_lobby_list(ship_client_t *c) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_lobby_list(c);

        case CLIENT_VERSION_BB:
            return send_bb_lobby_list(c);
    }

    return -1;
}

/* Send the packet to join a lobby to the client. */
static int send_dcnte_lobby_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    dcnte_lobby_join_pkt *pkt = (dcnte_lobby_join_pkt *)sendbuf;
    int i, pls = 0;
    uint16_t pkt_size = 8;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(dcnte_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = LOBBY_JOIN_TYPE;
    pkt->leader_id = l->leader_id;

    for(i = 0; i < l->max_clients; ++i) {
        /* Skip blank clients. */
        if(l->clients[i] == NULL)
            continue;

        /* If this is the client we're sending to, mark their client id. */
        if(l->clients[i] == c)
            pkt->client_id = (uint8_t)i;

        /* Copy the player's data into the packet. */
        pkt->entries[pls].hdr.player_tag = LE32(0x00010000);
        pkt->entries[pls].hdr.guildcard = LE32(l->clients[i]->guildcard);
        pkt->entries[pls].hdr.ip_addr = 0xFFFFFFFF;
        pkt->entries[pls].hdr.client_id = LE32(i);

        /* If its a Blue Burst client, iconv it. */
        if(l->clients[i]->version == CLIENT_VERSION_BB) {
            istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->entries[pls].hdr.name,
                           &l->clients[i]->pl->bb.character.name, 16, BB_CHARACTER_NAME_LENGTH);
        }
        else {
            memcpy(pkt->entries[pls].hdr.name, l->clients[i]->pl->v1.character.dress_data.guildcard_string, 16);
        }

        /* Do any character data conversion we have to deal with... */
        make_disp_data(l->clients[i], c, &pkt->entries[pls].data);

        /* Sigh... I should narrow the problem down a bit more, but this works
           for the time being... */
        if(!(l->clients[i]->flags & CLIENT_FLAG_IS_NTE) ||
           l->clients[i]->version != CLIENT_VERSION_DCV1)
            memset(&pkt->entries[pls].data.inv, 0, sizeof(inventory_t));

        ++pls;
        pkt_size += 1084;
    }

    /* Fill in the rest of it. */
    pkt->hdr.flags = (uint8_t)pls;
    pkt->hdr.pkt_len = LE16(pkt_size);

    /* 加密并发送 */
    return crypt_send(c, pkt_size, sendbuf);
}

static int send_dc_lobby_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    dc_lobby_join_pkt *pkt = (dc_lobby_join_pkt *)sendbuf;
    int i, pls = 0;
    uint16_t pkt_size = 0x10;
    uint8_t event = l->event;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(dc_lobby_join_pkt));

    /* Don't send lobby event codes to the Dreamcast version. */
    if(c->version < CLIENT_VERSION_GC)
        event = 0;

    /* Fill in the basics. */
    pkt->hdr.pkt_type = LOBBY_JOIN_TYPE;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 0;
    pkt->lobby_num = l->lobby_id - 1;
    pkt->block_num = LE16(l->block->b);
    pkt->event = LE16(event);

    for(i = 0; i < l->max_clients; ++i) {
        /* Skip blank clients. */
        if(l->clients[i] == NULL) {
            continue;
        }

        /* If this is the client we're sending to, mark their client id. */
        else if(l->clients[i] == c) {
            pkt->client_id = (uint8_t)i;
        }

        /* Copy the player's data into the packet. */
        pkt->entries[pls].hdr.player_tag = LE32(0x00010000);
        pkt->entries[pls].hdr.guildcard = LE32(l->clients[i]->guildcard);
        pkt->entries[pls].hdr.ip_addr = 0xFFFFFFFF;
        pkt->entries[pls].hdr.client_id = LE32(i);

        /* If its a Blue Burst client, iconv it. */
        if(l->clients[i]->version == CLIENT_VERSION_BB) {
            istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->entries[pls].hdr.name,
                           &l->clients[i]->pl->bb.character.name, 16, BB_CHARACTER_NAME_LENGTH);
        }
        else {
            memcpy(pkt->entries[pls].hdr.name, l->clients[i]->pl->v1.character.dress_data.guildcard_string, 16);
        }

        /* Do any character data conversion that we have to deal with... */
        make_disp_data(l->clients[i], c, &pkt->entries[pls].data);

        ++pls;
        pkt_size += 1084;
    }

    /* Fill in the rest of it. */
    pkt->hdr.flags = (uint8_t)pls;
    pkt->hdr.pkt_len = LE16(pkt_size);

    /* 加密并发送 */
    return crypt_send(c, pkt_size, sendbuf);
}

static int send_pc_lobby_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    pc_lobby_join_pkt *pkt = (pc_lobby_join_pkt *)sendbuf;
    int i, pls = 0;
    uint16_t pkt_size = 0x10;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(pc_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = LOBBY_JOIN_TYPE;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->lobby_num = l->lobby_id - 1;
    pkt->block_num = LE16(l->block->b);

    for(i = 0; i < l->max_clients; ++i) {
        /* Skip blank clients. */
        if(l->clients[i] == NULL) {
            continue;
        }
        /* If this is the client we're sending to, mark their client id. */
        else if(l->clients[i] == c) {
            pkt->client_id = (uint8_t)i;
        }

        /* Copy the player's data into the packet. */
        pkt->entries[pls].hdr.player_tag = LE32(0x00010000);
        pkt->entries[pls].hdr.guildcard = LE32(l->clients[i]->guildcard);
        pkt->entries[pls].hdr.ip_addr = 0xFFFFFFFF;
        pkt->entries[pls].hdr.client_id = LE32(i);

        /* Convert the name to UTF-16. */
        if(l->clients[i]->version == CLIENT_VERSION_BB) {
            memcpy(pkt->entries[pls].hdr.name,
                   &l->clients[i]->pl->bb.character.name.char_name[0], BB_CHARACTER_CHAR_NAME_WLENGTH);
            pkt->entries[pls].hdr.name[14] = 0;
            pkt->entries[pls].hdr.name[15] = 0;
        }
        else {
            istrncpy(ic_8859_to_utf16, (char *)pkt->entries[pls].hdr.name,
                     l->clients[i]->pl->v1.character.dress_data.guildcard_string, 32);
        }

        /* Do any character data conversion that we have to deal with... */
        make_disp_data(l->clients[i], c, &pkt->entries[pls].data);

        ++pls;
        pkt_size += 1100;
    }

    /* Fill in the rest of it. */
    pkt->hdr.flags = (uint8_t)pls;
    pkt->hdr.pkt_len = LE16(pkt_size);

    /* 加密并发送 */
    return crypt_send(c, pkt_size, sendbuf);
}

static int send_xbox_lobby_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    xb_lobby_join_pkt *pkt = (xb_lobby_join_pkt *)sendbuf;
    int i, pls = 0;
    uint16_t pkt_size = 0x28;
    uint8_t event = l->event;
    ship_client_t *cl;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(xb_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = LOBBY_JOIN_TYPE;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->lobby_num = l->lobby_id - 1;
    pkt->block_num = LE16(l->block->b);
    pkt->event = LE16(event);

    /* XXXX: Don't really know what the format of this is... Maybe a KEK? */
    memset(pkt->unk, 0x01, 0x18);

    for(i = 0; i < l->max_clients; ++i) {
        /* Skip blank clients. */
        if(l->clients[i] == NULL) {
            continue;
        }
        /* If this is the client we're sending to, mark their client id. */
        else if(l->clients[i] == c) {
            pkt->client_id = (uint8_t)i;
        }

        cl = l->clients[i];

        /* Copy the player's data into the packet. */
        pkt->entries[pls].hdr.player_tag = LE32(0x00010000);
        pkt->entries[pls].hdr.guildcard = LE32(cl->guildcard);

        if(cl->version == CLIENT_VERSION_XBOX) {
            memcpy(&pkt->entries[pls].hdr.xbox_ip, cl->xbl_ip,
                   sizeof(xbox_ip_t));
        } else {
            /* TODO: Not sure how we should do this for non-xb clients... */
            memset(&pkt->entries[pls].hdr.xbox_ip, 0, sizeof(xbox_ip_t));
            pkt->entries[pls].hdr.xbox_ip.lan_ip = 0x12345678;
            pkt->entries[pls].hdr.xbox_ip.wan_ip = 0x87654321;
            pkt->entries[pls].hdr.xbox_ip.port = 1234;
            memset(&pkt->entries[pls].hdr.xbox_ip.mac_addr, 0x12, 6);
            pkt->entries[pls].hdr.xbox_ip.sg_addr = 0x90807060;
            pkt->entries[pls].hdr.xbox_ip.sg_session_id = 0x12345678;
            pkt->entries[pls].hdr.xbox_ip.xbox_account_id =
                +(0xAE00000000000000ULL) | LE32(cl->guildcard);
        }

        /* TODO: I have no idea what these three things are. */
        pkt->entries[pls].hdr.d1 = 0x01010101;
        pkt->entries[pls].hdr.d2 = 0x01010101;
        pkt->entries[pls].hdr.d3 = LE32(1);
        pkt->entries[pls].hdr.client_id = LE32(i);

        /* If its a Blue Burst client, iconv it. */
        if(cl->version == CLIENT_VERSION_BB) {
            istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->entries[pls].hdr.name,
                           &cl->pl->bb.character.name, 16, BB_CHARACTER_NAME_LENGTH);
        }
        else {
            memcpy(pkt->entries[pls].hdr.name, cl->pl->v1.character.dress_data.guildcard_string, 16);
        }

        /* Do any character data conversion that we have to deal with... */
        make_disp_data(cl, c, &pkt->entries[pls].data);

        ++pls;
        pkt_size += 1128;
    }

    /* Fill in the rest of it. */
    pkt->hdr.flags = (uint8_t)pls;
    pkt->hdr.pkt_len = LE16(pkt_size);

    /* 加密并发送 */
    return crypt_send(c, pkt_size, sendbuf);
}

static int send_bb_lobby_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    bb_lobby_join_pkt *pkt = (bb_lobby_join_pkt *)sendbuf;
    int i, pls = 0;
    uint16_t pkt_size = 0x14;

    /*ERR_LOG(
        "send_bb_lobby_join端口 %d 版本识别 = %d",
        c->sock, c->version);*/

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(bb_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = LE16(LOBBY_JOIN_TYPE);
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 0x01;
    pkt->lobby_num = l->lobby_id - 1;
    pkt->block_num = l->block->b;
    pkt->unknown_a1 = 0;
    pkt->event = l->event;
    pkt->unknown_a2 = 0;

    for(i = 0; i < l->max_clients; ++i) {
        /* Skip blank clients. */
        if(l->clients[i] == NULL) {
            continue;
        }
        /* If this is the client we're sending to, mark their client id. */
        else if(l->clients[i] == c) {
            pkt->client_id = (uint8_t)i;
        }

        /* Copy the player's data into the packet. */
        memset(&pkt->entries[pls].hdr, 0, sizeof(bb_player_hdr_t));
        pkt->entries[pls].hdr.player_tag = LE32(0x00010000);
        pkt->entries[pls].hdr.guildcard = LE32(l->clients[i]->guildcard);
        pkt->entries[pls].hdr.client_id = LE32(i);

        /* Convert the name to UTF-16. */
        if(l->clients[i]->version == CLIENT_VERSION_BB) {
            memcpy(pkt->entries[pls].hdr.name,
                   &l->clients[i]->pl->bb.character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
        }
        else {
            istrncpy(ic_8859_to_utf16, (char *)pkt->entries[pls].hdr.name,
                     l->clients[i]->pl->v1.character.dress_data.guildcard_string, 32);
        }

        make_disp_data(l->clients[i], c, &pkt->entries[pls].inv);

        ++pls;
        pkt_size += sizeof(bb_player_hdr_t) + sizeof(inventory_t) +
            sizeof(psocn_bb_char_t);
    }

    /* Fill in the rest of it. */
    pkt->hdr.flags = LE32(pls);
    pkt->hdr.pkt_len = LE16(pkt_size);

    /*ERR_LOG(
        "send_bb_lobby_join端口 %d 版本识别 = %d",
        c->sock, c->version);*/

    /* 加密并发送 */
    return crypt_send(c, pkt_size, sendbuf);
}

int send_lobby_join(ship_client_t *c, lobby_t *l) {

    /*ERR_LOG(
        "send_lobby_join端口 %d 版本识别 = %d",
        c->sock, c->version);*/

    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
            if(!(c->flags & CLIENT_FLAG_IS_NTE)) {
                if(send_dc_lobby_join(c, l))
                    return -1;
            }
            else {
                if(send_dcnte_lobby_join(c, l))
                    return -1;
            }
            break;

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
            if(send_dc_lobby_join(c, l))
                return -1;

            if(send_lobby_c_rank(c, l))
                return -1;

            if(send_dc_lobby_arrows(l, c))
                return -1;
            break;

        case CLIENT_VERSION_PC:
            if(send_pc_lobby_join(c, l))
                return -1;

            if(send_lobby_c_rank(c, l))
                return -1;

            if(send_dc_lobby_arrows(l, c))
                return -1;
            break;

        case CLIENT_VERSION_XBOX:
            if(send_xbox_lobby_join(c, l))
                return -1;

            if(send_lobby_c_rank(c, l))
                return -1;

            if(send_dc_lobby_arrows(l, c))
                return -1;
            break;

        case CLIENT_VERSION_BB:
            if(send_bb_lobby_join(c, l))
                return -1;

//#if 0
            if(send_lobby_c_rank(c, l))
                return -1;
//#endif

            if(send_bb_lobby_arrows(l, c))
                return -1;
            break;

        default:
            return -1;
    }

    return 0;
}

/* Send a prepared packet to the given client. */
int send_pkt_dc(ship_client_t *c, const dc_pkt_hdr_t *pkt) {
    uint8_t *sendbuf = get_sendbuf();
    int len = (int)LE16(pkt->pkt_len);

    //DBG_LOG("发送DC数据至客户端 ：版本 = %d GC = %u", c->version, c->guildcard);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Adjust the packet for whatever version */
    if(c->version == CLIENT_VERSION_PC) {
        pc_pkt_hdr_t *hdr = (pc_pkt_hdr_t *)sendbuf;

        hdr->pkt_len = pkt->pkt_len;
        hdr->flags = pkt->flags;
        hdr->pkt_type = pkt->pkt_type;

        memcpy(sendbuf + 4, ((uint8_t *)pkt) + 4, len - 4);
    }
    else if(c->version == CLIENT_VERSION_BB) {
        bb_pkt_hdr_t *hdr = (bb_pkt_hdr_t *)sendbuf;

        hdr->pkt_len = LE16((len + 4));
        hdr->flags = LE32(pkt->flags);
        hdr->pkt_type = LE16(pkt->pkt_type);

        memcpy(sendbuf + 8, ((uint8_t *)pkt) + 4, len - 4);
        len += 4;
    }
    else {
        memcpy(sendbuf, pkt, len);
    }

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

/* Send a prepared packet to the given client. */
int send_pkt_bb(ship_client_t *c, const bb_pkt_hdr_t *pkt) {
    uint8_t *sendbuf = get_sendbuf();
    int len = (int)LE16(pkt->pkt_len);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Figure out what to do based on version... */
    if(c->version == CLIENT_VERSION_BB) {
        memcpy(sendbuf, pkt, len);
    }
    else if(c->version == CLIENT_VERSION_PC) {
        pc_pkt_hdr_t *hdr = (pc_pkt_hdr_t *)sendbuf;

        hdr->pkt_len = LE16(len - 4);
        hdr->flags = (uint8_t)pkt->flags;
        hdr->pkt_type = (uint8_t)pkt->pkt_type;

        memcpy(sendbuf + 4, ((uint8_t *)pkt) + 8, len - 8);
        len -= 4;
    }
    else {
        dc_pkt_hdr_t *hdr = (dc_pkt_hdr_t *)sendbuf;

        hdr->pkt_len = LE16(len - 4);
        hdr->flags = (uint8_t)pkt->flags;
        hdr->pkt_type = (uint8_t)pkt->pkt_type;

        memcpy(sendbuf + 4, ((uint8_t *)pkt) + 8, len - 8);
        len -= 4;
    }

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

/* 发送数据包至特定的大厅. */
int send_lobby_pkt(lobby_t* l, ship_client_t* nosend, const uint8_t* pkt,
    int check) {
    int i;
    ship_client_t* c2;

    if (!l)
        return 0;

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients_slot[i] && l->clients[i] != nosend) {
            c2 = l->clients[i];
            pthread_mutex_lock(&c2->mutex);

            /* Call the appropriate function. */
            switch (c2->version) {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
            case CLIENT_VERSION_GC:
            case CLIENT_VERSION_EP3:
            case CLIENT_VERSION_XBOX:
            case CLIENT_VERSION_PC:
                /**/
                if (!check) {
                    DBG_LOG("发送的客户端版本为 %d", c2->version);
                    send_pkt_dc(c2, (dc_pkt_hdr_t*)pkt);
                }
                break;

            case CLIENT_VERSION_BB:
                /**/
                send_pkt_bb(c2, (bb_pkt_hdr_t*)pkt);
                break;
            }

            pthread_mutex_unlock(&c2->mutex);
        }
    }

    return 0;
}

/* Send a packet to all clients in the lobby when a new player joins. */
static int send_dcnte_lobby_add_player(lobby_t *l, ship_client_t *c,
                                       ship_client_t *nc) {
    uint8_t *sendbuf = get_sendbuf();
    dcnte_lobby_join_pkt *pkt = (dcnte_lobby_join_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(dcnte_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = (l->type == LOBBY_TYPE_LOBBY) ?
        LOBBY_ADD_PLAYER_TYPE : GAME_ADD_PLAYER_TYPE;
    pkt->hdr.flags = 1;
    pkt->hdr.pkt_len = LE16(0x0444);
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;

    /* Copy the player's data into the packet. */
    pkt->entries[0].hdr.player_tag = LE32(0x00010000);
    pkt->entries[0].hdr.guildcard = LE32(nc->guildcard);
    pkt->entries[0].hdr.ip_addr = 0xFFFFFFFF;
    pkt->entries[0].hdr.client_id = LE32(nc->client_id);

    /* If its a Blue Burst client, iconv it. */
    if(nc->version == CLIENT_VERSION_BB) {
        istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->entries[0].hdr.name,
                       &nc->pl->bb.character.name.char_name[0], 16, BB_CHARACTER_CHAR_NAME_WLENGTH);
    }
    else {
        memcpy(pkt->entries[0].hdr.name, nc->pl->v1.character.dress_data.guildcard_string, 16);
    }

    /* Do any character data conversion that we have to deal with... */
    make_disp_data(nc, c, &pkt->entries[0].data);

    /* Sigh... I should narrow the problem down a bit more, but this works for
       the time being... */
    if(!(nc->flags & CLIENT_FLAG_IS_NTE) ||
       nc->version != CLIENT_VERSION_DCV1)
        memset(&pkt->entries[0].data.inv, 0, sizeof(inventory_t));

    /* 加密并发送 */
    return crypt_send(c, 0x0444, sendbuf);
}

static int send_dc_lobby_add_player(lobby_t *l, ship_client_t *c,
                                    ship_client_t *nc) {
    uint8_t *sendbuf = get_sendbuf();
    dc_lobby_join_pkt *pkt = (dc_lobby_join_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(dc_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = (l->type == LOBBY_TYPE_LOBBY) ?
        LOBBY_ADD_PLAYER_TYPE : GAME_ADD_PLAYER_TYPE;
    pkt->hdr.flags = 1;
    pkt->hdr.pkt_len = LE16(0x044C);
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->lobby_num = (l->type == LOBBY_TYPE_LOBBY) ? l->lobby_id - 1 : 0xFF;

    if(l->type == LOBBY_TYPE_LOBBY) {
        pkt->block_num = LE16(l->block->b);
    }
    else {
        pkt->block_num = LE16(0x0001);
        pkt->event = LE16(0x0001);
    }

    /* Copy the player's data into the packet. */
    pkt->entries[0].hdr.player_tag = LE32(0x00010000);
    pkt->entries[0].hdr.guildcard = LE32(nc->guildcard);
    pkt->entries[0].hdr.ip_addr = 0xFFFFFFFF;
    pkt->entries[0].hdr.client_id = LE32(nc->client_id);

    /* If its a Blue Burst client, iconv it. */
    if(nc->version == CLIENT_VERSION_BB) {
        istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->entries[0].hdr.name,
                       &nc->pl->bb.character.name.char_name[0], 16, BB_CHARACTER_CHAR_NAME_WLENGTH);
    }
    else {
        memcpy(pkt->entries[0].hdr.name, nc->pl->v1.character.dress_data.guildcard_string, 16);
    }

    /* Do any character data conversion that we have to deal with... */
    make_disp_data(nc, c, &pkt->entries[0].data);
    
    /* 加密并发送 */
    return crypt_send(c, 0x044C, sendbuf);
}

static int send_pc_lobby_add_player(lobby_t *l, ship_client_t *c,
                                    ship_client_t *nc) {
    uint8_t *sendbuf = get_sendbuf();
    pc_lobby_join_pkt *pkt = (pc_lobby_join_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(pc_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = (l->type == LOBBY_TYPE_LOBBY) ?
        LOBBY_ADD_PLAYER_TYPE : GAME_ADD_PLAYER_TYPE;
    pkt->hdr.flags = 1;
    pkt->hdr.pkt_len = LE16(0x045C);
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->lobby_num = (l->type == LOBBY_TYPE_LOBBY) ? l->lobby_id - 1 : 0xFF;

    if(l->type == LOBBY_TYPE_LOBBY) {
        pkt->block_num = LE16(l->block->b);
    }
    else {
        pkt->block_num = LE16(0x0001);
        pkt->event = LE16(0x0001);
    }

    /* Copy the player's data into the packet. */
    pkt->entries[0].hdr.player_tag = LE32(0x00010000);
    pkt->entries[0].hdr.guildcard = LE32(nc->guildcard);
    pkt->entries[0].hdr.ip_addr = 0xFFFFFFFF;
    pkt->entries[0].hdr.client_id = LE32(nc->client_id);

    /* Convert the name to UTF-16. */
    if(nc->version == CLIENT_VERSION_BB) {
        memcpy(pkt->entries[0].hdr.name,
               &nc->pl->bb.character.name.char_name[0], BB_CHARACTER_CHAR_NAME_WLENGTH);
        pkt->entries[0].hdr.name[14] = 0;
        pkt->entries[0].hdr.name[15] = 0;
    }
    else {
        istrncpy(ic_8859_to_utf16, (char *)pkt->entries[0].hdr.name,
                 nc->pl->v1.character.dress_data.guildcard_string, 32);
    }

    /* Do any character data conversion that we have to deal with... */
    make_disp_data(nc, c, &pkt->entries[0].data);

    /* 加密并发送 */
    return crypt_send(c, 0x045C, sendbuf);
}

static int send_xbox_lobby_add_player(lobby_t *l, ship_client_t *c,
                                      ship_client_t *nc) {
    uint8_t *sendbuf = get_sendbuf();
    xb_lobby_join_pkt *pkt = (xb_lobby_join_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(xb_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = (l->type == LOBBY_TYPE_LOBBY) ?
        LOBBY_ADD_PLAYER_TYPE : GAME_ADD_PLAYER_TYPE;
    pkt->hdr.flags = 1;
    pkt->hdr.pkt_len = LE16(0x0490);
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->lobby_num = (l->type == LOBBY_TYPE_LOBBY) ? l->lobby_id - 1 : 0xFF;

    if(l->type == LOBBY_TYPE_LOBBY) {
        pkt->block_num = LE16(l->block->b);
    }
    else {
        pkt->block_num = LE16(0x0001);
        pkt->event = LE16(0x0001);
    }

    /* XXXX: Don't really know what the format of this is... Maybe a KEK? */
    memset(pkt->unk, 0x01, 0x18);

    /* Copy the player's data into the packet. */
    pkt->entries[0].hdr.player_tag = LE32(0x00010000);
    pkt->entries[0].hdr.guildcard = LE32(nc->guildcard);

    if(nc->version == CLIENT_VERSION_XBOX) {
        memcpy(&pkt->entries[0].hdr.xbox_ip, nc->xbl_ip,
               sizeof(xbox_ip_t));
    } else {
        /* TODO: Not sure how we should do this for non-xb clients... */
        memset(&pkt->entries[0].hdr.xbox_ip, 0, sizeof(xbox_ip_t));
        pkt->entries[0].hdr.xbox_ip.lan_ip = 0x12345678;
        pkt->entries[0].hdr.xbox_ip.wan_ip = 0x87654321;
        pkt->entries[0].hdr.xbox_ip.port = 1234;
        memset(&pkt->entries[0].hdr.xbox_ip.mac_addr, 0x12, 6);
        pkt->entries[0].hdr.xbox_ip.sg_addr = 0x90807060;
        pkt->entries[0].hdr.xbox_ip.sg_session_id = 0x12345678;
    }

    /* TODO: I have no idea what these three things are. */
    pkt->entries[0].hdr.d1 = 0x01010101;
    pkt->entries[0].hdr.d2 = 0x01010101;
    pkt->entries[0].hdr.d3 = LE32(1);

    pkt->entries[0].hdr.client_id = LE32(nc->client_id);

    /* If its a Blue Burst client, iconv it. */
    if(nc->version == CLIENT_VERSION_BB) {
        istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->entries[0].hdr.name,
                       &nc->pl->bb.character.name.char_name[0], 16, BB_CHARACTER_CHAR_NAME_WLENGTH);
    }
    else {
        memcpy(pkt->entries[0].hdr.name, nc->pl->v1.character.dress_data.guildcard_string, 16);
    }

    make_disp_data(nc, c, &pkt->entries[0].data);

    /* 加密并发送 */
    return crypt_send(c, 0x0490, sendbuf);
}

static int send_bb_lobby_add_player(lobby_t *l, ship_client_t *c,
                                    ship_client_t *nc) {
    uint8_t *sendbuf = get_sendbuf();
    bb_lobby_join_pkt *pkt = (bb_lobby_join_pkt *)sendbuf;
    uint16_t pkt_size = 0x14;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(bb_lobby_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = (l->type == LOBBY_TYPE_LOBBY) ?
        LE16(LOBBY_ADD_PLAYER_TYPE) : LE16(GAME_ADD_PLAYER_TYPE);
    pkt->hdr.flags = LE32(1);
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->lobby_num = (l->type == LOBBY_TYPE_LOBBY) ? l->lobby_id - 1 : 0xFF;

    if(l->type == LOBBY_TYPE_LOBBY) {
        pkt->block_num = l->block->b;
    }
    else {
        pkt->block_num = 0x0001;
        pkt->event = 0x0001;
    }

    /* Copy the player's data into the packet. */
    memset(&pkt->entries[0].hdr, 0, sizeof(bb_player_hdr_t));
    pkt->entries[0].hdr.player_tag = LE32(0x00010000);
    pkt->entries[0].hdr.guildcard = LE32(nc->guildcard);
    pkt->entries[0].hdr.client_id = LE32(nc->client_id);

    /* Convert the name to UTF-16. */
    if(nc->version == CLIENT_VERSION_BB) {
        memcpy(pkt->entries[0].hdr.name, &nc->pl->bb.character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
    }
    else {
        pkt->entries[0].hdr.name[0] = '\t';
        pkt->entries[0].hdr.name[0] = 'J';
        istrncpy(ic_8859_to_utf16, (char *)&pkt->entries[0].hdr.name[2],
                 nc->pl->v1.character.dress_data.guildcard_string, 32);
    }

    make_disp_data(nc, c, &pkt->entries[0].inv);
    pkt_size += sizeof(bb_player_hdr_t) + sizeof(inventory_t) +
        sizeof(psocn_bb_char_t);
    pkt->hdr.pkt_len = LE16(pkt_size);

    /* 加密并发送 */
    return crypt_send(c, pkt_size, sendbuf);
}

int send_lobby_add_player(lobby_t *l, ship_client_t *c) {
    int i;

    /* Send the C-Rank of this new character. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        send_c_rank_update(c, l);
    }

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL && l->clients[i] != c) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                    if(!(l->clients[i]->flags & CLIENT_FLAG_IS_NTE))
                        send_dc_lobby_add_player(l, l->clients[i], c);
                    else
                        send_dcnte_lobby_add_player(l, l->clients[i], c);
                    break;

                case CLIENT_VERSION_PC:
                    send_pc_lobby_add_player(l, l->clients[i], c);
                    break;

                case CLIENT_VERSION_XBOX:
                    send_xbox_lobby_add_player(l, l->clients[i], c);
                    break;

                case CLIENT_VERSION_BB:
                    send_bb_lobby_add_player(l, l->clients[i], c);
                    break;
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    return 0;
}

/* Send a packet to a client that leaves a lobby. */
static int send_dc_lobby_leave(lobby_t *l, ship_client_t *c, int client_id) {
    uint8_t *sendbuf = get_sendbuf();
    dc_lobby_leave_pkt *pkt = (dc_lobby_leave_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* 填充数据头 */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = (l->type == LOBBY_TYPE_LOBBY) ?
            LOBBY_LEAVE_TYPE : GAME_LEAVE_TYPE;
        pkt->hdr.dc.flags = client_id;
        pkt->hdr.dc.pkt_len = LE16(DC_LOBBY_LEAVE_LENGTH);
    }
    else {
        pkt->hdr.pc.pkt_type = (l->type == LOBBY_TYPE_LOBBY) ?
            LOBBY_LEAVE_TYPE : GAME_LEAVE_TYPE;
        pkt->hdr.pc.flags = client_id;
        pkt->hdr.pc.pkt_len = LE16(DC_LOBBY_LEAVE_LENGTH);
    }

    pkt->client_id = client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;

    /* 加密并发送 */
    return crypt_send(c, DC_LOBBY_LEAVE_LENGTH, sendbuf);
}

static int send_bb_lobby_leave(lobby_t *l, ship_client_t *c, int client_id) {
    uint8_t *sendbuf = get_sendbuf();
    bb_lobby_leave_pkt *pkt = (bb_lobby_leave_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* 填充数据头 */
    pkt->hdr.pkt_type = (l->type == LOBBY_TYPE_LOBBY) ?
        LE16(LOBBY_LEAVE_TYPE) : LE16(GAME_LEAVE_TYPE);
    pkt->hdr.flags = LE32(client_id);
    pkt->hdr.pkt_len = LE16(BB_LOBBY_LEAVE_LENGTH);

    pkt->client_id = client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1; /* 2023.2.1 需确认是否正确*/

    /* 加密并发送 */
    return crypt_send(c, BB_LOBBY_LEAVE_LENGTH, sendbuf);
}

int send_lobby_leave(lobby_t *l, ship_client_t *c, int client_id) {
    int i;

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_PC:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    send_dc_lobby_leave(l, l->clients[i], client_id);
                    break;

                case CLIENT_VERSION_BB:
                    send_bb_lobby_leave(l, l->clients[i], client_id);
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    return 0;
}

static int send_dc_lobby_chat(lobby_t *l, ship_client_t *c, ship_client_t *s,
                              const char *msg, const char *cmsg) {
    uint8_t *sendbuf = get_sendbuf();
    dc_chat_pkt *pkt = (dc_chat_pkt *)sendbuf;
    iconv_t ic;
    char* tm = { 0 };
    size_t in, len = strlen(msg) + 32;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    tm = (char*)malloc(len);

    if (!tm) {
        ERR_LOG("无法分配内存");
        return -1;
    }

    memset(tm, 0, len);

    if(msg[0] == '\t' && msg[1] == 'E')
        ic = ic_gbk_to_8859;
    else
        ic = ic_gbk_to_sjis;

    if(!(c->flags & CLIENT_FLAG_WORD_CENSOR)) {
        if(!(s->flags & CLIENT_FLAG_IS_NTE) ||
           s->version != CLIENT_VERSION_DCV1) {
            if(!(c->flags & CLIENT_FLAG_IS_NTE))
                in = sprintf(tm, "%s\t%s", s->pl->v1.character.dress_data.guildcard_string, msg) + 1;
            else {
                in = sprintf(tm, "%s>%X%s", s->pl->v1.character.dress_data.guildcard_string, s->client_id,
                             msg + 2) + 1;
            }
        }
        else {
            if(!(c->flags & CLIENT_FLAG_IS_NTE))
                in = sprintf(tm, "%s\t\tJ%s", s->pl->v1.character.dress_data.guildcard_string, msg) + 1;
            else {
                in = sprintf(tm, "%s>%X%s", s->pl->v1.character.dress_data.guildcard_string, s->client_id,
                             msg) + 1;
            }
        }
    }
    else {
        if(!(s->flags & CLIENT_FLAG_IS_NTE) ||
           s->version != CLIENT_VERSION_DCV1) {
            if(!(c->flags & CLIENT_FLAG_IS_NTE))
                in = sprintf(tm, "%s\t%s", s->pl->v1.character.dress_data.guildcard_string, cmsg) + 1;
            else {
                in = sprintf(tm, "%s>%X%s", s->pl->v1.character.dress_data.guildcard_string, s->client_id,
                             cmsg + 2) + 1;
            }
        }
        else {
            if(!(c->flags & CLIENT_FLAG_IS_NTE))
                in = sprintf(tm, "%s\t\tJ%s", s->pl->v1.character.dress_data.guildcard_string, cmsg) + 1;
            else {
                in = sprintf(tm, "%s>%X%s", s->pl->v1.character.dress_data.guildcard_string, s->client_id,
                             cmsg) + 1;
            }
        }
    }

    if (!in) {
        ERR_LOG("无法发送对话");
        return 0;
    }

    /* Clear the packet header */
    memset(pkt, 0, len + sizeof(dc_chat_pkt));

    /* 填充数据并准备发送.. */
    pkt->client_id = LE32(0x00010000);
    pkt->guildcard = LE32(s->guildcard);

    /* Convert the message to the appropriate encoding. */
    istrncpy(ic, (char*)pkt->msg, tm, (len * 2) + 1);

    free_safe(tm);

    /* Figure out how long the new string is. */
    len = (strlen16_raw(pkt->msg) << 1) + 0x10;

    /* 结尾添加截断字符 0x00*/
    sendbuf[len++] = 0x00;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* Fill in the length */
    len += 0x0C;

    pkt->hdr.dc.pkt_type = CHAT_TYPE;
    pkt->hdr.dc.flags = 0;
    pkt->hdr.dc.pkt_len = LE16((uint16_t)len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_lobby_chat(lobby_t *l, ship_client_t *c, ship_client_t *s,
                              const char *msg, const char *cmsg) {
    uint8_t *sendbuf = get_sendbuf();
    dc_chat_pkt *pkt = (dc_chat_pkt *)sendbuf;
    char* tm = { 0 };
    size_t in, len = strlen(msg) + 32;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    tm = (char*)malloc(len);

    if (!tm) {
        ERR_LOG("无法分配内存");
        return -1;
    }

    memset(tm, 0, len);

    /* Fill in the message */
    if(!(c->flags & CLIENT_FLAG_WORD_CENSOR)) {
        if(!(s->flags & CLIENT_FLAG_IS_NTE) ||
           s->version != CLIENT_VERSION_DCV1)
            in = sprintf(tm, "%s\t%s", s->pl->v1.character.dress_data.guildcard_string, msg) + 1;
        else
            in = sprintf(tm, "%s\t\tJ%s", s->pl->v1.character.dress_data.guildcard_string, msg) + 1;
    }
    else {
        if(!(s->flags & CLIENT_FLAG_IS_NTE) ||
           s->version != CLIENT_VERSION_DCV1)
            in = sprintf(tm, "%s\t%s", s->pl->v1.character.dress_data.guildcard_string, cmsg) + 1;
        else
            in = sprintf(tm, "%s\t\tJ%s", s->pl->v1.character.dress_data.guildcard_string, cmsg) + 1;
    }

    if (!in) {
        ERR_LOG("无法发送对话");
        return 0;
    }

    /* Clear the packet header */
    memset(pkt, 0, len + sizeof(dc_chat_pkt));

    /* Fill in the basics */
    pkt->client_id = LE32(0x00010000);
    pkt->guildcard = LE32(s->guildcard);

    /* Convert the message to the appropriate encoding. */
    istrncpy(ic_gbk_to_utf16, (char*)pkt->msg, tm, (len * 2) + 1);

    free_safe(tm);

    /* Figure out how long the new string is. */
    len = (strlen16_raw(pkt->msg) << 1) + 0x10;

    /* 结尾添加截断字符 0x00*/
    sendbuf[len++] = 0x00;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* Fill in the length */
    len += 0x0C;

    pkt->hdr.pc.pkt_type = CHAT_TYPE;
    pkt->hdr.pc.flags = 0;
    pkt->hdr.pc.pkt_len = LE16((uint16_t)len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_lobby_chat(lobby_t *l, ship_client_t *c, ship_client_t *s,
                              const char *msg, const char *cmsg) {
    uint8_t *sendbuf = get_sendbuf();
    bb_chat_pkt *pkt = (bb_chat_pkt *)sendbuf;
    char *tm = { 0 };
    size_t in, len = strlen(msg) + 32;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    tm = (char*)malloc(len);

    if (!tm) {
        ERR_LOG("无法分配内存");
        return -1;
    }

    memset(tm, 0, len);

    /* Fill in the message */
    if(!(c->flags & CLIENT_FLAG_WORD_CENSOR)) {
        if(!(s->flags & CLIENT_FLAG_IS_NTE) ||
           s->version != CLIENT_VERSION_DCV1)
            in = sprintf(tm, "%s\t%s", s->pl->bb.character.dress_data.guildcard_string, msg) + 1;
        else
            in = sprintf(tm, "%s\t\tJ%s", s->pl->bb.character.dress_data.guildcard_string, msg) + 1;
    }
    else {
        if(!(s->flags & CLIENT_FLAG_IS_NTE) ||
           s->version != CLIENT_VERSION_DCV1)
            in = sprintf(tm, "%s\t%s", s->pl->bb.character.dress_data.guildcard_string, cmsg) + 1;
        else
            in = sprintf(tm, "%s\t\tJ%s", s->pl->bb.character.dress_data.guildcard_string, cmsg) + 1;
    }

    if (!in) {
        ERR_LOG("无法发送对话");
        return 0;
    }

    /* Clear the packet header */
    memset(pkt, 0, len + sizeof(bb_chat_pkt));

    /* Fill in the basics */
    pkt->client_id = LE32(0x00010000);
    pkt->guildcard = LE32(s->guildcard);

    /* Convert the message to the appropriate encoding. */
    istrncpy(ic_gbk_to_utf16, (char*)pkt->msg, tm, (len * 2) + 1);

    free_safe(tm);

    /* Figure out how long the new string is. */
    len = (strlen16_raw(pkt->msg) << 1) + 0x10;

    /* 结尾添加截断字符 0x00*/
    sendbuf[len++] = 0x00;
    sendbuf[len++] = 0x00;

    /* Add any padding needed */
    while(len & 0x07) {
        sendbuf[len++] = 0;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(CHAT_TYPE);
    pkt->hdr.flags = 0;
    pkt->hdr.pkt_len = LE16((uint16_t)len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

/* Send a talk packet to the specified lobby. */
int send_lobby_chat(lobby_t *l, ship_client_t *sender, const char *msg,
                    const char *cmsg) {
    int i;

    if((sender->flags & CLIENT_FLAG_STFU)) {
        switch(sender->version) {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
            case CLIENT_VERSION_GC:
            case CLIENT_VERSION_EP3:
            case CLIENT_VERSION_XBOX:
                return send_dc_lobby_chat(l, sender, sender, msg, cmsg);

            case CLIENT_VERSION_PC:
                return send_pc_lobby_chat(l, sender, sender, msg, cmsg);

            case CLIENT_VERSION_BB:
                return send_bb_lobby_chat(l, sender, sender, msg, msg);
        }

        return 0;
    }

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    /* Only send if they're not being /ignore'd */
                    if(!client_has_ignored(l->clients[i], sender->guildcard)) {
                        send_dc_lobby_chat(l, l->clients[i], sender, msg, cmsg);
                    }
                    break;

                case CLIENT_VERSION_PC:
                    /* Only send if they're not being /ignore'd */
                    if(!client_has_ignored(l->clients[i], sender->guildcard)) {
                        send_pc_lobby_chat(l, l->clients[i], sender, msg, cmsg);
                    }
                    break;

                case CLIENT_VERSION_BB:
                    /* Only send if they're not being /ignore'd */
                    if(!client_has_ignored(l->clients[i], sender->guildcard)) {
                        send_bb_lobby_chat(l, l->clients[i], sender, msg, cmsg);
                    }
                    break;
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    return 0;
}

static int send_dc_lobby_bbchat(lobby_t *l, ship_client_t *c, ship_client_t *s,
                                const uint8_t *msg, size_t len) {
    uint8_t *sendbuf = get_sendbuf();
    dc_chat_pkt *pkt = (dc_chat_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet header */
    memset(pkt, 0, sizeof(dc_chat_pkt));

    /* Fill in the basics */
    pkt->client_id = LE32(0x00010000);
    pkt->guildcard = LE32(s->guildcard);

    /* Convert the name string first. */
    in = BB_CHARACTER_CHAR_NAME_WLENGTH;
    out = 65520;
    inptr = (char *)&s->pl->bb.character.name.char_name[0];
    outptr = pkt->msg;
    iconv(ic_utf16_to_gbk/*尝试GBK*/, &inptr, &in, &outptr, &out);

    if(!(c->flags & CLIENT_FLAG_IS_NTE)) {
        /* Add the separator */
        *outptr++ = '\t';
        --out;
    }
    else {
        *outptr++ = '>';
        --out;
        sprintf(outptr, "%X", s->client_id);
        ++outptr;
        --out;
    }

    /* Fill in the message */
    in = len;

    if(!(c->flags & CLIENT_FLAG_IS_NTE))
        inptr = (char *)msg;
    else
        inptr = ((char *)msg) + 4;

    /* Convert the message to the appropriate encoding. */
    if((c->flags & CLIENT_FLAG_IS_NTE) || msg[1] == LE16('J'))
        iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
    else
        iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65520 - out;

    /* 结尾添加截断字符 0x00*/
    pkt->msg[len++] = 0x00;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* Fill in the length */
    len += 0x0C;

    pkt->hdr.dc.pkt_type = CHAT_TYPE;
    pkt->hdr.dc.flags = 0;
    pkt->hdr.dc.pkt_len = LE16((uint16_t)len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_lobby_bbchat(lobby_t *l, ship_client_t *c, ship_client_t *s,
                                const uint8_t *msg, size_t len) {
    uint8_t *sendbuf = get_sendbuf();
    dc_chat_pkt *pkt = (dc_chat_pkt *)sendbuf;
    uint16_t tmp[2] = { LE16('\t'), 0 };

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet header */
    memset(pkt, 0, sizeof(dc_chat_pkt));

    /* 填充数据头 */
    pkt->client_id = LE32(0x00010000);
    pkt->guildcard = LE32(s->guildcard);

    strcpy16_raw(pkt->msg, &s->pl->bb.character.name.char_name[0]);
    strcat16_raw(pkt->msg, tmp);
    strcat16_raw(pkt->msg, msg);
    len = (strlen16_raw(pkt->msg) << 1) + 0x0E;

    /* 结尾添加截断字符 0x00*/
    sendbuf[len++] = 0x00;

    /* Add any padding needed */
    while(len & 0x03) {
        sendbuf[len++] = 0;
    }

    /* 填充数据头 */
    pkt->hdr.pc.pkt_len = LE16((uint16_t)len);
    pkt->hdr.pc.pkt_type = CHAT_TYPE;
    pkt->hdr.pc.flags = 0;

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_lobby_bbchat(lobby_t *l, ship_client_t *c, ship_client_t *s,
                                const uint8_t *msg, size_t len) {
    uint8_t *sendbuf = get_sendbuf();
    bb_chat_pkt *pkt = (bb_chat_pkt *)sendbuf;
    uint16_t tmp[2] = { LE16('\t'), 0 };

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet header */
    memset(pkt, 0, sizeof(bb_chat_pkt));

    /* 填充数据头 */
    pkt->client_id = LE32(s->client_id);
    pkt->guildcard = LE32(s->guildcard);

    strcpy16_raw(pkt->msg, &s->pl->bb.character.name);
    strcat16_raw(pkt->msg, tmp);
    strcat16_raw(pkt->msg, msg);
    len = (strlen16_raw(pkt->msg) << 1) + 0x12;

    /* 结尾添加截断字符 0x00*/
    sendbuf[len++] = 0x00;
    sendbuf[len++] = 0x00;

    /* Add any padding needed */
    while(len & 0x07) {
        sendbuf[len++] = 0;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16((uint16_t)len);
    pkt->hdr.pkt_type = LE16(CHAT_TYPE);
    pkt->hdr.flags = 0;

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

/* Send a talk packet to the specified lobby (UTF-16 - Blue Burst). */
int send_lobby_bbchat(lobby_t *l, ship_client_t *sender, const uint8_t *msg,
                      size_t len) {
    int i;

    if((sender->flags & CLIENT_FLAG_STFU)) {
        return send_bb_lobby_bbchat(l, sender, sender, msg, len);
    }

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    /* Only send if they're not being /ignore'd */
                    if(!client_has_ignored(l->clients[i], sender->guildcard)) {
                        send_dc_lobby_bbchat(l, l->clients[i], sender, msg,
                                             len);
                    }
                    break;

                case CLIENT_VERSION_PC:
                    /* Only send if they're not being /ignore'd */
                    if(!client_has_ignored(l->clients[i], sender->guildcard)) {
                        send_pc_lobby_bbchat(l, l->clients[i], sender, msg,
                                             len);
                    }
                    break;

                case CLIENT_VERSION_BB:
                    /* Only send if they're not being /ignore'd */
                    if(!client_has_ignored(l->clients[i], sender->guildcard)) {
                        send_bb_lobby_bbchat(l, l->clients[i], sender, msg,
                                             len);
                    }
                    break;
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    return 0;
}

/* Send a guild card search reply to the specified client. */
static int send_dc_guild_reply(ship_client_t *c, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    dc_guild_reply_pkt *pkt = (dc_guild_reply_pkt *)sendbuf;
    lobby_t *l = s->cur_lobby;
    block_t *b = s->cur_block;
    uint16_t port = 0;
    char lname[17], lobby_name[32];

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* If they're not on a block or lobby, don't even try. */
    if(!l || !b)
        return 0;

    /* Clear it out first */
    memset(pkt, 0, DC_GUILD_REPLY_LENGTH);

    /* Adjust the port properly... */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            port = b->dc_port;
            break;

        case CLIENT_VERSION_GC:
            port = b->gc_port;
            break;

        case CLIENT_VERSION_EP3:
            port = b->ep3_port;
            break;

        case CLIENT_VERSION_XBOX:
            port = b->xb_port;
            break;
    }

    /* Fill in the simple stuff */
    pkt->hdr.pkt_type = GUILD_REPLY_TYPE;
    pkt->hdr.pkt_len = LE16(DC_GUILD_REPLY_LENGTH);
    pkt->player_tag = LE32(0x00010000);
    pkt->gc_searcher = LE32(c->guildcard);
    pkt->gc_target = LE32(s->guildcard);
    pkt->ip = ship_ip4;
    pkt->port = LE16(port);
    pkt->menu_id = LE32(MENU_ID_LOBBY);
    pkt->item_id = LE32(s->lobby_id);

    /* iconv the name, if needed... */
    if(s->version == CLIENT_VERSION_BB) {
        istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->name,
                       &s->bb_pl->character.name.char_name[0], 0x20, BB_CHARACTER_CHAR_NAME_WLENGTH);
    }
    else {
        strcpy(pkt->name, s->pl->v1.character.dress_data.guildcard_string);
    }

    /* Fill in the location string. The lobby name is UTF-8 and everything
       else is ASCII (which is safe to use as Shift-JIS or ISO-8859-1). */
    if(s->lobby_id <= 15) {
        sprintf(lobby_name, "BLOCK%02d-%02d", b->b, s->lobby_id);
    }
    else {
        sprintf(lobby_name, "BLOCK%02d-C%d", b->b, s->lobby_id - 15);
    }

    if(l->type == LOBBY_TYPE_LOBBY) {
        sprintf(pkt->location, "%s, ,%s", lobby_name, ship->cfg->name);
    }
    else {
        /* iconv the lobby name */
        if(l->name[0] == '\t' && l->name[1] == 'J') {
            istrncpy(ic_utf8_to_sjis, lname, l->name, 16);
        }
        else {
            istrncpy(ic_utf8_to_8859, lname, l->name, 16);
        }

        lname[16] = 0;

        /* Fill in the location string. Everything here is ASCII, so this is
           safe */
        sprintf(pkt->location, "%s,%s, ,%s", lname, lobby_name,
                ship->cfg->name);
    }

    /* 加密并发送 */
    return crypt_send(c, DC_GUILD_REPLY_LENGTH, sendbuf);
}

static int send_pc_guild_reply(ship_client_t *c, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    pc_guild_reply_pkt *pkt = (pc_guild_reply_pkt *)sendbuf;
    char tmp[0x44], lobby_name[32];
    size_t len;
    lobby_t *l = s->cur_lobby;
    block_t *b = s->cur_block;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* If they're not on a block or lobby, don't even try. */
    if(!l || !b)
        return 0;

    /* Clear it out first */
    memset(pkt, 0, PC_GUILD_REPLY_LENGTH);

    /* Fill in the simple stuff */
    pkt->hdr.pkt_type = GUILD_REPLY_TYPE;
    pkt->hdr.pkt_len = LE16(PC_GUILD_REPLY_LENGTH);
    pkt->player_tag = LE32(0x00010000);
    pkt->gc_searcher = LE32(c->guildcard);
    pkt->gc_target = LE32(s->guildcard);
    pkt->ip = ship_ip4;
    pkt->port = LE16(b->pc_port);
    pkt->menu_id = LE32(MENU_ID_LOBBY);
    pkt->item_id = LE32(s->lobby_id);

    /* Fill in the location string. The lobby name is UTF-8 and everything
       else is ASCII (which is safe to use as UTF-8). */
    if(s->lobby_id <= 15) {
        sprintf(lobby_name, "BLOCK%02d-%02d", b->b, s->lobby_id);
    }
    else {
        sprintf(lobby_name, "BLOCK%02d-C%d", b->b, s->lobby_id - 15);
    }

    if(l->type == LOBBY_TYPE_LOBBY) {
        sprintf(tmp, "%s, ,%s", lobby_name, ship->cfg->name);
        istrncpy(ic_gbk_to_utf16, (char *)pkt->location, tmp, 0x88);
    }
    else {
        istrncpy(ic_gbk_to_utf16, (char *)pkt->location, l->name, 0x1C);
        pkt->location[14] = pkt->location[15] = 0;
        len = strlen16_raw(pkt->location);

        sprintf(tmp, ",%s, ,%s", lobby_name, ship->cfg->name);
        istrncpy(ic_gbk_to_utf16, (char *)(pkt->location + len), tmp,
                 0x88 - len * 2);
    }

    /* ...and the name. */
    if(s->version == CLIENT_VERSION_BB) {
        memcpy(pkt->name, &s->bb_pl->character.name.char_name[0], BB_CHARACTER_CHAR_NAME_WLENGTH);
    }
    else {
        istrncpy(ic_8859_to_utf16, (char *)pkt->name, s->pl->v1.character.dress_data.guildcard_string, 0x40);
    }

    /* 加密并发送 */
    return crypt_send(c, PC_GUILD_REPLY_LENGTH, sendbuf);
}

static int send_bb_guild_reply(ship_client_t *c, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    bb_guild_reply_pkt *pkt = (bb_guild_reply_pkt *)sendbuf;
    char tmp[256], lobby_name[32];
    lobby_t *l = s->cur_lobby;
    block_t *b = s->cur_block;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* If they're not on a block or lobby, don't even try. */
    if(!l || !b)
        return 0;

    /* Clear it out first */
    memset(pkt, 0, BB_GUILD_REPLY_LENGTH);

    /* Fill in the simple stuff */
    pkt->hdr.pkt_type = LE16(GUILD_REPLY_TYPE);
    pkt->hdr.pkt_len = LE16(BB_GUILD_REPLY_LENGTH);
    pkt->player_tag = LE32(0x00010000);
    pkt->gc_searcher = LE32(c->guildcard);
    pkt->gc_target = LE32(s->guildcard);
    pkt->ip = ship_ip4;
    pkt->port = LE16(b->bb_port);
    pkt->menu_id = LE32(MENU_ID_LOBBY);
    pkt->item_id = LE32(s->lobby_id);

    /* Fill in the location string. The lobby name is UTF-8 and everything
       else is ASCII (which is safe to use as UTF-8). */
    if(s->lobby_id <= 15) {
        sprintf(lobby_name, "BLOCK%02d-%02d", b->b, s->lobby_id);
    }
    else {
        sprintf(lobby_name, "BLOCK%02d-C%d", b->b, s->lobby_id - 15);
    }

    if(l->type == LOBBY_TYPE_LOBBY) {
        sprintf(tmp, "%s, , %s", lobby_name, ship->cfg->name);
    }
    else {
        sprintf(tmp, "%s,%s, ,%s", l->name, lobby_name, ship->cfg->name);
    }

    istrncpy(ic_gbk_to_utf16, (char *)pkt->location, tmp, 0x88);

    /* ...and the name. */
    if(s->version == CLIENT_VERSION_BB) {
        memcpy(pkt->name, &s->bb_pl->character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
    }
    else {
        pkt->name[0] = LE16('\t');
        pkt->name[1] = LE16('E');
        istrncpy(ic_8859_to_utf16, (char *)&pkt->name[2], s->pl->v1.character.dress_data.guildcard_string, 0x3C);
    }

    /* 加密并发送 */
    return crypt_send(c, BB_GUILD_REPLY_LENGTH, sendbuf);
}

int send_guild_reply(ship_client_t *c, ship_client_t *s) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_guild_reply(c, s);

        case CLIENT_VERSION_PC:
            return send_pc_guild_reply(c, s);

        case CLIENT_VERSION_BB:
            return send_bb_guild_reply(c, s);
    }

    return -1;
}

#ifdef PSOCN_ENABLE_IPV6

/* Send an IPv6 guild card search reply to the specified client. */
static int send_dc_guild_reply6(ship_client_t *c, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    dc_guild_reply6_pkt *pkt = (dc_guild_reply6_pkt *)sendbuf;
    lobby_t *l = s->cur_lobby;
    block_t *b = s->cur_block;
    uint16_t port = 0;
    char lname[17], lobby_name[32];

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* If they're not on a block or lobby, don't even try. */
    if(!l || !b)
        return 0;

    /* Clear it out first */
    memset(pkt, 0, DC_GUILD_REPLY6_LENGTH);

    /* Adjust the port properly... */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            port = b->dc_port;
            break;

        case CLIENT_VERSION_GC:
            port = b->gc_port;
            break;

        case CLIENT_VERSION_EP3:
            port = b->ep3_port;
            break;

        case CLIENT_VERSION_XBOX:
            port = b->xb_port;
            break;
    }

    /* Fill in the simple stuff */
    pkt->hdr.pkt_type = GUILD_REPLY_TYPE;
    pkt->hdr.flags = 6;
    pkt->hdr.pkt_len = LE16(DC_GUILD_REPLY6_LENGTH);
    pkt->player_tag = LE32(0x00010000);
    pkt->gc_searcher = LE32(c->guildcard);
    pkt->gc_target = LE32(s->guildcard);
    memcpy(pkt->ip, ship_ip6, 16);
    pkt->port = LE16(port);
    pkt->menu_id = LE32(MENU_ID_LOBBY);
    pkt->item_id = LE32(s->lobby_id);

    /* iconv the name, if needed... */
    if(s->version == CLIENT_VERSION_BB) {
        istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->name,
                       &s->bb_pl->character.name[2], 0x20, 16);
    }
    else {
        strcpy(pkt->name, s->pl->v1.character.dress_data.guildcard_string);
    }

    /* Fill in the location string. The lobby name is UTF-8 and everything
       else is ASCII (which is safe to use as Shift-JIS or ISO-8859-1). */
    if(s->lobby_id <= 15) {
        sprintf(lobby_name, "BLOCK%02d-%02d", b->b, s->lobby_id);
    }
    else {
        sprintf(lobby_name, "BLOCK%02d-C%d", b->b, s->lobby_id - 15);
    }

    if(l->type == LOBBY_TYPE_LOBBY) {
        sprintf(pkt->location, "%s, ,%s", lobby_name, ship->cfg->name);
    }
    else {
        /* iconv the lobby name */
        if(l->name[0] == '\t' && l->name[1] == 'J') {
            istrncpy(ic_utf8_to_sjis, lname, l->name, 16);
        }
        else {
            istrncpy(ic_utf8_to_8859, lname, l->name, 16);
        }

        lname[16] = 0;

        /* Fill in the location string. Everything here is ASCII, so this is
           safe */
        sprintf(pkt->location, "%s,%s, ,%s", lname, lobby_name,
                ship->cfg->name);
    }

    /* 加密并发送 */
    return crypt_send(c, DC_GUILD_REPLY6_LENGTH, sendbuf);
}

static int send_pc_guild_reply6(ship_client_t *c, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    pc_guild_reply6_pkt *pkt = (pc_guild_reply6_pkt *)sendbuf;
    char tmp[0x44], lobby_name[32];
    size_t len;
    lobby_t *l = s->cur_lobby;
    block_t *b = s->cur_block;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* If they're not on a block or lobby, don't even try. */
    if(!l || !b)
        return 0;

    /* Clear it out first */
    memset(pkt, 0, PC_GUILD_REPLY6_LENGTH);

    /* Fill in the simple stuff */
    pkt->hdr.pkt_type = GUILD_REPLY_TYPE;
    pkt->hdr.flags = 6;
    pkt->hdr.pkt_len = LE16(PC_GUILD_REPLY6_LENGTH);
    pkt->player_tag = LE32(0x00010000);
    pkt->gc_searcher = LE32(c->guildcard);
    pkt->gc_target = LE32(s->guildcard);
    memcpy(pkt->ip, ship_ip6, 16);
    pkt->port = LE16(b->pc_port);
    pkt->menu_id = LE32(MENU_ID_LOBBY);
    pkt->item_id = LE32(s->lobby_id);

    /* Fill in the location string. The lobby name is UTF-8 and everything
       else is ASCII (which is safe to use as UTF-8). */
    if(s->lobby_id <= 15) {
        sprintf(lobby_name, "BLOCK%02d-%02d", b->b, s->lobby_id);
    }
    else {
        sprintf(lobby_name, "BLOCK%02d-C%d", b->b, s->lobby_id - 15);
    }

    if(l->type == LOBBY_TYPE_LOBBY) {
        sprintf(tmp, "%s, ,%s", lobby_name, ship->cfg->name);
        istrncpy(ic_gbk_to_utf16, (char *)pkt->location, tmp, 0x88);
    }
    else {
        istrncpy(ic_gbk_to_utf16, (char *)pkt->location, l->name, 0x1C);
        pkt->location[14] = pkt->location[15] = 0;
        len = strlen16_raw(pkt->location);

        sprintf(tmp, ",%s, ,%s", lobby_name, ship->cfg->name);
        istrncpy(ic_gbk_to_utf16, (char *)(pkt->location + len), tmp,
                 0x88 - len * 2);
    }

    /* ...and the name. */
    if(s->version == CLIENT_VERSION_BB) {
        memcpy(pkt->name, &s->bb_pl->character.name[2], 28);
    }
    else {
        istrncpy(ic_8859_to_utf16, (char *)pkt->name, s->pl->v1.character.dress_data.guildcard_string, 0x40);
    }


    /* 加密并发送 */
    return crypt_send(c, PC_GUILD_REPLY6_LENGTH, sendbuf);
}

static int send_bb_guild_reply6(ship_client_t *c, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    bb_guild_reply6_pkt *pkt = (bb_guild_reply6_pkt *)sendbuf;
    char tmp[256], lobby_name[32];
    lobby_t *l = s->cur_lobby;
    block_t *b = s->cur_block;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* If they're not on a block or lobby, don't even try. */
    if(!l || !b)
        return 0;

    /* Clear it out first */
    memset(pkt, 0, BB_GUILD_REPLY6_LENGTH);

    /* Fill in the simple stuff */
    pkt->hdr.pkt_type = LE16(GUILD_REPLY_TYPE);
    pkt->hdr.pkt_len = LE16(BB_GUILD_REPLY6_LENGTH);
    pkt->hdr.flags = LE32(6);
    pkt->player_tag = LE32(0x00010000);
    pkt->gc_searcher = LE32(c->guildcard);
    pkt->gc_target = LE32(s->guildcard);
    memcpy(pkt->ip, ship_ip6, 16);
    pkt->port = LE16(b->bb_port);
    pkt->menu_id = LE32(MENU_ID_LOBBY);
    pkt->item_id = LE32(s->lobby_id);

    /* Fill in the location string. The lobby name is UTF-8 and everything
       else is ASCII (which is safe to use as UTF-8). */
    if(s->lobby_id <= 15) {
        sprintf(lobby_name, "BLOCK%02d-%02d", b->b, s->lobby_id);
    }
    else {
        sprintf(lobby_name, "BLOCK%02d-C%d", b->b, s->lobby_id - 15);
    }

    if(l->type == LOBBY_TYPE_LOBBY) {
        sprintf(tmp, "%s, , %s", lobby_name, ship->cfg->name);
    }
    else {
        sprintf(tmp, "%s,%s, ,%s", l->name, lobby_name, ship->cfg->name);
    }

    istrncpy(ic_gbk_to_utf16, (char *)pkt->location, tmp, 0x88);

    /* ...and the name. */
    if(s->version == CLIENT_VERSION_BB) {
        memcpy(pkt->name, s->bb_pl->character.name, 32);
    }
    else {
        pkt->name[0] = LE16('\t');
        pkt->name[1] = LE16('E');
        istrncpy(ic_8859_to_utf16, (char *)&pkt->name[2], s->pl->v1.character.dress_data.guildcard_string, 0x3C);
    }

    /* 加密并发送 */
    return crypt_send(c, BB_GUILD_REPLY6_LENGTH, sendbuf);
}

int send_guild_reply6(ship_client_t *c, ship_client_t *s) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
            return send_dc_guild_reply6(c, s);

        case CLIENT_VERSION_PC:
            return send_pc_guild_reply6(c, s);

        case CLIENT_VERSION_BB:
            return send_bb_guild_reply6(c, s);
    }

    return -1;
}

#endif

/* Send a premade guild card search reply to the specified client. */
static int send_dc_guild_reply_sg(ship_client_t *c, dc_guild_reply_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    dc_guild_reply_pkt *pkt = (dc_guild_reply_pkt *)sendbuf;
    uint16_t port = LE16(p->port);

    memcpy(pkt, p, sizeof(dc_guild_reply_pkt));

    /* Adjust the port properly... */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            break;

        case CLIENT_VERSION_GC:
            pkt->port = LE16((port + 2));
            break;

        case CLIENT_VERSION_EP3:
            pkt->port = LE16((port + 3));
            break;

        case CLIENT_VERSION_XBOX:
            pkt->port = LE16((port + 5));
            break;
    }

    /* Convert the location string from UTF-8 to the appropriate encoding. */
    if(p->location[0] == '\t' && p->location[1] == 'J') {
        istrncpy(ic_utf8_to_sjis, pkt->location, p->location, 0x44);
    }
    else {
        istrncpy(ic_utf8_to_8859, pkt->location, p->location, 0x44);
    }

    /* 加密并发送 */
    return crypt_send(c, DC_GUILD_REPLY_LENGTH, sendbuf);
}

static int send_pc_guild_reply_sg(ship_client_t *c, dc_guild_reply_pkt *dc) {
    uint8_t *sendbuf = get_sendbuf();
    pc_guild_reply_pkt *pkt = (pc_guild_reply_pkt *)sendbuf;
    uint16_t port = LE16(dc->port);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Adjust the port properly... */
    ++port;

    /* Clear it out first */
    memset(pkt, 0, PC_GUILD_REPLY_LENGTH);

    /* Fill in the simple stuff */
    pkt->hdr.pkt_type = GUILD_REPLY_TYPE;
    pkt->hdr.pkt_len = LE16(PC_GUILD_REPLY_LENGTH);
    pkt->player_tag = LE32(0x00010000);
    pkt->gc_searcher = dc->gc_searcher;
    pkt->gc_target = dc->gc_target;
    pkt->ip = dc->ip;
    pkt->port = LE16(port);
    pkt->menu_id = dc->menu_id;
    pkt->item_id = dc->item_id;

    /* Fill in the location string and the name */
    istrncpy(ic_gbk_to_utf16, (char *)pkt->location, dc->location, 0x88);
    istrncpy(ic_8859_to_utf16, (char *)pkt->name, dc->name, 0x40);

    /* 加密并发送 */
    return crypt_send(c, PC_GUILD_REPLY_LENGTH, sendbuf);
}

int send_guild_reply_sg(ship_client_t *c, dc_guild_reply_pkt *pkt) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_guild_reply_sg(c, pkt);

        case CLIENT_VERSION_PC:
            return send_pc_guild_reply_sg(c, pkt);

        /* We should never get one of these for a Blue Burst client. They're
           handled separately. */
    }

    return -1;
}

#ifdef PSOCN_ENABLE_IPV6

/* Send a premade IPv6 guild card search reply to the specified client. */
static int send_dc_guild_reply6_sg(ship_client_t *c, dc_guild_reply6_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    dc_guild_reply6_pkt *pkt = (dc_guild_reply6_pkt *)sendbuf;
    uint16_t port = LE16(p->port);

    /* Adjust the port properly... */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            break;

        case CLIENT_VERSION_GC:
            pkt->port = LE16((port + 2));
            break;

        case CLIENT_VERSION_EP3:
            pkt->port = LE16((port + 3));
            break;

        case CLIENT_VERSION_XBOX:
            pkt->port = LE16((port + 5));
            break;
    }

    memcpy(pkt, p, sizeof(dc_guild_reply6_pkt));

    /* Convert the location string from UTF-8 to the appropriate encoding. */
    if(p->location[0] == '\t' && p->location[1] == 'J') {
        istrncpy(ic_utf8_to_sjis, pkt->location, p->location, 0x44);
    }
    else {
        istrncpy(ic_utf8_to_8859, pkt->location, p->location, 0x44);
    }

    /* 加密并发送 */
    return crypt_send(c, DC_GUILD_REPLY6_LENGTH, (uint8_t *)pkt);
}

static int send_pc_guild_reply6_sg(ship_client_t *c, dc_guild_reply6_pkt *dc) {
    uint8_t *sendbuf = get_sendbuf();
    pc_guild_reply6_pkt *pkt = (pc_guild_reply6_pkt *)sendbuf;
    uint16_t port = LE16(dc->port);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Adjust the port properly... */
    ++port;

    /* Clear it out first */
    memset(pkt, 0, PC_GUILD_REPLY6_LENGTH);

    /* Fill in the simple stuff */
    pkt->hdr.pkt_type = GUILD_REPLY_TYPE;
    pkt->hdr.flags = 6;
    pkt->hdr.pkt_len = LE16(PC_GUILD_REPLY6_LENGTH);
    pkt->player_tag = LE32(0x00010000);
    pkt->gc_searcher = dc->gc_searcher;
    pkt->gc_target = dc->gc_target;
    memcpy(pkt->ip, dc->ip, 16);
    pkt->port = LE16(port);
    pkt->menu_id = dc->menu_id;
    pkt->item_id = dc->item_id;

    /* Fill in the location string and the name */
    istrncpy(ic_gbk_to_utf16, (char *)pkt->location, dc->location, 0x88);
    istrncpy(ic_8859_to_utf16, (char *)pkt->name, dc->name, 0x40);

    /* 加密并发送 */
    return crypt_send(c, PC_GUILD_REPLY6_LENGTH, sendbuf);
}

int send_guild_reply6_sg(ship_client_t *c, dc_guild_reply6_pkt *pkt) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_guild_reply6_sg(c, pkt);

        case CLIENT_VERSION_PC:
            return send_pc_guild_reply6_sg(c, pkt);

        /* We should never get one of these for a Blue Burst client. They're
           handled separately. */
    }

    return -1;
}

#endif

static int send_dcnte_txt(ship_client_t *c, const char *fmt, va_list args) {
    uint8_t *sendbuf = get_sendbuf();
    dc_chat_pkt *pkt = (dc_chat_pkt *)sendbuf;
    int len;
    iconv_t ic;
    char tm[512] = { 0 };
    size_t in, out;
    size_t i;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet header */
    memset(pkt, 0, sizeof(dc_chat_pkt));

    /* Do the formatting */
    vsnprintf(tm + 2, 510, fmt, args);

    /* Make sure it starts with the right thing and ends with a terminator. */
    tm[0] = '>';
    tm[1] = ' ';                    /* It doesn't matter what we put here... */
    tm[511] = '\0';
    in = strlen(tm) + 1;

    /* Strip out any escapes, since the NTE doesn't support them. */
    for(i = 0; i < in; ++i) {
        if(tm[i] == '\t') {
            if(tm[i + 1] == 'C' && tm[i + 2] != '\0') {
                memmove(&tm[i], &tm[i + 3], in - i - 3);
                in -= 3;
            }
            else if(tm[i + 1] != '\t') {
                memmove(&tm[i], &tm[i + 2], in - i - 2);
                in -= 2;
            }
            else {
                memmove(&tm[i], &tm[i + 1], in - i - 1);
                --in;
            }

            --i;
        }
    }

    /* Convert to Shift-JIS... */
    ic = ic_gbk_to_sjis;
    out = 65520;
    inptr = tm;
    outptr = pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65520 - out;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* Fill in the length */
    len += 0x0C;
    pkt->hdr.dc.pkt_type = CHAT_TYPE;
    pkt->hdr.dc.flags = 0;
    pkt->hdr.dc.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_dc_message(ship_client_t *c, uint16_t type, const char *fmt,
                           va_list args) {
    uint8_t *sendbuf = get_sendbuf();
    dc_chat_pkt *pkt = (dc_chat_pkt *)sendbuf;
    int len;
    iconv_t ic;
    char tm[4096];
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, sizeof(dc_chat_pkt));

    /* Do the formatting */
    vsnprintf(tm, 4096, fmt, args);
    tm[4095] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if(tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    /* Set up to convert between encodings */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        if(tm[1] != 'J') {
            ic = ic_gbk_to_8859;
            //ic = ic_utf8_to_8859;
        }
        else {
            ic = ic_gbk_to_sjis;
            //ic = ic_utf8_to_sjis;
        }
    }
    else {
        //ic = ic_utf8_to_utf16;
        ic = ic_gbk_to_utf16;
    }

    in = strlen(tm) + 1;

    /* Convert the message to the appropriate encoding. */
    out = 65520;
    inptr = tm;
    outptr = pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65520 - out;

    /* 结尾添加截断字符 0x00*/
    pkt->msg[len++] = 0x00;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    char_add_color_tag(pkt->msg);

    /* Fill in the length */
    len += 0x0C;

    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = (uint8_t)type;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(len);
    }
    else {
        pkt->hdr.pc.pkt_type = (uint8_t)type;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(len);
    }

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_message(ship_client_t *c, uint16_t type, const char *fmt,
                           va_list args) {
    uint8_t *sendbuf = get_sendbuf();
    bb_chat_pkt *pkt = (bb_chat_pkt *)sendbuf;
    int len;
    char tm[4096];
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(tm, 0, sizeof(tm));
    memset(pkt, 0, sizeof(bb_chat_pkt));

    /* Do the formatting */
    vsnprintf(tm, 4096, fmt, args);
    tm[4095] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if(tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    /* Convert the message to the appropriate encoding. */
    in = strlen(tm) + 1;
    out = 65520;
    inptr = tm;
    outptr = (char *)pkt->msg;
    iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65520 - out + 0x10;

    /* 结尾添加截断字符 0x00*/
    sendbuf[len++] = 0x00;
    sendbuf[len++] = 0x00;

    /* Add any padding needed */
    while(len & 0x07) {
        sendbuf[len++] = 0;
    }

    wchar_add_color_tag(pkt->msg);

    pkt->hdr.pkt_type = LE16(type);
    pkt->hdr.flags = 0;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

/* Send a message to the client. */
int send_msg1(ship_client_t *c, const char *fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            rv = send_dc_message(c, MSG1_TYPE, fmt, args);
            break;

        case CLIENT_VERSION_BB:
            rv = send_bb_message(c, MSG1_TYPE, fmt, args);
            break;
    }

    va_end(args);

    return rv;
}

/* 模拟左下角信息框 暂时无效*/
const uint8_t LeftbuttomMes_Pkt11[] = {
    0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00
};

//大信息窗口文本代码
const uint8_t BigMes_Pkt1A[] = {
    0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x45, 0x00
};

//右下角弹窗信息数据包
const uint8_t BRMes_Pkt01[] = {
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x45, 0x00
};

//边栏信息数据包
const uint8_t SideMes_PktB0[] = {
    0x40, 0x00, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x45, 0x00 };

//顶部公告数据包
const uint8_t PacketEE[] = {
    0x00, 0x00, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* 发送文本类型的信息数据统一函数 (i.e, for stuff related to commands). */
int send_txt(ship_client_t *c, const char *fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
            /* The NTE will crash if it gets the standard version of this
               packet, so fake it in the easiest way possible... */
            if(c->flags & CLIENT_FLAG_IS_NTE) {
                rv = send_dcnte_txt(c, fmt, args);
                break;
            }

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            rv = send_dc_message(c, TEXT_MSG_TYPE, fmt, args);
            break;

        case CLIENT_VERSION_BB:
            rv = send_bb_message(c, TEXT_MSG_TYPE, fmt, args);
            break;
    }

    va_end(args);

    return rv;
}

/* Send a packet to the client indicating information about the game they're
   joining. */
static int send_dcnte_game_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    dcnte_game_join_pkt *pkt = (dcnte_game_join_pkt *)sendbuf;
    int clients = 0, i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear it out first. */
    memset(pkt, 0, sizeof(dcnte_game_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = GAME_JOIN_TYPE;
    pkt->hdr.pkt_len = LE16(sizeof(dcnte_game_join_pkt));
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;

    /* Fill in the variations array. */
    for(i = 0; i < 0x20; ++i)
        pkt->maps[i] = LE32(l->maps[i]);

    for(i = 0; i < 4; ++i) {
        if(l->clients[i]) {
            /* Copy the player's data into the packet. */
            pkt->players[i].player_tag = LE32(0x00010000);
            pkt->players[i].guildcard = LE32(l->clients[i]->guildcard);
            pkt->players[i].ip_addr = 0xFFFFFFFF;
            pkt->players[i].client_id = LE32(i);

            if(l->clients[i]->version == CLIENT_VERSION_BB) {
                istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->players[i].name,
                               &l->clients[i]->pl->bb.character.name, 16, BB_CHARACTER_NAME_LENGTH);
            }
            else {
                memcpy(pkt->players[i].name, l->clients[i]->pl->v1.character.dress_data.guildcard_string, 16);
            }

            ++clients;
        }
    }

    /* Copy the client count over. */
    pkt->hdr.flags = (uint8_t)clients;

    /* 加密并发送 */
    return crypt_send(c, sizeof(dcnte_game_join_pkt), sendbuf);
}

static int send_dc_game_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    dc_game_join_pkt *pkt = (dc_game_join_pkt *)sendbuf;
    int clients = 0, i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear it out first. */
    memset(pkt, 0, DC_GAME_JOIN_LENGTH);

    /* Fill in the basics. */
    pkt->hdr.pkt_type = GAME_JOIN_TYPE;
    pkt->hdr.pkt_len = LE16(DC_GAME_JOIN_LENGTH);
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->difficulty = l->difficulty;
    pkt->battle = l->battle;
    pkt->event = l->event;
    pkt->section = l->section;
    pkt->challenge = l->challenge;
    pkt->rand_seed = LE32(l->rand_seed);

    /* Fill in the variations array. */
    for(i = 0; i < 0x20; ++i) {
        pkt->maps[i] = LE32(l->maps[i]);
    }

    for(i = 0; i < 4; ++i) {
        if(l->clients[i]) {
            /* Copy the player's data into the packet. */
            pkt->players[i].player_tag = LE32(0x00010000);
            pkt->players[i].guildcard = LE32(l->clients[i]->guildcard);
            pkt->players[i].ip_addr = 0xFFFFFFFF;
            pkt->players[i].client_id = LE32(i);

            if(l->clients[i]->version == CLIENT_VERSION_BB) {
                istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->players[i].name,
                               &l->clients[i]->pl->bb.character.name, 16, BB_CHARACTER_NAME_LENGTH);
            }
            else {
                memcpy(pkt->players[i].name, l->clients[i]->pl->v1.character.dress_data.guildcard_string, 16);
            }

            ++clients;
        }
    }

    /* Copy the client count over. */
    pkt->hdr.flags = (uint8_t)clients;

    /* 加密并发送 */
    return crypt_send(c, DC_GAME_JOIN_LENGTH, sendbuf);
}

static int send_pc_game_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    pc_game_join_pkt *pkt = (pc_game_join_pkt *)sendbuf;
    int clients = 0, i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear it out first. */
    memset(pkt, 0, sizeof(pc_game_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = GAME_JOIN_TYPE;
    pkt->hdr.pkt_len = LE16(sizeof(pc_game_join_pkt));
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->difficulty = l->difficulty;
    pkt->battle = l->battle;
    pkt->event = l->event;
    pkt->section = l->section;
    pkt->challenge = l->challenge;
    pkt->rand_seed = LE32(l->rand_seed);

    /* Fill in the variations array. */
    for(i = 0; i < 0x20; ++i) {
        pkt->maps[i] = LE32(l->maps[i]);
    }

    for(i = 0; i < 4; ++i) {
        if(l->clients[i]) {
            /* Copy the player's data into the packet. */
            pkt->players[i].player_tag = LE32(0x00010000);
            pkt->players[i].guildcard = LE32(l->clients[i]->guildcard);
            pkt->players[i].ip_addr = 0xFFFFFFFF;
            pkt->players[i].client_id = LE32(i);

            if(l->clients[i]->version == CLIENT_VERSION_BB) {
                memcpy(pkt->players[i].name,
                       &l->clients[i]->pl->bb.character.name.char_name[0], BB_CHARACTER_CHAR_NAME_WLENGTH);
                pkt->players[i].name[14] = 0;
                pkt->players[i].name[15] = 0;
            }
            else {
                istrncpy(ic_8859_to_utf16, (char *)pkt->players[i].name,
                         l->clients[i]->pl->v1.character.dress_data.guildcard_string, 32);
            }

            ++clients;
        }
    }

    /* Copy the client count over. */
    pkt->hdr.flags = (uint8_t)clients;

    /* 加密并发送 */
    return crypt_send(c, sizeof(pc_game_join_pkt), sendbuf);
}

static int send_gc_game_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    gc_game_join_pkt *pkt = (gc_game_join_pkt *)sendbuf;
    int clients = 0, i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear it out first. */
    memset(pkt, 0, GC_GAME_JOIN_LENGTH);

    /* Fill in the basics. */
    pkt->hdr.pkt_type = GAME_JOIN_TYPE;
    pkt->hdr.pkt_len = LE16(GC_GAME_JOIN_LENGTH);
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->difficulty = l->difficulty;
    pkt->battle = l->battle;
    pkt->event = l->event;
    pkt->section = l->section;
    pkt->challenge = l->challenge;
    pkt->rand_seed = LE32(l->rand_seed);
    pkt->episode = l->episode;
    pkt->one2 = 1;

    /* Fill in the variations array. */
    for(i = 0; i < 0x20; ++i) {
        pkt->maps[i] = LE32(l->maps[i]);
    }

    for(i = 0; i < 4; ++i) {
        if(l->clients[i]) {
            /* Copy the player's data into the packet. */
            pkt->players[i].player_tag = LE32(0x00010000);
            pkt->players[i].guildcard = LE32(l->clients[i]->guildcard);
            pkt->players[i].ip_addr = 0xFFFFFFFF;
            pkt->players[i].client_id = LE32(i);

            if(l->clients[i]->version == CLIENT_VERSION_BB) {
                istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->players[i].name,
                               &l->clients[i]->pl->bb.character.name, 16, BB_CHARACTER_NAME_LENGTH);
            }
            else {
                memcpy(pkt->players[i].name, l->clients[i]->pl->v1.character.dress_data.guildcard_string, 16);
            }

            ++clients;
        }
    }

    /* Copy the client count over. */
    pkt->hdr.flags = (uint8_t)clients;

    /* 加密并发送 */
    return crypt_send(c, GC_GAME_JOIN_LENGTH, sendbuf);
}

static int send_xbox_game_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    xb_game_join_pkt *pkt = (xb_game_join_pkt *)sendbuf;
    int clients = 0, i;
    ship_client_t *cl;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear it out first. */
    memset(pkt, 0, sizeof(xb_game_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = GAME_JOIN_TYPE;
    pkt->hdr.pkt_len = LE16(sizeof(xb_game_join_pkt));
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->difficulty = l->difficulty;
    pkt->battle = l->battle;
    pkt->event = l->event;
    pkt->section = l->section;
    pkt->challenge = l->challenge;
    pkt->rand_seed = LE32(l->rand_seed);
    pkt->episode = l->episode;
    pkt->one2 = 0;

    /* Fill in the variations array. */
    for(i = 0; i < 0x20; ++i) {
        pkt->maps[i] = LE32(l->maps[i]);
    }

    for(i = 0; i < 4; ++i) {
        if(l->clients[i]) {
            cl = (ship_client_t *)l->clients[i];

            /* Copy the player's data into the packet. */
            pkt->players[i].player_tag = LE32(0x00010000);
            pkt->players[i].guildcard = LE32(cl->guildcard);

            if(cl->version == CLIENT_VERSION_XBOX) {
                memcpy(&pkt->players[i].xbox_ip, cl->xbl_ip,
                       sizeof(xbox_ip_t));
            } else {
                /* TODO: Not sure how we should do this for non-xb clients... */
                memset(&pkt->players[i].xbox_ip, 0, sizeof(xbox_ip_t));
                pkt->players[i].xbox_ip.lan_ip = 0x12345678;
                pkt->players[i].xbox_ip.wan_ip = 0x87654321;
                pkt->players[i].xbox_ip.port = 1234;
                memset(&pkt->players[i].xbox_ip.mac_addr, 0x12, 6);
                pkt->players[i].xbox_ip.sg_addr = 0x90807060;
                pkt->players[i].xbox_ip.sg_session_id = 0x12345678;
            }

            /* TODO: I have no idea what these three things are. */
            pkt->players[i].d1 = 0x01010101;
            pkt->players[i].d2 = 0x01010101;
            pkt->players[i].d3 = LE32(1);

            pkt->players[i].client_id = LE32(i);

            if(l->clients[i]->version == CLIENT_VERSION_BB) {
                istrncpy16_raw(ic_utf16_to_gbk/*尝试GBK*/, pkt->players[i].name,
                               &cl->pl->bb.character.name, 16, BB_CHARACTER_NAME_LENGTH);
            }
            else {
                memcpy(pkt->players[i].name, cl->pl->v1.character.dress_data.guildcard_string, 16);
            }

            ++clients;
        }
    }

    /* Copy the client count over. */
    pkt->hdr.flags = (uint8_t)clients;

    /* 加密并发送 */
    return crypt_send(c, sizeof(xb_game_join_pkt), sendbuf);
}

static int send_ep3_game_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    ep3_game_join_pkt *pkt = (ep3_game_join_pkt *)sendbuf;
    int clients = 0, i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear it out first. */
    memset(pkt, 0, EP3_GAME_JOIN_LENGTH);

    /* Fill in the basics. */
    pkt->hdr.pkt_type = GAME_JOIN_TYPE;
    pkt->hdr.pkt_len = LE16(EP3_GAME_JOIN_LENGTH);
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->difficulty = 0;
    pkt->battle = l->battle;
    pkt->event = l->event;
    pkt->section = l->section;
    pkt->challenge = 0;
    pkt->rand_seed = LE32(l->rand_seed);
    pkt->episode = 1;
    pkt->one2 = 0;

    /* Fill in the variations array? */
    //memcpy(pkt->maps, l->maps, 0x20 * 4);

    for(i = 0; i < 4; ++i) {
        if(l->clients[i]) {
            /* Copy the player's data into the packet. */
            pkt->players[i].player_tag = LE32(0x00010000);
            pkt->players[i].guildcard = LE32(l->clients[i]->guildcard);
            pkt->players[i].ip_addr = 0xFFFFFFFF;
            pkt->players[i].client_id = LE32(i);

            /* No need to iconv the names, they'll be good as is */
            memcpy(pkt->players[i].name, l->clients[i]->pl->v1.character.dress_data.guildcard_string, 16);

            /* Copy the player data to that part of the packet. */
            memcpy(&pkt->player_data[i], &l->clients[i]->pl->v1,
                   sizeof(v1_player_t));

            ++clients;
        }
    }

    /* Copy the client count over. */
    pkt->hdr.flags = (uint8_t)clients;

    /* 加密并发送 */
    return crypt_send(c, EP3_GAME_JOIN_LENGTH, sendbuf);
}

static int send_bb_game_join(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    bb_game_join_pkt *pkt = (bb_game_join_pkt *)sendbuf;
    int clients = 0, i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear it out first. */
    memset(pkt, 0, sizeof(bb_game_join_pkt));

    /* Fill in the basics. */
    pkt->hdr.pkt_type = LE16(GAME_JOIN_TYPE);
    pkt->hdr.pkt_len = LE16(sizeof(bb_game_join_pkt));
    pkt->client_id = c->client_id;
    pkt->leader_id = l->leader_id;
    pkt->disable_udp = 1;
    pkt->difficulty = l->difficulty;
    pkt->battle = l->battle;
    pkt->event = l->event;
    pkt->section = l->section;
    pkt->challenge = l->challenge;
    pkt->rand_seed = LE32(l->rand_seed);
    pkt->episode = l->episode;
    pkt->one2 = 1;
    pkt->single_player = (l->flags & LOBBY_FLAG_SINGLEPLAYER) ? 1 : 0;
    pkt->unused = 0;

    /* Fill in the variations array. */
    for(i = 0; i < 0x20; ++i) {
        pkt->maps[i] = LE32(l->maps[i]);
    }

    for(i = 0; i < 4; ++i) {
        if(l->clients[i]) {
            /* Copy the player's data into the packet. */
            pkt->players[i].player_tag = LE32(0x00010000);
            pkt->players[i].guildcard = LE32(l->clients[i]->guildcard);
            pkt->players[i].client_id = LE32(i);

            /* Convert the name to UTF-16. */
            if(l->clients[i]->version == CLIENT_VERSION_BB) {
                memcpy(pkt->players[i].name,
                       &l->clients[i]->pl->bb.character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
            }
            else {
                istrncpy(ic_8859_to_utf16, (char *)pkt->players[i].name,
                         l->clients[i]->pl->v1.character.dress_data.guildcard_string, 32);
            }

            ++clients;
        }
    }

    /* Copy the client count over. */
    pkt->hdr.flags = LE32(clients);

    /* 加密并发送 */
    return crypt_send(c, sizeof(bb_game_join_pkt), sendbuf);
}

int send_game_join(ship_client_t *c, lobby_t *l) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
            if(c->flags & CLIENT_FLAG_IS_NTE)
                return send_dcnte_game_join(c, l);

        case CLIENT_VERSION_DCV2:
            return send_dc_game_join(c, l);

        case CLIENT_VERSION_PC:
            return send_pc_game_join(c, l);

        case CLIENT_VERSION_GC:
            return send_gc_game_join(c, l);

        case CLIENT_VERSION_XBOX:
            return send_xbox_game_join(c, l);

        case CLIENT_VERSION_EP3:
            return send_ep3_game_join(c, l);

        case CLIENT_VERSION_BB:
            return send_bb_game_join(c, l);
    }

    return -1;
}

/* Send a packet to a client giving them the list of games on the block. */
static int send_dc_game_list(ship_client_t *c, block_t *b) {
    uint8_t *sendbuf = get_sendbuf();
    dc_game_list_pkt *pkt = (dc_game_list_pkt *)sendbuf;
    int entries = 1, len = 0x20;
    lobby_t *l;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear out the packet and the first entry */
    memset(pkt, 0, 0x20);

    /* 填充数据头 */
    pkt->hdr.pkt_type = GAME_LIST_TYPE;

    /* Fill in the first entry */
    pkt->entries[0].menu_id = MENU_ID_LOBBY;
    pkt->entries[0].item_id = 0xFFFFFFFF;
    pkt->entries[0].flags = 0x04;
    strcpy(pkt->entries[0].name, b->ship->cfg->name);

    pthread_rwlock_rdlock(&b->lobby_lock);

    TAILQ_FOREACH(l, &b->lobbies, qentry) {
        /* Lock the lobby */
        pthread_mutex_lock(&l->mutex);

        /* Ignore default lobbies and Gamecube/Blue Burst games */
        if(l->type != LOBBY_TYPE_GAME || l->episode) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Don't show v2-only lobbies to v1 players */
        if(c->version == CLIENT_VERSION_DCV1 && l->v2) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Don't show v1-only lobbies to v2 players */
        if(c->version == CLIENT_VERSION_DCV2 &&
           (l->flags & LOBBY_FLAG_V1ONLY)) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Don't show pc-only lobbies to dc players */
        if((l->flags & LOBBY_FLAG_PCONLY)) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Don't bother showing single-player lobbies */
        if((l->flags & LOBBY_FLAG_SINGLEPLAYER)) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Is the client on the NTE? If not, don't show NTE teams. If they
           are on the NTE, then only show NTE teams. */
        if(!(c->flags & CLIENT_FLAG_IS_NTE)) {
            /* Don't show DC NTE teams... */
            if((l->flags & LOBBY_FLAG_NTE)) {
                pthread_mutex_unlock(&l->mutex);
                continue;
            }
        }
        else {
            /* Only show DC NTE teams... */
            if(!(l->flags & LOBBY_FLAG_NTE)) {
                pthread_mutex_unlock(&l->mutex);
                continue;
            }
        }

        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x1C);

        /* Copy the lobby's data to the packet */
        pkt->entries[entries].menu_id = LE32(MENU_ID_GAME);
        pkt->entries[entries].item_id = LE32(l->lobby_id);
        pkt->entries[entries].difficulty = 0x22 + l->difficulty;
        pkt->entries[entries].players = l->num_clients;
        pkt->entries[entries].flags = (l->challenge ? 0x20 : 0x00) |
            (l->battle ? 0x10 : 0x00) | (l->passwd[0] ? 2 : 0) |
            (l->v2 ? 0x40 : 0x00);

        /* Copy the name. These are in UTF-8, and need converting... */
        if(l->name[1] == 'J') {
            istrncpy(ic_gbk_to_sjis, pkt->entries[entries].name, l->name, 16);
        }
        else {
            istrncpy(ic_gbk_to_8859, pkt->entries[entries].name, l->name, 16);
        }

        /* Unlock the lobby */
        pthread_mutex_unlock(&l->mutex);

        /* Update the counters */
        ++entries;
        len += 0x1C;
    }

    pthread_rwlock_unlock(&b->lobby_lock);

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries - 1;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_game_list(ship_client_t *c, block_t *b) {
    uint8_t *sendbuf = get_sendbuf();
    pc_game_list_pkt *pkt = (pc_game_list_pkt *)sendbuf;
    int entries = 1, len = 0x30;
    lobby_t *l;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear out the packet and the first entry */
    memset(pkt, 0, 0x30);

    /* 填充数据头 */
    pkt->hdr.pkt_type = GAME_LIST_TYPE;

    /* Fill in the first entry */
    pkt->entries[0].menu_id = MENU_ID_LOBBY;
    pkt->entries[0].item_id = 0xFFFFFFFF;
    pkt->entries[0].flags = 0x04;

    istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[0].name, ship->cfg->name,
             0x20);

    pthread_rwlock_rdlock(&b->lobby_lock);

    TAILQ_FOREACH(l, &b->lobbies, qentry) {
        /* Lock the lobby */
        pthread_mutex_lock(&l->mutex);

        /* 忽略默认的大厅和GC BB的房间 Ignore default lobbies and Gamecube/Blue Burst games */
        if(l->type != LOBBY_TYPE_GAME || l->episode) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Don't show v1-only or dc-only lobbies */
        if((l->flags & LOBBY_FLAG_V1ONLY) || (l->flags & LOBBY_FLAG_DCONLY)) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Don't bother showing single-player lobbies */
        if((l->flags & LOBBY_FLAG_SINGLEPLAYER)) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Is the client on the NTE? If not, don't show NTE teams. If they
           are on the NTE, then only show NTE teams. */
        if(!(c->flags & CLIENT_FLAG_IS_NTE)) {
            /* Don't show NTE teams... */
            if((l->flags & LOBBY_FLAG_NTE)) {
                pthread_mutex_unlock(&l->mutex);
                continue;
            }
        }
        else {
            /* Only show PC NTE teams... */
            if(!(l->flags & LOBBY_FLAG_NTE) ||
               l->version != CLIENT_VERSION_PC) {
                pthread_mutex_unlock(&l->mutex);
                continue;
            }
        }

        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x2C);

        /* Copy the lobby's data to the packet */
        pkt->entries[entries].menu_id = LE32(MENU_ID_GAME);
        pkt->entries[entries].item_id = LE32(l->lobby_id);
        pkt->entries[entries].difficulty = 0x22 + l->difficulty;
        pkt->entries[entries].players = l->num_clients;
        pkt->entries[entries].v2 = l->v2;
        pkt->entries[entries].flags = (l->challenge ? 0x20 : 0x00) |
            (l->battle ? 0x10 : 0x00) | (l->passwd[0] ? 2 : 0) |
            (l->v2 ? 0x40 : 0x00);

        /* Copy the name. For some unknown reason, PSOPC needs two NULs at the
           end of names, or something... */
        istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name, l->name,
                 0x1C);

        /* Unlock the lobby */
        pthread_mutex_unlock(&l->mutex);

        /* Update the counters */
        ++entries;
        len += 0x2C;
    }

    pthread_rwlock_unlock(&b->lobby_lock);

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries - 1;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_gc_game_list(ship_client_t *c, block_t *b) {
    uint8_t *sendbuf = get_sendbuf();
    dc_game_list_pkt *pkt = (dc_game_list_pkt *)sendbuf;
    int entries = 1, len = 0x20;
    lobby_t *l;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear out the packet and the first entry */
    memset(pkt, 0, 0x20);

    /* 填充数据头 */
    pkt->hdr.pkt_type = GAME_LIST_TYPE;

    /* Fill in the first entry */
    pkt->entries[0].menu_id = MENU_ID_LOBBY;
    pkt->entries[0].item_id = 0xFFFFFFFF;
    pkt->entries[0].flags = 0x04;
    strcpy(pkt->entries[0].name, b->ship->cfg->name);

    pthread_rwlock_rdlock(&b->lobby_lock);

    TAILQ_FOREACH(l, &b->lobbies, qentry) {
        /* Lock the lobby */
        pthread_mutex_lock(&l->mutex);

        /* Ignore default lobbies */
        if(l->type != LOBBY_TYPE_GAME) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Ignore Blue Burst lobbies */
        if(l->version == CLIENT_VERSION_BB) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Ignore DC/PC games if the user hasn't set the flag to show them or
           the lobby doesn't have the right flag set */
        if(!l->episode && (!(c->flags & CLIENT_FLAG_SHOW_DCPC_ON_GC) ||
                           !(l->flags & LOBBY_FLAG_GC_ALLOWED))) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Don't bother showing single-player lobbies */
        if((l->flags & LOBBY_FLAG_SINGLEPLAYER)) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Don't show DC or PC NTE teams... */
        if((l->flags & LOBBY_FLAG_NTE)) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x1C);

        /* Copy the lobby's data to the packet */
        pkt->entries[entries].menu_id = LE32(MENU_ID_GAME);
        pkt->entries[entries].item_id = LE32(l->lobby_id);
        pkt->entries[entries].difficulty = 0x22 + l->difficulty;
        pkt->entries[entries].players = l->num_clients;
        pkt->entries[entries].flags = ((l->episode <= 1) ? 0x40 : 0x80) |
            (l->challenge ? 0x20 : 0x00) | (l->battle ? 0x10 : 0x00) |
            (l->passwd[0] ? 0x02 : 0x00);

        /* Copy the name. These are in UTF-8, and need converting... */
        if(l->name[1] == 'J') {
            istrncpy(ic_gbk_to_sjis, pkt->entries[entries].name, l->name, 16);
        }
        else {
            istrncpy(ic_gbk_to_8859, pkt->entries[entries].name, l->name, 16);
        }

        /* Unlock the lobby */
        pthread_mutex_unlock(&l->mutex);

        /* Update the counters */
        ++entries;
        len += 0x1C;
    }

    pthread_rwlock_unlock(&b->lobby_lock);

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries - 1;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_ep3_game_list(ship_client_t *c, block_t *b) {
    uint8_t *sendbuf = get_sendbuf();
    dc_game_list_pkt *pkt = (dc_game_list_pkt *)sendbuf;
    int entries = 1, len = 0x20;
    lobby_t *l;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear out the packet and the first entry */
    memset(pkt, 0, 0x20);

    /* 填充数据头 */
    pkt->hdr.pkt_type = GAME_LIST_TYPE;

    /* Fill in the first entry */
    pkt->entries[0].menu_id = MENU_ID_LOBBY;
    pkt->entries[0].item_id = 0xFFFFFFFF;
    pkt->entries[0].flags = 0x04;
    strcpy(pkt->entries[0].name, b->ship->cfg->name);

    pthread_rwlock_rdlock(&b->lobby_lock);

    TAILQ_FOREACH(l, &b->lobbies, qentry) {
        /* Lock the lobby */
        pthread_mutex_lock(&l->mutex);

        /* Ignore non-Episode 3 and default lobbies */
        if(l->type != LOBBY_TYPE_EP3_GAME) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x1C);

        /* Copy the lobby's data to the packet */
        pkt->entries[entries].menu_id = LE32(MENU_ID_GAME);
        pkt->entries[entries].item_id = LE32(l->lobby_id);
        pkt->entries[entries].difficulty = 0x22 + l->difficulty;
        pkt->entries[entries].players = l->num_clients;
        pkt->entries[entries].flags = ((l->episode <= 1) ? 0x40 : 0x80) |
            (l->challenge ? 0x20 : 0x00) | (l->battle ? 0x10 : 0x00) |
            (l->passwd[0] ? 0x02 : 0x00);

        /* Copy the name. These are in UTF-8, and need converting... */
        if(l->name[1] == 'J') {
            istrncpy(ic_gbk_to_sjis, pkt->entries[entries].name, l->name, 16);
        }
        else {
            istrncpy(ic_gbk_to_8859, pkt->entries[entries].name, l->name, 16);
        }

        /* Unlock the lobby */
        pthread_mutex_unlock(&l->mutex);

        /* Update the counters */
        ++entries;
        len += 0x1C;
    }

    pthread_rwlock_unlock(&b->lobby_lock);

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries - 1;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_game_list(ship_client_t *c, block_t *b) {
    uint8_t *sendbuf = get_sendbuf();
    bb_game_list_pkt *pkt = (bb_game_list_pkt *)sendbuf;
    int entries = 1, len = 0x34;
    lobby_t *l;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear out the packet and the first entry */
    memset(pkt, 0, 0x34);

    /* 填充数据头 */
    pkt->hdr.pkt_type = GAME_LIST_TYPE;

    /* Fill in the first entry */
    pkt->entries[0].menu_id = MENU_ID_LOBBY;
    pkt->entries[0].item_id = 0xFFFFFFFF;
    pkt->entries[0].flags = 0x04;

    istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[0].name, ship->cfg->name,
             0x20);

    pthread_rwlock_rdlock(&b->lobby_lock);

    TAILQ_FOREACH(l, &b->lobbies, qentry) {
        /* Lock the lobby */
        pthread_mutex_lock(&l->mutex);

        /* 忽略默认的大厅和非BB游戏的房间 Ignore default lobbies and non-bb games */
        if(l->type != LOBBY_TYPE_GAME || l->version != CLIENT_VERSION_BB) {
            pthread_mutex_unlock(&l->mutex);
            continue;
        }

        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x2C);

        /* Copy the lobby's data to the packet */
        pkt->entries[entries].menu_id = LE32(MENU_ID_GAME);
        pkt->entries[entries].item_id = LE32(l->lobby_id);
        pkt->entries[entries].difficulty = 0x22 + l->difficulty;
        pkt->entries[entries].players = l->num_clients;
        pkt->entries[entries].episode = (l->max_clients << 4) | l->episode;
        pkt->entries[entries].flags = (l->challenge ? 0x20 : 0x00) |
            (l->battle ? 0x10 : 0x00) | (l->passwd[0] ? 2 : 0) |
            ((l->flags & LOBBY_FLAG_SINGLEPLAYER) ? 0x04 : 0x00);

        /* Copy the name */
        istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name, l->name,
                 0x20);

        /* Unlock the lobby */
        pthread_mutex_unlock(&l->mutex);

        /* Update the counters */
        ++entries;
        len += 0x2C;
    }

    pthread_rwlock_unlock(&b->lobby_lock);

    /* Fill in the rest of the header */
    pkt->hdr.flags = LE32(entries - 1);
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

int send_game_list(ship_client_t *c, block_t *b) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            return send_dc_game_list(c, b);

        case CLIENT_VERSION_PC:
            return send_pc_game_list(c, b);

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            return send_gc_game_list(c, b);

        case CLIENT_VERSION_EP3:
            return send_ep3_game_list(c, b);

        case CLIENT_VERSION_BB:
            return send_bb_game_list(c, b);
    }

    return -1;
}

/* Send the list of lobby info items to the client. */
static int send_dc_info_list(ship_client_t *c, ship_t *s, uint32_t v) {
    uint8_t *sendbuf = get_sendbuf();
    dc_block_list_pkt *pkt = (dc_block_list_pkt *)sendbuf;
    int i, len = 0x20, entries = 1;
    uint32_t lang = (1 << c->q_lang);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the base packet */
    memset(pkt, 0, sizeof(dc_block_list_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;
    istrncpy(ic_gbk_to_utf8, pkt->entries[0].name, s->cfg->name,
        0x10);
    //strncpy(pkt->entries[0].name, s->cfg->name, 0x10);

    /* Add what's needed at the end */
    pkt->entries[0].name[0x0F] = 0x00;
    pkt->entries[0].name[0x10] = 0x08;
    pkt->entries[0].name[0x11] = 0x00;

    /* Add each info item to the list. */
    for(i = 0; i < s->cfg->info_file_count; ++i) {
        if(!(s->cfg->info_files[i].versions & v)) {
            continue;
        }

        if(!(s->cfg->info_files[i].languages & lang)) {
            continue;
        }

        /* Skip MOTD entries. */
        if(!(s->cfg->info_files[i].desc)) {
            continue;
        }

        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x1C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(MENU_ID_INFODESK);
        pkt->entries[entries].item_id = LE32(i);
        pkt->entries[entries].flags = LE16(0x0000);

        /* These are always ASCII, so this is fine */
        istrncpy(ic_gbk_to_utf8, pkt->entries[entries].name, s->cfg->info_files[i].desc,
            0x11);
        //strncpy(pkt->entries[entries].name, s->cfg->info_files[i].desc, 0x11);
        pkt->entries[entries].name[0x11] = 0;

        len += 0x1C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_info_list(ship_client_t *c, ship_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    pc_block_list_pkt *pkt = (pc_block_list_pkt *)sendbuf;
    int i, len = 0x30, entries = 1;
    uint32_t lang = (1 << c->q_lang);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the base packet */
    memset(pkt, 0, 0x30);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;

    istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[0].name, s->cfg->name,
             0x20);

    /* Add each info item to the list. */
    for(i = 0; i < s->cfg->info_file_count; ++i) {
        if(!(s->cfg->info_files[i].versions & PSOCN_INFO_PC)) {
            continue;
        }

        if(!(s->cfg->info_files[i].languages & lang)) {
            continue;
        }

        /* Skip MOTD entries. */
        if(!(s->cfg->info_files[i].desc)) {
            continue;
        }

        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x2C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(MENU_ID_INFODESK);
        pkt->entries[entries].item_id = LE32(i);
        pkt->entries[entries].flags = LE16(0x0000);

        istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name,
                 s->cfg->info_files[i].desc, 0x20);

        len += 0x2C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_info_list(ship_client_t* c, ship_t* s) {
    uint8_t* sendbuf = get_sendbuf();
    bb_block_list_pkt* pkt = (bb_block_list_pkt*)sendbuf;
    int i, len = 0x30, entries = 1;
    uint32_t lang = (1 << c->q_lang);

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Clear the base packet */
    memset(pkt, 0, 0x30);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;

    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[0].name, s->cfg->name,
        0x20);

    /* Add each info item to the list. */
    for (i = 0; i < s->cfg->info_file_count; ++i) {
        if (!(s->cfg->info_files[i].versions & PSOCN_INFO_BB)) {
            continue;
        }

        if (!(s->cfg->info_files[i].languages & lang)) {
            continue;
        }

        /* Skip MOTD entries. */
        if (!(s->cfg->info_files[i].desc)) {
            continue;
        }

        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x2C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(MENU_ID_INFODESK);
        pkt->entries[entries].item_id = LE32(i);
        pkt->entries[entries].flags = LE16(0x0000);

        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name,
            s->cfg->info_files[i].desc, 0x20);

        len += 0x2C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

int send_info_list(ship_client_t *c, ship_t *s) {
    /* Call the appropriate function */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
            return send_dc_info_list(c, s, PSOCN_INFO_V1);

        case CLIENT_VERSION_DCV2:
            return send_dc_info_list(c, s, PSOCN_INFO_V2);

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            /* Don't bother if they can't see the results anyway... */
            if(!(c->flags & CLIENT_FLAG_GC_MSG_BOXES))
                return 0;

            return send_dc_info_list(c, s, PSOCN_INFO_GC);

        case CLIENT_VERSION_PC:
            return send_pc_info_list(c, s);

        case CLIENT_VERSION_BB:
            return send_bb_info_list(c, s);
    }

    return -1;
}

/* 这是PSOPC信息选择菜单的特例。这允许用户选择是否创建与V1兼容的游戏. */
int send_pc_game_type_sel(ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    pc_block_list_pkt *pkt = (pc_block_list_pkt *)sendbuf;
    uint32_t count = 0, len = 0x100, len2 = 0x1C, len3 = 0x20;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* 初始化数据包 */
    memset(pkt, 0, len);

    /* 填充菜单实例 */
    for (count; count < _countof(pso_game_create_menu_pc); ++count) {
        pkt->entries[count].menu_id = LE32(pso_game_create_menu_pc[count]->menu_id);
        pkt->entries[count].item_id = LE32(pso_game_create_menu_pc[count]->item_id);
        pkt->entries[count].flags = LE16(pso_game_create_menu_pc[count]->flag);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, pso_game_create_menu_pc[count]->name, len3);
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(LOBBY_INFO_TYPE);
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = count - 1;

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

/* 这是PSOBB信息选择菜单的特例。这允许用户选择是否创建与V1兼容的游戏. */
int send_bb_game_create(ship_client_t* c) {
    uint8_t* sendbuf = get_sendbuf();
    bb_game_create_pkt* pkt = (bb_game_create_pkt*)sendbuf;
    int i = 0, len = 0x58, entries = 1;
    DBG_LOG("创建房间");

    /* 填充数据头 */
    pkt->hdr.pkt_type = GAME_CREATE_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = 3;

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

int send_bb_game_type_sel(ship_client_t* c) {
    uint8_t* sendbuf = get_sendbuf();
    bb_block_list_pkt* pkt = (bb_block_list_pkt*)sendbuf;
    uint32_t count = 0, len = 0x100, len2 = 0x1C, len3 = 0x20;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* 初始化数据包 */
    memset(pkt, 0, len);

    /* 填充菜单实例 */
    for (count; count < _countof(pso_game_create_menu_bb); ++count) {
        pkt->entries[count].menu_id = LE32(pso_game_create_menu_bb[count]->menu_id);
        pkt->entries[count].item_id = LE32(pso_game_create_menu_bb[count]->item_id);
        pkt->entries[count].flags = LE16(pso_game_create_menu_bb[count]->flag);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, pso_game_create_menu_bb[count]->name, len3);
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(LOBBY_INFO_TYPE);
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = count - 1;

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

int send_bb_game_game_drop_set(ship_client_t* c) {
    uint8_t* sendbuf = get_sendbuf();
    bb_block_list_pkt* pkt = (bb_block_list_pkt*)sendbuf;
    uint32_t count = 0, len = 0x100, len2 = 0x1C, len3 = 0x20;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* 初始化数据包 */
    memset(pkt, 0, len);

    /* 填充菜单实例 */
    for (count; count < _countof(pso_game_drop_menu_bb); ++count) {
        pkt->entries[count].menu_id = LE32(pso_game_drop_menu_bb[count]->menu_id);
        pkt->entries[count].item_id = LE32(pso_game_drop_menu_bb[count]->item_id);
        pkt->entries[count].flags = LE16(pso_game_drop_menu_bb[count]->flag);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, pso_game_drop_menu_bb[count]->name, len3);
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(LOBBY_INFO_TYPE);
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = count - 1;

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

/* Send a message to the client. */
static int send_dc_message_box(ship_client_t *c, const char *fmt,
                               va_list args) {
    uint8_t *sendbuf = get_sendbuf();
    dc_msg_box_pkt *pkt = (dc_msg_box_pkt *)sendbuf;
    int len;
    iconv_t ic;
    size_t in, out;
    char *inptr;
    char *outptr;
    char tm[514];

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Don't send these to GC players with buggy versions. */
    if((c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
        c->version == CLIENT_VERSION_XBOX) &&
       !(c->flags & CLIENT_FLAG_TYPE_SHIP) &&
       !(c->flags & CLIENT_FLAG_GC_MSG_BOXES)) {
        SHIPS_LOG("Silently (to the user) dropping message box for GC");
        return 0;
    }

    /* Do the formatting */
    vsnprintf(tm, 512, fmt, args);
    tm[511] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if(tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    /* Set up to convert between encodings */
    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        if(tm[1] == 'J') {
            ic = ic_gbk_to_sjis;
        }
        else {
            ic = ic_gbk_to_8859;
        }
    }
    else {
        ic = ic_gbk_to_utf16;
    }

    /* Convert the message to the appropriate encoding. */
    out = 65500;
    inptr = tm;
    outptr = (char *)pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);
    len = 65500 - out;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* 填充数据头 */
    len += 0x04;

    if(c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
       c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = MSG_BOX_TYPE;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(len);
    }
    else {
        pkt->hdr.pc.pkt_type = MSG_BOX_TYPE;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(len);
    }

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

/* Send a message to the client. */
static int send_message_box(ship_client_t* c, const char* fmt,
    va_list args) {
    uint8_t* sendbuf = get_sendbuf();
    msg_box_pkt* pkt = (msg_box_pkt*)sendbuf;
    int len;
    iconv_t ic;
    size_t in, out;
    char* inptr;
    char* outptr;
    char tm[4096] = { 0 };

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Don't send these to GC players with buggy versions. */
    if ((c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
        c->version == CLIENT_VERSION_XBOX) &&
        !(c->flags & CLIENT_FLAG_TYPE_SHIP) &&
        !(c->flags & CLIENT_FLAG_GC_MSG_BOXES)) {
        SHIPS_LOG("Silently (to the user) dropping message box for GC");
        return 0;
    }

    memset(&tm[0], 0, sizeof(tm));

    /* Do the formatting */
    vsnprintf(tm, 4096, fmt, args);
    tm[4095] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if (tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    /* Set up to convert between encodings */
    if (c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
        c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
        c->version == CLIENT_VERSION_XBOX) {
        if (tm[1] == 'J') {
            ic = ic_gbk_to_sjis;
        }
        else {
            ic = ic_gbk_to_8859;
        }
    }
    else {
        ic = ic_gbk_to_utf16;
    }

    /* Convert the message to the appropriate encoding. */
    in = strlen(tm) + 1;
    out = 65500;
    inptr = tm;
    outptr = (char*)pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);
    len = 65500 - out;

    /* Add any padding needed */
    while (len & 0x03) {
        pkt->msg[len++] = 0;
    }

    char_add_color_tag(pkt->msg);

    /* 填充数据头 */
    len += 0x04;

    if (c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
        c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
        c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = MSG_BOX_TYPE;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(len);
    }
    else if(c->version == CLIENT_VERSION_PC){
        pkt->hdr.pc.pkt_type = MSG_BOX_TYPE;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(len);
    }
    else {
        pkt->hdr.bb.pkt_type = MSG_BOX_TYPE;
        pkt->hdr.bb.flags = 0;
        pkt->hdr.bb.pkt_len = LE16(len);
    }

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

int send_msg_box(ship_client_t *c, const char *fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
        case CLIENT_VERSION_BB:
            rv = send_message_box(c, fmt, args);
    }

    va_end(args);

    return rv;
}

/* Send a message to the client. */
static int send_message_type(ship_client_t* c, uint16_t type, const char* fmt,
    va_list args) {
    uint8_t* sendbuf = get_sendbuf();
    msg_box_pkt* pkt = (msg_box_pkt*)sendbuf;
    int len;
    iconv_t ic;
    size_t in, out;
    char* inptr;
    char* outptr;
    char tm[4096] = { 0 };

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Don't send these to GC players with buggy versions. */
    if ((c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
        c->version == CLIENT_VERSION_XBOX) &&
        !(c->flags & CLIENT_FLAG_TYPE_SHIP) &&
        !(c->flags & CLIENT_FLAG_GC_MSG_BOXES)) {
        SHIPS_LOG("Silently (to the user) dropping message box for GC");
        return 0;
    }

    memset(&tm[0], 0, sizeof(tm));

    /* Do the formatting */
    vsnprintf(tm, 4096, fmt, args);
    tm[4095] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if (tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    /* Set up to convert between encodings */
    if (c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
        c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
        c->version == CLIENT_VERSION_XBOX) {
        if (tm[1] == 'J') {
            ic = ic_gbk_to_sjis;
        }
        else {
            ic = ic_gbk_to_8859;
        }
    }
    else {
        ic = ic_gbk_to_utf16;
    }

    /* Convert the message to the appropriate encoding. */
    in = strlen(tm) + 1;
    out = 65500;
    inptr = tm;
    outptr = (char*)pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);
    len = 65500 - out;

    /* Add any padding needed */
    while (len & (c->hdr_size - 1)) {
        pkt->msg[len++] = 0;
    }

    char_add_color_tag(pkt->msg);

    /* 填充数据头 */
    len += c->hdr_size;

    if (c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
        c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
        c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = (uint8_t)type;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(len);
    }
    else if (c->version == CLIENT_VERSION_PC) {
        pkt->hdr.pc.pkt_type = (uint8_t)type;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(len);
    }
    else {
        pkt->hdr.bb.pkt_type = type;
        pkt->hdr.bb.flags = 0;
        pkt->hdr.bb.pkt_len = LE16(len);
    }

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

/* 发送不同类型的信息数据统一函数 (i.e, type/ for stuff related to commands). */
int send_msg(ship_client_t* c, uint16_t type, const char* fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch (type)
    {
    case MSG_BOX_TYPE:
        rv = send_message_type(c, type, fmt, args);
        break;

    default:
        switch (c->version)
        {
        case CLIENT_VERSION_DCV1:
            /* The NTE will crash if it gets the standard version of this
               packet, so fake it in the easiest way possible... */
            if (c->flags & CLIENT_FLAG_IS_NTE) {
                rv = send_dcnte_txt(c, fmt, args);
                break;
            }

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            rv = send_dc_message(c, type, fmt, args);
            break;

        case CLIENT_VERSION_BB:
            rv = send_bb_message(c, type, fmt, args);
            break;
        }
        break;
    }

    va_end(args);

    return rv;
}

uint32_t check_special_quest_type(lobby_t* l) {
    uint32_t type = PSOCN_QUEST_NORMAL;

    if (l->battle)
        type = PSOCN_QUEST_BATTLE;

    if (l->challenge)
        type = PSOCN_QUEST_CHALLENGE;

    if (l->oneperson)
        type = PSOCN_QUEST_ONEPERSON;

    if (l->govorlab)
        type = PSOCN_QUEST_GOVERNMENT;

    return type;
}

/* Send the list of quest categories to the client. */
static int send_dc_quest_categories(ship_client_t *c, int lang) {
    uint8_t *sendbuf = get_sendbuf();
    dc_quest_list_pkt *pkt = (dc_quest_list_pkt *)sendbuf;
    int i, len = 0x04, entries = 0;
    size_t in, out;
    char *inptr;
    char *outptr;
    psocn_quest_list_t *qlist;
    lobby_t *l = c->cur_lobby;
    uint32_t type = check_special_quest_type(l);
    uint32_t episode = LE32(l->episode);

    if(l->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
       l->version == CLIENT_VERSION_XBOX)
        qlist = &ship->qlist[CLIENT_VERSION_GC][lang];
    else if(!l->v2)
        qlist = &ship->qlist[CLIENT_VERSION_DCV1][lang];
    else
        qlist = &ship->qlist[CLIENT_VERSION_DCV2][lang];

    /* Fall back to English if there's no list for this language... */
    if(!qlist->cat_count) {
        lang = CLIENT_LANG_ENGLISH;

        if(l->version == CLIENT_VERSION_GC ||
           c->version == CLIENT_VERSION_EP3 ||
           l->version == CLIENT_VERSION_XBOX)
            qlist = &ship->qlist[CLIENT_VERSION_GC][lang];
        else if(!l->v2)
            qlist = &ship->qlist[CLIENT_VERSION_DCV1][lang];
        else
            qlist = &ship->qlist[CLIENT_VERSION_DCV2][lang];
    }

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* 填充数据头 */
    pkt->hdr.pkt_type = QUEST_LIST_TYPE;

    for(i = 0; i < qlist->cat_count; ++i) {

        /* 确认为该章节的任务 */
        if(l->version == CLIENT_VERSION_GC)
            if (!(qlist->cats[i].episodes == episode))
                continue;

        /* Skip quests not of the right type. */
        if((qlist->cats[i].type & PSOCN_QUEST_TYPE_MASK) != type)
            continue;

        /* Only show debug entries to GMs. */
        if((qlist->cats[i].type & PSOCN_QUEST_DEBUG) && !LOCAL_GM(c))
            continue;

        /* Make sure the user's privilege level is good enough. */
        if((qlist->cats[i].privileges & c->privilege) !=
           qlist->cats[i].privileges && !LOCAL_GM(c))
            continue;

        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x98);

        /* Copy the category's information over to the packet */
        pkt->entries[entries].menu_id = LE32((MENU_ID_QCATEGORY |
                                              (lang << 24)));
        pkt->entries[entries].item_id = LE32(i);

        /* Convert the name and the description to the appropriate encoding */
        in = 32;
        out = 30;
        inptr = qlist->cats[i].name;
        outptr = &pkt->entries[entries].name[2];

        if(lang == CLIENT_LANG_JAPANESE) {
            iconv(ic_gbk_to_sjis, &inptr, &in, &outptr, &out);
            pkt->entries[entries].name[0] = '\t';
            pkt->entries[entries].name[1] = 'J';
        }
        else {
            iconv(ic_gbk_to_8859, &inptr, &in, &outptr, &out);
            pkt->entries[entries].name[0] = '\t';
            pkt->entries[entries].name[1] = 'E';
        }

        in = 112;
        out = 110;
        inptr = qlist->cats[i].desc;
        outptr = &pkt->entries[entries].desc[2];

        if(lang == CLIENT_LANG_JAPANESE) {
            iconv(ic_gbk_to_sjis, &inptr, &in, &outptr, &out);
            pkt->entries[entries].desc[0] = '\t';
            pkt->entries[entries].desc[1] = 'J';
        }
        else {
            iconv(ic_gbk_to_8859, &inptr, &in, &outptr, &out);
            pkt->entries[entries].desc[0] = '\t';
            pkt->entries[entries].desc[1] = 'E';
        }

        ++entries;
        len += 0x98;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_quest_categories(ship_client_t *c, int lang) {
    uint8_t *sendbuf = get_sendbuf();
    pc_quest_list_pkt *pkt = (pc_quest_list_pkt *)sendbuf;
    int i, len = 0x04, entries = 0;
    size_t in, out;
    char *inptr;
    char *outptr;
    psocn_quest_list_t *qlist;
    lobby_t *l = c->cur_lobby;
    uint32_t type = check_special_quest_type(l);

    if(!l->v2)
        qlist = &ship->qlist[CLIENT_VERSION_DCV1][lang];
    else
        qlist = &ship->qlist[CLIENT_VERSION_PC][lang];

    /* Fall back to English if there's no list for this language... */
    if(!qlist->cat_count) {
        lang = CLIENT_LANG_ENGLISH;

        if(!l->v2)
            qlist = &ship->qlist[CLIENT_VERSION_DCV1][lang];
        else
            qlist = &ship->qlist[CLIENT_VERSION_DCV2][lang];
    }

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* 填充数据头 */
    pkt->hdr.pkt_type = QUEST_LIST_TYPE;

    for(i = 0; i < qlist->cat_count; ++i) {
        /* Skip quests not of the right type. */
        if((qlist->cats[i].type & PSOCN_QUEST_TYPE_MASK) != type)
            continue;

        /* Only show debug entries to GMs. */
        if((qlist->cats[i].type & PSOCN_QUEST_DEBUG) && !LOCAL_GM(c))
            continue;

        /* Make sure the user's privilege level is good enough. */
        if((qlist->cats[i].privileges & c->privilege) !=
           qlist->cats[i].privileges && !LOCAL_GM(c))
            continue;

        /* Clear the entry */
        memset(pkt->entries + i, 0, 0x128);

        /* Copy the category's information over to the packet */
        pkt->entries[entries].menu_id = LE32((MENU_ID_QCATEGORY |
                                              (lang << 24)));
        pkt->entries[entries].item_id = LE32(i);

        /* Convert the name and the description to UTF-16. */
        in = 32;
        out = 64;
        inptr = qlist->cats[i].name;
        outptr = (char *)pkt->entries[entries].name;
        iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

        wchar_add_color_tag(pkt->entries[entries].name);

        in = 112;
        out = 224;
        inptr = qlist->cats[i].desc;
        outptr = (char *)pkt->entries[entries].desc;
        iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

        wchar_add_color_tag(pkt->entries[entries].desc);

        ++entries;
        len += 0x128;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_xbox_quest_categories(ship_client_t *c, int lang) {
    uint8_t *sendbuf = get_sendbuf();
    xb_quest_list_pkt *pkt = (xb_quest_list_pkt *)sendbuf;
    int i, len = 0x04, entries = 0;
    size_t in, out;
    char *inptr;
    char *outptr;
    psocn_quest_list_t *qlist;
    lobby_t *l = c->cur_lobby;
    uint32_t type = check_special_quest_type(l);

    qlist = &ship->qlist[CLIENT_VERSION_GC][lang];

    /* Fall back to English if there's no list for this language... */
    if(!qlist->cat_count) {
        lang = CLIENT_LANG_ENGLISH;
        qlist = &ship->qlist[CLIENT_VERSION_GC][lang];
    }

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* 填充数据头 */
    pkt->hdr.pkt_type = QUEST_LIST_TYPE;

    for(i = 0; i < qlist->cat_count; ++i) {
        /* Skip quests not of the right type. */
        if((qlist->cats[i].type & PSOCN_QUEST_TYPE_MASK) != type)
            continue;

        /* Only show debug entries to GMs. */
        if((qlist->cats[i].type & PSOCN_QUEST_DEBUG) && !LOCAL_GM(c))
            continue;

        /* Make sure the user's privilege level is good enough. */
        if((qlist->cats[i].privileges & c->privilege) !=
           qlist->cats[i].privileges && !LOCAL_GM(c))
            continue;

        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0xA8);

        /* Copy the category's information over to the packet */
        pkt->entries[entries].menu_id = LE32((MENU_ID_QCATEGORY |
                                              (lang << 24)));
        pkt->entries[entries].item_id = LE32(i);

        /* Convert the name and the description to the appropriate encoding */
        in = 32;
        out = 30;
        inptr = qlist->cats[i].name;
        outptr = &pkt->entries[entries].name[2];

        if(lang == CLIENT_LANG_JAPANESE) {
            iconv(ic_gbk_to_sjis, &inptr, &in, &outptr, &out);
            pkt->entries[entries].name[0] = '\t';
            pkt->entries[entries].name[1] = 'J';
        }
        else {
            iconv(ic_gbk_to_8859, &inptr, &in, &outptr, &out);
            pkt->entries[entries].name[0] = '\t';
            pkt->entries[entries].name[1] = 'E';
        }

        in = 112;
        out = 126;
        inptr = qlist->cats[i].desc;
        outptr = &pkt->entries[entries].desc[2];

        if(lang == CLIENT_LANG_JAPANESE) {
            iconv(ic_gbk_to_sjis, &inptr, &in, &outptr, &out);
            pkt->entries[entries].desc[0] = '\t';
            pkt->entries[entries].desc[1] = 'J';
        }
        else {
            iconv(ic_gbk_to_8859, &inptr, &in, &outptr, &out);
            pkt->entries[entries].desc[0] = '\t';
            pkt->entries[entries].desc[1] = 'E';
        }

        ++entries;
        len += 0xA8;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_quest_categories(ship_client_t* c, int lang) {
    uint8_t* sendbuf = get_sendbuf();
    bb_quest_list_pkt* pkt = (bb_quest_list_pkt*)sendbuf;
    lobby_t* l = c->cur_lobby;
    int i, len = 0x08, entries = 0;
    size_t in, out;
    char* inptr;
    char* outptr;
    uint32_t type = check_special_quest_type(l);
    psocn_quest_list_t* qlist = &ship->qlist[CLIENT_VERSION_BB][lang];
    uint32_t episode = LE32(l->episode);

    /* Fall back to English if there's no list for this language... */
    if (!qlist->cat_count) {
        lang = CLIENT_LANG_ENGLISH;
        qlist = &ship->qlist[CLIENT_VERSION_BB][lang];
    }

    /* If we still don't have a list, then bail out... */
    if (!qlist->cat_count)
        return -1;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;
    
    /* 临时设置EP3的数值以对应任务列表中的数值 */
    if (l->episode == LOBBY_EPISODE_4)
        episode = 4;

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(QUEST_LIST_TYPE);

    for(i = 0; i < qlist->cat_count; ++i) {
        /* Skip quests not of the right type. */
        if((qlist->cats[i].type & PSOCN_QUEST_TYPE_MASK) != type)
            continue;

        /* Only show debug entries to GMs. */
        if((qlist->cats[i].type & PSOCN_QUEST_DEBUG) && !LOCAL_GM(c))
            continue;

        /* Make sure the user's privilege level is good enough. */
        if((qlist->cats[i].privileges & c->privilege) !=
           qlist->cats[i].privileges && !LOCAL_GM(c))
            continue;

        //printf("episodes %d %d \n", qlist->cats[i].episodes, episode);
        /* 确认为该章节的任务 */
        if (!(qlist->cats[i].episodes == episode))
            continue;

        /* Clear the entry */
        memset(pkt->entries + i, 0, 0x13C);

        /* Copy the category's information over to the packet */
        pkt->entries[entries].menu_id = LE32((MENU_ID_QCATEGORY |
                                              (lang << 24)));
        pkt->entries[entries].item_id = LE32(i);

        /* Convert the name and the description to UTF-16. */
        in = 32;
        out = 64;
        inptr = qlist->cats[i].name;
        outptr = (char *)pkt->entries[entries].name;
        iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

        wchar_add_color_tag(pkt->entries[entries].name);

        in = 112;
        out = 244;
        inptr = qlist->cats[i].desc;
        outptr = (char *)pkt->entries[entries].desc;
        iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

        wchar_add_color_tag(pkt->entries[entries].desc);

        ++entries;
        len += 0x13C;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = LE32(entries);
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

int send_quest_categories(ship_client_t *c, int lang) {
    if(lang < 0 || lang >= CLIENT_LANG_ALL)
        lang = c->language_code;

    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3: /* XXXX? */
            return send_dc_quest_categories(c, lang);

        case CLIENT_VERSION_PC:
            return send_pc_quest_categories(c, lang);

        case CLIENT_VERSION_XBOX:
            return send_xbox_quest_categories(c, lang);

        case CLIENT_VERSION_BB:
            return send_bb_quest_categories(c, lang);
    }

    return -1;
}

/* 任务的完成度检测 */
uint32_t quest_flag_check(uint8_t* flag_data, uint32_t flag, uint32_t difficulty) {
    if (flag_data[(difficulty * 0x80) + (flag >> 3)] & (1 << (7 - (flag & 0x07))))
        return 1;
    else
        return 0;
}

/* EP1 单人任务的完成度检测 */
uint32_t quest_flag_check_ep1_solo(uint8_t* flag_data, uint32_t difficulty) {
    int i;
    uint32_t quest_flag;

    for (i = 1; i <= 25; i++) {
        quest_flag = 0x63 + (i << 1);
        if (!quest_flag_check(flag_data, quest_flag, difficulty))
            return 0;
    }

    return 1;
}

/* EP2 单人任务的完成度检测 */
uint32_t quest_flag_check_ep2_solo(uint8_t* flag_data, uint32_t difficulty) {
    int i;
    uint32_t quest_flag;

    for (i = 1; i <= 25; i++) {
        quest_flag = 0x63 + (i << 1);
        if (!quest_flag_check(flag_data, quest_flag, difficulty))
            return 0;
    }

    return 1;
}

/* EP4 单人任务的完成度检测 */
uint32_t quest_flag_check_ep4_solo(uint8_t* flag_data, uint32_t difficulty) {
    int i;
    uint32_t quest_flag;

    for (i = 1; i <= 25; i++) {
        quest_flag = 0x63 + (i << 1);
        if (!quest_flag_check(flag_data, quest_flag, difficulty))
            return 0;
    }

    return 1;
}

/* TODO 挑战任务的完成度检测 */
uint32_t quest_flag_check_cmode(uint8_t* flag_data, uint32_t difficulty) {
    int i;
    uint32_t quest_flag;

    for (i = 1; i <= 14; i++) {
        quest_flag = 65535 + (i << 1);
        if (!quest_flag_check(flag_data, quest_flag, difficulty))
            return 0;
    }

    return 1;
}

/* 单人任务的完成度状态检测 */
uint32_t check_solo_quest_stat(uint32_t qid, uint8_t oneperson, uint8_t episode, uint32_t difficulty, uint8_t* flag_data) {
    uint32_t ep1solo = quest_flag_check_ep1_solo(flag_data, difficulty);
    uint32_t ep2solo = 0;
    uint32_t ep4solo = 0;
    uint32_t show_quest = 1, quest_flag = 0, tier1 = 0;

    if (oneperson) {
        switch (episode) 
        {
        case GAME_TYPE_EPISODE_1:
            if (qid <= 26)
                if (!ep1solo)
                    show_quest = 0;

            if (!show_quest) {
                quest_flag = 0x63 + (qid << 1);

                if (quest_flag_check(flag_data, quest_flag, difficulty))
                    show_quest = 2; //如果他们已经清除了任则设置新值 并显示任务
                else {
                    tier1 = 0;
                    if ((quest_flag_check(flag_data, 0x65, difficulty)) &&  // Cleared first tier 当清理完第一章时
                        (quest_flag_check(flag_data, 0x67, difficulty)) &&
                        (quest_flag_check(flag_data, 0x6B, difficulty)))
                        tier1 = 1;

                    if (quest_flag_check(flag_data, quest_flag, difficulty) == 0) {
                        switch (qid)
                        {
                        case 4: // Battle Training 试练
                        case 2: // Claiming a Stake 拉古奥尔的大地主
                        case 1: // Magnitude of Metal 钢之心
                            show_quest = 1;
                            break;
                        case 5: // Journalistic Pursuit 先驱者二号的新闻
                        case 6: // The Fake In Yellow 黄色的伪装
                        case 7: // Native Research 大地的呼声
                        case 9: // Gran Squall 疾风号
                            if (tier1)
                                show_quest = 1;
                            break;
                        case 8: // Forest of Sorrow 恸哭的森林
                            if (quest_flag_check(flag_data, 0x71, difficulty)) // Cleared Native Research 完成大地的呼声时 隐藏任务
                                show_quest = 1;
                            break;
                        case 26: // Central Dome Fire Swirl 中心圆顶的火灾
                        case 27: // Central Dome Fire Swirl 中心圆顶的火灾 V2
                            if (quest_flag_check(flag_data, 0x73, difficulty)) // Cleared Forest of Sorrow 完成恸哭的森林时 隐藏任务
                                show_quest = 1;
                            break;
                        }

                        if ((tier1) && (quest_flag_check(flag_data, 0x1F9, difficulty)))
                        {
                            // 洞穴任务 (shown after Dragon is defeated) 洞穴任务（当巨龙被击败时）
                            switch (qid)
                            {
                            case 03: // The Value of Money 金钱的价值
                            case 11: // The Lost Bride 消失的新娘
                            case 14: // Secret Delivery 秘密货物
                            case 17: // Grave's Butler 格雷夫家的管家
                            case 10: // Addicting Food 恶魔的食物
                                show_quest = 1; // Always shown if first tier was cleared  如果第一层被清除，总是显示
                                break;
                            case 12: // Waterfall Tears 无归瀑
                            case 15: // Soul of a Blacksmith 匠之魂
                                if ((quest_flag_check(flag_data, 0x77, difficulty)) && // Cleared Addicting Food 当完成恶魔的食物
                                    (quest_flag_check(flag_data, 0x79, difficulty)) && // Cleared The Lost Bride 当完成消失的新娘
                                    (quest_flag_check(flag_data, 0x7F, difficulty)) && // Cleared Secret Delivery 当完成秘密货物
                                    (quest_flag_check(flag_data, 0x85, difficulty)))  // Cleared Grave's Butler 当完成格雷夫家的管家
                                    show_quest = 1;
                                break;
                            case 13: // Black Paper 黑页
                                if (quest_flag_check(flag_data, 0x7B, difficulty)) // Cleared Waterfall Tears 当完成无归瀑
                                    show_quest = 1;
                                break;
                            }
                        }

                        if ((tier1) && (quest_flag_check(flag_data, 0x1FF, difficulty)))
                        {
                            // 矿坑任务 (shown after De Rol Le is defeated) 洞窟任务（当击败De Rol Le怪物时）
                            switch (qid)
                            {
                            case 16: // Letter from Lionel
                            case 18: // Knowing One's Heart
                            case 20: // Dr. Osto's Research
                                show_quest = 1; // Always shown if first tier was cleared
                                break;
                            case 21: // The Unsealed Door
                                if ((quest_flag_check(flag_data, 0x8B, difficulty)) && // Cleared Dr. Osto's Research
                                    (quest_flag_check(flag_data, 0x7F, difficulty)))  // Cleared Secret Delivery
                                    show_quest = 1;
                                break;
                            }
                        }

                        if ((tier1) && (quest_flag_check(flag_data, 0x207, difficulty)))
                        {
                            // 废墟任务 (shown after Vol Opt is defeated)
                            switch (qid)
                            {
                            case 19: // The Retired Hunter
                            case 24: // Seek My Master
                                show_quest = 1;  // Always shown if first tier was cleared
                                break;
                            case 25: // From the Depths
                            case 22: // Soul of Steel
                                if (quest_flag_check(flag_data, 0x91, difficulty)) // Cleared Doc's Secret Plan
                                    show_quest = 1;
                                break;
                            case 23: // Doc's Secret Plan
                                if (quest_flag_check(flag_data, 0x7F, difficulty)) // Cleared Secret Delivery
                                    show_quest = 1;
                                break;
                            }
                        }
                    }
                }
            }
            break;

        case GAME_TYPE_EPISODE_2:
            break;

        case GAME_TYPE_EPISODE_4:
            break;
        }
    }

    return show_quest;
}

/* EP1 2 4 总督任务的完成度状态检测 */
uint32_t check_government_quest_stat(uint32_t qid, uint8_t government, uint8_t episode, uint32_t difficulty, uint8_t* flag_data) {
    uint32_t show_quest = 1, quest_flag = 0;
    uint32_t i;

    // Grey out Government quests that the player is not qualified for...不符合资格的玩家显示灰色的政府任务 sancaros
    if (government) {
        switch (episode)
        {
        case GAME_TYPE_EPISODE_1:
            qid -= 401;
            qid <<= 1;
            qid += 0x1F3;
            for (i = 0x1F5; i <= qid; i += 2)
                if (!quest_flag_check(flag_data, i, difficulty))
                    show_quest = 0;
            break;

        case GAME_TYPE_EPISODE_2:
            qid -= 451; //qid = qid - 451
            qid <<= 1; //qid = qid 左移 1位 后赋值 十进制
            qid += 0x211; //qid = qid + 0x211     (int32_t(529))
            for (i = 0x213; i <= qid; i += 2)
                if (!quest_flag_check(flag_data, i, difficulty))
                    show_quest = 0;
            break;

        case GAME_TYPE_EPISODE_4:
            qid -= 701;
            qid += 0x2BC;
            for (i = 0x2BD; i <= qid; i++)
                if (!quest_flag_check(flag_data, i, difficulty))
                    show_quest = 0;
            break;
        }
    }

    return show_quest;
}

/* Send the list of quests in a category to the client. */
static int send_dc_quest_list(ship_client_t *c, int cn, int lang) {
    uint8_t *sendbuf = get_sendbuf();
    dc_quest_list_pkt *pkt = (dc_quest_list_pkt *)sendbuf;
    int i, len = sizeof(dc_pkt_hdr_t), entries = 0, max = INT_MAX, j, k, ver, v;
    size_t in, out;
    char *inptr;
    char *outptr;
    psocn_quest_list_t *qlist, *qlisten;
    lobby_t *l = c->cur_lobby;
    psocn_quest_category_t *cat, *caten;
    psocn_quest_t *quest;
    quest_map_elem_t *elem;
    ship_client_t *tmp;
    time_t now = time(NULL);
    int hasdc = 1, haspc = 0, hasgc = 0, hasxb = 0;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    if(!l->v2) {
        ver = CLIENT_VERSION_DCV1;
        qlist = &ship->qlist[CLIENT_VERSION_DCV1][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_DCV1][CLIENT_LANG_ENGLISH];
    }
    else {
        ver = CLIENT_VERSION_DCV2;
        qlist = &ship->qlist[CLIENT_VERSION_DCV2][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_DCV2][CLIENT_LANG_ENGLISH];
    }

    /* If this quest category isn't in range for this language, try it in
       English before giving up... */
    if(qlist->cat_count <= cn) {
        lang = CLIENT_LANG_ENGLISH;
        qlist = qlisten;

        /* If we still don't have it, something screwy's going on... */
        if(qlist->cat_count <= cn)
            return -1;
    }

    /* 合理性检查. */
    if(!qlist->cats)
        return -1;

    /* Grab the category... This implicitly assumes that the categories are in
       the same order, regardless of language. At some point, I'll work this out
       a better way, but for now, this will work. */
    cat = &qlist->cats[cn];
    caten = &qlisten->cats[cn];

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* If this is for challenge mode, figure out our limit. */
    if(l->challenge)
        max = l->max_chal & 0x0F;

    /* 填充数据头 */
    pkt->hdr.pkt_type = QUEST_LIST_TYPE;

    for(k = 0; k < 2; ++k) {
        for(i = 0; i < cat->quest_count && i < max; ++i) {
            quest = cat->quests[i];
            elem = (quest_map_elem_t *)quest->user_data;

            /* Skip quests we should have already covered if we're on the second
               pass through */
            if(k && elem->qptr[ver][lang])
                continue;

            /* Skip quests that aren't for the current event */
            if(!(quest->event & (1 << l->event)))
                continue;

            /* Skip quests where the number of players isn't in range. */
            if(quest->max_players < l->num_clients ||
               quest->min_players > l->num_clients)
                continue;

            /* Look through to make sure that all clients in the lobby can play
               the quest */
            for(j = 0; j < l->max_clients; ++j) {
                if(!(tmp = l->clients[j]))
                    continue;

                v = tmp->version;

                switch (v) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                    hasdc = 1;
                    break;

                case CLIENT_VERSION_PC:
                    haspc = 1;
                    break;

                case CLIENT_VERSION_GC:
                    hasgc = 1;
                    break;

                case CLIENT_VERSION_XBOX:
                    v = CLIENT_VERSION_GC;
                    hasxb = 1;
                    break;
                }

                if(!k && !elem->qptr[v][tmp->q_lang] &&
                   !elem->qptr[v][tmp->language_code] &&
                   !elem->qptr[v][CLIENT_LANG_ENGLISH] &&
                   !elem->qptr[v][lang])
                    break;
            }

            /* Skip quests where we can't play them due to restrictions by
               users' versions or language codes */
            if(j != l->max_clients)
                continue;

            /* Make sure the user's privilege level is good enough. */
            if((quest->privileges & c->privilege) != quest->privileges &&
               !LOCAL_GM(c))
                continue;

            /* Check the availability time against the current time. */
            if(quest->start_time && quest->start_time > (uint64_t)now)
                continue;
            else if(quest->end_time && quest->end_time < (uint64_t)now)
                continue;

            /* Check the hidden flag */
            if((quest->flags & PSOCN_QUEST_HIDDEN))
                continue;

            /* Check the various version-disable flags */
            if ((quest->versions & PSOCN_QUEST_NODC) && hasdc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOPC) && haspc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOGC) && hasgc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOXB) && hasxb)
                continue;

            /* Clear the entry */
            memset(pkt->entries + entries, 0, 0x98);

            /* Copy the category's information over to the packet */
            pkt->entries[entries].menu_id = LE32(((MENU_ID_QUEST) |
                                                  (lang << 24)));
            pkt->entries[entries].item_id = LE32(quest->qid);

            /* Convert the name and the description to the appropriate
               encoding */
            in = 32;
            out = 30;
            inptr = quest->name;
            outptr = &pkt->entries[entries].name[2];

            if(lang == CLIENT_LANG_JAPANESE && !k) {
                iconv(ic_utf8_to_sjis, &inptr, &in, &outptr, &out);
                pkt->entries[entries].name[0] = '\t';
                pkt->entries[entries].name[1] = 'J';
            }
            else {
                iconv(ic_utf8_to_8859, &inptr, &in, &outptr, &out);
                pkt->entries[entries].name[0] = '\t';
                pkt->entries[entries].name[1] = 'E';
            }

            in = 112;
            out = 110;
            inptr = quest->desc;
            outptr = &pkt->entries[entries].desc[2];

            if(lang == CLIENT_LANG_JAPANESE && !k) {
                iconv(ic_utf8_to_sjis, &inptr, &in, &outptr, &out);
                pkt->entries[entries].desc[0] = '\t';
                pkt->entries[entries].desc[1] = 'J';
            }
            else {
                iconv(ic_utf8_to_8859, &inptr, &in, &outptr, &out);
                pkt->entries[entries].desc[0] = '\t';
                pkt->entries[entries].desc[1] = 'E';
            }

            ++entries;
            len += 0x98;
        }

        /* If we already did English, then we're done. */
        if(cat == caten)
            break;

        cat = caten;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_quest_list(ship_client_t *c, int cn, int lang) {
    uint8_t *sendbuf = get_sendbuf();
    pc_quest_list_pkt *pkt = (pc_quest_list_pkt *)sendbuf;
    int i, len = sizeof(dc_pkt_hdr_t), entries = 0, max = INT_MAX, j, k, ver, v;
    size_t in, out;
    char *inptr;
    char *outptr;
    psocn_quest_list_t *qlist, *qlisten;
    lobby_t *l = c->cur_lobby;
    psocn_quest_category_t *cat, *caten;
    psocn_quest_t *quest;
    quest_map_elem_t *elem;
    ship_client_t *tmp;
    time_t now = time(NULL);
    int hasdc = 0, haspc = 0, hasgc = 0, hasxb = 0;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    if(!l->v2) {
        ver = CLIENT_VERSION_DCV1;
        qlist = &ship->qlist[CLIENT_VERSION_DCV1][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_DCV1][CLIENT_LANG_ENGLISH];
    }
    else {
        ver = CLIENT_VERSION_PC;
        qlist = &ship->qlist[CLIENT_VERSION_PC][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_PC][CLIENT_LANG_ENGLISH];
    }

    /* If this quest category isn't in range for this language, try it in
       English before giving up... */
    if(qlist->cat_count <= cn) {
        lang = CLIENT_LANG_ENGLISH;
        qlist = qlisten;

        /* If we still don't have it, something screwy's going on... */
        if(qlist->cat_count <= cn)
            return -1;
    }

    /* 合理性检查. */
    if(!qlist->cats)
        return -1;

    /* Grab the category... This implicitly assumes that the categories are in
       the same order, regardless of language. At some point, I'll work this out
       a better way, but for now, this will work. */
    cat = &qlist->cats[cn];
    caten = &qlisten->cats[cn];

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* If this is for challenge mode, figure out our limit. */
    if(l->challenge)
        max = l->max_chal & 0x0F;

    /* 填充数据头 */
    pkt->hdr.pkt_type = QUEST_LIST_TYPE;

    for(k = 0; k < 2; ++k) {
        for(i = 0; i < cat->quest_count && i < max; ++i) {
            quest = cat->quests[i];
            elem = (quest_map_elem_t *)quest->user_data;

            /* Skip quests we should have already covered if we're on the second
               pass through */
            if(k && elem->qptr[ver][lang])
                continue;

            /* Skip quests that aren't for the current event */
            if(!(quest->event & (1 << l->event)))
                continue;

            /* Skip quests where the number of players isn't in range. */
            if(quest->max_players < l->num_clients ||
               quest->min_players > l->num_clients)
                continue;

            /* Look through to make sure that all clients in the lobby can play
               the quest */
            for(j = 0; j < l->max_clients; ++j) {
                if(!(tmp = l->clients[j]))
                    continue;

                v = tmp->version;

                switch (v) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                    hasdc = 1;
                    break;

                case CLIENT_VERSION_PC:
                    haspc = 1;
                    break;

                case CLIENT_VERSION_GC:
                    hasgc = 1;
                    break;

                case CLIENT_VERSION_XBOX:
                    v = CLIENT_VERSION_GC;
                    hasxb = 1;
                    break;
                }

                if(!k && !elem->qptr[v][tmp->q_lang] &&
                   !elem->qptr[v][tmp->language_code] &&
                   !elem->qptr[v][CLIENT_LANG_ENGLISH] &&
                   !elem->qptr[v][lang])
                    break;
            }

            /* Skip quests where we can't play them due to restrictions by
               users' versions or language codes */
            if(j != l->max_clients)
                continue;

            /* Make sure the user's privilege level is good enough. */
            if((quest->privileges & c->privilege) != quest->privileges &&
               !LOCAL_GM(c))
                continue;

            /* Check the availability time against the current time. */
            if(quest->start_time && quest->start_time > (uint64_t)now)
                continue;
            else if(quest->end_time && quest->end_time < (uint64_t)now)
                continue;

            /* Check the hidden flag */
            if((quest->flags & PSOCN_QUEST_HIDDEN))
                continue;

            /* Check the various version-disable flags */
            if ((quest->versions & PSOCN_QUEST_NODC) && hasdc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOPC) && haspc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOGC) && hasgc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOXB) && hasxb)
                continue;

            /* Clear the entry */
            memset(pkt->entries + entries, 0, 0x128);

            /* Copy the category's information over to the packet */
            pkt->entries[entries].menu_id = LE32(((MENU_ID_QUEST) |
                                                  (lang << 24)));
            pkt->entries[entries].item_id = LE32(quest->qid);

            /* Convert the name and the description to UTF-16. */
            in = 32;
            out = 64;
            inptr = quest->name;
            outptr = (char *)pkt->entries[entries].name;
            iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

            wchar_add_color_tag(pkt->entries[entries].name);

            in = 112;
            out = 224;
            inptr = quest->desc;
            outptr = (char *)pkt->entries[entries].desc;
            iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

            wchar_add_color_tag(pkt->entries[entries].desc);

            ++entries;
            len += 0x128;
        }

        /* If we already did English, then we're done. */
        if(cat == caten)
            break;

        cat = caten;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_gc_quest_list(ship_client_t *c, int cn, int lang) {
    uint8_t *sendbuf = get_sendbuf();
    dc_quest_list_pkt *pkt = (dc_quest_list_pkt *)sendbuf;
    int i, len = sizeof(dc_pkt_hdr_t), entries = 0, max = INT_MAX, max2 = INT_MAX, j, k, ver, v;
    size_t in, out;
    char *inptr;
    char *outptr;
    psocn_quest_list_t *qlist, *qlisten;
    lobby_t *l = c->cur_lobby;
    psocn_quest_category_t *cat, *caten;
    psocn_quest_t *quest;
    quest_map_elem_t *elem;
    ship_client_t *tmp;
    time_t now = time(NULL);
    int hasdc = 0, haspc = 0, hasgc = 0, hasxb = 0;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    if(l->version == CLIENT_VERSION_GC || l->version == CLIENT_VERSION_XBOX) {
        ver = CLIENT_VERSION_GC;
        qlist = &ship->qlist[CLIENT_VERSION_GC][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_GC][CLIENT_LANG_ENGLISH];
    }
    else if(!l->v2) {
        ver = CLIENT_VERSION_DCV1;
        qlist = &ship->qlist[CLIENT_VERSION_DCV1][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_DCV1][CLIENT_LANG_ENGLISH];
    }
    else {
        ver = CLIENT_VERSION_DCV2;
        qlist = &ship->qlist[CLIENT_VERSION_DCV2][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_DCV2][CLIENT_LANG_ENGLISH];
    }

    /* If this quest category isn't in range for this language, try it in
       English before giving up... */
    if(qlist->cat_count <= cn) {
        lang = CLIENT_LANG_ENGLISH;
        qlist = qlisten;

        /* If we still don't have it, something screwy's going on... */
        if(qlist->cat_count <= cn)
            return -1;
    }

    /* 合理性检查. */
    if(!qlist->cats)
        return -1;

    /* Grab the category... This implicitly assumes that the categories are in
       the same order, regardless of language. At some point, I'll work this out
       a better way, but for now, this will work. */
    cat = &qlist->cats[cn];
    caten = &qlisten->cats[cn];

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* If this is for challenge mode, figure out our limit. */
    if(l->challenge) {
        max = l->max_chal & 0x0F;
        max2 = (l->max_chal >> 4) & 0x0F;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = QUEST_LIST_TYPE;

    for(k = 0; k < 2; ++k) {
        for(i = 0; i < cat->quest_count; ++i) {
            if(l->challenge) {
                /* Skip episode 1 challenge quests we're not qualified for. */
                if(i < 9 && i >= max)
                    continue;
                /* Same for episode 2. */
                else if(i > 9 && (i - 9) >= max2)
                    break;
            }

            quest = cat->quests[i];
            elem = (quest_map_elem_t *)quest->user_data;

            /* Skip quests we should have already covered if we're on the second
               pass through */
            if(k && elem->qptr[ver][lang])
                continue;

            /* Skip quests that aren't for the current event */
            if(!(quest->event & (1 << l->event)))
                continue;

            /* Skip quests where the number of players isn't in range. */
            if(quest->max_players < l->num_clients ||
               quest->min_players > l->num_clients)
                continue;

            /* Look through to make sure that all clients in the lobby can play
               the quest */
            for(j = 0; j < l->max_clients; ++j) {
                if(!(tmp = l->clients[j]))
                    continue;

                v = tmp->version;

                switch (v) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                    hasdc = 1;
                    break;

                case CLIENT_VERSION_PC:
                    haspc = 1;
                    break;

                case CLIENT_VERSION_GC:
                    hasgc = 1;
                    break;

                case CLIENT_VERSION_XBOX:
                    v = CLIENT_VERSION_GC;
                    hasxb = 1;
                    break;
                }

                if(!k && !elem->qptr[v][tmp->q_lang] &&
                   !elem->qptr[v][tmp->language_code] &&
                   !elem->qptr[v][CLIENT_LANG_ENGLISH] &&
                   !elem->qptr[v][lang])
                    break;
            }

            /* Skip quests where we can't play them due to restrictions by
               users' versions or language codes */
            if(j != l->max_clients)
                continue;

            /* Make sure the user's privilege level is good enough. */
            if((quest->privileges & c->privilege) != quest->privileges &&
               !LOCAL_GM(c))
                continue;

            /* Check the availability time against the current time. */
            if(quest->start_time && quest->start_time > (uint64_t)now)
                continue;
            else if(quest->end_time && quest->end_time < (uint64_t)now)
                continue;

            /* Check the hidden flag */
            if((quest->flags & PSOCN_QUEST_HIDDEN))
                continue;

            /* Check the various version-disable flags */
            if ((quest->versions & PSOCN_QUEST_NODC) && hasdc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOPC) && haspc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOGC) && hasgc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOXB) && hasxb)
                continue;

            /* Clear the entry */
            memset(pkt->entries + entries, 0, 0x98);

            /* Copy the category's information over to the packet */
            pkt->entries[entries].menu_id = LE32(((MENU_ID_QUEST) |
                                                  (quest->episode << 8) |
                                                  (lang << 24)));
            pkt->entries[entries].item_id = LE32(quest->qid);

            /* Convert the name and the description to the appropriate
               encoding */
            in = 32;
            out = 30;
            inptr = quest->name;
            outptr = &pkt->entries[entries].name[2];

            if(lang == CLIENT_LANG_JAPANESE && !k) {
                iconv(ic_utf8_to_sjis, &inptr, &in, &outptr, &out);
                pkt->entries[entries].name[0] = '\t';
                pkt->entries[entries].name[1] = 'J';
            }
            else {
                iconv(ic_utf8_to_8859, &inptr, &in, &outptr, &out);
                pkt->entries[entries].name[0] = '\t';
                pkt->entries[entries].name[1] = 'E';
            }

            in = 112;
            out = 110;
            inptr = quest->desc;
            outptr = &pkt->entries[entries].desc[2];

            if(lang == CLIENT_LANG_JAPANESE && !k) {
                iconv(ic_utf8_to_sjis, &inptr, &in, &outptr, &out);
                pkt->entries[entries].desc[0] = '\t';
                pkt->entries[entries].desc[1] = 'J';
            }
            else {
                iconv(ic_utf8_to_8859, &inptr, &in, &outptr, &out);
                pkt->entries[entries].desc[0] = '\t';
                pkt->entries[entries].desc[1] = 'E';
            }

            ++entries;
            len += 0x98;
        }

        /* If we already did English, then we're done. */
        if(cat == caten)
            break;

        cat = caten;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_xbox_quest_list(ship_client_t *c, int cn, int lang) {
    uint8_t *sendbuf = get_sendbuf();
    xb_quest_list_pkt *pkt = (xb_quest_list_pkt *)sendbuf;
    int i, len = sizeof(dc_pkt_hdr_t), entries = 0, max = INT_MAX, max2 = INT_MAX, j, k, ver, v;
    size_t in, out;
    char *inptr;
    char *outptr;
    psocn_quest_list_t *qlist, *qlisten;
    lobby_t *l = c->cur_lobby;
    psocn_quest_category_t *cat, *caten;
    psocn_quest_t *quest;
    quest_map_elem_t *elem;
    ship_client_t *tmp;
    time_t now = time(NULL);
    int hasdc = 0, haspc = 0, hasgc = 0, hasxb = 0;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    if(l->version == CLIENT_VERSION_GC || l->version == CLIENT_VERSION_XBOX) {
        ver = CLIENT_VERSION_GC;
        qlist = &ship->qlist[CLIENT_VERSION_GC][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_GC][CLIENT_LANG_ENGLISH];
    }
    else if(!l->v2) {
        ver = CLIENT_VERSION_DCV1;
        qlist = &ship->qlist[CLIENT_VERSION_DCV1][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_DCV1][CLIENT_LANG_ENGLISH];
    }
    else {
        ver = CLIENT_VERSION_DCV2;
        qlist = &ship->qlist[CLIENT_VERSION_DCV2][lang];
        qlisten = &ship->qlist[CLIENT_VERSION_DCV2][CLIENT_LANG_ENGLISH];
    }

    /* If this quest category isn't in range for this language, try it in
       English before giving up... */
    if(qlist->cat_count <= cn) {
        lang = CLIENT_LANG_ENGLISH;
        qlist = qlisten;

        /* If we still don't have it, something screwy's going on... */
        if(qlist->cat_count <= cn)
            return -1;
    }

    /* 合理性检查. */
    if(!qlist->cats)
        return -1;

    /* Grab the category... This implicitly assumes that the categories are in
       the same order, regardless of language. At some point, I'll work this out
       a better way, but for now, this will work. */
    cat = &qlist->cats[cn];
    caten = &qlisten->cats[cn];

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* If this is for challenge mode, figure out our limit. */
    if(l->challenge) {
        max = l->max_chal & 0x0F;
        max2 = (l->max_chal >> 4) & 0x0F;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = QUEST_LIST_TYPE;

    for(k = 0; k < 2; ++k) {
        for(i = 0; i < cat->quest_count; ++i) {
            if(l->challenge) {
                /* Skip episode 1 challenge quests we're not qualified for. */
                if(i < 9 && i >= max)
                    continue;
                /* Same for episode 2. */
                else if(i > 9 && (i - 9) >= max2)
                    break;
            }

            quest = cat->quests[i];
            elem = (quest_map_elem_t *)quest->user_data;

            /* Skip quests we should have already covered if we're on the second
               pass through */
            if(k && elem->qptr[ver][lang])
                continue;

            /* Skip quests that aren't for the current event */
            if(!(quest->event & (1 << l->event)))
                continue;

            /* Skip quests where the number of players isn't in range. */
            if(quest->max_players < l->num_clients ||
               quest->min_players > l->num_clients)
                continue;

            /* Look through to make sure that all clients in the lobby can play
               the quest */
            for(j = 0; j < l->max_clients; ++j) {
                if(!(tmp = l->clients[j]))
                    continue;

                v = tmp->version;

                switch (v) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                    hasdc = 1;
                    break;

                case CLIENT_VERSION_PC:
                    haspc = 1;
                    break;

                case CLIENT_VERSION_GC:
                    hasgc = 1;
                    break;

                case CLIENT_VERSION_XBOX:
                    v = CLIENT_VERSION_GC;
                    hasxb = 1;
                    break;
                }

                if(!k && !elem->qptr[v][tmp->q_lang] &&
                   !elem->qptr[v][tmp->language_code] &&
                   !elem->qptr[v][CLIENT_LANG_ENGLISH] &&
                   !elem->qptr[v][lang])
                    break;
            }

            /* Skip quests where we can't play them due to restrictions by
               users' versions or language codes */
            if(j != l->max_clients)
                continue;

            /* Make sure the user's privilege level is good enough. */
            if((quest->privileges & c->privilege) != quest->privileges &&
               !LOCAL_GM(c))
                continue;

            /* Check the availability time against the current time. */
            if(quest->start_time && quest->start_time > (uint64_t)now)
                continue;
            else if(quest->end_time && quest->end_time < (uint64_t)now)
                continue;

            /* Check the hidden flag */
            if((quest->flags & PSOCN_QUEST_HIDDEN))
                continue;

            /* Check the various version-disable flags */
            if ((quest->versions & PSOCN_QUEST_NODC) && hasdc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOPC) && haspc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOGC) && hasgc)
                continue;
            if ((quest->versions & PSOCN_QUEST_NOXB) && hasxb)
                continue;

            /* Clear the entry */
            memset(pkt->entries + entries, 0, 0x98);

            /* Copy the category's information over to the packet */
            pkt->entries[entries].menu_id = LE32(((MENU_ID_QUEST) |
                                                  (quest->episode << 8) |
                                                  (lang << 24)));
            pkt->entries[entries].item_id = LE32(quest->qid);

            /* Convert the name and the description to the appropriate
               encoding */
            in = 32;
            out = 30;
            inptr = quest->name;
            outptr = &pkt->entries[entries].name[2];

            if(lang == CLIENT_LANG_JAPANESE && !k) {
                iconv(ic_utf8_to_sjis, &inptr, &in, &outptr, &out);
                pkt->entries[entries].name[0] = '\t';
                pkt->entries[entries].name[1] = 'J';
            }
            else {
                iconv(ic_utf8_to_8859, &inptr, &in, &outptr, &out);
                pkt->entries[entries].name[0] = '\t';
                pkt->entries[entries].name[1] = 'E';
            }

            in = 112;
            out = 126;
            inptr = quest->desc;
            outptr = &pkt->entries[entries].desc[2];

            if(lang == CLIENT_LANG_JAPANESE && !k) {
                iconv(ic_utf8_to_sjis, &inptr, &in, &outptr, &out);
                pkt->entries[entries].desc[0] = '\t';
                pkt->entries[entries].desc[1] = 'J';
            }
            else {
                iconv(ic_utf8_to_8859, &inptr, &in, &outptr, &out);
                pkt->entries[entries].desc[0] = '\t';
                pkt->entries[entries].desc[1] = 'E';
            }

            ++entries;
            len += 0xA8;
        }

        /* If we already did English, then we're done. */
        if(cat == caten)
            break;

        cat = caten;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_quest_list(ship_client_t *c, int cn, int lang) {
    uint8_t *sendbuf = get_sendbuf();
    bb_quest_list_pkt *pkt = (bb_quest_list_pkt *)sendbuf;
    int i, len = sizeof(bb_pkt_hdr_t), entries = 0, max = INT_MAX, max2 = INT_MAX, j, k;
    psocn_quest_list_t *qlist, *qlisten;
    lobby_t *l = c->cur_lobby;
    psocn_quest_category_t *cat, *caten;
    psocn_quest_t *quest;
    quest_map_elem_t *elem;
    ship_client_t *tmp;
    time_t now = time(NULL);
    uint32_t episode = LE32(l->episode);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    qlist = &ship->qlist[CLIENT_VERSION_BB][lang];
    qlisten = &ship->qlist[CLIENT_VERSION_BB][CLIENT_LANG_ENGLISH];

    /* If this quest category isn't in range for this language, try it in
       English before giving up... */
    if(qlist->cat_count <= cn) {
        lang = CLIENT_LANG_ENGLISH;
        qlist = qlisten;

        /* If we still don't have it, something screwy's going on... */
        if(qlist->cat_count <= cn)
            return -1;
    }

    /* 合理性检查. */
    if(!qlist->cats)
        return -1;

    /* Grab the category... This implicitly assumes that the categories are in
       the same order, regardless of language. At some point, I'll work this out
       a better way, but for now, this will work. */
    cat = &qlist->cats[cn];
    caten = &qlisten->cats[cn];

    /* If this is for challenge mode, figure out our limit. */
    if(l->challenge) {
        max = l->max_chal & 0x0F;
        max2 = (l->max_chal >> 4) & 0x0F;
    }

    /* 临时设置EP3的数值以对应任务列表中的数值 */
    if (l->episode == LOBBY_EPISODE_4)
        episode = 4;

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(QUEST_LIST_TYPE);

    for(k = 0; k < 3; ++k) {
        for(i = 0; i < cat->quest_count; ++i) {
            if(l->challenge) {
                /*  跳过第1集：我们没有资格参加的挑战任务 Skip episode 1 challenge quests we're not qualified for. */
                if(i < 9 && i >= max)
                    continue;
                /* 和 episode 2 一样. */
                else if(i > 9 && (i - 9) >= max2)
                    break;
            }

            quest = cat->quests[i];
            elem = (quest_map_elem_t *)quest->user_data;

            /* TODO 完成任务*/

            /* Skip quests we should have already covered if we're on the second
               pass through */
            if(k && elem->qptr[CLIENT_VERSION_BB][lang])
                continue;

            //printf("episodes %d %d \n", cat->episodes, episode);
            /* 确认为该章节的任务. */
            if (!(quest->episode & episode))
                continue;

            /* 跳过不是当前季节事件的任务 */
            if(!(quest->event & (1 << l->event)))
                continue;

            /* TODO 完成任务*/

            /* 跳过房间玩家数不满足的任务. */
            if(quest->max_players < l->num_clients ||
               quest->min_players > l->num_clients)
                continue;

            /* Look through to make sure that all clients in the lobby can play
               the quest */
            for(j = 0; j < l->max_clients; ++j) {
                if(!(tmp = l->clients[j]))
                    continue;

                if(!k && !elem->qptr[tmp->version][tmp->q_lang] &&
                   !elem->qptr[tmp->version][tmp->language_code] &&
                   !elem->qptr[tmp->version][CLIENT_LANG_ENGLISH] &&
                   !elem->qptr[tmp->version][lang])
                    break;
            }

            /* Skip quests where we can't play them due to restrictions by
               users' versions or language codes */
            if(j != l->max_clients)
                continue;

            /* Make sure the user's privilege level is good enough. */
            if((quest->privileges & c->privilege) != quest->privileges &&
               !LOCAL_GM(c))
                continue;

            /* Check the availability time against the current time. */
            if(quest->start_time && quest->start_time > (uint64_t)now)
                continue;
            else if(quest->end_time && quest->end_time < (uint64_t)now)
                continue;

            /* 检测任务隐藏标志 */
            if((quest->flags & PSOCN_QUEST_HIDDEN))
                continue;

            /* 检测单人任务完成状态 */
            if(!check_solo_quest_stat(quest->qid, l->oneperson, l->episode, LE32(l->difficulty), &c->bb_pl->quest_data1.data[0]))
                continue;

            /* 检测EP1 2 4 总督任务完成状态 */
            if(!check_government_quest_stat(quest->qid, l->govorlab, l->episode, LE32(l->difficulty), &c->bb_pl->quest_data1.data[0]))
                continue;

            //printf("quest id = %d\n", quest->qid);

            /* Clear the entry */
            memset(pkt->entries + entries, 0, sizeof(bb_quest_t));

            /* Copy the category's information over to the packet */
            pkt->entries[entries].menu_id = LE32(((MENU_ID_QUEST) |
                                                  (quest->episode << 8) |
                                                  (lang << 24)));
            pkt->entries[entries].item_id = LE32(quest->qid);

            /* Convert the name and the description to UTF-16. */
            istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, quest->name, sizeof(pkt->entries[entries].name));

            wchar_add_color_tag(pkt->entries[entries].name);

            istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].desc, quest->desc, sizeof(pkt->entries[entries].desc));

            wchar_add_color_tag(pkt->entries[entries].desc);

            ++entries;
            len += sizeof(bb_quest_t);
        }

        /* If we already did English, then we're done. */
        if(cat == caten)
            break;

        cat = caten;
    }

    /* Fill in the rest of the header */
    pkt->hdr.flags = LE32(entries);
    pkt->hdr.pkt_len = LE16(len);

    //display_packet(sendbuf, len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

int send_quest_list(ship_client_t *c, int cat, int lang) {
    if(lang >= CLIENT_LANG_ALL)
        return -1;

    //DBG_LOG("lang = %d cat = %d c->version = %d", lang, cat, c->version);
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            return send_dc_quest_list(c, cat, lang);

        case CLIENT_VERSION_PC:
            return send_pc_quest_list(c, cat, lang);

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3: /* XXXX? */
            return send_gc_quest_list(c, cat, lang);

        case CLIENT_VERSION_XBOX:
            return send_xbox_quest_list(c, cat, lang);

        case CLIENT_VERSION_BB:
            return send_bb_quest_list(c, cat, lang);
    }

    //DBG_LOG("lang = %d cat = %d c->version = %d", lang, cat, c->version);
    return -1;
}

/* Send information about a quest to the lobby. */
static int send_dc_quest_info(ship_client_t *c, psocn_quest_t *q, int l) {
    uint8_t *sendbuf = get_sendbuf();
    dc_msg_box_pkt *pkt = (dc_msg_box_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, DC_QUEST_INFO_LENGTH);

    /* Fill in the basics */
    pkt->hdr.dc.pkt_type = QUEST_INFO_TYPE;
    pkt->hdr.dc.flags = 0;
    pkt->hdr.dc.pkt_len = LE16(DC_QUEST_INFO_LENGTH);

    /* Convert to the right encoding */
    in = 0x124;
    inptr = q->long_desc;
    out = 0x122;
    outptr = &pkt->msg[2];

    if(l == CLIENT_LANG_JAPANESE) {
        iconv(ic_gbk_to_sjis, &inptr, &in, &outptr, &out);
        pkt->msg[0] = '\t';
        pkt->msg[1] = 'J';
    }
    else {
        iconv(ic_gbk_to_8859, &inptr, &in, &outptr, &out);
        pkt->msg[0] = '\t';
        pkt->msg[1] = 'E';
    }

    /* 加密并发送 */
    return crypt_send(c, DC_QUEST_INFO_LENGTH, sendbuf);
}

static int send_pc_quest_info(ship_client_t *c, psocn_quest_t *q, int l) {
    uint8_t *sendbuf = get_sendbuf();
    dc_msg_box_pkt *pkt = (dc_msg_box_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, PC_QUEST_INFO_LENGTH);

    /* Fill in the basics */
    pkt->hdr.pc.pkt_type = QUEST_INFO_TYPE;
    pkt->hdr.pc.flags = 0;
    pkt->hdr.pc.pkt_len = LE16(PC_QUEST_INFO_LENGTH);

    /* Convert to the right encoding */
    in = 0x124;
    inptr = q->long_desc;
    out = 0x248;
    outptr = pkt->msg;
    iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

    /* 加密并发送 */
    return crypt_send(c, PC_QUEST_INFO_LENGTH, sendbuf);
}

static int send_bb_quest_info(ship_client_t *c, psocn_quest_t *q, int l) {
    uint8_t *sendbuf = get_sendbuf();
    bb_msg_box_pkt *pkt = (bb_msg_box_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, PC_QUEST_INFO_LENGTH);

    /* Fill in the basics */
    pkt->hdr.pkt_type = LE16(QUEST_INFO_TYPE);
    pkt->hdr.flags = 0;
    pkt->hdr.pkt_len = LE16(BB_QUEST_INFO_LENGTH);

    /* Convert to the right encoding */
    in = 0x124;
    inptr = q->long_desc;
    out = 0x248;
    outptr = pkt->msg;
    iconv(ic_utf8_to_utf16, &inptr, &in, &outptr, &out);

    /* 加密并发送 */
    return crypt_send(c, BB_QUEST_INFO_LENGTH, sendbuf);
}

int send_quest_info(lobby_t *l, uint32_t qid, int lang) {
    ship_client_t *c;
    int i;
    quest_map_elem_t *elem;
    psocn_quest_t *q;
    int sel_lang, v;

    /* Grab the mapped entry */
    c = l->clients[l->leader_id];
    elem = quest_lookup(&ship->qmap, qid);

    /* Make sure we get the quest we're looking for */
    if(!elem) {
        return -1;
    }

    for(i = 0; i < l->max_clients; ++i) {
        if((c = l->clients[i])) {
            v = c->version;

            if(v == CLIENT_VERSION_XBOX)
                v = CLIENT_VERSION_GC;

            q = elem->qptr[v][c->q_lang];
            sel_lang = c->q_lang;

            /* If we didn't find it on the quest language code, try the language
               code set in the character data. */
            if(!q) {
                q = elem->qptr[v][c->language_code];
                sel_lang = c->language_code;
            }

            /* Try English next, so as to have a reasonably sane fallback. */
            if(!q) {
                q = elem->qptr[v][CLIENT_LANG_ENGLISH];
                sel_lang = CLIENT_LANG_ENGLISH;
            }

            /* If all else fails, go with the language the quest was selected by
               the leader in, since that has to be there! */
            if(!q) {
                q = elem->qptr[v][lang];
                sel_lang = lang;

                /* If we still didn't find it, we've got trouble elsewhere... */
                if(!q) {
                    QERR_LOG("未加载任务无法发送任务信息数据!\n"
                          "ID: %d, 版本: %d, 语言: %d, 备用: %d, "
                          "备用 2: %d", qid, v, c->q_lang,
                          c->language_code, lang);
                    continue;
                }
            }

            /* Call the appropriate function. */
            switch(c->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    send_dc_quest_info(c, q, sel_lang);
                    break;

                case CLIENT_VERSION_PC:
                    send_pc_quest_info(c, q, sel_lang);
                    break;

                case CLIENT_VERSION_BB:
                    send_bb_quest_info(c, q, sel_lang);
                    break;
            }
        }
    }

    return 0;
}

/* Send a quest to everyone in a lobby. */
static int send_dcv1_quest(ship_client_t *c, quest_map_elem_t *qm, int v1,
                           int lang) {
    uint8_t *sendbuf = get_sendbuf();
    dc_quest_file_pkt *file = (dc_quest_file_pkt *)sendbuf;
    dc_quest_chunk_pkt *chunk = (dc_quest_chunk_pkt *)sendbuf;
    FILE *bin, *dat;
    uint32_t binlen, datlen;
    int bindone = 0, datdone = 0, chunknum = 0;
    char fn_base[256], filename[260];
    size_t amt;
    psocn_quest_t *q = qm->qptr[c->version][lang];

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf || !q) {
        return -1;
    }

    /* Each quest has two files: a .dat file and a .bin file, send a file packet
       for each of them. */
    snprintf(fn_base, 256, "%s/%s/%s/%s", ship->cfg->quests_dir,
        client_type[CLIENT_VERSION_DCV1]->ver_name, language_codes[lang],
             q->prefix);

    snprintf(filename, 260, "%s_%s.bin", fn_base, language_codes[lang]);
    bin = fopen(filename, "rb");

    if(!bin) {
        QERR_LOG("打开 %s BIN文件错误: %s", fn_base,
              strerror(errno));
        return -1;
    }

    snprintf(filename, 260, "%s_%s.dat", fn_base, language_codes[lang]);
    dat = fopen(filename, "rb");

    if(!dat) {
        QERR_LOG("打开 %s DAT文件错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        return -1;
    }

    /* Figure out how long each of the files are */
    fseek(bin, 0, SEEK_END);
    binlen = (uint32_t)ftell(bin);
    fseek(bin, 0, SEEK_SET);

    fseek(dat, 0, SEEK_END);
    datlen = (uint32_t)ftell(dat);
    fseek(dat, 0, SEEK_SET);

    /* Send the file info packets */
    /* Start with the .dat file. */
    memset(file, 0, sizeof(dc_quest_file_pkt));

    snprintf(file->name, 32, "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x02; /* ??? */
    file->hdr.pkt_len = LE16(DC_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.dat", q->prefix);
    file->length = LE32(datlen);

    if(crypt_send(c, DC_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now the .bin file. */
    memset(file, 0, sizeof(dc_quest_file_pkt));

    snprintf(file->name, 32, "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x02; /* ??? */
    file->hdr.pkt_len = LE16(DC_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.bin", q->prefix);
    file->length = LE32(binlen);

    if(crypt_send(c, DC_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s BIN文件数据头错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now send the chunks of the file, interleaved. */
    while(!bindone || !datdone) {
        /* Start with the dat file if we've got any more to send from it */
        if(!datdone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(dc_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.dc.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.dc.flags = (uint8_t)chunknum;
            chunk->hdr.dc.pkt_len = LE16(DC_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.dat", q->prefix);
            amt = fread(chunk->data, 1, 0x400, dat);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if(crypt_send(c, DC_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
                      strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if(amt != 0x400) {
                datdone = 1;
            }
        }

        /* Then the bin file if we've got any more to send from it */
        if(!bindone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(dc_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.dc.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.dc.flags = (uint8_t)chunknum;
            chunk->hdr.dc.pkt_len = LE16(DC_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.bin", q->prefix);
            amt = fread(chunk->data, 1, 0x400, bin);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if(crypt_send(c, DC_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("Error sending bin file %s: %s", fn_base,
                      strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if(amt != 0x400) {
                bindone = 1;
            }
        }

        ++chunknum;
    }

    /* We're done with the files, close them */
    fclose(bin);
    fclose(dat);

    return 0;
}

static int send_dcv2_quest(ship_client_t *c, quest_map_elem_t *qm, int v1,
                           int lang) {
    uint8_t *sendbuf = get_sendbuf();
    dc_quest_file_pkt *file = (dc_quest_file_pkt *)sendbuf;
    dc_quest_chunk_pkt *chunk = (dc_quest_chunk_pkt *)sendbuf;
    FILE *bin, *dat;
    uint32_t binlen, datlen;
    int bindone = 0, datdone = 0, chunknum = 0;
    char fn_base[256], filename[260];
    size_t amt;
    psocn_quest_t *q = qm->qptr[c->version][lang];

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf || !q) {
        return -1;
    }

    /* Each quest has two files: a .dat file and a .bin file, send a file packet
       for each of them. */
    if(!v1 || (q->versions & PSOCN_QUEST_V1)) {
        snprintf(fn_base, 256, "%s/%s/%s/%s", ship->cfg->quests_dir,
            client_type[c->version]->ver_name, language_codes[lang], q->prefix);
    }
    else {
        snprintf(fn_base, 256, "%s/%s/%s/%s", ship->cfg->quests_dir,
            client_type[CLIENT_VERSION_DCV1]->ver_name, language_codes[lang],
                 q->prefix);
    }

    snprintf(filename, 260, "%s_%s.bin", fn_base, language_codes[lang]);
    bin = fopen(filename, "rb");

    if(!bin) {
        QERR_LOG("打开 %s BIN文件错误: %s", fn_base,
              strerror(errno));
        return -1;
    }

    snprintf(filename, 260, "%s_%s.dat", fn_base, language_codes[lang]);
    dat = fopen(filename, "rb");

    if(!dat) {
        QERR_LOG("打开 %s DAT文件错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        return -1;
    }

    /* Figure out how long each of the files are */
    fseek(bin, 0, SEEK_END);
    binlen = (uint32_t)ftell(bin);
    fseek(bin, 0, SEEK_SET);

    fseek(dat, 0, SEEK_END);
    datlen = (uint32_t)ftell(dat);
    fseek(dat, 0, SEEK_SET);

    /* Send the file info packets */
    /* Start with the .dat file. */
    memset(file, 0, sizeof(dc_quest_file_pkt));

    snprintf(file->name, 32, "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x02; /* ??? */
    file->hdr.pkt_len = LE16(DC_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.dat", q->prefix);
    file->length = LE32(datlen);

    if(crypt_send(c, DC_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now the .bin file. */
    memset(file, 0, sizeof(dc_quest_file_pkt));

    snprintf(file->name, 32, "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x02; /* ??? */
    file->hdr.pkt_len = LE16(DC_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.bin", q->prefix);
    file->length = LE32(binlen);

    if(crypt_send(c, DC_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s BIN文件数据头错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now send the chunks of the file, interleaved. */
    while(!bindone || !datdone) {
        /* Start with the dat file if we've got any more to send from it */
        if(!datdone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(dc_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.dc.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.dc.flags = (uint8_t)chunknum;
            chunk->hdr.dc.pkt_len = LE16(DC_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.dat", q->prefix);
            amt = fread(chunk->data, 1, 0x400, dat);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if(crypt_send(c, DC_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
                      strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if(amt != 0x400) {
                datdone = 1;
            }
        }

        /* Then the bin file if we've got any more to send from it */
        if(!bindone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(dc_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.dc.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.dc.flags = (uint8_t)chunknum;
            chunk->hdr.dc.pkt_len = LE16(DC_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.bin", q->prefix);
            amt = fread(chunk->data, 1, 0x400, bin);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if(crypt_send(c, DC_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("Error sending bin file %s: %s", fn_base,
                      strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if(amt != 0x400) {
                bindone = 1;
            }
        }

        ++chunknum;
    }

    /* We're done with the files, close them */
    fclose(bin);
    fclose(dat);

    return 0;
}

static int send_pc_quest(ship_client_t *c, quest_map_elem_t *qm, int v1,
                         int lang) {
    uint8_t *sendbuf = get_sendbuf();
    pc_quest_file_pkt *file = (pc_quest_file_pkt *)sendbuf;
    dc_quest_chunk_pkt *chunk = (dc_quest_chunk_pkt *)sendbuf;
    FILE *bin, *dat;
    uint32_t binlen, datlen;
    int bindone = 0, datdone = 0, chunknum = 0;
    char fn_base[256], filename[260];
    size_t amt;
    psocn_quest_t *q = qm->qptr[c->version][lang];

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf || !q) {
        return -1;
    }

    /* Each quest has two files: a .dat file and a .bin file, send a file packet
       for each of them. */
    if(!v1 || (q->versions & PSOCN_QUEST_V1)) {
        snprintf(fn_base, 256, "%s/%s/%s/%s", ship->cfg->quests_dir,
                 client_type[c->version]->ver_name, language_codes[lang], q->prefix);
    }
    else {
        snprintf(fn_base, 256, "%s/%s/%s/%sv1", ship->cfg->quests_dir,
                 client_type[c->version]->ver_name, language_codes[lang], q->prefix);
    }

    snprintf(filename, 260, "%s_%s.bin", fn_base, language_codes[lang]);
    bin = fopen(filename, "rb");

    if(!bin) {
        QERR_LOG("打开 %s BIN文件错误: %s", fn_base,
              strerror(errno));
        return -1;
    }

    snprintf(filename, 260, "%s_%s.dat", fn_base, language_codes[lang]);
    dat = fopen(filename, "rb");

    if(!dat) {
        QERR_LOG("打开 %s DAT文件错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        return -1;
    }

    /* Figure out how long each of the files are */
    fseek(bin, 0, SEEK_END);
    binlen = (uint32_t)ftell(bin);
    fseek(bin, 0, SEEK_SET);

    fseek(dat, 0, SEEK_END);
    datlen = (uint32_t)ftell(dat);
    fseek(dat, 0, SEEK_SET);

    /* Send the file info packets */
    /* Start with the .dat file. */
    memset(file, 0, sizeof(pc_quest_file_pkt));

    snprintf(file->name, 32, "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x00;
    file->hdr.pkt_len = LE16(DC_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.dat", q->prefix);
    file->length = LE32(datlen);
    file->flags = 0x0002;

    if(crypt_send(c, DC_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now the .bin file. */
    memset(file, 0, sizeof(pc_quest_file_pkt));

    snprintf(file->name, 32, "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x00;
    file->hdr.pkt_len = LE16(DC_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.bin", q->prefix);
    file->length = LE32(binlen);
    file->flags = 0x0002;

    if(crypt_send(c, DC_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s BIN文件数据头错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now send the chunks of the file, interleaved. */
    while(!bindone || !datdone) {
        /* Start with the dat file if we've got any more to send from it */
        if(!datdone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(dc_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.pc.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.pc.flags = (uint8_t)chunknum;
            chunk->hdr.pc.pkt_len = LE16(DC_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.dat", q->prefix);
            amt = fread(chunk->data, 1, 0x400, dat);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if(crypt_send(c, DC_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
                      strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if(amt != 0x400) {
                datdone = 1;
            }
        }

        /* Then the bin file if we've got any more to send from it */
        if(!bindone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(dc_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.pc.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.pc.flags = (uint8_t)chunknum;
            chunk->hdr.pc.pkt_len = LE16(DC_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.bin", q->prefix);
            amt = fread(chunk->data, 1, 0x400, bin);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if(crypt_send(c, DC_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("Error sending bin file %s: %s", fn_base,
                      strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if(amt != 0x400) {
                bindone = 1;
            }
        }

        ++chunknum;
    }

    /* We're done with the files, close them */
    fclose(bin);
    fclose(dat);

    return 0;
}

static int send_gc_quest(ship_client_t *c, quest_map_elem_t *qm, int v1,
                         int lang) {
    uint8_t *sendbuf = get_sendbuf();
    gc_quest_file_pkt *file = (gc_quest_file_pkt *)sendbuf;
    dc_quest_chunk_pkt *chunk = (dc_quest_chunk_pkt *)sendbuf;
    FILE *bin, *dat;
    uint32_t binlen, datlen;
    int bindone = 0, datdone = 0, chunknum = 0, v = c->version;
    char fn_base[256], filename[260];
    size_t amt;
    psocn_quest_t *q;

    if(v == CLIENT_VERSION_XBOX)
        v = CLIENT_VERSION_GC;

    q = qm->qptr[v][lang];

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf || !q) {
        return -1;
    }

    /* Each quest has two files: a .dat file and a .bin file, send a file packet
       for each of them. */
    if(!v1 || (q->versions & PSOCN_QUEST_V1)) {
        snprintf(fn_base, 256, "%s/%s/%s/%s", ship->cfg->quests_dir,
                 client_type[v]->ver_name, language_codes[lang], q->prefix);
    }
    else {
        snprintf(fn_base, 256, "%s/%s/%s/%sv1", ship->cfg->quests_dir,
                 client_type[v]->ver_name, language_codes[lang], q->prefix);
    }

    snprintf(filename, 260, "%s_%s.bin", fn_base, language_codes[lang]);
    bin = fopen(filename, "rb");

    if(!bin) {
        QERR_LOG("打开 %s BIN文件错误: %s", fn_base,
              strerror(errno));
        return -1;
    }

    snprintf(filename, 260, "%s_%s.dat", fn_base, language_codes[lang]);
    dat = fopen(filename, "rb");

    if(!dat) {
        QERR_LOG("打开 %s DAT文件错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        return -1;
    }

    /* Figure out how long each of the files are */
    fseek(bin, 0, SEEK_END);
    binlen = (uint32_t)ftell(bin);
    fseek(bin, 0, SEEK_SET);

    fseek(dat, 0, SEEK_END);
    datlen = (uint32_t)ftell(dat);
    fseek(dat, 0, SEEK_SET);

    /* Send the file info packets */
    /* Start with the .dat file. */
    memset(file, 0, sizeof(gc_quest_file_pkt));

    snprintf(file->name, 32, "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x00;
    file->hdr.pkt_len = LE16(DC_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.dat", q->prefix);
    file->length = LE32(datlen);
    file->flags = 0x0002;

    if(crypt_send(c, DC_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now the .bin file. */
    memset(file, 0, sizeof(gc_quest_file_pkt));

    snprintf(file->name, 32, "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x00;
    file->hdr.pkt_len = LE16(DC_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.bin", q->prefix);
    file->length = LE32(binlen);
    file->flags = 0x0002;

    if(crypt_send(c, DC_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s BIN文件数据头错误: %s", fn_base,
              strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now send the chunks of the file, interleaved. */
    while(!bindone || !datdone) {
        /* Start with the dat file if we've got any more to send from it */
        if(!datdone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(dc_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.dc.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.dc.flags = (uint8_t)chunknum;
            chunk->hdr.dc.pkt_len = LE16(DC_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.dat", q->prefix);
            amt = fread(chunk->data, 1, 0x400, dat);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if(crypt_send(c, DC_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
                      strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if(amt != 0x400) {
                datdone = 1;
            }
        }

        /* Then the bin file if we've got any more to send from it */
        if(!bindone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(dc_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.dc.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.dc.flags = (uint8_t)chunknum;
            chunk->hdr.dc.pkt_len = LE16(DC_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.bin", q->prefix);
            amt = fread(chunk->data, 1, 0x400, bin);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if(crypt_send(c, DC_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("Error sending bin file %s: %s", fn_base,
                      strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if(amt != 0x400) {
                bindone = 1;
            }
        }

        ++chunknum;
    }

    /* We're done with the files, close them */
    fclose(bin);
    fclose(dat);

    return 0;
}

static int send_bb_quest(ship_client_t* c, quest_map_elem_t* qm, int v1,
    int lang) {
    uint8_t* sendbuf = get_sendbuf();
    bb_quest_file_pkt* file = (bb_quest_file_pkt*)sendbuf;
    bb_quest_chunk_pkt* chunk = (bb_quest_chunk_pkt*)sendbuf;
    FILE* bin, * dat;
    uint32_t binlen, datlen;
    int bindone = 0, datdone = 0, chunknum = 0, v = c->version;
    char fn_base[256], filename[260];
    size_t amt;
    psocn_quest_t* q;

    if (v == CLIENT_VERSION_XBOX)
        v = CLIENT_VERSION_GC;

    q = qm->qptr[v][lang];

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf || !q) {
        return -1;
    }

    /* Each quest has two files: a .dat file and a .bin file, send a file packet
       for each of them. */
    if (!v1 || (q->versions & PSOCN_QUEST_V1)) {
        snprintf(fn_base, 256, "%s/%s/%s/%s", ship->cfg->quests_dir,
            client_type[v]->ver_name, language_codes[lang], q->prefix);
    }
    else {
        snprintf(fn_base, 256, "%s/%s/%s/%sv1", ship->cfg->quests_dir,
            client_type[v]->ver_name, language_codes[lang], q->prefix);
    }

    snprintf(filename, 260, "%s_%s.bin", fn_base, language_codes[lang]);
    bin = fopen(filename, "rb");

    if (!bin) {
        QERR_LOG("打开 %s BIN文件错误: %s", fn_base,
            strerror(errno));
        return -1;
    }

    snprintf(filename, 260, "%s_%s.dat", fn_base, language_codes[lang]);
    dat = fopen(filename, "rb");

    if (!dat) {
        QERR_LOG("打开 %s DAT文件错误: %s", fn_base,
            strerror(errno));
        fclose(bin);
        return -1;
    }

    /* Figure out how long each of the files are */
    fseek(bin, 0, SEEK_END);
    binlen = (uint32_t)ftell(bin);
    fseek(bin, 0, SEEK_SET);

    fseek(dat, 0, SEEK_END);
    datlen = (uint32_t)ftell(dat);
    fseek(dat, 0, SEEK_SET);

    /* Send the file info packets */
    /* Start with the .dat file. */
    memset(file, 0, sizeof(bb_quest_file_pkt));

    snprintf(file->name, sizeof(file->name), "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x00;
    file->hdr.pkt_len = LE16(BB_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.dat", q->prefix);
    file->length = LE32(datlen);
    file->flags = 0x0002;

    if (crypt_send(c, BB_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
            strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now the .bin file. */
    memset(file, 0, sizeof(bb_quest_file_pkt));

    snprintf(file->name, sizeof(file->name), "PSO/%-.27s", q->name);

    file->hdr.pkt_type = QUEST_FILE_TYPE;
    file->hdr.flags = 0x00;
    file->hdr.pkt_len = LE16(BB_QUEST_FILE_LENGTH);
    snprintf(file->filename, 16, "%-.11s.bin", q->prefix);
    file->length = LE32(binlen);
    file->flags = 0x0002;

    if (crypt_send(c, BB_QUEST_FILE_LENGTH, sendbuf)) {
        QERR_LOG("发送 %s BIN文件数据头错误: %s", fn_base,
            strerror(errno));
        fclose(bin);
        fclose(dat);
        return -2;
    }

    /* Now send the chunks of the file, interleaved. */
    while (!bindone || !datdone) {
        /* Start with the dat file if we've got any more to send from it */
        if (!datdone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(bb_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.flags = (uint8_t)chunknum;
            chunk->hdr.pkt_len = LE16(BB_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, sizeof(chunk->filename), "%-.11s.dat", q->prefix);
            amt = fread(chunk->data, 1, 0x400, dat);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if (crypt_send(c, BB_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("发送 %s DAT文件数据头错误: %s", fn_base,
                    strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if (amt != 0x400) {
                datdone = 1;
            }
        }

        /* Then the bin file if we've got any more to send from it */
        if (!bindone) {
            /* Clear the packet */
            memset(chunk, 0, sizeof(bb_quest_chunk_pkt));

            /* 填充数据头 */
            chunk->hdr.pkt_type = QUEST_CHUNK_TYPE;
            chunk->hdr.flags = (uint8_t)chunknum;
            chunk->hdr.pkt_len = LE16(BB_QUEST_CHUNK_LENGTH);

            /* Fill in the rest */
            snprintf(chunk->filename, 16, "%-.11s.bin", q->prefix);
            amt = fread(chunk->data, 1, 0x400, bin);
            chunk->length = LE32(((uint32_t)amt));

            /* 加密并发送 */
            if (crypt_send(c, BB_QUEST_CHUNK_LENGTH, sendbuf)) {
                QERR_LOG("Error sending bin file %s: %s", fn_base,
                    strerror(errno));
                fclose(bin);
                fclose(dat);
                return -3;
            }

            /* Are we done with this file? */
            if (amt != 0x400) {
                bindone = 1;
            }
        }

        ++chunknum;
    }

    /* We're done with the files, close them */
    fclose(bin);
    fclose(dat);

    return 0;
}

static int send_qst_quest(ship_client_t *c, quest_map_elem_t *qm, int v1,
                          int lang, int ver) {
    char filename[256];
    FILE *fp;
    long len;
    size_t read;
    uint8_t *sendbuf = get_sendbuf();
    psocn_quest_t *q = qm->qptr[ver][lang];

    /* Make sure we got the sendbuf and the quest */
    if(!sendbuf || !q)
        return -1;

    /* Figure out what file we're going to send. */
    if(!v1 || (q->versions & PSOCN_QUEST_V1)) {
        if(c->version != CLIENT_VERSION_XBOX)
            snprintf(filename, 256, "%s/%s/%s/%s_%s.qst", ship->cfg->quests_dir,
                client_type[c->version]->ver_name, language_codes[lang], q->prefix, language_codes[lang]);
        else
            snprintf(filename, 256, "%s/%s/%s/%s_%s.qst", ship->cfg->quests_dir,
                client_type[CLIENT_VERSION_GC]->ver_name, language_codes[lang],
                     q->prefix, language_codes[lang]);
    }
    else {
        switch(c->version) {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
                snprintf(filename, 256, "%s/%s/%s/%s_%s.qst",
                         ship->cfg->quests_dir,
                         client_type[CLIENT_VERSION_DCV1]->ver_name,
                         language_codes[lang], q->prefix, language_codes[lang]);
                break;

            case CLIENT_VERSION_PC:
            case CLIENT_VERSION_GC:
                snprintf(filename, 256, "%s/%s/%s/%sv1_%s.qst",
                         ship->cfg->quests_dir, client_type[c->version]->ver_name,
                         language_codes[lang], q->prefix, language_codes[lang]);
                break;

            case CLIENT_VERSION_EP3:
                break;

            case CLIENT_VERSION_BB:
                snprintf(filename, 256, "%s/%s/%s/quest%s_%s.qst",
                    ship->cfg->quests_dir,
                    client_type[c->version]->ver_name, language_codes[lang],
                    q->prefix, language_codes[lang]);
                break;

            case CLIENT_VERSION_XBOX:
                snprintf(filename, 256, "%s/%s/%s/%sv1_%s.qst",
                         ship->cfg->quests_dir,
                         client_type[CLIENT_VERSION_GC]->ver_name, language_codes[lang],
                         q->prefix, language_codes[lang]);
                break;
        }
    }

    fp = fopen(filename, "rb");

    if(!fp) {
        QERR_LOG("无法打开 qst 任务文件 %s: %s", filename,
              strerror(errno));
        return -1;
    }

    /* Figure out how long the file is. */
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Copy the file (in chunks if necessary) to the sendbuf to actually send
       away. */
    while(len) {
        read = fread(sendbuf, 1, 65536, fp);

        /* If we can't read from the file, bail. */
        if(!read) {
            QERR_LOG("读取 qst 任务文件错误 %s: %s", filename,
                  strerror(errno));
            fclose(fp);
            return -2;
        }

        /* Make sure we read up to a header-size boundary. */
        if((read & (c->hdr_size - 1)) && !feof(fp)) {
            long amt = (read & (c->hdr_size - 1));

            fseek(fp, -amt, SEEK_CUR);
            read -= amt;
        }

        /* Send this chunk away. */
        if(crypt_send(c, read, sendbuf)) {
            QERR_LOG("发送 qst 任务文件错误 %s: %s", filename,
                  strerror(errno));
            fclose(fp);
            return -3;
        }

        len -= read;
    }

    /* We're finished. */
    fclose(fp);
    return 0;
}

int send_quest(lobby_t *l, uint32_t qid, int lc) {
    int i;
    int v1 = 0, rv;
    quest_map_elem_t *elem = quest_lookup(&ship->qmap, qid);
    psocn_quest_t *q;
    ship_client_t *c;
    int lang, ver;

    /* Make sure we get the quest */
    if(!elem)
        return -1;

    /* See if we're looking for a v1-compat quest */
    if(!l->v2 && l->version < CLIENT_VERSION_GC)
        v1 = 1;

    for(i = 0; i < l->max_clients; ++i) {
        if((c = l->clients[i])) {
            c->flags &= ~CLIENT_FLAG_QLOAD_DONE;

            /* What type of quest file are we sending? */
            if(v1 && c->version == CLIENT_VERSION_DCV2)
                ver = CLIENT_VERSION_DCV1;
            else if(c->version == CLIENT_VERSION_XBOX)
                ver = CLIENT_VERSION_GC;
            else
                ver = c->version;

            q = elem->qptr[ver][c->q_lang];
            lang = c->q_lang;

            /* If we didn't find it on the quest language code, try the language
               code set in the character data. */
            if(!q) {
                q = elem->qptr[ver][c->language_code];
                lang = c->language_code;
            }

            /* Next try English, so as to have a reasonably sane fallback. */
            if(!q) {
                q = elem->qptr[ver][CLIENT_LANG_ENGLISH];
                lang = CLIENT_LANG_ENGLISH;
            }

            /* If all else fails, go with the language the quest was selected by
               the leader in, since that has to be there! */
            if(!q) {
                q = elem->qptr[ver][lc];
                lang = lc;

                /* If we still didn't find it, we've got trouble elsewhere... */
                if(!q) {
                    QERR_LOG("未找到可发送的任务!\n"
                          "ID: %d, 版本: %d, 语言: %d, 备用: %d, "
                          "备用 2: %d", qid, ver, c->q_lang,
                          c->language_code, lc);

                    /* Unfortunately, we're going to have to disconnect the user
                       if this happens, since we really have no recourse. */
                    c->flags |= CLIENT_FLAG_DISCONNECTED;
                    continue;
                }
            }

            /* Set the counter we'll use to prevent v1 from sending silly
               packets to change other players' positions. */
            if(c->version == CLIENT_VERSION_DCV1)
                c->autoreply_len = l->num_clients - 1;

            if(q->format == PSOCN_QUEST_BINDAT) {
                /* Call the appropriate function. */
                switch(ver) {
                    case CLIENT_VERSION_DCV1:
                        rv = send_dcv1_quest(c, elem, v1, lang);
                        break;

                    case CLIENT_VERSION_DCV2:
                        rv = send_dcv2_quest(c, elem, v1, lang);
                        break;

                    case CLIENT_VERSION_PC:
                        rv = send_pc_quest(c, elem, v1, lang);
                        break;

                    case CLIENT_VERSION_GC:
                    case CLIENT_VERSION_XBOX:
                        rv = send_gc_quest(c, elem, v1, lang);
                        break;

                    case CLIENT_VERSION_BB:
                        rv = send_bb_quest(c, elem, v1, lang);
                        break;

                    case CLIENT_VERSION_EP3:    /* XXXX */
                    default:
                        return -1;
                }
            }
            else if(q->format == PSOCN_QUEST_QST) {
                rv = send_qst_quest(c, elem, v1, lang, ver);
            }
            else {
                return -1;
            }

            if(rv) {
                send_msg(c, MSG_BOX_TYPE, "Error reading quest file!\nPlease report "
                                 "this problem!\nInclude your guildcard\n"
                                 "number and the approximate\ntime in your "
                                 "error report.");
                c->flags |= CLIENT_FLAG_DISCONNECTED;
            }
        }
    }

    return 0;
}

int send_quest_one(lobby_t *l, ship_client_t *c, uint32_t qid, int lc) {
    int v1 = 0, rv;
    quest_map_elem_t *elem = quest_lookup(&ship->qmap, qid);
    psocn_quest_t *q;
    int lang, ver;

    /* Make sure we get the quest */
    if(!elem)
        return -1;

    /* See if we're looking for a v1-compat quest */
    if(!l->v2 && l->version < CLIENT_VERSION_GC)
        v1 = 1;

    c->flags &= ~CLIENT_FLAG_QLOAD_DONE;

    /* What type of quest file are we sending? */
    if(v1 && c->version == CLIENT_VERSION_DCV2)
        ver = CLIENT_VERSION_DCV1;
    else if(c->version == CLIENT_VERSION_XBOX)
        ver = CLIENT_VERSION_GC;
    else
        ver = c->version;

    q = elem->qptr[ver][c->q_lang];
    lang = c->q_lang;

    /* If we didn't find it on the quest language code, try the language
       code set in the character data. */
    if(!q) {
        q = elem->qptr[ver][c->language_code];
        lang = c->language_code;
    }

    /* Next try English, so as to have a reasonably sane fallback. */
    if(!q) {
        q = elem->qptr[ver][CLIENT_LANG_ENGLISH];
        lang = CLIENT_LANG_ENGLISH;
    }

    /* If all else fails, go with the language the quest was selected by
       the leader in, since that has to be there! */
    if(!q) {
        q = elem->qptr[ver][lc];
        lang = lc;

        /* If we still didn't find it, we've got trouble elsewhere... */
        if(!q) {
            QERR_LOG("未找到任务!\n"
                  "ID: %d, 版本: %d, 语言: %d, 备用: %d, "
                  "备用 2: %d", qid, ver, c->q_lang,
                  c->language_code, lc);

            /* Unfortunately, we're going to have to disconnect the user
               if this happens, since we really have no recourse. */
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return -1;
        }
    }

    /* Set the counter we'll use to prevent v1 from sending silly
       packets to change other players' positions. */
    if(c->version == CLIENT_VERSION_DCV1)
        c->autoreply_len = l->num_clients - 1;

    if(q->format == PSOCN_QUEST_BINDAT) {
        /* Call the appropriate function. */
        switch(ver) {
            case CLIENT_VERSION_DCV1:
                rv = send_dcv1_quest(c, elem, v1, lang);
                break;

            case CLIENT_VERSION_DCV2:
                rv = send_dcv2_quest(c, elem, v1, lang);
                break;

            case CLIENT_VERSION_PC:
                rv = send_pc_quest(c, elem, v1, lang);
                break;

            case CLIENT_VERSION_GC:
            case CLIENT_VERSION_XBOX:
                rv = send_gc_quest(c, elem, v1, lang);
                break;

            case CLIENT_VERSION_BB:
                rv = send_bb_quest(c, elem, v1, lang);
                break;

            case CLIENT_VERSION_EP3:    /* XXXX */
            default:
                return -1;
        }
    }
    else if(q->format == PSOCN_QUEST_QST) {
        rv = send_qst_quest(c, elem, v1, lang, ver);
    }
    else {
        return -1;
    }

    if(rv) {
        send_msg(c, MSG_BOX_TYPE, "读取任务文件错误!\n请上报"
                         "此错误!\n内容包含您的GC\n"
                         "和大约发生错误\n的时间.");
        c->flags |= CLIENT_FLAG_DISCONNECTED;
    }

    return 0;
}

/* Send the lobby name to the client. */
static int send_dcv2_lobby_name(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    dc_msg_box_pkt *pkt = (dc_msg_box_pkt *)sendbuf;
    uint16_t len;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Fill in the basics */
    pkt->hdr.dc.pkt_type = LOBBY_NAME_TYPE;
    pkt->hdr.dc.flags = 0;

    /* Fill in the message */
    if(l->name[0] == '\t' && l->name[1] == 'J') {
        istrncpy(ic_utf8_to_sjis, pkt->msg, l->name, 16);
        pkt->msg[16] = 0;
    }
    else {
        istrncpy(ic_utf8_to_8859, pkt->msg, l->name, 16);
        pkt->msg[16] = 0;
    }

    len = (uint16_t)strlen(pkt->msg) + 1;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* Fill in the length */
    len += 0x04;
    pkt->hdr.dc.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_lobby_name(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    dc_msg_box_pkt *pkt = (dc_msg_box_pkt *)sendbuf;
    uint16_t len;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Convert the message to the appropriate encoding. */
    in = strlen(l->name) + 1;
    out = 65532;
    inptr = l->name;
    outptr = pkt->msg;
    iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65532 - (uint16_t)out;

    /* Fill in the basics */
    pkt->hdr.pc.pkt_type = LOBBY_NAME_TYPE;
    pkt->hdr.pc.flags = 0;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* Fill in the length */
    len += 0x04;
    pkt->hdr.pc.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_lobby_name(ship_client_t *c, lobby_t *l) {
    uint8_t *sendbuf = get_sendbuf();
    bb_msg_box_pkt *pkt = (bb_msg_box_pkt *)sendbuf;
    uint16_t len;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Convert the message to the appropriate encoding. */
    in = strlen(l->name) + 1;
    out = 65532;
    inptr = l->name;
    outptr = pkt->msg;
    iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65532 - (uint16_t)out;

    /* Fill in the basics */
    pkt->hdr.pkt_type = LE16(LOBBY_NAME_TYPE);
    pkt->hdr.flags = 0;

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* Fill in the length */
    len += 0x08;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

int send_lobby_name(ship_client_t *c, lobby_t *l) {
    /* If they're not currently in a lobby of some sort (this normally shouldn't
       happen, but does on occasion), then we have nothing to do here... */
    if(!l)
        return 0;

    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dcv2_lobby_name(c, l);

        case CLIENT_VERSION_PC:
            return send_pc_lobby_name(c, l);

        case CLIENT_VERSION_BB:
            return send_bb_lobby_name(c, l);
    }

    return -1;
}

/* Send a packet to all clients in the lobby letting them know about a change to
   the arrows displayed. */
static int send_dc_lobby_arrows(lobby_t *l, ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    dc_arrow_list_pkt *pkt = (dc_arrow_list_pkt *)sendbuf;
    int clients = 0, len = 0x04, i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(dc_arrow_list_pkt));

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            /* Copy the player's data into the packet. */
            pkt->entries[clients].player_tag = LE32(0x00010000);
            pkt->entries[clients].guildcard = LE32(l->clients[i]->guildcard);
            pkt->entries[clients].arrow_color = LE32(l->clients[i]->arrow_color);

            ++clients;
            len += 0x0C;
        }
    }

    /* Fill in the rest of it */
    if(c->version == CLIENT_VERSION_DCV2 || c->version == CLIENT_VERSION_GC ||
       c->version == CLIENT_VERSION_EP3 || c->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = LOBBY_ARROW_LIST_TYPE;
        pkt->hdr.dc.flags = (uint8_t)clients;
        pkt->hdr.dc.pkt_len = LE16(((uint16_t)len));
    }
    else {
        pkt->hdr.pc.pkt_type = LOBBY_ARROW_LIST_TYPE;
        pkt->hdr.pc.flags = (uint8_t)clients;
        pkt->hdr.pc.pkt_len = LE16(((uint16_t)len));
    }

    /* Don't send anything if we have no clients. */
    if(!clients)
        return 0;

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_lobby_arrows(lobby_t *l, ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    bb_arrow_list_pkt *pkt = (bb_arrow_list_pkt *)sendbuf;
    int clients = 0, len = 0x08, i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, sizeof(bb_arrow_list_pkt));

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            /* Copy the player's data into the packet. */
            pkt->entries[clients].player_tag = LE32(0x00010000);
            pkt->entries[clients].guildcard = LE32(l->clients[i]->guildcard);
            pkt->entries[clients].arrow_color = LE32(l->clients[i]->arrow_color);

            ++clients;
            len += 0x0C;
        }
    }

    /* Fill in the rest of it */
    pkt->hdr.pkt_type = LE16(LOBBY_ARROW_LIST_TYPE);
    pkt->hdr.flags = LE32(clients);
    pkt->hdr.pkt_len = LE16(((uint16_t)len));

    /* Don't send anything if we have no clients. */
    if(!clients)
        return 0;

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

int send_lobby_arrows(lobby_t *l) {
    int i;

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                    /* V1 doesn't support this and will disconnect if it gets
                       this packet. */
                    break;

                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_PC:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    send_dc_lobby_arrows(l, l->clients[i]);
                    break;

                case CLIENT_VERSION_BB:
                    send_bb_lobby_arrows(l, l->clients[i]);
                    break;
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    return 0;
}

/* Send a packet to ONE client letting them know about the arrow colors in the
   given lobby. */
int send_arrows(ship_client_t *c, lobby_t *l) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
            /* V1 doesn't support this and will disconnect if it gets
               this packet. */
            break;

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_lobby_arrows(l, c);

        case CLIENT_VERSION_BB:
            return send_bb_lobby_arrows(l, c);
    }

    return -1;
}

static int is_ship_menu_empty(ship_client_t *c, ship_t *s, uint16_t menu_code) {
    int isgm = GLOBAL_GM(c);
    int ver;
    miniship_t *i;

    switch(c->version) {
        case CLIENT_VERSION_DCV1:
            if((c->flags & CLIENT_FLAG_IS_NTE))
                ver = LOGIN_FLAG_NODCNTE;
            else
                ver = LOGIN_FLAG_NOV1;
            break;

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_BB:
            ver = LOGIN_FLAG_NOV1 << c->version;
            break;

        case CLIENT_VERSION_XBOX:
            ver = LOGIN_FLAG_NOXBOX;
            break;

        case CLIENT_VERSION_PC:
            if((c->flags & CLIENT_FLAG_IS_NTE))
                ver = LOGIN_FLAG_NOPCNTE;
            else
                ver = LOGIN_FLAG_NOPC;
            break;

        default:
            return -1;
    }

    /* Look through the list of ships for one that is in that menu that the user
       can actually see... */
    TAILQ_FOREACH(i, &s->ships, qentry) {
        if(i->ship_id && i->menu_code == menu_code) {
            if((i->flags & LOGIN_FLAG_GMONLY) && !isgm)
                continue;
            else if((i->flags & ver))
                continue;
            else if((i->privileges & c->privilege) != i->privileges && !isgm)
                continue;

            /* 如果我们到了这里, then we found a ship the user can see, so the
               menu isn't empty. */
            return 0;
        }
    }

    /* If we didn't find anything above, we've got an empty menu from the user's
       point of view. */
    return 1;
}

/* Send a ship list packet to the client. */
static int send_ship_list_dc(ship_client_t *c, ship_t *s, uint16_t menu_code) {
    uint8_t *sendbuf = get_sendbuf();
    dc_ship_list_pkt *pkt = (dc_ship_list_pkt *)sendbuf;
    int len = 0x20, entries = 0, j;
    miniship_t *i;
    char tmp[18] = { 0 }, tmp2[3] = { 0 };

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, 0x20);

    /* Fill in the basics. */
    if(!(c->flags & CLIENT_FLAG_IS_NTE))
        pkt->hdr.pkt_type = SHIP_LIST_TYPE;
    else
        pkt->hdr.pkt_type = DCNTE_SHIP_LIST_TYPE;

    /* Fill in the "SHIP/US" entry */
    memset(&pkt->entries[entries], 0, 0x1C);
    pkt->entries[entries].menu_id = LE32(MENU_ID_SHIP);
    pkt->entries[entries].item_id = 0;
    pkt->entries[entries].flags = LE16(0x0004);
    strcpy(pkt->entries[entries].name, "SHIP/US");
    pkt->entries[entries].name[0x11] = 0x08;

    ++entries;

    tmp2[0] = (char)(menu_code);
    tmp2[1] = (char)(menu_code >> 8);
    tmp2[2] = '\0';

    TAILQ_FOREACH(i, &s->ships, qentry) {
        if(i->ship_id && i->menu_code == menu_code) {
            if((i->flags & LOGIN_FLAG_GMONLY) &&
               !(c->privilege & CLIENT_PRIV_GLOBAL_GM))
                continue;

            if(c->flags & CLIENT_FLAG_IS_NTE) {
                if(i->flags & LOGIN_FLAG_NODCNTE)
                    continue;
            }
            else if((i->privileges & c->privilege) != i->privileges &&
                    !GLOBAL_GM(c)) {
                continue;
            }

            if(c->version == CLIENT_VERSION_XBOX) {
                if((i->flags & LOGIN_FLAG_NOXBOX))
                    continue;
            }
            else if((i->flags & (LOGIN_FLAG_NOV1 << c->version))) {
                continue;
            }

            /* Clear the new entry */
            memset(&pkt->entries[entries], 0, 0x1C);

            /* Copy the ship's information to the packet. */
            pkt->entries[entries].menu_id = LE32(MENU_ID_SHIP);
            pkt->entries[entries].item_id = LE32(i->ship_id);
            pkt->entries[entries].flags = 0;

            sprintf(tmp, "%02x:%s%s%s", i->ship_number, tmp2,
                tmp2[0] ? "/" : "", i->name);

            /* Convert the name to UTF-8 */
            istrncpy(ic_gbk_to_utf8, (char*)pkt->entries[entries].name, tmp, 0x22);

            ++entries;
            len += 0x1C;
        }
    }

    /* Fill in the menu codes */
    for(j = 0; j < s->mccount; ++j) {
        if(s->menu_codes[j] != menu_code) {
            if(is_ship_menu_empty(c, s, s->menu_codes[j]))
                continue;

            tmp2[0] = (char)(s->menu_codes[j]);
            tmp2[1] = (char)(s->menu_codes[j] >> 8);
            tmp2[2] = '\0';

            /* Make sure the values are in-bounds */
            if((tmp2[0] || tmp2[1]) && (!isalpha(tmp2[0]) || !isalpha(tmp2[1])))
                continue;

            /* Clear out the ship information */
            memset(&pkt->entries[entries], 0, 0x1C);

            /* Fill in what we have */
            pkt->entries[entries].menu_id = LE32((MENU_ID_SHIP |
                                                  (s->menu_codes[j] << 8)));
            pkt->entries[entries].item_id = LE32(ITEM_ID_EMPTY);
            pkt->entries[entries].flags = LE16(0x0000);

            /* Create the name string */
            if (tmp2[0] && tmp2[1])
                sprintf(tmp, "舰船/%s", tmp2);
            else
                strcpy(tmp, "舰船/Main");

            /* And convert to UTF-8 */
            istrncpy(ic_gbk_to_utf8, (char*)pkt->entries[entries].name, tmp,
                0x22);

            /* We're done with this ship, increment the counter */
            ++entries;
            len += 0x1C;
        }
    }

    /* Make sure we have at least one ship... */
    if (entries == 1) {
        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x2C);
        pkt->entries[entries].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[entries].item_id = LE32(ITEM_ID_DISCONNECT);
        pkt->entries[entries].flags = LE16(0x0000);

        /* And convert to UTF8 */
        istrncpy(ic_gbk_to_utf8, (char*)pkt->entries[entries].name, "断开连接", 0x22);

        ++entries;
        len += 0x2C;
    }

    /* We'll definitely have at least one ship (ourselves), so just fill in the
       rest of it */
    pkt->hdr.pkt_len = LE16(((uint16_t)len));
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_ship_list_pc(ship_client_t *c, ship_t *s, uint16_t menu_code) {
    uint8_t *sendbuf = get_sendbuf();
    pc_ship_list_pkt *pkt = (pc_ship_list_pkt *)sendbuf;
    int len = 0x30, entries = 0, j;
    miniship_t *i;
    char tmp[18] = { 0 }, tmp2[3] = { 0 };

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, 0x30);

    /* Fill in the basics. */
    pkt->hdr.pkt_type = SHIP_LIST_TYPE;

    /* Fill in the "SHIP/US" entry */
    memset(&pkt->entries[entries], 0, 0x1C);
    pkt->entries[entries].menu_id = LE32(MENU_ID_SHIP);
    pkt->entries[entries].item_id = 0;
    pkt->entries[entries].flags = LE16(0x0004);
    memcpy(pkt->entries[entries].name, "S\0H\0I\0P\0/\0U\0S\0", 14);
    ++entries;

    tmp2[0] = (char)(menu_code);
    tmp2[1] = (char)(menu_code >> 8);
    tmp2[2] = '\0';

    TAILQ_FOREACH(i, &s->ships, qentry) {
        if(i->ship_id && i->menu_code == menu_code) {
            if((i->flags & LOGIN_FLAG_GMONLY) &&
               !(c->privilege & CLIENT_PRIV_GLOBAL_GM)) {
                continue;
            }
            else if(c->flags & CLIENT_FLAG_IS_NTE) {
                if(i->flags & LOGIN_FLAG_NOPCNTE)
                    continue;
            }
            else if((i->flags & (LOGIN_FLAG_NOV1 << c->version))) {
                continue;
            }
            else if((i->privileges & c->privilege) != i->privileges &&
                    !GLOBAL_GM(c)) {
                continue;
            }

            /* Clear the new entry */
            memset(&pkt->entries[entries], 0, 0x2C);

            /* Copy the ship's information to the packet. */
            pkt->entries[entries].menu_id = LE32(MENU_ID_SHIP);
            pkt->entries[entries].item_id = LE32(i->ship_id);
            pkt->entries[entries].flags = 0;

            sprintf(tmp, "%02x:%s%s%s", i->ship_number, tmp2,
                    tmp2[0] ? "/" : "", i->name);

            /* Convert the name to UTF-16 */
            istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name, tmp, 0x22);

            /* We're done with this ship, increment the counter */
            ++entries;
            len += 0x2C;
        }
    }

    /* Fill in the menu codes */
    for(j = 0; j < s->mccount; ++j) {
        if(s->menu_codes[j] != menu_code) {
            if(is_ship_menu_empty(c, s, s->menu_codes[j]))
                continue;

            tmp2[0] = (char)(s->menu_codes[j]);
            tmp2[1] = (char)(s->menu_codes[j] >> 8);
            tmp2[2] = '\0';

            /* Make sure the values are in-bounds */
            if((tmp2[0] || tmp2[1]) && (!isalpha(tmp2[0]) || !isalpha(tmp2[1])))
                continue;

            /* Clear out the ship information */
            memset(&pkt->entries[entries], 0, 0x2C);

            /* Fill in what we have */
            pkt->entries[entries].menu_id = LE32((MENU_ID_SHIP |
                                                  (s->menu_codes[j] << 8)));
            pkt->entries[entries].item_id = LE32(ITEM_ID_EMPTY);
            pkt->entries[entries].flags = LE16(0x0000);

            /* Create the name string (UTF-8) */
            if(tmp2[0] && tmp2[1])
                sprintf(tmp, "舰船/%s", tmp2);
            else
                strcpy(tmp, "舰船/Main");

            /* And convert to UTF-16 */
            istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name, tmp,
                     0x22);

            /* We're done with this "ship", increment the counter */
            ++entries;
            len += 0x2C;
        }
    }

    /* Make sure we have at least one ship... */
    if (entries == 1) {
        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x2C);
        pkt->entries[entries].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[entries].item_id = LE32(ITEM_ID_DISCONNECT);
        pkt->entries[entries].flags = LE16(0x0000);

        /* And convert to UTF16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, "断开连接", 0x22);

        ++entries;
        len += 0x2C;
    }

    /* We'll definitely have at least one ship (ourselves), so just fill in the
       rest of it */
    pkt->hdr.pkt_len = LE16(((uint16_t)len));
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

static int send_ship_list_bb(ship_client_t *c, ship_t *s, uint16_t menu_code) {
    uint8_t *sendbuf = get_sendbuf();
    bb_ship_list_pkt *pkt = (bb_ship_list_pkt *)sendbuf;
    int len = 0x34, entries = 0, j;
    miniship_t *i;
    char tmp[18] = { 0 }, tmp2[3] = { 0 };

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the packet's header. */
    memset(pkt, 0, 0x30);

    /* Fill in the basics. */
    pkt->hdr.pkt_type = LE16(SHIP_LIST_TYPE);

    /* Fill in the "SHIP/US" entry */
    memset(&pkt->entries[entries], 0, 0x1C);
    pkt->entries[entries].menu_id = LE32(MENU_ID_SHIP);
    pkt->entries[entries].item_id = 0;
    pkt->entries[entries].flags = LE16(0x0004);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, "S\0H\0I\0P\0/\0U\0S\0", 14);

    ++entries;

    tmp2[0] = (char)(menu_code);
    tmp2[1] = (char)(menu_code >> 8);
    tmp2[2] = '\0';

    TAILQ_FOREACH(i, &s->ships, qentry) {
        if(i->ship_id && i->menu_code == menu_code) {
            if((i->flags & LOGIN_FLAG_GMONLY) &&
               !(c->privilege & CLIENT_PRIV_GLOBAL_GM)) {
                continue;
            }
            else if((i->flags & LOGIN_FLAG_NOBB)) {
                continue;
            }
            else if((i->privileges & c->privilege) != i->privileges &&
                    !GLOBAL_GM(c)) {
                continue;
            }

            /* Clear the new entry */
            memset(&pkt->entries[entries], 0, 0x2C);

            /* Copy the ship's information to the packet. */
            pkt->entries[entries].menu_id = LE32(MENU_ID_SHIP);
            pkt->entries[entries].item_id = LE32(i->ship_id);
            pkt->entries[entries].flags = 0;

            sprintf(tmp, "%02x:%s%s%s", i->ship_number, tmp2,
                    tmp2[0] ? "/" : "", i->name);

            /* Convert the name to UTF-16 */
            istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name, tmp,
                     0x22);

            ++entries;
            len += 0x2C;
        }
    }

    /* Fill in the menu codes */
    for(j = 0; j < s->mccount; ++j) {
        if(s->menu_codes[j] != menu_code) {
            if(is_ship_menu_empty(c, s, s->menu_codes[j]))
                continue;

            tmp2[0] = (char)(s->menu_codes[j]);
            tmp2[1] = (char)(s->menu_codes[j] >> 8);
            tmp2[2] = '\0';

            /* Make sure the values are in-bounds */
            if((tmp2[0] || tmp2[1]) &&
               (!isalpha(tmp2[0]) || !isalpha(tmp2[1]))) {
                continue;
            }

            /* Clear out the ship information */
            memset(&pkt->entries[entries], 0, 0x2C);

            /* Fill in what we have */
            pkt->entries[entries].menu_id = LE32((MENU_ID_SHIP |
                                                  (s->menu_codes[j] << 8)));
            pkt->entries[entries].item_id = LE32(ITEM_ID_EMPTY);
            pkt->entries[entries].flags = LE16(0x0000);

            /* Create the name string (UTF-8) */
            if(tmp2[0] && tmp2[1])
                sprintf(tmp, "舰船/%s", tmp2);
            else
                strcpy(tmp, "舰船/Main");

            /* And convert to UTF-16 */
            istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name, tmp,
                     0x22);

            /* We're done with this "ship", increment the counter */
            ++entries;
            len += 0x2C;
        }
    }

    /* Make sure we have at least one ship... */
    if (entries == 1) {
        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x2C);
        pkt->entries[entries].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[entries].item_id = LE32(ITEM_ID_DISCONNECT);
        pkt->entries[entries].flags = LE16(0x0000);

        /* And convert to UTF16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, "断开连接", 0x22);

        ++entries;
        len += 0x2C;
    }

    /* We'll definitely have at least one ship (ourselves), so just fill in the
       rest of it */
    pkt->hdr.pkt_len = LE16(((uint16_t)len));
    pkt->hdr.flags = LE32(entries - 1);

    /* 加密并发送 */
    return crypt_send(c, len, sendbuf);
}

int send_ship_list(ship_client_t *c, ship_t *s, uint16_t menu_code) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_ship_list_dc(c, s, menu_code);

        case CLIENT_VERSION_PC:
            return send_ship_list_pc(c, s, menu_code);

        case CLIENT_VERSION_BB:
            return send_ship_list_bb(c, s, menu_code);
    }

    return -1;
}

/* Send a warp command to the client. */
static int send_dc_warp(ship_client_t *c, uint8_t area) {
    uint8_t *sendbuf = get_sendbuf();
    dc_pkt_hdr_t *pkt = (dc_pkt_hdr_t *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Fill in the basics. */
    pkt->pkt_type = GAME_COMMAND2_TYPE;
    pkt->flags = c->client_id;
    pkt->pkt_len = LE16(0x000C);

    /* Fill in the stuff that will make us warp. */
    sendbuf[4] = SUBCMD60_WARP;
    sendbuf[5] = 0x02;
    sendbuf[6] = c->client_id;
    sendbuf[7] = 0x00;
    sendbuf[8] = area;
    sendbuf[9] = 0x00;
    sendbuf[10] = 0x00;
    sendbuf[11] = 0x00;

    return crypt_send(c, 12, sendbuf);
}

static int send_pc_warp(ship_client_t *c, uint8_t area) {
    uint8_t *sendbuf = get_sendbuf();
    pc_pkt_hdr_t *pkt = (pc_pkt_hdr_t *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Fill in the basics. */
    pkt->pkt_type = GAME_COMMAND2_TYPE;
    pkt->flags = c->client_id;
    pkt->pkt_len = LE16(0x000C);

    /* Fill in the stuff that will make us warp. */
    sendbuf[4] = SUBCMD60_WARP;
    sendbuf[5] = 0x02;
    sendbuf[6] = c->client_id;
    sendbuf[7] = 0x00;
    sendbuf[8] = area;
    sendbuf[9] = 0x00;
    sendbuf[10] = 0x00;
    sendbuf[11] = 0x00;

    return crypt_send(c, 12, sendbuf);
}

static int send_bb_warp(ship_client_t *c, uint8_t area) {
    uint8_t *sendbuf = get_sendbuf();
    bb_pkt_hdr_t *pkt = (bb_pkt_hdr_t *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Fill in the basics. */
    pkt->pkt_type = LE16(GAME_COMMAND2_TYPE);
    pkt->flags = c->client_id;
    pkt->pkt_len = LE16(0x0010);

    /* Fill in the stuff that will make us warp. */
    sendbuf[8] = SUBCMD60_WARP;
    sendbuf[9] = 0x02;
    sendbuf[10] = c->client_id;
    sendbuf[11] = 0x00;
    sendbuf[12] = area;
    sendbuf[13] = 0x00;
    sendbuf[14] = 0x00;
    sendbuf[15] = 0x00;

    return crypt_send(c, 16, sendbuf);
}

int send_warp(ship_client_t *c, uint8_t area) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_warp(c, area);

        case CLIENT_VERSION_PC:
            return send_pc_warp(c, area);

        case CLIENT_VERSION_BB:
            return send_bb_warp(c, area);
    }

    return -1;
}

int send_lobby_warp(lobby_t *l, uint8_t area) {
    int i;

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    send_dc_warp(l->clients[i], area);
                    break;

                case CLIENT_VERSION_PC:
                    send_pc_warp(l->clients[i], area);
                    break;

                case CLIENT_VERSION_BB:
                    send_bb_warp(l->clients[i], area);
                    break;
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    return 0;
}

/* Send the choice search option list to the client. */
static int send_dc_choice_search(ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    dc_choice_search_pkt *pkt = (dc_choice_search_pkt *)sendbuf;
    uint16_t len = 4 + 0x20 * CS_OPTIONS_COUNT;
    int i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Fill in the basics. */
    pkt->hdr.pkt_type = CHOICE_OPTION_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = CS_OPTIONS_COUNT;

    /* Copy the options over. */
    for(i = 0; i < CS_OPTIONS_COUNT; ++i) {
        memset(&pkt->entries[i], 0, 0x20);
        pkt->entries[i].menu_id = LE16(cs_options[i].menu_id);
        pkt->entries[i].item_id = LE16(cs_options[i].item_id);
        strcpy(pkt->entries[i].text, cs_options[i].text);
    }

    return crypt_send(c, len, sendbuf);
}

static int send_pc_choice_search(ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    pc_choice_search_pkt *pkt = (pc_choice_search_pkt *)sendbuf;
    uint16_t len = 4 + 0x3C * CS_OPTIONS_COUNT;
    uint16_t i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Fill in the basics. */
    pkt->hdr.pkt_type = CHOICE_OPTION_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = CS_OPTIONS_COUNT;

    /* Copy the options over. */
    for(i = 0; i < CS_OPTIONS_COUNT; ++i) {
        pkt->entries[i].menu_id = LE16(cs_options[i].menu_id);
        pkt->entries[i].item_id = LE16(cs_options[i].item_id);

        /* Convert the text to UTF-16 */
        istrncpy(ic_8859_to_utf16, (char *)pkt->entries[i].text,
                 cs_options[i].text, 0x38);
    }

    return crypt_send(c, len, sendbuf);
}

static int send_bb_choice_search(ship_client_t* c) {
    uint8_t* sendbuf = get_sendbuf();
    bb_choice_search_pkt* pkt = (bb_choice_search_pkt*)sendbuf;
    uint16_t len = 4 + 0x3C * CS_OPTIONS_COUNT;
    uint16_t i;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Fill in the basics. */
    pkt->hdr.pkt_type = CHOICE_OPTION_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = CS_OPTIONS_COUNT;

    /* Copy the options over. */
    for (i = 0; i < CS_OPTIONS_COUNT; ++i) {
        pkt->entries[i].menu_id = LE16(cs_options[i].menu_id);
        pkt->entries[i].item_id = LE16(cs_options[i].item_id);

        /* Convert the text to UTF-16 */
        istrncpy(ic_utf16_to_gbk, (char*)pkt->entries[i].text,
            cs_options[i].text, 0x38);
    }

    return crypt_send(c, len, sendbuf);
}

int send_choice_search(ship_client_t *c) {
    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_choice_search(c);

        case CLIENT_VERSION_PC:
            return send_pc_choice_search(c);

        case CLIENT_VERSION_BB:
            return send_bb_choice_search(c);
    }

    return -1;
}

/* Send a reply to a choice search to the client. */
static int fill_one_choice_entry(uint8_t *sendbuf, int version,
                                 ship_client_t *it, int entry, int port_off) {
    block_t *b = it->cur_block;

    switch(version) {
        case CLIENT_VERSION_PC:
        {
            pc_choice_reply_pkt *pkt = (pc_choice_reply_pkt *)sendbuf;
            char tmp[64];
            size_t len;

            memset(&pkt->entries[entry], 0, 0x154);
            pkt->entries[entry].guildcard = LE32(it->guildcard);

            /* All the text needs to be converted with iconv to UTF-16LE... */
            istrncpy(ic_8859_to_utf16, (char *)pkt->entries[entry].name,
                     it->pl->v1.character.dress_data.guildcard_string, 0x20);

            sprintf(tmp, "%s Lvl %d\n", pso_class[it->pl->v1.character.dress_data.ch_class].cn_name,
                    it->pl->v1.character.disp.level + 1);
            istrncpy(ic_8859_to_utf16, (char *)pkt->entries[entry].cl_lvl, tmp,
                     0x40);

            istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entry].location,
                     it->cur_lobby->name, 0x1C);
            pkt->entries[entry].location[14] = 0;
            pkt->entries[entry].location[15] = 0;
            len = strlen16_raw(pkt->entries[entry].location);

            sprintf(tmp, ",BLOCK%02d,%s", b->b, ship->cfg->name);
            istrncpy(ic_gbk_to_utf16,
                     (char *)(pkt->entries[entry].location + len), tmp,
                     0x60 - (len * 2));

            pkt->entries[entry].ip = ship_ip4;
            pkt->entries[entry].port = LE16(b->dc_port + port_off);
            pkt->entries[entry].menu_id = LE32(MENU_ID_LOBBY);
            pkt->entries[entry].item_id = LE32(it->lobby_id);

            return 0x154;
        }

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
        {
            dc_choice_reply_pkt *pkt = (dc_choice_reply_pkt *)sendbuf;
            char lname[17];

            memset(&pkt->entries[entry], 0, 0xD4);
            pkt->entries[entry].guildcard = LE32(it->guildcard);

            /* Everything here is ISO-8859-1 already... so no need to
               iconv anything in here */
            strcpy(pkt->entries[entry].name, it->pl->v1.character.dress_data.guildcard_string);
            sprintf(pkt->entries[entry].cl_lvl, "%s Lvl %d\n",
                    pso_class[it->pl->v1.character.dress_data.ch_class].cn_name, it->pl->v1.character.disp.level + 1);

            /* iconv the lobby name */
            if(it->cur_lobby->name[0] == '\t' &&
               it->cur_lobby->name[1] == 'J') {
                istrncpy(ic_gbk_to_sjis, lname, it->cur_lobby->name, 16);
            }
            else {
                istrncpy(ic_gbk_to_8859, lname, it->cur_lobby->name, 16);
            }

            lname[16] = 0;

            sprintf(pkt->entries[entry].location, "%s,BLOCK%02d,%s", lname,
                    b->b, ship->cfg->name);

            pkt->entries[entry].ip = ship_ip4;
            pkt->entries[entry].port = LE16(b->dc_port + port_off);
            pkt->entries[entry].menu_id = LE32(MENU_ID_LOBBY);
            pkt->entries[entry].item_id = LE32(it->lobby_id);

            return 0xD4;
        }
    }

    return 0;
}

#ifdef PSOCN_ENABLE_IPV6

static int fill_one_choice6_entry(uint8_t *sendbuf, int version,
                                  ship_client_t *it, int entry, int port_off) {
    block_t *b = it->cur_block;

    switch(version) {
        case CLIENT_VERSION_PC:
        {
            pc_choice_reply6_pkt *pkt = (pc_choice_reply6_pkt *)sendbuf;
            char tmp[64];
            size_t len;

            memset(&pkt->entries[entry], 0, 0x160);
            pkt->entries[entry].guildcard = LE32(it->guildcard);

            /* All the text needs to be converted with iconv to UTF-16LE... */
            istrncpy(ic_8859_to_utf16, (char *)pkt->entries[entry].name,
                     it->pl->v1.character.dress_data.guildcard_string, 0x20);

            sprintf(tmp, "%s Lvl %d\n", pso_class[it->pl->v1.character.dress_data.ch_class].cn_name,
                    it->pl->v1.character.disp.level + 1);
            istrncpy(ic_8859_to_utf16, (char *)pkt->entries[entry].cl_lvl, tmp,
                     0x40);

            istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entry].location,
                     it->cur_lobby->name, 0x1C);
            pkt->entries[entry].location[14] = 0;
            pkt->entries[entry].location[15] = 0;
            len = strlen16_raw(pkt->entries[entry].location);

            sprintf(tmp, ",BLOCK%02d,%s", b->b, ship->cfg->name);
            istrncpy(ic_gbk_to_utf16,
                     (char *)(pkt->entries[entry].location + len), tmp,
                     0x60 - (len * 2));

            memcpy(pkt->entries[entry].ip, ship_ip6, 16);
            pkt->entries[entry].port = LE16(b->dc_port + port_off);
            pkt->entries[entry].menu_id = LE32(MENU_ID_LOBBY);
            pkt->entries[entry].item_id = LE32(it->lobby_id);

            return 0x160;
        }

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
        {
            dc_choice_reply6_pkt *pkt = (dc_choice_reply6_pkt *)sendbuf;
            char lname[17];

            memset(&pkt->entries[entry], 0, 0xE0);
            pkt->entries[entry].guildcard = LE32(it->guildcard);

            /* Everything here is ISO-8859-1 already... so no need to
               iconv anything in here */
            strcpy(pkt->entries[entry].name, it->pl->v1.character.dress_data.guildcard_string);
            sprintf(pkt->entries[entry].cl_lvl, "%s Lvl %d\n",
                pso_class[it->pl->v1.character.dress_data.ch_class].cn_name, it->pl->v1.character.disp.level + 1);

            /* iconv the lobby name */
            if(it->cur_lobby->name[0] == '\t' &&
               it->cur_lobby->name[1] == 'J') {
                istrncpy(ic_utf8_to_sjis, lname, it->cur_lobby->name, 16);
            }
            else {
                istrncpy(ic_utf8_to_8859, lname, it->cur_lobby->name, 16);
            }

            lname[16] = 0;

            sprintf(pkt->entries[entry].location, "%s,BLOCK%02d,%s", lname,
                    b->b, ship->cfg->name);

            memcpy(pkt->entries[entry].ip, ship_ip6, 16);
            pkt->entries[entry].port = LE16(b->dc_port + port_off);
            pkt->entries[entry].menu_id = LE32(MENU_ID_LOBBY);
            pkt->entries[entry].item_id = LE32(it->lobby_id);

            return 0xE0;
        }
    }

    return 0;
}

#endif

static int fill_choice_entries(ship_client_t *c, uint8_t *sendbuf,
                               dc_choice_set_pkt *search, int minlvl,
                               int maxlvl, int cl, int vmin, int vmax,
                               int port_off, uint16_t *lenp, int valt) {
    int len = 0, entries = 0;
    uint32_t i;
    block_t *b;
    ship_client_t *it;

    for(i = 0; i < ship->cfg->blocks; ++i) {
        b = ship->blocks[i];

        if(b && b->run) {
            pthread_rwlock_rdlock(&b->lock);

            /* 查看该舰仓的所有客户端. */
            TAILQ_FOREACH(it, b->clients, qentry) {
                /* Look to see if they match the search. */
                if(!it->pl) {
                    continue;
                }
                else if(!it->cur_lobby) {
                    continue;
                }
                else if((int)it->pl->v1.character.disp.level < minlvl ||
                    (int)it->pl->v1.character.disp.level > maxlvl) {
                    continue;
                }
                else if(cl != 0 && it->pl->v1.character.dress_data.ch_class != cl - 1) {
                    continue;
                }
                else if(it == c) {
                    continue;
                }
                else if((it->version > vmax || it->version < vmin) &&
                        it->version != valt) {
                    continue;
                }
                else if(it->flags & CLIENT_FLAG_IS_NTE) {
                    /* Don't allow any DC or PC NTE clients to show up in
                       choice results. */
                    continue;
                }

                /* 如果我们到了这里, they match the search. Fill in the
                   entry. */
#ifdef PSOCN_ENABLE_IPV6
                if((c->flags & CLIENT_FLAG_IPV6)) {
                    len += fill_one_choice6_entry(sendbuf, c->version, it,
                                                  entries++, port_off);
                }
                else {
                    len += fill_one_choice_entry(sendbuf, c->version, it,
                                                 entries++, port_off);
                }
#else
                len += fill_one_choice_entry(sendbuf, c->version, it, entries++,
                                             port_off);
#endif

                /* Choice search is limited to 32 entries by the game... */
                if(entries == 32) {
                    *lenp = len;
                    pthread_rwlock_unlock(&b->lock);
                    return entries;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    *lenp = len;
    return entries;
}

static int send_dc_choice_reply(ship_client_t *c, dc_choice_set_pkt *search,
                                int minlvl, int maxlvl, int cl) {
    uint8_t *sendbuf = get_sendbuf();
    dc_choice_reply_pkt *pkt = (dc_choice_reply_pkt *)sendbuf;
    uint16_t len;
    uint8_t entries;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Fill in the entries on the local ship */
    entries = fill_choice_entries(c, sendbuf, search, minlvl, maxlvl, cl,
                                  CLIENT_VERSION_DCV1, CLIENT_VERSION_PC, 0,
                                  &len, -1);
    len += 4;

    /* Put in a blank entry at the end... */
#ifdef PSOCN_ENABLE_IPV6
    if((c->flags & CLIENT_FLAG_IPV6)) {
        memset(&pkt->entries[entries], 0, 0xE0);
        len += 0xE0;

        /* 填充数据头. */
        pkt->hdr.pkt_type = CHOICE_REPLY_TYPE;
        pkt->hdr.pkt_len = LE16(len);
        pkt->hdr.flags = entries | 0x80;

        return crypt_send(c, len, sendbuf);
    }
#endif

    memset(&pkt->entries[entries], 0, 0xD4);
    len += 0xD4;

    /* 填充数据头. */
    pkt->hdr.pkt_type = CHOICE_REPLY_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = entries;

    return crypt_send(c, len, sendbuf);
}

static int send_pc_choice_reply(ship_client_t *c, dc_choice_set_pkt *search,
                                int minlvl, int maxlvl, int cl) {
    uint8_t *sendbuf = get_sendbuf();
    pc_choice_reply_pkt *pkt = (pc_choice_reply_pkt *)sendbuf;
    uint16_t len;
    uint8_t entries;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Fill in the entries on the local ship */
    entries = fill_choice_entries(c, sendbuf, search, minlvl, maxlvl, cl,
                                  CLIENT_VERSION_DCV1, CLIENT_VERSION_PC, 1,
                                  &len, -1);
    len += 4;

    /* Put in a blank entry at the end... */
#ifdef PSOCN_ENABLE_IPV6
    if((c->flags & CLIENT_FLAG_IPV6)) {
        memset(&pkt->entries[entries], 0, 0x160);
        len += 0x160;

        pkt->hdr.pkt_type = CHOICE_REPLY_TYPE;
        pkt->hdr.pkt_len = LE16(len);
        pkt->hdr.flags = entries | 0x80;

        return crypt_send(c, len, sendbuf);
    }
#endif

    memset(&pkt->entries[entries], 0, 0x154);
    len += 0x154;

    /* 填充数据头. */
    pkt->hdr.pkt_type = CHOICE_REPLY_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = entries;

    return crypt_send(c, len, sendbuf);
}

static int send_gc_choice_reply(ship_client_t *c, dc_choice_set_pkt *search,
                                int minlvl, int maxlvl, int cl) {
    uint8_t *sendbuf = get_sendbuf();
    dc_choice_reply_pkt *pkt = (dc_choice_reply_pkt *)sendbuf;
    uint16_t len;
    uint8_t entries;
    int off = 2;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    if(c->version == CLIENT_VERSION_XBOX)
        off = 5;

    /* Fill in the entries on the local ship */
    entries = fill_choice_entries(c, sendbuf, search, minlvl, maxlvl, cl,
                                  CLIENT_VERSION_GC, CLIENT_VERSION_GC, off,
                                  &len, CLIENT_VERSION_XBOX);
    len += 4;

    /* Put in a blank entry at the end... */
#ifdef PSOCN_ENABLE_IPV6
    if((c->flags & CLIENT_FLAG_IPV6)) {
        memset(&pkt->entries[entries], 0, 0xE0);
        len += 0xE0;

        /* 填充数据头. */
        pkt->hdr.pkt_type = CHOICE_REPLY_TYPE;
        pkt->hdr.pkt_len = LE16(len);
        pkt->hdr.flags = entries | 0x80;

        return crypt_send(c, len, sendbuf);
    }
#endif

    memset(&pkt->entries[entries], 0, 0xD4);
    len += 0xD4;

    /* 填充数据头. */
    pkt->hdr.pkt_type = CHOICE_REPLY_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = entries;

    return crypt_send(c, len, sendbuf);
}

static int send_ep3_choice_reply(ship_client_t *c, dc_choice_set_pkt *search,
                                 int minlvl, int maxlvl, int cl) {
    uint8_t *sendbuf = get_sendbuf();
    dc_choice_reply_pkt *pkt = (dc_choice_reply_pkt *)sendbuf;
    uint16_t len;
    uint8_t entries;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Fill in the entries on the local ship */
    entries = fill_choice_entries(c, sendbuf, search, minlvl, maxlvl, cl,
                                  CLIENT_VERSION_EP3, CLIENT_VERSION_EP3, 3,
                                  &len, -1);
    len += 4;

    /* Put in a blank entry at the end... */
#ifdef PSOCN_ENABLE_IPV6
    if((c->flags & CLIENT_FLAG_IPV6)) {
        memset(&pkt->entries[entries], 0, 0xE0);
        len += 0xE0;

        /* 填充数据头. */
        pkt->hdr.pkt_type = CHOICE_REPLY_TYPE;
        pkt->hdr.pkt_len = LE16(len);
        pkt->hdr.flags = entries | 0x80;

        return crypt_send(c, len, sendbuf);
    }
#endif

    memset(&pkt->entries[entries], 0, 0xD4);
    len += 0xD4;

    /* 填充数据头. */
    pkt->hdr.pkt_type = CHOICE_REPLY_TYPE;
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = entries;

    return crypt_send(c, len, sendbuf);
}

int send_choice_reply(ship_client_t *c, dc_choice_set_pkt *search) {
    int minlvl = 0, maxlvl = 199, cl;

    /* Parse the packet for the minimum and maximum levels. */
    switch(LE16(search->entries[0].item_id)) {
        case 0x0001:
            minlvl = c->pl->v1.character.disp.level - 5;
            maxlvl = c->pl->v1.character.disp.level + 5;
            break;

        case 0x0002:
            minlvl = 0;
            maxlvl = 9;
            break;

        case 0x0003:
            minlvl = 10;
            maxlvl = 19;
            break;

        case 0x0004:
            minlvl = 20;
            maxlvl = 39;
            break;

        case 0x0005:
            minlvl = 40;
            maxlvl = 59;
            break;

        case 0x0006:
            minlvl = 60;
            maxlvl = 79;
            break;

        case 0x0007:
            minlvl = 80;
            maxlvl = 99;
            break;

        case 0x0008:
            minlvl = 100;
            maxlvl = 119;
            break;

        case 0x0009:
            minlvl = 120;
            maxlvl = 159;
            break;

        case 0x000A:
            minlvl = 160;
            maxlvl = 199;
            break;
    }

    cl = LE16(search->entries[1].item_id);

    /* Call the appropriate function. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            return send_dc_choice_reply(c, search, minlvl, maxlvl, cl);

        case CLIENT_VERSION_PC:
            return send_pc_choice_reply(c, search, minlvl, maxlvl, cl);

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            return send_gc_choice_reply(c, search, minlvl, maxlvl, cl);

        case CLIENT_VERSION_EP3:
            return send_ep3_choice_reply(c, search, minlvl, maxlvl, cl);

        /* Blue Burst does not support Choice Search. */
    }

    return -1;
}

static int send_pc_simple_mail_dc(ship_client_t *c, simple_mail_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    simple_mail_pkt *pkt = (simple_mail_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;
    int i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Scrub the buffer. */
    memset(pkt, 0, PC_SIMPLE_MAIL_LENGTH);

    /* 填充数据头. */
    pkt->pc.pkt_type = SIMPLE_MAIL_TYPE;
    pkt->pc.flags = 0;
    pkt->pc.pkt_len = LE16(PC_SIMPLE_MAIL_LENGTH);

    /* Copy everything that doesn't need to be converted. */
    pkt->player_tag = p->player_tag;
    pkt->gc_sender = p->gc_sender;
    pkt->pcmaildata.gc_dest = p->dcmaildata.gc_dest;

    /* Convert the name and the text. */
    in = 0x10;
    out = 0x20;
    inptr = p->dcmaildata.dcname;
    outptr = (char *)pkt->pcmaildata.pcname;
    iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

    in = 0x90;
    out = 0x120;
    inptr = p->pcmaildata.stuff;
    outptr = pkt->pcmaildata.stuff;

    if(p->pcmaildata.stuff[1] == 'J') {
        iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
    }
    else {
        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
    }

    /* This is a BIT hackish (just a bit...). */
    for(i = 0; i < 0x150; ++i) {
        pkt->pcmaildata.stuff[(i << 1) + 0x150] = p->pcmaildata.stuff[i + 0xB0];
        pkt->pcmaildata.stuff[(i << 1) + 0x151] = 0;
    }

    /* 加密并发送. */
    return crypt_send(c, PC_SIMPLE_MAIL_LENGTH, sendbuf);
}

static int send_dc_simple_mail_pc(ship_client_t *c, simple_mail_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    simple_mail_pkt *pkt = (simple_mail_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;
    int i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Scrub the buffer. */
    memset(pkt, 0, DC_SIMPLE_MAIL_LENGTH);

    /* 填充数据头. */
    pkt->dc.pkt_type = SIMPLE_MAIL_TYPE;
    pkt->dc.flags = 0;
    pkt->dc.pkt_len = LE16(DC_SIMPLE_MAIL_LENGTH);

    /* Copy everything that doesn't need to be converted. */
    pkt->player_tag = p->player_tag;
    pkt->gc_sender = p->gc_sender;
    pkt->dcmaildata.gc_dest = p->pcmaildata.gc_dest;

    /* Convert the name. */
    in = 0x20;
    out = 0x10;
    inptr = (char *)p->pcmaildata.pcname;
    outptr = pkt->dcmaildata.dcname;
    iconv(ic_utf16_to_gbk/*尝试GBK*/, &inptr, &in, &outptr, &out);

    /* Convert the first instance of text. */
    in = 0x120;
    out = 0x90;
    inptr = p->dcmaildata.stuff;
    outptr = pkt->dcmaildata.stuff;

    if(p->dcmaildata.stuff[2] == 'J') {
        iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
    }
    else {
        iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
    }

    /* This is a BIT hackish (just a bit...). */
    for(i = 0; i < 0x150; ++i) {
        pkt->dcmaildata.stuff[i + 0xB0] = p->dcmaildata.stuff[(i << 1) + 0x150];
    }

    /* 加密并发送. */
    return crypt_send(c, DC_SIMPLE_MAIL_LENGTH, sendbuf);
}

static int send_pc_simple_mail_bb(ship_client_t *c, simple_mail_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    simple_mail_pkt *pkt = (simple_mail_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Scrub the buffer. */
    memset(pkt, 0, PC_SIMPLE_MAIL_LENGTH);

    /* 填充数据头. */
    pkt->pc.pkt_type = SIMPLE_MAIL_TYPE;
    pkt->pc.flags = 0;
    pkt->pc.pkt_len = LE16(PC_SIMPLE_MAIL_LENGTH);

    /* Copy everything that doesn't need to be converted. */
    pkt->player_tag = p->player_tag;
    pkt->gc_sender = p->gc_sender;
    pkt->pcmaildata.gc_dest = p->bbmaildata.gc_dest;

    /* Convert the name and the text. */
    memcpy(pkt->pcmaildata.pcname, &p->bbmaildata.bbname[2], 0x1C);
    memcpy(pkt->pcmaildata.stuff, p->bbmaildata.message, 0x0158);

    /* 加密并发送. */
    return crypt_send(c, PC_SIMPLE_MAIL_LENGTH, sendbuf);
}

static int send_dc_simple_mail_bb(ship_client_t *c, simple_mail_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    simple_mail_pkt *pkt = (simple_mail_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Scrub the buffer. */
    memset(pkt, 0, DC_SIMPLE_MAIL_LENGTH);

    /* 填充数据头. */
    pkt->dc.pkt_type = SIMPLE_MAIL_TYPE;
    pkt->dc.flags = 0;
    pkt->dc.pkt_len = LE16(DC_SIMPLE_MAIL_LENGTH);

    /* Copy everything that doesn't need to be converted. */
    pkt->player_tag = p->player_tag;
    pkt->gc_sender = p->gc_sender;
    pkt->dcmaildata.gc_dest = p->bbmaildata.gc_dest;

    /* Convert the name. */
    in = 0x20;
    out = 0x10;
    inptr = (char *)&p->bbmaildata.bbname[2];
    outptr = pkt->dcmaildata.dcname;
    iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

    /* Convert the text. */
    in = 0x158;
    out = 0x90;
    inptr = (char *)p->bbmaildata.message;
    outptr = pkt->dcmaildata.stuff;

    if(p->bbmaildata.message[1] == LE16('J')) {
        iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
    }
    else {
        iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
    }

    /* 加密并发送. */
    return crypt_send(c, DC_SIMPLE_MAIL_LENGTH, sendbuf);
}

static int send_bb_simple_mail_dc(ship_client_t *c, simple_mail_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    simple_mail_pkt *pkt = (simple_mail_pkt *)sendbuf;
    size_t in, out;
    char *inptr;
    char *outptr;
    char timestamp[20];
    //time_t rawtime;
    //struct tm cooked;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Scrub the buffer. */
    memset(pkt, 0, BB_SIMPLE_MAIL_LENGTH);

    /* 填充数据头. */
    pkt->bb.pkt_type = LE16(SIMPLE_MAIL_TYPE);
    pkt->bb.pkt_len = LE16(BB_SIMPLE_MAIL_LENGTH);

    /* Copy everything that doesn't need to be converted. */
    pkt->player_tag = p->player_tag;
    pkt->gc_sender = p->gc_sender;
    pkt->bbmaildata.gc_dest = p->dcmaildata.gc_dest;

    /* Convert the name and the text. */
    pkt->bbmaildata.bbname[0] = LE16('\t');
    pkt->bbmaildata.bbname[1] = LE16('E');
    in = 0x10;
    out = 0x1C;
    inptr = p->dcmaildata.dcname;
    outptr = (char *)&pkt->bbmaildata.bbname[2];
    iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

    in = 0x90;
    out = 0x158;
    inptr = p->dcmaildata.stuff;
    outptr = (char *)pkt->bbmaildata.message;

    if(p->dcmaildata.stuff[1] == 'J') {
        iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
    }
    else {
        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
    }

    /* Fill in the date/time */
    //rawtime = time(NULL);
    //gmtime_r(&rawtime, &cooked);
    sprintf(timestamp, "%04u.%02u.%02u %02u:%02uZ", rawtime.wYear,
        rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);

    in = 20;
    out = 40;
    inptr = timestamp;
    outptr = (char *)pkt->bbmaildata.timestamp;
    iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

    /* 加密并发送. */
    return crypt_send(c, BB_SIMPLE_MAIL_LENGTH, sendbuf);
}

static int send_bb_simple_mail_pc(ship_client_t *c, simple_mail_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    simple_mail_pkt *pkt = (simple_mail_pkt *)sendbuf;
    char timestamp[20];
    //time_t rawtime;
    //struct tm cooked;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    int i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Scrub the buffer. */
    memset(pkt, 0, BB_SIMPLE_MAIL_LENGTH);

    /* 填充数据头. */
    pkt->bb.pkt_type = LE16(SIMPLE_MAIL_TYPE);
    pkt->bb.pkt_len = LE16(BB_SIMPLE_MAIL_LENGTH);

    /* Copy everything that doesn't need to be converted. */
    pkt->player_tag = p->player_tag;
    pkt->gc_sender = p->gc_sender;
    pkt->bbmaildata.gc_dest = p->pcmaildata.gc_dest;

    /* Copy the name and the text. */
    pkt->bbmaildata.bbname[0] = LE16('\t');
    pkt->bbmaildata.bbname[1] = LE16('E');
    memcpy(&pkt->bbmaildata.bbname[2], p->pcmaildata.pcname, 0x1C);
    memcpy(pkt->bbmaildata.message, p->pcmaildata.stuff, 0x0180);

    /* Fill in the date/time */
    //rawtime = time(NULL);
    //gmtime_r(&rawtime, &cooked);
    sprintf(timestamp, "%04u.%02u.%02u %02u:%02uZ", rawtime.wYear,
        rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);

    for(i = 0; i < 20; ++i) {
        pkt->bbmaildata.timestamp[i] = LE16(timestamp[i]);
    }

    /* 加密并发送. */
    return crypt_send(c, BB_SIMPLE_MAIL_LENGTH, sendbuf);
}

static int send_bb_simple_mail_bb(ship_client_t *c, simple_mail_pkt *p) {
    uint8_t *sendbuf = get_sendbuf();
    simple_mail_pkt *pkt = (simple_mail_pkt *)sendbuf;
    char timestamp[20];
    //time_t rawtime;
    //struct tm cooked;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    int i;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Scrub the buffer. */
    memset(pkt, 0, BB_SIMPLE_MAIL_LENGTH);

    /* 填充数据头. */
    pkt->bb.pkt_type = LE16(SIMPLE_MAIL_TYPE);
    pkt->bb.pkt_len = LE16(BB_SIMPLE_MAIL_LENGTH);

    /* Copy everything that doesn't need to be converted. */
    pkt->player_tag = p->player_tag;
    pkt->gc_sender = p->gc_sender;
    pkt->bbmaildata.gc_dest = p->bbmaildata.gc_dest;

    /* Copy the name and the text. */
    memcpy(pkt->bbmaildata.bbname, p->bbmaildata.bbname, 0x20);
    memcpy(pkt->bbmaildata.message, p->bbmaildata.message, 0x0180);

    /* Fill in the date/time */
    //rawtime = time(NULL);
    //gmtime_r(&rawtime, &cooked);
    sprintf(timestamp, "%04u.%02u.%02u %02u:%02uZ", rawtime.wYear,
        rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute);

    for(i = 0; i < 20; ++i) {
        pkt->bbmaildata.timestamp[i] = LE16(timestamp[i]);
    }

    /* 加密并发送. */
    return crypt_send(c, BB_SIMPLE_MAIL_LENGTH, sendbuf);
}

/* Send a simple mail packet, doing any needed transformations. */
int send_simple_mail(int version, ship_client_t *c, dc_pkt_hdr_t *pkt) {
    switch(version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            if(c->version == CLIENT_VERSION_PC) {
                return send_pc_simple_mail_dc(c, (simple_mail_pkt *)pkt);
            }
            else if(c->version == CLIENT_VERSION_BB) {
                return send_bb_simple_mail_dc(c, (simple_mail_pkt *)pkt);
            }
            else {
                return send_pkt_dc(c, pkt);
            }
            break;

        case CLIENT_VERSION_PC:
            /* Don't send these to PC NTE clients. */
            if((c->flags & CLIENT_FLAG_IS_NTE))
                return 0;

            if(c->version == CLIENT_VERSION_PC) {
                return send_pkt_dc(c, pkt);
            }
            else if(c->version == CLIENT_VERSION_BB) {
                return send_bb_simple_mail_pc(c, (simple_mail_pkt *)pkt);
            }
            else {
                return send_dc_simple_mail_pc(c, (simple_mail_pkt *)pkt);
            }
            break;
    }

    return -1;
}

int send_bb_simple_mail(ship_client_t *c, simple_mail_pkt *pkt) {
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_simple_mail_bb(c, pkt);
            break;

        case CLIENT_VERSION_PC:
            return send_pc_simple_mail_bb(c, pkt);
            break;

        case CLIENT_VERSION_BB:
            return send_bb_simple_mail_bb(c, pkt);
    }

    return -1;
}

int send_mail_autoreply(ship_client_t *d, ship_client_t *s) {
    int i;

    switch(s->version) {
        case CLIENT_VERSION_PC:
        {
            simple_mail_pkt p = { 0 };
            dc_pkt_hdr_t *dc = (dc_pkt_hdr_t *)&p;

            /* Build the packet up */
            memset(&p, 0, sizeof(simple_mail_pkt));
            dc->pkt_type = SIMPLE_MAIL_TYPE;
            dc->pkt_len = LE16(PC_SIMPLE_MAIL_LENGTH);
            p.player_tag = LE32(0x00010000);
            p.gc_sender = LE32(s->guildcard);
            p.pcmaildata.gc_dest = LE32(d->guildcard);

            /* Copy the name */
            for(i = 0; i < 16; ++i) {
                p.pcmaildata.pcname[i] = LE16(s->pl->v1.character.dress_data.guildcard_string[i]);
            }

            /* Copy the message */
            memcpy(p.pcmaildata.stuff, s->autoreply, s->autoreply_len);

            /* Send it */
            send_simple_mail(s->version, d, dc);
            break;
        }

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
        {
            simple_mail_pkt p = { 0 };
            dc_pkt_hdr_t *dc = (dc_pkt_hdr_t *)&p;

            /* Build the packet up */
            memset(&p, 0, sizeof(simple_mail_pkt));
            p.dc.pkt_type = SIMPLE_MAIL_TYPE;
            p.dc.pkt_len = LE16(DC_SIMPLE_MAIL_LENGTH);
            p.player_tag = LE32(0x00010000);
            p.gc_sender = LE32(s->guildcard);
            p.dcmaildata.gc_dest = LE32(d->guildcard);

            /* Copy the name and message */
            memcpy(p.dcmaildata.dcname, s->pl->v1.character.dress_data.guildcard_string, 16);
            memcpy(p.dcmaildata.stuff, s->autoreply, s->autoreply_len);

            /* Send it */
            send_simple_mail(s->version, d, dc);
            break;
        }

        case CLIENT_VERSION_BB:
        {
            simple_mail_pkt p;

            /* Build the packet up */
            memset(&p, 0, sizeof(simple_mail_pkt));
            p.bb.pkt_type = LE16(SIMPLE_MAIL_TYPE);
            p.bb.pkt_len = LE16(BB_SIMPLE_MAIL_LENGTH);
            p.player_tag = LE32(0x00010000);
            p.gc_sender = LE32(s->guildcard);
            p.bbmaildata.gc_dest = LE32(d->guildcard);

            /* Copy the name, date/time, and message */
            memcpy(p.bbmaildata.bbname, &s->pl->bb.character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
            memcpy(p.bbmaildata.message, s->autoreply, s->autoreply_len);

            /* Send it */
            send_bb_simple_mail(d, &p);
            break;
        }
    }

    return 0;
}

/* Send the lobby's info board to the client. */
static int send_gc_infoboard(ship_client_t *c, lobby_t *l) {
    int i;
    uint8_t *sendbuf = get_sendbuf();
    gc_read_info_pkt *pkt = (gc_read_info_pkt *)sendbuf;
    int entries = 0, size = 4;
    ship_client_t *c2;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL && l->clients[i] != c) {
            c2 = l->clients[i];
            pthread_mutex_lock(&c2->mutex);

            /* Copy in the user's info if they provided any, otherwise skip
               them... */
            switch(c2->version) {
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    if(c2->infoboard[0]) {
                        memset(&pkt->entries[entries], 0, 0xBC);
                        strcpy(pkt->entries[entries].name, c2->pl->v1.character.dress_data.guildcard_string);
                        strcpy(pkt->entries[entries].msg, c2->infoboard);
                        break;
                    }

                    goto next;

                case CLIENT_VERSION_BB:
                    if(c2->infoboard[0]) {
                        memset(&pkt->entries[entries], 0, 0xBC);

                        /* Convert the name */
                        in = BB_CHARACTER_CHAR_NAME_WLENGTH;
                        out = BB_CHARACTER_NAME_LENGTH;
                        inptr = (char *)&c2->pl->bb.character.name.char_name[0];
                        outptr = pkt->entries[entries].name;
                        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

                        /* Convert the info */
                        in = 0x158;
                        out = 0xAC;
                        inptr = c2->infoboard;
                        outptr = pkt->entries[entries].msg;

                        if(c2->pl->bb.infoboard[1] == LE16('J')) {
                            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
                        }
                        else {
                            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
                        }

                        break;
                    }

                    goto next;

                default:
                    goto next;
            }

            ++entries;
            size += 0xBC;

next:
            pthread_mutex_unlock(&c2->mutex);
        }
    }

    /* 填充数据头. */
    pkt->hdr.pkt_type = INFOBOARD_TYPE;
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(size);

    return crypt_send(c, size, sendbuf);
}

static int send_bb_infoboard(ship_client_t *c, lobby_t *l) {
    int i;
    uint8_t *sendbuf = get_sendbuf();
    bb_read_info_pkt *pkt = (bb_read_info_pkt *)sendbuf;
    int entries = 0, size = 8;
    ship_client_t *c2;

    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL && l->clients[i] != c) {
            c2 = l->clients[i];
            pthread_mutex_lock(&c2->mutex);

            /* Copy in the user's info if they provided any, otherwise skip
               them... */
            switch(c2->version) {
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    if(c2->infoboard[0]) {
                        memset(&pkt->entries[entries], 0, 0x178);

                        /* Convert the name */
                        pkt->entries[entries].name[0] = LE16('\t');
                        pkt->entries[entries].name[1] = LE16('E');
                        in = 16;
                        out = 28;
                        inptr = c2->pl->v1.character.dress_data.guildcard_string;
                        outptr = (char *)&pkt->entries[entries].name[2];

                        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

                        /* Convert the info */
                        in = 0xAC;
                        out = 0x158;
                        inptr = c2->infoboard;
                        outptr = (char *)pkt->entries[entries].msg;

                        if(c2->infoboard[1] == 'J') {
                            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
                        }
                        else {
                            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
                        }
                        break;
                    }

                    goto next;

                case CLIENT_VERSION_BB:
                    if(c2->infoboard[0]) {
                        memset(&pkt->entries[entries], 0, 0x178);
                        memcpy(pkt->entries[entries].name,
                               &c2->pl->bb.character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
                        memcpy(pkt->entries[entries].msg,
                               c2->infoboard, 0x158);
                        break;
                    }

                    goto next;

                default:
                    goto next;
            }

            ++entries;
            size += 0x178;

next:
            pthread_mutex_unlock(&c2->mutex);
        }
    }

    /* 填充数据头. */
    pkt->hdr.pkt_type = LE16(INFOBOARD_TYPE);
    pkt->hdr.flags = LE32(entries);
    pkt->hdr.pkt_len = LE16(size);

    return crypt_send(c, size, sendbuf);
}

int send_infoboard(ship_client_t *c, lobby_t *l) {
    if(!l)
        return -1;

    switch(c->version) {
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_gc_infoboard(c, l);

        case CLIENT_VERSION_BB:
            return send_bb_infoboard(c, l);
    }

    return -1;
}

/* Utilities for C-Rank stuff... */
static void copy_c_rank_gc(gc_c_rank_update_pkt *pkt, int entry,
                           ship_client_t *s) {
    int j;

    switch(s->version) {
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            pkt->entries[entry].client_id = LE32(s->client_id);
            memcpy(pkt->entries[entry].c_rank, s->c_rank, 0x0118);
            break;

        case CLIENT_VERSION_DCV2:
            pkt->entries[entry].client_id = LE32(s->client_id);

            memset(pkt->entries[entry].c_rank, 0, 0x0118);

            pkt->entries[entry].unk1 = (s->pl->v2.chal_data.c_rank.part.unk1 >> 16) |
                (s->pl->v2.chal_data.c_rank.part.unk1 << 16);

            /* Copy the rank over. */
            memcpy(pkt->entries[entry].string, s->pl->v2.chal_data.c_rank.part.string,
                   0x0C);

            /* Copy the times for the levels and battle stuff over... */
            memcpy(pkt->entries[entry].times,
                   s->pl->v2.chal_data.c_rank.part.times, 9 * sizeof(uint32_t));
            memcpy(pkt->entries[entry].battle,
                   s->pl->v2.chal_data.c_rank.part.battle, 7 * sizeof(uint32_t));

            /* Copy over the color-related information... This also apparently
               has something to do with the length of the string... */
            memcpy(pkt->entries[entry].unk3, s->pl->v2.chal_data.c_rank.part.unk2,
                   sizeof(uint32_t));
            memcpy(pkt->entries[entry].unk3 + 8, s->pl->v2.chal_data.c_rank.part.unk2,
                   sizeof(uint32_t));
            memcpy(pkt->entries[entry].unk3 + 4, s->pl->v2.chal_data.c_rank.part.unk2 + 8,
                   sizeof(uint32_t));
            memcpy(pkt->entries[entry].unk3 + 12,
                   s->pl->v2.chal_data.c_rank.part.unk2 + 8, sizeof(uint32_t));
            break;

        case CLIENT_VERSION_PC:
            pkt->entries[entry].client_id = LE32(s->client_id);

            memset(pkt->entries[entry].c_rank, 0, 0x0118);

            pkt->entries[entry].unk1 = (s->pl->pc.chal_data.c_rank.part.unk1 >> 16) |
                (s->pl->pc.chal_data.c_rank.part.unk1 << 16);

            /* Copy the rank over. */
            for(j = 0; j < 0x0C; ++j) {
                pkt->entries[entry].string[j] =
                    (char)LE16(s->pl->pc.chal_data.c_rank.part.string[j]);
            }

            /* Copy the times for the levels and battle stuff over... */
            memcpy(pkt->entries[entry].times, s->pl->pc.chal_data.c_rank.part.times,
                   9 * sizeof(uint32_t));
            memcpy(pkt->entries[entry].battle, s->pl->pc.chal_data.c_rank.part.battle,
                   7 * sizeof(uint32_t));

            /* Copy over the color-related information... This also apparently
               has something to do with the length of the string... */
            memcpy(pkt->entries[entry].unk3, s->pl->pc.chal_data.c_rank.part.unk2,
                   sizeof(uint32_t));
            memcpy(pkt->entries[entry].unk3 + 8, s->pl->pc.chal_data.c_rank.part.unk2,
                   sizeof(uint32_t));
            memcpy(pkt->entries[entry].unk3 + 4, s->pl->pc.chal_data.c_rank.part.unk2 + 8,
                   sizeof(uint32_t));
            memcpy(pkt->entries[entry].unk3 + 12,
                   s->pl->pc.chal_data.c_rank.part.unk2 + 8, sizeof(uint32_t));
            break;

        case CLIENT_VERSION_BB:
            //DBG_LOG("copy_c_rank_gc");
            pkt->entries[entry].client_id = LE32(s->client_id);
            memcpy(pkt->entries[entry].c_rank, s->c_rank, 0x0118);
            break;

        default:
            pkt->entries[entry].client_id = LE32(s->client_id);
            memset(pkt->entries[entry].c_rank, 0, 0x0118);
    }
}

static void copy_c_rank_dc(dc_c_rank_update_pkt *pkt, int entry,
                           ship_client_t *s) {
    int j;
    size_t in, out;
    char *inptr;
    char *outptr;

    switch(s->version) {
        case CLIENT_VERSION_DCV2:
            pkt->entries[entry].client_id = LE32(s->client_id);
            memcpy(pkt->entries[entry].c_rank, s->c_rank, 0xB8);
            break;

        case CLIENT_VERSION_PC:
            pkt->entries[entry].client_id = LE32(s->client_id);

            memset(pkt->entries[entry].c_rank, 0, 0xB8);

            /* This is a bit hackish.... */
            pkt->entries[entry].unk1 = s->pl->pc.chal_data.c_rank.part.unk1;
            memcpy(pkt->entries[entry].unk2, s->pl->pc.chal_data.c_rank.part.unk2, 0x24);

            /* Copy the rank over. */
            for(j = 0; j < 0x0C; ++j) {
                pkt->entries[entry].string[j] =
                    (char)LE16(s->pl->pc.chal_data.c_rank.part.string[j]);
            }

            /* Copy the times for the levels and battle stuff over... */
            memcpy(pkt->entries[entry].times, s->pl->pc.chal_data.c_rank.part.times,
                   9 * sizeof(uint32_t));
            memcpy(pkt->entries[entry].battle, s->pl->pc.chal_data.c_rank.part.battle,
                   7 * sizeof(uint32_t));

            /* Deal with the grave data... */
            /* Copy over the simple stuff... */
            memcpy(&pkt->entries[entry].grave_unk4,
                   &s->pl->pc.chal_data.c_rank.part.grave_unk4, 24);

            /* Convert the team name */
            in = 40;
            out = 20;
            inptr = (char *)s->pl->pc.chal_data.c_rank.part.grave_team;
            outptr = pkt->entries[entry].grave_team;

            if(s->pl->pc.chal_data.c_rank.part.grave_team[1] == LE16('J')) {
                iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
            }

            /* Convert the message */
            in = 48;
            out = 24;
            inptr = (char *)s->pl->pc.chal_data.c_rank.part.grave_message;
            outptr = pkt->entries[entry].grave_message;

            if(s->pl->pc.chal_data.c_rank.part.grave_message[1] == LE16('J')) {
                iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
            }

            break;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            pkt->entries[entry].client_id = LE32(s->client_id);

            memset(pkt->entries[entry].c_rank, 0, 0xB8);

            pkt->entries[entry].unk1 = (s->pl->v3.chal_data.c_rank.part.unk1 >> 16) |
                (s->pl->v3.chal_data.c_rank.part.unk1 << 16);

            /* Copy the rank over. */
            memcpy(pkt->entries[entry].string, s->pl->v3.chal_data.c_rank.part.string,
                   0x0C);

            /* Copy the times for the levels and battle stuff over... */
            memcpy(pkt->entries[entry].times,
                   s->pl->v3.chal_data.c_rank.part.times, 9 * sizeof(uint32_t));
            memcpy(pkt->entries[entry].battle,
                   s->pl->v3.chal_data.c_rank.part.battle, 7 * sizeof(uint32_t));

            /* Copy over the color-related information... */
            memcpy(pkt->entries[entry].unk2, s->pl->v3.chal_data.c_rank.part.unk3,
                   sizeof(uint32_t));
            memcpy(pkt->entries[entry].unk2 + 8, s->pl->v3.chal_data.c_rank.part.unk3 + 4,
                   sizeof(uint32_t));
            break;

        case CLIENT_VERSION_BB:
            //DBG_LOG("copy_c_rank_dc");
            pkt->entries[entry].client_id = LE32(s->client_id);
            memcpy(pkt->entries[entry].c_rank, s->c_rank, 0xB8);
            break;

        default:
            pkt->entries[entry].client_id = LE32(s->client_id);
            memset(pkt->entries[entry].c_rank, 0, 0xB8);
    }
}

static void copy_c_rank_pc(pc_c_rank_update_pkt *pkt, int entry,
                           ship_client_t *s) {
    int j;
    size_t in, out;
    char *inptr;
    char *outptr;

    switch(s->version) {
        case CLIENT_VERSION_PC:
            pkt->entries[entry].client_id = LE32(s->client_id);
            memcpy(pkt->entries[entry].c_rank, s->c_rank, 0xF0);
            break;

        case CLIENT_VERSION_DCV2:
            pkt->entries[entry].client_id = LE32(s->client_id);

            memset(pkt->entries[entry].c_rank, 0, 0xF0);

            /* This is a bit hackish.... */
            pkt->entries[entry].unk1 = s->pl->v2.chal_data.c_rank.part.unk1;
            memcpy(pkt->entries[entry].unk2, s->pl->v2.chal_data.c_rank.part.unk2, 0x24);

            /* Copy the rank over. */
            for(j = 0; j < 0x0C; ++j) {
                pkt->entries[entry].string[j] =
                    LE16(s->pl->v2.chal_data.c_rank.part.string[j]);
            }

            /* Copy the times for the levels and battle stuff over... */
            memcpy(pkt->entries[entry].times, s->pl->v2.chal_data.c_rank.part.times,
                   9 * sizeof(uint32_t));
            memcpy(pkt->entries[entry].battle, s->pl->v2.chal_data.c_rank.part.battle,
                   7 * sizeof(uint32_t));

            /* Deal with the grave data... */
            /* Copy over the simple stuff... */
            memcpy(&pkt->entries[entry].grave_unk4,
                   &s->pl->v2.chal_data.c_rank.part.grave_unk4, 24);

            /* Convert the team name */
            in = 20;
            out = 40;
            inptr = s->pl->v2.chal_data.c_rank.part.grave_team;
            outptr = (char *)pkt->entries[entry].grave_team;

            if(s->pl->v2.chal_data.c_rank.part.grave_team[1] == 'J') {
                iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
            }

            /* Convert the message */
            in = 24;
            out = 48;
            inptr = s->pl->v2.chal_data.c_rank.part.grave_message;
            outptr = (char *)pkt->entries[entry].grave_message;

            if(s->pl->v2.chal_data.c_rank.part.grave_message[1] == 'J') {
                iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
            }

            break;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            pkt->entries[entry].client_id = LE32(s->client_id);

            memset(pkt->entries[entry].c_rank, 0, 0xF0);

            pkt->entries[entry].unk1 = (s->pl->v3.chal_data.c_rank.part.unk1 >> 16) |
                (s->pl->v3.chal_data.c_rank.part.unk1 << 16);

            /* Copy the rank over. */
            for(j = 0; j < 0x0C; ++j) {
                pkt->entries[entry].string[j] =
                    LE16(s->pl->v3.chal_data.c_rank.part.string[j]);
            }

            /* Copy the times for the levels and battle stuff over... */
            memcpy(pkt->entries[entry].times,
                   s->pl->v3.chal_data.c_rank.part.times, 9 * sizeof(uint32_t));
            memcpy(pkt->entries[entry].battle,
                   s->pl->v3.chal_data.c_rank.part.battle, 7 * sizeof(uint32_t));

            /* Copy over the color-related information... */
            memcpy(pkt->entries[entry].unk2, s->pl->v3.chal_data.c_rank.part.unk3,
                   sizeof(uint32_t));
            memcpy(pkt->entries[entry].unk2 + 8, s->pl->v3.chal_data.c_rank.part.unk3 + 4,
                   sizeof(uint32_t));
            break;

        case CLIENT_VERSION_BB:
            //DBG_LOG("copy_c_rank_pc");
            pkt->entries[entry].client_id = LE32(s->client_id);
            memcpy(pkt->entries[entry].c_rank, s->c_rank, 0xF0);
            break;

        default:
            pkt->entries[entry].client_id = LE32(s->client_id);
            memset(pkt->entries[entry].c_rank, 0, 0xF0);
    }
}

static void copy_c_rank_bb(bb_c_rank_update_pkt* pkt, int entry,
    ship_client_t* s) {
    int j;

    switch (s->version) {
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        pkt->entries[entry].client_id = LE32(s->client_id);
        memcpy(pkt->entries[entry].c_rank.all, s->c_rank, 0x0158);
        break;

    case CLIENT_VERSION_DCV2:
        pkt->entries[entry].client_id = LE32(s->client_id);

        memset(pkt->entries[entry].c_rank.all, 0, 0x0158);

        pkt->entries[entry].c_rank.part.unk1 = (s->pl->v2.chal_data.c_rank.part.unk1 >> 16) |
            (s->pl->v2.chal_data.c_rank.part.unk1 << 16);

        /* Copy the rank over. */
        memcpy(pkt->entries[entry].c_rank.part.string, s->pl->v2.chal_data.c_rank.part.string,
            0x0C);

        /* Copy the times for the levels and battle stuff over... */
        memcpy(pkt->entries[entry].c_rank.part.times,
            s->pl->v2.chal_data.c_rank.part.times, 9 * sizeof(uint32_t));
        memcpy(pkt->entries[entry].c_rank.part.battle,
            s->pl->v2.chal_data.c_rank.part.battle, 7 * sizeof(uint32_t));

        /* Copy over the color-related information... This also apparently
           has something to do with the length of the string... */
        memcpy(pkt->entries[entry].c_rank.part.unk3, s->pl->v2.chal_data.c_rank.part.unk2,
            sizeof(uint32_t));
        memcpy(pkt->entries[entry].c_rank.part.unk3 + 8, s->pl->v2.chal_data.c_rank.part.unk2,
            sizeof(uint32_t));
        memcpy(pkt->entries[entry].c_rank.part.unk3 + 4, s->pl->v2.chal_data.c_rank.part.unk2 + 8,
            sizeof(uint32_t));
        memcpy(pkt->entries[entry].c_rank.part.unk3 + 12,
            s->pl->v2.chal_data.c_rank.part.unk2 + 8, sizeof(uint32_t));
        break;

    case CLIENT_VERSION_PC:
        pkt->entries[entry].client_id = LE32(s->client_id);

        memset(pkt->entries[entry].c_rank.all, 0, 0x0158);

        pkt->entries[entry].c_rank.part.unk1 = (s->pl->pc.chal_data.c_rank.part.unk1 >> 16) |
            (s->pl->pc.chal_data.c_rank.part.unk1 << 16);

        /* Copy the rank over. */
        for (j = 0; j < 0x0C; ++j) {
            pkt->entries[entry].c_rank.part.string[j] =
                (char)LE16(s->pl->pc.chal_data.c_rank.part.string[j]);
        }

        /* Copy the times for the levels and battle stuff over... */
        memcpy(pkt->entries[entry].c_rank.part.times, s->pl->pc.chal_data.c_rank.part.times,
            9 * sizeof(uint32_t));
        memcpy(pkt->entries[entry].c_rank.part.battle, s->pl->pc.chal_data.c_rank.part.battle,
            7 * sizeof(uint32_t));

        /* Copy over the color-related information... This also apparently
           has something to do with the length of the string... */
        memcpy(pkt->entries[entry].c_rank.part.unk3, s->pl->pc.chal_data.c_rank.part.unk2,
            sizeof(uint32_t));
        memcpy(pkt->entries[entry].c_rank.part.unk3 + 8, s->pl->pc.chal_data.c_rank.part.unk2,
            sizeof(uint32_t));
        memcpy(pkt->entries[entry].c_rank.part.unk3 + 4, s->pl->pc.chal_data.c_rank.part.unk2 + 8,
            sizeof(uint32_t));
        memcpy(pkt->entries[entry].c_rank.part.unk3 + 12,
            s->pl->pc.chal_data.c_rank.part.unk2 + 8, sizeof(uint32_t));
        break;

    case CLIENT_VERSION_BB:
        //DBG_LOG("copy_c_rank_bb");
        pkt->entries[entry].client_id = LE32(s->client_id);
        memcpy(pkt->entries[entry].c_rank.all, s->c_rank, 0x0158);
        break;

    default:
        pkt->entries[entry].client_id = LE32(s->client_id);
        memset(pkt->entries[entry].c_rank.all, 0, 0x0158);
    }
}

/* Send the lobby's C-Rank data to the client. */
static int send_gc_lobby_c_rank(ship_client_t *c, lobby_t *l) {
    int i;
    uint8_t *sendbuf = get_sendbuf();
    gc_c_rank_update_pkt *pkt = (gc_c_rank_update_pkt *)sendbuf;
    int entries = 0, size = 4;
    ship_client_t *c2;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL) {
            c2 = l->clients[i];
            pthread_mutex_lock(&c2->mutex);

            /* Copy over this character's data... */
            copy_c_rank_gc(pkt, entries, c2);
            ++entries;
            size += 0x011C;

            pthread_mutex_unlock(&c2->mutex);
        }
    }

    /* 填充数据头. */
    pkt->hdr.pkt_type = C_RANK_TYPE;
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(size);

    return crypt_send(c, size, sendbuf);
}

static int send_dc_lobby_c_rank(ship_client_t *c, lobby_t *l) {
    int i;
    uint8_t *sendbuf = get_sendbuf();
    dc_c_rank_update_pkt *pkt = (dc_c_rank_update_pkt *)sendbuf;
    int entries = 0, size = 4;
    ship_client_t *c2;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL) {
            c2 = l->clients[i];
            pthread_mutex_lock(&c2->mutex);

            /* Copy this character's data... */
            copy_c_rank_dc(pkt, entries, c2);
            ++entries;
            size += 0xBC;

            pthread_mutex_unlock(&c2->mutex);
        }
    }

    /* 填充数据头. */
    pkt->hdr.pkt_type = C_RANK_TYPE;
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(size);

    return crypt_send(c, size, sendbuf);
}

static int send_pc_lobby_c_rank(ship_client_t *c, lobby_t *l) {
    int i;
    uint8_t *sendbuf = get_sendbuf();
    pc_c_rank_update_pkt *pkt = (pc_c_rank_update_pkt *)sendbuf;
    int entries = 0, size = 4;
    ship_client_t *c2;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL) {
            c2 = l->clients[i];
            pthread_mutex_lock(&c2->mutex);

            /* Copy over this character's data... */
            copy_c_rank_pc(pkt, entries, c2);
            ++entries;
            size += 0xF4;

            pthread_mutex_unlock(&c2->mutex);
        }
    }

    /* 填充数据头. */
    pkt->hdr.pkt_type = C_RANK_TYPE;
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(size);

    return crypt_send(c, size, sendbuf);
}

static int send_bb_lobby_c_rank(ship_client_t* c, lobby_t* l) {
    int i;
    uint8_t* sendbuf = get_sendbuf();
    bb_c_rank_update_pkt* pkt = (bb_c_rank_update_pkt*)sendbuf;
    int entries = 0, size = sizeof(bb_pkt_hdr_t);
    ship_client_t* c2;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] != NULL) {
            c2 = l->clients[i];
            pthread_mutex_lock(&c2->mutex);

            /* Copy over this character's data... */
            copy_c_rank_bb(pkt, entries, c2);
            ++entries;
            size += 0x015C;

            pthread_mutex_unlock(&c2->mutex);
        }
    }

    /* 填充数据头. */
    pkt->hdr.pkt_type = C_RANK_TYPE;
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(size);

    return crypt_send(c, size, sendbuf);
}

int send_lobby_c_rank(ship_client_t *c, lobby_t *l) {
    switch(c->version) {
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_gc_lobby_c_rank(c, l);

        case CLIENT_VERSION_DCV2:
            return send_dc_lobby_c_rank(c, l);

        case CLIENT_VERSION_PC:
            return send_pc_lobby_c_rank(c, l);

        case CLIENT_VERSION_BB:
            return send_bb_lobby_c_rank(c, l);
    }

    /* Don't send to unsupporting clients. */
    return 0;
}

/* Send a C-Rank update for a single client to the whole lobby. */
static int send_gc_c_rank_update(ship_client_t *d, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    gc_c_rank_update_pkt *pkt = (gc_c_rank_update_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Copy the data. */
    copy_c_rank_gc(pkt, 0, s);

    /* 填充数据头. */
    pkt->hdr.pkt_type = C_RANK_TYPE;
    pkt->hdr.flags = 1;
    pkt->hdr.pkt_len = LE16(0x0120);

    return crypt_send(d, 0x0120, sendbuf);
}

static int send_dc_c_rank_update(ship_client_t *d, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    dc_c_rank_update_pkt *pkt = (dc_c_rank_update_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Copy the data. */
    copy_c_rank_dc(pkt, 0, s);

    /* 填充数据头. */
    pkt->hdr.pkt_type = C_RANK_TYPE;
    pkt->hdr.flags = 1;
    pkt->hdr.pkt_len = LE16(0xC0);

    return crypt_send(d, 0xC0, sendbuf);
}

static int send_pc_c_rank_update(ship_client_t *d, ship_client_t *s) {
    uint8_t *sendbuf = get_sendbuf();
    pc_c_rank_update_pkt *pkt = (pc_c_rank_update_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Copy the data. */
    copy_c_rank_pc(pkt, 0, s);

    /* 填充数据头. */
    pkt->hdr.pkt_type = C_RANK_TYPE;
    pkt->hdr.flags = 1;
    pkt->hdr.pkt_len = LE16(0xF8);

    return crypt_send(d, 0xF8, sendbuf);
}

static int send_bb_c_rank_update(ship_client_t* d, ship_client_t* s) {
    uint8_t* sendbuf = get_sendbuf();
    bb_c_rank_update_pkt* pkt = (bb_c_rank_update_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Copy the data. */
    copy_c_rank_bb(pkt, 0, s);

    /* 填充数据头. */
    pkt->hdr.pkt_type = C_RANK_TYPE;
    pkt->hdr.flags = 1;
    pkt->hdr.pkt_len = LE16(0x0160);

    return crypt_send(d, 0x0160, sendbuf);
}

int send_c_rank_update(ship_client_t *c, lobby_t *l) {
    int i;

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] != NULL && l->clients[i] != c) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    send_gc_c_rank_update(l->clients[i], c);
                    break;

                case CLIENT_VERSION_DCV2:
                    send_dc_c_rank_update(l->clients[i], c);
                    break;

                case CLIENT_VERSION_PC:
                    send_pc_c_rank_update(l->clients[i], c);
                    break;

                case CLIENT_VERSION_BB:
                    send_bb_c_rank_update(l->clients[i], c);
                    break;
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    return 0;
}

/* Send a statistics mod packet to a client. */
static int send_dc_mod_stat(ship_client_t *d, ship_client_t *s, int stat,
                            int amt) {
    uint8_t *sendbuf = get_sendbuf();
    //subcmd_pkt_t *pkt = (subcmd_pkt_t *)sendbuf;
    subcmd_update_player_stat_t *pkt = (subcmd_update_player_stat_t *)sendbuf;
    int len = 0;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Fill in the main part of the packet */
    while(amt > 0) {
        pkt->shdr.type = SUBCMD60_CHANGE_STAT;
        pkt->shdr.size = 0x02;
        pkt->shdr.client_id = s->client_id;
        pkt->client_id2 = 0x0000;
        pkt->stat = stat;
        pkt->amount = (amt > 0xFF) ? 0xFF : amt;

        len += sizeof(subcmd_update_player_stat_t);

        //sendbuf[len++] = SUBCMD60_CHANGE_STAT;
        //sendbuf[len++] = 2;
        //sendbuf[len++] = s->client_id;
        //sendbuf[len++] = 0;
        //sendbuf[len++] = 0;
        //sendbuf[len++] = 0;
        //sendbuf[len++] = stat;
        //sendbuf[len++] = (amt > 0xFF) ? 0xFF : amt;
        amt -= 0xFF;
    }

    /* 填充数据头 */
    if(d->version == CLIENT_VERSION_DCV1 || d->version == CLIENT_VERSION_DCV2 ||
       d->version == CLIENT_VERSION_GC || d->version == CLIENT_VERSION_EP3 ||
       d->version == CLIENT_VERSION_XBOX) {
        pkt->hdr.dc.pkt_type = GAME_COMMAND0_TYPE;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(len);
    }
    else {
        pkt->hdr.pc.pkt_type = GAME_COMMAND0_TYPE;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(len);
    }

    /* 将数据包发送出去 */
    return crypt_send(d, len, sendbuf);
}

static int send_bb_mod_stat(ship_client_t *d, ship_client_t *s, int stat,
                            int amt) {
    uint8_t *sendbuf = get_sendbuf();
    //subcmd_bb_pkt_t *pkt = (subcmd_bb_pkt_t *)sendbuf;
    subcmd_bb_update_player_stat_t *pkt = (subcmd_bb_update_player_stat_t *)sendbuf;
    int len = 0;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Fill in the main part of the packet */
    while(amt > 0) {
        pkt->shdr.type = SUBCMD60_CHANGE_STAT;
        pkt->shdr.size = 0x02;
        pkt->shdr.client_id = s->client_id;
        pkt->client_id2 = 0x0000;
        pkt->stat = stat;
        pkt->amount = (amt > 0xFF) ? 0xFF : amt;

        len += sizeof(subcmd_bb_update_player_stat_t);

        //sendbuf[len++] = SUBCMD60_CHANGE_STAT;
        //sendbuf[len++] = 2;
        //sendbuf[len++] = s->client_id;
        //sendbuf[len++] = 0;
        //sendbuf[len++] = 0;
        //sendbuf[len++] = 0;
        //sendbuf[len++] = stat;
        //sendbuf[len++] = (amt > 0xFF) ? 0xFF : amt;
        amt -= 0xFF;
    }

    DBG_LOG("测试是否正常");

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt->hdr.flags = 0;

    /* 将数据包发送出去 */
    return crypt_send(d, len, sendbuf);
}

int send_lobby_mod_stat(lobby_t *l, ship_client_t *c, int stat, int amt) {
    int i;

    /* Don't send these to default lobbies, ever */
    if(l->type == LOBBY_TYPE_LOBBY) {
        return 0;
    }

    /* Make sure the request is sane */
    if(stat < SUBCMD60_STAT_HPDOWN || stat > SUBCMD60_STAT_TPUP || amt < 1 ||
       amt > 2040) {
        return 0;
    }

    pthread_mutex_lock(&l->mutex);

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_PC:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                case CLIENT_VERSION_XBOX:
                    send_dc_mod_stat(l->clients[i], c, stat, amt);
                    break;

                case CLIENT_VERSION_BB:
                    send_bb_mod_stat(l->clients[i], c, stat, amt);
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    pthread_mutex_unlock(&l->mutex);

    return 0;
}

/* Send a reply to an Episode III jukebox request (showing updated meseta values
   for the requesting client). */
int send_ep3_jukebox_reply(ship_client_t *c, uint16_t magic) {
    uint8_t *sendbuf = get_sendbuf();
    ep3_ba_pkt *pkt = (ep3_ba_pkt *)sendbuf;

    /* XXXX: Fill in meseta values with correct numbers. */

    /* 填充数据头 first... */
    pkt->hdr.pkt_type = EP3_COMMAND_TYPE;
    pkt->hdr.flags = EP3_COMMAND_JUKEBOX_REPLY;
    pkt->hdr.pkt_len = LE16(0x0010);
    pkt->meseta = LE32(0x0000012C);
    pkt->total_meseta = LE32(0x000008E8);
    pkt->unused = 0;
    pkt->magic = LE16(magic);

    return crypt_send(c, 0x10, sendbuf);
}

/* Send a reply to an Episode 3 leave team? packet. */
int send_ep3_ba01(ship_client_t *c, uint16_t magic) {
    uint8_t *sendbuf = get_sendbuf();
    ep3_ba_pkt *pkt = (ep3_ba_pkt *)sendbuf;

    /* XXXX: Fill in meseta values with correct numbers. */

    /* 填充数据头 first... */
    pkt->hdr.pkt_type = EP3_COMMAND_TYPE;
    pkt->hdr.flags = EP3_COMMAND_LEAVE_TEAM;
    pkt->hdr.pkt_len = LE16(0x0010);
    pkt->meseta = LE32(0x0000012C);
    pkt->total_meseta = LE32(0x000008E8);
    pkt->unused = 0;
    pkt->magic = LE16(magic);

    return crypt_send(c, 0x10, sendbuf);
}

/* Send a user the Blue Burst full character/option data packet. */
int send_bb_full_char(ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    bb_full_char_pkt *pkt = (bb_full_char_pkt *)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear it out first */
    memset(pkt, 0, BB_FULL_CHARACTER_DATA_LENGTH);

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(BB_FULL_CHARACTER_DATA_LENGTH);
    pkt->hdr.pkt_type = LE16(BB_FULL_CHARACTER_TYPE);
    pkt->hdr.flags = 0;

    /* Fill in all the parts of it... */
    ///////////////////////////////////////////////////////////////////////////////////////
    memcpy(&pkt->data.inv, &c->bb_pl->inv, sizeof(inventory_t));
    ///////////////////////////////////////////////////////////////////////////////////////
    memcpy(&pkt->data.character, &c->bb_pl->character, sizeof(psocn_bb_char_t));
    ///////////////////////////////////////////////////////////////////////////////////////
    pkt->data.option_flags = c->bb_opts->option_flags;
    ///////////////////////////////////////////////////////////////////////////////////////
    memcpy(&pkt->data.quest_data1, &c->bb_pl->quest_data1, sizeof(psocn_quest_data1_t));
    ///////////////////////////////////////////////////////////////////////////////////////
    memcpy(&pkt->data.bank, &c->bb_pl->bank, sizeof(psocn_bank_t));
    ///////////////////////////////////////////////////////////////////////////////////////
    pkt->data.gc_data.guildcard = LE32(c->guildcard);
    memcpy(&pkt->data.gc_data.name, &c->bb_pl->character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
    memcpy(pkt->data.gc_data.guild_name, c->bb_opts->guild_name, sizeof(c->bb_opts->guild_name));
    memcpy(pkt->data.gc_data.guildcard_desc, c->bb_pl->guildcard_desc, sizeof(c->bb_pl->guildcard_desc));
    pkt->data.gc_data.present = 1;
    pkt->data.gc_data.language = c->language_code;
    pkt->data.gc_data.ch_class = c->bb_pl->character.dress_data.ch_class;
    ///////////////////////////////////////////////////////////////////////////////////////
    memcpy(pkt->data.symbol_chats, c->bb_opts->symbol_chats, sizeof(c->bb_opts->symbol_chats));
    memcpy(pkt->data.shortcuts, c->bb_opts->shortcuts, sizeof(c->bb_opts->shortcuts));
    memcpy(pkt->data.autoreply, c->bb_pl->autoreply, sizeof(c->bb_pl->autoreply));
    memcpy(pkt->data.infoboard, c->bb_pl->infoboard, sizeof(c->bb_pl->infoboard));
    memcpy(pkt->data.challenge_data, c->bb_pl->challenge_data, sizeof(c->bb_pl->challenge_data));
    memcpy(pkt->data.tech_menu, c->bb_pl->tech_menu, sizeof(c->bb_pl->tech_menu));
    memcpy(pkt->data.quest_data2, c->bb_pl->quest_data2, sizeof(c->bb_pl->quest_data2));
    memcpy(&pkt->data.key_cfg, &c->bb_opts->key_cfg, sizeof(bb_key_config_t));
    ///////////////////////////////////////////////////////////////////////////////////////
    memcpy(&pkt->data.guild_data, &c->bb_guild, sizeof(bb_guild_t));

    /* FIXME: 需修复公会数据 */
    /* 将数据包发送出去 */
    return crypt_send(c, BB_FULL_CHARACTER_DATA_LENGTH, sendbuf);
}

/* Send a GM Menu to a client. */
static int send_dc_gm_menu(ship_client_t *c, uint32_t menu_id) {
    uint8_t *sendbuf = get_sendbuf();
    dc_block_list_pkt *pkt = (dc_block_list_pkt *)sendbuf;
    int i, len = 0x20, entries = 1;
    const char *opt;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the base packet */
    memset(pkt, 0, sizeof(dc_block_list_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;
    strncpy(pkt->entries[0].name, ship->cfg->name, 0x10);

    /* Add what's needed at the end */
    pkt->entries[0].name[0x0F] = 0x00;
    pkt->entries[0].name[0x10] = 0x08;
    pkt->entries[0].name[0x11] = 0x00;

    /* Add each info item to the list. */
    for(i = 0; gm_opts[i].menu_id; ++i) {
        /* Make sure the user is logged in and has the required privilege */
        if(!(c->flags & CLIENT_FLAG_LOGGED_IN) ||
           (c->privilege & gm_opts[i].privilege) != gm_opts[i].privilege) {
            continue;
        }

        /* Make sure the item is in the right menu */
        if(gm_opts[i].menu_id != menu_id) {
            continue;
        }

        /* Make sure the user is in an appropriate lobby type. */
        if(!(c->cur_lobby->type & gm_opts[i].lobby_type)) {
            continue;
        }

        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x1C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(menu_id);
        pkt->entries[entries].item_id = LE32(gm_opts[i].item_id);
        pkt->entries[entries].flags = LE16(0x0000);

        /* Fill in the text... */
        opt = __(c, gm_opts[i].text);

        if(opt[0] == '\t' && opt[1] == 'J') {
            istrncpy(ic_utf8_to_sjis, pkt->entries[entries].name, opt, 0x10);
        }
        else {
            istrncpy(ic_utf8_to_8859, pkt->entries[entries].name, opt, 0x10);
        }

        len += 0x1C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_gm_menu(ship_client_t *c, uint32_t menu_id) {
    uint8_t *sendbuf = get_sendbuf();
    pc_block_list_pkt *pkt = (pc_block_list_pkt *)sendbuf;
    int i, len = 0x30, entries = 1;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the base packet */
    memset(pkt, 0, 0x30);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;

    istrncpy(ic_8859_to_utf16, (char *)pkt->entries[0].name, ship->cfg->name,
             0x20);

    /* Add each info item to the list. */
    for(i = 0; gm_opts[i].menu_id; ++i) {
        /* Make sure the user is logged in and has the required privilege */
        if(!(c->flags & CLIENT_FLAG_LOGGED_IN) ||
           (c->privilege & gm_opts[i].privilege) != gm_opts[i].privilege) {
            continue;
        }

        /* Make sure the item is in the right menu */
        if(gm_opts[i].menu_id != menu_id) {
            continue;
        }

        /* Make sure the user is in an appropriate lobby type */
        if(!(c->cur_lobby->type & gm_opts[i].lobby_type)) {
            continue;
        }

        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x2C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(menu_id);
        pkt->entries[entries].item_id = LE32(gm_opts[i].item_id);
        pkt->entries[entries].flags = LE16(0x0000);

        istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name,
                 __(c, gm_opts[i].text), 0x20);

        len += 0x2C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_gm_menu(ship_client_t *c, uint32_t menu_id) {
    uint8_t *sendbuf = get_sendbuf();
    bb_block_list_pkt *pkt = (bb_block_list_pkt *)sendbuf;
    int i, len = 0x34, entries = 1;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf) {
        return -1;
    }

    /* Clear the base packet */
    memset(pkt, 0, 0x30);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;

    istrncpy(ic_8859_to_utf16, (char *)pkt->entries[0].name, ship->cfg->name,
             0x20);

    /* Add each info item to the list. */
    for(i = 0; gm_opts[i].menu_id; ++i) {
        /* Make sure the user is logged in and has the required privilege */
        if(!(c->flags & CLIENT_FLAG_LOGGED_IN) ||
           (c->privilege & gm_opts[i].privilege) != gm_opts[i].privilege) {
            continue;
        }

        /* Make sure the item is in the right menu */
        if(gm_opts[i].menu_id != menu_id) {
            continue;
        }

        /* Make sure the user is in an appropriate lobby type */
        if(!(c->cur_lobby->type & gm_opts[i].lobby_type)) {
            continue;
        }

        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x2C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(menu_id);
        pkt->entries[entries].item_id = LE32(gm_opts[i].item_id);
        pkt->entries[entries].flags = LE16(0x0000);

        istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name,
                 __(c, gm_opts[i].text), 0x20);

        len += 0x2C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = LE32(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

int send_gm_menu(ship_client_t *c, uint32_t menu_id) {
    /* Make sure the user's at least a Local GM */
    if(!LOCAL_GM(c)) {
        return -1;
    }

    /* Call the appropriate function */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_gm_menu(c, menu_id);

        case CLIENT_VERSION_PC:
            return send_pc_gm_menu(c, menu_id);

        case CLIENT_VERSION_BB:
            return send_bb_gm_menu(c, menu_id);
    }

    return -1;
}

int send_bb_rare_monster_data(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;
    uint8_t* sendbuf = get_sendbuf();
    bb_rare_monster_list_pkt* pkt = (bb_rare_monster_list_pkt*)sendbuf;

    /* Make sure we got the sendbuf */
    if (!sendbuf)
        return -1;

    uint16_t len = sizeof(bb_rare_monster_list_pkt);

    /* 填充数据并准备发送 */
    pkt->hdr.pkt_type = LE16(BB_RARE_MONSTER_LIST);
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = 0;

    memcpy(&pkt->enemy_ids[0], &l->map_enemies->enemies[0], sizeof(pkt->enemy_ids));

    return crypt_send(c, len, sendbuf);
}

static int send_bb_end_burst(ship_client_t *c) {
    uint8_t *sendbuf = get_sendbuf();
    subcmd_bb_end_burst_t *pkt = (subcmd_bb_end_burst_t *)sendbuf;

    /* Make sure we got the sendbuf */
    if(!sendbuf)
        return -1;

    /* 填充数据并准备发送 */
    pkt->hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt->hdr.pkt_len = LE16(0x000C);
    pkt->hdr.flags = 0;
    pkt->shdr.type = SUBCMD60_BURST_DONE;
    pkt->shdr.size = 0x03;
    pkt->shdr.params = 0x0818;

    return crypt_send(c, 0x000C, sendbuf);
}

int send_lobby_end_burst(lobby_t *l) {
    int i;

    /* Only send these to Blue Burst game lobbies */
    if(!(l->type & LOBBY_TYPE_GAME) || l->version != CLIENT_VERSION_BB)
        return 0;

    /*DBG_LOG(
        "send_lobby_end_burst");*/

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            send_bb_end_burst(l->clients[i]);
        }
    }

    return 0;
}

/* Send a generic menu to a client. */
static int send_dc_gen_menu(ship_client_t *c, uint32_t menu_id, size_t count,
                            gen_menu_entry_t *ents) {
    uint8_t *sendbuf = get_sendbuf();
    dc_block_list_pkt *pkt = (dc_block_list_pkt *)sendbuf;
    size_t i;
    uint16_t len = 0x20;
    uint8_t entries = 1;
    const char *opt;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the base packet */
    memset(pkt, 0, 0x20);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;
    strncpy(pkt->entries[0].name, ship->cfg->name, 0x10);

    /* Add each info item to the list. */
    for(i = 0; i < count; ++i) {
        /* Clear out the entry */
        memset(&pkt->entries[entries], 0, 0x1C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(menu_id);
        pkt->entries[entries].item_id = LE32(ents[i].item_id);
        pkt->entries[entries].flags = LE16(0x0000);

        /* Fill in the text... */
        opt = __(c, ents[i].text);

        if(opt[0] == '\t' && opt[1] == 'J') {
            istrncpy(ic_utf8_to_sjis, pkt->entries[entries].name, opt, 0x10);
        }
        else {
            istrncpy(ic_utf8_to_8859, pkt->entries[entries].name, opt, 0x10);
        }

        len += 0x1C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

static int send_pc_gen_menu(ship_client_t *c, uint32_t menu_id, size_t count,
                            gen_menu_entry_t *ents) {
    uint8_t *sendbuf = get_sendbuf();
    pc_block_list_pkt *pkt = (pc_block_list_pkt *)sendbuf;
    size_t i;
    uint16_t len = 0x30;
    uint8_t entries = 1;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the base packet */
    memset(pkt, 0, 0x30);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;

    istrncpy(ic_8859_to_utf16, (char *)pkt->entries[0].name, ship->cfg->name,
             0x20);

    /* Add each info item to the list. */
    for(i = 0; i < count; ++i) {
        /* Clear out the entry */
        memset(&pkt->entries[entries], 0, 0x2C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(menu_id);
        pkt->entries[entries].item_id = LE32(ents[i].item_id);
        pkt->entries[entries].flags = LE16(0x0000);

        istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name,
                 __(c, ents[i].text), 0x20);

        len += 0x2C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

static int send_bb_gen_menu(ship_client_t *c, uint32_t menu_id, size_t count,
                            gen_menu_entry_t *ents) {
    uint8_t *sendbuf = get_sendbuf();
    bb_block_list_pkt *pkt = (bb_block_list_pkt *)sendbuf;
    //int i, len = 0x34, entries = 1;
    size_t i;
    uint16_t len = 0x34;
    uint8_t entries = 1;

    /* 确认已获得数据发送缓冲 */
    if(!sendbuf)
        return -1;

    /* Clear the base packet */
    memset(pkt, 0, 0x30);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LOBBY_INFO_TYPE;

    /* Fill in the ship name entry */
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = 0;

    istrncpy(ic_8859_to_utf16, (char *)pkt->entries[0].name, ship->cfg->name,
             0x20);

    /* Add each info item to the list. */
    for(i = 0; i < count; ++i) {
        /* Clear out the ship information */
        memset(&pkt->entries[entries], 0, 0x2C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(menu_id);
        pkt->entries[entries].item_id = LE32(ents[i].item_id);
        pkt->entries[entries].flags = LE16(0x0000);

        istrncpy(ic_gbk_to_utf16, (char *)pkt->entries[entries].name,
                 __(c, ents[i].text), 0x20);

        len += 0x2C;
        ++entries;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = LE32(entries - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len, sendbuf);
}

int send_generic_menu(ship_client_t *c, uint32_t menu_id, size_t count,
                      gen_menu_entry_t *ents) {
    /* Call the appropriate function */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            return send_dc_gen_menu(c, menu_id, count, ents);

        case CLIENT_VERSION_PC:
            return send_pc_gen_menu(c, menu_id, count, ents);

        case CLIENT_VERSION_BB:
            return send_bb_gen_menu(c, menu_id, count, ents);
    }

    return -1;
}

static int send_dc_sync_register(ship_client_t *c, uint16_t reg_num, uint32_t value) {
    uint8_t *sendbuf = get_sendbuf();
    subcmd_sync_reg_t *pkt = (subcmd_sync_reg_t *)sendbuf;
    subcmd_pkt_t *pkt2 = (subcmd_pkt_t *)sendbuf;

    if(!sendbuf)
        return -1;

    /* Clear the packet */
    memset(pkt, 0, sizeof(subcmd_sync_reg_t));

    /* Fill it in */
    if(c->version == CLIENT_VERSION_PC) {
        pkt2->hdr.pc.pkt_type = GAME_COMMAND0_TYPE;
        pkt2->hdr.pc.pkt_len = LE16(sizeof(subcmd_sync_reg_t));
    }
    else {
        pkt2->hdr.dc.pkt_type = GAME_COMMAND0_TYPE;
        pkt2->hdr.dc.pkt_len = LE16(sizeof(subcmd_sync_reg_t));
    }

    pkt->shdr.type = SUBCMD60_SYNC_REG;
    pkt->shdr.size = 3;
    pkt->register_number = reg_num;
    pkt->value = LE32(value);

    /* 加密并发送 */
    return crypt_send(c, sizeof(subcmd_sync_reg_t), sendbuf);
}

static int send_bb_sync_register(ship_client_t* c, uint16_t reg_num, uint32_t value) {
    uint8_t* sendbuf = get_sendbuf();
    subcmd_bb_sync_reg_t* pkt = (subcmd_bb_sync_reg_t*)sendbuf;
    subcmd_bb_pkt_t* pkt2 = (subcmd_bb_pkt_t*)sendbuf;

    if (!sendbuf)
        return -1;

    /* Clear the packet */
    memset(pkt, 0, sizeof(subcmd_bb_sync_reg_t));

    /* Fill it in */

    pkt2->hdr.pkt_type = GAME_COMMAND0_TYPE;
    pkt2->hdr.pkt_len = LE16(sizeof(subcmd_bb_sync_reg_t));

    pkt->shdr.type = SUBCMD60_SYNC_REG;
    pkt->shdr.size = 3;
    pkt->register_number = reg_num;
    pkt->value = LE32(value);

    /* 加密并发送 */
    return crypt_send(c, sizeof(subcmd_bb_sync_reg_t), sendbuf);
}

int send_sync_register(ship_client_t *c, uint16_t reg_num, uint32_t value) {
    /* Call the appropriate function for the client's version */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            return send_dc_sync_register(c, reg_num, value);

        case CLIENT_VERSION_BB:     /* TODO  */
            return send_bb_sync_register(c, reg_num, value);

        case CLIENT_VERSION_EP3:    /* ???? */
            return -1;

    }

    return -1;
}

int send_lobby_sync_register(lobby_t *l, uint8_t n, uint32_t v) {
    int i;

    pthread_mutex_lock(&l->mutex);

    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            pthread_mutex_lock(&l->clients[i]->mutex);

            /* Call the appropriate function. */
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                case CLIENT_VERSION_PC:
                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_XBOX:
                    send_dc_sync_register(l->clients[i], n, v);
                    break;

                case CLIENT_VERSION_BB:
                    send_bb_sync_register(l->clients[i], n, v);
                    break;

                case CLIENT_VERSION_EP3:
                    /* XXXX */
                    break;
            }

            pthread_mutex_unlock(&l->clients[i]->mutex);
        }
    }

    pthread_mutex_unlock(&l->mutex);

    return 0;
}

int send_ban_msg(ship_client_t *c, time_t until, const char *reason) {
    char string[1024] = { 0 };
    //struct tm cooked;
    size_t len = 0;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    /* Create the ban string. */
    string[1023] = 0;
    len = snprintf(string, 1023, "%s\n%s\n%s\tC7\n\n%s\n",
                   __(c, "\tEYou have been banned from this ship."),
                   __(c, "Reason:"), reason, __(c, "Your ban expires:"));

    if(until == (time_t)-1) {
        strncat(string, __(c, "Never"), 1023 - len);
    }
    else {
        //gmtime_r(&until, &cooked);
        snprintf(string + len, 1023 - len, "%02u:%02u UTC %u.%02u.%02u",
            rawtime.wHour, rawtime.wMinute, rawtime.wYear,
            rawtime.wMonth, rawtime.wDay);
    }

    return send_msg(c, MSG_BOX_TYPE, "%s", string);
}

int check_size_v(size_t size, size_t min_size, size_t max_size) {
    if (size < min_size) {
        ERR_LOG("指令数据包太小 (最小应为 0x%zX 字节, 当前 0x%zX 字节)",
            min_size, size);
        return -1;
    }
    if (max_size < min_size) {
        max_size = min_size;
    }
    if (size > max_size) {
        ERR_LOG("指令数据包过大 (最大应为 0x%zX 字节, 当前 0x%zX 字节)",
            max_size, size);
        return -1;
    }

    return 0;
}

int send_bb_execute_item_trade(ship_client_t* c, item_t* items) {
    uint8_t* sendbuf = get_sendbuf();
    bb_trade_D0_D3_pkt* pkt = (bb_trade_D0_D3_pkt*)sendbuf;
    int rv = -1;
    size_t item_size = sizeof(items);

    if (/*items.size()*/item_size > sizeof(pkt->items) / sizeof(pkt->items[0])) {
        ERR_LOG("GC %" PRIu32 " 尝试交易的物品超出限制!",
            c->guildcard);
        return -1;
        //throw logic_error("too many items in execute trade command");
    }
    pkt->target_client_id = c->client_id;
    pkt->item_count = LE16((uint16_t)item_size);//items.size();
    for (size_t x = 0; x < item_size/*items.size()*/; x++) {
        pkt->items[x] = items[x];
    }

    lobby_t* l = c->cur_lobby;
    ship_client_t* c2;
    c2 = l->clients[pkt->target_client_id];
    DBG_LOG("GC %" PRIu32 " 尝试交易 %d 件物品 给 %" PRIu32 "!",
        c->guildcard, pkt->item_count, c2->guildcard);

    //send_command_t(c, 0xD3, 0x00, pkt);
    pkt->hdr.pkt_type = TRADE_3_TYPE;
    pkt->hdr.flags = 0x00;
    //pkt->type = SUBCMD62_TRADE;
    //pkt->size = 0x04;
    rv = send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
    return rv;
}

/* Send a message box containing an information file entry. */
int send_dc_info_file(ship_client_t* c, ship_t* s, uint32_t entry) {
    FILE* fp;
    char buf[4096];
    long len;

    /* The item_id should be the information the client wants. */
    if ((int)entry >= s->cfg->info_file_count) {
        send_msg(c, MSG1_TYPE, "%s\n%s",
            __(c, "\tE\tC4Something went wrong!"),
            __(c, "\tC7The information requested is missing."));
        return 0;
    }

    /* Attempt to open the file */
    fp = fopen(s->cfg->info_files[entry].filename, "r");

    if (!fp) {
        send_msg(c, MSG1_TYPE, "%s\n%s",
            __(c, "\tE\tC4Something went wrong!"),
            __(c, "\tC7The information requested is missing."));
        return 0;
    }

    /* Figure out the length of the file. */
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Truncate to about 1KB */
    if (len > 4096) {
        len = 4096;
    }

    /* Read the file in. */
    fread(buf, 1, len, fp);
    fclose(fp);
    buf[len] = 0;

    /* Send the message to the client. */
    return send_msg(c, MSG_BOX_TYPE, "%s", buf);
}

/* Send a message box containing an information file entry. */
int send_bb_info_file(ship_client_t* c, ship_t* s, uint32_t entry) {
    FILE* fp;
    char buf[4096];
    long len;

    /* The item_id should be the information the client wants. */
    if ((int)entry >= s->cfg->info_file_count) {
        send_msg(c, MSG1_TYPE, "%s\n%s",
            __(c, "\tE\tC4Something went wrong!"),
            __(c, "\tC7The information requested is missing."));
        return 0;
    }

    /* Attempt to open the file */
    fp = fopen(s->cfg->info_files[entry].filename, "r");

    if (!fp) {
        send_msg(c, MSG1_TYPE, "%s\n%s",
            __(c, "\tE\tC4Something went wrong!"),
            __(c, "\tC7The information requested is missing."));
        return 0;
    }

    /* Figure out the length of the file. */
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Truncate to about 4KB */
    if (len > 4096) {
        len = 4096;
    }

    /* Read the file in. */
    fread(buf, 1, len, fp);
    fclose(fp);
    buf[len] = 0;

    /* Send the message to the client. */
    return send_msg(c, MSG_BOX_TYPE, "%s", buf);
}

int send_info_file(ship_client_t* c, ship_t* s, uint32_t entry) {
    int rv = -1;

    /* Call the appropriate function. */
    switch (c->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
    case CLIENT_VERSION_PC:
        rv = send_dc_info_file(c, s, entry);

    case CLIENT_VERSION_BB:
        rv = send_bb_info_file(c, s, entry);
    }

    return rv;
}

static int send_mhit(ship_client_t* c, uint16_t enemy_id, uint16_t enemy_id2,
    uint16_t damage, uint32_t flags) {
    subcmd_mhit_pkt_t pkt;

    memset(&pkt, 0, sizeof(subcmd_mhit_pkt_t));
    pkt.hdr.pkt_type = GAME_COMMAND0_TYPE;
    pkt.hdr.pkt_len = LE16(0x0010);
    pkt.shdr.type = SUBCMD60_HIT_MONSTER;
    pkt.shdr.size = 0x03;
    pkt.shdr.enemy_id = LE16(enemy_id2);
    pkt.enemy_id2 = LE16(enemy_id);
    pkt.damage = LE16(damage);
    pkt.flags = LE32(flags);

    return send_pkt_dc(c, (dc_pkt_hdr_t*)&pkt);
}

static int send_mhit_gc(ship_client_t* c, uint16_t enemy_id, uint16_t enemy_id2,
    uint16_t damage, uint32_t flags) {
    subcmd_mhit_pkt_t pkt;

    memset(&pkt, 0, sizeof(subcmd_mhit_pkt_t));
    pkt.hdr.pkt_type = GAME_COMMAND0_TYPE;
    pkt.hdr.pkt_len = LE16(0x0010);
    pkt.shdr.type = SUBCMD60_HIT_MONSTER;
    pkt.shdr.size = 0x03;
    pkt.shdr.enemy_id = LE16(enemy_id2);
    pkt.enemy_id2 = LE16(enemy_id);
    pkt.damage = LE16(damage);
    pkt.flags = LE32(SWAP32(flags));

    return send_pkt_dc(c, (dc_pkt_hdr_t*)&pkt);
}

static int send_mhit_bb(ship_client_t* c, uint16_t enemy_id, uint16_t enemy_id2,
    uint16_t damage, uint32_t flags) {
    subcmd_bb_mhit_pkt_t pkt;

    memset(&pkt, 0, sizeof(subcmd_bb_mhit_pkt_t));
    pkt.hdr.pkt_type = GAME_COMMAND0_TYPE;
    pkt.hdr.pkt_len = LE16(0x0010);
    pkt.shdr.type = SUBCMD60_HIT_MONSTER;
    pkt.shdr.size = 0x03;
    pkt.shdr.enemy_id = LE16(enemy_id2);
    pkt.enemy_id2 = LE16(enemy_id);
    pkt.damage = LE16(damage);
    pkt.flags = LE32(SWAP32(flags));

    return send_pkt_bb(c, (bb_pkt_hdr_t*)&pkt);
}

int send_lobby_mhit(lobby_t* l, ship_client_t* c,
    uint16_t enemy_id, uint16_t enemy_id2,
    uint16_t damage, uint32_t flags) {
    int i;

    /* Send the packet to every connected client. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {

            /* Call the appropriate function. */
            switch (l->clients[i]->version) {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
            case CLIENT_VERSION_PC:
            case CLIENT_VERSION_XBOX:
            case CLIENT_VERSION_EP3:
                send_mhit(l->clients[i], enemy_id, enemy_id2, damage, flags);
                break;

            case CLIENT_VERSION_GC:
                send_mhit_gc(l->clients[i], enemy_id, enemy_id2, damage, flags);
                break;

            case CLIENT_VERSION_BB:
                send_mhit_bb(l->clients[i], enemy_id, enemy_id2, damage, flags);
                break;
            }
        }
    }

    return 0;
}

/* 构建公会完整数据包 */
uint8_t* build_guild_full_data_pkt(ship_client_t* c) {
    uint8_t* sendbuf = get_sendbuf(); 
    //bb_full_guild_data_pkt* pkt = (bb_full_guild_data_pkt*)sendbuf;
    //int len = sizeof(bb_full_guild_data_pkt);

    //memset(pkt, 0, len);

    ///* 填充数据头 */
    //pkt->hdr.pkt_len = LE16(0x0864);
    //pkt->hdr.pkt_type = BB_GUILD_FULL_DATA;
    //pkt->hdr.flags = LE32(0x00000001);

    ///* 填充剩余数据 */
    //pkt->guild_owner_gc = c->guildcard;
    //pkt->guild_id = c->bb_guild->guild_id;
    //memcpy(&pkt->guild_info[0], &c->bb_guild->guild_info[0], sizeof(pkt->guild_info));
    //pkt->guild_priv_level = c->bb_guild->guild_priv_level;
    //memcpy(&pkt->guild_name[0], &c->bb_guild->guild_name[0], sizeof(pkt->guild_name));
    //pkt->guild_rank = c->bb_guild->guild_rank;
    //pkt->target_guildcard = c->guildcard;
    //pkt->client_id = c->client_id;
    //memcpy(&pkt->char_name[0], &c->bb_pl->character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
    //pkt->guild_dress_rewards = c->bb_guild->guild_rewards;
    //memcpy(&pkt->guild_flag[0], &c->bb_guild->guild_flag[0], sizeof(pkt->guild_flag));

    //return (uint8_t*)pkt;

    sprintf(&sendbuf[0x00], "\x64\x08\xEA\x15\x01");
    memset(&sendbuf[0x05], 0, 3);

    *(unsigned*)&sendbuf[0x08] = c->guildcard;
    *(unsigned*)&sendbuf[0x0C] = c->bb_guild->guild_id;
    *(unsigned*)&sendbuf[0x18] = c->bb_guild->guild_priv_level;
    memcpy(&sendbuf[0x1C], &c->bb_guild->guild_name[0], sizeof(c->bb_guild->guild_name));
    sprintf(&sendbuf[0x38], "\x84\x6C\x98");
    *(unsigned*)&sendbuf[0x3C] = c->guildcard;
    sendbuf[0x40] = c->client_id;
    memcpy(&sendbuf[0x44], &c->bb_pl->character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
    memcpy(&sendbuf[0x64], &c->bb_guild->guild_flag[0], sizeof(c->bb_guild->guild_flag));
    return &sendbuf[0];
}

/* 用于 0x00EA BB公会 指令*/
int send_bb_guild_cmd(ship_client_t* c, uint16_t cmd_code) {
    uint8_t* sendbuf = get_sendbuf();
    bb_guild_pkt_pkt* pkt = (bb_guild_pkt_pkt*)sendbuf;
    lobby_t* l = c->cur_lobby;
    ship_client_t* c2;
    int i;
    uint32_t len = 0, num = 0;

    /* Make sure we got the sendbuf */
    if (!sendbuf)
        return -1;

    if (!l)
        return 0;

    if (c->version != CLIENT_VERSION_BB)
        return 0;

    /* Clear the packet */
    memset(pkt, 0, sizeof(bb_guild_pkt_pkt));

#ifdef DEBUG
    DBG_LOG("send_bb_guild_cmd 向GC %u 发送指令 0x%04X", c->guildcard, cmd_code);
#endif // DEBUG

    switch (cmd_code)
    {
        /* 02EA */
    case BB_GUILD_UNK_02EA:
        pkt->hdr.pkt_len = LE16(0x0008);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 04EA */
    case BB_GUILD_UNK_04EA:
        pkt->hdr.pkt_len = LE16(0x0008);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 0EEA */
    case BB_GUILD_UNK_0EEA:
        len = 0x0830 + sizeof(bb_pkt_hdr_t);

        memset(&pkt->data[0x00], 0, len);

        *(uint32_t*)&pkt->data[0x00] = c->bb_guild->guild_owner_gc;
        *(uint32_t*)&pkt->data[0x04] = c->bb_guild->guild_id;
        *(uint32_t*)&pkt->data[0x08] = c->bb_guild->guild_points_rank;
        *(uint32_t*)&pkt->data[0x0C] = c->bb_guild->guild_points_rest;
        memcpy(&pkt->data[0x10], &c->bb_guild->guild_name[0], sizeof(c->bb_guild->guild_name));
        *(uint32_t*)&pkt->data[0x2C] = c->bb_guild->guild_rank;
        memcpy(&pkt->data[0x30], &c->bb_guild->guild_flag[0], 0x800);

        pkt->hdr.pkt_len = LE16(len);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 10EA */
    case BB_GUILD_DISSOLVE:

        pkt->hdr.pkt_len = LE16(sizeof(bb_pkt_hdr_t));
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 11EA */
    case BB_GUILD_MEMBER_PROMOTE:

        pkt->hdr.pkt_len = LE16(sizeof(bb_pkt_hdr_t));
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 12EA */
    case BB_GUILD_INITIALIZATION_DATA:

        memset(&pkt->data[0x00], 0, 0x38);

        *(uint32_t*)&pkt->data[0x04] = c->bb_guild->guild_owner_gc;
        *(uint32_t*)&pkt->data[0x08] = c->bb_guild->guild_id;
        *(uint32_t*)&pkt->data[0x0C] = c->bb_guild->guild_points_rank;
        *(uint32_t*)&pkt->data[0x10] = c->bb_guild->guild_points_rest;
        *(uint32_t*)&pkt->data[0x14] = c->bb_guild->guild_priv_level;
        memcpy(&pkt->data[0x18], &c->bb_guild->guild_name[0], sizeof(c->bb_guild->guild_name));
        *(uint32_t*)&pkt->data[0x34] = c->bb_guild->guild_rank;

        pkt->hdr.pkt_len = LE16(0x0038 + sizeof(bb_pkt_hdr_t));//64 - 8 = 56
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 13EA */
    case BB_GUILD_LOBBY_SETTING:

        for (i = 0; i < l->max_clients; i++) {
            if ((l->clients_slot[i]) && (l->clients[i]) && (l->clients[i]->version >= CLIENT_VERSION_GC)) {
                c2 = l->clients[i];

                *(uint32_t*)&pkt->data[len] = c2->bb_guild->guild_owner_gc;
                len += 4;
                *(uint32_t*)&pkt->data[len] = c2->bb_guild->guild_id;
                len += 4;
                if (c2->bb_guild->guild_id != 0) {
                    *(uint32_t*)&pkt->data[len] = c2->bb_guild->guild_points_rank;
                    len += 4;
                    *(uint32_t*)&pkt->data[len] = c2->bb_guild->guild_points_rest;
                    len += 4;
                }
                else {
                    memset(&pkt->data[len], 0, 8);
                    len += 8;
                }
                pkt->data[len] = (uint8_t)c2->bb_guild->guild_priv_level;
                len += 4;
                memcpy(&pkt->data[len], &c2->bb_guild->guild_name[0], 28);
                len += 28;
                if (c2->bb_guild->guild_id != 0) {
                    *(uint32_t*)&pkt->data[len] = c2->bb_guild->guild_rank;
                } else {
                    memset(&pkt->data[len], 0, 4);
                }
                len += 4;
                *(uint32_t*)&pkt->data[len] = c2->guildcard;
                len += 4;
                *(uint32_t*)&pkt->data[len] = c2->client_id;
                len += 4;
                memcpy(&pkt->data[len], &c2->bb_pl->character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
                len += 24;
                memset(&pkt->data[len], 0, 8);
                len += 8;
                memcpy(&pkt->data[len], &c2->bb_guild->guild_flag[0], 0x800);
                len += 0x800;
                num++;
            }
        }

        pkt->hdr.pkt_len = LE16(len + sizeof(bb_pkt_hdr_t));
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = LE32(num);

        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 14EA */
    case BB_GUILD_MEMBER_TITLE:

        pkt->hdr.pkt_len = LE16(0x0008);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 15EA */
        /* 构建完整公会数据包 并发送*/
    case BB_GUILD_FULL_DATA:
        return send_bb_guild_data(c, NULL);

        /* 18EA 公会点数情报 */
    case BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO:

        bb_guild_buy_privilege_and_point_info_pkt* info = (bb_guild_buy_privilege_and_point_info_pkt*)sendbuf;

        len = 0x100;

        /* 初始化数据包 */
        memset(info, 0, len);

        len = 0, num = 0;

        /* 填充菜单实例 */
        //for (i = 0; i < l->max_clients; ++i) {
        //    if ((l->clients_slot[i]) && (l->clients[i]) && (l->clients[i]->version >= CLIENT_VERSION_GC)) {
        //        if (l->clients[i]->bb_guild->guild_id == c->bb_guild->guild_id) {
        //            c2 = l->clients[i];
        //            num++;

        //            info->entries[i].guild_rank_points = 0x11111111;
        //            sprintf(&info->entries[i].unk_data2[0], "\x02\x02\x02\x02");
        //            info->entries[i].guild_points_rest = 0x22222222;
        //            info->entries[i].guild_member_num = num;
        //            info->entries[i].leaderboard_index = num;
        //            info->entries[i].guild_priv_level = c2->bb_guild->guild_priv_level;
        //            info->entries[i].guildcard_client = c2->guildcard;
        //            memcpy(&info->entries[i].char_name[0], &c2->bb_pl->character.name[0], BB_CHARACTER_NAME_LENGTH * 2);
        //            sprintf(&info->entries[i].unk_data3[0], "\x03\x03\x03\x03");
        //            sprintf(&info->entries[i].unk_data4[0], "\x04\x04\x04\x04");

        //            info->entries[i].personal_donation_points = 0x33333333;
        //            sprintf(&info->entries[i].unk_data5[0], "\x05\x05\x05\x05");

        //            len += 0x44;
        //        }
        //    }
        //}


        //pkt->data[0x0C] = 0x01;
        //pkt->data[0x10] = 0x01;
        //*(uint32_t*)&pkt->data[0x14] = c->bb_guild->guild_priv_level;
        //*(uint32_t*)&pkt->data[0x18] = c->guildcard;
        //memcpy(&pkt->data[0x1C], &c->bb_pl->character.name[0], BB_CHARACTER_NAME_LENGTH * 2);
        //pkt->data[0x40] = 0x02;

        info->hdr.pkt_len = LE16(len + sizeof(bb_pkt_hdr_t));
        //pkt->hdr.pkt_len = LE16(0x004C); 76 - 8 = 68 单个玩家
        info->hdr.pkt_type = cmd_code;
        //pkt->hdr.flags = 0x00000000;
        info->hdr.flags = LE32(num); /* flag一般都是菜单数量 */


        display_packet((uint8_t*)info, info->hdr.pkt_len);

        return send_pkt_bb(c, (bb_pkt_hdr_t*)info);

        /* 19EA */
    case BB_GUILD_PRIVILEGE_LIST:

        memset(&pkt->data[0x00], 0, 0x04);

        pkt->hdr.pkt_len = LE16(0x000C);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 1AEA */
    case BB_GUILD_BUY_SPECIAL_ITEM:
        //len = 0x00;

        ///* 物品数量 */
        //*(uint32_t*)&pkt->data[len] = 2;
        //len += 4;

        ///* 物品名称 暂未查明最大字符数 但是已经到头了 */
        //sprintf(&pkt->data[len], "\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B");
        //len += 16;
        //sprintf(&pkt->data[len], "\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B");
        //len += 16;
        //sprintf(&pkt->data[len], "\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B");
        //len += 16;

        //*(uint32_t*)&pkt->data[len] = 0;
        //len += 4;

        ///* 物品名称 暂未查明最大字符数 但是已经到头了 */
        //sprintf(&pkt->data[len], "\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B");
        //len += 16;
        //sprintf(&pkt->data[len], "\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B");
        //len += 16;
        //sprintf(&pkt->data[len], "\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B\x4B\x6D\xD5\x8B");
        //len += 16;

        //pkt->hdr.pkt_len = LE16(len + sizeof(bb_pkt_hdr_t));
        //pkt->hdr.pkt_type = cmd_code;
        //pkt->hdr.flags = 0x00000000;

        bb_guild_buy_special_item_pkt* spec_item_list = (bb_guild_buy_special_item_pkt*)sendbuf;
        len = 0x100;

        /* 初始化数据包 */
        memset(spec_item_list, 0, len);

        size_t entries_size = sizeof(bb_guild_special_item_list_t);

        len = 0;

        spec_item_list->item_num = 4;

        /* 填充菜单实例 */
        for (i = 0; i < spec_item_list->item_num; ++i) {
            istrncpy(ic_gbk_to_utf16, (char*)spec_item_list->entries[i].item_name, pso_guild_rank_list_bb[i]->name, sizeof(spec_item_list->entries[i].item_name));
            istrncpy(ic_gbk_to_utf16, (char*)spec_item_list->entries[i].item_desc, "Bronze Pen1111111111111111", sizeof(spec_item_list->entries[i].item_desc));
            spec_item_list->entries[i].price = 123;
            len += entries_size;
        }

        /* 填充数据头 */
        spec_item_list->hdr.pkt_len = LE16(len + sizeof(bb_pkt_hdr_t));
        spec_item_list->hdr.pkt_type = cmd_code;
        spec_item_list->hdr.flags = i - 1;

        display_packet((uint8_t*)spec_item_list, spec_item_list->hdr.pkt_len);

        return send_pkt_bb(c, (bb_pkt_hdr_t*)spec_item_list);

        /* 1CEA */
    case BB_GUILD_RANKING_LIST:
        bb_guild_rank_list_pkt* menu = (bb_guild_rank_list_pkt*)sendbuf;
        len = 0x100;

        /* 初始化数据包 */
        memset(menu, 0, len);

        len = 0;

        /* 填充菜单实例 */
        for (i = 0; i < _countof(pso_guild_rank_list_bb); ++i) {
            menu->entries[i].menu_id = LE32(pso_guild_rank_list_bb[i]->menu_id);
            menu->entries[i].item_id = LE32(pso_guild_rank_list_bb[i]->item_id);
            menu->entries[i].flags = LE16(pso_guild_rank_list_bb[i]->flag);
            istrncpy(ic_gbk_to_utf16, (char*)menu->entries[i].name, pso_guild_rank_list_bb[i]->name, 0x20);
            len += 0x2C;
        }

        /* 填充数据头 */
        menu->hdr.pkt_len = LE16(len + sizeof(bb_pkt_hdr_t));
        menu->hdr.pkt_type = LE16(BLOCK_LIST_TYPE);
        menu->hdr.flags = i - 1;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)menu);

        /* 1DEA */
    case BB_GUILD_UNK_1DEA:

        pkt->hdr.pkt_len = LE16(0x0008);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 1EEA */
    case BB_GUILD_UNK_1EEA:
        pkt->hdr.pkt_len = LE16(0x0008);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 1FEA */
    case BB_GUILD_UNK_1FEA:
        pkt->hdr.pkt_len = LE16(0x0008);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

        /* 20EA */
    case BB_GUILD_UNK_20EA:
        pkt->hdr.pkt_len = LE16(0x0008);
        pkt->hdr.pkt_type = cmd_code;
        pkt->hdr.flags = 0x00000000;

        /* 加密并发送 */
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
    }

    return 0;
}

int send_dc_confirm_update_quest_statistics(ship_client_t* c, uint16_t request_token) {
    dc_confirm_update_quest_statistics_pkt pkt;

    memset(&pkt, 0, sizeof(dc_confirm_update_quest_statistics_pkt));

    /* 填充数据头并准备发送 */
    pkt.hdr.pkt_type = QUEST_CONFIRM_STATS_TYPE;
    pkt.hdr.flags = 0x00;
    pkt.hdr.pkt_len = LE16(0x000C);

    /* 填充剩余数据 */
    pkt.unknown_a1 = LE16(0x0000);
    pkt.unknown_a2 = LE16(0x0000);
    pkt.request_token = LE16(request_token);
    pkt.unknown_a3 = LE16(0xBFFF);

    return send_pkt_dc(c, (dc_pkt_hdr_t*)&pkt);
}

int send_bb_confirm_update_quest_statistics(ship_client_t* c, uint16_t request_token) {
    bb_confirm_update_quest_statistics_pkt pkt;
    uint16_t pkt_size = sizeof(bb_confirm_update_quest_statistics_pkt);

    memset(&pkt, 0, pkt_size);

    /* 填充数据头并准备发送 */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = QUEST_CONFIRM_STATS_TYPE;
    pkt.hdr.flags = LE16(0x0000);

    /* 填充剩余数据 */
    pkt.unknown_a1 = LE16(0x0000);
    pkt.unknown_a2 = LE16(0x0000);
    pkt.request_token = LE16(request_token);
    pkt.unknown_a3 = LE16(0xBFFF);

    return send_pkt_bb(c, (bb_pkt_hdr_t*)&pkt);
}

int send_bb_quest_data1(ship_client_t* c, psocn_quest_data1_t* quest_data1) {
    uint8_t* sendbuf = get_sendbuf();
    subcmd_bb_send_quest_data1_t* pkt = (subcmd_bb_send_quest_data1_t*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    memset(pkt, 0, sizeof(subcmd_bb_send_quest_data1_t));

    /* 填充数据头并准备发送 */
    pkt->hdr.pkt_len = LE16(0x0210);
    pkt->hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt->hdr.flags = 0;

    /* 填充子数据头并准备发送 */
    pkt->shdr.type = SUBCMD60_QUEST_DATA1;
    pkt->shdr.size = 0x84;
    pkt->shdr.unused = 0;

    /* 填充剩余数据 */
    pkt->quest_data1.quest_guildcard = c->guildcard;
    memcpy(&pkt->quest_data1.data, &quest_data1->data, sizeof(quest_data1->data));
    pkt->quest_data1.quest_flags = 0;

    return crypt_send(c, 0x0210, sendbuf);
}

/* 用于查询玩家是否在线 指令*/
int check_gc_online(ship_client_t* c, uint32_t target_gc) {
    uint32_t i;
    ship_client_t* it;
    int done = 0, rv = 0;

    /* 不允许查询预留的GC号. */
    if (target_gc < 1000)
        return 0;

    /* 首先搜索本地的舰船. */
    for (i = 0; i < ship->cfg->blocks && !done; ++i) {
        if (!ship->blocks[i] || !ship->blocks[i]->run) {
            continue;
        }

        /* 查看该舰仓的所有客户端. */
        TAILQ_FOREACH(it, ship->blocks[i]->clients, qentry) {
            /* Check if this is the target and the target has player
               data. */
            DBG_LOG("查询 GC %u 目标 GC %u", it->guildcard, target_gc);

            if (it->guildcard == target_gc && it->pl) {

                rv = 1;
                done = 1;
            }
            else if (it->guildcard == target_gc) {
                /* 如果只查询到GC却未查询到玩家数据,则查询成功并返回0. */
                rv = 0;
                done = 1;
            }

            if (done)
                break;
        }
    }

    return rv;
}

/* 发送剩余任务状态 */
int send_bb_quest_state(ship_client_t* c) {
    bb_send_quest_state_pkt pkt = { 0 };

    /* 填充数据头并准备发送 */
    pkt.hdr.pkt_len = sizeof(bb_send_quest_state_pkt);
    pkt.hdr.pkt_type = BB_SEND_QUEST_STATE;
    pkt.hdr.flags = c->client_id;

    return send_pkt_bb(c, (bb_pkt_hdr_t*)&pkt);
}

/* 物品兑换完成 */
int send_bb_ex_item_done(ship_client_t* c, uint32_t done) {
    bb_item_exchange_done_pkt pkt = { 0 };

    /* 填充数据并准备发送 */
    pkt.hdr.pkt_len = LE16(sizeof(bb_item_exchange_done_pkt));
    pkt.hdr.pkt_type = LE16(ITEM_EXCHANGE_DONE);
    pkt.hdr.flags = done;

    return send_pkt_bb(c, (bb_pkt_hdr_t*)&pkt);
}

int send_bb_guild_data(ship_client_t* c, ship_client_t* nosend) {
    lobby_t* l = c->cur_lobby;
    int  i = 0, rv = 0;

    /* 如果客户端不在大厅或者队伍中则忽略数据包. */
    if (!l) {
        ERR_LOG("玩家GC %u 未处于大厅中", c->guildcard);
        return rv;
    }

    /* 将房间中的玩家公会数据发送至新进入的客户端 */
    for (i = 0; i < l->max_clients; ++i) {
        if ((l->clients_slot[i]) && 
            (l->clients[i]) &&
            (l->clients[i] != nosend) &&
            (l->clients[i]->version == CLIENT_VERSION_BB)
            ) {
            rv = send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)build_guild_full_data_pkt(c));
            rv = send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)build_guild_full_data_pkt(l->clients[i]));
            rv = send_pkt_bb(c, (bb_pkt_hdr_t*)build_guild_full_data_pkt(l->clients[i]));
        }
    }

    return rv;
}

