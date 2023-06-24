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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <WinSock_Defines.h>

#include <debug.h>
#include <f_logs.h>
#include <encryption.h>
#include <mtwist.h>

#include "auth.h"
#include "auth_packets.h"

/* Storage for our client list. */
struct client_queue clients = TAILQ_HEAD_INITIALIZER(clients);

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

/* Check if an IP has been IP banned from the server. */
int is_ip_banned(login_client_t* c, time_t* until, char* reason) {
    char query[256];
    void* result;
    char** row;
    int rv = 0;
    struct sockaddr_in* addr = (struct sockaddr_in*)&c->ip_addr;

    /* XXXX: Need IPv6 bans too! */
    if (c->is_ipv6) {
        return 0;
    }

    /* Fill in the query. */
    sprintf(query, "SELECT enddate, reason FROM %s NATURAL JOIN %s "
        "WHERE addr = '%u' AND enddate >= UNIX_TIMESTAMP() "
        "AND startdate <= UNIX_TIMESTAMP()", AUTH_BANS_IP, AUTH_BANS,
        (unsigned int)ntohl(addr->sin_addr.s_addr));

    /* If we can't query the database, fail. */
    if (psocn_db_real_query(&conn, query)) {
        return -1;
    }

    /* Grab the results. */
    result = psocn_db_result_store(&conn);

    /* If there is a result, then the user is banned. */
    if ((row = psocn_db_result_fetch(result))) {
        rv = 1;
        *until = (time_t)strtoul(row[0], NULL, 0);
        strcpy(reason, row[1]);
    }

    psocn_db_result_free(result);
    return rv;
}

/* Check if a user is banned by guildcard. */
int is_gc_banned(uint32_t gc, time_t* until, char* reason) {
    char query[256];
    void* result;
    char** row;
    int rv = 0;

    /* Fill in the query. */
    sprintf(query, "SELECT enddate, reason FROM %s "
        "NATURAL JOIN %s WHERE guildcard = '%u' AND "
        "enddate >= UNIX_TIMESTAMP() AND "
        "startdate <= UNIX_TIMESTAMP()", AUTH_BANS_GC, AUTH_BANS, (unsigned int)gc);

    /* If we can't query the database, fail. */
    if (psocn_db_real_query(&conn, query)) {
        return -1;
    }

    /* Grab the results. */
    result = psocn_db_result_store(&conn);

    /* If there is a result, then the user is banned. */
    if ((row = psocn_db_result_fetch(result))) {
        rv = 1;
        *until = (time_t)strtoul(row[0], NULL, 0);
        strcpy(reason, row[1]);
    }

    psocn_db_result_free(result);
    return rv;
}

int send_ban_msg(login_client_t* c, time_t until, const char* reason) {
    char string[256];
    //struct tm cooked;
    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);

    /* Create the ban string. */
    sprintf(string, "%s\n%s:\n%s\n\n%s:\n",
        __(c, "\tEYou have been banned from this server."), __(c, "Reason"),
        reason, __(c, "Your ban expires"));

    if ((uint32_t)until == 0xFFFFFFFF) {
        strcat(string, __(c, "Never"));
    }
    else {
        //gmtime_r(&until, &cooked);
        sprintf(string, "%s%02u:%02u UTC %u.%02u.%02u", string, rawtime.wHour,
            rawtime.wMinute, rawtime.wYear, rawtime.wMonth,
            rawtime.wDay);
    }

    return send_large_msg(c, string);
}

int keycheck(char serial[8], char ak[8]) {
    uint64_t akv;
    int i;

    if (!serial[0] || !serial[1] || !serial[2] || !serial[3] ||
        !serial[4] || !serial[5] || !serial[6] || !serial[7])
        return -1;

    if (!ak[0] || !ak[1] || !ak[2] || !ak[3] ||
        !ak[4] || !ak[5] || !ak[6] || !ak[7])
        return -1;

    akv = (((uint64_t)ak[0]) << 0) | (((uint64_t)ak[1]) << 8) |
        (((uint64_t)ak[2]) << 16) | (((uint64_t)ak[3]) << 24) |
        (((uint64_t)ak[4]) << 32) | (((uint64_t)ak[5]) << 40) |
        (((uint64_t)ak[6]) << 48) | (((uint64_t)ak[7]) << 56);

    for (i = 0; i < 8; ++i) {
        if (!isdigit(serial[i]) && (serial[i] < 'A' || serial[i] > 'F'))
            return -1;

        if (!isalnum(ak[i]))
            return -1;
    }

    if (akv == 0300601403006014030060LLU || akv == 0304611423046114230461LLU ||
        akv == 0310621443106214431062LLU || akv == 0314631463146314631463LLU ||
        akv == 0320641503206415032064LLU || akv == 0324651523246515232465LLU ||
        akv == 0330661543306615433066LLU || akv == 0334671563346715633467LLU ||
        akv == 0340701603407016034070LLU || akv == 0344711623447116234471LLU ||
        akv == 0340671543246414631061LLU || akv == 0304621463206515433470LLU ||
        akv == 0310631503246615634071LLU)
        return -1;

    return 0;
}

/* Create a new connection, storing it in the list of clients. */
login_client_t *create_connection(int sock, int type, struct sockaddr *ip,
                                  socklen_t size, uint16_t port) {
    login_client_t *rv = (login_client_t *)malloc(sizeof(login_client_t));
    uint32_t client_seed_dc, server_seed_dc;
    uint8_t client_seed_bb[48] = { 0 }, server_seed_bb[48] = { 0 };
    int i;

    if(!rv) {
        perror("malloc");
        return NULL;
    }

    memset(rv, 0, sizeof(login_client_t));

    /* 将基础参数存储进客户端的结构. */
    rv->sock = sock;
    rv->type = type;
    rv->port = port;
    memcpy(&rv->ip_addr, ip, size);

    /* Is the user on IPv6? */
    if(ip->sa_family == AF_INET6) {
        rv->is_ipv6 = 1;
    }

    /* Initialize the random number generator. The seed value is the current
       UNIX time, xored with the port (so that each block will use a different
       seed even though they'll probably get the same timestamp). */
    mt19937_init(&rv->rng, (uint32_t)(time(NULL) ^ sock));

    switch(type) {
        case CLIENT_AUTH_DCNTE:
            /* Generate the encryption keys for the client and server. */
            rv->client_key = client_seed_dc = mt19937_genrand_int32(&rv->rng);
            rv->server_key = server_seed_dc = mt19937_genrand_int32(&rv->rng);

            CRYPT_CreateKeys(&rv->server_cipher, &server_seed_dc, CRYPT_PC);
            CRYPT_CreateKeys(&rv->client_cipher, &client_seed_dc, CRYPT_PC);

            /* Send the client the welcome packet, or die trying. */
            if (send_dc_welcome(rv, server_seed_dc, client_seed_dc)) {
                goto err;
            }


            rv->version = CLIENT_AUTH_DCNTE;
            rv->auth = 1;

            break;

        case CLIENT_AUTH_DC:
            /* Generate the encryption keys for the client and server. */
            rv->client_key = client_seed_dc = mt19937_genrand_int32(&rv->rng);
            rv->server_key = server_seed_dc = mt19937_genrand_int32(&rv->rng);

            CRYPT_CreateKeys(&rv->server_cipher, &server_seed_dc, CRYPT_PC);
            CRYPT_CreateKeys(&rv->client_cipher, &client_seed_dc, CRYPT_PC);

            /* Send the client the welcome packet, or die trying. */
            if (send_dc_welcome(rv, server_seed_dc, client_seed_dc)) {
                goto err;
            }

            rv->version = CLIENT_AUTH_DC;
            rv->auth = 1;

            break;

        case CLIENT_AUTH_PC:
            /* Generate the encryption keys for the client and server. */
            rv->client_key = client_seed_dc = mt19937_genrand_int32(&rv->rng);
            rv->server_key = server_seed_dc = mt19937_genrand_int32(&rv->rng);

            CRYPT_CreateKeys(&rv->server_cipher, &server_seed_dc, CRYPT_PC);
            CRYPT_CreateKeys(&rv->client_cipher, &client_seed_dc, CRYPT_PC);

            /* Send the client the welcome packet, or die trying. */
            if(send_dc_welcome(rv, server_seed_dc, client_seed_dc)) {
                goto err;
            }

            rv->version = CLIENT_AUTH_PC;
            rv->auth = 1;

            break;

        case CLIENT_AUTH_GC:
            /* Send a selective redirect packet to get any PSOPC users to
               connect to the right port. We can safely do the rest here either
               way, because PSOPC users should disconnect immediately on getting
               this packet (and connect to port 9300 instead). */
            if(send_selective_redirect(rv)) {
                goto err;
            }

            /* Fall through... */

            /* Generate the encryption keys for the client and server. */
            rv->client_key = client_seed_dc = mt19937_genrand_int32(&rv->rng);
            rv->server_key = server_seed_dc = mt19937_genrand_int32(&rv->rng);

            CRYPT_CreateKeys(&rv->server_cipher, &server_seed_dc,
                CRYPT_GAMECUBE);
            CRYPT_CreateKeys(&rv->client_cipher, &client_seed_dc,
                CRYPT_GAMECUBE);

            /* Send the client the welcome packet, or die trying. */
            if (send_dc_welcome(rv, server_seed_dc, client_seed_dc)) {
                goto err;
            }

            rv->version = CLIENT_AUTH_GC;
            rv->auth = 1;

            break;

        case CLIENT_AUTH_EP3:
            /* Generate the encryption keys for the client and server. */
            rv->client_key = client_seed_dc = mt19937_genrand_int32(&rv->rng);
            rv->server_key = server_seed_dc = mt19937_genrand_int32(&rv->rng);

            CRYPT_CreateKeys(&rv->server_cipher, &server_seed_dc,
                CRYPT_GAMECUBE);
            CRYPT_CreateKeys(&rv->client_cipher, &client_seed_dc,
                CRYPT_GAMECUBE);

            /* Send the client the welcome packet, or die trying. */
            if (send_dc_welcome(rv, server_seed_dc, client_seed_dc)) {
                goto err;
            }

            rv->version = CLIENT_AUTH_EP3;
            rv->auth = 1;

            break;

        case CLIENT_AUTH_XBOX:
            /* Generate the encryption keys for the client and server. */
            rv->client_key = client_seed_dc = mt19937_genrand_int32(&rv->rng);
            rv->server_key = server_seed_dc = mt19937_genrand_int32(&rv->rng);

            CRYPT_CreateKeys(&rv->server_cipher, &server_seed_dc,
                             CRYPT_GAMECUBE);
            CRYPT_CreateKeys(&rv->client_cipher, &client_seed_dc,
                             CRYPT_GAMECUBE);

            /* Send the client the welcome packet, or die trying. */
            if(send_dc_welcome(rv, server_seed_dc, client_seed_dc)) {
                goto err;
            }

            rv->version = CLIENT_AUTH_XBOX;
            rv->auth = 1;

            break;

        case CLIENT_AUTH_BB_LOGIN:
            /* Generate the encryption keys for the client and server. */
            for (i = 0; i < 48; i += 4) {
                client_seed_dc = mt19937_genrand_int32(&rv->rng);
                server_seed_dc = mt19937_genrand_int32(&rv->rng);

                client_seed_bb[i + 0] = (uint8_t)(client_seed_dc >> 0);
                client_seed_bb[i + 1] = (uint8_t)(client_seed_dc >> 8);
                client_seed_bb[i + 2] = (uint8_t)(client_seed_dc >> 16);
                client_seed_bb[i + 3] = (uint8_t)(client_seed_dc >> 24);
                server_seed_bb[i + 0] = (uint8_t)(server_seed_dc >> 0);
                server_seed_bb[i + 1] = (uint8_t)(server_seed_dc >> 8);
                server_seed_bb[i + 2] = (uint8_t)(server_seed_dc >> 16);
                server_seed_bb[i + 3] = (uint8_t)(server_seed_dc >> 24);
            }

            CRYPT_CreateKeys(&rv->server_cipher, server_seed_bb,
                CRYPT_BLUEBURST);
            CRYPT_CreateKeys(&rv->client_cipher, client_seed_bb,
                CRYPT_BLUEBURST);

            /* Send the client the welcome packet, or die trying. */
            if (send_bb_welcome(rv, server_seed_bb, client_seed_bb)) {
                goto err;
            }

            rv->version = CLIENT_AUTH_BB_LOGIN;
            rv->auth = 1;

            break;

        case CLIENT_AUTH_BB_CHARACTER:
            /* Generate the encryption keys for the client and server. */
            for(i = 0; i < 48; i += 4) {
                client_seed_dc = mt19937_genrand_int32(&rv->rng);
                server_seed_dc = mt19937_genrand_int32(&rv->rng);

                client_seed_bb[i + 0] = (uint8_t)(client_seed_dc >>  0);
                client_seed_bb[i + 1] = (uint8_t)(client_seed_dc >>  8);
                client_seed_bb[i + 2] = (uint8_t)(client_seed_dc >> 16);
                client_seed_bb[i + 3] = (uint8_t)(client_seed_dc >> 24);
                server_seed_bb[i + 0] = (uint8_t)(server_seed_dc >>  0);
                server_seed_bb[i + 1] = (uint8_t)(server_seed_dc >>  8);
                server_seed_bb[i + 2] = (uint8_t)(server_seed_dc >> 16);
                server_seed_bb[i + 3] = (uint8_t)(server_seed_dc >> 24);
            }

            CRYPT_CreateKeys(&rv->server_cipher, server_seed_bb,
                             CRYPT_BLUEBURST);
            CRYPT_CreateKeys(&rv->client_cipher, client_seed_bb,
                             CRYPT_BLUEBURST);

            /* Send the client the welcome packet, or die trying. */
            if(send_bb_welcome(rv, server_seed_bb, client_seed_bb)) {
                goto err;
            }

            rv->version = CLIENT_AUTH_BB_CHARACTER;
            rv->auth = 1;

            break;
    }

    /* Insert it at the end of our list, and we're done. */
    TAILQ_INSERT_TAIL(&clients, rv, qentry);
    return rv;

err:
    closesocket(sock);
    free(rv);
    return NULL;
}

/* Destroy a connection, closing the socket and removing it from the list. */
void destroy_connection(login_client_t *c) {
    TAILQ_REMOVE(&clients, c, qentry);

    if(c->gc_data) {
        free(c->gc_data);
    }

    if(c->sock >= 0) {
        closesocket(c->sock);
    }

    if(c->recvbuf) {
        free(c->recvbuf);
    }

    if(c->sendbuf) {
        free(c->sendbuf);
    }

    free(c);
}

/* Read data from a client that is connected to any port. */
int read_from_client(login_client_t *c) {
    ssize_t sz;
    int pkt_sz = c->pkt_sz, pkt_cur = c->pkt_cur, rv;
    pkt_header_t tmp_hdr = { 0 };
    dc_pkt_hdr_t dc;
    const int hs = client_type[c->type].hdr_size, hsm = 0x10000 - hs;

    if(!c->recvbuf) {
        /* Read in a new header... */
        sz = recv(c->sock, (char*)&tmp_hdr, hs, 0);

        if(sz < hs) {
            /* If we have an error, disconnect the client */
            if(sz <= 0) {
                //if(sz == SOCKET_ERROR)
                    //ERR_LOG("客户端接收数据错误: %s", strerror(errno));
                return -1;
            }

            /* Otherwise, its just not all there yet, so punt for now... */
            if(!(c->recvbuf = (unsigned char *)malloc(hs))) {
                ERR_LOG("malloc: %s", strerror(errno));
                return -1;
            }

            /* Copy over what we did get */
            memcpy(c->recvbuf, &tmp_hdr, sz);
            c->pkt_cur = sz;
            return 0;
        }
    }
    /* This case should be exceedingly rare... */
    else if(!pkt_sz) {
        /* Try to finish reading the header */
        sz = recv(c->sock, c->recvbuf + pkt_cur, hs - pkt_cur, 0);

        if(sz < hs - pkt_cur) {
            /* If we have an error, disconnect the client */
            if(sz <= 0) {
                //if(sz == SOCKET_ERROR)
                    //ERR_LOG("客户端接收数据错误: %s", strerror(errno));
                return -1;
            }

            /* Update the pointer... */
            c->pkt_cur += sz;
            return 0;
        }

        /* We now have the whole header, so ready things for that */
        memcpy(&tmp_hdr, c->recvbuf, hs);
        c->pkt_cur = 0;
        free(c->recvbuf);
    }

    /* If we haven't decrypted the packet header, do so now, since we definitely
       have the whole thing at this point. */
    if(!pkt_sz) {
        /* If the client says its DC, make sure it actually is, since it could
           be a PSOGC client using the EU version. */
        if((c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_DCNTE) &&
           !c->got_first) {
            dc = tmp_hdr.dc;
            CRYPT_CryptData(&c->client_cipher, &dc, 4, 0);

            /* Check if its one of the three packets we're expecting (0x90 for
               v1, 0x9A for v2, or 0x88 for NTE). Hopefully there's no way to
               get these particular combinations with the GC encryption... */
            if(dc.pkt_type == 0x90 && dc.flags == 0 &&
               (LE16(dc.pkt_len) == 0x0028 || LE16(dc.pkt_len) == 0x0026)) {
                c->got_first = 1;
                tmp_hdr.dc = dc;
            }
            else if(dc.pkt_type == 0x9A && dc.flags == 0 &&
                    LE16(dc.pkt_len) == 0x00E0) {
                c->got_first = 1;
                tmp_hdr.dc = dc;
            }
            else if(dc.pkt_type == 0x88 && dc.flags == 0 &&
                    LE16(dc.pkt_len) == 0x0026) {
                c->got_first = 1;
                tmp_hdr.dc = dc;
            }
            /* If we end up in here, its pretty much gotta be a Gamecube client,
               or someone messing with us. */
            else {
                c->type = CLIENT_AUTH_GC;
                CRYPT_CreateKeys(&c->client_cipher, &c->client_key,
                                 CRYPT_GAMECUBE);
                CRYPT_CreateKeys(&c->server_cipher, &c->server_key,
                                 CRYPT_GAMECUBE);
                CRYPT_CryptData(&c->client_cipher, &tmp_hdr, hs, 0);
            }
        }
        else {
            CRYPT_CryptData(&c->client_cipher, &tmp_hdr, hs, 0);
        }

        switch(c->type) {
            case CLIENT_AUTH_DCNTE:
            case CLIENT_AUTH_DC:
            case CLIENT_AUTH_GC:
            case CLIENT_AUTH_EP3:
            case CLIENT_AUTH_XBOX:
                pkt_sz = LE16(tmp_hdr.dc.pkt_len);
                break;

            case CLIENT_AUTH_PC:
                pkt_sz = LE16(tmp_hdr.pc.pkt_len);
                break;

            case CLIENT_AUTH_BB_LOGIN:
            case CLIENT_AUTH_BB_CHARACTER:
                pkt_sz = LE16(tmp_hdr.bb.pkt_len);
                break;
        }

        sz = (pkt_sz & (hs - 1)) ? (pkt_sz & hsm) + hs : pkt_sz;

        /* Allocate space for the packet */
        if(!(c->recvbuf = (unsigned char *)malloc(sz)))  {
            ERR_LOG("malloc: %s", strerror(errno));
            return -1;
        }

        /* Bah, stupid buggy versions of PSO handling this case in two very
           different ways... When JPv1 sends a packet with a size not divisible
           by the encryption word-size, it expects the server to pad the packet.
           When Blue Burst does it, it sends padding itself. God only knows what
           the other versions would do (but thankfully, they don't appear to do
           any of that broken behavior, at least not that I've seen). The DC
           NTE also has the same bug as JPv1, as expected. */
        if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_DCNTE)
            c->pkt_sz = pkt_sz;
        else
            c->pkt_sz = sz;

        memcpy(c->recvbuf, &tmp_hdr, hs);
        c->pkt_cur = hs;

        /* If this packet is only a header, short-circuit and process it now. */
        if(pkt_sz == hs)
            goto process;

        /* Return now, so we don't end up sleeping in the recv below. */
        return 0;
    }

    /* See if the rest of the packet is here... */
    if((sz = recv(c->sock, c->recvbuf + pkt_cur, pkt_sz - pkt_cur,
                  0)) < pkt_sz - pkt_cur) {
        if(sz <= 0) {
            //if(sz == SOCKET_ERROR)
                //ERR_LOG("客户端接收数据错误: %s", strerror(errno));
            return -1;
        }

        /* Didn't get it all, return for now... */
        c->pkt_cur += sz;
        return 0;
    }

    /* If we get this far, we've got the whole packet, so process it. */
    CRYPT_CryptData(&c->client_cipher, c->recvbuf + hs, pkt_sz - hs, 0);

process:
    /* Pass it onto the correct handler. */
    switch(c->type) {
        case CLIENT_AUTH_DCNTE:
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_PC:
        case CLIENT_AUTH_GC:
        case CLIENT_AUTH_EP3:
        case CLIENT_AUTH_XBOX:
            rv = process_dclogin_packet(c, c->recvbuf);
            break;

        case CLIENT_AUTH_BB_LOGIN:
            rv = process_bblogin_packet(c, c->recvbuf);
            break;

        case CLIENT_AUTH_BB_CHARACTER:
            rv = process_bbcharacter_packet(c, c->recvbuf);
            break;

        default:
            /* This should never happen... */
            rv = -1;
    }

    free(c->recvbuf);
    c->recvbuf = NULL;
    c->pkt_cur = c->pkt_sz = 0;
    return rv;
}

int ship_transfer(login_client_t* c, uint32_t shipid) {
    char query[256];
    void* result;
    char** row;
    in_addr_t ip;
    uint16_t port;
#ifdef ENABLE_IPV6
    uint64_t ip6_hi, ip6_lo;
    uint8_t ip6[16];
#endif
    //ERR_LOG("ship_transfer");

    /* Query the database for the ship in question */
    sprintf(query, "SELECT ip, port, ship_ip6_high, ship_ip6_low FROM "
        "%s WHERE ship_id='%lu'", SERVER_SHIPS_ONLINE, (unsigned long)shipid);

    if (psocn_db_real_query(&conn, query))
        return -1;

    if (!(result = psocn_db_result_store(&conn)))
        return -2;

    if (!(row = psocn_db_result_fetch(result)))
        return -3;

    /* Grab the data from the row */
    if (c->type < CLIENT_AUTH_BB_CHARACTER)
        port = (uint16_t)strtoul(row[1], NULL, 0) + c->type;
    else if (c->type == CLIENT_AUTH_DCNTE)
        port = (uint16_t)strtoul(row[1], NULL, 0);
    else if (c->type == CLIENT_AUTH_XBOX)
        port = (uint16_t)strtoul(row[1], NULL, 0) + 5;
    else
        port = (uint16_t)strtoul(row[1], NULL, 0) + 4;

#ifdef ENABLE_IPV6
    if (row[2] && row[3]) {
        ip6_hi = (uint64_t)strtoull(row[2], NULL, 0);
        ip6_lo = (uint64_t)strtoull(row[3], NULL, 0);
    }

    if (!c->is_ipv6 || !row[2] || !row[3] || !ip6_hi) {
#endif
        ip = htonl((in_addr_t)strtoul(row[0], NULL, 0));

        return send_redirect(c, ip, port);
#ifdef ENABLE_IPV6
    }
    else {
        ip6[0] = (uint8_t)(ip6_hi >> 56);
        ip6[1] = (uint8_t)(ip6_hi >> 48);
        ip6[2] = (uint8_t)(ip6_hi >> 40);
        ip6[3] = (uint8_t)(ip6_hi >> 32);
        ip6[4] = (uint8_t)(ip6_hi >> 24);
        ip6[5] = (uint8_t)(ip6_hi >> 16);
        ip6[6] = (uint8_t)(ip6_hi >> 8);
        ip6[7] = (uint8_t)(ip6_hi);
        ip6[8] = (uint8_t)(ip6_lo >> 56);
        ip6[9] = (uint8_t)(ip6_lo >> 48);
        ip6[10] = (uint8_t)(ip6_lo >> 40);
        ip6[11] = (uint8_t)(ip6_lo >> 32);
        ip6[12] = (uint8_t)(ip6_lo >> 24);
        ip6[13] = (uint8_t)(ip6_lo >> 16);
        ip6[14] = (uint8_t)(ip6_lo >> 8);
        ip6[15] = (uint8_t)(ip6_lo);

        return send_redirect6(c, ip6, port);
    }
#endif
}

/* Initialize mini18n support. */
void init_i18n(void) {
#ifdef HAVE_LIBMINI18N
    int i;
    char filename[256];

    for (i = 0; i < CLIENT_LANG_ALL; ++i) {
        langs[i] = mini18n_create();

        if (langs[i]) {
            sprintf(filename, "System\\Lang\\auth_server-%s.yts", language_codes[i]);

            /* Attempt to load the l10n file. */
            if (mini18n_load(langs[i], filename)) {
                /* If we didn't get it, clean up. */
                mini18n_destroy(langs[i]);
                langs[i] = NULL;
            }
            else {
                AUTH_LOG("读取 l10n 多语言文件 %s", language_codes[i]);
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
    for (i = 0; i < CLIENT_LANG_ALL; ++i) {
        mini18n_destroy(langs[i]);
    }
#endif
}
