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

int db_update_bb_char_option(psocn_bb_db_opts_t opts, uint32_t gc) {
    //DBG_LOG("更新设置 %d", gc);

    memset(myquery, 0, sizeof(myquery));

    /* Build the db query */
    sprintf(myquery, "UPDATE %s SET key_config='", CLIENTS_OPTION_BLUEBURST);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.key_cfg.key_config,
        sizeof(opts.key_cfg.key_config));

    strcat(myquery, "', joystick_config = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.key_cfg.joystick_config,
        sizeof(opts.key_cfg.joystick_config));

    strcat(myquery, "', shortcuts = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.shortcuts,
        PSOCN_STLENGTH_BB_DB_SHORTCUTS);

    strcat(myquery, "', symbol_chats = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&opts.symbol_chats,
        PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS);

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

/* 获取 BB 的设置数据 */
psocn_bb_db_opts_t db_get_bb_char_option(uint32_t gc) {
    void* result;
    char** row;
    psocn_bb_db_opts_t opts = { 0 };

    memset(myquery, 0, sizeof(myquery));

    /* Look up the user's saved config */
    sprintf(myquery, "SELECT key_config, joystick_config, "
        "option_flags, shortcuts, symbol_chats, guild_name FROM %s WHERE "
        "guildcard='%" PRIu32 "'", CLIENTS_OPTION_BLUEBURST, gc);

    if (!psocn_db_real_query(&conn, myquery)) {
        result = psocn_db_result_store(&conn);

        /* See if we got a hit... */
        if (psocn_db_result_rows(result)) {
            row = psocn_db_result_fetch(result);
            memcpy(&opts.key_cfg.key_config, row[0], sizeof(opts.key_cfg.key_config));
            memcpy(&opts.key_cfg.joystick_config, row[1], sizeof(opts.key_cfg.joystick_config));
            opts.option_flags = (uint32_t)strtoul(row[2], NULL, 10);
            memcpy(&opts.shortcuts, row[3], PSOCN_STLENGTH_BB_DB_SHORTCUTS);
            memcpy(&opts.symbol_chats, row[4], PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS);
            memcpy(&opts.guild_name, row[5], sizeof(opts.guild_name));
        }
        else {
            /* Initialize to defaults */
            memcpy(&opts.key_cfg.key_config, default_key_config, sizeof(default_key_config));
            memcpy(&opts.key_cfg.joystick_config, default_joystick_config, sizeof(default_joystick_config));
            memcpy(&opts.symbol_chats, default_symbolchats, PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS);

            sprintf(myquery, "INSERT INTO %s (guildcard, key_config, joystick_config, symbol_chats)"
                " VALUES ('%" PRIu32"', '", CLIENTS_OPTION_BLUEBURST, gc);

            psocn_db_escape_str(&conn, myquery + strlen(myquery),
                (char*)&opts.key_cfg.key_config, sizeof(opts.key_cfg.key_config));

            strcat(myquery, "', '");

            psocn_db_escape_str(&conn, myquery + strlen(myquery),
                (char*)&opts.key_cfg.joystick_config, sizeof(opts.key_cfg.joystick_config));

            strcat(myquery, "', '");

            psocn_db_escape_str(&conn, myquery + strlen(myquery),
                (char*)&opts.symbol_chats, PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS);

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
