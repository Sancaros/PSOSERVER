/*
    梦幻之星中国 认证服务器
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

/*Note:
    使用此服务器, 需确保服务器的53端口不被占用, 且开启UDP 53端口支持接入
*/

#include <pso_crash_handle.h>

#include "pso_dns_server.h"

int host_line = 0;

#if defined(_WIN32) && !defined(__CYGWIN__)
HWND consoleHwnd;
HWND hwndWindow;
WNDCLASS wc = { 0 };

//WSADATA是一种数据结构，用来存储被WSAStartup函数调用后返回的Windows sockets数据，包含Winsock.dll执行的数据。需要头文件
static WSADATA winsock_data;

static int init_wsa(void) {

    //MAKEWORD声明调用不同的Winsock版本。例如MAKEWORD(2,2)就是调用2.2版
    WORD sockVersion = MAKEWORD(2, 2);//使用winsocket2.2版本

    //WSAStartup函数必须是应用程序或DLL调用的第一个Windows套接字函数
    //可以进行初始化操作，检测winsock版本与调用dll是否一致，成功返回0
    errno_t err = WSAStartup(sockVersion, &winsock_data);

    if (err) return -1;

    return 0;
}

/* Borrowed from my code in KallistiOS... */
static int inet_pton4(const char* src, void* dst) {
    int parts[4] = { 0 };
    int count = 0;
    struct in_addr* addr = (struct in_addr*)dst;

    for (; *src && count < 4; ++src) {
        if (*src == '.') {
            ++count;
        }
        /* Unlike inet_aton(), inet_pton() only supports decimal parts */
        else if (*src >= '0' && *src <= '9') {
            parts[count] *= 10;
            parts[count] += *src - '0';
        }
        else {
            /* Invalid digit, and not a dot... bail */
            return 0;
        }
    }

    if (count != 3) {
        /* Not the right number of parts, bail */
        return 0;
    }

    /* Validate each part, note that unlike inet_aton(), inet_pton() only
       supports the standard xxx.xxx.xxx.xxx addresses. */
    if (parts[0] > 0xFF || parts[1] > 0xFF ||
        parts[2] > 0xFF || parts[3] > 0xFF)
        return 0;

    addr->s_addr = htonl(parts[0] << 24 | parts[1] << 16 |
        parts[2] << 8 | parts[3]);

    return 1;
}

#define inet_pton(a, b, c) inet_pton4(b, c)

#define close(a) closesocket(a)

#endif

static int cmp_str(const char* s1, const char* s2) {
    int v;

    for (;;) {
        v = toupper(*s1) - toupper(*s2);

        if (v || !*s1)
            return v;

        ++s1;
        ++s2;
    }
}

int isIPAddress(const char* hostname) {
    struct sockaddr_in sa = { 0 };
    return inet_pton(AF_INET, hostname, &(sa.sin_addr)) != 0;
}

int isIPv6Address(const char* hostname) {
    struct sockaddr_in6 sa = { 0 };
    return InetPton(AF_INET6, hostname, &(sa.sin6_addr)) == 1;
}

static int get_IPv4_from_hostname(const char* hostname, char* ipv4Address) {
    struct addrinfo* result = NULL, * ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int status = getaddrinfo(hostname, NULL, &hints, &result);
    if (status != 0) {
#ifdef DEBUG

#ifdef _WIN32
        fprintf(stderr, "getaddrinfo 失败 错误码: %d\n", WSAGetLastError());
#else
        fprintf(stderr, "getaddrinfo 失败 错误码: %s\n", gai_strerror(status));
#endif

#endif // DEBUG

        WSACleanup();
        return -1;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
        char* ip = inet_ntoa(sockaddr_ipv4->sin_addr);
        strncpy(ipv4Address, ip, INET_ADDRSTRLEN);

        break; // Use the first IPv4 address found
    }

    freeaddrinfo(result);

    return 0;
}

int get_IPv6_from_hostname(const char* hostname, char* ipAddress, size_t ipAddressSize) {
    struct addrinfo hints;
    struct addrinfo* result, * p;
    int status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo(hostname, NULL, &hints, &result);
    if (status != 0) {
#ifdef DEBUG

#ifdef _WIN32
        fprintf(stderr, "getaddrinfo 失败 错误码: %d\n", WSAGetLastError());
#else
        fprintf(stderr, "getaddrinfo 失败 错误码: %s\n", gai_strerror(status));
#endif

#endif // DEBUG

        return -1;
    }

    for (p = result; p != NULL; p = p->ai_next) {
        void* addr;
        char ip[INET6_ADDRSTRLEN];

        if (p->ai_family == AF_INET6) {
            // 获取 IPv6 地址
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        else {
            continue;
        }

        // 将地址转换为可读的 IP 字符串
        inet_ntop(p->ai_family, addr, ip, sizeof(ip));

        // 复制 IP 地址到输出参数
        strncpy(ipAddress, ip, ipAddressSize);
        ipAddress[ipAddressSize - 1] = '\0';

        break;
    }

    freeaddrinfo(result);

    return 0;
}

static int read_config(const char* dir, const char* fn) {
    size_t dlen = strlen(dir), flen = strlen(fn);
    char* fullfn;
    FILE* fp;
    int entries = 0, lineno = 0;
    char linebuf[1024], name[1024], host4[1024], host6[1024], ipv4[INET_ADDRSTRLEN], ipv6[INET6_ADDRSTRLEN];
    in_addr_t addr;
    void* tmp;

    /* Curse you, Visual C++... */
    if (!(fullfn = (char*)malloc(dlen + flen + 2))) {
        ERR_LOG("malloc");
        return -1;
    }

    /* Build the filename we'll read from. */
    sprintf(fullfn, "%s/%s", dir, fn);

    /* Open the configuration file */
    if (!(fp = fopen(fullfn, "r"))) {
        ERR_LOG("read_config - 打开文件错误");
        fprintf(stderr, "Filename was %s\n", fullfn);
        return -1;
    }

    /* Allocate some space to start. We probably won't need this many entries,
       so we'll trim it later on. */
    if (!(hosts = (host_info_t*)malloc(sizeof(host_info_t) * 256))) {
        ERR_LOG("read_config - 分配内存错误");
        return -1;
    }

    host_count = 256;

    /* Read in the configuration file one line at a time. */
    while (fgets(linebuf, 1024, fp)) {
        /* If the line is too long, bail out. */
        flen = strlen(linebuf);
        ++lineno;

        if (linebuf[flen - 1] != '\n' && flen == 1023) {
            fprintf(stderr, "设置行 %d 数据太长,请检查该行的设置!\n", lineno);
            return -1;
        }

        /* Ignore any blank lines or comments. */
        if (linebuf[0] == '\n' || linebuf[0] == '#')
            continue;

        /* Attempt to parse the line... */
        if (sscanf(linebuf, "%1023s %1023s %1023s %15s", name, host4, host6, ipv4) != 4)
            continue;

        if (!isStringEmpty(host4))
            if (!isIPAddress(host4)) {
                if (get_IPv4_from_hostname(host4, ipv4) == 0) {
#ifdef DEBUG
                    DNS_LOG("DNS (%d)%s 跳转IPv4地址为 %s:%s", lineno, name, host4, ipv4);
#endif // DEBUG
                }
                else {
                    ERR_LOG("DNS (%d)%s 跳转至IPv4域名 %s 失败", lineno, name, host4);
                }
            }

        if (!isStringEmpty(host6))
            if (!isIPv6Address(host6)) {
                if (get_IPv6_from_hostname(host6, ipv6, sizeof(ipv6)) == 0) {
#ifdef DEBUG
                    DNS_LOG("DNS行 (%d)%s 跳转IPv6地址为 %s:%s", lineno, name, host6, ipv6);
#endif // DEBUG
                }
                else {
                    /* 还未完成支持 */
                    //ERR_LOG("DNS (%d)%s 跳转至IPv6域名 %s 失败", lineno, name, host6);
                }
            }

        /* Make sure the IP looks sane. */
        if (inet_pton(AF_INET, ipv4, &addr) != 1) {
            ERR_LOG("read_config - inet_pton");
            return -1;
        }

        /* Make sure we have enough space in the hosts array. */
        if (entries == host_count) {
            tmp = realloc(hosts, host_count * sizeof(host_info_t) * 2);
            if (!tmp) {
                ERR_LOG("read_config - realloc");
                return -1;
            }

            host_count *= 2;
            hosts = (host_info_t*)tmp;
        }

        /* Make space to store the hostname. */
        hosts[entries].name = (char*)malloc(strlen(name) + 1);
        if (!hosts[entries].name) {
            ERR_LOG("read_config - malloc name");
            return -1;
        }

        /* Make space to store the hostname. */
        hosts[entries].host4 = (char*)malloc(strlen(host4) + 1);
        if (!hosts[entries].host4) {
            ERR_LOG("read_config - malloc host4");
            return -1;
        }

        /* Make space to store the hostname. */
        hosts[entries].host6 = (char*)malloc(strlen(host6) + 1);
        if (!hosts[entries].host6) {
            ERR_LOG("read_config - malloc host6");
            return -1;
        }

        /* Fill in the entry and move on to the next one (if any). */
        strcpy(hosts[entries].name, name);
        strcpy(hosts[entries].host4, host4);
        strcpy(hosts[entries].host6, host6);
        hosts[entries].addr = addr;

        //DNS_LOG("读取 DNS地址 %s", hosts[entries].name);
        //DNS_LOG("读取 DNShost4 %s", hosts[entries].host4);
        //DNS_LOG("读取 DNShost6 %s", hosts[entries].host6);

        ++entries;
    }

    /* We're done with the file, so close it. */
    fclose(fp);

    /* Trim the hosts array down to the size that it needs to be. */
    tmp = realloc(hosts, entries * sizeof(host_info_t));
    if (tmp)
        hosts = (host_info_t*)tmp;

    host_count = entries;

    free(fullfn);

    return 0;
}

static SOCKET open_sock(uint16_t port) {
    SOCKET sock;
    struct sockaddr_in addr;

    /* Create the socket. */
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock == INVALID_SOCKET) {
        ERR_LOG("socket");
        return INVALID_SOCKET;
    }

    /* Bind the socket to the specified port. */
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
        ERR_LOG("bind");
        close(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

static host_info_t* find_host(const char* hostname) {
    int i;

    /* Linear search through the list for the one we're looking for. */
    for (i = 0; i < host_count; ++i) {
        if (!cmp_str(hosts[i].name, hostname)) {
            host_line = i;
            return hosts + i;
        }
    }

    return NULL;
}

static void respond_to_query(SOCKET sock, size_t len, struct sockaddr_in* addr,
    host_info_t* h) {
    in_addr_t a;
    char ipv4[INET_ADDRSTRLEN];

    /* DNS specifies that any UDP messages over 512 bytes are truncated. We
       don't bother sending them at all, since we should never approach that
       size to start with. */
    if (len + 16 > 512)
        return;

    /* Copy the input to the output to start. */
    memcpy(outmsg, inmsg, len);

    /* Set the answer count to 1, and set the flags appropriately. */
    outmsg->ancount = htons(1);
    outmsg->flags = htons(0x8180);

    /* Subtract out the size of the header. */
    len -= sizeof(dnsmsg_t);

    if (!isStringEmpty(h->host4))
        if (!isIPAddress(h->host4)) {
            if (get_IPv4_from_hostname(h->host4, ipv4) == 0) {
                DNS_LOG("DNS (%d)%s 跳转IPv4地址为 %s:%s", host_line, h->name, h->host4, ipv4);
                /* Make sure the IP looks sane. */
                if (inet_pton(AF_INET, ipv4, &h->addr) != 1) {
                    ERR_LOG("read_config - inet_pton");
                }
            }
            else {
                //fprintf(stderr, "从 %s 获取 IPv4 地址失败\n", host4);
            }
        }

    /* Fill in the response. This is ugly, but it works. */
    a = ntohl(h->addr);
    outmsg->data[len++] = 0xc0;
    outmsg->data[len++] = 0x0c;
    outmsg->data[len++] = 0x00;
    outmsg->data[len++] = 0x01;
    outmsg->data[len++] = 0x00;
    outmsg->data[len++] = 0x01;
    outmsg->data[len++] = 0x00; /* Next 4 bytes are the TTL. */
    outmsg->data[len++] = 0x00;
    outmsg->data[len++] = 0x09;
    outmsg->data[len++] = 0xfc;
    outmsg->data[len++] = 0x00; /* Address length is the next 2. */
    outmsg->data[len++] = 0x04;
    outmsg->data[len++] = (uint8_t)(a >> 24);
    outmsg->data[len++] = (uint8_t)(a >> 16);
    outmsg->data[len++] = (uint8_t)(a >> 8);
    outmsg->data[len++] = (uint8_t)a;


    DNS_LOG("发送DNS信息至端口: %d", sock);

    /* Send ther response. */
    sendto(sock, outbuf, len + sizeof(dnsmsg_t), 0, (struct sockaddr*)addr,
        sizeof(struct sockaddr_in));
}

static void process_query(SOCKET sock, size_t len, struct sockaddr_in* addr) {
    size_t i;
    uint8_t partlen = 0;
    static char hostbuf[1024];
    size_t hostlen = 0;
    host_info_t* host;
    uint16_t qdc, anc, nsc, arc;
    size_t olen = len;

    /* Subtract out the size of the header. */
    len -= sizeof(dnsmsg_t);

    /* Switch the byte ordering as needed on everything. */
    qdc = ntohs(inmsg->qdcount);
    anc = ntohs(inmsg->ancount);
    nsc = ntohs(inmsg->nscount);
    arc = ntohs(inmsg->arcount);

    /* Check the message to make sure it looks like it came from PSO. */
    if (qdc != 1 || anc != 0 || nsc != 0 || arc != 0)
        return;

    /* Figure out the name of the host that the client is looking for. */
    i = 0;
    while (i < len) {
        partlen = inmsg->data[i];

        /* Make sure the length is sane. */
        if (len < i + partlen + 5) {
            /* Throw out the obvious bad packet. */
            ERR_LOG("端口 %d 抛出坏数据包.", sock);
            close(sock);
            return;
        }

        /* Did we read the last part? */
        if (partlen == 0) {
            /* Make sure the rest of the message is as we expect it. */
            if (inmsg->data[i + 1] != 0 || inmsg->data[i + 2] != 1 ||
                inmsg->data[i + 3] != 0 || inmsg->data[i + 4] != 1) {
                //ERR_LOG("端口 %d 抛出非IN A请求.", sock);
                close(sock);
                return;
            }

            hostbuf[hostlen - 1] = '\0';
            i = len;

            /* See if the requested host is in our list. If it is, respond.*/
            if ((host = find_host(hostbuf))) {
                respond_to_query(sock, olen, addr, host);
            }

            /* And we're done. */
            return;
        }

        /* We've got a part to copy into our string... Do so. */
        memcpy(hostbuf + hostlen, inmsg->data + i + 1, partlen);
        hostlen += partlen;
        hostbuf[hostlen++] = '.';
        i += partlen + 1;
    }
}

#if !defined(_WIN32) && !defined(_arch_dreamcast)
static int drop_privs(void) {
    struct passwd* pw;
    uid_t uid;
    gid_t gid;

    /* Make sure we're actually root, otherwise some of this will fail. */
    if (getuid() && geteuid()) {
        return 0;
    }

    /* Look for users. We're looking for the user "nobody". */
    if ((pw = getpwnam("nobody"))) {
        uid = pw->pw_uid;
        gid = pw->pw_gid;
    }
    else {
        fprintf(stderr, "Cannot find user \"nobody\". Bailing out!\n");
        return -1;
    }

    /* Set privileges. */
    if (setgroups(1, &gid)) {
        ERR_LOG("setgroups");
        return -1;
    }

    if (setgid(gid)) {
        ERR_LOG("setgid");
        return -1;
    }

    if (setuid(uid)) {
        ERR_LOG("setuid");
        return -1;
    }

    /* Make sure the privileges stick. */
    if (!getuid() || !geteuid()) {
        fprintf(stderr, "Cannot set non-root privileges. Bailing out!\n");
        return -1;
    }

    return 0;
}
#endif

#ifdef _arch_dreamcast
#include <arch/arch.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_NET);
extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

#endif

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

void move_console_to_center()
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    HWND consoleHwnd = GetConsoleWindow(); // 获取控制台窗口句柄

    // 获取屏幕尺寸
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 计算控制台窗口在屏幕中央的位置
    int consoleWidth = 980; // 控制台窗口宽度
    int consoleHeight = 510; // 控制台窗口高度
    int consoleX = (screenWidth - consoleWidth) / 2; // 控制台窗口左上角 x 坐标
    int consoleY = (screenHeight - consoleHeight) / 2; // 控制台窗口左上角 y 坐标

    // 移动控制台窗口到屏幕中央
    MoveWindow(consoleHwnd, consoleX, consoleY, consoleWidth, consoleHeight, TRUE);
#endif
}

/* 初始化 */
static void initialization() {

    load_log_config();

#if defined(_WIN32) && !defined(__CYGWIN__)
    if (init_wsa()) {
        ERR_EXIT("WSAStartup 错误...");
        getchar();
    }
#endif

    HINSTANCE hinst = GetModuleHandle(NULL);
    consoleHwnd = GetConsoleWindow();

    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.hIcon = LoadIcon(hinst, IDI_APPLICATION);
    wc.hCursor = LoadCursor(hinst, IDC_ARROW);
    wc.hInstance = hinst;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = "Sancaros";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClass(&wc)) {
        ERR_EXIT("注册类失败.");
    }

    hwndWindow = CreateWindow("Sancaros", "hidden window", WS_MINIMIZE, 1, 1, 1, 1,
        NULL,
        NULL,
        hinst,
        NULL);

    if (!hwndWindow) {
        ERR_EXIT("无法创建窗口.");
    }

    ShowWindow(hwndWindow, SW_HIDE);
    UpdateWindow(hwndWindow);
    //MoveWindow(consoleHwnd, 900, 510, 980, 510, SWP_SHOWWINDOW);	//把控制台拖到(100,100)
    ShowWindow(consoleHwnd, window_hide_or_show);
    UpdateWindow(consoleHwnd);

    move_console_to_center();

    // 设置崩溃处理函数
    SetUnhandledExceptionFilter(crash_handler);

}

int __cdecl main(int argc, char** argv) {
    SOCKET sock;
    struct sockaddr_in addr = { 0 };
    socklen_t alen;
    ssize_t rlen;

    initialization();

    __try {

        server_name_num = DNS_SERVER;

        load_program_info(server_name[DNS_SERVER].name, DNS_SERVER_VERSION);

        /* Open the socket. We will probably need root for this on UNIX-like
           systems. */
        if ((sock = open_sock(DNS_PORT)) == INVALID_SOCKET) {
            ERR_EXIT("open_sock 错误");
            getchar();
        }

#if !defined(_WIN32) && !defined(_arch_dreamcast)
        if (drop_privs()) {
            ERR_EXIT("drop_privs 错误");
        }

        /* Daemonize. */
        daemon(1, 0);
#endif

        /* Read in the configuration. */
        if (read_config(CONFIG_DIR, CONFIG_FILE)) {
            ERR_EXIT("read_config 错误");
            getchar();
        }

        DNS_LOG("DNS服务器启动完成");

        /* Go ahead and loop forever... */
        for (;;) {
            alen = sizeof(addr);

            /* Grab the next request from the socket. */
            if ((rlen = recvfrom(sock, inbuf, 1024, 0, (struct sockaddr*)&addr,
                &alen)) > sizeof(dnsmsg_t)) {

                process_query(sock, rlen, &addr);
            }
        }

        /* We'll never actually get here... */
#ifndef _WIN32
        close(sock);
#else
        closesocket(sock);
        WSACleanup();
#endif

    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        ERR_LOG("出现错误, 程序将退出.");
        (void)getchar();
    }

    return 0;
}
