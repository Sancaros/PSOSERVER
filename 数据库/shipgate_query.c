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

uint32_t db_get_char_data_length(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    unsigned long* len;
    uint32_t data_length;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data, size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the length of the character data */
    if (!(len = psocn_db_result_lengths(result))) {
        psocn_db_result_free(result);
        SQLERR_LOG("无法获取角色数据的长度");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    data_length = len[0];

    psocn_db_result_free(result);
    return data_length;
}

uint32_t db_get_char_data_size(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    uint32_t data_size;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    data_size = (uint32_t)atoi(row[0]);

    psocn_db_result_free(result);
    return data_size;
}

uint32_t db_get_char_data_play_time(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    uint32_t play_time;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT play_time FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER, gc, slot);

    /*SQLERR_LOG("gc %d = %d  slot %d = %d"
        , gc, pkt->guildcard, slot, pkt->slot);*/

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        SQLERR_LOG("未找到保存的角色数据 (%u: %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    play_time = (uint32_t)atoi(row[0]);

    psocn_db_result_free(result);
    return play_time;
}

char* db_get_char_raw_data(uint32_t gc, uint8_t slot, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法查询角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("未获取到角色数据 (%" PRIu32 ": %u)", gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    if ((row = psocn_db_result_fetch(result)) == NULL) {
        psocn_db_result_free(result);
        if (check) {
            SQLERR_LOG("未找到保存的角色数据 (%" PRIu32 ": %u)", gc, slot);
            SQLERR_LOG("%s", psocn_db_error(&conn));
        }
        return 0;
    }

    return row[0];
}

int db_get_char_disp(uint32_t gc, uint8_t slot, psocn_disp_char_t* data, int check) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%" PRIu32 "' "
        "AND slot = '%u'", CHARACTER_DISP, gc, slot);

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

    /* 获取玩家角色7项基本数值 */
    data->stats.atp = (uint16_t)strtoul(row[2], NULL, 0);
    data->stats.mst = (uint16_t)strtoul(row[3], NULL, 0);
    data->stats.evp = (uint16_t)strtoul(row[4], NULL, 0);
    data->stats.hp = (uint16_t)strtoul(row[5], NULL, 0);
    data->stats.dfp = (uint16_t)strtoul(row[6], NULL, 0);
    data->stats.ata = (uint16_t)strtoul(row[7], NULL, 0);
    data->stats.lck = (uint16_t)strtoul(row[8], NULL, 0);

    data->opt_flag1 = (uint8_t)strtoul(row[9], NULL, 0);
    data->opt_flag2 = (uint8_t)strtoul(row[10], NULL, 0);
    data->opt_flag3 = (uint8_t)strtoul(row[11], NULL, 0);
    data->opt_flag4 = (uint8_t)strtoul(row[12], NULL, 0);
    data->opt_flag5 = (uint8_t)strtoul(row[13], NULL, 0);
    data->opt_flag6 = (uint8_t)strtoul(row[14], NULL, 0);
    data->opt_flag7 = (uint8_t)strtoul(row[15], NULL, 0);
    data->opt_flag8 = (uint8_t)strtoul(row[16], NULL, 0);
    data->opt_flag9 = (uint8_t)strtoul(row[17], NULL, 0);
    data->opt_flag10 = (uint8_t)strtoul(row[18], NULL, 0);

    data->level = (uint32_t)strtoul(row[19], NULL, 0) - 1;
    data->exp = (uint32_t)strtoul(row[20], NULL, 0);
    data->meseta = (uint32_t)strtoul(row[21], NULL, 0);

    psocn_db_result_free(result);

    return 0;
}

/* 更新玩家基础数据至数据库 */
int db_update_char_challenge(psocn_bb_db_char_t* char_data, uint32_t gc, uint8_t slot, uint32_t flag) {
    static char query[PSOCN_STLENGTH_BB_DB_CHAR * 2 + 256];
    char name[64];
    char class_name[64];

    istrncpy16_raw(ic_utf16_to_utf8, name, &char_data->character.name.char_name[0], 64, BB_CHARACTER_CHAR_NAME_LENGTH);

    istrncpy(ic_gbk_to_utf8, class_name, pso_class[char_data->character.dress_data.ch_class].cn_name, 64);

    if (flag & PSOCN_DB_SAVE_CHAR) {
        sprintf(query, "INSERT INTO %s(guildcard, slot, name, class_name, version, data) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%s', '%" PRIu8 "', '", CHARACTER_RECORDS_CHALLENGE, gc, slot
        , name, class_name, char_data->character.dress_data.version);

        psocn_db_escape_str(&conn, query + strlen(query), (char*)&char_data->challenge_data,
            sizeof(char_data->challenge_data));

        strcat(query, "')");

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法保存挑战数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", CHARACTER_RECORDS_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            return 0;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        sprintf(query, "DELETE FROM %s WHERE guildcard="
            "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_RECORDS_CHALLENGE, gc,
            slot);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
                PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_RECORDS_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            return -1;
        }

        sprintf(query, "INSERT INTO %s(guildcard, slot, name, class_name, version, data) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%s', '%" PRIu8 "', '", CHARACTER_RECORDS_CHALLENGE, gc, slot
            , name, class_name, char_data->character.dress_data.version);

        psocn_db_escape_str(&conn, query + strlen(query), (char*)&char_data->challenge_data,
            sizeof(char_data->challenge_data));

        strcat(query, "')");

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法保存挑战数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", CHARACTER_RECORDS_CHALLENGE, gc, slot,
                psocn_db_error(&conn));
            return 0;
        }
    }

    return 0;
}

int db_initialize() {
    char query[256] = { 0 };

    SGATE_LOG("初始化在线舰船数据表"/*, SERVER_SHIPS_ONLINE*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_SHIPS_ONLINE);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_SHIPS_ONLINE);
        return -1;
    }

    SGATE_LOG("初始化在线玩家数据表"/*, SERVER_CLIENTS_ONLINE*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_CLIENTS_ONLINE);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_CLIENTS_ONLINE);
        return -1;
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", AUTH_ACCOUNT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", AUTH_ACCOUNT);
        return -1;
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' WHERE islogged = '1'", CHARACTER);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", CHARACTER);
        return -1;
    }

    SGATE_LOG("初始化临时玩家数据表"/*, SERVER_CLIENTS_TRANSIENT*/);

    sprintf_s(query, _countof(query), "DELETE FROM %s", SERVER_CLIENTS_TRANSIENT);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", SERVER_CLIENTS_TRANSIENT);
        return -1;
    }

    return 0;
}

int db_update_ship_ipv4(uint32_t ip, uint16_t key_idx) {

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET ip = '%lu' WHERE ship_id = '%u'",
        SERVER_SHIPS_ONLINE, ntohl(ip), key_idx);
    if (!psocn_db_real_query(&conn, myquery))
        return -1;

    return 0;
}