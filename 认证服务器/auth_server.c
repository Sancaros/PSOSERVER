/*
    梦幻之星中国 认证服务器
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

#include <mbstring.h>
#include <string.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <ctype.h>
#include <signal.h>
#include <Software_Defines.h>
#include <WinSock_Defines.h>
#include <windows.h>

#include <psoconfig.h>
#include <f_logs.h>
#include <f_iconv.h>
#include <f_checksum.h>
#include <pso_ping.h>

#include <database.h>
#include <encryption.h>
#include <mtwist.h>
#include <debug.h>
#include <quest.h>
#include <items.h>

#include "auth.h"
#include "auth_packets.h"
#include "patch.h"

#ifndef RUNAS_DEFAULT
#define RUNAS_DEFAULT "sylverant"
#endif

#define MYWM_NOTIFYICON (WM_USER+2)
//TODO 改为设置读取
int32_t program_hidden = 1;
uint32_t window_hide_or_show = 1;
HWND consoleHwnd;
HWND backupHwnd;

#ifndef ENABLE_IPV6
#define NUM_DCSOCKS  3
#define NUM_PCSOCKS  1
#define NUM_GCSOCKS  2
#define NUM_EP3SOCKS 4
#define NUM_WEBSOCKS 1
#define NUM_BBSOCKS  2
#define NUM_XBSOCKS  1
#else
#define NUM_DCSOCKS  6
#define NUM_PCSOCKS  2
#define NUM_GCSOCKS  4
#define NUM_EP3SOCKS 8
#define NUM_WEBSOCKS 2
#define NUM_BBSOCKS  4
#define NUM_XBSOCKS  2
#endif

static const int dcports[NUM_DCSOCKS][2] = {
    { PF_INET , 9200 },
    { PF_INET , 9201 },
    { PF_INET , 9000 },                 /* Dreamcast Network Trial Edition */
#ifdef ENABLE_IPV6
    { PF_INET6, 9200 },
    { PF_INET6, 9201 },
    { PF_INET6, 9000 }
#endif
};

static const int pcports[NUM_PCSOCKS][2] = {
    { PF_INET , 9300 },
#ifdef ENABLE_IPV6
    { PF_INET6, 9300 }
#endif
};

static const int gcports[NUM_GCSOCKS][2] = {
    { PF_INET , 9100 },
    { PF_INET , 9001 },
#ifdef ENABLE_IPV6
    { PF_INET6, 9100 },
    { PF_INET6, 9001 }
#endif
};

static const int ep3ports[NUM_EP3SOCKS][2] = {
    { PF_INET , 9103 },
    { PF_INET , 9003 },
    { PF_INET , 9203 },
    { PF_INET , 9002 },
#ifdef ENABLE_IPV6
    { PF_INET6, 9103 },
    { PF_INET6, 9003 },
    { PF_INET6, 9203 },
    { PF_INET6, 9002 }
#endif
};

static const int webports[NUM_WEBSOCKS][2] = {
    { PF_INET , 10003 },
#ifdef ENABLE_IPV6
    { PF_INET6, 10003 }
#endif
};

static const int bbports[NUM_BBSOCKS][2] = {
    { PF_INET , 12000 },
    { PF_INET , 12001 },
#ifdef ENABLE_IPV6
    { PF_INET6, 12000 },
    { PF_INET6, 12001 }
#endif
};

static const int xbports[NUM_XBSOCKS][2] = {
    { PF_INET , 9500 },
#ifdef ENABLE_IPV6
    { PF_INET6, 9500 }
#endif
};

/* Stuff read from the config files */
psocn_config_t *cfg;
psocn_srvconfig_t *srvcfg;
psocn_dbconfig_t *dbcfg;
psocn_dbconn_t conn;
psocn_limits_t *limits = NULL;
patch_list_t *patches_v2 = NULL;
patch_list_t *patches_gc = NULL;

psocn_quest_list_t qlist[CLIENT_VERSION_COUNT][CLIENT_LANG_ALL];
volatile sig_atomic_t shutting_down = 0;

static const char *config_file = NULL;
static const char *custom_dir = NULL;
static int dont_daemonize = 0;
static const char *runas_user = RUNAS_DEFAULT;

/* Print help to the user to stdout. */
static void print_help(const char *bin) {
    printf("帮助说明: %s [arguments]\n"
           "-----------------------------------------------------------------\n"
           "--version       Print version info and exit\n"
           "--verbose       Log many messages that might help debug a problem\n"
           "--quiet         Only log warning and error messages\n"
           "--reallyquiet   Only log error messages\n"
           "-C configfile   Use the specified configuration instead of the\n"
           "                default one.\n"
           "-D directory    Use the specified directory as the root\n"
           "--nodaemon      Don't daemonize\n"
           "-P filename     Use the specified name for the pid file to write\n"
           "                instead of the default.\n"
           "-U username     Run as the specified user instead of '%s'\n"
           "--help          Print this help and exit\n\n"
           "Note that if more than one verbosity level is specified, the last\n"
           "one specified will be used. The default is --verbose.\n", bin,
           RUNAS_DEFAULT);
}

/* Parse any command-line arguments passed in. */
static void parse_command_line(int argc, char *argv[]) {
    int i;

    for(i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "--version")) {
            load_program_info(AUTH_SERVER_NAME, AUTH_SERVER_VERSION);
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
        else if(!strcmp(argv[i], "-C")) {
            /* Save the config file's name. */
            if(i == argc - 1) {
                printf("-C 缺少参数!\n\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }

            config_file = argv[++i];
        }
        else if(!strcmp(argv[i], "-D")) {
            /* Save the custom dir */
            if(i == argc - 1) {
                printf("-D 缺少参数!\n\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }

            custom_dir = argv[++i];
        }
        else if(!strcmp(argv[i], "--nodaemon")) {
            dont_daemonize = 1;
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

/* 读取设置文件. */
static psocn_config_t* load_config(void) {
    psocn_config_t* cfg;

    AUTH_LOG("读取设置文件...");

    if (psocn_read_config(config_file, &cfg)) {
        ERR_LOG("无法读取设置文件!");
        exit(EXIT_FAILURE);
    }

    return cfg;
}

static psocn_srvconfig_t* load_srv_config(void) {
    psocn_srvconfig_t* cfg;

    if (psocn_read_srv_config(config_file, &cfg)) {
        ERR_LOG("无法读取设置文件!");
        exit(EXIT_FAILURE);
    }

    return cfg;
}

//static psocn_dbconfig_t* load_db_config(void) {
//    psocn_dbconfig_t* cfg;
//
//    if (psocn_read_db_config(config_file, &cfg)) {
//        ERR_LOG("无法读取设置文件!");
//        exit(EXIT_FAILURE);
//    }
//
//    return cfg;
//}

void read_quests() {
    int i, j;
    char fn[512];
    psocn_quest_list_t tmp;
    static int read_quests = 0;

    AUTH_LOG("读取任务...");

    if (cfg->quests_dir && cfg->quests_dir[0]) {
        for (i = 0; i < CLIENT_VERSION_COUNT; ++i) {
            if (client_type[i]->ver_name[0] == 0)
                continue;

            for (j = 0; j < CLIENT_LANG_ALL; ++j) {
                sprintf(fn, "%s/%s/quests_%s.xml", cfg->quests_dir,
                    client_type[i]->ver_name, language_codes[j]);
                if (!psocn_quests_read(fn, &tmp)) {
                    AUTH_LOG("读取任务 %s 语言 %s", client_type[i]->ver_name,
                        language_codes[j]);
                }

                /* Cleanup and move the new stuff in place. */
                if (read_quests) {
                    AUTH_LOG("无法索引任务 %s 语言 %s",
                        client_type[i]->ver_name, language_codes[j]);
                    psocn_quests_destroy(&qlist[i][j]);
                }

                qlist[i][j] = tmp;
            }
        }
    }

    read_quests = 1;
}

static void load_patch_config() {
    char query[256];
    char* fn;
    char* pfn;
    int i;

    /* Attempt to read each quests file... */
    read_quests();

    /* Attempt to read the legit items list */
    if (cfg->limits_enforced != -1) {
        fn = cfg->limits[cfg->limits_enforced].filename;

        AUTH_LOG("读取强制限制文件 %s (名称: %s)...", fn,
            cfg->limits[cfg->limits_enforced].name);
        if (fn && psocn_read_limits(fn, &limits)) {
            ERR_LOG("无法读取指定的限制文件");
        }
    }

    /* Print out the rest... */
    for (i = 0; i < cfg->limits_count; ++i) {
        if (!cfg->limits[i].enforce) {
            AUTH_LOG("忽略非强制限制文件 %s (名称: %s)",
                cfg->limits[i].filename, cfg->limits[i].name);
        }
    }

    /* Read the Blue Burst param data */
    if (load_param_data())
        exit(EXIT_FAILURE);

    if (load_bb_char_data())
        exit(EXIT_FAILURE);

    /* Read patch lists. */
    if (cfg->patch_dir) {
        AUTH_LOG("实时补丁路径: %s", cfg->patch_dir);

        pfn = (char*)malloc(strlen(cfg->patch_dir) + 32);
        if (!pfn) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        /* Start with v2... */
        sprintf(pfn, "%s/v2/v2patches.xml", cfg->patch_dir);

        AUTH_LOG("读取 DCv2 补丁列表 '%s'...", pfn);
        if (patch_list_read(pfn, &patches_v2)) {
            AUTH_LOG("无法读取 DCv2 补丁列表");
            patches_v2 = NULL;
        }
        else {
            AUTH_LOG("找到 %" PRIu32 " 个补丁文件",
                patches_v2->patch_count);
        }

        sprintf(pfn, "%s/gc/gcpatches.xml", cfg->patch_dir);

        AUTH_LOG("读取 GameCube 补丁列表 '%s'...", pfn);
        if (patch_list_read(pfn, &patches_gc)) {
            AUTH_LOG("无法读取 GameCube 补丁列表");
            patches_gc = NULL;
        }
        else {
            AUTH_LOG("找到 %" PRIu32 " 个补丁文件",
                patches_gc->patch_count);
        }
    }

    AUTH_LOG("初始化数据库...");

    if (psocn_db_open(dbcfg, &conn)) {
        ERR_LOG("连接数据库失败");
        exit(EXIT_FAILURE);
    }

    AUTH_LOG("数据库连接成功...");

    AUTH_LOG("初始化在线玩家数据表 ...");

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", AUTH_DATA_ACCOUNT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", AUTH_DATA_ACCOUNT);
        exit(EXIT_FAILURE);
    }
}

static int setup_addresses(psocn_srvconfig_t* cfg) {
    struct addrinfo hints;
    struct addrinfo* server, * j;
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in* addr4;
    struct sockaddr_in6* addr6;

    //psocn_read_srv_config(config_file, &cfg);

    /* Clear the addresses */
    cfg->server_ip = 0;
    memset(cfg->server_ip6, 0, 16);

    AUTH_LOG("获取服务器IP...");

    //CONFIG_LOG("检测域名获取: %s", host4);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(cfg->host4, "12000", &hints, &server)) {
        ERR_LOG("无效 IPv4 域名: %s", cfg->host4);
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
                AUTH_LOG("    获取到 IPv4 地址: %s", ipstr);
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
                AUTH_LOG("    获取到 IPv6 地址: %s", ipstr);
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

    if (getaddrinfo(cfg->host6, "12000", &hints, &server)) {
        ERR_LOG("无效 IPv6 域名: %s", cfg->host6);
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
                AUTH_LOG("    获取到 IPv6 地址: %s", ipstr);
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

const void *my_ntop(struct sockaddr_storage *addr, char str[INET6_ADDRSTRLEN]) {
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

static void run_server(int dcsocks[NUM_DCSOCKS], int pcsocks[NUM_PCSOCKS],
                       int gcsocks[NUM_GCSOCKS], int websocks[NUM_WEBSOCKS],
                       int ep3socks[NUM_EP3SOCKS], int bbsocks[NUM_BBSOCKS],
                       int xbsocks[NUM_XBSOCKS]) {
    fd_set readfds, writefds, exceptfds;
    struct timeval timeout;
    socklen_t len;
    struct sockaddr_storage addr;
    struct sockaddr *addr_p = (struct sockaddr *)&addr;
    char ipstr[INET6_ADDRSTRLEN];
    int nfds, asock, j, type, auth;
    login_client_t *i, *tmp;
    ssize_t sent;

    int select_result = 0;
    uint32_t client_count = 0;
    uint32_t client_count_bb_char = 0;
    uint32_t client_count_bb_login = 0;
    uint32_t client_count_dc = 0;
    uint32_t client_count_pc = 0;
    uint32_t client_count_gc = 0;
    uint32_t client_count_ep3 = 0;
    uint32_t client_count_xbox = 0;

    for(;;) {
        /* Clear the fd_sets so we can use them. */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        timeout.tv_sec = 9001;
        timeout.tv_usec = 0;
        nfds = 0;
        client_count = 0;

        /* Fill the sockets into the fd_set so we can use select below. */
        TAILQ_FOREACH(i, &clients, qentry) {
            FD_SET(i->sock, &readfds);

            FD_SET(i->sock, &exceptfds);

            /* Only add to the writing fd_set if we have something to write. */
            if(i->sendbuf_cur) {
                FD_SET(i->sock, &writefds);
            }

            nfds = max(nfds, i->sock);
            ++client_count;
        }

        /* If we have a shutdown scheduled and nobody's connected, go ahead and
           do it. */
        if(!client_count && shutting_down) {
            AUTH_LOG("收到程序关闭信号.");
            return;
        }

        /* Add the listening sockets for incoming connections to the fd_set. */
        for(j = 0; j < NUM_DCSOCKS; ++j) {
            FD_SET(dcsocks[j], &readfds);
            nfds = max(nfds, dcsocks[j]);
        }

        for(j = 0; j < NUM_GCSOCKS; ++j) {
            FD_SET(gcsocks[j], &readfds);
            nfds = max(nfds, gcsocks[j]);
        }

        for(j = 0; j < NUM_EP3SOCKS; ++j) {
            FD_SET(ep3socks[j], &readfds);
            nfds = max(nfds, ep3socks[j]);
        }

        for(j = 0; j < NUM_PCSOCKS; ++j) {
            FD_SET(pcsocks[j], &readfds);
            nfds = max(nfds, pcsocks[j]);
        }

        for(j = 0; j < NUM_BBSOCKS; ++j) {
            FD_SET(bbsocks[j], &readfds);
            nfds = max(nfds, bbsocks[j]);
        }

        for(j = 0; j < NUM_XBSOCKS; ++j) {
            FD_SET(xbsocks[j], &readfds);
            nfds = max(nfds, xbsocks[j]);
        }

        for(j = 0; j < NUM_WEBSOCKS; ++j) {
            FD_SET(websocks[j], &readfds);
            nfds = max(nfds, websocks[j]);
        }

        if((select_result = select(nfds + 1, &readfds, &writefds, &exceptfds, &timeout)) > 0) {
            /* See if we have an incoming client. */
            for(j = 0; j < NUM_DCSOCKS; ++j) {
                if(FD_ISSET(dcsocks[j], &readfds)) {
                    len = sizeof(struct sockaddr_storage);

                    if((asock = accept(dcsocks[j], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    AUTH_LOG("允许 %s:%d DreamCast 客户端连接", ipstr, dcports[j][1]);

                    if(!create_connection(asock, CLIENT_VERSION_DC, addr_p, len,
                                          dcports[j][1])) {
                        closesocket(asock);
                    }
                    else {
                        ++client_count;
                        ++client_count_dc;
                        AUTH_LOG("总玩家数: %d DreamCast 玩家数量: %d", client_count, client_count_dc);
                    }
                }
            }

            for(j = 0; j < NUM_PCSOCKS; ++j) {
                if(FD_ISSET(pcsocks[j], &readfds)) {
                    len = sizeof(struct sockaddr_storage);

                    if((asock = accept(pcsocks[j], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    AUTH_LOG("允许 %s:%d PC 客户端连接", ipstr, pcports[j][1]);

                    if(!create_connection(asock, CLIENT_VERSION_PC, addr_p, len,
                                          pcports[j][1])) {
                        closesocket(asock);
                    }
                    else {
                        ++client_count;
                        ++client_count_pc;
                        AUTH_LOG("总玩家数: %d PC 玩家数量: %d", client_count, client_count_pc);
                    }
                }
            }

            for(j = 0; j < NUM_GCSOCKS; ++j) {
                if(FD_ISSET(gcsocks[j], &readfds)) {
                    len = sizeof(struct sockaddr_storage);

                    if((asock = accept(gcsocks[j], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    AUTH_LOG("允许 %s:%d GameCube 客户端连接", ipstr, gcports[j][1]);

                    if(!create_connection(asock, CLIENT_VERSION_GC, addr_p, len,
                                          gcports[j][1])) {
                        closesocket(asock);
                    }
                    else {
                        ++client_count;
                        ++client_count_gc;
                        AUTH_LOG("总玩家数: %d GameCube 玩家数量: %d", client_count, client_count_gc);
                    }
                }
            }

            for(j = 0; j < NUM_EP3SOCKS; ++j) {
                if(FD_ISSET(ep3socks[j], &readfds)) {
                    len = sizeof(struct sockaddr_storage);

                    if((asock = accept(ep3socks[j], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    AUTH_LOG("允许 %s:%d Episode 3 客户端连接", ipstr, ep3ports[j][1]);

                    if(!create_connection(asock, CLIENT_VERSION_EP3, addr_p,
                                          len, ep3ports[j][1])) {
                        closesocket(asock);
                    }
                    else {
                        ++client_count;
                        ++client_count_ep3;
                        AUTH_LOG("总玩家数: %d Episode 3 玩家数量: %d", client_count, client_count_ep3);
                    }
                }
            }

            for(j = 0; j < NUM_BBSOCKS; ++j) {
                if(FD_ISSET(bbsocks[j], &readfds)) {
                    len = sizeof(struct sockaddr_storage);

                    if((asock = accept(bbsocks[j], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    if(j & 1) {
                        type = CLIENT_VERSION_BB_CHARACTER;
                        auth = 1;
                    }
                    else {
                        type = CLIENT_VERSION_BB_LOGIN;
                        auth = 0;
                    }

                    AUTH_LOG("允许 %s:%d Blue Burst 客户端%s", ipstr, bbports[auth][1], auth ? "登录" : "连接");

                    if(!create_connection(asock, type, addr_p, len,
                                          bbports[auth][1])) {
                        closesocket(asock);
                    }
                    else {
                        if (auth) {
                            ++client_count_bb_char;
                            ++client_count;
                            AUTH_LOG("总玩家数: %d Blue Burst 玩家登录数量: %d", client_count, client_count_bb_char);
                        }
                        else {
                            ++client_count_bb_login;
                            ++client_count;
                            AUTH_LOG("总玩家数: %d Blue Burst 玩家连接数量: %d", client_count, client_count_bb_login);
                        }
                    }
                }
            }

            for(j = 0; j < NUM_XBSOCKS; ++j) {
                if(FD_ISSET(xbsocks[j], &readfds)) {
                    len = sizeof(struct sockaddr_storage);

                    if((asock = accept(xbsocks[j], addr_p, &len)) < 0) {
                        perror("accept");
                    }

                    my_ntop(&addr, ipstr);
                    AUTH_LOG("允许 %s:%d Xbox 客户端连接", ipstr, xbports[j][1]);

                    if(!create_connection(asock, CLIENT_VERSION_XBOX, addr_p,
                                          len, xbports[j][1])) {
                        closesocket(asock);
                    }
                    else {
                        ++client_count;
                        ++client_count_xbox;
                        AUTH_LOG("总玩家数: %d Xbox 玩家数量: %d", client_count, client_count_xbox);
                    }
                }
            }

            for(j = 0; j < NUM_WEBSOCKS; ++j) {
                if(FD_ISSET(websocks[j], &readfds)) {
                    len = sizeof(struct sockaddr_storage);

                    if((asock = accept(websocks[j], addr_p, &len)) < 0) {
                        perror("accept");
                    }
                    else {
                        /* Send the number of connected clients, and close the
                           socket. */
                        client_count = LE32(client_count);
                        send(asock, (PCHAR)&client_count, 4, 0);

                        client_count_bb_char = LE32(client_count_bb_char);
                        send(asock, (PCHAR)&client_count_bb_char, 4, 0);

                        client_count_bb_login = LE32(client_count_bb_login);
                        send(asock, (PCHAR)&client_count_bb_login, 4, 0);

                        client_count_dc = LE32(client_count_dc);
                        send(asock, (PCHAR)&client_count_dc, 4, 0);

                        client_count_pc = LE32(client_count_pc);
                        send(asock, (PCHAR)&client_count_pc, 4, 0);

                        client_count_gc = LE32(client_count_gc);
                        send(asock, (PCHAR)&client_count_gc, 4, 0);

                        client_count_ep3 = LE32(client_count_ep3);
                        send(asock, (PCHAR)&client_count_ep3, 4, 0);

                        client_count_xbox = LE32(client_count_xbox);
                        send(asock, (PCHAR)&client_count_xbox, 4, 0);
                        closesocket(asock);
                    }
                }
            }

            /* Handle the client connections, if any. */
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
                        if(sent == SOCKET_ERROR) {
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
                }
            }
        } else if (select_result == -1) {
            ERR_LOG("select 套接字 = -1");
        }

        /* Clean up any dead connections (its not safe to do a TAILQ_REMOVE in
           the middle of a TAILQ_FOREACH, and destroy_connection does indeed
           use TAILQ_REMOVE). */
        i = TAILQ_FIRST(&clients);
        while(i) {
            tmp = TAILQ_NEXT(i, qentry);

            if(i->disconnected) {
                my_ntop(&i->ip_addr, ipstr);

                if(!i->guildcard) {
                    AUTH_LOG("客户端 (%s:%d) 断开连接",
                          ipstr, i->sock);
                }
                else {
                    AUTH_LOG("客户端 %" PRIu32 " (%s:%d) %s", i->guildcard, ipstr, i->sock, i->auth ? "进入跃迁轨道" : "断开认证");
                }

                switch (i->version) {
                case CLIENT_VERSION_BB_LOGIN:
                    --client_count_bb_login;
                    break;
                case CLIENT_VERSION_BB_CHARACTER:
                    --client_count_bb_char;
                    break;
                case CLIENT_VERSION_DC:
                    --client_count_dc;
                    break;
                case CLIENT_VERSION_PC:
                    --client_count_pc;
                    break;
                case CLIENT_VERSION_GC:
                    --client_count_gc;
                    break;
                case CLIENT_VERSION_EP3:
                    --client_count_ep3;
                    break;
                case CLIENT_VERSION_XBOX:
                    --client_count_xbox;
                    break;
                }
                --client_count;

                destroy_connection(i);
            }

            i = tmp;
        }
    }
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

    val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (PCHAR)&val, sizeof(int))) {
        SOCKET_ERR(sock, "setsockopt SO_REUSEADDR");
        /* We can ignore this error, pretty much... its just a convenience thing */
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

static void open_log(void) {
    FILE *dbgfp;

    errno_t err = fopen_s(&dbgfp, "Debug\\login_debug.log", "a");

    if(err) {
        ERR_LOG("无法打开日志文件");
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    debug_set_file(dbgfp);
}

static void reopen_log(void) {
    FILE *dbgfp, *ofp;

    errno_t err = fopen_s(&dbgfp, "Debug\\login_debug.log", "a");

    if (err) {
        /* Uhh... Welp, guess we'll try to continue writing to the old one,
           then... */
        ERR_LOG("无法打开日志文件");
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
    case SIGABRT:
    case SIGBREAK:
#else 
    case SIGHUP:
        reopen_log();
#endif 
        shutting_down = 1;
        break;
    case SIGINT:
        if (shutting_down) {
            shutting_down = 2;
        }
        break;
    }
}

bool already_hooked_up;

void HookupHandler() {
    if (already_hooked_up) {
        /*ERR_LOG("Tried to hookup signal handlers more than once.");
        already_hooked_up = false;*/
    }
    else {
        AUTH_LOG("%s启动完成.", AUTH_SERVER_NAME);
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
        ERR_LOG(
            "Cannot install SIGHUP handler.");
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        ERR_LOG(
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
            ERR_LOG(
                "Cannot uninstall SIGHUP handler.");
        }
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            ERR_LOG(
                "Cannot uninstall SIGINT handler.");
        }
#endif 

        already_hooked_up = false;
    }
}

int __cdecl main(int argc, char** argv) {
    int i, j;
    int dcsocks[NUM_DCSOCKS];
    int pcsocks[NUM_PCSOCKS];
    int gcsocks[NUM_GCSOCKS];
    int ep3socks[NUM_EP3SOCKS];
    int bbsocks[NUM_BBSOCKS];
    int xbsocks[NUM_XBSOCKS];
    int websocks[NUM_WEBSOCKS];
    WSADATA winsock_data;

    load_log_config();

    errno_t err = WSAStartup(MAKEWORD(2, 2), &winsock_data);
    if (err)
    {
        AUTH_LOG("WSAStartup 错误...");
        return 0;
    }

    HWND consoleHwnd;
    HWND backupHwnd;
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

    backupHwnd = hwndWindow;

    if (!hwndWindow)
    {
        printf("无法创建窗口.");
        exit(EXIT_FAILURE);
    }

    ShowWindow(hwndWindow, SW_HIDE);
    UpdateWindow(hwndWindow);
    MoveWindow(consoleHwnd, 900, 0, 980, 510, SWP_SHOWWINDOW);	//把控制台拖到(100,100)
    ShowWindow(consoleHwnd, window_hide_or_show);
    UpdateWindow(consoleHwnd);

    log_server_name_num = 1;

    load_program_info(AUTH_SERVER_NAME, AUTH_SERVER_VERSION);

    parse_command_line(argc, argv);

    cfg = load_config();

    srvcfg = load_srv_config();

    //dbcfg = load_db_config();

    open_log();

    if (setup_addresses(srvcfg))
        exit(EXIT_FAILURE);

restart:
    shutting_down = 0;
    load_patch_config();

    /* Initialize all the iconv contexts we'll need */
    if (init_iconv())
        exit(EXIT_FAILURE);

    /* Init mini18n if we have it. */
    init_i18n();

    HookupHandler();

    //AUTH_LOG("Opening Dreamcast ports for connections.");

    for(i = 0; i < NUM_DCSOCKS; ++i) {
        dcsocks[i] = open_sock(dcports[i][0], dcports[i][1]);

        if(dcsocks[i] < 0) {
            AUTH_LOG("监听 Dreamcast 端口 %u 失败.", dcports[i][1]);
            psocn_db_close(&conn);
            exit(EXIT_FAILURE);
        }
        else {
            AUTH_LOG("监听 Dreamcast 端口 %u 成功.", dcports[i][1]);
        }
    }

    //AUTH_LOG("Opening PSO for PC ports for connections.");

    for(i = 0; i < NUM_PCSOCKS; ++i) {
        pcsocks[i] = open_sock(pcports[i][0], pcports[i][1]);

        if(pcsocks[i] < 0) {
            AUTH_LOG("监听 PC 端口 %u 失败.", pcports[i][1]);
            psocn_db_close(&conn);
            exit(EXIT_FAILURE);
        }
        else {
            AUTH_LOG("监听 PC 端口 %u 成功.", pcports[i][1]);
        }
    }

    //AUTH_LOG("Opening PSO for Gamecube ports for connections.");

    for(i = 0; i < NUM_GCSOCKS; ++i) {
        gcsocks[i] = open_sock(gcports[i][0], gcports[i][1]);

        if(gcsocks[i] < 0) {
            AUTH_LOG("监听 Gamecube 端口 %u 失败.", gcports[i][1]);
            psocn_db_close(&conn);
            exit(EXIT_FAILURE);
        }
        else {
            AUTH_LOG("监听 Gamecube 端口 %u 成功.", gcports[i][1]);
        }
    }

    //AUTH_LOG("Opening PSO Episode 3 ports for connections.");

    for(i = 0; i < NUM_EP3SOCKS; ++i) {
        ep3socks[i] = open_sock(ep3ports[i][0], ep3ports[i][1]);

        if(ep3socks[i] < 0) {
            AUTH_LOG("监听 Episode 3 端口 %u 失败.", ep3ports[i][1]);
            psocn_db_close(&conn);
            exit(EXIT_FAILURE);
        }
        else {
            AUTH_LOG("监听 Episode 3 端口 %u 成功.", ep3ports[i][1]);
        }
    }

    //AUTH_LOG("Opening Blue Burst ports for connections.");

    for(i = 0; i < NUM_BBSOCKS; ++i) {
        bbsocks[i] = open_sock(bbports[i][0], bbports[i][1]);

        if(bbsocks[i] < 0) {
            AUTH_LOG("监听 Blue Burst 端口 %u 失败.", bbports[i][1]);
            psocn_db_close(&conn);
            exit(EXIT_FAILURE);
        }
        else {
            AUTH_LOG("监听 Blue Burst 端口 %u 成功.", bbports[i][1]);
        }
    }

    //AUTH_LOG("Opening Xbox ports for connections.");

    for(i = 0; i < NUM_XBSOCKS; ++i) {
        xbsocks[i] = open_sock(xbports[i][0], xbports[i][1]);

        if(xbsocks[i] < 0) {
            AUTH_LOG("监听 Xbox 端口 %u 失败.", xbports[i][1]);
            psocn_db_close(&conn);
            exit(EXIT_FAILURE);
        }
        else {
            AUTH_LOG("监听 Xbox 端口 %u 成功.", xbports[i][1]);
        }
    }

    //AUTH_LOG("Opening Web access ports for connections.");

    for(i = 0; i < NUM_WEBSOCKS; ++i) {
        websocks[i] = open_sock(webports[i][0], webports[i][1]);

        if(websocks[i] < 0) {
            AUTH_LOG("监听 网络数据 端口 %u 失败.", webports[i][1]);
            psocn_db_close(&conn);
            exit(EXIT_FAILURE);
        }
        else {
            AUTH_LOG("监听 网络数据 端口 %u 成功.", webports[i][1]);
        }
    }

    AUTH_LOG("程序运行中...");
    AUTH_LOG("请用 <Ctrl-C> 关闭程序.");

    /* Run the login server. */
    run_server(dcsocks, pcsocks, gcsocks, websocks, ep3socks, bbsocks, xbsocks);

    /* Clean up. */
    for(i = 0; i < NUM_DCSOCKS; ++i) {
        closesocket(dcsocks[i]);
    }

    for(i = 0; i < NUM_PCSOCKS; ++i) {
        closesocket(pcsocks[i]);
    }

    for(i = 0; i < NUM_GCSOCKS; ++i) {
        closesocket(gcsocks[i]);
    }

    for(i = 0; i < NUM_EP3SOCKS; ++i) {
        closesocket(ep3socks[i]);
    }

    for(i = 0; i < NUM_BBSOCKS; ++i) {
        closesocket(bbsocks[i]);
    }

    for(i = 0; i < NUM_XBSOCKS; ++i) {
        closesocket(xbsocks[i]);
    }

    for(i = 0; i < NUM_WEBSOCKS; ++i) {
        closesocket(websocks[i]);
    }

    WSACleanup();

    psocn_db_close(&conn);

    for(i = 0; i < CLIENT_VERSION_COUNT; ++i) {
        for(j = 0; j < CLIENT_LANG_ALL; ++j) {
            psocn_quests_destroy(&qlist[i][j]);
        }
    }

    patch_list_free(patches_v2);
    patches_v2 = NULL;

    patch_list_free(patches_gc);
    patches_gc = NULL;

    psocn_free_config(cfg);
    psocn_free_srv_config(srvcfg);
    psocn_free_db_config(dbcfg);
	cleanup_i18n();
    cleanup_iconv();

    /* Restart if we're supposed to be doing so. */
    if(shutting_down == 2) {
        load_config();
        UnhookHandler();
        goto restart;
    }

    return 0;
}
