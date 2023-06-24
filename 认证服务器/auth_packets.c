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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
//#include <iconv.h>
#include <ctype.h>
#include <stdarg.h>
#include <WinSock_Defines.h>

#include <f_logs.h>
#include <f_iconv.h>
#include <debug.h>
#include <psoconfig.h>
#include <encryption.h>
#include <database.h>
#include <quest.h>

#include <pso_cmd_code.h>
#include <pso_menu.h>

#include "auth_packets.h"
#include "patch_stubs.h"

extern psocn_dbconn_t conn;
extern psocn_config_t *cfg;
extern psocn_quest_list_t qlist[CLIENT_AUTH_VERSION_COUNT][CLIENT_LANG_ALL];
const void* my_ntop(struct sockaddr_storage* addr, char str[INET6_ADDRSTRLEN]);

uint8_t sendbuf[65536];

static void ascii_to_utf16(const char *in, uint16_t *out, int maxlen) {
    while(*in && maxlen) {
        *out++ = LE16(*in++);
        --maxlen;
    }

    while(maxlen--) {
        *out++ = 0;
    }
}

/* Send a raw packet away. */
static int send_raw(login_client_t *c, int len) {
    ssize_t rv, total = 0;
    void *tmp;

    /* Keep trying until the whole thing's sent. */
    if(!c->sendbuf_cur) {
        while(total < len) {
            rv = send(c->sock, sendbuf + total, len - total, 0);

            if(rv == SOCKET_ERROR && errno != EAGAIN) {
                return -1;
            }
            else if(rv == SOCKET_ERROR) {
                break;
            }

            total += rv;
        }
    }

    rv = len - total;

    if(rv) {
        /* Move out any already transferred data. */
        if(c->sendbuf_start) {
            memmove(c->sendbuf, c->sendbuf + c->sendbuf_start,
                    c->sendbuf_cur - c->sendbuf_start);
            c->sendbuf_cur -= c->sendbuf_start;
        }

        /* See if we need to reallocate the buffer. */
        if(c->sendbuf_cur + rv > c->sendbuf_size) {
            tmp = realloc(c->sendbuf, c->sendbuf_cur + rv);

            /* If we can't allocate the space, bail. */
            if(tmp == NULL) {
                return -1;
            }

            c->sendbuf_size = c->sendbuf_cur + rv;
            c->sendbuf = (unsigned char *)tmp;
        }

        /* Copy what's left of the packet into the output buffer. */
        memcpy(c->sendbuf + c->sendbuf_cur, sendbuf + total, rv);
        c->sendbuf_cur += rv;
    }

    return 0;
}

/* 加密并发送一个数据包. */
static int crypt_send(login_client_t *c, int len) {
    /* Expand it to be a multiple of 8/4 bytes long */
    while(len & (client_type[c->type].hdr_size - 1)) {
        sendbuf[len++] = 0;
    }

    /* Encrypt the packet */
    CRYPT_CryptData(&c->server_cipher, sendbuf, len, 1);

    return send_raw(c, len);
}

/* Send a Dreamcast/PC Welcome packet to the given client. */
int send_dc_welcome(login_client_t *c, uint32_t svect, uint32_t cvect) {
    dc_welcome_pkt *pkt = (dc_welcome_pkt *)sendbuf;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(dc_welcome_pkt));

    /* 填充数据头 */
    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        pkt->hdr.dc.pkt_len = LE16(DC_WELCOME_LENGTH);
        pkt->hdr.dc.pkt_type = LOGIN_WELCOME_TYPE;
    }
    else {
        pkt->hdr.pc.pkt_len = LE16(DC_WELCOME_LENGTH);
        pkt->hdr.pc.pkt_type = LOGIN_WELCOME_TYPE;
    }

    /* Fill in the required message */
    memcpy(pkt->copyright, login_dc_welcome_copyright, 52);

    /* Fill in the anti message */
    memcpy(pkt->after_message, anti_copyright, 188);

    /* Fill in the encryption vectors */
    pkt->svect = LE32(svect);
    pkt->cvect = LE32(cvect);

    /* 将数据包发送出去 */
    return send_raw(c, DC_WELCOME_LENGTH);
}

/* Send a Blue Burst Welcome packet to the given client. */
int send_bb_welcome(login_client_t *c, const uint8_t svect[48],
                    const uint8_t cvect[48]) {
    bb_welcome_pkt *pkt = (bb_welcome_pkt *)sendbuf;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(bb_welcome_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(BB_WELCOME_LENGTH);
    pkt->hdr.pkt_type = LE16(BB_WELCOME_TYPE);

    /* Fill in the required message */
    memcpy(pkt->copyright, login_bb_welcome_copyright, 75);

    /* Fill in the anti message */
    memcpy(pkt->after_message, anti_copyright, 188);

    /* Fill in the encryption vectors */
    memcpy(pkt->server_key, svect, 48);
    memcpy(pkt->client_key, cvect, 48);

    /* 将数据包发送出去 */
    return send_raw(c, BB_WELCOME_LENGTH);
}

static int send_large_msg_dc(login_client_t* c, uint16_t type, const char* fmt,
    va_list args) {
    dc_msg_box_pkt* pkt = (dc_msg_box_pkt*)sendbuf;
    int size = 4;
    iconv_t ic;
    //char tm[512];
    size_t in, out;
    char* inptr;
    char* outptr;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, sizeof(dc_msg_box_pkt));

    /* Do the formatting */
    //vsnprintf(tm, 512, fmt, args);
    //tm[511] = '\0';
    //in = strlen(fmt) + 1;

    /* Make sure we have a language code tag */
    //if (fmt[0] != '\t' || (fmt[1] != 'E' && fmt[1] != 'J')) {
    //    /* Assume Non-Japanese if we don't have a marker. */
    //    memmove(&fmt[2], &fmt[0], in);
    //    fmt[0] = '\t';
    //    fmt[1] = 'E';
    //    in += 2;
    //}

    if (c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
        c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
        c->type == CLIENT_AUTH_XBOX) {
        if (fmt[1] != 'J') {
            ic = ic_gbk_to_8859;
        }
        else {
            ic = ic_gbk_to_sjis;
        }
        //if (tm[1] == 'J') {
        //    //ic = iconv_open(SJIS, GBK);
        //    ic = ic_gbk_to_sjis;
        //}
        //else {
        //    //ic = iconv_open(ISO8859, GBK);
        //    ic = ic_gbk_to_8859;
        //}
    }
    else {
        //ic = iconv_open(UTF16LE, GBK);
        ic = ic_gbk_to_utf16;
    }

    //if (ic == (iconv_t)-1) {
    //    perror("iconv_open");
    //    return -1;
    //}

    /* Convert to the proper encoding */
    in = strlen(fmt) + 1;
    out = 65524;
    inptr = (char*)fmt;
    outptr = (char*)pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);
    iconv_close(ic);

    /* Figure out how long the packet is */
    size += 65524 - out;

    /* Pad to a length divisible by 4 */
    while (size & 0x03) {
        sendbuf[size++] = 0;
    }

    /* 填充数据头 */
    if (c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
        c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
        c->type == CLIENT_AUTH_XBOX) {
        pkt->hdr.dc.pkt_type = (uint8_t)type;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(size);
    }
    else {
        pkt->hdr.pc.pkt_type = (uint8_t)type;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(size);
    }

    /* 将数据包发送出去 */
    return crypt_send(c, size);
}

static int send_msg_bb(login_client_t *c, uint16_t type, const char* fmt,
                           va_list args) {
    bb_msg_box_pkt *pkt = (bb_msg_box_pkt *)sendbuf;
    int size = 8;
    iconv_t ic;
    //char tm[512];
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, sizeof(bb_msg_box_pkt));

    /* Do the formatting */
    //vsnprintf(tm, 512, fmt, args);
    //tm[511] = '\0';
    in = strlen(fmt) + 1;

    /* Make sure we have a language code tag */
    //if (fmt[0] != '\t' || (fmt[1] != 'E' && fmt[1] != 'J')) {
    //    /* Assume Non-Japanese if we don't have a marker. */
    //    memmove(&fmt[2], &fmt[0], in);
    //    fmt[0] = '\t';
    //    fmt[1] = 'E';
    //    in += 2;
    //}

    //ic = iconv_open(UTF16LE, GBK);
    ic = ic_gbk_to_utf16;

    //if(ic == (iconv_t)-1) {
    //    perror("iconv_open");
    //    return -1;
    //}

    /* Convert to the proper encoding */
    in = strlen(fmt) + 1;
    out = 65520;
    inptr = (char*)fmt;
    outptr = (char *)pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);
    iconv_close(ic);

    /* Figure out how long the packet is */
    size += 65520 - out + 0x10;

    sendbuf[size++] = 0;
    sendbuf[size++] = 0;

    /* Pad to a length divisible by 4 */
    while(size & 0x07) {
        sendbuf[size++] = 0;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(type);
    pkt->hdr.flags = 0;
    pkt->hdr.pkt_len = LE16(size);

    /* 将数据包发送出去 */
    return crypt_send(c, size);
}

int send_large_msg(login_client_t *c/*, const char msg[]*/, const char* fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch(c->type) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_PC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            return send_large_msg_dc(c, MSG_BOX_TYPE, fmt, args);

        case CLIENT_AUTH_BB_LOGIN:
        case CLIENT_AUTH_BB_CHARACTER:
            return send_msg_bb(c, MSG_BOX_TYPE, fmt, args);
    }

    va_end(args);

    return -1;
}

/* Send the Dreamcast security packet to the given client. */
int send_dc_security(login_client_t *c, uint32_t gc, const void *data,
                     int data_len) {
    dc_security_pkt *pkt = (dc_security_pkt *)sendbuf;

    /* Wipe the packet */
    memset(pkt, 0, sizeof(dc_security_pkt));

    /* 填充数据头 */
    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        pkt->hdr.dc.pkt_type = SECURITY_TYPE;
        pkt->hdr.dc.pkt_len = LE16((0x0C + data_len));
    }
    else {
        pkt->hdr.pc.pkt_type = SECURITY_TYPE;
        pkt->hdr.pc.pkt_len = LE16((0x0C + data_len));
    }

    /* Fill in the guildcard/tag */
    pkt->tag = LE32(0x00010000);
    pkt->guildcard = LE32(gc);

    /* Copy over any security data */
    if(data_len)
        memcpy(pkt->security_data, data, data_len);

    /* 将数据包发送出去 */
    return crypt_send(c, 0x0C + data_len);
}

/* Send a Blue Burst security packet to the given client. */
int send_bb_security(login_client_t *c, uint32_t gc, uint32_t err,
                     uint32_t guild_id, const void *data, int data_len) {
    bb_security_pkt *pkt = (bb_security_pkt *)sendbuf;

    //DBG_LOG("send_bb_security %d", data_len);

    /* Make sure the data is sane */
    if(data_len > 40 || data_len < 0) {
        return -1;
    }

    /* Wipe the packet */
    memset(pkt, 0, sizeof(bb_security_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(BB_SECURITY_LENGTH);
    pkt->hdr.pkt_type = LE16(BB_SECURITY_TYPE);

    /* Fill in the information */
    pkt->err_code = LE32(err);
    pkt->player_tag = LE32(0x00010000);
    pkt->guildcard = LE32(gc);
    pkt->guild_id = LE32(guild_id);
    pkt->caps = LE32(0x00000102);   /* ??? - newserv sets it this way */

    /* Copy over any security data */
    if(data_len)
        memcpy(pkt->security_data, data, data_len);

    //print_payload((uint8_t*)pkt, BB_SECURITY_LENGTH);

    /* 将数据包发送出去 */
    return crypt_send(c, BB_SECURITY_LENGTH);
}

/* Send a redirect packet to the given client. */
static int send_redirect_bb(login_client_t *c, in_addr_t ip, uint16_t port) {
    bb_redirect_pkt *pkt = (bb_redirect_pkt *)sendbuf;

    /* Wipe the packet */
    memset(pkt, 0, BB_REDIRECT_LENGTH);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(REDIRECT_TYPE);
    pkt->hdr.pkt_len = LE16(BB_REDIRECT_LENGTH);

    /* Fill in the IP and port */
    pkt->ip_addr = ip;
    pkt->port = LE16(port);

    /* 将数据包发送出去 */
    return crypt_send(c, BB_REDIRECT_LENGTH);
}

static int send_redirect_dc(login_client_t *c, in_addr_t ip, uint16_t port) {
    dc_redirect_pkt *pkt = (dc_redirect_pkt *)sendbuf;

    /* Wipe the packet */
    memset(pkt, 0, DC_REDIRECT_LENGTH);

    /* 填充数据头 */
    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        pkt->hdr.dc.pkt_type = REDIRECT_TYPE;
        pkt->hdr.dc.pkt_len = LE16(DC_REDIRECT_LENGTH);
    }
    else {
        pkt->hdr.pc.pkt_type = REDIRECT_TYPE;
        pkt->hdr.pc.pkt_len = LE16(DC_REDIRECT_LENGTH);
    }

    /* Fill in the IP and port */
    pkt->ip_addr = ip;
    pkt->port = LE16(port);

    /* 将数据包发送出去 */
    return crypt_send(c, DC_REDIRECT_LENGTH);
}

int send_redirect(login_client_t *c, in_addr_t ip, uint16_t port) {
    /* Call the appropriate function. */
    switch(c->type) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_PC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            return send_redirect_dc(c, ip, port);

        case CLIENT_AUTH_BB_LOGIN:
        case CLIENT_AUTH_BB_CHARACTER:
            return send_redirect_bb(c, ip, port);
    }

    return -1;
}

#ifdef PSOCN_ENABLE_IPV6
/* Send a redirect packet (IPv6) to the given client. */
static int send_redirect6_dc(login_client_t *c, const uint8_t ip[16],
                             uint16_t port) {
    dc_redirect6_pkt *pkt = (dc_redirect6_pkt *)sendbuf;

    /* Wipe the packet */
    memset(pkt, 0, DC_REDIRECT6_LENGTH);

    /* 填充数据头 */
    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        pkt->hdr.dc.pkt_type = REDIRECT_TYPE;
        pkt->hdr.dc.pkt_len = LE16(DC_REDIRECT6_LENGTH);
        pkt->hdr.dc.flags = 6;
    }
    else {
        pkt->hdr.pc.pkt_type = REDIRECT_TYPE;
        pkt->hdr.pc.pkt_len = LE16(DC_REDIRECT6_LENGTH);
        pkt->hdr.pc.flags = 6;
    }

    /* Fill in the IP and port */
    memcpy(pkt->ip_addr, ip, 16);
    pkt->port = LE16(port);

    /* 将数据包发送出去 */
    return crypt_send(c, DC_REDIRECT6_LENGTH);
}

int send_redirect6(login_client_t *c, const uint8_t ip[16], uint16_t port) {
    /* Call the appropriate function. */
    switch(c->type) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_PC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            return send_redirect6_dc(c, ip, port);
    }

    return -1;
}
#endif

/* Send a packet to clients connecting on the Gamecube port to sort out any PC
   clients that might end up there. This must be sent before encryption is set
   up! */
static int send_selective_redirect_ipv4(login_client_t *c) {
    dc_redirect_pkt *pkt = (dc_redirect_pkt *)sendbuf;
    dc_pkt_hdr_t *hdr2 = (dc_pkt_hdr_t *)(sendbuf + 0x19);
    in_addr_t addr = cfg->srvcfg.server_ip;

    /* Wipe the packet */
    memset(pkt, 0, 0xB0);

    /* Fill in the redirect packet. PC users will parse this out as a type 0x19
       (Redirect) with size 0xB0. GC/DC users would parse it out as a type 0xB0
       (Ignored) with a size of 0x19. The second header takes care of the rest
       of the 0xB0 size. */
    pkt->hdr.pc.pkt_type = REDIRECT_TYPE;
    pkt->hdr.pc.pkt_len = LE16(0x00B0);
    pkt->ip_addr = addr;
    pkt->port = LE16(9300);

    /* Fill in the secondary header */
    hdr2->pkt_type = 0xB0;
    hdr2->pkt_len = LE16(0x0097);

    /* 加密并发送 */
    return send_raw(c, 0xB0);
}

int send_selective_redirect(login_client_t *c) {
#ifdef PSOCN_ENABLE_IPV6
    if(c->is_ipv6) {
        /* This is handled in the proxy for IPv6. */
        return 0;
    }
#endif

    return send_selective_redirect_ipv4(c);
}

/* Send a timestamp packet to the given client. */
static int send_timestamp_dc(login_client_t *c) {
	dc_timestamp_pkt* pkt = (dc_timestamp_pkt*)sendbuf;
	SYSTEMTIME rawtime;

	/* Wipe the packet */
	memset(pkt, 0, DC_TIMESTAMP_LENGTH);

	/* 填充数据头 */
	if (c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
		c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
		c->type == CLIENT_AUTH_XBOX) {
		pkt->hdr.dc.pkt_type = TIMESTAMP_TYPE;
		pkt->hdr.dc.pkt_len = LE16(DC_TIMESTAMP_LENGTH);
	}
	else {
		pkt->hdr.pc.pkt_type = TIMESTAMP_TYPE;
		pkt->hdr.pc.pkt_len = LE16(DC_TIMESTAMP_LENGTH);
	}

	/* Get the timestamp */
	//gettimeofday(&rawtime, NULL);
    GetLocalTime(&rawtime);

	/* Get UTC */
	//gmtime(&rawtime.tv_sec, &cooked);

	/* Fill in the timestamp */
	sprintf(pkt->timestamp, "%u:%02u:%02u: %02u:%02u:%02u.%03u",
		rawtime.wYear, rawtime.wMonth, rawtime.wDay,
		rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
		rawtime.wMilliseconds);

	/* 将数据包发送出去 */
	return crypt_send(c, DC_TIMESTAMP_LENGTH);
}

static int send_timestamp_bb(login_client_t *c) {
    bb_timestamp_pkt* pkt = (bb_timestamp_pkt*)sendbuf;
    /*struct timeval rawtime;
    struct tm cooked;*/
    SYSTEMTIME rawtime;

    /* Wipe the packet */
    memset(pkt, 0, BB_TIMESTAMP_LENGTH);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(TIMESTAMP_TYPE);
    pkt->hdr.pkt_len = LE16(BB_TIMESTAMP_LENGTH);

    /* Get the timestamp */
    //gettimeofday(&rawtime, NULL);
    GetLocalTime(&rawtime);

    /* Get UTC */
    //gmtime(&rawtime.tv_sec);

    /* Fill in the timestamp */
    sprintf(pkt->timestamp, "%u:%02u:%02u: %02u:%02u:%02u.%03u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond,
        rawtime.wMilliseconds);

    /* 将数据包发送出去 */
    return crypt_send(c, BB_TIMESTAMP_LENGTH);
}

int send_timestamp(login_client_t *c) {
    /* Call the appropriate function */
    switch(c->type) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_PC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            return send_timestamp_dc(c);

        case CLIENT_AUTH_BB_CHARACTER:
            return send_timestamp_bb(c);
    }

    return -1;
}

/* Send the initial menu to clients, with the options of "Ship Select" and
   "Download". */
static int send_initial_menu_dc(login_client_t *c) {
    dc_ship_list_pkt *pkt = (dc_ship_list_pkt *)sendbuf;
    uint32_t count = 0, len = 0x100, len2 = 0x1C, len3 = 0x22;
    uint32_t v = c->ext_version & CLIENT_EXTVER_DC_VER_MASK;

    /* 初始化数据包 */
    memset(pkt, 0, len);

    /* 填充菜单实例 */
    for (count; count < _countof(pso_initial_menu_auth_dc); ++count) {
        pkt->entries[count].menu_id = LE32(pso_initial_menu_auth_dc[count]->menu_id);
        pkt->entries[count].item_id = LE32(pso_initial_menu_auth_dc[count]->item_id);
        pkt->entries[count].flags = LE16(pso_initial_menu_auth_dc[count]->flag);
        istrncpy(ic_gbk_to_utf8, (char*)pkt->entries[count].name, pso_initial_menu_auth_dc[count]->name, len3);
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.flags = count;
    pkt->hdr.pkt_len = LE16(len);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

static int send_initial_menu_pc(login_client_t *c) {
    pc_ship_list_pkt* pkt = (pc_ship_list_pkt*)sendbuf;
    uint32_t count = 0, len = 0x100, len2 = 0x1C, len3 = 0x20;

    /* 初始化数据包 */
    memset(pkt, 0, len);

    /* 填充菜单实例 */
    for (count; count < _countof(pso_initial_menu_auth_pc); ++count) {
        pkt->entries[count].menu_id = LE32(pso_initial_menu_auth_pc[count]->menu_id);
        pkt->entries[count].item_id = LE32(pso_initial_menu_auth_pc[count]->item_id);
        pkt->entries[count].flags = LE16(pso_initial_menu_auth_pc[count]->flag);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, pso_initial_menu_auth_pc[count]->name, len3);
        len += len2;
    }

    /* 填充数据包头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.flags = count;
    pkt->hdr.pkt_len = LE16(len);

    /* 发送数据 */
    return crypt_send(c, len);
}

static int send_initial_menu_gc(login_client_t *c) {
    dc_ship_list_pkt *pkt = (dc_ship_list_pkt *)sendbuf;
    uint32_t count = 0, len = 0x100, len2 = 0x1C, len3 = 0x20;

    /* 初始化数据包 */
    memset(pkt, 0, len);

    /* 填充菜单实例 */
    for (count; count < _countof(pso_initial_menu_auth_gc); ++count) {
        pkt->entries[count].menu_id = LE32(pso_initial_menu_auth_gc[count]->menu_id);
        pkt->entries[count].item_id = LE32(pso_initial_menu_auth_gc[count]->item_id);
        pkt->entries[count].flags = LE16(pso_initial_menu_auth_gc[count]->flag);
        istrncpy(ic_gbk_to_utf8, (char*)pkt->entries[count].name, pso_initial_menu_auth_gc[count]->name, len3);
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.flags = count;
    pkt->hdr.pkt_len = LE16(len);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

static int send_initial_menu_xbox(login_client_t *c) {
    dc_ship_list_pkt *pkt = (dc_ship_list_pkt *)sendbuf;
    uint32_t count = 0, len = 0x100, len2 = 0x1C, len3 = 0x20;

    /* 初始化数据包 */
    memset(pkt, 0, len);

    /* 填充菜单实例 */
    for (count; count < _countof(pso_initial_menu_auth_gc); ++count) {
        pkt->entries[count].menu_id = LE32(pso_initial_menu_auth_gc[count]->menu_id);
        pkt->entries[count].item_id = LE32(pso_initial_menu_auth_gc[count]->item_id);
        pkt->entries[count].flags = LE16(pso_initial_menu_auth_gc[count]->flag);
        istrncpy(ic_gbk_to_utf8, (char*)pkt->entries[count].name, pso_initial_menu_auth_gc[count]->name, len3);
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.flags = count;
    pkt->hdr.pkt_len = LE16(len);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

static int send_initial_menu_bb(login_client_t* c) {
    bb_ship_list_pkt* pkt = (bb_ship_list_pkt*)sendbuf;
    uint32_t count = 0, len = 0x100, len2 = 0x1C, len3 = 0x20;

    /* 初始化数据包 */
    memset(pkt, 0, len);

    /* 填充菜单实例 */
    for (count; count < _countof(pso_initial_menu_auth_bb); ++count) {
        pkt->entries[count].menu_id = LE32(pso_initial_menu_auth_bb[count]->menu_id);
        pkt->entries[count].item_id = LE32(pso_initial_menu_auth_bb[count]->item_id);
        pkt->entries[count].flags = LE16(pso_initial_menu_auth_bb[count]->flag);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, pso_initial_menu_auth_bb[count]->name, len3);
        len += len2;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(BLOCK_LIST_TYPE);
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = count - 1;

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

int send_initial_menu(login_client_t *c) {
    /* Call the appropriate function */
    switch(c->type) {
        case CLIENT_AUTH_DC:
            return send_initial_menu_dc(c);

        case CLIENT_AUTH_PC:
            return send_initial_menu_pc(c);

        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
            return send_initial_menu_gc(c);

        case CLIENT_AUTH_XBOX:
            return send_initial_menu_xbox(c);

        case CLIENT_AUTH_BB_CHARACTER:
            return send_initial_menu_bb(c);
    }

    return -1;
}

int send_menu(login_client_t* c, uint32_t type) {
    switch (type)
    {
    default:
        return send_initial_menu(c);
    }

    return -1;
}

/* Send the list of ships to the client. */
static int send_ship_list_dc(login_client_t *c, uint16_t menu_code) {
    dc_ship_list_pkt *pkt = (dc_ship_list_pkt *)sendbuf;
    char no_ship_msg[] = "No Ships";
    char query[256];
    uint32_t num_ships = 0;
    void *result;
    char **row;
    uint32_t ship_id, players, priv;
    int i, len = 0x20, gm_only, flags, ship_num;
    char tmp[3] = { (char)menu_code, (char)(menu_code >> 8), 0 };

    /* Clear the base packet */
    memset(pkt, 0, sizeof(dc_ship_list_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;

    /* Fill in the "SHIP/cc" entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = LE32(ITEM_ID_INIT_SHIP);
    pkt->entries[0].flags = LE16(0x0004);

    if(menu_code)
        sprintf(pkt->entries[0].name, "SHIP/%c%c", (char)menu_code,
                (char)(menu_code >> 8));
    else
        strcpy(pkt->entries[0].name, "SHIP/US");

    pkt->entries[0].name[0x11] = 0x08;
    ++num_ships;

    /* Figure out what ships we might exclude by flags */
    if(c->type == CLIENT_AUTH_GC) {
        flags = 0x80;
    }
    else if(c->type == CLIENT_AUTH_EP3) {
        flags = 0x100;
    }
    else if(c->type == CLIENT_AUTH_DCNTE) {
        flags = 0x400;
    }
    else if(c->type == CLIENT_AUTH_XBOX) {
        flags = 0x800;
    }
    else {
        if(c->version == PSOCN_QUEST_V1) {
            flags = 0x10;
        }
        else {
            flags = 0x20;
        }
    }

    /* Get ready to query the database */
    sprintf(query, "SELECT ship_id, name, players, gm_only, ship_number, "
        "privileges FROM %s WHERE menu_code='%"PRIu16"' AND "
        "(flags & 0x%04x) = 0 ORDER BY ship_number", SERVER_SHIPS_ONLINE, menu_code, flags);

    /* Query the database and see what we've got */
    if(psocn_db_real_query(&conn, query)) {
        i = -1;
        goto out;
    }

    if((result = psocn_db_result_store(&conn)) == NULL) {
        i = -2;
        goto out;
    }

    /* As long as we have some rows, go */
    while((row = psocn_db_result_fetch(result))) {
        gm_only = atoi(row[3]);
        priv = strtoul(row[5], NULL, 0);

        if(gm_only && !IS_GLOBAL_GM(c))
            continue;
        else if(!IS_GLOBAL_GM(c) && (priv & c->priv) != priv)
            continue;

        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x1C);

        /* Grab info from the row */
        ship_id = (uint32_t)strtoul(row[0], NULL, 0);
        players = (uint32_t)strtoul(row[2], NULL, 0);
        ship_num = atoi(row[4]);

        /* Fill in what we have */
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_SHIP);
        pkt->entries[num_ships].item_id = LE32(ship_id);
        pkt->entries[num_ships].flags = LE16(0x0F04);

        /* Create the name string */
        if(menu_code)
            sprintf(pkt->entries[num_ships].name, "%02X:%c%c/%s", ship_num,
                    (char)menu_code, (char)(menu_code >> 8), row[1]);
        else
            sprintf(pkt->entries[num_ships].name, "%02X:%s", ship_num,
                    row[1]);

        /* We're done with this ship, increment the counter */
        ++num_ships;
        len += 0x1C;
    }

    psocn_db_result_free(result);

    /* Figure out any lists we need to allow to be seen */
    if (!IS_GLOBAL_GM(c)) {
        sprintf(query, "SELECT DISTINCT menu_code FROM %s WHERE "
            "(flags & 0x%04x) = 0 AND gm_only=0 AND "
            "(privileges & 0x%08x) = privileges ORDER BY menu_code", SERVER_SHIPS_ONLINE, flags,
            c->priv);
    }
    else {
        sprintf(query, "SELECT DISTINCT menu_code FROM %s WHERE "
            "(flags & 0x%04x) = 0 ORDER BY menu_code", SERVER_SHIPS_ONLINE, flags);
    }

    /* Query the database and see what we've got */
    if(psocn_db_real_query(&conn, query)) {
        i = -3;
        goto out;
    }

    if((result = psocn_db_result_store(&conn)) == NULL) {
        i = -4;
        goto out;
    }

    /* As long as we have some rows, go */
    while((row = psocn_db_result_fetch(result))) {
        /* Grab info from the row */
        ship_id = (uint16_t)strtoul(row[0], NULL, 0);

        /* Skip the entry we're filling in now */
        if(ship_id == menu_code)
            continue;

        tmp[0] = (char)(ship_id);
        tmp[1] = (char)(ship_id >> 8);
        tmp[2] = '\0';

        /* Make sure the values are in-bounds */
        if((tmp[0] || tmp[1]) && (!isalpha(tmp[0]) || !isalpha(tmp[1])))
            continue;

        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x1C);

        /* Fill in what we have */
        pkt->entries[num_ships].menu_id = LE32((MENU_ID_SHIP | (ship_id << 8)));
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_INIT_SHIP);
        pkt->entries[num_ships].flags = LE16(0x0F04);

        /* Create the name string */
        if(tmp[0] && tmp[1])
            sprintf(pkt->entries[num_ships].name, "SHIP/%s", tmp);
        else
            strcpy(pkt->entries[num_ships].name, "SHIP/Main");

        /* We're done with this ship, increment the counter */
        ++num_ships;
        len += 0x1C;
    }

    psocn_db_result_free(result);

    /* Make sure we have at least one ship... */
    if(num_ships == 1) {
        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x1C);
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_INIT_SHIP);
        pkt->entries[num_ships].flags = LE16(0x0000);
        strcpy(pkt->entries[num_ships].name, no_ship_msg);

        ++num_ships;
        len += 0x1C;
    }else{
        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_DISCONNECT);
        pkt->entries[num_ships].flags = LE16(0x0000);

        /* And convert to UTF8 */
        istrncpy(ic_gbk_to_utf8, (char*)pkt->entries[num_ships].name, "断开连接", 0x22);

        ++num_ships;
        len += 0x1C;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(num_ships - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len);

out:
    return i;
}

/* Send the list of ships to the client. */
static int send_ship_list_pc(login_client_t* c, uint16_t menu_code) {
    pc_ship_list_pkt* pkt = (pc_ship_list_pkt*)sendbuf;
    char no_ship_msg[] = "未找到舰船";
    char query[256] = { 0 }, tmp[18] = { 0 }, tmp2[3] = { 0 };
    uint32_t num_ships = 0;
    void* result;
    char** row;
    uint32_t ship_id, players, priv;
    int i, len = 0x30, gm_only, ship_num, flags = 0x40;

    /* Clear the base packet */
    memset(pkt, 0, sizeof(pc_ship_list_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;

    /* Fill in the "SHIP/cc" entry */
    memset(&pkt->entries[num_ships], 0, 0x2C);
    pkt->entries[num_ships].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[num_ships].item_id = LE32(ITEM_ID_INIT_SHIP);
    pkt->entries[num_ships].flags = LE16(0x0004);
    memcpy(pkt->entries[num_ships].name, "S\0H\0I\0P\0/\0U\0S\0", 14);

    if (menu_code) {
        pkt->entries[num_ships].name[5] = LE16((menu_code & 0x00FF));
        pkt->entries[num_ships].name[6] = LE16((menu_code >> 8));
    }

    ++num_ships;

    if ((c->ext_version & CLIENT_EXTVER_PCNTE))
        flags = 0x1040;

    /* Get ready to query the database */
    sprintf(query, "SELECT ship_id, name, players, gm_only, ship_number, "
        "privileges FROM %s WHERE menu_code='%"PRIu16"' AND "
        "(flags & 0x%04x) = 0 ORDER BY ship_number", SERVER_SHIPS_ONLINE, menu_code, flags);

    /* Query the database and see what we've got */
    if (psocn_db_real_query(&conn, query)) {
        i = -1;
        goto out;
    }

    if ((result = psocn_db_result_store(&conn)) == NULL) {
        i = -2;
        goto out;
    }

    /* As long as we have some rows, go */
    while ((row = psocn_db_result_fetch(result))) {
        gm_only = atoi(row[3]);
        priv = strtoul(row[5], NULL, 0);

        if (gm_only && !IS_GLOBAL_GM(c))
            continue;
        else if (!IS_GLOBAL_GM(c) && (priv & c->priv) != priv)
            continue;

        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);

        /* Grab info from the row */
        ship_id = (uint32_t)strtoul(row[0], NULL, 0);
        players = (uint32_t)strtoul(row[2], NULL, 0);
        ship_num = atoi(row[4]);

        /* Fill in what we have */
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_SHIP);
        pkt->entries[num_ships].item_id = LE32(ship_id);
        pkt->entries[num_ships].flags = LE16(0x0F04);

        /* Create the name string (UTF-8) */
        if (menu_code)
            sprintf(tmp, "%02X:%c%c/%s", ship_num, (char)menu_code,
                (char)(menu_code >> 8), row[1]);
        else
            sprintf(tmp, "%02X:%s", ship_num, row[1]);

        /* And convert to UTF-16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, tmp, 0x22);

        /* We're done with this ship, increment the counter */
        ++num_ships;
        len += 0x2C;
    }

    psocn_db_result_free(result);

    /* Figure out any lists we need to allow to be seen */
    if (!IS_GLOBAL_GM(c)) {
        sprintf(query, "SELECT DISTINCT menu_code FROM %s WHERE "
            "(flags & 0x%04x) = 0 AND gm_only=0 AND "
            "(privileges & 0x%08x) = privileges ORDER BY menu_code", SERVER_SHIPS_ONLINE, flags,
            c->priv);
    }
    else {
        sprintf(query, "SELECT DISTINCT menu_code FROM %s WHERE "
            "(flags & 0x%04x) = 0 ORDER BY menu_code", SERVER_SHIPS_ONLINE, flags);
    }

    /* Query the database and see what we've got */
    if (psocn_db_real_query(&conn, query)) {
        i = -3;
        goto out;
    }

    if ((result = psocn_db_result_store(&conn)) == NULL) {
        i = -4;
        goto out;
    }

    /* As long as we have some rows, go */
    while ((row = psocn_db_result_fetch(result))) {
        /* Grab info from the row */
        ship_id = (uint16_t)strtoul(row[0], NULL, 0);

        /* Skip the entry we're filling in now */
        if (ship_id == menu_code)
            continue;

        tmp2[0] = (char)(ship_id);
        tmp2[1] = (char)(ship_id >> 8);
        tmp2[2] = '\0';

        /* Make sure the values are in-bounds */
        if ((tmp2[0] || tmp2[1]) && (!isalpha(tmp2[0]) || !isalpha(tmp2[1])))
            continue;

        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);

        /* Fill in what we have */
        pkt->entries[num_ships].menu_id = LE32((MENU_ID_SHIP | (ship_id << 8)));
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_INIT_SHIP);
        pkt->entries[num_ships].flags = LE16(0x0F04);

        /* Create the name string (UTF-8) */
        if (tmp2[0] && tmp2[1])
            sprintf(tmp, "舰船/%s", tmp2);
        else
            strcpy(tmp, "舰船/Main");

        /* And convert to UTF-16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, tmp, 0x22);

        /* We're done with this ship, increment the counter */
        ++num_ships;
        len += 0x2C;
    }

    psocn_db_result_free(result);

    /* Make sure we have at least one ship... */
    if (num_ships == 1) {
        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_INIT_SHIP);
        pkt->entries[num_ships].flags = LE16(0x0000);

        /* And convert to UTF16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, no_ship_msg, 0x22);

        ++num_ships;
        len += 0x2C;
    }else{
        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_DISCONNECT);
        pkt->entries[num_ships].flags = LE16(0x0000);

        /* And convert to UTF16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, "断开连接", 0x22);

        ++num_ships;
        len += 0x2C;
    }

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(num_ships - 1);

    /* 将数据包发送出去 */
    return crypt_send(c, len);

out:
    return i;
}

static int send_ship_list_bb(login_client_t *c, uint16_t menu_code) {
    bb_ship_list_pkt *pkt = (bb_ship_list_pkt *)sendbuf;
    char no_ship_msg[] = "未找到舰船";
    char query[256] = { 0 }, tmp[18] = { 0 }, tmp2[3] = { 0 };
    uint32_t num_ships = 0;
    void *result;
    char **row;
    uint32_t ship_id, players, priv;
    int i, len = 0x34, gm_only, ship_num;

    /* Clear the base packet */
    memset(pkt, 0, sizeof(bb_ship_list_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;

    /* Fill in the "SHIP/cc" entry */
    memset(&pkt->entries[0], 0, 0x2C);
    pkt->entries[num_ships].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[num_ships].item_id = LE32(MENU_ID_INITIAL);
    pkt->entries[num_ships].flags = LE16(0x0004);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, "S\0H\0I\0P\0/\0U\0S\0", 14);
    //memcpy(pkt->entries[num_ships].name, "S\0H\0I\0P\0/\0U\0S\0", 14);

    if(menu_code) {
        pkt->entries[num_ships].name[5] = LE16((menu_code & 0x00FF));
        pkt->entries[num_ships].name[6] = LE16((menu_code >> 8));
    }

    num_ships = 1;

    /* Get ready to query the database */
    sprintf(query, "SELECT ship_id, name, players, gm_only, ship_number, "
        "privileges FROM %s WHERE menu_code='%" PRIu16 "' AND "
        "(flags & 0x200) = 0 ORDER BY ship_number", SERVER_SHIPS_ONLINE, menu_code);

    /* Query the database and see what we've got */
    if(psocn_db_real_query(&conn, query)) {
        i = -1;
        goto out;
    }

    if((result = psocn_db_result_store(&conn)) == NULL) {
        i = -2;
        goto out;
    }

    /* As long as we have some rows, go */
    while((row = psocn_db_result_fetch(result))) {
        gm_only = atoi(row[3]);
        priv = strtoul(row[5], NULL, 0);

        if(gm_only && !IS_GLOBAL_GM(c))
            continue;
        else if(!IS_GLOBAL_GM(c) && (priv & c->priv) != priv)
            continue;

        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);

        /* Grab info from the row */
        ship_id = (uint32_t)strtoul(row[0], NULL, 0);
        players = (uint32_t)strtoul(row[2], NULL, 0);
        ship_num = atoi(row[4]);

        /* Fill in what we have */
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_SHIP);
        pkt->entries[num_ships].item_id = LE32(ship_id);
        pkt->entries[num_ships].flags = LE16(0x0F04);

        /* Create the name string (UTF-8) */
        if(menu_code)
            sprintf(tmp, "%02X:%c%c/%s", ship_num, (char)menu_code,
                    (char)(menu_code >> 8), row[1]);
        else
            sprintf(tmp, "%02X:%s", ship_num, row[1]);

        /* And convert to UTF-16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, tmp, 0x22);

        /* We're done with this ship, increment the counter */
        ++num_ships;
        len += 0x2C;
    }

    psocn_db_result_free(result);

    /* Figure out any lists we need to allow to be seen */
    if (!IS_GLOBAL_GM(c)) {
        sprintf(query, "SELECT DISTINCT menu_code FROM %s WHERE "
            "(flags & 0x200) = 0 AND gm_only=0 AND "
            "(privileges & 0x%08x) = privileges ORDER BY menu_code"
            , SERVER_SHIPS_ONLINE
            , c->priv);
    }
    else {
        sprintf(query, "SELECT DISTINCT menu_code FROM %s WHERE "
            "(flags & 0x200) = 0 ORDER BY menu_code", SERVER_SHIPS_ONLINE);
    }

    /* Query the database and see what we've got */
    if(psocn_db_real_query(&conn, query)) {
        i = -3;
        goto out;
    }

    if((result = psocn_db_result_store(&conn)) == NULL) {
        i = -4;
        goto out;
    }

    /* As long as we have some rows, go */
    while((row = psocn_db_result_fetch(result))) {
        /* Grab info from the row */
        ship_id = (uint16_t)strtoul(row[0], NULL, 0);

        /* Skip the entry we're filling in now */
        if(ship_id == menu_code)
            continue;

        tmp2[0] = (char)(ship_id);
        tmp2[1] = (char)(ship_id >> 8);
        tmp2[2] = '\0';

        /* Make sure the values are in-bounds */
        if((tmp2[0] || tmp2[1]) && (!isalpha(tmp2[0]) || !isalpha(tmp2[1])))
            continue;

        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);

        /* Fill in what we have */
        pkt->entries[num_ships].menu_id = LE32((MENU_ID_SHIP | (ship_id << 8)));
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_INIT_SHIP);
        pkt->entries[num_ships].flags = LE16(0x0F04);

        /* Create the name string (UTF-8) */
        if(tmp2[0] && tmp2[1])
            sprintf(tmp, "舰船/%s", tmp2);
        else
            strcpy(tmp, "舰船/Main");

        /* And convert to UTF-16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, tmp, 0x22);

        /* We're done with this ship, increment the counter */
        ++num_ships;
        len += 0x2C;
    }

    psocn_db_result_free(result);

    /* Make sure we have at least one ship... */
    if(num_ships == 1) {
        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_INIT_SHIP);
        pkt->entries[num_ships].flags = LE16(0x0000);

        /* And convert to UTF16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, no_ship_msg, 0x22);

        ++num_ships;
        len += 0x2C;
    }
    else {
        /* Clear out the ship information */
        memset(&pkt->entries[num_ships], 0, 0x2C);
        pkt->entries[num_ships].menu_id = LE32(MENU_ID_LAST_SHIP);
        pkt->entries[num_ships].item_id = LE32(ITEM_ID_DISCONNECT);
        pkt->entries[num_ships].flags = LE16(0x0000);

        /* And convert to UTF16 */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[num_ships].name, "断开连接", 0x22);

        ++num_ships;
        len += 0x2C;
    }


    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = LE32((num_ships - 1));

    /* 将数据包发送出去 */
    return crypt_send(c, len);

out:
    return i;
}

int send_ship_list(login_client_t *c, uint16_t menu_code) {
    /* Call the appropriate function */

    switch(c->type) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            return send_ship_list_dc(c, menu_code);

        case CLIENT_AUTH_PC:
            return send_ship_list_pc(c, menu_code);

        case CLIENT_AUTH_BB_CHARACTER:
            return send_ship_list_bb(c, menu_code);
    }

    return -1;
}

static int send_info_reply_dc(login_client_t *c, uint16_t type, const char* fmt,
    va_list args) {
    dc_info_reply_pkt *pkt = (dc_info_reply_pkt *)sendbuf;
    int len;
    iconv_t ic;
    char tm[4096];
    size_t in, out;
    char *inptr;
    char *outptr;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, sizeof(dc_chat_pkt));

    /* Do the formatting */
    vsnprintf(tm, 4096, fmt, args);
    tm[4095] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if (tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        if(tm[1] == 'J') {
            ic = ic_gbk_to_sjis;
            //ic = iconv_open(SJIS, GBK);
        }
        else {
            ic = ic_gbk_to_8859;
            //ic = iconv_open(ISO8859, GBK);
        }
    }
    else {
        ic = ic_gbk_to_utf16;
        //ic = iconv_open(UTF16LE, GBK);
    }

    in = strlen(tm) + 1;

    /* Convert the message to the appropriate encoding. */
    out = 65520;
    inptr = tm;
    outptr = pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65520 - out;

    /* Fill in the oddities of the packet. */
    pkt->odd[0] = LE32(0x00200000);
    pkt->odd[1] = LE32(0x00200020);

    /* Pad to a length that's at divisible by 4. */
    while(len & 0x03) {
        sendbuf[len++] = 0;
    }

    /* Fill in the length */
    len += 0x0C;

    /* 填充数据头 */
    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        pkt->hdr.dc.pkt_type = (uint8_t)type;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(len);
    }
    else {
        pkt->hdr.pc.pkt_type = (uint8_t)type;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(len);
    }

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

static int send_info_reply_bb(login_client_t *c, uint16_t type, const char* fmt,
                              va_list args) {
    bb_info_reply_pkt* pkt = (bb_info_reply_pkt*)sendbuf;
    int len;
    char tm[4096];
    size_t in, out;
    char* inptr;
    char* outptr;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, sizeof(bb_info_reply_pkt));

    /* Do the formatting */
    vsnprintf(tm, 4096, fmt, args);
    tm[4095] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if (tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    /* Convert the message to the appropriate encoding. */
    in = strlen(tm) + 1;
    out = 65520;
    inptr = tm;
    outptr = (char*)pkt->msg;
    iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65520 - out + 0x10;

    /* Fill in the unused fields of the packet. */
    pkt->unused[0] = pkt->unused[1] = 0;

    /* 结尾添加截断字符 0x00*/
    sendbuf[len++] = 0x00;
    sendbuf[len++] = 0x00;

    /* Pad to a length that is divisible by 8. */
    while (len & 0x07) {
        sendbuf[len++] = 0;
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(type);
    pkt->hdr.flags = 0;
    pkt->hdr.pkt_len = LE16(len);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

int send_info_reply(login_client_t *c, const char* fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch(c->type) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_PC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            rv = send_info_reply_dc(c, INFO_REPLY_TYPE, fmt, args);
            break;

        case CLIENT_AUTH_BB_CHARACTER:
            rv = send_info_reply_bb(c, INFO_REPLY_TYPE, fmt, args);
            break;
    }

    va_end(args);

    return rv;
}

/* Send a Blue Burst style scrolling message to the client */
int send_scroll_msg(login_client_t *c, const char* fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch(c->type) {
        case CLIENT_AUTH_BB_LOGIN:
        case CLIENT_AUTH_BB_CHARACTER:
            rv = send_info_reply_bb(c, BB_SCROLL_MSG_TYPE, fmt, args);
            break;
    }

    va_end(args);

    return rv;
}

/* Send a simple (header-only) packet to the client */
static int send_simple_dc(login_client_t *c, int type, int flags) {
    dc_pkt_hdr_t *pkt = (dc_pkt_hdr_t *)sendbuf;

    /* 填充数据头 */
    pkt->pkt_type = (uint8_t)type;
    pkt->flags = (uint8_t)flags;
    pkt->pkt_len = LE16(4);

    /* 将数据包发送出去 */
    return crypt_send(c, 4);
}

static int send_simple_pc(login_client_t *c, int type, int flags) {
    pc_pkt_hdr_t *pkt = (pc_pkt_hdr_t *)sendbuf;

    /* 填充数据头 */
    pkt->pkt_type = (uint8_t)type;
    pkt->flags = (uint8_t)flags;
    pkt->pkt_len = LE16(4);

    /* 将数据包发送出去 */
    return crypt_send(c, 4);
}

int send_simple(login_client_t *c, int type, int flags) {
    /* Call the appropriate function. */
    switch(c->type) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            return send_simple_dc(c, type, flags);

        case CLIENT_AUTH_PC:
            return send_simple_pc(c, type, flags);
    }

    return -1;
}

/* Send the list of quests in a category to the client. */
static int send_dc_quest_list(login_client_t *c,
                              psocn_quest_category_t *l, uint32_t ver) {
    dc_quest_list_pkt *pkt = (dc_quest_list_pkt *)sendbuf;
    int i, len = 0x04, entries = 0;
    iconv_t ic;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* Quest names are stored internally as UTF-8, convert to the appropriate
       encoding. */
    if(c->language_code == CLIENT_LANG_JAPANESE) {
        ic = iconv_open(SJIS, GBK);
    }
    else {
        ic = iconv_open(ISO8859, GBK);
    }

    if(ic == (iconv_t)-1) {
        perror("iconv_open");
        return -1;
    }

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* 填充数据头 */
    pkt->hdr.pkt_type = DL_QUEST_LIST_TYPE;

    for(i = 0; i < l->quest_count; ++i) {
        if(!(l->quests[i]->versions & ver)) {
            continue;
        }

        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x98);

        /* Copy the category's information over to the packet */
        pkt->entries[entries].menu_id = LE32(0x00000004);
        pkt->entries[entries].item_id = LE32(i);

        /* Convert the name and the description to the right encoding. */
        in = 32;
        out = 32;
        inptr = l->quests[i]->name;
        outptr = (char *)pkt->entries[entries].name;
        iconv(ic, &inptr, &in, &outptr, &out);

        in = 112;
        out = 112;
        inptr = l->quests[i]->desc;
        outptr = (char *)pkt->entries[entries].desc;
        iconv(ic, &inptr, &in, &outptr, &out);

        ++entries;
        len += 0x98;
    }

    iconv_close(ic);

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len);
}

static int send_pc_quest_list(login_client_t *c,
                              psocn_quest_category_t *l) {
    pc_quest_list_pkt *pkt = (pc_quest_list_pkt *)sendbuf;
    int i, len = 0x04, entries = 0;
    iconv_t ic;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* Quest names are stored internally as UTF-8, convert to UTF-16. */
    ic = iconv_open(UTF16LE, GBK);

    if(ic == (iconv_t)-1) {
        perror("iconv_open");
        return -1;
    }

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* 填充数据头 */
    pkt->hdr.pkt_type = DL_QUEST_LIST_TYPE;

    for(i = 0; i < l->quest_count; ++i) {
        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x0128);

        /* Copy the category's information over to the packet */
        pkt->entries[entries].menu_id = LE32(0x00000004);
        pkt->entries[entries].item_id = LE32(i);

        /* Convert the name and the description to UTF-16. */
        in = 32;
        out = 64;
        inptr = l->quests[i]->name;
        outptr = (char *)pkt->entries[entries].name;
        iconv(ic, &inptr, &in, &outptr, &out);

        in = 112;
        out = 224;
        inptr = l->quests[i]->desc;
        outptr = (char *)pkt->entries[entries].desc;
        iconv(ic, &inptr, &in, &outptr, &out);

        ++entries;
        len += 0x128;
    }

    iconv_close(ic);

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len);
}

static int send_gc_quest_list(login_client_t *c,
                              psocn_quest_category_t *l) {
    dc_quest_list_pkt *pkt = (dc_quest_list_pkt *)sendbuf;
    int i, len = 0x04, entries = 0;
    iconv_t ic;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* Quest names are stored internally as UTF-8, convert to the appropriate
       encoding. */
    if(c->language_code == CLIENT_LANG_JAPANESE) {
        ic = iconv_open(SJIS, GBK);
    }
    else {
        ic = iconv_open(ISO8859, GBK);
    }

    if(ic == (iconv_t)-1) {
        perror("iconv_open");
        return -1;
    }

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* 填充数据头 */
    pkt->hdr.pkt_type = DL_QUEST_LIST_TYPE;

    for(i = 0; i < l->quest_count; ++i) {
        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0x98);

        /* Copy the category's information over to the packet */
        pkt->entries[entries].menu_id = LE32(0x00000004);
        pkt->entries[entries].item_id = LE32(i);

        /* Convert the name and the description to the right encoding. */
        in = 32;
        out = 32;
        inptr = l->quests[i]->name;
        outptr = (char *)pkt->entries[entries].name;
        iconv(ic, &inptr, &in, &outptr, &out);

        in = 112;
        out = 112;
        inptr = l->quests[i]->desc;
        outptr = (char *)pkt->entries[entries].desc;
        iconv(ic, &inptr, &in, &outptr, &out);

        ++entries;
        len += 0x98;
    }

    iconv_close(ic);

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len);
}

static int send_xbox_quest_list(login_client_t *c,
                                psocn_quest_category_t *l) {
    xb_quest_list_pkt *pkt = (xb_quest_list_pkt *)sendbuf;
    int i, len = 0x04, entries = 0;
    iconv_t ic;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* Quest names are stored internally as UTF-8, convert to the appropriate
       encoding. */
    if(c->language_code == CLIENT_LANG_JAPANESE) {
        ic = iconv_open(SJIS, GBK);
    }
    else {
        ic = iconv_open(ISO8859, GBK);
    }

    if(ic == (iconv_t)-1) {
        perror("iconv_open");
        return -1;
    }

    /* Clear out the header */
    memset(pkt, 0, 0x04);

    /* 填充数据头 */
    pkt->hdr.pkt_type = DL_QUEST_LIST_TYPE;

    for(i = 0; i < l->quest_count; ++i) {
        /* Clear the entry */
        memset(pkt->entries + entries, 0, 0xA8);

        /* Copy the category's information over to the packet */
        pkt->entries[entries].menu_id = LE32(0x00000004);
        pkt->entries[entries].item_id = LE32(i);

        /* Convert the name and the description to the right encoding. */
        in = 32;
        out = 32;
        inptr = l->quests[i]->name;
        outptr = (char *)pkt->entries[entries].name;
        iconv(ic, &inptr, &in, &outptr, &out);

        in = 112;
        out = 128;
        inptr = l->quests[i]->desc;
        outptr = (char *)pkt->entries[entries].desc;
        iconv(ic, &inptr, &in, &outptr, &out);

        ++entries;
        len += 0xA8;
    }

    iconv_close(ic);

    /* Fill in the rest of the header */
    pkt->hdr.flags = entries;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len);
}

int send_quest_list(login_client_t *c, psocn_quest_category_t *l) {
    /* Call the appropriate function. */
    switch(c->type) {
        case CLIENT_AUTH_DC:
            return send_dc_quest_list(c, l, c->version);

        case CLIENT_AUTH_PC:
            return send_pc_quest_list(c, l);

        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
            return send_gc_quest_list(c, l);

        case CLIENT_AUTH_XBOX:
            return send_xbox_quest_list(c, l);
    }

    return -1;
}

/* Send a quest to a client. This only supports the .qst format that Qedit spits
   out by default (Download quest format). */
int send_quest(login_client_t *c, psocn_quest_t *q) {
    char filename[256];
    FILE *fp;
    long len;
    size_t read;
    int v = c->type;

    /* Figure out what file we're going to send. */
    sprintf(filename, "%s/%s/%s/%s.qst", cfg->quests_dir, client_type[v].ver_name,
            language_codes[c->language_code], q->prefix);
    fp = fopen(filename, "rb");

    if(!fp) {
        return -1;
    }

    /* Figure out how long the file is. */
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Copy the file (in chunks if necessary) to the sendbuf to actually send
       away. */
    while(len) {
        read = fread(sendbuf, 1, 65536, fp);

        /* If we can't read from the file, bail. */
        if(!read) {
            fclose(fp);
            return -2;
        }

        /* Make sure we read up to a header-size boundary. */
        if((read & (client_type[c->type].hdr_size - 1)) && !feof(fp)) {
            long amt = (read & (client_type[c->type].hdr_size - 1));

            fseek(fp, -amt, SEEK_CUR);
            read -= amt;
        }

        /* Send this chunk away. */
        if(crypt_send(c, read)) {
            fclose(fp);
            return -3;
        }

        len -= read;
    }

    /* We're finished. */
    fclose(fp);
    return 0;
}

/* Send an Episode 3 rank update to a client. */
int send_ep3_rank_update(login_client_t *c) {
    ep3_rank_update_pkt *pkt = (ep3_rank_update_pkt *)sendbuf;

    /* XXXX: Need to actually do something with this in the future */
    memset(pkt, 0, sizeof(ep3_rank_update_pkt));
    pkt->hdr.pkt_type = EP3_RANK_UPDATE_TYPE;
    pkt->hdr.pkt_len = LE16(0x0020);
    pkt->meseta = LE32(0x00FFFFFF);
    pkt->max_meseta = LE32(0x00FFFFFF);
    pkt->jukebox = LE32(0xFFFFFFFF);

    return crypt_send(c, 0x0020);
}

/* Send the Episode 3 card list to a client. */
int send_ep3_card_update(login_client_t *c) {
    ep3_card_update_pkt *pkt = (ep3_card_update_pkt *)sendbuf;
    FILE *fp;
    long size;
    uint16_t pkt_len;

    /* Make sure we're actually dealing with Episode 3 */
    if(c->type != CLIENT_AUTH_EP3) {
        return -1;
    }

    /* Grab the card list */
    fp = fopen("System/Ep3/cardupdate.mnr", "rb");
    if(!fp) {
        return -1;
    }

    /* Figure out how big the file is */
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Make sure the size is sane... */
    if(size > 0x8000) {
        return -1;
    }

    /* Not sure why this size is used, but for now we'll go with it (borrowed
       from Fuzziqer's newserv) */
    pkt_len = (13 + size) & 0xFFFC;

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(pkt_len);
    pkt->hdr.pkt_type = EP3_CARD_UPDATE_TYPE;
    pkt->hdr.flags = 0;
    pkt->size = LE32(size);
    fread(pkt->data, 1, size, fp);

    /* 加密并发送 */
    return crypt_send(c, pkt_len);
}

/* Send a Blue Burst option reply to the client. */
int send_bb_option_reply(login_client_t *c, bb_key_config_t keys, bb_guild_t guild_data) {
    bb_opt_config_pkt *pkt = (bb_opt_config_pkt *)sendbuf;

    /* Clear it out first */
    memset(pkt, 0, sizeof(bb_opt_config_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(sizeof(bb_opt_config_pkt));
    pkt->hdr.pkt_type = LE16(BB_OPTION_CONFIG_TYPE);

    /* 复制角色键位数据 */
    memcpy(&pkt->key_cfg, &keys, sizeof(bb_key_config_t));

    /* 复制角色公会数据 */
    memcpy(&pkt->guild_data, &guild_data, sizeof(bb_guild_t));

    /* 将数据包发送出去 */
    return crypt_send(c, sizeof(bb_opt_config_pkt));
}

/* Send a Blue Burst character acknowledgement to the client. */
int send_bb_char_ack(login_client_t *c, uint8_t slot, uint8_t code) {
    bb_char_ack_pkt *pkt = (bb_char_ack_pkt *)sendbuf;

    /* Clear it out first */
    memset(pkt, 0, sizeof(bb_char_ack_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(sizeof(bb_char_ack_pkt));
    pkt->hdr.pkt_type = LE16(BB_CHARACTER_ACK_TYPE);
    pkt->slot = slot;
    pkt->code = code;

    /* 加密并发送 */
    return crypt_send(c, sizeof(bb_char_ack_pkt));
}

/* Send a Blue Burst checksum acknowledgement to the client. */
int send_bb_checksum_ack(login_client_t *c, uint32_t ack) {
    bb_checksum_ack_pkt *pkt = (bb_checksum_ack_pkt *)sendbuf;

    /* Clear it out first */
    memset(pkt, 0, sizeof(bb_checksum_ack_pkt));

    /* Fill it in */
    pkt->hdr.pkt_len = LE16(sizeof(bb_checksum_ack_pkt));
    pkt->hdr.pkt_type = LE16(BB_CHECKSUM_ACK_TYPE);
    pkt->ack = LE32(ack);

    /* 加密并发送 */
    return crypt_send(c, sizeof(bb_checksum_ack_pkt));
}

/* Send a Blue Burst guildcard header packet. */
int send_bb_guild_header(login_client_t *c, uint32_t checksum) {
    bb_guildcard_hdr_pkt *pkt = (bb_guildcard_hdr_pkt *)sendbuf;

    /* Clear it out first */
    memset(pkt, 0, sizeof(bb_guildcard_hdr_pkt));

    /* Fill it in */
    pkt->hdr.pkt_len = LE16(sizeof(bb_guildcard_hdr_pkt));
    pkt->hdr.pkt_type = LE16(BB_GUILDCARD_HEADER_TYPE);
    pkt->one = 1;
    pkt->len = LE16(sizeof(bb_guildcard_data_t));
    pkt->checksum = LE32(checksum);

    /* 加密并发送 */
    return crypt_send(c, sizeof(bb_guildcard_hdr_pkt));
}

/* Send a Blue Burst guildcard chunk packet. */
int send_bb_guild_chunk(login_client_t *c, uint32_t chunk_index) {
    bb_guildcard_chunk_pkt *pkt = (bb_guildcard_chunk_pkt *)sendbuf;
    uint32_t offset = (chunk_index * 0x6800);
    uint16_t size = sizeof(bb_guildcard_data_t) - offset;
    uint8_t *ptr = ((uint8_t *)c->gc_data) + offset;

    /* 合理性检查... */
    if(offset > sizeof(bb_guildcard_data_t)) {
        return -1;
    }

    /* Don't send a chunk bigger than PSO wants */
    if(size > 0x6800) {
        size = 0x6800;
    }

    /* Clear it out first */
    memset(pkt, 0, size + 0x10);

    /* Fill it in */
    pkt->hdr.pkt_len = LE16((size + 0x10));
    pkt->hdr.pkt_type = LE16(BB_GUILDCARD_CHUNK_TYPE);
    pkt->chunk_index = LE32(chunk_index);
    memcpy(pkt->data, ptr, size);

    /* 加密并发送 */
    return crypt_send(c, size + 0x10);
}

/* Send a prepared Blue Burst packet. */
int send_bb_pkt(login_client_t *c, bb_pkt_hdr_t *hdr) {
    uint16_t len = LE16(hdr->pkt_len);

    /* Copy it into our buffer */
    memcpy(sendbuf, hdr, len);

    /* 加密并发送 */
    return crypt_send(c, len);
}

int send_bb_char_dressing_room(psocn_bb_char_t *c, psocn_bb_mini_char_t *mc) {
    
    //DBG_LOG("更新角色更衣室");

    memcpy(&c->dress_data.guildcard_string, &mc->dress_data.guildcard_string, sizeof(mc->dress_data.guildcard_string));

    c->dress_data.dress_unk1 = mc->dress_data.dress_unk1;
    c->dress_data.dress_unk2 = mc->dress_data.dress_unk2;

    c->dress_data.name_color_b = mc->dress_data.name_color_b;
    c->dress_data.name_color_g = mc->dress_data.name_color_g;
    c->dress_data.name_color_r = mc->dress_data.name_color_r;
    c->dress_data.name_color_transparency = mc->dress_data.name_color_transparency;
    c->dress_data.model = mc->dress_data.model;

    for (int i = 0; i < 10; ++i) {
        c->dress_data.dress_unk3[i] = mc->dress_data.dress_unk3[i];
    }

    c->dress_data.create_code = mc->dress_data.create_code;
    c->dress_data.name_color_checksum = mc->dress_data.name_color_checksum;
    c->dress_data.section = mc->dress_data.section;
    c->dress_data.ch_class = mc->dress_data.ch_class;
    c->dress_data.v2flags = mc->dress_data.v2flags;
    c->dress_data.version = mc->dress_data.version;
    c->dress_data.v1flags = mc->dress_data.v1flags;
    c->dress_data.costume = mc->dress_data.costume;
    c->dress_data.skin = mc->dress_data.skin;
    c->dress_data.face = mc->dress_data.face;
    c->dress_data.head = mc->dress_data.head;
    c->dress_data.hair = mc->dress_data.hair;
    c->dress_data.hair_r = mc->dress_data.hair_r;
    c->dress_data.hair_g = mc->dress_data.hair_g;
    c->dress_data.hair_b = mc->dress_data.hair_b;
    c->dress_data.prop_x = mc->dress_data.prop_x;
    c->dress_data.prop_y = mc->dress_data.prop_y;

    memcpy(&c->name, &mc->name, sizeof(mc->name));

    return 0;
}

/* Send a Blue Burst character preview packet. */
int send_bb_char_preview(login_client_t *c, const psocn_bb_mini_char_t *mc,
                         uint8_t slot) {
    bb_char_preview_pkt *pkt = (bb_char_preview_pkt *)sendbuf;

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(BB_CHARACTER_UPDATE_TYPE);
    pkt->hdr.pkt_len = LE16(sizeof(bb_char_preview_pkt));
    pkt->hdr.flags = 0;

    pkt->slot = slot;
    pkt->sel_char = 0;
    pkt->flags = 0;

    /* Copy in the character data */
    memcpy(&pkt->data, mc, sizeof(psocn_bb_mini_char_t));

    return crypt_send(c, sizeof(bb_char_preview_pkt));
}

/* Send the content of the "Information" menu. */
static int send_gc_info_list(login_client_t *c, uint32_t ver) {
    dc_block_list_pkt *pkt = (dc_block_list_pkt *)sendbuf;
    int i, len = 0x20, entries = 1;
    uint32_t lang = (1 << c->language_code);

    /* Clear the base packet */
    memset(pkt, 0, sizeof(dc_block_list_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;

    /* Fill in the DATABASE entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = LE16(0x0004);
    strcpy(pkt->entries[0].name, "DATABASE/US");
    pkt->entries[0].name[0x11] = 0x08;

    /* Add each info item to the list. */
    for(i = 0; i < cfg->info_file_count; ++i) {
        /* See if we should look at this entry. */
        if(!(cfg->info_files[i].versions & ver)) {
            continue;
        }

        if(!(cfg->info_files[i].languages & lang)) {
            continue;
        }

        /* Skip MOTD entries. */
        if(!(cfg->info_files[i].desc)) {
            continue;
        }

        /* Clear out the info file information */
        memset(&pkt->entries[entries], 0, 0x1C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(MENU_ID_INFODESK);
        pkt->entries[entries].item_id = LE32(i);
        pkt->entries[entries].flags = LE16(0x0000);

        /* These are always ASCII, so this is fine */
        strncpy(pkt->entries[entries].name, cfg->info_files[i].desc, 0x11);
        pkt->entries[entries].name[0x11] = 0;

        len += 0x1C;
        ++entries;
    }

    /* Add the entry to return to the initial menu. */
    memset(&pkt->entries[entries], 0, 0x1C);

    pkt->entries[entries].menu_id = LE32(MENU_ID_INFODESK);
    pkt->entries[entries].item_id = LE32(0xFFFFFFFF);
    pkt->entries[entries].flags = LE16(0x0000);

    strcpy(pkt->entries[entries].name, "Main Menu");
    len += 0x1C;

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

/* Send the content of the "Information" menu. */
static int send_bb_info_list(login_client_t* c, uint32_t ver) {
    bb_block_list_pkt* pkt = (bb_block_list_pkt*)sendbuf;
    int i, len = 0x100, entries = 0;
    uint32_t lang = (1 << c->language_code);

    /* Clear the base packet */
    memset(pkt, 0, len);

    /* 填充数据头 */
    pkt->hdr.pkt_type = LE16(BLOCK_LIST_TYPE);

    /* Fill in the DATABASE entry */
    memset(&pkt->entries[0], 0, 0x1C);
    pkt->entries[0].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[0].item_id = 0;
    pkt->entries[0].flags = LE16(0x0004);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, "DATABASE/US", 0x12);

    /* Add each info item to the list. */
    for (i = 0; i < cfg->info_file_count; ++i) {
        /* See if we should look at this entry. */
        if (!(cfg->info_files[i].versions & ver)) {
            continue;
        }

        if (!(cfg->info_files[i].languages & lang)) {
            continue;
        }

        /* Skip MOTD entries. */
        if (!(cfg->info_files[i].desc)) {
            continue;
        }

        ++entries;
        /* Clear out the info file information */
        memset(&pkt->entries[entries], 0, 0x1C);

        /* Fill in what we have */
        pkt->entries[entries].menu_id = LE32(MENU_ID_INFODESK);
        pkt->entries[entries].item_id = LE32(i);
        pkt->entries[entries].flags = LE16(0x0000);

        /* These are always ASCII, so this is fine */
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, cfg->info_files[i].desc, 0x20);

        len += 0x1C;
    }

    ++entries;
    /* Add the entry to return to the initial menu. */
    memset(&pkt->entries[entries], 0, 0x1C);

    pkt->entries[entries].menu_id = LE32(MENU_ID_INFODESK);
    pkt->entries[entries].item_id = LE32(ITEM_ID_LAST);
    pkt->entries[entries].flags = LE16(0x0000);

    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[entries].name, "主菜单", 0x20);
    len += 0x1C;

    /* Fill in the rest of the header */
    pkt->hdr.pkt_len = LE16(len);
    pkt->hdr.flags = (uint8_t)(entries);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

int send_info_list(login_client_t *c) {
    /* Call the appropriate function */
    switch(c->type) {
        case CLIENT_AUTH_GC:
            return send_gc_info_list(c, PSOCN_INFO_GC);

        case CLIENT_AUTH_EP3:
            return send_gc_info_list(c, PSOCN_INFO_EP3);

        case CLIENT_AUTH_XBOX:
            return send_gc_info_list(c, PSOCN_INFO_XBOX);

        case CLIENT_AUTH_BB_LOGIN:
        case CLIENT_AUTH_BB_CHARACTER:
            return send_bb_info_list(c, PSOCN_INFO_BB);
    }

    return -1;
}

/* Send a message to the client. */
static int send_gc_message_box(login_client_t *c, const char *fmt,
                               va_list args) {
    dc_msg_box_pkt *pkt = (dc_msg_box_pkt *)sendbuf;
    int len;

    /* Do the formatting */
    vsnprintf(pkt->msg, 1024, fmt, args);
    pkt->msg[1024] = '\0';
    len = strlen(pkt->msg) + 1;

    /* Make sure we have a language code tag */
    if(pkt->msg[0] != '\t' || (pkt->msg[1] != 'E' && pkt->msg[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&pkt->msg[2], &pkt->msg[0], len);
        pkt->msg[0] = '\t';
        pkt->msg[1] = 'E';
        len += 2;
    }

    /* Add any padding needed */
    while(len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* 填充数据头 */
    len += 0x04;

    pkt->hdr.dc.pkt_type = GC_MSG_BOX_TYPE;
    pkt->hdr.dc.flags = 0;
    pkt->hdr.dc.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len);
}

/* Send a message to the client. */
static int send_bb_message_box(login_client_t* c, const char* fmt,
    va_list args) {
    bb_msg_box_pkt* pkt = (bb_msg_box_pkt*)sendbuf;
    int len;

    /* Do the formatting */
    vsnprintf(pkt->msg, 1024, fmt, args);
    pkt->msg[1024] = '\0';
    len = strlen(pkt->msg) + 1;

    /* Make sure we have a language code tag */
    if (pkt->msg[0] != '\t' || (pkt->msg[1] != 'E' && pkt->msg[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&pkt->msg[2], &pkt->msg[0], len);
        pkt->msg[0] = '\t';
        pkt->msg[1] = 'E';
        len += 2;
    }

    /* Add any padding needed */
    while (len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* 填充数据头 */
    len += 0x04;

    pkt->hdr.pkt_type = MSG_BOX_TYPE;
    pkt->hdr.flags = 0;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len);
}

int send_message_box(login_client_t *c, const char *fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch(c->type) {
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            rv = send_gc_message_box(c, fmt, args);

        case CLIENT_AUTH_BB_LOGIN:
        case CLIENT_AUTH_BB_CHARACTER:
            rv = send_bb_message_box(c, fmt, args);
    }

    va_end(args);

    return rv;
}

/* Send a message box containing an information file entry. */
int send_dc_info_file(login_client_t* c, uint32_t entry) {
    FILE* fp;
    char buf[1024];
    long len;

    /* The item_id should be the information the client wants. */
    if ((int)entry >= cfg->info_file_count) {
        send_message_box(c, "%s\n%s",
            __(c, "\tE\tC4Something went wrong!"),
            __(c, "\tC7The information requested is missing."));
        return 0;
    }

    /* Attempt to open the file */
    fp = fopen(cfg->info_files[entry].filename, "r");

    if (!fp) {
        send_message_box(c, "%s\n%s",
            __(c, "\tE\tC4Something went wrong!"),
            __(c, "\tC7The information requested is missing."));
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

    /* Send the message to the client. */
    return send_message_box(c, "%s", buf);
}

/* Send a message box containing an information file entry. */
int send_bb_info_file(login_client_t *c, uint32_t entry) {
    FILE *fp;
    char buf[4096];
    long len;

    /* The item_id should be the information the client wants. */
    if((int)entry >= cfg->info_file_count) {
        send_message_box(c, "%s\n%s",
                         __(c, "\tE\tC4Something went wrong!"),
                         __(c, "\tC7The information requested is missing."));
        return 0;
    }

    /* Attempt to open the file */
    fp = fopen(cfg->info_files[entry].filename, "r");

    if(!fp) {
        send_message_box(c, "%s\n%s",
                         __(c, "\tE\tC4Something went wrong!"),
                         __(c, "\tC7The information requested is missing."));
        return 0;
    }

    /* Figure out the length of the file. */
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Truncate to about 4KB */
    if(len > 4096) {
        len = 4096;
    }

    /* Read the file in. */
    fread(buf, 1, len, fp);
    fclose(fp);
    buf[len] = 0;

    /* Send the message to the client. */
    return send_message_box(c, "%s", buf);
}

int send_info_file(login_client_t* c, uint32_t entry) {
    int rv = -1;

    /* Call the appropriate function. */
    switch (c->type) {

    case CLIENT_AUTH_DC:
    case CLIENT_AUTH_PC:
    case CLIENT_AUTH_GC:
    case CLIENT_AUTH_EP3:
    case CLIENT_AUTH_DCNTE:
    case CLIENT_AUTH_XBOX:
        rv = send_dc_info_file(c, entry);

    case CLIENT_AUTH_BB_LOGIN:
    case CLIENT_AUTH_BB_CHARACTER:
        rv = send_bb_info_file(c, entry);
    }

    return rv;
}

static int send_dc_message(login_client_t* c, uint16_t type, const char* fmt,
    va_list args) {
    //uint8_t* sendbuf = get_sendbuf();
    dc_chat_pkt* pkt = (dc_chat_pkt*)sendbuf;
    int len;
    iconv_t ic;
    char tm[4096];
    size_t in, out;
    char* inptr;
    char* outptr;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, sizeof(dc_chat_pkt));

    /* Do the formatting */
    vsnprintf(tm, 4096, fmt, args);
    tm[4095] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if (tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    /* Set up to convert between encodings */
    if (c->version == CLIENT_AUTH_DC || c->version == CLIENT_AUTH_PC ||
        c->version == CLIENT_AUTH_GC || c->version == CLIENT_AUTH_EP3 ||
        c->version == CLIENT_AUTH_XBOX) {
        if (tm[1] != 'J') {
            ic = ic_gbk_to_8859;
            //ic = ic_utf8_to_8859;
        }
        else {
            ic = ic_gbk_to_sjis;
            //ic = ic_utf8_to_sjis;
        }
    }
    else {
        //ic = ic_utf8_to_utf16;
        ic = ic_gbk_to_utf16;
    }

    in = strlen(tm) + 1;

    /* Convert the message to the appropriate encoding. */
    out = 65520;
    inptr = tm;
    outptr = pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65520 - out;

    /* 结尾添加截断字符 0x00*/
    pkt->msg[len++] = 0x00;

    /* Add any padding needed */
    while (len & 0x03) {
        pkt->msg[len++] = 0;
    }

    /* Fill in the length */
    len += 0x0C;

    if (c->version == CLIENT_AUTH_DC || c->version == CLIENT_AUTH_PC ||
        c->version == CLIENT_AUTH_GC || c->version == CLIENT_AUTH_EP3 ||
        c->version == CLIENT_AUTH_XBOX) {
        pkt->hdr.dc.pkt_type = (uint8_t)type;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(len);
    }
    else {
        pkt->hdr.pc.pkt_type = (uint8_t)type;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(len);
    }

    /* 加密并发送 */
    return crypt_send(c, len);
}

static int send_bb_message(login_client_t* c, uint16_t type, const char* fmt,
    va_list args) {
    //uint8_t* sendbuf = get_sendbuf();
    bb_chat_pkt* pkt = (bb_chat_pkt*)sendbuf;
    int len;
    char tm[4096];
    size_t in, out;
    char* inptr;
    char* outptr;

    /* 确认已获得数据发送缓冲 */
    if (!sendbuf) {
        return -1;
    }

    /* Clear the packet header */
    memset(pkt, 0, sizeof(bb_chat_pkt));

    /* Do the formatting */
    vsnprintf(tm, 4096, fmt, args);
    tm[4095] = '\0';
    in = strlen(tm) + 1;

    /* Make sure we have a language code tag */
    if (tm[0] != '\t' || (tm[1] != 'E' && tm[1] != 'J')) {
        /* Assume Non-Japanese if we don't have a marker. */
        memmove(&tm[2], &tm[0], in);
        tm[0] = '\t';
        tm[1] = 'E';
        in += 2;
    }

    /* Convert the message to the appropriate encoding. */
    out = 65520;
    inptr = tm;
    outptr = (char*)pkt->msg;
    iconv(ic_gbk_to_utf16, &inptr, &in, &outptr, &out);

    /* Figure out how long the new string is. */
    len = 65520 - out + 0x10;

    /* 结尾添加截断字符 0x00*/
    sendbuf[len++] = 0x00;
    sendbuf[len++] = 0x00;

    /* Add any padding needed */
    while (len & 0x07) {
        sendbuf[len++] = 0;
    }

    pkt->hdr.pkt_type = LE16(type);
    pkt->hdr.flags = 0;
    pkt->hdr.pkt_len = LE16(len);

    /* 加密并发送 */
    return crypt_send(c, len);
}

/* Send a text message to the client (i.e, for stuff related to commands). */
int send_msg(login_client_t* c, uint16_t type, const char* fmt, ...) {
    va_list args;
    int rv = -1;

    va_start(args, fmt);

    /* Call the appropriate function. */
    switch (c->version) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_PC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            rv = send_dc_message(c, type, fmt, args);
            break;

        case CLIENT_AUTH_BB_LOGIN:
        case CLIENT_AUTH_BB_CHARACTER:
            rv = send_bb_message(c, type, fmt, args);
            break;
    }

    va_end(args);

    return rv;
}

/* Send the GM operations menu to the user. */
static int send_gm_menu_dc(login_client_t *c) {
    dc_ship_list_pkt *pkt = (dc_ship_list_pkt *)sendbuf;
    int len = 0x04, count = 0;

    /* Fill in the "DATABASE/US" entry */
    pkt->entries[count].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[count].item_id = 0;
    pkt->entries[count].flags = LE16(0x0004);
    strncpy(pkt->entries[count].name, "DATABASE/US", 0x12);
    pkt->entries[count].name[0x11] = 0x08;
    ++count;
    len += 0x1C;

    /* Add our entries... */
    pkt->entries[count].menu_id = LE32(MENU_ID_GM);
    pkt->entries[count].item_id = LE32(ITEM_ID_GM_REF_QUESTS);
    pkt->entries[count].flags = LE16(0x0004);
    istrncpy(ic_gbk_to_utf8, pkt->entries[count].name, "刷新任务", 0x12);
    //strncpy(pkt->entries[count].name, "Refresh Quests", 0x12);
    ++count;
    len += 0x1C;

    if(IS_GLOBAL_ROOT(c)) {
        pkt->entries[count].menu_id = LE32(MENU_ID_GM);
        pkt->entries[count].item_id = LE32(ITEM_ID_GM_RESTART);
        pkt->entries[count].flags = LE16(0x0004);
        istrncpy(ic_gbk_to_utf8, pkt->entries[count].name, "重启服务器", 0x12);
        //strncpy(pkt->entries[count].name, "Restart", 0x12);
        ++count;
        len += 0x1C;

        pkt->entries[count].menu_id = LE32(MENU_ID_GM);
        pkt->entries[count].item_id = LE32(ITEM_ID_GM_SHUTDOWN);
        pkt->entries[count].flags = LE16(0x0004);
        istrncpy(ic_gbk_to_utf8, pkt->entries[count].name, "关闭服务器", 0x12);
        //strncpy(pkt->entries[count].name, "Shutdown", 0x12);
        ++count;
        len += 0x1C;
    }

    pkt->entries[count].menu_id = LE32(MENU_ID_GM);
    pkt->entries[count].item_id = LE32(0xFFFFFFFF);
    pkt->entries[count].flags = LE16(0x0004);
    istrncpy(ic_gbk_to_utf8, pkt->entries[count].name, "主菜单", 0x12);
    //strncpy(pkt->entries[count].name, "Main Menu", 0x12);
    ++count;
    len += 0x1C;

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.flags = (uint8_t)(count - 1);
    pkt->hdr.pkt_len = LE16(len);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

static int send_gm_menu_pc(login_client_t *c) {
    pc_ship_list_pkt *pkt = (pc_ship_list_pkt *)sendbuf;
    int len = 0x04, count = 0;

    /* Fill in the "DATABASE/US" entry */
    pkt->entries[count].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[count].item_id = 0;
    pkt->entries[count].flags = LE16(0x0004);
    ascii_to_utf16("DATABASE/US", pkt->entries[count].name, 0x11);
    pkt->entries[count].name[0x11] = 0x08;
    ++count;
    len += 0x2C;

    /* Add our entries... */
    pkt->entries[count].menu_id = LE32(MENU_ID_GM);
    pkt->entries[count].item_id = LE32(ITEM_ID_GM_REF_QUESTS);
    pkt->entries[count].flags = LE16(0x0004);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, "刷新任务", 0x11);
    //ascii_to_utf16("Refresh Quests", pkt->entries[count].name, 0x11);
    ++count;
    len += 0x2C;

    if(IS_GLOBAL_ROOT(c)) {
        pkt->entries[count].menu_id = LE32(MENU_ID_GM);
        pkt->entries[count].item_id = LE32(ITEM_ID_GM_RESTART);
        pkt->entries[count].flags = LE16(0x0004);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, "重启服务器", 0x11);
        //ascii_to_utf16("Restart", pkt->entries[count].name, 0x11);
        ++count;
        len += 0x2C;

        pkt->entries[count].menu_id = LE32(MENU_ID_GM);
        pkt->entries[count].item_id = LE32(ITEM_ID_GM_SHUTDOWN);
        pkt->entries[count].flags = LE16(0x0004);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, "关闭服务器", 0x11);
        //ascii_to_utf16("Shutdown", pkt->entries[count].name, 0x11);
        ++count;
        len += 0x2C;
    }

    pkt->entries[count].menu_id = LE32(MENU_ID_GM);
    pkt->entries[count].item_id = LE32(0xFFFFFFFF);
    pkt->entries[count].flags = LE16(0x0F04);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, "主菜单", 0x11);
    //ascii_to_utf16("Main Menu", pkt->entries[count].name, 0x11);
    ++count;
    len += 0x2C;

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.flags = (uint8_t)(count  - 1);
    pkt->hdr.pkt_len = LE16(len);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

static int send_gm_menu_bb(login_client_t* c) {
    bb_ship_list_pkt* pkt = (bb_ship_list_pkt*)sendbuf;
    int len = 0x04, count = 0;

    /* Fill in the "DATABASE/US" entry */
    pkt->entries[count].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[count].item_id = 0;
    pkt->entries[count].flags = LE16(0x0004);
    ascii_to_utf16("DATABASE/US", pkt->entries[count].name, 0x11);
    //pkt->entries[count].name[0x11] = 0x08;
    ++count;
    len += 0x2C;

    /* Add our entries... */
    pkt->entries[count].menu_id = LE32(MENU_ID_GM);
    pkt->entries[count].item_id = LE32(ITEM_ID_GM_REF_QUESTS);
    pkt->entries[count].flags = LE16(0x0004);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, "刷新任务", 0x12);
    //ascii_to_utf16("Refresh Quests", pkt->entries[count].name, 0x11);
    ++count;
    len += 0x2C;

    if (IS_GLOBAL_ROOT(c)) {
        pkt->entries[count].menu_id = LE32(MENU_ID_GM);
        pkt->entries[count].item_id = LE32(ITEM_ID_GM_RESTART);
        pkt->entries[count].flags = LE16(0x0004);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, "重启服务器", 0x12);
        //ascii_to_utf16("Restart", pkt->entries[count].name, 0x11);
        ++count;
        len += 0x2C;

        pkt->entries[count].menu_id = LE32(MENU_ID_GM);
        pkt->entries[count].item_id = LE32(ITEM_ID_GM_SHUTDOWN);
        pkt->entries[count].flags = LE16(0x0004);
        istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, "关闭服务器", 0x12);
        //ascii_to_utf16("Shutdown", pkt->entries[count].name, 0x11);
        ++count;
        len += 0x2C;
    }

    pkt->entries[count].menu_id = LE32(MENU_ID_GM);
    pkt->entries[count].item_id = LE32(0xFFFFFFFF);
    pkt->entries[count].flags = LE16(0x0F04);
    istrncpy(ic_gbk_to_utf16, (char*)pkt->entries[count].name, "主菜单", 0x12);
    //ascii_to_utf16("Main Menu", pkt->entries[count].name, 0x11);
    ++count;
    len += 0x2C;

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.flags = (uint8_t)(count - 1);
    pkt->hdr.pkt_len = LE16(len);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

int send_gm_menu(login_client_t *c) {
    /* Make sure the user is actually a GM... */
    if (!IS_GLOBAL_GM(c)) {
        if (c->type == CLIENT_AUTH_BB_LOGIN || c->type == CLIENT_AUTH_BB_CHARACTER)
            send_msg(c, MSG1_TYPE, "%s", __(c, "\tE\tC4您没有权限这样做!"));
        return send_initial_menu(c);
    }

    /* Call the appropriate function */
    switch(c->type) {
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            return send_gm_menu_dc(c);

        case CLIENT_AUTH_PC:
            return send_gm_menu_pc(c);

        case CLIENT_AUTH_BB_LOGIN:
        case CLIENT_AUTH_BB_CHARACTER:
            return send_gm_menu_bb(c);
    }

    return -1;
}

/* Send the message of the day to the given client. */
int send_motd(login_client_t *c) {
    FILE *fp;
    char buf[1024];
    long len;
    uint32_t lang = (1 << c->language_code), ver;
    int i, found = 0;
    psocn_info_file_t* f = { 0 };

    switch(c->type) {
        case CLIENT_AUTH_GC:
            ver = PSOCN_INFO_GC;
            break;

        case CLIENT_AUTH_EP3:
            ver = PSOCN_INFO_EP3;
            break;

        case CLIENT_AUTH_XBOX:
            ver = PSOCN_INFO_XBOX;
            break;

        case CLIENT_AUTH_BB_LOGIN:
        case CLIENT_AUTH_BB_CHARACTER:
            ver = PSOCN_INFO_BB;
            break;

        default:
            return 1;
    }

    for(i = 0; i < cfg->info_file_count && !found; ++i) {
        f = &cfg->info_files[i];

        if(!f->desc && (f->versions & ver) && (f->languages & lang)) {
            found = 1;
        }
    }

    /* No MOTD found for the given version/language combination. */
    if(!found) {
        return 1;
    }

    /* Attempt to open the file */
    fp = fopen(f->filename, "r");

    /* Can't find the file? Punt. */
    if(!fp) {
        return 1;
    }

    /* Figure out the length of the file. */
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Truncate to about 1KB */
    if(len > 1023) {
        len = 1023;
    }

    /* Read the file in. */
    fread(buf, 1, len, fp);
    fclose(fp);
    buf[len] = 0;

    /* Send the message to the client. */
    return send_message_box(c, "%s", buf);
}

int send_quest_description(login_client_t *c, psocn_quest_t *q) {
    dc_msg_box_pkt *pkt = (dc_msg_box_pkt *)sendbuf;
    int size = 4;
    iconv_t ic;
    size_t in, out;
    char *inptr;
    char *outptr;

    /* If we don't have a description, don't even try. */
    if(!q->long_desc || q->long_desc[0] == '\0')
        return 0;

    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        if(c->language_code == CLIENT_LANG_JAPANESE)
            ic = iconv_open(SJIS, GBK);
        else
            ic = iconv_open(ISO8859, GBK);
    }
    else {
        ic = iconv_open(UTF16LE, GBK);
    }

    if(ic == (iconv_t)-1) {
        perror("iconv_open");
        return -1;
    }

    /* Convert to the proper encoding */
    in = strlen(q->long_desc) + 1;
    out = 65524;
    inptr = (char *)q->long_desc;
    outptr = (char *)pkt->msg;
    iconv(ic, &inptr, &in, &outptr, &out);
    iconv_close(ic);

    /* Figure out how long the packet is */
    size += 65524 - out;

    /* Pad to a length divisible by 4 */
    while(size & 0x03) {
        sendbuf[size++] = 0;
    }

    /* 填充数据头 */
    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        pkt->hdr.dc.pkt_type = DL_QUEST_INFO_TYPE;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(size);
    }
    else {
        pkt->hdr.pc.pkt_type = DL_QUEST_INFO_TYPE;
        pkt->hdr.pc.flags = 0;
        pkt->hdr.pc.pkt_len = LE16(size);
    }

    /* 将数据包发送出去 */
    return crypt_send(c, size);
}

int send_dc_version_detect(login_client_t *c) {
    patch_send_pkt *pkt = (patch_send_pkt *)sendbuf;
    uint16_t size;
    patch_send_footer *ftr;
    uint32_t v;

    /* Make sure we don't accidentially send this to any V1 or NTE clients. */
    v = c->ext_version & CLIENT_EXTVER_DC_VER_MASK;
    if(v == CLIENT_EXTVER_DCV1 || v == CLIENT_EXTVER_DCNTE)
        return -1;

    size = DC_PATCH_HEADER_LENGTH + DC_PATCH_FOOTER_LENGTH +
        patch_ver_detect_dc_len;
    ftr = (patch_send_footer *)(sendbuf + DC_PATCH_HEADER_LENGTH +
                                patch_ver_detect_dc_len);

    /* Fill in the information before the patch data. */
    pkt->entry_offset = LE32(size - 0x08);
    pkt->crc_start = LE32(0x00000000);
    pkt->crc_length = LE32(0x00000000);
    pkt->code_begin = LE32(4);
    memcpy(pkt->code, patch_ver_detect_dc, patch_ver_detect_dc_len);

    /* Fill in the footer... */
    ftr->offset_count = LE32(size - 0x14);
    ftr->num_ptrs = 1;
    ftr->unk1 = ftr->unk2 = 0;
    ftr->offset_start = 0;
    ftr->offset_entry = 0;
    ftr->offsets[0] = 0;                /* Padding... Not actually used. */

    /* 填充数据头. */
    pkt->hdr.dc.pkt_type = PATCH_TYPE;
    pkt->hdr.dc.flags = 0xff;
    pkt->hdr.dc.pkt_len = LE16(size);

    /* 将数据包发送出去 */
    return crypt_send(c, size);
}

static int send_gc_real_version_detect(login_client_t *c) {
    patch_send_pkt *pkt = (patch_send_pkt *)sendbuf;
    uint16_t size;
    patch_send_footer *ftr;

    size = DC_PATCH_HEADER_LENGTH + DC_PATCH_FOOTER_LENGTH +
        patch_ver_detect_gc_len;
    ftr = (patch_send_footer *)(sendbuf + DC_PATCH_HEADER_LENGTH +
                               patch_ver_detect_gc_len);

    /* Fill in the information before the patch data. */
    pkt->entry_offset = LE32(size - 0x08);
    pkt->crc_start = LE32(0x00000000);
    pkt->crc_length = LE32(0x00000000);
    pkt->code_begin = htonl(4);
    memcpy(pkt->code, patch_ver_detect_gc, patch_ver_detect_gc_len);

    /* Fill in the footer... */
    ftr->offset_count = htonl(size - 0x14);
    ftr->num_ptrs = htonl(1);
    ftr->unk1 = ftr->unk2 = 0;
    ftr->offset_start = 0;
    ftr->offset_entry = 0;
    ftr->offsets[0] = 0;                /* Padding... Not actually used. */

    /* 填充数据头. */
    pkt->hdr.dc.pkt_type = PATCH_TYPE;
    pkt->hdr.dc.flags = 0xff;
    pkt->hdr.dc.pkt_len = LE16(size);

    /* 将数据包发送出去 */
    return crypt_send(c, size);
}

static int send_gc_patch_cache_invalidate(login_client_t *c) {
    patch_send_pkt *pkt = (patch_send_pkt *)sendbuf;
    uint16_t size;
    patch_send_footer *ftr;

    size = DC_PATCH_HEADER_LENGTH + DC_PATCH_FOOTER_LENGTH +
        patch_ver_detect_gc_len;
    ftr = (patch_send_footer *)(sendbuf + DC_PATCH_HEADER_LENGTH +
                               patch_ver_detect_gc_len);

    /* Fill in the information before the patch data. */
    pkt->entry_offset = LE32(size - 0x08);
    pkt->crc_start = LE32(0x00000000);
    pkt->crc_length = LE32(0x00000000);
    pkt->code_begin = htonl(0x8000c274);
    memcpy(pkt->code, patch_ver_detect_gc, patch_ver_detect_gc_len);

    /* Fill in the footer... */
    ftr->offset_count = htonl(0x7f2634ec); /* Makes the flush len about 32KiB */
    ftr->num_ptrs = htonl(0);
    ftr->unk1 = ftr->unk2 = 0;
    ftr->offset_start = 0;
    ftr->offset_entry = 0;
    ftr->offsets[0] = 0;                /* Padding... Not actually used. */

    /* 填充数据头. */
    pkt->hdr.dc.pkt_type = PATCH_TYPE;
    pkt->hdr.dc.flags = 0xfe;
    pkt->hdr.dc.pkt_len = LE16(size);

    /* 将数据包发送出去 */
    return crypt_send(c, size);
}

int send_gc_version_detect(login_client_t *c) {
    uint32_t v = c->ext_version & CLIENT_EXTVER_GC_EP_MASK;

    /* This shouldn't happen, but just in case. */
    if(v == CLIENT_EXTVER_GC_EP12PLUS)
        return -1;

    /* Now, grab what we really care about here. */
    v = (c->ext_version >> 8) & 0xFF;

    /* Is the user on Japanese v1.02 or either version of the US releases? */
    if(v == 0x30 || v == 0x31)
        send_gc_patch_cache_invalidate(c);

    return send_gc_real_version_detect(c);
}

int send_single_patch_dc(login_client_t *c, const patchset_t *p) {
    patch_send_pkt *pkt = (patch_send_pkt *)sendbuf;
    uint16_t size;
    uint16_t code_len;
    patch_send_footer *ftr;
    login_patch_file_t *pf;
    char fn[256];                       /* XXXX */
    uint32_t v;
    int rv;

    /* Make sure we don't accidentially send this to any V1 or NTE clients. */
    v = c->ext_version & CLIENT_EXTVER_DC_VER_MASK;
    if(v == CLIENT_EXTVER_DCV1 || v == CLIENT_EXTVER_DCNTE)
        return -1;

    /* Read the specified patch */
    sprintf(fn, "%s/v2/%s", cfg->patch_dir, p->filename);
    pf = patch_file_read(fn);

    if(!pf) {
        /* Uh oh... */
        return -1;
    }

    /* Fill in the information before the patch data. */
    pkt->crc_start = LE32(0x00000000);
    pkt->crc_length = LE32(0x00000000);
    pkt->code_begin = LE32(4);
    memcpy(pkt->code, patch_stub_dc, patch_stub_dc_len);

    while(pf) {
        code_len = patch_stub_dc_len;
        v = 0;

        /* Copy the patch data to the packet, as best we can. If this finishes,
           it will clean up the patch file for us, otherwise, it'll do it on a
           future pass... */
        code_len += patch_build_packet(pkt->code + patch_stub_dc_len,
                                       800 - patch_stub_dc_len, &pf, &v, 0);

        /* Fill in the rest... */
        size = DC_PATCH_HEADER_LENGTH + DC_PATCH_FOOTER_LENGTH + code_len;
        pkt->entry_offset = LE32(size - 0x08);

        /* Fill in the footer... */
        ftr = (patch_send_footer *)(sendbuf + DC_PATCH_HEADER_LENGTH +
                                    code_len);
        ftr->offset_count = LE32(size - 0x14);
        ftr->num_ptrs = LE32(1);
        ftr->unk1 = ftr->unk2 = 0;
        ftr->offset_start = 0;
        ftr->offset_entry = 0;
        ftr->offsets[0] = 0;                /* Padding... Not actually used. */

        /* 填充数据头. */
        pkt->hdr.dc.pkt_type = PATCH_TYPE;
        pkt->hdr.dc.flags = 0;
        pkt->hdr.dc.pkt_len = LE16(size);

        /* 将数据包发送出去 */
        if((rv = crypt_send(c, size)) < 0) {
            if(pf)
                patch_file_free(pf);
            return rv;
        }
    }

    return 0;
}

static int send_single_patch_gc_inval(login_client_t *c, const patchset_t *p) {
    patch_send_pkt *pkt = (patch_send_pkt *)sendbuf;
    uint16_t size;
    uint16_t code_len = 64;
    patch_send_footer *ftr;
    login_patch_file_t *pf;
    char fn[256];                       /* XXXX */

    /* Read the specified patch */
    sprintf(fn, "%s/gc/%s", cfg->patch_dir, p->filename);
    pf = patch_file_read(fn);

    if(!pf) {
        /* Uh oh... */
        return -1;
    }

    /* We have to do this in two parts, because of Sega's stupid bug. In the
       first part, we send a small portion of the code of the patch stub (just
       the part that actually flushes/invalidates the cache, basically) over
       and have some code in the game flush the cache over that. Afterwards,
       we send the whole patch stub and patch data over, having the code that
       we previously sent do the job. It's ugly and hackish, but it works.*/
    pkt->crc_start = LE32(0x00000000);
    pkt->crc_length = LE32(0x00000000);
    pkt->code_begin = htonl(0x8000c274);
    memcpy(pkt->code, patch_stub_gc, code_len); /* Ugly hack, but whatever. */

    size = DC_PATCH_HEADER_LENGTH + DC_PATCH_FOOTER_LENGTH + code_len;
    pkt->entry_offset = LE32(size - 0x08);

    /* Fill in the footer... */
    ftr = (patch_send_footer *)(sendbuf + DC_PATCH_HEADER_LENGTH +
                                code_len);
    ftr->offset_count = htonl(0x7f2634ec); /* Makes the flush len about 32KiB */
    ftr->num_ptrs = 0;
    ftr->unk1 = ftr->unk2 = 0;
    ftr->offset_start = 0;
    ftr->offset_entry = 0;
    ftr->offsets[0] = 0;                /* Padding... Not actually used. */

    /* 填充数据头. */
    pkt->hdr.dc.pkt_type = PATCH_TYPE;
    pkt->hdr.dc.flags = 0;
    pkt->hdr.dc.pkt_len = LE16(size);

    /* 将数据包发送出去 */
    crypt_send(c, size);

    /* Fill in the information before the patch data, for real this time. */
    code_len = patch_stub_gc_len;
    pkt->crc_start = LE32(0x00000000);
    pkt->crc_length = LE32(0x00000000);
    pkt->code_begin = htonl(4);
    memcpy(pkt->code, patch_stub_gc, patch_stub_gc_len);
    pkt->code[patch_stub_gc_len] = (uint8_t)(pf->patch_count >> 24);
    pkt->code[patch_stub_gc_len + 1] = (uint8_t)(pf->patch_count >> 16);
    pkt->code[patch_stub_gc_len + 2] = (uint8_t)(pf->patch_count >> 8);
    pkt->code[patch_stub_gc_len + 3] = (uint8_t)pf->patch_count;
    memcpy(pkt->code + patch_stub_gc_len + 4, pf->data, pf->length);
    code_len += 4 + pf->length;

    patch_file_free(pf);

    /* Fill in the rest... */
    size = DC_PATCH_HEADER_LENGTH + DC_PATCH_FOOTER_LENGTH + code_len;
    pkt->entry_offset = LE32(size - 0x08);

    /* Fill in the footer... */
    ftr = (patch_send_footer *)(sendbuf + DC_PATCH_HEADER_LENGTH +
                                code_len);
    ftr->offset_count = htonl(size - 0x14);
    ftr->num_ptrs = htonl(1);
    ftr->unk1 = ftr->unk2 = 0;
    ftr->offset_start = 0;
    ftr->offset_entry = 0;
    ftr->offsets[0] = 0;                /* Padding... Not actually used. */

    /* 填充数据头. */
    pkt->hdr.dc.pkt_type = PATCH_TYPE;
    pkt->hdr.dc.flags = 0;
    pkt->hdr.dc.pkt_len = LE16(size);

    /* 将数据包发送出去 */
    return crypt_send(c, size);
}

static int send_single_patch_gc_noinv(login_client_t *c, const patchset_t *p) {
    patch_send_pkt *pkt = (patch_send_pkt *)sendbuf;
    uint16_t size;
    uint16_t code_len = patch_stub_gc_len;
    patch_send_footer *ftr;
    login_patch_file_t *pf;
    char fn[256];                       /* XXXX */

    /* Read the specified patch */
    sprintf(fn, "%s/gc/%s", cfg->patch_dir, p->filename);
    pf = patch_file_read(fn);

    if(!pf) {
        /* Uh oh... */
        return -1;
    }

    /* Fill in the information before the patch data. */
    pkt->crc_start = LE32(0x00000000);
    pkt->crc_length = LE32(0x00000000);
    pkt->code_begin = htonl(4);
    memcpy(pkt->code, patch_stub_gc, patch_stub_gc_len);
    pkt->code[patch_stub_gc_len] = (uint8_t)(pf->patch_count >> 24);
    pkt->code[patch_stub_gc_len + 1] = (uint8_t)(pf->patch_count >> 16);
    pkt->code[patch_stub_gc_len + 2] = (uint8_t)(pf->patch_count >> 8);
    pkt->code[patch_stub_gc_len + 3] = (uint8_t)pf->patch_count;
    memcpy(pkt->code + patch_stub_gc_len + 4, pf->data, pf->length);
    code_len += 4 + pf->length;

    patch_file_free(pf);

    /* Fill in the rest... */
    size = DC_PATCH_HEADER_LENGTH + DC_PATCH_FOOTER_LENGTH + code_len;
    pkt->entry_offset = LE32(size - 0x08);

    /* Fill in the footer... */
    ftr = (patch_send_footer *)(sendbuf + DC_PATCH_HEADER_LENGTH +
                                code_len);
    ftr->offset_count = htonl(size - 0x14);
    ftr->num_ptrs = htonl(1);
    ftr->unk1 = ftr->unk2 = 0;
    ftr->offset_start = 0;
    ftr->offset_entry = 0;
    ftr->offsets[0] = 0;                /* Padding... Not actually used. */

    /* 填充数据头. */
    pkt->hdr.dc.pkt_type = PATCH_TYPE;
    pkt->hdr.dc.flags = 0;
    pkt->hdr.dc.pkt_len = LE16(size);

    /* 将数据包发送出去 */
    return crypt_send(c, size);
}

int send_single_patch_gc(login_client_t *c, const patchset_t *p) {
    uint32_t v = c->ext_version & CLIENT_EXTVER_GC_EP_MASK;

    /* This shouldn't happen, but just in case. */
    if(v == CLIENT_EXTVER_GC_EP12PLUS)
        return -1;

    /* Now, grab what we really care about here. */
    v = (c->ext_version >> 8) & 0xFF;

    /* Is the user on Japanese v1.02 or either version of the US releases? */
    if(v == 0x30 || v == 0x31)
        return send_single_patch_gc_inval(c, p);
    else
        return send_single_patch_gc_noinv(c, p);
}

static int send_patch_menu_dcgc(login_client_t *c) {
    dc_ship_list_pkt *pkt = (dc_ship_list_pkt *)sendbuf;
    int len = 0x04, count = 0;
    uint32_t i, j;
    patch_t *p;
    patch_list_t *pl;

    /* Fill in the "DATABASE/US" entry */
    pkt->entries[count].menu_id = LE32(MENU_ID_DATABASE);
    pkt->entries[count].item_id = 0;
    pkt->entries[count].flags = LE16(0x0004);
    strncpy(pkt->entries[count].name, "DATABASE/US", 0x12);
    pkt->entries[count].name[0x11] = 0x08;
    ++count;
    len += 0x1C;

    /* Add our entries... */
    pkt->entries[count].menu_id = LE32(MENU_ID_PATCH);
    pkt->entries[count].item_id = LE32(ITEM_ID_LAST);
    pkt->entries[count].flags = LE16(0x0004);
    strncpy(pkt->entries[count].name, "Main Menu", 0x12);
    ++count;
    len += 0x1C;

    /* Which list are we reading from? */
    if(c->type == CLIENT_AUTH_DC)
        pl = patches_v2;
    else if(c->type == CLIENT_AUTH_GC)
        pl = patches_gc;
    else
        return -1;

    for(i = 0; i < pl->patch_count; ++i) {
        p = pl->patches[i];

        for(j = 0; j < p->patchset_count; ++j) {
            /* Skip any GM-only patches for anyone who isn't a GM. */
            if(!IS_GLOBAL_GM(c) && p->gmonly)
                continue;

            /* Make sure they have the required permissions if any are required
               by the patch. */
            if(!IS_GLOBAL_ROOT(c) && (p->perms & c->priv) != p->perms)
                continue;

            if(p->patches[j]->version == c->det_version) {
                pkt->entries[count].menu_id = LE32(MENU_ID_PATCH);
                pkt->entries[count].item_id = LE32(p->id);
                pkt->entries[count].flags = LE16(0x0004);
                strncpy(pkt->entries[count].name, p->name[CLIENT_LANG_ENGLISH],
                        0x12);
                ++count;
                len += 0x1C;
            }
        }
    }

    /* 填充数据头 */
    pkt->hdr.pkt_type = BLOCK_LIST_TYPE;
    pkt->hdr.flags = (uint8_t)(count - 1);
    pkt->hdr.pkt_len = LE16(len);

    /* 将数据包发送出去 */
    return crypt_send(c, len);
}

int send_patch_menu(login_client_t *c) {
    uint32_t v;

    switch(c->type) {
        case CLIENT_AUTH_GC:
            v = c->ext_version & CLIENT_EXTVER_GC_EP_MASK;
            if (c->type == CLIENT_AUTH_GC && v != CLIENT_EXTVER_GC_EP12PLUS &&
                patches_gc) {
                /* Make sure we don't send this to Episode I & II Plus. */
                if (v == CLIENT_EXTVER_GC_EP12PLUS)
                    return send_initial_menu(c);
                else if (c->det_version)
                    return send_patch_menu_dcgc(c);
            }

            return send_initial_menu(c);

        case CLIENT_AUTH_DC:
            /* Make sure we don't send this to V1 or NTE */
            v = c->ext_version & CLIENT_EXTVER_DC_VER_MASK;
            if (v != CLIENT_EXTVER_DCV1 && v != CLIENT_EXTVER_GC_TRIAL &&
                patches_v2) {
                if (v == CLIENT_EXTVER_DCV1 || v == CLIENT_EXTVER_DCNTE ||
                    v == CLIENT_EXTVER_GC_TRIAL)
                    return send_initial_menu(c);
                else if (c->det_version)
                    return send_patch_menu_dcgc(c);
            }

            return send_initial_menu(c);
    }

    return -1;
}

static int send_crc_check_pc(login_client_t *c, uint32_t st, uint32_t count) {
    patch_send_pkt *pkt = (patch_send_pkt *)sendbuf;

    /* Fill in the request to CRC data... */
    pkt->entry_offset = 0;
    pkt->crc_start = LE32(st);
    pkt->crc_length = LE32(count);
    pkt->code_begin = 0;

    /* 填充数据头. */
    pkt->hdr.pc.pkt_type = PATCH_TYPE;
    pkt->hdr.pc.flags = 0xFE;
    pkt->hdr.pc.pkt_len = LE16(DC_PATCH_HEADER_LENGTH);

    /* 将数据包发送出去 */
    return crypt_send(c, DC_PATCH_HEADER_LENGTH);
}

int send_crc_check(login_client_t *c, uint32_t start, uint32_t count) {
    switch(c->type) {
        case CLIENT_AUTH_PC:
            return send_crc_check_pc(c, start, count);
    }

    return -1;
}

int send_disconnect(login_client_t* c, uint32_t flags) {
    //char query[512];
    char ipstr[INET6_ADDRSTRLEN];

    my_ntop(&c->ip_addr, ipstr);

    c->disconnected = 1;

    if (flags) {
        c->islogged = 0;

        if (db_update_gc_login_state(c->guildcard, c->islogged, c->sec_data.slot, NULL))
            return -4;
    }

    return 0;
}