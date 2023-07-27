/*
    �λ�֮���й� ��բ������ ���ݿ����
    ��Ȩ (C) 2022, 2023 Sancaros

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

/* ��ʼ�����ݿ����� */
extern psocn_dbconn_t conn;

#define TABLE CHARACTER_RECORDS_CHALLENGE

static int db_insert_c_records(bb_challenge_records_t* c_records, uint32_t gc, uint8_t slot) {

    memset(myquery, 0, sizeof(myquery));

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

        "grave_team, grave_message, unk3, string, rank_title, "

        "data"
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

        , c_records->grave_unk4, c_records->grave_deaths, c_records->unknown_u4

        /* grave_coords_time */
        , c_records->grave_coords_time[0], c_records->grave_coords_time[1], c_records->grave_coords_time[2], c_records->grave_coords_time[3]
        , c_records->grave_coords_time[4]

        /* battle */
        , c_records->battle[0], c_records->battle[1], c_records->battle[2], c_records->battle[3]
        , c_records->battle[4], c_records->battle[5], c_records->battle[6]

        );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->grave_team,
        20);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->grave_message,
        32);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->unk3,
        4);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->string,
        18);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records->rank_title,
        12);

    strcat(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&c_records,
        PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷�������ս���ݱ� %s (GC %" PRIu32 ", "
            "��λ %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
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
        SQLERR_LOG("�޷��������� %s ���� (GC %"
            PRIu32 ", ��λ %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

/* ������һ������������ݿ� */
int db_update_char_c_records(bb_challenge_records_t* c_records, uint32_t gc, uint8_t slot, uint32_t flag) {

    if (flag & PSOCN_DB_SAVE_CHAR) {

        if (db_insert_c_records(c_records, gc, slot)) {
            SQLERR_LOG("�޷�������ս���ݱ� %s (GC %" PRIu32 ", "
                "��λ %" PRIu8 ")", TABLE, gc, slot);
            return 0;
        }

    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {

        if (db_clean_up_char_c_records(gc, slot)) {
            SQLERR_LOG("�޷��������� %s ���� (GC %"
                PRIu32 ", ��λ %" PRIu8 ")", TABLE, gc, slot);
            return -1;
        }

        if (db_insert_c_records(c_records, gc, slot)) {
            SQLERR_LOG("�޷�������ս���ݱ� %s (GC %" PRIu32 ", "
                "��λ %" PRIu8 ")", TABLE, gc, slot);
            return 0;
        }
    }

    return 0;
}
