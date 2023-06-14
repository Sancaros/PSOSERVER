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
#include "iitems.h"

/* ��ʼ�����ݿ����� */
extern psocn_dbconn_t conn;

static int db_cleanup_bank_items(uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "DELETE FROM %s WHERE "
        "guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'",
        CHARACTER_BANK_ITEMS,
        gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷����������������� (GC %"
            PRIu32 ", ��λ %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

static int db_insert_bank_items(bitem_t* item, uint32_t gc, uint8_t slot, int item_index) {
    char item_name_text[32];

    istrncpy(ic_gbk_to_utf8, item_name_text, item_get_name(&item->data, 5), sizeof(item_name_text));

    memset(myquery, 0, sizeof(myquery));

    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "data_b0, data_b1, data_b2, data_b3, "
        "data_b4, data_b5, data_b6, data_b7, "
        "data_b8, data_b9, data_b10, data_b11, "
        "item_id, "
        "data2_b0, data2_b1, data2_b2, data2_b3, "
        "item_index, amount, show_flags, "
        "item_name, "
        "guildcard, slot"
        ") VALUES ("
        "'%02X', '%02X', '%02X', '%02X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%08X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%d', '%04X', '%04X', "
        "'%s', "
        "'%" PRIu32 "', '%" PRIu8 "'"
        ")",
        CHARACTER_BANK_ITEMS,
        item->data.data_b[0], item->data.data_b[1], item->data.data_b[2], item->data.data_b[3],
        item->data.data_b[4], item->data.data_b[5], item->data.data_b[6], item->data.data_b[7],
        item->data.data_b[8], item->data.data_b[9], item->data.data_b[10], item->data.data_b[11],
        item->data.item_id,
        item->data.data2_b[0], item->data.data2_b[1], item->data.data2_b[2], item->data.data2_b[3],
        item_index, item->amount, item->show_flags,
        item_name_text,
        gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���������������� (GC %"
            PRIu32 ", ��λ %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

static int db_update_bank_param(psocn_bank_t* bank, uint32_t gc, uint8_t slot) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)bank, sizeof(psocn_bank_t));
    memset(myquery, 0, sizeof(myquery));

    _snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "item_count = '%" PRIu32 "', meseta = '%" PRIu32 "', bank_check_num = '%" PRIu32 "'"
        " WHERE "
        "(guildcard = '%" PRIu32 "') AND (slot = '%" PRIu8 "')",
        CHARACTER_BANK,
        bank->item_count, bank->meseta, inv_crc32,
        gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("psocn_db_real_query() ʧ��: %s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

/* ���Ȼ�ȡ�������ݿ��е���Ʒ���� */
static int db_get_char_bank_param(uint32_t gc, uint8_t slot, psocn_bank_t* bank, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_BANK, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("δ�ҵ�����Ľ�ɫ�������� (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    int i = 2;
    bank->item_count = (uint32_t)strtoul(row[i], NULL, 0);
    i++;
    bank->meseta = (uint32_t)strtoul(row[i], NULL, 0);

    psocn_db_result_free(result);

    return 0;
}

static int db_get_char_bank_items(uint32_t gc, uint8_t slot, bitem_t* item, int item_index, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE "
        "guildcard = '%" PRIu32 "' AND (slot = '%" PRIu8 "') AND (item_index = '%d')",
        CHARACTER_BANK_ITEMS,
        gc, slot, item_index
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("δ�ҵ����� %d ��������Ʒ���� (%" PRIu32 ": %u)", item_index, gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    int i = 4;
    sscanf(row[i], "%hx", &item->amount);
    i++;
    sscanf(row[i], "%hx", &item->show_flags);
    i++;

    /* ��ȡ��Ʒ�Ķ��������� */
    sscanf(row[i], "%hhx", &item->data.data_b[0]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[1]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[2]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[3]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[4]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[5]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[6]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[7]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[8]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[9]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[10]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data_b[11]);
    i++;
    sscanf(row[i], "%X", &item->data.item_id);
    i++;
    sscanf(row[i], "%hhx", &item->data.data2_b[0]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data2_b[1]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data2_b[2]);
    i++;
    sscanf(row[i], "%hhx", &item->data.data2_b[3]);

    psocn_db_result_free(result);

    return 0;
}

/* ����������б������������ݿ� */
int db_insert_bank(psocn_bank_t* bank, uint32_t gc, uint8_t slot) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)bank, sizeof(psocn_bank_t));
    // ��ѯ������ ID
    uint32_t i = 0, item_index = 0;

    memset(myquery, 0, sizeof(myquery));

    // �����������
    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "guildcard, slot, item_count, meseta, bank_check_num"
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', "
        "'%" PRIu32 "', '%" PRIu32 "','%" PRIu32 "'"
        ")",
        CHARACTER_BANK,
        gc, slot,
        bank->item_count, bank->meseta, inv_crc32
    );

    if (psocn_db_real_query(&conn, myquery)) {
        db_update_bank_param(bank, gc, slot);
    }

    db_cleanup_bank_items(gc, slot);

    // �����������ݣ����뵽���ݿ���
    for (i; i < bank->item_count; i++) {
        if (db_insert_bank_items(&bank->bitems[i], gc, slot, item_index)) {
            SQLERR_LOG("�޷�����(GC%" PRIu32 ":%" PRIu8 "��)��ɫ������Ʒ����", gc, slot);
            return -1;
        }
        item_index++;
    }

    return 0;
}

int db_update_bank(psocn_bank_t* bank, uint32_t gc, uint8_t slot) {
    // ��ѯ������ ID
    uint32_t i = 0, item_index = 0;

    db_update_bank_param(bank, gc, slot);

    db_cleanup_bank_items(gc, slot);

    if (bank->item_count == 0)
        return 0;

    // �����������ݣ����뵽���ݿ���
    for (i; i < bank->item_count; i++) {
        if (db_insert_bank_items(&bank->bitems[i], gc, slot, item_index)) {
            SQLERR_LOG("�޷�����(GC%" PRIu32 ":%" PRIu8 "��)��ɫ������Ʒ����", gc, slot);
            return -1;
        }
        item_index++;
    }

    return 0;
}

/* ��ȡ��ҽ�ɫ�������������� */
int db_get_char_bank(uint32_t gc, uint8_t slot, psocn_bank_t* bank, int check) {
    uint32_t i = 0;

    if (db_get_char_bank_param(gc, slot, bank, 0)) {
        //SQLERR_LOG("�޷���ѯ(GC%" PRIu32 ":%" PRIu8 "��)��ɫ���в�������", gc, slot);
        return -1;
    }

    /* �������Ϊ���򷵻�0 */
    if (bank->item_count == 0) {
        //SQLERR_LOG("�޷���ѯ(GC%" PRIu32 ":%" PRIu8 "��)��ɫ������Ʒ����", gc, slot);
        return 0;
    }

    for (i; i < MAX_PLAYER_BANK_ITEMS; i++) {
        if (db_get_char_bank_items(gc, slot, &bank->bitems[i], i, 0))
            break;
    }

    return 0;
}

/* ���Ȼ�ȡ��������checkum */
uint32_t db_get_char_bank_checkum(uint32_t gc, uint8_t slot) {
    uint32_t bank_crc32 = 0;
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT bank_check_num FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_BANK, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("δ�ҵ�����Ľ�ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -3;
    }

    bank_crc32 = (uint32_t)strtoul(row[0], NULL, 0);

    psocn_db_result_free(result);

    return bank_crc32;
}
