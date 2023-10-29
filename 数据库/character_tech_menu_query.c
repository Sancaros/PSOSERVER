/*
    �λ�֮���й� ������ͨ�� ���ݿ����
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
#include "f_checksum.h"

static int db_del_char_tech_menu(uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "DELETE FROM %s"
        " WHERE "
        "guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'"
        , CHARACTER_TECH_MENU
        , gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷��������� %s ���� (GC %"
            PRIu32 ", ��λ %" PRIu8 "):\n%s", CHARACTER_TECH_MENU, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

static int db_updata_tech_menu(uint8_t* tech_menu, uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "UPDATE %s SET "
        "data = '",
        CHARACTER_TECH_MENU
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)tech_menu,
        PSOCN_STLENGTH_BB_DB_TECH_MENU);

    sprintf(myquery + strlen(myquery), "' WHERE guildcard = '%" PRIu32 "' AND "
        "slot = '%" PRIu8 "'", gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷�������� %s ���� (GC %"
            PRIu32 ", ��λ %" PRIu8 "):\n%s", CHARACTER_TECH_MENU, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

int db_insert_char_tech_menu(uint8_t* tech_menu, uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO %s "
        "(guildcard, slot, data)"
        " VALUES "
        "('%" PRIu32 "', '%" PRIu8 "', '"
        ,
        CHARACTER_TECH_MENU
        , gc, slot
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)tech_menu,
        PSOCN_STLENGTH_BB_DB_TECH_MENU);

    SAFE_STRCAT(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷��������ݱ� %s (GC %" PRIu32 ", "
            "��λ %" PRIu8 "):\n%s", CHARACTER_TECH_MENU, gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_char_tech_menu(uint8_t* tech_menu, uint32_t gc, uint8_t slot, uint32_t flag) {

    if (flag & PSOCN_DB_SAVE_CHAR) {
        if (db_insert_char_tech_menu(tech_menu, gc, slot)) {
            return -1;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        if (db_check_bb_char_split_data_exist(gc, slot, CHARACTER_TECH_MENU)) {
            if (db_updata_tech_menu(tech_menu, gc, slot)) {
                SQLERR_LOG("�޷��������ݱ� %s (GC %" PRIu32 ", "
                    "��λ %" PRIu8 ")", CHARACTER_TECH_MENU, gc, slot);

                return -2;
            }
        }
        else {

            if (db_del_char_tech_menu(gc, slot)) {
                SQLERR_LOG("�޷�ɾ������ %s (GC %" PRIu32 ", "
                    "��λ %" PRIu8 ")", CHARACTER_TECH_MENU, gc, slot);
                return -3;
            }

            if (db_insert_char_tech_menu(tech_menu, gc, slot)) {
                SQLERR_LOG("�޷��������� %s (GC %" PRIu32 ", "
                    "��λ %" PRIu8 ")", CHARACTER_TECH_MENU, gc, slot);
                return -4;
            }
        }
    }

    return 0;
}

/* ��ȡ���TECH_MENU���������� */
int db_get_char_tech_menu(uint32_t gc, uint8_t slot, uint8_t* tech_menu, int check) {
    void* result;
    char** row;
    //char* endptr;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%" PRIu8 "'", CHARACTER_TECH_MENU, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ���� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto build;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ���� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        goto build;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("δ�ҵ�����Ľ�ɫ���� (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        goto build;
    }

    ///* ��ȡ���������� */
    //int i = 2;
    //tech_menu->quest_guildcard = (uint32_t)strtoul(row[i], &endptr, 10);
    //i++;
    memcpy(tech_menu, row[0], PSOCN_STLENGTH_BB_DB_TECH_MENU);
    //i++;
    //tech_menu->quest_flags = (uint32_t)strtoul(row[i], &endptr, 10);

    //if (*endptr != '\0') {
    //    SQLERR_LOG("��ȡ������ ���� %d �ַ�����ȡ����", i);
    //    // ת��ʧ�ܣ������ַ����а�����ʮ�������ַ�
    //}

    psocn_db_result_free(result);

    return 0;

build:
    if (db_insert_char_tech_menu(tech_menu, gc, slot)) {
        return -1;
    }

    return 0;
}
