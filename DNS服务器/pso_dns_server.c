/*
    �λ�֮���й� ��֤������
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

/*Note:
    ʹ�ô˷�����, ��ȷ����������53�˿ڲ���ռ��, �ҿ���UDP 53�˿�֧�ֽ���
*/

#include <pso_crash_handle.h>

#include "pso_dns_server.h"
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <pthread.h>

int host_line = 0;

static jmp_buf jmpbuf;
static volatile sig_atomic_t rehash = 0;
static volatile sig_atomic_t should_exit = 0;
static volatile sig_atomic_t canjump = 0;

/* Storage for our client list. */
struct client_queue clients = TAILQ_HEAD_INITIALIZER(clients);

#if defined(_WIN32) && !defined(__CYGWIN__)
HWND consoleHwnd;
HWND hwndWindow;
WNDCLASS wc = { 0 };

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
        fprintf(stderr, "getaddrinfo ʧ�� ������: %d\n", WSAGetLastError());
#else
        fprintf(stderr, "getaddrinfo ʧ�� ������: %s\n", gai_strerror(status));
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
        fprintf(stderr, "getaddrinfo ʧ�� ������: %d\n", WSAGetLastError());
#else
        fprintf(stderr, "getaddrinfo ʧ�� ������: %s\n", gai_strerror(status));
#endif

#endif // DEBUG

        return -1;
    }

    for (p = result; p != NULL; p = p->ai_next) {
        void* addr;
        char ip[INET6_ADDRSTRLEN];

        if (p->ai_family == AF_INET6) {
            // ��ȡ IPv6 ��ַ
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        else {
            continue;
        }

        // ����ַת��Ϊ�ɶ��� IP �ַ���
        inet_ntop(p->ai_family, addr, ip, sizeof(ip));

        // ���� IP ��ַ���������
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
        ERR_LOG("read_config - ���ļ�����");
        fprintf(stderr, "Filename was %s\n", fullfn);
        return -1;
    }

    /* Allocate some space to start. We probably won't need this many entries,
       so we'll trim it later on. */
    if (!(hosts = (host_info_t*)malloc(sizeof(host_info_t) * 256))) {
        ERR_LOG("read_config - �����ڴ����");
        return -1;
    }

    host_count = 256;

    /* Read in the configuration file one line at a time. */
    while (fgets(linebuf, 1024, fp)) {
        /* If the line is too long, bail out. */
        flen = strlen(linebuf);
        ++lineno;

        if (linebuf[flen - 1] != '\n' && flen == 1023) {
            fprintf(stderr, "������ %d ����̫��,������е�����!\n", lineno);
            return -1;
        }

        /* Ignore any blank lines or comments. */
        if (linebuf[0] == '\n' || linebuf[0] == '#')
            continue;

        /* Attempt to parse the line... */
        if (sscanf(linebuf, "%1023s %1023s %1023s %15s", name, host4, host6, ipv4) != 4)
            continue;

        if (!isEmptyString(host4))
            if (!isIPAddress(host4)) {
                if (get_IPv4_from_hostname(host4, ipv4) == 0) {
#ifdef DEBUG
                    DNS_LOG("DNS (%d)%s ��תIPv4��ַΪ %s:%s", lineno, name, host4, ipv4);
#endif // DEBUG
                }
                else {
                    ERR_LOG("DNS (%d)%s ��ת��IPv4���� %s ʧ��", lineno, name, host4);
                }
            }

        if (!isEmptyString(host6))
            if (!isIPv6Address(host6)) {
                if (get_IPv6_from_hostname(host6, ipv6, sizeof(ipv6)) == 0) {
#ifdef DEBUG
                    DNS_LOG("DNS�� (%d)%s ��תIPv6��ַΪ %s:%s", lineno, name, host6, ipv6);
#endif // DEBUG
                }
                else {
                    /* ��δ���֧�� */
                    //ERR_LOG("DNS (%d)%s ��ת��IPv6���� %s ʧ��", lineno, name, host6);
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

#ifdef DEBUG

        DNS_LOG("��ȡ DNS��ַ %s", hosts[entries].name);
        DNS_LOG("��ȡ DNShost4 %s", hosts[entries].host4);
        DNS_LOG("��ȡ DNShost6 %s", hosts[entries].host6);

#endif // DEBUG

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

//static SOCKET open_sock(uint16_t port) {
//    SOCKET sock;
//    struct sockaddr_in addr;
//
//    /* Create the socket. */
//    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//
//    if (sock == INVALID_SOCKET) {
//        ERR_LOG("socket");
//        return INVALID_SOCKET;
//    }
//
//    /* Bind the socket to the specified port. */
//    memset(&addr, 0, sizeof(struct sockaddr_in));
//    addr.sin_family = AF_INET;
//    addr.sin_addr.s_addr = INADDR_ANY;
//    addr.sin_port = htons(port);
//
//    if (bind(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
//        ERR_LOG("bind");
//        close(sock);
//        return INVALID_SOCKET;
//    }
//
//    return sock;
//}

/* Destroy a connection, closing the socket and removing it from the list. */
void destroy_connection(dns_client_t* c) {
    TAILQ_REMOVE(&clients, c, qentry);

    c->auth = 0;

    if (c->sock >= 0) {
        close(c->sock);
    }

    if (c->recvbuf) {
        free_safe(c->recvbuf);
    }

    if (c->sendbuf) {
        free_safe(c->sendbuf);
    }

    free_safe(c);
}

/* ����һ�������ӣ��������洢�ڿͻ����б��С�*/
dns_client_t* create_connection(int sock, struct sockaddr_in* ip, socklen_t size) {
    pthread_mutexattr_t attr;
    uint16_t qdc, anc, nsc, arc;

    /* Ϊ�¿ͻ��˷����ڴ�ռ䡣*/
    dns_client_t* rv = (dns_client_t*)malloc(sizeof(dns_client_t));
    if (!rv) {
        return NULL;
    }

    memset(rv, 0, sizeof(dns_client_t));

    /* �����������洢���ͻ��˽ṹ�С�*/
    rv->sock = sock;

    /* ����û��Ƿ�ʹ��IPv6��*/
    if (ip->sin_family == AF_INET6) {
        rv->is_ipv6 = 1;
    }

    /* ������������*/
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&(rv->mutex), &attr);
    pthread_mutexattr_destroy(&attr);

    memcpy(&(rv->ip_addr), ip, size);

    uint32_t rng_seed = (uint32_t)(time(NULL) ^ sock);
    sfmt_init_gen_rand(&(rv->sfmt_rng), rng_seed);

    /* ����Ҫ���������ֶε��ֽ�˳��*/
    qdc = ntohs(inmsg->qdcount);
    anc = ntohs(inmsg->ancount);
    nsc = ntohs(inmsg->nscount);
    arc = ntohs(inmsg->arcount);

#ifdef DEBUG
    print_ascii_hex(inmsg, sizeof(dnsmsg_t));
#endif // DEBUG

    /* �����Ϣ��ȷ�����ƺ�����PSO��*/
    if (qdc != 1 || anc != 0 || nsc != 0 || arc != 0) {
#ifdef DEBUG
        ERR_LOG("�Ӷ˿� %d ���͵�DNS������Ч��", sock);
#endif // DEBUG
        goto err;
}

    /* ���¿ͻ��˲��뵽�б�ĩβ��*/
    TAILQ_INSERT_TAIL(&clients, rv, qentry);

    rv->auth = 1;

    return rv;

err:
    pthread_mutex_destroy(&(rv->mutex));
    free_safe(rv);
    return NULL;
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

static int respond_to_query(SOCKET sock, size_t len, struct sockaddr_in* addr,
    host_info_t* h) {
    in_addr_t a;
    char ipv4[INET_ADDRSTRLEN];

    /* DNS specifies that any UDP messages over 512 bytes are truncated. We
       don't bother sending them at all, since we should never approach that
       size to start with. */
    if (len + 16 > 512) {
        ERR_LOG("�˿� %d / %d �ֽ� ��������", sock, len + 16);
        return -1;
    }

    /* Copy the input to the output to start. */
    memcpy(outmsg, inmsg, len);

    /* Set the answer count to 1, and set the flags appropriately. */
    outmsg->ancount = htons(1);
    outmsg->flags = htons(0x8180);

    /* Subtract out the size of the header. */
    len -= sizeof(dnsmsg_t);

    if (!isEmptyString(h->host4))
        if (!isIPAddress(h->host4)) {
            if (get_IPv4_from_hostname(h->host4, ipv4) == 0) {
                DNS_LOG("DNS (%d)%s ��תIPv4��ַΪ %s:%s", host_line, h->name, h->host4, ipv4);
                /* Make sure the IP looks sane. */
                if (inet_pton(AF_INET, ipv4, &h->addr) != 1) {
                    ERR_LOG("read_config - inet_pton");
                    return -2;
                }
            }
            else {
                ERR_LOG("�� %s ��ȡ IPv4 ��ַʧ��", h->host4);
                return -3;
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



    /* Send ther response. */
    int bytes_sent = sendto(sock, outbuf, len + sizeof(dnsmsg_t), 0, (struct sockaddr*)addr,
        sizeof(struct sockaddr_in));
    if (bytes_sent < 0) {
        ERR_LOG("sendto %d ����", sock);
        return -4;
    }

    DNS_LOG("����DNS��Ϣ���˿� %d / %d �ֽ�", sock, bytes_sent);

    return 0;
}

static int process_query(SOCKET sock, size_t len, struct sockaddr_in* addr) {
    size_t i;
    uint8_t partlen = 0;
    static char hostbuf[1024];
    size_t hostlen = 0;
    host_info_t* host;
    uint16_t qdc, anc, nsc, arc;
    size_t olen = len;
    char ip_str[INET6_ADDRSTRLEN];

    /* ��ȥͷ�����ݵĴ�С. */
    len -= sizeof(dnsmsg_t);

    /* ������Ҫ�л��������ݵ��ֽ�˳��. */
    qdc = ntohs(inmsg->qdcount);
    anc = ntohs(inmsg->ancount);
    nsc = ntohs(inmsg->nscount);
    arc = ntohs(inmsg->arcount);

#ifdef DEBUG

    print_ascii_hex(inmsg, sizeof(dnsmsg_t));

#endif // DEBUG

    /* �����Ϣ��ȷ������������������PSO. */
    if (qdc != 1 || anc != 0 || nsc != 0 || arc != 0) {
#ifdef DEBUG

        ERR_LOG("�˿� %d ���͵�DNS������Ч.", sock);

#endif // DEBUG
        return -1;
    }

    /* �ҳ��ͻ���Ҫ���ҵ�����������. */
    i = 0;
    while (i < len) {
        partlen = inmsg->data[i];

        /* Make sure the length is sane. */
        if (len < i + partlen + 5) {
            /* Throw out the obvious bad packet. */

            if (addr->sin_family == AF_INET) {
                struct sockaddr_in* ipv4 = (struct sockaddr_in*)addr;
                // �� IPv4 ��ַת��Ϊ���ʮ�����ַ���
                inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, INET_ADDRSTRLEN);
            }
            else if (addr->sin_family == AF_INET6) {
                struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)addr;
                // �� IPv6 ��ַת��Ϊ���ʮ�������ַ���
                inet_ntop(AF_INET6, &(ipv6->sin6_addr), ip_str, INET6_ADDRSTRLEN);
            }
            else {
                ERR_LOG("����˿� %d ��ЧIP��ַfamily.", sock);
                return -2;
            }

            ERR_LOG("���� %s:%d �׳������ݰ�,�Ͽ�������.", ip_str, sock);
            return -3;
        }

        /* ���Ƕ������һ��������? */
        if (partlen == 0) {
            /* ȷ����Ϣ�����ಿ��������Ԥ�ڵ�һ��. */
            if (inmsg->data[i + 1] != 0 || inmsg->data[i + 2] != 1 ||
                inmsg->data[i + 3] != 0 || inmsg->data[i + 4] != 1) {
                ERR_LOG("�˿� %d �׳���IN A����.", sock);
                return -4;
            }

            hostbuf[hostlen - 1] = '\0';
            i = len;

            /* �鿴����������Ƿ������ǵ��б���.�����,��ظ�.*/
            host = find_host(hostbuf);
            if (!host) {
                ERR_LOG("�˿� %d ����� %s ���������б���.", sock, hostbuf);
                return -5;
            }

            if (respond_to_query(sock, olen, addr, host)) {
                ERR_LOG("�˿� %d ���ݻ�Ӧ����.", sock);
                return -6;
            }

            /* And we're done. */
            return 0;
        }

        /* We've got a part to copy into our string... Do so. */
        memcpy(hostbuf + hostlen, inmsg->data + i + 1, partlen);
        hostlen += partlen;
        hostbuf[hostlen++] = '.';
        i += partlen + 1;
    }

    return 0;
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
#if defined(_WIN32) && !defined(__CYGWIN__)
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
#endif
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

    if (!RegisterClass(&wc)) {
        ERR_EXIT("ע����ʧ��.");
    }

    hwndWindow = CreateWindow("Sancaros", "hidden window", WS_MINIMIZE, 1, 1, 1, 1,
        NULL,
        NULL,
        hinst,
        NULL);

    if (!hwndWindow) {
        ERR_EXIT("�޷���������.");
    }

    ShowWindow(hwndWindow, SW_HIDE);
    UpdateWindow(hwndWindow);
    //MoveWindow(consoleHwnd, 900, 510, 980, 510, SWP_SHOWWINDOW);	//�ѿ���̨�ϵ�(100,100)
    ShowWindow(consoleHwnd, window_hide_or_show);
    UpdateWindow(consoleHwnd);

    move_console_to_center();

    // ���ñ���������
    SetUnhandledExceptionFilter(crash_handler);
#endif

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

const void* my_ntop(struct sockaddr_storage* addr,
    char str[INET6_ADDRSTRLEN]) {
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

static int open_sock(int family, uint16_t port) {
    int sock = SOCKET_ERROR, val;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;

    /* �����˿ڲ�����. */
    sock = socket(family, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0) {
        SOCKET_ERR(sock, "�����˿ڼ���");
        //ERR_LOG("�����˿ڼ��� %d:%d ���ִ���", family, port);
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
            SOCKET_ERR(sock, "bind error");
            ERR_LOG("�󶨶˿� %d:%d ���ִ���", family, port);
        }

        //if (listen(sock, 10)) {
        //    SOCKET_ERR(sock, "listen error");
        //    ERR_LOG("�����˿� %d:%d ���ִ���", family, port);
        //}
    }
    else if (family == PF_INET6) {
        /* ����֧��IPV6. */
        val = 1;
        if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (PCHAR)&val, sizeof(int))) {
            SOCKET_ERR(sock, "���ö˿���Ϣ IPV6_V6ONLY");
            ERR_LOG("���ö˿���ϢIPV6_V6ONLY %d:%d ���ִ���", family, port);
        }

        memset(&addr6, 0, sizeof(struct sockaddr_in6));
        addr6.sin6_family = family;
        addr6.sin6_addr = in6addr_any;
        addr6.sin6_port = htons(port);

        if (bind(sock, (struct sockaddr*)&addr6, sizeof(struct sockaddr_in6))) {
            SOCKET_ERR(sock, "�󶨶˿�");
            ERR_LOG("�󶨶˿� %d:%d ���ִ���", family, port);
        }

        //if (listen(sock, 10)) {
        //    SOCKET_ERR(sock, "�����˿�");
        //    ERR_LOG("�����˿� %d:%d ���ִ���", family, port);
        //}
    }
    else {
        ERR_LOG("�򿪶˿� %d:%d ���ִ���", family, port);
        close(sock);
        return -1;
    }

    return sock;
}

#ifdef PSOCN_ENABLE_IPV6
#define DNS_CLIENT_SOCKETS_TYPE_MAX 2
#else
#define DNS_CLIENT_SOCKETS_TYPE_MAX 1
#endif

psocn_srvsockets_t dns_sockets[DNS_CLIENT_SOCKETS_TYPE_MAX] = {
    { PF_INET , 53 , CLIENT_TYPE_DNS                , "DNS�˿�" },
#ifdef PSOCN_ENABLE_IPV6
    { PF_INET6 , 53 , CLIENT_TYPE_DNS                , "DNS(IPV6)�˿�" },
#endif
};

static void listen_sockets(int sockets[DNS_CLIENT_SOCKETS_TYPE_MAX]) {
    int i;

    /* �������м����˿� */
    for (i = 0; i < DNS_CLIENT_SOCKETS_TYPE_MAX; ++i) {
        sockets[i] = open_sock(dns_sockets[i].sock_type, dns_sockets[i].port);

        if (sockets[i] < 0) {
            ERR_EXIT("���� %d (%s : %s) ����, �����˳�", dns_sockets[i].port
                , dns_sockets[i].sockets_name, dns_sockets[i].sock_type == PF_INET ? "IPv4" : "IPv6");
        }
        else {
            DNS_LOG("���� %d (%s) �ɹ�.", dns_sockets[i].port, dns_sockets[i].sockets_name);
        }
    }
}

static void cleanup_sockets(int sockets[DNS_CLIENT_SOCKETS_TYPE_MAX]) {
    int i;

    /* Clean up. */
    for (i = 0; i < DNS_CLIENT_SOCKETS_TYPE_MAX; ++i) {
        close(sockets[i]);
    }
}

void get_ip_address(struct sockaddr_in* addr, char* ip_buffer) {
    const char* ip = inet_ntoa(addr->sin_addr);
    strncpy(ip_buffer, ip, INET_ADDRSTRLEN);
}

static void run_server(int sockets[DNS_CLIENT_SOCKETS_TYPE_MAX]) {
    __try {

        struct sockaddr_in client_addr = { 0 };
        char ipstr[INET6_ADDRSTRLEN];
        socklen_t len;
        ssize_t recive_len;
        dns_client_t* i = { 0 }, * tmp;
        int sock = SOCKET_ERROR, j;
        int rv = 0, dns_size = sizeof(dnsmsg_t);
        size_t client_count = 0;

        /* Go ahead and loop forever... */
        while (!should_exit) {

            /* Set this up in case a signal comes in during the time between calling
               this and the select(). */
            if (!_setjmp(jmpbuf)) {
                canjump = 1;
            }

            /* If we need to, rehash the patches and welcome message. */
            if (rehash) {
                canjump = 0;
                rehash = 0;
                canjump = 1;
            }

            /* Make sure a rehash event doesn't interrupt any of this stuff,
                   it will get handled the next time through the loop. */
            canjump = 0;

            /* Check the listening sockets first. */
            for (j = 0; j < DNS_CLIENT_SOCKETS_TYPE_MAX; ++j) {
                len = sizeof(struct sockaddr);
                recive_len = recvfrom(sockets[j], inbuf, 1024, 0, (struct sockaddr*)&client_addr, &len);
                if (recive_len <= dns_size) {
                    ERR_LOG("recvfrom");
                    perror("recvfrom");
                    continue; // �����ȴ���һ�ν���
                }
                else {
                    sock = ntohs(client_addr.sin_port);

                    i = create_connection(sock, &client_addr, len);
                    if (!i) {
                        ERR_LOG("����DNS��������ʧ�� �˿ں�: %d", sock);
                        close(sock);
                        continue; // �����ȴ���һ�ν���
                    }

                    rv = process_query(sockets[j], recive_len, &client_addr);
                    if (rv) {
#ifdef DEBUG
                        ERR_LOG("�Ͽ��˿� %d ���ݽ���. ������ %d", sock, rv);
#endif // DEBUG
                        close(sock);
                        continue; // �����ȴ���һ�ν���
                    }
                    else {
                        ++client_count;
                        set_console_title("�λ�֮���й� %s %s�汾 Ver%s ���� Sancaros [��� %d]", server_name[DNS_SERVER].name, PSOBBCN_PLATFORM_STR, DNS_SERVER_VERSION, client_count);

                        get_ip_address(&i->ip_addr, ipstr);
                        DNS_LOG("���� %s:%u �ͻ��˻�ȡDNS����", ipstr, i->sock);
                        i->disconnected = 1;
                    }
                }
            }

            /* ������Ч������ (its not safe to do a TAILQ_REMOVE in
               the middle of a TAILQ_FOREACH, and destroy_connection does indeed
               use TAILQ_REMOVE). */
            canjump = 0;
            i = TAILQ_FIRST(&clients);

            while (i) {
                tmp = TAILQ_NEXT(i, qentry);

                if (i->disconnected) {
                    get_ip_address(&i->ip_addr, ipstr);
                    if (i->auth) {
                        DC_LOG("�Ͽ� %s:%u DNS����", ipstr, i->sock);
                        --client_count;
                        set_console_title("�λ�֮���й� %s %s�汾 Ver%s ���� Sancaros [��� %d]", server_name[DNS_SERVER].name, PSOBBCN_PLATFORM_STR, DNS_SERVER_VERSION, client_count);
                    }
                    else {
                        DC_LOG("�Ͽ� %s:%u DNS��Ч����", ipstr, i->sock);
                    }
                    destroy_connection(i);
                }

                i = tmp;
            }
        }

    }

    __except (crash_handler(GetExceptionInformation())) {
        // ������ִ���쳣�������߼��������ӡ������Ϣ���ṩ�û��Ѻõ���ʾ��

        CRASH_LOG("���ִ���, �����˳�.");
        (void)getchar();
    }
}

// �����߳����еĺ���
void* server_thread(void* arg) {
    int* sockets = (int*)arg;

    // ���з���������
    run_server(sockets);

    return NULL;
}

/* The key for accessing our thread-specific receive buffer. */
pthread_key_t recvbuf_key;

/* The key for accessing our thread-specific send buffer. */
pthread_key_t sendbuf_key;

/* Destructor for the thread-specific receive buffer */
static void buf_dtor(void* rb) {
    free_safe(rb);
}

/* Initialize the clients system, allocating any thread specific keys */
int client_init() {
    if (pthread_key_create(&recvbuf_key, &buf_dtor)) {
        perror("pthread_key_create");
        return -1;
    }

    if (pthread_key_create(&sendbuf_key, &buf_dtor)) {
        perror("pthread_key_create");
        return -1;
    }

    return 0;
}

/* Clean up the clients system. */
void client_shutdown(void) {
    pthread_key_delete(recvbuf_key);
    pthread_key_delete(sendbuf_key);
}

int __cdecl main(int argc, char** argv) {
    void* tmp;
    int sockets[DNS_CLIENT_SOCKETS_TYPE_MAX] = { 0 };

    initialization();

    server_name_num = DNS_SERVER;

    __try {

        load_program_info(server_name[DNS_SERVER].name, DNS_SERVER_VERSION);

        /* Open the socket. We will probably need root for this on UNIX-like
           systems. */
           //if ((sock = open_sock(DNS_PORT)) == INVALID_SOCKET) {
           //    ERR_EXIT("open_sock ����, ����˿� %u �Ƿ�ռ��.", DNS_PORT);
           //}

#if !defined(_WIN32) && !defined(_arch_dreamcast)
        if (drop_privs()) {
            ERR_EXIT("drop_privs ����");
        }

        /* Daemonize. */
        daemon(1, 0);
#endif

        /* Read in the configuration. */
        if (read_config(CONFIG_DIR, CONFIG_FILE)) {
            ERR_EXIT("read_config ����");
        }

        /* ��ȡ�źż���. */
        HookupHandler();

        /* Set up things for clients to connect. */
        if (client_init())
            ERR_EXIT("�޷����� �ͻ��� ����");

        listen_sockets(sockets);

        DNS_LOG("%s�������.", server_name[DNS_SERVER].name);
        DNS_LOG("����������...");
        DNS_LOG("���� <Ctrl-C> �رճ���.");

        /* Go ahead and loop forever... */
        // �����߳�
        pthread_t server_tid;
        pthread_create(&server_tid, NULL, server_thread, sockets);
        //run_server(sockets);

//        /* We'll never actually get here... */
//#ifndef _WIN32
//        close(sock);
//#else
//        cleanup_sockets(sockets);
//
//        //close(sock);
//        WSACleanup();
//#endif
        // �ȴ��������߳̽���
        pthread_join(server_tid, NULL);

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


        // �ͷ���Դ
#ifndef _WIN32
        close(sockets[0]);
#else
        cleanup_sockets(sockets);
        WSACleanup();
#endif

        client_shutdown();

        UnhookHandler();
    }

    __except (crash_handler(GetExceptionInformation())) {
        // ������ִ���쳣�������߼��������ӡ������Ϣ���ṩ�û��Ѻõ���ʾ��

        CRASH_LOG("���ִ���, �����˳�.");
        (void)getchar();
    }

    return 0;
}
