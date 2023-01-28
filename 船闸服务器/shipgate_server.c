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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <iconv.h>
#ifndef _WIN32
#include <pwd.h>
#include <grp.h>
#endif
#include <queue.h>
#include <signal.h>
#include <Software_Defines.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <WinSock_Defines.h>
#endif
#include <direct.h>

#include <gnutls/gnutls.h>

#include <mtwist.h>
#include <psoconfig.h>
#include <f_logs.h>
#include <f_iconv.h>
#include <debug.h>
#include <database.h>

#ifndef _WIN32
#if HAVE_LIBUTIL_H == 1
#include <libutil.h>
#elif HAVE_BSD_LIBUTIL_H == 1
#include <bsd/libutil.h>
#else
/* From pidfile.c */
struct pidfh;
struct pidfh *pidfile_open(const char *path, mode_t mode, pid_t *pidptr);
int pidfile_write(struct pidfh *pfh);
int pidfile_remove(struct pidfh *pfh);
#endif
#endif

#include "shipgate.h"
#include "ship.h"
#include "scripts.h"
#include "packets.h"

#ifndef _WIN32
#ifndef PID_DIR
#define PID_DIR "/var/run"
#endif
#endif

#ifndef RUNAS_DEFAULT
#define RUNAS_DEFAULT "sylverant"
#endif

#define MYWM_NOTIFYICON (WM_USER+2)
int32_t program_hidden = 1;
HWND consoleHwnd;
HWND backupHwnd;
uint32_t window_hide_or_show = 1;

/* Storage for our list of ships. */
struct ship_queue ships = TAILQ_HEAD_INITIALIZER(ships);

/* Configuration/database connections. */
psocn_config_t* cfg;
psocn_dbconfig_t* dbcfg;
psocn_dbconn_t conn;

/* GnuTLS 加密数据交换... */
gnutls_anon_server_credentials_t anoncred;
//gnutls_certificate_credentials_t tls_cred;
gnutls_priority_t tls_prio;
//static gnutls_dh_params_t dh_params;
//static gnutls_sec_param_t dh_params;

static volatile sig_atomic_t shutting_down = 0;
static volatile sig_atomic_t resend_scripts = 0;

/* Events... */
uint32_t event_count;
monster_event_t *events;

static const char *config_file = NULL;
static const char *custom_dir = NULL;
static int dont_daemonize = 0;
static const char *pidfile_name = NULL;
static struct pidfh *pf = NULL;
static const char *runas_user = RUNAS_DEFAULT;

extern ship_script_t *scripts;
extern uint32_t script_count;

const void* my_ntop(struct sockaddr_storage* addr, char str[INET6_ADDRSTRLEN]);

/* Print information about this program to stdout. */
static void print_program_info() {
    printf("%s version %s\n", server_name[SGATE_SERVER].name, SGATE_SERVER_VERSION);
    printf("Copyright (C) 2022 Sancaros\n\n");
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
static void print_help(const char* bin) {
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
static void parse_command_line(int argc, char* argv[]) {
    int i;

    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--version")) {
            load_program_info(server_name[SGATE_SERVER].name, SGATE_SERVER_VERSION);
            exit(EXIT_SUCCESS);
        }
        else if (!strcmp(argv[i], "--verbose")) {
            debug_set_threshold(DBG_LOGS);
        }
        else if (!strcmp(argv[i], "--quiet")) {
            debug_set_threshold(DBG_WARN);
        }
        else if (!strcmp(argv[i], "--reallyquiet")) {
            debug_set_threshold(DBG_ERROR);
        }
        else if (!strcmp(argv[i], "-C")) {
            if (i == argc - 1) {
                printf("-C 缺少参数!\n\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }

            /* Save the config file's name. */
            config_file = argv[++i];
        }
        else if (!strcmp(argv[i], "-D")) {
            if (i == argc - 1) {
                printf("-D 缺少参数!\n\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }

            /* Save the custom dir */
            custom_dir = argv[++i];
        }
        else if (!strcmp(argv[i], "--nodaemon")) {
            dont_daemonize = 1;
        }
        else if (!strcmp(argv[i], "-P")) {
            if (i == argc - 1) {
                printf("-P 缺少参数!\n\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }

            pidfile_name = argv[++i];
        }
        else if (!strcmp(argv[i], "-U")) {
            if (i == argc - 1) {
                printf("-U 缺少参数!\n\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);
            }

            runas_user = argv[++i];
        }
        else if (!strcmp(argv[i], "--help")) {
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        }
        else {
            printf("命令行参数非法: %s\n", argv[i]);
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

/* Load the configuration file and print out parameters with DBG_LOG. */
static psocn_config_t* load_config(void) {
    psocn_config_t* cfg;

    if (psocn_read_config(psocn_global_cfg, &cfg))
        ERR_EXIT("无法读取设置文件 %s", psocn_global_cfg);

    return cfg;
}

static int init_gnutls() {
    int rv;

    if (gnutls_check_version("3.1.4") == NULL) {
        fprintf(stderr, "GnuTLS 3.1.4 or later is required for this example\n");
        return -1;
    }

    /* Do the initial init */
    gnutls_global_init();

    /* Set up our credentials */
    if ((rv = gnutls_anon_allocate_server_credentials(&anoncred))) {
        ERR_LOG("无法为匿名 GnuTLS 分配内存: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    /* Set our priorities */
    if ((rv = gnutls_priority_init(&tls_prio, "PERFORMANCE:+ANON-ECDH:+ANON-DH", NULL))) {
        ERR_LOG("无法初始化 GnuTLS 优先权: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    //gnutls_certificate_set_dh_params(anoncred, dh_params);

    gnutls_anon_set_server_known_dh_params(anoncred, GNUTLS_SEC_PARAM_MEDIUM);

    return 0;
}

static void cleanup_gnutls() {
    //gnutls_dh_params_deinit(dh_params);
    gnutls_anon_free_server_credentials(anoncred);
    gnutls_priority_deinit(tls_prio);
    gnutls_global_deinit();
}

static void free_events() {
    uint32_t i;

    for (i = 0; i < event_count; ++i) {
        free_safe(events[i].monsters);
    }

    free_safe(events);
}

static int read_events_table() {
    void* result;
    char** row;
    long long row_count, i, row_count2, j;
    char query[256];

    SGATE_LOG("获取数据库节日事件");

    sprintf(query, "SELECT event_id, title, start_time, "
        "end_time, difficulties, versions, "
        "allow_quests FROM %s WHERE "
        "end_time > UNIX_TIMESTAMP() ORDER BY "
        "start_time", SERVER_EVENTS);

    if (psocn_db_real_query(&conn, query)) {
        ERR_LOG("无法从数据库中获取节日数据!");
        return -1;
    }

    if ((result = psocn_db_result_store(&conn)) == NULL) {
        ERR_LOG("无法存储节日数据选择的结果!");
        return -1;
    }

    /* Make sure we have something. */
    row_count = psocn_db_result_rows(result);
    if (row_count < 0) {
        ERR_LOG("无法获取节日数量!");
        psocn_db_result_free(result);
        return -1;
    }
    else if (!row_count) {
        ERR_LOG("节日数据库为空.");
        psocn_db_result_free(result);
        return 0;
    }

    /* Allocate space for the events... */
    events = (monster_event_t *)malloc((size_t)row_count * sizeof(monster_event_t));
    if (!events) {
        ERR_LOG("分配节日数据内存错误!");
        psocn_db_result_free(result);
        return -1;
    }

    memset(events, 0, (size_t)row_count * sizeof(monster_event_t));
    event_count = (uint32_t)row_count;

    /* Save each one. */
    i = 0;
    while ((row = psocn_db_result_fetch(result))) {
        /* This shouldn't ever happen... but whatever, doesn't hurt to check. */
        if (i >= row_count) {
            ERR_LOG("获得的结果行比预期的多?!");
            psocn_db_result_free(result);
            free_safe(events);
            return -1;
        }

        /* Copy the data over. */
        events[i].event_id = strtoul(row[0], NULL, 0);
        events[i].start_time = strtoul(row[2], NULL, 0);
        events[i].end_time = strtoul(row[3], NULL, 0);
        events[i].difficulties = (uint8_t)strtoul(row[4], NULL, 0);
        events[i].versions = (uint8_t)strtoul(row[5], NULL, 0);
        events[i].allow_quests = (uint8_t)strtoul(row[6], NULL, 0);

        if (!(events[i].event_title = _strdup(row[1]))) {
            ERR_LOG("复制事件标题时出错!");
            psocn_db_result_free(result);
            free_safe(events);
            return -1;
        }

        ++i;
    }

    psocn_db_result_free(result);

    if (i < row_count) {
        ERR_LOG("获得的结果行比预期的少?!");
        free_safe(events);
        return -1;
    }

    /* Run through each event and read the monster lists... */
    for (i = 0; i < row_count; ++i) {
        sprintf(query, "SELECT monster_type, episode FROM "
            "%s WHERE event_id= '%" PRIu32 "'",
			 SERVER_EVENT_MONSTER, events[i].event_id);

        if (psocn_db_real_query(&conn, query)) {
            ERR_LOG("Couldn't fetch monsters from database!");
            free_events();
            return -1;
        }

        if ((result = psocn_db_result_store(&conn)) == NULL) {
            ERR_LOG("Could not store results of monster select!");
            free_events();
            return -1;
        }

        /* Make sure we have something. */
        row_count2 = psocn_db_result_rows(result);
        events[i].monster_count = (uint32_t)row_count2;

        SGATE_LOG("节日事件 ID %" PRIu32 " (%s) - %" PRIu32 " 个敌人.",
            events[i].event_id, events[i].event_title,
            events[i].monster_count);

        if (row_count2 == 0) {
            psocn_db_result_free(result);
            continue;
        }
        /* This shouldn't happen, but check anyway... */
        else if (row_count2 < 0 || row_count2 > 256) {
            psocn_db_result_free(result);
            ERR_LOG("Got less than zero monsters in event?!");
            free_events();
            return -1;
        }

        /* Allocate space for the monsters... */
        events[i].monsters =
            (event_monster_t*)malloc((size_t)row_count2 * sizeof(event_monster_t));
        if (!events[i].monsters) {
            ERR_LOG("为敌人分配内存错误!");
            psocn_db_result_free(result);
            free_events();
            return -1;
        }

        memset(events[i].monsters, 0, (size_t)(row_count2 * sizeof(event_monster_t)));

        j = 0;
        while ((row = psocn_db_result_fetch(result))) {
            /* Once again, shouldn't happen, but check anyway... */
            if (j >= row_count2) {
                ERR_LOG("获得的结果行比预期的多?!");
                psocn_db_result_free(result);
                free_events();
                return -1;
            }

            events[i].monsters[j].monster = (uint16_t)strtoul(row[0], NULL, 0);
            events[i].monsters[j].episode = (uint8_t)strtoul(row[1], NULL, 0);
            ++j;
        }

        psocn_db_result_free(result);

        /* Same warning about it shouldn't happen applies here... */
        if (j < row_count2) {
            ERR_LOG("获得的结果行比预期的少?!");
            free_events();
            return -1;
        }
    }

    SGATE_LOG("成功读取 %" PRIu32 " 个节日事件.", event_count);

    return 0;
}

static void open_db() {
    char query[256];

    SGATE_LOG("初始化数据库连接");

    if (psocn_db_open(dbcfg, &conn)) {
        SQLERR_LOG("无法连接至数据库");
        exit(EXIT_FAILURE);
    }

    SGATE_LOG("初始化在线舰船数据表"/*, SERVER_SHIPS_ONLINE*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_SHIPS_ONLINE);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_SHIPS_ONLINE);
        exit(EXIT_FAILURE);
    }

    SGATE_LOG("初始化在线玩家数据表"/*, SERVER_CLIENTS_ONLINE*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_CLIENTS_ONLINE);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_CLIENTS_ONLINE);
        exit(EXIT_FAILURE);
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", AUTH_DATA_ACCOUNT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", AUTH_DATA_ACCOUNT);
        exit(EXIT_FAILURE);
    }

    SGATE_LOG("初始化临时玩家数据表"/*, SERVER_CLIENTS_TRANSIENT*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_CLIENTS_TRANSIENT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_CLIENTS_TRANSIENT);
        exit(EXIT_FAILURE);
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", CHARACTER_DATA);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", CHARACTER_DATA);
        exit(EXIT_FAILURE);
    }

    if (read_events_table()) {
        exit(EXIT_FAILURE);
    }
}

void run_server(int tsock, int tsock6) {
    int nfds;
    int select_result = 0;
#ifdef ENABLE_LUA
    uint32_t j;
#endif
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    int asock;
    socklen_t len;
    struct timeval timeout;
    fd_set readfds, writefds, exceptfds;
    ship_t *i, *tmp;
    ssize_t sent;
    time_t now;
    char ipstr[INET6_ADDRSTRLEN];

    for(;;) {
        /* Clear the fd_sets so we can use them. */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);
        nfds = 0;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        now = time(NULL);

        if (shutting_down) {
            SGATE_LOG("收到关闭信号");
            return;
        }

        /* Fill the sockets into the fd_set so we can use select below. */
        i = TAILQ_FIRST(&ships);
        while (i) {
            tmp = TAILQ_NEXT(i, qentry);

            /* If we haven't heard from a ship in 2 minutes, its dead.
               Disconnect it. */
            if(now > i->last_message + 120 && i->last_ping &&
               now > i->last_ping + 60) {
                destroy_connection(i);
                i = tmp;
                continue;
            }
            /* Otherwise, if we haven't heard from it in a minute, ping it. */
            else if(now > i->last_message + 60 && now > i->last_ping + 10) {
                send_ping(i, 0);
                i->last_ping = now;
            }

            FD_SET(i->sock, &readfds);


            if (i->sendbuf_cur) {
                FD_SET(i->sock, &writefds);
            }

            nfds = max(nfds, i->sock);

            /* Check GnuTLS' buffer for the connection. */
            if(gnutls_record_check_pending(i->session)) {
                if(handle_pkt(i)) {
                    i->disconnected = 1;
                }
            }

#ifdef ENABLE_LUA
            if (!i->disconnected && resend_scripts) {
                /* Send script check packets, if the ship supports scripting */
                if (i->proto_ver >= 16 && i->flags & LOGIN_FLAG_LUA) {
                    for (j = 0; j < script_count; ++j) {
                        send_script_check(i, &scripts[j]);
                    }
                }
            }
#endif

            i = tmp;
        }

        resend_scripts = 0;

        /* Add the main listening sockets to the read fd_set */
        if (tsock > -1) {
            FD_SET(tsock, &readfds);
            nfds = max(nfds, tsock);
        }

        if (tsock6 > -1) {
            FD_SET(tsock6, &readfds);
            nfds = max(nfds, tsock6);
        }

        if((select_result = select(nfds + 1, &readfds, &writefds, &exceptfds, &timeout)) > 0) {
            /* Check each ship's socket for activity. */
            TAILQ_FOREACH(i, &ships, qentry) {
                if (i->disconnected) {
                    continue;
                }

                /* Check if this ship was trying to send us anything. */
                if (FD_ISSET(i->sock, &readfds)) {
                    if(handle_pkt(i)) {
                        i->disconnected = 1;
                        continue;
                    }

                    i->last_ping = 0;
                }

                if (FD_ISSET(i->sock, &exceptfds)) {
                    ERR_LOG("客户端端口 %d 套接字异常", i->sock);
                    i->disconnected = 1;
                }

                /* If we have anything to write, check if we can. */
                if (FD_ISSET(i->sock, &writefds)) {
                    if (i->sendbuf_cur) {
                        sent = send(i->sock, i->sendbuf + i->sendbuf_start,
                            i->sendbuf_cur - i->sendbuf_start, 0);

                        /* If we fail to send, and the error isn't EAGAIN,
                           bail. */
                        if (sent == SOCKET_ERROR) {
                            if (errno != EAGAIN) {
                                i->disconnected = 1;
                            }
                        }
                        else {
                            i->sendbuf_start += sent;

                            /* If we've sent everything, free the buffer. */
                            if (i->sendbuf_start == i->sendbuf_cur) {
                                free_safe(i->sendbuf);
                                i->sendbuf = NULL;
                                i->sendbuf_cur = 0;
                                i->sendbuf_size = 0;
                                i->sendbuf_start = 0;
                            }
                        }
                    }
                }
            }

            /* Clean up any dead connections (its not safe to do a TAILQ_REMOVE
               in the middle of a TAILQ_FOREACH, and destroy_connection does
               indeed use TAILQ_REMOVE). */
            i = TAILQ_FIRST(&ships);
            while (i) {
                tmp = TAILQ_NEXT(i, qentry);

                if (i->disconnected)
                    destroy_connection(i);

                i = tmp;
            }

            /* Check the listening port to see if we have a ship. */
            if (tsock > -1 && FD_ISSET(tsock, &readfds)) {
                len = sizeof(struct sockaddr_in);

                if ((asock = accept(tsock, (struct sockaddr*)&addr,
                    &len)) < 0) {
                    perror("accept");
                    continue;
                }

                if (!create_connection_tls(asock, (struct sockaddr*)&addr,
                    len)) {
                    continue;
                }

                if (!inet_ntop(PF_INET, &addr.sin_addr, ipstr,
                    INET6_ADDRSTRLEN)) {
                    perror("inet_ntop");
                    continue;
                }

                SGATE_LOG("允许 TLS 舰船连接 IP:%s", ipstr);
            }

            /* If we have IPv6 support, check it too */
            if (tsock6 > -1 && FD_ISSET(tsock6, &readfds)) {
                len = sizeof(struct sockaddr_in6);

                if ((asock = accept(tsock6, (struct sockaddr*)&addr6,
                    &len)) < 0) {
                    perror("accept");
                    continue;
                }

                if (!create_connection_tls(asock, (struct sockaddr*)&addr6,
                    len)) {
                    continue;
                }

                if (!inet_ntop(PF_INET6, &addr6.sin6_addr, ipstr,
                    INET6_ADDRSTRLEN)) {
                    perror("inet_ntop");
                    continue;
                }

                SGATE_LOG("允许 TLS 舰船连接 IP:%s", ipstr);
            }
        } else if (select_result == -1) {
            ERR_LOG("select 套接字 = -1");
        }
    }
}

static void open_log() {
    FILE* dbgfp;

    errno_t err = fopen_s(&dbgfp, "Debug\\shipgate_debug.log", "a");

    if (err)
        ERR_EXIT("无法打开日志文件");

    debug_set_file(dbgfp);
}

static void reopen_log(void) {
    FILE* dbgfp;

    errno_t err = fopen_s(&dbgfp, "Debug\\shipgate_debug.log", "a");

    if (err)
        ERR_EXIT("无法打开日志文件");
    else{
        /* Swap out the file pointers and close the old one. */
        dbgfp = debug_set_file(dbgfp);
        fclose(dbgfp);
	}
}

static int open_sock(int family, uint16_t port) {
    int sock = SOCKET_ERROR, val = 1;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;

    /* 创建端口并监听. */
    sock = socket(family, SOCK_STREAM, IPPROTO_TCP);

    if (sock < 0) {
        SOCKET_ERR(sock, "创建端口监听");
        ERR_LOG("创建端口监听 %d:%d 出现错误", family, port);
    }

    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (PCHAR)&val, sizeof(int))) {
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
        if(setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (PCHAR)&val, sizeof(int))) {
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

void handle_signal(int signal) {
    switch (signal) {
#ifdef _WIN32 
    case SIGTERM:
        shutting_down = 1;
        break;
    case SIGABRT:
        shutting_down = 2;
        break;
    case SIGBREAK:
        cleanup_scripts();
        init_scripts();
        resend_scripts = 1;
        break;
#else 
    case SIGHUP:
        reopen_log();
        break;
#endif 
    case SIGINT:
        reopen_log();
        break;
    }
}

bool already_hooked_up;

void HookupHandler() {
    if (already_hooked_up) {
        /*SGATE_LOG(
            "Tried to hookup signal handlers more than once.");
        already_hooked_up = false;*/
    }
    else {
        SGATE_LOG(
            "%s启动完成.", server_name[SGATE_SERVER].name);
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
        SGATE_LOG(
            "Cannot install SIGHUP handler.");
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        SGATE_LOG(
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
            SGATE_LOG(
                "Cannot uninstall SIGHUP handler.");
        }
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            SGATE_LOG(
                "Cannot uninstall SIGINT handler.");
        }
#endif 

        already_hooked_up = false;
    }
}

const void* my_ntop(struct sockaddr_storage* addr, char str[INET6_ADDRSTRLEN]) {
    int family = addr->ss_family;

    switch (family) {
    case PF_INET:
    {
        struct sockaddr_in* a = (struct sockaddr_in*)addr;
        return inet_ntop(family, &a->sin_addr, str, INET6_ADDRSTRLEN);
    }

    case PF_INET6:
    {
        struct sockaddr_in6* a = (struct sockaddr_in6*)addr;
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

int __cdecl main(int argc, char** argv) {
    int tsock = SOCKET_ERROR, tsock6 = SOCKET_ERROR;
    WSADATA winsock_data;

    load_log_config();

    errno_t err = WSAStartup(MAKEWORD(2, 2), &winsock_data);

    if (err)
        ERR_EXIT("WSAStartup 错误...");

    HWND consoleHwnd;
    HWND backupHwnd;
    //NOTIFYICONDATA nid = { 0 };
    WNDCLASS wc = { 0 };
    HWND hwndWindow;
    //MSG msg;
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
        ERR_EXIT("注册类失败.");

    hwndWindow = CreateWindow("Sancaros", "hidden window", WS_MINIMIZE, 1, 1, 1, 1,
        NULL,
        NULL,
        hinst,
        NULL);

    backupHwnd = hwndWindow;

    if (!hwndWindow)
        ERR_EXIT("无法创建窗口.");

    ShowWindow(hwndWindow, SW_HIDE);
    UpdateWindow(hwndWindow);
    MoveWindow(consoleHwnd, 0, 510, 980, 510, SWP_SHOWWINDOW);	//把控制台拖到(100,100)
    ShowWindow(consoleHwnd, window_hide_or_show);
    UpdateWindow(consoleHwnd);

    server_name_num = SGATE_SERVER;

    load_program_info(server_name[SGATE_SERVER].name, SGATE_SERVER_VERSION);

    /* Parse the command line and read our configuration. */
    parse_command_line(argc, argv);

    cfg = load_config();

    open_log();

restart:
    shutting_down = 0;

    /* Initialize GnuTLS */
    if (init_gnutls(cfg))
        ERR_EXIT("无法设置 GnuTLS 证书");

    /* Initialize all the iconv contexts we'll need */
    if (init_iconv())
        ERR_EXIT("无法读取 iconv 参数设置");

    /* Install signal handlers */
    HookupHandler();
	
    /* Create the socket and listen for TLS connections. */
    tsock = open_sock(PF_INET, cfg->sgcfg.shipgate_port);
    tsock6 = open_sock(PF_INET6, cfg->sgcfg.shipgate_port);

    if (tsock == SOCKET_ERROR && tsock6 == SOCKET_ERROR)
        ERR_EXIT("无法创建 IPv4 或 IPv6 TLS 端口 tsock值 = %d tsock6值 = %d!", tsock, tsock6);

    init_scripts();

    load_guild_default_flag("System\\guild\\默认公会标志.flag");

    /* Clean up the DB now that we've done everything else that might fail... */
    open_db();

    SGATE_LOG("程序运行中...");
    SGATE_LOG("请用 <Ctrl-C> 关闭程序.");

    /* Run the shipgate server. */
    run_server(tsock, tsock6);

    /* Clean up. */
    closesocket(tsock);
    closesocket(tsock6);
    WSACleanup();
    cleanup_scripts();
    free_events();
    cleanup_iconv();
    psocn_db_close(&conn);
    cleanup_gnutls();
    psocn_free_config(cfg);
    psocn_free_db_config(dbcfg);
    UnhookHandler();

    /* Restart if we're supposed to be doing so. */
    if(shutting_down == 2) {
        load_config();
        goto restart;
    }

    return 0;
}
