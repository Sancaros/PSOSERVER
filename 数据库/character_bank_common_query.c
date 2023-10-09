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
#include "handle_player_items.h"

static int db_insert_bank_common_param(psocn_bank_t* bank, uint32_t gc) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)bank, PSOCN_STLENGTH_BANK);

    memset(myquery, 0, sizeof(myquery));

    if (bank->meseta > MAX_PLAYER_MESETA)
        bank->meseta = MAX_PLAYER_MESETA;

    // 插入玩家数据
    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "guildcard, "
        "item_count, meseta, bank_check_num, "
        "full_data"
        ") VALUES ("
        "'%" PRIu32 "', "
        "'%" PRIu32 "', '%" PRIu32 "','%" PRIu32 "', '"
        /*")"*/,
        CHARACTER_BANK_COMMON,
        gc,
        bank->item_count, bank->meseta, inv_crc32
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)bank,
        PSOCN_STLENGTH_BANK);

    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        //SQLERR_LOG("psocn_db_real_query() 失败: %s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_insert_bank_common_backup_param(psocn_bank_t* bank, uint32_t gc) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)bank, PSOCN_STLENGTH_BANK);

    memset(myquery, 0, sizeof(myquery));

    if (bank->meseta > MAX_PLAYER_MESETA)
        bank->meseta = MAX_PLAYER_MESETA;

    // 插入玩家数据
    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "guildcard, "
        "item_count, meseta, bank_check_num, change_time, "
        "full_data"
        ") VALUES ("
        "'%" PRIu32 "', "
        "'%" PRIu32 "', '%" PRIu32 "','%" PRIu32 "', NOW(), '"
        /*")"*/,
        CHARACTER_BANK_COMMON_FULL_DATA,
        gc,
        bank->item_count, bank->meseta, inv_crc32
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)bank,
        PSOCN_STLENGTH_BANK);

    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        //SQLERR_LOG("psocn_db_real_query() 失败: %s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_update_bank_common_param(psocn_bank_t* bank, uint32_t gc) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)bank, PSOCN_STLENGTH_BANK);
    memset(myquery, 0, sizeof(myquery));

    if (bank->meseta > MAX_PLAYER_MESETA)
        bank->meseta = MAX_PLAYER_MESETA;

    _snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "item_count = '%" PRIu32 "', meseta = '%" PRIu32 "', bank_check_num = '%" PRIu32 "', "
        "`full_data` = '", CHARACTER_BANK_COMMON, bank->item_count, bank->meseta, inv_crc32);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)bank,
        PSOCN_STLENGTH_BANK);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "' WHERE `guildcard` = '%u'", gc);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("psocn_db_real_query() 失败: %s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

/* 优先获取银行数据库中的物品数量 */
static int db_get_char_bank_common_param(uint32_t gc, psocn_bank_t* bank, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT "
        "item_count, meseta"
        " FROM "
        "%s"
        " WHERE "
        "guildcard = '%" PRIu32 "'",
        CHARACTER_BANK_COMMON,
        gc
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("未找到保存的公共银行数据 (%" PRIu32 ")", gc);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    bank->item_count = (uint32_t)strtoul(row[0], NULL, 10);
    bank->meseta = (uint32_t)strtoul(row[1], NULL, 10);

    if (bank->meseta > MAX_PLAYER_MESETA)
        bank->meseta = MAX_PLAYER_MESETA;

    if (bank->item_count == 0 && bank->meseta == 0) {
        SQLERR_LOG("保存的公共银行数据为 %d 件 %d 美赛塔 (%" PRIu32 ")", bank->item_count, bank->meseta, gc);
        psocn_db_result_free(result);
        return -4;
    }

    psocn_db_result_free(result);

    return 0;
}

/* 优先获取银行数据库中的物品数量 */
static int db_get_char_bank_common_full_data(uint32_t gc, psocn_bank_t* bank, int backup) {
    void* result;
    char** row;
    const char* tbl_nm = CHARACTER_BANK_COMMON;

    memset(myquery, 0, sizeof(myquery));

    if (backup)
        tbl_nm = CHARACTER_BANK_COMMON_FULL_DATA;

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT full_data FROM "
        "%s "
        "WHERE "
        "guildcard = '%" PRIu32 "' "
        "ORDER BY change_time DESC "
        "LIMIT 1",
        tbl_nm,
        gc
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -3;
    }

    if (isPacketEmpty(row[0], sizeof(psocn_bank_t))) {
        psocn_db_result_free(result);

        SQLERR_LOG("保存的公共数据数据为空 (%" PRIu32 ")", gc);
        return -4;
    }

    memcpy(&bank->item_count, row[0], sizeof(psocn_bank_t));

    psocn_db_result_free(result);

    return 0;
}

static int db_cleanup_bank_common_items(uint32_t gc) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "DELETE FROM %s WHERE "
        "guildcard = '%" PRIu32 "'",
        CHARACTER_BANK_COMMON_ITEMS,
        gc
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理旧玩家银行数据 (GC %"
            PRIu32 "):\n%s", gc,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

static int db_insert_bank_common_items(bitem_t* item, uint32_t gc, int item_index) {
    char item_name_text[64];

    istrncpy(ic_gbk_to_utf8, item_name_text, item_get_name(&item->data, 5, 0), sizeof(item_name_text));

    memset(myquery, 0, sizeof(myquery));

    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "data_b0, data_b1, data_b2, data_b3, "
        "data_b4, data_b5, data_b6, data_b7, "
        "data_b8, data_b9, data_b10, data_b11, "
        "item_id, "
        "data2_b0, data2_b1, data2_b2, data2_b3, "
        "item_index, amount, show_flags, "
        "item_name, "
        "guildcard"
        ") VALUES ("
        "'%02X', '%02X', '%02X', '%02X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%08X', "
        "'%02X', '%02X', '%02X', '%02X', "
        "'%d', '%04X', '%04X', "
        "'%s', "
        "'%" PRIu32 "'"
        ")",
        CHARACTER_BANK_COMMON_ITEMS,
        item->data.datab[0], item->data.datab[1], item->data.datab[2], item->data.datab[3],
        item->data.datab[4], item->data.datab[5], item->data.datab[6], item->data.datab[7],
        item->data.datab[8], item->data.datab[9], item->data.datab[10], item->data.datab[11],
        item->data.item_id,
        item->data.data2b[0], item->data.data2b[1], item->data.data2b[2], item->data.data2b[3],
        item_index, item->amount, item->show_flags,
        item_name_text,
        gc
    );

    if (psocn_db_real_query(&conn, myquery)) {
#ifdef DEBUG
        SQLERR_LOG("无法新增玩家银行数据 (GC %"
            PRIu32 "):\n%s", gc,
            psocn_db_error(&conn));
#endif // DEBUG
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

/* 优先获取银行数据库中的数量 */
uint32_t db_get_char_bank_common_item_count(uint32_t gc) {
    uint32_t item_count = 0;
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT COUNT(*) FROM %s WHERE guildcard = '%" PRIu32 "'", 
        CHARACTER_BANK_COMMON_ITEMS, gc);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    item_count = (uint32_t)strtoul(row[0], NULL, 10);

    psocn_db_result_free(result);

    return item_count;
}

static int db_update_bank_common_items(bitem_t* item, uint32_t gc, int item_index) {
    char item_name_text[64];

    istrncpy(ic_gbk_to_utf8, item_name_text, item_get_name(&item->data, 5, 0), sizeof(item_name_text));

    memset(myquery, 0, sizeof(myquery));

    _snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "data_b0 = '%02X', data_b1 = '%02X', data_b2 = '%02X', data_b3 = '%02X', "
        "data_b4 = '%02X', data_b5 = '%02X', data_b6 = '%02X', data_b7 = '%02X', "
        "data_b8 = '%02X', data_b9 = '%02X', data_b10 = '%02X', data_b11 = '%02X', "
        "item_id = '%08X', "
        "data2_b0 = '%02X', data2_b1 = '%02X', data2_b2 = '%02X', data2_b3 = '%02X', "
        "amount = '%04X', show_flags = '%04X', "
        "item_name = '%s'"
        " WHERE "
        "guildcard = '%" PRIu32 "' AND item_index = '%d'",
        CHARACTER_BANK_COMMON_ITEMS,
        item->data.datab[0], item->data.datab[1], item->data.datab[2], item->data.datab[3],
        item->data.datab[4], item->data.datab[5], item->data.datab[6], item->data.datab[7],
        item->data.datab[8], item->data.datab[9], item->data.datab[10], item->data.datab[11],
        item->data.item_id,
        item->data.data2b[0], item->data.data2b[1], item->data.data2b[2], item->data.data2b[3],
        item->amount, item->show_flags,
        item_name_text,
        gc, item_index
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("psocn_db_real_query() 失败: %s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

/* 备份并删除旧银行物品数据 */
static int db_del_bank_common_items(uint32_t gc, int item_index, int del_count) {
    memset(myquery, 0, sizeof(myquery));
    void* result;
    char** row;

    sprintf(myquery, "SELECT * FROM %s WHERE "
        "guildcard = '%" PRIu32 "'",
        CHARACTER_BANK_COMMON_ITEMS,
        gc
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if (result = psocn_db_result_store(&conn)) {
        //DBG_LOG("正在修改 '%" PRIu32 "' '%" PRIu8 "' %d\n", gc, slot, item_index);
        if ((row = mysql_fetch_row((MYSQL_RES*)result))) {

            sprintf(myquery, "INSERT INTO %s SELECT * FROM %s WHERE "
                "guildcard = '%" PRIu32 "'",
                CHARACTER_BANK_COMMON_ITEMS_BACKUP, CHARACTER_BANK_COMMON_ITEMS,
                gc
            );

            psocn_db_real_query(&conn, myquery);

            for (item_index; item_index < MAX_PLAYER_BANK_ITEMS; item_index++) {

                sprintf(myquery, "DELETE FROM %s WHERE "
                    "guildcard = '%" PRIu32 "' AND item_index = '%" PRIu32 "'",
                    CHARACTER_BANK_COMMON_ITEMS,
                    gc, item_index
                );

                if (psocn_db_real_query(&conn, myquery)) {
                    SQLERR_LOG("无法清理旧玩家数据 (GC %"
                        PRIu32 ":\n%s", gc,
                        psocn_db_error(&conn));
                    /* XXXX: 未完成给客户端发送一个错误信息 */
                    return -1;
                }
            }

            //DBG_LOG("正在修改 %s\n", row[2]);

        }
        psocn_db_result_free(result);
    }


    return 0;
}

static int db_get_char_bank_common_items(uint32_t gc, bitem_t* item, int item_index, int check) {
    void* result;
    char** row;
    char* endptr;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT "
        "*"
        " FROM "
        "%s"
        " WHERE "
        "guildcard = '%" PRIu32 "' AND item_index = '%d'",
        CHARACTER_BANK_COMMON_ITEMS,
        gc, item_index
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("未找到索引 %d 的公共银行物品数据 (%" PRIu32 ")", item_index, gc);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    /* 获取物品的二进制数据 */
    int i = 3;
    item->amount = (uint16_t)strtoul(row[i], &endptr, 16);
    i++;
    item->show_flags = (uint16_t)strtoul(row[i], &endptr, 16);
    i++;

    item->data.datab[0] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[1] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[2] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[3] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[4] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[5] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[6] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[7] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[8] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[9] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[10] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.datab[11] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;

    item->data.item_id = (uint32_t)strtoul(row[i], &endptr, 16);
    i++;

    item->data.data2b[0] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data2b[1] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data2b[2] = (uint8_t)strtoul(row[i], &endptr, 16);
    i++;
    item->data.data2b[3] = (uint8_t)strtoul(row[i], &endptr, 16);

    if (*endptr != '\0') {
        SQLERR_LOG("获取的物品数据 索引 %d 字符串读取有误", item_index);
        // 转换失败，输入字符串中包含非十六进制字符
    }

    psocn_db_result_free(result);

    return 0;
}

static int db_get_char_bank_common_itemdata(uint32_t gc, psocn_bank_t* bank) {
    void* result;
    char** row;
    char* endptr;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT "
        "*"
        " FROM "
        "%s"
        " WHERE "
        "guildcard = '%" PRIu32 "'",
        CHARACTER_BANK_COMMON_ITEMS,
        gc
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_use(&conn)) == NULL) {
        SQLERR_LOG("未获取到公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    int k = 0, j = 0;

    while ((row = psocn_db_result_fetch(result)) != NULL) {
        if (!isEmptyInt((uint16_t)strtoul(row[4], &endptr, 16))) {
            j = 3;
            bank->bitems[k].amount = (uint16_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].show_flags = (uint16_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[0] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[1] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[2] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[3] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[4] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[5] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[6] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[7] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[8] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[9] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[10] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.datab[11] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.item_id = (uint32_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.data2b[0] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.data2b[1] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.data2b[2] = (uint8_t)strtoul(row[j], &endptr, 16);
            j++;
            bank->bitems[k].data.data2b[3] = (uint8_t)strtoul(row[j], &endptr, 16);

            k++;
        }
    }

    psocn_db_result_free(result);

    return k;
}

void clean_up_char_common_bank(psocn_bank_t* bank, int item_index, int del_count) {
    for (item_index; item_index < del_count; item_index++) {

        bank->bitems[item_index].data.datal[0] = 0;
        bank->bitems[item_index].data.datal[1] = 0;
        bank->bitems[item_index].data.datal[2] = 0;
        bank->bitems[item_index].data.item_id = 0xFFFFFFFF;
        bank->bitems[item_index].data.data2l = 0;

        bank->bitems[item_index].amount = 0;
        bank->bitems[item_index].show_flags = LE16(0x0000);
    }
}

/* 新增玩家银行银行数据至数据库 */
int db_insert_bank_common(psocn_bank_t* bank, uint32_t gc) {
    uint32_t inv_crc32 = psocn_crc32((uint8_t*)bank, PSOCN_STLENGTH_BANK);
    size_t i = 0;

    memset(myquery, 0, sizeof(myquery));

    // 插入玩家数据
    _snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "guildcardt, item_count, meseta, bank_check_num"
        ") VALUES ("
        "'%" PRIu32 "', "
        "'%" PRIu32 "', '%" PRIu32 "','%" PRIu32 "'"
        ")",
        CHARACTER_BANK_COMMON,
        gc,
        bank->item_count, bank->meseta, inv_crc32
    );

    if (psocn_db_real_query(&conn, myquery))
        db_update_bank_common_param(bank, gc);

    // 遍历银行数据，插入到数据库中
    for (i; i < bank->item_count; i++) {
        if (db_insert_bank_common_items(&bank->bitems[i], gc, i)) {
            db_update_bank_common_items(&bank->bitems[i], gc, i);
        }
    }

    clean_up_char_common_bank(bank, bank->item_count, MAX_PLAYER_BANK_ITEMS);

    if (db_del_bank_common_items(gc, bank->item_count, MAX_PLAYER_BANK_ITEMS))
        return -1;

    return 0;
}

int db_update_char_bank_common(psocn_bank_t* bank, uint32_t gc) {
    size_t i = 0, ic = bank->item_count;

    if (bank->item_count == 0)
        return 0;

    if (ic > MAX_PLAYER_BANK_ITEMS)
        ic = db_get_char_bank_common_item_count(gc);

    bank->item_count = ic;

    db_insert_bank_common_backup_param(bank, gc);

    if (db_update_bank_common_param(bank, gc)) {
        SQLERR_LOG("无法查询(GC%" PRIu32 ")公共银行参数数据", gc);

        db_insert_bank_common_param(bank, gc);

        for (i = 0; i < ic; i++) {
            if (db_insert_bank_common_items(&bank->bitems[i], gc, i))
                break;
        }

        return 0;
    }

    // 遍历银行数据，插入到数据库中
    for (i = 0; i < ic; i++) {
        if (db_insert_bank_common_items(&bank->bitems[i], gc, i)) {
#ifdef DEBUG
            SQLERR_LOG("无法新增(GC%" PRIu32 ")公共银行 %d 物品数据", gc, bank->item_count);
#endif // DEBUG
            if (db_update_bank_common_items(&bank->bitems[i], gc, i)) {
                SQLERR_LOG("无法更新(GC%" PRIu32 ")公共银行 %d 物品数据", gc, bank->item_count);
                return -1;
            }
        }
    }

    clean_up_char_common_bank(bank, ic, MAX_PLAYER_BANK_ITEMS);

    if (db_del_bank_common_items(gc, ic, MAX_PLAYER_BANK_ITEMS))
        return -1;

    return 0;
}

/* 获取玩家公共银行数据数据项 */
int db_get_char_bank_common(uint32_t gc, psocn_bank_t* bank) {
    size_t i = 0;
    int rv = 0;

    if (rv = db_get_char_bank_common_param(gc, bank, 0)) {

        if(rv == -4)
            if (db_get_char_bank_common_full_data(gc, bank, 0)) {
                SQLERR_LOG("获取(GC%" PRIu32 ")公共银行原始数据为空,执行获取备份数据计划.", gc);
                if (db_get_char_bank_common_full_data(gc, bank, 1)) {
                    SQLERR_LOG("获取(GC%" PRIu32 ")公共银行备份数据失败,执行初始化.", gc);
                    db_insert_bank_common_param(bank, gc);

                    db_insert_bank_common_backup_param(bank, gc);

                    for (i = 0; i < bank->item_count; i++) {
                        if (bank->bitems[i].show_flags)
                            if (db_insert_bank_common_items(&bank->bitems[i], gc, i))
                                break;
                    }
                }

            }

        return 0;
    }

    bank->item_count = db_get_char_bank_common_itemdata(gc, bank);

    clean_up_char_common_bank(bank, bank->item_count, MAX_PLAYER_BANK_ITEMS);

    if (db_del_bank_common_items(gc, bank->item_count, MAX_PLAYER_BANK_ITEMS))
        return -1;

    return 0;
}

/* 优先获取银行数据checkum */
uint32_t db_get_char_bank_common_checkum(uint32_t gc) {
    uint32_t bank_crc32 = 0;
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT bank_check_num FROM %s WHERE guildcard = '%" PRIu32 "'", 
        CHARACTER_BANK_COMMON, gc);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的公共银行数据 (%" PRIu32 ")", gc);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -3;
    }

    bank_crc32 = (uint32_t)strtoul(row[0], NULL, 10);

    psocn_db_result_free(result);

    return bank_crc32;
}
