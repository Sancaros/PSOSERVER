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
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <errno.h>
#include <time.h>

#include <WinSock_Defines.h>
#include <psopipe.h>
#include <f_logs.h>
#include <f_iconv.h>
#include <psoconfig.h>
#include <SFMT.h>
#include <psomemory.h>
#include <pso_menu.h>

#include "block.h"
#include "ship.h"
#include "clients.h"
#include "lobby.h"
#include "ship_packets.h"
#include "utils.h"
#include "shipgate.h"
#include "commands.h"
#include "gm.h"
#include "subcmd.h"
#include "scripts.h"
#include "admin.h"
#include "smutdata.h"
#include "handle_player_items.h"
#include "records.h"

extern int enable_ipv6;
extern char ship_host4[32];
extern char ship_host6[128];
extern uint32_t ship_ip4;
extern uint8_t ship_ip6[16];
extern time_t srv_time;

/* 用于更新服务器的最新IP地址 */
static int update_addresses(psocn_ship_t* cfg) {
    struct addrinfo hints;
    struct addrinfo* server, * j;
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in* addr4;
    struct sockaddr_in6* addr6;

    /* Clear the addresses */
    ship_ip4 = 0;
    cfg->ship_ip4 = 0;
    memset(ship_ip6, 0, 16);
    memset(cfg->ship_ip6, 0, 16);

    SHIPS_LOG("更新舰船地址...");

    //CONFIG_LOG("检测域名获取: %s", host4);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(cfg->ship_host4, "9000", &hints, &server)) {
        ERR_LOG("无效舰船地址: %s", cfg->ship_host4);
        return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET) {
            addr4 = (struct sockaddr_in*)j->ai_addr;
            inet_ntop(j->ai_family, &addr4->sin_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv4 地址错误: %s", ipstr);
            //    return -1;
            //}
            //else
            //SHIPS_LOG("    获取到 IPv4 地址: %s", ipstr);
            cfg->ship_ip4 = ship_ip4 = addr4->sin_addr.s_addr;
        }
        else if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv6 地址错误: %s", ipstr);
            //    //return -1;
            //}
            //else
            //SHIPS_LOG("    获取到 IPv6 地址: %s", ipstr);
            memcpy(ship_ip6, &addr6->sin6_addr, 16);
            memcpy(cfg->ship_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    /* Make sure we found at least an IPv4 address */
    if (!ship_ip4 || !cfg->ship_ip4) {
        ERR_LOG("无法获取到IPv4地址!");
        return -1;
    }

    /* If we don't have a separate IPv6 host set, we're done. */
    if (!cfg->ship_host6) {
        return 0;
    }

    /* Now try with IPv6 only */
    memset(ship_ip6, 0, 16);
    memset(cfg->ship_ip6, 0, 16);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(cfg->ship_host6, "9000", &hints, &server)) {
        ERR_LOG("无效舰船地址 (v6): %s", cfg->ship_host6);
        //return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv6 地址错误: %s", ipstr);
            //    //return -1;
            //}
            //else
            //SHIPS_LOG("    获取到 IPv6 地址: %s", ipstr);
            memcpy(ship_ip6, &addr6->sin6_addr, 16);
            memcpy(cfg->ship_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    if (!ship_ip6[0]) {
        ERR_LOG("无法获取到IPv6地址 (但设置了IPv6域名)!");
        return -1;
    }

    return 0;
}

#pragma warning(push) 
#pragma warning(disable:4716)
static void* block_thd(void* d) {
    block_t* b = (block_t*)d;
    ship_t* s = b->ship;
    int nfds, i;
    struct timeval timeout = { 0 };
    fd_set readfds = { 0 }, writefds = { 0 }, exceptfds = { 0 };
    ship_client_t* it, * tmp;
    socklen_t len = { 0 };
    struct sockaddr_storage addr = { 0 };
    struct sockaddr* addr_p = (struct sockaddr*)&addr;
#ifdef DEBUG
    char ipstr[INET6_ADDRSTRLEN];
#endif // DEBUG
    //char nm[64];
    int sock;
    ssize_t sent;
    //time_t now;
    int numsocks = 1;
    int select_result = 0;

#ifdef PSOCN_ENABLE_IPV6
    if (enable_ipv6) {
        numsocks = 2;
    }
#endif

    //ERR_LOG("舰仓%s(%d): 已开启", s->cfg->name, b->b);

    /* While we're still supposed to run... do it. */
    while (b->run) {
        /* Clear the fd_sets so we can use them again. */
        nfds = 0;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        timeout.tv_sec = 15;
        timeout.tv_usec = 0;

        /* Ping pong?! */
        srv_time = time(NULL);

        /* Fill the sockets into the fd_sets so we can use select below. */
        pthread_rwlock_rdlock(&b->lock);

        TAILQ_FOREACH(it, b->clients, qentry) {
            /* If we haven't heard from a client in a minute and a half, it is
               probably dead. Disconnect it. */
            if (srv_time > it->last_message + 90) {
                if (it->pl) {
                    DC_LOG("Ping 超时: %s(%d:%d)", get_player_name(it->pl, it->version, false), 
                        it->guildcard, it->sec_data.slot);
                }

                it->flags |= CLIENT_FLAG_DISCONNECTED;

                /* Make sure that we disconnect the client ASAP! */
                timeout.tv_sec = 0;

                continue;
            }
            /* Otherwise, if we haven't heard from them in half of a minute,
               ping them. */
            else if (srv_time > it->last_message + 30 && srv_time > it->last_sent + 10) {
                if (send_simple(it, PING_TYPE, 0)) {
                    it->flags |= CLIENT_FLAG_DISCONNECTED;
                    timeout.tv_sec = 0;
                    continue;
                }

                it->last_sent = srv_time;
            }

            /* 默认间隔5秒存储一次数据 */
            if ((it->save_time + 5 < srv_time) && (it->need_save_data)) {
                client_send_bb_data(it);
                it->need_save_data = true;
            }

            /* Check if their timeout expired to login after getting a
               protection message. */
            if ((it->flags & CLIENT_FLAG_GC_PROTECT) &&
                it->join_time + 60 < srv_time) {
                it->flags |= CLIENT_FLAG_DISCONNECTED;
                timeout.tv_sec = 0;
                continue;
            }

            FD_SET(it->sock, &readfds);

            /* Only add to the write fd set if we have something to send out. */
            if (it->sendbuf_cur) {
                FD_SET(it->sock, &writefds);
            }

            nfds = max(nfds, it->sock);
        }

        pthread_rwlock_unlock(&b->lock);

        /* Add the listening sockets to the read fd_set. */
        for (i = 0; i < numsocks; ++i) {
            FD_SET(b->dcsock[i], &readfds);
            nfds = max(nfds, b->dcsock[i]);
            FD_SET(b->pcsock[i], &readfds);
            nfds = max(nfds, b->pcsock[i]);
            FD_SET(b->gcsock[i], &readfds);
            nfds = max(nfds, b->gcsock[i]);
            FD_SET(b->ep3sock[i], &readfds);
            nfds = max(nfds, b->ep3sock[i]);
            FD_SET(b->bbsock[i], &readfds);
            nfds = max(nfds, b->bbsock[i]);
            FD_SET(b->xbsock[i], &readfds);
            nfds = max(nfds, b->xbsock[i]);
        }

        FD_SET(b->pipes[1], &readfds);
        nfds = max(nfds, b->pipes[1]);

        /* Wait for some activity... */
        if ((select_result = select(nfds + 1, &readfds, &writefds, &exceptfds, &timeout)) > 0) {
            if (FD_ISSET(b->pipes[1], &readfds)) {
                recv(b->pipes[1], (char*)&len, 1, 0);
            }

            for (i = 0; i < numsocks; ++i) {
                if (FD_ISSET(b->dcsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(b->dcsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

#ifdef DEBUG
                    my_ntop(&addr, ipstr);
                    BLOCK_LOG("%s(%d): 允许 DC 舰仓连接 IP: "
                        "%s", s->cfg->name, b->b, ipstr);
#endif // DEBUG

                    if (!client_create_connection(sock, CLIENT_VERSION_DCV1,
                        CLIENT_TYPE_BLOCK, b->clients,
                        s, b, addr_p, len)) {
                        closesocket(sock);
                    }
                }

                if (FD_ISSET(b->pcsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(b->pcsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

#ifdef DEBUG
                    my_ntop(&addr, ipstr);
                    BLOCK_LOG("%s(%d): 允许 PC 舰仓连接 IP: "
                        "%s", s->cfg->name, b->b, ipstr);
#endif // DEBUG

                    if (!client_create_connection(sock, CLIENT_VERSION_PC,
                        CLIENT_TYPE_BLOCK, b->clients,
                        s, b, addr_p, len)) {
                        closesocket(sock);
                    }
                }

                if (FD_ISSET(b->gcsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(b->gcsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

#ifdef DEBUG
                    my_ntop(&addr, ipstr);
                    BLOCK_LOG("%s(%d): 允许 GameCube 舰仓连接 IP: "
                        "%s", s->cfg->name, b->b, ipstr);
#endif // DEBUG

                    if (!client_create_connection(sock, CLIENT_VERSION_GC,
                        CLIENT_TYPE_BLOCK, b->clients,
                        s, b, addr_p, len)) {
                        closesocket(sock);
                    }
                }

                if (FD_ISSET(b->ep3sock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(b->ep3sock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

#ifdef DEBUG
                    my_ntop(&addr, ipstr);
                    BLOCK_LOG("%s(%d): 允许 Episode 3 舰仓连接 IP: "
                        "%s", s->cfg->name, b->b, ipstr);
#endif // DEBUG

                    if (!client_create_connection(sock, CLIENT_VERSION_EP3,
                        CLIENT_TYPE_BLOCK, b->clients,
                        s, b, addr_p, len)) {
                        closesocket(sock);
                    }
                }

                if (FD_ISSET(b->bbsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(b->bbsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

#ifdef DEBUG
                    my_ntop(&addr, ipstr);
                    BLOCK_LOG("%s(%d): 允许 Blue Burst 舰仓连接 IP: "
                        "%s", s->cfg->name, b->b, ipstr);
#endif // DEBUG

                    if (!client_create_connection(sock, CLIENT_VERSION_BB,
                        CLIENT_TYPE_BLOCK, b->clients,
                        s, b, addr_p, len)) {
                        closesocket(sock);
                    }
                }

                if (FD_ISSET(b->xbsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(b->xbsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

#ifdef DEBUG
                    my_ntop(&addr, ipstr);
                    BLOCK_LOG("%s(%d): 允许 Xbox 舰仓连接 IP: "
                        "%s", s->cfg->name, b->b, ipstr);
#endif // DEBUG

                    if (!client_create_connection(sock, CLIENT_VERSION_XBOX,
                        CLIENT_TYPE_BLOCK, b->clients,
                        s, b, addr_p, len)) {
                        closesocket(sock);
                    }
                }
            }

            pthread_rwlock_rdlock(&b->lock);

            /* Process client connections. */
            TAILQ_FOREACH(it, b->clients, qentry) {
                pthread_mutex_lock(&it->mutex);

                /* Check if this connection was trying to send us something. */
                if (FD_ISSET(it->sock, &readfds)) {
                    if (client_process_pkt(it)) {
                        it->flags |= CLIENT_FLAG_DISCONNECTED;
                        pthread_mutex_unlock(&it->mutex);
                        continue;
                    }
                }

                /* Check if this connection was trying to send us something. */
                if (FD_ISSET(it->sock, &exceptfds)) {
                    it->flags |= CLIENT_FLAG_DISCONNECTED;
                    pthread_mutex_unlock(&it->mutex);
                    continue;
                }

                /* If we have anything to write, check if we can right now. */
                if (FD_ISSET(it->sock, &writefds)) {
                    if (it->sendbuf_cur) {
                        sent = send(it->sock, it->sendbuf + it->sendbuf_start,
                            it->sendbuf_cur - it->sendbuf_start, 0);

                        /*ERR_LOG(
                            "舰仓writefds端口 %d 发送数据 = %d 字节 版本识别 = %d",
                            it->sock, sent, it->version);*/

                            /* If we fail to send, and the error isn't EAGAIN,
                               bail. */
                        if (sent == SOCKET_ERROR) {
                            if (errno != EAGAIN) {
                                it->flags |= CLIENT_FLAG_DISCONNECTED;
                                pthread_mutex_unlock(&it->mutex);
                                continue;
                            }
                        }
                        else {
                            it->sendbuf_start += sent;

                            /* If we've sent everything, free the buffer. */
                            if (it->sendbuf_start == it->sendbuf_cur) {
                                free_safe(it->sendbuf);
                                it->sendbuf = NULL;
                                it->sendbuf_cur = 0;
                                it->sendbuf_size = 0;
                                it->sendbuf_start = 0;
                            }
                        }
                    }
                }

                pthread_mutex_unlock(&it->mutex);
            }

            pthread_rwlock_unlock(&b->lock);
        }
        else if (select_result == -1) {
            ERR_LOG("select 套接字 = -1");
        }

        /* Clean up any dead connections (its not safe to do a TAILQ_REMOVE
           in the middle of a TAILQ_FOREACH, and client_destroy_connection
           does indeed use TAILQ_REMOVE). */
        pthread_rwlock_wrlock(&b->lock);
        it = TAILQ_FIRST(b->clients);
        while (it) {
            tmp = TAILQ_NEXT(it, qentry);

            if (it->flags & CLIENT_FLAG_DISCONNECTED) {
                if (it->guildcard) {
                    char ipstr[INET6_ADDRSTRLEN];
                    my_ntop(&it->ip_addr, ipstr);
                    DC_LOG("%s(舰仓%02d[%02d]):%s(%d:%d:%s)断开(%s:%d)"
                        , ship->cfg->ship_name
                        , it->cur_block->b
                        , it->cur_block->num_clients
                        , get_player_name(it->pl, it->version, false)
                        , it->guildcard
                        , it->sec_data.slot
                        , client_type[it->version].ver_name
                        , ipstr
                        , it->sock
                    );
                    //DC_LOG("客户端 %s(%d:%d) 断开连接 %s", get_player_name(it->pl, it->version, false),
                    //    it->guildcard, it->sec_data.slot, client_type[it->version].ver_name);
                }
#ifdef DEBUG
                else {
                    my_ntop(&it->ip_addr, ipstr);
                    DC_LOG("切断未识别的连接 (IP: %s).",
                        ipstr);
                }
#endif // DEBUG

                /* Remove the player from the lobby before disconnecting
                   them, or else bad things might happen. */
                lobby_remove_player(it);
                client_destroy_connection(it, b->clients);
                --b->num_clients;
            }

            it = tmp;
        }

        pthread_rwlock_unlock(&b->lock);
    }

    pthread_exit(NULL);
}
#pragma warning(pop)

block_t* block_server_start(ship_t* s, int b, uint16_t port) {
    block_t* rv;
    int dcsock[2] = { -1, -1 }, pcsock[2] = { -1, -1 };
    int gcsock[2] = { -1, -1 }, ep3sock[2] = { -1, -1 };
    int bbsock[2] = { -1, -1 }, xbsock[2] = { -1, -1 }, i;
    lobby_t* l, * l2;
    uint32_t rng_seed;

    //BLOCK_LOG("%s: 开启舰仓 %d...", s->cfg->name, b);

    /* Create the sockets for listening for connections. */
    dcsock[0] = open_sock(PF_INET, port);
    if (dcsock[0] < 0) {
        return NULL;
    }

    /* Limit receive window size on DC versions to ensure that we don't
       mistakenly negotiate window scaling. */
    i = 32767;
    if (setsockopt(dcsock[0], SOL_SOCKET, SO_RCVBUF, (PCHAR)&i, sizeof(int)) < 0) {
        perror("setsockopt");
    }

    pcsock[0] = open_sock(PF_INET, port + 1);
    if (pcsock[0] < 0) {
        goto err_close_dc;
    }

    gcsock[0] = open_sock(PF_INET, port + 2);
    if (gcsock[0] < 0) {
        goto err_close_pc;
    }

    ep3sock[0] = open_sock(PF_INET, port + 3);
    if (ep3sock[0] < 0) {
        goto err_close_gc;
    }

    bbsock[0] = open_sock(PF_INET, port + 4);
    if (bbsock[0] < 0) {
        goto err_close_ep3;
    }

    xbsock[0] = open_sock(PF_INET, port + 5);
    if (xbsock[0] < 0) {
        goto err_close_bb;
    }

#ifdef PSOCN_ENABLE_IPV6
    if (enable_ipv6) {
        dcsock[1] = open_sock(PF_INET6, port);
        if (dcsock[1] < 0) {
            goto err_close_xb;
        }

        /* Limit receive window size on DC versions to ensure that we don't
           mistakenly negotiate window scaling. */
        i = 32767;
        if (setsockopt(dcsock[1], SOL_SOCKET, SO_RCVBUF, (PCHAR)&i, sizeof(int)) < 0) {
            perror("setsockopt");
        }

        pcsock[1] = open_sock(PF_INET6, port + 1);
        if (pcsock[1] < 0) {
            goto err_close_dc_6;
        }

        gcsock[1] = open_sock(PF_INET6, port + 2);
        if (gcsock[1] < 0) {
            goto err_close_pc_6;
        }

        ep3sock[1] = open_sock(PF_INET6, port + 3);
        if (ep3sock[1] < 0) {
            goto err_close_gc_6;
        }

        bbsock[1] = open_sock(PF_INET6, port + 4);
        if (bbsock[1] < 0) {
            goto err_close_ep3_6;
        }

        xbsock[1] = open_sock(PF_INET6, port + 5);
        if (xbsock[1] < 0) {
            goto err_close_bb_6;
        }
    }
#endif

    /* Make space for the block structure. */
    rv = (block_t*)malloc(sizeof(block_t));

    if (!rv) {
        ERR_LOG("%s(%d): 无法分配内存!", s->cfg->ship_name, b);
        goto err_close_all;
    }

    memset(rv, 0, sizeof(block_t));

    /* Make our pipe */
    if (pipe(rv->pipes) == -1) {
        ERR_LOG("%s(%d): 无法建立管道通信!", s->cfg->ship_name, b);
        goto err_free;
    }

    /* Make room for the client list. */
    rv->clients = (struct client_queue*)malloc(sizeof(struct client_queue));

    if (!rv->clients) {
        ERR_LOG("%s(%d): 无法为客户端分配内存!",
            s->cfg->ship_name, b);
        goto err_pipes;
    }

    /* Fill in the structure. */
    TAILQ_INIT(rv->clients);
    rv->ship = s;
    rv->b = b;
    rv->dc_port = port;
    rv->pc_port = port + 1;
    rv->gc_port = port + 2;
    rv->ep3_port = port + 3;
    rv->bb_port = port + 4;
    rv->xb_port = port + 5;
    rv->dcsock[0] = dcsock[0];
    rv->pcsock[0] = pcsock[0];
    rv->gcsock[0] = gcsock[0];
    rv->ep3sock[0] = ep3sock[0];
    rv->bbsock[0] = bbsock[0];
    rv->xbsock[0] = xbsock[0];
    rv->dcsock[1] = dcsock[1];
    rv->pcsock[1] = pcsock[1];
    rv->gcsock[1] = gcsock[1];
    rv->ep3sock[1] = ep3sock[1];
    rv->bbsock[1] = bbsock[1];
    rv->xbsock[1] = xbsock[1];
    rv->run = 1;

    TAILQ_INIT(&rv->lobbies);

    /* Create the first 20 lobbies (the default ones) */
    for (i = 1; i <= 20; ++i) {
        /* Grab a new lobby. XXXX: Check the return value. */
        l = lobby_create_default(rv, i, s->lobby_event);

        /* Add it into our list of lobbies */
        TAILQ_INSERT_TAIL(&rv->lobbies, l, qentry);
    }

    /* Create the reader-writer locks */
    pthread_rwlock_init(&rv->lock, NULL);
    pthread_rwlock_init(&rv->lobby_lock, NULL);

    /* Initialize the random number generator. The seed value is the current
       UNIX time, xored with the port (so that each block will use a different
       seed even though they'll probably get the same timestamp). */
    rng_seed = (uint32_t)(time(NULL) ^ port);
    //mt19937_init(&rv->rng, rng_seed);
    sfmt_init_gen_rand(&rv->sfmt_rng, rng_seed);

    /* Start up the thread for this block. */
    if (pthread_create(&rv->thd, NULL, &block_thd, rv)) {
        ERR_LOG("%s(%d): 无法开启舰仓线程!",
            s->cfg->ship_name, b);
        goto err_lobbies;
    }

    return rv;

err_lobbies:
    l2 = TAILQ_FIRST(&rv->lobbies);
    while (l2) {
        l = TAILQ_NEXT(l2, qentry);
        lobby_destroy(l2);
        l2 = l;
    }

    pthread_rwlock_destroy(&rv->lock);
    pthread_rwlock_destroy(&rv->lobby_lock);
    free_safe(rv->clients);
err_pipes:
    closesocket(rv->pipes[0]);
    closesocket(rv->pipes[1]);
err_free:
    free_safe(rv);
err_close_all:
#ifdef PSOCN_ENABLE_IPV6
    if (enable_ipv6) {
        closesocket(xbsock[1]);
    err_close_bb_6:
        closesocket(bbsock[1]);
    err_close_ep3_6:
        closesocket(ep3sock[1]);
    err_close_gc_6:
        closesocket(gcsock[1]);
    err_close_pc_6:
        closesocket(pcsock[1]);
    err_close_dc_6:
        closesocket(dcsock[1]);
    }
err_close_xb:
#endif
    closesocket(xbsock[0]);
err_close_bb:
    closesocket(bbsock[0]);
err_close_ep3:
    closesocket(ep3sock[0]);
err_close_gc:
    closesocket(gcsock[0]);
err_close_pc:
    closesocket(pcsock[0]);
err_close_dc:
    closesocket(dcsock[0]);

    return NULL;
}

void block_server_stop(block_t* b) {
    lobby_t* it2, * tmp2;
    ship_client_t* it, * tmp;

    /* Set the flag to kill the block. */
    b->run = 0;

    /* Send a byte to the pipe so that we actually break out of the select. */
    const char* data = "\xFF";
    send(b->pipes[0], data, strlen(data), 0);

    /* Wait for it to die. */
    pthread_join(b->thd, NULL);

    /* Close all the sockets so nobody can connect... */
    closesocket(b->pipes[0]);
    closesocket(b->pipes[1]);
    closesocket(b->dcsock[0]);
    closesocket(b->pcsock[0]);
    closesocket(b->gcsock[0]);
    closesocket(b->ep3sock[0]);
    closesocket(b->bbsock[0]);
    closesocket(b->xbsock[0]);
#ifdef PSOCN_ENABLE_IPV6
    if (enable_ipv6) {
        closesocket(b->dcsock[1]);
        closesocket(b->pcsock[1]);
        closesocket(b->gcsock[1]);
        closesocket(b->ep3sock[1]);
        closesocket(b->bbsock[1]);
        closesocket(b->xbsock[1]);
    }
#endif

    /* Disconnect any clients. */
    pthread_rwlock_wrlock(&b->lock);

    it = TAILQ_FIRST(b->clients);
    while (it) {
        tmp = TAILQ_NEXT(it, qentry);
        client_destroy_connection(it, b->clients);
        it = tmp;
    }

    pthread_rwlock_unlock(&b->lock);

    /* Destroy the lobbies that exist. */
    pthread_rwlock_wrlock(&b->lobby_lock);

    it2 = TAILQ_FIRST(&b->lobbies);
    while (it2) {
        tmp2 = TAILQ_NEXT(it2, qentry);
        lobby_destroy(it2);
        it2 = tmp2;
    }

    pthread_rwlock_unlock(&b->lobby_lock);

    /* Finish with our cleanup... */
    pthread_rwlock_destroy(&b->lobby_lock);
    pthread_rwlock_destroy(&b->lock);

    free_safe(b->clients);
    free_safe(b);
}

int block_info_reply(ship_client_t* c, uint32_t block) {
    block_t* b;
    char string[256];
    int players, games;

    /* Make sure the block selected is in range. */
    if ((int)block > ship->cfg->blocks) {
        return 0;
    }

    /* Make sure that block is up and running. */
    if (ship->blocks[block - 1] == NULL || ship->blocks[block - 1]->run == 0) {
        return 0;
    }

    //if ((int)block - 1 <= ship->cfg->blocks) {
        /* Grab the block in question */
    b = ship->blocks[block - 1];

    /* Grab the stats from the block structure */
    pthread_rwlock_rdlock(&b->lobby_lock);
    games = b->num_games;
    pthread_rwlock_unlock(&b->lobby_lock);

    pthread_rwlock_rdlock(&b->lock);
    players = b->num_clients;
    pthread_rwlock_unlock(&b->lock);

    /* Fill in the string. */
    snprintf(string, 256, "舰仓%02d\n%d %s\n%d %s", b->b, players,
        __(c, "玩家"), games, __(c, "房间"));
    // }

     /* Fill in the string. */
     //snprintf(string, 256, "NO MESSAGE");

     /* Send the information away. */
    return send_info_reply(c, string);
}

lobby_t* block_get_lobby(block_t* b, uint32_t lobby_id) {
    lobby_t* rv = NULL, * l;

    /* Look through all the lobbies in this block. */
    TAILQ_FOREACH(l, &b->lobbies, qentry) {
        pthread_mutex_lock(&l->mutex);

        if (l->lobby_id == lobby_id) {
            rv = l;
        }

        pthread_mutex_unlock(&l->mutex);

        if (rv) {
            break;
        }
    }

    return rv;
}

static int dc_join_game(ship_client_t* c, lobby_t* l) {
    int rv;

    /* Make sure they don't have the protection flag on */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7You must login\nbefore joining a\nteam."));
        return -1;
    }

    /* See if they can change lobbies... */
    rv = lobby_change_lobby(c, l);
    if (rv == LOBBY_FLAG_ERROR_GAME_V1_CLASS) {
        /* HUcaseal, FOmar, or RAmarl trying to join a v1 game */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Your class is\nnot allowed in a\n"
                "PSOv1 game."));
    }
    if (rv == LOBBY_FLAG_ERROR_SINGLEPLAYER) {
        /* Single player mode */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\nin single player\nmode."));
    }
    else if (rv == LOBBY_FLAG_ERROR_PC_ONLY) {
        /* PC only */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\nfor PSOPC only."));
    }
    else if (rv == LOBBY_FLAG_ERROR_V1_ONLY) {
        /* V1 only */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\nfor PSOv1 only."));
    }
    else if (rv == LOBBY_FLAG_ERROR_DC_ONLY) {
        /* DC only */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\nfor PSODC only."));
    }
    else if (rv == LOBBY_FLAG_ERROR_LEGIT_TEMP_UNAVAIL) {
        /* Temporarily unavailable */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\ntemporarily\nunavailable."));
    }
    else if (rv == LOBBY_FLAG_ERROR_LEGIT_MODE) {
        /* Legit check failed */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Game mode is set\nto legit and you\n"
                "failed the legit\ncheck!"));
    }
    else if (rv == LOBBY_FLAG_ERROR_QUESTSEL) {
        /* Quest selection in progress */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Quest selection\nis in progress"));
    }
    else if (rv == LOBBY_FLAG_ERROR_QUESTING) {
        /* Questing in progress */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7A quest is in\nprogress."));
    }
    else if (rv == LOBBY_FLAG_ERROR_DCV2_ONLY) {
        /* V1 client attempting to join a V2 only game */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7This game is for\nVersion 2 only."));
    }
    else if (rv == LOBBY_FLAG_ERROR_MAX_LEVEL) {
        /* Level is too high */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7你的等级\n太高了."));
    }
    else if (rv == LOBBY_FLAG_ERROR_MIN_LEVEL) {
        /* Level is too low */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7你的等级\n太低了."));
    }
    else if (rv == LOBBY_FLAG_ERROR_BURSTING) {
        /* A client is bursting. */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7玩家正在跃迁中"));
    }
    else if (rv == LOBBY_FLAG_ERROR_REMOVE_CLIENT) {
        /* The lobby has disappeared. */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7This game is\nnon-existant."));
    }
    else if (rv == LOBBY_FLAG_ERROR_ADD_CLIENT) {
        /* The lobby is full. */
        send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7游戏房间人数已满."));
    }
    else {
        /* Clear their legit mode flag... */
        if ((c->flags & CLIENT_FLAG_LEGIT)) {
            c->flags &= ~CLIENT_FLAG_LEGIT;
            if (c->limits)
                release(c->limits);
        }

        if (!c->game_data->err.has_error)
            regenerate_lobby_item_id(l, c);
    }

    c->game_info.guildcard = c->guildcard;
    c->game_info.slot = c->sec_data.slot;
    c->game_info.block = c->cur_block->b;
    c->game_info.c_version = c->version;

    strncpy((char*)c->game_info.name, c->pl->v1.character.dress_data.gc_string, sizeof(c->game_info.name));
    c->game_info.name[31] = 0;

    /* Try to backup their character data */
    if (c->version != CLIENT_VERSION_BB &&
        (c->flags & CLIENT_FLAG_AUTO_BACKUP)) {
        if (shipgate_send_cbkup(&ship->sg, &c->game_info, &c->pl->v1, 1052)) {
            /* XXXX: Should probably notify them... */
            return rv;
        }
    }

    return rv;
}

static int bb_join_game(ship_client_t* c, lobby_t* l) {
    int rv;

    /* Make sure they don't have the protection flag on */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7请先登录\n才可加入游戏."));
        return LOBBY_FLAG_ERROR_ADD_CLIENT;
    }

    item_class_tag_equip_flag(c);

    /* See if they can change lobbies... */
    rv = lobby_change_lobby(c, l);

    if (rv == LOBBY_FLAG_ERROR_GAME_V1_CLASS) {
        /* HUcaseal, FOmar, or RAmarl trying to join a v1 game */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7你的职业无法进入\n"
                "PSO V1 版本游戏."));
    }
    if (rv == LOBBY_FLAG_ERROR_SINGLEPLAYER/* && !c->reset_quest*/) {
        /* Single player mode */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7无法进入单人模式房间."));
    }
    else if (rv == LOBBY_FLAG_ERROR_PC_ONLY) {
        /* PC only */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7该房间仅限 PSOPC 版本进入."));
    }
    else if (rv == LOBBY_FLAG_ERROR_V1_ONLY) {
        /* V1 only */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7该房间仅限 PSOv1 版本进入."));
    }
    else if (rv == LOBBY_FLAG_ERROR_DC_ONLY) {
        /* DC only */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7该房间仅限 PSODC 版本进入."));
    }
    else if (rv == LOBBY_FLAG_ERROR_LEGIT_TEMP_UNAVAIL) {
        /* Temporarily unavailable */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\ntemporarily\nunavailable."));
    }
    else if (rv == LOBBY_FLAG_ERROR_LEGIT_MODE) {
        /* Legit check failed */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Game mode is set\nto legit and you\n"
                "failed the legit\ncheck!"));
    }
    else if (rv == LOBBY_FLAG_ERROR_QUESTSEL) {
        /* Quest selection in progress */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7该房间已在选择任务中."));
    }
    else if (rv == LOBBY_FLAG_ERROR_QUESTING) {
        /* Questing in progress */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7该房间已在任务中."));
    }
    else if (rv == LOBBY_FLAG_ERROR_DCV2_ONLY) {
        /* V1 client attempting to join a V2 only game */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7该房间仅限 DCV2 版本进入."));
    }
    else if (rv == LOBBY_FLAG_ERROR_MAX_LEVEL) {
        /* Level is too high */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7你的等级太高了."));
    }
    else if (rv == LOBBY_FLAG_ERROR_MIN_LEVEL) {
        /* Level is too high */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7你的等级太低了."));
    }
    else if (rv == LOBBY_FLAG_ERROR_BURSTING) {
        /* A client is bursting. */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7玩家正在跃迁中"));
    }
    else if (rv == LOBBY_FLAG_ERROR_REMOVE_CLIENT) {
        /* The lobby has disappeared. */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7游戏已不存在."));
    }
    else if (rv == LOBBY_FLAG_ERROR_ADD_CLIENT) {
        /* The lobby is full. */
        send_msg(c, MSG1_TYPE, "%s\n\n详情:%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7游戏房间人数已满."));
    }
    else {
        /* Clear their legit mode flag... */
        if ((c->flags & CLIENT_FLAG_LEGIT)) {
            c->flags &= ~CLIENT_FLAG_LEGIT;
            if (c->limits)
                release(c->limits);
        }

        if (!c->game_data->err.has_error)
            regenerate_lobby_item_id(l, c);
    }

    c->game_info.guildcard = c->guildcard;
    c->game_info.slot = c->sec_data.slot;
    c->game_info.block = c->cur_block->b;
    c->game_info.c_version = c->version;

    char tmp_name[32] = { 0 };
    istrncpy16_raw(ic_utf16_to_utf8, tmp_name, (char*)&c->pl->bb.character.name.char_name, 32, 10);

    strncpy((char*)c->game_info.name, tmp_name, sizeof(c->game_info.name));
    c->game_info.name[31] = 0;

    c->mode = 0;
    c->bank_type = false;
    c->game_data->expboost = 0;

    /* 备份临时数据 TODO BB版本未完成 */
    if (c->version == CLIENT_VERSION_BB) {
        if ((c->flags & CLIENT_FLAG_AUTO_BACKUP) || c->game_data->auto_backup) {
            //DBG_LOG("%s 自动备份中", get_player_describe(c));
            if (shipgate_send_cbkup(&ship->sg, &c->game_info, c->bb_pl, PSOCN_STLENGTH_BB_DB_CHAR)) {
                DBG_LOG("%s 备份临时数据 版本 %d", get_player_describe(c), c->version);
                return rv;
            }
        }
    }

    return 0;
}

int join_game(ship_client_t* c, lobby_t* l) {

    switch (c->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        return dc_join_game(c, l);

    case CLIENT_VERSION_BB:
        return bb_join_game(c, l);
    }

    return -1;
}

/* Process a login packet, sending security data, a lobby list, and a character
   data request. */
static int dcnte_process_login(ship_client_t* c, dcnte_login_8b_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;

    /* Make sure v1 is allowed on this ship. */
    if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NODCNTE)) {
        send_msg(c, MSG_BOX_TYPE, "%s", __(c, "\tEPSO NTE is not supported on\n"
            "this ship.\n\nDisconnecting."));
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    /* See if the user is banned */
    if (is_guildcard_banned(ship, c->guildcard, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }
    else if (is_ip_banned(ship, &c->ip_addr, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }

    /* Save what we care about in here. */
    c->guildcard = LE32(pkt->guildcard);
    //c->language_code = CLIENT_LANG_JAPANESE;
    //c->q_lang = CLIENT_LANG_JAPANESE;
    c->language_code = pkt->language;
    c->q_lang = 0; /* 初始化为默认任务语言 */
    //c->q_lang = pkt->language;
    c->flags |= CLIENT_FLAG_IS_NTE;

    /* See if this person is a GM. */
    c->privilege = is_gm(c->guildcard, ship);

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_lobby_list(c)) {
        return -2;
    }

    if (send_simple(c, DCNTE_CHAR_DATA_REQ_TYPE, 0)) {
        return -3;
    }

    return 0;
}

/* Process a login packet, sending security data, a lobby list, and a character
   data request. */
static int dc_process_login(ship_client_t* c, dc_login_93_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;
    uint16_t len;
    dc_login_93_meet_ext* ext;
    uint32_t menu;
    lobby_t* i;

    /* Make sure v1 is allowed on this ship. */
    if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOV1)) {
        send_msg(c, MSG_BOX_TYPE, "%s", __(c, "\tEPSO Version 1 is not supported on\n"
            "this ship.\n\nDisconnecting."));
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    /* See if the user is banned */
    if (is_guildcard_banned(ship, c->guildcard, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }
    else if (is_ip_banned(ship, &c->ip_addr, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }

    /* Save what we care about in here. */
    c->guildcard = LE32(pkt->guildcard);
    c->language_code = pkt->language_code;
    c->q_lang = 0; /* 初始化为默认任务语言 */
    //c->q_lang = pkt->language_code;

#ifdef DEBUG

    DBG_LOG("语言 %d %d", c->q_lang, c->language_code);

#endif // DEBUG

    c->flags |= CLIENT_FLAG_GC_MSG_BOXES;

    /* See if this person is a GM. */
    c->privilege = is_gm(c->guildcard, ship);

    /* Is this in response to a "Meet the User" from a guild card search? */
    len = LE16(pkt->hdr.pkt_len);
    if (len == 0x0114) {
        ext = (dc_login_93_meet_ext*)pkt->extra_data;
        menu = LE32(ext->lobby_menu);

        if (menu == MENU_ID_LOBBY) {
            menu = LE32(ext->lobby_id);

            TAILQ_FOREACH(i, &c->cur_block->lobbies, qentry) {
                if (i->lobby_id == menu && i->type == LOBBY_TYPE_LOBBY) {
                    c->lobby_req = i;
                    break;
                }
            }
        }
    }

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_lobby_list(c)) {
        return -2;
    }

    if (send_simple(c, CHAR_DATA_REQUEST_TYPE, 0)) {
        return -3;
    }

    return 0;
}

static int is_pctrial(dcv2_login_9d_pkt* pkt) {
    int i = 0;

    for (i = 0; i < 8; ++i) {
        if (pkt->serial_number[i] || pkt->access_key[i])
            return 0;
    }

    return 1;
}

/* Process a v2 login packet, sending security data, a lobby list, and a
   character data request. */
static int dcv2_process_login(ship_client_t* c, dcv2_login_9d_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;
    uint16_t len;
    dcv2_login_9d_meet_ext* extd;
    pc_login_9d_meet_ext* extp;
    uint32_t menu;
    lobby_t* i;

    /* Make sure the client's version is allowed on this ship. */
    if (c->version != CLIENT_VERSION_PC) {
        if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOV2)) {
            send_msg(c, MSG_BOX_TYPE, "%s", __(c, "\tEPSO Version 2 is not supported "
                "on\nthis ship.\n\nDisconnecting."));
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return 0;
        }
    }
    else {
        if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOPC)) {
            send_msg(c, MSG_BOX_TYPE, "%s", __(c, "\tEPSO for PC is not supported "
                "on\nthis ship.\n\nDisconnecting."));
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return 0;
        }

        /* Mark trial users as trial users. */
        if (is_pctrial(pkt)) {
            c->flags |= CLIENT_FLAG_IS_NTE;

            if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOPCNTE)) {
                send_msg(c, MSG_BOX_TYPE, "%s", __(c, "\tEPSO for PC Network Trial "
                    "Edition\nis not supported on "
                    "this ship.\n\nDisconnecting."));
                c->flags |= CLIENT_FLAG_DISCONNECTED;
                return 0;
            }
        }
    }

    /* See if the user is banned */
    if (is_guildcard_banned(ship, c->guildcard, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }
    else if (is_ip_banned(ship, &c->ip_addr, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }

    /* Save what we care about in here. */
    c->guildcard = LE32(pkt->guildcard);
    c->language_code = pkt->language_code;
    c->q_lang = 0; /* 初始化为默认任务语言 */

    if (c->version != CLIENT_VERSION_PC)
        c->version = CLIENT_VERSION_DCV2;

    /* See if this person is a GM. */
    c->privilege = is_gm(c->guildcard, ship);

    /* See if it looks like we're here because of a "meet the user". */
    len = LE16(pkt->hdr.dc.pkt_len);
    if (c->version == CLIENT_VERSION_DCV2 && len == 0x0130) {
        extd = (dcv2_login_9d_meet_ext*)pkt->extra_data;
        menu = LE32(extd->lobby_menu);

        if (menu == MENU_ID_LOBBY) {
            menu = LE32(extd->lobby_id);

            TAILQ_FOREACH(i, &c->cur_block->lobbies, qentry) {
                if (i->lobby_id == menu && i->type == LOBBY_TYPE_LOBBY) {
                    c->lobby_req = i;
                    break;
                }
            }
        }
    }
    else if (c->version == CLIENT_VERSION_PC && len == 0x0150) {
        extp = (pc_login_9d_meet_ext*)pkt->extra_data;
        menu = LE32(extp->lobby_menu);

        if (menu == MENU_ID_LOBBY) {
            menu = LE32(extp->lobby_id);

            TAILQ_FOREACH(i, &c->cur_block->lobbies, qentry) {
                if (i->lobby_id == menu && i->type == LOBBY_TYPE_LOBBY) {
                    c->lobby_req = i;
                    break;
                }
            }
        }
    }

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_lobby_list(c)) {
        return -2;
    }

    if (send_simple(c, CHAR_DATA_REQUEST_TYPE, 0)) {
        return -3;
    }

    return 0;
}

/* Process a GC login packet, sending security data, a lobby list, and a
   character data request. */
static int gc_process_login(ship_client_t* c, gc_login_9e_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;
    uint16_t len;
    gc_login_9e_meet_ext* ext;
    uint32_t menu;
    lobby_t* i;

    /* Make sure PSOGC is allowed on this ship. */
    if (c->version == CLIENT_VERSION_GC) {
        if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOEP12)) {
            send_msg(c, MSG_BOX_TYPE, "%s", __(c, "\tEPSO Episode 1 & 2 is not "
                "supported on\nthis ship.\n\n"
                "Disconnecting."));
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return 0;
        }
    }
    else {
        if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOEP3)) {
            send_msg(c, MSG_BOX_TYPE, "%s", __(c, "\tEPSO Episode 3 is not "
                "supported on\nthis ship.\n\n"
                "Disconnecting."));
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return 0;
        }
    }

    /* See if the user is banned */
    if (is_guildcard_banned(ship, c->guildcard, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }
    else if (is_ip_banned(ship, &c->ip_addr, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }

    /* Save what we care about in here. */
    c->guildcard = LE32(pkt->guildcard);
    c->language_code = pkt->language_code;
    c->q_lang = 0; /* 初始化为默认任务语言 */

    /* See if this user can get message boxes properly... */
    switch (pkt->version) {
    case 0x32: /* Episode 1 & 2 (Europe, 50hz) */
    case 0x33: /* Episode 1 & 2 (Europe, 60hz) */
    case 0x34: /* Episode 1 & 2 (Japan, v1.03) */
    case 0x35: /* Episode 1 & 2 (Japan, v1.04) */
    case 0x36: /* Episode 1 & 2 Plus (US) */
    case 0x39: /* Episode 1 & 2 Plus (Japan) */
        c->flags |= CLIENT_FLAG_GC_MSG_BOXES;
        break;
    }

    /* See if this person is a GM. */
    c->privilege = is_gm(c->guildcard, ship);

    /* See if it looks like we're here because of a "meet the user". */
    len = LE16(pkt->hdr.pkt_len);
    if (len == 0x0150) {
        ext = (gc_login_9e_meet_ext*)pkt->extra_data;
        menu = LE32(ext->lobby_menu);

        if (menu == MENU_ID_LOBBY) {
            menu = LE32(ext->lobby_id);

            TAILQ_FOREACH(i, &c->cur_block->lobbies, qentry) {
                if (i->lobby_id == menu && i->type == LOBBY_TYPE_LOBBY) {
                    c->lobby_req = i;
                    break;
                }
            }
        }
    }

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_lobby_list(c)) {
        return -2;
    }

    if (send_simple(c, CHAR_DATA_REQUEST_TYPE, 0)) {
        return -3;
    }

    return 0;
}

/* Process a Xbox login packet, sending security data, a lobby list, and a
   character data request. */
static int xb_process_login(ship_client_t* c, xb_login_9e_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;

    /* Make sure PSOX is allowed on this ship. */
    if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOPSOX)) {
        send_msg(c, MSG_BOX_TYPE, "%s", __(c, "\tEPSO Episode 1 & 2 for Xbox "
            "is not supported on\nthis ship.\n\n"
            "Disconnecting."));
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    /* See if the user is banned */
    if (is_guildcard_banned(ship, c->guildcard, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }
    else if (is_ip_banned(ship, &c->ip_addr, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }

    /* Save what we care about in here. */
    c->guildcard = LE32(pkt->guildcard);
    c->language_code = pkt->language_code;
    c->q_lang = 0; /* 初始化为默认任务语言 */
    //c->q_lang = pkt->language_code;

    /* See if this person is a GM. */
    c->privilege = is_gm(c->guildcard, ship);

    memcpy(c->xbl_ip, &pkt->xbl_ip, sizeof(xbox_ip_t));

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_lobby_list(c)) {
        return -2;
    }

    if (send_simple(c, CHAR_DATA_REQUEST_TYPE, 0)) {
        return -3;
    }

    return 0;
}

/* Process incoming character data, and add to a lobby, if the character isn't
   currently in a lobby. */
static int dc_process_char(ship_client_t* c, dc_char_data_pkt* pkt) {
    uint8_t type = pkt->hdr.dc.pkt_type;
    uint16_t len = LE16(pkt->hdr.dc.pkt_len);
    uint8_t version = pkt->hdr.dc.flags;
    uint32_t v;
    int i;

    /*DBG_LOG("%s(%d): DC处理角色数据 for GC %" PRIu32
        "", ship->cfg->name, c->cur_block->b,
        c->guildcard);*/

    pthread_mutex_lock(&c->mutex);

    /* If they already had character data, then check if it's still sane. */
    if (c->pl->v1.character.dress_data.gc_string[0]) {
        i = client_check_character(c, &pkt->data, version);
        if (i) {
            ERR_LOG("%s(%d): 角色数据检查失败 GC %" PRIu32
                " 错误码 %d", ship->cfg->ship_name, c->cur_block->b,
                c->guildcard, i);
            if (c->cur_lobby) {
                ERR_LOG("        房间: %s (类型: 难度:%d,对战模式:%d,挑战模式:%d,V2:%d)",
                    c->cur_lobby->name, c->cur_lobby->difficulty,
                    c->cur_lobby->battle, c->cur_lobby->challenge,
                    c->cur_lobby->v2);
            }
        }
    }

    /* Do some more sanity checking...
       XXXX: This should probably be more thorough and done as part of the
       client_check_character() function. */
    v = LE32(pkt->data.v1.character.disp.level + 1);
    if (v > MAX_PLAYER_LEVEL) {
        send_msg(c, MSG_BOX_TYPE, __(c, "\tEHacked characters are not allowed\n"
            "on this server.\n\n"
            "This will be reported to the server\n"
            "administration."));
        ERR_LOG("%s(%d): 检测到无效级别的角色!\n"
            "        GC %" PRIu32 ", 等级: %" PRIu32 "",
            c->guildcard, v);
        return -1;
    }

    /* Copy out the player data, and set up pointers. */
    if (version == 1) {
        memcpy(c->pl, &pkt->data, sizeof(v1_player_t));
        c->infoboard = NULL;
        //c->records = NULL;
    }
    else if (version == 2 && c->version == CLIENT_VERSION_DCV2) {
        memcpy(c->pl, &pkt->data, sizeof(v2_player_t));
        c->infoboard = NULL;
        c->records->v2 = c->pl->v2.records;
    }
    else if (version == 2 && c->version == CLIENT_VERSION_PC) {
        if (pkt->data.pc.autoreply[0]) {
            /* Copy in the autoreply */
            client_set_autoreply(c, pkt->data.pc.autoreply,
                len - 4 - sizeof(pc_player_t));
        }

        memcpy(c->pl, &pkt->data, sizeof(pc_player_t));
        c->infoboard = NULL;
        if (!&c->pl->pc.records) {
            c->records->pc = c->pl->pc.records;
            c->records->pc.challenge.title_color = encode_xrgb1555(c->pl->pc.records.challenge.title_color);
        }
        memcpy(c->blacklist, c->pl->pc.blacklist, 30 * sizeof(uint32_t));
    }
    else if (version == 3) {
        if (pkt->data.v3.autoreply[0]) {
            /* Copy in the autoreply */
            client_set_autoreply(c, pkt->data.v3.autoreply,
                len - 4 - sizeof(v3_player_t));
        }

        memcpy(c->pl, &pkt->data, sizeof(v3_player_t));
        c->infoboard = c->pl->v3.infoboard;
        if (!&c->pl->v3.records) {
            c->records->v3 = c->pl->v3.records;
            c->records->v3.challenge.title_color = encode_xrgb1555(c->pl->v3.records.challenge.title_color);
        }
        memcpy(c->blacklist, c->pl->v3.blacklist, 30 * sizeof(uint32_t));
    }
    else if (version == 4) {
        /* XXXX: Not right, but work with it for now. */
        memcpy(c->pl, &pkt->data, sizeof(v3_player_t));
        c->infoboard = c->pl->v3.infoboard;
        if (!&c->pl->v3.records) {
            c->records->v3 = c->pl->v3.records;
            c->records->v3.challenge.title_color = encode_xrgb1555(c->pl->v3.records.challenge.title_color);
        }
        memcpy(c->blacklist, c->pl->v3.blacklist, 30 * sizeof(uint32_t));
    }

    /* Copy out the inventory data */
    memcpy(c->iitems, c->pl->v1.character.inv.iitems, PSOCN_STLENGTH_IITEM * 30);
    c->item_count = (int)c->pl->v1.character.inv.item_count;

    /* Renumber the inventory data so we know what's going on later */
    for (i = 0; i < c->item_count; ++i) {
        v = 0x00210000 | i;
        c->iitems[i].data.item_id = LE32(v);
    }

    /* If this packet is coming after the client has left a game, then don't
       do anything else here, they'll take care of it by sending an 0x84. */
    if (type == LEAVE_GAME_PL_DATA_TYPE) {
        c->flags &= ~(CLIENT_FLAG_TRACK_INVENTORY | CLIENT_FLAG_LEGIT);
        pthread_mutex_unlock(&c->mutex);

        /* Remove the client from the lobby they're in, which will force the
           0x84 sent later to act like we're adding them to any lobby. */
        return lobby_remove_player(c);
    }

    /* If the client isn't in a lobby/team already, then add them to the first
       available lobby. */
    if (!c->cur_lobby) {
        if (lobby_add_to_any(c, c->lobby_req)) {
            pthread_mutex_unlock(&c->mutex);
            return -1;
        }

        if (send_lobby_join(c, c->cur_lobby)) {
            pthread_mutex_unlock(&c->mutex);
            return -2;
        }

        if (send_lobby_add_player(c->cur_lobby, c)) {
            pthread_mutex_unlock(&c->mutex);
            return -3;
        }

        /* Do a few things that should only be done once per session... */
        if (!(c->flags & CLIENT_FLAG_SENT_MOTD)) {
            script_execute(ScriptActionClientBlockLogin, c, SCRIPT_ARG_PTR,
                c, SCRIPT_ARG_END);

            /* Notify the shipgate */
            shipgate_send_block_login(&ship->sg, 1, c->guildcard,
                c->cur_block->b, c->pl->v1.character.dress_data.gc_string);

            if (c->cur_lobby)
                shipgate_send_lobby_chg(&ship->sg, c->guildcard,
                    c->cur_lobby->lobby_id, c->cur_lobby->name);
            else
                ERR_LOG(
                    "shipgate_send_lobby_chg 错误");

            /* Set up to send the Message of the Day if we have one and the
               client hasn't already gotten it this session.
               Disabled for some Gamecube versions, due to bugginess (of the
               game) and for the DC NTE. */
            if ((c->version == CLIENT_VERSION_DCV1 &&
                (c->flags & CLIENT_FLAG_IS_NTE)) ||
                ((c->version == CLIENT_VERSION_GC ||
                    c->version == CLIENT_VERSION_EP3 ||
                    c->version == CLIENT_VERSION_XBOX) &&
                    !(c->flags & CLIENT_FLAG_GC_MSG_BOXES))) {
                /* Just say that we sent the MOTD, even though we won't ever do
                   it at all. */
                c->flags |= CLIENT_FLAG_SENT_MOTD;
            }
        }
        else {
            if (c->cur_lobby)
                shipgate_send_lobby_chg(&ship->sg, c->guildcard,
                    c->cur_lobby->lobby_id, c->cur_lobby->name);
            else
                ERR_LOG(
                    "shipgate_send_lobby_chg 错误");
        }

        /* Send a ping so we know when they're done loading in. This is useful
           for sending the MOTD as well as enforcing always-legit mode. */
        send_simple(c, PING_TYPE, 0);
    }

    pthread_mutex_unlock(&c->mutex);

    if (c->cur_lobby) {
        /* Log the connection. */
        char ipstr[INET6_ADDRSTRLEN];
        my_ntop(&c->ip_addr, ipstr);
        DC_LOG("%s(舰仓%02d[%02d]):%s连接(%s:%d)"
            , ship->cfg->ship_name
            , c->cur_block->b
            , c->cur_block->num_clients
            , get_player_describe(c)
            , ipstr
            , c->sock
        );
    }

    return 0;
}

/* Process a change lobby packet. */
int process_change_lobby(ship_client_t* c, uint32_t item_id) {
    lobby_t* i, * req = NULL;
    int rv;

    /* Make sure they don't have the protection flag on */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4Can't change lobby!"),
            __(c, "\tC7You must login\nbefore changing\n"
                "lobbies."));
    }

    pthread_rwlock_rdlock(&c->cur_block->lobby_lock);

    TAILQ_FOREACH(i, &c->cur_block->lobbies, qentry) {
        if (i->lobby_id == item_id) {
            req = i;
            break;
        }
    }

    /* The requested lobby is non-existant? What to do... */
    if (req == NULL) {
        pthread_rwlock_unlock(&c->cur_block->lobby_lock);
        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法切换大厅!"),
            __(c, "\tC6大厅已不存在."));
    }

    rv = lobby_change_lobby(c, req);

    pthread_rwlock_unlock(&c->cur_block->lobby_lock);

    if (rv == -1) {
        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法切换大厅!"),
            __(c, "\tC4大厅人数已满."));
    }
    else if (rv < 0) {
        return send_msg(c, MSG1_TYPE, "%s\n\n%s %d.", __(c, "\tE\tC4无法切换大厅!"),
            __(c, "\tC8发生未知错误 错误码"), rv);
    }
    else {
        /* Send a ping so we know when they're done loading in. This is useful
           for enforcing always-legit mode. */
        send_simple(c, PING_TYPE, 0);
        return rv;
    }
}

static int dc_process_change_lobby(ship_client_t* c, dc_select_pkt* pkt) {
    uint32_t item_id = LE32(pkt->item_id);

    return process_change_lobby(c, item_id);
}

/* Process a chat packet. */
static int dc_process_chat(ship_client_t* c, dc_chat_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    size_t i;
    char* u8msg, * cmsg;
    size_t len;

    /* 合理性检查... this shouldn't happen. */
    if (!l)
        return -1;

    len = strlen(pkt->msg);

    /* Fill in escapes for the color chat stuff */
    if (c->cc_char) {
        for (i = 0; i < len; ++i) {
            /* Only accept it if it has a C right after, since that means we
               should have a color code... Also, make sure there's at least one
               character after the C, or we get junk... */
            if (pkt->msg[i] == c->cc_char && pkt->msg[i + 1] == 'C' &&
                pkt->msg[i + 2] != '\0') {
                pkt->msg[i] = '\t';
            }
        }
    }

    /* Convert it to UTF-8. */
    if (!(u8msg = (char*)malloc((len + 1) << 2)))
        return -1;

    memset(u8msg, 0, (len + 1) << 2);

    if ((pkt->msg[0] == '\t' && pkt->msg[1] == 'J') ||
        (c->flags & CLIENT_FLAG_IS_NTE)) {
        istrncpy(ic_sjis_to_gbk, u8msg, pkt->msg, (len + 1) << 2);
    }
    else {
        istrncpy(ic_8859_to_gbk, u8msg, pkt->msg, (len + 1) << 2);
    }

    /* Check for commands. */
    if (pkt->msg[2] == '/') {
        return command_parse(c, pkt);
    }

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Create a censored version. */
    cmsg = smutdata_censor_string(u8msg, SMUTDATA_BOTH);

    /* Send the message to the lobby. */
    i = send_lobby_chat(l, c, u8msg, cmsg);

    /* Clean up. */
    free_safe(cmsg);
    free_safe(u8msg);
    return i;
}

/* Process a chat packet from a PC client. */
static int pc_process_chat(ship_client_t* c, dc_chat_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    size_t len = LE16(pkt->hdr.dc.pkt_len) - 12;
    size_t i;
    char* u8msg, * cmsg;

    /* 合理性检查... this shouldn't happen. */
    if (!l)
        return -1;

    /* Fill in escapes for the color chat stuff */
    if (c->cc_char) {
        for (i = 0; i < len; i += 2) {
            /* Only accept it if it has a C right after, since that means we
               should have a color code... Also, make sure there's at least one
               character after the C, or we get junk..
               只有在它后面有一个C时才接受它，因为这意味着我们
               应该有颜色代码…而且，确保至少有一个
               C后面的字符，否则我们会得到垃圾。。. */
            if (pkt->msg[i] == c->cc_char && pkt->msg[i + 1] == '\0' &&
                pkt->msg[i + 2] == 'C' && pkt->msg[i + 3] == '\0' &&
                pkt->msg[i + 4] != '\0') {
                pkt->msg[i] = '\t';
            }
        }
    }

    /* Convert it to UTF-8. */
    if (!(u8msg = (char*)malloc((len + 1) << 1)))
        return -1;

    memset(u8msg, 0, (len + 1) << 1);

    istrncpy16(ic_utf16_to_gb18030, u8msg, (uint16_t*)pkt->msg, (len + 1) << 1);

    /* Check for commands. */
    if (pkt->msg[4] == '/') {
        return wcommand_parse(c, pkt);
    }

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    cmsg = smutdata_censor_string(u8msg, SMUTDATA_BOTH);

    /* Send the message to the lobby. */
    i = send_lobby_chat(l, c, u8msg, cmsg);

    /* Clean up. */
    free_safe(cmsg);
    free_safe(u8msg);
    return i;
}

/* Process a Guild Search request. */
static int dc_process_guild_search(ship_client_t* c, dc_guild_search_pkt* pkt) {
    uint32_t i;
    ship_client_t* it;
    uint32_t gc = LE32(pkt->gc_target);
    int done = 0, rv = -1;
    uint32_t flags = 0;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't allow guild searches for any reserved guild card numbers. */
    if (gc < 1000)
        return 0;

    /* 首先搜索本地的舰船. */
    for (i = 0; i < ship->cfg->blocks && !done; ++i) {
        if (!ship->blocks[i] || !ship->blocks[i]->run) {
            continue;
        }

        pthread_rwlock_rdlock(&ship->blocks[i]->lock);

        /* 查看该舰仓的所有客户端. */
        TAILQ_FOREACH(it, ship->blocks[i]->clients, qentry) {
            /* Check if this is the target and the target has player
               data. */
            if (it->guildcard == gc && it->pl) {
                pthread_mutex_lock(&it->mutex);
#ifdef PSOCN_ENABLE_IPV6
                if ((c->flags & CLIENT_FLAG_IPV6)) {
                    rv = send_guild_reply6(c, it);
                }
                else {
                    rv = send_guild_reply(c, it);
                }
#else
                rv = send_guild_reply(c, it);
#endif
                done = 1;
                pthread_mutex_unlock(&it->mutex);
            }
            else if (it->guildcard == gc) {
                /* If they're on but don't have data, we're not going to
                   find them anywhere else, return success. */
                rv = 0;
                done = 1;
            }

            if (done)
                break;
        }

        pthread_rwlock_unlock(&ship->blocks[i]->lock);
    }

    /* 如果我们到了这里, we didn't find it locally. Send to the shipgate to
       continue searching. */
    if (!done) {
#ifdef PSOCN_ENABLE_IPV6
        if ((c->flags & CLIENT_FLAG_IPV6)) {
            flags |= FW_FLAG_PREFER_IPV6;
        }
#endif

        return shipgate_fw_dc(&ship->sg, pkt, flags, c);
    }

    return rv;
}

static int dc_process_mail(ship_client_t* c, simple_mail_pkt* pkt) {
    uint32_t i;
    ship_client_t* it;
    uint32_t gc = LE32(pkt->dcmaildata.gc_dest);
    int done = 0, rv = -1;

    /* Don't send mail if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send mail for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    /* First check if this is to the bug report "character". */
    if (gc == BUG_REPORT_GC) {
        dc_bug_report(c, pkt);
        return 0;
    }

    /* Don't allow mails for any reserved guild card numbers other than the bug
       report one. */
    if (gc < 1000)
        return 0;

    /* 首先搜索本地的舰船. */
    for (i = 0; i < ship->cfg->blocks && !done; ++i) {
        if (!ship->blocks[i] || !ship->blocks[i]->run) {
            continue;
        }

        pthread_rwlock_rdlock(&ship->blocks[i]->lock);

        /* 查看该舰仓的所有客户端. */
        TAILQ_FOREACH(it, ship->blocks[i]->clients, qentry) {
            /* Check if this is the target and the target has player
               data. */
            if (it->guildcard == gc && it->pl) {
                pthread_mutex_lock(&it->mutex);

                /* Make sure the user hasn't blacklisted the sender. */
                if (client_has_blacklisted(it, c->guildcard) ||
                    client_has_ignored(it, c->guildcard)) {
                    done = 1;
                    pthread_mutex_unlock(&it->mutex);
                    rv = 0;
                    break;
                }

                /* Check if the user has an autoreply set. */
                if (it->autoreply_on) {
                    send_mail_autoreply(c, it);
                }

                /* Send the mail. */
                rv = send_simple_mail(c->version, it, (dc_pkt_hdr_t*)pkt);
                pthread_mutex_unlock(&it->mutex);
                done = 1;
                break;
            }
            else if (it->guildcard == gc) {
                /* If they're on but don't have data, we're not going to
                   find them anywhere else, return success. */
                rv = 0;
                done = 1;
                break;
            }
        }

        pthread_rwlock_unlock(&ship->blocks[i]->lock);
    }

    if (!done) {
        /* 如果我们到了这里, we didn't find it locally. Send to the shipgate to
           continue searching. */
        return shipgate_fw_dc(&ship->sg, pkt, 0, c);
    }

    return rv;
}

static int pc_process_mail(ship_client_t* c, simple_mail_pkt* pkt) {
    uint32_t i;
    ship_client_t* it;
    uint32_t gc = LE32(pkt->pcmaildata.gc_dest);
    int done = 0, rv = -1;

    /* Don't send mail if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send mail for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    /* First check if this is to the bug report "character". */
    if (gc == BUG_REPORT_GC) {
        pc_bug_report(c, pkt);
        return 0;
    }

    /* Don't allow mails for any reserved guild card numbers other than the bug
       report one. */
    if (gc < 1000)
        return 0;

    /* 首先搜索本地的舰船. */
    for (i = 0; i < ship->cfg->blocks && !done; ++i) {
        if (!ship->blocks[i] || !ship->blocks[i]->run) {
            continue;
        }

        pthread_rwlock_rdlock(&ship->blocks[i]->lock);

        /* 查看该舰仓的所有客户端. */
        TAILQ_FOREACH(it, ship->blocks[i]->clients, qentry) {
            /* Check if this is the target and the target has player
               data. */
            if (it->guildcard == gc && it->pl) {
                pthread_mutex_lock(&it->mutex);

                /* Make sure the user hasn't blacklisted the sender. */
                if (client_has_blacklisted(it, c->guildcard) ||
                    client_has_ignored(it, c->guildcard)) {
                    done = 1;
                    pthread_mutex_unlock(&it->mutex);
                    rv = 0;
                    break;
                }

                /* Check if the user has an autoreply set. */
                if (it->autoreply_on) {
                    send_mail_autoreply(c, it);
                }

                rv = send_simple_mail(c->version, it, (dc_pkt_hdr_t*)pkt);
                pthread_mutex_unlock(&it->mutex);
                done = 1;
                break;
            }
            else if (it->guildcard == gc) {
                /* If they're on but don't have data, we're not going to
                   find them anywhere else, return success. */
                rv = 0;
                done = 1;
                break;
            }
        }

        pthread_rwlock_unlock(&ship->blocks[i]->lock);
    }

    if (!done) {
        /* 如果我们到了这里, we didn't find it locally. Send to the shipgate to
           continue searching. */
        return shipgate_fw_pc(&ship->sg, pkt, 0, c);
    }

    return rv;
}

static int dcnte_process_game_create(ship_client_t* c,
    dcnte_game_create_pkt* pkt) {
    lobby_t* l;
    uint8_t event = ship->game_event;
    char name[32] = { 0 }, tmp[19] = { 0 };

    /* Convert the team name to UTF-8 */
    tmp[0] = '\t';
    tmp[1] = 'J';
    memcpy(tmp, pkt->name + 2, 16);
    tmp[18] = 0;

    istrncpy(ic_sjis_to_utf8, name, tmp, 32);

    /* Create the lobby structure. */
    l = lobby_create_game(c->cur_block, name, pkt->password,
        0, 0, 0, 0, c->version, get_player_section(c),
        event, 0, c, 0, 0);

    /* If we don't have a game, something went wrong... tell the user. */
    if (!l) {
        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4创建游戏失败!"),
            __(c, "\tC7请稍后重试."));
    }

    /* We've got a new game, but nobody's in it yet... Lets put the requester
       in the game. */
    if (dc_join_game(c, l)) {
        /* Something broke, destroy the created lobby before anyone tries to
           join it. */
        pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
        lobby_destroy(l);
        pthread_rwlock_unlock(&c->cur_block->lobby_lock);
    }

    /* All is good in the world. */
    return 0;
}

static int dc_process_game_create(ship_client_t* c, dc_game_create_pkt* pkt) {
    lobby_t* l;
    uint8_t event = ship->game_event;
    char name[32], tmp[17];

    /* Convert the team name to UTF-8 */
    memcpy(tmp, pkt->name, 16);
    tmp[16] = 0;

    if (pkt->name[1] == 'J') {
        istrncpy(ic_sjis_to_utf8, name, tmp, 32);
    }
    else {
        istrncpy(ic_8859_to_utf8, name, tmp, 32);
    }

    /* Check the user's ability to create a game of that difficulty. */
    if (!(c->flags & CLIENT_FLAG_OVERRIDE_GAME)) {
        if ((LE32((int)c->pl->v1.character.disp.level) + 1) < game_required_level[pkt->difficulty]) {
            return send_msg(c, MSG1_TYPE, "%s\n %s\n%s\n\n%s\n%s%d",
                __(c, "\tE房间名称: "),
                name,
                __(c, "\tC4创建失败!"),
                __(c, "\tC2您的等级太低."),
                __(c, "\tC2需求等级: Lv.\tC6"),
                game_required_level[pkt->difficulty]);
        }
    }

    /* Create the lobby structure. */
    l = lobby_create_game(c->cur_block, name, pkt->password,
        pkt->difficulty, pkt->battle, pkt->challenge,
        pkt->version, c->version, get_player_section(c),
        event, 0, c, 0, 0);

    /* If we don't have a game, something went wrong... tell the user. */
    if (!l) {
        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4创建游戏失败!"),
            __(c, "\tC7请稍后重试."));
    }

    /* We've got a new game, but nobody's in it yet... Lets put the requester
       in the game. */
    if (dc_join_game(c, l)) {
        /* Something broke, destroy the created lobby before anyone tries to
           join it. */
        pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
        lobby_destroy(l);
        pthread_rwlock_unlock(&c->cur_block->lobby_lock);
    }

    /* All is good in the world. */
    return 0;
}

static int pc_process_game_create(ship_client_t* c, pc_game_create_pkt* pkt) {
    lobby_t* l = NULL;
    uint8_t event = ship->game_event;
    char name[32], password[16];

    /* Convert the name/password to the appropriate encoding. */
    istrncpy16_raw(ic_utf16_to_gb18030, name, pkt->name, 32, 16);
    istrncpy16_raw(ic_utf16_to_ascii, password, pkt->password, 16, 16);

    /* Check the user's ability to create a game of that difficulty. */
    if (!(c->flags & CLIENT_FLAG_OVERRIDE_GAME)) {
        if ((LE32((int)c->pl->v1.character.disp.level) + 1) < game_required_level[pkt->difficulty]) {
            return send_msg(c, MSG1_TYPE, "%s\n %s\n%s\n\n%s\n%s%d",
                __(c, "\tE房间名称: "),
                name,
                __(c, "\tC4创建失败!"),
                __(c, "\tC2您的等级太低."),
                __(c, "\tC2需求等级: Lv.\tC6"),
                game_required_level[pkt->difficulty]);
        }
    }

    /* Create the lobby structure. */
    l = lobby_create_game(c->cur_block, name, password, pkt->difficulty,
        pkt->battle, pkt->challenge, 1, c->version,
        get_player_section(c), event, 0, c, 0, 0);

    /* If we don't have a game, something went wrong... tell the user. */
    if (!l) {
        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4创建游戏失败!"),
            __(c, "\tC7请稍后重试."));
    }

    /* If its a non-challenge, non-battle, non-ultimate game, ask the user if
       they want v1 compatibility or not. */
    if (!pkt->battle && !pkt->challenge && pkt->difficulty != 3 &&
        !(c->flags & CLIENT_FLAG_IS_NTE)) {
        c->create_lobby = l;
        return send_pc_game_type_sel(c);
    }

    /* We've got a new game, but nobody's in it yet... Lets put the requester
       in the game (as long as we're still here). */
    if (dc_join_game(c, l)) {
        /* Something broke, destroy the created lobby before anyone tries to
           join it. */
        pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
        lobby_destroy(l);
        pthread_rwlock_unlock(&c->cur_block->lobby_lock);
    }

    /* All is good in the world. */
    return 0;
}

static int gc_process_game_create(ship_client_t* c, gc_game_create_pkt* pkt) {
    lobby_t* l;
    uint8_t event = ship->game_event;
    char name[32], tmp[17];

    /* Convert the team name to UTF-8 */
    memcpy(tmp, pkt->name, 16);
    tmp[16] = 0;

    if (pkt->name[1] == 'J') {
        istrncpy(ic_sjis_to_utf8, name, tmp, 32);
    }
    else {
        istrncpy(ic_8859_to_utf8, name, tmp, 32);
    }

    /* Check the user's ability to create a game of that difficulty. */
    if (!(c->flags & CLIENT_FLAG_OVERRIDE_GAME)) {
        if ((LE32((int)c->pl->v1.character.disp.level) + 1) < game_required_level[pkt->difficulty]) {
            return send_msg(c, MSG1_TYPE, "%s\n %s\n%s\n\n%s\n%s%d",
                __(c, "\tE房间名称: "),
                name,
                __(c, "\tC4创建失败!"),
                __(c, "\tC2您的等级太低."),
                __(c, "\tC2需求等级: Lv.\tC6"),
                game_required_level[pkt->difficulty]);
        }
    }

    /* Create the lobby structure. */
    l = lobby_create_game(c->cur_block, name, pkt->password,
        pkt->difficulty, pkt->battle, pkt->challenge,
        0, c->version, get_player_section(c), event,
        pkt->episode, c, 0, 0);

    /* If we don't have a game, something went wrong... tell the user. */
    if (!l) {
        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4创建游戏失败!"),
            __(c, "\tC7请稍后重试."));
    }

    /* We've got a new game, but nobody's in it yet... Lets put the requester
       in the game. */
    if (dc_join_game(c, l)) {
        /* Something broke, destroy the created lobby before anyone tries to
           join it. */
        pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
        lobby_destroy(l);
        pthread_rwlock_unlock(&c->cur_block->lobby_lock);
    }

    /* All is good in the world. */
    return 0;
}

static int ep3_process_game_create(ship_client_t* c, ep3_game_create_pkt* pkt) {
    lobby_t* l;
    char name[32], tmp[17];

    /* Convert the team name to UTF-8 */
    memcpy(tmp, pkt->name, 16);
    tmp[16] = 0;

    if (pkt->name[1] == 'J') {
        istrncpy(ic_sjis_to_utf8, name, tmp, 32);
    }
    else {
        istrncpy(ic_8859_to_utf8, name, tmp, 32);
    }

    /* Create the lobby structure. */
    l = lobby_create_ep3_game(c->cur_block, name, pkt->password,
        pkt->view_battle, get_player_section(c), c);

    /* If we don't have a game, something went wrong... tell the user. */
    if (!l) {
        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4创建游戏失败!"),
            __(c, "\tC7请稍后重试."));
    }

    /* We've got a new game, but nobody's in it yet... Lets put the requester
       in the game. */
    if (dc_join_game(c, l)) {
        /* Something broke, destroy the created lobby before anyone tries to
           join it. */
        pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
        lobby_destroy(l);
        pthread_rwlock_unlock(&c->cur_block->lobby_lock);
    }

    /* All is good in the world. */
    return 0;
}

/* Process a client's done bursting signal. */
static int dc_process_done_burst(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;
    int rv;

    /* 合理性检查... Is the client in a game lobby? */
    if (!l || l->type == LOBBY_TYPE_LOBBY) {
        return -1;
    }

    /* Lock the lobby, clear its bursting flag, send the resume game packet to
       the rest of the lobby, and continue on. */
    pthread_mutex_lock(&l->mutex);

    /* Handle the end of burst stuff with the lobby */
    if (!(l->flags & LOBBY_FLAG_QUESTING)) {
        l->flags &= ~LOBBY_FLAG_BURSTING;
        c->flags &= ~CLIENT_FLAG_BURSTING;

        if (l->version == CLIENT_VERSION_BB) {
            send_lobby_end_burst(l);
        }

        rv = send_simple(c, PING_TYPE, 0) | lobby_handle_done_burst(l, c);
    }
    else {
        rv = send_quest_one(l, c, l->qid, l->qlang);
        c->flags |= CLIENT_FLAG_WAIT_QPING;
        rv |= send_simple(c, PING_TYPE, 0);
    }

    pthread_mutex_unlock(&l->mutex);

    return rv;
}

static int process_gm_menu(ship_client_t* c, uint32_t menu_id,
    uint32_t item_id) {
    if (!LOCAL_GM(c)) {
        return send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC7您的权限不足."));
    }

    switch (menu_id) {
    case MENU_ID_GM:
        switch (item_id) {
        case ITEM_ID_GM_REF_QUESTS:
            return refresh_quests(c, send_msg);

        case ITEM_ID_GM_REF_GMS:
            return refresh_gms(c, send_msg);

        case ITEM_ID_GM_REF_LIMITS:
            return refresh_limits(c, send_msg);

        case ITEM_ID_GM_RESTART:
        case ITEM_ID_GM_SHUTDOWN:
        case ITEM_ID_GM_GAME_EVENT:
        case ITEM_ID_GM_LOBBY_EVENT:
            return send_gm_menu(c, MENU_ID_GM | (item_id << 8));
        }

        break;

    case MENU_ID_GM_SHUTDOWN:
    case MENU_ID_GM_RESTART:
        return schedule_shutdown(c, item_id, menu_id == MENU_ID_GM_RESTART,
            send_msg);

    case MENU_ID_GM_GAME_EVENT:
        if (item_id < 7) {
            ship->game_event = item_id;
            return send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC7游戏节日设置成功."));
        }

        break;

    case MENU_ID_GM_LOBBY_EVENT:
        if (item_id < 15) {
            ship->lobby_event = item_id;
            update_lobby_event();
            return send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC7大厅节日设置成功."));
        }

        break;
    }

    return send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC4什么情况?"));
}

int process_menu(ship_client_t* c, uint32_t menu_id, uint32_t item_id,
    const uint8_t* passwd, uint16_t passwd_len) {

    //DBG_LOG("process_menu指令: 0x%08X item_id %d", menu_id & 0xFF, item_id);

    /* Figure out what the client is selecting. */
    switch (menu_id & 0xFF) {
        /* Lobby Information Desk */
    case MENU_ID_INFODESK:
        return send_info_file(c, ship, item_id);

        /* Blocks */
    case MENU_ID_BLOCK:
    {
        uint16_t port;

        /* See if it's the "Ship Select" entry */
        if (item_id == ITEM_ID_LAST) {
            return send_ship_list(c, ship, ship->cfg->menu_code);
        }

        /* Make sure the block selected is in range. */
        if ((int)item_id > ship->cfg->blocks) {
            return -1;
        }

        /* Make sure that block is up and running. */
        if (ship->blocks[item_id - 1] == NULL ||
            ship->blocks[item_id - 1]->run == 0) {
            return -2;
        }

        switch (c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            port = ship->blocks[item_id - 1]->dc_port;
            break;

        case CLIENT_VERSION_PC:
            port = ship->blocks[item_id - 1]->pc_port;
            break;

        case CLIENT_VERSION_GC:
            port = ship->blocks[item_id - 1]->gc_port;
            break;

        case CLIENT_VERSION_EP3:
            port = ship->blocks[item_id - 1]->ep3_port;
            break;

        case CLIENT_VERSION_BB:
            port = ship->blocks[item_id - 1]->bb_port;
            break;

        case CLIENT_VERSION_XBOX:
            port = ship->blocks[item_id - 1]->xb_port;
            break;

        default:
            return -1;
        }

        /* Redirect the client where we want them to go. */
#ifdef PSOCN_ENABLE_IPV6
        if (c->flags & CLIENT_FLAG_IPV6) {
            return send_redirect6(c, ship_host6, ship_ip6, port);
        }
        else {
            return send_redirect(c, ship_host4, ship_ip4, port);
        }
#else
        return send_redirect(c, ship_host4, ship_ip4, port);
#endif
    }

    /* Game Selection */
    case MENU_ID_GAME:
    {
        char passwd_cmp[17];
        lobby_t* l;
        int override = c->flags & CLIENT_FLAG_OVERRIDE_GAME;

        memset(passwd_cmp, 0, 17);

        /* Read the password, if the client provided one. */
        if (c->version == CLIENT_VERSION_PC ||
            c->version == CLIENT_VERSION_BB) {
            char tmp[32];

            if (passwd_len > 0x20) {
                return -1;
            }

            memset(tmp, 0, 32);
            memcpy(tmp, passwd, passwd_len);
            istrncpy16(ic_utf16_to_ascii, passwd_cmp, (uint16_t*)tmp, 16);
            passwd_cmp[16] = 0x00;
        }
        else {
            if (passwd_len > 0x10) {
                return -1;
            }

            memcpy(passwd_cmp, passwd, passwd_len);
        }

        /* The client is selecting a game to join. */
        l = block_get_lobby(c->cur_block, item_id);

        if (!l) {
            /* The lobby has disappeared. */
            send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
                __(c, "\tC7This game is\nnon-existant."));
            return 0;
        }

        /* Check the provided password (if any). */
        if (!override) {
            if (l->passwd[0] && strcmp(passwd_cmp, l->passwd)) {
                send_msg(c, MSG1_TYPE, "%s\n\n%s",
                    __(c, "\tE\tC4无法加入游戏!"),
                    __(c, "\tC7密码错误."));
                return 0;
            }
        }

        /* Attempt to change the player's lobby. */
        dc_join_game(c, l);

        return 0;
    }

    /* Quest category */
    case MENU_ID_QCATEGORY:
    {
        int rv;
        int lang;

        pthread_rwlock_rdlock(&ship->qlock);

        /* Do we have quests configured? */
        if (!TAILQ_EMPTY(&ship->qmap)) {
            lang = (menu_id >> 24) & 0xFF;
            rv = send_quest_list(c, (int)item_id, lang);
        }
        else {
            rv = send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC4未读取任务."));
        }

        pthread_rwlock_unlock(&ship->qlock);
        return rv;
    }

    /* Quest */
    case MENU_ID_QUEST:
    {
        int lang = (menu_id >> 24) & 0xFF;
        lobby_t* l = c->cur_lobby;

        if (l->flags & LOBBY_FLAG_BURSTING) {
            return send_msg(c, MSG1_TYPE, "%s",
                __(c, "\tE\tC4请等待一会后操作."));
        }

        return lobby_setup_quest(l, c, item_id, lang);
    }

    /* Ship */
    case MENU_ID_SHIP:
    {
        miniship_t* i;
        int off = 0;

        /* See if the user picked a Ship List item */
        if (item_id == 0) {
            return send_ship_list(c, ship, (uint16_t)(menu_id >> 8));
        }

        switch (c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            off = 0;
            break;

        case CLIENT_VERSION_PC:
            off = 1;
            break;

        case CLIENT_VERSION_GC:
            off = 2;
            break;

        case CLIENT_VERSION_EP3:
            off = 3;
            break;

        case CLIENT_VERSION_BB:
            off = 4;
            break;

        case CLIENT_VERSION_XBOX:
            off = 5;
            break;
        }

        /* Go through all the ships that we know about looking for the one
           that the user has requested. */
        TAILQ_FOREACH(i, &ship->ships, qentry) {
            if (i->ship_id == item_id) {
#ifdef PSOCN_ENABLE_IPV6
                if (c->flags & CLIENT_FLAG_IPV6 && i->ship_addr6[0]) {
                    return send_redirect6(c, i->ship_host6, i->ship_addr6,
                        i->ship_port + off);
                }
                else {
                    return send_redirect(c, i->ship_host4, i->ship_addr,
                        i->ship_port + off);
                }
#else
                return send_redirect(c, i->ship_host4, i->ship_addr, i->ship_port + off);
#endif
            }
        }

        /* We didn't find it, punt. */
        return send_msg(c, MSG1_TYPE, "%s",
            __(c, "\tE\tC4当前选择的舰船\n已离线."));
    }

    /* Game type (PSOPC only) */
    case MENU_ID_GAME_TYPE:
    {
        lobby_t* l = c->create_lobby;

        if (l) {
            if (item_id == 0) {
                l->v2 = 0;
                l->version = CLIENT_VERSION_DCV1;
            }
            else if (item_id == 2) {
                l->flags |= LOBBY_FLAG_PCONLY;
            }

            /* Add the lobby to the list of lobbies on the block. */
            pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
            TAILQ_INSERT_TAIL(&c->cur_block->lobbies, l, qentry);
            ship_inc_games(ship);
            ++c->cur_block->num_games;
            pthread_rwlock_unlock(&c->cur_block->lobby_lock);
            c->create_lobby = NULL;

            /* Add the user to the lobby... */
            if (join_game(c, l)) {
                /* Something broke, destroy the created lobby before anyone
                   tries to join it. */
                pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
                lobby_destroy(l);
                pthread_rwlock_unlock(&c->cur_block->lobby_lock);
            }

            /* All's well in the world if we get here. */
            return 0;
        }

        return send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC4Huh?"));
    }

    /* GM Menu */
    case MENU_ID_GM:
        return process_gm_menu(c, menu_id, item_id);

    default:
        printf("menu_id & 0xFF = %u", menu_id & 0xFF);
        if (script_execute(ScriptActionUnknownMenu, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_UINT32, menu_id, SCRIPT_ARG_UINT32,
            item_id, SCRIPT_ARG_END) > 0)
            return 0;
    }

    return -1;
}

static int dc_process_menu(ship_client_t* c, dc_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);
    uint8_t* bp = (uint8_t*)pkt + 0x0C;
    uint16_t len = LE16(pkt->hdr.dc.pkt_len) - 0x0C;

    return process_menu(c, menu_id, item_id, bp, len);
}

static int process_info_req(ship_client_t* c, uint32_t menu_id,
    uint32_t item_id) {
    /* What kind of information do they want? */
    switch (menu_id & 0xFF) {
        /* Block */
    case MENU_ID_BLOCK:
        if (item_id == -1)
            return 0;

        return block_info_reply(c, item_id);

        /* Game List */
    case MENU_ID_GAME:
        return lobby_info_reply(c, item_id);

        /* Quest */
    case MENU_ID_QUEST:
    {
        int lang = (menu_id >> 24) & 0xFF;
        int rv;

        pthread_rwlock_rdlock(&ship->qlock);

        /* Do we have quests configured? */
        if (!TAILQ_EMPTY(&ship->qmap)) {
            rv = send_quest_info(c->cur_lobby, item_id, lang);
        }
        else {
            rv = send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC4任务未加载."));
        }

        pthread_rwlock_unlock(&ship->qlock);
        return rv;
    }

    /* Ship */
    case MENU_ID_SHIP:
    {
        miniship_t* i;

        /* Find the ship if its still online */
        TAILQ_FOREACH(i, &ship->ships, qentry) {
            if (i->ship_id == item_id) {
                char string[256];
                char tmp[3] = { (char)i->menu_code,
                    (char)(i->menu_code >> 8), 0 };

                sprintf(string, "%02x:%s%s%s\n%d %s\n%d %s", i->ship_number,
                    tmp, tmp[0] ? "/" : "", i->name, i->clients,
                    __(c, "玩家"), i->games, __(c, "房间"));
                return send_info_reply(c, string);
            }
        }

        return 0;
    }

    default:
        return -1;
    }
}

static int dc_process_info_req(ship_client_t* c, dc_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    return process_info_req(c, menu_id, item_id);
}

/* Process a client's arrow update request. */
static int dc_process_arrow(ship_client_t* c, uint8_t flag) {
    c->arrow_color = flag;
    return send_lobby_arrows(c->cur_lobby);
}

/* Process a client's trade request. */
static int process_trade(ship_client_t* c, gc_trade_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t target_client_id = pkt->who;
    ship_client_t* dest;

    if (!l || l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %u : %d 不在游戏或房间中", c->guildcard, c->sec_data.slot);
        return 0;
    }

    /* 搜索目标客户端. */
    dest = ge_target_client_by_id(l, target_client_id);

    if (dest == NULL) {
        ERR_LOG("GC %" PRIu32 " 尝试与不存在的玩家交易!",
            c->guildcard);
        return send_msg(c, MSG1_TYPE, "%s",
            __(c, "\tE未找到需要交易的玩家."));
    }

    send_simple(dest, TRADE_1_TYPE, 0);
    pkt->hdr.pkt_type = TRADE_3_TYPE;
    send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
    return send_simple(dest, TRADE_4_TYPE, 1);
}

/* Process a blacklist update packet. */
static int gc_process_blacklist(ship_client_t* c,
    gc_blacklist_update_pkt* pkt) {
    memcpy(c->blacklist, pkt->list, 30 * sizeof(uint32_t));
    return send_txt(c, "%s", __(c, "\tE\tC7Updated blacklist."));
}

/* Process an infoboard update packet. */
static int process_infoboard(ship_client_t* c, gc_write_info_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.dc.pkt_len) - 4;

    if (!c->infoboard || len > 0xAC) {
        return -1;
    }

    memcpy(c->infoboard, pkt->msg, len);
    memset(c->infoboard + len, 0, 0xAC - len);
    return 0;
}

/* Process a 0xAA packet. This command is used in Maximum Attack 2 */
static int process_dc_update_quest_stats(ship_client_t* c,
    dc_update_quest_stats_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);
    lobby_t* l = c->cur_lobby;

    print_ascii_hex(dbgl, pkt, len);

    if (!l || !(l->flags & LOBBY_FLAG_QUESTING))
        return -1;

    if (c->flags & 0x00002000)
        ERR_LOG("trial edition client sent update quest stats command.");

    DBG_LOG("process_dc_update_quest_stats qid %d  quest_internal_id %d", l->qid, pkt->quest_internal_id);

    if (l->qid != pkt->quest_internal_id) {
        ERR_LOG("l->qid != pkt->quest_internal_id.");
        return -1;
    }

    return send_dc_confirm_update_quest_statistics(c, pkt->hdr.flags, pkt->function_id1);
}

/* Process a 0xBA packet. */
static int process_ep3_command(ship_client_t* c, uint8_t* pkt) {
    dc_pkt_hdr_t* hdr = (dc_pkt_hdr_t*)pkt;
    uint16_t len = LE16(hdr->pkt_len);
    uint16_t tmp;

    switch (hdr->flags) {
    case EP3_COMMAND_LEAVE_TEAM:
        /* Make sure the size looks ok... */
        if (len != 0x10)
            return -1;

        tmp = pkt[0x0E] | (pkt[0x0F] << 8);
        return send_ep3_ba01(c, tmp);

    case EP3_COMMAND_JUKEBOX_REQUEST:
        /* Make sure the size looks ok... */
        if (len != 0x10)
            return -1;

        tmp = pkt[0x0E] | (pkt[0x0F] << 8);
        return send_ep3_jukebox_reply(c, tmp);

    default:
        if (!script_execute_pkt(ScriptActionUnknownEp3Packet, c, pkt, len)) {
            DBG_LOG("GC %u 未知 Episode 3 指令: 0x%04X", c->guildcard, hdr->flags);
            print_ascii_hex(dbgl, pkt, len);
            return -1;
        }
        return 0;
    }
}

/* Process a 0xCA packet. */
static int on_CA_Ep3(ship_client_t* c, uint8_t* pkt) {
    lobby_t* l = c->cur_lobby;
    dc_pkt_hdr_t* hdr = (dc_pkt_hdr_t*)pkt;
    uint16_t len = LE16(hdr->pkt_len);
    uint16_t tmp = 0;

    DBG_LOG("Ep3 服务器数据来自 %s (%d)", c->pl->v1.character.dress_data.gc_string,
        c->guildcard);
    print_ascii_hex(dbgl, pkt, len);
//[2023年09月24日 16:04:38:035] 调试(block.c 2823): Ep3 服务器数据来自 Sancaros (10000000)
//[2023年09月24日 16:04:38:037] 调试(f_logs.c 0088): 数据包如下:
//(00000000) CA 00 14 00 B3 04 42 01  40 00 00 10 FF FF FF FF    ......B.@.......
//(00000010) 00 00 00 00    ....

    return 0;
    //try {
    //    l = s->find_lobby(c->lobby_id);
    //}
    //catch (const out_of_range&) {
    //    // In rare cases (e.g. when two players end a tournament's match results
    //    // screens at exactly the same time), the client can send a server data
    //    // command when it's not in any lobby at all. We just ignore such commands.
    //    return;
    //}
    //if (!l->is_game() || !l->is_ep3()) {
    //    throw runtime_error("Episode 3 server data request sent outside of Episode 3 game");
    //}

    //const auto& header = check_size_t<G_CardServerDataCommandHeader>(data, 0xFFFF);
    //if (header.subcommand != 0xB3) {
    //    throw runtime_error("unknown Episode 3 server data request");
    //}

    //if (!l->ep3_server_base || l->ep3_server_base->server->battle_finished) {
    //    if (!l->ep3_server_base) {
    //        l->log.info("Creating Episode 3 server state");
    //    }
    //    else {
    //        l->log.info("Recreating Episode 3 server state");
    //    }
    //    auto tourn = l->tournament_match ? l->tournament_match->tournament.lock() : nullptr;
    //    bool is_trial = (l->flags & Lobby::Flag::IS_EP3_TRIAL);
    //    l->ep3_server_base = make_shared<Episode3::ServerBase>(
    //        l,
    //        is_trial ? s->ep3_card_index_trial : s->ep3_card_index,
    //        s->ep3_map_index,
    //        s->ep3_behavior_flags,
    //        l->random_crypt,
    //        tourn ? tourn->get_map() : nullptr);
    //    l->ep3_server_base->init();

    //    if (s->ep3_behavior_flags & Episode3::BehaviorFlag::ENABLE_STATUS_MESSAGES) {
    //        for (size_t z = 0; z < l->max_clients; z++) {
    //            if (l->clients[z]) {
    //                send_text_message_printf(l->clients[z], "Your client ID: $C6%zu", z);
    //            }
    //        }
    //    }

    //    if (s->ep3_behavior_flags & Episode3::BehaviorFlag::ENABLE_RECORDING) {
    //        if (l->battle_record) {
    //            l->prev_battle_record = l->battle_record;
    //            l->prev_battle_record->set_battle_end_timestamp();
    //        }
    //        l->battle_record.reset(new Episode3::BattleRecord(s->ep3_behavior_flags));
    //        for (auto existing_c : l->clients) {
    //            if (existing_c) {
    //                PlayerLobbyDataDCGC lobby_data;
    //                lobby_data.name = encode_sjis(existing_c->game_data.player()->disp.name);
    //                lobby_data.player_tag = 0x00010000;
    //                lobby_data.guild_card = existing_c->license->serial_number;
    //                l->battle_record->add_player(lobby_data,
    //                    existing_c->game_data.player()->inventory,
    //                    existing_c->game_data.player()->disp.to_dcpcv3());
    //            }
    //        }
    //        if (l->prev_battle_record) {
    //            send_text_message(l, u"$C6Recording complete");
    //        }
    //        send_text_message(l, u"$C6Recording enabled");
    //    }
    //}
    //l->ep3_server_base->server->on_server_data_input(data);
    //if (l->tournament_match &&
    //    l->ep3_server_base->server->setup_phase == Episode3::SetupPhase::BATTLE_ENDED &&
    //    !l->ep3_server_base->server->tournament_match_result_sent) {
    //    int8_t winner_team_id = l->ep3_server_base->server->get_winner_team_id();
    //    if (winner_team_id == -1) {
    //        throw runtime_error("match complete, but winner team not specified");
    //    }

    //    auto tourn = l->tournament_match->tournament.lock();
    //    tourn->print_bracket(stderr);

    //    if (winner_team_id == 0) {
    //        l->tournament_match->set_winner_team(l->tournament_match->preceding_a->winner_team);
    //    }
    //    else if (winner_team_id == 1) {
    //        l->tournament_match->set_winner_team(l->tournament_match->preceding_b->winner_team);
    //    }
    //    else {
    //        throw logic_error("invalid winner team id");
    //    }
    //    send_ep3_tournament_match_result(s, l, l->tournament_match);

    //    on_tournament_bracket_updated(s, tourn);
    //    l->ep3_server_base->server->tournament_match_result_sent = true;
    //}
}

static int process_qload_done(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* c2;
    int i;

    if (!l || l->type != LOBBY_TYPE_GAME)
        return -1;

    c->flags |= CLIENT_FLAG_QLOAD_DONE;

    /* See if everyone's done now. */
    for (i = 0; i < l->max_clients; ++i) {
        if ((c2 = l->clients[i])) {
            /* This client isn't done, so we can end the search now. */
            if (!(c2->flags & CLIENT_FLAG_QLOAD_DONE) &&
                c2->version >= CLIENT_VERSION_GC)
                return 0;
        }
    }

    /* 如果我们到了这里, everyone's done. Send out the packet to everyone now. */
    for (i = 0; i < l->max_clients; ++i) {
        if ((c2 = l->clients[i]) && c2->version >= CLIENT_VERSION_GC) {
            if (send_simple(c2, QUEST_LOAD_DONE_TYPE, 0))
                c2->flags |= CLIENT_FLAG_DISCONNECTED;
        }
    }

    return 0;
}

static int process_qlist(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;
    int rv;

    if (!l || l->type != LOBBY_TYPE_GAME)
        return -1;

    pthread_rwlock_rdlock(&ship->qlock);
    pthread_mutex_lock(&l->mutex);

    /* Do we have quests configured? */
    if (!TAILQ_EMPTY(&ship->qmap)) {
        l->flags |= LOBBY_FLAG_QUESTSEL;
        rv = send_quest_categories(c, c->q_lang);
    }
    else {
        rv = send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC4任务未设置."));
    }

    pthread_mutex_unlock(&l->mutex);
    pthread_rwlock_unlock(&ship->qlock);

    return rv;
}

static int process_qlist_end(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;

    if (!l || l->type != LOBBY_TYPE_GAME)
        return -1;

    pthread_mutex_lock(&l->mutex);
    c->cur_lobby->flags &= ~LOBBY_FLAG_QUESTSEL;
    pthread_mutex_unlock(&l->mutex);

    return 0;
}

int send_motd(ship_client_t* c) {
    FILE* fp;
    char buf[1024] = { 0 };
    long len;
    uint32_t lang = (1 << c->q_lang), ver;
    int i, found = 0;
    psocn_info_file_t* f;

    if (!in_lobby(c))
        return send_txt(c, "%s", __(c, "\tE\tC4无法在游戏房间中使用"));

    switch (c->version) {
    case CLIENT_VERSION_DCV1:
        ver = PSOCN_INFO_V1;
        break;

    case CLIENT_VERSION_DCV2:
        ver = PSOCN_INFO_V2;
        break;

    case CLIENT_VERSION_PC:
        ver = PSOCN_INFO_PC;
        break;

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_XBOX:
        ver = PSOCN_INFO_GC;
        break;

    case CLIENT_VERSION_BB:
        ver = PSOCN_INFO_BB;
        break;

    default:
        return 0;
    }

    for (i = 0; i < ship->cfg->info_file_count && !found; ++i) {
        f = &ship->cfg->info_files[i];

        if (!f->desc && (f->versions & ver) && (f->languages & lang)) {
            found = 1;
        }
    }

    /* No MOTD found for the given version/language combination. */
    if (!found) {
        ERR_LOG("未找到 ver %d lang %d 的MOTD信息", ver, lang);
        return 0;
    }

    /* Attempt to open the file */
    fp = fopen(f->filename, "r");

    /* Can't find the file? Punt. */
    if (!fp) {
        return 0;
    }

    /* Figure out the length of the file. */
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Truncate to about 1KB */
    if (len > 1023) {
        len = 1023;
    }

    /* Read the file in. */
    fread(buf, 1, len, fp);
    fclose(fp);
    buf[len] = 0;

    //ERR_LOG("block ：send_motd c->version = %d gc = %u", c->version, c->guildcard);

    /* Send the message to the client. */
    return send_msg(c, MSG_BOX_TYPE, "%s", buf);
}

ship_client_t* block_find_client(block_t* b, uint32_t gc) {
    ship_client_t* it;

    pthread_rwlock_rdlock(&b->lock);

    TAILQ_FOREACH(it, b->clients, qentry) {
        if (it->guildcard == gc) {
            pthread_rwlock_unlock(&b->lock);
            return it;
        }
    }

    pthread_rwlock_unlock(&b->lock);
    return NULL;
}

/* Process block commands for a Dreamcast client. */
int dc_process_pkt(ship_client_t* c, uint8_t* pkt) {
    __try {
        uint8_t type;
        uint8_t flags;
        uint16_t len;
        dc_pkt_hdr_t* dc = (dc_pkt_hdr_t*)pkt;
        pc_pkt_hdr_t* pc = (pc_pkt_hdr_t*)pkt;
        int rv;

        if (c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
            c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
            c->version == CLIENT_VERSION_XBOX) {
            type = dc->pkt_type;
            len = LE16(dc->pkt_len);
            flags = dc->flags;
        }
        else {
            type = pc->pkt_type;
            len = LE16(pc->pkt_len);
            flags = pc->flags;
            dc->pkt_type = type;
            dc->pkt_len = LE16(len);
            dc->flags = flags;
        }

#ifdef DEBUG

        DBG_LOG("舰仓：DC指令 = 0x%04X %s 长度 = %d 标志 = %d 字节 GC = %u", type, c_cmd_name(type, 0), len, flags, c->guildcard);

#endif // DEBUG

        switch (type) {
        case LOGIN_8B_TYPE:
            return dcnte_process_login(c, (dcnte_login_8b_pkt*)pkt);

        case LOGIN_93_TYPE:
            return dc_process_login(c, (dc_login_93_pkt*)pkt);

        case CHAR_DATA_TYPE:
            return dc_process_char(c, (dc_char_data_pkt*)pkt);

        case GAME_SUBCMD60_TYPE:
            return subcmd_handle_60(c, (subcmd_pkt_t*)pkt);

        case GAME_SUBCMD62_TYPE:
            return subcmd_handle_62(c, (subcmd_pkt_t*)pkt);

        case GAME_SUBCMD6D_TYPE:
            return subcmd_handle_6D(c, (subcmd_pkt_t*)pkt);

        case LOBBY_CHANGE_TYPE:
            return dc_process_change_lobby(c, (dc_select_pkt*)pkt);

        case PING_TYPE:
            if (!(c->flags & CLIENT_FLAG_SENT_MOTD)) {
                send_motd(c);
                c->flags |= CLIENT_FLAG_SENT_MOTD;
            }

            /* If they've got the always legit flag set, but we haven't run the
               legit check yet, then do so now. The reason we wait until now is
               so that if they fail the legit check, we can actually inform them
               of that. */
            if ((c->flags & CLIENT_FLAG_ALWAYS_LEGIT) &&
                !(c->flags & CLIENT_FLAG_LEGIT)) {
                psocn_limits_t* limits;

                pthread_rwlock_rdlock(&ship->llock);
                if (!ship->def_limits) {
                    pthread_rwlock_unlock(&ship->llock);
                    c->flags &= ~CLIENT_FLAG_ALWAYS_LEGIT;
                    send_txt(c, "%s", __(c, "\tE\tC7Legit mode not\n"
                        "available on this\n"
                        "ship."));
                    return 0;
                }

                limits = ship->def_limits;

                if (client_legit_check(c, limits)) {
                    c->flags &= ~CLIENT_FLAG_ALWAYS_LEGIT;
                    send_txt(c, "%s", __(c, "\tE\tC7You failed the legit "
                        "check."));
                }
                else {
                    /* Set the flag and retain the limits list on the client. */
                    c->flags |= CLIENT_FLAG_LEGIT;
                    c->limits = retain(limits);
                }

                pthread_rwlock_unlock(&ship->llock);
            }

            if (c->flags & CLIENT_FLAG_WAIT_QPING) {
                lobby_t* l = c->cur_lobby;

                if (!l)
                    return -1;

                pthread_mutex_lock(&l->mutex);
                l->flags &= ~LOBBY_FLAG_BURSTING;
                c->flags &= ~(CLIENT_FLAG_BURSTING | CLIENT_FLAG_WAIT_QPING);

                rv = lobby_resend_burst(l, c);
                rv = send_simple(c, PING_TYPE, 0) |
                    lobby_handle_done_burst(l, c);
                pthread_mutex_unlock(&l->mutex);
                return rv;
            }

            return 0;

        case BURSTING_TYPE:
            /* If we've already gotten one of these, disconnect the client. */
            if (c->flags & CLIENT_FLAG_GOT_05) {
                c->flags |= CLIENT_FLAG_DISCONNECTED;
            }

            c->flags |= CLIENT_FLAG_GOT_05;
            return 0;

        case CHAT_TYPE:
            if (c->version != CLIENT_VERSION_PC) {
                return dc_process_chat(c, (dc_chat_pkt*)pkt);
            }
            else {
                return pc_process_chat(c, (dc_chat_pkt*)pkt);
            }

            return -1;

        case GUILD_SEARCH_TYPE:
            return dc_process_guild_search(c, (dc_guild_search_pkt*)pkt);

        case SIMPLE_MAIL_TYPE:
            if (c->version != CLIENT_VERSION_PC) {
                return dc_process_mail(c, (simple_mail_pkt*)pkt);
            }
            else {
                return pc_process_mail(c, (simple_mail_pkt*)pkt);
            }

            return -1;

        case DC_GAME_CREATE_TYPE:
        case GAME_CREATE_TYPE:
            if (c->version == CLIENT_VERSION_DCV1 &&
                (c->flags & CLIENT_FLAG_IS_NTE)) {
                return dcnte_process_game_create(c,
                    (dcnte_game_create_pkt*)pkt);
            }
            else if (c->version != CLIENT_VERSION_PC &&
                c->version != CLIENT_VERSION_GC &&
                c->version != CLIENT_VERSION_XBOX) {
                return dc_process_game_create(c, (dc_game_create_pkt*)pkt);
            }
            else if (c->version == CLIENT_VERSION_PC) {
                return pc_process_game_create(c, (pc_game_create_pkt*)pkt);
            }
            else {
                return gc_process_game_create(c, (gc_game_create_pkt*)pkt);
            }

            return -1;

        case DONE_BURSTING_TYPE:
            return dc_process_done_burst(c);

        case GAME_LIST_TYPE:
            return send_game_list(c, c->cur_block);

        case MENU_SELECT_TYPE:
            return dc_process_menu(c, (dc_select_pkt*)pkt);

        case LEAVE_GAME_PL_DATA_TYPE:
            return dc_process_char(c, (dc_char_data_pkt*)pkt);

        case LOBBY_INFO_TYPE:
            return send_info_list(c, ship);

        case BLOCK_LIST_REQ_TYPE:
        case DCNTE_BLOCK_LIST_REQ_TYPE:
            return send_block_list(c, ship);

        case INFO_REQUEST_TYPE:
            return dc_process_info_req(c, (dc_select_pkt*)pkt);

        case QUEST_LIST_TYPE:
            return process_qlist(c);

        case QUEST_END_LIST_TYPE:
            return process_qlist_end(c);

        case LOGIN_9D_TYPE:
            return dcv2_process_login(c, (dcv2_login_9d_pkt*)pkt);

        case LOBBY_NAME_TYPE:
            return send_lobby_name(c, c->cur_lobby);

        case LOBBY_ARROW_CHANGE_TYPE:
            return dc_process_arrow(c, flags);

        case SHIP_LIST_TYPE:
        case DCNTE_SHIP_LIST_TYPE:
            return send_ship_list(c, ship, ship->cfg->menu_code);

        case CHOICE_OPTION_TYPE:
            return send_choice_search(c);

        case CHOICE_SETTING_TYPE:
            /* Ignore these for now. */
            return 0;

        case CHOICE_SEARCH_TYPE:
            return send_choice_reply(c, (dc_choice_set_pkt*)pkt);

        case LOGIN_9E_TYPE:
            if (c->version != CLIENT_VERSION_XBOX)
                return gc_process_login(c, (gc_login_9e_pkt*)pkt);
            else
                return xb_process_login(c, (xb_login_9e_pkt*)pkt);

            return -1;

        case QUEST_CHUNK_TYPE:
        case QUEST_FILE_TYPE:
            /* Uhh... Ignore these for now, we've already sent it by the time we
               get this packet from the client. */
            return 0;

        case QUEST_LOAD_DONE_TYPE:
            return process_qload_done(c);

        case INFOBOARD_WRITE_TYPE:
            return process_infoboard(c, (gc_write_info_pkt*)pkt);

        case INFOBOARD_TYPE:
            return send_infoboard(c, c->cur_lobby);

        case TRADE_0_TYPE:
            return process_trade(c, (gc_trade_pkt*)pkt);

        case TRADE_2_TYPE:
            /* Ignore. */
            return 0;

        case GC_MSG_BOX_CLOSED_TYPE:
            /* Ignore. */
            return 0;

        case BLACKLIST_TYPE:
            return gc_process_blacklist(c, (gc_blacklist_update_pkt*)pkt);

        case AUTOREPLY_SET_TYPE:
            return client_set_autoreply(c, ((autoreply_set_pkt*)dc)->msg,
                len - 4);

        case AUTOREPLY_CLEAR_TYPE:
            return client_disable_autoreply(c);

        case GAME_COMMAND_C9_TYPE:
        case GAME_COMMAND_CB_TYPE:
            return subcmd_handle_ep3_bcast(c, (subcmd_pkt_t*)pkt);

        case EP3_COMMAND_TYPE:
            return process_ep3_command(c, pkt);

        case EP3_SERVER_DATA_TYPE:
            return on_CA_Ep3(c, pkt);

        case EP3_MENU_CHANGE_TYPE:
            if (dc->flags != 0) {
                return send_simple(c, EP3_MENU_CHANGE_TYPE, 0);
            }
            return 0;

        case EP3_GAME_CREATE_TYPE:
            return ep3_process_game_create(c, (ep3_game_create_pkt*)pkt);

        case QUEST_STATS_TYPE:
            return process_dc_update_quest_stats(c, (dc_update_quest_stats_pkt*)pkt);

        default:
            if (!script_execute_pkt(ScriptActionUnknownBlockPacket, c, pkt,
                len)) {
                ERR_LOG("GC %u 未知数 0x%X 据包!", c->guildcard, type);
                print_ascii_hex(errl, pkt, len);
                return -3;
            }
            return 0;
        }
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

/* Process any packet that comes into a block. */
int block_process_pkt(ship_client_t* c, uint8_t* pkt) {

    //DBG_LOG("舰仓 ：舰仓处理数据 端口 %d 版本 = %d", c->sock, c->version);
    switch (c->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        return dc_process_pkt(c, pkt);

    case CLIENT_VERSION_BB:
        return bb_process_pkt(c, pkt);
    }

    return -1;
}
