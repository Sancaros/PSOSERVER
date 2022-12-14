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
#include <stdarg.h>
#include <time.h>
//#include <sys/time.h>
#include <iconv.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <WinSock_Defines.h>

#include <f_logs.h>
#include <debug.h>
#include <f_iconv.h>

#include "utils.h"
#include "clients.h"
#include <pso_player.h>

#ifdef HAVE_LIBMINI18N
mini18n_t langs[CLIENT_LANG_ALL];
#endif

void print_packet(unsigned char* pkt, int len) {
    unsigned char* pos = pkt, * row = pkt;
    int line = 0, type = 0;

    /* Print the packet both in hex and ASCII. */
    while (pos < pkt + len) {
        if (type == 0) {
            printf("%02X ", *pos);
        }
        else {
            if (*pos >= 0x20 && *pos < 0x7F) {
                printf("%c", *pos);
            }
            else {
                printf(".");
            }
        }

        ++line;
        ++pos;

        if (line == 16) {
            if (type == 0) {
                printf("\t");
                pos = row;
                type = 1;
                line = 0;
            }
            else {
                printf("\n");
                line = 0;
                row = pos;
                type = 0;
            }
        }
    }

    /* Finish off the last row's ASCII if needed. */
    if (len & 0x1F) {
        /* Put spaces in place of the missing hex stuff. */
        while (line != 16) {
            printf("   ");
            ++line;
        }

        pos = row;
        printf("\t");

        /* Here comes the ASCII. */
        while (pos < pkt + len) {
            if (*pos >= 0x20 && *pos < 0x7F) {
                printf("%c", *pos);
            }
            else {
                printf(".");
            }

            ++pos;
        }

        printf("\n");
    }
}

void print_packet2(const unsigned char *pkt, int len) {
    /* With NULL, this will simply grab the current output for the debug log.
       It won't try to set the file to NULL. */
    FILE *fp = debug_set_file(NULL);

    if(!fp) {
        fp = stdout;
    }

    fprint_packet(fp, pkt, len, -1);
    fflush(fp);
}

void fprint_packet(FILE *fp, const unsigned char *pkt, int len, int rec) {
    const unsigned char *pos = pkt, *row = pkt;
    int line = 0, type = 0;
    time_t now;
    char tstr[26];

    if(rec != -1) {
        now = time(NULL);
        ctime_s(tstr, sizeof(tstr), &now);
        tstr[strlen(tstr) - 1] = 0;
        fprintf(fp, "[%s] Packet 由服务器 %s\n", tstr,
                rec ? "接收" : "发送");
    }

    /* Print the packet both in hex and ASCII. */
    while(pos < pkt + len) {
        if(line == 0 && type == 0) {
            fprintf(fp, "%04X ", (uint16_t)(pos - pkt));
        }

        if(type == 0) {
            fprintf(fp, "%02X ", *pos);
        }
        else {
            if(*pos >= 0x20 && *pos < 0x7F) {
                fprintf(fp, "%c", *pos);
            }
            else {
                fprintf(fp, ".");
            }
        }

        ++line;
        ++pos;

        if(line == 16) {
            if(type == 0) {
                fprintf(fp, "\t");
                pos = row;
                type = 1;
                line = 0;
            }
            else {
                fprintf(fp, "\n");
                line = 0;
                row = pos;
                type = 0;
            }
        }
    }

    /* Finish off the last row's ASCII if needed. */
    if(len & 0x1F) {
        /* Put spaces in place of the missing hex stuff. */
        while(line != 16) {
            fprintf(fp, "   ");
            ++line;
        }

        pos = row;
        fprintf(fp, "\t");

        /* Here comes the ASCII. */
        while(pos < pkt + len) {
            if(*pos >= 0x20 && *pos < 0x7F) {
                fprintf(fp, "%c", *pos);
            }
            else {
                fprintf(fp, ".");
            }

            ++pos;
        }

        fprintf(fp, "\n");
    }

    fflush(fp);
}

int dc_bug_report(ship_client_t *c, simple_mail_pkt *pkt) {
    //struct timeval rawtime;
    //struct tm cooked;
    char filename[64];
    char text[0x91];
    FILE *fp;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    /* Get the timestamp */
    //gettimeofday(&rawtime, NULL);

    /* Get UTC */
    //gmtime_r(&rawtime.tv_sec, &cooked);

    /* Figure out the name of the file we'll be writing to. */
    sprintf(filename, "Bugs/%u.%02u.%02u.%02u.%02u.%02u.%03u-%d",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
            rawtime.wMilliseconds, c->guildcard);

    strncpy(text, pkt->dcmaildata.stuff, 0x90);
    text[0x90] = '\0';

    /* Attempt to open up the file. */
    fp = fopen(filename, "w");

    if(!fp) {
        return -1;
    }

    /* Write the bug report out. */
    fprintf(fp, "来自 %s 的BUG报告 (%d) v%d @ %u.%02u.%02u %02u:%02u:%02u\n\n",
            c->pl->v1.character.disp.dress_data.guildcard_name, c->guildcard, c->version, rawtime.wYear,
        rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute,
        rawtime.wSecond);

    fprintf(fp, "%s", text);

    fclose(fp);

    return send_txt(c, "%s", __(c, "\tE\tC7Thank you for your report."));
}

int pc_bug_report(ship_client_t *c, simple_mail_pkt *pkt) {
    //struct timeval rawtime;
    //struct tm cooked;
    char filename[64];
    char text[0x91];
    FILE *fp;
    char *inptr;
    char *outptr;
    size_t in, out;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    /* Get the timestamp */
   // gettimeofday(&rawtime, NULL);

    /* Get UTC */
    //gmtime_r(&rawtime.tv_sec, &cooked);

    /* Figure out the name of the file we'll be writing to. */
    sprintf(filename, "Bugs/%u.%02u.%02u.%02u.%02u.%02u.%03u-%d",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
            rawtime.wMilliseconds, c->guildcard);

    in = 0x120;
    out = 0x90;
    inptr = pkt->pcmaildata.stuff;
    outptr = text;
    iconv(ic_utf16_to_gbk, &inptr, &in, &outptr, &out);

    text[0x90] = '\0';

    /* Attempt to open up the file. */
    fp = fopen(filename, "w");

    if(!fp) {
        return -1;
    }

    /* Write the bug report out. */
    fprintf(fp, "来自 %s 的BUG报告 (%d) v%d @ %u.%02u.%02u %02u:%02u:%02u\n\n",
            c->pl->v1.character.disp.dress_data.guildcard_name, c->guildcard, c->version, rawtime.wYear,
        rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute,
        rawtime.wSecond);

    fprintf(fp, "%s", text);

    fclose(fp);

    return send_txt(c, "%s", __(c, "\tE\tC7Thank you for your report."));
}

int bb_bug_report(ship_client_t *c, simple_mail_pkt *pkt) {
    //struct timeval rawtime;
    //struct tm cooked;
    char filename[64];
    char text[0x200], name[0x40];
    FILE *fp;
    char *inptr;
    char *outptr;
    size_t in, out;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    /* Get the timestamp */
   // gettimeofday(&rawtime, NULL);

    /* Get UTC */
    //gmtime_r(&rawtime.tv_sec, &cooked);

    /* Figure out the name of the file we'll be writing to. */
    sprintf(filename, "Bugs/%u.%02u.%02u.%02u.%02u.%02u.%03u-%d",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
            rawtime.wMilliseconds, c->guildcard);

    in = 0x158;
    out = 0x200;
    inptr = (char *)pkt->bbmaildata.message;
    outptr = text;
    iconv(ic_utf16_to_gbk, &inptr, &in, &outptr, &out);

    text[0x1FF] = '\0';

    istrncpy16_raw(ic_utf16_to_gbk, name, &c->pl->bb.character.name[2], 0x40,
        BB_CHARACTER_NAME_LENGTH - 2);

    /* Attempt to open up the file. */
    fp = fopen(filename, "w");

    if(!fp) {
        return -1;
    }

    /* Write the bug report out. */
    fprintf(fp, "来自 %s 的BUG报告 (%d) v%d @ %u.%02u.%02u %02u:%02u:%02u\n\n",
            name, c->guildcard, c->version, rawtime.wYear,
            rawtime.wMonth, rawtime.wDay, rawtime.wHour, rawtime.wMinute,
            rawtime.wSecond);

    fprintf(fp, "%s", text);

    fclose(fp);

    return send_txt(c, "%s", __(c, "\tE\tC7非常感谢您的BUG报告."));
}

/* Begin logging the specified client's packets */
int pkt_log_start(ship_client_t *i) {
    //struct timeval rawtime;
    //struct tm cooked;
    char str[128];
    FILE *fp;
    time_t now;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    pthread_mutex_lock(&i->mutex);

    if(i->logfile) {
        pthread_mutex_unlock(&i->mutex);
        return -1;
    }

    /* Get the timestamp */
    //gettimeofday(&rawtime, NULL);

    /* Get UTC */
    //gmtime_r(&rawtime.tv_sec, &cooked);

    /* Figure out the name of the file we'll be writing to */
    if(i->guildcard) {
        sprintf(str, "logs/%u.%02u.%02u.%02u.%02u.%02u.%03u-%d",
                rawtime.wYear, rawtime.wMonth, rawtime.wDay,
                rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
                rawtime.wMilliseconds, i->guildcard);
    }
    else {
        sprintf(str, "logs/%u.%02u.%02u.%02u.%02u.%02u.%03u",
                rawtime.wYear, rawtime.wMonth, rawtime.wDay,
                rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
                rawtime.wMilliseconds);
    }

    fp = fopen(str, "wt");

    if(!fp) {
        pthread_mutex_unlock(&i->mutex);
        return -2;
    }

    /* Write a nice header to the log */
    now = time(NULL);
    ctime_s(str, sizeof(str), &now);
    str[strlen(str) - 1] = 0;

    fprintf(fp, "[%s] Packet 日志开始\n", str);
    i->logfile = fp;

    /* We're done, so clean up */
    pthread_mutex_unlock(&i->mutex);
    return 0;
}

/* Stop logging the specified client's packets */
int pkt_log_stop(ship_client_t *i) {
    time_t now;
    char str[64];

    pthread_mutex_lock(&i->mutex);

    if(!i->logfile) {
        pthread_mutex_unlock(&i->mutex);
        return -1;
    }

    /* Write a nice footer to the log */
    now = time(NULL);
    ctime_s(str, sizeof(str), &now);
    str[strlen(str) - 1] = 0;

    fprintf(i->logfile, "[%s] Packet 日志结束\n", str);
    fclose(i->logfile);
    i->logfile = NULL;

    /* We're done, so clean up */
    pthread_mutex_unlock(&i->mutex);
    return 0;
}

/* Begin logging the specified team */
int team_log_start(lobby_t *i) {
    //struct timeval rawtime;
    //struct tm cooked;
    char str[128];
    FILE *fp;
    time_t now;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    pthread_mutex_lock(&i->mutex);

    if(i->logfp) {
        pthread_mutex_unlock(&i->mutex);
        return -1;
    }

    /* Get the timestamp */
    //gettimeofday(&rawtime, NULL);

    /* Get UTC */
    //gmtime_r(&rawtime.tv_sec, &cooked);

    /* Figure out the name of the file we'll be writing to */
    sprintf(str, "logs/team/%u.%02u.%02u.%02u.%02u.%02u.%03u-%d",
            rawtime.wYear, rawtime.wMonth, rawtime.wDay,
            rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
            rawtime.wMilliseconds, (int)i->lobby_id);
    fp = fopen(str, "wt");

    if(!fp) {
        pthread_mutex_unlock(&i->mutex);
        return -2;
    }

    /* Write a nice header to the log */
    now = time(NULL);
    ctime_s(str, sizeof(str), &now);
    str[strlen(str) - 1] = 0;

    fprintf(fp, "[%s] Team 日志开始\n", str);
    lobby_print_info(i, i->logfp);
    i->logfp = fp;

    /* We're done, so clean up */
    pthread_mutex_unlock(&i->mutex);
    return 0;
}

/* Stop logging the specified team's packets */
int team_log_stop(lobby_t *i) {
    time_t now;
    char str[64];

    pthread_mutex_lock(&i->mutex);

    if(!i->logfp) {
        pthread_mutex_unlock(&i->mutex);
        return -1;
    }

    /* Write a nice footer to the log */
    now = time(NULL);
    ctime_s(str, sizeof(str), &now);
    str[strlen(str) - 1] = 0;

    fprintf(i->logfp, "[%s] Team 日志结束\n", str);
    lobby_print_info(i, i->logfp);

    fclose(i->logfp);
    i->logfp = NULL;

    /* We're done, so clean up */
    pthread_mutex_unlock(&i->mutex);
    return 0;
}

int team_log_write(lobby_t *l, uint32_t msg_type, const char *fmt, ...) {
    va_list args;
    int rv;

    /* Don't bother if we're not logging this team... */
    if(!l->logfp)
        return 0;

    /* XXXX: Check the message type. */
    (void)msg_type;

    va_start(args, fmt);
    rv = vfdebug(l->logfp, fmt, args);
    va_end(args);

    return rv;
}

void *xmalloc(size_t size) {
    void *rv = malloc(size);

    if(!rv)
        ERR_EXIT("malloc");

    return rv;
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

int my_pton(int family, const char *str, struct sockaddr_storage *addr) {
    switch(family) {
        case PF_INET:
        {
            struct sockaddr_in *a = (struct sockaddr_in *)addr;
            return inet_pton(family, str, &a->sin_addr);
        }

        case PF_INET6:
        {
            struct sockaddr_in6 *a = (struct sockaddr_in6 *)addr;
            return inet_pton(family, str, &a->sin6_addr);
        }
    }

    return -1;
}

int open_sock(int family, uint16_t port) {
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
        val = 1;
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

const char *skip_lang_code(const char *input) {
    if(!input || input[0] == '\0') {
        return NULL;
    }

    if(input[0] == '\t' && (input[1] == 'E' || input[1] == 'J')) {
        return input + 2;
    }

    return input;
}

static void convert_dcpcgc_to_bb(ship_client_t *s, uint8_t *buf) {
    psocn_bb_char_t *c;
    v1_player_t *sp = &s->pl->v1;
    int i;

    /* Inventory doesn't change... */
    memcpy(buf, &s->pl->v1.inv, sizeof(inventory_t));

    /* Copy the character data now... */
    c = (psocn_bb_char_t *)(buf + sizeof(inventory_t));
    memset(c, 0, sizeof(psocn_bb_char_t));
    c->disp.stats.atp = sp->character.disp.stats.atp;
    c->disp.stats.mst = sp->character.disp.stats.mst;
    c->disp.stats.evp = sp->character.disp.stats.evp;
    c->disp.stats.hp = sp->character.disp.stats.hp;
    c->disp.stats.dfp = sp->character.disp.stats.dfp;
    c->disp.stats.ata = sp->character.disp.stats.ata;
    c->disp.stats.lck = sp->character.disp.stats.lck;
    for (int i = 0; i < 10;i++) {
        c->disp.opt_flag[i] = sp->character.disp.opt_flag[i];
    }
    //c->disp.unk1 = sp->unk1;
    //c->disp.unk2[0] = sp->unk2[0];
    //c->disp.unk2[1] = sp->unk2[1];
    c->disp.level = sp->character.disp.level;
    c->disp.exp = sp->character.disp.exp;
    c->disp.meseta = sp->character.disp.meseta;
    strcpy(c->disp.dress_data.guildcard_name, "         0");
    c->disp.dress_data.dress_unk1 = sp->character.disp.dress_data.dress_unk1;
    c->disp.dress_data.dress_unk2 = sp->character.disp.dress_data.dress_unk2;
    c->disp.dress_data.name_color_b = sp->character.disp.dress_data.name_color_b;
    c->disp.dress_data.name_color_g = sp->character.disp.dress_data.name_color_g;
    c->disp.dress_data.name_color_r = sp->character.disp.dress_data.name_color_r;
    c->disp.dress_data.name_color_transparency = sp->character.disp.dress_data.name_color_transparency;
    c->disp.dress_data.model = sp->character.disp.dress_data.model;
    memcpy(c->disp.dress_data.dress_unk3, sp->character.disp.dress_data.dress_unk3, 11);
    c->disp.dress_data.create_code = sp->character.disp.dress_data.create_code;
    c->disp.dress_data.name_color_checksum = sp->character.disp.dress_data.name_color_checksum;
    c->disp.dress_data.section = sp->character.disp.dress_data.section;
    c->disp.dress_data.ch_class = sp->character.disp.dress_data.ch_class;
    c->disp.dress_data.v2flags = sp->character.disp.dress_data.v2flags;
    c->disp.dress_data.version = sp->character.disp.dress_data.version;
    c->disp.dress_data.v1flags = sp->character.disp.dress_data.v1flags;
    c->disp.dress_data.costume = sp->character.disp.dress_data.costume;
    c->disp.dress_data.skin = sp->character.disp.dress_data.skin;
    c->disp.dress_data.face = sp->character.disp.dress_data.face;
    c->disp.dress_data.head = sp->character.disp.dress_data.head;
    c->disp.dress_data.hair = sp->character.disp.dress_data.hair;
    c->disp.dress_data.hair_r = sp->character.disp.dress_data.hair_r;
    c->disp.dress_data.hair_g = sp->character.disp.dress_data.hair_g;
    c->disp.dress_data.hair_b = sp->character.disp.dress_data.hair_b;
    c->disp.dress_data.prop_x = sp->character.disp.dress_data.prop_x;
    c->disp.dress_data.prop_y = sp->character.disp.dress_data.prop_y;
    memcpy(c->config, sp->character.config, 0x48);
    memcpy(c->techniques, sp->character.techniques, 0x14);

    /* Copy the name over */
    c->name[0] = LE16('\t');
    c->name[1] = LE16('J');

    for(i = 2; i < BB_CHARACTER_NAME_LENGTH; ++i) {
        c->name[i] = LE16(sp->character.disp.dress_data.guildcard_name[i - 2]);
    }
}

static void convert_bb_to_dcpcgc(ship_client_t *s, uint8_t *buf) {
    psocn_bb_char_t *sp = &s->pl->bb.character;
    v1_player_t *c = (v1_player_t *)buf;

    memset(c, 0, sizeof(v1_player_t));

    /* Inventory doesn't change... */
    memcpy(buf, &s->pl->bb.inv, sizeof(inventory_t));

    /* Copy the character data now... */
    c->character.disp.stats.atp = sp->disp.stats.atp;
    c->character.disp.stats.mst = sp->disp.stats.mst;
    c->character.disp.stats.evp = sp->disp.stats.evp;
    c->character.disp.stats.hp = sp->disp.stats.hp;
    c->character.disp.stats.dfp = sp->disp.stats.dfp;
    c->character.disp.stats.ata = sp->disp.stats.ata;
    c->character.disp.stats.lck = sp->disp.stats.lck;
    for (int i = 0; i < 10; i++) {
        c->character.disp.opt_flag[i] = sp->disp.opt_flag[i];
    }
    //c->character.unk1 = sp->disp.unk1;
    //c->character.unk2[0] = sp->disp.unk2[0];
    //c->character.unk2[1] = sp->disp.unk2[1];
    c->character.disp.level = sp->disp.level;
    c->character.disp.exp = sp->disp.exp;
    c->character.disp.meseta = sp->disp.meseta;
    strcpy(c->character.disp.dress_data.guildcard_name, "---");
    c->character.disp.dress_data.dress_unk1 = sp->disp.dress_data.dress_unk1;
    c->character.disp.dress_data.dress_unk2 = sp->disp.dress_data.dress_unk2;
    c->character.disp.dress_data.name_color_b = sp->disp.dress_data.name_color_b;
    c->character.disp.dress_data.name_color_g = sp->disp.dress_data.name_color_g;
    c->character.disp.dress_data.name_color_r = sp->disp.dress_data.name_color_r;
    c->character.disp.dress_data.name_color_transparency = sp->disp.dress_data.name_color_transparency;
    c->character.disp.dress_data.model = sp->disp.dress_data.model;
    memcpy(c->character.disp.dress_data.dress_unk3, sp->disp.dress_data.dress_unk3, 11);
    c->character.disp.dress_data.create_code = sp->disp.dress_data.create_code;
    c->character.disp.dress_data.name_color_checksum = sp->disp.dress_data.name_color_checksum;
    c->character.disp.dress_data.section = sp->disp.dress_data.section;
    c->character.disp.dress_data.ch_class = sp->disp.dress_data.ch_class;
    c->character.disp.dress_data.v2flags = sp->disp.dress_data.v2flags;
    c->character.disp.dress_data.version = sp->disp.dress_data.version;
    c->character.disp.dress_data.v1flags = sp->disp.dress_data.v1flags;
    c->character.disp.dress_data.costume = sp->disp.dress_data.costume;
    c->character.disp.dress_data.skin = sp->disp.dress_data.skin;
    c->character.disp.dress_data.face = sp->disp.dress_data.face;
    c->character.disp.dress_data.head = sp->disp.dress_data.head;
    c->character.disp.dress_data.hair = sp->disp.dress_data.hair;
    c->character.disp.dress_data.hair_r = sp->disp.dress_data.hair_r;
    c->character.disp.dress_data.hair_g = sp->disp.dress_data.hair_g;
    c->character.disp.dress_data.hair_b = sp->disp.dress_data.hair_b;
    c->character.disp.dress_data.prop_x = sp->disp.dress_data.prop_x;
    c->character.disp.dress_data.prop_y = sp->disp.dress_data.prop_y;
    memcpy(c->character.config, sp->config, 0x48);
    memcpy(c->character.techniques, sp->techniques, 0x14);

    /* Copy the name over */
    istrncpy16_raw(ic_utf16_to_ascii, c->character.disp.dress_data.guildcard_name, &sp->name[2], 16, BB_CHARACTER_NAME_LENGTH);
}

void make_disp_data(ship_client_t *s, ship_client_t *d, void *buf) {
    uint8_t *bp = (uint8_t *)buf;

    if((s->version < CLIENT_VERSION_BB || s->version == CLIENT_VERSION_XBOX) &&
       (d->version < CLIENT_VERSION_BB || d->version == CLIENT_VERSION_XBOX)) {
        /* Neither are Blue Burst -- trivial */
        memcpy(buf, &s->pl->v1, sizeof(v1_player_t));
    }
    else if(s->version == d->version) {
        /* Both are Blue Burst -- easy */
        memcpy(bp, &s->pl->bb.inv, sizeof(inventory_t));
        bp += sizeof(inventory_t);
        memcpy(bp, &s->pl->bb.character, sizeof(psocn_bb_char_t));
    }
    else if(s->version != CLIENT_VERSION_BB) {
        /* The data we're copying is from an earlier version... */
        convert_dcpcgc_to_bb(s, bp);
    }
    else if(d->version != CLIENT_VERSION_BB) {
        /* The data we're copying is from Blue Burst... */
        convert_bb_to_dcpcgc(s, bp);
    }
}

void update_lobby_event(void) {
    int j;
    uint32_t i;
    block_t *b;
    lobby_t *l;
    ship_client_t *c2;
    uint8_t event = ship->lobby_event;

    /* Go through all the blocks... */
    for(i = 0; i < ship->cfg->blocks; ++i) {
        b = ship->blocks[i];

        if(b && b->run) {
            pthread_rwlock_rdlock(&b->lobby_lock);

            /* ... and set the event code on each default lobby. */
            TAILQ_FOREACH(l, &b->lobbies, qentry) {
                pthread_mutex_lock(&l->mutex);

                if(l->type == LOBBY_TYPE_DEFAULT) {
                    l->event = event;

                    for(j = 0; j < l->max_clients; ++j) {
                        if(l->clients[j] != NULL) {
                            c2 = l->clients[j];

                            pthread_mutex_lock(&c2->mutex);

                            if(c2->version > CLIENT_VERSION_PC) {
                                send_simple(c2, LOBBY_EVENT_TYPE, event);
                            }

                            pthread_mutex_unlock(&c2->mutex);
                        }
                    }
                }

                pthread_mutex_unlock(&l->mutex);
            }

            pthread_rwlock_unlock(&b->lobby_lock);
        }
    }
}

uint64_t get_ms_time(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void memcpy_str(void * restrict d, const char * restrict s, size_t sz) {
    size_t l = strlen(s);
    memcpy(d, s, sz < l ? sz : l);
}

/* Initialize mini18n support. */
void init_i18n(void) {
#ifdef HAVE_LIBMINI18N
    int i;
    char filename[256];

    for(i = 0; i < CLIENT_LANG_ALL; ++i) {
        langs[i] = mini18n_create();

        if(langs[i]) {
            sprintf(filename, "System\\Lang\\ship_server-%s.yts", language_codes[i]);

            /* Attempt to load the l10n file. */
            if(mini18n_load(langs[i], filename)) {
                /* If we didn't get it, clean up. */
                mini18n_destroy(langs[i]);
                langs[i] = NULL;
            }
            else {
                SHIPS_LOG("读取 l10n 多语言文件 %s", language_codes[i]);
            }
        }
    }
#endif
}

/* Clean up when we're done with mini18n. */
void cleanup_i18n(void) {
#ifdef HAVE_LIBMINI18N
    int i;

    /* Just call the destroy function... It'll handle null values fine. */
    for(i = 0; i < CLIENT_LANG_ALL; ++i) {
        mini18n_destroy(langs[i]);
    }
#endif
}
