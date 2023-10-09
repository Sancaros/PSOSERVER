/*
    梦幻之星中国 船闸服务器 数据库操作
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

#define TABLE CHARACTER_RECORDS_BATTLE

static int db_insert_b_records(battle_records_t* b_records, uint32_t gc, uint8_t slot) {

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO %s("
        "guildcard, slot, "
        "first, second, third, fourth, "
        "disconnect_count, "
        "data0, data1, data2, data3, "
        "data4, data5, data6, data7, "
        "data8, data9, data10, data11, "
        "data12, data13, "
        "unknown_a1"
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', "
        "'%04X', '%04X', '%04X', '%04X', "
        "'%04X', "
        "'%d', '%d', '%d', '%d', "
        "'%d', '%d', '%d', '%d', "
        "'%d', '%d', '%d', '%d', "
        "'%d', '%d', "
        "'", 
        TABLE, 
        gc, slot
        , b_records->place_counts[0], b_records->place_counts[1], b_records->place_counts[2], b_records->place_counts[3]
        , b_records->disconnect_count
        , b_records->data0, b_records->data1, b_records->data2, b_records->data3
        , b_records->data4, b_records->data5, b_records->data6, b_records->data7
        , b_records->data8, b_records->data9, b_records->data10, b_records->data11
        , b_records->data12, b_records->data13
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&b_records->unknown_a1, 14);

    SAFE_STRCAT(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法保存对战数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_clean_up_char_b_records(uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "DELETE FROM %s WHERE guildcard="
        "'%" PRIu32 "' AND slot='%" PRIu8 "'", TABLE, gc,
        slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

/* 更新玩家基础数据至数据库 */
int db_update_char_b_records(battle_records_t* b_records, uint32_t gc, uint8_t slot, uint32_t flag) {

    if (flag & PSOCN_DB_SAVE_CHAR) {

        if (db_insert_b_records(b_records, gc, slot)) {
            SQLERR_LOG("无法保存对战数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 ")", TABLE, gc, slot);
            return 0;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {

        if (db_clean_up_char_b_records(gc, slot)) {
            SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 ")", TABLE, gc, slot);
            return -1;
        }

        if (db_insert_b_records(b_records, gc, slot)) {
            SQLERR_LOG("无法保存对战数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 ")", TABLE, gc, slot);
            return 0;
        }
    }

    return 0;
}

int db_get_b_records(uint32_t gc, uint8_t slot, battle_records_t* b_records) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT *"
        " FROM "
        "%s"
        " WHERE "
        "guildcard = '%" PRIu32 "' "
        "AND slot = '%" PRIu8 "'"
        , TABLE
        , gc
        , slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);

        SQLERR_LOG("未找到保存的角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -3;
    }

    int j = 2;

    for (int i = 0; i < 4; i++) {
        b_records->place_counts[i] = (uint16_t)strtoul(row[j], NULL, 16);
        j++;
    }
    b_records->disconnect_count = (uint16_t)strtoul(row[j], NULL, 16);
    j++;
    memcpy((char*)&b_records->unknown_a1, row[j], 14);
    j++;
    b_records->data0 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data1 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data2 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data3 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data4 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data5 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data6 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data7 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data8 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data9 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data10 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data11 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data12 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    b_records->data13 = (uint8_t)strtoul(row[j], NULL, 10);

    psocn_db_result_free(result);

    return 0;
}
