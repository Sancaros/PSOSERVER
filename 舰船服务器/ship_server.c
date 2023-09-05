/*
    �λ�֮���й� ����������
    ��Ȩ (C) 2022, 2023 Sancaros

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
#include <time.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <tchar.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <direct.h>
#include <Windows.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <pthread.h>
#include <signal.h>
#include <Software_Defines.h>
#include <WinSock_Defines.h>
#include <windows.h>
#include <urlmon.h>
#include <SFMT.h>

#include <gnutls/gnutls.h>
#include <libxml/parser.h>

#include <psoconfig.h>
#include <f_logs.h>
#include <debug.h>
#include <f_iconv.h>
#include <pso_crash_handle.h>
#include <pso_StringReader.h>

#include "version.h"
#include "ship.h"
#include "clients.h"
#include "shipgate.h"
#include "utils.h"
#include "scripts.h"
#include "mapdata.h"
#include "ptdata.h"
#include "pmtdata.h"
#include "rtdata.h"
#include "shop.h"
#include "admin.h"
#include "smutdata.h"
#include "mageditdata.h"
#include "item_random.h"

#ifndef RUNAS_DEFAULT
#define RUNAS_DEFAULT "psocn"
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#define MYWM_NOTIFYICON (WM_USER+2)
int32_t program_hidden = 1;
HWND consoleHwnd;
HWND hwndWindow;
WNDCLASS wc = { 0 };
uint32_t window_hide_or_show = 1;

//WSADATA��һ�����ݽṹ�������洢��WSAStartup�������ú󷵻ص�Windows sockets���ݣ�����Winsock.dllִ�е����ݡ���Ҫͷ�ļ�
static WSADATA winsock_data;

static int init_wsa(void) {

    //MAKEWORD�������ò�ͬ��Winsock�汾������MAKEWORD(2,2)���ǵ���2.2��
    WORD sockVersion = MAKEWORD(2, 2);//ʹ��winsocket2.2�汾

    //WSAStartup����������Ӧ�ó����DLL���õĵ�һ��Windows�׽��ֺ���
    //���Խ��г�ʼ�����������winsock�汾�����dll�Ƿ�һ�£��ɹ�����0
    errno_t err = WSAStartup(sockVersion, &winsock_data);

    if (err) return -1;

    return 0;
}

#endif

// 1024 lines which can carry 64 parameters each 1024�� ÿ�п��Դ��64������
uint32_t csv_lines = 0;
// Release RAM from loaded CSV  �Ӽ��ص�CSV���ͷ�RAM 
char* csv_params[1024][64];

/* The actual ship structures. */
ship_t* ship;
int enable_ipv6 = 1;
int restart_on_shutdown = 0;
char ship_host4[32];
char ship_host6[128];
uint32_t ship_ip4;
uint8_t ship_ip6[16];

/* TLS stuff */
gnutls_certificate_credentials_t tls_cred;
gnutls_priority_t tls_prio;
static gnutls_dh_params_t dh_params;

static const char* config_file = NULL;
static const char* custom_dir = NULL;
static int dont_daemonize = 0;
static int check_only = 0;
static const char* pidfile_name = NULL;
static struct pidfh *pf = NULL;
static const char* runas_user = RUNAS_DEFAULT;

/* Print help to the user to stdout. */
static void print_help(const char* bin) {
    printf("����˵��: %s [arguments]\n"
        "-----------------------------------------------------------------\n"
        "--version       Print version info and exit\n"
        "--verbose       Log many messages that might help debug a problem\n"
        "--quiet         Only log warning and error messages\n"
        "--reallyquiet   Only log error messages\n"
        "-C configfile   Use the specified configuration instead of the\n"
        "                default one.\n"
        "-D directory    Use the specified directory as the root\n"
        "--nodaemon      Don't daemonize\n"
#ifdef PSOCN_ENABLE_IPV6
        "--no-ipv6       Disable IPv6 support for incoming connections\n"
#endif
        "--check-config  Load and parse the configuration, but do not\n"
        "                actually start the ship server. This implies the\n"
        "                --nodaemon option as well.\n"
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
            load_program_info(server_name[SHIPS_SERVER].name, SHIPS_SERVER_VERSION);
            exit(0);
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
            /* Save the config file's name. */
            if (i == argc - 1) {
                print_help(argv[0]);
                ERR_EXIT("-C ȱ�ٲ���!");
            }

            config_file = argv[++i];
        }
        else if (!strcmp(argv[i], "-D")) {
            /* Save the custom dir */
            if (i == argc - 1) {
                print_help(argv[0]);
                ERR_EXIT("-D ȱ�ٲ���!");
            }

            custom_dir = argv[++i];
        }
        else if (!strcmp(argv[i], "--nodaemon")) {
            dont_daemonize = 1;
        }
        else if (!strcmp(argv[i], "--no-ipv6")) {
            enable_ipv6 = 0;
        }
        else if (!strcmp(argv[i], "--check-config")) {
            check_only = 1;
            dont_daemonize = 1;
        }
        else if (!strcmp(argv[i], "-P")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                ERR_EXIT("-P ȱ�ٲ���!");
            }

            pidfile_name = argv[++i];
        }
        else if (!strcmp(argv[i], "-U")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                ERR_EXIT("-U ȱ�ٲ���!");
            }

            runas_user = argv[++i];
        }
        else if (!strcmp(argv[i], "--help")) {
            print_help(argv[0]);
            ERR_EXIT("Illegal command line argument: %s\n", argv[i]);
        }
        else {
            print_help(argv[0]);
            ERR_EXIT("Illegal command line argument: %s\n", argv[i]);
        }
    }
}

/* Load the configuration file and print out parameters with DBG_LOG. */
static psocn_ship_t* load_config(void) {
    psocn_ship_t* cfg;

    if (psocn_read_ship_config(psocn_ship_cfg, &cfg)) {
        ERR_EXIT("�޷���ȡ�����ļ� %s", psocn_ship_cfg);
    }

    return cfg;
}

static int init_gnutls(psocn_ship_t* cfg) {
    int rv;

    /* Do the global init */
    gnutls_global_init();

    if (gnutls_check_version("3.8.0") == NULL) {
        ERR_LOG("GNUTLS *** ע��: GnuTLS 3.8.0 or later is required for this example");
        return -1;
    }

    /* Set up our credentials */
    if ((rv = gnutls_certificate_allocate_credentials(&tls_cred)) != GNUTLS_E_SUCCESS) {
        ERR_LOG("GNUTLS *** ע��: Cannot allocate GnuTLS credentials: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    gnutls_datum_t ca_cert = { 0 }, cert = { 0 }, key = { 0 };

    ca_cert.data = read_file_all(cfg->shipgate_ca, &ca_cert.size);

    if ((rv = gnutls_certificate_set_x509_trust_mem(tls_cred, &ca_cert,
                                                     GNUTLS_X509_FMT_PEM)) < 0) {
        ERR_LOG("GNUTLS *** ע��: Cannot set GnuTLS CA Certificate: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    cert.data = read_file_all(cfg->ship_cert, &cert.size);
    key.data = read_file_all(cfg->ship_key, &key.size);

    if ((rv = gnutls_certificate_set_x509_key_mem(tls_cred, &cert,
                                                   &key,
                                                   GNUTLS_X509_FMT_PEM))) {
        ERR_LOG("GNUTLS *** ע��: Cannot set GnuTLS key file: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    /* Generate Diffie-Hellman parameters */
    CONFIG_LOG("��������Diffie-Hellman����..."
        "���Ե�.");
    if ((rv = gnutls_dh_params_init(&dh_params))) {
        ERR_LOG("GNUTLS *** ע��: Cannot initialize GnuTLS DH parameters: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    if ((rv = gnutls_dh_params_generate2(dh_params, 1024))) {
        ERR_LOG("GNUTLS *** ע��: Cannot generate GnuTLS DH parameters: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    CONFIG_LOG("Gnutls *** Tls �ͻ����������!");

    /* Set our priorities */
    if ((rv = gnutls_priority_init(&tls_prio, "NORMAL:+COMP-DEFLATE", NULL))) {
        ERR_LOG("GNUTLS *** ע��: Cannot initialize GnuTLS priorities: %s (%s)",
            gnutls_strerror(rv), gnutls_strerror_name(rv));
        return -1;
    }

    /* Set the Diffie-Hellman parameters */
    gnutls_certificate_set_dh_params(tls_cred, dh_params);

    return 0;
}

static void cleanup_gnutls() {
    gnutls_dh_params_deinit(dh_params);
    gnutls_certificate_free_credentials(tls_cred);
    gnutls_priority_deinit(tls_prio);
    gnutls_global_deinit();
}

static int setup_addresses(psocn_ship_t* cfg) {
    struct addrinfo hints;
    struct addrinfo* server, * j;
    //char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in* addr4;
    struct sockaddr_in6* addr6;

    /* Clear the addresses */
    memcpy(ship_host4, cfg->ship_host4, sizeof(ship_host4));
    ship_host4[31] = 0;
    if (cfg->ship_host6 != NULL) {
        memcpy(ship_host6, cfg->ship_host6, sizeof(ship_host6));
        ship_host6[127] = 0;
    }
    ship_ip4 = 0;
    cfg->ship_ip4 = 0;
    memset(ship_ip6, 0, 16);
    memset(cfg->ship_ip6, 0, 16);

    CONFIG_LOG("��ȡ������ַ...");

    //CONFIG_LOG("���������ȡ: %s", host4);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ship_host4, "9000", &hints, &server)) {
        ERR_LOG("��Ч������ַ: %s", ship_host4);
        return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET) {
            addr4 = (struct sockaddr_in*)j->ai_addr;
            //inet_ntop(j->ai_family, &addr4->sin_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv4 ��ַ����: %s", ipstr);
            //    return -1;
            //}
            //else
                //SHIPS_LOG("    ��ȡ�� IPv4 ��ַ: %s", ipstr);
            cfg->ship_ip4 = ship_ip4 = addr4->sin_addr.s_addr;
        }
        else if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            //inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv6 ��ַ����: %s", ipstr);
            //    //return -1;
            //}
            //else
                //SHIPS_LOG("    ��ȡ�� IPv6 ��ַ: %s", ipstr);
            memcpy(ship_ip6, &addr6->sin6_addr, 16);
            memcpy(cfg->ship_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    /* Make sure we found at least an IPv4 address */
    if (!ship_ip4 || !cfg->ship_ip4) {
        ERR_LOG("�޷���ȡ��IPv4��ַ!");
        return -1;
    }

    /* If we don't have a separate IPv6 host set, we're done. */
    if (!ship_host6) {
        return 0;
    }

    /* Now try with IPv6 only */
    memset(ship_ip6, 0, 16);
    memset(cfg->ship_ip6, 0, 16);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ship_host6, "9000", &hints, &server)) {
        ERR_LOG("��Ч������ַ (v6): %s", ship_host6);
        //return -1;
    }

    for (j = server; j != NULL; j = j->ai_next) {
        if (j->ai_family == PF_INET6) {
            addr6 = (struct sockaddr_in6*)j->ai_addr;
            //inet_ntop(j->ai_family, &addr6->sin6_addr, ipstr, INET6_ADDRSTRLEN);
            //if (!check_ipaddr(ipstr)) {
            //    ERR_LOG("    IPv6 ��ַ����: %s", ipstr);
            //    //return -1;
            //}
            //else
                //SHIPS_LOG("    ��ȡ�� IPv6 ��ַ: %s", ipstr);
            memcpy(ship_ip6, &addr6->sin6_addr, 16);
            memcpy(cfg->ship_ip6, &addr6->sin6_addr, 16);
        }
    }

    freeaddrinfo(server);

    if (!ship_ip6[0]) {
        ERR_LOG("�޷���ȡ��IPv6��ַ (��������IPv6����)!");
        return -1;
    }

    return 0;
}

static void print_config(psocn_ship_t* cfg) {
    int i;

    /* Print out the configuration. */
    CONFIG_LOG("���ò���:");

    CONFIG_LOG("��բ����: %s", cfg->shipgate_host);
    CONFIG_LOG("��բ�˿�: %d", (int)cfg->shipgate_port);

    /* Print out the ship's information. */
    CONFIG_LOG("��������: %s", cfg->name);

    CONFIG_LOG("���� IPv4 ����: %s", cfg->ship_host4);

    if (cfg->ship_host6)
        CONFIG_LOG("���� IPv6 ����: %s", cfg->ship_host6);
    else
        CONFIG_LOG("���� IPv6 ����: �Զ����û򲻴���");

    CONFIG_LOG("�����˿�: %d", (int)cfg->base_port);
    CONFIG_LOG("��������: %d", cfg->blocks);
    CONFIG_LOG("Ĭ�ϴ�������: %d", cfg->events[0].lobby_event);
    CONFIG_LOG("Ĭ����Ϸ����: %d", cfg->events[0].game_event);

    if (cfg->event_count != 1) {
        for (i = 1; i < cfg->event_count; ++i) {
            CONFIG_LOG("���� (%d-%d �� %d-%d):",
                cfg->events[i].start_month, cfg->events[i].start_day,
                cfg->events[i].end_month, cfg->events[i].end_day);
            CONFIG_LOG("\t����: %d, ��Ϸ: %d",
                cfg->events[i].lobby_event, cfg->events[i].game_event);
        }
    }

    if (cfg->menu_code)
        CONFIG_LOG("�˵�: %c%c", (char)cfg->menu_code,
            (char)(cfg->menu_code >> 8));
    else
        CONFIG_LOG("�˵�: ���˵�");

    if (cfg->v2_map_dir)
        CONFIG_LOG("v2 ��ͼ·��: %s", cfg->v2_map_dir);

    if (cfg->gc_map_dir)
        CONFIG_LOG("GC ��ͼ·��: %s", cfg->bb_map_dir);

    if (cfg->bb_param_dir)
        CONFIG_LOG("BB ����·��: %s", cfg->bb_param_dir);

    if (cfg->v2_param_dir)
        CONFIG_LOG("v2 ����·��: %s", cfg->v2_param_dir);

    if (cfg->bb_map_dir)
        CONFIG_LOG("BB ��ͼ·��: %s", cfg->bb_map_dir);

    if (cfg->v2_ptdata_file)
        CONFIG_LOG("v2 ItemPT �ļ�: %s", cfg->v2_ptdata_file);

    if (cfg->gc_ptdata_file)
        CONFIG_LOG("GC ItemPT �ļ�: %s", cfg->gc_ptdata_file);

    if (cfg->bb_ptdata_file)
        CONFIG_LOG("BB ItemPT �ļ�: %s", cfg->bb_ptdata_file);

    if (cfg->v2_pmtdata_file)
        CONFIG_LOG("v2 ItemPMT �ļ�: %s", cfg->v2_pmtdata_file);

    if (cfg->gc_pmtdata_file)
        CONFIG_LOG("GC ItemPMT �ļ�: %s", cfg->gc_pmtdata_file);

    if (cfg->bb_pmtdata_file)
        CONFIG_LOG("BB ItemPMT �ļ�: %s", cfg->bb_pmtdata_file);

    CONFIG_LOG("װ�� +/- ����: v2: %s, GC: %s, BB: %s",
        (cfg->local_flags & PSOCN_SHIP_PMT_LIMITV2) ? "true" : "false",
        (cfg->local_flags & PSOCN_SHIP_PMT_LIMITGC) ? "true" : "false",
        (cfg->local_flags & PSOCN_SHIP_PMT_LIMITBB) ? "true" : "false");

    if (cfg->v2_rtdata_file)
        CONFIG_LOG("v2 ItemRT �ļ�: %s", cfg->v2_rtdata_file);

    if (cfg->gc_rtdata_file)
        CONFIG_LOG("GC ItemRT �ļ�: %s", cfg->gc_rtdata_file);

    if (cfg->bb_rtdata_file)
        CONFIG_LOG("BB ItemRT �ļ�: %s", cfg->bb_rtdata_file);

    if (cfg->v2_rtdata_file || cfg->gc_rtdata_file || cfg->bb_rtdata_file) {
        CONFIG_LOG("����ϡ�е���: %s",
            (cfg->local_flags & PSOCN_SHIP_QUEST_RARES) ? "true" :
            "false");
        CONFIG_LOG("�����ϡ�е���: %s",
            (cfg->local_flags & PSOCN_SHIP_QUEST_SRARES) ? "true" :
            "false");
    }

    if (cfg->smutdata_file)
        CONFIG_LOG("Smutdata �ļ�: %s", cfg->smutdata_file);

    if (cfg->mageditdata_file)
        CONFIG_LOG("MagEditdata �ļ�: %s", cfg->mageditdata_file);

    if (cfg->limits_count) {
        CONFIG_LOG("������ %d /legit ���ļ�:", cfg->limits_count);

        for (i = 0; i < cfg->limits_count; ++i) {
            CONFIG_LOG("%d: \"%s\": %s", i, cfg->limits[i].name,
                cfg->limits[i].filename);
        }

        CONFIG_LOG("Ĭ�� /legit �ļ�����: %d", cfg->limits_default);
    }

    CONFIG_LOG("��բ��ʶ: 0x%08X", cfg->shipgate_flags);
    CONFIG_LOG("֧�ְ汾:");

    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NODCNTE))
        CONFIG_LOG("Dreamcast Network Trial Edition DC������԰�");
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOV1))
        CONFIG_LOG("Dreamcast Version 1 DC�汾 v1");
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOV2))
        CONFIG_LOG("Dreamcast Version 2 DC�汾 v2");
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOPCNTE))
        CONFIG_LOG("PSO for PC Network Trial Edition PC������԰�");
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOPC))
        CONFIG_LOG("PSO for PC PC�����");
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOEP12))
        CONFIG_LOG("Gamecube Episode I & II GC�½� 1/2");
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOEP3))
        CONFIG_LOG("Gamecube Episode III GC�½� 3");
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOPSOX))
        CONFIG_LOG("Xbox Episode I & II XBOX�½� 1/2");
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOBB))
        CONFIG_LOG("Blue Burst ��ɫ����");
}

static void open_log(psocn_ship_t* cfg) {
    char fn[64 + 32];
    FILE* dbgfp;

    sprintf(fn, "Debug\\%s_debug.log", cfg->name);
    errno_t err = fopen_s(&dbgfp, fn, "a");

    if (err) {
        perror("fopen");
        ERR_EXIT("�޷�����־�ļ�");
    }

    debug_set_file(dbgfp);
}

static void reopen_log(void) {
    char fn[64 + 32];
    FILE* dbgfp, * ofp;

    sprintf(fn, "Debug\\%s_debug.log", ship->cfg->name);
    errno_t err = fopen_s(&dbgfp, fn, "a");

    if (err) {
        /* Uhh... Welp, guess we'll try to continue writing to the old one,
           then... */
        perror("fopen");
        ERR_EXIT("�޷����´���־�ļ�");
    }
    else {
        ofp = debug_set_file(dbgfp);
        fclose(ofp);
    }
}

/* Install any handlers for signals we care about */
void handle_signal(int signal) {
    switch (signal) {
#ifdef _WIN32 
    case SIGTERM:
        /* Now, shutdown with slightly more grace! */
        schedule_shutdown(NULL, 0, 0, NULL);
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
        schedule_shutdown(NULL, 0, 1, NULL);
        break;
    }
}

bool already_hooked_up;

void HookupHandler() {
    if (already_hooked_up) {
        /*SHIPS_LOG(
            "Tried to hookup signal handlers more than once.");
        already_hooked_up = false;*/
    }
    else {
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
        SHIPS_LOG(
            "Cannot install SIGHUP handler.");
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        SHIPS_LOG(
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
            SHIPS_LOG(
                "Cannot uninstall SIGHUP handler.");
        }
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            SHIPS_LOG(
                "Cannot uninstall SIGINT handler.");
        }
#endif 

        already_hooked_up = false;
    }
}

//�����ص�ר��
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

void move_console_to_center()
{
    HWND consoleHwnd = GetConsoleWindow(); // ��ȡ����̨���ھ��

    // ��ȡ��Ļ�ߴ�
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // �������̨��������Ļ�����λ��
    int consoleWidth = 980; // ����̨���ڿ��
    int consoleHeight = 510; // ����̨���ڸ߶�
    int consoleX = (screenWidth - consoleWidth) / 2; // ����̨�������Ͻ� x ����
    int consoleY = (screenHeight - consoleHeight) / 2; // ����̨�������Ͻ� y ����

    // �ƶ�����̨���ڵ���Ļ����
    MoveWindow(consoleHwnd, consoleX, consoleY, consoleWidth, consoleHeight, TRUE);
}

/* ��ʼ�� */
static void initialization() {

    load_log_config();

#if defined(_WIN32) && !defined(__CYGWIN__)
    if (init_wsa()) {
        ERR_EXIT("WSAStartup ����...");
    }

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
        ERR_EXIT("ע����ʧ��.");

    hwndWindow = CreateWindow("Sancaros", "hidden window", WS_MINIMIZE, 1, 1, 1, 1,
        NULL,
        NULL,
        hinst,
        NULL);

    if (!hwndWindow)
        ERR_EXIT("�޷���������.");

    ShowWindow(hwndWindow, SW_HIDE);
    UpdateWindow(hwndWindow);
    MoveWindow(consoleHwnd, 900, 510, 980, 510, SWP_SHOWWINDOW);	//�ѿ���̨�ϵ�(100,100)
    ShowWindow(consoleHwnd, window_hide_or_show);
    UpdateWindow(consoleHwnd);

    // ���ñ���������
    SetUnhandledExceptionFilter(crash_handler);

#endif
}

int read_param_file(psocn_ship_t* cfg) {
    int rv;

    CONFIG_LOG("��ȡ v2 �����ļ�/////////////////////////");

    /* Try to read the v2 ItemPT data... */
    if (cfg->v2_ptdata_file) {
        CONFIG_LOG("��ȡ v2 ItemPT �ļ�: %s"
            , cfg->v2_ptdata_file);
        if (pt_read_v2(cfg->v2_ptdata_file)) {
            ERR_LOG("�޷���ȡ v2 ItemPT �ļ�: %s"
                ", ȡ��֧�� v2 �汾!", cfg->v2_ptdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOV2;
        }
    }
    else {
        ERR_LOG("δָ�� v2 ItemPT �ļ�"
            ", ȡ��֧�� v2 �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOV2;
    }

    /* Read the v2 ItemPMT file... */
    if (cfg->v2_pmtdata_file) {
        CONFIG_LOG("��ȡ v2 ItemPMT �ļ�: %s"
            , cfg->v2_pmtdata_file);
        if (pmt_read_v2(cfg->v2_pmtdata_file,
            !(cfg->local_flags & PSOCN_SHIP_PMT_LIMITV2))) {
            ERR_LOG("�޷���ȡ v2 ItemPMT �ļ�: %s"
                ", ȡ��֧�� v2 �汾!", cfg->v2_pmtdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOV2;
        }
    }
    else {
        ERR_LOG("δָ�� v2 ItemPMT �ļ�"
            ", ȡ��֧�� v2 �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOV2;
    }

    /* If we have a v2 map dir set, try to read the maps. */
    if (cfg->v2_map_dir) {
        CONFIG_LOG("��ȡ v2 map ����·��: %s"
            , cfg->v2_map_dir);

        if (v2_read_params(cfg) < 0) {
            ERR_LOG("�޷���ȡ v2 map ·��: %s"
                , cfg->v2_map_dir);
            printf("�� %d \n", __LINE__);//ERR_EXIT();
        }
    }

    /* Read the v2 ItemRT file... */
    if (cfg->v2_rtdata_file) {
        CONFIG_LOG("��ȡ v2 ItemRT �ļ�: %s"
            , cfg->v2_rtdata_file);
        if (rt_read_v2(cfg->v2_rtdata_file)) {
            ERR_LOG("�޷���ȡ v2 ItemRT �ļ�: %s"
                , cfg->v2_rtdata_file);
        }
    }

    CONFIG_LOG("��ȡ GameCube �����ļ�/////////////////////////");

    /* Read the GC ItemPT file... */
    if (cfg->gc_ptdata_file) {
        CONFIG_LOG("��ȡ GameCube ItemPT �ļ�: %s"
            , cfg->gc_ptdata_file);

        if (pt_read_v3(cfg->gc_ptdata_file)) {
            ERR_LOG("�޷���ȡ GameCube ItemPT �ļ�: %s"
                , cfg->gc_ptdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOEP12;
        }
    }
    else {
        ERR_LOG("δָ�� GC ItemPT �ļ�"
            ", ȡ��֧�� GameCube �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOEP12;
    }

    /* Read the GC ItemPMT file... */
    if (cfg->gc_pmtdata_file) {
        CONFIG_LOG("��ȡ GameCube ItemPMT �ļ�: %s"
            , cfg->gc_pmtdata_file);
        if (pmt_read_gc(cfg->gc_pmtdata_file,
            !(cfg->local_flags & PSOCN_SHIP_PMT_LIMITGC))) {
            ERR_LOG("�޷���ȡ GameCube ItemPMT �ļ�: %s"
                , cfg->gc_pmtdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOEP12;
        }
    }
    else {
        ERR_LOG("δָ�� GC ItemPMT �ļ�"
            ", ȡ��֧�� GameCube �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOEP12;
    }

    /* If we have a GC map dir set, try to read the maps. */
    if (cfg->gc_map_dir) {
        CONFIG_LOG("��ȡ GameCube map ����·��: %s"
            , cfg->gc_map_dir);

        if (gc_read_params(cfg) < 0)
            ERR_EXIT("�޷���ȡ GameCube map ·��: %s", cfg->v2_map_dir);
    }

    /* Read the GC ItemRT file... */
    if (cfg->gc_rtdata_file) {
        CONFIG_LOG("��ȡ GameCube ItemRT �ļ�: %s"
            , cfg->gc_rtdata_file);
        if (rt_read_gc(cfg->gc_rtdata_file)) {
            ERR_LOG("�޷���ȡ GameCube ItemRT �ļ�: %s"
                , cfg->gc_rtdata_file);
        }
    }

    CONFIG_LOG("��ȡ Blue Burst �����ļ�/////////////////////////");

    /* Read the BB ItemPT data, which is needed for Blue Burst... */
    if (cfg->bb_ptdata_file) {
        CONFIG_LOG("��ȡ Blue Burst ItemPT �ļ�: %s"
            , cfg->bb_ptdata_file);

        if (pt_read_bb(cfg->bb_ptdata_file)) {
            ERR_LOG("�޷���ȡ Blue Burst ItemPT �ļ�: %s"
                ", ȡ��֧�� Blue Burst �汾!", cfg->bb_ptdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
        }
    }
    else {
        ERR_LOG("δָ�� BB ItemPT �ļ�"
            ", ȡ��֧�� Blue Burst �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
    }

    /* Read the BB ItemPMT file... */
    if (cfg->bb_pmtdata_file) {
        CONFIG_LOG("��ȡ Blue Burst ItemPMT �ļ�: %s"
            , cfg->bb_pmtdata_file);
        if (pmt_read_bb(cfg->bb_pmtdata_file,
            !(cfg->local_flags & PSOCN_SHIP_PMT_LIMITBB))) {
            ERR_LOG("�޷���ȡ Blue Burst ItemPMT �ļ�: %s"
                , cfg->bb_pmtdata_file);
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
        }
    }
    else {
        ERR_LOG("δָ�� BB ItemPMT �ļ�"
            ", ȡ��֧�� Blue Burst �汾!");
        cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
    }

    /* If Blue Burst isn't disabled already, read the parameter data and map
       data... */
    if (!(cfg->shipgate_flags & SHIPGATE_FLAG_NOBB)) {
        rv = bb_read_params(cfg);

        /* Read the BB ItemRT file... */
        if (cfg->bb_rtdata_file) {
            CONFIG_LOG("��ȡ Blue Burst ItemRT �ļ�: %s", cfg->bb_rtdata_file);
            rv = rt_read_bb(cfg->bb_rtdata_file);
            if (rv) {
                ERR_LOG("�޷���ȡ Blue Burst ItemRT �ļ�: %s", cfg->bb_rtdata_file);
            }
        }

        /* Less than 0 = fatal error. Greater than 0 = Blue Burst problem. */
        if (rv > 0) {
            ERR_LOG("��ȡ Blue Burst �����ļ�����"
                ", ȡ��֧�� Blue Burst �汾!");
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
        }
        else if (rv < 0) {
            ERR_LOG("�޷���ȡ Blue Burst �����ļ�"
                ", ȡ��֧�� Blue Burst �汾!");
            cfg->shipgate_flags |= SHIPGATE_FLAG_NOBB;
        }

        CONFIG_LOG("��ȡ Mag Edit �����ļ�/////////////////////////");

        /* Read the Mag Edit file... */
        if (cfg->mageditdata_file) {
            CONFIG_LOG("��ȡ Mag Edit �ļ�: %s"
                , cfg->mageditdata_file);
            if (magedit_read_bb(cfg->mageditdata_file, 0)) {
                ERR_LOG("�޷���ȡ Mag Edit �ļ�: %s"
                    , cfg->mageditdata_file);
            }
        }
    }

    //load_ArmorRandomSet_data("System\\ItemRandom\\ArmorRandom_GC.rel");

    return 0;
}

int __cdecl main(int argc, char** argv) {
    void* tmp;
    psocn_ship_t* cfg;

    initialization();

    __try {

        server_name_num = SHIPS_SERVER;

        load_program_info(server_name[SHIPS_SERVER].name, SHIPS_SERVER_VERSION);

        /* Parse the command line... */
        parse_command_line(argc, argv);

        cfg = load_config();

        open_log(cfg);

    restart:
        print_config(cfg);

        /* Parse the addresses */
        if (setup_addresses(cfg))
            ERR_EXIT("��ȡ IP ��ַʧ��");

        /* Initialize GnuTLS stuff... */
        if (!check_only) {
            if (init_gnutls(cfg))
                ERR_EXIT("�޷����� GnuTLS ֤��");

            /* Set up things for clients to connect. */
            if (client_init(cfg))
                ERR_EXIT("�޷����� �ͻ��� ����");
        }

        if (0 != read_param_file(cfg))
            ERR_EXIT("�޷���ȡ��������");

        /* Set a few other shipgate flags, if appropriate. */
#ifdef ENABLE_LUA
        cfg->shipgate_flags |= LOGIN_FLAG_LUA;
#endif

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
        cfg->shipgate_flags |= LOGIN_FLAG_BE;
#endif

#if (SIZEOF_VOID_P == 4)
        cfg->shipgate_flags |= LOGIN_FLAG_32BIT;
#endif

        /* Initialize all the iconv contexts we'll need */
        if (init_iconv())
            ERR_EXIT("�޷���ȡ iconv ��������");

        /* Init mini18n if we have it */
        init_i18n();

        /* Init the word censor. */
        if (cfg->smutdata_file) {
            SHIPS_LOG("��ȡ smutdata �ļ�: %s", cfg->smutdata_file);
            if (smutdata_read(cfg->smutdata_file)) {
                ERR_LOG("�޷���ȡ smutdata �ļ�!");
            }
        }

        if (!check_only) {
            /* Install signal handlers */
            HookupHandler();

            /* Set up the ship and start it. */
            ship = ship_server_start(cfg);
            if (ship)
                pthread_join(ship->thd, NULL);

            /* Clean up... */
            tmp = pthread_getspecific(sendbuf_key);
            if (tmp) {
                free_safe(tmp);
                pthread_setspecific(sendbuf_key, NULL);
            }

            tmp = pthread_getspecific(recvbuf_key);
            if (tmp) {
                free_safe(tmp);
                pthread_setspecific(recvbuf_key, NULL);
            }
        }
        else {
            ship_check_cfg(cfg);
        }

        WSACleanup();
        smutdata_cleanup();
        cleanup_i18n();
        cleanup_iconv();

        if (!check_only) {
            client_shutdown();
            cleanup_gnutls();
        }

        psocn_free_ship_config(cfg);
        bb_free_params();
        v2_free_params();
        gc_free_params();
        pmt_cleanup();

        if (restart_on_shutdown) {
            cfg = load_config();
            goto restart;
        }

        xmlCleanupParser();

    }

    __except (crash_handler(GetExceptionInformation())) {
        // ������ִ���쳣�������߼��������ӡ������Ϣ���ṩ�û��Ѻõ���ʾ��

        ERR_LOG("���ִ���, �����˳�.");
        (void)getchar();
    }

    return 0;
}
