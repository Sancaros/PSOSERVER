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

#define TABLE CHARACTER_RECORDS_CHALLENGE

static int db_insert_c_records(bb_challenge_records_t* c_records, uint32_t gc, uint8_t slot) {
    char tmp_name[20];

    memset(tmp_name, 0, 20);

    istrncpy16_raw(ic_utf16_to_utf8, tmp_name, (char*)&c_records->title_str, 20, 10);

    c_records->grave_team_tag = 0x0009;
    c_records->grave_team_tag2 = 0x0042;

    if (c_records->title_tag != 0x002D)
        c_records->title_tag = 0x002D;

    if (c_records->title_tag2 != 0x007C)
        c_records->title_tag2 = 0x007C;

    memset(myquery, 0, sizeof(myquery));

    //print_ascii_hex(c_records, sizeof(bb_challenge_records_t));

    sprintf(myquery, "INSERT INTO %s("
        "guildcard, slot, "
        "title_color, unknown_u0, "

        "times_ep1_online0, times_ep1_online1, times_ep1_online2, times_ep1_online3, "
        "times_ep1_online4, times_ep1_online5, times_ep1_online6, times_ep1_online7, "
        "times_ep1_online8, "

        "times_ep2_online0, times_ep2_online1, times_ep2_online2, times_ep2_online3, "
        "times_ep2_online4, "

        "times_ep1_offline0, times_ep1_offline1, times_ep1_offline2, times_ep1_offline3, "
        "times_ep1_offline4, times_ep1_offline5, times_ep1_offline6, times_ep1_offline7, "
        "times_ep1_offline8, "

        "grave_unk4, grave_deaths, unknown_u4, "

        "grave_coords_time0, grave_coords_time1, grave_coords_time2, grave_coords_time3, "
        "grave_coords_time4, "

        "battle0, battle1, battle2, battle3, "
        "battle4, battle5, battle6, "
        
        "title_tag, title_tag2, rank_title, "

        "grave_team, grave_message, unk3, string"

        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', "

        "'%04X', '%04X', "

        /* times_ep1_online */
        "'%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', "
        "'%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', "
        "'%" PRIu32 "', "

        /* times_ep2_online */
        "'%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', "
        "'%" PRIu32 "', "

        /* times_ep1_offline */
        "'%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', "
        "'%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', "
        "'%" PRIu32 "', "

        "'%" PRIu32 "', '%" PRIu32 "', '%04X', "

        /* grave_coords_time */
        "'%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', "
        "'%" PRIu32 "', "

        /* battle */
        "'%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', "
        "'%" PRIu32 "', '%" PRIu32 "', '%" PRIu32 "', "

        "'%04X', '%04X', '%s', "

        "'"

        , TABLE
        , gc, slot
        , c_records->title_color, c_records->unknown_u0

        /* times_ep1_online */
        , c_records->times_ep1_online[0], c_records->times_ep1_online[1], c_records->times_ep1_online[2], c_records->times_ep1_online[3]
        , c_records->times_ep1_online[4], c_records->times_ep1_online[5], c_records->times_ep1_online[6], c_records->times_ep1_online[7]
        , c_records->times_ep1_online[8]

        /* times_ep2_online */
        , c_records->times_ep2_online[0], c_records->times_ep2_online[1], c_records->times_ep2_online[2], c_records->times_ep2_online[3]
        , c_records->times_ep2_online[4]

        /* times_ep1_offline */
        , c_records->times_ep1_offline[0], c_records->times_ep1_offline[1], c_records->times_ep1_offline[2], c_records->times_ep1_offline[3]
        , c_records->times_ep1_offline[4], c_records->times_ep1_offline[5], c_records->times_ep1_offline[6], c_records->times_ep1_offline[7]
        , c_records->times_ep1_offline[8]

        , c_records->grave_is_ep2, c_records->grave_deaths, c_records->grave_floor

        /* grave_coords_time */
        , c_records->grave_time, c_records->grave_defeated_by_enemy_rt_index, c_records->grave_x, c_records->grave_y
        , c_records->grave_z

        /* battle */
        , c_records->battle[0], c_records->battle[1], c_records->battle[2], c_records->battle[3]
        , c_records->battle[4], c_records->battle[5], c_records->battle[6]

        , c_records->title_tag, c_records->title_tag2, tmp_name

        );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->grave_team,
        20);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->grave_message,
        32);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->unk3,
        4);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->unknown_t6[0],
        18);

    //SAFE_STRCAT(myquery, "', '");

    //psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->title_str,
    //    10);

    SAFE_STRCAT(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法保存挑战数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        SQLERR_LOG("语句 %s", myquery);
        return -1;
    }

    return 0;
}

static int db_clean_up_char_c_records(uint32_t gc, uint8_t slot) {
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
int db_update_char_c_records(bb_challenge_records_t* c_records, uint32_t gc, uint8_t slot, uint32_t flag) {

    if (flag & PSOCN_DB_SAVE_CHAR) {

        if (db_insert_c_records(c_records, gc, slot)) {
            SQLERR_LOG("无法保存挑战数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 ")", TABLE, gc, slot);
            return 0;
        }

    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {

        //if (db_check_bb_char_split_data_exist(gc, slot, TABLE)) {
        //    if (db_updata_tech_menu(tech_menu, gc, slot)) {
        //        SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
        //            "槽位 %" PRIu8 ")", TABLE, gc, slot);

        //        return -2;
        //    }
        //}
        //else {
            if (db_clean_up_char_c_records(gc, slot)) {
                SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
                    PRIu32 ", 槽位 %" PRIu8 ")", TABLE, gc, slot);
                return -1;
            }

            if (db_insert_c_records(c_records, gc, slot)) {
                SQLERR_LOG("无法保存挑战数据表 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", TABLE, gc, slot);
                return 0;
            }
        //}
    }

    return 0;
}

int db_get_c_records(uint32_t gc, uint8_t slot, bb_challenge_records_t* c_records) {
    void* result;
    char tmp_name[20] = { 0 };
    char** row;
    size_t in, out;
    char* inptr;
    char* outptr;
    int i = 0, j;

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

    j = 2;

    if (isPacketEmpty(row[j], 2)) {
        psocn_db_result_free(result);

        SQLERR_LOG("保存的角色数据为空 (%" PRIu32 ": %u)", gc, slot);
        return -4;
    }

    c_records->title_color = (uint16_t)strtoul(row[j], NULL, 16);
    j++;

    c_records->unknown_u0 = (uint16_t)strtoul(row[j], NULL, 16);
    j++;

    for (i = 0; i < 9; i++) {
        c_records->times_ep1_online[i] = (uint32_t)strtoul(row[j], NULL, 10);
        j++;
    }

    for (i = 0; i < 5; i++) {
        c_records->times_ep2_online[i] = (uint32_t)strtoul(row[j], NULL, 10);
        j++;
    }

    for (i = 0; i < 9; i++) {
        c_records->times_ep1_offline[i] = (uint32_t)strtoul(row[j], NULL, 10);
        j++;
    }

    c_records->grave_is_ep2 = (uint32_t)strtoul(row[j], NULL, 10);
    j++;

    c_records->grave_deaths = (uint16_t)strtoul(row[j], NULL, 10);
    j++;

    c_records->grave_floor = (uint16_t)strtoul(row[j], NULL, 16);
    j++;

    c_records->grave_time = (uint32_t)strtoul(row[j], NULL, 10);
    j++;

    c_records->grave_defeated_by_enemy_rt_index = (uint32_t)strtoul(row[j], NULL, 10);
    j++;

    c_records->grave_x = (float)strtoul(row[j], NULL, 10);
    j++;

    c_records->grave_y = (float)strtoul(row[j], NULL, 10);
    j++;

    c_records->grave_z = (float)strtoul(row[j], NULL, 10);
    j++;

    memcpy((char*)&c_records->grave_team, row[j], 20);
    j++;

    c_records->grave_team_tag = 0x0009;
    c_records->grave_team_tag2 = 0x0042;

    memcpy((char*)&c_records->grave_message, row[j], 32);
    j++;

    memcpy((char*)&c_records->unk3, row[j], 4);
    j++;

    memcpy((char*)&c_records->unknown_t6[0], row[j], 18);
    j++;

    c_records->title_tag = (uint16_t)strtoul(row[j], NULL, 16);
    if (c_records->title_tag != 0x002D)
        c_records->title_tag = 0x002D;

    j++;

    c_records->title_tag2 = (uint16_t)strtoul(row[j], NULL, 16);
    if (c_records->title_tag2 != 0x007C)
        c_records->title_tag2 = 0x007C;

    j++;

    memcpy(&tmp_name[0], row[j], 20);

    memset(c_records->title_str, 0, sizeof(c_records->title_str));

    in = strlen(tmp_name);
    inptr = tmp_name;
    out = 0x14;
    outptr = (char*)&c_records->title_str[0];

    iconv(ic_utf8_to_utf16, &inptr, &in, &outptr, &out);

    j++;

    for (i = 0; i < 7; i++) {
        c_records->battle[i] = (uint32_t)strtoul(row[j], NULL, 10);
        j++;
    }

    psocn_db_result_free(result);

    return 0;
}
