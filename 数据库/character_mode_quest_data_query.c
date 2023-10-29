/*
    梦幻之星中国 服务器通用 数据库操作
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
#include "f_checksum.h"

static int db_del_char_mode_quest_data(uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "DELETE FROM %s"
        " WHERE "
        "guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'"
        , CHARACTER_MODE_QUEST_DATA
        , gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_MODE_QUEST_DATA, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

static int db_updata_mode_quest_data(psocn_bb_mode_quest_data_t* mode_quest_data, uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "UPDATE %s SET "
        "change_time = NOW(), "
        "data0 = '%d', data1 = '%d', data2 = '%d', data3 = '%d', data4 = '%d', "
        "data5 = '%d', data6 = '%d', data7 = '%d', data8 = '%d', data9 = '%d', "
        "data10 = '%d', data11 = '%d', data12 = '%d', data13 = '%d', data14 = '%d', "
        "data15 = '%d', data16 = '%d', data17 = '%d', data18 = '%d', data19 = '%d', "
        "data20 = '%d', data21 = '%d', "
        "full_data = '"
        ,CHARACTER_MODE_QUEST_DATA
        , mode_quest_data->part[0], mode_quest_data->part[1], mode_quest_data->part[2], mode_quest_data->part[3], mode_quest_data->part[4]
        , mode_quest_data->part[5], mode_quest_data->part[6], mode_quest_data->part[7], mode_quest_data->part[8], mode_quest_data->part[9]
        , mode_quest_data->part[10], mode_quest_data->part[11], mode_quest_data->part[12], mode_quest_data->part[13], mode_quest_data->part[14]
        , mode_quest_data->part[15], mode_quest_data->part[16], mode_quest_data->part[17], mode_quest_data->part[18], mode_quest_data->part[19]
        , mode_quest_data->part[20], mode_quest_data->part[21]
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)mode_quest_data->all,
        PSOCN_DATALENGTH_BB_DB_MODE_QUEST_DATA);

    sprintf(myquery + strlen(myquery), "' WHERE guildcard = '%" PRIu32 "' AND "
        "slot = '%" PRIu8 "'", gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_MODE_QUEST_DATA, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

int db_insert_char_mode_quest_data(psocn_bb_mode_quest_data_t* mode_quest_data, uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, sizeof(myquery), "INSERT INTO %s ("
        "guildcard, slot, change_time, "
        "data0, data1, data2, data3, data4, "
        "data5, data6, data7, data8, data9, "
        "data10, data11, data12, data13, data14, "
        "data15, data16, data17, data18, data19, "
        "data20, data21, "
        "full_data"
        ") VALUES ("
        "'%d', '%d', NOW(), "
        "'%d', '%d', '%d', '%d', '%d', "
        "'%d', '%d', '%d', '%d', '%d', "
        "'%d', '%d', '%d', '%d', '%d', "
        "'%d', '%d', '%d', '%d', '%d', "
        "'%d', '%d', "
        "'"
        , CHARACTER_MODE_QUEST_DATA
        , gc, slot
        , mode_quest_data->part[0], mode_quest_data->part[1], mode_quest_data->part[2], mode_quest_data->part[3], mode_quest_data->part[4]
        , mode_quest_data->part[5], mode_quest_data->part[6], mode_quest_data->part[7], mode_quest_data->part[8], mode_quest_data->part[9]
        , mode_quest_data->part[10], mode_quest_data->part[11], mode_quest_data->part[12], mode_quest_data->part[13], mode_quest_data->part[14]
        , mode_quest_data->part[15], mode_quest_data->part[16], mode_quest_data->part[17], mode_quest_data->part[18], mode_quest_data->part[19]
        , mode_quest_data->part[20], mode_quest_data->part[21]
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)mode_quest_data->all,
        PSOCN_DATALENGTH_BB_DB_MODE_QUEST_DATA);

    SAFE_STRCAT(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法保存数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", CHARACTER_MODE_QUEST_DATA, gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_char_mode_quest_data(psocn_bb_mode_quest_data_t* mode_quest_data, uint32_t gc, uint8_t slot, uint32_t flag) {

    if (flag & PSOCN_DB_SAVE_CHAR) {
        if (db_insert_char_mode_quest_data(mode_quest_data, gc, slot)) {
            return -1;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        if (db_check_bb_char_split_data_exist(gc, slot, CHARACTER_MODE_QUEST_DATA)) {
            if (db_updata_mode_quest_data(mode_quest_data, gc, slot)) {
                SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", CHARACTER_MODE_QUEST_DATA, gc, slot);

                return -2;
            }
        }
        else {

            if (db_del_char_mode_quest_data(gc, slot)) {
                SQLERR_LOG("无法删除数据 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", CHARACTER_MODE_QUEST_DATA, gc, slot);
                return -3;
            }

            if (db_insert_char_mode_quest_data(mode_quest_data, gc, slot)) {
                SQLERR_LOG("无法保存数据 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", CHARACTER_MODE_QUEST_DATA, gc, slot);
                return -4;
            }
        }
    }

    return 0;
}

/* 获取玩家mode_quest_data数据数据项 */
int db_get_char_mode_quest_data(uint32_t gc, uint8_t slot, psocn_bb_mode_quest_data_t* mode_quest_data, int check) {
    void* result;
    char** row;
    char* endptr;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%" PRIu8 "'", CHARACTER_MODE_QUEST_DATA, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto build;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto build;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("未找到保存的角色数据 (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        goto build;
    }

    /* 获取二进制数据 */
    int i = 3;
    memcpy(mode_quest_data, row[i], PSOCN_DATALENGTH_BB_DB_MODE_QUEST_DATA);
    i++;

    for (size_t x = 0; x < PSOCN_STLENGTH_BB_DB_MODE_QUEST_DATA; x++) {
        mode_quest_data->part[x] = (uint32_t)strtoul(row[i], &endptr, 10);
        i++;
    }

    if (*endptr != '\0') {
        SQLERR_LOG("获取的数据 索引 %d 字符串读取有误", i);
        // 转换失败，输入字符串中包含非十六进制字符
    }

    psocn_db_result_free(result);

    return 0;

build:
    if (db_insert_char_mode_quest_data(mode_quest_data, gc, slot)) {
        return -1;
    }

    return 0;
}
