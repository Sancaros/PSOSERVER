/*
    梦幻之星中国 认证服务器 数据库操作
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

#include "database.h"
#include "database_query.h"
#include "packets_bb.h"

/* 数据库连接 */
extern psocn_dbconn_t conn;

int db_upload_temp_data(void* data, size_t size) {

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO temp_t (temp_data)"
        " VALUES ('");

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)data, size);

    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法插入数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_character_default(psocn_bb_db_char_t* data, int index) {
    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "update_time = NOW(), inventory = '", CHARACTER_DEFAULT);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->inv,
        PSOCN_STLENGTH_INV);

    strcat(myquery, "', `character` = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->character,
        PSOCN_STLENGTH_BB_CHAR);

    strcat(myquery, "', quest_data1 = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_data1,
        PSOCN_STLENGTH_BB_DB_QUEST_DATA1);

    strcat(myquery, "', bank = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->bank,
        PSOCN_STLENGTH_BANK);

    strcat(myquery, "', guildcard_desc = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->guildcard_desc,
        88);

    strcat(myquery, "', autoreply = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->autoreply,
        172);

    strcat(myquery, "', infoboard = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->infoboard,
        172);

    strcat(myquery, "', challenge_data = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->challenge_data,
        PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

    strcat(myquery, "', tech_menu = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->tech_menu,
        PSOCN_STLENGTH_BB_DB_TECH_MENU);

    strcat(myquery, "', quest_data2 = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_data2,
        PSOCN_STLENGTH_BB_DB_QUEST_DATA2);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "' WHERE `index` = %d", index);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_get_character_default(psocn_bb_db_char_t* data, int index) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "SELECT "
        "inventory, `character`, quest_data1, bank, guildcard_desc, "
        "autoreply, infoboard, challenge_data, tech_menu, quest_data2 "
        "FROM %s "
        "WHERE `index` = %d", CHARACTER_DEFAULT, index);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法获取数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    result = psocn_db_result_store(&conn);
    if (result == NULL) {
        SQLERR_LOG("无法获取查询结果");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    if (psocn_db_result_rows(result) < 1) {
        SQLERR_LOG("未找到匹配的记录");
        return -1;
    }

    row = psocn_db_result_fetch(result);
    if (row == NULL) {
        SQLERR_LOG("无法获取查询结果集");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    memcpy((char*)&data->inv, row[0], PSOCN_STLENGTH_INV);
    memcpy((char*)&data->character, row[1], PSOCN_STLENGTH_BB_CHAR);
    memcpy((char*)&data->quest_data1, row[2], PSOCN_STLENGTH_BB_DB_QUEST_DATA1);
    memcpy((char*)&data->bank, row[3], PSOCN_STLENGTH_BANK);
    memcpy((char*)&data->guildcard_desc, row[4], 88);
    memcpy((char*)&data->autoreply, row[5], 172);
    memcpy((char*)&data->infoboard, row[6], 172);
    memcpy((char*)&data->challenge_data, row[7], PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);
    memcpy((char*)&data->tech_menu, row[8], PSOCN_STLENGTH_BB_DB_TECH_MENU);
    memcpy((char*)&data->quest_data2, row[9], PSOCN_STLENGTH_BB_DB_QUEST_DATA2);

    return 0;
}

int db_insert_character_default(psocn_bb_db_char_t* data, int index, char* class_name) {

    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "`index`, class_name, "
        "inventory, `character`, quest_data1, bank, guildcard_desc, "
        "autoreply, infoboard, challenge_data, tech_menu, quest_data2"
        ") VALUES ("
        "'%d', '%s', '",
        CHARACTER_DEFAULT,
        index, class_name
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->inv,
        PSOCN_STLENGTH_INV);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->character,
        PSOCN_STLENGTH_BB_CHAR);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_data1,
        PSOCN_STLENGTH_BB_DB_QUEST_DATA1);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->bank,
        PSOCN_STLENGTH_BANK);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->guildcard_desc,
        88);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->autoreply,
        172);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->infoboard,
        172);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->challenge_data,
        PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->tech_menu,
        PSOCN_STLENGTH_BB_DB_TECH_MENU);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_data2,
        PSOCN_STLENGTH_BB_DB_QUEST_DATA2);

    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法插入数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_char_auth_msg(char ipstr[INET6_ADDRSTRLEN], uint32_t gc, uint8_t slot) {
    //char query[256];

    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    char timestamp[28];
    sprintf(timestamp, "%u-%02u-%02u %02u:%02u:%02u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond);

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET ip = '%s', last_login_time = '%s'"
        "WHERE guildcard = '%" PRIu32 "' AND slot = '%"PRIu8"'", CHARACTER, ipstr, timestamp, gc, slot);
    if (psocn_db_real_query(&conn, myquery))
    {
        SQLERR_LOG("无法更新角色登录数据 (GC %" PRIu32 ", 槽位 %"
            PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_char_dressflag(uint32_t gc, uint32_t flags) {

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "UPDATE %s SET dressflag = '%" PRIu32 "' WHERE "
        "guildcard = '%" PRIu32 "'", AUTH_ACCOUNT, flags, gc);

    if (psocn_db_real_query(&conn, myquery)) {
        /* Should send an error message to the user */
        SQLERR_LOG("无法初始化更衣室 (GC %" PRIu32 ", 标签 %"
            PRIu8 "):\n%s", gc, flags,
            psocn_db_error(&conn));
        return -1;
    }
    return 0;
}

int db_update_auth_server_list(psocn_srvconfig_t* cfg) {

    sprintf_s(myquery, _countof(myquery), "DELETE FROM %s", AUTH_SERVER_LIST);
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", AUTH_SERVER_LIST);
        exit(EXIT_FAILURE);
    }

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO %s "
        "(id, name, host4, "
        "host6, port, authflags, "
        "timezone, allowedSecurityLevel, authbuilds)"
        " VALUES ("
        "'%d', '%s', '%s', "
        "'%s', '%d', '%d', "
        "'%d', '%d', '%s'"
        ")", AUTH_SERVER_LIST,
        1, "PSOSERVER", cfg->host4,
        cfg->host6, 12000, 1,
        1, 0, "0.00.01");

    if (psocn_db_real_query(&conn, myquery)) {
        /* Should send an error message to the user */
        SQLERR_LOG("无法初始化认证服务器:\n%s",
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

psocn_srvconfig_t db_get_auth_server_list(uint32_t id) {
    void* result;
    char** row;
    psocn_srvconfig_t cfg;

    memset(&cfg, 0, sizeof(psocn_srvconfig_t));

    memset(myquery, 0, sizeof(myquery));

    /* Look up the user's saved config */
    sprintf(myquery, "SELECT host4, host6, "
        "port, authflags, timezone, allowedSecurityLevel, authbuilds FROM %s WHERE "
        "id = '%" PRIu32 "'", AUTH_SERVER_LIST, id);

    if (!psocn_db_real_query(&conn, myquery)) {

        result = psocn_db_result_store(&conn);

        if (!result) {
            SQLERR_LOG("无法获取 %s 数据 "
                "id %" PRIu32 "", AUTH_SERVER_LIST, id);
            return cfg;
        }

        row = psocn_db_result_fetch(result);
        /* See if we got a hit... */
        if (!row) {
            SQLERR_LOG("无法获取 %s 数据 "
                "id %" PRIu32 "", AUTH_SERVER_LIST, id);
            return cfg;
        }

        cfg.host4 = row[0];
        cfg.host6 = row[1];

        psocn_db_next_result_free(&conn, result);
    }

    return cfg;
}
