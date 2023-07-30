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

#define TABLE CHARACTER_DISP

static int db_insert_char_disp(psocn_disp_char_t* disp_data,
    uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO "
        "%s (guildcard, slot, "
        "atp, mst, evp, hp, "
        "dfp, ata, lck, "
        "opt_flags1, opt_flags2, opt_flags3, opt_flags4, opt_flags5, "
        "opt_flags6, opt_flags7, opt_flags8, opt_flags9, opt_flags10, "
        "level, exp, meseta) "
        "VALUES ('%" PRIu32 "', '%" PRIu8 "', "
        "'%d', '%d', '%d', '%d', "
        "'%d', '%d', '%d', "
        "'%d', '%d', '%d', '%d', '%d',"
        "'%d', '%d', '%d', '%d', '%d',"
        "'%d', '%d', '%d')"
        , TABLE, gc, slot
        , disp_data->stats.atp, disp_data->stats.mst, disp_data->stats.evp, disp_data->stats.hp
        , disp_data->stats.dfp, disp_data->stats.ata, disp_data->stats.lck
        , disp_data->opt_flag1, disp_data->opt_flag2, disp_data->opt_flag3, disp_data->opt_flag4, disp_data->opt_flag5
        , disp_data->opt_flag6, disp_data->opt_flag7, disp_data->opt_flag8, disp_data->opt_flag9, disp_data->opt_flag10
        , disp_data->level + 1, disp_data->exp, disp_data->meseta
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷��������ݱ� %s (GC %" PRIu32 ", "
            "��λ %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -6;
    }

    return 0;
}

static int db_upd_char_disp(psocn_disp_char_t* disp_data,
    uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "UPDATE %s SET"
        " atp='%d', mst='%d', evp='%d', hp='%d', dfp='%d', ata='%d', lck='%d',"
        " opt_flags1='%d', opt_flags2='%d', opt_flags3='%d', opt_flags4='%d', opt_flags5='%d',"
        " opt_flags6='%d', opt_flags7='%d', opt_flags8='%d', opt_flags9='%d', opt_flags10='%d',"
        " level='%d', exp='%d', meseta='%d'"
        " WHERE guildcard='%" PRIu32 "' AND slot='%" PRIu8 "'", TABLE
        , disp_data->stats.atp, disp_data->stats.mst, disp_data->stats.evp, disp_data->stats.hp, disp_data->stats.dfp, disp_data->stats.ata, disp_data->stats.lck
        , disp_data->opt_flag1, disp_data->opt_flag2, disp_data->opt_flag3, disp_data->opt_flag4, disp_data->opt_flag5
        , disp_data->opt_flag6, disp_data->opt_flag7, disp_data->opt_flag8, disp_data->opt_flag9, disp_data->opt_flag10
        , disp_data->level + 1, disp_data->exp, disp_data->meseta
        , gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷��������ݱ� %s (GC %" PRIu32 ", "
            "��λ %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
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
        SQLERR_LOG("�޷��������� %s ���� (GC %"
            PRIu32 ", ��λ %" PRIu8 "):\n%s", TABLE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

/* ������һ������������ݿ� */
int db_update_char_disp(psocn_disp_char_t* disp_data,
    uint32_t gc, uint8_t slot, uint32_t flag) {

    if (flag & PSOCN_DB_SAVE_CHAR) {

        if (db_insert_char_disp(disp_data, gc, slot)) {
            SQLERR_LOG("�޷�������ֵ���� %s (GC %" PRIu32 ", "
                "��λ %" PRIu8 ")", TABLE, gc, slot);
            return -1;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {

        if (db_upd_char_disp(disp_data, gc, slot)) {
            SQLERR_LOG("�޷��������ݱ� %s (GC %" PRIu32 ", "
                "��λ %" PRIu8 ")", TABLE, gc, slot);

            if (db_del_char_disp(gc, slot)) {

                SQLERR_LOG("�޷�ɾ����ֵ���� %s (GC %" PRIu32 ", "
                    "��λ %" PRIu8 ")", TABLE, gc, slot);
                return -3;
            }

            if (db_insert_char_disp(disp_data, gc, slot)) {
                SQLERR_LOG("�޷�������ֵ���� %s (GC %" PRIu32 ", "
                    "��λ %" PRIu8 ")", TABLE, gc, slot);
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
        SQLERR_LOG("�޷���ѯ��ɫ���� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ���� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("δ�ҵ�����Ľ�ɫ���� (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    /* ��ȡ��ҽ�ɫ7�������ֵ */

    j = 2;

    if (isEmptyString(row[j])) {
        psocn_db_result_free(result);

        SQLERR_LOG("����Ľ�ɫ����Ϊ�� (%" PRIu32 ": %u)", gc, slot);
        return -4;
    }

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

    data->opt_flag1 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag2 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag3 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag4 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag5 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag6 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag7 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag8 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag9 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    data->opt_flag10 = (uint8_t)strtoul(row[j], NULL, 10);
    j++;

    data->level = (uint32_t)strtoul(row[j], NULL, 10) - 1;
    j++;
    data->exp = (uint32_t)strtoul(row[j], NULL, 10);
    j++;
    data->meseta = (uint32_t)strtoul(row[j], NULL, 10);

    psocn_db_result_free(result);

    return 0;
}

