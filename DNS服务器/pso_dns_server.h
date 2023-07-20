/*
    梦幻之星中国 DNS服务器
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

#ifndef DNS_H
#define DNS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <Software_Defines.h>
#include "version.h"

#define DNS_SERVER_VERSION VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#ifndef _arch_dreamcast
#include <pwd.h>
#endif

/* I hate you, Windows. I REALLY hate you. */
typedef int SOCKET;
#define INVALID_SOCKET -1

#else
#include <WinSock_Defines.h>
#include <windows.h>
#include <psoconfig.h>
#include <f_logs.h>
//#include <winsock2.h>
//#include <ws2tcpip.h>

typedef u_long in_addr_t;

#ifdef _MSC_VER
//#pragma comment(lib, "ws2_32.lib")

#define MYWM_NOTIFYICON (WM_USER+2)
int32_t program_hidden = 1;
uint32_t window_hide_or_show = 1;
HWND consoleHwnd;

typedef int ssize_t;

#endif

#endif

/* If it hasn't been overridden at compile time, give a sane default for the
   configuration directory. */
#ifndef CONFIG_DIR
#if defined(_WIN32) && !defined(__CYGWIN__)
#define CONFIG_DIR "."
#elif defined(_arch_dreamcast)
#define CONFIG_DIR "/rd"
#else
#define CONFIG_DIR "/etc/sylverant"
#endif
#endif

#ifndef CONFIG_FILE
#define CONFIG_FILE "Config\\psocn_config_dns.conf"
#endif

#define SOCKET_ERR(err, s) if(err==-1){perror(s);closesocket(err);return(-1);}

typedef struct dnsmsg {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
    uint8_t data[];
} dnsmsg_t;

#define QTYPE_A         1
#define DNS_PORT        53

typedef struct host_info {
    char* name;
    char* host4;
    char* host6;
    in_addr_t addr;
} host_info_t;

/* Input and output packet buffers. */
static uint8_t inbuf[1024];
static uint8_t outbuf[1024];

static dnsmsg_t* inmsg = (dnsmsg_t*)inbuf;
static dnsmsg_t* outmsg = (dnsmsg_t*)outbuf;

/* Read from the configuration. */
static host_info_t* hosts;
static int host_count;

#endif /* !DNS_H */