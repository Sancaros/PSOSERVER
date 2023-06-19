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

/* 初始化数据库连接 */
extern psocn_dbconn_t conn;

#define TABLE1 CHARACTER_DRESS

/* 更新玩家更衣室数据至数据库 */
int db_update_char_dress_data(psocn_dress_data_t* dress_data, uint32_t gc, uint8_t slot, uint32_t flag) {

    //DBG_LOG("更新角色更衣室数据");

    memset(myquery, 0, sizeof(myquery));

    if (flag & PSOCN_DB_SAVE_CHAR) {
        sprintf(myquery, "INSERT INTO %s "
            "(guildcard, slot, "
            "guildcard_string, "//4
            "dress_unk1, dress_unk2, name_color_b, name_color_g, name_color_r, "//5
            "name_color_transparency, model, dress_unk3, create_code, name_color_checksum, section, "//5
            "ch_class, v2flags, version, v1flags, costume, "//5
            "skin, face, head, hair, "//4
            "hair_r, hair_g, hair_b, prop_x, prop_y) "//5
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', "
            "'%s', "//4
            "'%d', '%d', '%d', '%d', '%d', "//5
            "'%d', '%d', '%s', '%d', '%d', '%d', "//5
            "'%d', '%d', '%d', '%d', '%d', "//5
            "'%d', '%d', '%d', '%d', "//4
            "'%d', '%d', '%d', '%f', '%f')",//5
            TABLE1,
            gc, slot,
            dress_data->guildcard_string,
            dress_data->dress_unk1, dress_data->dress_unk2, dress_data->name_color_b, dress_data->name_color_g, dress_data->name_color_r,
            dress_data->name_color_transparency, dress_data->model, (char*)dress_data->dress_unk3, dress_data->create_code, dress_data->name_color_checksum, dress_data->section,
            dress_data->ch_class, dress_data->v2flags, dress_data->version, dress_data->v1flags, dress_data->costume,
            dress_data->skin, dress_data->face, dress_data->head, dress_data->hair,
            dress_data->hair_r, dress_data->hair_g, dress_data->hair_b, dress_data->prop_x, dress_data->prop_y
        );

        //DBG_LOG("保存角色更衣室数据 %d", dress_data->create_code);

        if (psocn_db_real_query(&conn, myquery)) {
            SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", TABLE1, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            return -6;
        }
    }
    //else if (flag & PSOCN_DB_UPDATA_CHAR) {
    //    sprintf(myquery, "UPDATE %s SET "
    //        "costume = '%d', "
    //        "hair = '%d', hair_r = '%d', hair_g = '%d', hair_b = '%d' "
    //        "WHERE guildcard = '%" PRIu32 "' AND slot =  '%" PRIu8 "'",
    //        CHARACTER_DRESS,
    //        dress_data->costume, 
    //        dress_data->hair, dress_data->hair_r, dress_data->hair_g, dress_data->hair_b,
    //        gc, slot
    //    );

    //    //DBG_LOG("更新角色更衣室数据");

    //    if (psocn_db_real_query(&conn, myquery)) {
    //        SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
    //            "槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
    //            psocn_db_error(&conn));
    //        /* XXXX: 未完成给客户端发送一个错误信息 */
    //        return -6;
    //    }

    //} 
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        sprintf(myquery, "UPDATE %s SET "
            "guildcard =  '%" PRIu32 "', slot = '%" PRIu8 "', "
            "guildcard_string = '%s', dress_unk1 = '%d', dress_unk2 = '%d', "
            "name_color_b = '%d', name_color_g = '%d', name_color_r = '%d', name_color_transparency = '%d', "
            "model = '%d', dress_unk3 = '%s', create_code = '%d', name_color_checksum = '%d', section = '%d', "
            "ch_class = '%d', v2flags = '%d', version = '%d', v1flags = '%d', "
            "costume = '%d', skin = '%d', face = '%d', head = '%d', "
            "hair = '%d', hair_r = '%d', hair_g = '%d', hair_b = '%d', "
            "prop_x = '%f', prop_y = '%f' "
            "WHERE guildcard = '%" PRIu32 "' AND slot =  '%" PRIu8 "'",
            TABLE1, gc, slot,
            dress_data->guildcard_string, dress_data->dress_unk1, dress_data->dress_unk2,
            dress_data->name_color_b, dress_data->name_color_g, dress_data->name_color_r, dress_data->name_color_transparency,
            dress_data->model, (char*)dress_data->dress_unk3, dress_data->create_code, dress_data->name_color_checksum, dress_data->section,
            dress_data->ch_class, dress_data->v2flags, dress_data->version, dress_data->v1flags,
            dress_data->costume, dress_data->skin, dress_data->face, dress_data->head,
            dress_data->hair, dress_data->hair_r, dress_data->hair_g, dress_data->hair_b,
            dress_data->prop_x, dress_data->prop_y
            , gc, slot
        );

        //DBG_LOG("更新角色更衣室数据");

        if (psocn_db_real_query(&conn, myquery)) {
            SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", TABLE1, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            return -6;
        }
        else {
            //DBG_LOG("更新角色更衣室数据");

            sprintf(myquery, "DELETE FROM %s WHERE guildcard="
                "'%" PRIu32 "' AND slot='%" PRIu8 "'", TABLE1, gc,
                slot);

            if (psocn_db_real_query(&conn, myquery)) {
                SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
                    PRIu32 ", 槽位 %" PRIu8 "):\n%s", TABLE1, gc, slot,
                    psocn_db_error(&conn));
                /* XXXX: 未完成给客户端发送一个错误信息 */
                return -1;
            }

            //DBG_LOG("更新角色更衣室数据");

            sprintf(myquery, "INSERT INTO %s "
                "(guildcard, slot, "
                "guildcard_string, "//4
                "dress_unk1, dress_unk2, name_color_b, name_color_g, name_color_r, "//5
                "name_color_transparency, model, dress_unk3, create_code, name_color_checksum, section, "//5
                "ch_class, v2flags, version, v1flags, costume, "//5
                "skin, face, head, hair, "//4
                "hair_r, hair_g, hair_b, prop_x, prop_y) "//5
                "VALUES ('%" PRIu32 "', '%" PRIu8 "', "
                "'%s', "//4
                "'%d', '%d', '%d', '%d', '%d', "//5
                "'%d', '%d', '%s', '%d', '%d', '%d', "//5
                "'%d', '%d', '%d', '%d', '%d', "//5
                "'%d', '%d', '%d', '%d', "//4
                "'%d', '%d', '%d', '%f', '%f')",//5
                TABLE1,
                gc, slot,
                dress_data->guildcard_string,
                dress_data->dress_unk1, dress_data->dress_unk2, dress_data->name_color_b, dress_data->name_color_g, dress_data->name_color_r,
                dress_data->name_color_transparency, dress_data->model, (char*)dress_data->dress_unk3, dress_data->create_code, dress_data->name_color_checksum, dress_data->section,
                dress_data->ch_class, dress_data->v2flags, dress_data->version, dress_data->v1flags, dress_data->costume,
                dress_data->skin, dress_data->face, dress_data->head, dress_data->hair,
                dress_data->hair_r, dress_data->hair_g, dress_data->hair_b, dress_data->prop_x, dress_data->prop_y
            );

            if (psocn_db_real_query(&conn, myquery)) {
                SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 "):\n%s", TABLE1, gc, slot,
                    psocn_db_error(&conn));
                /* XXXX: 未完成给客户端发送一个错误信息 */
                return -6;
            }
        }
    }

    return 0;

}

int db_get_dress_data(uint32_t gc, uint8_t slot, psocn_dress_data_t* dress_data, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", TABLE1, gc, slot);

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

    if (row != NULL) {
        int i = 2;
        memcpy(&dress_data->guildcard_string[0], row[i], sizeof(dress_data->guildcard_string));
        i++;
        dress_data->dress_unk1 = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->dress_unk2 = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->name_color_b = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->name_color_g = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->name_color_r = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->name_color_transparency = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->model = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        memcpy(&dress_data->dress_unk3[0], row[i], sizeof(dress_data->dress_unk3));
        i++;
        dress_data->create_code = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->name_color_checksum = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->section = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->ch_class = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->v2flags = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->version = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->v1flags = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->costume = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->skin = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->face = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->head = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->hair = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->hair_r = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->hair_g = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->hair_b = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data->prop_x = strtof(row[i], NULL);
        i++;
        dress_data->prop_y = strtof(row[i], NULL);
    }

    psocn_db_result_free(result);

    return 0;
}
