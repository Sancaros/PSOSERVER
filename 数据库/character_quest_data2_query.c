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

#define TABLE1 CHARACTER_QUEST_DATA2

static int db_del_char_quest_data2(uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "DELETE FROM %s"
        " WHERE "
        "guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'"
        , TABLE1
        , gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", TABLE1, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

static int db_updata_quest_data2(psocn_bb_quest_data2_t* quest_data2, uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "UPDATE %s SET "
        "change_time = NOW(), "
        "data0 = '%d', data1 = '%d', data2 = '%d', data3 = '%d', data4 = '%d', "
        "data5 = '%d', data6 = '%d', data7 = '%d', data8 = '%d', data9 = '%d', "
        "data10 = '%d', data11 = '%d', data12 = '%d', data13 = '%d', data14 = '%d', "
        "data15 = '%d', data16 = '%d', data17 = '%d', data18 = '%d', data19 = '%d', "
        "data20 = '%d', data21 = '%d', "
        "full_data = '"
        ,TABLE1
        , quest_data2->part[0], quest_data2->part[1], quest_data2->part[2], quest_data2->part[3], quest_data2->part[4]
        , quest_data2->part[5], quest_data2->part[6], quest_data2->part[7], quest_data2->part[8], quest_data2->part[9]
        , quest_data2->part[10], quest_data2->part[11], quest_data2->part[12], quest_data2->part[13], quest_data2->part[14]
        , quest_data2->part[15], quest_data2->part[16], quest_data2->part[17], quest_data2->part[18], quest_data2->part[19]
        , quest_data2->part[20], quest_data2->part[21]
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)quest_data2->all,
        PSOCN_DATALENGTH_BB_DB_QUEST_DATA2);

    sprintf(myquery + strlen(myquery), "' WHERE guildcard = '%" PRIu32 "' AND "
        "slot = '%" PRIu8 "'", gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", TABLE1, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

int db_insert_char_quest_data2(psocn_bb_quest_data2_t* quest_data2, uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO %s ("
        "guildcard, slot, change_time, "
        "data0, data1, data2, data3, data4, "
        "data5, data6, data7, data8, data9, "
        "data10, data11, data12, data13, data14, "
        "data15, data16, data17, data18, data19, "
        "data20, data21, "
        "full_data"
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', NOW(), ",
        TABLE1,
        gc, slot);

    for (int i = 0; i < PSOCN_STLENGTH_BB_DB_QUEST_DATA2; i++) {
        // 用于暂存每个 `quest_data2->part[]` 的字符串表示
        char temp[100] = { 0 };
        snprintf(temp, sizeof(temp), "'%" PRIu32 "', ", quest_data2->part[i]);
        SAFE_STRCAT(myquery, temp);
    }

    // 移除最后一个多余的逗号和空格
    myquery[strlen(myquery) - 2] = '\0';

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)quest_data2->all,
        PSOCN_DATALENGTH_BB_DB_QUEST_DATA2);

    SAFE_STRCAT(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法保存数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", TABLE1, gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_char_quest_data2(psocn_bb_quest_data2_t* quest_data2, uint32_t gc, uint8_t slot, uint32_t flag) {

    if (flag & PSOCN_DB_SAVE_CHAR) {
        if (db_insert_char_quest_data2(quest_data2, gc, slot)) {
            return -1;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        if (db_updata_quest_data2(quest_data2, gc, slot)) {

            if (db_del_char_quest_data2(gc, slot)) {
                return -1;
            }

            if (db_insert_char_quest_data2(quest_data2, gc, slot)) {
                return -1;
            }
        }
    }

    return 0;
}

/* 获取玩家QUEST_DATA2数据数据项 */
int db_get_char_quest_data2(uint32_t gc, uint8_t slot, psocn_bb_quest_data2_t* quest_data2, int check) {
    void* result;
    char** row;
    char* endptr;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%" PRIu8 "'", TABLE1, gc, slot);

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
    memcpy(quest_data2, row[i], PSOCN_DATALENGTH_BB_DB_QUEST_DATA2);
    i++;

    for (size_t x = 0; x < PSOCN_STLENGTH_BB_DB_QUEST_DATA2; x++) {
        quest_data2->part[x] = (uint32_t)strtoul(row[i], &endptr, 10);
        i++;
    }

    if (*endptr != '\0') {
        SQLERR_LOG("获取的数据 索引 %d 字符串读取有误", i);
        // 转换失败，输入字符串中包含非十六进制字符
    }

    psocn_db_result_free(result);

    return 0;

build:
    if (db_insert_char_quest_data2(quest_data2, gc, slot)) {
        return -1;
    }

    return 0;
}
