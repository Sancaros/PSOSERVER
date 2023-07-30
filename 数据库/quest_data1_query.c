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

#define TABLE1 CHARACTER_QUEST_DATA1

static int db_del_char_quest_data1(uint32_t gc, uint8_t slot) {
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

static int db_updata_quest_data1(uint8_t* quest_data1, uint32_t gc, uint8_t slot) {
    size_t len = 0x208;
    memset(myquery, 0, sizeof(myquery));

    /*sprintf(myquery, "UPDATE %s SET"
        " quest_guildcard = '%" PRIu32 "', data = '%s', quest_flags = '%" PRIu32 "'"
        " WHERE "
        "guildcard='%" PRIu32 "' AND slot='%" PRIu8 "'"
        , TABLE1
        , quest_data1->quest_guildcard, (char*)&quest_data1->data, quest_data1->quest_flags
        , gc, slot
    );*/

    //sprintf(myquery, "UPDATE %s SET"
    //    " quest_guildcard = '%" PRIu32 "', quest_flags = '%" PRIu32 "'"
    //    " WHERE "
    //    "guildcard='%" PRIu32 "' AND slot='%" PRIu8 "'"
    //    , TABLE1
    //    , quest_data1->quest_guildcard, quest_data1->quest_flags
    //    , gc, slot
    //);

    sprintf(myquery, "UPDATE %s SET "
        "data = '",
        TABLE1
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)quest_data1,
        len);

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

int db_insert_char_quest_data1(uint8_t* quest_data1, uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO %s "
        "(guildcard, slot, data)"
        " VALUES "
        "('%" PRIu32 "', '%" PRIu8 "', '"
        ,
        TABLE1
        , gc, slot
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)quest_data1,
        0x208);

    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法保存数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", TABLE1, gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_char_quest_data1(uint8_t* quest_data1, uint32_t gc, uint8_t slot, uint32_t flag) {
    size_t len = 0x208;
    memset(myquery, 0, sizeof(myquery));

    if (flag & PSOCN_DB_SAVE_CHAR) {
        if (db_insert_char_quest_data1(quest_data1, gc, slot)) {
            return -1;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        if (db_updata_quest_data1(quest_data1, gc, slot)) {

            if (db_del_char_quest_data1(gc, slot)) {
                return -1;
            }

            if (db_insert_char_quest_data1(quest_data1, gc, slot)) {
                return -1;
            }
        }
    }

    return 0;
}

/* 获取玩家QUEST_DATA1数据数据项 */
int db_get_char_quest_data1(uint32_t gc, uint8_t slot, uint8_t* quest_data1, int check) {
    void* result;
    char** row;
    //char* endptr;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data FROM %s WHERE guildcard = '%" PRIu32 "' "
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

    ///* 获取二进制数据 */
    //int i = 2;
    //quest_data1->quest_guildcard = (uint32_t)strtoul(row[i], &endptr, 10);
    //i++;
    memcpy(quest_data1, row[0], 0x208);
    //i++;
    //quest_data1->quest_flags = (uint32_t)strtoul(row[i], &endptr, 10);

    //if (*endptr != '\0') {
    //    SQLERR_LOG("获取的数据 索引 %d 字符串读取有误", i);
    //    // 转换失败，输入字符串中包含非十六进制字符
    //}

    psocn_db_result_free(result);

    return 0;

build:
    if (db_insert_char_quest_data1(quest_data1, gc, slot)) {
        return -1;
    }

    return 0;
}
