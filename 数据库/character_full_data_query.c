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

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* Blue Burst packet for sending the full character data and options */
typedef struct bb_full_char {
    uint16_t pkt_len;
    uint16_t pkt_type;
    uint32_t flags;
    psocn_bb_full_char_t data;
} PACKED bb_full_char_pkt;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

int db_get_orignal_char_full_data(uint32_t gc, uint8_t slot, psocn_bb_db_char_t* char_data, int check) {
    void* result;
    char** row;
    int j = 0;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT full_data FROM "
        "%s"
        " WHERE "
        "guildcard = '%" PRIu32 "' "
        "AND slot = '%u' "
        "ORDER BY update_time DESC "
        "LIMIT 1"
        , CHARACTER_DATA_FULL
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
        if (check) {
            SQLERR_LOG("未找到保存的角色数据 (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return -3;
    }

    if (isEmptyString(row[0])) {
        psocn_db_result_free(result);

        SQLERR_LOG("保存的角色数据为空 (%" PRIu32 ": %u)", gc, slot);
        return -4;
    }

    //print_ascii_hex(dbgl, row[0], sizeof(bb_full_char_pkt));

    bb_full_char_pkt* org_char = (bb_full_char_pkt*)row[0];

    memcpy(&char_data->character, &org_char->data.character, sizeof(psocn_bb_char_t));
    memcpy(char_data->quest_data1, org_char->data.quest_data1, PSOCN_STLENGTH_BB_DB_QUEST_DATA1);
    memcpy(&char_data->bank, &org_char->data.bank, sizeof(psocn_bank_t));
    memcpy(char_data->guildcard_desc, org_char->data.gc.guildcard_desc, sizeof(char_data->guildcard_desc));
    memcpy(char_data->autoreply, org_char->data.autoreply, sizeof(char_data->autoreply));
    memcpy(char_data->infoboard, org_char->data.infoboard, sizeof(char_data->infoboard));
    memcpy(&char_data->c_records, &org_char->data.c_records, sizeof(bb_challenge_records_t));
    memcpy(char_data->tech_menu, org_char->data.tech_menu, sizeof(char_data->tech_menu));
    memcpy(char_data->quest_data2, org_char->data.quest_data2, sizeof(char_data->quest_data2));
    memcpy(&char_data->b_records, &org_char->data.b_records, sizeof(battle_records_t));

    psocn_db_result_free(result);

    return 0;
}

int db_insert_bb_full_char_data(void* data, uint32_t gc, uint32_t slot, uint8_t char_class, char* class_name) {

    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
        "guildcard, slot, ch_class, class_name, update_time, "
        "`full_data`"
        ") VALUES ("
        "'%d', '%d', '%d', '%s', NOW(), '",
        CHARACTER_DATA_FULL,
        gc, slot, char_class, class_name
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)data,
        PSOCN_STLENGTH_BB_FULL_CHAR);

    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        //SQLERR_LOG("无法插入数据");
        //SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}


int db_update_bb_full_char_data(void* data, uint32_t gc, uint32_t slot, uint8_t char_class, char* class_name) {
    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "update_time = NOW(), ch_class = '%d', class_name = '%s', `full_data` = '", CHARACTER_DATA_FULL, char_class, class_name);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)data,
        PSOCN_STLENGTH_BB_FULL_CHAR);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "' WHERE `guildcard` = '%u' AND `slot` = '%u'", gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}