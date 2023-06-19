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

static int db_insert_inv_param(inventory_t* inv, uint32_t gc, uint8_t slot) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)inv, sizeof(inventory_t));

    memset(myquery, 0, sizeof(myquery));

    // �����������
    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "guildcard, slot, item_count, hpmats_used, tpmats_used, language, inv_check_num"
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', "
        "'%" PRIu8 "', '%" PRIu8 "', '%" PRIu8 "', '%" PRIu8 "','%" PRIu32 "'"
        ")",
        CHARACTER_INVENTORY,
        gc, slot,
        inv->item_count, inv->hpmats_used, inv->tpmats_used, inv->language, inv_crc32
    );

    if (psocn_db_real_query(&conn, myquery)) {
        //SQLERR_LOG("psocn_db_real_query() ʧ��: %s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_update_inv_param(inventory_t* inv, uint32_t gc, uint8_t slot) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)inv, sizeof(inventory_t));
    memset(myquery, 0, sizeof(myquery));

    _snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "item_count = '%" PRIu8 "', hpmats_used = '%" PRIu8 "', tpmats_used = '%" PRIu8 "', language = '%" PRIu8 "', inv_check_num = '%" PRIu32 "'"
        " WHERE "
        "(guildcard = '%" PRIu32 "') AND (slot = '%" PRIu8 "')",
        CHARACTER_INVENTORY,
        inv->item_count, inv->hpmats_used, inv->tpmats_used, inv->language, inv_crc32,
        gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        //SQLERR_LOG("psocn_db_real_query() ʧ��: %s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_update_inv_items(iitem_t* item, uint32_t gc, uint8_t slot, int item_index) {
    char item_name_text[256];

    istrncpy(ic_gbk_to_utf8, item_name_text, item_get_name(&item->data, 5), sizeof(item_name_text));

    memset(myquery, 0, sizeof(myquery));

    _snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "data_b0 = '%02X', data_b1 = '%02X', data_b2 = '%02X', data_b3 = '%02X', "
        "data_b4 = '%02X', data_b5 = '%02X', data_b6 = '%02X', data_b7 = '%02X', "
        "data_b8 = '%02X', data_b9 = '%02X', data_b10 = '%02X', data_b11 = '%02X', "
        "item_id = '%08X', "
        "data2_b0 = '%02X', data2_b1 = '%02X', data2_b2 = '%02X', data2_b3 = '%02X', "
        "present = '%04X', tech = '%04X', flags = '%08X', "
        "item_name = '%s'"
        " WHERE "
        "(guildcard = '%" PRIu32 "') AND (slot = '%" PRIu8 "') AND (item_index = '%d')",
        CHARACTER_INVENTORY_ITEMS,
        item->data.data_b[0], item->data.data_b[1], item->data.data_b[2], item->data.data_b[3],
        item->data.data_b[4], item->data.data_b[5], item->data.data_b[6], item->data.data_b[7],
        item->data.data_b[8], item->data.data_b[9], item->data.data_b[10], item->data.data_b[11],
        item->data.item_id,
        item->data.data2_b[0], item->data.data2_b[1], item->data.data2_b[2], item->data.data2_b[3],
        item->present, item->tech, item->flags,
        item_name_text,
        gc, slot, item_index
    );

    if (psocn_db_real_query(&conn, myquery)) {
        //SQLERR_LOG("psocn_db_real_query() ʧ��: %s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_insert_inv_items(iitem_t* item, uint32_t gc, uint8_t slot, int item_index) {
    char item_name_text[32];

    istrncpy(ic_gbk_to_utf8, item_name_text, item_get_name(&item->data, 5), sizeof(item_name_text));

    memset(myquery, 0, sizeof(myquery));

    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "data_b0, data_b1, data_b2, data_b3, "
        "data_b4, data_b5, data_b6, data_b7, "
        "data_b8, data_b9, data_b10, data_b11, "
        "item_id, "
        "data2_b0, data2_b1, data2_b2, data2_b3, "
        "item_index, present, tech, flags, "
        "item_name, "
        "guildcard, slot"
        ") VALUES ("
        "'%02X', '%02X', '%02X', '%02X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%08X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%d', '%04X', '%04X', '%08X', "
        "'%s', "
        "'%" PRIu32 "', '%" PRIu8 "'"
        ")",
        CHARACTER_INVENTORY_ITEMS,
        item->data.data_b[0], item->data.data_b[1], item->data.data_b[2], item->data.data_b[3],
        item->data.data_b[4], item->data.data_b[5], item->data.data_b[6], item->data.data_b[7],
        item->data.data_b[8], item->data.data_b[9], item->data.data_b[10], item->data.data_b[11],
        item->data.item_id,
        item->data.data2_b[0], item->data.data2_b[1], item->data.data2_b[2], item->data.data2_b[3],
        item_index, item->present, item->tech, item->flags,
        item_name_text,
        gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        //SQLERR_LOG("�޷�������ұ������� (GC %"
        //    PRIu32 ", ��λ %" PRIu8 "):\n%s", gc, slot,
        //    psocn_db_error(&conn));
        /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
        return -1;
    }

    return 0;
}

static int db_del_inv_items(uint32_t gc, uint8_t slot, int item_index, int del_count) {
    memset(myquery, 0, sizeof(myquery));
    void* result;
    char** row;

    sprintf(myquery, "SELECT * FROM %s WHERE "
        "guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'",
        CHARACTER_INVENTORY_ITEMS,
        gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if (result = psocn_db_result_store(&conn)) {
        //DBG_LOG("�����޸� '%" PRIu32 "' '%" PRIu8 "' %d\n", gc, slot, item_index);
        if ((row = mysql_fetch_row((MYSQL_RES*)result))) {

            sprintf(myquery, "INSERT INTO %s SELECT * FROM %s WHERE "
                "guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'",
                CHARACTER_INVENTORY_ITEMS_BACKUP, CHARACTER_INVENTORY_ITEMS,
                gc, slot
            );

            psocn_db_real_query(&conn, myquery);

            for (item_index; item_index < MAX_PLAYER_INV_ITEMS; item_index++) {

                sprintf(myquery, "DELETE FROM %s WHERE "
                    "guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "' AND item_index = '%" PRIu32 "'",
                    CHARACTER_INVENTORY_ITEMS,
                    gc, slot, item_index
                );

                if (psocn_db_real_query(&conn, myquery)) {
                    SQLERR_LOG("�޷������������� (GC %"
                        PRIu32 ", ��λ %" PRIu8 "):\n%s", gc, slot,
                        psocn_db_error(&conn));
                    /* XXXX: δ��ɸ��ͻ��˷���һ��������Ϣ */
                    return -1;
                }
            }

            //DBG_LOG("�����޸� %s\n", row[2]);

        }
        psocn_db_result_free(result);
    }


    return 0;
}

/* ���Ȼ�ȡ�������ݿ��е���Ʒ���� */
static int db_get_char_inv_param(uint32_t gc, uint8_t slot, inventory_t* inv, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_INVENTORY, gc, slot);

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

    int i = 2;
    inv->item_count = (uint8_t)strtoul(row[i], NULL, 16);
    i++;
    inv->hpmats_used = (uint8_t)strtoul(row[i], NULL, 16);
    i++;
    inv->tpmats_used = (uint8_t)strtoul(row[i], NULL, 16);
    i++;
    inv->language = (uint8_t)strtoul(row[i], NULL, 16);

    psocn_db_result_free(result);

    return 0;
}

static int db_get_char_inv_items(uint32_t gc, uint8_t slot, iitem_t* item, int item_index, int check) {
    void* result;
    char** row;
    char* endptr;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE "
        "guildcard = '%" PRIu32 "' AND (slot = '%" PRIu8 "') AND (item_index = '%d')",
        CHARACTER_INVENTORY_ITEMS,
        gc, slot, item_index
    );

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
            SQLERR_LOG("δ�ҵ����� %d �ı�����Ʒ���� (%" PRIu32 ": %u)", item_index, gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    /* ��ȡ��Ʒ�Ķ��������� */
    int i = 4;
    item->present = (uint16_t)strtoul(row[i], &endptr, 16);
    i++;
    item->tech = (uint16_t)strtoul(row[i], &endptr, 16);
    i++;
    item->flags = (uint32_t)strtoul(row[i], &endptr, 16);
    i++;

    item->data.data_b[0] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[1] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[2] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[3] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[4] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[5] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[6] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[7] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[8] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[9] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[10] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data_b[11] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;

    item->data.item_id = (uint32_t)strtoul(row[i], &endptr, 16);
    i++;

    item->data.data2_b[0] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data2_b[1] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data2_b[2] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data2_b[3] = (uint8_t)strtoul(row[i], &endptr, 16);

    if (*endptr != '\0') {
        SQLERR_LOG("��ȡ����Ʒ���� ���� %d �ַ�����ȡ����", item_index);
        // ת��ʧ�ܣ������ַ����а�����ʮ�������ַ�
    }

    psocn_db_result_free(result);

    return 0;
}

/* ������ұ������������ݿ� */
int db_insert_char_inv(inventory_t* inv, uint32_t gc, uint8_t slot) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)inv, sizeof(inventory_t));
    size_t i = 0;

    memset(myquery, 0, sizeof(myquery));

    // �����������
    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "guildcard, slot, item_count, hpmats_used, tpmats_used, language, inv_check_num"
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', "
        "'%" PRIu8 "', '%" PRIu8 "', '%" PRIu8 "', '%" PRIu8 "','%" PRIu32 "'"
        ")",
        CHARACTER_INVENTORY,
        gc, slot,
        inv->item_count, inv->hpmats_used, inv->tpmats_used, inv->language, inv_crc32
    );

    if (psocn_db_real_query(&conn, myquery))
        db_update_inv_param(inv, gc, slot);

    // �����������ݣ����뵽���ݿ���
    for (i; i < inv->item_count; i++) {
        if (db_insert_inv_items(&inv->iitems[i], gc, slot, i)) {
            db_update_inv_items(&inv->iitems[i], gc, slot, i);
        }
    }

    if (db_del_inv_items(gc, slot, inv->item_count, MAX_PLAYER_INV_ITEMS))
        return -1;

    return 0;
}

int db_update_char_inv(inventory_t* inv, uint32_t gc, uint8_t slot) {
    uint8_t db_item_count = 0;
    size_t i = 0;
    uint8_t ic = inv->item_count;

    ///* ��ȡ���ݿ��� ������Ʒ������ ���ڶԱ� */
    //db_item_count = db_get_char_inv_item_count(gc, slot);

    ///* ȡĿǰ�������� */
    //if (ic != db_item_count)
    //    ic = db_item_count;

    if (ic > MAX_PLAYER_INV_ITEMS)
        ic = db_get_char_inv_item_count(gc, slot);;

    inv->item_count = ic;

    if (db_update_inv_param(inv, gc, slot)) {
        //SQLERR_LOG("�޷���ѯ(GC%" PRIu32 ":%" PRIu8 "��)��ɫ������������", gc, slot);
        return -1;
    }

    // �����������ݣ����뵽���ݿ���
    for (i = 0; i < ic; i++) {
        if (db_insert_inv_items(&inv->iitems[i], gc, slot, i)) {
            if (db_update_inv_items(&inv->iitems[i], gc, slot, i)) {
                SQLERR_LOG("�޷�����(GC%" PRIu32 ":%" PRIu8 "��)��ɫ������Ʒ����", gc, slot);
                return -1;
            }
        }
    }

    if (db_del_inv_items(gc, slot, ic, MAX_PLAYER_INV_ITEMS))
        return -1;

    return 0;
}

/* ��ȡ��ҽ�ɫ�������������� */
int db_get_char_inv(uint32_t gc, uint8_t slot, inventory_t* inv, int check) {
    uint8_t db_item_count = 0;
    size_t i = 0, ic = inv->item_count;

    /* ��ȡ���ݿ��� ������Ʒ������ ���ڶԱ� */
    db_item_count = db_get_char_inv_item_count(gc, slot);

    if (ic != db_item_count)
        ic = db_item_count;

    if (ic > MAX_PLAYER_INV_ITEMS)
        ic = MAX_PLAYER_INV_ITEMS;

    inv->item_count = (uint8_t)ic;

    if (db_get_char_inv_param(gc, slot, inv, 0)) {
        //SQLERR_LOG("�޷���ѯ(GC%" PRIu32 ":%" PRIu8 "��)��ɫ������������", gc, slot);
        goto build;
    }

    for (i = 0; i < ic; i++) {
        if (db_get_char_inv_items(gc, slot, &inv->iitems[i], i, 0))
            break;
    }

    if (db_del_inv_items(gc, slot, ic, MAX_PLAYER_INV_ITEMS))
        return -1;

    return 0;

build:
    db_insert_inv_param(inv, gc, slot);

    for (i = 0; i < ic; i++) {
        if (db_insert_inv_items(&inv->iitems[i], gc, slot, i))
            break;
    }

    return 0;
}

/* ���Ȼ�ȡ�������ݿ��е����� */
uint8_t db_get_char_inv_item_count(uint32_t gc, uint8_t slot) {
    uint8_t item_count = 0;
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT COUNT(*) FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_INVENTORY_ITEMS, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("�޷���ѯ��ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("δ��ȡ����ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("δ�ҵ�����Ľ�ɫ�������� (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    item_count = (uint8_t)strtoul(row[0], NULL, 0);

    psocn_db_result_free(result);

    return item_count;
}

/* ���Ȼ�ȡ��������checkum */
uint32_t db_get_char_inv_checkum(uint32_t gc, uint8_t slot) {
    uint32_t inv_crc32 = 0;
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT inv_check_num FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_INVENTORY, gc, slot);

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

    inv_crc32 = (uint32_t)strtoul(row[0], NULL, 0);

    psocn_db_result_free(result);

    return inv_crc32;
}