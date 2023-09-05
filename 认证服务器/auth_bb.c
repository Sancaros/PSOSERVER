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
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <md5.h>
#include <debug.h>
#include <f_logs.h>
#include <database.h>

#include <pso_opcodes_block.h>
#include <pso_version.h>

#include "auth.h"
#include <pso_player.h>
#include "packets.h"
#include "auth_packets.h"

const void* my_ntop(struct sockaddr_storage* addr, char str[INET6_ADDRSTRLEN]);

/* 0x0005 5*/
static int handle_bb_burst(login_client_t* c, bb_burst_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    char ipstr[INET6_ADDRSTRLEN];

    my_ntop(&c->ip_addr, ipstr);

    c->disconnected = 1;

    return 0;
}

/* 0x0093 147*/
static int handle_bb_login(login_client_t *c, bb_login_93_pkt *pkt) {
    char query[512];
    int len;
    char tmp_username[32];
    void *result;
    char **row;
    //uint8_t hash[32];
    uint32_t ch;
    int8_t security_sixtyfour_binary[18] = { 0 };
    //int64_t security_sixtyfour_check;
    uint32_t hwinfo[2] = { 0 };
    uint32_t /*guild_id = 0, priv, guildcard,*/ isbanded, isactive, islogged;
    //uint16_t clientver;
    uint8_t MDBuffer[0x30] = { 0 };
    int8_t password[0x30] = { 0 };
    int8_t md5password[0x30] = { 0 };
    time_t banlen;
    int banned = is_ip_banned(c, &banlen, query);
    int logged = 0;

    //display_packet((uint8_t*)pkt, pkt->hdr.pkt_len);

    bool is_old_format;
    if (pkt->hdr.pkt_len == sizeof(bb_login_93_pkt) - sizeof(pkt->hdr)) {
        is_old_format = true;
        //DBG_LOG("低版本客户端登录 %d %d %d %d", is_old_format, pkt->hdr.pkt_len, sizeof(bb_login_93_pkt) - 8, sizeof(bb_login_93_pkt));
    }
    else if (pkt->hdr.pkt_len == sizeof(bb_login_93_pkt)) {
        is_old_format = false;
        //DBG_LOG("高版本客户端登录 %d %d %d %d", is_old_format, pkt->hdr.pkt_len, sizeof(bb_login_93_pkt) - 8, sizeof(bb_login_93_pkt));
    }
    else {
        DBG_LOG("未知版本客户端登录 %d %d %d", pkt->hdr.pkt_len, sizeof(bb_login_93_pkt) - sizeof(pkt->hdr), sizeof(bb_login_93_pkt));
        return -1;
    }

    c->bbversion = pkt->version;

    /*memset(&hwinfo[0], 0, 18);
    psocn_db_escape_str(&conn, &hwinfo[0], &pkt->hwinfo[0], 8);
    memcpy(&c->login_hwinfo[0], &hwinfo[0], 18);*/

    /* Make sure the username string is sane... */
    len = strlen(pkt->username); 
    
    //AUTH_LOG("账号 %s 长度 %d 密码 %s 长度 %d ", pkt->username, len, pkt->password, len);

    if(len > 48 || strlen(pkt->password) > 48) {
        send_bb_security(c, 0, LOGIN_93BB_FORCED_DISCONNECT, 0, NULL, 0);
        return -1;
    }

    psocn_db_escape_str(&conn, tmp_username, pkt->username, len);

    sprintf_s(query, _countof(query), "SELECT %s.account_id, isbanned, isactive, islogged, guild_id, "
        "privlevel, %s.guildcard, dressflag, isgm, regtime, %s.password, menu_id, preferred_lobby_id FROM "
        "%s INNER JOIN %s ON "
        "%s.account_id = %s.account_id WHERE "
        "%s.username='%s'"
        , AUTH_ACCOUNT
        , AUTH_ACCOUNT, AUTH_ACCOUNT_BLUEBURST
        , AUTH_ACCOUNT, AUTH_ACCOUNT_BLUEBURST
        , AUTH_ACCOUNT, AUTH_ACCOUNT_BLUEBURST
        , AUTH_ACCOUNT_BLUEBURST, tmp_username);

    /* Query the database for the user... */
    if(psocn_db_real_query(&conn, query)) {
        send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
        return -2;
    }

    result = psocn_db_result_store(&conn);
    if(!result) {
        send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
        return -2;
    }

    row = psocn_db_result_fetch(result);
    if(!row) {
        send_bb_security(c, 0, LOGIN_93BB_NO_USER_RECORD, 0, NULL, 0);
        psocn_db_result_free(result);
        return -3;
    }

    isbanded = atoi(row[1]);

    /* Make sure some simple checks pass first... */
    if (isbanded) {
        /* 玩家已被封禁. */
        send_bb_security(c, 0, LOGIN_93BB_BANNED, 0, NULL, 0);
        psocn_db_result_free(result);
        return -4;
    }

    isactive = atoi(row[2]);

    /* Make sure some simple checks pass first... */
    if (!isactive) {
        /* 账号未激活. */
        send_bb_security(c, 0, LOGIN_93BB_LOCKED, 0, NULL, 0);
        psocn_db_result_free(result);
        return -4;
    }

    islogged = db_check_gc_online((uint32_t)strtoul(row[6], NULL, 0));

    /* Make sure some simple checks pass first... */
    if (islogged) {
        /* 账号已在线. */
        send_bb_security(c, 0, LOGIN_93BB_ALREADY_ONLINE, 0, NULL, 0);
        psocn_db_result_free(result);
        return -4;
    }

    memcpy(&password, &pkt->password, sizeof(pkt->password));

    sprintf_s(&password[strlen(password)],
        sizeof(password) - strlen(password), "_%s_salt", row[9]);
    md5((unsigned char*)password, strlen(password), MDBuffer);
    for (ch = 0; ch < 16; ch++)
        sprintf_s(&md5password[ch * 2],
            sizeof(md5password) - ch * 2, "%02x", (uint8_t)MDBuffer[ch]);
    md5password[32] = 0;

    if (memcmp(&md5password[0], row[10], 32)) {
        /* Password check failed... */
        send_bb_security(c, 0, LOGIN_93BB_BAD_USER_PWD, 0, NULL, 0);
        psocn_db_result_free(result);
        return -6;
    }

    /* Grab the rest of what we care about from the query... */
    errno = 0;
    c->account_id = (uint32_t)strtoul(row[0], NULL, 0);
    c->guild_id = (uint32_t)strtoul(row[4], NULL, 0);
    c->priv = (uint32_t)strtoul(row[5], NULL, 0);
    c->guildcard = (uint32_t)strtoul(row[6], NULL, 0);
    //c->flags = (uint32_t)strtoul(row[7], NULL, 0);
    c->isgm = (uint32_t)strtoul(row[8], NULL, 0);
    //c->menu_id = (uint32_t)strtoul(row[11], NULL, 0);
    c->menu_id = pkt->menu_id;
    //c->preferred_lobby_id = (uint32_t)strtoul(row[12], NULL, 0);

    psocn_db_next_result_free(&conn, result);

    if(errno) {
        ERR_LOG("handle_bb_L_login errno = %d  %d %d %d", errno, c->guild_id, c->priv, c->guildcard);
        send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
        return -2;
    }

    memcpy_s(&c->bbversion_string, _countof(c->bbversion_string), &pkt->var.new_clients.cfg.version_string, sizeof(pkt->var.new_clients.cfg.version_string));

    sprintf_s(query, _countof(query), "UPDATE %s SET versionstring = '%s', version = '%d' where guildcard = '%u'",
        AUTH_ACCOUNT_BLUEBURST, c->bbversion_string, c->bbversion, c->guildcard);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("更新GC %u 数据错误:\n %s", c->guildcard, psocn_db_error(&conn));
        return -4;
    }

    /* TODO 这里要完成一个服务器版本的限制功能 从数据库或者网络获取正确的客户端版本*/
    //char* PSO_CLIENT_VER_STRING = "CNBBVER0.00.01";

    //// 检测登陆版本客户端是否正确.
    //if (strcmp(PSO_CLIENT_VER_STRING, c->bbversion_string)) {
    //    send_bb_security(c, 0, LOGIN_93BB_BAD_VERSION, 0, NULL, 0);
    //    return -2;
    //}

    /* Set up the security data (everything else is already 0'ed). */
    c->sec_data.cfg.magic = CLIENT_CONFIG_MAGIC; //这个直接覆盖了版本信息 magic就是版本信息

    if (pkt->menu_id == MENU_ID_LOBBY) {
        c->preferred_lobby_id = pkt->preferred_lobby_id;
    }

    sprintf_s(query, _countof(query), "SELECT guildcard FROM %s WHERE username = '%s'", AUTH_SECURITY, tmp_username);
    psocn_db_real_query(&conn, query);
    result = psocn_db_result_store(&conn);
    row = psocn_db_result_fetch(result);

    /* Query the database for the user... */
    if (!row) {
        sprintf_s(query, _countof(query), "INSERT INTO %s (guildcard, username, menu_id, preferred_lobby_id, isgm, security_data) "
            "VALUES ('%u', '%s', '%d', '%d', '%d', '%s')",
            AUTH_SECURITY, c->guildcard, tmp_username, c->menu_id, c->preferred_lobby_id, c->isgm, (char*)&c->sec_data);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("新增GC %u 数据错误:\n %s", c->guildcard, psocn_db_error(&conn));
            send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
            return -1;
        }
    }
    else {
        sprintf_s(query, _countof(query), "UPDATE %s SET menu_id = '%d', preferred_lobby_id = '%d', isgm = '%d', security_data = '%s'"
            "WHERE username = '%s'", AUTH_SECURITY, c->menu_id, c->preferred_lobby_id, c->isgm, (char*)&c->sec_data, 
            tmp_username);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("更新GC %u 数据错误:\n %s", c->guildcard, psocn_db_error(&conn));
            send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
            return -1;
        }
    }

    psocn_db_next_result_free(&conn, result);

    /* Send the security data packet */
    if(send_bb_security(c, c->guildcard, LOGIN_93BB_OK, c->guild_id, &c->sec_data,
                        sizeof(bb_client_config_pkt))) {
        send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
        return -7;
    }

    /* Last step is to redirect them to the charater data port... */
    return send_redirect(c, srvcfg->server_ip, bb_sockets[1].port);
}

int process_login_packet_bb(login_client_t* c, void* pkt) {
    __try {
        bb_pkt_hdr_t* bb = (bb_pkt_hdr_t*)pkt;
        uint16_t type = LE16(bb->pkt_type);
        uint16_t len = LE16(bb->pkt_len);

#ifdef DEBUG
        DBG_LOG("BB角色指令: 0x%04X %s", type, c_cmd_name(type, 0));
        display_packet(pkt, LE16(bb->pkt_len));
#endif // DEBUG

        switch (type) {
            /* 0x0005 5*/
        case BURSTING_TYPE:
            return handle_bb_burst(c, (bb_burst_pkt*)pkt);

            /* 0x0093 147*/
        case LOGIN_93_TYPE:
            return handle_bb_login(c, (bb_login_93_pkt*)pkt);

        default:
            DBG_LOG("未知 BB 认证 : 0x%02X", type);
            //UNK_CPD(type, c->version, pkt);
            //display_packet(pkt, len);
            return -1;
        }

        return 0;

    }
    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        ERR_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -2;
    }
}
