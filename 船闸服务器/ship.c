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
#include <SFMT.h>
#include <md5.h>
#include <pso_menu.h>
#include <pso_character.h>
#include <f_checksum.h>

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
extern gnutls_certificate_credentials_t tls_cred;
extern gnutls_priority_t tls_prio;

/* Events... */
extern uint32_t event_count;
extern monster_event_t* events;

/* Scripts */
extern uint32_t script_count;
extern ship_script_t* scripts;

//static uint8_t recvbuf[65536];

/* 公会相关 */
static uint8_t default_guild_flag[2048];
static uint8_t default_guild_flag_slashes[4098];

///* 默认玩家数据 */
//static psocn_bb_default_char_t default_chars;

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

/* 完成数据包设置,发送至DC舰船... */
static int send_dc_pkt_to_ship(ship_t* c, uint32_t target_guildcard, uint8_t* pkt) {
    int ship_id;
    ship_t* s;

    ship_id = db_get_char_ship_id(target_guildcard);

    if (ship_id >= 0) {
        /* If we've got this far, we should have the ship we need to send to */
        s = find_ship(ship_id);
        if (s == NULL) {
            ERR_LOG("无效 %d 号舰船.", ship_id);
            return 2;
        }

        /* 完成数据包设置,发送至舰船... */
        return forward_dreamcast(s, (dc_pkt_hdr_t*)pkt, c->key_idx, target_guildcard, 0);
    }

#ifdef DEBUG
    ERR_LOG("未找到GC %u 玩家所在舰船.", target_guildcard);
#endif // DEBUG

    return 0;
}

/* 完成数据包设置,发送至PC舰船... */
static int send_pc_pkt_to_ship(ship_t* c, uint32_t target_guildcard, uint8_t* pkt) {
    int ship_id;
    ship_t* s;

    ship_id = db_get_char_ship_id(target_guildcard);

    if (ship_id >= 0) {
        /* If we've got this far, we should have the ship we need to send to */
        s = find_ship(ship_id);
        if (s == NULL) {
            ERR_LOG("无效 %d 号舰船.", ship_id);
            return 2;
        }

        /* 完成数据包设置,发送至舰船... */
        return forward_pc(s, (dc_pkt_hdr_t*)pkt, c->key_idx, target_guildcard, 0);
    }

#ifdef DEBUG
    ERR_LOG("未找到GC %u 玩家所在舰船.", target_guildcard);
#endif // DEBUG

    return 0;
}

/* 完成数据包设置,发送至BB舰船... */
static int send_bb_pkt_to_ship(ship_t* c, uint32_t target_guildcard, uint8_t* pkt) {
    int ship_id;
    ship_t* s;

    ship_id = db_get_char_ship_id(target_guildcard);

    if (ship_id >= 0) {
        /* If we've got this far, we should have the ship we need to send to */
        s = find_ship(ship_id);
        if (s == NULL) {
            ERR_LOG("无效 %d 号舰船.", ship_id);
            return 2;
        }

        /* 完成数据包设置,发送至舰船... */
        return forward_bb(s, (bb_pkt_hdr_t*)pkt, c->key_idx, target_guildcard, 0);
    }

#ifdef DEBUG
    ERR_LOG("未找到GC %u 玩家所在舰船.", target_guildcard);
#endif // DEBUG

    return 0;
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

        /* 如果我们到了这里, then the event is valid, return it. */
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
        , SERVER_BLOCKS_LIST, AUTH_ACCOUNT_BLUEBURST
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
    unsigned int peer_status, cert_list_size;
    gnutls_x509_crt_t cert;
    const gnutls_datum_t* cert_list;
    uint8_t hash[20];
    size_t sz = 20;
    char query[256], fingerprint[40];
    void* result;
    char** row;

    rv = (ship_t*)malloc(sizeof(ship_t));

    if (!rv) {
        ERR_LOG("malloc");
        perror("malloc");
        closesocket(sock);
        return NULL;
    }

    memset(rv, 0, sizeof(ship_t));

    /* 将基础参数存储进客户端的结构. */
    rv->sock = sock;
    rv->last_message = time(NULL);
    memcpy(&rv->conn_addr, addr, size);

    /* Create the TLS session */
    gnutls_init(&rv->session, GNUTLS_SERVER);
    gnutls_priority_set(rv->session, tls_prio);
    tmp = gnutls_credentials_set(rv->session, GNUTLS_CRD_CERTIFICATE, tls_cred);
    if (tmp < 0) {
        ERR_LOG("GNUTLS *** 注意: TLS 匿名凭据错误: %s", gnutls_strerror(tmp));
        goto err_tls;
    }

    gnutls_certificate_server_set_request(rv->session, GNUTLS_CERT_REQUIRE);

#if (SIZEOF_INT != SIZEOF_VOIDP) && (SIZEOF_LONG_INT == SIZEOF_VOIDP)
    gnutls_transport_set_ptr(rv->session, (gnutls_transport_ptr_t)((long)sock));
#else
    gnutls_transport_set_ptr(rv->session, (gnutls_transport_ptr_t)sock);
#endif

    /* Do the TLS handshake */
    do {
        tmp = gnutls_handshake(rv->session);
    } while (tmp < 0 && gnutls_error_is_fatal(tmp) == 0);

    if (tmp < 0) {
        ERR_LOG("GNUTLS *** 注意: TLS 握手失败 %s", gnutls_strerror(tmp));
        goto err_hs;
    }
    else {
        SGATE_LOG("GNUTLS *** TLS 握手成功");

        char* desc;
        desc = gnutls_session_get_desc(rv->session);
        SGATE_LOG("GNUTLS *** Session 信息: %d - %s", rv->sock, desc);
        gnutls_free(desc);
    }

    /* Verify that the peer has a valid certificate */
    tmp = gnutls_certificate_verify_peers2(rv->session, &peer_status);

    if (tmp < 0) {
        ERR_LOG("GNUTLS *** 注意: 验证证书有效性失败: %s", gnutls_strerror(tmp));
        goto err_cert;
    }

    /* Check whether or not the peer is trusted... */
    if (peer_status & GNUTLS_CERT_INVALID) {
        ERR_LOG("GNUTLS *** 注意: 不受信任的对等连接, 原因如下 (%08x):"
            , peer_status);

        if (peer_status & GNUTLS_CERT_SIGNER_NOT_FOUND)
            ERR_LOG("未找到发卡机构");
        if (peer_status & GNUTLS_CERT_SIGNER_NOT_CA)
            ERR_LOG("发卡机构不是CA");
        if (peer_status & GNUTLS_CERT_NOT_ACTIVATED)
            ERR_LOG("证书尚未激活");
        if (peer_status & GNUTLS_CERT_EXPIRED)
            ERR_LOG("证书已过期");
        if (peer_status & GNUTLS_CERT_REVOKED)
            ERR_LOG("证书已吊销");
        if (peer_status & GNUTLS_CERT_INSECURE_ALGORITHM)
            ERR_LOG("不安全的证书签名");

        goto err_cert;
    }

    /* Verify that we know the peer */
    if (gnutls_certificate_type_get(rv->session) != GNUTLS_CRT_X509) {
        ERR_LOG("GNUTLS *** 注意: 无效证书类型!");
        goto err_cert;
    }

    tmp = gnutls_x509_crt_init(&cert);
    if (tmp < 0) {
        ERR_LOG("GNUTLS *** 注意: 无法初始化证书: %s", gnutls_strerror(tmp));
        goto err_cert;
    }

    /* Get the peer's certificate */
    cert_list = gnutls_certificate_get_peers(rv->session, &cert_list_size);
    if (cert_list == NULL) {
        ERR_LOG("GNUTLS *** 注意: No certs found for connection!?");
        goto err_cert;
    }

    tmp = gnutls_x509_crt_import(cert, &cert_list[0], GNUTLS_X509_FMT_DER);
    if (tmp < 0) {
        ERR_LOG("GNUTLS *** 注意: Cannot parse certificate: %s", gnutls_strerror(tmp));
        goto err_cert;
    }

    /* Get the SHA1 fingerprint */
    tmp = gnutls_x509_crt_get_fingerprint(cert, GNUTLS_DIG_SHA1, hash, &sz);
    if (tmp < 0) {
        ERR_LOG("GNUTLS *** 注意: Cannot read hash: %s", gnutls_strerror(tmp));
        goto err_cert;
    }

    /* Figure out what ship is connecting by the fingerprint */
    psocn_db_escape_str(&conn, fingerprint, (char*)hash, 20);

#ifdef DEBUG

    display_packet(fingerprint, 20);
    db_upload_temp_data(hash, 20);
    getchar();

#endif // DEBUG

research_cert:
    sprintf(query, "SELECT idx FROM %s WHERE sha1_fingerprint='%s'"
        , SERVER_SHIPS, fingerprint);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法查询舰船密钥数据库");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto err_cert;
    }

    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未从数据库中匹配到舰船密钥");
        goto err_cert;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        // 若指纹不存在，则执行插入操作
        snprintf(query, sizeof(query), "INSERT INTO %s (sha1_fingerprint) VALUES ('%s')", SERVER_SHIPS, fingerprint);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("插入舰船密钥失败: %s\n", psocn_db_error(&conn));
            goto err_cert;
        }
        SQLERR_LOG("未知舰船密钥");
        goto research_cert;
    }

    /* Store the ship ID */
    rv->key_idx = atoi(row[0]);
    psocn_db_result_free(result);
    gnutls_x509_crt_deinit(cert);

    /* Send the client the welcome packet, or die trying. */
    if (send_welcome(rv)) {
        ERR_LOG("*** 警告: %d 舰船离线",
            rv->key_idx);
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

void db_remove_client(ship_t* s) {
    ship_t* i;

    TAILQ_REMOVE(&ships, s, qentry);

    if (s->key_idx) {
        /* Send a status packet to everyone telling them its gone away
         向每个人发送状态数据包，告诉他们它已经消失了
        */
        TAILQ_FOREACH(i, &ships, qentry) {
            send_ship_status(i, s, 0);
        }

        db_delete_online_clients(s->name, s->key_idx);

        db_delete_transient_clients(s->name, s->key_idx);

        db_delete_online_ships(s->name, s->key_idx);

    }
}

/* Destroy a connection, closing the socket and removing it from the list. */
void destroy_connection(ship_t* s) {
    if (s->name[0]) {
        DC_LOG("关闭与 %s 的连接", s->name);
    }
    else {
        DC_LOG("取消与未认证舰船的连接");
    }

    db_remove_client(s);

    /* Clean up the TLS resources and the socket. */
    if (s->sock >= 0) {
        gnutls_bye(s->session, GNUTLS_SHUT_RDWR);
        closesocket(s->sock);
        gnutls_deinit(s->session);
        s->sock = SOCKET_ERROR;
    }

    if (s->recvbuf) {
        free_safe(s->recvbuf);
    }

    if (s->sendbuf) {
        free_safe(s->sendbuf);
    }

    free_safe(s);
}

/* 更新舰船IPv4 暂未完成更新IPv6 */
int update_ship_ipv4(ship_t* s) {
    struct addrinfo hints;
    struct addrinfo* server, * j;
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in* addr4;
    struct sockaddr_in6* addr6;

    if (!s->remote_host4)
        return 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(s->remote_host4, "11000", &hints, &server)) {
        ERR_LOG("无效 IPv4 地址: %s", s->remote_host4);
        return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET) {
            addr4 = (struct sockaddr_in*)j->ai_addr;
            inet_ntop(j->ai_family, &addr4->sin_addr, ipstr, INET6_ADDRSTRLEN);
            s->remote_addr4 = addr4->sin_addr.s_addr;
            //SGATE_LOG("    获取到 IPv4 地址: %s", ipstr);
        }
        else if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //memcpy(cfg->server_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    /* Make sure we found at least an IPv4 address */
    if (!s->remote_addr4) {
        ERR_LOG("无法获取IPv4地址!");
        return -1;
    }
    else
        db_update_ship_ipv4(s->remote_addr4, s->key_idx);

    return 0;
}

/* 处理 ship's login response. */
static int handle_shipgate_login6t(ship_t* s, shipgate_login6_reply_pkt* pkt) {
    char query[512];
    ship_t* j;
    void* result;
    char** row;
    uint32_t pver = s->proto_ver = ntohl(pkt->proto_ver);
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

        send_error(s, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_LOGIN_BAD_PROTO, NULL, 0);
        return -1;
    }

    /* Attempt to grab the key for this ship. */
    sprintf(query, "SELECT main_menu, ship_number FROM %s WHERE "
        "idx='%u'", SERVER_SHIPS, s->key_idx);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法查询舰船密钥数据库");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        send_error(s, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, NULL, 0);
        return -1;
    }

    if ((result = psocn_db_result_store(&conn)) == NULL ||
        (row = psocn_db_result_fetch(result)) == NULL) {
        SQLERR_LOG("无效密钥idx索引 %d", s->key_idx);
        send_error(s, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_LOGIN_BAD_KEY, NULL, 0);
        return -1;
    }

    /* Check the menu code for validity */
    if (menu_code && (!isalpha(menu_code & 0xFF) | !isalpha(menu_code >> 8))) {
        SQLERR_LOG("菜单代码错误 idx: %d", s->key_idx);
        send_error(s, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_LOGIN_BAD_MENU, NULL, 0);
        return -1;
    }

    /* If the ship requests the main menu and they aren't allowed there, bail */
    if (!menu_code && !atoi(row[0])) {
        SQLERR_LOG("菜单代码无效 id: %d", s->key_idx);
        send_error(s, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_LOGIN_INVAL_MENU, NULL, 0);
        return -1;
    }

    /* Grab the key from the result */
    ship_number = atoi(row[1]);
    psocn_db_result_free(result);

    /* Fill in the ship structure */
    memcpy(s->remote_host4, pkt->ship_host4, 32);
    memcpy(s->remote_host6, pkt->ship_host6, 128);
    s->remote_addr4 = pkt->ship_addr4;
    memcpy(&s->remote_addr6, pkt->ship_addr6, 16);
    s->port = ntohs(pkt->ship_port);
    s->clients = ntohs(pkt->clients);
    s->games = ntohs(pkt->games);
    s->flags = ntohl(pkt->flags);
    s->menu_code = menu_code;
    memcpy(s->name, pkt->name, 12);
    s->ship_number = ship_number;
    s->privileges = ntohl(pkt->privileges);

    pack_ipv6(&s->remote_addr6, &ip6_hi, &ip6_lo);

    sprintf(query, "INSERT INTO %s(name, players, ship_host4, ship_host6, ip, port, int_ip, "
        "ship_id, gm_only, games, menu_code, flags, ship_number, "
        "ship_ip6_high, ship_ip6_low, protocol_ver, privileges) VALUES "
        "('%s', '%hu', '%s', '%s', '%lu', '%hu', '%u', '%u', '%d', '%hu', '%hu', '%u', "
        "'%d', '%llu', '%llu', '%u', '%u')", SERVER_SHIPS_ONLINE, s->name, s->clients,
        s->remote_host4, s->remote_host6, ntohl(s->remote_addr4), s->port, 0, s->key_idx,
        !!(s->flags & LOGIN_FLAG_GMONLY), s->games, s->menu_code, s->flags,
        ship_number, (unsigned long long)ip6_hi,
        (unsigned long long)ip6_lo, pver, s->privileges);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法将 %s 新增至在线舰船列表.",
            s->name);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        send_error(s, SHDR_TYPE_LOGIN6, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, NULL, 0);
        return -1;
    }

    /* Hooray for misusing functions! */
    if (send_error(s, SHDR_TYPE_LOGIN6, SHDR_RESPONSE, ERR_NO_ERROR, NULL, 0)) {
        ERR_LOG("滥用函数万岁!");
        return -1;
    }

#ifdef ENABLE_LUA
    /* Send script check packets, if the ship supports scripting */
    if (pver >= 16 && s->flags & LOGIN_FLAG_LUA) {
        for (i = 0; i < script_count; ++i) {
            send_script_check(s, &scripts[i]);
        }
    }
#endif

    /* Does this ship support the initial shipctl packets? If so, send it a
       uname and a version request 这艘船是否支持初始shipctl数据包？如果是，请向其发送uname和版本请求 . */
    if (pver >= 19) {
        if (send_sctl(s, SCTL_TYPE_UNAME, 0)) {
            ERR_LOG("滥用函数万岁!");
            return -1;
        }

        if (send_sctl(s, SCTL_TYPE_VERSION, 0)) {
            ERR_LOG("滥用函数万岁!");
            return -1;
        }
    }

    /* 向每艘船发送状态数据包. */
    TAILQ_FOREACH(j, &ships, qentry) {
        /* Don't send ships with privilege bits set to ships not running
           protocol v18 or newer 不发送特权位设置为未运行协议v18或更新版本的装运 . */
        if (!s->privileges || j->proto_ver >= 18)
            send_ship_status(j, s, 1);

        /*  把这艘船送到新船上去，只要不是在上面 . */
        if (j != s) {
            send_ship_status(s, j, 1);
        }

        clients += j->clients;
    }

    //SHIPS_LOG("handle_shipgate_login6t c->privileges = %d", c->privileges);
    /* Update the table of client counts, if it might have actually changed from
       this update packet. */
    if (s->clients) {
        sprintf(query, "INSERT INTO %s (clients) VALUES('%" PRIu32
            "') ON DUPLICATE KEY UPDATE clients=VALUES(clients)", SERVER_CLIENTS_COUNT, clients);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法更新全服 玩家/游戏 数量");
        }
    }

    SGATE_LOG("%s: 舰船与船闸完成对接", s->name);
    return 0;
}

/* 处理 ship's update counters packet. */
static int handle_count(ship_t* s, shipgate_cnt_pkt* pkt) {
    char query[256];
    ship_t* j;
    uint32_t clients = 0;
    uint16_t sclients = s->clients;

    s->clients = ntohs(pkt->clients);
    s->games = ntohs(pkt->games);

    sprintf(query, "UPDATE %s SET players='%hu', games='%hu' WHERE "
        "ship_id='%u'", SERVER_SHIPS_ONLINE, s->clients, s->games, s->key_idx);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法更新舰船 %s 玩家/游戏 数量", s->name);
        SQLERR_LOG("%s", psocn_db_error(&conn));
    }

    /* Update all of the ships */
    TAILQ_FOREACH(j, &ships, qentry) {
        send_counts(j, s->key_idx, s->clients, s->games);
        clients += j->clients;
    }

    /* Update the table of client counts, if the number actually changed from
       this update packet. */
    if (sclients != s->clients) {
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
    char msg[512] = { 0 }, name[64];
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
        , AUTH_ACCOUNT_GUILDCARDS, gc);
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

    /* See if the client being sent the mail has blocked the user sending it */
    if (check_user_blocklist(sender, guildcard, BLOCKLIST_MAIL))
        return 0;

    if (send_dc_pkt_to_ship(c, guildcard, (uint8_t*)pkt)) {
        return save_mail(guildcard, sender, pkt, VERSION_DC);
    }

    return 0;
}

static int handle_pc_mail(ship_t* c, simple_mail_pkt* pkt) {
    uint32_t guildcard = LE32(pkt->pcmaildata.gc_dest);
    uint32_t sender = LE32(pkt->gc_sender);

    /* See if the client being sent the mail has blocked the user sending it */
    if (check_user_blocklist(sender, guildcard, BLOCKLIST_MAIL))
        return 0;

    if (send_pc_pkt_to_ship(c, guildcard, (uint8_t*)pkt)) {
        return save_mail(guildcard, sender, pkt, VERSION_PC);
    }

    return 0;
}

static int handle_bb_mail(ship_t* c, simple_mail_pkt* pkt) {
    uint32_t guildcard = LE32(pkt->bbmaildata.gc_dest);
    uint32_t sender = LE32(pkt->gc_sender);

    /* See if the client being sent the mail has blocked the user sending it */
    if (check_user_blocklist(sender, guildcard, BLOCKLIST_MAIL))
        return 0;


    if (send_bb_pkt_to_ship(c, guildcard, (uint8_t*)pkt)) {
        return save_mail(guildcard, sender, pkt, VERSION_BB);
    }

    return 0;
}

static int handle_guild_search(ship_t* c, dc_guild_search_pkt* pkt, uint32_t flags) {
    uint32_t guildcard = LE32(pkt->gc_target);
    uint32_t searcher = LE32(pkt->gc_searcher);
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
        reply6.player_tag = LE32(0x00010000);
        reply6.gc_searcher = pkt->gc_searcher;
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

        /* 加密并发送 */
        forward_dreamcast(c, (dc_pkt_hdr_t*)&reply6, c->key_idx, 0, 0);
    }
    else {
        memset(&reply, 0, DC_GUILD_REPLY_LENGTH);

        /* Fill it in */
        reply.hdr.pkt_type = GUILD_REPLY_TYPE;
        reply.hdr.pkt_len = LE16(DC_GUILD_REPLY_LENGTH);
        reply.player_tag = LE32(0x00010000);
        reply.gc_searcher = pkt->gc_searcher;
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

        /* 加密并发送 */
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

    if (!db_check_gc_online(guildcard))
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
    reply.player_tag = LE32(0x00010000);
    reply.gc_searcher = p->gc_searcher;
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

    /* 加密并发送 */
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
    uint32_t fr_gc = LE32(gc->gc_data.guildcard);
    char query[1024];
    char name[97];
    char guild_name[65];
    char text[373];

    /* Make sure the packet is sane */
    if (len != 0x0110) {
        return -1;
    }

    /* Escape all the strings first */
    psocn_db_escape_str(&conn, name, (char*)&gc->gc_data.name, 48);
    psocn_db_escape_str(&conn, guild_name, (char*)gc->gc_data.guild_name, 32);
    psocn_db_escape_str(&conn, text, (char*)gc->gc_data.guildcard_desc, 176);

    /* Add the entry in the db... */
    sprintf(query, "INSERT INTO %s (guildcard, friend_gc, "
        "name, guild_name, text, language, section_id, class) VALUES ('%"
        PRIu32 "', '%" PRIu32 "', '%s', '%s', '%s', '%" PRIu8 "', '%"
        PRIu8 "', '%" PRIu8 "') ON DUPLICATE KEY UPDATE "
        "name=VALUES(name), text=VALUES(text), language=VALUES(language), "
        "section_id=VALUES(section_id), class=VALUES(class)", CLIENTS_GUILDCARDS, sender,
        fr_gc, name, guild_name, text, gc->gc_data.language, gc->gc_data.section,
        gc->gc_data.char_class);

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
    sprintf(query, "CALL clients_guildcards_delete('%" PRIu32 "', '%" PRIu32
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
    sprintf(query, "CALL clients_guildcards_sort('%" PRIu32 "', '%" PRIu32
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
    uint32_t bl_gc = LE32(gc->gc_data.guildcard);
    char query[1024];
    char name[97];
    char guild_name[65];
    char text[373];

    /* Make sure the packet is sane */
    if (len != 0x0110) {
        return -1;
    }

    /* Escape all the strings first */
    psocn_db_escape_str(&conn, name, (char*)&gc->gc_data.name, 48);
    psocn_db_escape_str(&conn, guild_name, (char*)gc->gc_data.guild_name, 32);
    psocn_db_escape_str(&conn, text, (char*)gc->gc_data.guildcard_desc, 176);

    /* Add the entry in the db... */
    sprintf(query, "INSERT INTO %s (guildcard, blocked_gc, "
        "name, guild_name, text, language, section_id, class) VALUES ('%"
        PRIu32 "', '%" PRIu32 "', '%s', '%s', '%s', '%" PRIu8 "', '%"
        PRIu8 "', '%" PRIu8 "') ON DUPLICATE KEY UPDATE "
        "name=VALUES(name), text=VALUES(text), language=VALUES(language), "
        "section_id=VALUES(section_id), class=VALUES(class)", CHARACTER_BLACKLIST, sender,
        bl_gc, name, guild_name, text, gc->gc_data.language, gc->gc_data.section,
        gc->gc_data.char_class);

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
        "' AND blocked_gc='%" PRIu32 "'", CHARACTER_BLACKLIST, sender, bl_gc);

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
    while (len < 0x88 && gc->guildcard_desc[len]) ++len;
    memset(&gc->guildcard_desc[len], 0, (0x88 - len) * 2);
    len = (len + 1) * 2;

    psocn_db_escape_str(&conn, comment, (char*)gc->guildcard_desc, len);

    /* Build the query and run it */
    sprintf(query, "UPDATE %s SET comment='%s' WHERE "
        "guildcard='%" PRIu32"' AND friend_gc='%" PRIu32 "'"
        , CLIENTS_GUILDCARDS, comment,
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
    char guild_name[24];
    uint32_t create_res;
    bb_guild_data_pkt* guild;

    if (len != sizeof(bb_guild_create_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

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

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name, &g_data->guild_name[2], 24, sizeof(g_data->guild_name) - 4);

    create_res = db_insert_bb_char_guild(g_data->guild_name, default_guild_flag, g_data->guildcard, g_data->hdr.flags);

    guild = (bb_guild_data_pkt*)malloc(sizeof(bb_guild_data_pkt));

    if (!guild) {
        ERR_LOG("分配公会数据内存错误.");
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return -1;
    }

    memset(guild, 0, sizeof(bb_guild_data_pkt));

    guild->hdr.pkt_type = g_data->hdr.pkt_type;
    guild->hdr.pkt_len = sizeof(bb_guild_data_pkt);
    guild->hdr.flags = g_data->hdr.flags;

    switch (create_res)
    {
    case 0x00:
        guild->guild = db_get_bb_char_guild(sender);

        if (send_bb_pkt_to_ship(c, sender, (uint8_t*)guild)) {
            guild->hdr.flags = sender;
            send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_GUILD_SENT_PKT, (uint8_t*)guild, len);
        }

        //DBG_LOG("创建GC %d (%s)公会数据成功.", sender, guild_name);
        break;

    case 0x01:
        ERR_LOG("创建GC %d 公会数据失败, 数据库错误.", sender);
        guild->hdr.flags = sender;
        /* TODO 需要给客户端返回错误信息 */
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_GUILD_SQLERR, (uint8_t*)guild, len);
        break;

    case 0x02:
        ERR_LOG("创建GC %d 公会数据失败, %s 公会已存在.", sender, guild_name);
        guild->hdr.flags = sender;
        /* TODO 需要给客户端返回错误信息 */
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_GUILD_EXIST, (uint8_t*)guild, len);
        break;

    case 0x03:
        ERR_LOG("创建GC %d 公会数据失败,该 GC 已在公会中.", sender);
        guild->hdr.flags = sender;
        /* TODO 需要给客户端返回错误信息 */
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_GUILD_ALREADY_IN, (uint8_t*)guild, len);
        break;
    }

    /* 完成数据包设置,发送至舰船... */
    free_safe(guild);
    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_02EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_02EA_pkt* g_data = (bb_guild_unk_02EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_02EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 成员增加 */
static int handle_bb_guild_member_add(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_add_pkt* g_data = (bb_guild_member_add_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t guild_id = pkt->fw_flags, gc_target = g_data->target_guildcard;

    if (len != sizeof(bb_guild_member_add_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    /* 如果没搜传递GC过来 就不要回应 */
    if (!gc_target)
        return 0;

    if (db_update_bb_guild_member_add(guild_id, gc_target)) {

        SQLERR_LOG("新增 BB %s 数据包 (%d) 失败", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    /* 不需要通知舰船了 */
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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 成员移除 */
static int handle_bb_guild_member_remove(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_remove_pkt* g_data = (bb_guild_member_remove_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t target_gc = g_data->target_guildcard;
    uint32_t guild_id = pkt->fw_flags;

    if (len != sizeof(bb_guild_member_remove_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    if (!target_gc || guild_id < 1)
        return send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);

    //display_packet((uint8_t*)g_data, len);

    //DBG_LOG("公会ID %u 移除的目标 %u", guild_id, target_gc);

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET guild_id = '-1', guild_priv_level = '0' "
        "WHERE guildcard = '%" PRIu32 "' AND guild_id = '%" PRIu32 "'", AUTH_ACCOUNT, target_gc, guild_id);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("未能找到GC %u 的公会成员信息", sender);

        display_packet((uint8_t*)g_data, len);
        return send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
    }

    return send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data);
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_06EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_06EA_pkt* g_data = (bb_guild_unk_06EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_06EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 聊天 */
static int handle_bb_guild_member_chat(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_chat_pkt* g_data = (bb_guild_member_chat_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t i = 0, num_mates, guild_id = pkt->fw_flags;
    void* result;
    char** row;

    sprintf_s(myquery, _countof(myquery), "SELECT "
        "%s.guildcard, %s.guild_priv_level, "
        "lastchar_blob, "
        "%s.guild_reward0, guild_reward1, guild_reward2, guild_reward3, "
        "guild_reward4, guild_reward5, guild_reward6, guild_reward7"
        " FROM "
        "%s"
        " INNER JOIN "
        "%s"
        " ON "
        "%s.guild_id = %s.guild_id"
        " WHERE "
        "%s.guild_id = '%" PRIu32 "' ORDER BY guild_priv_level DESC"
        , AUTH_ACCOUNT, AUTH_ACCOUNT
        , CLIENTS_GUILD
        , AUTH_ACCOUNT
        , CLIENTS_GUILD
        , AUTH_ACCOUNT, CLIENTS_GUILD
        , CLIENTS_GUILD, guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("未能找到GC %u 的公会成员信息", sender);
        return 0;
    }

    result = psocn_db_result_store(&conn);
    num_mates = (uint32_t)psocn_db_result_rows(result);


    for (i = 0; i < num_mates; i++) {
        row = psocn_db_result_fetch(result);

        sender = (uint32_t)strtoul(row[0], NULL, 0);

        if (send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data)) {
            send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)g_data, len);
            return 0;
        }
    }

    psocn_db_result_free(result);

    return 0;
}

/* 处理 Blue Burst 公会 成员设置 */
static int handle_bb_guild_member_setting(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_setting_pkt* g_data = (bb_guild_member_setting_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t num_mates, guild_id = pkt->fw_flags;
    size_t i = 0, size = 0, entries_size = 0x30, len3 = BB_CHARACTER_NAME_LENGTH * 2;
    void* result;
    char** row;

    sprintf_s(myquery, _countof(myquery), "SELECT %s.guildcard, %s.guild_priv_level, "
        "lastchar_blob, "
        "%s.guild_reward0, guild_reward1, guild_reward2, guild_reward3, "
        "guild_reward4, guild_reward5, guild_reward6, guild_reward7"
        " FROM "
        "%s"
        " INNER JOIN "
        "%s"
        " ON "
        "%s.guild_id = %s.guild_id"
        //" INNER JOIN "
        //"%s"
        //" ON "
        //"%s.guildcard = %s.guildcard"
        " WHERE "
        "%s.guild_id = '%" PRIu32 "' ORDER BY guild_priv_level DESC"
        , AUTH_ACCOUNT, AUTH_ACCOUNT
        , CLIENTS_GUILD
        , AUTH_ACCOUNT
        , CLIENTS_GUILD
        , AUTH_ACCOUNT, CLIENTS_GUILD
        //, SERVER_CLIENTS_ONLINE
        //, AUTH_ACCOUNT, SERVER_CLIENTS_ONLINE
        , CLIENTS_GUILD, guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("未能找到GC %u 的公会成员信息", sender);
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    result = psocn_db_result_store(&conn);
    num_mates = (uint32_t)psocn_db_result_rows(result);

    g_data->guild_member_amount = num_mates;

    for (i = 0; i < num_mates; i++) {
        row = psocn_db_result_fetch(result);

        g_data->entries[i].member_index = i + 1;
        g_data->entries[i].guild_priv_level = (uint32_t)strtoul(row[1], NULL, 0);
        g_data->entries[i].guildcard_client = (uint32_t)strtoul(row[0], NULL, 0);
        memcpy(&g_data->entries[i].char_name[0], row[2], len3);
        g_data->entries[i].char_name[1] = 0x0045;
        //g_data->entries[i].guild_reward[0] = (uint32_t)strtoul(row[3], NULL, 0);
        //g_data->entries[i].guild_reward[1] = (uint32_t)strtoul(row[4], NULL, 0);
        //g_data->entries[i].guild_reward[2] = (uint32_t)strtoul(row[5], NULL, 0);
        //g_data->entries[i].guild_reward[3] = (uint32_t)strtoul(row[6], NULL, 0);
        //g_data->entries[i].guild_reward[4] = (uint32_t)strtoul(row[7], NULL, 0);
        //g_data->entries[i].guild_reward[5] = (uint32_t)strtoul(row[8], NULL, 0);
        //g_data->entries[i].guild_reward[6] = (uint32_t)strtoul(row[9], NULL, 0);
        //g_data->entries[i].guild_reward[7] = (uint32_t)strtoul(row[10], NULL, 0);

        size += entries_size;
    }

    psocn_db_result_free(result);

    g_data->hdr.pkt_len = LE16((uint16_t)size);
    g_data->hdr.pkt_type = LE16(BB_GUILD_UNK_09EA);
    g_data->hdr.flags = num_mates;

    return send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data);

}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_unk_09EA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_09EA_pkt* g_data = (bb_guild_unk_09EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_unk_09EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);

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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 公会邀请*/
static int handle_bb_guild_invite_0DEA(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_invite_0DEA_pkt* g_data = (bb_guild_invite_0DEA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_invite_0DEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    uint32_t gcn = 0, teamid = 0;

    if (send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data)) {
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    //display_packet((uint8_t*)g_data, len);

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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 成员图标设置 */
static int handle_bb_guild_member_flag_setting(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_member_flag_setting_pkt* g_data = (bb_guild_member_flag_setting_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t guild_id = g_data->hdr.flags;

    if (len != sizeof(bb_guild_member_flag_setting_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return -1;
    }

    if (db_update_bb_guild_flag(g_data->guild_flag, guild_id)) {
        SQLERR_LOG("更新 BB %s 数据包 (%d) 失败!", c_cmd_name(type, 0), len);
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return -1;
    }

    if (send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data)) {
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return -1;
    }

    return 0;
}

/* 处理 Blue Burst 公会 解散公会 */
static int handle_bb_guild_dissolve(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_dissolve_pkt* g_data = (bb_guild_dissolve_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t guild_id = pkt->fw_flags;
    int res = 0;

    if (len != sizeof(bb_guild_dissolve_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    res = db_dissolve_bb_guild(guild_id);

    if (res == 1) {
        ERR_LOG("GC %d 解散不存在的公会失败", sender);
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    if (res == 2) {
        ERR_LOG("GC %d 更新账户公会信息失败", sender);
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    g_data->hdr.flags = guild_id;

    return send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data);
}

/* 处理 Blue Burst 公会 成员权限提升 */
static int handle_bb_guild_member_promote(ship_t* c, shipgate_fw_9_pkt* pkt) {
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t guild_id = pkt->fw_flags;
    bb_guild_member_promote_pkt* g_data = (bb_guild_member_promote_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t guild_priv_level = g_data->hdr.flags;
    uint32_t target_gc = g_data->target_guildcard;

    if (len != sizeof(bb_guild_member_promote_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

#ifdef DEBUG

    DBG_LOG("转换GC %u GID %u 指令发送 %u", target_gc, guild_id, sender);

#endif // DEBUG

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
        "guild_priv_level = '%u'"
        " WHERE "
        "guildcard = '%u' AND guild_id = '%u'"
        , AUTH_ACCOUNT
        , guild_priv_level
        , target_gc, guild_id
    );
    psocn_db_real_query(&conn, myquery);

    if (guild_priv_level == BB_GUILD_PRIV_LEVEL_MASTER) {  // 会长转让

        /* 将原会长的数据库公会权限降级为管理员 */
        sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
            "guild_priv_level = '%u'"
            " WHERE "
            "guildcard = '%u' AND guild_id = '%u'"
            , AUTH_ACCOUNT
            , BB_GUILD_PRIV_LEVEL_ADMIN
            , sender, guild_id
        );

        psocn_db_real_query(&conn, myquery);

        /* 将原公会数据库公会所有人改为新的会长的GC */
        sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
            "guildcard = '%u'"
            " WHERE "
            "guildcard = '%u' AND guild_id = '%u'",
            CLIENTS_GUILD
            , target_gc
            , sender, guild_id
        );

        psocn_db_real_query(&conn, myquery);
    }

    //display_packet((uint8_t*)g_data, len);

    if (send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data)) {
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_initialization_data(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_unk_12EA_pkt* g_data = (bb_guild_unk_12EA_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t guild_id = pkt->fw_flags;
    //void* result;
    //char** row;

    if (len != sizeof(bb_guild_unk_12EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    //guild_id = db_get_bb_char_guild_id(sender);

    //g_data->unk = 0;

    //if (guild_id) {
    //    memset(myquery, 0, sizeof(myquery));

    //    sprintf_s(myquery, _countof(myquery), "SELECT %s.guildcard, %s.guild_priv_level, "
    //        "lastchar_blob, guild_points_personal_donation, "
    //        "%s.guild_points_rank, guild_points_rest, "
    //        "guild_reward0, guild_reward1, guild_reward2, guild_reward3, "
    //        "guild_reward4, guild_reward5, guild_reward6, guild_reward7"
    //        " FROM "
    //        "%s INNER JOIN %s ON "
    //        "%s.guild_id = %s.guild_id WHERE "
    //        "%s.guild_id = '%" PRIu32 "' ORDER BY guild_points_personal_donation DESC"
    //        , AUTH_ACCOUNT, AUTH_ACCOUNT
    //        , CLIENTS_GUILD
    //        , AUTH_ACCOUNT, CLIENTS_GUILD
    //        , AUTH_ACCOUNT, CLIENTS_GUILD
    //        , CLIENTS_GUILD, guild_id
    //    );

    //    sprintf(myquery, "SELECT "
    //        "guild_priv_level"
    //        " FROM %s WHERE guild_id = '%" PRIu32 "'", 
    //        CLIENTS_GUILD, guild_id);

    //    if (psocn_db_real_query(&conn, myquery)) {
    //        SQLERR_LOG("无法查询角色数据 (%u)", sender);
    //        SQLERR_LOG("%s", psocn_db_error(&conn));

    //        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
    //            ERR_BAD_ERROR, (uint8_t*)g_data, len);
    //        return -1;
    //    }

    //    /* Grab the data we got. */
    //    if ((result = psocn_db_result_store(&conn)) == NULL) {
    //        SQLERR_LOG("未获取到角色数据 (%u)", sender);
    //        SQLERR_LOG("%s", psocn_db_error(&conn));

    //        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
    //            ERR_BAD_ERROR, (uint8_t*)g_data, len);
    //        return -2;
    //    }

    //    if ((row = psocn_db_result_fetch(result)) == NULL) {
    //        psocn_db_result_free(result);
    //        SQLERR_LOG("未找到保存的角色数据 (%u)", sender);
    //        SQLERR_LOG("%s", psocn_db_error(&conn));

    //        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
    //            ERR_BAD_ERROR, (uint8_t*)g_data, len);
    //        return -3;
    //    }


    //}



    //if (send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data)) {
    //    send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
    //        ERR_BAD_ERROR, (uint8_t*)g_data, len);
    //    return 0;
    //}

    display_packet((uint8_t*)g_data, len);
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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    //display_packet((uint8_t*)g_data, len);

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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    //display_packet((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会 完整公会数据*/
static int handle_bb_guild_full_data(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_full_guild_data_pkt* g_data = (bb_full_guild_data_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_full_guild_data_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    //用作以后验证

    //display_packet((uint8_t*)g_data, len);

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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会 点数 */
static int handle_bb_guild_buy_privilege_and_point_info(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_buy_privilege_and_point_info_pkt* g_data = (bb_guild_buy_privilege_and_point_info_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t num_mates, guild_id = pkt->fw_flags;
    size_t i = 0, j = 0, d = 0, size = 0x10 + sizeof(bb_pkt_hdr_t), entries_size = sizeof(bb_guild_point_info_list_t);
    void* result;
    char** row;

    /* 更新BB公会排行榜 */
    if (db_update_bb_guild_ranks(&conn)) {
        SQLERR_LOG("更新公会排行榜失败");

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    if (guild_id != db_get_bb_char_guild_id(sender)) {
        SQLERR_LOG("获取公会ID失败 %d", guild_id);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    sprintf_s(myquery, _countof(myquery), "SELECT %s.guildcard, %s.guild_priv_level, "
        "lastchar_blob, guild_points_personal_donation, "
        "%s.guild_points_rank, guild_points_rest, "
        "guild_reward0, guild_reward1, guild_reward2, guild_reward3, "
        "guild_reward4, guild_reward5, guild_reward6, guild_reward7"
        " FROM "
        "%s INNER JOIN %s ON "
        "%s.guild_id = %s.guild_id WHERE "
        "%s.guild_id = '%" PRIu32 "' ORDER BY guild_points_personal_donation DESC"
        , AUTH_ACCOUNT, AUTH_ACCOUNT
        , CLIENTS_GUILD
        , AUTH_ACCOUNT, CLIENTS_GUILD
        , AUTH_ACCOUNT, CLIENTS_GUILD
        , CLIENTS_GUILD, guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("未能找到GC %u 的公会成员信息", sender);
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    result = psocn_db_result_store(&conn);
    num_mates = (uint32_t)psocn_db_result_rows(result);

    g_data->guild_member_amount = num_mates;

    sprintf(&g_data->unk_data2[0], "\x01\x00\x00\x00");

    for (i = 0; i < num_mates; i++) {
        row = psocn_db_result_fetch(result);

        g_data->guild_points_rank = (uint32_t)strtoul(row[4], NULL, 0);
        g_data->guild_points_rest = (uint32_t)strtoul(row[5], NULL, 0);

        g_data->entries[i].member_list.member_index = i + 1;
        g_data->entries[i].member_list.guild_priv_level = (uint32_t)strtoul(row[1], NULL, 0);
        g_data->entries[i].member_list.guildcard_client = (uint32_t)strtoul(row[0], NULL, 0);
        memcpy(&g_data->entries[i].member_list.char_name[0], row[2], BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
        g_data->entries[i].member_list.char_name[1] = 0x0045; //颜色代码
        //g_data->entries[i].member_list.guild_reward[0] = (uint32_t)strtoul(row[6], NULL, 0);
        //g_data->entries[i].member_list.guild_reward[1] = (uint32_t)strtoul(row[7], NULL, 0);
        //g_data->entries[i].member_list.guild_reward[2] = (uint32_t)strtoul(row[8], NULL, 0);
        //g_data->entries[i].member_list.guild_reward[3] = (uint32_t)strtoul(row[9], NULL, 0);
        //g_data->entries[i].member_list.guild_reward[4] = (uint32_t)strtoul(row[10], NULL, 0);
        //g_data->entries[i].member_list.guild_reward[5] = (uint32_t)strtoul(row[11], NULL, 0);
        //g_data->entries[i].member_list.guild_reward[6] = (uint32_t)strtoul(row[12], NULL, 0);
        //g_data->entries[i].member_list.guild_reward[7] = (uint32_t)strtoul(row[13], NULL, 0);

        g_data->entries[i].guild_points_personal_donation = (uint32_t)strtoul(row[3], NULL, 0);

        size += entries_size;
    }

    psocn_db_result_free(result);

    g_data->hdr.pkt_len = LE16((uint16_t)size);
    g_data->hdr.pkt_type = LE16(BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO);
    g_data->hdr.flags = 0x00000000;

    if (send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data)) {
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);

    if (send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data)) {
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    return 0;
}

/* 处理 Blue Burst 公会  */
static int handle_bb_guild_buy_special_item(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_buy_special_item_pkt* g_data = (bb_guild_buy_special_item_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    if (len != sizeof(bb_guild_buy_special_item_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);

    return 0;
}

/* 处理 Blue Burst 公会 排行榜 */
static int handle_bb_guild_rank_list(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_guild_rank_list_pkt* g_data = (bb_guild_rank_list_pkt*)pkt->pkt;
    uint16_t type = LE16(g_data->hdr.pkt_type);
    uint16_t len = LE16(g_data->hdr.pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

    /* 更新BB公会排行榜 */
    if (db_update_bb_guild_ranks(&conn)) {
        SQLERR_LOG("更新公会排行榜失败");

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    if (len != sizeof(bb_guild_rank_list_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    TEST_LOG("handle_bb_guild_rank_list guild_id = %u", pkt->fw_flags);

    if (send_bb_pkt_to_ship(c, sender, (uint8_t*)g_data)) {
        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);

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
        display_packet((uint8_t*)g_data, len);

        send_error(c, SHDR_TYPE_BB, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)g_data, len);
        return 0;
    }

    display_packet((uint8_t*)g_data, len);
    return 0;
}

/* 处理 Blue Burst 公会功能 */
static int handle_bb_guild(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_pkt_hdr_t* hdr = (bb_pkt_hdr_t*)pkt->pkt;
    uint16_t type = LE16(hdr->pkt_type);
    uint16_t len = LE16(hdr->pkt_len);
    uint32_t sender = ntohl(pkt->guildcard);

#ifdef DEBUG_GUILD
    DBG_LOG("舰闸:BB公会指令 0x%04X %s (长度%d)", type, c_cmd_name(type, 0), len);
#endif // DEBUG_GUILD

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

    case BB_GUILD_INVITE:
        return handle_bb_guild_invite_0DEA(c, pkt);

    case BB_GUILD_UNK_0EEA:
        return handle_bb_guild_unk_0EEA(c, pkt);

    case BB_GUILD_MEMBER_FLAG_SETTING:
        return handle_bb_guild_member_flag_setting(c, pkt);

    case BB_GUILD_DISSOLVE:
        return handle_bb_guild_dissolve(c, pkt);

    case BB_GUILD_MEMBER_PROMOTE:
        return handle_bb_guild_member_promote(c, pkt);

    case BB_GUILD_INITIALIZATION_DATA:
        return handle_bb_guild_initialization_data(c, pkt);

    case BB_GUILD_LOBBY_SETTING:
        return handle_bb_guild_lobby_setting(c, pkt);

    case BB_GUILD_MEMBER_TITLE:
        return handle_bb_guild_member_tittle(c, pkt);

    case BB_GUILD_FULL_DATA:
        return handle_bb_guild_full_data(c, pkt);

    case BB_GUILD_UNK_16EA:
        return handle_bb_guild_unk_16EA(c, pkt);

    case BB_GUILD_UNK_17EA:
        return handle_bb_guild_unk_17EA(c, pkt);

    case BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO:
        return handle_bb_guild_buy_privilege_and_point_info(c, pkt);

    case BB_GUILD_PRIVILEGE_LIST:
        return handle_bb_guild_privilege_list(c, pkt);

    case BB_GUILD_BUY_SPECIAL_ITEM:
        return handle_bb_guild_buy_special_item(c, pkt);

    case BB_GUILD_UNK_1BEA:
        return handle_bb_guild_unk_1BEA(c, pkt);

    case BB_GUILD_RANKING_LIST:
        return handle_bb_guild_rank_list(c, pkt);

    case BB_GUILD_UNK_1DEA:
        return handle_bb_guild_unk_1DEA(c, pkt);

    default:
        UDONE_CPD(type, 0, pkt);
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

static int handle_bb_cmode_char_data(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_full_char_pkt* char_full = (bb_full_char_pkt*)pkt->pkt;
    psocn_bb_full_char_t* cmode_char = &char_full->data;
    uint32_t qid = pkt->fw_flags;
    char char_class_name_text[64];

    DBG_LOG("Qid %d ch_class %d", qid, cmode_char->gc.char_class);

    istrncpy(ic_gbk_to_utf8, char_class_name_text, pso_class[cmode_char->gc.char_class].cn_name, sizeof(char_class_name_text));
    //display_packet(&cmode_char->character, sizeof(psocn_bb_char_t));

    if (db_insert_character_default_mode(&cmode_char->character, cmode_char->gc.char_class, qid, char_class_name_text)) {
        DBG_LOG("qid %d %s 数据已存在,进行更新操作", qid, pso_class[cmode_char->gc.char_class].cn_name);
        db_update_character_default_mode(&cmode_char->character, cmode_char->gc.char_class, qid);
    }

    return 0;
}

static int handle_bb_full_char_data(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_full_char_pkt* full_data_pkt = (bb_full_char_pkt*)pkt->pkt;
    uint32_t slot = pkt->fw_flags, gc = ntohl(pkt->guildcard);
    char char_class_name_text[64];

#ifdef DEBUG
    DBG_LOG("slot %d ch_class %d", slot, full_data_pkt->data.gc.char_class);
    display_packet(full_data_pkt, PSOCN_STLENGTH_BB_FULL_CHAR);
#endif // DEBUG

    istrncpy(ic_gbk_to_utf8, char_class_name_text, pso_class[full_data_pkt->data.gc.char_class].cn_name, sizeof(char_class_name_text));
    
    if (db_insert_bb_full_char_data(full_data_pkt, gc, slot, full_data_pkt->data.gc.char_class, char_class_name_text)) {
        //DBG_LOG("qid %d %s 数据已存在,进行更新操作", slot, pso_class[full_data_pkt->data.gc.char_class].cn_name);
        db_update_bb_full_char_data(full_data_pkt, gc, slot, full_data_pkt->data.gc.char_class, char_class_name_text);
    }

    return 0;
}

static int handle_bb(ship_t* c, shipgate_fw_9_pkt* pkt) {
    bb_pkt_hdr_t* hdr = (bb_pkt_hdr_t*)pkt->pkt;
    uint16_t type = LE16(hdr->pkt_type);
    uint16_t len = LE16(hdr->pkt_len);
#ifdef DEBUG
    DBG_LOG("舰船:BB指令 0x%04X %s %d 字节", type, c_cmd_name(type, 0), len);
#endif // DEBUG

    /* 整合为综合指令集 */
    switch (type & 0x00FF) {
        /* 0x00DF 挑战模式 */
    //case BB_CHALLENGE_DF:
    //    return handle_bb_challenge(c, pkt);

        /* 0x00EA 公会功能 */
    case BB_GUILD_COMMAND:
        return handle_bb_guild(c, pkt);

    case BB_FULL_CHARACTER_TYPE:
        if(pkt->fw_flags > 3)
            return handle_bb_cmode_char_data(c, pkt);
        else
            return handle_bb_full_char_data(c, pkt);

    default:
        break;
    }

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
static int handle_char_data_save(ship_t* c, shipgate_char_data_pkt* pkt) {
    uint32_t gc, slot;
    uint16_t data_len = ntohs(pkt->hdr.pkt_len) - sizeof(shipgate_char_data_pkt);
    psocn_bb_db_char_t* char_data = (psocn_bb_db_char_t*)pkt->data;

#ifdef DEBUG

    display_packet(&pkt->data[0], PSOCN_STLENGTH_BB_DB_CHAR);
    DBG_LOG("数据存储");

#endif // DEBUG

    gc = ntohl(pkt->guildcard);
    slot = ntohl(pkt->slot);

    if (gc == 0) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }

    if (db_update_char_inv(&char_data->character.inv, gc, slot)) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        SQLERR_LOG("无法更新玩家背包数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 ")", gc, slot);
        return 0;
    }

    if (db_update_char_disp(&char_data->character.disp, gc, slot, PSOCN_DB_UPDATA_CHAR)) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        SQLERR_LOG("无法更新玩家数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 ")", gc, slot);
        return 0;
    }

    if (db_update_char_dress_data(&char_data->character.dress_data, gc, slot, PSOCN_DB_UPDATA_CHAR)) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        SQLERR_LOG("无法更新玩家外观数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 ")", gc, slot);
        return 0;
    }

    if (db_update_char_name(&char_data->character.name, gc, slot)) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        SQLERR_LOG("无法更新玩家名字数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 ")", gc, slot);
        return 0;
    }

    if (db_update_char_techniques(&char_data->character.tech, gc, slot, PSOCN_DB_UPDATA_CHAR)) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        SQLERR_LOG("无法更新玩家科技数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 ")", gc, slot);
        return 0;
    }

    if (db_update_char_bank(&char_data->bank, gc, slot)) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        SQLERR_LOG("无法更新玩家银行数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 ")", gc, slot);
        return 0;
    }

    if (db_update_char_b_records(&char_data->b_records, gc, slot, PSOCN_DB_UPDATA_CHAR)) {
        SQLERR_LOG("无法保存角色挑战数据 (%" PRIu32 ": %" PRIu8 ")", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }

    if (db_update_char_c_records(&char_data->c_records, gc, slot, PSOCN_DB_UPDATA_CHAR)) {
        SQLERR_LOG("无法保存角色挑战数据 (%" PRIu32 ": %" PRIu8 ")", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }

    if (db_update_char_quest_data1(char_data->quest_data1, gc, slot, PSOCN_DB_UPDATA_CHAR)) {
        SQLERR_LOG("无法保存角色挑战数据 (%" PRIu32 ": %" PRIu8 ")", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }

    if (db_update_gc_login_state(gc, 0, -1, (char*)&char_data->character.name)) {
        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return 0;
    }

    if (db_compress_char_data(char_data, data_len, gc, slot)) {
        ERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 ")", CHARACTER, gc, slot);

        send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        return -6;
    }

    /* Return success (yeah, bad use of this function, but whatever). */
    return send_error(c, SHDR_TYPE_CDATA, SHDR_RESPONSE, ERR_NO_ERROR,
        (uint8_t*)&pkt->guildcard, 8);
}

static int handle_char_data_backup_req(ship_t* c, shipgate_char_bkup_pkt* pkt, uint32_t gc,
    const char name[], uint32_t block) {
    char query[256];
    char name2[65];
    uint8_t* data;
    void* result;
    char** row;
    unsigned long* len;
    int sz, rv;
    uLong sz2, csz;
    int slot = pkt->game_info.slot, version = ntohl(pkt->game_info.c_version);
    int dbversion = 0, dbslot = 0;

    /* Build the query asking for the data. */
    psocn_db_escape_str(&conn, name2, name, strlen(name));
    sprintf(query, "SELECT data, size, version, slot FROM %s WHERE "
        "guildcard='%u' AND name='%s'", CHARACTER_BACKUP, gc, name2);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法获取角色备份数据 (%u: %s)", gc, name);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未查询到角色备份数据 (%u: %s)", gc, name);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到角色备份数据 (%u: %s)", gc, name);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_CREQ_NO_DATA, (uint8_t*)&pkt->game_info.guildcard, 8);
        return 0;
    }

    dbversion = atoi(row[2]);
    dbslot = atoi(row[3]);

    if (dbversion != version) {
        psocn_db_result_free(result);
        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
        //SQLERR_LOG("角色备份数据版本不匹配 (%u: %s) 数据版本 %d 请求版本 %d", gc, name, dbversion, version);

        //send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
        //    ERR_CREQ_NO_DATA, (uint8_t*)&pkt->game_info.guildcard, 8);
        return 0;
    }

    if (dbslot != slot) {
        psocn_db_result_free(result);
        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
        //SQLERR_LOG("角色备份数据不存在 (%u: %s) 数据槽位 %d 请求槽位 %d", gc, name, dbslot, slot);

        //send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
        //    ERR_CREQ_NO_DATA, (uint8_t*)&pkt->game_info.guildcard, 8);
        return 0;
    }

    /* Grab the length of the character data */
    if (!(len = psocn_db_result_lengths(result))) {
        psocn_db_result_free(result);
        SQLERR_LOG("无法获取角色备份数据的长度 %s", psocn_db_error(&conn));
        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
        return 0;
    }

    /* Grab the data from the result */
    sz = (int)len[0];

    if (row[1]) {
        sz2 = (uLong)atoi(row[1]);
        csz = (uLong)sz;

        data = (uint8_t*)malloc(sz2);
        if (!data) {
            SQLERR_LOG("无法为解压角色备份数据分配内存空间");
            SQLERR_LOG("%s", strerror(errno));
            psocn_db_result_free(result);

            send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
            return 0;
        }

        /* 解压缩数据 */
        if (uncompress((Bytef*)data, &sz2, (Bytef*)row[0], csz) != Z_OK) {
            SQLERR_LOG("无法解压角色备份数据");
            psocn_db_result_free(result);

            send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
            return 0;
        }

        sz = sz2;
    }
    else {
        data = (uint8_t*)malloc(sz);
        if (!data) {
            SQLERR_LOG("无法分配玩家备份内存");
            SQLERR_LOG("%s", strerror(errno));
            psocn_db_result_free(result);

            send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
            return 0;
        }

        memcpy(data, row[0], len[0]);
    }

    psocn_db_result_free(result);

    /* 将数据发回舰船. */
    rv = send_cdata(c, gc, (uint32_t)-1, data, sz, block);

    /* 清理内存并结束 */
    free_safe(data);
    return rv;
}

static int handle_char_data_backup(ship_t* c, shipgate_char_bkup_pkt* pkt) {
    static char query[16384];
    uint32_t gc, block, version;
    int slot;
    uint16_t len = ntohs(pkt->hdr.pkt_len) - sizeof(shipgate_char_bkup_pkt);
    char name[32], name2[65];
    Bytef* cmp_buf;
    uLong cmp_sz;
    int compressed = ~Z_OK;

    slot = pkt->game_info.slot;
    gc = ntohl(pkt->game_info.guildcard);
    block = ntohl(pkt->game_info.block);
    version = ntohl(pkt->game_info.c_version);

    strncpy(name, (const char*)pkt->game_info.name, 32);
    name[31] = 0;

    /* Is this a restore request or are we saving the character data? */
    if (len == 0) {
        return handle_char_data_backup_req(c, pkt, gc, name, block);
    }

    /* Is it a Blue Burst character or not? */
    if (len > 1056) {
        len = PSOCN_STLENGTH_BB_DB_CHAR;
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
        sprintf(query, "INSERT INTO %s(guildcard, version, slot, size, name, "
            "data) VALUES ('%u', '%u', '%u', '%u', '%s', '", CHARACTER_BACKUP, gc, version, slot, (unsigned)len, name2);
        psocn_db_escape_str(&conn, query + strlen(query), (char*)cmp_buf,
            cmp_sz);
    }
    else {
        sprintf(query, "INSERT INTO %s(guildcard, version, slot, name, data) "
            "VALUES ('%u', '%u', '%u', '%s', '", CHARACTER_BACKUP, gc, version, slot, name2);
        psocn_db_escape_str(&conn, query + strlen(query), (char*)pkt->data,
            len);
    }

    strcat(query, "') ON DUPLICATE KEY UPDATE data=VALUES(data)");
    free_safe(cmp_buf);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法保存玩家备份 (%u: %s)", gc, name);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE | SHDR_FAILURE,
            ERR_BAD_ERROR, (uint8_t*)&pkt->game_info.guildcard, 8);
        return 0;
    }

    /* Return success (yeah, bad use of this function, but whatever). */
    return send_error(c, SHDR_TYPE_CBKUP, SHDR_RESPONSE, ERR_NO_ERROR,
        (uint8_t*)&pkt->game_info.guildcard, 8);
}

void repair_client_character_data(psocn_bb_char_t* character) {

    uint16_t maxFace, maxHair, maxHairColorRed, maxHairColorBlue, maxHairColorGreen,
        maxCostume, maxSkin, maxHead;

    // 修复格式错误的数据
    character->name.name_tag = 0x0009;
    character->name.name_tag2 = 0x0045;

    if (character->disp.level > 199) {
        character->disp.level = 199;
    }

    if ((character->dress_data.ch_class == CLASS_HUMAR) ||
        (character->dress_data.ch_class == CLASS_HUNEWEARL) ||
        (character->dress_data.ch_class == CLASS_RAMAR) ||
        (character->dress_data.ch_class == CLASS_RAMARL) ||
        (character->dress_data.ch_class == CLASS_FOMARL) ||
        (character->dress_data.ch_class == CLASS_FONEWM) ||
        (character->dress_data.ch_class == CLASS_FONEWEARL) ||
        (character->dress_data.ch_class == CLASS_FOMAR)) {
        maxFace = 0x05;
        maxHair = 0x0A;
        maxHairColorRed = 0xFF;
        maxHairColorBlue = 0xFF;
        maxHairColorGreen = 0xFF;
        maxCostume = 0x11;
        maxSkin = 0x03;
        maxHead = 0x00;
    } else {
        maxFace = 0x00;
        maxHair = 0x00;
        maxHairColorRed = 0x00;
        maxHairColorBlue = 0x00;
        maxHairColorGreen = 0x00;
        maxCostume = 0x00;
        maxSkin = 0x18;
        maxHead = 0x04;
    }

    character->dress_data.name_color_transparency = 0xFF;

    if (character->dress_data.section > 0x09)
        character->dress_data.section = rand() % 10;
    if (character->dress_data.prop_x > 0x3F800000)
        character->dress_data.prop_x = 0x3F800000;
    if (character->dress_data.prop_y > 0x3F800000)
        character->dress_data.prop_y = 0x3F800000;
    if (character->dress_data.face > maxFace)
        character->dress_data.face = rand() % maxFace;
    if (character->dress_data.hair > maxHair)
        character->dress_data.hair = rand() % maxHair;
    if (character->dress_data.hair_r > maxHairColorRed)
        character->dress_data.hair_r = rand() % maxHairColorRed;
    if (character->dress_data.hair_g > maxHairColorBlue)
        character->dress_data.hair_g = rand() % maxHairColorBlue;
    if (character->dress_data.hair_b > maxHairColorGreen)
        character->dress_data.hair_b = rand() % maxHairColorGreen;
    if (character->dress_data.costume > maxCostume)
        character->dress_data.costume = rand() % maxCostume;
    if (character->dress_data.skin > maxSkin)
        character->dress_data.skin = rand() % maxSkin;
    if (character->dress_data.head > maxHead)
        character->dress_data.head = rand() % maxHead;
}

/* 处理舰船获取角色数据请求. 目前仅限于PSOBB使用*/
static int handle_char_data_req(ship_t* c, shipgate_char_req_pkt* pkt) {
    __try
    {
        uint32_t gc, slot;
        int rv;
        gc = ntohl(pkt->guildcard);
        slot = ntohl(pkt->slot);

        psocn_bb_db_char_t* bb_data = (psocn_bb_db_char_t*)malloc(PSOCN_STLENGTH_BB_DB_CHAR);

        if (!bb_data) {
            ERR_LOG("无法分配角色数据的内存空间");
            ERR_LOG("%s", strerror(errno));

            send_error(c, SHDR_TYPE_CREQ, SHDR_RESPONSE | SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
            return 0;
        }

        memset(bb_data, 0, PSOCN_STLENGTH_BB_DB_CHAR);

        bb_data = db_get_uncompress_char_data(gc, slot);

        /* 从数据库中获取玩家角色的背包数据 */
        if ((rv = db_get_char_inv(gc, slot, &bb_data->character.inv, 0))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色背包数据, 错误码:%d", gc, slot, rv);
        }

        /* 从数据库中获取玩家角色数值数据 */
        if ((rv = db_get_char_disp(gc, slot, &bb_data->character.disp, 0))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色数值数据, 错误码:%d", gc, slot, rv);
            db_update_char_disp(&bb_data->character.disp, gc, slot, PSOCN_DB_SAVE_CHAR);
        }

        /* 从数据库中获取玩家角色外观数据 */
        if ((rv = db_get_dress_data(gc, slot, &bb_data->character.dress_data, 0))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色外观数据, 错误码:%d", gc, slot, rv);
            db_update_char_dress_data(&bb_data->character.dress_data, gc, slot, PSOCN_DB_SAVE_CHAR);
        }

        /* 从数据库中获取玩家角色名称数据 */
        if ((rv = db_get_char_name(gc, slot, &bb_data->character.name))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色名称数据, 错误码:%d", gc, slot, rv);
            db_update_char_name(&bb_data->character.name, gc, slot);
        }

        /* 防止玩家名称内存溢出 */
        bb_data->character.padding = 0;

        /* 从数据库中获取玩家角色科技数据 */
        if ((rv = db_get_char_techniques(gc, slot, &bb_data->character.tech, 0))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色科技数据, 错误码:%d", gc, slot, rv);
            db_update_char_techniques(&bb_data->character.tech, gc, slot, PSOCN_DB_SAVE_CHAR);
        }

        /* 从数据库中获取玩家角色的QUEST_DATA1数据 */
        if ((rv = db_get_char_quest_data1(gc, slot, bb_data->quest_data1, 0))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色QUEST_DATA1数据, 错误码:%d", gc, slot, rv);
        }

        /* 从数据库中获取玩家角色的银行数据 */
        if ((rv = db_get_char_bank(gc, slot, &bb_data->bank))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色银行数据, 错误码:%d", gc, slot, rv);
        }

        /* 从数据库中获取玩家角色的b_records数据 */
        if ((rv = db_get_b_records(gc, slot, &bb_data->b_records))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色b_records数据, 错误码:%d", gc, slot, rv);
            db_update_char_b_records(&bb_data->b_records, gc, slot, PSOCN_DB_SAVE_CHAR);
        }

        /* 从数据库中获取玩家角色的b_records数据 */
        if ((rv = db_get_c_records(gc, slot, &bb_data->c_records))) {
            SQLERR_LOG("无法获取(GC%u:%u槽)角色c_records数据, 错误码:%d", gc, slot, rv);
            db_update_char_c_records(&bb_data->c_records, gc, slot, PSOCN_DB_SAVE_CHAR);
        }

        //repair_client_character_data(&bb_data->character);

        /* 将数据发回舰船. */
        rv = send_cdata(c, gc, slot, bb_data, PSOCN_STLENGTH_BB_DB_CHAR, 0);

        /* 清理内存并结束 */
        free_safe(bb_data);
        return rv;
    }
    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。
        ERR_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

/* 处理客户端来自舰船的登陆请求. */
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
        , AUTH_ACCOUNT_GUILDCARDS
        , AUTH_ACCOUNT, gc, esc);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("登录失败 - 查询 %s 数据表失败 (玩家: %s, gc: %u)", AUTH_ACCOUNT_GUILDCARDS,
            pkt->username, gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("登录失败 - 玩家 %s 数据不存在 (玩家: %s, gc: %u)", AUTH_ACCOUNT_GUILDCARDS,
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


    if (db_update_gc_login_state(gc, islogged, -1, NULL)) {
        return -4;
    }

    /* We're done if we got this far. */
    psocn_db_result_free(result);

    /* Send a success message. */
    return send_usrloginreply(c, gc, block, 1, priv);
}

/* 处理来自舰船的封禁数据. */
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
        , AUTH_ACCOUNT_GUILDCARDS
        , AUTH_ACCOUNT, req);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法从 %s 获取到数据 (%u)", AUTH_ACCOUNT_GUILDCARDS, req);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法从 %s 获取到结果 (%u)", AUTH_ACCOUNT_GUILDCARDS, req);
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
        "WHERE guildcard='%u'", AUTH_ACCOUNT_GUILDCARDS, AUTH_ACCOUNT, target);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法从 %s 获取到数据 (%u)", AUTH_ACCOUNT_GUILDCARDS, target);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法从 %s 获取到结果 (%u)", AUTH_ACCOUNT_GUILDCARDS, target);
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
        "('%u', '%u', '", AUTH_BANS, until, account_id);
    psocn_db_escape_str(&conn, query + strlen(query), (char*)pkt->message,
        strlen(pkt->message));
    strcat(query, "')");

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法向 %s 插入封禁数据", AUTH_BANS);
        SQLERR_LOG("%s", psocn_db_error(&conn));

        return send_error(c, type, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->req_gc, 16);
    }

    /* Now that we have that, add the ban to the right table... */
    switch (type) {
    case SHDR_TYPE_GCBAN:
        sprintf(query, "INSERT INTO %s(ban_id, guildcard) "
            "VALUES(LAST_INSERT_ID(), '%lu')", AUTH_BANS_GC, ntohl(pkt->target));
        break;

    case SHDR_TYPE_IPBAN:
        sprintf(query, "INSERT INTO %s(ban_id, addr) VALUES("
            "LAST_INSERT_ID(), '%lu')", AUTH_BANS_IP, ntohl(pkt->target));
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
    char tmp_name[128];
    uint32_t gc, bl, gc2, bl2, opt, acc = 0;
    uint32_t slot;
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
        inptr = (char*)&pkt->ch_name[4];
        outptr = name;

        istrncpy16_raw(ic_utf16_to_utf8, name, inptr, out, in);

        //iconv(ic_utf16_to_gb18030, &inptr, &in, &outptr, &out);
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
    slot = ntohl(pkt->slot);
    bl = ntohl(pkt->blocknum);

    /* Is this a transient client (that is to say someone on the PC NTE)? */
    if (gc >= 500 && gc < 600)
        tbl_nm = SERVER_CLIENTS_TRANSIENT;

    /* Insert the client into the online_clients table */
    psocn_db_escape_str(&conn, tmp_name, name, strlen(name));
    sprintf(query, "INSERT INTO %s(guildcard, name, ship_id, "
        "block) VALUES('%u', '%s', '%hu', '%u')", tbl_nm,
        gc, tmp_name, c->key_idx, bl);

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

    if (db_update_gc_char_last_block(gc, slot, bl)) {
        SQLERR_LOG("GC %u 槽位 %d 存在非法玩家数据", (uint8_t*)&pkt->guildcard, slot);
        return send_error(c, SHDR_TYPE_BLKLOGIN, SHDR_FAILURE,
            ERR_BLOGIN_ONLINE, (uint8_t*)&pkt->guildcard, 8);
    }

    if (db_update_gc_char_login_state(gc, 1)) {
        SQLERR_LOG("GC %u 槽位 %d 存在非法玩家数据", (uint8_t*)&pkt->guildcard, slot);
        return send_error(c, SHDR_TYPE_BLKLOGIN, SHDR_FAILURE,
            ERR_BLOGIN_ONLINE, (uint8_t*)&pkt->guildcard, 8);
    }

    /* Find anyone that has the user in their friendlist so we can send a
       message to them */
    sprintf(query, "SELECT guildcard, block, ship_id, nickname FROM "
        "%s INNER JOIN %s ON "
        "%s.guildcard = %s.owner WHERE "
        "%s.friend = '%u'"
        , SERVER_CLIENTS_ONLINE, CHARACTER_FRIENDLIST
        , SERVER_CLIENTS_ONLINE, CHARACTER_FRIENDLIST
        , CHARACTER_FRIENDLIST, gc
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
        "guildcard='%u'", CLIENTS_OPTION, gc);

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
        PRIu32 "'", AUTH_ACCOUNT_GUILDCARDS, gc);

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
        , SERVER_SIMPLE_MAIL, AUTH_ACCOUNT_GUILDCARDS
        , SERVER_SIMPLE_MAIL, AUTH_ACCOUNT_GUILDCARDS
        , AUTH_ACCOUNT_GUILDCARDS, acc, SERVER_SIMPLE_MAIL
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
        , SERVER_SIMPLE_MAIL, AUTH_ACCOUNT_GUILDCARDS
        , SERVER_SIMPLE_MAIL, AUTH_ACCOUNT_GUILDCARDS
        , SERVER_SIMPLE_MAIL, AUTH_ACCOUNT_GUILDCARDS
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
    uint32_t slot;
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
        inptr = (char*)&pkt->ch_name[4];
        outptr = name;

        istrncpy16_raw(ic_utf16_to_utf8, outptr, inptr, out, in);

        //iconv(ic_utf16_to_gb18030, &inptr, &in, &outptr, &out);
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
    slot = ntohl(pkt->slot);
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

    if (db_update_gc_char_last_block(gc, slot, bl)) {
        SQLERR_LOG("GC %u 槽位 %d 存在非法玩家数据", (uint8_t*)&pkt->guildcard, slot);
        return send_error(c, SHDR_TYPE_BLKLOGIN, SHDR_FAILURE,
            ERR_BLOGIN_ONLINE, (uint8_t*)&pkt->guildcard, 8);
    }

    if (db_update_gc_char_login_state(gc, 0)) {
        SQLERR_LOG("GC %u 槽位 %d 存在非法玩家数据", (uint8_t*)&pkt->guildcard, slot);
        return send_error(c, SHDR_TYPE_BLKLOGIN, SHDR_FAILURE,
            ERR_BLOGIN_ONLINE, (uint8_t*)&pkt->guildcard, 8);
    }

    /* Delete the client from the online_clients table */
    sprintf(query, "DELETE FROM %s WHERE guildcard='%u' AND "
        "ship_id='%hu'", SERVER_CLIENTS_ONLINE, gc, c->key_idx);

    if (psocn_db_real_query(&conn, query)) {
        return 0;
    }

    /* Find anyone that has the user in their friendlist so we can send a
       message to them */
    sprintf(query, "SELECT guildcard, block, ship_id, nickname FROM "
        "%s INNER JOIN %s ON "
        "%s.guildcard = %s.owner WHERE "
        "%s.friend = '%u'"
        , SERVER_CLIENTS_ONLINE, CHARACTER_FRIENDLIST
        , SERVER_CLIENTS_ONLINE, CHARACTER_FRIENDLIST
        , CHARACTER_FRIENDLIST, gc
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
        "VALUES('%u', '%u', '%s')", CHARACTER_FRIENDLIST, ugc, fgc, nickname);

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
        , CHARACTER_FRIENDLIST, ugc, fgc);

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
    char tmp_lobby_name[128];
    uint32_t gc, lid;
    const char* tbl_nm = SERVER_CLIENTS_ONLINE;

    /* Make sure the name is terminated properly */
    pkt->lobby_name[31] = 0;

    /* Parse out some stuff we'll use */
    gc = ntohl(pkt->guildcard);
    lid = ntohl(pkt->lobby_id);

    /* Update the client's entry */
    psocn_db_escape_str(&conn, tmp_lobby_name, pkt->lobby_name,
        strlen(pkt->lobby_name));

    /* Is this a transient client (that is to say someone on the PC NTE)? */
    if (gc >= 500 && gc < 600)
        tbl_nm = SERVER_CLIENTS_TRANSIENT;


    if (lid > 20) {
        sprintf(query, "UPDATE %s SET lobby_id='%u', lobby='%s' "
            "WHERE guildcard='%u' AND ship_id='%hu'", tbl_nm, lid, tmp_lobby_name, gc,
            c->key_idx);
    }
    else {
        sprintf(query, "UPDATE %s SET lobby_id='%u', lobby='%s', "
            "dlobby_id='%u' WHERE guildcard='%u' AND ship_id='%hu'", tbl_nm,
            lid, tmp_lobby_name, lid, gc, c->key_idx);
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
        , AUTH_ACCOUNT, AUTH_ACCOUNT_GUILDCARDS
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
        "WHERE guildcard='%u'", AUTH_ACCOUNT_GUILDCARDS, AUTH_ACCOUNT
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
    friendlist_data_t entries[5] = { 0 };
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
        , CHARACTER_FRIENDLIST
        , SERVER_CLIENTS_ONLINE, CHARACTER_FRIENDLIST
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
        , AUTH_ACCOUNT, AUTH_ACCOUNT_GUILDCARDS
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
        "value=VALUES(value)", CLIENTS_OPTION, ugc, opttype, data);

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
    uint32_t guild_points_personal_donation = 0;
    psocn_bank_t* common_bank = (psocn_bank_t*)malloc(PSOCN_STLENGTH_BANK);
    int rv = 0;

    /* Parse out the guildcard */
    gc = ntohl(pkt->guildcard);
    block = ntohl(pkt->block);

    opts = db_get_bb_char_option(gc);
    guild = db_get_bb_char_guild(gc);
    guild_points_personal_donation = db_get_bb_guild_points_personal_donation(gc);

    if (common_bank) {
        memset(common_bank, 0, PSOCN_STLENGTH_BANK);
        db_get_char_bank_common(gc, common_bank);
    }

    rv = send_bb_opts(c, gc, block, &opts, &guild, guild_points_personal_donation, common_bank);

    if (rv) {
        rv = send_error(c, SHDR_TYPE_BBOPTS, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    free_safe(common_bank);

    return rv;
}

static int handle_bbopts(ship_t* c, shipgate_bb_opts_pkt* pkt) {
    uint32_t gc;
    int rv = 0;

    /* Parse out the guildcard */
    gc = ntohl(pkt->guildcard);

    rv = db_update_bb_char_option(pkt->opts, gc);

    rv = db_update_bb_guild_points_personal_donation(gc, pkt->guild_points_personal_donation);

    rv = db_update_char_bank_common(&pkt->common_bank, gc);

    if (rv) {
        rv = send_error(c, SHDR_TYPE_BBOPTS, SHDR_FAILURE, ERR_BAD_ERROR,
            (uint8_t*)&pkt->guildcard, 8);
    }

    return rv;
}

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
        PRIu32 "'", AUTH_ACCOUNT_GUILDCARDS, gc);

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
        "MINUTE < NOW()", AUTH_LOGIN_TOKENS);

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
    if (pkt->hdr.version == TLOGIN_VER_NORMAL) {
        sprintf(query, "SELECT privlevel, account_id FROM %s NATURAL "
            "JOIN %s NATURAL JOIN %s WHERE "
            "guildcard='%u' AND username='%s' AND token='%s'"
            , AUTH_ACCOUNT
            , AUTH_ACCOUNT_GUILDCARDS, AUTH_LOGIN_TOKENS, gc
            , esc, esc2);
    }
    else {
        sprintf(query, "SELECT privlevel, %s.account_id FROM "
            "%s NATURAL JOIN %s, %s NATURAL "
            "JOIN %s WHERE username='%s' AND token='%s' AND "
            "guildcard='%u' AND %s.account_id is NULL", AUTH_ACCOUNT
            , AUTH_ACCOUNT, AUTH_LOGIN_TOKENS, AUTH_ACCOUNT_GUILDCARDS
            , AUTH_ACCOUNT_XBOX, esc, esc2
            , gc, AUTH_ACCOUNT_GUILDCARDS);
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
    if (pkt->hdr.version == TLOGIN_VER_XBOX) {
        sprintf(query, "UPDATE %s SET account_id='%u' WHERE "
            "guildcard='%u' AND account_id is NULL", AUTH_ACCOUNT_GUILDCARDS, account_id, gc);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("Couldn't update guild card data (user: %s, "
                "gc: %u)", pkt->username, gc);
            SQLERR_LOG("%s", psocn_db_error(&conn));

            return send_error(c, SHDR_TYPE_USRLOGIN, SHDR_FAILURE,
                ERR_BAD_ERROR, (uint8_t*)&pkt->guildcard, 8);
        }
    }

    /* Delete the request. */
    sprintf(query, "DELETE FROM %s WHERE account_id='%u'"
        , AUTH_LOGIN_TOKENS, account_id);

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
    uint8_t tmp[6] = { 0 }, esc[43] = { 0 };
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
        PRIu32 "'", AUTH_ACCOUNT_GUILDCARDS, gc);

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

static int handle_ship_login6(ship_t* c, shipgate_hdr_t* pkt) {
    shipgate_login6_reply_pkt* p = (shipgate_login6_reply_pkt*)pkt;
    uint16_t length = ntohs(pkt->pkt_len);
    uint16_t type = ntohs(pkt->pkt_type);
    uint16_t flags = ntohs(pkt->flags);
    int rv;

    if (!(flags & SHDR_RESPONSE)) {
        ERR_LOG("舰船发送了无效的登录响应");
        return -1;
    }

    rv = handle_shipgate_login6t(c, p);

    /* TODO 从此处开始 增加各项数据从数据库获取后发送至舰船 */
    if (!rv) {
        send_player_max_tech_level_table_bb(c);

        send_player_level_table_bb(c);

        send_default_mode_char_data_bb(c);

        /* 最终给舰船发送一个 数据完成的响应 */
        send_recive_data_complete(c);

        SGATE_LOG("%s: 舰闸数据发送完成", c->name);
    }

    return rv;
}

static int handle_ship_ping(ship_t* c, shipgate_ping_t* pkt) {
    shipgate_ping_t* p = (shipgate_ping_t*)pkt;
    uint16_t length = ntohs(pkt->hdr.pkt_len);
    uint16_t type = ntohs(pkt->hdr.pkt_type);
    uint16_t flags = ntohs(pkt->hdr.flags);

    if (!(flags & SHDR_RESPONSE)) {
        ERR_LOG("舰船发送了无效的PING响应");
        return -1;
    }

    //DBG_LOG("handle_ship_ping %s", p->host4);

    return 0;
}

static int handle_char_common_bank_req(ship_t* c, shipgate_common_bank_data_pkt* pkt) {
    uint32_t sender = ntohl(pkt->guildcard);
    uint32_t sender_block = ntohl(pkt->block);
    uint32_t common_bank_req = ntohl(pkt->common_bank_req);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    DBG_LOG("%s", (common_bank_req ? "请求" :"存储"));


        /* 加密并发送 */
    return forward_bb(c, (bb_pkt_hdr_t*)&pkt, c->key_idx, sender, sender_block);
}

/* Process one ship packet. */
int process_ship_pkt(ship_t* c, shipgate_hdr_t* pkt) {
    __try
    {
        uint16_t length = ntohs(pkt->pkt_len);
        uint16_t type = ntohs(pkt->pkt_type);
        uint16_t flags = ntohs(pkt->flags);

#ifdef DEBUG
        DBG_LOG("G->S指令: 0x%04X %s 标志 = %d 长度 = %d", type, s_cmd_name(type, 0), flags, length);
        display_packet((unsigned char*)pkt, length);
#endif // DEBUG

        switch (type) {
        case SHDR_TYPE_LOGIN6:
            return handle_ship_login6(c, pkt);

        case SHDR_TYPE_COUNT:
            return handle_count(c, (shipgate_cnt_pkt*)pkt);

        case SHDR_TYPE_DC:
            return handle_dreamcast(c, (shipgate_fw_9_pkt*)pkt);

        case SHDR_TYPE_PC:
            return handle_pc(c, (shipgate_fw_9_pkt*)pkt);

        case SHDR_TYPE_BB:
            return handle_bb(c, (shipgate_fw_9_pkt*)pkt);

        case SHDR_TYPE_PING:

            if (handle_ship_ping(c, (shipgate_ping_t*)pkt)) {
                ERR_LOG("handle_ship_ping");
                return -1;
            }

            /* If this is a ping request, reply. Otherwise, ignore it, the work
               has already been done. */
            if (!(flags & SHDR_RESPONSE)) {
                return send_ping(c, 1);
            }

            return 0;

        case SHDR_TYPE_CDATA:
            return handle_char_data_save(c, (shipgate_char_data_pkt*)pkt);

        case SHDR_TYPE_CREQ:
            return handle_char_data_req(c, (shipgate_char_req_pkt*)pkt);

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
            return handle_char_data_backup(c, (shipgate_char_bkup_pkt*)pkt);

        case SHDR_TYPE_MKILL:
            return handle_mkill(c, (shipgate_mkill_pkt*)pkt);

        case SHDR_TYPE_TLOGIN:
            return handle_tlogin(c, (shipgate_usrlogin_req_pkt*)pkt);

        case SHDR_TYPE_SCHUNK:
            /* 合理性检查... */
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

        case SHDR_TYPE_CHECK_PLONLINE:
            DBG_LOG("收到查询指令");
            return 0;

        case SHDR_TYPE_BB_COMMON_BANK_DATA:
            DBG_LOG("收到公共仓库查询指令");
            return handle_char_common_bank_req(c, (shipgate_common_bank_data_pkt*)pkt);

        default:
            //DBG_LOG("G->S指令: 0x%04X %s 标志 = %d 长度 = %d", type, s_cmd_name(type, 0), flags, length);
            //display_packet((unsigned char*)pkt, length);
            return -3;
        }

    }
    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。
        ERR_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

// 接收消息
static ssize_t receive_message(ship_t* c, char* buffer, size_t buffer_size) {
    size_t total_len = 0;
    ssize_t ret;

    while (1) {
        // 接收数据
        ret = gnutls_record_recv(c->session, buffer + total_len, buffer_size - total_len);
        if (ret <= 0) {
            perror("gnutls_record_recv");
            return -1;
        }

        total_len += ret;

        // 判断是否接收完整消息
        if ((size_t)ret < buffer_size - total_len) {
            break;
        }
    }

    // 解压缩接收到的数据
    char* compressed_msg = buffer;
    uLongf decompressed_len = buffer_size;
    uncompress((Bytef*)buffer, &decompressed_len, (const Bytef*)compressed_msg, total_len);

    SGATE_LOG("Received data: %d 字节", decompressed_len);

    return ret;
}

static ssize_t ship_recv(ship_t* c, void* buffer, size_t len) {
    int ret;
    LOOP_CHECK(ret, gnutls_record_recv(c->session, buffer, len));
    return ret;
    //return gnutls_record_recv(c->session, buffer, len);
}

/* Retrieve the thread-specific recvbuf for the current thread. */
uint8_t* get_recvbuf(void) {
    uint8_t* recvbuf = (uint8_t*)malloc(65536);

    if (!recvbuf) {
        ERR_LOG("malloc");
        perror("malloc");
        return NULL;
    }

    memset(recvbuf, 0, 65536);

    return recvbuf;
}

/* Handle incoming data to the shipgate. */
int handle_pkt(ship_t* c) {
    ssize_t sz;
    uint16_t pkt_sz;
    int rv = 0;
    unsigned char* rbp;
    uint8_t* recvbuf = get_recvbuf();
    void* tmp;
    /* 确保8字节的倍数传输 */
    int recv_size = 8;

    /* If we've got anything buffered, copy it out to the main buffer to make
       the rest of this a bit easier. */
    if (c->recvbuf_cur) {
        memcpy(recvbuf, c->recvbuf, c->recvbuf_cur);

    }

    /* Attempt to read, and if we don't get anything, punt. */
    sz = ship_recv(c, recvbuf + c->recvbuf_cur,
                   65536 - c->recvbuf_cur);

    //DBG_LOG("从端口 %d 接收数据 %d 字节", c->sock, sz);

    /* Attempt to read, and if we don't get anything, punt. */
    if (sz <= 0) {
        if (sz == SOCKET_ERROR) {
            ERR_LOG("Gnutls *** 注意: SOCKET_ERROR");
        }else
            ERR_LOG("Gnutls *** 注意: ship_recv sz = %d", sz);

        goto end;
    }

    if (sz == SOCKET_ERROR) {
        DBG_LOG("Gnutls *** 注意: SOCKET_ERROR");
        goto end;
    }
    else if (sz == 0) {
        DBG_LOG("Gnutls *** 注意: 对等方已关闭TLS连接");
        goto end;
    }
    else if (sz < 0 && gnutls_error_is_fatal(sz) == 0) {
        ERR_LOG("Gnutls *** 警告: %s", gnutls_strerror(sz));
        goto end;
    }
    else if (sz < 0) {
        ERR_LOG("Gnutls *** 错误: %s", gnutls_strerror(sz));
        ERR_LOG("Gnutls *** 接收到损坏的数据(%d). 关闭连接.", sz);
        goto end;
    }
    else if (sz > 0) {
    sz += c->recvbuf_cur;
    c->recvbuf_cur = 0;
    rbp = recvbuf;

    /* As long as what we have is long enough, decrypt it. */
    if (sz >= recv_size) {
        while (sz >= recv_size && rv == 0) {
            /* Grab the packet header so we know what exactly we're looking
               for, in terms of packet length. */
            if (!c->hdr_read) {
                memcpy(&c->pkt, rbp, recv_size);
                c->hdr_read = 1;
            }

            pkt_sz = ntohs(c->pkt.pkt_len);

            /* Do we have the whole packet? */
            if (sz >= (ssize_t)pkt_sz) {
                /* Yep, copy it and process it */
                memcpy(rbp, &c->pkt, recv_size);

                /* Pass it onto the correct handler. */
                c->last_message = time(NULL);
                rv = process_ship_pkt(c, (shipgate_hdr_t*)rbp);
                if (rv)
                    ERR_LOG("process_ship_pkt rv = %d", rv);

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
    if (sz) {
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
        if (c->recvbuf)
            free_safe(c->recvbuf);
        c->recvbuf = NULL;
        c->recvbuf_size = 0;
    }

    return rv;
    }

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
