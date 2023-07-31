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

//是否与 db_update_bb_char_guild 函数重叠了？？？？？
static int db_updata_bb_char_guild_data(uint32_t guild_id, uint32_t gc) {

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
        "guild_id = '%" PRIu32"' "
        "WHERE guildcard = '%" PRIu32 "'"
        , CLIENTS_GUILD
        , guild_id
        , gc);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新角色 %s 公会数据!", CLIENTS_GUILD);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_del_guild_data(uint32_t guild_id) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "DELETE FROM %s"
        " WHERE "
        "guild_id = '%" PRIu32 "'"
        , CLIENTS_GUILD
        , guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理旧公会数据 (GUILD ID %"
            PRIu32 "):\n%s", guild_id,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

static int db_update_account_guild_data(uint32_t guild_id, uint32_t new_guild_id, uint32_t new_guild_priv_level) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s"
        " SET "
        "guild_id = '%" PRIu32 "', guild_priv_level = '%" PRIu32 "'"
        " WHERE "
        "guild_id = '%" PRIu32 "'"
        , AUTH_ACCOUNT
        , new_guild_id, new_guild_priv_level
        , guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新账号公会数据 (GUILD ID %"
            PRIu32 "):\n%s", guild_id,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

static int db_initialize_account_guild_data(uint32_t guild_id) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s"
        " SET "
        "guild_id = '-1', "
        "guild_priv_level = '0', "
        "guild_points_personal_donation = '0'"
        " WHERE "
        "guild_id = '%" PRIu32 "'"
        , AUTH_ACCOUNT
        , guild_id
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法初始化账号公会数据 (GUILD ID %"
            PRIu32 "):\n%s", guild_id,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

/* 获取 BB 的公会数据 */
psocn_bb_db_guild_t db_get_bb_char_guild(uint32_t gc) {
    void* result;
    char** row;
    psocn_bb_db_guild_t guild = { 0 };
    uint32_t guild_id, guild_priv_level;

    memset(myquery, 0, sizeof(myquery));

    /* 查询数据表 */
    sprintf(myquery, "SELECT "
        "guild_id, guild_priv_level"
        " FROM %s WHERE "
        "guildcard = '%" PRIu32 "'", 
        AUTH_ACCOUNT, 
        gc
    );

    if (!psocn_db_real_query(&conn, myquery)) {

        result = psocn_db_result_store(&conn);
        if (psocn_db_result_rows(result)) {
            row = psocn_db_result_fetch(result);
            guild_id = (uint32_t)strtoul(row[0], NULL, 10);
            guild_priv_level = (uint32_t)strtoul(row[1], NULL, 10);
            psocn_db_result_free(result);
        }
        else {
            psocn_db_result_free(result);
            return guild;
        }

#ifdef DEBUG
        DBG_LOG("%u %d %d", gc, guild_id, guild_priv_level);
#endif // DEBUG

        if (guild_id != -1) {

            /* 同步角色账号与角色公会数据的公会ID与公会等级 */
            if (db_updata_bb_char_guild_data(guild_id, gc))
                return guild;

            /* 查询数据表 */
            sprintf(myquery, "SELECT "
                "guildcard, guild_id, guild_points_rank, guild_points_rest, "
                "guild_name, guild_rank, guild_flag, "
                "guild_reward0, guild_reward1, guild_reward2, guild_reward3, "
                "guild_reward4, guild_reward5, guild_reward6, guild_reward7"
                " FROM "
                "%s"
                " WHERE "
                "guild_id = '%" PRIu32 "'", 
                CLIENTS_GUILD, 
                guild_id
            );

            if (!psocn_db_real_query(&conn, myquery)) {
                result = psocn_db_result_store(&conn);
                /* 查找是否有数据 */
                if (psocn_db_result_rows(result)) {
                    row = psocn_db_result_fetch(result);
                    guild.data.guild_owner_gc = (uint32_t)strtoul(row[0], NULL, 10);
                    guild.data.guild_id = (uint32_t)strtoul(row[1], NULL, 10);
                    guild.data.guild_points_rank = (uint32_t)strtoul(row[2], NULL, 10);
                    guild.data.guild_points_rest = (uint32_t)strtoul(row[3], NULL, 10);
                    guild.data.guild_priv_level = guild_priv_level;
                    /* 赋予名称颜色代码 */
                    guild.data.guild_name[0] = 0x0009;
                    guild.data.guild_name[1] = 0x0045;
                    memcpy(&guild.data.guild_name, row[4], sizeof(guild.data.guild_name) - 4);

                    /* TODO 公会等级未实现 */
                    guild.data.guild_rank = (uint32_t)strtoul(row[5], NULL, 10);

                    memcpy(&guild.data.guild_flag, row[6], sizeof(guild.data.guild_flag));
                    guild.data.guild_reward[0] = (uint8_t)strtoul(row[7], NULL, 10);
                    guild.data.guild_reward[1] = (uint8_t)strtoul(row[8], NULL, 10);
                    guild.data.guild_reward[2] = (uint8_t)strtoul(row[9], NULL, 10);
                    guild.data.guild_reward[3] = (uint8_t)strtoul(row[10], NULL, 10);
                    guild.data.guild_reward[4] = (uint8_t)strtoul(row[11], NULL, 10);
                    guild.data.guild_reward[5] = (uint8_t)strtoul(row[12], NULL, 10);
                    guild.data.guild_reward[6] = (uint8_t)strtoul(row[13], NULL, 10);
                    guild.data.guild_reward[7] = (uint8_t)strtoul(row[14], NULL, 10);

                }

                psocn_db_result_free(result);
            }
            else {
                /* 初始化默认数据 */
                guild.data.guild_owner_gc = gc;
                guild.data.guild_id = guild_id;
                guild.data.guild_points_rank = 0;
                guild.data.guild_points_rest = 0;
                guild.data.guild_priv_level = guild_priv_level;
                /* 赋予名称颜色代码 */
                guild.data.guild_name[0] = 0x0009;
                guild.data.guild_name[1] = 0x0045;
                memcpy(&guild.data.guild_name, L"公会数据错误", 14);

                //guild.data.guild_rank = 0; // ?? 应该是排行榜未完成的参数了

                //memset(&guild.data.guild_flag, 0, sizeof(guild.data.guild_flag));

                ///* TODO 其他数据未获得初始数据 可以从默认的完整角色数据中获取初始数据*/
                //guild.data.guild_reward[0] = 0;
                //guild.data.guild_reward[1] = 0;
                //guild.data.guild_reward[2] = 0;
                //guild.data.guild_reward[3] = 0;
                //guild.data.guild_reward[4] = 0;
                //guild.data.guild_reward[5] = 0;
                //guild.data.guild_reward[6] = 0;
                //guild.data.guild_reward[7] = 0;

                sprintf(myquery, "INSERT INTO %s ("
                    "guildcard, guild_id, guild_points_rank, guild_points_rest, guild_priv_level, "
                    "guild_rank, "
                    "guild_reward0, guild_reward1, guild_reward2, guild_reward3, "
                    "guild_reward4, guild_reward5, guild_reward6, guild_reward7, "
                    "guild_points_rank, guild_points_rest, "
                    "guild_name, guild_flag"
                    ") VALUES ("
                    "'%" PRIu32"', '%" PRIu32"', '%" PRIu32"', "
                    "'%" PRIu32"', "
                    "'%" PRIu8"', '%" PRIu8"', '%" PRIu8"', '%" PRIu8"', "
                    "'%" PRIu8"', '%" PRIu8"', '%" PRIu8"', '%" PRIu8"', "
                    "'%" PRIu32"', '%" PRIu32"', '"
                    ,
                    CLIENTS_GUILD,
                    guild.data.guild_owner_gc, guild.data.guild_id, guild.data.guild_priv_level,
                    guild.data.guild_rank,
                    guild.data.guild_reward0, guild.data.guild_reward1, guild.data.guild_reward2, guild.data.guild_reward3,
                    guild.data.guild_reward4, guild.data.guild_reward5, guild.data.guild_reward6, guild.data.guild_reward7,
                    guild.data.guild_points_rank, guild.data.guild_points_rest
                );

                psocn_db_escape_str(&conn, myquery + strlen(myquery),
                    (char*)&guild.data.guild_name, sizeof(guild.data.guild_name));

                strcat(myquery, "', '");

                psocn_db_escape_str(&conn, myquery + strlen(myquery),
                    (char*)&guild.data.guild_flag, sizeof(guild.data.guild_flag));

                strcat(myquery, "')");

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

int db_get_bb_char_guild_id(uint32_t gc) {
    int guild_id = -1;
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "SELECT guild_id FROM %s WHERE guildcard='%u'", AUTH_ACCOUNT, gc);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -3;
    }

    guild_id = (int)strtol(row[0], NULL, 10);

    psocn_db_result_free(result);

    return guild_id;
}

uint32_t db_get_bb_char_guild_priv_level(uint32_t gc) {
    uint32_t guild_priv_level = -1;
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "SELECT guild_priv_level FROM %s WHERE guildcard='%u'", AUTH_ACCOUNT, gc);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -3;
    }

    guild_priv_level = (uint32_t)strtoul(row[0], NULL, 10);

    psocn_db_result_free(result);

    return guild_priv_level;
}

int db_insert_bb_char_guild(uint16_t* guild_name, uint8_t* default_guild_flag, uint32_t gc, uint32_t version) {
    void* result;
    //char** row;
    bb_guild_t* g_data;
    uint8_t create_res;
    uint32_t guild_exists = 0, guild_id;
    char guild_name_text[24];

    g_data = (bb_guild_t*)malloc(PSOCN_STLENGTH_BB_GUILD);

    if (!g_data) {
        ERR_LOG("无法分配公会数据的内存空间");
        ERR_LOG("%s", strerror(errno));
        return -1;
    }

    memcpy(g_data->guild_name, guild_name, sizeof(g_data->guild_name));
    memcpy(g_data->guild_flag, default_guild_flag, sizeof(g_data->guild_flag));

    istrncpy16_raw(ic_utf16_to_utf8, guild_name_text, &g_data->guild_name[2], 24, 12);

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "SELECT * FROM %s WHERE guild_name_text = '%s'",
        CLIENTS_GUILD, guild_name_text);
    if (!psocn_db_real_query(&conn, myquery))
    {
        result = psocn_db_result_store(&conn);
        guild_exists = (uint32_t)psocn_db_result_rows(result);
        psocn_db_result_free(result);
    }
    else
        create_res = 1;

    if (!guild_exists)
    {
        memset(myquery, 0, sizeof(myquery));

        // It doesn't... but it will now. :)
        sprintf_s(myquery, _countof(myquery), "INSERT INTO %s "
            "(guildcard, version, guild_name_text, guild_name, guild_flag)"
            " VALUES "
            "('%" PRIu32 "', '%u', '%s', '",
            CLIENTS_GUILD,
            gc, version, guild_name_text);

        psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&g_data->guild_name,
            sizeof(g_data->guild_name));

        strcat(myquery, "', '");

        psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&g_data->guild_flag,
            sizeof(g_data->guild_flag));

        strcat(myquery, "')");

        if (!psocn_db_real_query(&conn, myquery))
        {
            memset(myquery, 0, sizeof(myquery));

            guild_id = (uint32_t)psocn_db_insert_id(&conn);
            sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
                "guild_id = '%u', guild_priv_level = '%u'"
                " WHERE "
                "guildcard = '%" PRIu32 "'"
                , AUTH_ACCOUNT
                , guild_id, 0x00000040
                , gc
            );

            if (psocn_db_real_query(&conn, myquery))
                create_res = 1;
            else
                create_res = 0;
        }
        else
            create_res = 1;
    }
    else
        create_res = 2;

    free_safe(g_data);

    return create_res;
}

int db_update_bb_guild_member_add(uint32_t guild_id, uint32_t gc_target) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
        "guild_id = '%" PRIu32"',guild_priv_level = '0' WHERE guildcard = '%" PRIu32"'",
        AUTH_ACCOUNT, guild_id, gc_target);
    if (!psocn_db_real_query(&conn, myquery))
        return 0;

    return 1;
}

int db_update_bb_guild_flag(uint8_t* guild_flag, uint32_t guild_id) {
    memset(myquery, 0, sizeof(myquery));
    uint8_t flag_slashes[4098] = { 0 };

    psocn_db_escape_str(&conn, &flag_slashes[0], guild_flag, 0x800);

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET guild_flag = '%s' WHERE guild_id = '%" PRIu32"'",
        CLIENTS_GUILD, (char*)&flag_slashes[0], guild_id);

    if (!psocn_db_real_query(&conn, myquery))
        return 0;

    return 1;
}

int db_dissolve_bb_guild(uint32_t guild_id) {

    if (db_del_guild_data(guild_id))
        return 1;

    if (db_initialize_account_guild_data(guild_id))
        return 2;

    return 0;
}

int db_update_bb_char_guild(psocn_bb_db_guild_t guild, uint32_t gc) {
    //DBG_LOG("更新 guild 设置");

    memset(myquery, 0, sizeof(myquery));

    /* Build the db query */
    sprintf(myquery, "UPDATE %s SET guild_name = '", CLIENTS_GUILD);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&guild.data.guild_name,
        sizeof(guild.data.guild_name));

    strcat(myquery, "', guild_flag = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&guild.data.guild_flag,
        sizeof(guild.data.guild_flag));

    sprintf(myquery + strlen(myquery),
        "', guildcard = '%" PRIu32"', guild_id = '%" PRIu32"', guild_priv_level = '%" PRIu32"'"
        ", guild_rank = '%" PRIu32"', guild_reward0 = '%" PRIu8"', guild_reward1 = '%" PRIu8"', guild_reward2 = '%" PRIu8"'"
        ", guild_reward3 = '%" PRIu8"', guild_reward4 = '%" PRIu8"', guild_reward5 = '%" PRIu8"', guild_reward6 = '%" PRIu8"'"
        ", guild_reward7 = '%" PRIu8"'"
        " WHERE "
        "guildcard = '%" PRIu32 "'",
        guild.data.guild_owner_gc, guild.data.guild_id, guild.data.guild_priv_level,
        guild.data.guild_rank, guild.data.guild_reward0, guild.data.guild_reward1, guild.data.guild_reward2, 
        guild.data.guild_reward3, guild.data.guild_reward4, guild.data.guild_reward5, guild.data.guild_reward6, 
        guild.data.guild_reward7, 
        gc
    );

    /* Execute the query */
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新角色 %s 公会数据!", CLIENTS_GUILD);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_bb_guild_ranks(psocn_dbconn_t* conn) {

    /* 在mysql中初始化一个rank变量 */
    if (psocn_db_real_query(conn, "SET @rank := 0")) {
        SQLERR_LOG("无法清空 %s 公会排行榜数据!", CLIENTS_GUILD);
        SQLERR_LOG("%s", psocn_db_error(conn));
        return -1;
    }

    /* 执行更新排行榜语句 */
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "UPDATE %s AS cg"
        " JOIN ( SELECT "
        "guild_id, @rank := @rank + 1 AS new_rank"
        " FROM "
        "%s"
        " ORDER BY guild_points_rank DESC ) AS ranks ON "
        "cg.guild_id = ranks.guild_id"
        " SET "
        "cg.guild_rank = ranks.new_rank"
        , CLIENTS_GUILD
        , CLIENTS_GUILD
    );

    if (psocn_db_real_query(conn, myquery)) {
        SQLERR_LOG("无法重新排序 %s 公会排行榜数据!", CLIENTS_GUILD);
        SQLERR_LOG("%s", psocn_db_error(conn));
        return -1;
    }

    return 0;
}

uint32_t db_get_bb_guild_points_personal_donation(uint32_t gc) {
    void* result;
    char** row;
    uint32_t guild_points_personal_donation = 0;

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "SELECT guild_points_personal_donation"
        " FROM %s WHERE guildcard='%u'", AUTH_ACCOUNT, gc);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u)", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -3;
    }

    guild_points_personal_donation = (uint32_t)strtoul(row[0], NULL, 10);

    psocn_db_result_free(result);

    return guild_points_personal_donation;
}

