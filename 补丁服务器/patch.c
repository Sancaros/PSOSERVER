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
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <queue.h>
#include <mtwist.h>

#include "patch_server.h"
#include "patch_packets.h"
#include <debug.h>
#include <f_logs.h>
#include <pso_ping.h>

/* Create a new connection, storing it in the list of clients. */
patch_client_t* create_connection(int sock, int type,
    struct sockaddr* ip, socklen_t size) {
    patch_client_t* rv;
    uint32_t svect, cvect;

    /* Allocate the space for the new client. */
    rv = (patch_client_t*)malloc(sizeof(patch_client_t));

    if (!rv) {
        return NULL;
    }

    memset(rv, 0, sizeof(patch_client_t));

    /* Store basic parameters in the client structure. */
    rv->sock = sock;
    rv->type = type;
    memcpy(&rv->ip_addr, ip, size);

    /* Is the user on IPv6? */
    if (ip->sa_family == AF_INET6) {
        rv->is_ipv6 = 1;
    }

    /* Initialize the random number generator. The seed value is the current
       UNIX time, xored with the port (so that each block will use a different
       seed even though they'll probably get the same timestamp). */
    mt19937_init(&rv->rng, (uint32_t)(time(NULL) ^ sock));

    /* Generate the encryption keys for the client and server. */
    cvect = mt19937_genrand_int32(&rv->rng);
    svect = mt19937_genrand_int32(&rv->rng);

    CRYPT_CreateKeys(&rv->client_cipher, &cvect, CRYPT_PC);
    CRYPT_CreateKeys(&rv->server_cipher, &svect, CRYPT_PC);

    /* Send the client the welcome packet, or die trying. */
    if (send_welcome(rv, svect, cvect)) {
        closesocket(sock);
        free_safe(rv);
        return NULL;
    }

    /* Initialize the file list */
    TAILQ_INIT(&rv->files);

    /* Insert it at the end of our list, and we're done. */
    TAILQ_INSERT_TAIL(&clients, rv, qentry);

    return rv;
}

/* Destroy a connection, closing the socket and removing it from the list. */
void destroy_connection(patch_client_t* c) {
    patch_cfile_t* i, * tmp;

    TAILQ_REMOVE(&clients, c, qentry);

    i = TAILQ_FIRST(&c->files);

    while (i) {
        tmp = TAILQ_NEXT(i, qentry);
        free_safe(i);
        i = tmp;
    }

    if (c->sock >= 0) {
        closesocket(c->sock);
    }

    if (c->recvbuf) {
        free_safe(c->recvbuf);
    }

    if (c->sendbuf) {
        free_safe(c->sendbuf);
    }

    free_safe(c);
}

/* Send the patch packets needed to change the client's current directory to the
   given destination. */
int change_directory(patch_client_t* c, const char cur[],
    const char dst[]) {
    char* s1, * s2, * d1 = { 0 }, * d2 = { 0 }, * t1, * t2;
    int rv = 0;

    /* If the current and destination are the same directory, return. */
    if (!strcmp(cur, dst)) {
        return 0;
    }

    /* Otherwise, split up the two directories, and figure out where they
       differ. */
    s1 = _strdup(cur);
    s2 = _strdup(dst);

    t1 = strtok_s(s1, "/", &d1);
    t2 = strtok_s(s2, "/", &d2);

    while (t1 && t2 && !strcmp(t1, t2)) {
        t1 = strtok_s(NULL, "/", &d1);
        t2 = strtok_s(NULL, "/", &d2);
    }

    /* If t1 is non-NULL, we need to go up the tree as many times as we have
       path components left to be parsed. */
    while (t1) {
        if (send_simple(c, PATCH_ONE_DIR_UP)) {
            rv = -1;
            goto out;
        }

        t1 = strtok_s(NULL, "/", &d1);
    }

    /* Now, if t2 is non-NULL, we need to go down the tree as many times as we
       have path components left to be parsed. */
    while (t2) {
        if (send_chdir(c, t2)) {
            rv = -1;
            goto out;
        }

        t2 = strtok_s(NULL, "/", &d2);
    }

out:
    /* We should be where we belong, clean up. */
    free(s1);
    free(s2);

    return rv;
}

/* Send the list of files to check for patching to the client. */
int send_file_list(patch_client_t* c, struct file_queue* q) {
    uint32_t filenum = 0;
    patch_file_t* i;
    char dir[MAX_PATH], dir2[MAX_PATH];
    char* bn;
    int dlen;

    /* Send the initial chdir "." packet */
    if (send_chdir(c, ".")) {
        return -1;
    }

    strcpy(dir, "");

    /* Loop through each patch file, sending the appropriate packets for it. */
    TAILQ_FOREACH(i, q, qentry) {
        bn = strrchr(i->filename, '/');

        if (bn) {
            bn += 1;
            dlen = strlen(i->filename) - strlen(bn) - 1;
        }
        else {
            dlen = 0;
            bn = i->filename;
        }

        /* Copy over the directory that the file exists in. */
        strncpy(dir2, i->filename, dlen);
        dir2[dlen] = 0;

        /* Change the directory the client is in, if appropriate. */
        if (change_directory(c, dir, dir2)) {
            return -3;
        }

        /* Send the file info request. */
        if (send_file_info(c, filenum, bn)) {
            return -2;
        }

        /* We're now in dir2, so save it for the next pass. */
        strcpy(dir, dir2);
        ++filenum;
    }

    /* Change back to the base directory. */
    if (change_directory(c, dir, "")) {
        return -3;
    }

    /* Tethealla always preceeds the done packet with a one-directory up packet,
       so we probably should too. */
    if (send_simple(c, PATCH_ONE_DIR_UP)) {
        return -1;
    }

    /* Send the file list complete marker. */
    if (send_simple(c, PATCH_INFO_FINISHED)) {
        return -1;
    }

    return 0;
}

/* Fetch the given patch index. */
patch_file_t* fetch_patch(uint32_t idx, struct file_queue* q) {
    patch_file_t* i = TAILQ_FIRST(q);

    while (i && idx) {
        i = TAILQ_NEXT(i, qentry);
        --idx;
    }

    return i;
}

/* 处理补丁欢迎信息 */
static int handle_patch_welcome(patch_client_t* c) {
    /* TODO 是否 还需要二次开发？*/

    return send_simple(c, PATCH_LOGIN_TYPE);
}

/* 处理补丁登录 */
static int handle_patch_login(patch_client_t* c) {
    size_t msglen = 0;
    uint16_t port = 0;
    struct file_queue* patch_files = { 0 };

    switch (c->type)
    {
    case CLIENT_TYPE_PC_PATCH:
        port = htons(PC_DATA_PORT);
        goto patch;

    case CLIENT_TYPE_BB_PATCH:
        port = htons(BB_DATA_PORT);
        goto patch;

    case CLIENT_TYPE_PC_DATA:
        patch_files = &cfg->pc_files;
        goto data;

    case CLIENT_TYPE_BB_DATA:
        patch_files = &cfg->bb_files;
        goto data;
    }

patch:
    memset(&patch_welcom_msg[0], 0, sizeof(patch_welcom_msg));

    psocn_web_server_getfile(cfg->w_motd.web_host, cfg->w_motd.web_port, cfg->w_motd.patch_welcom_file, Welcome_Files[0]);
    msglen = psocn_web_server_loadfile(Welcome_Files[0], &patch_welcom_msg[0]);

    if (send_message(c, (uint16_t*)&patch_welcom_msg[0], (uint16_t)msglen))
        //if (send_message(c, cfg->pc_welcome, cfg->pc_welcome_size))
        return -2;

#ifdef ENABLE_IPV6
    if (c->is_ipv6) {
        if (send_redirect6(c, srvcfg->server_ip6, port))
            return -2;
    } else
#endif
    if (send_redirect(c, srvcfg->server_ip, port))
        return -2;

    /* Force the client to disconnect at this point to prevent problems
       later on down the line if it decides to reconnect before we close
       the current socket. */
    c->disconnected = 1;

    return 0;

data:
    if (send_simple(c, PATCH_START_LIST))
        return -2;

    /* Send the list of patches. */
    if (send_file_list(c, patch_files))
        return -2;

    return 0;
}

/* Save the file info sent by the client in their list. */
int handle_file_info_reply(patch_client_t* c, patch_file_info_reply* pkt) {
    patch_cfile_t* n;
    patch_file_t* f;
    patch_file_entry_t* ent;

    if (c->type == CLIENT_TYPE_PC_DATA) {
        f = fetch_patch(LE32(pkt->patch_id), &cfg->pc_files);
    }
    else {
        f = fetch_patch(LE32(pkt->patch_id), &cfg->bb_files);
    }

    if (!f) {
        return -1;
    }

    /* Add it to the list only if we need to send it. */
    if (f->flags & PATCH_FLAG_NO_IF) {
        /* With a single entry, this is easy... */
        if (f->entries->checksum != LE32(pkt->checksum) ||
            f->entries->size != LE32(pkt->size)) {
            n = (patch_cfile_t*)malloc(sizeof(patch_cfile_t));

            if (!n) {
                perror("malloc");
                return -1;
            }

            /* Store the file info. */
            n->file = f;
            n->ent = f->entries;

            /* Add it to the list. */
            TAILQ_INSERT_TAIL(&c->files, n, qentry);
        }
    }
    else {
        ent = f->entries;

        while (ent) {
            if (ent->client_checksum == LE32(pkt->checksum) ||
                ((f->flags & PATCH_FLAG_HAS_ELSE) && !ent->next &&
                    (ent->checksum != LE32(pkt->checksum) ||
                        ent->size != LE32(pkt->size)))) {
                n = (patch_cfile_t*)malloc(sizeof(patch_cfile_t));

                if (!n) {
                    perror("malloc");
                    return -1;
                }

                /* Store the file info. */
                n->file = f;
                n->ent = ent;

                /* Add it to the list. */
                TAILQ_INSERT_TAIL(&c->files, n, qentry);
                break;
            }

            ent = ent->next;
        }
    }

    return 0;
}

/* Act on a list done packet from the client. */
int handle_list_done(patch_client_t* c) {
    uint32_t files = 0, size = 0;
    patch_cfile_t* i, * tmp;
    char dir[MAX_PATH], dir2[MAX_PATH];
    char* bn;
    int dlen;

    /* If we don't have anything to send, send out the send done packet. */
    if (TAILQ_EMPTY(&c->files)) {
        goto done;
    }

    /* If we've got files to send and we haven't started yet, start out. */
    if (c->sending_data == 0) {
        c->sending_data = 1;

        /* Look through the list, and tabulate the data we need to send. */
        TAILQ_FOREACH(i, &c->files, qentry) {
            ++files;
            size += i->ent->size;
        }

        /* Send the informational packet telling about what we're sending. */
        if (send_send_info(c, size, files)) {
            return -2;
        }

        /* Send the initial chdir "." packet */
        if (send_chdir(c, ".")) {
            return -1;
        }

        return 0;
    }

    /* Find the first thing on the top of the list. */
    i = TAILQ_FIRST(&c->files);

    /* Figure out if this is the first file to go, and if we need to figure out
       the current directory. */
    if (c->sending_data == 1) {
        strcpy(dir, "");
    }
    else if (c->sending_data == 2) {
        bn = strrchr(i->file->filename, '/');

        if (bn) {
            bn += 1;
            dlen = strlen(i->file->filename) - strlen(bn) - 1;
        }
        else {
            bn = i->file->filename;
            dlen = 0;
        }

        strncpy(dir, i->file->filename, dlen);
        dir[dlen] = 0;

        /* Figure out what the file is we're going to send. */
        tmp = TAILQ_NEXT(i, qentry);

        /* Remove the current head, we're done with it. */
        TAILQ_REMOVE(&c->files, i, qentry);
        free(i);
        i = tmp;
    }
    /* If we're just starting on a file, change the directory if appropriate. */
    if (c->sending_data < 3 && i) {
        bn = strrchr(i->file->filename, '/');

        if (bn) {
            bn += 1;
            dlen = strlen(i->file->filename) - strlen(bn) - 1;
        }
        else {
            bn = i->file->filename;
            dlen = 0;
        }

        /* Copy over the directory that the file exists in. */
        strncpy(dir2, i->file->filename, dlen);
        dir2[dlen] = 0;

        /* Change the directory the client is in, if appropriate. */
        if (change_directory(c, dir, dir2)) {
            return -3;
        }

        c->sending_data = 3;

        /* Send the file header. */
        return send_file_send(c, i->ent->size, bn);
    }

    /* If we've got this far and we have a file to send still, send the current
       chunk of the file. */
    if (i) {
        if (c->type == CLIENT_TYPE_PC_DATA) {
            dlen = send_file_chunk(c, i->ent->filename, cfg->pc_dir);
        }
        else {
            dlen = send_file_chunk(c, i->ent->filename, cfg->bb_dir);
        }

        if (dlen < 0) {
            /* Something went wrong, bail. */
            return -4;
        }
        else if (dlen > 0) {
            /* We're done with this file. */
            c->sending_data = 2;
            c->cur_chunk = 0;
            c->cur_pos = 0;
            return send_file_done(c);
        }

        return 0;
    }

    /* Change back to the base directory. dir should be set here, since
       c->sending_data has to be 2 if we're in this state. */
    if (change_directory(c, dir, "")) {
        return -3;
    }

    /* Tethealla always preceeds the done packet with a one-directory up packet,
       so we probably should too? */
    if (send_simple(c, PATCH_ONE_DIR_UP)) {
        return -1;
    }

done:
    c->sending_data = 0;
    return send_simple(c, PATCH_SEND_DONE);
}

/* 处理数据包. */
int process_packet(patch_client_t* c, void* pkt) {
    pkt_header_t* data = (pkt_header_t*)pkt;
    uint16_t type = LE16(data->pkt_type);
    uint16_t len = LE16(data->pkt_len);

    switch (type) {
    case PATCH_WELCOME_TYPE:
        return handle_patch_welcome(c);

    case PATCH_LOGIN_TYPE:
        return handle_patch_login(c);

    case PATCH_FILE_INFO_REPLY:
        return handle_file_info_reply(c, (patch_file_info_reply*)pkt);

    case PATCH_FILE_LIST_DONE:
        /* Check if we have to send anything... */
        return handle_list_done(c);

    default:
        UNK_CPD(type, c->version, (uint8_t*)pkt);
        return -3;
    }

    return 0;
}