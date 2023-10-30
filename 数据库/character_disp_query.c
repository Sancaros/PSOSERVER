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

#define TABLE CHARACTER_DISP

static int db_insert_char_disp(psocn_disp_char_t* disp_data,
    uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    if (disp_data->level + 1 > MAX_PLAYER_LEVEL) {
        disp_data->level = MAX_PLAYER_LEVEL - 1;
    }
    else if (disp_data->level + 1 == MAX_PLAYER_LEVEL) {
        disp_data->exp = MAX_PLAYER_EXP;
    }

    if (disp_data->meseta > MAX_PLAYER_MESETA)
        disp_data->meseta = MAX_PLAYER_MESETA;

    sprintf(myquery, "INSERT INTO "
        "%s ("
        "guildcard, slot, "
        "atp, mst, evp, hp, "
        "dfp, ata, lck, "
        "unknown_a1, unknown_a2, unknown_a3, "
        "level, exp, meseta"
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', "
        "'%u', '%u', '%u', '%u', "
        "'%u', '%u', '%u', "
        "'%u', '%f', '%f',"
        "'%u', '%u', '%u'"
        ")"
        , TABLE, gc, slot
        , disp_data->stats.atp, disp_data->stats.mst, disp_data->stats.evp, disp_data->stats.hp
        , disp_data->stats.dfp, disp_data->stats.ata, disp_data->stats.lck
        , disp_data->unknown_a1, disp_data->unknown_a2, disp_data->unknown_a3
        , disp_data->level + 1, disp_data->exp, disp_data->meseta
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -6;
    }

    return 0;
}

static int db_upd_char_disp(psocn_disp_char_t* disp_data,
    uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    if (disp_data->level + 1 > MAX_PLAYER_LEVEL) {
        disp_data->level = MAX_PLAYER_LEVEL - 1;
    }
    else if (disp_data->level + 1 == MAX_PLAYER_LEVEL) {
        disp_data->exp = MAX_PLAYER_EXP;
    }

    if (disp_data->meseta > MAX_PLAYER_MESETA)
        disp_data->meseta = MAX_PLAYER_MESETA;

    sprintf(myquery, "UPDATE %s SET"
        " atp='%u', mst='%u', evp='%u', hp='%u', dfp='%u', ata='%u', lck='%u',"
        " unknown_a1='%u', unknown_a2='%f', unknown_a3='%f',"
        " level='%u', exp='%u', meseta='%u'"
        " WHERE guildcard='%" PRIu32 "' AND slot='%" PRIu8 "'", TABLE
        , disp_data->stats.atp, disp_data->stats.mst, disp_data->stats.evp, disp_data->stats.hp, disp_data->stats.dfp, disp_data->stats.ata, disp_data->stats.lck
        , disp_data->unknown_a1, disp_data->unknown_a2, disp_data->unknown_a3
        , disp_data->level + 1, disp_data->exp, disp_data->meseta
        , gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -6;
    }

    return 0;
}

static int db_del_char_disp(uint32_t gc, uint8_t slot) {
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
int db_update_char_disp(psocn_disp_char_t* disp_data,
    uint32_t gc, uint8_t slot, uint32_t flag) {

    if (flag & PSOCN_DB_SAVE_CHAR) {

        if (db_insert_char_disp(disp_data, gc, slot)) {
            SQLERR_LOG("无法保存数值数据 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 ")", TABLE, gc, slot);
            return -1;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {

        if (db_check_bb_char_split_data_exist(gc, slot, TABLE)) {
            if (db_upd_char_disp(disp_data, gc, slot)) {
                SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", TABLE, gc, slot);

                return -2;
            }
        }
        else {
            if (db_del_char_disp(gc, slot)) {

                SQLERR_LOG("无法删除数值数据 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", TABLE, gc, slot);
                return -3;
            }

            if (db_insert_char_disp(disp_data, gc, slot)) {
                SQLERR_LOG("无法保存数值数据 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", TABLE, gc, slot);
                return -4;
            }
        }
    }

    return 0;
}

int db_get_char_disp(uint32_t gc, uint8_t slot, psocn_disp_char_t* data, int check) {
    void* result;
    char** row;
    int j = 0;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_DISP, gc, slot);

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

    /* 获取玩家角色7项基本数值 */

    j = 2;

    data->stats.atp = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    data->stats.mst = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    data->stats.evp = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    data->stats.hp = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    data->stats.dfp = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    data->stats.ata = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    data->stats.lck = (uint16_t)strtoul(row[j], NULL, 10);
    j++;

    data->unknown_a1 = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    data->unknown_a2 = strtof(row[j], NULL);
    j++;
    data->unknown_a3 = strtof(row[j], NULL);
    j++;

    data->level = (uint32_t)strtoul(row[j], NULL, 10) - 1;
    j++;
    data->exp = (uint32_t)strtoul(row[j], NULL, 10);
    j++;
    data->meseta = (uint32_t)strtoul(row[j], NULL, 10);

    if (data->level + 1 > MAX_PLAYER_LEVEL) {
        data->level = MAX_PLAYER_LEVEL - 1;
    }
    else if (data->level + 1 == MAX_PLAYER_LEVEL) {
        data->exp = MAX_PLAYER_EXP;
    }

    if (data->meseta > MAX_PLAYER_MESETA)
        data->meseta = MAX_PLAYER_MESETA;

    psocn_db_result_free(result);

    return 0;
}

