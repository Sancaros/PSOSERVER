/*
    梦幻之星中国 船闸服务器
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <time.h>
#include <ctype.h>
#include <errno.h>
//#include <iconv.h>
#include <f_iconv.h>
#include <WinSock_Defines.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include <zlib.h>

#include <debug.h>
#include <database.h>
#include <f_logs.h>
#include <mtwist.h>
#include <md5.h>

#ifdef ENABLE_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif

#include "ship.h"
#include "shipgate.h"

#define CLIENT_PRIV_LOCAL_GM    0x00000001
#define CLIENT_PRIV_GLOBAL_GM   0x00000002
#define CLIENT_PRIV_LOCAL_ROOT  0x00000004
#define CLIENT_PRIV_GLOBAL_ROOT 0x00000008

#define VERSION_DC              0
#define VERSION_PC              1
#define VERSION_GC              2
#define VERSION_EP3             3
#define VERSION_BB              4
#define VERSION_XBOX            5

/* 数据库连接 */
extern psocn_dbconn_t conn;

/* GnuTLS 加密数据交换... */
extern gnutls_anon_server_credentials_t anoncred;
extern gnutls_priority_t tls_prio;

/* Events... */
extern uint32_t event_count;
extern monster_event_t* events;

/* Scripts */
extern uint32_t script_count;
extern ship_script_t* scripts;

static uint8_t recvbuf[65536];

/* 公会相关 */
static uint8_t default_guild_flag[2048];
static uint8_t default_guild_flag_slashes[4098];

/* Find a ship by its id */
static ship_t* find_ship(uint16_t id) {
    ship_t* i;

    TAILQ_FOREACH(i, &ships, qentry) {
        if (i->key_idx == id) {
            return i;
        }
    }

    return NULL;
}

static inline void pack_ipv6(struct in6_addr* addr, uint64_t* hi,
    uint64_t* lo) {
    *hi = ((uint64_t)addr->s6_addr[0] << 56) |
        ((uint64_t)addr->s6_addr[1] << 48) |
        ((uint64_t)addr->s6_addr[2] << 40) |
        ((uint64_t)addr->s6_addr[3] << 32) |
        ((uint64_t)addr->s6_addr[4] << 24) |
        ((uint64_t)addr->s6_addr[5] << 16) |
        ((uint64_t)addr->s6_addr[6] << 8) |
        ((uint64_t)addr->s6_addr[7]);
    *lo = ((uint64_t)addr->s6_addr[8] << 56) |
        ((uint64_t)addr->s6_addr[9] << 48) |
        ((uint64_t)addr->s6_addr[10] << 40) |
        ((uint64_t)addr->s6_addr[11] << 32) |
        ((uint64_t)addr->s6_addr[12] << 24) |
        ((uint64_t)addr->s6_addr[13] << 16) |
        ((uint64_t)addr->s6_addr[14] << 8) |
        ((uint64_t)addr->s6_addr[15]);
}

static inline void parse_ipv6(uint64_t hi, uint64_t lo, uint8_t buf[16]) {
    buf[0] = (uint8_t)(hi >> 56);
    buf[1] = (uint8_t)(hi >> 48);
    buf[2] = (uint8_t)(hi >> 40);
    buf[3] = (uint8_t)(hi >> 32);
    buf[4] = (uint8_t)(hi >> 24);
    buf[5] = (uint8_t)(hi >> 16);
    buf[6] = (uint8_t)(hi >> 8);
    buf[7] = (uint8_t)hi;
    buf[8] = (uint8_t)(lo >> 56);
    buf[9] = (uint8_t)(lo >> 48);
    buf[10] = (uint8_t)(lo >> 40);
    buf[11] = (uint8_t)(lo >> 32);
    buf[12] = (uint8_t)(lo >> 24);
    buf[13] = (uint8_t)(lo >> 16);
    buf[14] = (uint8_t)(lo >> 8);
    buf[15] = (uint8_t)lo;
}

/* Check if a user is already online. */
int is_gc_online(uint32_t gc) {
    char query[256];
    void* result;
    char** row;

    /* Fill in the query. */
    sprintf(query, "SELECT guildcard FROM %s WHERE guildcard='%u'", SERVER_CLIENTS_ONLINE,
        (unsigned int)gc);

    /* If we can't query the database, fail. */
    if (psocn_db_real_query(&conn, query)) {
        return -1;
    }

    /* Grab the results. */
    result = psocn_db_result_store(&conn);
    if (!result) {
        return -1;
    }

    row = psocn_db_result_fetch(result);
    if (!row) {
        psocn_db_result_free(result);
        return 0;
    }

    /* If there is a result, then the user is already online. */
    psocn_db_result_free(result);
    return 1;
}

static monster_event_t* find_current_event(uint8_t difficulty, uint8_t ver,
    int any) {
    uint32_t i;
    uint8_t questing;
    time_t now;

    /* For now, just reject any challenge/battle mode updates... We don't care
       about them at all. */
    if (ver & 0xC0)
        return NULL;

    now = time(NULL);
    questing = ver & 0x20;
    ver &= 0x07;

    for (i = 0; i < event_count; ++i) {
        /* Skip all quests that don't meet the requirements passed in. */
        if (now > events[i].end_time || now < events[i].start_time)
            continue;

        if (!any) {
            if (!(events[i].versions & (1 << ver)))
                continue;
            if (!(events[i].difficulties & (1 << difficulty)))
                continue;
            if (!events[i].allow_quests && questing)
                continue;
        }

        /* If we get here, then the event is valid, return it. */
        return events + i;
    }

    return NULL;
}

static ship_script_t* find_script(const char* fn, int module) {
#ifdef ENABLE_LUA
    uint32_t i;

    /* Go through the list looking for a match. */
    for (i = 0; i < script_count; ++i) {
        if (scripts[i].module == module &&
            !strcmp(scripts[i].remote_fn, fn))
            return scripts + i;
    }
#else
    (void)fn;
    (void)module;
#endif

    return NULL;
}

/* Returns non-zero if the specified access should be blocked */
static int check_user_blocklist(uint32_t searcher, uint32_t guildcard,
    uint32_t flags) {
    char query[512];
    uint32_t read_flags;
    int rv = 0;
    void* result;
    char** row;

    sprintf(query, "SELECT flags FROM %s NATURAL JOIN %s "
            "WHERE %s.blocked_gc='%u' AND guildcard='%u'"
        , SERVER_BLOCKS_LIST, CLIENTS_BLUEBURST
        , SERVER_BLOCKS_LIST, searcher, guildcard);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("屏蔽列表错误: %s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法获取屏蔽列表结果: %s",
            psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result))) {
        /* There's a hit on the blocklist, check if they're blocking search. */
        errno = 0;
        read_flags = (uint32_t)strtoul(row[0], NULL, 0);

        if (!errno && (read_flags & flags) == flags)
            rv = 1;
    }

    /* Clean up and return what we got */
    psocn_db_result_free(result);
    return rv;
}

static size_t my_strnlen(const uint8_t* str, size_t len) {
    size_t rv = 0;

    while (*str++ && ++rv < len);
    return rv;
}

/* 创建一个新的协议连接, 认证后并将其列入舰船清单. */
ship_t* create_connection_tls(int sock, struct sockaddr* addr, socklen_t size) {
    ship_t* rv;
    int tmp;
    size_t sz = 20;
    char query[256];
    //uint8_t rc4key[128];
    void* result;
    char** row;

    rv = (ship_t*)malloc(sizeof(ship_t));

    if (!rv) {
        perror("malloc");
        closesocket(sock);
        return NULL;
    }

    memset(rv, 0, sizeof(ship_t));

    /* Store basic parameters in the client structure. */
    rv->sock = sock;
    rv->last_message = time(NULL);
    memcpy(&rv->conn_addr, addr, size);

    /* Create the TLS session */
    gnutls_init(&rv->session, GNUTLS_SERVER);

    gnutls_priority_set(rv->session, tls_prio);

    tmp = gnutls_credentials_set(rv->session, GNUTLS_CRD_ANON, anoncred);

    if (tmp < 0) {
        ERR_LOG("TLS 匿名凭据错误: %s", gnutls_strerror(tmp));
        goto err_tls;
    }

#if (SIZEOF_INT != SIZEOF_VOIDP) && (SIZEOF_LONG_INT == SIZEOF_VOIDP)
    gnutls_transport_set_ptr(rv->session, (gnutls_transport_ptr_t)((long)sock));
#else
    gnutls_transport_set_ptr(rv->session, (gnutls_transport_ptr_t)sock);
    //gnutls_transport_set_int(rv->session, sock);
#endif

    /* Do the TLS handshake */
    do {
        tmp = gnutls_handshake(rv->session);
    } while (tmp < 0 && gnutls_error_is_fatal(tmp) == 0);

    if (tmp < 0) {
        ERR_LOG("GNUTLS *** 注意: TLS 握手失败 %s", gnutls_strerror(tmp));
        goto err_hs;
    }
    //else {
    //    SGATE_LOG("GNUTLS *** TLS 握手成功");

    //    char* desc;
    //    desc = gnutls_session_get_desc(rv->session);
    //    SGATE_LOG("GNUTLS *** Session 信息: %d - %s", rv->sock, desc);
    //    gnutls_free(desc);
    //}

    sprintf(query, "SELECT idx FROM %s WHERE idx='%d'"
        , SERVER_SHIPS_DATA, 1);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法查询舰船密钥数据库");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto err_cert;
    }

    if ((result = psocn_db_result_store(&conn)) == NULL ||
        (row = psocn_db_result_fetch(result)) == NULL) {
        SQLERR_LOG("未知舰船密钥");
        goto err_cert;
    }

    /* Store the ship ID */
    rv->key_idx = atoi(row[0]);
    psocn_db_result_free(result);
    //gnutls_x509_crt_deinit(cert);

    /* Send the client the welcome packet, or die trying. */
    if (send_welcome(rv)) {
        ERR_LOG("send_welcome *** 警告: %d",
            rv);
        goto err_cert;
    }

    /* Insert it at the end of our list, and we're done. */
    TAILQ_INSERT_TAIL(&ships, rv, qentry);
    return rv;

err_hs:
    closesocket(sock);
    gnutls_deinit(rv->session);
    free_safe(rv);
    return NULL;
err_tls:
    gnutls_bye(rv->session, GNUTLS_SHUT_RDWR);
    closesocket(sock);
    gnutls_deinit(rv->session);
    free_safe(rv);
    return NULL;
err_cert:
    gnutls_bye(rv->session, GNUTLS_SHUT_RDWR);
    closesocket(sock);
    gnutls_deinit(rv->session);
    free_safe(rv);
    return NULL;
}

void db_remove_client(ship_t* c)
{
    char query[256];
    ship_t* i;

    TAILQ_REMOVE(&ships, c, qentry);

    if (c->key_idx) {
        /* Send a status packet to everyone telling them its gone away
         向每个人发送状态数据包，告诉他们它已经消失了
        */
        TAILQ_FOREACH(i, &ships, qentry) {
            send_ship_status(i, c, 0);
        }

        /* Remove the ship from the online_ships table. */
        sprintf(query, "DELETE FROM %s WHERE ship_id='%hu'"
            , SERVER_SHIPS_ONLINE, c->key_idx);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法清理在线舰船数据库表 %s",
                c->name);
        }

        /* Remove any clients in the online_clients table on that ship */
        sprintf(query, "DELETE FROM %s WHERE ship_id='%hu'"
            , SERVER_CLIENTS_ONLINE, c->key_idx);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法清理在线玩家数据库表 %s", c->name);
        }

        /* Remove any clients in the transient_clients table on that ship */
        sprintf(query, "DELETE FROM %s WHERE ship_id='%hu'"
            , SERVER_CLIENTS_TRANSIENT, c->key_idx);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法清理临时玩家数据库表 %s", c->name);
        }
    }
}

/* Destroy a connection, closing the socket and removing it from the list. */
void destroy_connection(ship_t* c)
{
    if (c->name[0]) {
        SGATE_LOG("关闭与 %s 的连接", c->name);
    }
    else {
        SGATE_LOG("取消与未认证舰船的连接");
    }

    db_remove_client(c);

    /* Clean up the TLS resources and the socket. */
    if (c->sock >= 0) {
        gnutls_bye(c->session, GNUTLS_SHUT_RDWR);
        closesocket(c->sock);
        gnutls_deinit(c->session);
        c->sock = SOCKET_ERROR;
    }

    if (c->recvbuf) {
        free_safe(c->recvbuf);
    }

    if (c->sendbuf) {
        free_safe(c->sendbuf);
    }

    free_safe(c);
}

/* 处理 ship's login response. */
static int handle_shipgate_login6t(ship_t* c, shipgate_login6_reply_pkt* pkt) {
    char query[512];
    ship_t* j;
    void* result;
    char** row;
    uint32_t pver = c->proto_ver = ntohl(pkt->proto_ver);
    uint16_t menu_code = ntohs(pkt->menu_code);
    int ship_number;
    uint64_t ip6_hi, ip6_lo;
    uint32_t clients = 0;
#ifdef ENABLE_LUA
    uint32_t i;
#endif

    /* Check the protocol version for support */
    if (pver < SHIPGATE_MINIMUM_PROTO_VER || pver > SHIPGATE_MAXIMUM_PROTO_VER) {
        ERR_LOG("无效协议版本: %lu", pver);

        send_error(c, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_LOGIN_BAD_PROTO, NULL, 0);
        return -1;
    }

    /* Attempt to grab the key for this ship. */
    sprintf(query, "SELECT main_menu, ship_number FROM %s WHERE "
        "idx='%u'", SERVER_SHIPS_DATA, c->key_idx);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法查询舰船密钥数据库");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        send_error(c, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, NULL, 0);
        return -1;
    }

    if ((result = psocn_db_result_store(&conn)) == NULL ||
        (row = psocn_db_result_fetch(result)) == NULL) {
        SQLERR_LOG("无效密钥idx索引 %d", c->key_idx);
        send_error(c, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_LOGIN_BAD_KEY, NULL, 0);
        return -1;
    }

    /* Check the menu code for validity */
    if (menu_code && (!isalpha(menu_code & 0xFF) | !isalpha(menu_code >> 8))) {
        SQLERR_LOG("菜单代码错误 idx: %d", c->key_idx);
        send_error(c, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_LOGIN_BAD_MENU, NULL, 0);
        return -1;
    }

    /* If the ship requests the main menu and they aren't allowed there, bail */
    if (!menu_code && !atoi(row[0])) {
        SQLERR_LOG("菜单代码无效 id: %d", c->key_idx);
        send_error(c, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_LOGIN_INVAL_MENU, NULL, 0);
        return -1;
    }

    /* Grab the key from the result */
    ship_number = atoi(row[1]);
    psocn_db_result_free(result);

    /* Fill in the ship structure */
    c->remote_addr = pkt->ship_addr4;
    memcpy(&c->remote_addr6, pkt->ship_addr6, 16);
    c->port = ntohs(pkt->ship_port);
    c->clients = ntohs(pkt->clients);
    c->games = ntohs(pkt->games);
    c->flags = ntohl(pkt->flags);
    c->menu_code = menu_code;
    memcpy(c->name, pkt->name, 12);
    c->ship_number = ship_number;
    c->privileges = ntohl(pkt->privileges);

    pack_ipv6(&c->remote_addr6, &ip6_hi, &ip6_lo);

    //SHIPS_LOG("handle_shipgate_login6t c->privileges = %d", c->privileges);

    sprintf(query, "INSERT INTO %s(name, players, ip, port, int_ip, "
        "ship_id, gm_only, games, menu_code, flags, ship_number, "
        "ship_ip6_high, ship_ip6_low, protocol_ver, privileges) VALUES "
        "('%s', '%hu', '%lu', '%hu', '%u', '%u', '%d', '%hu', '%hu', '%u', "
        "'%d', '%llu', '%llu', '%u', '%u')", SERVER_SHIPS_ONLINE, c->name, c->clients,
        ntohl(c->remote_addr), c->port, 0, c->key_idx,
        !!(c->flags & LOGIN_FLAG_GMONLY), c->games, c->menu_code, c->flags,
        ship_number, (unsigned long long)ip6_hi,
        (unsigned long long)ip6_lo, pver, c->privileges);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法将 %s 新增至在线舰船列表.",
            c->name);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        send_error(c, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, NULL, 0);
        return -1;
    }

    /* Hooray for misusing functions! */
    if (send_error(c, SHDR_TYPE_LOGIN6, SHDR_RESPONSE, ERR_NO_ERROR, NULL, 0)) {
        ERR_LOG("滥用函数万岁!");
        return -1;
    }

#ifdef ENABLE_LUA
    /* Send script check packets, if the ship supports scripting */
    if (pver >= 16 && c->flags & LOGIN_FLAG_LUA) {
        for (i = 0; i < script_count; ++i) {
            send_script_check(c, &scripts[i]);
        }
    }
#endif

    /* Does this ship support the initial shipctl packets? If so, send it a
       uname and a version request 这艘船是否支持初始shipctl数据包？如果是，请向其发送uname和版本请求 . */
    if (pver >= 19) {
        if (send_sctl(c, SCTL_TYPE_UNAME, 0)) {
            ERR_LOG("滥用函数万岁!");
            return -1;
        }

        if (send_sctl(c, SCTL_TYPE_VERSION, 0)) {
            ERR_LOG("滥用函数万岁!");
            return -1;
        }
    }

    /* 向每艘船发送状态数据包. */
    TAILQ_FOREACH(j, &ships, qentry) {
        /* Don't send ships with privilege bits set to ships not running
           protocol v18 or newer 不发送特权位设置为未运行协议v18或更新版本的装运 . */
        if (!c->privileges || j->proto_ver >= 18)
            send_ship_status(j, c, 1);

        /*  把这艘船送到新船上去，只要不是在上面 . */
        if (j != c) {
            send_ship_status(c, j, 1);
        }

        clients += j->clients;
    }

    //SHIPS_LOG("handle_shipgate_login6t c->privileges = %d", c->privileges);
    /* Update the table of client counts, if it might have actually changed from
       this update packet. */
    if (c->clients) {
        sprintf(query, "INSERT INTO %s (clients) VALUES('%" PRIu32
            "') ON DUPLICATE KEY UPDATE clients=VALUES(clients)", SERVER_CLIENTS_COUNT, clients);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法更新全服 玩家/游戏 数量");
        }
    }

    SGATE_LOG("%s: 舰船与船闸完成对接", c->name);
    return 0;
}

/* 处理 ship's update counters packet. */
static int handle_count(ship_t* c, shipgate_cnt_pkt* pkt) {
    char query[256];
    ship_t* j;
    uint32_t clients = 0;
    uint16_t sclients = c->clients;

    c->clients = ntohs(pkt->clients);
    c->games = ntohs(pkt->games);

    sprintf(query, "UPDATE %s SET players='%hu', games='%hu' WHERE "
        "ship_id='%u'", SERVER_SHIPS_ONLINE, c->clients, c->games, c->key_idx);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法更新舰船 %s 玩家/游戏 数量", c->name);
        SQLERR_LOG("%s", psocn_db_error(&conn));
    }

    /* Update all of the ships */
    TAILQ_FOREACH(j, &ships, qentry) {
        send_counts(j, c->key_idx, c->clients, c->games);
        clients += j->clients;
    }

    /* Update the table of client counts, if the number actually changed from
       this update packet. */
    if (sclients != c->clients) {
        sprintf(query, "INSERT INTO %s (clients) VALUES('%" PRIu32
            "') ON DUPLICATE KEY UPDATE clients=VALUES(clients)", SERVER_CLIENTS_COUNT, clients);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法更新全服 玩家/游戏 数量");
        }
    }

    return 0;
}

static size_t strlen16(const uint16_t* str) {
    size_t sz = 0;

    while (*str++) ++sz;
    return sz;
}

static int save_mail(uint32_t gc, uint32_t from, void* pkt, int version) {
    char msg[512], name[64];
    static char query[2048];
    void* result;
    char** row;
    size_t in, out, nmlen;
    char* inptr;
    char* outptr;
    simple_mail_pkt* dcpkt = (simple_mail_pkt*)pkt;
    simple_mail_pkt* pcpkt = (simple_mail_pkt*)pkt;
    simple_mail_pkt* bbpkt = (simple_mail_pkt*)pkt;

    /* See if the user is registered first. */
    sprintf(query, "SELECT account_id FROM %s WHERE guildcard='%u'"
        , CLIENTS_GUILDCARDS, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("邮件: 无法查询到账户: %s",
            psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("邮件: 无法获取数据库结果: %s",
            psocn_db_error(&conn));
        return 0;
    }

    /* Grab the result set */
    if (!(row = psocn_db_result_fetch(result))) {
        SQLERR_LOG("邮件: 无效 guildcard: %u (数据来源 %u)",
            gc, from);
        psocn_db_result_free(result);
        return 0;
    }

    /* Is there an account_id associated with the guildcard? */
    if (row[0] == NULL) {
        /* No account associated with the guildcard, so we're done. */
        psocn_db_result_free(result);
        return 0;
    }

    /* We're done with the result set, so clean it up. */
    psocn_db_result_free(result);

    /* Convert the message to UTF-8 for storage. */
    switch (version) {
    case VERSION_DC:
    case VERSION_GC:
    case VERSION_EP3:
        dcpkt->dcmaildata.stuff[144] = 0;
        in = strlen(dcpkt->dcmaildata.stuff);
        out = 511;
        inptr = dcpkt->dcmaildata.stuff;
        outptr = msg;

        if (inptr[0] == '\t' && inptr[1] == 'J')
            iconv(ic_sjis_to_utf8, &inptr, &in, &outptr, &out);
        else
            iconv(ic_8859_to_utf8, &inptr, &in, &outptr, &out);

        msg[511 - out] = 0;
        memcpy(name, dcpkt->dcmaildata.dcname, 16);
        name[16] = 0;
        nmlen = strlen(name);
        break;

    case VERSION_PC:
        pcpkt->pcmaildata.stuff[288] = 0;
        pcpkt->pcmaildata.stuff[289] = 0;
        in = strlen16((uint16_t*)pcpkt->pcmaildata.stuff) * 2;
        out = 511;
        inptr = (char*)pcpkt->pcmaildata.stuff;
        outptr = msg;

        iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &out);

        msg[511 - out] = 0;

        in = (strlen16(pcpkt->pcmaildata.pcname) * 2);
        nmlen = 0x40;
        inptr = (char*)pcpkt->pcmaildata.pcname;
        outptr = name;

        iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &nmlen);
        nmlen = 64 - nmlen;
        name[nmlen] = 0;
        break;

    case VERSION_BB:
        bbpkt->bbmaildata.unk2[0] = 0;
        bbpkt->bbmaildata.unk2[1] = 0;
        in = strlen16(bbpkt->bbmaildata.message) * 2;
        out = 511;
        inptr = (char*)bbpkt->bbmaildata.message;
        outptr = msg;

        iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &out);

        msg[511 - out] = 0;

        in = (strlen16(bbpkt->bbmaildata.bbname) * 2) - 4;
        nmlen = 0x40;
        inptr = (char*)&bbpkt->bbmaildata.bbname[2];
        outptr = name;

        iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &nmlen);
        nmlen = 64 - nmlen;
        name[nmlen] = 0;
        break;

    default:
        /* XXXX */
        return 0;
    }

    /* Fill in the query. */
    sprintf(query, "INSERT INTO %s(recipient, sender, sent_time, "
        "sender_name, message) VALUES ('%u', '%u', UNIX_TIMESTAMP(), '", SERVER_SIMPLE_MAIL, gc,
        from);
    psocn_db_escape_str(&conn, query + strlen(query), name, nmlen);

    strcat(query, "', '");
    psocn_db_escape_str(&conn, query + strlen(query), msg, 511 - out);
    strcat(query, "');");

    /* Execute the query on the db. */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法保存 simple mail (送往: %" PRIu32 " 来自: %"
            PRIu32 ")", gc, from);
        SQLERR_LOG("    %s", psocn_db_error(&conn));
        return 0;
    }

    /* And... we're done! */
    return 0;
}

static int handle_dc_mail(ship_t* c, simple_mail_pkt* pkt) {
    uint32_t guildcard = LE32(pkt->dcmaildata.gc_dest);
    uint32_t sender = LE32(pkt->gc_sender);
    uint16_t ship_id;
    ship_t* s;

    /* See if the client being sent the mail has blocked the user sending it */
    if (check_user_blocklist(sender, guildcard, BLOCKLIST_MAIL))
        return 0;

    ship_id = db_get_char_ship_id(guildcard);

    if (ship_id < 0)
        return save_mail(guildcard, sender, pkt, VERSION_DC);

    /* If we've got this far, we should have the ship we need to send to */
    s = find_ship(ship_id);
    if (!s) {
        ERR_LOG("无效舰船?!?!");
        return 0;
    }

    /* 完成数据包设置,发送至舰船... */
    forward_dreamcast(s, (dc_pkt_hdr_t*)pkt, c->key_idx, 0, 0);
    return 0;
}

static int handle_pc_mail(ship_t* c, simple_mail_pkt* pkt) {
    uint32_t guildcard = LE32(pkt->pcmaildata.gc_dest);
    uint32_t sender = LE32(pkt->gc_sender);
    uint16_t ship_id;
    ship_t* s;

    /* See if the client being sent the mail has blocked the user sending it */
    if (check_user_blocklist(sender, guildcard, BLOCKLIST_MAIL))
        return 0;

    ship_id = db_get_char_ship_id(guildcard);

    if (ship_id < 0)
        return save_mail(guildcard, sender, pkt, VERSION_PC);

    /* If we've got this far, we should have the ship we need to send to */
    s = find_ship(ship_id);
    if (!s) {
        ERR_LOG("无效舰船?!?!?");
        return 0;
    }

    /* 完成数据包设置,发送至舰船... */
    forward_pc(s, (dc_pkt_hdr_t*)pkt, c->key_idx, 0, 0);
    return 0;
}

static int handle_bb_mail(ship_t* c, simple_mail_pkt* pkt) {
    uint32_t guildcard = LE32(pkt->bbmaildata.gc_dest);
    uint32_t sender = LE32(pkt->gc_sender);
    uint16_t ship_id;
    ship_t* s;

    /* See if the client being sent the mail has blocked the user sending it */
    if (check_user_blocklist(sender, guildcard, BLOCKLIST_MAIL))
        return 0;

    ship_id = db_get_char_ship_id(guildcard);

    if (ship_id < 0)
        return save_mail(guildcard, sender, pkt, VERSION_BB);

    /* If we've got this far, we should have the ship we need to send to */
    s = find_ship(ship_id);
    if (!s) {
        ERR_LOG("无效舰船?!?!?");
        return 0;
    }

    /* 完成数据包设置,发送至舰船... */
    forward_bb(s, (bb_pkt_hdr_t*)pkt, c->key_idx, 0, 0);
    return 0;
}

static int handle_guild_search(ship_t* c, dc_guild_search_pkt* pkt, uint32_t flags) {
    uint32_t guildcard = LE32(pkt->gc_target);
    uint32_t searcher = LE32(pkt->gc_search);
    char query[512];
    void* result;
    char** row;
    uint16_t ship_id, port;
    uint32_t lobby_id, ip, block, dlobby_id;
    uint64_t ip6_hi, ip6_lo;
    ship_t* s;
    dc_guild_reply_pkt reply;
    dc_guild_reply6_pkt reply6;
    char lobby_name[32], gname[17];

    /* See if the client being searched for has blocked the one doing the
       searching... */
    if (check_user_blocklist(searcher, guildcard, BLOCKLIST_GSEARCH))
        return 0;

    /* Figure out where the user requested is */
    sprintf(query, "SELECT %s.name, %s.ship_id, block, "
        "lobby, lobby_id, %s.name, ip, port, gm_only, "
        "ship_ip6_high, ship_ip6_low, dlobby_id FROM %s INNER "
        "JOIN %s ON %s.ship_id = "
        "%s.ship_id WHERE guildcard='%u'"
        , SERVER_CLIENTS_ONLINE, SERVER_CLIENTS_ONLINE
        , SERVER_SHIPS_ONLINE
        , SERVER_CLIENTS_ONLINE
        , SERVER_SHIPS_ONLINE, SERVER_CLIENTS_ONLINE
        , SERVER_SHIPS_ONLINE, guildcard
    );

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Guild Search Error: %s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't fetch Guild Search result: %s",
            psocn_db_error(&conn));
        return 0;
    }

    if (!(row = psocn_db_result_fetch(result))) {
        /* The user's not online, give up. */
        goto out;
    }

    /* Make sure the user isn't on a GM only ship... if they are, bail now */
    if (atoi(row[8])) {
        goto out;
    }

    /* Grab the ship we're looking at first */
    errno = 0;
    ship_id = (uint16_t)strtoul(row[1], NULL, 0);

    if (errno) {
        ERR_LOG("在舰船上分析GC时发生错误: %s", strerror(errno));
        goto out;
    }

    /* If we've got this far, we should have the ship we need to send to */
    s = find_ship(ship_id);
    if (!s) {
        ERR_LOG("无效舰船?!?!?!");
        goto out;
    }

    /* If any of these are NULL, the user is not in a lobby. Thus, the client
       doesn't really exist just yet. */
    if (row[4] == NULL || row[3] == NULL || row[11] == NULL) {
        goto out;
    }

    /* Grab the data from the result */
    port = (uint16_t)strtoul(row[7], NULL, 0);
    block = (uint32_t)strtoul(row[2], NULL, 0);
    lobby_id = (uint32_t)strtoul(row[4], NULL, 0);
    ip = (uint32_t)strtoul(row[6], NULL, 0);
    ip6_hi = (uint64_t)strtoull(row[9], NULL, 0);
    ip6_lo = (uint64_t)strtoull(row[10], NULL, 0);
    dlobby_id = (uint32_t)strtoul(row[11], NULL, 0);

    if (errno) {
        ERR_LOG("无法分析公会搜索: %s", strerror(errno));
        goto out;
    }

    if (dlobby_id <= 15) {
        sprintf(lobby_name, "BLOCK%02d-%02d", block, dlobby_id);
    }
    else {
        sprintf(lobby_name, "BLOCK%02d-C%d", block, dlobby_id - 15);
    }

    /* Set up the reply, we should have enough data now */
    if ((flags & FW_FLAG_PREFER_IPV6) && ip6_hi) {
        memset(&reply6, 0, DC_GUILD_REPLY6_LENGTH);

        /* Fill it in */
        reply6.hdr.pkt_type = GUILD_REPLY_TYPE;
        reply6.hdr.pkt_len = LE16(DC_GUILD_REPLY6_LENGTH);
        reply6.hdr.flags = 6;
        reply6.tag = LE32(0x00010000);
        reply6.gc_search = pkt->gc_search;
        reply6.gc_target = pkt->gc_target;
        parse_ipv6(ip6_hi, ip6_lo, reply6.ip);
        reply6.port = LE16((port + block * 6));

        reply6.menu_id = LE32(0xFFFFFFFF);
        reply6.item_id = LE32(dlobby_id);
        strcpy(reply6.name, row[0]);

        if (dlobby_id != lobby_id) {
            /* See if we need to truncate the team name */
            if (flags & FW_FLAG_IS_PSOPC) {
                if (row[3][0] == '\t') {
                    strncpy(gname, row[3], 14);
                    gname[14] = 0;
                }
                else {
                    strncpy(gname + 2, row[3], 12);
                    gname[0] = '\t';
                    gname[1] = 'E';
                    gname[14] = 0;
                }
            }
            else {
                if (row[3][0] == '\t') {
                    strncpy(gname, row[3], 16);
                    gname[16] = 0;
                }
                else {
                    strncpy(gname + 2, row[3], 14);
                    gname[0] = '\t';
                    gname[1] = 'E';
                    gname[16] = 0;
                }
            }

            sprintf(reply6.location, "%s,%s, ,%s", gname, lobby_name, row[5]);
        }
        else {
            sprintf(reply6.location, "%s, ,%s", lobby_name, row[5]);
        }

        /* Send it away */
        forward_dreamcast(c, (dc_pkt_hdr_t*)&reply6, c->key_idx, 0, 0);
    }
    else {
        memset(&reply, 0, DC_GUILD_REPLY_LENGTH);

        /* Fill it in */
        reply.hdr.pkt_type = GUILD_REPLY_TYPE;
        reply.hdr.pkt_len = LE16(DC_GUILD_REPLY_LENGTH);
        reply.tag = LE32(0x00010000);
        reply.gc_search = pkt->gc_search;
        reply.gc_target = pkt->gc_target;
        reply.ip = htonl(ip);
        reply.port = LE16((port + block * 6));

        reply.menu_id = LE32(0xFFFFFFFF);
        reply.item_id = LE32(dlobby_id);
        strcpy(reply.name, row[0]);

        if (dlobby_id != lobby_id) {
            /* See if we need to truncate the team name */
            if (flags & FW_FLAG_IS_PSOPC) {
                if (row[3][0] == '\t') {
                    strncpy(gname, row[3], 14);
                    gname[14] = 0;
                }
                else {
                    strncpy(gname + 2, row[3], 12);
                    gname[0] = '\t';
                    gname[1] = 'E';
                    gname[14] = 0;
                }
            }
            else {
                if (row[3][0] == '\t') {
                    strncpy(gname, row[3], 16);
                    gname[16] = 0;
                }
                else {
                    strncpy(gname + 2, row[3], 14);
                    gname[0] = '\t';
                    gname[1] = 'E';
                    gname[16] = 0;
                }
            }

            sprintf(reply.location, "%s,%s, ,%s", gname, lobby_name, row[5]);
        }
        else {
            sprintf(reply.location, "%s, ,%s", lobby_name, row[5]);
        }

        /* Send it away */
        forward_dreamcast(c, (dc_pkt_hdr_t*)&reply, c->key_idx, 0, 0);
    }

out:
    /* Finally, we're finished, clean up and return! */
    psocn_db_result_free(result);
    return 0;
}

static int handle_bb_guild_search(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_search_pkt* p = (bb_guild_search_pkt*)pkt->pkt;
    uint32_t guildcard = LE32(p->gc_target);
    uint32_t gc_sender = ntohl(pkt->guildcard);
    uint32_t b_sender = ntohl(pkt->block);
    char query[512];
    void* result;
    char** row;
    uint16_t ship_id, port;
    uint32_t lobby_id, ip, block, dlobby_id;
    /* uint64_t ip6_hi, ip6_lo; */
    ship_t* s;
    bb_guild_reply_pkt reply;
    size_t in, out;
    char* inptr;
    char* outptr;
    char lobby_name[32], gname[17];

    /* See if the client being searched for has blocked the one doing the
       searching... */
    if (check_user_blocklist(gc_sender, guildcard, BLOCKLIST_GSEARCH))
        return 0;

    if (!is_gc_online(guildcard))
        return 0;

    /* Figure out where the user requested is */
    sprintf(query, "SELECT %s.name, %s.ship_id, block, "
        "lobby, lobby_id, %s.name, ip, port, gm_only, "
        "ship_ip6_high, ship_ip6_low, dlobby_id FROM %s INNER "
        "JOIN %s ON %s.ship_id = "
        "%s.ship_id WHERE guildcard='%u'"
        , SERVER_CLIENTS_ONLINE, SERVER_CLIENTS_ONLINE
        , SERVER_SHIPS_ONLINE
        , SERVER_CLIENTS_ONLINE
        , SERVER_SHIPS_ONLINE, SERVER_CLIENTS_ONLINE
        , SERVER_SHIPS_ONLINE, guildcard
    );

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Guild Search Error: %s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't fetch Guild Search result: %s",
            psocn_db_error(&conn));
        return 0;
    }

    if (!(row = psocn_db_result_fetch(result))) {
        /* The user's not online, give up. */
        goto out;
    }

    /* Make sure the user isn't on a GM only ship... if they are, bail now */
    if (atoi(row[8])) {
        goto out;
    }

    /* Grab the ship we're looking at first */
    errno = 0;
    ship_id = (uint16_t)strtoul(row[1], NULL, 0);

    if (errno) {
        ERR_LOG("Error parsing in guild ship: %s", strerror(errno));
        goto out;
    }

    /* If we've got this far, we should have the ship we need to send to */
    s = find_ship(ship_id);
    if (!s) {
        ERR_LOG("无效舰船?!?!?!");
        goto out;
    }

    /* If any of these are NULL, the user is not in a lobby. Thus, the client
       doesn't really exist just yet. */
    if (row[4] == NULL || row[3] == NULL || row[11] == NULL) {
        goto out;
    }

    /* Grab the data from the result */
    port = (uint16_t)strtoul(row[7], NULL, 0);
    block = (uint32_t)strtoul(row[2], NULL, 0);
    lobby_id = (uint32_t)strtoul(row[4], NULL, 0);
    ip = (uint32_t)strtoul(row[6], NULL, 0);
    /* Not supported yet.... */
    /*
    ip6_hi = (uint64_t)strtoull(row[9], NULL, 0);
    ip6_lo = (uint64_t)strtoull(row[10], NULL, 0);
    */
    dlobby_id = (uint32_t)strtoul(row[11], NULL, 0);

    if (errno) {
        ERR_LOG("Error parsing in guild search: %s", strerror(errno));
        goto out;
    }

    /* Set up the reply, we should have enough data now */
    memset(&reply, 0, BB_GUILD_REPLY_LENGTH);

    /* Fill it in */
    reply.hdr.pkt_type = LE16(GUILD_REPLY_TYPE);
    reply.hdr.pkt_len = LE16(BB_GUILD_REPLY_LENGTH);
    reply.tag = LE32(0x00010000);
    reply.gc_search = p->gc_search;
    reply.gc_target = p->gc_target;
    reply.ip = htonl(ip);
    reply.port = LE16((port + block * 6 + 4));
    reply.menu_id = LE32(0xFFFFFFFF);
    reply.item_id = LE32(dlobby_id);

    /* Convert the name to the right encoding */
    strcpy(query, row[0]);
    in = strlen(query);
    inptr = query;

    if (query[0] == '\t' && (query[1] == 'J' || query[1] == 'E')) {
        outptr = (char*)reply.name;
        out = 0x40;
    }
    else {
        outptr = (char*)&reply.name[2];
        reply.name[0] = LE16('\t');
        reply.name[1] = LE16('J');
        out = 0x3C;
    }

    iconv(ic_utf8_to_utf16, &inptr, &in, &outptr, &out);

    /* Build the location string, and convert it */
    if (dlobby_id != lobby_id) {
        if (row[3][0] == '\t') {
            strncpy(gname, row[3], 16);
            gname[16] = 0;
        }
        else {
            strncpy(gname + 2, row[3], 14);
            gname[0] = '\t';
            gname[1] = 'E';
            gname[16] = 0;
        }

        sprintf(query, "%s,%s, ,%s", gname, lobby_name, row[5]);
    }
    else {
        sprintf(query, "%s, ,%s", lobby_name, row[5]);
    }

    in = strlen(query);
    inptr = query;
    out = 0x88;
    outptr = (char*)reply.location;
    iconv(ic_utf8_to_utf16, &inptr, &in, &outptr, &out);

    /* Send it away */
    forward_bb(c, (bb_pkt_hdr_t*)&reply, c->key_idx, gc_sender, b_sender);

out:
    /* Finally, we're finished, clean up and return! */
    psocn_db_result_free(result);

    return 0;
}

/* 处理 Blue Burst user's request to add a guildcard to their list */
static int handle_bb_gcadd(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guildcard_add_pkt* gc = (bb_guildcard_add_pkt*)pkt->pkt;
    uint16_t len = LE16(gc->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t fr_gc = LE32(gc->guildcard);
    char query[1024];
    char name[97];
    char guild_name[65];
    char text[373];

    /* Make sure the packet is sane */
    if (len != 0x0110) {
        return -1;
    }

    /* Escape all the strings first */
    psocn_db_escape_str(&conn, name, (char*)gc->name, 48);
    psocn_db_escape_str(&conn, guild_name, (char*)gc->guild_name, 32);
    psocn_db_escape_str(&conn, text, (char*)gc->text, 176);

    /* Add the entry in the db... */
    sprintf(query, "INSERT INTO %s (guildcard, friend_gc, "
        "name, guild_name, text, language, section_id, class) VALUES ('%"
        PRIu32 "', '%" PRIu32 "', '%s', '%s', '%s', '%" PRIu8 "', '%"
        PRIu8 "', '%" PRIu8 "') ON DUPLICATE KEY UPDATE "
        "name=VALUES(name), text=VALUES(text), language=VALUES(language), "
        "section_id=VALUES(section_id), class=VALUES(class)", CLIENTS_BLUEBURST_GUILDCARDS, sender,
        fr_gc, name, guild_name, text, gc->language, gc->section,
        gc->char_class);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法新增GC好友 (%" PRIu32 ": %" PRIu32
            ")", sender, fr_gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)gc, len);
        return 0;
    }

    /* And, we're done... */
    return 0;
}

/* 处理 Blue Burst user's request to delete a guildcard from their list */
static int handle_bb_gcdel(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guildcard_del_pkt* gc = (bb_guildcard_del_pkt*)pkt->pkt;
    uint16_t len = LE16(gc->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t fr_gc = LE32(gc->guildcard);
    char query[256];

    if (len != 0x000C) {
        return -1;
    }

    /* Build the query and run it */
    sprintf(query, "CALL clients_blueburst_guildcards_delete('%" PRIu32 "', '%" PRIu32
        "')", sender, fr_gc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法删除 Blue Burst GC 好友 (%" PRIu32 ": %" PRIu32
            ")", sender, fr_gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)gc, len);
        return 0;
    }

    /* And, we're done... */
    return 0;
}

/* 处理 Blue Burst user's request to sort guildcards */
static int handle_bb_gcsort(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guildcard_sort_pkt* gc = (bb_guildcard_sort_pkt*)pkt->pkt;
    uint16_t len = LE16(gc->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t fr_gc1 = LE32(gc->guildcard1);
    uint32_t fr_gc2 = LE32(gc->guildcard2);
    char query[256];

    if (len != 0x0010) {
        return -1;
    }

    /* Build the query and run it */
    sprintf(query, "CALL clients_blueburst_guildcards_sort('%" PRIu32 "', '%" PRIu32
        "', '%" PRIu32 "')", sender, fr_gc1, fr_gc2);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法整理 Blue Burst GC 好友 (%" PRIu32 ": %" PRIu32
            " - %" PRIu32 ")", sender, fr_gc1, fr_gc2);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)gc, len);
        return 0;
    }

    /* And, we're done... */
    return 0;
}

/* 处理 Blue Burst user's request to add a user to their blacklist */
static int handle_bb_blacklistadd(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_blacklist_add_pkt* gc = (bb_blacklist_add_pkt*)pkt->pkt;
    uint16_t len = LE16(gc->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t bl_gc = LE32(gc->guildcard);
    char query[1024];
    char name[97];
    char guild_name[65];
    char text[373];

    /* Make sure the packet is sane */
    if (len != 0x0110) {
        return -1;
    }

    /* Escape all the strings first */
    psocn_db_escape_str(&conn, name, (char*)gc->name, 48);
    psocn_db_escape_str(&conn, guild_name, (char*)gc->guild_name, 32);
    psocn_db_escape_str(&conn, text, (char*)gc->text, 176);

    /* Add the entry in the db... */
    sprintf(query, "INSERT INTO %s (guildcard, blocked_gc, "
        "name, guild_name, text, language, section_id, class) VALUES ('%"
        PRIu32 "', '%" PRIu32 "', '%s', '%s', '%s', '%" PRIu8 "', '%"
        PRIu8 "', '%" PRIu8 "') ON DUPLICATE KEY UPDATE "
        "name=VALUES(name), text=VALUES(text), language=VALUES(language), "
        "section_id=VALUES(section_id), class=VALUES(class)", CHARACTER_DATA_BLACKLIST, sender,
        bl_gc, name, guild_name, text, gc->language, gc->section,
        gc->char_class);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法新增黑名单实例 (%" PRIu32 ": %" PRIu32
            ")", sender, bl_gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)gc, len);
        return 0;
    }

    /* And, we're done... */
    return 0;
}

/* 处理 Blue Burst user's request to delete a user from their blacklist */
static int handle_bb_blacklistdel(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_blacklist_del_pkt* gc = (bb_blacklist_del_pkt*)pkt->pkt;
    uint16_t len = LE16(gc->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t bl_gc = LE32(gc->guildcard);
    char query[256];

    if (len != 0x000C) {
        return -1;
    }

    /* Build the query and run it */
    sprintf(query, "DELETE FROM %s WHERE guildcard='%" PRIu32
        "' AND blocked_gc='%" PRIu32 "'", CHARACTER_DATA_BLACKLIST, sender, bl_gc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法删除黑名单实例 (%" PRIu32 ": %"
            PRIu32 ")", sender, bl_gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)gc, len);
        return 0;
    }

    /* And, we're done... */
    return 0;
}

/* 处理 Blue Burst set guildcard comment packet */
static int handle_bb_set_comment(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guildcard_comment_pkt* gc = (bb_guildcard_comment_pkt*)pkt->pkt;
    uint16_t pkt_len = LE16(gc->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t fr_gc = LE32(gc->guildcard);
    char query[512];
    char comment[0x88 * 4 + 1];
    int len = 0;

    if (pkt_len != 0x00BC) {
        return -1;
    }

    /* Scan the string for its length */
    while (len < 0x88 && gc->text[len]) ++len;
    memset(&gc->text[len], 0, (0x88 - len) * 2);
    len = (len + 1) * 2;

    psocn_db_escape_str(&conn, comment, (char*)gc->text, len);

    /* Build the query and run it */
    sprintf(query, "UPDATE %s SET comment='%s' WHERE "
        "guildcard='%" PRIu32"' AND friend_gc='%" PRIu32 "'"
        , CLIENTS_BLUEBURST_GUILDCARDS, comment,
        sender, fr_gc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法更新 GC 评论 (%" PRIu32 ": %"
            PRIu32 ")", sender, fr_gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)gc, len);
        return 0;
    }

    /* And, we're done... */
    return 0;
}

/* 处理 Blue Burst 公会 创建 */
static int handle_bb_guild_create(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_create_pkt* g_data = (bb_guild_create_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint16_t ship_id;
    ship_t* s;
    char guild_name[24];
    uint32_t create_res;
    bb_guild_data_pkt* guild;

    if (len != sizeof(bb_guild_create_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    if (g_data->guildcard != sender) {
        ERR_LOG("无效 BB %s 数据包 (%d) 公会创建者GC不一致(%d:%d)", 
            c_cmd_name(type, 0), len, g_data->guildcard, sender);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    istrncpy16_raw(ic_utf16_to_gbk, guild_name, &g_data->guild_name[2], 24, sizeof(g_data->guild_name) - 4);

    DBG_LOG("%s %d %d", guild_name, g_data->guildcard, sender);

    create_res = db_insert_bb_char_guild(g_data->guild_name, default_guild_flag, g_data->guildcard);

    switch (create_res)
    {
    case 0x00:
        guild = (bb_guild_data_pkt*)malloc(sizeof(bb_guild_data_pkt));

        if (!guild) {
            ERR_LOG("分配公会数据内存错误.");
            send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)g_data, len);
            create_res = 1;
            return 0;
        }

        guild->guild = db_get_bb_char_guild(sender);

        guild->hdr.pkt_type = g_data->hdr.pkt_type;
        guild->hdr.pkt_len = sizeof(bb_guild_data_pkt);
        guild->hdr.flags = g_data->hdr.flags;

        ship_id = db_get_char_ship_id(sender);

        if (ship_id < 0) {
            send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)g_data, len);
            return 0;
        }

        /* If we've got this far, we should have the ship we need to send to */
        s = find_ship(ship_id);
        if (!s) {
            ERR_LOG("无效舰船?!?!?");
            return 0;
        }

        forward_bb(s, (bb_pkt_hdr_t*)guild, c->key_idx, 0, 0);
        DBG_LOG("创建GC %d (%s)公会数据成功.", sender, guild_name);
        free_safe(guild);
        break;

    case 0x01:
        ERR_LOG("创建GC %d 公会数据失败, 数据库错误.", sender);
        break;

    case 0x02:
        ERR_LOG("创建GC %d 公会数据失败, %s 公会已存在.", sender, guild_name);
        break;

    case 0x03:
        ERR_LOG("创建GC %d 公会数据失败,该 GC 已在公会中.", sender);
        break;
    }

    /* 完成数据包设置,发送至舰船... */
    return create_res;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_02EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_02EA_pkt* g_data = (bb_guild_unk_02EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_02EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 成员增加 */
static int handle_bb_guild_member_add(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_add_pkt* g_data = (bb_guild_member_add_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_member_add_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_04EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_04EA_pkt* g_data = (bb_guild_unk_04EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_04EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 成员移除 */
static int handle_bb_guild_member_remove(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_remove_pkt* g_data = (bb_guild_member_remove_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_member_remove_pkt) + 0x0004) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_06EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_06EA_pkt* g_data = (bb_guild_unk_06EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_06EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 聊天 */
static int handle_bb_guild_member_chat(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_chat_pkt* g_data = (bb_guild_member_chat_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint16_t* n;

    if (len != sizeof(bb_guild_member_chat_pkt) + 0x004C) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    n = (uint16_t*)&pkt[0x2C];
    while (*n != 0x0000)
    {
        if ((*n == 0x0009) || (*n == 0x000A))
            *n = 0x0020;
        n++;
    }

    //uint32_t size;

    //ship->encrypt_buf_code[0x00] = 0x09;
    //ship->encrypt_buf_code[0x01] = 0x04;
    //*(uint32_t*)&ship->encrypt_buf_code[0x02] = teamid;
    //while (chatsize % 8)
    //    ship->encrypt_buf_code[6 + (chatsize++)] = 0x00;
    //*text = chatsize;
    //memcpy(&ship->encrypt_buf_code[0x06], text, chatsize);
    //size = chatsize + 6;
    //Compress_Ship_Packet_Data(SHIP_SERVER, ship, &ship->encrypt_buf_code[0x00], size);

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会 成员设置 */
static int handle_bb_guild_member_setting(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_setting_pkt* g_data = (bb_guild_member_setting_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint16_t ship_id;
    ship_t* s;

    uint32_t packet_offset = 0;
    int32_t num_mates;
    int32_t ch;
    uint32_t guildcard, privlevel;
    void* result;
    char** row;

    //if (len != (sizeof(bb_guild_member_setting_pkt) - 0x0008)) {
    //    ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
    //    print_payload((uint8_t*)g_data, len);

    //    send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
    //        ERR_BAD_ERROR, (uint8_t*)g_data, len);
    //    return 0;
    //}

    sprintf_s(myquery, _countof(myquery), "SELECT guildcard, guild_priv_level "
        "FROM %s WHERE guild_id = '%" PRIu32 "'", AUTH_DATA_ACCOUNT, g_data->guild_id);
    if (!psocn_db_real_query(&conn, myquery))
    {
        result = psocn_db_result_store(&conn);
        num_mates = (int32_t)psocn_db_result_rows(result);
        *(uint32_t*)&g_data->data[packet_offset] = num_mates;
        packet_offset += 4;
        for (ch = 1; ch <= num_mates; ch++)
        {
            row = mysql_fetch_row(result);
            guildcard = atoi(row[0]);
            privlevel = atoi(row[1]);
            *(uint32_t*)&g_data->data[packet_offset] = ch;
            packet_offset += 4;
            *(uint32_t*)&g_data->data[packet_offset] = privlevel;
            packet_offset += 4;
            *(uint32_t*)&g_data->data[packet_offset] = guildcard;
            packet_offset += 4;
            memcpy(&g_data->data[packet_offset], g_data->char_name, 24);
            packet_offset += 24;
            memset(&g_data->data[packet_offset], 0, 8);
            packet_offset += 8;
            DBG_LOG("%d %d %d %d", packet_offset, guildcard, privlevel, num_mates);
        }
        mysql_free_result(result);
        //packet_offset -= 0x0A;
        //*(uint16_t*)&temp_pkt[0x0A] = (uint16_t)packet_offset;
        //packet_offset += 0x0A;

        g_data->hdr.pkt_len = packet_offset;
        g_data->hdr.pkt_type = BB_GUILD_UNK_09EA;
        g_data->hdr.flags = 0x00000000;


        ship_id = db_get_char_ship_id(sender);

        if (ship_id < 0) {
            send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)g_data, len);
            return 0;
        }

        /* If we've got this far, we should have the ship we need to send to */
        s = find_ship(ship_id);
        if (!s) {
            ERR_LOG("无效舰船?!?!?");
            return 0;
        }

        forward_bb(s, (bb_pkt_hdr_t*)g_data, c->key_idx, 0, 0);
    }
    else
    {
        Logs(__LINE__, mysqlerr_log_console_show, MYSQLERR_LOG, "未能找到 %u 队伍信息", g_data->guild_id);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_09EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_09EA_pkt* g_data = (bb_guild_unk_09EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_09EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_0AEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_0AEA_pkt* g_data = (bb_guild_unk_0AEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_0AEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_0BEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_0BEA_pkt* g_data = (bb_guild_unk_0BEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_0BEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_0CEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_0CEA_pkt* g_data = (bb_guild_unk_0CEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_0CEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_0DEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_0DEA_pkt* g_data = (bb_guild_unk_0DEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_0DEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_0EEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_0EEA_pkt* g_data = (bb_guild_unk_0EEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_0EEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 成员图标设置 */
static int handle_bb_guild_member_flag_setting(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_flag_setting_pkt* g_data = (bb_guild_member_flag_setting_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_member_flag_setting_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会 解散公会 */
static int handle_bb_guild_dissolve_team(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_dissolve_team_pkt* g_data = (bb_guild_dissolve_team_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_dissolve_team_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 成员权限提升 */
static int handle_bb_guild_member_promote(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_promote_pkt* g_data = (bb_guild_member_promote_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_member_promote_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_12EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_12EA_pkt* g_data = (bb_guild_unk_12EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_12EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 大厅设置 */
static int handle_bb_guild_lobby_setting(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_lobby_setting_pkt* g_data = (bb_guild_lobby_setting_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_lobby_setting_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会 人物抬头显示 */
static int handle_bb_guild_member_tittle(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_tittle_pkt* g_data = (bb_guild_member_tittle_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_member_tittle_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_15EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_15EA_pkt* g_data = (bb_guild_unk_15EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_15EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_16EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_16EA_pkt* g_data = (bb_guild_unk_16EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_16EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_17EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_17EA_pkt* g_data = (bb_guild_unk_17EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_17EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会 点数 */
static int handle_bb_guild_buy_privilege_and_point_info(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_buy_privilege_and_point_info_pkt* g_data = (bb_guild_buy_privilege_and_point_info_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_buy_privilege_and_point_info_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 权限列表 */
static int handle_bb_guild_privilege_list(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_privilege_list_pkt* g_data = (bb_guild_privilege_list_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_privilege_list_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_1AEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_1AEA_pkt* g_data = (bb_guild_unk_1AEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_1AEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_1BEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_1BEA_pkt* g_data = (bb_guild_unk_1BEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_1BEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会 排行榜 */
static int handle_bb_guild_rank_list(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_rank_list_pkt* g_data = (bb_guild_rank_list_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_rank_list_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_1DEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_1DEA_pkt* g_data = (bb_guild_unk_1DEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_1DEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    print_payload((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会功能 */
static int handle_bb_guild(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_pkt_hdr_t* hdr = (bb_pkt_hdr_t*)pkt->pkt;
    uint16_t type = LE16(hdr->pkt_type);
    uint16_t len = LE16(hdr->pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    DBG_LOG("舰闸：BB 公会功能指令 0x%04X %s (长度%d)", type, c_cmd_name(type, 0), len);

    ///* Warn the ship that sent the packet, then drop it */
    //send_error(c, SHDR_TYPE_PC, SHDR_FAILURE, ERR_GAME_UNK_PACKET,
    //    (uint8_t*)pkt, ntohs(pkt->hdr.pkt_len));

    switch (type) {
    case BB_GUILD_CREATE:
        return handle_bb_guild_create(c, pkt);

    case BB_GUILD_UNK_02EA:
        return handle_bb_guild_unk_02EA(c, pkt);

    case BB_GUILD_MEMBER_ADD:
        return handle_bb_guild_member_add(c, pkt);

    case BB_GUILD_UNK_04EA:
        return handle_bb_guild_unk_04EA(c, pkt);

    case BB_GUILD_MEMBER_REMOVE:
        return handle_bb_guild_member_remove(c, pkt);

    case BB_GUILD_UNK_06EA:
        return handle_bb_guild_06EA(c, pkt);

    case BB_GUILD_CHAT:
        return handle_bb_guild_member_chat(c, pkt);

    case BB_GUILD_MEMBER_SETTING:
        return handle_bb_guild_member_setting(c, pkt);

    case BB_GUILD_UNK_09EA:
        return handle_bb_guild_unk_09EA(c, pkt);

    case BB_GUILD_UNK_0AEA:
        return handle_bb_guild_unk_0AEA(c, pkt);

    case BB_GUILD_UNK_0BEA:
        return handle_bb_guild_unk_0BEA(c, pkt);

    case BB_GUILD_UNK_0CEA:
        return handle_bb_guild_unk_0CEA(c, pkt);

    case BB_GUILD_UNK_0DEA:
        return handle_bb_guild_unk_0DEA(c, pkt);

    case BB_GUILD_UNK_0EEA:
        return handle_bb_guild_unk_0EEA(c, pkt);

    case BB_GUILD_MEMBER_FLAG_SETTING:
        return handle_bb_guild_member_flag_setting(c, pkt);

    case BB_GUILD_DISSOLVE_TEAM:
        return handle_bb_guild_dissolve_team(c, pkt);

    case BB_GUILD_MEMBER_PROMOTE:
        return handle_bb_guild_member_promote(c, pkt);

    case BB_GUILD_UNK_12EA:
        return handle_bb_guild_unk_12EA(c, pkt);

    case BB_GUILD_LOBBY_SETTING:
        return handle_bb_guild_lobby_setting(c, pkt);

    case BB_GUILD_MEMBER_TITLE:
        return handle_bb_guild_member_tittle(c, pkt);

    case BB_GUILD_UNK_15EA:
        return handle_bb_guild_unk_15EA(c, pkt);

    case BB_GUILD_UNK_16EA:
        return handle_bb_guild_unk_16EA(c, pkt);

    case BB_GUILD_UNK_17EA:
        return handle_bb_guild_unk_17EA(c, pkt);

    case BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO:
        return handle_bb_guild_buy_privilege_and_point_info(c, pkt);

    case BB_GUILD_PRIVILEGE_LIST:
        return handle_bb_guild_privilege_list(c, pkt);

    case BB_GUILD_UNK_1AEA:
        return handle_bb_guild_unk_1AEA(c, pkt);

    case BB_GUILD_UNK_1BEA:
        return handle_bb_guild_unk_1BEA(c, pkt);

    case BB_GUILD_RANKING_LIST:
        return handle_bb_guild_rank_list(c, pkt);

    case BB_GUILD_UNK_1DEA:
        return handle_bb_guild_unk_1DEA(c, pkt);

    default:
        UDONE_CPD(type, pkt);
        break;
    }


    return 0;
}

/* 处理 ship's forwarded Dreamcast packet. */
static int handle_dreamcast(ship_t* c, shipgate_fw_9_pkt* pkt) {
    dc_pkt_hdr_t* hdr = (dc_pkt_hdr_t*)pkt->pkt;
    uint8_t type = hdr->pkt_type;
    uint32_t tmp;

    switch (type) {
    case GUILD_SEARCH_TYPE:
        tmp = ntohl(pkt->fw_flags);
        return handle_guild_search(c, (dc_guild_search_pkt*)hdr, tmp);

    case SIMPLE_MAIL_TYPE:
        return handle_dc_mail(c, (simple_mail_pkt*)hdr);

    case GUILD_REPLY_TYPE:
        /* We shouldn't get these anymore (as of protocol v3)... */
    default:
        /* Warn the ship that sent the packet, then drop it */
        send_error(c, SHDR_TYPE_DC, SHDR_FAILURE, ERR_GAME_UNK_PACKET,
            (uint8_t*)pkt, ntohs(pkt->hdr.pkt_len));
        return 0;
    }
}

/* 处理 ship's forwarded PC packet. */
static int handle_pc(ship_t* c, shipgate_fw_9_pkt* pkt) {
    dc_pkt_hdr_t* hdr = (dc_pkt_hdr_t*)pkt->pkt;
    uint8_t type = hdr->pkt_type;
    uint16_t len = LE16(hdr->pkt_len);

    //DBG_LOG("舰船：DC处理数据 指令= 0x%04X %s 长度 = %d 字节", type, c_cmd_name(type, 0), len);

    switch (type) {
    case SIMPLE_MAIL_TYPE:
        return handle_pc_mail(c, (simple_mail_pkt*)hdr);

    default:
        /* Warn the ship that sent the packet, then drop it */
        send_error(c, SHDR_TYPE_PC, SHDR_FAILURE, ERR_GAME_UNK_PACKET,
            (uint8_t*)pkt, ntohs(pkt->hdr.pkt_len));
        return 0;
    }
}

static int handle_bb(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_pkt_hdr_t* hdr = (bb_pkt_hdr_t*)pkt->pkt;
    uint16_t type = LE16(hdr->pkt_type);
    uint16_t len = LE16(hdr->pkt_len);

    //DBG_LOG("舰船：BB处理数据 指令= 0x%04X %s 长度 = %d 字节", type, c_cmd_name(type, 0), len);

    switch (type) {
    case BB_ADD_GUILDCARD_TYPE:
        return handle_bb_gcadd(c, pkt);

    case BB_DEL_GUILDCARD_TYPE:
        return handle_bb_gcdel(c, pkt);

    case BB_SORT_GUILDCARD_TYPE:
        return handle_bb_gcsort(c, pkt);

    case BB_ADD_BLOCKED_USER_TYPE:
        return handle_bb_blacklistadd(c, pkt);

    case BB_DEL_BLOCKED_USER_TYPE:
        return handle_bb_blacklistdel(c, pkt);

    case SIMPLE_MAIL_TYPE:
        return handle_bb_mail(c, (simple_mail_pkt*)hdr);

    case GUILD_SEARCH_TYPE:
        return handle_bb_guild_search(c, pkt);

    case BB_SET_GUILDCARD_COMMENT_TYPE:
        return handle_bb_set_comment(c, pkt);

        /* 0x00EA 公会功能 */
    case BB_GUILD_CREATE:
    case BB_GUILD_UNK_02EA:
    case BB_GUILD_MEMBER_ADD:
    case BB_GUILD_UNK_04EA:
    case BB_GUILD_MEMBER_REMOVE:
    case BB_GUILD_UNK_06EA:
    case BB_GUILD_CHAT:
    case BB_GUILD_MEMBER_SETTING:
    case BB_GUILD_UNK_09EA:
    case BB_GUILD_UNK_0AEA:
    case BB_GUILD_UNK_0BEA:
    case BB_GUILD_UNK_0CEA:
    case BB_GUILD_UNK_0DEA:
    case BB_GUILD_UNK_0EEA:
    case BB_GUILD_MEMBER_FLAG_SETTING:
    case BB_GUILD_DISSOLVE_TEAM:
    case BB_GUILD_MEMBER_PROMOTE:
    case BB_GUILD_UNK_12EA:
    case BB_GUILD_LOBBY_SETTING:
    case BB_GUILD_MEMBER_TITLE:
    case BB_GUILD_UNK_15EA:
    case BB_GUILD_UNK_16EA:
    case BB_GUILD_UNK_17EA:
    case BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO:
    case BB_GUILD_PRIVILEGE_LIST:
    case BB_GUILD_UNK_1AEA:
    case BB_GUILD_UNK_1BEA:
    case BB_GUILD_RANKING_LIST:
    case BB_GUILD_UNK_1DEA:
        return handle_bb_guild(c, pkt);

    default:
        /* Warn the ship that sent the packet, then drop it
         警告发送包裹的船，然后丢弃它
        */
        UNK_SPD(type, (uint8_t*)pkt);
        send_error(c, SHDR_TYPE_BB, SHDR_FAILURE, ERR_GAME_UNK_PACKET,
            (uint8_t*)pkt, len);
        return 0;
    }
}

/* 船闸保存角色数据. */
static int handle_cdata(ship_t* c, shipgate_char_data_pkt* pkt) {
    uint32_t gc, slot;
    uint16_t data_len = ntohs(pkt->hdr.pkt_len) - sizeof(shipgate_char_data_pkt);
    psocn_bb_db_char_t* char_data = (psocn_bb_db_char_t*)pkt->data;
    static char query[16384];

    gc = ntohl(pkt->guildcard);
    slot = ntohl(pkt->slot);

    if (gc == 0) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }
    
    if (db_update_char_stat(char_data, gc, slot, PSOCN_DB_UPDATA_CHAR)) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        SQLERR_LOG("无法更新玩家数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 ")", gc, slot);
        return 0;
    }

    if (db_compress_char_data(char_data, data_len, gc, slot)) {
        ERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 ")", CHARACTER_DATA, gc, slot);

        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return -6;
    }

    if (db_update_char_challenge(char_data, gc, slot, PSOCN_DB_UPDATA_CHAR)) {
        SQLERR_LOG("无法保存角色挑战数据 (%" PRIu32 ": %" PRIu8 ")", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }





    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '%d' WHERE guildcard = '%" PRIu32 "'",
        AUTH_DATA_ACCOUNT, 0, gc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("更新GC %" PRIu32 " 槽位 %" PRIu8 " 数据错误:\n %s", gc, slot, psocn_db_error(&conn));
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }

    /* Return success (yeah, bad use of this function, but whatever). */
    return send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE, ERR_NO_ERROR,
        (uint8_t*)&pkt->guildcard, 8);
}

static int handle_cbkup_req(ship_t* c, shipgate_char_bkup_pkt* pkt, uint32_t gc,
    const char name[], uint32_t block) {
    char query[256];
    char name2[65];
    uint8_t *data;
    void *result;
    char **row;
    unsigned long *len;
    int sz, rv;
    uLong sz2, csz;

    /* Build the query asking for the data. */
    psocn_db_escape_str(&conn, name2, name, strlen(name));
    sprintf(query, "SELECT data, size FROM %s WHERE "
            "guildcard='%u' AND name='%s'", CHARACTER_DATA_BACKUP, gc, name2);

    if(psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法获取角色备份数据 (%u: %s)", gc, name);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                   ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
        return 0;
    }

    /* Grab the data we got. */
    if((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未查询到角色备份数据 (%u: %s)", gc, name);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                   ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
        return 0;
    }

    if((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到角色备份数据 (%u: %s)", gc, name);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                   ERR_CREQ_NO_DATA, (uint8_t *)&pkt->guildcard, 8);
        return 0;
    }

    /* Grab the length of the character data */
    if(!(len = psocn_db_result_lengths(result))) {
        psocn_db_result_free(result);
        SQLERR_LOG("无法获取角色备份数据的长度");
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                   ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
        return 0;
    }

    /* Grab the data from the result */
    sz = (int)len[0];

    if(row[1]) {
        sz2 = (uLong)atoi(row[1]);
        csz = (uLong)sz;

        data = (uint8_t *)malloc(sz2);
        if(!data) {
            SQLERR_LOG("无法为解压角色备份数据分配内存空间");
            SQLERR_LOG("%s", strerror(errno));
            psocn_db_result_free(result);

            send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                       ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
            return 0;
        }

        /* 解压缩数据 */
        if(uncompress((Bytef *)data, &sz2, (Bytef *)row[0], csz) != Z_OK) {
            SQLERR_LOG("无法解压角色备份数据");
            psocn_db_result_free(result);

            send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                       ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
            return 0;
        }

        sz = sz2;
    }
    else {
        data = (uint8_t *)malloc(sz);
        if(!data) {
            SQLERR_LOG("无法分配玩家备份内存");
            SQLERR_LOG("%s", strerror(errno));
            psocn_db_result_free(result);

            send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                       ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
            return 0;
        }

        memcpy(data, row[0], len[0]);
    }

    psocn_db_result_free(result);

    /* Send the data back to the ship. */
    rv = send_cdata(c, gc, (uint32_t)-1, data, sz, block);

    /* Clean up and finish */
    free_safe(data);
    return rv;
}

static int handle_cbkup(ship_t* c, shipgate_char_bkup_pkt* pkt) {
    static char query[16384];
    uint32_t gc, block;
    uint16_t len = ntohs(pkt->hdr.pkt_len) - sizeof(shipgate_char_bkup_pkt);
    char name[32], name2[65];
    Bytef* cmp_buf;
    uLong cmp_sz;
    int compressed = ~Z_OK;

    gc = ntohl(pkt->guildcard);
    block = ntohl(pkt->block);
    strncpy(name, (const char*)pkt->name, 32);
    name[31] = 0;

    /* Is this a restore request or are we saving the character data? */
    if (len == 0) {
        return handle_cbkup_req(c, pkt, gc, name, block);
    }

    /* Is it a Blue Burst character or not? */
    if (len > 1056) {
        len = sizeof(psocn_bb_db_char_t);
    }
    else {
        len = 1052;
    }

    psocn_db_escape_str(&conn, name2, name, strlen(name));

    /* Compress the character data */
    cmp_sz = compressBound((uLong)len);

    if ((cmp_buf = (Bytef*)malloc(cmp_sz))) {
        compressed = compress2(cmp_buf, &cmp_sz, (Bytef*)pkt->data,
            (uLong)len, 9);
    }

    /* Build up the store query for it. */
    if (compressed == Z_OK && cmp_sz < len) {
        sprintf(query, "INSERT INTO %s(guildcard, size, name, "
            "data) VALUES ('%u', '%u', '%s', '", CHARACTER_DATA_BACKUP, gc, (unsigned)len, name2);
        psocn_db_escape_str(&conn, query + strlen(query), (char*)cmp_buf,
            cmp_sz);
    }
    else {
        sprintf(query, "INSERT INTO %s(guildcard, name, data) "
            "VALUES ('%u', '%s', '", CHARACTER_DATA_BACKUP, gc, name2);
        psocn_db_escape_str(&conn, query + strlen(query), (char*)pkt->data,
            len);
    }

    strcat(query, "') ON DUPLICATE KEY UPDATE data=VALUES(data)");
    free_safe(cmp_buf);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法保存玩家备份 (%u: %s)", gc, name);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }

    /* Return success (yeah, bad use of this function, but whatever). */
    return send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE, ERR_NO_ERROR,
        (uint8_t*)&pkt->guildcard, 8);
}

/* 处理舰船角色数据请求. */
static int handle_creq(ship_t *c, shipgate_char_req_pkt *pkt) {
    uint32_t gc, slot;
    uint8_t *data;
    char *raw_data;
    uint32_t data_length, data_size;
    int sz, rv;
    uLong sz2, csz;

    gc = ntohl(pkt->guildcard);
    slot = ntohl(pkt->slot);

    /* Grab the data from the result */
    data_length = db_get_char_data_length(gc, slot);
    data_size = db_get_char_data_size(gc, slot);
    raw_data = db_get_char_raw_data(gc, slot, 1);

    sz = (int)data_length;

    if(data_size) {
        sz2 = data_size;
        csz = (uLong)sz;

        data = (uint8_t *)malloc(sz2);
        if(!data) {
            SQLERR_LOG("无法分配解压角色数据的内存空间");
            SQLERR_LOG("%s", strerror(errno));

            send_error(c, SHDR_TYPE_CREQ, SHDR_RESPONSE | SHDR_FAILURE,
                       ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
            return 0;
        }

        /* 解压缩数据 */
        if(uncompress((Bytef *)data, &sz2, (Bytef *)raw_data, csz) != Z_OK) {
            ERR_LOG("无法解压角色数据");

            send_error(c, SHDR_TYPE_CREQ, SHDR_RESPONSE | SHDR_FAILURE,
                       ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
            return 0;
        }

        sz = sz2;
    }
    else {
        data = (uint8_t *)malloc(sz);
        if(!data) {
            ERR_LOG("无法分配角色数据的内存空间");
            ERR_LOG("%s", strerror(errno));

            send_error(c, SHDR_TYPE_CREQ, SHDR_RESPONSE | SHDR_FAILURE,
                       ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
            return 0;
        }
        memcpy(data, raw_data, data_length);
    }

    /* Send the data back to the ship. */
    rv = send_cdata(c, gc, slot, data, sz, 0);

    /* Clean up and finish */
    free_safe(data);
    return rv;
}

/* 处理 client login request coming from a ship. */
static int handle_usrlogin(ship_t* c, shipgate_usrlogin_req_pkt* pkt) {
    uint32_t gc, block;
    char query[256];
    void* result;
    char** row;
    //int i;
    uint32_t islogged = 0;
    unsigned char hash[16] = { 0 };
    uint8_t MDBuffer[34 * 2] = { 0 };
    int8_t password[34] = { 0 };
    int8_t md5password[34] = { 0 };
    uint32_t ch;
    char esc[65];
    uint16_t len;
    uint32_t priv;

    /* Check the sanity of the packet. Disconnect the ship if there's some odd
       issue with the packet's sanity. */
    len = ntohs(pkt->hdr.pkt_len);
    if (len != sizeof(shipgate_usrlogin_req_pkt)) {
        ERR_LOG("舰船 %s 发送了无效的客户端登录申请!?", c->name);
        return -1;
    }

    if (pkt->username[31] != '\0' || pkt->password[31] != '\0') {
        ERR_LOG("舰船 %s 发送了未认证的客户端登录申请", c->name);
        return -1;
    }

    /* Escape the username and grab the data we need. */
    psocn_db_escape_str(&conn, esc, pkt->username, strlen(pkt->username));
    gc = ntohl(pkt->guildcard);
    block = ntohl(pkt->block);

    /* Build the query asking for the data. */
    sprintf(query, "SELECT password, regtime, privlevel, islogged FROM %s "
        "NATURAL JOIN %s WHERE guildcard='%u' AND username='%s'"
        , CLIENTS_GUILDCARDS
        , AUTH_DATA_ACCOUNT, gc, esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("登录失败 - 查询 %s 数据表失败 (玩家: %s, gc: %u)", CLIENTS_GUILDCARDS,
            pkt->username, gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("登录失败 - 玩家 %s 数据不存在 (玩家: %s, gc: %u)", CLIENTS_GUILDCARDS,
            pkt->username, gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SGATE_LOG("登录失败 - 用户名不存在 (玩家: %s, gc: %u)",
            pkt->username, gc);

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE,
            ERR_USRLOGIN_BAD_CRED, (uint8_t*)&pkt->guildcard, 8);
    }

    memcpy(&password, &pkt->password, sizeof(pkt->password));

    sprintf_s(&password[strlen(password)],
        _countof(password) - strlen(password), "_%s_salt", row[1]);
    md5((unsigned char*)password, strlen(password), MDBuffer);
    for (ch = 0; ch < 16; ch++)
        sprintf_s(&md5password[ch * 2],
            _countof(md5password) - ch * 2, "%02x", (uint8_t)MDBuffer[ch]);
    md5password[32] = 0;

    if (memcmp(&md5password[0], row[0], 32)) {
        SGATE_LOG("登录失败 - 密码错误 (玩家: %s, gc: %u)",
            pkt->username, gc);
        psocn_db_result_free(result);

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE,
            ERR_USRLOGIN_BAD_CRED, (uint8_t*)&pkt->guildcard, 8);
    }

    /* Grab the privilege level out of the packet */
    errno = 0;
    priv = (uint32_t)strtoul(row[2], NULL, 0);
    islogged = (uint32_t)strtoul(row[3], NULL, 0);

    if (errno != 0) {
        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE,
            ERR_USRLOGIN_BAD_PRIVS, (uint8_t*)&pkt->guildcard,
            8);
    }

    /* Filter out any privileges that don't make sense. Can't have global GM
       without local GM support. Also, anyone set as a root this way must have
       BOTH root bits set, not just one! */
    if (((priv & CLIENT_PRIV_GLOBAL_GM) && !(priv & CLIENT_PRIV_LOCAL_GM)) ||
        ((priv & CLIENT_PRIV_GLOBAL_ROOT) && !(priv & CLIENT_PRIV_LOCAL_ROOT)) ||
        ((priv & CLIENT_PRIV_LOCAL_ROOT) && !(priv & CLIENT_PRIV_GLOBAL_ROOT))) {
        SQLERR_LOG("登录失败 - 玩家权限无效 %u: %02x", pkt->username,
            priv);
        psocn_db_result_free(result);

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE,
            ERR_USRLOGIN_BAD_PRIVS, (uint8_t*)&pkt->guildcard,
            8);
    }

    /* The privilege field went to 32-bits in version 18. */
    if (c->proto_ver < 18) {
        priv &= (CLIENT_PRIV_LOCAL_GM | CLIENT_PRIV_GLOBAL_GM |
            CLIENT_PRIV_LOCAL_ROOT | CLIENT_PRIV_GLOBAL_ROOT);
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '%d' where guildcard = '%u'",
        AUTH_DATA_ACCOUNT, islogged, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("更新GC %u 数据错误:\n %s", gc, psocn_db_error(&conn));
        return -4;
    }

    /* We're done if we got this far. */
    psocn_db_result_free(result);

    /* Send a success message. */
    return send_usrloginreply(c, gc, block, 1, priv);
}

/* 处理 ban request coming from a ship. */
static int handle_ban(ship_t* c, shipgate_ban_req_pkt* pkt, uint16_t type) {
    uint32_t req, target, until;
    char query[1024];
    void* result;
    char** row;
    int account_id;
    int priv, priv2;

    req = ntohl(pkt->req_gc);
    target = ntohl(pkt->target);
    until = ntohl(pkt->until);

    /* Make sure the requester has permission. */
    sprintf(query, "SELECT account_id, privlevel FROM %s NATURAL JOIN "
        "%s WHERE guildcard='%u' AND privlevel>'2'"
        , CLIENTS_GUILDCARDS
        , AUTH_DATA_ACCOUNT, req);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法从 %s 获取到数据 (%u)", CLIENTS_GUILDCARDS, req);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法从 %s 获取到结果 (%u)", CLIENTS_GUILDCARDS, req);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("用户数据不存在或没有GM (%u)", req);

        return send_error(c, type, SHDR_FAILURE, ERR_BAN_NOT_GM,
            (uint8_t*)&pkt->req_gc, 16);
    }

    /* We've verified they're legit, continue on. */
    account_id = atoi(row[0]);
    priv = atoi(row[1]);
    psocn_db_result_free(result);

    /* Make sure the user isn't trying to ban someone with a higher privilege
       level than them... */
    sprintf(query, "SELECT privlevel FROM %s NATURAL JOIN %s "
        "WHERE guildcard='%u'", CLIENTS_GUILDCARDS, AUTH_DATA_ACCOUNT, target);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法从 %s 获取到数据 (%u)", CLIENTS_GUILDCARDS, target);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法从 %s 获取到结果 (%u)", CLIENTS_GUILDCARDS, target);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    if ((row = psocn_db_result_fetch(result))) {
        priv2 = atoi(row[0]);

        if (priv2 >= priv) {
            psocn_db_result_free(result);
            SQLERR_LOG("%u 尝试封禁 %u 但权限不足",
                req, target);

            return send_error(c, type, SHDR_FAILURE, ERR_BAN_PRIVILEGE,
                (uint8_t*)&pkt->req_gc, 16);
        }
    }

    /* We're done with that... */
    psocn_db_result_free(result);

    /* Build up the ban insert query. */
    sprintf(query, "INSERT INTO %s(enddate, setby, reason) VALUES "
        "('%u', '%u', '", AUTH_DATA_BANS, until, account_id);
    psocn_db_escape_str(&conn, query + strlen(query), (char*)pkt->message,
        strlen(pkt->message));
    strcat(query, "')");

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法向 %s 插入封禁数据", AUTH_DATA_BANS);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    /* Now that we have that, add the ban to the right table... */
    switch (type) {
    case SHDR_TYPE_GCBAN:
        sprintf(query, "INSERT INTO %s(ban_id, guildcard) "
            "VALUES(LAST_INSERT_ID(), '%lu')", AUTH_DATA_BANS_GC, ntohl(pkt->target));
        break;

    case SHDR_TYPE_IPBAN:
        sprintf(query, "INSERT INTO %s(ban_id, addr) VALUES("
            "LAST_INSERT_ID(), '%lu')", AUTH_DATA_BANS_IP, ntohl(pkt->target));
        break;

    default:
        return send_error(c, type, SHDR_FAILURE, ERR_BAN_BAD_TYPE,
            (uint8_t*)&pkt->req_gc, 16);
    }

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法插入封禁数据 (步骤 2)");
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    /* Another misuse of the error function, but whatever */
    return send_error(c, type, SHDR_RESPONSE, ERR_NO_ERROR,
        (uint8_t*)&pkt->req_gc, 16);
}

static int handle_blocklogin(ship_t* c, shipgate_block_login_pkt* pkt) {
    char query[512];
    char name[64];
    char tmp[128];
    uint32_t gc, bl, gc2, bl2, opt, acc = 0;
    uint16_t ship_id;
    ship_t* c2;
    void* result;
    char** row;
    void* optpkt;
    unsigned long* lengths;
    size_t in, out;
    char* inptr;
    char* outptr;
    monster_event_t* ev;
    const char* tbl_nm = SERVER_CLIENTS_ONLINE;

    /* Is the name a Blue Burst-style (UTF-16) name or not? */
    if (pkt->ch_name[0] == '\t') {
        memset(name, 0, 64);
        in = 32;
        out = 64;
        inptr = (char*)&pkt->ch_name[2];
        outptr = name;

        istrncpy16_raw(ic_utf16_to_utf8, name, &pkt->ch_name[2], out, in);

        //iconv(ic_utf16_to_gbk, &inptr, &in, &outptr, &out);
    }
    else {
        /* Make sure the name is terminated properly */
        if (pkt->ch_name[31] != '\0') {
            return send_error(c, SHDR_TYPE_BLKLOGIN, SHDR_FAILURE,
                ERR_BLOGIN_INVAL_NAME, (uint8_t*)&pkt->guildcard,
                8);
        }

        /* The name is ASCII, which is safe to use as UTF-8 */
        strcpy(name, pkt->ch_name);
    }

    /* Parse out some stuff we'll use */
    gc = ntohl(pkt->guildcard);
    bl = ntohl(pkt->blocknum);

    /* Is this a transient client (that is to say someone on the PC NTE)? */
    if (gc >= 500 && gc < 600)
        tbl_nm = SERVER_CLIENTS_TRANSIENT;

    /* Insert the client into the online_clients table */
    psocn_db_escape_str(&conn, tmp, name, strlen(name));
    sprintf(query, "INSERT INTO %s(guildcard, name, ship_id, "
        "block) VALUES('%u', '%s', '%hu', '%u')", tbl_nm, gc, tmp,
        c->key_idx, bl);

    /* If the query fails, most likely its a primary key violation, so assume
       the user is already logged in */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("新增客户端至 %s 数据表错误: %s", tbl_nm,
            psocn_db_error(&conn));
        return send_error(c, SHDR_TYPE_BLKLOGIN, SHDR_FAILURE,
            ERR_BLOGIN_ONLINE, (uint8_t*)&pkt->guildcard, 8);
    }

    /* None of the rest of this applies to PC NTE clients at all... */
    if (gc >= 500 && gc < 600)
        return 0;

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '%d', lastblock = '%d' WHERE guildcard = '%u'",
        CHARACTER_DATA, 1, bl, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("更新GC %u 数据错误:\n %s", gc, psocn_db_error(&conn));
        return 0;
    }

    /* Find anyone that has the user in their friendlist so we can send a
       message to them */
    sprintf(query, "SELECT guildcard, block, ship_id, nickname FROM "
        "%s INNER JOIN %s ON "
        "%s.guildcard = %s.owner WHERE "
        "%s.friend = '%u'"
        , SERVER_CLIENTS_ONLINE, CHARACTER_DATA_FRIENDLIST
        , SERVER_CLIENTS_ONLINE, CHARACTER_DATA_FRIENDLIST
        , CHARACTER_DATA_FRIENDLIST, gc
    );

    /* Query for any results */
    if (psocn_db_real_query(&conn, query)) {
        /* Silently fail here (to the ship anyway), since this doesn't spell
           doom at all for the logged in user */
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_friends;
    }

    /* Grab any results we got */
    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_friends;
    }

    /* For each bite we get, send out a friend login packet */
    while ((row = psocn_db_result_fetch(result))) {
        gc2 = (uint32_t)strtoul(row[0], NULL, 0);

        if (check_user_blocklist(gc2, gc, BLOCKLIST_FLIST))
            continue;

        bl2 = (uint32_t)strtoul(row[1], NULL, 0);
        ship_id = (uint16_t)strtoul(row[2], NULL, 0);
        c2 = find_ship(ship_id);

        if (c2) {
            send_friend_message(c2, 1, gc2, bl2, gc, bl, c->key_idx, name,
                row[3]);
        }
    }

    psocn_db_result_free(result);

skip_friends:
    /* See what options we have to deliver to the user */
    sprintf(query, "SELECT opt, value FROM %s WHERE "
        "guildcard='%u'", CLIENTS_OPTIONS, gc);

    /* Query for any results */
    if (psocn_db_real_query(&conn, query)) {
        /* Silently fail here (to the ship anyway), since this doesn't spell
           doom at all for the logged in user (although, it might spell some
           inconvenience, potentially) */
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_opts;
    }

    /* Grab any results we got */
    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_opts;
    }

    /* Begin the options packet */
    optpkt = user_options_begin(gc, bl);

    /* Loop through each option to add it to the packet */
    while ((row = psocn_db_result_fetch(result))) {
        lengths = psocn_db_result_lengths(result);
        opt = (uint32_t)strtoul(row[0], NULL, 0);

        optpkt = user_options_append(optpkt, opt, (uint32_t)lengths[1],
            (uint8_t*)row[1]);
    }

    psocn_db_result_free(result);

    /* We're done, send it */
    send_user_options(c);

skip_opts:
    /* See if the user has an account or not. */
    sprintf(query, "SELECT account_id FROM %s WHERE guildcard='%"
        PRIu32 "'", CLIENTS_GUILDCARDS, gc);

    /* Query for any results */
    if (psocn_db_real_query(&conn, query)) {
        /* Silently fail here (to the ship anyway), since this doesn't spell
           doom at all for the logged in user (although, it might spell some
           inconvenience, potentially) */
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_mail;
    }

    /* Grab any results we got */
    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_mail;
    }

    /* Find the account_id, if any. */
    if (!(row = psocn_db_result_fetch(result)) || !row[0])
        goto skip_mail;

    acc = (uint32_t)strtoul(row[0], NULL, 0);
    psocn_db_result_free(result);

    /* See whether the user has any saved mail. */
    sprintf(query, "SELECT COUNT(*) FROM %s INNER JOIN %s ON "
        "%s.recipient = %s.guildcard WHERE "
        "%s.account_id='%" PRIu32 "' AND %s.status='0'"
        , SERVER_SIMPLE_MAIL, CLIENTS_GUILDCARDS
        , SERVER_SIMPLE_MAIL, CLIENTS_GUILDCARDS
        , CLIENTS_GUILDCARDS, acc, SERVER_SIMPLE_MAIL
    );

    /* Query for any results */
    if (psocn_db_real_query(&conn, query)) {
        /* Silently fail here (to the ship anyway), since this doesn't spell
           doom for the logged in user */
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_mail;
    }

    /* Grab any results we got */
    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_mail;
    }

    /* Look at the number from the result set. */
    row = psocn_db_result_fetch(result);
    opt = (uint32_t)strtoul(row[0], NULL, 0);

    /* Do they have any mail waiting for them? */
    if (opt) {
        if (opt > 1)
            sprintf(query, "\tEYou have %" PRIu32 " unread messages. Please "
                "visit the server website to read your mail.", opt);
        else
            sprintf(query, "\tEYou have an unread message. Please visit the "
                "server website to read your mail.");

        send_simple_mail(c, gc, bl, 2, "\tESys.Message", query);
            /*
            sprintf(query, "\tE您有 %" PRIu32 " 条未读邮件,请访问服务器网站阅读您的邮件.", opt);
        else
            sprintf(query, "\tE您有一条未读邮件,请访问服务器网站阅读您的邮件.");

        send_simple_mail(c, gc, bl, 2, "系统.短消息", query);*/
    }

    psocn_db_result_free(result);

    /* Set the flag to say that the user has been notified about any messages
       that happen to have been mentioned above. There would be a potentially
       race condition here between the select above and the update here, if we
       were multithreaded. However, shipgate isn't multithreaded, and even if it
       were, I don't think anyone will really care if the count up there is off
       by that slight of an amount... */
    sprintf(query, "UPDATE %s INNER JOIN %s ON "
        "%s.recipient = %s.guildcard SET "
        "%s.status='2' WHERE %s.account_id='%" PRIu32
        "' AND %s.status='0'"
        , SERVER_SIMPLE_MAIL, CLIENTS_GUILDCARDS
        , SERVER_SIMPLE_MAIL, CLIENTS_GUILDCARDS
        , SERVER_SIMPLE_MAIL, CLIENTS_GUILDCARDS
        , acc, SERVER_SIMPLE_MAIL
    );

    /* Do the update. */
    if (psocn_db_real_query(&conn, query)) {
        /* Silently fail here (to the ship anyway), since this doesn't spell
           doom for the logged in user */
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_mail;
    }

skip_mail:
    /* If there's an event ongoing, make sure the user isn't disqualified (or
       has already been nofified of their disqualification).
       TODO: Figure out a better way of searching, since this limits us to one
             ongoing event at a time... */
    if (!(ev = find_current_event(0, 0, 1)))
        goto skip_event;

    /* See if they're disqualified (and haven't been notified). */
    sprintf(query, "SELECT account_id FROM %s WHERE "
        "account_id='%" PRIu32 "' AND event_id='%" PRIu32 "' AND flags='0'"
        , SERVER_EVENT_MONSTER_DISQ
        , acc, ev->event_id);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Couldn't query if disqualified (%" PRIu32 ")", acc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_event;
    }

    /* Grab any data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't store disqualification (%" PRIu32 ")", acc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_event;
    }

    /* If there's a result row, then they're disqualified... */
    if ((row = psocn_db_result_fetch(result)) && row[0]) {
        send_simple_mail(c, gc, bl, 2, "\tESys.Message", "\tEYou have been "
            "disqualified from the current event for violating "
            "the rules of the event.");
        //send_simple_mail(c, gc, bl, 2, "系统.短消息", "您因违反活动规则而被取消当前活动的资格.");

        sprintf(query, "UPDATE %s SET flags='1' WHERE "
            "event_id='%" PRIu32 "' AND account_id='%" PRIu32 "'"
            , SERVER_EVENT_MONSTER_DISQ
            , ev->event_id, acc);

        /* Do the update. */
        if (psocn_db_real_query(&conn, query)) {
            /* Silently fail here (to the ship anyway), since this doesn't spell
               doom for the logged in user */
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
    }

    /* We don't actually care about the content of the row... If we get this
       far, then we should be good to go... */
    psocn_db_result_free(result);

skip_event:
    /* Skip the client's blocklist if the ship is running earlier than protocol
       version 19. */
    if (c->proto_ver < 19)
        goto skip_bl;

    /* See if they've got anyone on their list */
    sprintf(query, "SELECT blocked_gc, flags FROM %s WHERE "
        "account_id='%" PRIu32 "'", SERVER_BLOCKS_LIST, acc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Couldn't query for blocklist (%" PRIu32 ")", acc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_bl;
    }

    /* Grab any data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't store blocklist (%" PRIu32 ")", acc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto skip_bl;
    }

    /* Put together the blocklist packet */
    user_blocklist_begin(gc, bl);

    while ((row = psocn_db_result_fetch(result))) {
        gc2 = (uint32_t)strtoul(row[0], NULL, 0);
        bl2 = (uint32_t)strtoul(row[1], NULL, 0);
        user_blocklist_append(gc2, bl2);
    }

    psocn_db_result_free(result);

    /* We're done, send it */
    send_user_blocklist(c);

skip_bl:
    /* We're done (no need to tell the ship on success) */
    return 0;
}

static int handle_blocklogout(ship_t* c, shipgate_block_login_pkt* pkt) {
    char query[512];
    char name[32];
    uint32_t gc, bl, gc2, bl2;
    uint16_t ship_id;
    ship_t* c2;
    void* result;
    char** row;
    size_t in, out;
    char* inptr;
    char* outptr;

    /* Is the name a Blue Burst-style (UTF-16) name or not? */
    if (pkt->ch_name[0] == '\t') {
        memset(name, 0, 32);
        in = 32;
        out = 32;
        inptr = (char*)pkt->ch_name[2];
        outptr = name;

        istrncpy16_raw(ic_utf16_to_utf8, outptr, &pkt->ch_name[2], out, in);

        //iconv(ic_utf16_to_gbk, &inptr, &in, &outptr, &out);
    }
    else {
        /* Make sure the name is terminated properly */
        if (pkt->ch_name[31] != '\0') {
            /* Maybe we should send an error here... Probably not worth it. */
            return 0;
        }

        /* The name is ASCII, which is safe to use as UTF-8 */
        strcpy(name, pkt->ch_name);
    }

    /* Parse out some stuff we'll use */
    gc = ntohl(pkt->guildcard);
    bl = ntohl(pkt->blocknum);

    /* Is this a transient client (that is to say someone on the PC NTE)? */
    if (gc >= 500 && gc < 600) {
        /* Delete the client from the transient_clients table */
        sprintf(query, "DELETE FROM %s WHERE guildcard='%u' AND "
            "ship_id='%hu'", SERVER_CLIENTS_TRANSIENT, gc, c->key_idx);

        psocn_db_real_query(&conn, query);

        /* There's nothing else to do with PC NTE clients, so skip the rest. */
        return 0;
    }

    /* Delete the client from the online_clients table */
    sprintf(query, "DELETE FROM %s WHERE guildcard='%u' AND "
        "ship_id='%hu'", SERVER_CLIENTS_ONLINE, gc, c->key_idx);

    if (psocn_db_real_query(&conn, query)) {
        return 0;
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '%d', lastblock = '%d' WHERE guildcard = '%u'",
        CHARACTER_DATA, 0, bl, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("更新GC %u 数据错误:\n %s", gc, psocn_db_error(&conn));
        return 0;
    }

    /* Find anyone that has the user in their friendlist so we can send a
       message to them */
    sprintf(query, "SELECT guildcard, block, ship_id, nickname FROM "
        "%s INNER JOIN %s ON "
        "%s.guildcard = %s.owner WHERE "
        "%s.friend = '%u'"
        , SERVER_CLIENTS_ONLINE, CHARACTER_DATA_FRIENDLIST
        , SERVER_CLIENTS_ONLINE, CHARACTER_DATA_FRIENDLIST
        , CHARACTER_DATA_FRIENDLIST, gc
    );

    /* Query for any results */
    if (psocn_db_real_query(&conn, query)) {
        /* Silently fail here (to the ship anyway), since this doesn't spell
           doom at all for the logged in user */
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab any results we got */
    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* For each bite we get, send out a friend logout packet */
    while ((row = psocn_db_result_fetch(result))) {
        gc2 = (uint32_t)strtoul(row[0], NULL, 0);

        if (check_user_blocklist(gc2, gc, BLOCKLIST_FLIST))
            continue;

        bl2 = (uint32_t)strtoul(row[1], NULL, 0);
        ship_id = (uint16_t)strtoul(row[2], NULL, 0);
        c2 = find_ship(ship_id);

        if (c2) {
            send_friend_message(c2, 0, gc2, bl2, gc, bl, c->key_idx, name,
                row[3]);
        }
    }

    psocn_db_result_free(result);

    /* We're done (no need to tell the ship on success) */
    return 0;
}

static int handle_friendlist_add(ship_t* c, shipgate_friend_add_pkt* pkt) {
    uint32_t ugc, fgc;
    char query[256];
    char nickname[64];

    /* Make sure the length is sane */
    if (pkt->hdr.pkt_len != htons(sizeof(shipgate_friend_add_pkt))) {
        return -1;
    }

    /* Parse out the guildcards */
    ugc = ntohl(pkt->user_guildcard);
    fgc = ntohl(pkt->friend_guildcard);

    /* Is this a transient client (that is to say someone on the PC NTE)? */
    if ((ugc >= 500 && ugc < 600) || (fgc >= 500 && fgc < 600)) {
        /* There's no good way to support this, so respond with an error. */
        return send_error(c, SHDR_TYPE_ADDFRIEND, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->user_guildcard, 8);
    }

    /* Escape the name string */
    pkt->friend_nick[31] = 0;
    psocn_db_escape_str(&conn, nickname, pkt->friend_nick,
        strlen(pkt->friend_nick));

    /* Build the db query */
    sprintf(query, "INSERT INTO %s(owner, friend, nickname) "
        "VALUES('%u', '%u', '%s')", CHARACTER_DATA_FRIENDLIST, ugc, fgc, nickname);

    /* Execute the query */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return send_error(c, SHDR_TYPE_ADDFRIEND, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->user_guildcard, 8);
    }

    /* Return success to the ship */
    return send_error(c, SHDR_TYPE_ADDFRIEND, SHDR_RESPONSE, ERR_NO_ERROR,
        (uint8_t*)&pkt->user_guildcard, 8);
}

static int handle_friendlist_del(ship_t* c, shipgate_friend_upd_pkt* pkt) {
    uint32_t ugc, fgc;
    char query[256];

    /* Make sure the length is sane */
    if (pkt->hdr.pkt_len != htons(sizeof(shipgate_friend_upd_pkt))) {
        return -1;
    }

    /* Parse out the guildcards */
    ugc = ntohl(pkt->user_guildcard);
    fgc = ntohl(pkt->friend_guildcard);

    /* Build the db query */
    sprintf(query, "DELETE FROM %s WHERE owner='%u' AND friend='%u'"
        , CHARACTER_DATA_FRIENDLIST, ugc, fgc);

    /* Execute the query */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return send_error(c, SHDR_TYPE_DELFRIEND, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->user_guildcard, 8);
    }

    /* Return success to the ship */
    return send_error(c, SHDR_TYPE_DELFRIEND, SHDR_RESPONSE, ERR_NO_ERROR,
        (uint8_t*)&pkt->user_guildcard, 8);
}

static int handle_lobby_chg(ship_t* c, shipgate_lobby_change_pkt* pkt) {
    char query[512];
    char tmp[128];
    uint32_t gc, lid;
    const char* tbl_nm = SERVER_CLIENTS_ONLINE;

    /* Make sure the name is terminated properly */
    pkt->lobby_name[31] = 0;

    /* Parse out some stuff we'll use */
    gc = ntohl(pkt->guildcard);
    lid = ntohl(pkt->lobby_id);

    /* Update the client's entry */
    psocn_db_escape_str(&conn, tmp, pkt->lobby_name,
        strlen(pkt->lobby_name));

    /* Is this a transient client (that is to say someone on the PC NTE)? */
    if (gc >= 500 && gc < 600)
        tbl_nm = SERVER_CLIENTS_TRANSIENT;

    if (lid > 20) {
        sprintf(query, "UPDATE %s SET lobby_id='%u', lobby='%s' "
            "WHERE guildcard='%u' AND ship_id='%hu'", tbl_nm, lid, tmp, gc,
            c->key_idx);
    }
    else {
        sprintf(query, "UPDATE %s SET lobby_id='%u', lobby='%s', "
            "dlobby_id='%u' WHERE guildcard='%u' AND ship_id='%hu'", tbl_nm,
            lid, tmp, lid, gc, c->key_idx);
    }

    /* This shouldn't ever "fail" so to speak... */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* We're done (no need to tell the ship on success) */
    return 0;
}

static int handle_clients(ship_t* c, shipgate_block_clients_pkt* pkt) {
    char query[512];
    char tmp[128], tmp2[128], name[64];
    uint32_t gc, lid, dlid, count, bl, i;
    uint16_t len;
    size_t in, out;
    char* inptr;
    char* outptr;
    const char* tbl_nm;

    /* Verify the length is right */
    count = ntohl(pkt->count);
    len = ntohs(pkt->hdr.pkt_len);

    //SHIPS_LOG("handle_clients %d %d", count, len);

    if (len != 16 + count * 80 || count < 1) {
        ERR_LOG("接收到无效的块客户端数据包");
        return -1;
    }

    /* Grab the global stuff */
    bl = ntohl(pkt->block);

    //SHIPS_LOG("handle_clients %d %d block %d", count, len, bl);

    /* Make sure there's nothing for this ship/block in the db */
    sprintf(query, "DELETE FROM %s WHERE ship_id='%hu' AND "
        "block='%u'", SERVER_CLIENTS_ONLINE, c->key_idx, bl);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    sprintf(query, "DELETE FROM %s WHERE ship_id='%hu' AND "
        "block='%u'", SERVER_CLIENTS_TRANSIENT, c->key_idx, bl);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Run through each entry */
    for (i = 0; i < count; ++i) {
        /* Is the name a Blue Burst-style (UTF-16) name or not? */
        if (pkt->entries[i].ch_name[0] == '\t') {
            memset(name, 0, 64);
            in = 32;
            out = 64;
            inptr = (char*)&pkt->entries[i].ch_name[2];
            outptr = name;

            istrncpy16_raw(ic_utf16_to_utf8, outptr, &pkt->entries[i].ch_name[2], out, in);

            //iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &out);
        }
        else {
            /* Make sure the name is terminated properly */
            if (pkt->entries[i].ch_name[31] != '\0') {
                continue;
            }

            /* The name is ASCII, which is safe to use as UTF-8 */
            strcpy(name, pkt->entries[i].ch_name);
        }

        /* Make sure the names look sane */
        if (pkt->entries[i].lobby_name[31]) {
            continue;
        }

        /* Grab the integers out */
        gc = ntohl(pkt->entries[i].guildcard);
        lid = ntohl(pkt->entries[i].lobby);
        dlid = ntohl(pkt->entries[i].dlobby);

        /* Is this a transient client (that is to say someone on the PC NTE)? */
        if (gc >= 500 && gc < 600)
            tbl_nm = SERVER_CLIENTS_TRANSIENT;
        else
            tbl_nm = SERVER_CLIENTS_ONLINE;

        /* Escape the name string */
        psocn_db_escape_str(&conn, tmp, name, strlen(name));

        /* If we're not in a lobby, that's all we need */
        if (lid == 0) {
            sprintf(query, "INSERT INTO %s(guildcard, name, "
                "ship_id, block) VALUES('%u', '%s', '%hu', '%u')", tbl_nm,
                gc, tmp, c->key_idx, bl);
        }
        else {
            psocn_db_escape_str(&conn, tmp2, pkt->entries[i].lobby_name,
                strlen(pkt->entries[i].lobby_name));
            sprintf(query, "INSERT INTO %s(guildcard, name, "
                "ship_id, block, lobby_id, lobby, dlobby_id) VALUES('%u', "
                "'%s', '%hu', '%u', '%u', '%s', '%u')", tbl_nm, gc, tmp,
                c->key_idx, bl, lid, tmp2, dlid);
            //SHIPS_LOG("handle_clients gc %u %d %d block %d tmp %s", gc, count, len, bl, tmp);
        }

        /* Run the query */
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("%s", psocn_db_error(&conn));
            continue;
        }
    }

    /* We're done (no need to tell the ship on success) */
    return 0;
}

static int handle_kick(ship_t* c, shipgate_kick_pkt* pkt) {
    uint32_t gc, gcr, bl;
    uint16_t sid;
    char query[256];
    void* result;
    char** row;
    ship_t* c2;
    int priv, priv2;

    /* Parse out what we care about */
    gcr = ntohl(pkt->requester);
    gc = ntohl(pkt->guildcard);

    /* Make sure the requester is a GM */
    sprintf(query, "SELECT privlevel FROM %s NATURAL JOIN %s "
        "WHERE privlevel>'1' AND guildcard='%u'"
        , AUTH_DATA_ACCOUNT, CLIENTS_GUILDCARDS
        , gcr);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data from the DB */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't fetch GM data (%u)", gcr);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return 0;
    }

    /* Make sure we actually have a row, if not the ship is possibly trying to
       trick us into giving someone without GM privileges GM abilities... */
    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("Failed kick - not gm (gc: %u ship: %hu)", gcr,
            c->key_idx);

        return -1;
    }

    /* Grab the privilege level of the GM doing the kick */
    priv = atoi(row[0]);

    /* We're done with the data we got */
    psocn_db_result_free(result);

    /* Make sure the user isn't trying to kick someone with a higher privilege
       level than them... */
    sprintf(query, "SELECT privlevel FROM %s NATURAL JOIN %s "
        "WHERE guildcard='%u'", CLIENTS_GUILDCARDS, AUTH_DATA_ACCOUNT
        , gc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Couldn't fetch account data (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't fetch account data (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return 0;
    }

    /* See if we got anything */
    if ((row = psocn_db_result_fetch(result))) {
        priv2 = atoi(row[0]);

        if (priv2 >= priv) {
            psocn_db_result_free(result);
            SQLERR_LOG("Attempt by %u to kick %u overturned by priv",
                gcr, gc);

            return 0;
        }
    }

    /* We're done with that... */
    psocn_db_result_free(result);

    /* Now that we're done with that, work on the kick */
    sprintf(query, "SELECT ship_id, block FROM %s WHERE "
        "guildcard='%u'", SERVER_CLIENTS_ONLINE, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data from the DB */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't fetch online data (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return 0;
    }

    /* Grab the location of the user. If the user's not on, silently fail */
    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        return 0;
    }

    /* If we're here, we have a row, so parse out what we care about */
    errno = 0;
    sid = (uint16_t)strtoul(row[0], NULL, 0);
    bl = (uint32_t)strtoul(row[1], NULL, 0);
    psocn_db_result_free(result);

    if (errno != 0) {
        ERR_LOG("无效 %s 数据: %s", SERVER_CLIENTS_ONLINE, strerror(errno));
        return 0;
    }

    /* Grab the ship we need to send this to */
    if (!(c2 = find_ship(sid))) {
        ERR_LOG("无效舰船?!?");
        return -1;
    }

    /* Send off the message */
    send_kick(c2, gcr, gc, bl, pkt->reason);
    return 0;
}

static int handle_frlist_req(ship_t* c, shipgate_friend_list_req* pkt) {
    uint32_t gcr, gcf, block, start;
    char query[256];
    void* result;
    char** row;
    friendlist_data_t entries[5];
    int i;

    /* Parse out what we need */
    gcr = ntohl(pkt->requester);
    block = ntohl(pkt->block);
    start = ntohl(pkt->start);

    /* Grab the friendlist data */
    sprintf(query, "SELECT friend, nickname, ship_id, block FROM %s "
        "LEFT OUTER JOIN %s ON %s.friend = "
        "%s.guildcard WHERE owner='%u' ORDER BY friend "
        "LIMIT 5 OFFSET %u"
        , CHARACTER_DATA_FRIENDLIST
        , SERVER_CLIENTS_ONLINE, CHARACTER_DATA_FRIENDLIST
        , SERVER_CLIENTS_ONLINE
        , gcr, start);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法为 %u 选择好友列表", gcr);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data from the DB */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法获取 %u 好友列表", gcr);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return 0;
    }

    /* Fetch our max of 5 entries... i will be left as the number found */
    for (i = 0; i < 5 && (row = psocn_db_result_fetch(result)); ++i) {
        gcf = (uint32_t)strtoul(row[0], NULL, 0);
        entries[i].guildcard = htonl(gcf);

        /* Make sure the user isn't blocked from seeing this person. */
        if (check_user_blocklist(gcr, gcf, BLOCKLIST_FLIST)) {
            entries[i].ship = 0;
            entries[i].block = 0;
        }
        else {
            if (row[2]) {
                entries[i].ship = htonl(strtoul(row[2], NULL, 0));
            }
            else {
                entries[i].ship = 0;
            }

            if (row[3]) {
                entries[i].block = htonl(strtoul(row[3], NULL, 0));
            }
            else {
                entries[i].block = 0;
            }
        }

        entries[i].reserved = 0;

        strncpy(entries[i].name, row[1], 31);
        entries[i].name[31] = 0;
    }

    /* We're done with that, so clean up */
    psocn_db_result_free(result);

    /* Send the packet to the user */
    send_friendlist(c, gcr, block, i, entries);

    return 0;
}

static int handle_globalmsg(ship_t* c, shipgate_global_msg_pkt* pkt) {
    uint32_t gcr;
    uint16_t text_len;
    char query[256];
    void* result;
    char** row;
    ship_t* i;

    /* Parse out what we really need */
    gcr = ntohl(pkt->requester);
    text_len = ntohs(pkt->hdr.pkt_len) - sizeof(shipgate_global_msg_pkt);

    /* Make sure the string is NUL terminated */
    if (pkt->text[text_len - 1]) {
        ERR_LOG("非终止全局消息 (%hu)", c->key_idx);
        return 0;
    }

    /* Make sure the requester is a GM */
    sprintf(query, "SELECT privlevel FROM %s NATURAL JOIN %s "
        "WHERE privlevel>'1' AND guildcard='%u'"
        , AUTH_DATA_ACCOUNT, CLIENTS_GUILDCARDS
        , gcr);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data from the DB */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法获取 GM 数据 (%u)", gcr);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return 0;
    }

    /* Make sure we actually have a row, if not the ship is possibly trying to
       trick us into giving someone without GM privileges GM abilities... */
    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("失败的全局消息 - 未找到 GM (gc: %u ship: %hu)", gcr,
            c->key_idx);

        return -1;
    }

    /* We're done with that... */
    psocn_db_result_free(result);

    /* Send the packet along to all the ships that support it */
    TAILQ_FOREACH(i, &ships, qentry) {
        if (send_global_msg(i, gcr, pkt->text, text_len)) {
            i->disconnected = 1;
        }
    }

    return 0;
}

static int handle_useropt(ship_t* c, shipgate_user_opt_pkt* pkt) {
    char data[512];
    char query[1024];
    uint16_t len = htons(pkt->hdr.pkt_len);
    uint32_t ugc, optlen, opttype;
    int realoptlen;

    /* Make sure the length is sane */
    if (len < sizeof(shipgate_user_opt_pkt) + 16) {
        return -2;
    }

    /* Parse out the guildcard */
    ugc = ntohl(pkt->guildcard);
    optlen = ntohl(pkt->options[0].length);
    opttype = ntohl(pkt->options[0].option);

    /* Make sure the length matches up properly */
    if (optlen != len - sizeof(shipgate_user_opt_pkt)) {
        return -3;
    }

    /* Handle each option separately */
    switch (opttype) {
    case USER_OPT_QUEST_LANG:
    case USER_OPT_ENABLE_BACKUP:
    case USER_OPT_GC_PROTECT:
    case USER_OPT_TRACK_KILLS:
    case USER_OPT_LEGIT_ALWAYS:
    case USER_OPT_WORD_CENSOR:
        /* The full option should be 16 bytes */
        if (optlen != 16) {
            return -4;
        }

        /* However, we only pay attention to the first byte */
        realoptlen = 1;
        break;

    default:
        ERR_LOG("舰仓发送未知用户选项: %lu", opttype);
        return -5;
    }

    /* Escape the data */
    psocn_db_escape_str(&conn, data, (const char*)pkt->options[0].data,
        realoptlen);

    /* Build the db query... This uses a MySQL extension, so will have to be
       fixed if any other DB type is to be supported... */
    sprintf(query, "INSERT INTO %s(guildcard, opt, value) "
        "VALUES('%u', '%u', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", CLIENTS_OPTIONS, ugc, opttype, data);

    /* Execute the query */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return send_error(c, SHDR_TYPE_USEROPT, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 24);
    }

    /* Return success to the ship */
    return send_error(c, SHDR_TYPE_USEROPT, SHDR_RESPONSE, ERR_NO_ERROR,
        (uint8_t*)&pkt->guildcard, 24);
}

static int handle_bbopt_req(ship_t* c, shipgate_bb_opts_req_pkt* pkt) {
    uint32_t gc, block;
    psocn_bb_db_opts_t opts;
    psocn_bb_db_guild_t guild;
    int rv = 0;

    /* Parse out the guildcard */
    gc = ntohl(pkt->guildcard);
    block = ntohl(pkt->block);

    opts = db_get_bb_char_option(gc);
    guild = db_get_bb_char_guild(gc);

    rv = send_bb_opts(c, gc, block, &opts, &guild);

    if (rv) {
        rv = send_error(c, SHDR_TYPE_BBOPTS, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    return rv;
}

static int handle_bbopts(ship_t* c, shipgate_bb_opts_pkt* pkt) {
    uint32_t gc;
    int rv = 0;

    /* Parse out the guildcard */
    gc = ntohl(pkt->guildcard);

    rv = db_update_bb_char_option(pkt->opts, gc);

    if (rv) {
        rv = send_error(c, SHDR_TYPE_BBOPTS, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    return rv;
}
//
//static int handle_bbguild_req(ship_t* c, shipgate_bb_guild_req_pkt* pkt) {
//    uint32_t gc, block;
//    psocn_bb_db_guild_t guild;
//    int rv = 0;
//
//    /* Parse out the guildcard */
//    gc = ntohl(pkt->guildcard);
//    block = ntohl(pkt->block);
//
//    memset(&guild, 0, sizeof(psocn_bb_db_guild_t));
//
//    guild = db_get_bb_char_guild(gc);
//
//    rv = send_bb_guild(c, gc, block, &guild);
//
//    if (rv) {
//        rv = send_error(c, SHDR_TYPE_BBGUILD, SHDR_FAILURE, ERR_BAD_ERROR,
//            (uint8_t*)&pkt->guildcard, 8);
//    }
//
//    return rv;
//}
//
//static int handle_bbguild(ship_t* c, shipgate_bb_guild_pkt* pkt) {
//    uint32_t gc;
//    int rv = 0;
//
//    /* Parse out the guildcard */
//    gc = ntohl(pkt->guildcard);
//
//    rv = db_update_bb_char_guild(pkt->guild, gc);
//
//    if (rv) {
//        rv = send_error(c, SHDR_TYPE_BBGUILD, SHDR_FAILURE, ERR_BAD_ERROR,
//            (uint8_t*)&pkt->guildcard, 8);
//    }
//
//    return rv;
//}

static int handle_mkill(ship_t* c, shipgate_mkill_pkt* pkt) {
    char query[256];
    uint32_t gc, ct, acc;
    int i;
    void* result;
    char** row;
    monster_event_t* ev;

    /* Ignore any packets that aren't version 1 or later. They're useless. */
    if (pkt->hdr.version < 1)
        return 0;

    /* See if there's an event currently running, otherwise we can safely drop
       any monster kill packets we get. */
    if (!(ev = find_current_event(pkt->difficulty, pkt->version, 0)))
        return 0;

    /* Parse out the guildcard */
    gc = ntohl(pkt->guildcard);

    /* Find the user's account id */
    sprintf(query, "SELECT account_id FROM %s WHERE guildcard='%"
        PRIu32 "'", CLIENTS_GUILDCARDS, gc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Couldn't fetch account data (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_MKILL, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 16);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't fetch account data (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_MKILL, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("Couldn't fetch account data (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_MKILL, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    /* If their account id in the table is NULL, then bail. No need to report an
       error for this. */
    if (!row[0]) {
        psocn_db_result_free(result);
        return 0;
    }

    /* We've verified they've got an account, continue on. */
    acc = atoi(row[0]);
    psocn_db_result_free(result);

    /* Make sure they're not disqualified from the event... */
    sprintf(query, "SELECT account_id FROM %s WHERE "
        "account_id='%" PRIu32 "' AND event_id='%" PRIu32 "'"
        , SERVER_EVENT_MONSTER_DISQ
        , acc, ev->event_id);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Couldn't query if disqualified (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_MKILL, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 16);
    }

    /* Grab any data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't store disqualification (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_MKILL, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    /* If there's a result row, then they're disqualified... */
    if ((row = psocn_db_result_fetch(result)) && row[0]) {
        SGATE_LOG("Rejecting monster kill update for disqualified player "
            "%" PRIu32 " (gc %" PRIu32 ")", acc, gc);
        psocn_db_result_free(result);
        return 0;
    }

    /* We don't actually care about the content of the row... If we get this
       far, then we should be good to go... */
    psocn_db_result_free(result);

    /* Are we recording all monsters, or just a few? */
    if (ev->monster_count) {
        for (i = 0; i < (int)ev->monster_count; ++i) {
            if (ev->monsters[i].monster > 0x60)
                continue;

            ct = ntohl(pkt->counts[ev->monsters[i].monster]);

            if (!ct || pkt->episode != ev->monsters[i].episode)
                continue;

            sprintf(query, "INSERT INTO %s (event_id, account_id, "
                " guildcard, episode, difficulty, enemy, count) VALUES('%"
                PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%u', '%u', '%d', "
                "'%" PRIu32"') ON DUPLICATE KEY UPDATE "
                "count=count+VALUES(count)", SERVER_EVENT_MONSTER_KILLS, ev->event_id, acc, gc,
                (unsigned int)pkt->episode, (unsigned int)pkt->difficulty,
                ev->monsters[i].monster, ct);

            /* Execute the query */
            if (psocn_db_real_query(&conn, query)) {
                SQLERR_LOG("%s", psocn_db_error(&conn));
                return send_error(c, SHDR_TYPE_MKILL, SHDR_FAILURE,
                    ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
            }
        }

        return 0;
    }

    /* Go through each entry... */
    for (i = 0; i < 0x60; ++i) {
        ct = ntohl(pkt->counts[i]);

        if (!ct)
            continue;

        sprintf(query, "INSERT INTO %s (event_id, account_id, "
            " guildcard, episode, difficulty, enemy, count) VALUES('%"
            PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%u', '%u', '%zd', "
            "'%" PRIu32"') ON DUPLICATE KEY UPDATE "
            "count=count+VALUES(count)", SERVER_EVENT_MONSTER_KILLS
            , ev->event_id, acc, gc, (unsigned int)pkt->episode, (unsigned int)pkt->difficulty, i
            , ct);

        /* Execute the query */
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("%s", psocn_db_error(&conn));
            return send_error(c, SHDR_TYPE_MKILL, SHDR_FAILURE, ERR_BAD_ERROR,
                (uint8_t*)&pkt->guildcard, 8);
        }
    }

    return 0;
}

/* 处理 token-based user login request. */
static int handle_tlogin(ship_t* c, shipgate_usrlogin_req_pkt* pkt) {
    uint32_t gc, block;
    char query[512];
    void* result;
    char** row;
    char esc[65], esc2[65];
    uint16_t len;
    uint32_t priv;
    uint32_t account_id;

    /* Clear old requests from the table. */
    sprintf(query, "DELETE FROM %s WHERE req_time + INTERVAL 10 "
        "MINUTE < NOW()", AUTH_DATA_LOGIN_TOKENS);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Couldn't clear old tokens!");
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    /* Check the sanity of the packet. Disconnect the ship if there's some odd
       issue with the packet's sanity. */
    len = ntohs(pkt->hdr.pkt_len);
    if (len != sizeof(shipgate_usrlogin_req_pkt)) {
        SQLERR_LOG("Ship %s sent invalid token Login!?", c->name);
        return -1;
    }

    if (pkt->username[31] != '\0' || pkt->password[31] != '\0') {
        SQLERR_LOG("Ship %s sent unterminated token Login", c->name);
        return -1;
    }

    /* Escape the username and grab the data we need. */
    psocn_db_escape_str(&conn, esc, pkt->username, strlen(pkt->username));
    psocn_db_escape_str(&conn, esc2, pkt->password, strlen(pkt->password));
    block = ntohl(pkt->block);
    gc = ntohl(pkt->guildcard);

    /* Build the query asking for the data. */
    if(pkt->hdr.version == TLOGIN_VER_NORMAL) {
        sprintf(query, "SELECT privlevel, account_id FROM %s NATURAL "
                "JOIN %s NATURAL JOIN %s WHERE "
                "guildcard='%u' AND username='%s' AND token='%s'"
				, AUTH_DATA_ACCOUNT
				, CLIENTS_GUILDCARDS, AUTH_DATA_LOGIN_TOKENS, gc
				, esc, esc2);
    }
    else {
        sprintf(query, "SELECT privlevel, %s.account_id FROM "
                "%s NATURAL JOIN %s, %s NATURAL "
                "JOIN %s WHERE username='%s' AND token='%s' AND "
                "guildcard='%u' AND %s.account_id is NULL", AUTH_DATA_ACCOUNT
            , AUTH_DATA_ACCOUNT, AUTH_DATA_LOGIN_TOKENS, CLIENTS_GUILDCARDS
            , CLIENTS_XBOX, esc, esc2
            , gc, CLIENTS_GUILDCARDS);
    }

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Couldn't lookup account data (user: %s, gc: %u)",
            pkt->username, gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("Couldn't fetch account data (user: %s, gc: %u)",
            pkt->username, gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("Failed token login (user: %s, gc: %u)",
            pkt->username, gc);

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE,
            ERR_USRLOGIN_BAD_CRED, (uint8_t*)&pkt->guildcard, 8);
    }

    /* Grab the privilege level out of the packet */
    errno = 0;
    priv = (uint32_t)strtoul(row[0], NULL, 0);

    if (errno != 0) {
        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE,
            ERR_USRLOGIN_BAD_PRIVS, (uint8_t*)&pkt->guildcard,
            8);
    }

    account_id = (uint32_t)strtoul(row[1], NULL, 0);

    /* Filter out any privileges that don't make sense. Can't have global GM
       without local GM support. Also, anyone set as a root this way must have
       BOTH root bits set, not just one! */
    if (((priv & CLIENT_PRIV_GLOBAL_GM) && !(priv & CLIENT_PRIV_LOCAL_GM)) ||
        ((priv & CLIENT_PRIV_GLOBAL_ROOT) && !(priv & CLIENT_PRIV_LOCAL_ROOT)) ||
        ((priv & CLIENT_PRIV_LOCAL_ROOT) && !(priv & CLIENT_PRIV_GLOBAL_ROOT))) {
        ERR_LOG("Invalid privileges for user %u: %02x", pkt->username,
            priv);
        psocn_db_result_free(result);

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    /* We're done if we got this far. */
    psocn_db_result_free(result);

    /* If this was a request to associate an XBL account, then do it. */
    if(pkt->hdr.version == TLOGIN_VER_XBOX) {
        sprintf(query, "UPDATE %s SET account_id='%u' WHERE "
                "guildcard='%u' AND account_id is NULL", CLIENTS_GUILDCARDS, account_id, gc);

        if(psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("Couldn't update guild card data (user: %s, "
                            "gc: %u)", pkt->username, gc);
            SQLERR_LOG("%s", psocn_db_error(&conn));

            return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE,
                              ERR_BAD_ERROR, (uint8_t *)&pkt->guildcard, 8);
        }
    }
	
    /* Delete the request. */
    sprintf(query, "DELETE FROM %s WHERE account_id='%u'"
        , AUTH_DATA_LOGIN_TOKENS, account_id);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Couldn't clear spent token!");
        SQLERR_LOG("%s", psocn_db_error(&conn));
    }

    /* The privilege field went to 32-bits in version 18. */
    if (c->proto_ver < 18) {
        priv &= (CLIENT_PRIV_LOCAL_GM | CLIENT_PRIV_GLOBAL_GM |
            CLIENT_PRIV_LOCAL_ROOT | CLIENT_PRIV_GLOBAL_ROOT);
    }

    /* Send a success message. */
    return send_usrloginreply(c, gc, block, 1, priv);
}

static int handle_schunk(ship_t* s, shipgate_schunk_err_pkt* pkt) {
    ship_script_t* scr;
    int module;

    /* Make sure it looks sane... */
    if (pkt->filename[31] || pkt->type > SCHUNK_TYPE_MODULE)
        return -1;

    if (ntohl(pkt->base.error_code) != ERR_SCHUNK_NEED_SCRIPT)
        return -1;

    module = pkt->type == SCHUNK_TYPE_MODULE;

    /* Find the script requested. */
    if (!(scr = find_script(pkt->filename, module))) {
        ERR_LOG("Ship requesting unknown script '%s'",
            pkt->filename);
        return -1;
    }

    return send_script(s, scr);
}

static int handle_sdata(ship_t* s, shipgate_sdata_pkt* pkt) {
    uint32_t event_id, data_len, guildcard, block;

    /* Parse data out of the packet */
    event_id = ntohl(pkt->event_id);
    data_len = ntohl(pkt->data_len);
    guildcard = ntohl(pkt->guildcard);
    block = ntohl(pkt->block);

    if (ntohs(pkt->hdr.pkt_len) < sizeof(shipgate_sdata_pkt) + data_len) {
        ERR_LOG("舰船发送了错误长度的舰船数据包");
        return -1;
    }

    return script_execute(ScriptActionSData, SCRIPT_ARG_PTR, s,
        SCRIPT_ARG_UINT32, s->key_idx, SCRIPT_ARG_UINT32,
        block, SCRIPT_ARG_UINT32, event_id, SCRIPT_ARG_UINT32,
        guildcard, SCRIPT_ARG_UINT8, pkt->episode,
        SCRIPT_ARG_UINT8, pkt->difficulty, SCRIPT_ARG_UINT8,
        pkt->version, SCRIPT_ARG_STRING, (size_t)data_len,
        pkt->data, SCRIPT_ARG_END);
}

static int handle_qflag_set(ship_t* c, shipgate_qflag_pkt* pkt) {
    char query[1024];
    uint32_t gc, block, flag_id, value, qid, ctl;

    /* Parse out the packet data */
    gc = ntohl(pkt->guildcard);
    block = ntohl(pkt->block);
    flag_id = ntohl(pkt->flag_id);
    ctl = flag_id & 0xFFFF0000;
    flag_id = (flag_id & 0x0000FFFF) | (ntohs(pkt->flag_id_hi) << 16);
    qid = ntohl(pkt->quest_id);
    value = ntohl(pkt->value);

    /* Build the db query */
    if (!(ctl & QFLAG_DELETE_FLAG)) {
        if (!(ctl & QFLAG_LONG_FLAG)) {
            sprintf(query, "INSERT INTO %s (guildcard, flag_id, "
                " value) VALUES('%" PRIu32 "', '%" PRIu32 "', '%" PRIu32
                "') ON DUPLICATE KEY UPDATE value=VALUES(value)", QUEST_FLAGS_SHORT, gc,
                flag_id, value & 0xFFFF);
        }
        else {
            sprintf(query, "INSERT INTO %s (guildcard, flag_id, "
                " value) VALUES('%" PRIu32 "', '%" PRIu32 "', '%" PRIu32
                "') ON DUPLICATE KEY UPDATE value=VALUES(value)", QUEST_FLAGS_LONG, gc,
                flag_id, value);
        }
    }
    else {
        if (!(ctl & QFLAG_LONG_FLAG)) {
            sprintf(query, "DELETE FROM %s WHERE guildcard='%"
                PRIu32 "' AND flag_id='%" PRIu32 "'", QUEST_FLAGS_SHORT, gc,
                flag_id);
        }
        else {
            sprintf(query, "DELETE FROM %s WHERE guildcard='%"
                PRIu32 "' AND flag_id='%" PRIu32 "'", QUEST_FLAGS_LONG, gc,
                flag_id);
        }
    }

    /* Execute the query */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return send_error(c, SHDR_TYPE_QFLAG_SET, SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 16);
    }

    return send_qflag(c, SHDR_TYPE_QFLAG_SET, gc, block, flag_id, qid,
        value, ctl);
}

static int handle_qflag_get(ship_t* c, shipgate_qflag_pkt* pkt) {
    char query[1024];
    uint32_t gc, block, flag_id, value, qid, ctl;
    void* result;
    char** row;

    /* Parse out the packet data */
    gc = ntohl(pkt->guildcard);
    block = ntohl(pkt->block);
    flag_id = ntohl(pkt->flag_id);
    ctl = flag_id & 0xFFFF0000;
    flag_id = (flag_id & 0x0000FFFF) | (ntohs(pkt->flag_id_hi) << 16);
    qid = ntohl(pkt->quest_id);

    /* Make sure the packet is sane... */
    if ((ctl & QFLAG_DELETE_FLAG)) {
        ERR_LOG("Ship sent delete flag bit on get flag packet!"
            "Dropping request.");
        return send_error(c, SHDR_TYPE_QFLAG_GET, SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 16);
    }

    /* Build the db query */
    if (!(ctl & QFLAG_LONG_FLAG)) {
        sprintf(query, "SELECT value FROM %s WHERE guildcard='%"
            PRIu32 "' AND flag_id='%" PRIu32 "'", QUEST_FLAGS_SHORT, gc, flag_id);
    }
    else {
        sprintf(query, "SELECT value FROM %s WHERE guildcard='%"
            PRIu32 "' AND flag_id='%" PRIu32 "'", QUEST_FLAGS_LONG, gc, flag_id);
    }

    /* Execute the query */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return send_error(c, SHDR_TYPE_QFLAG_GET, SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 16);
    }

    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return send_error(c, SHDR_TYPE_QFLAG_GET, SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 16);
    }

    if (!(row = psocn_db_result_fetch(result))) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        psocn_db_result_free(result);
        return send_error(c, SHDR_TYPE_QFLAG_GET, SHDR_FAILURE,
            ERR_QFLAG_NO_DATA, (uint8_t*)&pkt->guildcard,
            16);
    }

    value = (uint32_t)strtoul(row[0], NULL, 0);
    psocn_db_result_free(result);

    return send_qflag(c, SHDR_TYPE_QFLAG_GET, gc, block, flag_id, qid,
        value, ctl);
}

static int handle_sctl_uname(ship_t* c, shipgate_sctl_uname_reply_pkt* pkt,
    uint16_t len, uint16_t flags) {
    char str[65], esc[132];
    char query[1024];
    (void)len;
    (void)flags;

    /* Parse each part and insert it into the db. */
    memcpy(str, pkt->name, 64);
    str[64] = 0;
    psocn_db_escape_str(&conn, esc, str, strlen(str));
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_UNAME_NAME, esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法存储未命名舰船 '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    memcpy(str, pkt->node, 64);
    str[64] = 0;
    psocn_db_escape_str(&conn, esc, str, strlen(str));
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_UNAME_NODE, esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Cannot store uname node for ship '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    memcpy(str, pkt->release, 64);
    str[64] = 0;
    psocn_db_escape_str(&conn, esc, str, strlen(str));
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_UNAME_RELEASE,
        esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Cannot store uname release for ship '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    memcpy(str, pkt->version, 64);
    str[64] = 0;
    psocn_db_escape_str(&conn, esc, str, strlen(str));
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_UNAME_VERSION,
        esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Cannot store uname version for ship '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    memcpy(str, pkt->machine, 64);
    str[64] = 0;
    psocn_db_escape_str(&conn, esc, str, strlen(str));
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_UNAME_MACHINE,
        esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Cannot store uname machine for ship '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    return 0;
}

static int handle_sctl_version(ship_t* c, shipgate_sctl_ver_reply_pkt* pkt,
    uint16_t len, uint16_t flags) {
    uint8_t tmp[6], esc[43];
    char query[1024];
    uint8_t* esc2;
    (void)flags;

    tmp[0] = pkt->ver_major;
    tmp[1] = 0;
    tmp[2] = pkt->ver_minor;
    tmp[3] = 0;
    tmp[4] = pkt->ver_micro;
    tmp[5] = 0;
    psocn_db_escape_str(&conn, (char*)esc, (char*)tmp, 6);
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_VER_VERSION, esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法存储舰船版本 '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    tmp[0] = pkt->flags;
    tmp[1] = 0;
    tmp[2] = 0;
    tmp[3] = 0;
    psocn_db_escape_str(&conn, (char*)esc, (char*)tmp, 4);
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_VER_FLAGS, esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法存储舰船 flags '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    psocn_db_escape_str(&conn, (char*)esc, (char*)pkt->commithash, 20);
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_VER_CMT_HASH, esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Cannot store version hash for ship '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    psocn_db_escape_str(&conn, (char*)esc, (char*)&pkt->committime, 8);
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_VER_CMT_TIME, esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Cannot store version timestamp for ship '%s': %s",
            c->name, psocn_db_error(&conn));
    }

    len -= sizeof(shipgate_sctl_ver_reply_pkt) - 1;
    esc2 = (uint8_t*)malloc(len * 2 + 1);
    if (!esc2) {
        SQLERR_LOG("Cannot allocate for version ref for ship '%s': %s",
            c->name, strerror(errno));
        return -1;
    }

    psocn_db_escape_str(&conn, (char*)esc2, (char*)pkt->remoteref,
        my_strnlen(pkt->remoteref, len));
    sprintf(query, "INSERT INTO %s (ship_id, metadata_id, value) "
        "VALUES ('%" PRIu16 "', '%d', '%s') ON DUPLICATE KEY UPDATE "
        "value=VALUES(value)", SERVER_SHIPS_METADATA, c->key_idx, SHIP_METADATA_VER_CMT_REF, esc2);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("Cannot store version ref for ship '%s': %s",
            c->name, psocn_db_error(&conn));
    }
    free_safe(esc2);

    return 0;
}

static int handle_shipctl_reply(ship_t* c, shipgate_shipctl_pkt* pkt,
    uint16_t len, uint16_t flags) {
    uint32_t ctl;

    /* Make sure the packet is at least safe to parse */
    if (len < sizeof(shipgate_shipctl_pkt)) {
        ERR_LOG("%s 发送的 shipctl 回复太短", c->name);
        return -1;
    }

    if (!(flags & SHDR_RESPONSE)) {
        ERR_LOG("%s 发送未响应 shipctl", c->name);
        return -1;
    }
    else if ((flags & SHDR_FAILURE)) {
        ERR_LOG("%s 发送失败的 shipctl 响应", c->name);
        return -1;
    }

    /* Figure out what we're dealing with a response to */
    ctl = ntohl(pkt->ctl);

    //DBG_LOG("handle_shipctl_reply ctl = %d flags = %d", ctl, flags);

    switch (ctl) {
    case SCTL_TYPE_RESTART:
    case SCTL_TYPE_SHUTDOWN:
        ERR_LOG("%s 发送 关闭/重启 响应", c->name);
        return -1;

    case SCTL_TYPE_UNAME:
        if (pkt->hdr.version != 0) {
            ERR_LOG("%s 发送未知 sctl 未命名版本: %" PRIu8
                "", c->name, pkt->hdr.version);
            return -1;
        }

        if (len < sizeof(shipgate_sctl_uname_reply_pkt)) {
            ERR_LOG("%s 发送过短 sctl 未命名响应", c->name);
            return -1;
        }

        return handle_sctl_uname(c, (shipgate_sctl_uname_reply_pkt*)pkt,
            len, flags);

    case SCTL_TYPE_VERSION:
        if (pkt->hdr.version != 0) {
            ERR_LOG("%s 发送未知 sctl 版本号: %" PRIu8
                "", c->name, pkt->hdr.version);
            return -1;
        }

        if (len < sizeof(shipgate_sctl_ver_reply_pkt)) {
            ERR_LOG("%s 发送过短 sctl 版本响应", c->name);
            return -1;
        }

        return handle_sctl_version(c, (shipgate_sctl_ver_reply_pkt*)pkt,
            len, flags);

    default:
        ERR_LOG("%s 发送未知 shipctl (%" PRIu32 ")", c->name,
            ctl);
        return -1;
    }
}

static int handle_ubl_add(ship_t* c, shipgate_ubl_add_pkt* pkt) {
    char query[1024];
    char tmp[33], name[67];
    uint32_t gc, block, blocked, flags, acc;
    void* result;
    char** row;

    /* Parse out the packet data */
    gc = ntohl(pkt->requester);
    block = ntohl(pkt->block);
    blocked = ntohl(pkt->blocked_player);
    flags = ntohl(pkt->flags);

    /* See if the user has an account or not. */
    sprintf(query, "SELECT account_id FROM %s WHERE guildcard='%"
        PRIu32 "'", CLIENTS_GUILDCARDS, gc);

    /* Query for any results */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法查询帐户数据 gc(%" PRIu32 ") %s", gc
            , psocn_db_error(&conn));
        return send_user_error(c, SHDR_TYPE_UBL_ADD, ERR_BAD_ERROR, gc, block,
            NULL);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法获取帐户数据 gc(%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return send_user_error(c, SHDR_TYPE_UBL_ADD, ERR_BAD_ERROR, gc, block,
            NULL);
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        SQLERR_LOG("未找到帐户数据 gc(%" PRIu32 ")", gc);
        psocn_db_result_free(result);
        return send_user_error(c, SHDR_TYPE_UBL_ADD, ERR_BAD_ERROR, gc, block,
            NULL);
    }

    /* If their account id in the table is NULL, then bail... */
    if (!row[0]) {
        psocn_db_result_free(result);
        return send_user_error(c, SHDR_TYPE_UBL_ADD, ERR_REQ_LOGIN, gc, block,
            NULL);
    }

    /* We've verified they've got an account, continue on. */
    acc = atoi(row[0]);
    psocn_db_result_free(result);

    memcpy(tmp, pkt->blocked_name, 32);
    tmp[32] = 0;
    psocn_db_escape_str(&conn, name, tmp, strlen(tmp));

    sprintf(query, "INSERT INTO %s (account_id, blocked_gc, name, "
        "class, flags) VALUES ('%" PRIu32 "', '%" PRIu32 "', '%s', "
        "'%" PRIu8 "', '%" PRIu32 "')", SERVER_BLOCKS_LIST, acc, blocked, name,
        pkt->blocked_class, flags);

    /* Execute the query */
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return send_user_error(c, SHDR_TYPE_UBL_ADD, ERR_BAD_ERROR, gc, block,
            NULL);
    }

    /* Return success here, even if the row wasn't added (since the only reason
       it shouldn't get added is if there is a key conflict, which implies that
       the user was already blocked). */
    return send_user_error(c, SHDR_TYPE_UBL_ADD, ERR_NO_ERROR, gc, block, NULL);
}

/* Process one ship packet. */
int process_ship_pkt(ship_t* c, shipgate_hdr_t* pkt) {
    uint16_t length = ntohs(pkt->pkt_len);
    uint16_t type = ntohs(pkt->pkt_type);
    uint16_t flags = ntohs(pkt->flags);

    DBG_LOG("G->S指令: 0x%04X %s 标志 = %d 长度 = %d", type, s_cmd_name(type, 0), flags, length);
    //print_payload((unsigned char*)pkt, length);

    switch (type) {
    case SHDR_TYPE_LOGIN6:
    {
        shipgate_login6_reply_pkt* p = (shipgate_login6_reply_pkt*)pkt;

        if (!(flags & SHDR_RESPONSE)) {
            ERR_LOG("舰船发送了无效的登录响应");
            return -1;
        }

        return handle_shipgate_login6t(c, p);
    }

    case SHDR_TYPE_COUNT:
        return handle_count(c, (shipgate_cnt_pkt*)pkt);

    case SHDR_TYPE_DC:
        return handle_dreamcast(c, (shipgate_fw_9_pkt*)pkt);

    case SHDR_TYPE_PC:
        return handle_pc(c, (shipgate_fw_9_pkt*)pkt);

    case SHDR_TYPE_BB:
        return handle_bb(c, (shipgate_fw_9_pkt*)pkt);

    case SHDR_TYPE_PING:
        /* If this is a ping request, reply. Otherwise, ignore it, the work
           has already been done. */
        if (!(flags & SHDR_RESPONSE)) {
            return send_ping(c, 1);
        }

        return 0;

    case SHDR_TYPE_CDATA:
        return handle_cdata(c, (shipgate_char_data_pkt*)pkt);

    case SHDR_TYPE_CREQ:
        return handle_creq(c, (shipgate_char_req_pkt*)pkt);

    case SHDR_TYPE_USRLOGIN:
        return handle_usrlogin(c, (shipgate_usrlogin_req_pkt*)pkt);

    case SHDR_TYPE_GCBAN:
    case SHDR_TYPE_IPBAN:
        return handle_ban(c, (shipgate_ban_req_pkt*)pkt, type);

    case SHDR_TYPE_BLKLOGIN:
        return handle_blocklogin(c, (shipgate_block_login_pkt*)pkt);

    case SHDR_TYPE_BLKLOGOUT:
        return handle_blocklogout(c, (shipgate_block_login_pkt*)pkt);

    case SHDR_TYPE_ADDFRIEND:
        return handle_friendlist_add(c, (shipgate_friend_add_pkt*)pkt);

    case SHDR_TYPE_DELFRIEND:
        return handle_friendlist_del(c, (shipgate_friend_upd_pkt*)pkt);

    case SHDR_TYPE_LOBBYCHG:
        return handle_lobby_chg(c, (shipgate_lobby_change_pkt*)pkt);

    case SHDR_TYPE_BCLIENTS:
        return handle_clients(c, (shipgate_block_clients_pkt*)pkt);

    case SHDR_TYPE_KICK:
        return handle_kick(c, (shipgate_kick_pkt*)pkt);

    case SHDR_TYPE_FRLIST:
        return handle_frlist_req(c, (shipgate_friend_list_req*)pkt);

    case SHDR_TYPE_GLOBALMSG:
        return handle_globalmsg(c, (shipgate_global_msg_pkt*)pkt);

    case SHDR_TYPE_USEROPT:
        return handle_useropt(c, (shipgate_user_opt_pkt*)pkt);

    case SHDR_TYPE_BBOPTS:
        return handle_bbopts(c, (shipgate_bb_opts_pkt*)pkt);

    case SHDR_TYPE_BBOPT_REQ:
        return handle_bbopt_req(c, (shipgate_bb_opts_req_pkt*)pkt);

    case SHDR_TYPE_CBKUP:
        return handle_cbkup(c, (shipgate_char_bkup_pkt*)pkt);

    case SHDR_TYPE_MKILL:
        return handle_mkill(c, (shipgate_mkill_pkt*)pkt);

    case SHDR_TYPE_TLOGIN:
        return handle_tlogin(c, (shipgate_usrlogin_req_pkt*)pkt);

    case SHDR_TYPE_SCHUNK:
        /* Sanity check... */
        if (!(flags & SHDR_RESPONSE)) {
            ERR_LOG("舰船发送了脚本块?");
            return -1;
        }

        /* If it's a success response, don't bother doing anything */
        if (!(flags & SHDR_FAILURE))
            return 0;

        return handle_schunk(c, (shipgate_schunk_err_pkt*)pkt);

    case SHDR_TYPE_SDATA:
        return handle_sdata(c, (shipgate_sdata_pkt*)pkt);

    case SHDR_TYPE_QFLAG_SET:
        return handle_qflag_set(c, (shipgate_qflag_pkt*)pkt);

    case SHDR_TYPE_QFLAG_GET:
        return handle_qflag_get(c, (shipgate_qflag_pkt*)pkt);

    case SHDR_TYPE_SHIP_CTL:
        return handle_shipctl_reply(c, (shipgate_shipctl_pkt*)pkt,
            length, flags);

    case SHDR_TYPE_UBL_ADD:
        return handle_ubl_add(c, (shipgate_ubl_add_pkt*)pkt);

    //case SHDR_TYPE_BBGUILD:
    //    return handle_bbguild(c, (shipgate_bb_guild_pkt*)pkt);

    //case SHDR_TYPE_BBGUILD_REQ:
    //    return handle_bbguild_req(c, (shipgate_bb_guild_req_pkt*)pkt);

    default:
        UNK_SPD(type,(uint8_t*)pkt);
        return -3;
    }
}

static ssize_t ship_recv(ship_t* c, void* buffer, size_t len) {
    int ret;
    LOOP_CHECK(ret, gnutls_record_recv(c->session, buffer, len));
    return ret;
    //return gnutls_record_recv(c->session, buffer, len);
}

/* Handle incoming data to the shipgate. */
int handle_pkt(ship_t* c) {
    ssize_t sz;
    uint16_t pkt_sz;
    int rv = 0;
    unsigned char* rbp;
    void* tmp;

    /* If we've got anything buffered, copy it out to the main buffer to make
       the rest of this a bit easier. */
    if (c->recvbuf_cur) {
        memcpy(recvbuf, c->recvbuf, c->recvbuf_cur);

    }

    /* Attempt to read, and if we don't get anything, punt. */
    sz = ship_recv(c, recvbuf + c->recvbuf_cur,
        65536 - c->recvbuf_cur);

    //TEST_LOG("船闸处理端口 %d 接收数据 %d 字节", c->sock, sz);

    /* Attempt to read, and if we don't get anything, punt. */
    if (sz <= 0) {
        if (sz == SOCKET_ERROR) {
            ERR_LOG("Gnutls *** 注意: SOCKET_ERROR");
        }

        goto end;
    }

    //if (sz == SOCKET_ERROR) {
    //    DBG_LOG("Gnutls *** 注意: SOCKET_ERROR");
    //    goto end;
    //}
    //else if (sz == 0) {
    //    DBG_LOG("Gnutls *** 注意: 对等方已关闭TLS连接");
    //    goto end;
    //}
    //else if (sz < 0 && gnutls_error_is_fatal(sz) == 0) {
    //    ERR_LOG("Gnutls *** 警告: %s", gnutls_strerror(sz));
    //    goto end;
    //}
    //else if (sz < 0) {
    //    ERR_LOG("Gnutls *** 错误: %s", gnutls_strerror(sz));
    //    ERR_LOG("Gnutls *** 接收到损坏的数据(%d). 关闭连接.", sz);
    //    goto end;
    //}
    //else if (sz > 0) {
    sz += c->recvbuf_cur;
    c->recvbuf_cur = 0;
    rbp = recvbuf;

        /* As long as what we have is long enough, decrypt it. */
    if(sz >= 8) {
        while (sz >= 8 && rv == 0) {
            /* Grab the packet header so we know what exactly we're looking
               for, in terms of packet length. */
            if (!c->hdr_read) {
                memcpy(&c->pkt, rbp, 8);
                c->hdr_read = 1;
            }

            pkt_sz = ntohs(c->pkt.pkt_len);

            /* Do we have the whole packet? */
            if (sz >= (ssize_t)pkt_sz) {
                /* Yep, copy it and process it */
                memcpy(rbp, &c->pkt, 8);

                /* Pass it onto the correct handler. */
                c->last_message = time(NULL);
                rv = process_ship_pkt(c, (shipgate_hdr_t*)rbp);

                rbp += pkt_sz;
                sz -= pkt_sz;

                c->hdr_read = 0;
            }
            else {
                /* Nope, we're missing part, break out of the loop, and buffer
                   the remaining data. */
                break;
            }
        }
    }

        /* If we've still got something left here, buffer it for the next pass. */
    if(sz) {
            /* Reallocate the recvbuf for the client if its too small. */
            if (c->recvbuf_size < sz) {
                tmp = realloc(c->recvbuf, sz);

                if (!tmp) {
                    perror("realloc");
                    return -1;
                }

                c->recvbuf = (unsigned char*)tmp;
                c->recvbuf_size = sz;
            }

            memcpy(c->recvbuf, rbp, sz);
            c->recvbuf_cur = sz;
        }
    else {
            /* Free the buffer, if we've got nothing in it. */
            free_safe(c->recvbuf);
            c->recvbuf = NULL;
            c->recvbuf_size = 0;
        }

        return rv;
    /*}*/

end:
    return sz;
}

#ifdef ENABLE_LUA

static int ship_sendsdata_lua(lua_State* l) {
    ship_t* c;
    uint32_t event, gc, block;
    const uint8_t* s;
    size_t len;
    lua_Integer rv = -1;

    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2) &&
        lua_isinteger(l, 3) && lua_isinteger(l, 4) && lua_isstring(l, 5)) {
        c = (ship_t*)lua_touserdata(l, 1);
        event = (uint32_t)lua_tointeger(l, 2);
        gc = (uint32_t)lua_tointeger(l, 3);
        block = (uint32_t)lua_tointeger(l, 4);
        s = (const uint8_t*)lua_tolstring(l, 5, &len);

        rv = send_sdata(c, gc, block, event, s, (uint32_t)len);
    }

    lua_pushinteger(l, rv);
    return 1;
}

static int ship_writeLog_lua(lua_State* l) {
    const char* s;

    if (lua_isstring(l, 1)) {
        s = (const char*)lua_tostring(l, 1);
        SGATE_LOG("%s", s);
    }

    return 0;
}

static const luaL_Reg shiplib[] = {
    { "sendScriptData", ship_sendsdata_lua },
    { "writeLog", ship_writeLog_lua },
    { NULL, NULL }
};

int ship_register_lua(lua_State* l) {
    luaL_newlib(l, shiplib);
    return 1;
}

#endif

int load_guild_default_flag(char* file) {
    FILE* fp;
    errno_t err;
    long len = 0;

    err = fopen_s(&fp, file, "rb");
    if (err)
    {
        ERR_LOG("文件 %s 缺失!", file);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (!fread(default_guild_flag, 1, sizeof(default_guild_flag), fp))
    {
        ERR_LOG("读取 %s 失败!", file);
        return -1;
    }

    fclose(fp);

    psocn_db_escape_str(&conn, &default_guild_flag_slashes[0], &default_guild_flag[0], sizeof(default_guild_flag));

    SGATE_LOG("读取初始角色公会图标文件 共 %d 字节.", len);

    return 0;
}
