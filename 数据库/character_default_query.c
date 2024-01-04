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

int db_update_character_default(psocn_bb_db_char_t* data, int index) {
    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "update_time = NOW(), `character2` = '", CHARACTER_DEFAULT);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->character,
        PSOCN_STLENGTH_BB_CHAR2);

    SAFE_STRCAT(myquery, "', bank = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->bank,
        PSOCN_STLENGTH_BANK);

    SAFE_STRCAT(myquery, "', quest_data1 = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_data1,
        PSOCN_STLENGTH_BB_DB_QUEST_DATA1);

    SAFE_STRCAT(myquery, "', guildcard_desc = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->guildcard_desc,
        88);

    SAFE_STRCAT(myquery, "', autoreply = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->autoreply,
        172);

    SAFE_STRCAT(myquery, "', infoboard = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->infoboard,
        172);

    SAFE_STRCAT(myquery, "', b_records = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->b_records,
        PSOCN_STLENGTH_BATTLE_RECORDS);

    SAFE_STRCAT(myquery, "', c_records = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->c_records,
        PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

    SAFE_STRCAT(myquery, "', tech_menu = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->tech_menu,
        PSOCN_STLENGTH_BB_DB_TECH_MENU);

    SAFE_STRCAT(myquery, "', mode_quest_data = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_global_flags.all,
        PSOCN_DATALENGTH_BB_DB_QUEST_GLOBAL_FLAGS_DATA);

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
        "`character2`, bank, quest_data1, guildcard_desc, "
        "autoreply, infoboard, b_records, c_records, tech_menu, mode_quest_data "
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

    memcpy((char*)&data->character, row[0], PSOCN_STLENGTH_BB_CHAR2);
    memcpy((char*)&data->bank, row[1], PSOCN_STLENGTH_BANK);
    memcpy((char*)&data->quest_data1, row[2], PSOCN_STLENGTH_BB_DB_QUEST_DATA1);
    memcpy((char*)&data->guildcard_desc, row[3], 88);
    memcpy((char*)&data->autoreply, row[4], 172);
    memcpy((char*)&data->infoboard, row[5], 172);
    memcpy((char*)&data->b_records, row[6], PSOCN_STLENGTH_BATTLE_RECORDS);
    memcpy((char*)&data->c_records, row[7], PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);
    memcpy((char*)&data->tech_menu, row[8], PSOCN_STLENGTH_BB_DB_TECH_MENU);
    memcpy((char*)&data->quest_global_flags.all, row[9], PSOCN_DATALENGTH_BB_DB_QUEST_GLOBAL_FLAGS_DATA);

    return 0;
}

int db_get_character_mode(psocn_bb_mode_char_t* data) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "SELECT "
        "`character2`"
        " FROM %s", CHARACTER_DEFAULT);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法获取数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    if ((result = psocn_db_result_use(&conn)) == NULL) {
        SQLERR_LOG("无法获取查询结果");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    int i = 0;

    while ((row = psocn_db_result_fetch(result)) != NULL) {
        memcpy((char*)&data->cdata[i], row[0], PSOCN_STLENGTH_BB_CHAR2);

        i++;
        if ( i > 11) {
            break;
        }
    }

    psocn_db_result_free(result);

    return 0;
}

int db_insert_character_default(psocn_bb_db_char_t* data, int index, char* class_name) {

    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "`index`, class_name, "
        "`character2`, bank, quest_data1, guildcard_desc, "
        "autoreply, infoboard, b_records, c_records, tech_menu, mode_quest_data"
        ") VALUES ("
        "'%d', '%s', '",
        CHARACTER_DEFAULT,
        index, class_name
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->character,
        PSOCN_STLENGTH_BB_CHAR2);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->bank,
        PSOCN_STLENGTH_BANK);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_data1,
        PSOCN_STLENGTH_BB_DB_QUEST_DATA1);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->guildcard_desc,
        88);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->autoreply,
        172);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->infoboard,
        172);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->b_records,
        PSOCN_STLENGTH_BATTLE_RECORDS);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->c_records,
        PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->tech_menu,
        PSOCN_STLENGTH_BB_DB_TECH_MENU);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_global_flags.all,
        PSOCN_DATALENGTH_BB_DB_QUEST_GLOBAL_FLAGS_DATA);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法插入数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}
