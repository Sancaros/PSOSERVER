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

/* ������һ������������ݿ� */
int db_update_char_c_records(psocn_bb_db_char_t* char_data, uint32_t gc, uint8_t slot, uint32_t flag) {
    static char query[PSOCN_STLENGTH_BB_DB_CHAR * 2 + 256];
    char name[64];
    char class_name[64];

    istrncpy16_raw(ic_utf16_to_utf8, name, &char_data->character.name.char_name[0], 64, BB_CHARACTER_CHAR_NAME_LENGTH);

    istrncpy(ic_gbk_to_utf8, class_name, pso_class[char_data->character.dress_data.ch_class].cn_name, 64);

    if (flag & PSOCN_DB_SAVE_CHAR) {
        sprintf(query, "INSERT INTO %s(guildcard, slot, name, class_name, version, data) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%s', '%" PRIu8 "', '", CHARACTER_RECORDS_CHALLENGE, gc, slot
            , name, class_name, char_data->character.dress_data.version);

        psocn_db_escape_str(&conn, query + strlen(query), (char*)&char_data->c_records,
            PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

        strcat(query, "')");

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("�޷�������ս���ݱ� %s (GC %" PRIu32 ", "
                "��λ %" PRIu8 "):\n%s", CHARACTER_RECORDS_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            return 0;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        sprintf(query, "DELETE FROM %s WHERE guildcard="
            "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_RECORDS_CHALLENGE, gc,
            slot);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("�޷��������� %s ���� (GC %"
                PRIu32 ", ��λ %" PRIu8 "):\n%s", CHARACTER_RECORDS_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
            return -1;
        }

        sprintf(query, "INSERT INTO %s(guildcard, slot, name, class_name, version, data) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%s', '%" PRIu8 "', '", CHARACTER_RECORDS_CHALLENGE, gc, slot
            , name, class_name, char_data->character.dress_data.version);

        psocn_db_escape_str(&conn, query + strlen(query), (char*)&char_data->c_records,
            PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

        strcat(query, "')");

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("�޷�������ս���ݱ� %s (GC %" PRIu32 ", "
                "��λ %" PRIu8 "):\n%s", CHARACTER_RECORDS_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            return 0;
        }
    }

    return 0;
}
