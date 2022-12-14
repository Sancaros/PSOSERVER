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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <errno.h>
#include <time.h>

#include <WinSock_Defines.h>

#include <f_logs.h>
#include <psopipe.h>
#include <pso_menu.h>
#include <pso_ping.h>
//#include <pso_version.h>

#include "ship.h"
#include "clients.h"
#include "ship_packets.h"
#include "shipgate.h"
#include "utils.h"
#include "bans.h"
#include "scripts.h"
#include "admin.h"

#ifdef ENABLE_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif

extern int enable_ipv6;
extern uint32_t ship_ip4;
extern uint8_t ship_ip6[16];

miniship_t* ship_find_ship(ship_t* s, uint32_t sid) {
    miniship_t* i;

    TAILQ_FOREACH(i, &s->ships, qentry) {
        if (i->ship_id == sid) {
            return i;
        }
    }

    return NULL;
}

static void clean_shiplist(ship_t* s) {
    miniship_t* i, * tmp;

    i = TAILQ_FIRST(&s->ships);
    while (i) {
        tmp = TAILQ_NEXT(i, qentry);
        free_safe(i);
        i = tmp;
    }

    free_safe(s->menu_codes);
}

static psocn_event_t* find_current_event(ship_t* s) {
    time_t now;
    int i;
    struct tm tm_now = { 0 };
    int m, d;

    if (s->cfg->event_count == 1) {
        return s->cfg->events;
    }

    /* If we have more than one event, then its a bit more work... */
    now = time(NULL);
    if (gmtime_s(&tm_now, &now))
        return NULL;
    m = tm_now.tm_mon + 1;
    d = tm_now.tm_mday;

    for (i = 1; i < s->cfg->event_count; ++i) {
        /* Are we completely outside the month(s) of the event? */
        if (s->cfg->events[i].start_month <= s->cfg->events[i].end_month &&
            (m < s->cfg->events[i].start_month ||
                m > s->cfg->events[i].end_month)) {
            continue;
        }
        /* We need a special case for events that span the end of a year... */
        else if (s->cfg->events[i].start_month > s->cfg->events[i].end_month &&
            m > s->cfg->events[i].end_month &&
            m < s->cfg->events[i].start_month) {
            continue;
        }
        /* If we're in the start month, are we before the start day? */
        else if (m == s->cfg->events[i].start_month &&
            d < s->cfg->events[i].start_day) {
            continue;
        }
        /* If we're in the end month, are we after the end day? */
        else if (m == s->cfg->events[i].end_month &&
            d > s->cfg->events[i].end_day) {
            continue;
        }

        /* This is the event we're looking for! */
        return &s->cfg->events[i];
    }

    /* No events matched, so use the default event. */
    return s->cfg->events;
}

static void* ship_thd(void* d) {
    int nfds;
    uint32_t i;
    int select_result = 0;
    ship_t* s = (ship_t*)d;
    struct timeval timeout;
    fd_set readfds, writefds, exceptfds;
    ship_client_t* it, * tmp;
    socklen_t len;
    struct sockaddr_storage addr;
    struct sockaddr* addr_p = (struct sockaddr*)&addr;
    char ipstr[INET6_ADDRSTRLEN];
    int sock, rv;
    ssize_t sent;
    time_t now;
    time_t last_ban_sweep = time(NULL);
    uint32_t numsocks = 1;
    psocn_event_t* event, * oldevent = s->cfg->events;

#ifdef PSOCN_ENABLE_IPV6
    if (enable_ipv6) {
        numsocks = 2;
    }
#endif

    BLOCK_LOG("%s: 舰仓 (1 - %d) 启动中...", s->cfg->name, s->cfg->blocks);

    /* Fire up the threads for each block. */
    for (i = 1; i <= s->cfg->blocks; ++i) {
        s->blocks[i - 1] = block_server_start(s, i, s->cfg->base_port +
            (i * 6));
    }

    BLOCK_LOG("%s: 舰仓 (1 - %d) 已开启", s->cfg->name, i - 1);

    SHIPS_LOG("程序运行中...");
    SHIPS_LOG("请用 <Ctrl-C> 关闭程序.");

    /* We've now started up completely, so run the startup script, if one is
       configured. */
    script_execute(ScriptActionStartup, NULL, SCRIPT_ARG_PTR, s, 0);

    /* While we're still supposed to run... do it. */
    while (s->run) {
        /* Clear the fd_sets so we can use them again. */
        nfds = 0;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        now = time(NULL);

        /* Break out if we're shutting down now */
        if(s->shutdown_time && s->shutdown_time <= now) {
            s->run = 0;
            break;
        }

        /* If we haven't swept the bans list in the last day, do it now. */
        if((last_ban_sweep + 3600 * 24) <= now) {
            ban_sweep(s);
            last_ban_sweep = now = time(NULL);
        }

        /* If the shipgate isn't there, attempt to reconnect */
        if(s->sg.sock == SOCKET_ERROR && s->sg.login_attempt < now) {
            if (shipgate_reconnect(&s->sg)) {
                /* Set the next login attempt to ~15 seconds from now... */
                s->sg.login_attempt = now + 14;
                timeout.tv_sec = 15;
            }
            else {
                s->sg.login_attempt = 0;
            }
        }

        /* Check the event to see if its changed on us... */
        event = find_current_event(s);

        if (event != oldevent) {
            SHIPS_LOG("节日事件切换 (大厅: %d, 房间: %d)",
                (int)event->lobby_event, (int)event->game_event);

            if (event->lobby_event != 0xFF) {
                s->lobby_event = event->lobby_event;
                update_lobby_event();
            }

            if (event->game_event != 0xFF) {
                s->game_event = event->game_event;
            }

            oldevent = event;
        }

        /* Fill the sockets into the fd_sets so we can use select below. */
        TAILQ_FOREACH(it, s->clients, qentry) {
            /* If we haven't heard from a client in 2 minutes, its dead.
               Disconnect it. */
            if(now > it->last_message + 120) {
                it->flags |= CLIENT_FLAG_DISCONNECTED;
                continue;
            }
            /* Otherwise, if we haven't heard from them in a minute, ping it. */
            else if (now > it->last_message + 60 && now > it->last_sent + 10) {
                if (send_simple(it, PING_TYPE, 0)) {
                    it->flags |= CLIENT_FLAG_DISCONNECTED;
                    continue;
                }

                it->last_sent = now;
            }

            FD_SET(it->sock, &readfds);

            /* Only add to the write fd set if we have something to send out. */
            if (it->sendbuf_cur) {
                FD_SET(it->sock, &writefds);
            }

            nfds = max(nfds, (int)it->sock);
        }

        /* Add the listening sockets to the read fd_set. */
        for (i = 0; i < numsocks; ++i) {
            FD_SET(s->dcsock[i], &readfds);
            nfds = max(nfds, s->dcsock[i]);
            FD_SET(s->pcsock[i], &readfds);
            nfds = max(nfds, s->pcsock[i]);
            FD_SET(s->gcsock[i], &readfds);
            nfds = max(nfds, s->gcsock[i]);
            FD_SET(s->ep3sock[i], &readfds);
            nfds = max(nfds, s->ep3sock[i]);
            FD_SET(s->bbsock[i], &readfds);
            nfds = max(nfds, s->bbsock[i]);
            FD_SET(s->xbsock[i], &readfds);
            nfds = max(nfds, s->xbsock[i]);
        }

        FD_SET(s->pipes[1], &readfds);
        nfds = max(nfds, s->pipes[1]);

        /* Add the shipgate socket to the fd_sets */
        if (s->sg.sock != -1) {
            FD_SET(s->sg.sock, &readfds);

            FD_SET(s->sg.sock, &exceptfds);

            if (s->sg.sendbuf_cur) {
                FD_SET(s->sg.sock, &writefds);
            }

            nfds = max(nfds, s->sg.sock);
        }

        /* If we're supposed to shut down soon, make sure we aren't in the
           middle of a select still when its supposed to happen. */
        if(s->shutdown_time && now + timeout.tv_sec > s->shutdown_time) {
            timeout.tv_sec = (long)(s->shutdown_time - now);
        }

        /* Wait for some activity... */
        if((select_result = select(nfds + 1, &readfds, &writefds, &exceptfds, &timeout)) > 0) {
            /* Clear anything written to the pipe */
            if (FD_ISSET(s->pipes[1], &readfds)) {
                fread(&len, 1, 1, (FILE*)s->pipes[1]);
            }

            for (i = 0; i < numsocks; ++i) {
                if (FD_ISSET(s->dcsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(s->dcsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    SHIPS_LOG("%s: 舰船收到 Dreamcast 客户端连接 IP: %s",
                        s->cfg->name, ipstr);

                    if (!(tmp = client_create_connection(sock,
                        CLIENT_VERSION_DCV1,
                        CLIENT_TYPE_SHIP,
                        s->clients, s, NULL,
                        addr_p, len))) {
                        closesocket(sock);
                    }

                    if (s->shutdown_time) {
                        send_msg_box(tmp, "%s\n\n%s\n%s",
                            __(tmp, "\tE舰船已被击沉."),
                            __(tmp, "请登录其他舰船."),
                            __(tmp, "断开连接."));
                        tmp->flags |= CLIENT_FLAG_DISCONNECTED;
                    }
                }

                if (FD_ISSET(s->pcsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(s->pcsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    SHIPS_LOG("%s: 舰船收到 PC 客户端连接 IP: %s",
                        s->cfg->name, ipstr);

                    if (!(tmp = client_create_connection(sock, CLIENT_VERSION_PC,
                        CLIENT_TYPE_SHIP,
                        s->clients, s, NULL,
                        addr_p, len))) {
                        closesocket(sock);
                    }

                    if(s->shutdown_time) {
                        send_msg_box(tmp, "%s\n\n%s\n%s",
                            __(tmp, "\tE舰船已被击沉."),
                            __(tmp, "请登录其他舰船."),
                            __(tmp, "断开连接."));
                        tmp->flags |= CLIENT_FLAG_DISCONNECTED;
                    }
                }

                if (FD_ISSET(s->gcsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(s->gcsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    SHIPS_LOG("%s: 舰船收到 GameCube 客户端连接 IP: %s",
                        s->cfg->name, ipstr);

                    if (!(tmp = client_create_connection(sock, CLIENT_VERSION_GC,
                        CLIENT_TYPE_SHIP,
                        s->clients, s, NULL,
                        addr_p, len))) {
                        closesocket(sock);
                    }

                    if(s->shutdown_time) {
                        send_msg_box(tmp, "%s\n\n%s\n%s",
                            __(tmp, "\tE舰船已被击沉."),
                            __(tmp, "请登录其他舰船."),
                            __(tmp, "断开连接."));
                        tmp->flags |= CLIENT_FLAG_DISCONNECTED;
                    }
                }

                if (FD_ISSET(s->ep3sock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(s->ep3sock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    SHIPS_LOG("%s: 舰船收到 Episode 3 客户端连接 "
                        "IP: %s", s->cfg->name, ipstr);

                    if (!(tmp = client_create_connection(sock,
                        CLIENT_VERSION_EP3,
                        CLIENT_TYPE_SHIP,
                        s->clients, s, NULL,
                        addr_p, len))) {
                        closesocket(sock);
                    }

                    if(s->shutdown_time) {
                        send_msg_box(tmp, "%s\n\n%s\n%s",
                            __(tmp, "\tE舰船已被击沉."),
                            __(tmp, "请登录其他舰船."),
                            __(tmp, "断开连接."));
                        tmp->flags |= CLIENT_FLAG_DISCONNECTED;
                    }
                }

                if (FD_ISSET(s->bbsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(s->bbsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    SHIPS_LOG("%s: 舰船收到 Blue Burst 客户端连接 "
                        "IP: %s", s->cfg->name, ipstr);

                    if (!(tmp = client_create_connection(sock,
                        CLIENT_VERSION_BB,
                        CLIENT_TYPE_SHIP,
                        s->clients, s, NULL,
                        addr_p, len))) {
                        closesocket(sock);
                    }

                    if(s->shutdown_time) {
                        send_msg_box(tmp, "%s\n\n%s\n%s",
                            __(tmp, "\tE舰船已被击沉."),
                            __(tmp, "请登录其他舰船."),
                            __(tmp, "断开连接."));
                        tmp->flags |= CLIENT_FLAG_DISCONNECTED;
                    }
                }

                if (FD_ISSET(s->xbsock[i], &readfds)) {
                    len = sizeof(struct sockaddr_storage);
                    if ((sock = accept(s->xbsock[i], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    SHIPS_LOG("%s: 舰船收到 Xbox 客户端连接 "
                        "IP: %s", s->cfg->name, ipstr);

                    if (!(tmp = client_create_connection(sock,
                        CLIENT_VERSION_XBOX,
                        CLIENT_TYPE_SHIP,
                        s->clients, s, NULL,
                        addr_p, len))) {
                        closesocket(sock);
                    }

                    if(s->shutdown_time) {
                        send_msg_box(tmp, "%s\n\n%s\n%s",
                            __(tmp, "\tE舰船已被击沉."),
                            __(tmp, "请登录其他舰船."),
                            __(tmp, "断开连接."));
                        tmp->flags |= CLIENT_FLAG_DISCONNECTED;
                    }
                }
            }

            /* Process the shipgate */
            if (s->sg.sock != SOCKET_ERROR && FD_ISSET(s->sg.sock, &readfds)) {
                if((rv = shipgate_process_pkt(&s->sg))) {
                    SHIPS_LOG("%s: 失去与船闸的连接1 rv = %d",
                        s->cfg->name, rv);

                    /* Close the connection so we can attempt to reconnect */
                    gnutls_bye(s->sg.session, GNUTLS_SHUT_RDWR);
                    closesocket(s->sg.sock);
                    gnutls_deinit(s->sg.session);
                    s->sg.sock = SOCKET_ERROR;

                    if (rv < -1) {
                        SHIPS_LOG("%s: 与船闸连接出错1, 断开!",
                            s->cfg->name);
                        s->run = 0;
                    }
                }
            }

            if (s->sg.sock != SOCKET_ERROR && FD_ISSET(s->sg.sock, &writefds)) {
                if(rv = shipgate_send_pkts(&s->sg)) {
                    SHIPS_LOG("%s: 失去与船闸的连接2 rv = %d",
                        s->cfg->name, rv);

                    /* Close the connection so we can attempt to reconnect */
                    gnutls_bye(s->sg.session, GNUTLS_SHUT_RDWR);
                    closesocket(s->sg.sock);
                    gnutls_deinit(s->sg.session);
                    s->sg.sock = SOCKET_ERROR;
                    
                    if (rv < -1) {
                        SHIPS_LOG("%s: 与船闸连接出错2, 断开!",
                            s->cfg->name);
                    }
                }
            }

            /* Process client connections. */
            TAILQ_FOREACH(it, s->clients, qentry) {
                /* Check if this connection was trying to send us something. */
                if (FD_ISSET(it->sock, &readfds)) {
                    if (client_process_pkt(it)) {
                        it->flags |= CLIENT_FLAG_DISCONNECTED;
                        continue;
                    }
                }

                if (FD_ISSET(it->sock, &exceptfds)) {
                    it->flags |= CLIENT_FLAG_DISCONNECTED;
                    continue;
                }

                /* If we have anything to write, check if we can right now. */
                if (FD_ISSET(it->sock, &writefds)) {
                    if (it->sendbuf_cur) {
                        sent = send(it->sock, it->sendbuf + it->sendbuf_start,
                            it->sendbuf_cur - it->sendbuf_start, 0);

                        /*ERR_LOG(
                            "舰船writefds端口 %d 发送数据 = %d 字节 版本识别 = %d",
                            it->sock, sent, it->version);*/

                        /* If we fail to send, and the error isn't EAGAIN,
                           bail. */
                        if (sent == SOCKET_ERROR) {
                            if (errno != EAGAIN) {
                                it->flags |= CLIENT_FLAG_DISCONNECTED;
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
            }
        } else if (select_result == -1) {
            ERR_LOG("select 套接字 = -1");
        }

        /* Clean up any dead connections (its not safe to do a TAILQ_REMOVE in
           the middle of a TAILQ_FOREACH, and destroy_connection does indeed
           use TAILQ_REMOVE). */
        it = TAILQ_FIRST(s->clients);
        while (it) {
            tmp = TAILQ_NEXT(it, qentry);

            if (it->flags & CLIENT_FLAG_DISCONNECTED) {
                client_destroy_connection(it, s->clients);
            }

            it = tmp;
        }
    }

    SHIPS_LOG("%s: 关闭舰船服务器...", s->cfg->name);

    /* Before we shut down, run the shutdown script, if one is configured. */
    script_execute(ScriptActionShutdown, NULL, SCRIPT_ARG_PTR, s, 0);

#ifdef ENABLE_LUA
    /* Remove the table from the registry */
    luaL_unref(s->lstate, LUA_REGISTRYINDEX, s->script_ref);
#endif

    /* Disconnect any clients. */
    it = TAILQ_FIRST(s->clients);
    while (it) {
        tmp = TAILQ_NEXT(it, qentry);
        client_destroy_connection(it, s->clients);
        it = tmp;
    }

    /* Wait for the block threads to die. */
    for (i = 0; i < s->cfg->blocks; ++i) {
        if (s->blocks[i]) {
            block_server_stop(s->blocks[i]);
        }
    }

    /* Free the ship structure. */
    ban_list_clear(s);
    cleanup_scripts(s);
    pthread_rwlock_destroy(&s->banlock);
    pthread_rwlock_destroy(&s->qlock);
    pthread_rwlock_destroy(&s->llock);
    ship_free_limits(s);
    shipgate_cleanup(&s->sg);
    free_safe(s->gm_list);
    clean_quests(s);
    closesocket(s->pipes[0]);
    closesocket(s->pipes[1]);
#ifdef PSOCN_ENABLE_IPV6
    if (enable_ipv6) {
        closesocket(s->xbsock[1]);
        closesocket(s->bbsock[1]);
        closesocket(s->ep3sock[1]);
        closesocket(s->gcsock[1]);
        closesocket(s->pcsock[1]);
        closesocket(s->dcsock[1]);
    }
#endif
    closesocket(s->xbsock[0]);
    closesocket(s->bbsock[0]);
    closesocket(s->ep3sock[0]);
    closesocket(s->gcsock[0]);
    closesocket(s->pcsock[0]);
    closesocket(s->dcsock[0]);
    clean_shiplist(s);
    free_safe(s->clients);
    free_safe(s->blocks);
    free_safe(s);
    return NULL;
}

ship_t* ship_server_start(psocn_ship_t* s) {
    ship_t* rv;
    int dcsock[2] = { -1, -1 }, pcsock[2] = { -1, -1 };
    int gcsock[2] = { -1, -1 }, ep3sock[2] = { -1, -1 };
    int bbsock[2] = { -1, -1 }, xbsock[2] = { -1, -1 };
    int i;
    psocn_limits_t* l;
    limits_entry_t* ent;

    SHIPS_LOG("%s: 启动舰船...", s->name);

    /* Create the sockets for listening for connections. */
    dcsock[0] = open_sock(PF_INET, s->base_port);
    if (dcsock[0] < 0) {
        return NULL;
    }

    pcsock[0] = open_sock(PF_INET, s->base_port + 1);
    if (pcsock[0] < 0) {
        goto err_close_dc;
    }

    gcsock[0] = open_sock(PF_INET, s->base_port + 2);
    if (gcsock[0] < 0) {
        goto err_close_pc;
    }

    ep3sock[0] = open_sock(PF_INET, s->base_port + 3);
    if (ep3sock[0] < 0) {
        goto err_close_gc;
    }

    bbsock[0] = open_sock(PF_INET, s->base_port + 4);
    if (bbsock[0] < 0) {
        goto err_close_ep3;
    }

    xbsock[0] = open_sock(PF_INET, s->base_port + 5);
    if (xbsock[0] < 0) {
        goto err_close_bb;
    }

#ifdef PSOCN_ENABLE_IPV6
    if (enable_ipv6) {
        dcsock[1] = open_sock(PF_INET6, s->base_port);
        if (dcsock[1] < 0) {
            goto err_close_xb;
        }

        pcsock[1] = open_sock(PF_INET6, s->base_port + 1);
        if (pcsock[1] < 0) {
            goto err_close_dc_6;
        }

        gcsock[1] = open_sock(PF_INET6, s->base_port + 2);
        if (gcsock[1] < 0) {
            goto err_close_pc_6;
        }

        ep3sock[1] = open_sock(PF_INET6, s->base_port + 3);
        if (ep3sock[1] < 0) {
            goto err_close_gc_6;
        }

        bbsock[1] = open_sock(PF_INET6, s->base_port + 4);
        if (bbsock[1] < 0) {
            goto err_close_ep3_6;
        }

        xbsock[1] = open_sock(PF_INET6, s->base_port + 5);
        if (xbsock[1] < 0) {
            goto err_close_bb_6;
        }
    }
#endif

    /* Make space for the ship structure. */
    rv = (ship_t*)malloc(sizeof(ship_t));

    if (!rv) {
        ERR_LOG("%s: 无法分配内存!", s->name);
        goto err_close_all;
    }

    /* Clear it out */
    memset(rv, 0, sizeof(ship_t));

    /* Make the pipe */
    if (pipe(rv->pipes) == -1) {
        ERR_LOG("%s: 无法创建通信管道!", s->name);
        goto err_free;
    }

    /* Make room for the block structures. */
    rv->blocks = (block_t**)malloc(sizeof(block_t*) * s->blocks);

    if (!rv->blocks) {
        ERR_LOG("%s: 无法分配舰仓内存!", s->name);
        goto err_pipes;
    }

    /* Make room for the client list. */
    rv->clients = (struct client_queue*)malloc(sizeof(struct client_queue));

    if (!rv->clients) {
        ERR_LOG("%s: 无法为客户端分配内存!", s->name);
        goto err_blocks;
    }

    /* Attempt to read the quest list in. */
    if (s->quests_file && s->quests_file[0]) {
        SHIPS_LOG("%s: 忽略旧任务设置!", s->name);
    }

    /* Deal with loading the quest data... */
    pthread_rwlock_init(&rv->qlock, NULL);
    load_quests(rv, s, 1);

    /* Attempt to read the GM list in. */
    if (s->gm_file) {
        SHIPS_LOG("%s: 获取本地 GM 列表...", s->name);

        if (gm_list_read(s->gm_file, rv)) {
            ERR_LOG("%s: 无法读取 GM 文件!", s->name);
            goto err_quests;
        }

        SHIPS_LOG("%s: 读取到 %d 名本地GM", s->name, rv->gm_count);
    }

    /* Read in all limits files. */
    TAILQ_INIT(&rv->all_limits);
    pthread_rwlock_init(&rv->llock, NULL);

    for (i = 0; i < s->limits_count; ++i) {
        SHIPS_LOG("%s: 解析 /legit 列表 %d...", s->name, i);
        /* Check if they've given us one of the reserved names... */
        if (s->limits[i].name && (!strcmp(s->limits[i].name, "default") ||
            !strcmp(s->limits[i].name, "list"))) {
            ERR_LOG("%s: 非法限制列表名称: %s",
                s->name, s->limits[i].name);
            goto err_limits;
        }

        SHIPS_LOG("%s:     名称: %s", s->name, s->limits[i].name);

        if (psocn_read_limits(s->limits[i].filename, &l)) {
            ERR_LOG("%s: 无法读取限制文件 %s: %s",
                s->name, s->limits[i].name, s->limits[i].filename);
            goto err_limits;
        }

        SHIPS_LOG("%s:    完成解析!", s->name);

        if (!(ent = malloc(sizeof(limits_entry_t)))) {
            ERR_LOG("%s: %s", s->name, strerror(errno));
            psocn_free_limits(l);
            goto err_limits;
        }

        if (s->limits[i].name) {
            if (!(ent->name = _strdup(s->limits[i].name))) {
                ERR_LOG("%s: %s", s->name, strerror(errno));
                psocn_free_limits(l);
                free_safe(ent);
                goto err_limits;
            }
        }
        else {
            ent->name = NULL;
        }

        ent->limits = l;
        TAILQ_INSERT_TAIL(&rv->all_limits, ent, qentry);

        if (s->limits_default == i)
            rv->def_limits = l;
    }

    /* Fill in the structure. */
    pthread_rwlock_init(&rv->banlock, NULL);
    TAILQ_INIT(rv->clients);
    TAILQ_INIT(&rv->ships);
    TAILQ_INIT(&rv->guildcard_bans);
    TAILQ_INIT(&rv->ip_bans);
    rv->cfg = s;
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
    rv->lobby_event = s->events[0].lobby_event;
    rv->game_event = s->events[0].game_event;

    /* Initialize scripting support */
    init_scripts(rv);

#ifdef ENABLE_LUA
    /* Initialize the script table */
    lua_newtable(rv->lstate);
    rv->script_ref = luaL_ref(rv->lstate, LUA_REGISTRYINDEX);
#endif

    /* Attempt to read the ban list */
    if (s->bans_file) {
        if (ban_list_read(s->bans_file, rv)) {
            SHIPS_LOG("%s: 无法读取封禁文件!", s->name);
        }
    }

    /* Create the random number generator state */
    mt19937_init(&rv->rng, (uint32_t)time(NULL));

    /* Connect to the shipgate. */
    if (shipgate_connect(rv, &rv->sg)) {
        ERR_LOG("%s: 无法连接至船闸!", s->name);
        goto err_bans_locks;
    }

    /* Start up the thread for this ship. */
    if (pthread_create(&rv->thd, NULL, &ship_thd, rv)) {
        ERR_LOG("%s: 无法开启舰船线程!", s->name);
        goto err_shipgate;
    }

    return rv;

err_shipgate:
    shipgate_cleanup(&rv->sg);
err_bans_locks:
    pthread_rwlock_destroy(&rv->banlock);
    ban_list_clear(rv);
    cleanup_scripts(rv);
err_limits:
    ship_free_limits(rv);
    pthread_rwlock_destroy(&rv->llock);
    free_safe(rv->gm_list);
err_quests:
    pthread_rwlock_destroy(&rv->qlock);
    clean_quests(rv);
    free_safe(rv->clients);
err_blocks:
    free_safe(rv->blocks);
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

void ship_check_cfg(psocn_ship_t* s) {
    ship_t* rv;
    int i, j;
    char fn[512];
    psocn_limits_t* l;
    limits_entry_t* ent;

    SHIPS_LOG("检测舰船 %s 的设置...", s->name);

    /* Make space for the ship structure. */
    rv = (ship_t*)malloc(sizeof(ship_t));

    if (!rv) {
        ERR_LOG("%s: 无法分配舰船内存空间!", s->name);
        return;
    }

    /* Clear it out */
    memset(rv, 0, sizeof(ship_t));
    TAILQ_INIT(&rv->qmap);
    TAILQ_INIT(&rv->all_limits);
    pthread_rwlock_init(&rv->llock, NULL);
    rv->cfg = s;

    /* Attempt to read the quest list in. */
    if (s->quests_file && s->quests_file[0]) {
        SHIPS_LOG("%s: 忽略旧的任务设置. 请更新设置!", s->name);
    }

    if (s->quests_dir && s->quests_dir[0]) {
        for (i = 0; i < CLIENT_VERSION_ALL; ++i) {
            for (j = 0; j < CLIENT_LANG_ALL; ++j) {
                sprintf(fn, "%s/%s/quests_%s.xml", s->quests_dir,
                    client_type[i]->ver_name, language_codes[j]);
                if (!psocn_quests_read(fn, &rv->qlist[i][j])) {
                    if (!quest_map(&rv->qmap, &rv->qlist[i][j], i, j)) {
                        SHIPS_LOG("已读取任务 %s 语言 %s",
                            client_type[i]->ver_name, language_codes[j]);
                    }
                    else {
                        SHIPS_LOG("无法读取任务 %s 语言 %s",
                            client_type[i]->ver_name, language_codes[j]);
                        psocn_quests_destroy(&rv->qlist[i][j]);
                    }
                }
            }
        }
    }

    /* Attempt to read the GM list in. */
    if (s->gm_file) {
        SHIPS_LOG("%s: 读取本地 GM 列表...", s->name);

        if (gm_list_read(s->gm_file, rv)) {
            ERR_LOG("%s: 无法读取 GM 文件!", s->name);
        }

        SHIPS_LOG("%s: 已获取 %d 名本地 GM", s->name, rv->gm_count);
    }

    /* Read in all limits files. */
    for (i = 0; i < s->limits_count; ++i) {
        SHIPS_LOG("%s: 分析 /legit 列表 %d...", s->name, i);

        /* Check if they've given us one of the reserved names... */
        if (s->limits[i].name && (!strcmp(s->limits[i].name, "default") ||
            !strcmp(s->limits[i].name, "list"))) {
            ERR_LOG("%s: 跳过非法限制列表名称: %s",
                s->name, s->limits[i].name);
            continue;
        }

        SHIPS_LOG("%s:     名称: %s", s->name, s->limits[i].name);

        if (psocn_read_limits(s->limits[i].filename, &l)) {
            ERR_LOG("%s: 无法读取的限制文件 %s: %s",
                s->name, s->limits[i].name, s->limits[i].filename);
        }

        SHIPS_LOG("%s:     解析完成!", s->name);

        if (!(ent = malloc(sizeof(limits_entry_t)))) {
            ERR_LOG("%s: %s", s->name, strerror(errno));
            psocn_free_limits(l);
            continue;
        }

        if (s->limits[i].name) {
            if (!(ent->name = _strdup(s->limits[i].name))) {
                ERR_LOG("%s: %s", s->name, strerror(errno));
                free_safe(ent);
                psocn_free_limits(l);
                continue;
            }

            if (!(l->name = _strdup(s->limits[i].name))) {
                ERR_LOG("%s: %s", s->name, strerror(errno));
                free_safe(ent->name);
                free_safe(ent);
                psocn_free_limits(l);
                continue;
            }
        }
        else {
            ent->name = NULL;
            l->name = NULL;
        }

        ent->limits = l;
        TAILQ_INSERT_TAIL(&rv->all_limits, ent, qentry);

        if (s->limits_default == i)
            rv->def_limits = l;
    }

    /* Initialize scripting support */
    init_scripts(rv);

    /* Attempt to read the ban list */
    if (s->bans_file) {
        if (ban_list_read(s->bans_file, rv)) {
            SHIPS_LOG("%s: 无法读取封禁文件!", s->name);
        }
    }

    ban_list_clear(rv);
    ship_free_limits(rv);
    pthread_rwlock_destroy(&rv->llock);
    free_safe(rv->gm_list);
    clean_quests(rv);
    free_safe(rv);
}

void ship_server_stop(ship_t* s) {
    /* Set the flag to kill the ship. */
    s->run = 0;

    /* Send a byte to the pipe so that we actually break out of the select. */
    fwrite("\xFF", 1, 1, (FILE*)s->pipes[0]);

    /* Wait for it to die. */
    pthread_join(s->thd, NULL);
}

void ship_server_shutdown(ship_t* s, time_t when) {
    if (when >= time(NULL)) {
        s->shutdown_time = when;

        /* Send a byte to the pipe so that we actually break out of the select
           and put a probably more sane amount in the timeout there */
        fwrite("\xFF", 1, 1, (FILE*)s->pipes[0]);
    }
}

static int dcnte_process_login(ship_client_t* c, dcnte_login_8b_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;

    /* Make sure v1 is allowed on this ship. */
    if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NODCNTE)) {
        send_msg_box(c, "%s", __(c, "\tEPSO NTE is not supported on\n"
            "this ship.\n\nDisconnecting."));
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    c->language_code = CLIENT_LANG_JAPANESE;
    c->flags |= CLIENT_FLAG_IS_NTE;
    c->guildcard = LE32(pkt->guildcard);

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

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_block_list(c, ship)) {
        return -2;
    }

    return 0;
}

static int dc_process_login(ship_client_t* c, dc_login_93_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;

    /* Make sure v1 is allowed on this ship. */
    if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOV1)) {
        send_msg_box(c, "%s", __(c, "\tEPSO Version 1 is not supported on\n"
            "this ship.\n\nDisconnecting."));
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    c->language_code = pkt->language_code;
    c->guildcard = LE32(pkt->guildcard);

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

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_block_list(c, ship)) {
        return -2;
    }

    return 0;
}

static int is_pctrial(dcv2_login_9d_pkt* pkt) {
    int i = 0;

    for (i = 0; i < 8; ++i) {
        if (pkt->serial[i] || pkt->access_key[i])
            return 0;
    }

    return 1;
}

/* Just in case I ever use the rest of the stuff... */
static int dcv2_process_login(ship_client_t* c, dcv2_login_9d_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;

    /* Make sure the client's version is allowed on this ship. */
    if (c->version != CLIENT_VERSION_PC) {
        if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOV2)) {
            send_msg_box(c, "%s", __(c, "\tEPSO Version 2 is not supported "
                "on\nthis ship.\n\nDisconnecting."));
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return 0;
        }
    }
    else {
        if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOPC)) {
            send_msg_box(c, "%s", __(c, "\tEPSO for PC is not supported "
                "on\nthis ship.\n\nDisconnecting."));
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return 0;
        }

        /* Mark trial users as trial users. */
        if (is_pctrial(pkt)) {
            c->flags |= CLIENT_FLAG_IS_NTE;

            if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOPCNTE)) {
                send_msg_box(c, "%s", __(c, "\tEPSO for PC Network Trial "
                    "Edition\nis not supported on "
                    "this ship.\n\nDisconnecting."));
                c->flags |= CLIENT_FLAG_DISCONNECTED;
                return 0;
            }
        }
    }

    c->language_code = pkt->language_code;
    c->guildcard = LE32(pkt->guildcard);

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

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_block_list(c, ship)) {
        return -2;
    }

    return 0;
}

static int gc_process_login(ship_client_t* c, gc_login_9e_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;

    /* Make sure PSOGC is allowed on this ship. */
    if(c->version == CLIENT_VERSION_GC) {
        if((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOEP12)) {
            send_msg_box(c, "%s", __(c, "\tEPSO Episode 1 & 2 is not "
                                         "supported on\nthis ship.\n\n"
                                         "Disconnecting."));
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return 0;
        }
    }
    else {
        if((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOEP3)) {
            send_msg_box(c, "%s", __(c, "\tEPSO Episode 3 is not "
                                         "supported on\nthis ship.\n\n"
                                         "Disconnecting."));
            c->flags |= CLIENT_FLAG_DISCONNECTED;
            return 0;
        }
    }

    c->language_code = pkt->language_code;
    c->guildcard = LE32(pkt->guildcard);

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

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_block_list(c, ship)) {
        return -2;
    }

    return 0;
}

static int xb_process_login(ship_client_t* c, xb_login_9e_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;

    /* Make sure PSOX is allowed on this ship. */
    if ((ship->cfg->shipgate_flags & SHIPGATE_FLAG_NOPSOX)) {
        send_msg_box(c, "%s", __(c, "\tEPSO Episode 1 & 2 for Xbox "
                                     "is not supported on\nthis ship.\n\n"
                                     "Disconnecting."));
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    c->language_code = pkt->language_code;
    c->guildcard = LE32(pkt->guildcard);

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

    if (send_dc_security(c, c->guildcard, NULL, 0)) {
        return -1;
    }

    if (send_block_list(c, ship)) {
        return -2;
    }

    return 0;
}

static int bb_process_login(ship_client_t* c, bb_login_93_pkt* pkt) {
    char* ban_reason;
    time_t ban_end;
    uint32_t guild_id;

    /* Make sure PSOBB is allowed on this ship. */
    if ((ship->cfg->shipgate_flags & LOGIN_FLAG_NOBB)) {
        send_msg_box(c, "%s", __(c, "\tEPSO Blue Burst is not "
                                        "supported on\nthis ship.\n\n"
                                        "Disconnecting."));
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    c->guildcard = LE32(pkt->guildcard);
    guild_id = LE32(pkt->guild_id);
    c->menu_id = pkt->menu_id;
    c->preferred_lobby_id = pkt->preferred_lobby_id;

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

    //bool is_old_format;
    //if (pkt->hdr.pkt_len == sizeof(bb_login_93_pkt) - 8) {
    //    is_old_format = true;
    //}
    //else if (pkt->hdr.pkt_len == sizeof(bb_login_93_pkt)) {
    //    is_old_format = false;
    //}
    //else {
    //    send_msg_box(c, "%s", __(c, "\tEPSO Blue Burst is not "
    //        "supported on\nthis ship.\n\n"
    //        "Disconnecting."));
    //    c->flags |= CLIENT_FLAG_DISCONNECTED;
    //    return 0;
    //}

    /* Copy in the security data */
    memcpy(&c->sec_data, &pkt->var.new_clients.cfg, sizeof(bb_client_config_pkt));

    if (c->sec_data.cfg.magic != CLIENT_CONFIG_MAGIC) {
        send_bb_security(c, 0, LOGIN_93BB_FORCED_DISCONNECT, 0, NULL, 0);
        return -1;
    }

    /* Send the security data packet */
    if (send_bb_security(c, c->guildcard, LOGIN_93BB_OK, guild_id,
        &c->sec_data, sizeof(bb_client_config_pkt))) {
        return -2;
    }

    if (send_block_list(c, ship)) {
        return -3;
    }

    return 0;
}

static int dc_process_block_sel(ship_client_t* c, dc_select_pkt* pkt) {
    uint32_t block = LE32((int)pkt->item_id);
    uint32_t item_id = LE32(pkt->item_id);
    uint16_t port;

    /* See if the block selected is the "Ship Select" block */
    if (block == 0xFFFFFFFF) {
        return send_ship_list(c, ship, ship->cfg->menu_code);
    }

    /* Make sure the block selected is in range. */
    if(block > ship->cfg->blocks) {
        return -1;
    }

    /* Make sure that block is up and running. */
    if (ship->blocks[block - 1] == NULL || ship->blocks[block - 1]->run == 0) {
        return -2;
    }

    /* Redirect the client where we want them to go. */
    switch (c->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
        port = ship->blocks[block - 1]->dc_port;
        break;

    case CLIENT_VERSION_PC:
        port = ship->blocks[block - 1]->pc_port;
        break;

    case CLIENT_VERSION_GC:
        port = ship->blocks[block - 1]->gc_port;
        break;

    case CLIENT_VERSION_EP3:
        port = ship->blocks[block - 1]->ep3_port;
        break;

    case CLIENT_VERSION_BB:
        port = ship->blocks[block - 1]->bb_port;
        break;

    case CLIENT_VERSION_XBOX:
        port = ship->blocks[block - 1]->xb_port;
        break;

    default:
        return -3;
    }

#ifdef PSOCN_ENABLE_IPV6
    if (c->flags & CLIENT_FLAG_IPV6) {
        return send_redirect6(c, ship_ip6, port);
    }
    else {
        return send_redirect(c, ship_ip4, port);
    }
#else
    if (port > 0) {
        //char ip_str[64];
        //sprintf(ip_str, PRINT_IP_FORMAT, PRINT_HIP(ship_ip4));
        //printf("IP地址  %u is ip %s \n", ship_ip4, ip_str);
        //struct hostent* IP_host;
        ////printf("当前信息：%s\n", hostaddr);
        //IP_host = gethostbyname(host4);
        //uint8_t serverIP[4] = { 0 };
        //memcpy(&serverIP[0], IP_host->h_addr, 4);
        //printf("服务器IP: %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3]);
        ////IP地址  2052177884 is ip 122.81.191.220 实际应该是 220.191.81.122

        return send_redirect(c, ship_ip4, port);
    }
#endif
    return -1;
}

static int dc_process_menu(ship_client_t* c, dc_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    //DBG_LOG("dc_process_menu指令: 0x%04X item_id %d", menu_id & 0xFF, item_id);

    /* Figure out what the client is selecting. */
    switch (menu_id & 0xFF) {
        /* Blocks */
    case MENU_ID_BLOCK:
        return dc_process_block_sel(c, pkt);

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
                    return send_redirect6(c, i->ship_addr6,
                        i->ship_port + off);
                }
                else {
                    return send_redirect(c, i->ship_addr,
                        i->ship_port + off);
                }
#else
                return send_redirect(c, i->ship_addr, i->ship_port + off);
#endif
            }
        }

        /* We didn't find it, punt. */

        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return send_msg1(c, "%s",
            __(c, "\tE\tC4当前选择的舰船\n已离线."));
    }
    case ITEM_ID_DISCONNECT & 0xFF:
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    c->flags |= CLIENT_FLAG_DISCONNECTED;
    return -1;
}

static int bb_process_block_sel(ship_client_t* c, bb_select_pkt* pkt) {
    uint32_t block = LE32((int)pkt->item_id);
    uint32_t item_id = LE32(pkt->item_id);
    uint16_t port;

    //DBG_LOG("bb_process_block_sel指令: 0x%08X item_id %d", block & 0x000000FF, item_id);

    /* 查看所选舰仓是否为“舰船选择”的舰仓  */
    if (block == 0xFFFFFFFF) {
        return send_ship_list(c, ship, ship->cfg->menu_code);
    }

    /* 确保选定的舰仓在范围内. */
    if (block > ship->cfg->blocks) {
        return -1;
    }

    /* 确保该舰仓已启动并正在运行. */
    if (ship->blocks[block - 1] == NULL || ship->blocks[block - 1]->run == 0) {
        return -2;
    }

    /* 将客户端重定向到我们希望他们去的地方. */
    switch (c->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
        port = ship->blocks[block - 1]->dc_port;
        break;

    case CLIENT_VERSION_PC:
        port = ship->blocks[block - 1]->pc_port;
        break;

    case CLIENT_VERSION_GC:
        port = ship->blocks[block - 1]->gc_port;
        break;

    case CLIENT_VERSION_EP3:
        port = ship->blocks[block - 1]->ep3_port;
        break;

    case CLIENT_VERSION_BB:
        port = ship->blocks[block - 1]->bb_port;
        break;

    case CLIENT_VERSION_XBOX:
        port = ship->blocks[block - 1]->xb_port;
        break;

    default:
        return -3;
    }

#ifdef PSOCN_ENABLE_IPV6
    if (c->flags & CLIENT_FLAG_IPV6) {
        return send_redirect6(c, ship_ip6, port);
    }
    else {
        return send_redirect(c, ship_ip4, port);
    }
#else
    if (port > 0) {
        //char ip_str[64];
        //sprintf(ip_str, PRINT_IP_FORMAT, PRINT_HIP(ship_ip4));
        //printf("IP地址  %u is ip %s \n", ship_ip4, ip_str);
        //struct hostent* IP_host;
        ////printf("当前信息：%s\n", hostaddr);
        //IP_host = gethostbyname(host4);
        //uint8_t serverIP[4] = { 0 };
        //memcpy(&serverIP[0], IP_host->h_addr, 4);
        //printf("服务器IP: %u.%u.%u.%u\n", serverIP[0], serverIP[1], serverIP[2], serverIP[3]);
        ////IP地址  2052177884 is ip 122.81.191.220 实际应该是 220.191.81.122

        return send_redirect(c, ship_ip4, port);
    }
#endif
    return -1;
}

static int bb_process_menu(ship_client_t* c, bb_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    //DBG_LOG("bb_process_menu指令: 0x%08X item_id %d", menu_id & 0xFF, item_id);

    /* Figure out what the client is selecting. */
    switch (menu_id & 0xFF) {
        /* Blocks */
    case MENU_ID_BLOCK:
        /* See if it's the "Ship Select" entry */
        if (item_id == ITEM_ID_LAST) {
            return send_ship_list(c, ship, (uint16_t)(menu_id >> 8));
        }

        return bb_process_block_sel(c, pkt);

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
                    return send_redirect6(c, i->ship_addr6,
                        i->ship_port + off);
                }
                else {
                    return send_redirect(c, i->ship_addr,
                        i->ship_port + off);
                }
#else
                return send_redirect(c, i->ship_addr, i->ship_port + off);
#endif
            }
        }

        //printf("That ship is now menu_id & 0x000000FF = %u", menu_id & 0x000000FF);
        /* We didn't find it, punt. */
        return send_msg1(c, "%s",
            __(c, "\tE\tC4当前选择的舰船\n已离线."));
    }
    case ITEM_ID_DISCONNECT & 0xFF:
        DBG_LOG("断开连接");
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    return -1;
}

static int dc_process_info_req(ship_client_t* c, dc_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    /* What kind of information do they want? */
    switch (menu_id & 0xFF) {
        /* Block */
    case MENU_ID_BLOCK:
        if (item_id == -1)
            return 0;

        return block_info_reply(c, item_id);

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

            return send_info_reply(c,
                                   __(c, "\tE\tC4当前选择的舰船\n已离线."));
        }

    default:
        return -1;
    }
}

static int bb_process_info_req(ship_client_t* c, bb_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    //DBG_LOG("bb_process_info_req指令: 0x%04X item_id %d", menu_id & 0xFF, item_id);

    /* What kind of information do they want? */
    switch (menu_id & 0xFF) {
        /* Block */
    case MENU_ID_BLOCK:
        if(item_id == -1)
            return 0;

        return block_info_reply(c, item_id);

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

        return send_info_reply(c,
            __(c, "\tE\tC4当前选择的舰船\n已离线."));
    }

    default:
        return -1;
    }
}

static int dc_process_pkt(ship_client_t* c, uint8_t* pkt) {
    uint8_t type;
    uint16_t len;
    dc_pkt_hdr_t* dc = (dc_pkt_hdr_t*)pkt;
    pc_pkt_hdr_t* pc = (pc_pkt_hdr_t*)pkt;

    if (c->version == CLIENT_VERSION_DCV1 || c->version == CLIENT_VERSION_DCV2 ||
        c->version == CLIENT_VERSION_GC || c->version == CLIENT_VERSION_EP3 ||
        c->version == CLIENT_VERSION_XBOX) {
        type = dc->pkt_type;
        len = LE16(dc->pkt_len);
    }
    else {
        type = pc->pkt_type;
        len = LE16(pc->pkt_len);
    }

    switch (type) {
    case PING_TYPE:
        /* Ignore these. */
        return 0;

    case LOGIN_8B_TYPE:
        return dcnte_process_login(c, (dcnte_login_8b_pkt*)pkt);

    case LOGIN_93_TYPE:
        return dc_process_login(c, (dc_login_93_pkt*)pkt);

    case MENU_SELECT_TYPE:
        return dc_process_menu(c, (dc_select_pkt*)pkt);

    case INFO_REQUEST_TYPE:
        return dc_process_info_req(c, (dc_select_pkt*)pkt);

    case LOGIN_9D_TYPE:
        return dcv2_process_login(c, (dcv2_login_9d_pkt*)pkt);

    case LOGIN_9E_TYPE:
        if (c->version != CLIENT_VERSION_XBOX)
            return gc_process_login(c, (gc_login_9e_pkt*)pkt);
        else
            return xb_process_login(c, (xb_login_9e_pkt*)pkt);

    case GC_MSG_BOX_CLOSED_TYPE:
        return send_block_list(c, ship);

    case GAME_COMMAND0_TYPE:
        print_payload((unsigned char*)pkt, len);
        /* Ignore these, since taking screenshots on PSOPC generates them
           for some reason. */
        return 0;

    default:
        if (!script_execute_pkt(ScriptActionUnknownShipPacket, c, pkt,
            len)) {
            ERR_LOG("未知数据包!");
            print_payload((unsigned char*)pkt, len);
            return -3;
        }
        return 0;
    }
}

static int bb_process_pkt(ship_client_t* c, uint8_t* pkt) {
    bb_pkt_hdr_t* hdr = (bb_pkt_hdr_t*)pkt;
    uint16_t type = LE16(hdr->pkt_type);
    uint16_t len = LE16(hdr->pkt_len);

    //DBG_LOG("舰船：处理BB数据 指令 = 0x%04X %s 长度 = %d 字节 GC = %u", type, c_cmd_name(type, 0), len, c->guildcard);

    //print_payload((unsigned char*)pkt, len);

    switch (type) {
        /* 0x0005 5*/
    case BURSTING_TYPE:
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;

        /* 0x0009 9*/
    case INFO_REQUEST_TYPE:
        return bb_process_info_req(c, (bb_select_pkt*)pkt);

        /* 0x0010 16*/
    case MENU_SELECT_TYPE:
        return bb_process_menu(c, (bb_select_pkt*)pkt);

        /* 0x001D 29*/
    case PING_TYPE:
        /* Ignore these. */
        c->last_message = time(NULL);
        return 0;

        /* 0x0060 96*/
    case GAME_COMMAND0_TYPE:
        /* Ignore these, since taking screenshots on PSOPC generates them
           for some reason. */
        UDONE_CPD(type,pkt);
        return 0;

        /* 0x0093 147*/
    case LOGIN_93_TYPE:
        return bb_process_login(c, (bb_login_93_pkt*)pkt);

        /* 0x00E7 231*/
    case BB_FULL_CHARACTER_TYPE:
        // Client sending character data... 客户端离线发送角色数据
        UDONE_CPD(type,pkt);
        return 0;

    default:
        UNK_CPD(type, pkt);
        if (!script_execute_pkt(ScriptActionUnknownShipPacket, c, pkt,
            len)) {
            ERR_LOG("未知数据包!");
            print_payload((unsigned char*)pkt, len);
            return -3;
        }
        return 0;
    }
}

int ship_process_pkt(ship_client_t* c, uint8_t* pkt) {
    //DBG_LOG("舰船 ：舰船处理数据端口 %d 版本 = %d GC = %u", c->sock, c->version, c->guildcard);
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

void ship_inc_clients(ship_t* s) {
    ++s->num_clients;
    shipgate_send_cnt(&s->sg, s->num_clients, s->num_games);
}

void ship_dec_clients(ship_t* s) {
    --s->num_clients;
    shipgate_send_cnt(&s->sg, s->num_clients, s->num_games);
}

void ship_inc_games(ship_t* s) {
    ++s->num_games;
    shipgate_send_cnt(&s->sg, s->num_clients, s->num_games);
}

void ship_dec_games(ship_t* s) {
    --s->num_games;
    shipgate_send_cnt(&s->sg, s->num_clients, s->num_games);
}

void ship_free_limits(ship_t* s) {
    pthread_rwlock_wrlock(&s->llock);
    ship_free_limits_ex(&s->all_limits);
    pthread_rwlock_unlock(&s->llock);
}

void ship_free_limits_ex(struct limits_queue* l) {
    limits_entry_t* it, * tmp;

    it = TAILQ_FIRST(l);

    while (it) {
        tmp = TAILQ_NEXT(it, qentry);
        psocn_free_limits(it->limits);
        free_safe(it->name);
        free_safe(it);
        it = tmp;
    }

    TAILQ_INIT(l);
}

psocn_limits_t* ship_lookup_limits(const char* name) {
    limits_entry_t* it;

    /* Not really efficient, but simple. */
    TAILQ_FOREACH(it, &ship->all_limits, qentry) {
        if (!strcmp(name, it->name))
            return it->limits;
    }

    return NULL;
}

#ifdef ENABLE_LUA

static int ship_name_lua(lua_State* l) {
    ship_t* sl;

    if (lua_islightuserdata(l, 1)) {
        sl = (ship_t*)lua_touserdata(l, 1);
        lua_pushstring(l, sl->cfg->name);
    }
    else {
        lua_pushstring(l, "");
    }

    return 1;
}

static int ship_getTable_lua(lua_State* l) {
    ship_t* sl;

    if (lua_islightuserdata(l, 1)) {
        sl = (ship_t*)lua_touserdata(l, 1);
        lua_rawgeti(l, LUA_REGISTRYINDEX, sl->script_ref);
    }
    else {
        lua_pushnil(l);
    }

    return 1;
}

static int ship_writeLog_lua(lua_State* l) {
    const char* s;

    if (lua_isstring(l, 1)) {
        s = (const char*)lua_tostring(l, 1);
        SHIPS_LOG("%s", s);
    }

    return 0;
}

static const luaL_Reg shiplib[] = {
    { "name", ship_name_lua },
    { "getTable", ship_getTable_lua },
    { "writeLog", ship_writeLog_lua },
    { NULL, NULL }
};

int ship_register_lua(lua_State* l) {
    luaL_newlib(l, shiplib);
    return 1;
}

#endif /* ENABLE_LUA */
