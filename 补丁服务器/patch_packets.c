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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <WinSock_Defines.h>

#include <f_checksum.h>

#include "patch_server.h"
#include "patch_packets.h"

#define CHUNK_MAX 24576

static uint8_t sendbuf[65536];

/* Send a raw packet away. */
static int send_raw(patch_client_t *c, int len) {
    ssize_t rv, total = 0;
    void *tmp;

    /* Keep trying until the whole thing's sent. */
    if(!c->sendbuf_cur) {
        while(total < len) {
            rv = send(c->sock, (PCHAR)(sendbuf + total), len - total, 0);

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

/* Encrypt and send a packet away. */
static int crypt_send(patch_client_t* c, int len) {
    /* Expand it to be a multiple of 8/4 bytes long */
    while (len & (hdr_sizes[c->type] - 1)) {
        sendbuf[len++] = 0;
    }

    /* Encrypt the packet */
    CRYPT_CryptData(&c->server_cipher, sendbuf, len, 1);

    return send_raw(c, len);
}

/* Send a simple "header-only" packet to the given client. */
int send_simple(patch_client_t *c, uint16_t type) {
    pkt_header_t *pkt = (pkt_header_t *)sendbuf;

    /* 填充数据头 */
    pkt->pkt_len = LE16(PACKET_HEADER_LENGTH);
    pkt->pkt_type = LE16(type);

    /* Encrypt the packet */
    CRYPT_CryptData(&c->server_cipher, pkt, PACKET_HEADER_LENGTH, 1);

    /* 将数据包发送出去 */
    return send_raw(c, PACKET_HEADER_LENGTH);
}

/* Send a Welcome packet to the given client. */
int send_welcome(patch_client_t *c, uint32_t svect, uint32_t cvect) {
    patch_welcome_pkt *pkt = (patch_welcome_pkt *)sendbuf;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(patch_welcome_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = LE16(PATCH_WELCOME_LENGTH);
    pkt->hdr.pkt_type = LE16(PATCH_WELCOME_TYPE);

    /* Fill in the required message */
    memcpy(pkt->copyright, patch_welcome_copyright, 44);

    /* Fill in the two vectors */
    pkt->server_vector = LE32(svect);
    pkt->client_vector = LE32(cvect);

    /* 将数据包发送出去 */
    return send_raw(c, PATCH_WELCOME_LENGTH);
}

/* Send the packet containing the textual welcome message to the client. */
int send_message(patch_client_t *c, uint16_t *msg, uint16_t size) {
    pkt_header_t *pkt = (pkt_header_t *)sendbuf;
    uint16_t s = size + PACKET_HEADER_LENGTH;

    /* If we have a non-divisible-by-four number of bytes in the message, it
       must still be divisible by 2, so, add 2 and we'll have something
       divisible by 4. */
    if(size & 0x03) {
        s += 2;
        sendbuf[PACKET_HEADER_LENGTH + size] = 0x00;
        sendbuf[PACKET_HEADER_LENGTH + size + 1] = 0x00;
    }

    pkt->pkt_type = LE16(PATCH_MESSAGE_TYPE);
    pkt->pkt_len = LE16(s);
    memcpy(sendbuf + PACKET_HEADER_LENGTH, msg, size);

    /* Encrypt the packet */
    CRYPT_CryptData(&c->server_cipher, pkt, s, 1);

    /* 将数据包发送出去 */
    return send_raw(c, s);
}

/* Send the data server redirect packet to the given client.
   IP and port MUST both be in network byte-order. */
int send_redirect(patch_client_t *c, in_addr_t ip, uint16_t port) {
    patch_redirect_pkt *pkt = (patch_redirect_pkt *)sendbuf;

    /* Wipe the packet */
    memset(pkt, 0, PATCH_REDIRECT_LENGTH);

    /* 填充数据头, and copy the IP/Port. */
    pkt->hdr.pkt_len = LE16(PATCH_REDIRECT_LENGTH);
    pkt->hdr.pkt_type = LE16(PATCH_REDIRECT_TYPE);
    pkt->data_ip = ip;
    pkt->data_port = LE16(port);
    pkt->padding = 0;

    /* Encrypt the packet */
    CRYPT_CryptData(&c->server_cipher, pkt, PATCH_REDIRECT_LENGTH, 1);

    /* 将数据包发送出去 */
    return send_raw(c, PATCH_REDIRECT_LENGTH);
}

#ifdef ENABLE_IPV6

/* Send a IPv6 redirect packet to the given client.
   The port must be in network byte order. */
int send_redirect6(patch_client_t *c, const uint8_t ip[16], uint16_t port) {
    patch_redirect6_pkt *pkt = (patch_redirect6_pkt *)sendbuf;

    /* 填充数据头, and copy the IP/Port. */
    pkt->hdr.pkt_len = LE16(PATCH_REDIRECT6_LENGTH);
    pkt->hdr.pkt_type = LE16(PATCH_REDIRECT6_TYPE);
    memcpy(pkt->data_ip, ip, 16);
    pkt->data_port = port;
    pkt->padding = 0;

    /* Encrypt the packet */
    CRYPT_CryptData(&c->server_cipher, pkt, PATCH_REDIRECT6_LENGTH, 1);

    /* 将数据包发送出去 */
    return send_raw(c, PATCH_REDIRECT6_LENGTH);
}

#endif

/* Send a change directory packet ot the given client. */
int send_chdir(patch_client_t *c, const char dir[]) {
    patch_chdir_pkt *pkt = (patch_chdir_pkt *)sendbuf;

    /* Make sure the directory name will fit. */
    if(strlen(dir) > 64) {
        return -1;
    }

    /* 填充数据头, and copy the directory name. */
    memset(pkt, 0, PATCH_SET_DIRECTORY_LENGTH);
    pkt->hdr.pkt_len = LE16(PATCH_SET_DIRECTORY_LENGTH);
    pkt->hdr.pkt_type = LE16(PATCH_SET_DIRECTORY);
    strcpy(pkt->dir, dir);

    /* Encrypt the packet. */
    CRYPT_CryptData(&c->server_cipher, pkt, PATCH_SET_DIRECTORY_LENGTH, 1);

    /* 将数据包发送出去. */
    return send_raw(c, PATCH_SET_DIRECTORY_LENGTH);
}

/* Send a file information packet to the given client. */
int send_file_info(patch_client_t *c, uint32_t idx, const char fn[]) {
    patch_file_info_pkt *pkt = (patch_file_info_pkt *)sendbuf;

    /* Make sure the filename will fit. */
    if(strlen(fn) > 32) {
        return -1;
    }

    /* 填充数据头, and copy the file name */
    memset(pkt, 0, PATCH_FILE_INFO_LENGTH);
    pkt->hdr.pkt_len = LE16(PATCH_FILE_INFO_LENGTH);
    pkt->hdr.pkt_type = LE16(PATCH_FILE_INFO);
    pkt->patch_id = LE32(idx);
    strcpy(pkt->filename, fn);

    /* Encrypt the packet. */
    CRYPT_CryptData(&c->server_cipher, pkt, PATCH_FILE_INFO_LENGTH, 1);

    /* 将数据包发送出去. */
    return send_raw(c, PATCH_FILE_INFO_LENGTH);
}

/* Send a file-send information packet to the given client. */
int send_send_info(patch_client_t *c, uint32_t size, uint32_t files) {
    patch_send_info_pkt *pkt = (patch_send_info_pkt *)sendbuf;

    /* 填充数据头 and copy the data. */
    pkt->hdr.pkt_len = LE16(PATCH_SEND_INFO_LENGTH);
    pkt->hdr.pkt_type = LE16(PATCH_SEND_INFO);
    pkt->total_length = LE32(size);
    pkt->total_files = LE32(files);

    /* Encrypt the packet. */
    CRYPT_CryptData(&c->server_cipher, pkt, PATCH_SEND_INFO_LENGTH, 1);

    /* 将数据包发送出去. */
    return send_raw(c, PATCH_SEND_INFO_LENGTH);
}

/* Send a file send packet to the given client. */
int send_file_send(patch_client_t *c, uint32_t size, const char fn[]) {
    patch_file_send_pkt *pkt = (patch_file_send_pkt *)sendbuf;

    /* Make sure the file name's length is short enough. */
    if(strlen(fn) > 48) {
        return -1;
    }

    /* 填充数据头 and information. */
    memset(pkt, 0, PATCH_FILE_SEND_LENGTH);
    pkt->hdr.pkt_len = LE16(PATCH_FILE_SEND_LENGTH);
    pkt->hdr.pkt_type = LE16(PATCH_FILE_SEND);
    pkt->padding = 0;
    pkt->size = LE32(size);
    strcpy(pkt->filename, fn);

    /* Encrypt the packet. */
    CRYPT_CryptData(&c->server_cipher, pkt, PATCH_FILE_SEND_LENGTH, 1);

    /* 将数据包发送出去. */
    return send_raw(c, PATCH_FILE_SEND_LENGTH);
}

/* Send a part of a file to the given client (dividing it into chunks). */
int send_file_chunk(patch_client_t *c, const char fn[], const char dir[]) {
    char realfn[256 + 2];
    FILE *fp;
    uint32_t sz;
    patch_data_send_pkt *pkt = (patch_data_send_pkt *)sendbuf;
    uint32_t cks;
    uint16_t len;
    int rv = 0;

    /* Open up the file */
    sprintf(realfn, "%s/%s", dir, fn);
    errno_t err = fopen_s(&fp, realfn, "rb");
    //fp = fopen(realfn, "rb");

    if(err) {
        return -1;
    }

    /* Move to where we need to be to send the current chunk. */
    fseek(fp, c->cur_pos, SEEK_SET);

    /* Read the data from the file, and calculate the checksum and length. */
    sz = (uint32_t)fread(sendbuf + PATCH_DATA_SEND_LENGTH, 1, CHUNK_MAX, fp);

    /* If we don't have anything, there's been some kind of error... */
    if(sz == 0) {
        fclose(fp);
        return -1;
    }

    cks = psocn_crc32(sendbuf + PATCH_DATA_SEND_LENGTH, (int)sz);
    len = ((uint16_t)sz + PATCH_DATA_SEND_LENGTH);

    /* Round to a nice even 4-byte boundary. */
    while(len & 0x03) {
        sendbuf[len++] = 0;
    }

    /* Fill in information about this chunk. */
    pkt->hdr.pkt_type = LE16(PATCH_DATA_SEND);
    pkt->hdr.pkt_len = LE16(len);
    pkt->chunk_num = LE32(c->cur_chunk);
    pkt->checksum = LE32(cks);
    pkt->chunk_size = LE32(sz);

    /* Encrypt the packet. */
    CRYPT_CryptData(&c->server_cipher, pkt, len, 1);

    /* Send the chunk away. */
    if(send_raw(c, len)) {
        fclose(fp);
        return -2;
    }

    /* Update our state. */
    ++c->cur_chunk;
    c->cur_pos += sz;

    /* Figure out if we've read the end of the file. */
    rv = !!feof(fp);

    /* We're done, close the file, and return success. */
    fclose(fp);

    return rv;
}

/* Send a file done packet to the given client. */
int send_file_done(patch_client_t *c) {
    patch_file_done_pkt *pkt = (patch_file_done_pkt *)sendbuf;

    /* 填充数据头. */
    pkt->hdr.pkt_len = LE16(PATCH_FILE_DONE_LENGTH);
    pkt->hdr.pkt_type = LE16(PATCH_FILE_DONE);
    pkt->padding = 0;

    /* Encrypt the packet. */
    CRYPT_CryptData(&c->server_cipher, pkt, PATCH_FILE_DONE_LENGTH, 1);

    /* 将数据包发送出去. */
    return send_raw(c, PATCH_FILE_DONE_LENGTH);
}
