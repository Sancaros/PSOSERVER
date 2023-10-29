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

int db_updata_bb_char_create_code(uint32_t code,
    uint32_t gc, uint8_t slot) {
    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET create_code = '%d' "
        "WHERE guildcard = '%" PRIu32 "' AND slot =  '%" PRIu8 "'",
        CHARACTER_DRESS, code,
        gc, slot);
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新角色更衣室数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

static int db_insert_char_dress_data(psocn_dress_data_t* dress_data, uint32_t gc, uint8_t slot) {
    if (isPacketEmpty(dress_data->gc_string, sizeof(dress_data->gc_string))) {
        SQLERR_LOG("插入的角色数据为空 (%" PRIu32 ": %u)", gc, slot);
        return -4;
    }

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO %s ("
        "guildcard, slot, "//2
        "gc_string, "//1
        "unk1, "
        "name_color_b, name_color_g, name_color_r, name_color_transparency, "//5
        "model, "
        "unk3, "
        "create_code, name_color_checksum, section, "//5
        "ch_class, v2flags, version, v1flags, costume, "//5
        "skin, face, head, hair, "//4
        "hair_r, hair_g, hair_b, "
        "prop_x, prop_y"//5
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', "//2
        "'%s', "//1
        "'%s', "
        "'%d', '%d', '%d', '%d', "//5
        "'%d', "
        "'%s', "
        "'%d', '%d', '%d', "//5
        "'%d', '%d', '%d', '%d', '%d', "//5
        "'%d', '%d', '%d', '%d', "//4
        "'%d', '%d', '%d', "
        "'%f', '%f'"
        ")",//5
        CHARACTER_DRESS,
        gc, slot,
        (char*)dress_data->gc_string,
        (char*)dress_data->unk1,
        dress_data->name_color_b, dress_data->name_color_g, dress_data->name_color_r, dress_data->name_color_transparency,
        dress_data->model,
        (char*)dress_data->unk3,
        dress_data->create_code, dress_data->name_color_checksum, dress_data->section,
        dress_data->ch_class, dress_data->v2flags, dress_data->version, dress_data->v1flags, dress_data->costume,
        dress_data->skin, dress_data->face, dress_data->head, dress_data->hair,
        dress_data->hair_r, dress_data->hair_g, dress_data->hair_b,
        dress_data->prop_x, dress_data->prop_y
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -6;
    }

    return 0;
}

static int db_upd_char_dress_data(psocn_dress_data_t* dress_data, uint32_t gc, uint8_t slot) {
    if (isPacketEmpty(dress_data->gc_string, sizeof(dress_data->gc_string))) {
        SQLERR_LOG("更新的角色数据为空 (%" PRIu32 ": %u)", gc, slot);
        return -4;
    }

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "UPDATE %s SET "
        "guildcard =  '%" PRIu32 "', slot = '%" PRIu8 "', "
        "gc_string = '%s', "
        "unk1 = '%s', "
        "name_color_b = '%d', name_color_g = '%d', name_color_r = '%d', name_color_transparency = '%d', "
        "model = '%d', "
        "unk3 = '%s', "
        "create_code = '%d', name_color_checksum = '%d', section = '%d', "
        "ch_class = '%d', v2flags = '%d', version = '%d', v1flags = '%d', "
        "costume = '%d', skin = '%d', face = '%d', head = '%d', "
        "hair = '%d', hair_r = '%d', hair_g = '%d', hair_b = '%d', "
        "prop_x = '%f', prop_y = '%f'"
        " WHERE "
        "guildcard = '%" PRIu32 "' AND slot =  '%" PRIu8 "'",
        CHARACTER_DRESS,
        gc, slot,
        (char*)dress_data->gc_string,
        (char*)dress_data->unk1,
        dress_data->name_color_b, dress_data->name_color_g, dress_data->name_color_r, dress_data->name_color_transparency,
        dress_data->model,
        (char*)dress_data->unk3,
        dress_data->create_code, dress_data->name_color_checksum, dress_data->section,
        dress_data->ch_class, dress_data->v2flags, dress_data->version, dress_data->v1flags,
        dress_data->costume, dress_data->skin, dress_data->face, dress_data->head,
        dress_data->hair, dress_data->hair_r, dress_data->hair_g, dress_data->hair_b,
        dress_data->prop_x, dress_data->prop_y
        , gc, slot
    );

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -6;
    }

    return 0;
}

static int db_del_char_dress_data(uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "DELETE FROM %s WHERE guildcard="
        "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_DRESS, gc,
        slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

/* 更新玩家更衣室数据至数据库 */
int db_update_char_dress_data(psocn_dress_data_t* dress_data, uint32_t gc, uint8_t slot, uint32_t flag) {

    //DBG_LOG("更新角色更衣室数据");

    memset(myquery, 0, sizeof(myquery));

    if (flag & PSOCN_DB_SAVE_CHAR) {
        if (db_insert_char_dress_data(dress_data, gc, slot)) {
            SQLERR_LOG("无法保存外观数据 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 ")", CHARACTER_DRESS, gc, slot);
            return -1;
        }

    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {

        if (db_check_bb_char_split_data_exist(gc, slot, CHARACTER_DRESS)) {
            if (db_upd_char_dress_data(dress_data, gc, slot)) {
                SQLERR_LOG("无法更新外观数据 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", CHARACTER_DRESS, gc, slot);

                return -2;
            }
        }
        else {

            if (db_del_char_dress_data(gc, slot)) {
                SQLERR_LOG("无法删除外观数据 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", CHARACTER_DRESS, gc, slot);
                return -3;
            }

            SQLERR_LOG("插入新外观数据 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 ")", CHARACTER_DRESS, gc, slot);
            if (db_insert_char_dress_data(dress_data, gc, slot)) {
                SQLERR_LOG("无法保存外观数据 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 ")", CHARACTER_DRESS, gc, slot);
                return -4;
            }
        }
    }

    return 0;
}

int db_get_dress_data(uint32_t gc, uint8_t slot, psocn_dress_data_t* dress_data, int check) {
    void* result;
    char** row;
    int j = 0;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT "
        "*"
        " FROM "
        "%s"
        " WHERE "
        "guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_DRESS, gc, slot
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
        if (check) {
            SQLERR_LOG("未找到保存的角色数据 (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    j = 2;

    if (isPacketEmpty(row[j], sizeof(dress_data->gc_string))) {
        psocn_db_result_free(result);

        SQLERR_LOG("保存的角色数据为空 (%" PRIu32 ": %u)", gc, slot);
        return -4;
    }

    memcpy(&dress_data->gc_string, row[j], sizeof(dress_data->gc_string));
    j++;

    memcpy(&dress_data->unk1, row[j], sizeof(dress_data->unk1));
    j++;

    dress_data->name_color_b = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->name_color_g = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->name_color_r = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->name_color_transparency = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->model = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    memcpy(&dress_data->unk3, row[j], sizeof(dress_data->unk3));
    j++;
    dress_data->create_code = (uint32_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->name_color_checksum = (uint32_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->section = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->ch_class = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->v2flags = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->version = (uint8_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->v1flags = (uint32_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->costume = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->skin = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->face = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->head = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->hair = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->hair_r = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->hair_g = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->hair_b = (uint16_t)strtoul(row[j], NULL, 10);
    j++;
    dress_data->prop_x = strtof(row[j], NULL);
    j++;
    dress_data->prop_y = strtof(row[j], NULL);
    
    psocn_db_result_free(result);

    return 0;
}