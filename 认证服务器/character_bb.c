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
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <direct.h>
#include <urlmon.h>
#include <time.h>
#include <locale.h>
#include <inttypes.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <zlib.h>
#include <md5.h>


#include <f_checksum.h>
#include <debug.h>
#include <database.h>
#include <f_logs.h>
#include <f_iconv.h>
#include <pso_version.h>

#include <PRS.h>

#include "auth.h"
#include <pso_player.h>
#include "packets.h"
#include "packets_bb.h"
#include "auth_packets.h"

extern volatile sig_atomic_t shutting_down;

#define MAX_DRESS_FLAGS 500

/* This stuff is cached at the start of the program */
static bb_param_hdr_pkt *param_hdr = NULL;
static bb_param_chunk_pkt **param_chunks = NULL;
static int num_param_chunks = 0;
static psocn_bb_full_char_t default_full_chars;
static psocn_bb_db_char_t default_chars[12];
static bb_level_table_t bb_char_stats;
static dress_flag_t dress_flags[MAX_DRESS_FLAGS];
const void* my_ntop(struct sockaddr_storage* addr, char str[INET6_ADDRSTRLEN]);

static int handle_ignored_pkt(login_client_t* c, const char* cmd, void* pkt, int codeline) {
    bb_pkt_hdr_t* bb = (bb_pkt_hdr_t*)pkt;
    uint16_t type = LE16(bb->pkt_type);

    //DBG_LOG("忽略的BB数据包指令 0x%04X", type);
    UDONE_CPD(type, c->version, pkt);
    //print_payload(pkt, LE16(bb->pkt_len));

    return 0;
}

/* 0x0005 5*/
static int handle_bb_burst(login_client_t* c, bb_burst_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    send_disconnect(c, c->auth);

    return 0;
}

/* 0x0009 9*/
static int handle_info_req(login_client_t* c, bb_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);
    uint16_t menu_code;
    int ship_num;
    char str[256];
    void* result;
    char** row;

    switch (menu_id & 0xFF) {
        /* Ship */
    case MENU_ID_SHIP:
        /* If its a list, say nothing */
        if (item_id == 0) {
            return send_info_reply(c, __(c, "\tE舰船港口."));
        }

        /* We should have a ship ID as the item_id at this point, so query
           the db for the info we want. */
        sprintf(str, "SELECT name, players, games, menu_code, ship_number "
            "FROM %s WHERE ship_id='%lu'",
            SERVER_SHIPS_ONLINE, (unsigned long)item_id);

        /* Query for what we're looking for */
        if (psocn_db_real_query(&conn, str)) {
            return -1;
        }

        if (!(result = psocn_db_result_store(&conn))) {
            return -2;
        }

        /* If we don't have a row, then the ship is offline */
        if (!(row = psocn_db_result_fetch(result))) {
            return send_info_reply(c, __(c, "\tE\tC4该船已离线."));
        }

        /* Parse out the menu code */
        menu_code = (uint16_t)atoi(row[3]);
        ship_num = atoi(row[4]);

        /* Send the info reply */
        if (!menu_code) {
            sprintf(str, "%02X:%s\n%s %s\n%s %s", ship_num, row[0],
                row[1], __(c, "玩家"), row[2], __(c, "房间"));
        }
        else {
            sprintf(str, "%02X:%c%c/%s\n%s %s\n%s %s", ship_num,
                (char)menu_code, (char)(menu_code >> 8), row[0],
                row[1], __(c, "玩家"), row[2], __(c, "房间"));
        }

        psocn_db_result_free(result);

        return send_info_reply(c, str);

    default:
        /* Ignore any other info requests. */
        return 0;
    }
}

/* Handle a client's ship select packet. */
/* 0x0010 16*/
static int handle_ship_select(login_client_t* c, bb_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    //DBG_LOG("handle_ship_select指令: 0x%08X item_id %d", menu_id & 0x000000FF, item_id);

    switch (menu_id & 0xFF) {
        /* Initial menu */
    case MENU_ID_INITIAL:
        if (item_id == ITEM_ID_INIT_SHIP) {
            /* Ship Select */
            return send_ship_list(c, 0);
        }
        else if (item_id == ITEM_ID_INIT_INFO) {
            return send_info_list(c);
        }
        else if (item_id == ITEM_ID_INIT_GM) {
            return send_gm_menu(c);
        }
        else if (item_id == ITEM_ID_DISCONNECT) {
            return send_disconnect(c, c->auth);
        }

        return send_disconnect(c, c->auth);

        /* Ship */
    case MENU_ID_SHIP:
        if (item_id == 0) {
            /* A "Ship List" menu item */
            return send_ship_list(c, (uint16_t)(menu_id >> 8));
        }
        else {
            /* An actual ship */
            return ship_transfer(c, item_id);
        }

        return -1;

        /* Information Desk */
    case MENU_ID_INFODESK:
        if (item_id == 0xFFFFFFFF) {
            return send_initial_menu(c);
        }
        else {
            return send_initial_menu(c);
            //return send_info_file(c, item_id);
        }

        return send_disconnect(c, c->auth);

        /* GM Operations */
    case MENU_ID_GM:
        /* Make sure the user is a GM, and not just messing with packets. */
        if (!IS_GLOBAL_GM(c)) {
            return -1;
        }

        switch (item_id) {
        case ITEM_ID_GM_REF_QUESTS:
            read_quests();

            send_info_reply(c, __(c, "\tEQuests refreshed."));
            return send_gm_menu(c);

        case ITEM_ID_GM_RESTART:
            if (!IS_GLOBAL_ROOT(c)) {
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
            if (!IS_GLOBAL_ROOT(c)) {
                return -1;
            }

            shutting_down = 1;
            send_info_reply(c, __(c, "\tEShutdown scheduled.\n"
                "Will shut down when\n"
                "the server is empty."));
            AUTH_LOG("Shutdown scheduled by %" PRIu32 "",
                c->guildcard);
            return send_gm_menu(c);

        case ITEM_ID_LAST:
            return send_initial_menu(c);

        default:
            return send_disconnect(c, c->auth);
        }

    case MENU_ID_NO_SHIP:
        ERR_LOG("GC %u 未找到舰船", c->guildcard);
        return send_disconnect(c, c->auth);

    default:
        UNK_CPD(menu_id & 0xFF, c->version, (uint8_t*)pkt);
        return send_disconnect(c, c->auth);
    }
}

/* 0x001D 29*/

/* 0x0093 147*/
static int handle_bb_login(login_client_t *c, bb_login_93_pkt *pkt) {
    char query[512];
    int len;
    char tmp[32];
    void *result;
    char **row;
    //uint8_t hash[32];
    uint32_t hwinfo[2] = { 0 };
    //uint16_t clientver;
    uint32_t isbanded, isactive;
    uint8_t MDBuffer[0x30] = { 0 };
    int8_t password[0x30] = { 0 };
    int8_t md5password[0x30] = { 0 };
    uint32_t ch;
    int32_t /*security_client_thirtytwo,*/ security_thirtytwo_check = 0;
    int64_t /*security_client_sixtyfour,*/ security_sixtyfour_check = 0;

    char ipstr[INET6_ADDRSTRLEN];
    my_ntop(&c->ip_addr, ipstr);

    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    char timestamp[28];

    memset(&login_welcom_msg[0], 0, sizeof(login_welcom_msg));
    psocn_web_server_getfile(cfg->w_motd.web_host, cfg->w_motd.web_port, cfg->w_motd.login_welcom_file, Welcome_Files[1]);
    psocn_web_server_loadfile(Welcome_Files[1], &login_welcom_msg[0]);

    c->bbversion = pkt->version;

    //if (clientver != 65) {
    //    send_bb_security(c, 0, LOGIN_93BB_FORCED_DISCONNECT, 0, NULL, 0);
    //    return -1;
    //}

    /* Make sure the username string is sane... */
    len = strlen(pkt->username);
    //ERR_LOG("账号 %s 长度 %d 密码 %s 长度 %d ", pkt->username, len, pkt->password, len);
    if(len > 48 || strlen(pkt->password) > 48) {
        send_bb_security(c, 0, LOGIN_93BB_FORCED_DISCONNECT, 0, NULL, 0);
        return -1;
    }

    memcpy(&c->username[0], &pkt->username[0], 16);

    psocn_db_escape_str(&conn, tmp, pkt->username, len);
    sprintf_s(query, _countof(query), "SELECT %s.account_id, isbanned, islogged ,isactive, guild_id, "
        "privlevel, %s.guildcard, dressflag, isgm, regtime, %s.password, menu_id, preferred_lobby_id FROM "
        "%s INNER JOIN %s ON "
        "%s.account_id = %s.account_id WHERE "
        "%s.username='%s'"
        , AUTH_ACCOUNT
        , AUTH_ACCOUNT, AUTH_ACCOUNT_BLUEBURST
        , AUTH_ACCOUNT, AUTH_ACCOUNT_BLUEBURST
        , AUTH_ACCOUNT, AUTH_ACCOUNT_BLUEBURST
        , AUTH_ACCOUNT_BLUEBURST, tmp
    );

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
    if(isbanded) {
        /* User is banned by account. */
        send_bb_security(c, 0, LOGIN_93BB_BANNED, 0, NULL, 0);
        psocn_db_result_free(result);
        return -4;
    }

    /* Make sure some simple checks pass first... */
    if (db_check_gc_online((uint32_t)strtoul(row[6], NULL, 0))) {
        /* 玩家已在线. */
        //send_large_msg(c, __(c, "该账户已登录.\n\n请等候120秒后再次尝试登录."));
        send_bb_security(c, 0, LOGIN_93BB_ALREADY_ONLINE, 0, NULL, 0);
        psocn_db_result_free(result);
        return -4;
    }
    else
        db_update_gc_login_state((uint32_t)strtoul(row[6], NULL, 0), 0, -1, NULL);

    if (db_remove_gc_char_login_state((uint32_t)strtoul(row[6], NULL, 0))) {
        /* 玩家已在线. */
        //send_large_msg(c, __(c, "该账户已登录.\n\n请等候120秒后再次尝试登录."));
        send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
        psocn_db_result_free(result);
        return -4;
    }

    isactive = atoi(row[3]);

    /* Make sure some simple checks pass first... */
    if (!isactive) {
        /* User is banned by account. */
        send_bb_security(c, 0, LOGIN_93BB_LOCKED, 0, NULL, 0);
        psocn_db_result_free(result);
        return -4;
    }

    memcpy(&password, &pkt->password, sizeof(pkt->password));

    sprintf_s(&password[strlen(password)],
        _countof(password) - strlen(password), "_%s_salt", row[9]);
    md5((unsigned char*)password, strlen(password), MDBuffer);
    for (ch = 0; ch < 16; ch++)
        sprintf_s(&md5password[ch * 2],
            _countof(md5password) - ch * 2, "%02x", (uint8_t)MDBuffer[ch]);
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
    c->flags = (uint32_t)strtoul(row[7], NULL, 0);
    c->isgm = (uint32_t)strtoul(row[8], NULL, 0);
    c->menu_id = pkt->menu_id;
    c->preferred_lobby_id = pkt->preferred_lobby_id;
    psocn_db_result_free(result);

    if(errno) {
        send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
        return -2;
    }

    /* Copy in the security data */
    memcpy(&c->sec_data, &pkt->var.new_clients.cfg, sizeof(bb_client_config_pkt));

    if(c->sec_data.cfg.magic != CLIENT_CONFIG_MAGIC) {
        send_bb_security(c, 0, LOGIN_93BB_FORCED_DISCONNECT, 0, NULL, 0);
        return -8;
    }

    /* Send the security data packet */
    if(send_bb_security(c, c->guildcard, LOGIN_93BB_OK, c->guild_id,
                        &c->sec_data, sizeof(bb_client_config_pkt))) {
        return -7;
    }

    sprintf(timestamp, "%u-%02u-%02u %02u:%02u:%02u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond);

    sprintf_s(query, _countof(query), "UPDATE %s SET menu_id = '%d', preferred_lobby_id = '%d', lastbbversion = '%u', lastip = '%s', last_login_time = '%s'"
        "WHERE username = '%s'", AUTH_ACCOUNT, c->menu_id, c->preferred_lobby_id, c->bbversion, ipstr, timestamp, pkt->username);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("更新GC %u 数据错误:\n %s", c->guildcard, psocn_db_error(&conn));
        return -4;
    }

    /* Has the user picked a character already? */
    if(c->sec_data.sel_char) {

        c->islogged = 1;

        c->auth = 1;

        if (db_update_gc_login_state(c->guildcard, c->islogged, c->sec_data.slot, NULL)) {
            return -4;
        }

        sprintf_s(query, _countof(query), "UPDATE %s SET "
            "menu_id = '%d', preferred_lobby_id = '%d', isgm = '%d', security_data = '%s', slot = '%d' "
            "WHERE guildcard = '%u'", 
            AUTH_SECURITY, c->menu_id, c->preferred_lobby_id, c->isgm, (char*)&c->sec_data, c->sec_data.slot, c->guildcard);
        if (psocn_db_real_query(&conn, query))
        {
            SQLERR_LOG("更新GC %u 数据错误:\n %s", c->guildcard, psocn_db_error(&conn));
            send_bb_security(c, 0, LOGIN_93BB_UNKNOWN_ERROR, 0, NULL, 0);
            return -1;
        }

        if(send_timestamp(c)) {
            return -9;
        }

        if(send_initial_menu(c)) {
            return -10;
        }

        if (send_msg(c, BB_SCROLL_MSG_TYPE, __(c, &login_welcom_msg[0]))) {
            return -11;
        }
    }

    return 0;
}

/* 0x03DC 988*/
static int handle_guild_chunk(login_client_t* c, bb_guildcard_req_pkt* pkt) {
    uint32_t chunk_index, cont;

    chunk_index = LE32(pkt->chunk_index);
    cont = LE32(pkt->cont);

    /* Send data as long as the client is still looking for it. */
    if (cont) {
        /* Send the chunk */
        return send_bb_guild_chunk(c, chunk_index);
    }

    return 0;
}

/* 0x00E0 224*/
static int handle_option_request(login_client_t *c, bb_option_req_pkt* pkt) {
    psocn_bb_db_opts_t opts;
    psocn_bb_db_guild_t guild_data;
    int rv = 0;

    opts = db_get_bb_char_option(c->guildcard);

    guild_data = db_get_bb_char_guild(c->guildcard);

    rv = send_bb_option_reply(c, opts.key_cfg, guild_data.guild_data);

    if (rv) {
        send_large_msg(c, __(c, "\tE数据库错误.\n\n"
            "请联系服务器管理员."));
        return -1;
    }

    return rv;
}

/* 0x00E2 226*/
static int handle_option(login_client_t* c, bb_opt_config_pkt* pkt) {

    int config_size = sizeof(bb_opt_config_pkt);

    print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    DBG_LOG("测试是否经过");

    c->disconnected = 1;
    return 0;
}

/* 0x00E3 227*/
static int handle_char_select(login_client_t *c, bb_char_select_pkt *pkt) {
    //char query[256];
    //void *result;
    //char **row;
    //unsigned long *len, sz;
    int rv = 0;
    psocn_bb_db_char_t *char_data;
    psocn_bb_mini_char_t mc = { 0 };

    /* Make sure the slot is sane */
    if(pkt->slot > 3) {
        return -1;
    }

    if(pkt->reason == 0) {
        char_data = db_get_uncompress_char_data(c->guildcard, pkt->slot);

        /* The client wants the preview data for character select... */
        if(char_data != NULL) {
           
            /* 已获得角色数据... 将其从检索的行中复制出来. */
            if (db_get_dress_data(c->guildcard, pkt->slot, &mc.dress_data, 0)) {
                SQLERR_LOG("无法更新玩家数据 (GC %"
                    PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
                /* XXXX: 未完成给客户端发送一个错误信息 */
                return -3;
            }

            //memcpy(&mc.name[0], &char_data->character.name[0], BB_CHARACTER_NAME_LENGTH * 2);

            mc.name.name_tag = char_data->character.name[0];
            mc.name.name_tag2 = char_data->character.name[1];
            memcpy(&mc.name.char_name[0], &char_data->character.name[2], sizeof(mc.name.char_name));
            

            mc.level = char_data->character.disp.level;
            mc.exp = char_data->character.disp.exp;
            mc.play_time = char_data->character.play_time;
            mc.dress_data.create_code = 0;
            
            if (db_updata_char_play_time(mc.play_time, c->guildcard, pkt->slot))
            {
                ERR_LOG("无法更新角色 %s 游戏时间数据!", CHARACTER);
                return -3;
            }

            free(char_data);

            rv = send_bb_char_preview(c, &mc, pkt->slot);
        }
        else {
            //ERR_LOG("未发现角色数据! 槽位 %d", pkt->slot);
            /* No data's there, so let the client know */
            rv = send_bb_char_ack(c, pkt->slot, BB_CHAR_ACK_NONEXISTANT);
        }
    }
    else {
        /* The client is actually selecting the character to play with. Update
           the data on the client, then send the acknowledgement. */
        c->sec_data.slot = pkt->slot;
        c->sec_data.sel_char = pkt->reason;

        char ipstr[INET6_ADDRSTRLEN];
        my_ntop(&c->ip_addr, ipstr);

        if (db_update_char_auth_msg(ipstr, c->guildcard, c->sec_data.slot)) {
            ERR_LOG("无法更新角色 %s 认证数据!", CHARACTER);
            rv = -4;
        }

        if(send_bb_security(c, c->guildcard, 0, c->guild_id, &c->sec_data,
                            sizeof(bb_client_config_pkt))) {
            rv = -4;
        }
        else {
            rv = send_bb_char_ack(c, pkt->slot, BB_CHAR_ACK_SELECT);
        }

        sprintf_s(myquery, _countof(myquery), "UPDATE %s SET slot = '%"PRIu8"' WHERE guildcard = '%"
            PRIu32 "'", AUTH_SECURITY, pkt->slot, c->guildcard);
        if (psocn_db_real_query(&conn, myquery)) {
            SQLERR_LOG("无法更新角色 %s 数据!", AUTH_SECURITY);
            //handle_todc(__LINE__, c);
        }
    }

    //psocn_db_result_free(result);

    return rv;
}

/* 0x00E5 229*/
static int handle_update_char(login_client_t* c, bb_char_preview_pkt* pkt) {
    uint32_t create_code, flags = c->flags;
    psocn_bb_db_char_t *char_data;
    uint8_t ch_class = pkt->data.dress_data.ch_class;

    //DBG_LOG("handle_update_char 标签 %d", flags);

    char_data = (psocn_bb_db_char_t*)malloc(sizeof(psocn_bb_db_char_t));

    if (!char_data) {
        ERR_LOG("无法分配角色数据内存空间");
        ERR_LOG("%s", strerror(errno));
        return -1;
    }

    switch (flags)
    {

    case PSOCN_DB_LOAD_CHAR:
        //DBG_LOG("载入角色数据");
        break;

    case PSOCN_DB_SAVE_CHAR:

        //if (db_upload_temp_data(&default_full_chars, sizeof(psocn_bb_full_char_t))) {

        //    ERR_LOG("上传完整角色 数据失败!");
        //    return -1;
        //}
        //for (int j = 0; j < 12; j++) {
        //    if (db_upload_temp_data(&default_chars[j], sizeof(psocn_bb_db_char_t))) {

        //        ERR_LOG("上传完整角色 数据失败!");
        //        return -1;
        //    }
        //    else {
        //        DBG_LOG("%d 上传完整 %s 角色", j, pso_class[j].cn_name);
        //    }
        //}


        /* 复制一份缓冲默认角色数据 根据选取的角色职业定义 */
        memcpy(char_data, &default_chars[ch_class], sizeof(psocn_bb_db_char_t));

        /* 复制初始角色数值 */
        memcpy(&char_data->character.disp.stats, &bb_char_stats.start_stats[ch_class], sizeof(psocn_pl_stats_t));

        /* TODO 加载设置文件 定义角色初始美赛塔*/
        char_data->character.disp.meseta = 100000;

        /* 复制外观数据 */
        if (send_bb_char_dressing_room(&char_data->character, &pkt->data)) {
            ERR_LOG("无法更新玩家更衣室数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
        }
        
        //DBG_LOG("重建角色 create_code 数值 %d", char_data->character.dress_data.create_code);

        create_code = char_data->character.dress_data.create_code;

        char_data->character.play_time = 0;

        if (db_backup_bb_char_data(c->guildcard, pkt->slot)) {
            ERR_LOG("无法备份已删除的玩家数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            goto err;
        }

        if (db_delete_bb_char_data(c->guildcard, pkt->slot)) {
            SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            goto err;
        }

        if (db_insert_char_data(char_data, c->guildcard, pkt->slot)) {
            ERR_LOG("无法创建玩家数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        /* 获取玩家角色背包数据数据项 */
        if (db_insert_char_inv(&char_data->inv, c->guildcard, pkt->slot)) {
            SQLERR_LOG("无法更新玩家数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        if (db_update_char_disp(&char_data->character.disp, c->guildcard, pkt->slot, flags)) {
            SQLERR_LOG("无法更新玩家数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        if (db_update_char_dress_data(&char_data->character.dress_data, c->guildcard, pkt->slot, flags)) {
            ERR_LOG("无法更新玩家更衣室数据至数据库 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        if (db_update_char_techniques(char_data->character.techniques, c->guildcard, pkt->slot, flags)) {
            ERR_LOG("无法更新玩家科技数据至数据库 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        /* 获取玩家角色背包数据数据项 */
        if (db_update_char_bank(&char_data->bank, c->guildcard, pkt->slot)) {
            ERR_LOG("无法更新玩家数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        /* 获取玩家角色背包数据数据项 */
        if (db_update_char_quest_data1(&char_data->quest_data1, c->guildcard, pkt->slot, flags)) {
            ERR_LOG("无法更新玩家数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }






        if (db_updata_bb_char_create_code(create_code,
            c->guildcard, pkt->slot)) {
            ERR_LOG("无法更新玩家更衣室数据至数据库 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        //DBG_LOG("保存角色数据");
        break;

    case PSOCN_DB_UPDATA_CHAR:
        /* 使用更衣室 */

        /* 复制数据,并赋值到结构 */

        char_data = db_get_uncompress_char_data(c->guildcard, pkt->slot);

        if (db_update_char_dress_data(&pkt->data.dress_data, c->guildcard, pkt->slot, flags)) {
            ERR_LOG("无法更新玩家更衣室数据至数据库 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        if (db_get_dress_data(c->guildcard, pkt->slot, &char_data->character.dress_data, flags)) {
            SQLERR_LOG("无法获取玩家外观数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        memcpy(char_data->character.name, &pkt->data.name, sizeof(char_data->character.name));

        if (db_update_char_disp(&char_data->character.disp, c->guildcard, pkt->slot, flags)) {
            SQLERR_LOG("无法更新玩家数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", c->guildcard, pkt->slot);
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        if (db_compress_char_data(char_data, sizeof(psocn_bb_db_char_t), c->guildcard, pkt->slot)) {
            SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):%s", CHARACTER, c->guildcard, pkt->slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            goto err;
        }

        //DBG_LOG("更新角色更衣室");
        break;

    default:

        DBG_LOG("未知角色指令 0x%08X ", flags);
        break;
    }

    if (db_update_char_dressflag(c->guildcard, 0)) {
        /* Should send an error message to the user */
        ERR_LOG("无法初始化更衣室 (GC %" PRIu32 ", 标签 %"
            PRIu8 ")", c->guildcard, c->flags);
        goto err;
    }
    
    /* Send state information down to the client to have them inform the
       server when they connect again... */
    c->sec_data.slot = pkt->slot;
    c->sec_data.sel_char = 1;
    c->flags = 0;

    //DBG_LOG("完成角色数据更新");

    if (send_bb_security(c, c->guildcard, 0, c->guild_id, &c->sec_data,
        sizeof(bb_client_config_pkt))) {
        goto err;
    }

    return send_bb_char_ack(c, pkt->slot, BB_CHAR_ACK_UPDATE);

err:
    free(char_data);
    return -1;
}

/* 0x00E7 231*/
static int handle_full_char(login_client_t* c, bb_full_char_pkt* pkt) {
    //char query[256];
    //char mysqlerr[1024];
    
    print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    printf("刷新玩家在线数据... ");

    if (db_update_gc_login_state(c->guildcard, c->islogged, c->sec_data.slot, (char*)pkt->data.character.name)) {
        return -1;
    }

    printf("完成!\n");
    return 0;
}

/* 0x00EC 236*/
static int handle_setflag(login_client_t* c, bb_setflag_pkt* pkt) {
    c->dress_flag = LE32(pkt->flags);
    //char query[256];
    time_t servertime = time(NULL);

    //DBG_LOG("更衣室标签 %d", pkt->flags);

    if (c->dress_flag == 0x02) {
        for (int ch = 0; ch < MAX_DRESS_FLAGS; ch++)
            if (dress_flags[ch].guildcard == 0)
            {
                dress_flags[ch].guildcard = c->guildcard;
                dress_flags[ch].flagtime = servertime;
                break;
                if (ch == (MAX_DRESS_FLAGS - 1))
                {
                    ERR_LOG("无法保存更衣室标签");
                    return -1;
                }
            }
    }

    if (db_update_char_dressflag(c->guildcard, c->dress_flag)) {
        /* Should send an error message to the user */
        ERR_LOG("无法初始化更衣室 (GC %" PRIu32 ", 标签 %"
            PRIu8 ")", c->guildcard, c->dress_flag);
        return -1;
    }

    return 0;
}

/* 0x01E8 488*/
static int handle_checksum(login_client_t *c, bb_checksum_pkt *pkt) {
    uint32_t ack_checksum = 1;

    if (!c->gc_data) {
        c->gc_data = (bb_guildcard_data_t*)malloc(sizeof(bb_guildcard_data_t));

        if (!c->gc_data) {
            ERR_LOG("无法分配GC数据的内存空间");
            ERR_LOG("%s", strerror(errno));
            return -1;
        }
    }

    ack_checksum = psocn_crc32((uint8_t*)c->gc_data, sizeof(bb_guildcard_data_t));

    //DBG_LOG("ack_checksum = %d %d", ack_checksum, pkt->checksum);

    /* XXXX: Do something with this some time... */
    return send_bb_checksum_ack(c, ack_checksum);
}

/* 0x03E8 1000*/
static int handle_guild_request(login_client_t *c) {
    char query[256];
    void *result;
    char **row;
    unsigned long *lengths;
    uint32_t checksum;
    int i = 0;
    uint32_t gc;

    if(!c->gc_data) {
        c->gc_data = (bb_guildcard_data_t *)malloc(sizeof(bb_guildcard_data_t));

        if(!c->gc_data) {
            /* XXXX: Should send an error message to the user */
            return -1;
        }
    }

    /* Clear it out */
    memset(c->gc_data, 0, sizeof(bb_guildcard_data_t));

    /* Query the DB for the user's guildcard data */
    sprintf(query, "SELECT friend_gc, name, guild_name, text, language, "
            "section_id, class, comment FROM %s WHERE "
            "guildcard='%" PRIu32 "' ORDER BY priority ASC", CLIENTS_GUILDCARDS, c->guildcard);

    if(psocn_db_real_query(&conn, query)) {
        /* Should send an error message to the user */
        SQLERR_LOG("无法查询到结果 guildcards (GC %" PRIu32 "):\n"
              "%s", c->guildcard, psocn_db_error(&conn));
        return -1;
    }

    if(!(result = psocn_db_result_store(&conn))) {
        /* Should send an error message to the user */
        SQLERR_LOG("无法存储 guildcard 结果 (GC %" PRIu32 "):\n"
              "%s", c->guildcard, psocn_db_error(&conn));
        return -1;
    }

    /* Fill in guildcard data */
    while((row = psocn_db_result_fetch(result)) && i < 105) {
        lengths = psocn_db_result_lengths(result);

        gc = (uint32_t)strtoul(row[0], NULL, 0);
        c->gc_data->entries[i].data.guildcard = LE32(gc);
        memcpy(c->gc_data->entries[i].data.name, row[1], MIN(48, lengths[1]));
        memcpy(c->gc_data->entries[i].data.guild_name, row[2], MIN(32, lengths[2]));
        memcpy(c->gc_data->entries[i].data.guildcard_desc, row[3], MIN(176, lengths[3]));
        memcpy(c->gc_data->entries[i].comment, row[7], MIN(176, lengths[7]));
        c->gc_data->entries[i].data.present = 1;
        c->gc_data->entries[i].data.language = (uint8_t)strtoul(row[4], NULL, 0);
        c->gc_data->entries[i].data.section = (uint8_t)strtoul(row[5], NULL, 0);
        c->gc_data->entries[i].data.ch_class = (uint8_t)strtoul(row[6], NULL, 0);

        ++i;
    }

    /* Clean up... */
    psocn_db_result_free(result);

    /* Query the DB for the user's blacklist data */
    sprintf(query, "SELECT blocked_gc, name, guild_name, text, language, "
            "section_id, class FROM %s WHERE guildcard='%"
            PRIu32 "' ORDER BY blocked_gc ASC", CHARACTER_BLACKLIST, c->guildcard);

    if(psocn_db_real_query(&conn, query)) {
        /* Should send an error message to the user */
        SQLERR_LOG("无法查询黑名单数据库 (GC %" PRIu32 "):"
              "%s", c->guildcard, psocn_db_error(&conn));
        return -1;
    }

    if(!(result = psocn_db_result_store(&conn))) {
        /* Should send an error message to the user */
        SQLERR_LOG("Couldn't store blacklist result (GC %" PRIu32 "):"
              "%s", c->guildcard, psocn_db_error(&conn));
        return -1;
    }

    /* Fill in blacklist data */
    i = 0;

    while((row = psocn_db_result_fetch(result)) && i < 30) {
        lengths = psocn_db_result_lengths(result);

        gc = (uint32_t)strtoul(row[0], NULL, 0);
        c->gc_data->black_list[i].guildcard = LE32(gc);
        memcpy(c->gc_data->black_list[i].name, row[1], MIN(48, lengths[1]));
        memcpy(c->gc_data->black_list[i].guild_name, row[2], MIN(32, lengths[2]));
        memcpy(c->gc_data->black_list[i].guildcard_desc, row[3], MIN(176, lengths[3]));
        c->gc_data->black_list[i].present = 1;
        c->gc_data->black_list[i].language = (uint8_t)strtoul(row[4], NULL, 0);
        c->gc_data->black_list[i].section = (uint8_t)strtoul(row[5], NULL, 0);
        c->gc_data->black_list[i].ch_class = (uint8_t)strtoul(row[6], NULL, 0);

        ++i;
    }

    /* Clean up... */
    psocn_db_result_free(result);

    /* Calculate the checksum, and send the header */
    checksum = psocn_crc32((uint8_t *)c->gc_data, sizeof(bb_guildcard_data_t));

    return send_bb_guild_header(c, checksum);
}

/* 0x04EB 1259*/
static int handle_param_hdr_req(login_client_t *c) {
    return send_bb_pkt(c, (bb_pkt_hdr_t *)param_hdr);
}

/* 0x03EB 1003*/
static int handle_param_chunk_req(login_client_t *c, bb_pkt_hdr_t *pkt) {
    uint32_t chunk = LE32(pkt->flags);

    if((int)chunk < num_param_chunks) {
        return send_bb_pkt(c, (bb_pkt_hdr_t *)param_chunks[chunk]);
    }

    return -1;
}

int process_bbcharacter_packet(login_client_t *c, void *pkt) {
    bb_pkt_hdr_t *bb = (bb_pkt_hdr_t *)pkt;
    uint16_t type = LE16(bb->pkt_type);

    //DBG_LOG("BB角色指令: 0x%04X %s", type, c_cmd_name(type, 0));

    //print_payload(pkt, LE16(bb->pkt_len));

    switch(type) {
            /* 0x0005 5*/
        case BURSTING_TYPE:
            //c->disconnected = 1;
            return handle_bb_burst(c, (bb_burst_pkt*)pkt);

            /* 0x0009 9*/
        case INFO_REQUEST_TYPE:
            return handle_info_req(c, (bb_select_pkt*)pkt);

            /* 0x0010 16*/
        case MENU_SELECT_TYPE:
            return handle_ship_select(c, (bb_select_pkt*)pkt);

            /* 0x001D 29*/
        case PING_TYPE:
            return handle_ignored_pkt(c, c_cmd_name(type, 0), pkt, __LINE__);

            /* 0x0093 147*/
        case LOGIN_93_TYPE:
            return handle_bb_login(c, (bb_login_93_pkt *)pkt);

            /* 0x03DC 988*/
        case BB_GUILDCARD_CHUNK_REQ_TYPE:
            return handle_guild_chunk(c, (bb_guildcard_req_pkt *)pkt);

            /* 0x00E0 224*/
        case BB_OPTION_REQUEST_TYPE:
            return handle_option_request(c, (bb_option_req_pkt *)pkt);

            /* 0x00E2 226*/
        case BB_OPTION_CONFIG_TYPE:
            return handle_option(c, (bb_opt_config_pkt *)pkt);

            /* 0x00E3 227*/
        case BB_CHARACTER_SELECT_TYPE:
            return handle_char_select(c, (bb_char_select_pkt *)pkt);

            /* 0x00E5 229*/
        case BB_CHARACTER_UPDATE_TYPE:
            return handle_update_char(c, (bb_char_preview_pkt *)pkt);

            /* 0x00E7 231*/
        case BB_FULL_CHARACTER_TYPE:
            /* Ignore these... they're meaningless and very broken when they
               manage to get sent to the login server... */
            return handle_full_char(c, (bb_full_char_pkt *)pkt);

            /* 0x00EC 236*/
        case BB_SETFLAG_TYPE:
            return handle_setflag(c, (bb_setflag_pkt *)pkt);

            //////////////
            //E8
            /* 0x01E8 488*/
        case BB_CHECKSUM_TYPE:
            return handle_checksum(c, (bb_checksum_pkt *)pkt);

            /* 0x03E8 1000*/
        case BB_GUILD_REQUEST_TYPE:
            return handle_guild_request(c);
            ///////////////

            //////////////
            //EB
            /* 0x03EB 1003*/
        case BB_PARAM_CHUNK_REQ_TYPE:
            return handle_param_chunk_req(c, (bb_pkt_hdr_t *)pkt);

            /* 0x04EB 1259*/
        case BB_PARAM_HEADER_REQ_TYPE:
            return handle_param_hdr_req(c);
            ///////////////

        default:
            UNK_CPD(type, c->version, pkt);
            //print_payload(pkt, LE16(bb->pkt_len));
            return -1;
    }
}

int load_param_data(void) {
    FILE *fp2;
    const char *fn;
    int i = 0, len;
    long filelen;
    int checksum, offset = 0;
    uint8_t *rawbuf;

    /* Allocate space for the buffer first */
    rawbuf = (uint8_t *)malloc(MAX_PARAMS_SIZE);
    if(!rawbuf) {
        ERR_LOG("无法分配参数缓冲区内存:\n%s",
              strerror(errno));
        return -1;
    }

    /* Allocate space for the parameter header */
    len = 0x08 + (NUM_PARAM_FILES * 0x4C);
    param_hdr = (bb_param_hdr_pkt *)malloc(len);
    if(!param_hdr) {
        ERR_LOG("无法分配参数标头内存:\n%s",
              strerror(errno));
        free(rawbuf);
        return -3;
    }

    param_hdr->hdr.pkt_type = LE16(BB_PARAM_HEADER_TYPE);
    param_hdr->hdr.pkt_len = LE16(len);
    param_hdr->hdr.flags = LE32(NUM_PARAM_FILES);

    /* Load each of the parameter files. */
    for(i = 0; i < NUM_PARAM_FILES; ++i) {
        char dir[36] = { 0 };
        char fn2[256] = { 0 };
        fn = param_files[i];
        if (i < 2) {
            sprintf_s(dir, sizeof(dir), "System\\BlueBurst\\item\\");
        }
        else if (i > 7) {
            sprintf_s(dir, sizeof(dir), "System\\Player\\leveltbl\\");
        }
        else {
            sprintf_s(dir, sizeof(dir), "System\\BlueBurst\\param\\");
        }

        strcpy(fn2, dir);
        strcat(fn2, fn);

        AUTH_LOG("读取参数文件: %s", fn2);

        errno_t err = fopen_s(&fp2, fn2, "rb");

        if(err) {
            ERR_LOG("无法打开参数文件: %s", fn2);
            if (fp2) {
                fclose(fp2);
            }
            free(rawbuf);
            return -3;
        }

        /* Figure out how long it is, and make sure its not going to overflow
           the buffer */
        fseek(fp2, 0, SEEK_END);
        filelen = ftell(fp2);
        fseek(fp2, 0, SEEK_SET);

        if(filelen > 0x10000) {
            ERR_LOG("参数文件 %s 数据过长 (%l)", fn2, filelen);
            fclose(fp2);
            free(rawbuf);
            return -3;
        }

        /* Make sure we aren't going over the max size... */
        if(filelen + offset > MAX_PARAMS_SIZE) {
            ERR_LOG("参数文件缓冲区溢出 %s", fn2);
            fclose(fp2);
            free(rawbuf);
            return -3;
        }

        /* Read it in */
        fread(rawbuf + offset, 1, filelen, fp2);
        fclose(fp2);

        /* Fill in the stuff in the header first */
        checksum = psocn_crc32(rawbuf + offset, filelen);

        param_hdr->entries[i].size = LE32(((uint32_t)filelen));
        param_hdr->entries[i].checksum = LE32(checksum);
        param_hdr->entries[i].offset = LE32(offset);
        strncpy(param_hdr->entries[i].filename, fn, 0x40);

        offset += filelen;
    }

    /* Now that the header is built, time to make the chunks */
    num_param_chunks = offset / 0x6800;

    if(offset % 0x6800) {
        ++num_param_chunks;
    }

    /* Allocate space for the array of chunks */
    param_chunks = (bb_param_chunk_pkt **)malloc(sizeof(bb_param_chunk_pkt *) *
                                                 num_param_chunks);
    if(!param_chunks) {
        ERR_LOG("Couldn't make param chunk array: %s",
              strerror(errno));
        free(rawbuf);
        return -4;
    }

    /* Scrub it, for safe-keeping */
    memset(param_chunks, 0, sizeof(bb_param_chunk_pkt *) * num_param_chunks);

    for(i = 0; i < num_param_chunks; ++i) {
        if(offset < (i + 1) * 0x6800) {
            len = (offset % 0x6800) + 0x0C;
        }
        else {
            len = 0x680C;
        }

        param_chunks[i] = (bb_param_chunk_pkt *)malloc(len);

        if(!param_chunks[i]) {
            ERR_LOG("Couldn't make chunk: %s", strerror(errno));
            free(rawbuf);
            return -5;
        }

        /* Fill in the chunk */
        param_chunks[i]->hdr.pkt_type = LE16(BB_PARAM_CHUNK_TYPE);
        param_chunks[i]->hdr.pkt_len = LE16(len);
        param_chunks[i]->hdr.flags = 0;
        param_chunks[i]->chunk_index = LE32(i);
        memcpy(param_chunks[i]->data, rawbuf + (i * 0x6800), len - 0x0C);
    }

    /* Cleanup time */
    free(rawbuf);
    //chdir("../..");
    AUTH_LOG("读取 %" PRIu32 " 个参数文件至 %d 区块", NUM_PARAM_FILES,
          num_param_chunks);

    return 0;
}

void cleanup_param_data(void) {
    int i;

    for(i = 0; i < num_param_chunks; ++i) {
        free(param_chunks[i]);
    }

    free(param_chunks);
    param_chunks = NULL;
    num_param_chunks = 0;
    free(param_hdr);
    param_hdr = NULL;
}

int load_bb_char_data(void) {
    FILE* fp;
    long templen = 0, len = 0;
    errno_t err = 0;
    int i;
    char file[64];
    psocn_bb_db_char_t* cur = { 0 };

    const char* path = "System\\Player\\character";

    /* 加载newserv角色数据文件 */
    for (i = 0; i < 12; ++i) {
        memset(&default_chars[i], 0, sizeof(psocn_bb_db_char_t));//必须初始化为0

        cur = &default_chars[i];

        sprintf(file, "%s\\%s", path, pso_class[i].class_file);

        err = fopen_s(&fp , file, "rb");

        if (err) {
            ERR_LOG("初始 %s 角色数据文件不存在",
                pso_class[i].class_file);
            return -1;
        }

        /* Skip over the parts we don't care about, then read in the data. */
        fseek(fp, 0, SEEK_END);
        templen = ftell(fp);
        fseek(fp, 0x40 + sizeof(psocn_bb_mini_char_t), SEEK_SET);
        fread(cur->autoreply, 1, 344, fp);
        fread(&cur->bank, 1, sizeof(psocn_bank_t), fp);
        fread(cur->challenge_data, 1, 320, fp);
        fread(&cur->character, 1, sizeof(psocn_bb_char_t), fp);
        fread(cur->guildcard_desc, 1, 176, fp);
        fread(cur->infoboard, 1, 344, fp);
        fread(&cur->inv, 1, sizeof(inventory_t), fp);
        fread(&cur->quest_data1, 1, sizeof(psocn_quest_data1_t), fp);
        fread(cur->quest_data2, 1, 88, fp);
        fread(cur->tech_menu, 1, 40, fp);
        fclose(fp);
        len += templen;
    }

    AUTH_LOG("读取初始角色数据文件 %d 个, 共 %d 字节.", i, len);

    /* 加载角色完整数据 */

    sprintf(file, "%s\\%s", path, &pso_class[CLASS_FULL_CHAR].class_file);

    err = fopen_s(&fp, file, "rb");
    if (err)
    {
        ERR_LOG("文件 %s 缺失!", file);
        //return -1;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    /* 跳过数据头 长度 0x399C 14748*/
    fseek(fp, sizeof(bb_pkt_hdr_t), SEEK_SET);

    if (!fread(&default_full_chars, 1, sizeof(psocn_bb_full_char_t), fp))
    {
        ERR_LOG("读取 %s 数据失败!", file);
        //return -1;
    }

    AUTH_LOG("读取完整角色数据表,共 %d 字节.", len);

    fclose(fp);

    /* 加载角色等级初始数据 */
    if (read_player_level_table_bb(&bb_char_stats)) {
        ERR_LOG("无法读取 Blue Burst 等级数据表");
        return -2;
    }

    /* Read the stats table */
    AUTH_LOG("读取等级数据表,共 %d 字节.", sizeof(bb_char_stats));

    return 0;
}

