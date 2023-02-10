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

/*  To give credit where credit is due... This program is based in large part
    upon information obtained by reading the source of Tethealla Patch Server
    (Sylverant started as a port of Tethealla to *nix). Tethealla Patch Server
    is 版权 (C) 2008 Terry Chatman Jr. and is also released under the
    GPLv3. This code however isn't directly started from that code, I wrote
    梦幻之星中国 补丁服务器 based on what I learned from reading the code, not
    from the code itself (I documented it (and studied PSOBB's responses to
    develop the documents fully), and based this on my documents). */

#include <mbstring.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <queue.h>
#include <Software_Defines.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <WinSock_Defines.h>
#endif
#include <urlmon.h>

#include <psoconfig.h>
#include <debug.h>
#include <mtwist.h>
#include <encryption.h>
#include <f_dirent.h>
#include <f_logs.h>
#include <f_iconv.h>
#include <f_checksum.h>
#include <pso_ping.h>

#ifndef _WIN32
#if HAVE_LIBUTIL_H == 1
#include <libutil.h>
#elif HAVE_BSD_LIBUTIL_H == 1
#include <bsd/libutil.h>
#else
    /* From pidfile.c */
struct pidfh;
struct pidfh* pidfile_open(const char* path, mode_t mode, pid_t* pidptr);
int pidfile_write(struct pidfh* pfh);
int pidfile_remove(struct pidfh* pfh);
#endif
#endif

#include "patch_packets.h"
#include "patch_server.h"

#ifndef _WIN32
#ifndef PID_DIR
#define PID_DIR "/var/run"
#endif
#endif

#ifndef RUNAS_DEFAULT
#define RUNAS_DEFAULT "psocn"
#endif

#define MYWM_NOTIFYICON (WM_USER+2)
int32_t program_hidden = 1;
HWND consoleHwnd;
uint32_t window_hide_or_show = 1;

struct client_queue clients = TAILQ_HEAD_INITIALIZER(clients);

patch_config_t *cfg;
psocn_srvconfig_t* srvcfg;
extern time_t srv_time;

static jmp_buf jmpbuf;
static volatile sig_atomic_t rehash = 0;
static volatile sig_atomic_t should_exit = 0;
static volatile sig_atomic_t canjump = 0;
static int dont_daemonize = 0;

static const char *pidfile_name = NULL;
static struct pidfh *pf = NULL;
static const char *runas_user = RUNAS_DEFAULT;

/* Forward declaration... */
static void rehash_files();

/* Print information about this program to stdout. */
static void print_program_info() {
    printf("%s version %s\n", PATCH_SERVER_VERSION, PATCH_SERVER_VERSION);
    printf("版权 (C) 2022 Sancaros\n\n");
    printf("This program is free software: you can redistribute it and/or\n"
        "modify it under the terms of the GNU Affero General Public\n"
        "License version 3 as published by the Free Software Foundation.\n\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n\n"
        "You should have received a copy of the GNU Affero General Public\n"
        "License along with this program.  If not, see\n"
        "<http://www.gnu.org/licenses/>.\n");
}

/* Print help to the user to stdout. */
static void print_help(const char *bin) {
    printf("帮助说明: %s [arguments]\n"
           "-----------------------------------------------------------------\n"
           "--version       Print version info and exit\n"
           "--verbose       Log many messages that might help debug a problem\n"
           "--quiet         Only log warning and error messages\n"
           "--reallyquiet   Only log error messages\n"
           "--nodaemon      Don't daemonize\n"
           "-P filename     Use the specified name for the pid file to write\n"
           "                instead of the default.\n"
           "-U username     Run as the specified user instead of '%s'\n"
           "--help          Print this help and exit\n\n"
           "Note that if more than one verbosity level is specified, the last\n"
           "one specified will be used. The default is --verbose.\n",
           bin, RUNAS_DEFAULT);
}

/* Parse any command-line arguments passed in. */
static void parse_command_line(int argc, char *argv[]) {
    int i;

    for(i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "--version")) {
            load_program_info(server_name[PATCH_SERVER].name, PATCH_SERVER_VERSION);
            exit(EXIT_SUCCESS);
        }
        else if(!strcmp(argv[i], "--verbose")) {
            debug_set_threshold(DBG_LOGS);
        }
        else if(!strcmp(argv[i], "--quiet")) {
            debug_set_threshold(DBG_WARN);
        }
        else if(!strcmp(argv[i], "--reallyquiet")) {
            debug_set_threshold(DBG_ERROR);
        }
        else if(!strcmp(argv[i], "--nodaemon")) {
            dont_daemonize = 1;
        }
        else if(!strcmp(argv[i], "-P")) {
            if(i == argc - 1) {
                printf("-P 缺少参数!\n\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }

            pidfile_name = argv[++i];
        }
        else if(!strcmp(argv[i], "-U")) {
            if(i == argc - 1) {
                printf("-U 缺少参数!\n\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }

            runas_user = argv[++i];
        }
        else if(!strcmp(argv[i], "--help")) {
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        }
        else {
            printf("Illegal command line argument: %s\n", argv[i]);
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

static int setup_addresses(psocn_srvconfig_t* cfg) {
    struct addrinfo hints;
    struct addrinfo* server, * j;
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in* addr4;
    struct sockaddr_in6* addr6;

    /* Clear the addresses */
    cfg->server_ip = 0;
    memset(cfg->server_ip6, 0, 16);

    PATCH_LOG("获取服务器IP...");

    //CONFIG_LOG("检测域名获取: %s", host4);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(cfg->host4, "11000", &hints, &server)) {
        ERR_LOG("无效 IPv4 地址: %s", cfg->host4);
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
                PATCH_LOG("    获取到 IPv4 地址: %s", ipstr);
            cfg->server_ip = addr4->sin_addr.s_addr;
        }
        else if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv6 地址错误: %s", ipstr);
            //    //return -1;
            //}
            //else
                PATCH_LOG("    获取到 IPv6 地址: %s", ipstr);
            memcpy(cfg->server_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    /* Make sure we found at least an IPv4 address */
    if (!cfg->server_ip) {
        ERR_LOG("无法获取IPv4地址!");
        return -1;
    }

    /* If we don't have a separate IPv6 host set, we're done. */
    if (!cfg->host6) {
        return 0;
    }

    /* Now try with IPv6 only */
    memset(cfg->server_ip6, 0, 16);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(cfg->host6, "11000", &hints, &server)) {
        ERR_LOG("无效IPv6地址: %s", cfg->host6);
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
                PATCH_LOG("    获取到 IPv6 地址: %s", ipstr);
            memcpy(cfg->server_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    if (!cfg->server_ip6[0]) {
        ERR_LOG("无法获取IPv6地址(但设置了IPv6域名)!");
        //return -1;
    }

    return 0;
}

/* Load the configuration file and print out parameters with DBG_LOG. */
static patch_config_t *load_config(void) {
    patch_config_t* cfg;

    if(patch_read_config(psocn_patch_cfg, &cfg)) {
        ERR_LOG("无法读取补丁设置文件!");
        exit(EXIT_FAILURE);
    }

    return cfg;
}

static psocn_srvconfig_t* load_srv_config(void) {
    psocn_srvconfig_t* cfg;

    if (psocn_read_srv_config(psocn_server_cfg, &cfg)) {
        ERR_LOG("无法载服务器设置文件!");
        exit(EXIT_FAILURE);
    }

    return cfg;
}

/* Read data from a client that is connected to either port. */
static int read_from_client(patch_client_t* c) {
    ssize_t sz;
    int pkt_sz = c->pkt_sz, pkt_cur = c->pkt_cur, rv = 0;
    pkt_header_t tmp_hdr = { 0 };

    if (!c->recvbuf) {
        /* Read in a new header... */
        if ((sz = recv(c->sock, (char*)&tmp_hdr, 4, 0)) < 4) {
            /* If we have an error, disconnect the client */
            if (sz <= 0) {
                //if (sz == SOCKET_ERROR)
                    //ERR_LOG("客户端接收数据错误: %s", strerror(errno));
                return -1;
            }

            /* Otherwise, its just not all there yet, so punt for now... */
            if (!(c->recvbuf = (unsigned char*)malloc(4))) {
                ERR_LOG("接收数据内存分配错误: %s", strerror(errno));
                return -1;
            }

            /* Copy over what we did get */
            memcpy(c->recvbuf, &tmp_hdr, sz);
            c->pkt_cur = sz;
            return 0;
        }
    }
    /* This case should be exceedingly rare... */
    else if (!pkt_sz) {
        /* Try to finish reading the header */
        if ((sz = recv(c->sock, (PCHAR)(c->recvbuf + pkt_cur), 4 - pkt_cur,
            0)) < 4 - pkt_cur) {
            /* If we have an error, disconnect the client */
            if (sz <= 0) {
                //if (sz == SOCKET_ERROR)
                    //ERR_LOG("客户端接收数据错误: %s", strerror(errno));
                return -1;
            }

            /* Update the pointer... */
            c->pkt_cur += sz;
            return 0;
        }

        /* We now have the whole header, so ready things for that */
        memcpy(&tmp_hdr, c->recvbuf, 4);
        c->pkt_cur = 0;
        free(c->recvbuf);
    }

    /* If we haven't decrypted the packet header, do so now, since we definitely
       have the whole thing at this point. */
    if (!pkt_sz) {
        CRYPT_CryptData(&c->client_cipher, &tmp_hdr, 4, 0);
        pkt_sz = LE16(tmp_hdr.pkt_len);
        sz = (pkt_sz & 0x0003) ? (pkt_sz & 0xFFFC) + 4 : pkt_sz;

        /* Allocate space for the packet */
        if (!(c->recvbuf = (unsigned char*)malloc(sz))) {
            ERR_LOG("接收数据内存分配错误: %s", strerror(errno));
            return -1;
        }

        c->pkt_sz = pkt_sz;
        memcpy(c->recvbuf, &tmp_hdr, 4);
        c->pkt_cur = 4;

        /* If this packet is only a header, short-circuit and process it now. */
        if (pkt_sz == 4)
            goto process;

        /* Return now, so we don't end up sleeping in the recv below. */
        return 0;
    }

    /* See if the rest of the packet is here... */
    if (c->recvbuf) {
        if ((sz = recv(c->sock, (PCHAR)(c->recvbuf + pkt_cur), pkt_sz - pkt_cur,
            0)) < pkt_sz - pkt_cur) {
            if (sz <= 0) {
                //if (sz == SOCKET_ERROR)
                    //ERR_LOG("客户端接收数据错误: %s", strerror(errno));
                return -1;
            }

            /* Didn't get it all, return for now... */
            c->pkt_cur += sz;
            return 0;
        }
    }

    /* If we get this far, we've got the whole packet, so process it. */
    CRYPT_CryptData(&c->client_cipher, c->recvbuf + 4, pkt_sz - 4, 0);

process:
    /* Pass it onto the correct handler. */
    if (c->type == CLIENT_TYPE_PC_PATCH || c->type == CLIENT_TYPE_BB_PATCH) {
        rv = process_patch_packet(c, c->recvbuf);
    }
    else if (c->type == CLIENT_TYPE_PC_DATA || c->type == CLIENT_TYPE_BB_DATA) {
        rv = process_data_packet(c, c->recvbuf);
    }

    free(c->recvbuf);
    c->recvbuf = NULL;
    c->pkt_cur = c->pkt_sz = 0;
    return rv;
}

const void *my_ntop(struct sockaddr_storage *addr,
                           char str[INET6_ADDRSTRLEN]) {
    int family = addr->ss_family;

    switch(family) {
        case PF_INET:
        {
            struct sockaddr_in *a = (struct sockaddr_in *)addr;
            return inet_ntop(family, &a->sin_addr, str, INET6_ADDRSTRLEN);
        }

        case PF_INET6:
        {
            struct sockaddr_in6 *a = (struct sockaddr_in6 *)addr;
            return inet_ntop(family, &a->sin6_addr, str, INET6_ADDRSTRLEN);
        }
    }

    return NULL;
}

//参数回调专用
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == MYWM_NOTIFYICON)
    {
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
            switch (wParam)
            {
            case 100:
                if (program_hidden)
                {
                    program_hidden = 0;
                    ShowWindow(consoleHwnd, SW_NORMAL);
                    SetForegroundWindow(consoleHwnd);
                    SetFocus(consoleHwnd);
                }
                else
                {
                    program_hidden = 1;
                    ShowWindow(consoleHwnd, SW_HIDE);
                }
                return TRUE;
                break;
            }
            break;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

/* Connection handling loop... */
static void run_server(int sockets[NUM_PORTS]) {
    fd_set readfds = { 0 }, writefds = { 0 }, exceptfds = { 0 };
    struct timeval timeout = { 0 };
    socklen_t len;
    struct sockaddr_storage addr = { 0 };
    struct sockaddr* addr_p = (struct sockaddr*)&addr;
    char ipstr[INET6_ADDRSTRLEN];
    int nfds, sock, j;
    patch_client_t* i, * tmp;
    ssize_t sent;
    int select_result = 0;
    int client_count = 0;

    while(!should_exit) {
        /* Set this up in case a signal comes in during the time between calling
           this and the select(). */
        if(!_setjmp(jmpbuf)) {
            canjump = 1;
        }

        /* If we need to, rehash the patches and welcome message. */
        if(rehash && client_count == 0) {
            canjump = 0;
            rehash_files();
            rehash = 0;
            canjump = 1;
        }

        /* Clear out the fd_sets and set the timeout value for select. */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        nfds = 0;

        //if (!check_ipaddr(int2ipstr(cfg->server_ip))) {
        //    setup_addresses(cfg);
        //}

        /* Set the timeout differently if we're waiting on a rehash. */
        if(!rehash) {
            timeout.tv_sec = 9001;
        }
        else {
            timeout.tv_sec = 10;
        }

        timeout.tv_usec = 0;

        /* Ping pong?! */
        srv_time = time(NULL);

        /* Fill the sockets into the fd_set so we can use select below. */
        TAILQ_FOREACH(i, &clients, qentry) {
            FD_SET(i->sock, &readfds);

            FD_SET(i->sock, &exceptfds);

            /* Only add to the write fd_set if we have something to send. */
            if(i->sendbuf_cur || i->sending_data) {
                FD_SET(i->sock, &writefds);
            }

            nfds = max(nfds, i->sock);
        }

        /* Add the listening sockets to the read fd_set if we aren't waiting on
           all clients to disconnect for a rehash operation. Since they won't be
           in the fd_set if we are waiting, we don't have to worry about clients
           connecting while we're trying to do a rehash operation. */
        if(!rehash) {
            for(j = 0; j < NUM_PORTS; ++j) {
                FD_SET(sockets[j], &readfds);
                nfds = max(nfds, sockets[j]);
            }
        }

        /* Wait to see if we get any incoming data. */
        if((select_result = select(nfds + 1, &readfds, &writefds, &exceptfds, &timeout)) > 0) {
            /* Make sure a rehash event doesn't interrupt any of this stuff,
               it will get handled the next time through the loop. */
            canjump = 0;

            /* Check the listening sockets first. */
            for(j = 0; j < NUM_PORTS; ++j) {
                if(FD_ISSET(sockets[j], &readfds)) {
                    len = sizeof(struct sockaddr_storage);

                    if((sock = accept(sockets[j], addr_p, &len)) < 0) {
                        perror("accept");
                    }
                    else {
                        if(ports[j][2] == CLIENT_TYPE_WEB) {
                            /* Send the number of connected clients, and close
                               the socket. */
                            nfds = LE32(client_count);
                            send(sock, (PCHAR)&nfds, 4, 0);
                            closesocket(sock);
                            continue;
                        }

                        if(!create_connection(sock, ports[j][2], addr_p, len)) {
                            closesocket(sock);
                        }
                        else {
                            my_ntop(&addr, ipstr);
                            if(ports[j][2] == CLIENT_TYPE_PC_PATCH ||
                               ports[j][2] == CLIENT_TYPE_BB_PATCH) {
                                if (ports[j][2] == CLIENT_TYPE_BB_PATCH) {
                                    PATCH_LOG("允许 %s:%d BB客户端获取补丁", ipstr, ports[j][1]);
                                }
                                else {
                                    PATCH_LOG("允许 %s:%d PC客户端获取补丁", ipstr, ports[j][1]);
                                }
                            }
                            else {
                                ++client_count;
                                PATCH_LOG("允许 %s:%d 客户端获取数据", ipstr, ports[j][1]);
                            }
                        }
                    }
                }
            }

            TAILQ_FOREACH(i, &clients, qentry) {
                /* Check if this connection was trying to send us something. */
                if(FD_ISSET(i->sock, &readfds)) {
                    if(read_from_client(i)) {
                        i->disconnected = 1;
                    }
                }

                if (FD_ISSET(i->sock, &exceptfds)) {
                    ERR_LOG("客户端端口 %d 套接字异常", i->sock);
                    i->disconnected = 1;
                }

                /* If we have anything to write, check if we can right now. */
                if(FD_ISSET(i->sock, &writefds)) {
                    if(i->sendbuf_cur) {
                        sent = send(i->sock, (PCHAR)i->sendbuf + i->sendbuf_start,
                                    i->sendbuf_cur - i->sendbuf_start, 0);

                        /* If we fail to send, and the error isn't EAGAIN,
                           bail. */
                        if(sent == -1) {
                            if(errno != EAGAIN) {
                                i->disconnected = 1;
                            }
                        }
                        else {
                            i->sendbuf_start += sent;

                            /* If we've sent everything, free the buffer. */
                            if(i->sendbuf_start == i->sendbuf_cur) {
                                free(i->sendbuf);
                                i->sendbuf = NULL;
                                i->sendbuf_cur = 0;
                                i->sendbuf_size = 0;
                                i->sendbuf_start = 0;
                            }
                        }
                    }
                    else if(i->sending_data) {
                        if(handle_list_done(i)) {
                            i->disconnected = 1;
                        }
                    }
                }
            }
        }
        else if (select_result == -1) {
            ERR_LOG("select 套接字 = -1");
        }

        /* Clean up any dead connections (its not safe to do a TAILQ_REMOVE in
           the middle of a TAILQ_FOREACH, and destroy_connection does indeed
           use TAILQ_REMOVE). */
        canjump = 0;
        i = TAILQ_FIRST(&clients);

        while(i) {
            tmp = TAILQ_NEXT(i, qentry);

            if(i->disconnected) {
                destroy_connection(i);

                --client_count;
            }

            i = tmp;
        }
    }
}

static void rehash_files() {
    CONFIG_LOG("重载设置文件...");
    cfg = load_config();
    CONFIG_LOG("重载设置文件...完成");
}

static int open_sock(int family, uint16_t port) {
    int sock = SOCKET_ERROR, val;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;

    /* 创建端口并监听. */
    sock = socket(family, SOCK_STREAM, IPPROTO_TCP);

    if (sock < 0) {
        SOCKET_ERR(sock, "创建端口监听");
        ERR_LOG("创建端口监听 %d:%d 出现错误", family, port);
    }

    /* Set SO_REUSEADDR so we don't run into issues when we kill the login
       server bring it back up quickly... */
    val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (PCHAR)&val, sizeof(int))) {
        SOCKET_ERR(sock, "setsockopt SO_REUSEADDR");
        /* We can ignore this error, pretty much... its just a convenience thing
           anyway... */
    }

    if (family == PF_INET) {
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = family;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
            SOCKET_ERR(sock, "绑定端口");
            ERR_LOG("绑定端口 %d:%d 出现错误", family, port);
        }

        if (listen(sock, 10)) {
            SOCKET_ERR(sock, "监听端口");
            ERR_LOG("监听端口 %d:%d 出现错误", family, port);
        }
    }
    else if (family == PF_INET6) {
        /* 单独支持IPV6. */
        val = 1;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (PCHAR)&val, sizeof(int))) {
            SOCKET_ERR(sock, "设置端口信息 IPV6_V6ONLY");
            ERR_LOG("设置端口信息IPV6_V6ONLY %d:%d 出现错误", family, port);
        }

        memset(&addr6, 0, sizeof(struct sockaddr_in6));
        addr6.sin6_family = family;
        addr6.sin6_addr = in6addr_any;
        addr6.sin6_port = htons(port);

        if (bind(sock, (struct sockaddr*)&addr6, sizeof(struct sockaddr_in6))) {
            SOCKET_ERR(sock, "绑定端口");
            ERR_LOG("绑定端口 %d:%d 出现错误", family, port);
        }

        if (listen(sock, 10)) {
            SOCKET_ERR(sock, "监听端口");
            ERR_LOG("监听端口 %d:%d 出现错误", family, port);
        }
    }
    else {
        ERR_LOG("打开端口 %d:%d 出现错误", family, port);
        closesocket(sock);
        return -1;
    }

    return sock;
}

static void open_log() {
    FILE *dbgfp;

    errno_t err = fopen_s(&dbgfp, "Debug\\patch_debug.log", "a");

    if(err) {
        ERR_LOG("无法打开日志文件");
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    debug_set_file(dbgfp);
}

static void reopen_log(void) {
    FILE *dbgfp, *ofp;

    errno_t err = fopen_s(&dbgfp, "Debug\\patch_debug.log", "a");

    if (err) {
        /* Uhh... Welp, guess we'll try to continue writing to the old one,
           then... */
        ERR_LOG("无法重新打开日志文件");
        perror("fopen");
    }
    else {
        ofp = debug_set_file(dbgfp);
        fclose(ofp);
    }
}

void handle_signal(int signal) {
    switch (signal) {
#ifdef _WIN32 
    case SIGTERM:
        should_exit = 1;
        break;
    case SIGABRT:
    case SIGBREAK:
        break;
#else 
    case SIGHUP:
        reopen_log();
        break;
#endif 
    case SIGINT:
        rehash = 1;

        if (canjump) {
            canjump = 0;
            longjmp(jmpbuf, 1);
        }
        break;
    }
}

bool already_hooked_up;

void HookupHandler() {
    if (already_hooked_up) {
        /*PATCH_LOG(
            "Tried to hookup signal handlers more than once.");
        already_hooked_up = false;*/
    }
    else {
        PATCH_LOG(
            "%s启动完成.", server_name[PATCH_SERVER].name);
        already_hooked_up = true;
    }
#ifdef _WIN32 
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGABRT, handle_signal);
#else 
    struct sigaction sa;
    // Setup the handler 
    sa.sa_handler = &handle_signal;
    // Restart the system call, if at all possible 
    sa.sa_flags = SA_RESTART;
    // Block every signal during the handler 
    sigfillset(&sa.sa_mask);
    // Intercept SIGHUP and SIGINT 
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        PATCH_LOG(
            "Cannot install SIGHUP handler.");
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        PATCH_LOG(
            "Cannot install SIGINT handler.");
    }
#endif 
}

void UnhookHandler() {
    if (already_hooked_up) {
#ifdef _WIN32 
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
#else 
        struct sigaction sa;
        // Setup the sighub handler 
        sa.sa_handler = SIG_DFL;
        // Restart the system call, if at all possible 
        sa.sa_flags = SA_RESTART;
        // Block every signal during the handler 
        sigfillset(&sa.sa_mask);
        // Intercept SIGHUP and SIGINT 
        if (sigaction(SIGHUP, &sa, NULL) == -1) {
            PATCH_LOG(
                "Cannot uninstall SIGHUP handler.");
        }
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            PATCH_LOG(
                "Cannot uninstall SIGINT handler.");
        }
#endif 

        already_hooked_up = false;
    }
}

int __cdecl main(int argc, char** argv) {
    int i;
    int sockets[NUM_PORTS] = { 0 };
    WSADATA winsock_data;
    //patch_config_t* cfg;

    load_log_config();

    errno_t err = WSAStartup(MAKEWORD(2, 2), &winsock_data);
    if (err)
    {
        ERR_LOG("WSAStartup 错误...");
        return 0;
    }

    HWND consoleHwnd;
    WNDCLASS wc = { 0 };
    HWND hwndWindow;
    HINSTANCE hinst = GetModuleHandle(NULL);
    consoleHwnd = GetConsoleWindow();

    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hIcon = LoadIcon(hinst, IDI_APPLICATION);
    wc.hCursor = LoadCursor(hinst, IDC_ARROW);
    wc.hInstance = hinst;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = "Sancaros";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClass(&wc))
    {
        printf("注册类失败.\n");
        exit(EXIT_FAILURE);
}

    hwndWindow = CreateWindow("Sancaros", "hidden window", WS_MINIMIZE, 1, 1, 1, 1,
        NULL,
        NULL,
        hinst,
        NULL);

    if (!hwndWindow)
    {
        printf("无法创建窗口.");
        exit(EXIT_FAILURE);
    }

    ShowWindow(hwndWindow, SW_HIDE);
    UpdateWindow(hwndWindow);
    MoveWindow(consoleHwnd, 0, 0, 980, 510, SWP_SHOWWINDOW);	//把控制台拖到(100,100)
    ShowWindow(consoleHwnd, window_hide_or_show);
    UpdateWindow(consoleHwnd);

    server_name_num = PATCH_SERVER;

    load_program_info(server_name[PATCH_SERVER].name, PATCH_SERVER_VERSION);

    /* 读取命令行并读取设置. */
    parse_command_line(argc, argv);

    cfg = load_config();

    srvcfg = load_srv_config();

    open_log();

    /* 获取地址 */
    if (setup_addresses(srvcfg))
        exit(EXIT_FAILURE);

    /* 读取随机32位种子. */
    init_genrand((uint32_t)time(NULL));

    /* Initialize all the iconv contexts we'll need */
    if (init_iconv())
        exit(EXIT_FAILURE);

    /* 读取信号监听. */
    HookupHandler();

    /* 开启所有监听端口 */
    for(i = 0; i < NUM_PORTS; ++i) {
        sockets[i] = open_sock(ports[i][0], ports[i][1]);

        if(sockets[i] < 0) {
            ERR_LOG("监听补丁端口 %d (%s) 错误, 程序退出",
                  ports[i][1], ports[i][0] == PF_INET ? "IPv4" : "IPv6");
            exit(EXIT_FAILURE);
        }
        else {
            PATCH_LOG("监听补丁端口 %d 成功.", ports[i][1]);
        }
    }

    PATCH_LOG("程序运行中...");
    PATCH_LOG("请用 <Ctrl-C> 关闭程序.");

    /* 进入主链接监听循环. */
    run_server(sockets);

    for (i = 0; i < NUM_PORTS; ++i) {
        closesocket(sockets[i]);
    }

    WSACleanup();
    cleanup_iconv();

    /* Clean up after ourselves... */
    if (cfg) {
        patch_free_config(cfg);
    }

    UnhookHandler();

    return 0;
}
