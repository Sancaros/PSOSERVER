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

//是否与 db_update_bb_char_guild 函数重叠了？？？？？
int db_updata_bb_char_guild_data(uint32_t guild_id, uint32_t gc) {

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
        "guild_id = '%" PRIu32"' "
        "WHERE guildcard = '%" PRIu32 "'", CLIENTS_GUILD, 
        guild_id, 
        gc);
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新角色 %s 公会数据!", CLIENTS_GUILD);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

/* 获取 BB 的公会数据 */
psocn_bb_db_guild_t db_get_bb_char_guild(uint32_t gc) {
    void* result;
    char** row;
    psocn_bb_db_guild_t guild;
    uint32_t guild_id, guild_priv_level;

    memset(&guild, 0, sizeof(bb_guild_t));

    memset(myquery, 0, sizeof(myquery));

    /* 查询数据表 */
    sprintf(myquery, "SELECT guild_id, guild_priv_level FROM %s WHERE "
        "guildcard = '%" PRIu32 "'", AUTH_ACCOUNT, gc);

    if (!psocn_db_real_query(&conn, myquery)) {

        result = psocn_db_result_store(&conn);
        if (psocn_db_result_rows(result)) {
            row = psocn_db_result_fetch(result);
            guild_id = (uint32_t)strtoul(row[0], NULL, 0);
            guild_priv_level = (uint32_t)strtoul(row[1], NULL, 0);
            psocn_db_result_free(result);
        }
        else {
            psocn_db_result_free(result);
            return guild; 
        }

        //DBG_LOG("%d %d %d", gc, guild_id, guild_priv_level);

        if (guild_id != -1) {

            /* 同步角色账号与角色公会数据的公会ID与公会等级 */
            if (db_updata_bb_char_guild_data(guild_id, gc))
                return guild;

            /* 查询数据表 */
            sprintf(myquery, "SELECT * FROM %s WHERE "
                "guild_id = '%" PRIu32 "'", CLIENTS_GUILD, guild_id);

            if (!psocn_db_real_query(&conn, myquery)) {
                result = psocn_db_result_store(&conn);
                /* 查找是否有数据 */
                if (psocn_db_result_rows(result)) {
                    row = psocn_db_result_fetch(result);
                    guild.guild_data.guildcard = atoi(row[0])/*(uint32_t)strtoul(row[0], NULL, 0)*/;
                    guild.guild_data.guild_id = atoi(row[1])/*(uint32_t)strtoul(row[1], NULL, 0)*/;
                    memcpy(&guild.guild_data.guild_info, row[2], sizeof(guild.guild_data.guild_info));
                    //memcpy(&guild.guild_data.guild_priv_level, row[3], sizeof(guild.guild_data.guild_priv_level));
                    guild.guild_data.guild_priv_level = guild_priv_level/*(uint32_t)strtoul(row[3], NULL, 0)*/;
                    //memcpy(&guild.guild_data.reserved, row[4], sizeof(guild.guild_data.reserved));

                    /* 赋予名称颜色代码 */
                    guild.guild_data.guild_name[0] = 0x0009;
                    guild.guild_data.guild_name[1] = 0x0045;
                    memcpy(&guild.guild_data.guild_name, row[4], sizeof(guild.guild_data.guild_name) - 4);

                    /* TODO 公会等级未实现 */
                    guild.guild_data.guild_rank = atoi(row[5])/*(uint32_t)strtoul(row[5], NULL, 0)*/;

                    memcpy(&guild.guild_data.guild_flag, row[6], sizeof(guild.guild_data.guild_flag));
                    guild.guild_data.guild_rewards[0] = atoi(row[7])/*(uint32_t)strtoul(row[7], NULL, 0)*/;
                    guild.guild_data.guild_rewards[1] = atoi(row[8])/*(uint32_t)strtoul(row[8], NULL, 0)*/;
                }

                psocn_db_result_free(result);
            }
            else {
                /* 初始化默认数据 */
                guild.guild_data.guildcard = gc;
                guild.guild_data.guild_id = guild_id;
                guild.guild_data.guild_priv_level = guild_priv_level;

                //guild.guild_data.guild_rank = 0x00986C84; // ?? 应该是排行榜未完成的参数了

                guild.guild_data.guild_rank = 0x00000000; // ?? 应该是排行榜未完成的参数了

                /* TODO 其他数据未获得初始数据 可以从默认的完整角色数据中获取初始数据*/
                guild.guild_data.guild_rewards[0] = 0xFFFFFFFF; //和更衣室有关
                guild.guild_data.guild_rewards[1] = 0xFFFFFFFF;

                sprintf(myquery, "INSERT INTO %s (guildcard, guild_id, guild_priv_level, "
                    "guild_rank, guild_rewards1, guild_rewards2, "
                    "guild_info, guild_name, guild_flag)"
                    " VALUES ('%" PRIu32"', '%d', '%d', "
                    "'%d', '%d', '%d', '", CLIENTS_GUILD,
                    guild.guild_data.guildcard, guild.guild_data.guild_id, guild.guild_data.guild_priv_level,
                    guild.guild_data.guild_rank, guild.guild_data.guild_rewards[0], guild.guild_data.guild_rewards[1]);

                psocn_db_escape_str(&conn, myquery + strlen(myquery),
                    (char*)&guild.guild_data.guild_info, sizeof(guild.guild_data.guild_info));

                strcat(myquery, "', '");

                psocn_db_escape_str(&conn, myquery + strlen(myquery),
                    (char*)&guild.guild_data.guild_name, sizeof(guild.guild_data.guild_name));

                strcat(myquery, "', '");

                psocn_db_escape_str(&conn, myquery + strlen(myquery),
                    (char*)&guild.guild_data.guild_flag, sizeof(guild.guild_data.guild_flag));

                strcat(myquery, "')");

                //printf("%s \n", myquery);

                if (psocn_db_real_query(&conn, myquery)) {
                    SQLERR_LOG("无法插入设置数据 "
                        "guildcard %" PRIu32 "", gc);
                    SQLERR_LOG("%s", psocn_db_error(&conn));
                }
            }
        }
    }

    return guild;
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
