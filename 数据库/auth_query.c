/*
    梦幻之星中国 认证服务器 数据库操作
    版权 (C) 2022 Sancaros

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
#include "packets_bb.h"

/* 数据库连接 */
extern psocn_dbconn_t conn;

int db_upload_temp_data(void* data, uint32_t size) {

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

/* 获取 BB 的设置数据 */
psocn_bb_db_opts_t db_get_bb_char_option(uint32_t gc) {
    void* result;
    char** row;
    psocn_bb_db_opts_t opts;

    memset(&opts, 0, sizeof(psocn_bb_db_opts_t));

    memset(myquery, 0, sizeof(myquery));

    /* Look up the user's saved config */
    sprintf(myquery, "SELECT key_config, joystick_config, "
        "option_flags, shortcuts, symbol_chats, guild_name FROM %s WHERE "
        "guildcard='%" PRIu32 "'", CLIENTS_BLUEBURST_OPTION, gc);

    if (!psocn_db_real_query(&conn, myquery)) {
        result = psocn_db_result_store(&conn);

        /* See if we got a hit... */
        if (psocn_db_result_rows(result)) {
            row = psocn_db_result_fetch(result);
            memcpy(&opts.key_cfg.key_config, row[0], sizeof(opts.key_cfg.key_config));
            memcpy(&opts.key_cfg.joystick_config, row[1], sizeof(opts.key_cfg.joystick_config));
            opts.option_flags = (uint32_t)strtoul(row[2], NULL, 0);
            memcpy(&opts.shortcuts, row[3], sizeof(opts.shortcuts));
            memcpy(&opts.symbol_chats, row[4], sizeof(opts.symbol_chats));
            memcpy(&opts.guild_name, row[5], sizeof(opts.guild_name));
        }
        else {
            /* Initialize to defaults */
            memcpy(&opts.key_cfg.key_config, default_key_config, sizeof(default_key_config));
            memcpy(&opts.key_cfg.joystick_config, default_joystick_config, sizeof(default_joystick_config));
            memcpy(&opts.symbol_chats, default_symbolchats, sizeof(default_symbolchats));

            sprintf(myquery, "INSERT INTO %s (guildcard, key_config, joystick_config, symbol_chats)"
                " VALUES ('%" PRIu32"', '", CLIENTS_BLUEBURST_OPTION, gc);

            psocn_db_escape_str(&conn, myquery + strlen(myquery),
                (char*)&opts.key_cfg.key_config, sizeof(opts.key_cfg.key_config));

            strcat(myquery, "', '");

            psocn_db_escape_str(&conn, myquery + strlen(myquery),
                (char*)&opts.key_cfg.joystick_config, sizeof(opts.key_cfg.joystick_config));

            strcat(myquery, "', '");

            psocn_db_escape_str(&conn, myquery + strlen(myquery),
                (char*)&opts.symbol_chats, sizeof(opts.symbol_chats));

            strcat(myquery, "')");

            //printf("%s \n", myquery);

            if (psocn_db_real_query(&conn, myquery)) {
                SQLERR_LOG("无法插入设置数据 "
                    "guildcard %" PRIu32 "", gc);
                SQLERR_LOG("%s", psocn_db_error(&conn));
            }
        }
        psocn_db_result_free(result);
    }

    return opts;
}

//是否与 db_update_bb_char_guild 函数重叠了？？？？？
int db_updata_bb_char_guild_data(uint32_t guild_id, uint32_t gc) {

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET "
        "guild_id = '%" PRIu32"' "
        "WHERE guildcard = '%" PRIu32 "'", CLIENTS_BLUEBURST_GUILD, 
        guild_id, 
        gc);
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新角色 %s 公会数据!", CLIENTS_BLUEBURST_GUILD);
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
        "guildcard = '%" PRIu32 "'", AUTH_DATA_ACCOUNT, gc);

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

        DBG_LOG("%d %d %d", gc, guild_id, guild_priv_level);

        if (guild_id != -1) {

            /* 同步角色账号与角色公会数据的公会ID与公会等级 */
            if (db_updata_bb_char_guild_data(guild_id, gc))
                return guild;

            /* 查询数据表 */
            sprintf(myquery, "SELECT * FROM %s WHERE "
                "guild_id = '%" PRIu32 "'", CLIENTS_BLUEBURST_GUILD, guild_id);

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
                    "'%d', '%d', '%d', '", CLIENTS_BLUEBURST_GUILD,
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
        "WHERE guildcard = '%" PRIu32 "' AND slot = '%"PRIu8"'", CHARACTER_DATA, ipstr, timestamp, gc, slot);
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
    //char query[256];
    time_t servertime = time(NULL);

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "UPDATE %s SET dressflag = '%" PRIu32 "' WHERE "
        "guildcard = '%" PRIu32 "'", AUTH_DATA_ACCOUNT, flags, gc);

    if (psocn_db_real_query(&conn, myquery)) {
        /* Should send an error message to the user */
        SQLERR_LOG("无法初始化更衣室 (GC %" PRIu32 ", 标签 %"
            PRIu8 "):\n%s", gc, flags,
            psocn_db_error(&conn));
        return -1;
    }
    return 0;
}
