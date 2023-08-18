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

int db_update_character_default_mode(psocn_bb_char_t* data, int qid) {
    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "update_time = NOW(), `character` = '", CHARACTER_DEFAULT_MODE);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)data,
        PSOCN_STLENGTH_BB_CHAR2);

    strcat(myquery, "', inventory = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->inv,
        PSOCN_STLENGTH_INV);

    strcat(myquery, "', tech = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->tech,
        0x14);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "' WHERE `qid` = %d AND `ch_class` = %d", qid, data->dress_data.ch_class);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_insert_character_default_mode(psocn_bb_char_t* data, int qid, char* class_name) {

    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "`qid`, ch_class, class_name, "
        "`character`, inventory, tech"
        ") VALUES ("
        "'%d', '%d', '%s', '",
        CHARACTER_DEFAULT_MODE,
        qid, data->dress_data.ch_class, class_name
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data,
        PSOCN_STLENGTH_BB_CHAR2);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->inv,
        PSOCN_STLENGTH_INV);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->tech,
        0x14);

    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法插入数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}