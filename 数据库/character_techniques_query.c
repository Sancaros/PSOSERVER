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
#include "pso_player.h"

static int db_insert_techniques(psocn_bb_char_t* character, uint32_t gc, uint32_t slot) {
    memset(myquery, 0, sizeof(myquery));
    techniques_t* tech = &character->technique_levels_v1;
    //uint8_t tech_level[20] = { 0 };

    //for (int i = 0; i < MAX_PLAYER_TECHNIQUES; i++) {
    //    tech_level[i] = get_technique_level(character, i);
    //}

    sprintf(myquery, "INSERT INTO %s "
        "(guildcard, slot"
        ", data0, data1, data2, data3, data4, data5, data6"
        ", data7, data8, data9, data10, data11, data12, data13"
        ", data14, data15, data16, data17, data18, data19"
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "'"
        ", '%02X', '%02X', '%02X', '%02X', '%02X', '%02X', '%02X'"
        ", '%02X', '%02X', '%02X', '%02X', '%02X', '%02X', '%02X'"
        ", '%02X', '%02X', '%02X', '%02X', '%02X', '%02X'"
        ")",
        CHARACTER_TECHNIQUES,
        gc, slot,
        tech->all[0], tech->all[1], tech->all[2], tech->all[3], tech->all[4], tech->all[5], tech->all[6],
        tech->all[7], tech->all[8], tech->all[9], tech->all[10], tech->all[11], tech->all[12], tech->all[13],
        tech->all[14], tech->all[15], tech->all[16], tech->all[17], tech->all[18], tech->all[19]
    );

    //DBG_LOG("保存角色更衣室数据 %d", dress_data->create_code);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", CHARACTER_TECHNIQUES, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -6;
    }

    return 0;
}

static int db_update_techniques(psocn_bb_char_t* character, uint32_t gc, uint32_t slot) {
    memset(myquery, 0, sizeof(myquery));
    techniques_t* tech = &character->technique_levels_v1;
    //uint8_t tech_level[20] = { 0 };

    //for (int i = 0; i < MAX_PLAYER_TECHNIQUES; i++) {
    //    tech_level[i] = get_technique_level(character, i);
    //}

    sprintf(myquery, "UPDATE %s SET "
        "data0 = '%02X', data1 = '%02X', data2 = '%02X', data3 = '%02X', data4 = '%02X', data5 = '%02X', data6 = '%02X', "
        "data7 = '%02X', data8 = '%02X', data9 = '%02X', data10 = '%02X', data11 = '%02X', data12 = '%02X', data13 = '%02X', "
        "data14 = '%02X', data15 = '%02X', data16 = '%02X', data17 = '%02X', data18 = '%02X', data19 = '%02X'"
        " WHERE "
        "guildcard = '%" PRIu32 "' AND slot =  '%" PRIu8 "'",
        CHARACTER_TECHNIQUES,
        tech->all[0], tech->all[1], tech->all[2], tech->all[3], tech->all[4], tech->all[5], tech->all[6],
        tech->all[7], tech->all[8], tech->all[9], tech->all[10], tech->all[11], tech->all[12], tech->all[13],
        tech->all[14], tech->all[15], tech->all[16], tech->all[17], tech->all[18], tech->all[19]
        , gc, slot
    );

    //DBG_LOG("更新角色更衣室数据");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", CHARACTER_TECHNIQUES, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -6;
    }
    
    return 0;
}

static int db_del_techniques(uint32_t gc, uint32_t slot) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "SELECT * FROM %s WHERE guildcard="
        "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_TECHNIQUES, gc,
        slot);

    if (!psocn_db_real_query(&conn, myquery)) {

        result = psocn_db_result_use(&conn);

        while ((row = psocn_db_result_fetch(result)) != NULL) {

            sprintf(myquery, "DELETE FROM %s WHERE guildcard="
                "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_TECHNIQUES, gc,
                slot);

            if (psocn_db_real_query(&conn, myquery)) {
                SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
                    PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_TECHNIQUES, gc, slot,
                    psocn_db_error(&conn));
                /* XXXX: 未完成给客户端发送一个错误信息 */
                return -1;
            }
        }

        psocn_db_result_free(result);
    }
    else {
        SQLERR_LOG("未找到旧玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 ")", CHARACTER_TECHNIQUES, gc, slot);
    }

    return 0;
}

int db_update_char_techniques(psocn_bb_char_t* character, uint32_t gc, uint32_t slot, uint32_t flag) {

    memset(myquery, 0, sizeof(myquery));

    if (flag & PSOCN_DB_SAVE_CHAR) {
        return db_insert_techniques(character, gc, slot);
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        if (db_update_techniques(character, gc, slot)) {

            if (db_del_techniques(gc, slot))
                return -1;

            /* TODO 这里需要检测 职业 新建人物的默认魔法值 */

            return db_insert_techniques(character, gc, slot);
        }
    }

    return 0;
}

int db_get_char_techniques(uint32_t gc, uint8_t slot, psocn_bb_char_t* character, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_TECHNIQUES, gc, slot);

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
        if (check) {
            SQLERR_LOG("未找到保存的角色数据 (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    if (row != NULL) {
        int i2 = 2;
        for (int i = 0; i < MAX_PLAYER_TECHNIQUES; i++) {
            character->technique_levels_v1.all[i] = (uint8_t)strtoul(row[i2], NULL, 16);
            //set_technique_level(character, i, (uint8_t)strtoul(row[i2], NULL, 16));

            i2++;
        }
    }

    psocn_db_result_free(result);

    return 0;
}
