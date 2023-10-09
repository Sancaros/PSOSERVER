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

int db_upload_temp_data(void* data, size_t size) {

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO temp_t (temp_data)"
        " VALUES ('");

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)data, size);

    //strcat(myquery, "')");

    int remaining_size = sizeof(myquery) - strlen(myquery) - 1; // 计算剩余可用长度

    // 格式化拼接字符串，并确保不会溢出缓冲区
    int n = snprintf(myquery + strlen(myquery), remaining_size, "')");

    if (n < 0 || n >= remaining_size) {
        SQLERR_LOG("无法写入myquery数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法插入数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
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
    psocn_srvconfig_t cfg = { 0 };

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
