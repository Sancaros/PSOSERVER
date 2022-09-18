/*
    �λ�֮���й� ��֤������ ���ݿ����
    ��Ȩ (C) 2022 Sancaros

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

/* Database connection */
extern psocn_dbconn_t conn;

/* ��ȡ BB ���������� */
psocn_bb_db_opts_t db_get_bb_char_option(uint32_t gc) {
    char query[sizeof(psocn_bb_db_opts_t) * 2 + 256 + 1248 * 2];
    void* result;
    char** row;
    psocn_bb_db_opts_t opts;

    memset(&opts, 0, sizeof(psocn_bb_db_opts_t));

    /* Look up the user's saved config */
    sprintf(query, "SELECT key_config, joystick_config, "
        "option_flags, shortcuts, symbol_chats, guild_name FROM %s WHERE "
        "guildcard='%" PRIu32 "'", CLIENTS_BLUEBURST_OPTION, gc);

    if (!psocn_db_real_query(&conn, query)) {
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

            sprintf(query, "INSERT INTO %s (guildcard, key_config, joystick_config, symbol_chats)"
                " VALUES ('%" PRIu32"', '", CLIENTS_BLUEBURST_OPTION, gc);

            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)&opts.key_cfg.key_config, sizeof(opts.key_cfg.key_config));

            strcat(query, "', '");

            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)&opts.key_cfg.joystick_config, sizeof(opts.key_cfg.joystick_config));

            strcat(query, "', '");

            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)&opts.symbol_chats, sizeof(opts.symbol_chats));

            strcat(query, "')");

            //printf("%s \n", query);

            if (psocn_db_real_query(&conn, query)) {
                SQLERR_LOG("�޷������������� "
                    "guildcard %" PRIu32 "", gc);
                SQLERR_LOG("%s", psocn_db_error(&conn));
            }
        }
        psocn_db_result_free(result);
    }

    return opts;
}

/* ��ȡ BB �Ĺ��᣿���� */
psocn_bb_db_guild_t db_get_bb_char_guild(uint32_t gc) {
    char query[sizeof(psocn_bb_db_guild_t) * 2 + 256];
    void* result;
    char** row;
    psocn_bb_db_guild_t guild;

    memset(&guild, 0, sizeof(psocn_bb_db_guild_t));

    /* ��ѯ���ݱ� */
    sprintf(query, "SELECT * FROM %s WHERE "
        "guildcard='%" PRIu32 "'", CLIENTS_GUILD, gc);

    if (!psocn_db_real_query(&conn, query)) {
        result = psocn_db_result_store(&conn);

        /* �����Ƿ������� */
        if (psocn_db_result_rows(result)) {
            row = psocn_db_result_fetch(result);
            memcpy(&guild.guild_data.guildcard, row[0], sizeof(guild.guild_data.guildcard));
            guild.guild_data.guild_id = (uint32_t)strtoul(row[1], NULL, 0);
            memcpy(&guild.guild_data.guild_info, row[2], sizeof(guild.guild_data.guild_info));
            memcpy(&guild.guild_data.guild_priv_level, row[3], sizeof(guild.guild_data.guild_priv_level));
            memcpy(&guild.guild_data.reserved, row[4], sizeof(guild.guild_data.reserved));
            memcpy(&guild.guild_data.guild_name, row[5], sizeof(guild.guild_data.guild_name));
            memcpy(&guild.guild_data.guild_flag, row[6], sizeof(guild.guild_data.guild_flag));
            guild.guild_data.guild_rewards[0] = (uint32_t)strtoul(row[7], NULL, 0);
            guild.guild_data.guild_rewards[1] = (uint32_t)strtoul(row[8], NULL, 0);
        }
        else {
            /* ��ʼ��Ĭ������ */
            guild.guild_data.guildcard = gc;
            guild.guild_data.guild_id = -1;

            /* TODO ��������δ��ó�ʼ���� ���Դ�Ĭ�ϵ�������ɫ�����л�ȡ��ʼ����*/
            guild.guild_data.guild_rewards[0] = 0xFFFFFFFF;
            guild.guild_data.guild_rewards[1] = 0xFFFFFFFF;

            sprintf(query, "INSERT INTO %s (guildcard, guild_id, guild_priv_level, guild_rewards1, guild_rewards2, "
                "guild_info, reserved, guild_name, guild_flag)"
                " VALUES ('%" PRIu32"', '%d', '%" PRIu16"', '%d', '%d', '", CLIENTS_GUILD,
                guild.guild_data.guildcard, guild.guild_data.guild_id, guild.guild_data.guild_priv_level, 
                guild.guild_data.guild_rewards[0], guild.guild_data.guild_rewards[1]);

            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)&guild.guild_data.guild_info, sizeof(guild.guild_data.guild_info));

            strcat(query, "', '");

            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)&guild.guild_data.reserved, sizeof(guild.guild_data.reserved));

            strcat(query, "', '");

            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)&guild.guild_data.guild_name, sizeof(guild.guild_data.guild_name));

            strcat(query, "', '");

            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)&guild.guild_data.guild_flag, sizeof(guild.guild_data.guild_flag));

            strcat(query, "')");

            //printf("%s \n", query);

            if (psocn_db_real_query(&conn, query)) {
                SQLERR_LOG("�޷������������� "
                    "guildcard %" PRIu32 "", gc);
                SQLERR_LOG("%s", psocn_db_error(&conn));
            }
        }
        psocn_db_result_free(result);
    }

    return guild;
}

int db_update_char_auth_msg(char ipstr[INET6_ADDRSTRLEN], uint32_t gc, uint8_t slot) {
    char query[256];

    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    char timestamp[28];
    sprintf(timestamp, "%u-%02u-%02u %02u:%02u:%02u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond);

    sprintf_s(query, _countof(query), "UPDATE %s SET ip = '%s', last_login_time = '%s'"
        "WHERE guildcard = '%" PRIu32 "' AND slot = '%"PRIu8"'", CHARACTER_DATA, ipstr, timestamp, gc, slot);
    if (psocn_db_real_query(&conn, query))
    {
        SQLERR_LOG("�޷����½�ɫ��¼���� (GC %" PRIu32 ", ��λ %"
            PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_char_dressflag(uint32_t gc, uint32_t flags) {
    char query[256];
    time_t servertime = time(NULL);

    sprintf(query, "UPDATE %s SET dressflag = '%" PRIu32 "' WHERE "
        "guildcard = '%" PRIu32 "'", AUTH_DATA_ACCOUNT, flags, gc);

    if (psocn_db_real_query(&conn, query)) {
        /* Should send an error message to the user */
        SQLERR_LOG("�޷���ʼ�������� (GC %" PRIu32 ", ��ǩ %"
            PRIu8 "):\n%s", gc, flags,
            psocn_db_error(&conn));
        return -1;
    }
    return 0;
}
