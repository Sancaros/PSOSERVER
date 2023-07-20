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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <WinSock_Defines.h>

#include <debug.h>
#include <f_logs.h>
#include <database.h>
#include <quest.h>
#include <md5.h>
#include <items.h>

#include <pso_cmd_code.h>

#include "auth.h"
#include <pso_player.h>
#include "auth_packets.h"

#ifdef HAVE_LIBMINI18N
mini18n_t langs[CLIENT_LANG_ALL];
#endif

extern psocn_dbconn_t conn;
extern psocn_quest_list_t qlist[CLIENT_AUTH_VERSION_COUNT][CLIENT_LANG_ALL];
extern psocn_limits_t *limits;
extern volatile sig_atomic_t shutting_down;
static uint32_t next_pcnte_gc = 500;
const void *my_ntop(struct sockaddr_storage *addr, char str[INET6_ADDRSTRLEN]);

/* Handle a client's login request packet. */
static int handle_ntelogin(login_client_t *c, dcnte_login_88_pkt *pkt) {
    char query[256], serial[64], access[64];
    void *result;
    char **row;
    time_t banlen;
    int banned = is_ip_banned(c, &banlen, query), resp = LOGIN_88_NEW_USER;

    /* Make sure the user isn't IP banned. */
    if(banned == -1) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }
    else if(banned) {
        send_ban_msg(c, banlen, query);
        return -1;
    }

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, serial, pkt->serial_number, 16);
    psocn_db_escape_str(&conn, access, pkt->access_key, 16);

    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "serial_number='%s' AND access_key='%s'"
        , AUTH_ACCOUNT_DREAMCAST_NTE
        , serial, access);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if((row = psocn_db_result_fetch(result))) {
        /* We have seen this client before, set the response code properly. */
        resp = LOGIN_88_OK;
        psocn_db_result_free(result);
    }
    else {
        psocn_db_result_free(result);
        memcpy(c->serial_number, pkt->serial_number, 16);
        memcpy(c->access_key, pkt->access_key, 16);
    }

    c->type = CLIENT_AUTH_DCNTE;
    c->ext_version = CLIENT_EXTVER_DC | CLIENT_EXTVER_DCNTE;

    /* We should check to see if the client exists here, and get it to send us
       an 0x8a if it doesn't. Otherwise, we should have it send an 0x8b. */
    return send_simple(c, LOGIN_88_TYPE, resp);
}

static int handle_ntelogin8a(login_client_t *c, dcnte_login_8a_pkt *pkt) {
    uint32_t gc;
    char query[256], dc_id[32], serial[64], access[64];
    void *result;
    char **row;
    int banned;
    time_t banlen;

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, dc_id, pkt->dc_id, 8);
    psocn_db_escape_str(&conn, serial, c->serial_number, 16);
    psocn_db_escape_str(&conn, access, c->access_key, 16);
    
    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "(dc_id='%s' OR dc_id IS NULL) AND serial_number='%s' AND "
        "access_key='%s'"
        , AUTH_ACCOUNT_DREAMCAST_NTE
        , dc_id, serial
        , access);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if((row = psocn_db_result_fetch(result))) {
        /* We have seen this client before, save their guildcard for use. */
        gc = (uint32_t)strtoul(row[0], NULL, 0);
        psocn_db_result_free(result);
    }
    else {
        psocn_db_result_free(result);

        /* Assign a nice fresh new guildcard number to the client. */
        sprintf(query, "INSERT INTO %s (account_id) VALUES (NULL)", AUTH_ACCOUNT_GUILDCARDS);

        if(psocn_db_real_query(&conn, query)) {
            send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                                 "请稍后再尝试."));
            return -1;
        }

        /* Grab the new guildcard for the user. */
        gc = (uint32_t)psocn_db_insert_id(&conn);

        /* Add the client into our database. */
        sprintf(query, "INSERT INTO dreamcast_nte_clients (guildcard, "
                "serial_number, access_key, dc_id) VALUES ('%u', '%s', '%s', "
                "'%s')", gc, serial, access, dc_id);

        if(psocn_db_real_query(&conn, query)) {
            send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                                 "请稍后再尝试."));
            return -1;
        }
    }

    /* Make sure the guildcard isn't banned. */
    banned = is_gc_banned(gc, &banlen, query);

    if(banned == -1) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }
    else if(banned) {
        send_ban_msg(c, banlen, query);
        return -1;
    }

    /* Make sure the guildcard isn't online already. */
    banned = db_check_gc_online(gc);

    if(banned == -1) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }
    else if(banned) {
        send_large_msg(c, __(c, "\tE您的账户已在线."));
        return -1;
    }

    /* Check if the user is a GM or not. */
    sprintf(query, "SELECT privlevel FROM %s NATURAL JOIN %s "
            "WHERE guildcard='%u'", AUTH_ACCOUNT, AUTH_ACCOUNT_GUILDCARDS, gc);

    if(psocn_db_real_query(&conn, query)) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if(result) {
        if((row = psocn_db_result_fetch(result))) {
            c->priv = strtoul(row[0], NULL, 0);
        }

        psocn_db_result_free(result);
    }

    /* Force the client to send us an 0x8B to finish the login. */
    return send_simple(c, LOGIN_8A_TYPE, LOGIN_8A_REGISTER_OK);
}

static int handle_ntelogin8b(login_client_t *c, dcnte_login_8b_pkt *pkt) {
    uint32_t gc;
    char query[256], dc_id[32], serial[64], access[64];
    void *result;
    char **row;
    int banned;
    time_t banlen;

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, dc_id, (char *)pkt->dc_id, 8);
    psocn_db_escape_str(&conn, serial, pkt->serial, 16);
    psocn_db_escape_str(&conn, access, pkt->access_key, 16);

    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "(dc_id='%s' OR dc_id IS NULL) AND serial_number='%s' AND "
        "access_key='%s'"
        , AUTH_ACCOUNT_DREAMCAST_NTE
        , dc_id, serial
        , access);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if((row = psocn_db_result_fetch(result))) {
        /* We have seen this client before, save their guildcard for use. */
        gc = (uint32_t)strtoul(row[0], NULL, 0);
        psocn_db_result_free(result);
    }
    else {
        psocn_db_result_free(result);
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }

    /* Make sure the guildcard isn't banned. */
    banned = is_gc_banned(gc, &banlen, query);

    if(banned == -1) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }
    else if(banned) {
        send_ban_msg(c, banlen, query);
        return -1;
    }

    /* Make sure the guildcard isn't online already. */
    banned = db_check_gc_online(gc);

    if(banned == -1) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }
    else if(banned) {
        send_large_msg(c, __(c, "\tE您的账户已在线."));
        return -1;
    }

    /* Check if the user is a GM or not. */
    sprintf(query, "SELECT privlevel FROM %s NATURAL JOIN %s "
            "WHERE guildcard='%u'", AUTH_ACCOUNT, AUTH_ACCOUNT_GUILDCARDS, gc);

    if(psocn_db_real_query(&conn, query)) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if(result) {
        if((row = psocn_db_result_fetch(result))) {
            c->priv = strtoul(row[0], NULL, 0);
        }

        psocn_db_result_free(result);
    }

    c->guildcard = gc;
    send_dc_security(c, gc, NULL, 0);
    return send_ship_list(c, 0);
}

static int handle_login0(login_client_t *c, dc_login_90_pkt *pkt) {
    char query[256],  serial[32], access[32];
    void *result;
    char **row;
    uint8_t resp = LOGIN_90_OK;
    time_t banlen;
    int banned = is_ip_banned(c, &banlen, query);

    /* Make sure the user isn't IP banned. */
    if(banned == -1) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                       "请稍后再尝试."));
        return -1;
    }
    else if(banned) {
        send_ban_msg(c, banlen, query);
        return -1;
    }

    if(keycheck(pkt->serial_number, pkt->access_key)) {
        send_large_msg(c, __(c, "\tECannot connect to server.\n"
                       "Please check your settings."));
        return -1;
    }

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, serial, pkt->serial_number, 8);
    psocn_db_escape_str(&conn, access, pkt->access_key, 8);

    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "serial_number='%s' AND access_key='%s'"
        , AUTH_ACCOUNT_DREAMCAST
        , serial, access);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                       "请稍后再尝试."));
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if(!(row = psocn_db_result_fetch(result))) {
        /* We've not seen this client before, get them to send us a 0x92. */
        resp = LOGIN_90_NEW_USER;
    }

    psocn_db_result_free(result);

    c->version = PSOCN_QUEST_V1;
    c->ext_version = CLIENT_EXTVER_DC | CLIENT_EXTVER_DCV1;

    return send_simple(c, LOGIN_90_TYPE, resp);
}

static int handle_login3(login_client_t *c, dc_login_93_pkt *pkt) {
    uint32_t gc;
    char query[256], dc_id[32], serial[32], access[32];
    void *result;
    char **row;
    int banned;
    time_t banlen;

    c->language_code = pkt->language_code;

    if(keycheck(pkt->serial_number, pkt->access_key)) {
        send_large_msg(c, __(c, "\tECannot connect to server.\n"
                       "Please check your settings."));
        return -1;
    }

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, dc_id, pkt->dc_id, 8);
    psocn_db_escape_str(&conn, serial, pkt->serial_number, 8);
    psocn_db_escape_str(&conn, access, pkt->access_key, 8);

    sprintf(query, "SELECT guildcard FROM %s WHERE (dc_id='%s' "
        "OR dc_id IS NULL) AND serial_number='%s' AND access_key='%s'"
        , AUTH_ACCOUNT_DREAMCAST, dc_id
        , serial, access);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                       "请稍后再尝试."));
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if((row = psocn_db_result_fetch(result))) {
        /* We have seen this client before, save their guildcard for use. */
        gc = (uint32_t)strtoul(row[0], NULL, 0);
        psocn_db_result_free(result);
    }
    else {
        psocn_db_result_free(result);

        /* Assign a nice fresh new guildcard number to the client. */
        sprintf(query, "INSERT INTO %s (account_id) VALUES (NULL)", AUTH_ACCOUNT_GUILDCARDS);

        if(psocn_db_real_query(&conn, query)) {
            send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                           "请稍后再尝试."));
            return -1;
        }

        /* Grab the new guildcard for the user. */
        gc = (uint32_t)psocn_db_insert_id(&conn);

        /* Add the client into our database. */
        sprintf(query, "INSERT INTO %s (guildcard, "
                "serial_number, access_key, dc_id) VALUES ('%u', '%s', '%s', "
                "'%s')", AUTH_ACCOUNT_DREAMCAST, gc, serial, access, dc_id);

        if(psocn_db_real_query(&conn, query)) {
            send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                           "请稍后再尝试."));
            return -1;
        }
    }

    /* Make sure the guildcard isn't banned. */
    banned = is_gc_banned(gc, &banlen, query);

    if(banned == -1) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                       "请稍后再尝试."));
        return -1;
    }
    else if(banned) {
        send_ban_msg(c, banlen, query);
        return -1;
    }

    /* Make sure the guildcard isn't online already. */
    banned = db_check_gc_online(gc);

    if(banned == -1) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                             "请稍后再尝试."));
        return -1;
    }
    else if(banned) {
        send_large_msg(c, __(c, "\tE您的账户已在线."));
        return -1;
    }

    /* Check if the user is a GM or not. */
    sprintf(query, "SELECT privlevel FROM %s NATURAL JOIN %s "
            "WHERE guildcard='%u'", AUTH_ACCOUNT, AUTH_ACCOUNT_GUILDCARDS, gc);

    if(psocn_db_real_query(&conn, query)) {
        send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                       "请稍后再尝试."));
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if(result) {
        if((row = psocn_db_result_fetch(result))) {
            c->priv = strtoul(row[0], NULL, 0);
        }

        psocn_db_result_free(result);
    }

    c->guildcard = gc;
    return send_dc_security(c, gc, NULL, 0);
}

static int is_pctrial(dcv2_login_9a_pkt *pkt) {
    int i = 0;

    for(i = 0; i < 8; ++i) {
        if(pkt->serial_number[i] || pkt->access_key[i])
            return 0;
    }

    return 1;
}

/* Handle a client's login request packet (yes, this function is the same as the
   one above, but it uses a different structure). */
static int handle_logina(login_client_t *c, dcv2_login_9a_pkt *pkt) {
    uint32_t gc;
    char query[256], dc_id[32], serial[32], access[32];
    void *result;
    char **row;
    time_t banlen;
    int banned = is_ip_banned(c, &banlen, query);

    //display_packet((unsigned char*)pkt, LE16(pkt->hdr.pc.pkt_len));

    /* Make sure the user isn't IP banned. */
    if(banned == -1) {
        return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_ERROR);
    }
    else if(banned) {
        send_ban_msg(c, banlen, query);
        return -1;
    }

    if(c->type != CLIENT_AUTH_PC && keycheck(pkt->serial_number, pkt->access_key)) {
        send_large_msg(c, __(c, "\tECannot connect to server.\n"
                       "Please check your settings."));
        return -1;
    }

    c->version = PSOCN_QUEST_V2;

    if(!is_pctrial(pkt)) {
        /* Escape all the important strings. */
        psocn_db_escape_str(&conn, dc_id, pkt->dc_id, 8);
        psocn_db_escape_str(&conn, serial, pkt->serial_number, 8);
        psocn_db_escape_str(&conn, access, pkt->access_key, 8);

        if(c->type != CLIENT_AUTH_PC) {
            sprintf(query, "SELECT guildcard FROM %s WHERE "
                "(dc_id='%s' OR dc_id IS NULL) AND serial_number='%s' AND "
                "access_key='%s'"
                , AUTH_ACCOUNT_DREAMCAST
                , dc_id, serial
                , access);
            c->ext_version = CLIENT_EXTVER_DC | CLIENT_EXTVER_DCV2;
        }
        else {
            sprintf(query, "SELECT guildcard FROM %s WHERE "
                "serial_number='%s' AND access_key='%s'"
                , AUTH_ACCOUNT_PC
                , serial, access);
            c->ext_version = CLIENT_EXTVER_PC;
        }

        /* If we can't query the database, fail. */
        if(psocn_db_real_query(&conn, query)) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_ERROR);
        }

        result = psocn_db_result_store(&conn);

        if((row = psocn_db_result_fetch(result))) {
            /* We have seen this client before, save their guildcard for use. */
            gc = (uint32_t)strtoul(row[0], NULL, 0);
            psocn_db_result_free(result);
        }
        else if(c->type == CLIENT_AUTH_PC) {
            /* If we're here, then that means either the PSOPC user is not
               registered or they've put their information in wrong. Disconnect
               them so that they can fix that problem
               如果我们在这里，那么这意味着PSOPC用户不是
               注册，或者他们把他们的信息放错了。断开
               这样他们就可以解决这个问题. */
            psocn_db_result_free(result);
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_BAD_SERIAL);
        }
        else {
            /* 如果我们到了这里, we have a PSOv2 (DC) user that isn't known to the
               server yet. Give them a nice fresh guildcard. */
            psocn_db_result_free(result);

            /* Assign a nice fresh new guildcard number to the client. */
            sprintf(query, "INSERT INTO %s (account_id) VALUES (NULL)", AUTH_ACCOUNT_GUILDCARDS);

            if(psocn_db_real_query(&conn, query)) {
                return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_ERROR);
            }

            /* Grab the new guildcard for the user. */
            gc = (uint32_t)psocn_db_insert_id(&conn);

            /* Add the client into our database. */
            sprintf(query, "INSERT INTO %s (guildcard, "
                "serial_number, access_key, dc_id) VALUES ('%u', '%s', "
                "'%s', '%s')"
                , AUTH_ACCOUNT_DREAMCAST
                , gc, serial
                , access, dc_id);

            if(psocn_db_real_query(&conn, query)) {
                return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_ERROR);
            }
        }

        /* Make sure the guildcard isn't banned. */
        banned = is_gc_banned(gc, &banlen, query);

        if(banned == -1) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_ERROR);
        }
        else if(banned) {
            send_ban_msg(c, banlen, query);
            return -1;
        }

        /* Make sure the guildcard isn't online already. */
        banned = db_check_gc_online(gc);

        if(banned == -1) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_ERROR);
        }
        else if(banned) {
            send_large_msg(c, __(c, "\tE您的账户已在线."));
            return -1;
        }

        /* Check if the user is a GM or not. */
        sprintf(query, "SELECT privlevel FROM %s NATURAL JOIN "
            "%s WHERE guildcard='%u'"
            , AUTH_ACCOUNT
            , AUTH_ACCOUNT_GUILDCARDS, gc);

        if(psocn_db_real_query(&conn, query)) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_ERROR);
        }

        result = psocn_db_result_store(&conn);

        if(result) {
            if((row = psocn_db_result_fetch(result))) {
                c->priv = strtoul(row[0], NULL, 0);
            }

            psocn_db_result_free(result);
        }

        c->guildcard = gc;
    }
    else {
        /* This is kinda inelegant, but it will mostly work... */
        c->guildcard = next_pcnte_gc++;

        /* Reset if we've reached the end of the list. */
        if(next_pcnte_gc == 600)
            next_pcnte_gc = 500;

        c->ext_version = CLIENT_EXTVER_PC | CLIENT_EXTVER_PCNTE;
    }

    /* Force them to send us a 0x9D so we have their language code, since this
       packet doesn't have it. */
    return send_simple(c, LOGIN_9A_TYPE, LOGIN_9A_OK2);
}

/* The next few functions look the same pretty much... All added for gamecube
   support. */
static int handle_gchlcheck(login_client_t *c, v3_hlcheck_pkt *pkt) {
    uint32_t account, gc;
    char query[256], serial[32], access[32];
    void *result;
    char **row;
    time_t banlen;
    int banned = is_ip_banned(c, &banlen, query);

    c->ext_version = CLIENT_EXTVER_GC;

    /* Check the version code of the connecting client since some clients seem
       to want to connect on wonky ports... */
    switch(pkt->sub_version) {
        case 0x30: /* Episode 1 & 2 (Japanese, v1.02) */
            c->type = CLIENT_AUTH_GC;
            c->ext_version |= CLIENT_EXTVER_GC_EP12 | CLIENT_EXTVER_GC_REG_JP;
            break;

        case 0x31: /* Episode 1 & 2 (US 1.0/1.01) */
            c->type = CLIENT_AUTH_GC;
            c->ext_version |= CLIENT_EXTVER_GC_EP12 | CLIENT_EXTVER_GC_REG_US;
            break;

        case 0x32: /* Episode 1 & 2 (Europe, 50hz) */
            c->type = CLIENT_AUTH_GC;
            c->ext_version |= CLIENT_EXTVER_GC_EP12 |
                CLIENT_EXTVER_GC_REG_PAL50;
            break;

        case 0x33: /* Episode 1 & 2 (Europe, 60hz) */
            c->type = CLIENT_AUTH_GC;
            c->ext_version |= CLIENT_EXTVER_GC_EP12 |
                CLIENT_EXTVER_GC_REG_PAL60;
            break;

        case 0x34: /* Episode 1 & 2 (Japan, v1.03) */
            c->type = CLIENT_AUTH_GC;
            c->ext_version |= CLIENT_EXTVER_GC_EP12 | CLIENT_EXTVER_GC_REG_JP;
            break;

        case 0x36: /* Episode 1 & 2 Plus (US) */
            c->type = CLIENT_AUTH_GC;
            c->ext_version |= CLIENT_EXTVER_GC_EP12PLUS |
                CLIENT_EXTVER_GC_REG_US;
            break;

        case 0x35:
        case 0x39: /* Episode 1 & 2 Plus (Japan) */
            c->type = CLIENT_AUTH_GC;
            c->ext_version |= CLIENT_EXTVER_GC_EP12PLUS |
                CLIENT_EXTVER_GC_REG_JP;
            break;

        case 0x40: /* Episode 3 (trial?) */
            c->type = CLIENT_AUTH_EP3;
            c->ext_version |= CLIENT_EXTVER_GC_EP3 | CLIENT_EXTVER_GC_REG_JP;
            break;

        case 0x41: /* Episode 3 (US) */
            c->type = CLIENT_AUTH_EP3;
            c->ext_version |= CLIENT_EXTVER_GC_EP3 | CLIENT_EXTVER_GC_REG_US;
            break;

        case 0x42: /* Episode 3 (Japanese) */
            c->type = CLIENT_AUTH_EP3;
            c->ext_version |= CLIENT_EXTVER_GC_EP3 | CLIENT_EXTVER_GC_REG_JP;
            break;

        case 0x43: /* Episode 3 (Europe) */
            c->type = CLIENT_AUTH_EP3;
            c->ext_version |= CLIENT_EXTVER_GC_EP3 | CLIENT_EXTVER_GC_REG_PAL60;
            break;

        default:
            AUTH_LOG("未知版本代码: %02x", pkt->sub_version);
            c->type = CLIENT_AUTH_GC;
    }

    /* Save the raw version code in the extended version field too... */
    c->ext_version |= (pkt->sub_version << 8);

    /* Make sure the user isn't IP banned. */
    if(banned == -1) {
        return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
    }
    else if(banned) {
        send_ban_msg(c, banlen, query);
        return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_SUSPENDED);
    }

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, serial, pkt->serial_number1, 8);
    psocn_db_escape_str(&conn, access, pkt->access_key1, 12);

    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "serial_number='%s' AND access_key='%s'"
        , AUTH_ACCOUNT_GAMECUBE
        , serial, access);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
    }

    result = psocn_db_result_store(&conn);

    if((row = psocn_db_result_fetch(result))) {
        gc = (uint32_t)strtoul(row[0], NULL, 0);
        psocn_db_result_free(result);

        /* Make sure the guildcard isn't banned. */
        banned = is_gc_banned(gc, &banlen, query);

        if(banned == -1) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }
        else if(banned) {
            send_ban_msg(c, banlen, query);
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_SUSPENDED);
        }

        /* Make sure the guildcard isn't online already. */
        banned = db_check_gc_online(gc);

        if(banned == -1) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }
        else if(banned) {
            send_large_msg(c, __(c, "\tE您的账户已在线."));
            return -1;
        }

        /* The client has at least registered, check the password...
           We need the account to do that though. */
        sprintf(query, "SELECT account_id FROM %s WHERE guildcard='%u'"
            , AUTH_ACCOUNT_GUILDCARDS, gc);

        if(psocn_db_real_query(&conn, query)) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }

        result = psocn_db_result_store(&conn);

        if(!(row = psocn_db_result_fetch(result))) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }

        account = (uint32_t)strtoul(row[0], NULL, 0);
        psocn_db_result_free(result);

        sprintf(query, "SELECT privlevel FROM %s WHERE "
            "account_id='%u'"
            , AUTH_ACCOUNT
            , account);

        /* If we can't query the DB, fail. */
        if(psocn_db_real_query(&conn, query)) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }

        result = psocn_db_result_store(&conn);

        if((row = psocn_db_result_fetch(result))) {
            c->priv = strtoul(row[0], NULL, 0);
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_OK);
        }
    }

    AUTH_LOG("版本代码: %02x %s %s", pkt->sub_version, serial, access);

    psocn_db_result_free(result);

    /* 如果我们到了这里, we didn't find them, bail out. */
    return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_NO_HL);
}

static int handle_gcloginc(login_client_t *c, gc_login_9c_pkt *pkt) {
    uint32_t account, gc;
    char query[256], serial[32], access[32];
    void *result;
    char **row;
    unsigned char hash[16];
    size_t i;

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, serial, pkt->serial, 8);
    psocn_db_escape_str(&conn, access, pkt->access_key, 12);

    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "serial_number='%s' AND access_key='%s'", AUTH_ACCOUNT_GAMECUBE, serial, access);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if((row = psocn_db_result_fetch(result))) {
        /* The client has at least registered, check the password. */
        gc = (uint32_t)strtoul(row[0], NULL, 0);

        psocn_db_result_free(result);

        /* We need the account to do that though... */
        sprintf(query, "SELECT account_id FROM %s WHERE guildcard='%u'", AUTH_ACCOUNT_GUILDCARDS,
            gc);

        if(psocn_db_real_query(&conn, query)) {
            return -1;
        }

        result = psocn_db_result_store(&conn);

        if(!(row = psocn_db_result_fetch(result))) {
            return -1;
        }

        account = (uint32_t)strtoul(row[0], NULL, 0);
        psocn_db_result_free(result);

        sprintf(query, "SELECT password, regtime FROM %s WHERE "
            "account_id='%u'", AUTH_ACCOUNT, account);

        /* If we can't query the DB, fail. */
        if(psocn_db_real_query(&conn, query)) {
            return -1;
        }

        result = psocn_db_result_store(&conn);

        if((row = psocn_db_result_fetch(result))) {
            /* Check the password. */
            sprintf(query, "%s_%s_salt", pkt->password, row[1]);
            md5((unsigned char *)query, strlen(query), hash);

            query[0] = '\0';
            for(i = 0; i < 16; ++i) {
                sprintf(query, "%s%02x", query, hash[i]);
            }

            for(i = 0; i < strlen(row[0]); ++i) {
                row[0][i] = tolower(row[0][i]);
            }

            if(!strcmp(row[0], query)) {
                psocn_db_result_free(result);
                return send_simple(c, LOGIN_9C_TYPE, LOGIN_9CGC_OK);
            }
            else {
                return send_simple(c, LOGIN_9C_TYPE, LOGIN_9CGC_BAD_PWD);
            }
        }
    }

    psocn_db_result_free(result);

    /* 如果我们到了这里, we didn't find them, bail out. */
    return -1;
}

static int handle_gclogine(login_client_t *c, gc_login_9e_pkt *pkt) {
    uint32_t gc, v;
    char query[256], serial[32], access[32];
    void *result;
    char **row;

    c->language_code = pkt->language_code;

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, serial, pkt->serial, 8);
    psocn_db_escape_str(&conn, access, pkt->access_key, 12);

    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "serial_number='%s' AND access_key='%s'"
        , AUTH_ACCOUNT_GAMECUBE
        , serial, access);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if((row = psocn_db_result_fetch(result))) {
        /* Grab the client's guildcard number. */
        gc = (uint32_t)strtoul(row[0], NULL, 0);
        psocn_db_result_free(result);

        /* Only send the version detection packet to non-plus versions of
           the game, as the plus versions will immediately disconnect if
           they receive a packet 0xB2. Note that this means it is probably
           impossible to do runtime patches for PSO Episode I&II Plus. */
        v = c->ext_version & CLIENT_EXTVER_GC_EP_MASK;
        if(c->type == CLIENT_AUTH_GC && v != CLIENT_EXTVER_GC_EP12PLUS)
            send_gc_version_detect(c);

        c->guildcard = gc;
        return send_dc_security(c, gc, NULL, 0);
    }

    psocn_db_result_free(result);

    /* 如果我们到了这里, we didn't find them, bail out. */
    return -1;
}

/* The next few functions look the same pretty much... All added for xbox
   support. */
static int handle_xbhlcheck(login_client_t *c, xb_hlcheck_pkt *pkt) {
    uint32_t gc;
    char query[256], xbluid[32];
    void *result;
    char **row;
    time_t banlen;
    int banned;

    c->ext_version = CLIENT_EXTVER_GC | CLIENT_EXTVER_GC_EP12;

    /* Save the raw version code in the extended version field too... */
    c->ext_version |= (pkt->version << 8);

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, xbluid, (const char *)pkt->xbl_userid, 16);

    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "xbl_userid='%s'"
        , AUTH_ACCOUNT_XBOX
        , xbluid);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
    }

    result = psocn_db_result_store(&conn);

    /* Did we find them? */
    if((row = psocn_db_result_fetch(result))) {
        gc = (uint32_t)strtoul(row[0], NULL, 0);
        psocn_db_result_free(result);

        /* Make sure the guildcard isn't banned. */
        banned = is_gc_banned(gc, &banlen, query);

        if(banned == -1) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }
        else if(banned) {
            send_ban_msg(c, banlen, query);
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_SUSPENDED);
        }

        /* Make sure the guildcard isn't online already. */
        banned = db_check_gc_online(gc);

        if(banned == -1) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }
        else if(banned) {
            send_large_msg(c, __(c, "\tE您的账户已在线."));
            return -1;
        }

        return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_OK);
    }
    else {
        /* They aren't registered yet, so... Let's solve that. */
        psocn_db_result_free(result);

        return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_NEW_USER);
    }
}

static int handle_xbloginc(login_client_t *c, xb_login_9c_pkt *pkt) {
    uint32_t gc;
    char query[256], xbluid[32], xblgt[32];
    void *result;
    char **row;

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, xbluid, (const char *)pkt->xbl_userid, 16);

    sprintf(query, "SELECT guildcard FROM %s WHERE "
        "xbl_userid='%s'"
        , AUTH_ACCOUNT_XBOX
        , xbluid);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        return -1;
    }

    result = psocn_db_result_store(&conn);

    if((row = psocn_db_result_fetch(result))) {
        /* The client registered, so we really shouldn't be here... */
        psocn_db_result_free(result);
        return send_simple(c, LOGIN_9C_TYPE, LOGIN_9CGC_OK);
    }
    else {
        psocn_db_result_free(result);
        psocn_db_escape_str(&conn, xblgt, (const char *)pkt->xbl_gamertag,
                                16);

        /* Assign a nice fresh new guildcard number to the client. */
        sprintf(query, "INSERT INTO %s (account_id) VALUES (NULL)"
            , AUTH_ACCOUNT_GUILDCARDS);

        if(psocn_db_real_query(&conn, query)) {
            send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                                 "请稍后再尝试."));
            return -1;
        }

        /* Grab the new guildcard for the user. */
        gc = (uint32_t)psocn_db_insert_id(&conn);

        /* Add the client into our database. */
        sprintf(query, "INSERT INTO %s (guildcard, xbl_userid,"
            "xbl_gamertag) VALUES ('%u', '%s', '%s')"
            , AUTH_ACCOUNT_XBOX
            , gc, xbluid, xblgt);

        if(psocn_db_real_query(&conn, query)) {
            send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                                 "请稍后再尝试."));
            return -1;
        }

        /* Get them to send us a 9E next. */
        return send_simple(c, LOGIN_9C_TYPE, LOGIN_9CGC_OK);
    }
}

static int handle_xblogine(login_client_t *c, xb_login_9e_pkt *pkt) {
    uint32_t gc;
    char query[256], xbluid[32], xblgt[32];
    void *result;
    char **row;
    xbox_ip_t *ip = (xbox_ip_t *)&pkt->xbl_ip;
    time_t banlen;
    int banned;
    struct sockaddr_in *addr = (struct sockaddr_in *)&c->ip_addr;
    char ipstr[INET6_ADDRSTRLEN];

    /* Check if the user is IP banned. */
    memset(&c->ip_addr, 0, sizeof(struct sockaddr_storage));
    addr->sin_family = PF_INET;
    addr->sin_addr.s_addr = ip->wan_ip;
    addr->sin_port = ip->port;

    banned = is_ip_banned(c, &banlen, query);

    if(banned == -1) {
        return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
    }
    else if(banned) {
        send_ban_msg(c, banlen, query);
        return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_SUSPENDED);
    }

    c->language_code = pkt->language_code;
    c->ext_version = CLIENT_EXTVER_GC | CLIENT_EXTVER_GC_EP12;
    c->ext_version |= (pkt->version << 8);

    /* Escape all the important strings. */
    psocn_db_escape_str(&conn, xbluid, (const char *)pkt->xbl_userid, 16);

    sprintf(query, "SELECT guildcard, privlevel FROM %s NATURAL JOIN "
            "%s NATURAL LEFT OUTER JOIN %s WHERE "
            "xbl_userid='%s'", AUTH_ACCOUNT_XBOX, AUTH_ACCOUNT_GUILDCARDS, AUTH_ACCOUNT, xbluid);

    /* If we can't query the database, fail. */
    if(psocn_db_real_query(&conn, query)) {
        return -1;
    }

    result = psocn_db_result_store(&conn);

    /* Did we find them? */
    if((row = psocn_db_result_fetch(result))) {
        /* Grab the client's guildcard number and their privilege level if one
           is set. */
        gc = (uint32_t)strtoul(row[0], NULL, 0);

        if(row[1]) {
            c->priv = (uint32_t)strtoul(row[1], NULL, 0);
        }

        psocn_db_result_free(result);

        my_ntop(&c->ip_addr, ipstr);
        AUTH_LOG("Xbox GC %" PRIu32 " connected from real IP %s",
              gc, ipstr);

        /* Make sure the guildcard isn't banned. */
        banned = is_gc_banned(gc, &banlen, query);

        if(banned == -1) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }
        else if(banned) {
            send_ban_msg(c, banlen, query);
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_SUSPENDED);
        }

        /* Make sure the guildcard isn't online already. */
        banned = db_check_gc_online(gc);

        if(banned == -1) {
            return send_simple(c, LOGIN_9A_TYPE, LOGIN_DB_CONN_ERROR);
        }
        else if(banned) {
            send_large_msg(c, __(c, "\tE您的账户已在线."));
            return -1;
        }

        c->guildcard = gc;
        send_dc_security(c, gc, NULL, 0);
        return 0;
    }
    else {
        /* Temporary(?) workaround... */
        psocn_db_result_free(result);
        psocn_db_escape_str(&conn, xblgt, (const char *)pkt->xbl_gamertag,
                                16);

        /* Assign a nice fresh new guildcard number to the client. */
        sprintf(query, "INSERT INTO %s (account_id) VALUES (NULL)", AUTH_ACCOUNT_GUILDCARDS);

        if(psocn_db_real_query(&conn, query)) {
            send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                                 "请稍后再尝试."));
            return -1;
        }

        /* Grab the new guildcard for the user. */
        gc = (uint32_t)psocn_db_insert_id(&conn);

        /* Add the client into our database. */
        sprintf(query, "INSERT INTO %s (guildcard, xbl_userid,"
                "xbl_gamertag) VALUES ('%u', '%s', '%s')", AUTH_ACCOUNT_XBOX, gc, xbluid, xblgt);

        if(psocn_db_real_query(&conn, query)) {
            send_large_msg(c, __(c, "\tE因特网服务错误.\n"
                                 "请稍后再尝试."));
            return -1;
        }

        my_ntop(&c->ip_addr, ipstr);
        DBG_LOG("Xbox GC %" PRIu32 " connected from real IP %s\n",
              gc, ipstr);

        c->guildcard = gc;
        send_dc_security(c, gc, NULL, 0);
        return 0;
    }
}

static int handle_logind(login_client_t *c, dcv2_login_9d_pkt *pkt) {
    /* Well, this function used to be here just to fetch the language code, but
       now we actually use more here, so I guess I shouldn't complain too much
       about it anymore... */
    c->language_code = pkt->language_code;

    if(c->type != CLIENT_AUTH_PC && pkt->sub_version != 0x00000030 &&
       keycheck(pkt->serial_number, pkt->access_key)) {
           send_large_msg(c, __(c, "\tECannot connect to server.\n"
                          "Please check your settings."));
           return -1;
    }

    /* The Gamecube Episode I & II trial looks like it's a Dreamcast client up
       until this point. */
    if(c->type == CLIENT_AUTH_DC && pkt->sub_version == 0x00000030)
        c->ext_version = CLIENT_EXTVER_DC | CLIENT_EXTVER_GC_TRIAL;
    else if(c->type == CLIENT_AUTH_DC)
        send_dc_version_detect(c);

    return send_dc_security(c, c->guildcard, NULL, 0);
}

/* Handle a client's ship select packet. */
static int handle_ship_select(login_client_t *c, dc_select_pkt *pkt) {
    psocn_quest_list_t *l = NULL;
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);
    int rv;
    const patchset_t *p;

    /* Don't go out of bounds... */
    if(c->type == CLIENT_AUTH_XBOX)
        l = &qlist[CLIENT_AUTH_XBOX][c->language_code];
    else if(c->type < CLIENT_AUTH_DCNTE)
         l = &qlist[c->type][c->language_code];

    DBG_LOG("menu_idFF 0x%zX menu_id 0x%zX item_id 0x%zX", menu_id & 0xFF, menu_id, item_id);

    switch(menu_id & 0xFF) {
        /* Initial menu */
        case MENU_ID_INITIAL:
            if(item_id == ITEM_ID_INIT_SHIP) {
                /* Ship Select */
                return send_ship_list(c, 0);
            }
            else if(item_id == ITEM_ID_INIT_DOWNLOAD) {
                if (l->cat_count == 1) {
                    return send_quest_list(c, &l->cats[0]);
                }
            }
            else if(item_id == ITEM_ID_INIT_INFO) {
                return send_info_list(c);
            }
            else if(item_id == ITEM_ID_INIT_GM) {
                return send_gm_menu(c);
            }
            else if(item_id == ITEM_ID_INIT_PATCH) {
                return send_patch_menu(c);
            }

            return -1;

        /* Ship */
        case MENU_ID_SHIP:
            if(item_id == 0) {
                /* A "Ship List" menu item */
                return send_ship_list(c, (uint16_t)(menu_id >> 8));
            }
            else {
                /* An actual ship */
                return ship_transfer(c, item_id);
            }

        /* Quest */
        case MENU_ID_QUEST:
            /* Make sure the item is valid */
            if (l) {
                if ((int)item_id < l->cats[0].quest_count) {
                    rv = send_quest(c, l->cats[0].quests[item_id]);

                    if (c->type == CLIENT_AUTH_PC) {
                        rv |= send_initial_menu(c);
                    }

                    return rv;
                }
            }
            else {
                return -1;
            }

        /* Information Desk */
        case MENU_ID_INFODESK:
            if(item_id == 0xFFFFFFFF) {
                return send_initial_menu(c);
            }
            else {
                return send_info_file(c, item_id);
            }

        /* GM Operations */
        case MENU_ID_GM:
            /* Make sure the user is a GM, and not just messing with packets. */
            if(!IS_GLOBAL_GM(c)) {
                return -1;
            }

            switch(item_id) {
                case ITEM_ID_GM_REF_QUESTS:
                    read_quests();

                    send_info_reply(c, __(c, "\tEQuests refreshed."));
                    return send_gm_menu(c);

                case ITEM_ID_GM_RESTART:
                    if(!IS_GLOBAL_ROOT(c)) {
                        return -1;
                    }

                    shutting_down = 2;
                    send_info_reply(c, __(c, "\tERestart scheduled.\n"
                                          "Will restart when\n"
                                          "the server is empty."));
                    AUTH_LOG("Restart scheduled by %" PRIu32 "",
                          c->guildcard);
                    return send_gm_menu(c);

                case ITEM_ID_GM_SHUTDOWN:
                    if(!IS_GLOBAL_ROOT(c)) {
                        return -1;
                    }

                    shutting_down = 1;
                    send_info_reply(c, __(c, "\tEShutdown scheduled.\n"
                                          "Will shut down when\n"
                                          "the server is empty."));
                    AUTH_LOG("Shutdown scheduled by %" PRIu32 "",
                          c->guildcard);
                    return send_gm_menu(c);

                case 0xFFFFFFFF:
                    return send_initial_menu(c);

                default:
                    return -1;
            }

        case MENU_ID_PATCH:
            if(item_id == ITEM_ID_LAST) {
                return send_initial_menu(c);
            }
            else if(c->type == CLIENT_AUTH_DC) {
                p = patch_find(patches_v2, c->det_version, item_id);
                if(p) {
                    send_single_patch_dc(c, p);
                }
                else {
                    return -1;
                }
            }
            else if(c->type == CLIENT_AUTH_GC) {
                p = patch_find(patches_gc, c->det_version, item_id);
                if(p) {
                    send_single_patch_gc(c, p);
                }
                else {
                    return -1;
                }
            }

            return send_patch_menu(c);

        default:
            return -1;
    }
}

/* Check a player's character data for potential hackery. */
static int handle_char_data(login_client_t *c, dc_char_data_pkt *pkt) {
    int j, rv = 1;
    iitem_t *item;
    player_t *pl = &pkt->data;
    uint32_t v;

    /* Make sure some basic stuff is in range. */
    v = LE32(pl->v1.character.disp.level);
    if(v > 199) {
        send_large_msg(c, __(c, "\tEHacked characters are not allowed\n"
                                "on this server.\n\n"
                                "This will be reported to the server\n"
                                "administration."));
        ERR_LOG("Character with invalid level (%d) detected!\n"
              "    Guild card: %" PRIu32 "", v + 1, c->guildcard);
        return -1;
    }

    if(pl->v1.character.dress_data.model != 0 || (pl->v1.character.dress_data.v2flags & 0x02)) {
        send_large_msg(c, __(c, "\tEHacked characters are not allowed\n"
                                "on this server.\n\n"
                                "This will be reported to the server\n"
                                "administration."));
        ERR_LOG("Character with invalid model (%d:%02x) detected!\n"
              "    Guild card: %" PRIu32 "", (int)pl->v1.character.dress_data.model,
              pl->v1.character.dress_data.v2flags, c->guildcard);
        return -1;
    }

    /* If we don't have a legit mode set, then everyone's legit,
       as long as they passed the earlier checks. */
    if(!limits) {
        return 0;
    }

    switch(c->type) {
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_PC:
            v = ITEM_VERSION_V2;
            break;

        case CLIENT_AUTH_GC:
            v = ITEM_VERSION_GC;
            break;

        case CLIENT_AUTH_XBOX:
            v = ITEM_VERSION_XBOX;
            break;

        default:
            return -1;
    }

    /* Look through each item */
    for(j = 0; j < pl->v1.character.inv.item_count && rv; ++j) {
        item = (iitem_t *)&pl->v1.character.inv.iitems[j];
        rv = psocn_limits_check_item(limits, item, v);
    }

    /* If the person has banned items, boot them */
    if(!rv) {
        send_large_msg(c, __(c, "\tEYou have one or more banned items in\n"
                             "your inventory. Please remove them and\n"
                             "try again later."));
        return -1;
    }

    return 0;
}

static int handle_info_req(login_client_t *c, dc_select_pkt *pkt) {
    psocn_quest_list_t *l = NULL;
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);
    uint16_t menu_code;
    int ship_num;
    char str[256];
    void *result;
    char **row;
    const char *desc;

    /* Don't go out of bounds... */
    if(c->type < CLIENT_AUTH_DCNTE)
         l = &qlist[c->type][c->language_code];

    switch(menu_id & 0xFF) {
        /* Ship */
        case MENU_ID_SHIP:
            /* If its a list, say nothing */
            if(item_id == 0) {
                return send_info_reply(c, __(c, "\tE空空如也."));
            }

            /* We should have a ship ID as the item_id at this point, so query
               the db for the info we want. */
            sprintf(str, "SELECT name, players, games, menu_code, ship_number "
                "FROM %s WHERE ship_id='%lu'"
                , SERVER_SHIPS_ONLINE, (unsigned long)item_id);

            /* Query for what we're looking for */
            if(psocn_db_real_query(&conn, str)) {
                return -1;
            }

            if(!(result = psocn_db_result_store(&conn))) {
                return -2;
            }

            /* If we don't have a row, then the ship is offline */
            if(!(row = psocn_db_result_fetch(result))) {
                return send_info_reply(c, __(c, "\tE\tC4当前舰船已\n"
                                             "离线."));
            }

            /* Parse out the menu code */
            menu_code = (uint16_t)atoi(row[3]);
            ship_num = atoi(row[4]);

            /* Send the info reply */
            if(!menu_code) {
                sprintf(str, "%02X:%s\n%s %s\n%s %s", ship_num, row[0], row[1],
                        __(c, "玩家"), row[2], __(c, "房间"));
            }
            else {
                sprintf(str, "%02X:%c%c/%s\n%s %s\n%s %s", ship_num,
                        (char)menu_code, (char)(menu_code >> 8), row[0], row[1],
                        __(c, "玩家"), row[2], __(c, "房间"));
            }

            psocn_db_result_free(result);

            return send_info_reply(c, str);

        /* Quest */
        case MENU_ID_QUEST:
            if(!l)
                return -1;

            /* Make sure the item is valid */
            if((int)item_id < l->cats[0].quest_count)
                return send_quest_description(c, l->cats[0].quests[item_id]);
            else
                return -1;

        /* Patches */
        case MENU_ID_PATCH:
            if(item_id == ITEM_ID_LAST)
                return 0;

            if(c->type == CLIENT_AUTH_DC)
                desc = patch_get_desc(patches_v2, item_id, c->language_code);
            else if(c->type == CLIENT_AUTH_GC)
                desc = patch_get_desc(patches_gc, item_id, c->language_code);
            else
                return 0;

            return send_info_reply(c, desc);

        default:
            /* Ignore any other info requests. */
            return 0;
    }
}

/* Process a patch return packet, mainly looking for what version the client is
   running. */
int handle_patch_return(login_client_t *c, patch_return_pkt *pkt) {
    uint32_t ver;

    /* See if it's one corresponding to our version check... */
    switch(c->type) {
        case CLIENT_AUTH_DC:
        case CLIENT_AUTH_GC:
            if(pkt->hdr.dc.flags != 0xff)
                return 0;

            ver = LE32(pkt->return_value);
            c->det_version = ver;
            break;

        case CLIENT_AUTH_PC:
            if(pkt->hdr.pc.flags != 0xfe)
                return 0;

    }

    return 0;
}

/* Process one login packet. */
int process_dclogin_packet(login_client_t *c, void *pkt) {
    dc_pkt_hdr_t *dc = (dc_pkt_hdr_t *)pkt;
    pc_pkt_hdr_t *pc = (pc_pkt_hdr_t *)pkt;
    uint8_t type;
    uint16_t len;
    int tmp = 0;

    if(c->type == CLIENT_AUTH_DC || c->type == CLIENT_AUTH_GC ||
       c->type == CLIENT_AUTH_EP3 || c->type == CLIENT_AUTH_DCNTE ||
       c->type == CLIENT_AUTH_XBOX) {
        type = dc->pkt_type;
        len = LE16(dc->pkt_len);
    }
    else {
        type = pc->pkt_type;
        len = LE16(pc->pkt_len);
    }

#ifdef DEBUG

    DBG_LOG("DC登录指令: 0x%04X %s c->ext_version %d c->type %d", type, c_cmd_name(type, 0), c->ext_version, c->type);

#endif // DEBUG

    switch(type) {
        case LOGIN_88_TYPE:
            /* XXXX: Oh look, we have a network trial edition up in here. */
            return handle_ntelogin(c, (dcnte_login_88_pkt *)pkt);

        case LOGIN_8A_TYPE:
            /* XXXX: Uhm... Sega... WTF? */
            return handle_ntelogin8a(c, (dcnte_login_8a_pkt *)pkt);

        case LOGIN_8B_TYPE:
            /* XXXX: Maybe this will fix things? */
            return handle_ntelogin8b(c, (dcnte_login_8b_pkt *)pkt);

        case LOGIN_90_TYPE:
            /* XXXX: Hey! this does something now! */
            return handle_login0(c, (dc_login_90_pkt *)pkt);

        case LOGIN_92_TYPE:
            /* XXXX: Do something with this sometime. */
            return send_simple(c, LOGIN_92_TYPE, LOGIN_92_OK);

        case LOGIN_93_TYPE:
            /* XXXX: Figure this all out sometime. */
            return handle_login3(c, (dc_login_93_pkt *)pkt);

        case LOGIN_9A_TYPE:
            /* XXXX: You had to switch packets on me, didn't you Sega? */
            return handle_logina(c, (dcv2_login_9a_pkt *)pkt);

        case CHECKSUM_TYPE:
            /* XXXX: ??? */
            if(send_simple(c, CHECKSUM_REPLY_TYPE, 1)) {
                return -1;
            }

            return send_simple(c, CHAR_DATA_REQUEST_TYPE, 0);

        case TIMESTAMP_TYPE:
            /* XXXX: Actually, I've got nothing here. */
            return send_timestamp(c);

        case EP3_UPDATE_REQ_TYPE:
            /* XXXX: We'll fall through the bottom of this... */
            if(c->type == CLIENT_AUTH_EP3) {
                if(send_ep3_rank_update(c)) {
                    return -1;
                }

                if(send_ep3_card_update(c)) {
                    return -1;
                }
            }

        case SHIP_LIST_TYPE:
            /* XXXX: I don't have anything here either, but thought I'd be
               funny anyway. */
            /* TODO fix */
            //tmp = send_motd(c);

            //if(!tmp) {
            //    c->motd_wait = 1;
            //    return 0;
            //}
            //else if(tmp < 0) {
            //    return tmp;
            //}

            /* Don't send the initial menu to the PC NTE, as there's no good
               reason to send it quest files that it can't do anything useful
               with. */
            if(c->ext_version != (CLIENT_EXTVER_PC | CLIENT_EXTVER_PCNTE))
                return send_initial_menu(c);
            else
                return send_ship_list(c, 0);

        case INFO_REQUEST_TYPE:
            /* XXXX: Relevance, at last! */
            return handle_info_req(c, (dc_select_pkt *)pkt);

        case MENU_SELECT_TYPE:
            /* XXXX: This might actually work, at least if there's a ship. */
            return handle_ship_select(c, (dc_select_pkt *)pkt);

        case GC_VERIFY_LICENSE_TYPE:
            /* XXXX: Why in the world do they duplicate so much data here? */
            if(c->type != CLIENT_AUTH_XBOX)
                return handle_gchlcheck(c, (v3_hlcheck_pkt *)pkt);
            else
                return handle_xbhlcheck(c, (xb_hlcheck_pkt *)pkt);

        case LOGIN_9C_TYPE:
            /* XXXX: Yep... check things here too. */
            if(c->type != CLIENT_AUTH_XBOX)
                return handle_gcloginc(c, (gc_login_9c_pkt *)pkt);
            else
                return handle_xbloginc(c, (xb_login_9c_pkt *)pkt);

        case LOGIN_9E_TYPE:
            /* XXXX: One final check, and give them their guildcard. */
            if(c->type != CLIENT_AUTH_XBOX)
                return handle_gclogine(c, (gc_login_9e_pkt *)pkt);
            else
                return handle_xblogine(c, (xb_login_9e_pkt *)pkt);

        case LOGIN_9D_TYPE:
            /* XXXX: All this work for a language and version code... */
            return handle_logind(c, (dcv2_login_9d_pkt *)pkt);

        case CHAR_DATA_TYPE:
            /* XXXX: Gee, I can be mean, can't I? */
            return handle_char_data(c, (dc_char_data_pkt *)pkt);

        case GAME_COMMAND0_TYPE:
            /* XXXX: Added so screenshots work on the ship list... */
            return 0;

        case BURSTING_TYPE:
            /* XXXX: Why would you ask to disconnect? */
            c->disconnected = 1;
            return 0;

        case EP3_RANK_UPDATE_TYPE:
        case EP3_CARD_UPDATE_TYPE:
            /* XXXX: I have no idea what to do with these... */
            return 0;

        case GC_MSG_BOX_CLOSED_TYPE:
            /* XXXX: Fixed for having an initial MOTD. I wonder if Blue Burst
               has this packet type too. If so, then starting it with GC doesn't
               necessarily make sense. */
            if(c->motd_wait) {
                c->motd_wait = 0;
                return send_initial_menu(c);
            }

            return send_info_list(c);

        case DL_QUEST_FILE_TYPE:
        case DL_QUEST_CHUNK_TYPE:
            /* XXXX: Nothing useful to do with these by the time we get them. */
            return 0;

        case PATCH_RETURN_TYPE:
            /* XXXX: Hopefully nobody crashes, right? */
            return handle_patch_return(c, (patch_return_pkt *)pkt);

        default:
            DBG_LOG("未知 DC 认证 : 0x%02X\n", type);
            //UNK_CPD(type, c->version, pkt);
            display_packet((unsigned char *)pkt, len);
            return -3;
    }

    return 0;
}
