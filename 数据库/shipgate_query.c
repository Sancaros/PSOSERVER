/*
    梦幻之星中国 船闸服务器 数据库操作
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

/* 初始化数据库连接 */
extern psocn_dbconn_t conn;

uint32_t db_get_char_data_length(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    unsigned long* len;
    uint32_t data_length;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data, size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER_DATA, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the length of the character data */
    if (!(len = psocn_db_result_lengths(result))) {
        psocn_db_result_free(result);
        SQLERR_LOG("无法获取角色数据的长度");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    data_length = len[0];

    psocn_db_result_free(result);
    return data_length;
}

uint32_t db_get_char_data_size(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    uint32_t data_size;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER_DATA, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    data_size = (uint32_t)atoi(row[0]);

    psocn_db_result_free(result);
    return data_size;
}

uint32_t db_get_char_data_play_time(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    uint32_t play_time;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT play_time FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER_DATA, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    play_time = (uint32_t)atoi(row[0]);

    psocn_db_result_free(result);
    return play_time;
}

char* db_get_char_raw_data(uint32_t gc, uint8_t slot, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_DATA, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("未找到保存的角色数据 (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return 0;
    }

    return row[0];
}

int db_update_bb_char_guild(psocn_bb_db_guild_t guild, uint32_t gc) {
    //DBG_LOG("更新 guild 设置");

    memset(myquery, 0, sizeof(myquery));

    /* Build the db query */
    sprintf(myquery, "UPDATE %s SET guild_info = '", CLIENTS_BLUEBURST_GUILD);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&guild.guild_data.guild_info,
        sizeof(guild.guild_data.guild_info));

    strcat(myquery, "', guild_name = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&guild.guild_data.guild_name,
        sizeof(guild.guild_data.guild_name));

    strcat(myquery, "', guild_flag = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&guild.guild_data.guild_flag,
        sizeof(guild.guild_data.guild_flag));

    sprintf(myquery + strlen(myquery),
        "', guildcard = '%d', guild_id = '%d', guild_priv_level = '%d'"
        ", guild_rank = '%d', guild_rewards1 = '%d', guild_rewards2 = '%d'"
        " WHERE guildcard = '%" PRIu32 "'",
        guild.guild_data.guildcard, guild.guild_data.guild_id, guild.guild_data.guild_priv_level,
        guild.guild_data.guild_rank, guild.guild_data.guild_rewards[0], guild.guild_data.guild_rewards[1], gc);

    /* Execute the query */
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新角色 %s 公会数据!", CLIENTS_BLUEBURST_GUILD);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_bb_char_option(psocn_bb_db_opts_t opts, uint32_t gc) {
    //DBG_LOG("更新设置 %d", gc);

    memset(myquery, 0, sizeof(myquery));

    /* Build the db query */
    sprintf(myquery, "UPDATE %s SET key_config='", CLIENTS_BLUEBURST_OPTION);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.key_cfg.key_config,
        sizeof(opts.key_cfg.key_config));

    strcat(myquery, "', joystick_config = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.key_cfg.joystick_config,
        sizeof(opts.key_cfg.joystick_config));

    strcat(myquery, "', shortcuts = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.shortcuts,
        sizeof(opts.shortcuts));

    strcat(myquery, "', symbol_chats = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.symbol_chats,
        sizeof(opts.symbol_chats));

    strcat(myquery, "', guild_name = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.guild_name,
        sizeof(opts.guild_name));

    sprintf(myquery + strlen(myquery), "', option_flags = '%d' WHERE guildcard='%" PRIu32 "'", opts.option_flags, gc);

    /* Execute the query */
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

/* 更新玩家基础数据至数据库 */
int db_update_char_challenge(psocn_bb_db_char_t* char_data, uint32_t gc, uint8_t slot, uint32_t flag) {
    static char query[sizeof(psocn_bb_db_char_t) * 2 + 256];
    char name[64];
    char class_name[64];

    istrncpy16_raw(ic_utf16_to_utf8, name, &char_data->character.name[2], 64, BB_CHARACTER_NAME_LENGTH);

    istrncpy(ic_gbk_to_utf8, class_name, pso_class[char_data->character.disp.dress_data.ch_class].cn_name, 64);

    if (flag & PSOCN_DB_SAVE_CHAR) {
        sprintf(query, "INSERT INTO %s(guildcard, slot, name, class_name, version, data) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%s', '%" PRIu8 "', '", CHARACTER_DATA_CHALLENGE, gc, slot
        , name, class_name, char_data->character.disp.dress_data.version);

        psocn_db_escape_str(&conn, query + strlen(query), (char*)char_data->challenge_data,
            sizeof(char_data->challenge_data));

        strcat(query, "')");

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法保存挑战数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", CHARACTER_DATA_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            return 0;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        sprintf(query, "DELETE FROM %s WHERE guildcard="
            "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_DATA_CHALLENGE, gc,
            slot);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_DATA_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            return -1;
        }

        sprintf(query, "INSERT INTO %s(guildcard, slot, name, class_name, version, data) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%s', '%" PRIu8 "', '", CHARACTER_DATA_CHALLENGE, gc, slot
            , name, class_name, char_data->character.disp.dress_data.version);

        psocn_db_escape_str(&conn, query + strlen(query), (char*)char_data->challenge_data,
            sizeof(char_data->challenge_data));

        strcat(query, "')");

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法保存挑战数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", CHARACTER_DATA_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            return 0;
        }
    }

    return 0;
}

int db_insert_bb_char_guild(uint16_t* guild_name, uint8_t* default_guild_flag, uint32_t gc) {
    void* result;
    //char** row;
    bb_guild_t* g_data;
    uint8_t create_res;
    int32_t guild_exists = 0, guild_id;
    char guild_name_text[24];

    g_data = (bb_guild_t*)malloc(sizeof(bb_guild_t));

    if (!g_data) {
        ERR_LOG("无法分配公会数据的内存空间");
        ERR_LOG("%s", strerror(errno));
        return -1;
    }

    memcpy(g_data->guild_name, guild_name, sizeof(g_data->guild_name));
    memcpy(g_data->guild_flag, default_guild_flag, sizeof(g_data->guild_flag));

    istrncpy16_raw(ic_utf16_to_utf8, guild_name_text, &g_data->guild_name[2], 24, sizeof(g_data->guild_name) - 4);

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "SELECT * from %s WHERE guild_name_text = '%s'",
        CLIENTS_BLUEBURST_GUILD, guild_name_text);
    if (!psocn_db_real_query(&conn, myquery))
    {
        result = psocn_db_result_store(&conn);
        guild_exists = (int32_t)psocn_db_result_rows(result);
        psocn_db_result_free(result);
    }
    else
        create_res = 1;

    if (!guild_exists)
    {
        // It doesn't... but it will now. :)
        sprintf_s(myquery, _countof(myquery), "INSERT INTO %s "
            "(guildcard, guild_name_text, guild_name, guild_flag) "
            "VALUES ('%" PRIu32 "', '%s', '",
            CLIENTS_BLUEBURST_GUILD, 
            gc, guild_name_text);

        psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&g_data->guild_name,
            sizeof(g_data->guild_name));

        strcat(myquery, "', '");

        psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&g_data->guild_flag,
            sizeof(g_data->guild_flag));

        strcat(myquery, "')");

        if (!psocn_db_real_query(&conn, myquery))
        {
            guild_id = (uint32_t)psocn_db_insert_id(&conn);
            sprintf_s(myquery, _countof(myquery), "UPDATE %s SET guild_id = '%u', guild_priv_level = '%u' "
                "WHERE guildcard = '%" PRIu32 "'", AUTH_DATA_ACCOUNT, guild_id, 0x40, gc);
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
