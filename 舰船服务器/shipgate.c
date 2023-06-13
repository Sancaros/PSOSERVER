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
#ifndef _WIN32
#include <unistd.h>
#endif
#include <errno.h>

#include <pthread.h>

#include <Software_Defines.h>
#include <WinSock_Defines.h>
#include <windows.h>

#include <utsname.h>
#include <f_logs.h>
#include <psoconfig.h>
#include <debug.h>
#include <f_checksum.h>

#include <zlib.h>

#include "admin.h"
#include "ship.h"
#include "utils.h"
#include "clients.h"
#include "shipgate.h"
#include "ship_packets.h"
#include "scripts.h"
#include "quest_functions.h"
#include "max_tech_level.h"
#include "iitems.h"

/* TLS stuff -- from ship_server.c */
extern gnutls_anon_client_credentials_t anoncred;
//extern gnutls_certificate_credentials_t tls_cred;
extern gnutls_priority_t tls_prio;

extern int enable_ipv6;
extern char ship_host4[32];
extern char ship_host6[128];
extern uint32_t ship_ip4;
extern uint8_t ship_ip6[16];

/* Player levelup data */
bb_level_table_t bb_char_stats;
v2_level_table_t v2_char_stats;

static inline ssize_t sg_recv(shipgate_conn_t *c, void *buffer, size_t len) {
    int ret;
    LOOP_CHECK(ret, gnutls_record_recv(c->session, buffer, len));
    return ret;
    //return gnutls_record_recv(c->session, buffer, len);
}

static inline ssize_t sg_send(shipgate_conn_t *c, void *buffer, size_t len) {
    int ret;
    LOOP_CHECK(ret, gnutls_record_send(c->session, buffer, len));
    return ret;
    //return gnutls_record_send(c->session, buffer, len);
}

/* Send a raw packet away. */
static int send_raw(shipgate_conn_t* c, int len, uint8_t* sendbuf, int crypt) {
    ssize_t rv, total = 0;
    /* Keep trying until the whole thing's sent. */
    if((!crypt || c->has_key) && c->sock >= 0 && !c->sendbuf_cur) {
        while(total < len) {
            rv = sg_send(c, sendbuf + total, len - total);

            //TEST_LOG("舰船端口 %d 发送数据 %d 字节", c->sock, rv);

            /* Did the data send? */
            if(rv == GNUTLS_E_AGAIN || rv == GNUTLS_E_INTERRUPTED) {
                /* Try again. */
                continue;
            }
            else if(rv <= 0) {
                return SOCKET_ERROR;
            }

            total += rv;
        }
    }

    return 0;
}

/* Encrypt a packet, and send it away. */
static int send_crypt(shipgate_conn_t *c, int len, uint8_t *sendbuf) {
    /* Make sure its at least a header. */
    if(len < 8) {
        ERR_LOG(
            "send_crypt端口 %d 长度 = %d字节", c->sock, len);
        return -1;
    }

    return send_raw(c, len, sendbuf, 1);
}

/* Send a ping packet to the server. */
int shipgate_send_ping(shipgate_conn_t* c, int reply) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_ping_t* pkt = (shipgate_ping_t*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* 填充数据头. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_ping_t));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_PING);
    pkt->hdr.version = 0;
    pkt->hdr.reserved = 0;

    //DBG_LOG("%s", c->ship->cfg->ship_host4);

    memcpy(pkt->host4, c->ship->cfg->ship_host4, sizeof(pkt->host4));
    memcpy(pkt->host6, c->ship->cfg->ship_host6, sizeof(pkt->host6));

    if(reply) {
        pkt->hdr.flags = htons(SHDR_RESPONSE);
    }

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_ping_t), sendbuf);
}

/* Attempt to connect to the shipgate. Returns < 0 on error, returns the socket
   for communciation on success. */
static int shipgate_conn(ship_t* s, shipgate_conn_t* rv, int reconn) {
    int sock = SOCKET_ERROR, irv;
    //unsigned int peer_status;
    miniship_t* i, * tmp;
    struct addrinfo hints;
    struct addrinfo* server, * j;
    char sg_port[16];
    //char ipstr[INET6_ADDRSTRLEN];
    void* addr;

    if (reconn) {
        /* Clear all ships so we don't keep around stale stuff */
        i = TAILQ_FIRST(&s->ships);
        while (i) {
            tmp = TAILQ_NEXT(i, qentry);
            TAILQ_REMOVE(&s->ships, i, qentry);
            free_safe(i);
            i = tmp;
        }

        rv->has_key = 0;
        rv->hdr_read = 0;
        free_safe(rv->recvbuf);
        rv->recvbuf = NULL;
        rv->recvbuf_cur = rv->recvbuf_size = 0;

        free_safe(rv->sendbuf);
        rv->sendbuf = NULL;
        rv->sendbuf_cur = rv->sendbuf_size = 0;
    }
    else {
        /* Clear it first. */
        memset(rv, 0, sizeof(shipgate_conn_t));
    }

    SHIPS_LOG("%s: 搜寻船闸 (%s)...", s->cfg->name,
        s->cfg->shipgate_host);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(sg_port, sizeof(sg_port), "%hu", s->cfg->shipgate_port);

    if (getaddrinfo(s->cfg->shipgate_host, sg_port, &hints, &server)) {
        ERR_LOG("%s: 船闸地址无效: %s", s->cfg->name,
            s->cfg->shipgate_host);
        return -1;
    }

    SHIPS_LOG("%s: 连接船闸...", s->cfg->name);

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET) {
            addr = &((struct sockaddr_in*)j->ai_addr)->sin_addr;
        }
        else if (j->ai_family == PF_INET6) {
            addr = &((struct sockaddr_in6*)j->ai_addr)->sin6_addr;
        }
        else {
            continue;
        }

        //inet_ntop(j->ai_family, addr, ipstr, INET6_ADDRSTRLEN);
        //SHIPS_LOG("    地址 %s", ipstr);

        sock = socket(j->ai_family, SOCK_STREAM, IPPROTO_TCP);

        if (sock < 0) {
            ERR_LOG("端口错误: %s", strerror(errno));
            freeaddrinfo(server);
            return -1;
        }

        if (connect(sock, j->ai_addr, j->ai_addrlen)) {
            ERR_LOG("连接错误: %s", strerror(errno));
            closesocket(sock);
            sock = SOCKET_ERROR;
            continue;
        }

        SHIPS_LOG("        连接成功!");
        break;
    }

    freeaddrinfo(server);

    /* Did we connect? */
    if (sock == SOCKET_ERROR) {
        ERR_LOG("无法连接至船闸!");
        return -1;
    }

    /* Set up the TLS session */
    gnutls_init(&rv->session, GNUTLS_CLIENT);

    /* Use default priorities */
	gnutls_priority_set(rv->session, tls_prio);
    //gnutls_priority_set_direct(rv->session,
        //"PERFORMANCE:+ANON-ECDH:+ANON-DH",
        //NULL);

    irv = gnutls_credentials_set(rv->session, GNUTLS_CRD_ANON, anoncred);

    if (irv < 0) {
        ERR_LOG("TLS 匿名凭据错误: %s", gnutls_strerror(irv));
        goto err_tls;
    }

#if (SIZEOF_INT != SIZEOF_VOID_P) && (SIZEOF_LONG_INT == SIZEOF_VOID_P)
    gnutls_transport_set_ptr(rv->session, (gnutls_transport_ptr_t)((long)sock));
#else
    gnutls_transport_set_ptr(rv->session, (gnutls_transport_ptr_t)sock);
    //gnutls_transport_set_int(rv->session, sock);
#endif

    /* Do the TLS handshake */
    irv = gnutls_handshake(rv->session);

    if (irv < 0) {
        ERR_LOG("GNUTLS *** 注意: TLS 握手失败 %s", gnutls_strerror(irv));
        goto err_tls;
    }
    //else {
    //    SHIPS_LOG("GNUTLS *** TLS 握手成功");

    //    char* desc;
    //    desc = gnutls_session_get_desc(rv->session);
    //    SHIPS_LOG("GNUTLS *** Session 信息: %d - %s", sock, desc);
    //    gnutls_free(desc);
    //}

    ///* Verify that the peer has a valid certificate */
    //irv = gnutls_certificate_verify_peers2(rv->session, &peer_status);

    //if(irv < 0) {
    //    ERR_LOG("验证证书有效性失败: %s", gnutls_strerror(irv));
    //    gnutls_bye(rv->session, GNUTLS_SHUT_RDWR);
    //    closesocket(sock);
    //    gnutls_deinit(rv->session);
    //    return -4;
    //}

    ///* Check whether or not the peer is trusted... */
    //if(peer_status & GNUTLS_CERT_INVALID) {
    //    ERR_LOG("不受信任的对等连接, 原因如下 (%08x):"
    //        , peer_status);
    //    
    //    if (peer_status & GNUTLS_CERT_SIGNER_NOT_FOUND)
    //        ERR_LOG("未找到发卡机构");
    //    if (peer_status & GNUTLS_CERT_SIGNER_NOT_CA)
    //        ERR_LOG("发卡机构不是CA");
    //    if (peer_status & GNUTLS_CERT_NOT_ACTIVATED)
    //        ERR_LOG("证书尚未激活");
    //    if (peer_status & GNUTLS_CERT_EXPIRED)
    //        ERR_LOG("证书已过期");
    //    if (peer_status & GNUTLS_CERT_REVOKED)
    //        ERR_LOG("证书已吊销");
    //    if (peer_status & GNUTLS_CERT_INSECURE_ALGORITHM)
    //        ERR_LOG("不安全的证书签名");
    //    
    //    goto err_cert;
    //    gnutls_bye(rv->session, GNUTLS_SHUT_RDWR);
    //    closesocket(sock);
    //    gnutls_deinit(rv->session);
    //    return -5;
    //}

    /* Save a few other things in the struct */
    rv->sock = sock;
    rv->ship = s;

    return 0;
err_tls:
    closesocket(sock);
    gnutls_deinit(rv->session);
    return -3;
//err_cert:
//    gnutls_bye(rv->session, GNUTLS_SHUT_RDWR);
//    closesocket(sock);
//    gnutls_deinit(rv->session);
//    return -5;
}

int shipgate_connect(ship_t* s, shipgate_conn_t* rv) {
    return shipgate_conn(s, rv, 0);
}

/* Reconnect to the shipgate if we are disconnected for some reason. */
int shipgate_reconnect(shipgate_conn_t* conn) {
    return shipgate_conn(conn->ship, conn, 1);
}

/* Clean up a shipgate connection. */
void shipgate_cleanup(shipgate_conn_t* c) {
    if (c->sock > 0) {
        gnutls_bye(c->session, GNUTLS_SHUT_RDWR);
        closesocket(c->sock);
        gnutls_deinit(c->session);
    }

    free_safe(c->recvbuf);
    free_safe(c->sendbuf);
}

static int handle_dc_greply(shipgate_conn_t* conn, dc_guild_reply_pkt* pkt) {
    uint32_t i;
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* c;
    uint32_t dest = LE32(pkt->gc_searcher);
    int done = 0, rv = 0;

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                if (c->guildcard == dest) {
#ifdef PSOCN_ENABLE_IPV6
                    if (pkt->hdr.flags != 6) {
                        send_guild_reply_sg(c, pkt);
                    }
                    else {
                        send_guild_reply6_sg(c, (dc_guild_reply6_pkt*)pkt);
                    }
#else
                    send_guild_reply_sg(c, pkt);
#endif

                    done = 1;
                }

                pthread_mutex_unlock(&c->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return rv;
}

static int handle_bb_greply(shipgate_conn_t* conn, bb_guild_reply_pkt* pkt,
    uint32_t block) {
    block_t* b;
    ship_client_t* c;
    uint32_t dest = LE32(pkt->gc_searcher);

    /* Make sure the block given is sane */
    if((int)block > ship->cfg->blocks || !ship->blocks[block - 1]) {
        return 0;
    }

    b = ship->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Look for the client */
    TAILQ_FOREACH(c, b->clients, qentry) {
        pthread_mutex_lock(&c->mutex);

        if (c->guildcard == dest) {
            send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
            pthread_mutex_unlock(&c->mutex);
            pthread_rwlock_unlock(&b->lock);
            return 0;
        }

        pthread_mutex_unlock(&c->mutex);
    }

    pthread_rwlock_unlock(&b->lock);

    return 0;
}

static void handle_mail_autoreply(shipgate_conn_t* c, ship_client_t* s,
    uint32_t dest) {
    int i;

    switch (s->version) {
    case CLIENT_VERSION_PC:
    {
        simple_mail_pkt p = { 0 };
        dc_pkt_hdr_t* dc = (dc_pkt_hdr_t*)&p;

        /* Build the packet up */
        memset(&p, 0, sizeof(simple_mail_pkt));
        dc->pkt_type = SIMPLE_MAIL_TYPE;
        dc->pkt_len = LE16(PC_SIMPLE_MAIL_LENGTH);
        p.player_tag = LE32(0x00010000);
        p.gc_sender = LE32(s->guildcard);
        p.pcmaildata.gc_dest = LE32(dest);

        /* Copy the name */
        for (i = 0; i < 16; ++i) {
            p.pcmaildata.pcname[i] = LE16(s->pl->v1.character.dress_data.guildcard_string[i]);
        }

        /* Copy the message */
        memcpy(p.pcmaildata.stuff, s->autoreply, s->autoreply_len);

        /* Send it */
        shipgate_fw_pc(c, dc, 0, s);
        break;
    }

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
    {
        simple_mail_pkt p = { 0 };
        dc_pkt_hdr_t* dc = (dc_pkt_hdr_t*)&p;

        /* Build the packet up */
        memset(&p, 0, sizeof(simple_mail_pkt));
        p.dc.pkt_type = SIMPLE_MAIL_TYPE;
        p.dc.pkt_len = LE16(DC_SIMPLE_MAIL_LENGTH);
        p.player_tag = LE32(0x00010000);
        p.gc_sender = LE32(s->guildcard);
        p.dcmaildata.gc_dest = LE32(dest);

        /* Copy the name and message */
        memcpy(p.dcmaildata.dcname, s->pl->v1.character.dress_data.guildcard_string, 16);
        memcpy(p.dcmaildata.stuff, s->autoreply, s->autoreply_len);

        /* Send it */
        shipgate_fw_dc(c, dc, 0, s);
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
        p.bbmaildata.gc_dest = LE32(dest);

        /* Copy the name, date/time, and message */
        memcpy(p.bbmaildata.bbname, s->pl->bb.character.name, BB_CHARACTER_NAME_LENGTH * 2);
        memcpy(p.bbmaildata.message, s->autoreply, s->autoreply_len);

        /* Send it */
        shipgate_fw_bb(c, (bb_pkt_hdr_t*)&p, 0, s);
        break;
    }
    }
}

static int handle_dc_mail(shipgate_conn_t* conn, simple_mail_pkt* pkt) {
    uint32_t i;
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* c;
    uint32_t dest = LE32(pkt->dcmaildata.gc_dest);
    uint32_t sender = LE32(pkt->gc_sender);
    int done = 0, rv = 0;

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                if (c->guildcard == dest && c->pl) {
                    /* Make sure the user hasn't blacklisted the sender. */
                    if (client_has_blacklisted(c, sender) ||
                        client_has_ignored(c, sender)) {
                        done = 1;
                        pthread_mutex_unlock(&c->mutex);
                        rv = 0;
                        break;
                    }

                    /* Check if the user has an autoreply set. */
                    if (c->autoreply_on) {
                        handle_mail_autoreply(conn, c, sender);
                    }

                    /* Forward the packet there. */
                    rv = send_simple_mail(CLIENT_VERSION_DCV1, c,
                        (dc_pkt_hdr_t*)pkt);
                    done = 1;
                }

                pthread_mutex_unlock(&c->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return rv;
}

static int handle_pc_mail(shipgate_conn_t* conn, simple_mail_pkt* pkt) {
    uint32_t i;
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* c;
    uint32_t dest = LE32(pkt->pcmaildata.gc_dest);
    uint32_t sender = LE32(pkt->gc_sender);
    int done = 0, rv = 0;

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                if (c->guildcard == dest && c->pl) {
                    /* Make sure the user hasn't blacklisted the sender. */
                    if (client_has_blacklisted(c, sender) ||
                        client_has_ignored(c, sender)) {
                        done = 1;
                        pthread_mutex_unlock(&c->mutex);
                        rv = 0;
                        break;
                    }

                    /* Check if the user has an autoreply set. */
                    if (c->autoreply) {
                        handle_mail_autoreply(conn, c, sender);
                    }

                    /* Forward the packet there. */
                    rv = send_simple_mail(CLIENT_VERSION_PC, c,
                        (dc_pkt_hdr_t*)pkt);
                    done = 1;
                }

                pthread_mutex_unlock(&c->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return rv;
}

static int handle_bb_mail(shipgate_conn_t* conn, simple_mail_pkt* pkt) {
    uint32_t i;
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* c;
    uint32_t dest = LE32(pkt->bbmaildata.gc_dest);
    uint32_t sender = LE32(pkt->gc_sender);
    int done = 0, rv = 0;

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                if (c->guildcard == dest && c->pl) {
                    /* Make sure the user hasn't blacklisted the sender. */
                    if (client_has_blacklisted(c, sender) ||
                        client_has_ignored(c, sender)) {
                        done = 1;
                        pthread_mutex_unlock(&c->mutex);
                        rv = 0;
                        break;
                    }

                    /* Check if the user has an autoreply set. */
                    if (c->autoreply) {
                        handle_mail_autoreply(conn, c, sender);
                    }

                    /* Forward the packet there. */
                    rv = send_bb_simple_mail(c, pkt);
                    done = 1;
                }

                pthread_mutex_unlock(&c->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return rv;
}

static int handle_bb_guild(shipgate_conn_t* conn, shipgate_fw_9_pkt* pkt) {
    bb_guild_rv_data_pkt* g = (bb_guild_rv_data_pkt*)pkt->pkt;
    uint16_t type = LE16(g->hdr.pkt_type);
    uint16_t len = LE16(g->hdr.pkt_len);
    psocn_bb_db_guild_t* guild = (psocn_bb_db_guild_t*)g->data;
    uint32_t i;
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* c = { 0 };
    ship_client_t* c2 = { 0 };
    uint32_t gc = ntohl(pkt->guildcard);
    uint32_t guild_id = 0;

    DBG_LOG("G->S 指令0x%04X %d %d %d", type, gc, ntohl(pkt->guildcard), ntohl(pkt->ship_id));

    //这是从船闸返回来的公会数据包

    for (i = 0; i < s->cfg->blocks; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                //DBG_LOG("客户端版本 %d GC %u", c->version, c->guildcard);

                if (c->version >= CLIENT_VERSION_GC) {
                    switch (type)
                    {
                        /* OK */
                    case BB_GUILD_CREATE:
                        if (c->guildcard == gc) {

                            c->bb_guild->guild_data = guild->guild_data;
                            send_bb_guild_cmd(c, BB_GUILD_UNK_02EA);
                            send_bb_guild_cmd(c, BB_GUILD_FULL_DATA);
                            send_bb_guild_cmd(c, BB_GUILD_INITIALIZATION_DATA);
                            send_bb_guild_cmd(c, BB_GUILD_UNK_1DEA);
                        }
                        break;

                        /* OK */
                    case BB_GUILD_UNK_02EA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                        /* OK */
                    case BB_GUILD_MEMBER_ADD:
                        if (c->guildcard == gc) {

                            c->bb_guild->guild_data = guild->guild_data;
                            send_bb_guild_cmd(c, BB_GUILD_FULL_DATA);

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            //print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_UNK_04EA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                        /* OK */
                    case BB_GUILD_MEMBER_REMOVE:
                        bb_guild_member_remove_pkt* remove_pkt = (bb_guild_member_remove_pkt*)pkt->pkt;
                        guild_id = pkt->fw_flags;

                        if (c->guildcard == remove_pkt->target_guildcard &&
                            c->bb_guild->guild_data.guild_id == guild_id) {

                            memset(&c->bb_guild->guild_data, 0, sizeof(bb_guild_t));
                            send_bb_guild_cmd(c, BB_GUILD_FULL_DATA);
                            send_bb_guild_cmd(c, BB_GUILD_INITIALIZATION_DATA);
                        }
                        break;

                    case BB_GUILD_UNK_06EA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                        /* OK */
                    case BB_GUILD_CHAT:
                        bb_guild_member_chat_pkt* chat_pkt = (bb_guild_member_chat_pkt*)pkt->pkt;
                        guild_id = chat_pkt->guild_id;

                        if (c->bb_guild->guild_data.guild_id == guild_id && c->guildcard != 0)
                            send_pkt_bb(c, (bb_pkt_hdr_t*)g);

                        break;

                        /* OK */
                    case BB_GUILD_MEMBER_SETTING:
                        if (c->guildcard == gc)
                            send_pkt_bb(c, (bb_pkt_hdr_t*)g);
                        break;

                    case BB_GUILD_UNK_09EA:
                        if (c->guildcard == gc)
                            send_pkt_bb(c, (bb_pkt_hdr_t*)g);
                        break;

                    case BB_GUILD_UNK_0AEA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_UNK_0BEA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_UNK_0CEA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                        /* OK */
                    case BB_GUILD_INVITE:
                        if (c->guildcard == gc) {

                            send_bb_guild_cmd(c, BB_GUILD_UNK_0EEA);
                        }
                        break;

                        /* OK */
                    case BB_GUILD_UNK_0EEA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                        /* 暂时完成了 但是要移除原有的新手flag标记 */
                    case BB_GUILD_MEMBER_FLAG_SETTING:
                        bb_guild_member_flag_setting_pkt* flag_pkt = (bb_guild_member_flag_setting_pkt*)pkt->pkt;
                        guild_id = pkt->fw_flags;

                        if (c->bb_guild->guild_data.guild_id == guild_id) {
                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            memcpy(&c->bb_guild->guild_data.guild_flag[0], &flag_pkt->guild_flag[0], sizeof(c->bb_guild->guild_data.guild_flag));
                            send_bb_guild_cmd(c, BB_GUILD_FULL_DATA);
                        }
                        break;

                    case BB_GUILD_DISSOLVE:
                        guild_id = pkt->fw_flags;

                        if (c->bb_guild->guild_data.guild_id == guild_id) {
                            send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC4公会已被解散!"));
                            memset(&c->bb_guild->guild_data, 0, sizeof(psocn_bb_db_guild_t));
                            send_bb_guild_cmd(c, BB_GUILD_FULL_DATA);
                            send_bb_guild_cmd(c, BB_GUILD_INITIALIZATION_DATA);
                        }
                        break;

                        /* OK */
                    case BB_GUILD_MEMBER_PROMOTE:
                        bb_guild_member_promote_pkt* promote_pkt = (bb_guild_member_promote_pkt*)pkt->pkt;
                        guild_id = pkt->fw_flags;

                        if (c->bb_guild->guild_data.guild_id == guild_id) {

                            if (c->guildcard == promote_pkt->target_guildcard) {

                                DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                                print_payload((uint8_t*)g, len);
                                send_msg(c, MSG_BOX_TYPE, "%s",
                                    __(c, "\tE您的公会权限已被提升."));
                            }
                            else {
                                DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                                /*return send_msg(c, MSG_BOX_TYPE, "%s",
                                    __(c, "\tE公会权限已被提升."));*/
                            }
                        }
                        break;

                    case BB_GUILD_INITIALIZATION_DATA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_LOBBY_SETTING:
                        if (c->guildcard == gc) {

                            send_bb_guild_cmd(c, BB_GUILD_LOBBY_SETTING);
                        }
                        break;

                    case BB_GUILD_MEMBER_TITLE:
                        if (c->guildcard == gc) {

                            send_bb_guild_cmd(c, BB_GUILD_MEMBER_TITLE);
                        }
                        break;

                    case BB_GUILD_FULL_DATA:
                        if (c->guildcard == gc) {
                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_UNK_16EA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_UNK_17EA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO:
                        if (c->guildcard == gc) {

                            send_bb_guild_cmd(c, BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO);
                        }
                        break;

                    case BB_GUILD_PRIVILEGE_LIST:
                        if (c->guildcard == gc) {

                            send_bb_guild_cmd(c, BB_GUILD_PRIVILEGE_LIST);
                        }
                        break;

                    case BB_GUILD_BUY_SPECIAL_ITEM:
                        if (c->guildcard == gc) {

                            send_bb_guild_cmd(c, BB_GUILD_BUY_SPECIAL_ITEM);
                        }
                        break;

                    case BB_GUILD_UNK_1BEA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_RANKING_LIST:
                        if (c->guildcard == gc) {

                            send_bb_guild_cmd(c, BB_GUILD_RANKING_LIST);
                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                    case BB_GUILD_UNK_1DEA:
                        if (c->guildcard == gc) {

                            DBG_LOG("handle_bb_guild 0x%04X %d %d", type, len, c->guildcard);
                            print_payload((uint8_t*)g, len);
                        }
                        break;

                        /* 卡住需要载入某种数据 */
                    case BB_GUILD_UNK_1EEA:
                        if (c->guildcard == gc) {
                            send_bb_guild_cmd(c, BB_GUILD_UNK_1EEA);
                        }
                        break;

                        /* 菜单直接关闭了 */
                    case BB_GUILD_UNK_1FEA:
                        if (c->guildcard == gc) {
                            send_bb_guild_cmd(c, BB_GUILD_UNK_1FEA);
                        }
                        break;

                    case BB_GUILD_UNK_20EA:
                        if (c->guildcard == gc) {
                            send_bb_guild_cmd(c, BB_GUILD_UNK_20EA);
                        }
                        break;

                    default:
                        DBG_LOG("未知 handle_bb_guild 0x%04X %d %d", type, len, gc);
                        break;
                    }
                }

                pthread_mutex_unlock(&c->mutex);

            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

static int handle_dc(shipgate_conn_t* conn, shipgate_fw_9_pkt* pkt) {
    dc_pkt_hdr_t* dc = (dc_pkt_hdr_t*)pkt->pkt;
    uint8_t type = dc->pkt_type;

    switch (type) {
    case GUILD_REPLY_TYPE:
        return handle_dc_greply(conn, (dc_guild_reply_pkt*)dc);

    case GUILD_SEARCH_TYPE:
        /* We should never get these... Ignore them, but log a warning. */
        SHIPS_LOG("船闸发送了公会搜索?!");
        return 0;

    case SIMPLE_MAIL_TYPE:
        return handle_dc_mail(conn, (simple_mail_pkt*)dc);
    }

    return -2;
}

static int handle_pc(shipgate_conn_t* conn, shipgate_fw_9_pkt* pkt) {
    dc_pkt_hdr_t* dc = (dc_pkt_hdr_t*)pkt->pkt;
    uint8_t type = dc->pkt_type;

    switch (type) {
    case SIMPLE_MAIL_TYPE:
        return handle_pc_mail(conn, (simple_mail_pkt*)dc);
    }

    return -2;
}

static int handle_bb(shipgate_conn_t *conn, shipgate_fw_9_pkt *pkt) {
    bb_pkt_hdr_t *bb = (bb_pkt_hdr_t *)pkt->pkt;
    uint16_t type = LE16(bb->pkt_type);
    uint16_t len = LE16(bb->pkt_len);
    uint32_t block = ntohl(pkt->block);

    /* 整合为综合指令集 */
    switch (type & 0x00FF) {
        /* 0x00DF 挑战模式 */
    //case BB_CHALLENGE_DF:
    //    return handle_bb_challenge(conn, pkt);

        /* 0x00EA 公会功能 */
    case BB_GUILD_COMMAND:
        return handle_bb_guild(conn, pkt);

    default:
        break;
    }

    switch (type) {
    case SIMPLE_MAIL_TYPE:
        return handle_bb_mail(conn, (simple_mail_pkt*)bb);

    case GUILD_REPLY_TYPE:
        return handle_bb_greply(conn, (bb_guild_reply_pkt*)bb, block);

    default:
        /* Warn the ship that sent the packet, then drop it
         警告发送包裹的船，然后丢弃它
        */
        print_payload((uint8_t*)bb, len);
        return 0;
    }

    return -2;
}

static void menu_code_sort(uint16_t* codes, int count) {
    int i, j;
    uint16_t tmp;

    /* This list shouldn't ever get too big, and will be mostly sorted to start
       with, so insertion sort should do well here. */
    for (j = 1; j < count; ++j) {
        tmp = codes[j];
        i = j - 1;

        while (i >= 0 && codes[i] > tmp) {
            codes[i + 1] = codes[i];
            --i;
        }

        codes[i + 1] = tmp;
    }
}

static int menu_code_exists(uint16_t* codes, int count, uint16_t target) {
    int i = 0, j = count, k;

    /* Simple binary search */
    while (i < j) {
        k = i + (j - i) / 2;

        if (codes[k] < target) {
            i = k + 1;
        }
        else {
            j = k;
        }
    }

    /* If we've found the value we're looking for, return success */
    if (i < count && codes[i] == target) {
        return 1;
    }

    return 0;
}

static int handle_sstatus(shipgate_conn_t* conn, shipgate_ship_status6_pkt* p) {
    uint16_t status = ntohs(p->status);
    uint32_t sid = ntohl(p->ship_id);
    ship_t* s = conn->ship;
    miniship_t *i, *j, *k;
    uint16_t code = 0;
    int ship_found = 0;
    void* tmp;
    
    /* Did a ship go down or come up? */
    if (!status) {
        /* A ship has gone down */
        TAILQ_FOREACH(i, &s->ships, qentry) {
            /* Clear the ship from the list, if we've found the right one */
            if (sid == i->ship_id) {
                //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
                TAILQ_REMOVE(&s->ships, i, qentry);
                ship_found = 1;
                break;
            }
        }

        if (!ship_found) {
            return 0;
        }

        /* Figure out if the menu code is still in use */
        TAILQ_FOREACH(j, &s->ships, qentry) {
            //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
            if (i->menu_code == j->menu_code) {
                code = 1;
                break;
            }
        }

        /* If the code is not in use, get rid of it */
        if (!code) {
            //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
            /* Move all higher menu codes down the list */
            for (code = 0; code < s->mccount - 1; ++code) {
                if (s->menu_codes[code] >= i->menu_code) {
                    s->menu_codes[code] = s->menu_codes[code + 1];
                }
            }

            /* Chop off the last element (its now a duplicated entry or the one
               we want to get rid of) */
            tmp = realloc(s->menu_codes, (s->mccount - 1) * sizeof(uint16_t));

            if (!tmp) {
                perror("realloc");
                s->menu_codes[s->mccount - 1] = 0xFFFF;
            }
            else {
                s->menu_codes = (uint16_t*)tmp;
                --s->mccount;
            }
        }

        /* Clean up the miniship */
        free_safe(i);
    }
    else {
        /* Look for the ship first of all, just in case. */
        TAILQ_FOREACH(i, &s->ships, qentry) {
            //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
            if (sid == i->ship_id) {
                //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
                ship_found = 1;

                /* Remove it from the list, it'll get re-added later (in the
                   correct position). This also saves us some trouble, in case
                   anything goes wrong... */
                TAILQ_REMOVE(&s->ships, i, qentry);

                break;
            }
        }

        /* If we didn't find the ship (in most cases we won't), allocate space
           for it. */
        if (!ship_found) {
            //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
            /* Allocate space, and punt if we can't */
            i = (miniship_t*)malloc(sizeof(miniship_t));

            if (!i) {
                return 0;
            }
        }

        /* See if we need to deal with the menu code or not here */
        code = ntohs(p->menu_code);

        if (!menu_code_exists(s->menu_codes, s->mccount, code)) {
            //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
            tmp = realloc(s->menu_codes, (s->mccount + 1) * sizeof(uint16_t));

            /* Can't make space, punt */
            if (!tmp) {
                perror("realloc");
                free_safe(i);
                return 0;
            }

            /* Put the new code in, and sort the list */
            s->menu_codes = (uint16_t*)tmp;
            s->menu_codes[s->mccount++] = code;
            menu_code_sort(s->menu_codes, s->mccount);
        }

        /* Copy the ship data */
        memset(i, 0, sizeof(miniship_t));
        memcpy(i->name, p->name, 12);
        memcpy(i->ship_addr6, p->ship_addr6, 16);
        i->ship_id = sid;
        memcpy(i->ship_host4, p->ship_host4, 32);
        memcpy(i->ship_host6, p->ship_host6, 128);

        DBG_LOG("此处需要继续往下写 %s", i->ship_host4);
        i->ship_addr = p->ship_addr4;
        i->ship_port = ntohs(p->ship_port);
        i->clients = ntohs(p->clients);
        i->games = ntohs(p->games);
        i->menu_code = code;
        i->flags = ntohl(p->flags);
        i->ship_number = p->ship_number;
        i->privileges = ntohl(p->privileges);

        /* Add the new ship to the list */
        j = TAILQ_FIRST(&s->ships);
        if (j && j->ship_number < i->ship_number) {
            //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
            /* Figure out where this entry is going. */
            while (j && i->ship_number > j->ship_number) {
                k = j;
                j = TAILQ_NEXT(j, qentry);
            }

            /* We've got the spot to put it at, so add it in. */
            TAILQ_INSERT_AFTER(&s->ships, k, i, qentry);
        }
        else {
            //SHIPS_LOG("handle_sstatus status = %d sid = %d", status, sid);
            /* Nothing here (or the first entry goes after this one), add us to
               the front of the list. */
            TAILQ_INSERT_HEAD(&s->ships, i, qentry);
        }
    }

    return 0;
}

static int handle_char_data_req(shipgate_conn_t *conn, shipgate_char_data_pkt *pkt) {
    uint32_t i;
    ship_t *s = conn->ship;
    block_t *b;
    ship_client_t *c;
    uint32_t dest = ntohl(pkt->guildcard);
    int done = 0;
    uint16_t flags = ntohs(pkt->hdr.flags);
    uint16_t plen = ntohs(pkt->hdr.pkt_len);
    int clen = plen - sizeof(shipgate_char_data_pkt);

    /* Make sure the packet looks sane */
    if(!(flags & SHDR_RESPONSE)) {
        return 0;
    }

    for(i = 0; i < s->cfg->blocks && !done; ++i) {
        if(s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                if(c->guildcard == dest) {
                    if(!c->bb_pl && c->pl) {
                        /* We've found them, overwrite their data, and send the
                           refresh packet. */
                        memcpy(c->pl, pkt->data, clen);

                        ITEM_LOG("////////////////////////////////////////////////////////////");
                        for (i = 0; i < c->pl->bb.inv.item_count; ++i) {
                            print_iitem_data(&c->pl->bb.inv.iitems[i], i, c->version);
                        }
                        
                        send_lobby_join(c, c->cur_lobby);

                    }
                    else if(c->bb_pl) {
                        memcpy(c->bb_pl, pkt->data, clen);

                        /* 清理背包的物品ID以备重写. */
                        for(i = 0; i < MAX_PLAYER_INV_ITEMS; ++i) {
                            c->bb_pl->inv.iitems[i].data.item_id = EMPTY_STRING;
                        }

                        ITEM_LOG("////////////////////////////////////////////////////////////");
                        for (i = 0; i < MAX_PLAYER_INV_ITEMS; ++i) {
                            if (c->bb_pl->inv.iitems[i].present) {
                                print_biitem_data(&c->bb_pl->inv.iitems[i], i, c->version, 1, 0);
                                fix_inv_bank_item(&c->bb_pl->inv.iitems[i].data);
                            }
                            //else
                            //    if ((c->bb_pl->inv.iitems[i].data.data_l[0] != 0) ||
                            //        (c->bb_pl->inv.iitems[i].data.data_l[1] != 0) ||
                            //        (c->bb_pl->inv.iitems[i].data.data_l[2] != 0) ||
                            //        (c->bb_pl->inv.iitems[i].data.data_l[3] != 0) ||
                            //        (c->bb_pl->inv.iitems[i].data.data2_l != 0)
                            //        )
                            //        print_biitem_data(&c->bb_pl->inv.iitems[i], i, c->version, 1, 1);
                        }

                        fix_equip_item(&c->bb_pl->inv);

                        ITEM_LOG("////////////////////////////////////////////////////////////");
                        for (i = 0; i < MAX_PLAYER_INV_ITEMS; ++i) {
                            if (c->bb_pl->bank.bitems[i].show_flags) {
                                print_biitem_data(&c->bb_pl->bank.bitems[i], i, c->version, 0, 0);
                                fix_inv_bank_item(&c->bb_pl->bank.bitems[i].data);
                            }
                            //else
                            //    if ((c->bb_pl->inv.iitems[i].data.data_l[0] != 0) ||
                            //        (c->bb_pl->inv.iitems[i].data.data_l[1] != 0) ||
                            //        (c->bb_pl->inv.iitems[i].data.data_l[2] != 0) ||
                            //        (c->bb_pl->inv.iitems[i].data.data_l[3] != 0) ||
                            //        (c->bb_pl->inv.iitems[i].data.data2_l != 0)
                            //        )
                            //        print_biitem_data(&c->bb_pl->inv.iitems[i], i, c->version, 0, 1);
                        }
                    }

                    done = 1;
                }

                pthread_mutex_unlock(&c->mutex);

                if(done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

static int handle_usrlogin(shipgate_conn_t* conn,
    shipgate_usrlogin_reply_pkt* pkt) {
    uint16_t flags = ntohs(pkt->hdr.flags);
    uint32_t gc = ntohl(pkt->guildcard);
    uint32_t block = ntohl(pkt->block);
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* i;

    /* Make sure the packet looks sane */
    if (!(flags & SHDR_RESPONSE)) {
        return 0;
    }

    /* Check the block number first. */
    if((int)block > s->cfg->blocks) {
        return 0;
    }

    b = s->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            i->privilege |= ntohl(pkt->priv);
            i->flags |= CLIENT_FLAG_LOGGED_IN;
            i->flags &= ~CLIENT_FLAG_GC_PROTECT;
            send_txt(i, "%s %u", __(i, "\tE\tC7欢迎回来."), gc);
            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return 0;
}

static int handle_login(shipgate_conn_t* conn, shipgate_login_pkt* pkt) {
    /* Check the header of the packet. */
    if (ntohs(pkt->hdr.pkt_len) < SHIPGATE_LOGINV0_SIZE) {
        return -2;
    }

    /* Check the copyright message of the packet. */
    if (strcmp(pkt->msg, shipgate_login_msg)) {
        return -3;
    }

    SHIPS_LOG("%s: 成功对接船闸 版本 %d.%d.%d",
        conn->ship->cfg->name, (int)pkt->ver_major, (int)pkt->ver_minor,
        (int)pkt->ver_micro);

    /* Send our info to the shipgate so it can have things set up right. */
    return shipgate_send_ship_info(conn, conn->ship);
}

static int handle_count(shipgate_conn_t* conn, shipgate_cnt_pkt* pkt) {
    uint32_t id = ntohl(pkt->ship_id);
    miniship_t* i;
    ship_t* s = conn->ship;

    TAILQ_FOREACH(i, &s->ships, qentry) {
        /* Find the requested ship and update its counts */
        if (i->ship_id == id) {
            i->clients = ntohs(pkt->clients);
            i->games = ntohs(pkt->games);
            /*DBG_LOG(
                , "handle_count id %d i->clients = %d i->games = %d conn->has_key = %d"
                , id, i->clients, i->games, conn->has_key);*/
            return 0;
        }
    }
    /* We didn't find it... this shouldn't ever happen */
    return -1;
}

static int handle_char_data_db_save(shipgate_conn_t* conn, shipgate_cdata_err_pkt* pkt) {
    uint32_t i;
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* c;
    uint32_t dest = ntohl(pkt->guildcard);
    int done = 0;
    uint16_t flags = ntohs(pkt->base.hdr.flags);

    /* Make sure the packet looks sane */
    if (!(flags & SHDR_RESPONSE)) {
        return 0;
    }

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                if (c->guildcard == dest && c->pl) {
                    /* We've found them, figure out what to tell them. */
                    if (flags & SHDR_FAILURE) {
                        send_txt(c, "%s", __(c, "\tE\tC7无法保存角色数据."));
                    }
                    else {
                        send_txt(c, "%s", __(c, "\tE\tC7角色数据已保存."));
                    }

                    done = 1;
                }
                else if (c->guildcard == dest) {
                    /* Act like they don't exist for right now (they don't
                       really exist right now)
                        假装他们现在不存在（他们现在真的不存在） */
                    done = 1;
                }

                pthread_mutex_unlock(&c->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

static int handle_ban(shipgate_conn_t* conn, shipgate_ban_err_pkt* pkt) {
    uint32_t i;
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* c;
    uint32_t dest = ntohl(pkt->req_gc);
    int done = 0;
    uint16_t flags = ntohs(pkt->base.hdr.flags);

    /* Make sure the packet looks sane */
    if (!(flags & SHDR_RESPONSE) && !(flags & SHDR_FAILURE)) {
        return 0;
    }

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                if (c->guildcard == dest && c->pl) {
                    /* We've found them, figure out what to tell them. */
                    if (flags & SHDR_FAILURE) {
                        /* If the not gm flag is set, disconnect the user. */
                        if (ntohl(pkt->base.error_code) == ERR_BAN_NOT_GM) {
                            c->flags |= CLIENT_FLAG_DISCONNECTED;
                        }

                        send_txt(c, "%s", __(c, "\tE\tC7封禁用户时发生错误."));

                    }
                    else {
                        send_txt(c, "%s", __(c, "\tE\tC7用户已被封禁."));
                    }

                    done = 1;
                }
                else if (c->guildcard == dest) {
                    /* Act like they don't exist for right now (they don't
                       really exist right now) */
                    done = 1;
                }

                pthread_mutex_unlock(&c->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

static int handle_char_data_req_err(shipgate_conn_t* conn, shipgate_cdata_err_pkt* pkt) {
    uint32_t i;
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* c;
    uint32_t dest = ntohl(pkt->guildcard);
    int done = 0;
    uint16_t flags = ntohs(pkt->base.hdr.flags);
    uint32_t err = ntohl(pkt->base.error_code);

    /* Make sure the packet looks sane */
    if (!(flags & SHDR_FAILURE)) {
        return 0;
    }

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(c, b->clients, qentry) {
                pthread_mutex_lock(&c->mutex);

                if (c->guildcard == dest && c->pl) {
                    /* We've found them, figure out what to tell them. */
                    if (err == ERR_CREQ_NO_DATA) {
                        ERR_LOG("未找到角色数据");
                        send_txt(c, "%s", __(c, "\tE\tC7未找到角色数据."));
                    }
                    else {
                        ERR_LOG("无法获取角色数据");
                        send_txt(c, "%s", __(c, "\tE\tC7无法获取角色数据."));
                    }

                    done = 1;
                }
                else if (c->guildcard == dest) {
                    /* Act like they don't exist for right now (they don't
                       really exist right now) */
                    done = 1;
                }

                pthread_mutex_unlock(&c->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

static int handle_usrlogin_err(shipgate_conn_t* conn,
    shipgate_gm_err_pkt* pkt) {
    uint16_t flags = ntohs(pkt->base.hdr.flags);
    uint32_t gc = ntohl(pkt->guildcard);
    uint32_t block = ntohl(pkt->block);
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* i;
    int rv = 0;

    /* Make sure the packet looks sane */
    if (!(flags & SHDR_FAILURE)) {
        return 0;
    }

    /* Check the block number first. */
    if((int)block > s->cfg->blocks) {
        return 0;
    }

    b = s->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            /* XXXX: Maybe send specific error messages sometime later? */
            send_txt(i, "%s", __(i, "\tE\tC7登录失败~请检查您输入的信息."));
            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return rv;
}

static int handle_blogin_err(shipgate_conn_t* c, shipgate_blogin_err_pkt* pkt) {
    uint32_t gc = ntohl(pkt->guildcard);
    uint32_t block = ntohl(pkt->blocknum);
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* i;

    /* Grab the block first */
    if((int)block > s->cfg->blocks || !(b = s->blocks[block - 1])) {
        return 0;
    }

    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client and boot them off (regardless of the error type
       for now) */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            i->flags |= CLIENT_FLAG_DISCONNECTED;
        }
    }

    pthread_rwlock_unlock(&b->lock);

    return 0;
}

static int handle_login_reply(shipgate_conn_t* conn, shipgate_error_pkt* pkt) {
    uint32_t err = ntohl(pkt->error_code);
    uint16_t flags = ntohs(pkt->hdr.flags);
    ship_t* s = conn->ship;

    /* Make sure the packet looks sane */
    if (!(flags & SHDR_RESPONSE)) {
        return -1;
    }

    /* Is this an error or success? */
    if (flags & SHDR_FAILURE) {
        switch (err) {
        case ERR_LOGIN_BAD_PROTO:
            SHIPS_LOG("%s: 不支持的船闸协议版本!",
                s->cfg->name);
            break;

        case ERR_BAD_ERROR:
            SHIPS_LOG("%s: 船闸连接出错, 稍后再尝试对接.",
                s->cfg->name);
            break;

        case ERR_LOGIN_BAD_KEY:
            SHIPS_LOG("%s: 无效舰船密钥!", s->cfg->name);
            break;

        case ERR_LOGIN_BAD_MENU:
            SHIPS_LOG("%s: 无效菜单代码!", s->cfg->name);
            break;

        case ERR_LOGIN_INVAL_MENU:
            SHIPS_LOG("%s: 请在配置中选择有效的菜单代码!",
                s->cfg->name);
            break;
        }

        /* Whatever the problem, we're hosed here. */
        return -9001;
    }
    else {
        /* We have a response. Set the has key flag. */
        conn->has_key = 1;
        SHIPS_LOG("%s: 舰船与船闸完成对接", s->cfg->name);
    }

    /* Send the burst of client data if we have any to send */
    return shipgate_send_clients(conn);
}

static int handle_friend(shipgate_conn_t* c, shipgate_friend_login_4_pkt* pkt) {
    uint16_t type = ntohs(pkt->hdr.pkt_type);
    uint32_t ugc, ubl, fsh, fbl;
    miniship_t* ms;
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* cl;
    int on = type == SHDR_TYPE_FRLOGIN;

    ugc = ntohl(pkt->dest_guildcard);
    ubl = ntohl(pkt->dest_block);
    fsh = ntohl(pkt->friend_ship);
    fbl = ntohl(pkt->friend_block);

    /* Grab the block structure where the user is */
    if((int)ubl > s->cfg->blocks || !(b = s->blocks[ubl - 1])) {
        return 0;
    }

    /* Find the ship in question */
    TAILQ_FOREACH(ms, &s->ships, qentry) {
        if (ms->ship_id == fsh) {
            break;
        }
    }

    /* If we can't find the ship, give up */
    if (!ms) {
        return 0;
    }

    /* Find the user in question */
    pthread_rwlock_rdlock(&b->lock);

    TAILQ_FOREACH(cl, b->clients, qentry) {
        if (cl->guildcard == ugc) {
            /* The rest is easy */
            client_send_friendmsg(cl, on, pkt->friend_name, ms->name, fbl,
                pkt->friend_nick);
            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);

    return 0;
}

static int handle_addfriend(shipgate_conn_t* c, shipgate_friend_err_pkt* pkt) {
    uint32_t i;
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* cl;
    uint32_t dest = ntohl(pkt->user_gc);
    int done = 0;
    uint16_t flags = ntohs(pkt->base.hdr.flags);
    uint32_t err = ntohl(pkt->base.error_code);

    /* Make sure the packet looks sane */
    if (!(flags & SHDR_FAILURE) && !(flags & SHDR_RESPONSE)) {
        return 0;
    }

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(cl, b->clients, qentry) {
                pthread_mutex_lock(&cl->mutex);

                if (cl->guildcard == dest && cl->pl) {
                    /* We've found them, figure out what to tell them. */
                    if (err == ERR_NO_ERROR) {
                        send_txt(cl, "%s", __(cl, "\tE\tC7新增好友完成."));
                    }
                    else {
                        send_txt(cl, "%s", __(cl, "\tE\tC7无法新增好友."));
                    }

                    done = 1;
                }
                else if (cl->guildcard == dest) {
                    /* Act like they don't exist for right now (they don't
                       really exist right now) */
                    done = 1;
                }

                pthread_mutex_unlock(&cl->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

static int handle_delfriend(shipgate_conn_t* c, shipgate_friend_err_pkt* pkt) {
    uint32_t i;
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* cl;
    uint32_t dest = ntohl(pkt->user_gc);
    int done = 0;
    uint16_t flags = ntohs(pkt->base.hdr.flags);
    uint32_t err = ntohl(pkt->base.error_code);

    /* Make sure the packet looks sane */
    if (!(flags & SHDR_FAILURE) && !(flags & SHDR_RESPONSE)) {
        return 0;
    }

    for (i = 0; i < s->cfg->blocks && !done; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(cl, b->clients, qentry) {
                pthread_mutex_lock(&cl->mutex);

                if (cl->guildcard == dest && cl->pl) {
                    /* We've found them, figure out what to tell them. */
                    if (err == ERR_NO_ERROR) {
                        send_txt(cl, "%s", __(cl, "\tE\tC7好友已删除."));
                    }
                    else {
                        send_txt(cl, "%s", __(cl, "\tE\tC7无法删除好友."));
                    }

                    done = 1;
                }
                else if (cl->guildcard == dest) {
                    /* Act like they don't exist for right now (they don't
                       really exist right now) */
                    done = 1;
                }

                pthread_mutex_unlock(&cl->mutex);

                if (done) {
                    break;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

static int handle_kick(shipgate_conn_t* conn, shipgate_kick_pkt* pkt) {
    uint32_t gc = ntohl(pkt->guildcard);
    uint32_t block = ntohl(pkt->block);
    ship_t* s = conn->ship;
    block_t* b;
    ship_client_t* i;

    /* Check the block number first. */
    if((int)block > s->cfg->blocks) {
        return 0;
    }

    b = s->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            /* Found them, send the message and disconnect the client */
            if (strlen(pkt->reason) > 0) {
                send_msg(i, MSG_BOX_TYPE, "%s\n\n%s\n%s",
                                 __(i, "\tE您被 GM 踢出游戏."),
                                 __(i, "理由:"), pkt->reason);
            }
            else {
                send_msg(i, MSG_BOX_TYPE, "%s",
                                 __(i, "\tE您被 GM 踢出游戏."));
            }

            i->flags |= CLIENT_FLAG_DISCONNECTED;
            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return 0;
}

static int handle_frlist(shipgate_conn_t* c, shipgate_friend_list_pkt* pkt) {
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* i;
    uint32_t gc = ntohl(pkt->requester), gc2;
    uint32_t block = ntohl(pkt->block), bl2, ship;
    int j, total;
    char msg[1024] = { 0 };
    miniship_t* ms;
    size_t len = 0;

    /* Check the block number first. */
    if((int)block > s->cfg->blocks)
        return 0;

    b = s->blocks[block - 1];
    total = ntohs(pkt->hdr.pkt_len) - sizeof(shipgate_friend_list_pkt);
    msg[0] = msg[1023] = 0;
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            pthread_mutex_lock(&i->mutex);

            if (!total) {
                strcpy(msg, __(i, "\tE在该偏移处未找到好友."));
            }

            for (j = 0; total; ++j, total -= 48) {
                ship = ntohl(pkt->entries[j].ship);
                bl2 = ntohl(pkt->entries[j].block);
                gc2 = ntohl(pkt->entries[j].guildcard);

                if (ship && block) {
                    /* Grab the ship the user is on */
                    ms = ship_find_ship(s, ship);

                    if (!ms) {
                        continue;
                    }

                    /* Fill in the message */
                    if (ms->menu_code) {
                        len += snprintf(msg + len, 1023 - len,
                                        "\tC2%s (%d)\n\tC7%02x:%c%c/%s "
                                        "舰仓%02d\n", pkt->entries[j].name,
                                        gc2, ms->ship_number,
                                        (char)(ms->menu_code),
                                        (char)(ms->menu_code >> 8), ms->name,
                                        bl2);
                    }
                    else {
                        len += snprintf(msg + len, 1023 - len,
                                       "\tC2%s (%d)\n\tC7%02x:%s 舰仓%02d\n",
                                       pkt->entries[j].name, gc2,
                                       ms->ship_number, ms->name, bl2);
                    }
                }
                else {
                    /* Not online? Much easier to deal with! */
                    len += snprintf(msg + len, 1023 - len, "\tC4%s (%d)\n",
                        pkt->entries[j].name, gc2);
                }
            }

            /* Send the message to the user */
            if (send_msg(i, MSG_BOX_TYPE, "%s", msg)) {
                i->flags |= CLIENT_FLAG_DISCONNECTED;
            }

            pthread_mutex_unlock(&i->mutex);

            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return 0;
}

static int handle_globalmsg(shipgate_conn_t* c, shipgate_global_msg_pkt* pkt) {
    uint16_t text_len;
    ship_t* s = c->ship;
    uint32_t i;
    block_t* b;
    ship_client_t* i2;

    /* Make sure the message looks sane */
    text_len = ntohs(pkt->hdr.pkt_len) - sizeof(shipgate_global_msg_pkt);

    /* Make sure the string is NUL terminated */
    if (pkt->text[text_len - 1]) {
        SHIPS_LOG("Ignoring Non-terminated global msg");
        return 0;
    }

    /* Go through each block and send the message to anyone that is alive. */
    for (i = 0; i < s->cfg->blocks; ++i) {
        b = s->blocks[i];

        if (b && b->run) {
            pthread_rwlock_rdlock(&b->lock);

            /* Send the message to each player. */
            TAILQ_FOREACH(i2, b->clients, qentry) {
                pthread_mutex_lock(&i2->mutex);

                if (i2->pl) {
                    send_txt(i2, "%s\n%s", __(i2, "\tE\tC7全球消息:"),
                        pkt->text);
                }

                pthread_mutex_unlock(&i2->mutex);
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return 0;
}

static int handle_useropt(shipgate_conn_t* c, shipgate_user_opt_pkt* pkt) {
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* i;
    uint32_t gc = ntohl(pkt->guildcard), block = ntohl(pkt->block);
    uint8_t* optptr = (uint8_t*)pkt->options;
    uint8_t* endptr = ((uint8_t*)pkt) + ntohs(pkt->hdr.pkt_len);
    shipgate_user_opt_t* opt = (shipgate_user_opt_t*)optptr;
    uint32_t option, length;

    /* Check the block number first. */
    if((int)block > s->cfg->blocks) {
        return 0;
    }

    b = s->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            pthread_mutex_lock(&i->mutex);

            /* Deal with the options */
            while (optptr < endptr && opt->option != 0) {
                option = ntohl(opt->option);
                length = ntohl(opt->length);

                switch (option) {
                case USER_OPT_QUEST_LANG:
                    /* Make sure the length is right */
                    if (length != 16)
                        break;

                    /* The only byte of the data that's used is the first
                       one. It has the language code in it. */
                    i->q_lang = opt->data[0];
                    break;

                case USER_OPT_ENABLE_BACKUP:
                    /* Make sure the length is right */
                    if (length != 16)
                        break;

                    /* The only byte of the data that's used is the first
                       one. It is a boolean saying whether or not to enable
                       the auto backup feature. */
                    if (opt->data[0])
                        i->flags |= CLIENT_FLAG_AUTO_BACKUP;
                    break;

                case USER_OPT_GC_PROTECT:
                    /* Make sure the length is right */
                    if (length != 16)
                        break;

                    /* The only byte of the data that's used is the first
                       one. It is a boolean saying whether or not to enable
                       the guildcard protection feature. */
                    if (opt->data[0]) {
                        i->flags |= CLIENT_FLAG_GC_PROTECT;
                        send_txt(i, __(i, "\tE\tC7Guildcard is "
                            "protected.\nYou will be kicked\n"
                            "if you do not login."));
                        i->join_time = time(NULL);
                    }
                    break;

                case USER_OPT_TRACK_KILLS:
                    /* Make sure the length is right */
                    if (length != 16)
                        break;

                    /* The only byte of the data that's used is the first
                       one. It is a boolean saying whether or not to enable
                       kill tracking. */
                    if (opt->data[0])
                        i->flags |= CLIENT_FLAG_TRACK_KILLS;
                    break;

                case USER_OPT_LEGIT_ALWAYS:
                    /* Make sure the length is right */
                    if (length != 16)
                        break;

                    /* The only byte of the data that's used is the first
                       one. It is a boolean saying whether or not to always
                       enable /legit automatically. */
                    if (opt->data[0])
                        i->flags |= CLIENT_FLAG_ALWAYS_LEGIT;
                    break;

                case USER_OPT_WORD_CENSOR:
                    /* Make sure the length is right */
                    if (length != 16)
                        break;

                    /* The only byte of the data that's used is the first
                       one. It is a boolean saying whether or not to enable
                       the word censor. */
                    if (opt->data[0])
                        i->flags |= CLIENT_FLAG_WORD_CENSOR;
                    break;
                }

                /* Adjust the pointers to the next option */
                optptr = optptr + ntohl(opt->length);
                opt = (shipgate_user_opt_t*)optptr;
            }

            pthread_mutex_unlock(&i->mutex);
            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return 0;
}

static int handle_bbopts(shipgate_conn_t* c, shipgate_bb_opts_pkt* pkt) {
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* i;
    uint32_t gc = ntohl(pkt->guildcard), block = ntohl(pkt->block);

    /* Check the block number first. */
    if(block > s->cfg->blocks) {
        return 0;
    }

    b = s->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            pthread_mutex_lock(&i->mutex);

            /* 复制角色选项数据 */
            memcpy(i->bb_opts, &pkt->opts, sizeof(psocn_bb_db_opts_t));

            /* 复制角色选项数据 */
            //TEST_LOG("1111 %d 发送数据 %d 字节", c->sock, pkt->guild_id);
            if (pkt->guild_id != 0) {
                i->bb_guild->guild_data.guildcard = i->guildcard;
                i->bb_guild->guild_data.guild_id = pkt->guild_id;
                memcpy(&i->bb_guild->guild_data.guild_info[0], &pkt->guild_info[0], sizeof(pkt->guild_info));
                i->bb_guild->guild_data.guild_priv_level = pkt->guild_priv_level;
                memcpy(&i->bb_guild->guild_data.guild_name, &pkt->guild_name[0], sizeof(pkt->guild_name));
                i->bb_guild->guild_data.guild_rank = pkt->guild_rank;
                memcpy(&i->bb_guild->guild_data.guild_flag, &pkt->guild_flag[0], sizeof(pkt->guild_flag));
                i->bb_guild->guild_data.guild_rewards[0] = pkt->guild_rewards[0];
                i->bb_guild->guild_data.guild_rewards[1] = pkt->guild_rewards[1];
            }

            //memcpy(i->bb_guild, &pkt->guild, sizeof(psocn_bb_db_guild_t));

            /* Move the user on now that we have everything... */
            send_lobby_list(i);
            send_bb_full_char(i);
            send_simple(i, CHAR_DATA_REQUEST_TYPE, 0);

            pthread_mutex_unlock(&i->mutex);
            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return 0;
}

static int handle_schunk(shipgate_conn_t* c, shipgate_schunk_pkt* pkt) {
    ship_t* s = c->ship;
    FILE* fp;
    char filename[64];
    uint32_t len, crc;
    long len2;
    uint8_t* buf;
    uint8_t* sendbuf = get_sendbuf();
    shipgate_schunk_err_pkt* err = (shipgate_schunk_err_pkt*)sendbuf;
    uint8_t chtype = pkt->chunk_type & 0x7F;
    script_action_t action = ScriptActionInvalid;

    //DBG_LOG("handle_schunk chtype = %d", chtype);

    /* Make sure we have scripting enabled first, otherwise, just ignore this */
    if (!(s->cfg->shipgate_flags & LOGIN_FLAG_LUA)) {
        SHIPS_LOG("当前脚本已禁用,但是船闸服务器还是发送了脚本!");
        return 0;
    }

    len = ntohl(pkt->chunk_length);
    crc = ntohl(pkt->chunk_crc);
    action = (script_action_t)ntohl(pkt->action);

    /* Basic sanity checks... */
    if (len > 32768) {
        SHIPS_LOG("船闸服务器发送了巨大的脚本数据");
        return -1;
    }

    if (pkt->filename[31] || chtype > SCHUNK_TYPE_MODULE) {
        SHIPS_LOG("船闸服务器发送了无效的脚本数据块!");
        return -1;
    }

    if (action >= ScriptActionCount) {
        SHIPS_LOG("船闸服务器发送了未知类型的脚本文件!");
        return -1;
    }

    if (chtype == SCHUNK_TYPE_SCRIPT)
        snprintf(filename, 64, "Scripts/%s", pkt->filename);
    else
        snprintf(filename, 64, "Scripts/modules/%s", pkt->filename);

    /* Is this an actual chunk or a check packet? */
    if ((pkt->chunk_type & SCHUNK_CHECK)) {
        /* Check packet */
        /* Attempt to check the file, if it exists. */
        if ((fp = fopen(filename, "rb"))) {
            /* File exists, check the length */
            fseek(fp, 0, SEEK_END);
            len2 = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            if (len2 == (long)len) {
                /* File exists and is the right length, check the crc */
                if (!(buf = (uint8_t*)malloc(len))) {
                    ERR_LOG("分配脚本BUFF内存错误!");
                    fclose(fp);
                    return -1;
                }

                if (fread(buf, 1, len, fp) != len) {
                    ERR_LOG("无法读取脚本文件 '%s'",
                        filename);
                    free_safe(buf);
                    fclose(fp);
                    return -1;
                }

                fclose(fp);

                /* Check the CRC */
                if (psocn_crc32(buf, len) == crc) {
                    /* The CRCs match, so we already have this script. */
                    SHIPS_LOG("已存在脚本文件 '%s'", filename);
                    free_safe(buf);

                    /* If the action field is non-zero, go ahead and add the
                       script now, since we have it already. */
                    if (pkt->action && chtype == SCHUNK_TYPE_SCRIPT)
                        script_add(action, pkt->filename);

                    /* Notify the shipgate */
                    if (!sendbuf)
                        return -1;

                    memset(err, 0, sizeof(shipgate_schunk_err_pkt));
                    err->base.hdr.pkt_len =
                        htons(sizeof(shipgate_schunk_err_pkt));
                    err->base.hdr.pkt_type = htons(SHDR_TYPE_SCHUNK);
                    err->base.hdr.flags = htons(SHDR_RESPONSE);
                    err->type = chtype;
                    memcpy(err->filename, pkt->filename, 32);
                    return send_crypt(c, sizeof(shipgate_schunk_err_pkt),
                        sendbuf);
                }
            }
        }

        SHIPS_LOG("正在从船闸服务器获取脚本文件 '%s'", pkt->filename);

        /* 如果我们到了这里, we don't have a matching script, let the shipgate
           know by sending an error packet. */
        if (!sendbuf)
            return -1;

        memset(err, 0, sizeof(shipgate_schunk_err_pkt));
        err->base.hdr.pkt_len = htons(sizeof(shipgate_schunk_err_pkt));
        err->base.hdr.pkt_type = htons(SHDR_TYPE_SCHUNK);
        err->base.hdr.flags = htons(SHDR_RESPONSE | SHDR_FAILURE);
        err->base.error_code = htonl(ERR_SCHUNK_NEED_SCRIPT);
        err->type = chtype;
        memcpy(err->filename, pkt->filename, 32);
        return send_crypt(c, sizeof(shipgate_schunk_err_pkt), sendbuf);
    }
    else {
        /* Chunk packet */
        /* 合理性检查 the packet. */
        if (psocn_crc32(pkt->chunk, len) != crc) {
            /* XXXX */
            SHIPS_LOG("船闸服务器发送了带有错误校验的脚本");
            return 0;
        }

        if (!(fp = fopen(filename, "wb"))) {
            /* XXXX */
            SHIPS_LOG("无法读写脚本文件 '%s'",
                filename);
            return 0;
        }

        if (fwrite(pkt->chunk, 1, len, fp) != len) {
            /* XXXX */
            SHIPS_LOG("无法读写 '%s': %s",
                filename, strerror(errno));
            fclose(fp);
            return 0;
        }

        fclose(fp);

        SHIPS_LOG("船闸服务器发送了脚本文件 '%s' (校验: %08" PRIx32 ")",
            filename, crc);

        /* If the action field is non-zero, go ahead and add the script now. */
        if (pkt->action && chtype == SCHUNK_TYPE_SCRIPT)
            script_add(action, pkt->filename);
        else if (chtype == SCHUNK_TYPE_MODULE)
            script_update_module(pkt->filename);

        /* Notify the shipgate that we got it. */
        if (!sendbuf)
            return -1;

        memset(err, 0, sizeof(shipgate_schunk_err_pkt));
        err->base.hdr.pkt_len = htons(sizeof(shipgate_schunk_err_pkt));
        err->base.hdr.pkt_type = htons(SHDR_TYPE_SCHUNK);
        err->base.hdr.flags = htons(SHDR_RESPONSE);
        memcpy(err->filename, pkt->filename, 32);
        err->type = chtype;
        return send_crypt(c, sizeof(shipgate_schunk_err_pkt), sendbuf);
    }
}

static int handle_sset(shipgate_conn_t* c, shipgate_sset_pkt* pkt) {
    script_action_t action;

    /* 合理性检查 the packet. */
    if (pkt->action >= ScriptActionCount) {
        SHIPS_LOG("船闸服务器设置了无效事件的脚本 %" PRIu32
            "", pkt->action);
        return -1;
    }

    if (pkt->filename[31]) {
        SHIPS_LOG("船闸服务器设置的脚本文件名过长");
        return -1;
    }

    action = (script_action_t)pkt->action;

    /* Are we setting or unsetting the script? */
    if (pkt->filename[0]) {
        return script_add(action, pkt->filename);
    }
    else {
        /* The only reason script_remove() could return -1 is if the script
           wasn't set. Don't error out on that case. */
        script_remove(action);
        return 0;
    }
}

static int handle_sdata(shipgate_conn_t* c, shipgate_sdata_pkt* pkt) {
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* i;
    uint32_t gc = ntohl(pkt->guildcard), block = ntohl(pkt->block);

    /* Check the block number first. */
    if((int)block > s->cfg->blocks)
        return 0;

    b = s->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            pthread_mutex_lock(&i->mutex);
            script_execute(ScriptActionSData, i, SCRIPT_ARG_PTR, i,
                SCRIPT_ARG_UINT32, ntohl(pkt->event_id),
                SCRIPT_ARG_STRING, ntohl(pkt->data_len), pkt->data,
                SCRIPT_ARG_END);
            pthread_mutex_unlock(&i->mutex);
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return 0;
}

static int handle_qflag(shipgate_conn_t* c, shipgate_qflag_pkt* pkt) {
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* i;
    lobby_t* l;
    uint32_t gc = ntohl(pkt->guildcard), block = ntohl(pkt->block);
    uint32_t flag_id = ntohl(pkt->flag_id), value = ntohl(pkt->value), ctl;
    uint16_t flags = ntohs(pkt->hdr.flags), type = ntohs(pkt->hdr.pkt_type);
    uint8_t flag_reg;

    ctl = flag_id & 0xFFFF0000;
    flag_id = (flag_id & 0x0000FFFF) | (ntohs(pkt->flag_id_hi) << 16);

    /* Make sure the packet looks sane... */
    if (!(flags & SHDR_RESPONSE) || (flags & SHDR_FAILURE)) {
        SHIPS_LOG("船闸发送了错误的qflag数据包!");
        return -1;
    }

    /* Check the block number for sanity... */
    if((int)block > s->cfg->blocks)
        return -1;

    b = s->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            pthread_mutex_lock(&i->mutex);
            l = i->cur_lobby;

            /* 合理性检查... Make sure the user hasn't been booted from the
               lobby somehow. */
            if (!l) {
                pthread_mutex_unlock(&i->mutex);
                break;
            }

            /* Is this in response to a direct flag set or from a quest
               function call? */
            if (!(i->flags & CLIENT_FLAG_QSTACK_LOCK)) {
                /* 合理性检查... If we got this far, we should not have a long
                   flag. */
                if ((ctl & 0x80000000)) {
                    /* Drop the sync, because it either wasn't requested or
                       something else like that... */
                    SHIPS_LOG("任务功能未做请求时,船闸服务器尝试同步过长的FLAG!");
                    return 0;
                }

                /* Grab the register from the lobby... */
                pthread_mutex_lock(&l->mutex);
                flag_reg = l->q_shortflag_reg;
                pthread_mutex_unlock(&l->mutex);

                /* Make the value that the quest is expecting... */
                if (flag_id > 0xFF) {
                    SHIPS_LOG("船闸服务器尝试同步无效的任务FLAG (ID %" PRIu32 ") FLAG 请求", flag_id);
                    return 0;
                }

                if (type == SHDR_TYPE_QFLAG_GET)
                    value = (value & 0xFFFF) | 0x40000000 | (flag_id << 16);
                else
                    value = (value & 0xFFFF) | 0x60000000 | (flag_id << 16);

                send_sync_register(i, flag_reg, value);
            }
            else {
                block = ((type == SHDR_TYPE_QFLAG_SET) ?
                    QFLAG_REPLY_SET : QFLAG_REPLY_GET);
                quest_flag_reply(i, block, value);
            }

            pthread_mutex_unlock(&i->mutex);
            break;

        }
    }

    pthread_rwlock_unlock(&b->lock);
    return 0;
}

static int handle_qflag_err(shipgate_conn_t* c, shipgate_qflag_err_pkt* pkt) {
    uint32_t gc = ntohl(pkt->guildcard);
    uint32_t block = ntohl(pkt->block);
    uint32_t value = ntohl(pkt->base.error_code);
    uint32_t type = ntohs(pkt->base.hdr.pkt_type);
    uint32_t flag_id = ntohl(pkt->flag_id);
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* i;
    lobby_t* l;
    uint8_t flag_reg;

    /* Grab the block first */
    if((int)block > s->cfg->blocks || !(b = s->blocks[block - 1])) {
        return 0;
    }

    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client and inform the quest of the error. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            pthread_mutex_lock(&i->mutex);
            l = i->cur_lobby;

            /* 合理性检查... Make sure the user hasn't been booted from the
               lobby somehow. */
            if (!l) {
                pthread_mutex_unlock(&i->mutex);
                break;
            }

            /* Is this in response to a direct flag set or from a quest
               function call? */
            if (!(i->flags & CLIENT_FLAG_QSTACK_LOCK)) {
                /* 合理性检查... If we got this far, we should not have a long
                   flag. */
                if ((flag_id & 0x80000000)) {
                    /* Drop the sync, because it either wasn't requested or
                       something else like that... */
                    SHIPS_LOG("任务功能未做请求时,船闸服务器尝试同步过长的FLAG!");
                    return 0;
                }

                /* Grab the register from the lobby... */
                pthread_mutex_lock(&l->mutex);
                flag_reg = l->q_shortflag_reg;
                pthread_mutex_unlock(&l->mutex);

                /* Make the value that the quest is expecting... */
                if (value == ERR_BAD_ERROR)
                    value = 0x8000FFFF;
                else
                    value = (value & 0xFFFF) | 0x80000000;

                send_sync_register(i, flag_reg, value);
            }
            else {
                /* What's the error code? */
                if (value == ERR_QFLAG_INVALID_FLAG)
                    value = (uint32_t)-1;
                else if (value == ERR_QFLAG_NO_DATA)
                    value = (uint32_t)-3;
                else
                    value = (uint32_t)-2;

                block = ((type == SHDR_TYPE_QFLAG_SET) ?
                    QFLAG_REPLY_SET : QFLAG_REPLY_GET) |
                    QFLAG_REPLY_ERROR;

                quest_flag_reply(i, block, value);
            }
            pthread_mutex_unlock(&i->mutex);
            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);

    return 0;
}

static int handle_sctl_sd(shipgate_conn_t* c, shipgate_sctl_shutdown_pkt* pkt,
    int restart) {
    uint32_t when = ntohl(pkt->when);
    uint8_t* sendbuf = get_sendbuf();
    shipgate_sctl_err_pkt* err = (shipgate_sctl_err_pkt*)sendbuf;

    SHIPS_LOG("船闸将要在 %" PRIu32 " 分钟后 %s ", when,
        restart ? "重启" : "关闭");
    schedule_shutdown(NULL, when, restart, NULL);

    if (!sendbuf)
        return -1;

    memset(err, 0, sizeof(shipgate_sctl_err_pkt));
    err->base.hdr.pkt_len = htons(sizeof(shipgate_sctl_err_pkt));
    err->base.hdr.pkt_type = htons(SHDR_TYPE_SHIP_CTL);
    err->base.hdr.flags = htons(SHDR_RESPONSE);
    err->base.error_code = 0;
    err->ctl = pkt->ctl;
    err->acc = pkt->acc;
    err->reserved1 = pkt->reserved1;
    err->reserved2 = pkt->reserved2;
    return send_crypt(c, sizeof(shipgate_sctl_err_pkt), sendbuf);
}

static void conv_shaid(uint8_t out[20], const char* shaid) {
    int i;
#define NTI(ch) ((ch <= '9') ? (ch - '0') : (ch - 'a' + 0x0a))

    for (i = 0; i < 20; ++i) {
        out[i] = (NTI(shaid[i << 1]) << 4) | (NTI(shaid[(i << 1) + 1]));
    }

#undef NTI
}

#define GIT_REMOTE_URL "http://www.phantasystaronline.cn"
#define GIT_BRANCH "main"
#define GIT_SHAID "111111111"
#define GIT_TIMESTAMP 1402221655
#define GIT_IS_DIST

static int handle_sctl_ver(shipgate_conn_t* c, shipgate_shipctl_pkt* pkt) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_sctl_ver_reply_pkt* rep = (shipgate_sctl_ver_reply_pkt*)sendbuf;
    uint16_t len;

#ifndef GIT_IS_DIST
    uint16_t reflen = strlen(GIT_REMOTE_URL) + strlen(GIT_BRANCH) + 3;
    char remote_ref[56];

    if (!sendbuf)
        return -1;

    sprintf(remote_ref, "%s::%s", GIT_REMOTE_URL, GIT_BRANCH);
    len = (reflen + 7 + sizeof(shipgate_sctl_ver_reply_pkt)) & 0xfff8;

    memset(rep, 0, len);
    rep->hdr.pkt_len = htons(len);
    rep->hdr.pkt_type = htons(SHDR_TYPE_SHIP_CTL);
    rep->hdr.flags = htons(SHDR_RESPONSE);
    rep->ctl = htonl(SCTL_TYPE_VERSION);
    rep->unused = pkt->acc;
    rep->reserved1 = pkt->reserved1;
    rep->reserved2 = pkt->reserved2;
    parse_version(&rep->ver_major, &rep->ver_minor, &rep->ver_micro, SHIPS_SERVER_VERSION);
    rep->flags = 1;
#ifdef GIT_DIRTY
    rep->flags |= GIT_DIRTY ? 2 : 0;
#endif
    conv_shaid(rep->commithash, GIT_SHAID);
    rep->committime = SWAP64(GIT_TIMESTAMP);
    memcpy(rep->remoteref, remote_ref, reflen);
#else

    if (!sendbuf)
        return -1;

    len = sizeof(shipgate_sctl_ver_reply_pkt);
    memset(rep, 0, len);
    rep->hdr.pkt_len = htons(len);
    rep->hdr.pkt_type = htons(SHDR_TYPE_SHIP_CTL);
    rep->hdr.flags = htons(SHDR_RESPONSE);
    rep->ctl = htonl(SCTL_TYPE_VERSION);
    rep->unused = pkt->acc;
    rep->reserved1 = pkt->reserved1;
    rep->reserved2 = pkt->reserved2;
    parse_version(&rep->ver_major, &rep->ver_minor, &rep->ver_micro, SHIPS_SERVER_VERSION);
    rep->flags = 0;
#endif

    return send_crypt(c, len, sendbuf);
}

static int handle_sctl_uname(shipgate_conn_t* c, shipgate_shipctl_pkt* pkt) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_sctl_uname_reply_pkt* r = (shipgate_sctl_uname_reply_pkt*)sendbuf;
    struct win_utsname un;

    if (!sendbuf)
        return -1;

    if(win_uname(&un))
        return -1;

    memset(r, 0, sizeof(shipgate_sctl_uname_reply_pkt));
    r->hdr.pkt_len = htons(sizeof(shipgate_sctl_uname_reply_pkt));
    r->hdr.pkt_type = htons(SHDR_TYPE_SHIP_CTL);
    r->hdr.flags = htons(SHDR_RESPONSE);
    r->ctl = htonl(SCTL_TYPE_UNAME);
    r->unused = pkt->acc;
    r->reserved1 = pkt->reserved1;
    r->reserved2 = pkt->reserved2;
    memcpy_str(r->name, un.sysname, 64);
    memcpy_str(r->node, un.nodename, 64);
    memcpy_str(r->release, un.release, 64);
    memcpy_str(r->version, un.version, 64);
    memcpy_str(r->machine, un.machine, 64);

    return send_crypt(c, sizeof(shipgate_sctl_uname_reply_pkt), sendbuf);
}

static int handle_sctl(shipgate_conn_t* c, shipgate_shipctl_pkt* pkt) {
    uint32_t type = ntohl(pkt->ctl);
    uint8_t* sendbuf = get_sendbuf();
    shipgate_sctl_err_pkt* err = (shipgate_sctl_err_pkt*)sendbuf;

    //DBG_LOG("handle_sctl type = %d", type);

    switch (type) {
    case SCTL_TYPE_RESTART:
        return handle_sctl_sd(c, (shipgate_sctl_shutdown_pkt*)pkt, 1);

    case SCTL_TYPE_SHUTDOWN:
        return handle_sctl_sd(c, (shipgate_sctl_shutdown_pkt*)pkt, 0);

    case SCTL_TYPE_VERSION:
        return handle_sctl_ver(c, (shipgate_shipctl_pkt*)pkt);

    case SCTL_TYPE_UNAME:
        return handle_sctl_uname(c, (shipgate_shipctl_pkt*)pkt);
    }

    /* If we get this far, then we don't know what the ctl is, send an error */
    if (!sendbuf)
        return -1;

    memset(err, 0, sizeof(shipgate_sctl_err_pkt));
    err->base.hdr.pkt_len = htons(sizeof(shipgate_sctl_err_pkt));
    err->base.hdr.pkt_type = htons(SHDR_TYPE_SHIP_CTL);
    err->base.hdr.flags = htons(SHDR_RESPONSE | SHDR_FAILURE);
    err->base.error_code = htonl(ERR_SCTL_UNKNOWN_CTL);
    err->ctl = pkt->ctl;
    err->acc = pkt->acc;
    err->reserved1 = pkt->reserved1;
    err->reserved2 = pkt->reserved2;
    return send_crypt(c, sizeof(shipgate_sctl_err_pkt), sendbuf);
}

static int handle_ubl(shipgate_conn_t* c, shipgate_user_blocklist_pkt* pkt) {
    ship_t* s = c->ship;
    block_t* b;
    ship_client_t* i;
    uint32_t gc, block, count, j;
    client_blocklist_t* list;
    uint16_t len = ntohs(pkt->hdr.pkt_len);

    /* Make sure the packet is one we understand and of a valid size. */
    if (len < sizeof(shipgate_user_blocklist_pkt))
        return -1;

    if (pkt->hdr.version != 0)
        return -1;

    gc = ntohl(pkt->guildcard);
    block = ntohl(pkt->block);
    count = ntohl(pkt->count);

    /* 合理性检查 */
    if (len < sizeof(shipgate_user_blocklist_pkt) + 8 * count)
        return -1;

    if ((int)block > s->cfg->blocks)
        return 0;

    b = s->blocks[block - 1];
    pthread_rwlock_rdlock(&b->lock);

    /* Find the requested client and copy the data over. */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            pthread_mutex_lock(&i->mutex);

            list = (client_blocklist_t*)malloc(sizeof(client_blocklist_t) *
                count);
            if (!list) {
                ERR_LOG("%s: 从船闸服务器处理客户端封禁列表时内存不足");
                pthread_mutex_unlock(&i->mutex);
                pthread_rwlock_unlock(&b->lock);
                return -1;
            }

            for (j = 0; j < count; ++j) {
                list[j].gc = ntohl(pkt->entries[j].gc);
                list[j].flags = ntohl(pkt->entries[j].flags);
            }

            if (i->blocklist)
                free_safe(i->blocklist);

            i->blocklist_size = count;
            i->blocklist = list;

            pthread_mutex_unlock(&i->mutex);
            break;
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return 0;
}

static int handle_max_tech_level_bb(shipgate_conn_t* conn, shipgate_max_tech_lvl_bb_pkt* pkt) {
    int i, j;

    /* TODO 需增加加密传输认证,防止黑客行为 */
    if (!pkt->data) {
        ERR_LOG("舰船接收职业最大法术数据失败, 请检查函数错误");
        return -1;
    }

    memcpy(max_tech_level, pkt->data, sizeof(max_tech_level));

    for (i = 0; i < BB_MAX_TECH_LEVEL; i++) {
        for (j = 0; j < MAX_PLAYER_CLASS_BB; j++) {
            if (max_tech_level[i].tech_name == NULL) {
                ERR_LOG("舰船接收职业最大法术名称为空, 请检查函数错误");
                return -1;
            }
        }
    }

    CONFIG_LOG("接收 Blue Burst 玩家 %d 个职业 %d 个法术最大等级数据", j, i);

#ifdef DEBUG
    DBG_LOG("刷新BB职业最大法术数据");
    for (i = 0; i < BB_MAX_TECH_LEVEL; i++) {
        for (j = 0; j < MAX_PLAYER_CLASS_BB; j++) {
            DBG_LOG("法术 %d.%s 职业 %d 等级 %d", i, max_tech_level[i].tech_name, j, max_tech_level[i].max_lvl[j]);
        }
    }
#endif // DEBUG

    return 0;
}

static int handle_pl_level_bb(shipgate_conn_t* conn, shipgate_pl_level_bb_pkt* pkt) {
    int i, j;

    /* TODO 需增加加密传输认证,防止黑客行为 */
    if (!&pkt->data) {
        ERR_LOG("舰船接收职业等级数据失败, 请检查函数错误");
        return -1;
    }

    bb_char_stats = pkt->data;
    //memcpy(&bb_char_stats, &pkt->data, sizeof(bb_level_table_t));

    for (i = 0; i < MAX_PLAYER_CLASS_BB; i++) {
        if (bb_char_stats.start_stats_index[i] != i * 14) {
            ERR_LOG("舰船接收职业等级索引错误, 请检查函数错误");
            return -1;
        }
    }

    for (j = 0; j < MAX_PLAYER_CLASS_BB; j++) {
        for (i = 0; i < MAX_PLAYER_LEVEL; i++) {
            //DBG_LOG("职业 %d 等级 %d ATP数值 %d", j, i, bb_char_stats.levels[j][i].atp);
        }
        CONFIG_LOG("接收 Blue Burst 职业 %s %d 等级数据", pso_class[j].cn_name, i);
    }

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
    /* Swap all the exp values */
    for (j = 0; j < MAX_PLAYER_CLASS_BB; ++j) {
        for (i = 0; i < MAX_PLAYER_LEVEL; ++i) {
            bb_char_stats.levels[j][i].exp = LE32(bb_char_stats.levels[j][i].exp);
        }
    }
#endif

#ifdef DEBUG
    //[2023年02月08日 13:45:14:010] 设置(1337): 读取 Blue Burst 升级数据表...
//[2023年02月08日 13:52:48:814] 调试(mapdata.c 0235): start_stats_index 0
//[2023年02月08日 13:52:48:823] 调试(mapdata.c 0235): start_stats_index 14
//[2023年02月08日 13:52:48:833] 调试(mapdata.c 0235): start_stats_index 28
//[2023年02月08日 13:52:48:842] 调试(mapdata.c 0235): start_stats_index 42
//[2023年02月08日 13:52:48:851] 调试(mapdata.c 0235): start_stats_index 56
//[2023年02月08日 13:52:48:860] 调试(mapdata.c 0235): start_stats_index 70
//[2023年02月08日 13:52:48:870] 调试(mapdata.c 0235): start_stats_index 84
//[2023年02月08日 13:52:48:879] 调试(mapdata.c 0235): start_stats_index 98
//[2023年02月08日 13:52:48:887] 调试(mapdata.c 0235): start_stats_index 112
//[2023年02月08日 13:52:48:896] 调试(mapdata.c 0235): start_stats_index 126
//[2023年02月08日 13:52:48:905] 调试(mapdata.c 0235): start_stats_index 140
//[2023年02月08日 13:52:48:915] 调试(mapdata.c 0235): start_stats_index 154
//[2023年02月08日 13:45:14:023] 调试(mapdata.c 0224): unk 0
//[2023年02月08日 13:45:14:030] 调试(mapdata.c 0224): unk E
//[2023年02月08日 13:45:14:037] 调试(mapdata.c 0224): unk 1C
//[2023年02月08日 13:45:14:044] 调试(mapdata.c 0224): unk 2A
//[2023年02月08日 13:45:14:053] 调试(mapdata.c 0224): unk 38
//[2023年02月08日 13:45:14:060] 调试(mapdata.c 0224): unk 46
//[2023年02月08日 13:45:14:069] 调试(mapdata.c 0224): unk 54
//[2023年02月08日 13:45:14:076] 调试(mapdata.c 0224): unk 62
//[2023年02月08日 13:45:14:082] 调试(mapdata.c 0224): unk 70
//[2023年02月08日 13:45:14:089] 调试(mapdata.c 0224): unk 7E
//[2023年02月08日 13:45:14:098] 调试(mapdata.c 0224): unk 8C
//[2023年02月08日 13:45:14:107] 调试(mapdata.c 0224): unk 9A
    for (i = 0; i < MAX_PLAYER_CLASS_BB; i++) {
        DBG_LOG("start_stats_index %d", bb_char_stats.start_stats_index[i]);
    }

#endif // DEBUG

    return 0;
}

static int handle_pkt(shipgate_conn_t* conn, shipgate_hdr_t* pkt) {
    uint16_t type = ntohs(pkt->pkt_type);
    uint16_t flags = ntohs(pkt->flags);

    //DBG_LOG("S->G指令: 0x%04X %s 标识 = %d 密钥 = %d 失败代码 %d"
        //, type, s_cmd_name(type, 0), flags, conn->has_key, flags & SHDR_FAILURE);

    if (!conn->has_key) {
        /* Silently ignore non-login packets when we're without a key
         当我们没有密钥时，自动忽略非登录数据包 
        . */
        if (type != SHDR_TYPE_LOGIN && type != SHDR_TYPE_LOGIN6) {
            return 0;
        }

        if (type == SHDR_TYPE_LOGIN && !(flags & SHDR_RESPONSE)) {
            return handle_login(conn, (shipgate_login_pkt*)pkt);
        }
        else if (type == SHDR_TYPE_LOGIN6 && (flags & SHDR_RESPONSE)) {
            return handle_login_reply(conn, (shipgate_error_pkt*)pkt);
        }
        else {
            return 0;
        }
    }

    /* See if this is an error packet */
    if (flags & SHDR_FAILURE) {
        switch (type) {
        case SHDR_TYPE_DC:
        case SHDR_TYPE_PC:
        case SHDR_TYPE_BB:
            /* Ignore these for now, we shouldn't get them. */
            return 0;

        case SHDR_TYPE_CDATA:
            return handle_char_data_db_save(conn, (shipgate_cdata_err_pkt*)pkt);

        case SHDR_TYPE_CREQ:
        case SHDR_TYPE_CBKUP:
            return handle_char_data_req_err(conn, (shipgate_cdata_err_pkt*)pkt);

        case SHDR_TYPE_USRLOGIN:
            return handle_usrlogin_err(conn, (shipgate_gm_err_pkt*)pkt);

        case SHDR_TYPE_IPBAN:
        case SHDR_TYPE_GCBAN:
            return handle_ban(conn, (shipgate_ban_err_pkt*)pkt);

        case SHDR_TYPE_BLKLOGIN:
        case SHDR_TYPE_BLKLOGOUT:
            return handle_blogin_err(conn, (shipgate_blogin_err_pkt*)pkt);

        case SHDR_TYPE_ADDFRIEND:
            return handle_addfriend(conn, (shipgate_friend_err_pkt*)pkt);

        case SHDR_TYPE_DELFRIEND:
            return handle_delfriend(conn, (shipgate_friend_err_pkt*)pkt);

        case SHDR_TYPE_QFLAG_SET:
        case SHDR_TYPE_QFLAG_GET:
            return handle_qflag_err(conn, (shipgate_qflag_err_pkt*)pkt);

        default:
            ERR_LOG("%s: 船闸发送未知错误1! 指令 = 0x%04X 标识 = %d 密钥 = %d"
                , conn->ship->cfg->name, type, flags, conn->has_key);
            return -1;
        }
    }
    else {
        switch (type) {
        case SHDR_TYPE_DC:
            return handle_dc(conn, (shipgate_fw_9_pkt*)pkt);

        case SHDR_TYPE_PC:
            return handle_pc(conn, (shipgate_fw_9_pkt*)pkt);

        case SHDR_TYPE_BB:
            return handle_bb(conn, (shipgate_fw_9_pkt*)pkt);

        case SHDR_TYPE_SSTATUS:
            return handle_sstatus(conn, (shipgate_ship_status6_pkt*)pkt);

        case SHDR_TYPE_PING:
            /* Ignore responses for now... we don't send these just yet. */
            if (flags & SHDR_RESPONSE) {
                return 0;
            }

            return shipgate_send_ping(conn, 1);

        case SHDR_TYPE_CREQ:
            return handle_char_data_req(conn, (shipgate_char_data_pkt*)pkt);

        case SHDR_TYPE_USRLOGIN:
            return handle_usrlogin(conn, (shipgate_usrlogin_reply_pkt*)pkt);

        case SHDR_TYPE_COUNT:
            return handle_count(conn, (shipgate_cnt_pkt*)pkt);

        case SHDR_TYPE_CDATA:
            return handle_char_data_db_save(conn, (shipgate_cdata_err_pkt*)pkt);

        case SHDR_TYPE_IPBAN:
        case SHDR_TYPE_GCBAN:
            return handle_ban(conn, (shipgate_ban_err_pkt*)pkt);

        case SHDR_TYPE_BLKLOGIN:
            return 0;

        case SHDR_TYPE_BLKLOGOUT:
            return 0;

        case SHDR_TYPE_FRLOGIN:
        case SHDR_TYPE_FRLOGOUT:
            return handle_friend(conn, (shipgate_friend_login_4_pkt*)pkt);

        case SHDR_TYPE_ADDFRIEND:
            return handle_addfriend(conn, (shipgate_friend_err_pkt*)pkt);

        case SHDR_TYPE_DELFRIEND:
            return handle_delfriend(conn, (shipgate_friend_err_pkt*)pkt);

        case SHDR_TYPE_KICK:
            return handle_kick(conn, (shipgate_kick_pkt*)pkt);

        case SHDR_TYPE_FRLIST:
            return handle_frlist(conn, (shipgate_friend_list_pkt*)pkt);

        case SHDR_TYPE_GLOBALMSG:
            return handle_globalmsg(conn, (shipgate_global_msg_pkt*)pkt);

        case SHDR_TYPE_USEROPT:
            /* We really should notify the user either way... but for now,
               punt. */
            if (flags & (SHDR_RESPONSE | SHDR_FAILURE)) {
                return 0;
            }

            return handle_useropt(conn, (shipgate_user_opt_pkt*)pkt);

        case SHDR_TYPE_BBOPTS:
            return handle_bbopts(conn, (shipgate_bb_opts_pkt*)pkt);

        case SHDR_TYPE_BBMAXTECH:
            return handle_max_tech_level_bb(conn, (shipgate_max_tech_lvl_bb_pkt*)pkt);

        case SHDR_TYPE_BBLVLDATA:
            return handle_pl_level_bb(conn, (shipgate_pl_level_bb_pkt*)pkt);

        case SHDR_TYPE_CBKUP:
            if (!(flags & SHDR_RESPONSE)) {
                /* We should never get a non-response version of this. */
                return -1;
            }

            /* No need really to notify the user. */
            return 0;

        case SHDR_TYPE_SCHUNK:
            return handle_schunk(conn, (shipgate_schunk_pkt*)pkt);

        case SHDR_TYPE_SSET:
            return handle_sset(conn, (shipgate_sset_pkt*)pkt);

        case SHDR_TYPE_SDATA:
            return handle_sdata(conn, (shipgate_sdata_pkt*)pkt);

        case SHDR_TYPE_QFLAG_SET:
        case SHDR_TYPE_QFLAG_GET:
            return handle_qflag(conn, (shipgate_qflag_pkt*)pkt);

        case SHDR_TYPE_SHIP_CTL:
            return handle_sctl(conn, (shipgate_shipctl_pkt*)pkt);

        case SHDR_TYPE_UBLOCKS:
            return handle_ubl(conn, (shipgate_user_blocklist_pkt*)pkt);
            /*
        case SHDR_TYPE_8000:
            return 0;

        default:
            ERR_LOG(
                "%s: 船闸发送未知错误2! 指令 = 0x%04X 标识 = %d 密钥 = %d"
                , conn->ship->cfg->name, type, flags, conn->has_key
            );
            return 0;*/

        case SHDR_TYPE_CHECK_PLONLINE:
            DBG_LOG("测试");
            return 0;
        }
    }

    return -1;
}

/* 从船闸服务器读取数据流. */
int shipgate_process_pkt(shipgate_conn_t* c) {
    ssize_t sz;
    uint16_t pkt_sz;
    int rv = 0;
    unsigned char* rbp;
    uint8_t* recvbuf = get_recvbuf();
    void* tmp;

    /* If we've got anything buffered, copy it out to the main buffer to make
       the rest of this a bit easier. */
    if (c->recvbuf_cur) {
        memcpy(recvbuf, c->recvbuf, c->recvbuf_cur);
    }

    /* Attempt to read, and if we don't get anything, punt. */
    sz = sg_recv(c, recvbuf + c->recvbuf_cur,
        65536 - c->recvbuf_cur);

    //TEST_LOG("舰船接收数据 %d 接收数据 %d 字节", c->sock, sz);

    /* Attempt to read, and if we don't get anything, punt. */
    if (sz <= 0) {
        if (sz == SOCKET_ERROR) {
            ERR_LOG("舰船接收数据错误: %s", strerror(errno));
        }

        goto end;
    }

    sz += c->recvbuf_cur;
    c->recvbuf_cur = 0;
    rbp = recvbuf;

    /* As long as what we have is long enough, decrypt it. */
    while(sz >= 8 && rv == 0) {
        /* Copy out the packet header so we know what exactly we're looking
           for, in terms of packet length. */
        if(!c->hdr_read) {
            memcpy(&c->pkt, rbp, 8);
            c->hdr_read = 1;
        }

        /* Read the packet size to see how much we're expecting. */
        pkt_sz = ntohs(c->pkt.pkt_len);

        /* We'll always need a multiple of 8 bytes. */
        if(pkt_sz & 0x07) {
            pkt_sz = (pkt_sz & 0xFFF8) + 8;
        }

        /* Do we have the whole packet? */
        if(sz >= (ssize_t)pkt_sz) {
            /* Yes, we do, copy it out. */
            memcpy(rbp, &c->pkt, 8);

            /* Pass it on. */
            if((rv = handle_pkt(c, (shipgate_hdr_t *)rbp))) {
                break;
            }

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

    /* If we've still got something left here, buffer it for the next pass. */
    if(sz && rv == 0) {
        /* Reallocate the recvbuf for the client if its too small. */
        if(c->recvbuf_size < sz) {
            tmp = realloc(c->recvbuf, sz);

            if(!tmp) {
                perror("realloc");
                return -1;
            }

            c->recvbuf = (unsigned char *)tmp;
            c->recvbuf_size = sz;
        }

        memcpy(c->recvbuf, rbp, sz);
        c->recvbuf_cur = sz;
    }
    else if(c->recvbuf) {
        /* Free the buffer, if we've got nothing in it. */
        free_safe(c->recvbuf);
        c->recvbuf = NULL;
        c->recvbuf_size = 0;
    }

    return rv;

end:
    return sz;
}

/* Send any piled up data. */
int shipgate_send_pkts(shipgate_conn_t* c) {
    ssize_t amt;

    /* Don't even try if there's not a connection. */
    if (!c->has_key || c->sock < 0) {
        return 0;
    }

    /* Send as much as we can. */
    amt = sg_send(c, c->sendbuf, c->sendbuf_cur);

    //DBG_LOG("零碎数据端口 %d 发送数据 %d 字节", c->sock, amt);

    if (amt == SOCKET_ERROR) {
        perror("send");
        return -1;
    }
    else if (amt == c->sendbuf_cur) {
        c->sendbuf_cur = 0;
    }
    else {
        memmove(c->sendbuf, c->sendbuf + amt, c->sendbuf_cur - amt);
        c->sendbuf_cur -= amt;
    }

    return 0;
}

/* Packets are below here. */
/* Send the shipgate a character data save request. */
int shipgate_send_cdata(shipgate_conn_t* c, uint32_t gc, uint32_t slot,
    const void* cdata, size_t len, uint32_t block) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_char_data_pkt* pkt = (shipgate_char_data_pkt*)sendbuf;
    uint16_t pkt_size = (uint16_t)(sizeof(shipgate_char_data_pkt) + len);

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* 填充数据头. */
    pkt->hdr.pkt_len = htons(pkt_size);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_CDATA);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    /* Fill in the body. */
    pkt->guildcard = htonl(gc);
    pkt->slot = htonl(slot);
    pkt->block = htonl(block);
    memcpy(pkt->data, cdata, len);

    /* 加密并发送. */
    return send_crypt(c, pkt_size, sendbuf);
}

/* Send the shipgate a request for character data. */
int shipgate_send_creq(shipgate_conn_t* c, uint32_t gc, uint32_t slot) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_char_req_pkt* pkt = (shipgate_char_req_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* 填充数据头 and the body. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_char_req_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_CREQ);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;
    pkt->guildcard = htonl(gc);
    pkt->slot = htonl(slot);

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_char_req_pkt), sendbuf);
}

/* Send a newly opened ship's information to the shipgate. */
int shipgate_send_ship_info(shipgate_conn_t* c, ship_t* ship) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_login6_reply_pkt* pkt = (shipgate_login6_reply_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Clear the packet first */
    memset(pkt, 0, sizeof(shipgate_login6_reply_pkt));

    /* 填充数据头. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_login6_reply_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_LOGIN6);
    pkt->hdr.flags = htons(SHDR_RESPONSE);

    /* 填充数据并准备发送 */
    pkt->proto_ver = htonl(ship->cfg->shipgate_proto_ver);//htonl(SHIPGATE_PROTO_VER);
    pkt->flags = htonl(ship->cfg->shipgate_flags);
    strncpy((char*)pkt->name, ship->cfg->name, 11);
    pkt->name[11] = 0;
    strncpy((char*)pkt->ship_host4, ship->cfg->ship_host4, 31);
    pkt->ship_host4[31] = 0;
    pkt->ship_addr4 = ship_ip4;

    if (enable_ipv6) {
        strncpy((char*)pkt->ship_host6, ship->cfg->ship_host6, 127);
        pkt->ship_host6[127] = 0;
        //pkt->ship_host6 = ship->cfg->ship_host6;
        memcpy(pkt->ship_addr6, ship_ip6, 16);
    }
    else {
        memset(pkt->ship_host6, 0, sizeof(pkt->ship_host6));
        //pkt->ship_host6 = 0;
        memset(pkt->ship_addr6, 0, 16);
    }

    pkt->ship_port = htons(ship->cfg->base_port);
    pkt->clients = htons(ship->num_clients);
    pkt->games = htons(ship->num_games);
    pkt->menu_code = htons(ship->cfg->menu_code);
    pkt->privileges = ntohl(ship->cfg->privileges);

    /* 加密并发送 */
    return send_raw(c, sizeof(shipgate_login6_reply_pkt), sendbuf, 0);
}

/* Send a client count update to the shipgate. */
int shipgate_send_cnt(shipgate_conn_t* c, uint16_t clients, uint16_t games) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_cnt_pkt* pkt = (shipgate_cnt_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* 填充数据头. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_cnt_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_COUNT);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    /* 填充数据并准备发送 */
    pkt->clients = htons(clients);
    pkt->games = htons(games);
    pkt->ship_id = 0;                   /* Ignored on ship->gate packets. */

    /* 加密并发送 */
    return send_crypt(c, sizeof(shipgate_cnt_pkt), sendbuf);
}

/* Forward a Dreamcast packet to the shipgate. */
int shipgate_fw_dc(shipgate_conn_t* c, const void* dcp, uint32_t flags,
    ship_client_t* req) {
    uint8_t* sendbuf = get_sendbuf();
    const dc_pkt_hdr_t* dc = (const dc_pkt_hdr_t*)dcp;
    shipgate_fw_9_pkt* pkt = (shipgate_fw_9_pkt*)sendbuf;
    int dc_len = LE16(dc->pkt_len);
    int full_len = sizeof(shipgate_fw_9_pkt) + dc_len;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Copy the packet, unchanged */
    memmove(pkt->pkt, dc, dc_len);

    /* Round up the packet size, if needed. */
    while (full_len & 0x07) {
        sendbuf[full_len++] = 0;
    }

    /* Fill in the shipgate header */
    pkt->hdr.pkt_len = htons(full_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_DC);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;
    pkt->fw_flags = htonl(flags);
    pkt->guildcard = htonl(req->guildcard);
    pkt->block = htonl(req->cur_block->b);

    /* 将数据包发送出去 */
    return send_crypt(c, full_len, sendbuf);
}

/* Forward a PC packet to the shipgate. */
int shipgate_fw_pc(shipgate_conn_t* c, const void* pcp, uint32_t flags,
    ship_client_t* req) {
    uint8_t* sendbuf = get_sendbuf();
    const dc_pkt_hdr_t* pc = (const dc_pkt_hdr_t*)pcp;
    shipgate_fw_9_pkt* pkt = (shipgate_fw_9_pkt*)sendbuf;
    int pc_len = LE16(pc->pkt_len);
    int full_len = sizeof(shipgate_fw_9_pkt) + pc_len;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Copy the packet, unchanged */
    memmove(pkt->pkt, pc, pc_len);

    /* Round up the packet size, if needed. */
    while (full_len & 0x07) {
        sendbuf[full_len++] = 0;
    }

    /* Fill in the shipgate header */
    pkt->hdr.pkt_len = htons(full_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_PC);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;
    pkt->fw_flags = flags;
    pkt->guildcard = htonl(req->guildcard);
    pkt->block = htonl(req->cur_block->b);

    /* 将数据包发送出去 */
    return send_crypt(c, full_len, sendbuf);
}

/* Forward a Blue Burst packet to the shipgate. */
int shipgate_fw_bb(shipgate_conn_t* c, const void* bbp, uint32_t flags,
    ship_client_t* req) {
    uint8_t* sendbuf = get_sendbuf();
    const bb_pkt_hdr_t* bb = (const bb_pkt_hdr_t*)bbp;
    shipgate_fw_9_pkt* pkt = (shipgate_fw_9_pkt*)sendbuf;
    int bb_len = LE16(bb->pkt_len);
    int full_len = sizeof(shipgate_fw_9_pkt) + bb_len;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Copy the packet, unchanged */
    memmove(pkt->pkt, bb, bb_len);

    /* Round up the packet size, if needed. */
    while (full_len & 0x07) {
        sendbuf[full_len++] = 0;
    }

    /* Fill in the shipgate header */
    pkt->hdr.pkt_len = htons(full_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_BB);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;
    pkt->fw_flags = flags;
    pkt->guildcard = htonl(req->guildcard);
    pkt->block = htonl(req->cur_block->b);

    /* 将数据包发送出去 */
    return send_crypt(c, full_len, sendbuf);
}

/* Send a user login request. */
int shipgate_send_usrlogin(shipgate_conn_t* c, uint32_t gc, uint32_t block,
                           const char *username, const char *password,
                           int tok, uint8_t ver) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_usrlogin_req_pkt* pkt = (shipgate_usrlogin_req_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Fill in the data. */
    memset(pkt, 0, sizeof(shipgate_usrlogin_req_pkt));

    pkt->hdr.pkt_len = htons(sizeof(shipgate_usrlogin_req_pkt));

    if (!tok)
        pkt->hdr.pkt_type = htons(SHDR_TYPE_USRLOGIN);
    else
        pkt->hdr.pkt_type = htons(SHDR_TYPE_TLOGIN);

    pkt->hdr.version = ver;
    pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);
    strncpy(pkt->username, username, 31);
    strncpy(pkt->password, password, 31);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_usrlogin_req_pkt), sendbuf);
}

/* Send a ban request. */
int shipgate_send_ban(shipgate_conn_t* c, uint16_t type, uint32_t requester,
    uint32_t target, uint32_t until, const char* msg) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_ban_req_pkt* pkt = (shipgate_ban_req_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Make sure we're requesting something valid. */
    switch (type) {
    case SHDR_TYPE_IPBAN:
    case SHDR_TYPE_GCBAN:
        break;

    default:
        return -1;
    }

    /* Fill in the data. */
    memset(pkt, 0, sizeof(shipgate_ban_req_pkt));

    pkt->hdr.pkt_len = htons(sizeof(shipgate_ban_req_pkt));
    pkt->hdr.pkt_type = htons(type);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->req_gc = htonl(requester);
    pkt->target = htonl(target);
    pkt->until = htonl(until);
    strncpy(pkt->message, msg, 255);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_ban_req_pkt), sendbuf);
}

/* Send a friendlist update */
int shipgate_send_friend_del(shipgate_conn_t* c, uint32_t user,
    uint32_t friend_gc) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_friend_upd_pkt* pkt = (shipgate_friend_upd_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_friend_upd_pkt));

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_friend_upd_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_DELFRIEND);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->user_guildcard = htonl(user);
    pkt->friend_guildcard = htonl(friend_gc);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_friend_upd_pkt), sendbuf);
}

int shipgate_send_friend_add(shipgate_conn_t* c, uint32_t user,
    uint32_t friend_gc, const char* nick) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_friend_add_pkt* pkt = (shipgate_friend_add_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_friend_add_pkt));

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_friend_add_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_ADDFRIEND);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->user_guildcard = htonl(user);
    pkt->friend_guildcard = htonl(friend_gc);
    strncpy(pkt->friend_nick, nick, 32);
    pkt->friend_nick[31] = 0;

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_friend_add_pkt), sendbuf);
}

/* Send a block login/logout */
int shipgate_send_block_login(shipgate_conn_t* c, int on, uint32_t gc,
    uint32_t block, const char* name) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_block_login_pkt* pkt = (shipgate_block_login_pkt*)sendbuf;
    uint16_t type = on ? SHDR_TYPE_BLKLOGIN : SHDR_TYPE_BLKLOGOUT;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_block_login_pkt));

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_block_login_pkt));
    pkt->hdr.pkt_type = htons(type);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->guildcard = htonl(gc);
    pkt->blocknum = htonl(block);
    strncpy(pkt->ch_name, name, 31);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_block_login_pkt), sendbuf);
}

int shipgate_send_block_login_bb(shipgate_conn_t* c, int on, uint32_t gc, uint8_t slot,
    uint32_t block, const uint16_t* name) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_block_login_pkt* pkt = (shipgate_block_login_pkt*)sendbuf;
    uint16_t type = on ? SHDR_TYPE_BLKLOGIN : SHDR_TYPE_BLKLOGOUT;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_block_login_pkt));

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_block_login_pkt));
    pkt->hdr.pkt_type = htons(type);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->guildcard = htonl(gc);
    pkt->slot = htonl(slot);
    pkt->blocknum = htonl(block);
    memcpy(pkt->ch_name, name, 31);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_block_login_pkt), sendbuf);
}

/* Send a lobby change packet */
int shipgate_send_lobby_chg(shipgate_conn_t* c, uint32_t user, uint32_t lobby,
    const char* lobby_name) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_lobby_change_pkt* pkt = (shipgate_lobby_change_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_lobby_change_pkt));

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_lobby_change_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_LOBBYCHG);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->guildcard = htonl(user);
    pkt->lobby_id = htonl(lobby);
    strncpy(pkt->lobby_name, lobby_name, 31);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_lobby_change_pkt), sendbuf);
}

/* Send a full client list */
int shipgate_send_clients(shipgate_conn_t* c) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_block_clients_pkt* pkt = (shipgate_block_clients_pkt*)sendbuf;
    uint32_t count;
    uint16_t size;
    ship_t* s = c->ship;
    uint32_t i;
    block_t* b;
    lobby_t* l;
    ship_client_t* cl;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* 循环遍历所有块以查找客户端，每个块发送一个数据包 */
    for (i = 0; i < s->cfg->blocks; ++i) {
        if (s->blocks[i]) {
            b = s->blocks[i];
            pthread_rwlock_rdlock(&b->lock);

            /* 设置此通行证 */
            pkt->block = htonl(b->b);
            size = 16;
            count = 0;

            TAILQ_FOREACH(cl, b->clients, qentry) {
                pthread_mutex_lock(&cl->mutex);

                /* Only do this if we have enough info to actually have sent
                   the block login before */
                if (cl->pl->v1.character.dress_data.guildcard_string[0]) {
                    l = cl->cur_lobby;

                    /* Fill in what we have */
                    pkt->entries[count].guildcard = htonl(cl->guildcard);
                    pkt->entries[count].dlobby = htonl(cl->lobby_id);

                    if (cl->version != CLIENT_VERSION_BB) {
                        strncpy(pkt->entries[count].ch_name, cl->pl->v1.character.dress_data.guildcard_string,
                            32);
                    }
                    else {
                        memcpy(pkt->entries[count].ch_name,
                            cl->bb_pl->character.name, BB_CHARACTER_NAME_LENGTH * 2);
                    }

                    if (l) {
                        pkt->entries[count].lobby = htonl(l->lobby_id);
                        memcpy(pkt->entries[count].lobby_name, l->name, 31);
                        pkt->entries[count].lobby_name[31] = 0;
                    }
                    else {
                        pkt->entries[count].lobby = htonl(0);
                        memset(pkt->entries[count].lobby_name, 0, 32);
                    }

                    /* Increment the counter/size */
                    ++count;
                    size += 80;
                }

                pthread_mutex_unlock(&cl->mutex);
            }

            pthread_rwlock_unlock(&b->lock);

            if (count) {
                /* 填充数据头 */
                pkt->hdr.pkt_len = htons(size);
                pkt->hdr.pkt_type = htons(SHDR_TYPE_BCLIENTS);
                pkt->hdr.version = pkt->hdr.reserved = 0;
                pkt->hdr.flags = 0;
                pkt->count = htonl(count);

                /* 将数据包发送出去 */
                send_crypt(c, size, sendbuf);
            }
        }
    }

    return 0;
}

/* Send a kick packet */
int shipgate_send_kick(shipgate_conn_t* c, uint32_t requester, uint32_t user,
    const char* reason) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_kick_pkt* pkt = (shipgate_kick_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_kick_pkt));

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_kick_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_KICK);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->requester = htonl(requester);
    pkt->guildcard = htonl(user);

    if (reason)
        strncpy(pkt->reason, reason, 63);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_kick_pkt), sendbuf);
}

/* Send a friend list request packet */
int shipgate_send_frlist_req(shipgate_conn_t* c, uint32_t gc, uint32_t block,
    uint32_t start) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_friend_list_req* pkt = (shipgate_friend_list_req*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_friend_list_req));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_FRLIST);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->requester = htonl(gc);
    pkt->block = htonl(block);
    pkt->start = htonl(start);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_friend_list_req), sendbuf);
}

/* Send a global message packet */
int shipgate_send_global_msg(shipgate_conn_t* c, uint32_t gc,
    const char* text) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_global_msg_pkt* pkt = (shipgate_global_msg_pkt*)sendbuf;
    size_t text_len = strlen(text) + 1, tl2 = (text_len + 7) & 0xFFF8;
    size_t len = tl2 + sizeof(shipgate_global_msg_pkt);

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Make sure its sane */
    if (text_len > 0x100 || !text) {
        return -1;
    }

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons((uint16_t)len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_GLOBALMSG);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->requester = htonl(gc);
    memcpy(pkt->text, text, text_len);
    memset(pkt->text + text_len, 0, tl2 - text_len);

    /* 将数据包发送出去 */
    return send_crypt(c, len, sendbuf);
}

/* Send a user option update packet */
int shipgate_send_user_opt(shipgate_conn_t* c, uint32_t gc, uint32_t block,
    uint32_t opt, uint32_t len, const uint8_t* data) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_user_opt_pkt* pkt = (shipgate_user_opt_pkt*)sendbuf;
    int padding = 8 - (len & 7);
    uint16_t pkt_len = len + sizeof(shipgate_user_opt_pkt) + 8 + padding;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Make sure its sane */
    if (!len || len >= 0x100 || !data) {
        return -1;
    }

    /* Fill in the option data first */
    pkt->options[0].option = htonl(opt);
    memcpy(pkt->options[0].data, data, len);

    /* Options must be a multiple of 8 bytes in length */
    while (padding--) {
        pkt->options[0].data[len++] = 0;
    }

    pkt->options[0].length = htonl(len + 8);

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(pkt_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_USEROPT);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);
    pkt->count = htonl(1);
    pkt->reserved = 0;

    /* 将数据包发送出去 */
    return send_crypt(c, pkt_len, sendbuf);
}

int shipgate_send_bb_opt_req(shipgate_conn_t* c, uint32_t gc, uint32_t block) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_bb_opts_req_pkt* pkt = (shipgate_bb_opts_req_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_bb_opts_req_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_BBOPT_REQ);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_bb_opts_req_pkt), sendbuf);
}

/* 发送玩家 Blue Burst 选项数据至数据库 */
int shipgate_send_bb_opts(shipgate_conn_t* c, ship_client_t* cl) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_bb_opts_pkt* pkt = (shipgate_bb_opts_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_bb_opts_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_BBOPTS);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->guildcard = htonl(cl->guildcard);
    pkt->block = htonl(cl->cur_block->b);
    memcpy(&pkt->opts, cl->bb_opts, sizeof(psocn_bb_db_opts_t));

    pkt->guild_id = cl->bb_guild->guild_data.guild_id;
    memcpy(&pkt->guild_info[0], &cl->bb_guild->guild_data.guild_info[0], sizeof(cl->bb_guild->guild_data.guild_info));
    pkt->guild_priv_level = cl->bb_guild->guild_data.guild_priv_level;
    memcpy(&pkt->guild_name[0], &cl->bb_guild->guild_data.guild_name[0], sizeof(cl->bb_guild->guild_data.guild_name));
    pkt->guild_rank = cl->bb_guild->guild_data.guild_rank;
    memcpy(&pkt->guild_flag[0], &cl->bb_guild->guild_data.guild_flag[0], sizeof(cl->bb_guild->guild_data.guild_flag));
    pkt->guild_rewards[0] = cl->bb_guild->guild_data.guild_rewards[0];
    pkt->guild_rewards[1] = cl->bb_guild->guild_data.guild_rewards[1];

    //memcpy(&pkt->guild, cl->bb_guild, sizeof(psocn_bb_db_guild_t));

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_bb_opts_pkt), sendbuf);
}

/* Send the shipgate a character data backup request. */
int shipgate_send_cbkup(shipgate_conn_t* c, sg_char_bkup_pkt* game_info, const void* cdata, int len) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_char_bkup_pkt* pkt = (shipgate_char_bkup_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* 填充数据头. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_char_bkup_pkt) + len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_CBKUP);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    /* Fill in the body. */
    pkt->game_info.guildcard = htonl(game_info->guildcard);
    pkt->game_info.slot = game_info->slot;
    pkt->game_info.block = htonl(game_info->block);
    pkt->game_info.c_version = htonl(game_info->c_version);
    strncpy((char*)pkt->game_info.name, game_info->name, sizeof(pkt->game_info.name));
    pkt->game_info.name[31] = 0;
    memcpy(pkt->data, cdata, len);

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_char_bkup_pkt) + len, sendbuf);
}

/* Send the shipgate a request for character backup data. */
int shipgate_send_cbkup_req(shipgate_conn_t* c, sg_char_bkup_pkt* game_info) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_char_bkup_pkt* pkt = (shipgate_char_bkup_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* 填充数据头 and the body. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_char_bkup_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_CBKUP);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    /* Fill in the body. */
    pkt->game_info.guildcard = htonl(game_info->guildcard);
    pkt->game_info.slot = game_info->slot;
    pkt->game_info.block = htonl(game_info->block);
    pkt->game_info.c_version = htonl(game_info->c_version);
    strncpy((char*)pkt->game_info.name, game_info->name, sizeof(pkt->game_info.name));
    pkt->game_info.name[31] = 0;

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_char_bkup_pkt), sendbuf);
}

/* Send a monster kill count update */
int shipgate_send_mkill(shipgate_conn_t* c, uint32_t gc, uint32_t block,
    ship_client_t* cl, lobby_t* l) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_mkill_pkt* pkt = (shipgate_mkill_pkt*)sendbuf;
    int i;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* 填充数据头 and the body. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_mkill_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_MKILL);
    pkt->hdr.version = 1;
    pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;
    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);
    pkt->episode = l->episode ? l->episode : 1;
    pkt->difficulty = l->difficulty;
    pkt->version = (uint8_t)cl->version;
    pkt->reserved = 0;

    if (l->battle)
        pkt->version |= CLIENT_BATTLE_MODE;

    if (l->challenge)
        pkt->version |= CLIENT_CHALLENGE_MODE;

    if (l->qid)
        pkt->version |= CLIENT_QUESTING;

    for (i = 0; i < 0x60; ++i) {
        pkt->counts[i] = ntohl(cl->enemy_kills[i]);
    }

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_mkill_pkt), sendbuf);
}

/* Send a script data packet */
int shipgate_send_sdata(shipgate_conn_t* c, ship_client_t* sc, uint32_t event,
    const uint8_t* data, uint32_t len) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_sdata_pkt* pkt = (shipgate_sdata_pkt*)sendbuf;
    uint16_t pkt_len;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* Make sure the length is sane... */
    if (len > 32768) {
        SHIPS_LOG("丢弃巨大的sdata数据包");
        return -1;
    }

    pkt_len = sizeof(shipgate_sdata_pkt) + len;
    if (pkt_len & 0x07)
        pkt_len = (pkt_len + 8) & 0xFFF8;

    /* 填充数据并准备发送.. */
    memset(pkt, 0, pkt_len);
    pkt->hdr.pkt_len = htons(pkt_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_SDATA);
    pkt->event_id = htonl(event);
    pkt->data_len = htonl(len);
    pkt->guildcard = htonl(sc->guildcard);
    pkt->block = htonl(sc->cur_block->b);

    if (sc->cur_lobby) {
        pkt->episode = sc->cur_lobby->episode;
        pkt->difficulty = sc->cur_lobby->difficulty;
    }

    pkt->version = sc->version;
    memcpy(pkt->data, data, len);

    /* 加密并发送. */
    return send_crypt(c, pkt_len, sendbuf);
}

/* Send a quest flag request or update */
int shipgate_send_qflag(shipgate_conn_t* c, ship_client_t* sc, int set,
    uint32_t fid, uint32_t qid, uint32_t value,
    uint32_t ctl) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_qflag_pkt* pkt = (shipgate_qflag_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf)
        return -1;

    /* 填充数据并准备发送.. */
    memset(pkt, 0, sizeof(shipgate_qflag_pkt));
    pkt->hdr.pkt_len = htons(sizeof(shipgate_qflag_pkt));
    if (set)
        pkt->hdr.pkt_type = htons(SHDR_TYPE_QFLAG_SET);
    else
        pkt->hdr.pkt_type = htons(SHDR_TYPE_QFLAG_GET);
    pkt->guildcard = htonl(sc->guildcard);
    pkt->block = htonl(sc->cur_block->b);
    pkt->flag_id = htonl((fid & 0xFFFF) | (ctl & 0xFFFF0000));
    pkt->quest_id = htonl(qid);
    pkt->flag_id_hi = htons(fid >> 16);
    pkt->value = htonl(value);

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_qflag_pkt), sendbuf);
}

/* 发送查询在线玩家请求 */
int shipgate_send_check_plonline_req(shipgate_conn_t* c, ship_client_t* cl) {
    uint8_t* sendbuf = get_sendbuf();
    shipgate_check_plonline_pkt* pkt = (shipgate_check_plonline_pkt*)sendbuf;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Fill in the packet */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_check_plonline_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_CHECK_PLONLINE);
    pkt->hdr.version = pkt->hdr.reserved = 0;
    pkt->hdr.flags = 0;

    pkt->req_gc = htonl(cl->guildcard);
    pkt->block = htonl(cl->cur_block->b);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_check_plonline_pkt), sendbuf);
}
