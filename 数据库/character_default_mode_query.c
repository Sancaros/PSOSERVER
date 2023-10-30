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
#include "pso_character.h"

int db_update_character_default_mode(psocn_bb_char_t* data, uint8_t char_class, int qid) {
    /* 计算 myquery 缓冲区的长度 */
    const size_t myquery_size = sizeof(myquery);
    char char_buf[PSOCN_STLENGTH_BB_CHAR2] = { 0 }, inv_buf[PSOCN_STLENGTH_INV] = { 0 }, tech_buf[PSOCN_STLENGTH_BB_TECH] = { 0 };

    /* 使用 sprintf 函数构造 SQL 查询语句 */
    const char* sql_template = "UPDATE %s SET update_time = NOW(), `character` = '%s', inventory = '%s', tech = '%s' WHERE `qid` = %d AND `ch_class` = %d";
    sprintf(myquery, sql_template, CHARACTER_DEFAULT_MODE,
        psocn_db_escape_str(&conn, char_buf, (char*)data, PSOCN_STLENGTH_BB_CHAR2),
        psocn_db_escape_str(&conn, inv_buf, (char*)&data->inv, PSOCN_STLENGTH_INV),
        psocn_db_escape_str(&conn, tech_buf, (char*)&data->technique_levels_v1, PSOCN_STLENGTH_BB_TECH),
        qid, char_class);

    /* 检查 myquery 是否超出了缓冲区边界 */
    if (strnlen_s(myquery, myquery_size) == myquery_size - 1) {
        SQLERR_LOG("写入数据超出缓冲区");
        return -1;
    }

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    return 0;
}

//int db_update_character_default_mode(psocn_bb_char_t* data, uint8_t char_class, int qid) {
//    memset(myquery, 0, sizeof(myquery));
//
//    snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
//        "update_time = NOW(), `character` = '", CHARACTER_DEFAULT_MODE);
//
//    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)data,
//        PSOCN_STLENGTH_BB_CHAR2);
//
//    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', inventory = '");
//
//    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->inv,
//        PSOCN_STLENGTH_INV);
//
//    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "', tech = '");
//
//    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->tech,
//        PSOCN_STLENGTH_BB_TECH);
//
//    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "' WHERE `qid` = %d AND `ch_class` = %d", qid, char_class);
//
//    if (psocn_db_real_query(&conn, myquery)) {
//        SQLERR_LOG("无法更新数据");
//        SQLERR_LOG("%s", psocn_db_error(&conn));
//        return -1;
//    }
//
//    return 0;
//}

int db_insert_character_default_mode(psocn_bb_char_t* data, uint8_t char_class, int qid, char* class_name) {

    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "`qid`, ch_class, class_name, "
        "`character`, inventory, tech"
        ") VALUES ("
        "'%d', '%u', '%s', '",
        CHARACTER_DEFAULT_MODE,
        qid, char_class, class_name
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data,
        PSOCN_STLENGTH_BB_CHAR2);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->inv,
        PSOCN_STLENGTH_INV);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->technique_levels_v1,
        PSOCN_STLENGTH_BB_TECH);

    SAFE_STRCAT(myquery, "')");

    //int remaining_size = sizeof(myquery) - strlen(myquery) - 1; // 计算剩余可用长度

    //// 格式化拼接字符串，并确保不会溢出缓冲区
    //int n = snprintf(myquery + strlen(myquery), remaining_size, "')");

    //if (n < 0 || n >= remaining_size) {
    //    SQLERR_LOG("无法写入myquery数据");
    //    SQLERR_LOG("%s", psocn_db_error(&conn));
    //    return -1;
    //}

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法插入数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_get_character_default_mode(psocn_bb_mode_char_t* data) {
    void* result;
    char** row;
    int i, j, k, len = 0;

    memset(myquery, 0, sizeof(myquery));


    for (i = 0; i < 14; i++) {
        for (j = 0; j < MAX_PLAYER_CLASS_BB; j++) {
            k = i + 65522;

            snprintf(myquery, sizeof(myquery), "SELECT "
                "`character`, inventory, tech"
                " FROM %s "
                "WHERE `qid` = %d AND `ch_class` = %d", 
                CHARACTER_DEFAULT_MODE, 
                k, j
            );

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

            memcpy((char*)&data->cdata[j][i], row[0], PSOCN_STLENGTH_BB_CHAR2);
            memcpy((char*)&data->cdata[j][i].inv, row[1], PSOCN_STLENGTH_INV);
            memcpy((char*)&data->cdata[j][i].technique_levels_v1, row[2], PSOCN_STLENGTH_BB_TECH);

            psocn_db_result_free(result);

#ifdef DEBUG
            CONFIG_LOG("获取 Blue Burst 初始MODE数据 MODE任务索引 %d MODE职业索引 %d 职业 %s",
                i, j, pso_class[data->cdata[j][i].dress_data.ch_class].cn_name);
#endif // DEBUG

        }

    }
    return 0;
}
