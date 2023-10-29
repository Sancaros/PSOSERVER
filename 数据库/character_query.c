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

/// <summary>
/// //////////////////////////////////////////////////////
/// 玩家数据其他操作

int db_update_char_auth_msg(char ipstr[INET6_ADDRSTRLEN], uint32_t gc, uint8_t slot) {
    //char query[256];

    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    char timestamp[28];
    sprintf(timestamp, "%u-%02u-%02u %02u:%02u:%02u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond);

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET ip = '%s', last_login_time = '%s'"
        "WHERE guildcard = '%" PRIu32 "' AND slot = '%"PRIu8"'", CHARACTER, ipstr, timestamp, gc, slot);
    if (psocn_db_real_query(&conn, myquery))
    {
        SQLERR_LOG("无法更新角色登录数据 (GC %" PRIu32 ", 槽位 %"
            PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_update_gc_char_login_state(uint32_t gc, uint32_t islogined) {
    char query[512];

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '%" PRIu32 "'"
        " WHERE guildcard = '%" PRIu32 "'",
        AUTH_ACCOUNT, islogined, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", AUTH_ACCOUNT);
        return -1;
    }

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '%" PRIu32 "'"
        " WHERE guildcard = '%" PRIu32 "'",
        CHARACTER, islogined, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", CHARACTER);
        return -2;
    }

    return 0;
}

int db_updata_char_play_time(uint32_t play_time, uint32_t gc, uint8_t slot) {

    memset(myquery, 0, sizeof(myquery));

    sprintf_s(myquery, _countof(myquery), "UPDATE %s SET play_time = '%d' WHERE guildcard = '%"
        PRIu32 "' AND slot = '%"PRIu8"' ", CHARACTER, play_time, gc, slot);
    if (psocn_db_real_query(&conn, myquery))
    {
        SQLERR_LOG("无法更新角色 %s 数据!", CHARACTER);
        return -1;
    }

    return 0;
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

char* db_get_char_create_time(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "SELECT create_time FROM %s WHERE guildcard = '%"
        PRIu32 "' AND slot = '%"PRIu8"'", CHARACTER, gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("选择玩家登录IP数据错误 (%u: %u)",
            gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }


    result = psocn_db_result_store(&conn);
    if (!result) {
        SQLERR_LOG("无法获取玩家登录IP数据错误 (%u: %u)",
            gc, slot);
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return 0;
    }

    row = psocn_db_result_fetch(result);

    return row[0];
}

/// <summary>
/// //////////////////////////////////////////////////////
/// 玩家数据操作

uint32_t db_get_char_data_length(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;
    unsigned long* len;
    uint32_t data_length;

    memset(myquery, 0, sizeof(myquery));

    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT data, size FROM %s WHERE guildcard='%u' "
        "AND slot='%u'", CHARACTER, gc, slot);

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

psocn_bb_db_char_t* db_uncompress_char_data(unsigned long* len, char** row, uint32_t data_size) {
    psocn_bb_db_char_t* char_data;
    int sz;
    uLong sz2, csz;

    char_data = (psocn_bb_db_char_t*)malloc(PSOCN_STLENGTH_BB_DB_CHAR);

    if (!char_data) {
        ERR_LOG("无法分配角色数据内存空间");
        ERR_LOG("%s", strerror(errno));
    }

    /* Grab the data from the result */
    sz = (int)len[0];

    if (data_size) {
        sz2 = (uLong)atoi(row[1]);
        csz = (uLong)sz;

        /* 解压缩数据 */
        if (uncompress((Bytef*)char_data, &sz2, (Bytef*)row[0], csz) != Z_OK) {
            ERR_LOG("无法解压角色数据");
            free(char_data);
        }

        sz = sz2;
    }
    else {

        if (len[0] != PSOCN_STLENGTH_BB_DB_CHAR) {
            ERR_LOG("无效(未知)角色数据,长度不一致!");
        }
        else
            memcpy(char_data, row[0], len[0]);
    }

    return char_data;
}

/* 获取并解压未结构化的角色数据 */
int db_uncompress_char_data_raw(uint8_t* data, char* raw_data, uint32_t data_legth, uint32_t data_size) {
    //uint8_t* data;
    int sz;
    uLong sz2, csz;

    /* Grab the data from the result */
    sz = (int)data_legth;

    if (data_size) {
        sz2 = data_size;
        csz = (uLong)sz;

        data = (uint8_t*)malloc(sz2);
        if (!data) {
            ERR_LOG("无法分配解压角色数据的内存空间");
            ERR_LOG("%s", strerror(errno));
        }

        /* 解压缩数据 */
        if (uncompress((Bytef*)data, &sz2, (Bytef*)raw_data, csz) != Z_OK) {
            ERR_LOG("无法解压角色数据");
        }

        sz = sz2;
    }
    else {
        data = (uint8_t*)malloc(sz);
        if (!data) {
            ERR_LOG("无法分配角色数据的内存空间");
            ERR_LOG("%s", strerror(errno));
        }
        memcpy(data, raw_data, sz);
    }

    return sz;
}

psocn_bb_db_char_t* db_get_uncompress_char_data(uint32_t gc, uint8_t slot) {
    psocn_bb_db_char_t* char_data;
    uint32_t* len;
    char** row;
    uint32_t data_size;
    void* result;

    char_data = (psocn_bb_db_char_t*)malloc(PSOCN_STLENGTH_BB_DB_CHAR);

    if (!char_data) {
        ERR_LOG("无法分配角色数据内存空间");
        ERR_LOG("%s", strerror(errno));
        return NULL;
    }

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "SELECT data, size FROM %s WHERE guildcard='%"
        PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER, gc, slot);

    /* 获取旧玩家数据... */
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        goto err;
    }

    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        goto err;
    }

    if (!(row = psocn_db_result_fetch(result))) {
        //SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
        //    "槽位 %" PRIu8 "):\n%s", gc, slot,
        //    psocn_db_error(&conn));
        psocn_db_result_free(result);

        goto err;
    }

    /* Grab the length of the character data */
    if (!(len = psocn_db_result_lengths(result))) {
        SQLERR_LOG("无法获取角色数据的长度");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        psocn_db_result_free(result);

        goto err;
    }

    data_size = (uint32_t)strtoul(row[1], NULL, 10);

    char_data = db_uncompress_char_data(len, row, data_size);

    return char_data;

err:
    free_safe(char_data);
    return NULL;
}

static int db_update_character(psocn_bb_db_char_t* data, char* table, uint32_t gc, uint8_t slot) {
    memset(myquery, 0, sizeof(myquery));

    snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
        "`character2` = '", table);

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->character,
        PSOCN_STLENGTH_BB_CHAR2);

    SAFE_STRCAT(myquery, "', bank = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->bank,
        PSOCN_STLENGTH_BANK);

    SAFE_STRCAT(myquery, "', quest_data1 = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->quest_data1,
        PSOCN_STLENGTH_BB_DB_QUEST_DATA1);

    SAFE_STRCAT(myquery, "', guildcard_desc = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->guildcard_desc,
        88);

    SAFE_STRCAT(myquery, "', autoreply = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->autoreply,
        172);

    SAFE_STRCAT(myquery, "', infoboard = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->infoboard,
        172);

    SAFE_STRCAT(myquery, "', b_records = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->b_records,
        PSOCN_STLENGTH_BATTLE_RECORDS);

    SAFE_STRCAT(myquery, "', c_records = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->c_records,
        PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

    SAFE_STRCAT(myquery, "', tech_menu = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->tech_menu,
        PSOCN_STLENGTH_BB_DB_TECH_MENU);

    SAFE_STRCAT(myquery, "', mode_quest_data = '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&data->mode_quest_data.all,
        PSOCN_DATALENGTH_BB_DB_MODE_QUEST_DATA);

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "' WHERE guildcard = '%" PRIu32 "' AND "
        "slot = '%" PRIu8 "'", gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    return 0;
}

int db_compress_char_data(psocn_bb_db_char_t* char_data, uint16_t data_len, uint32_t gc, uint8_t slot) {
    Bytef* cmp_buf;
    uLong cmp_sz;
    int compressed = ~Z_OK;
    int compress_power = 0;
    uint32_t* len;
    char** row;
    //uint32_t data_size;
    void* result;
    uint32_t play_time;
    char lastip[INET6_ADDRSTRLEN];

    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    char timestamp[28];
    sprintf(timestamp, "%u-%02u-%02u %02u:%02u:%02u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond);

    play_time = char_data->character.play_time;

    memcpy(&lastip, db_get_auth_ip(gc, slot), sizeof(lastip));

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "SELECT data FROM %s WHERE guildcard='%"
        PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER, gc, slot);

    /* 获取旧玩家数据... */
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        return -2;
    }

    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        return -3;
    }

    if (!(row = psocn_db_result_fetch(result))) {
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        return -4;
    }

    /* Grab the length of the character data */
    if (!(len = psocn_db_result_lengths(result))) {
        SQLERR_LOG("无法获取角色数据的长度");
        SQLERR_LOG("%s", psocn_db_error(&conn));
        psocn_db_result_free(result);
        return -5;
    }

    /* Is it a Blue Burst character or not? */
    if (data_len > 1056) {
        data_len = PSOCN_STLENGTH_BB_DB_CHAR;
    }
    else {
        data_len = 1052;
    }

    /* 压缩角色数据 */
    cmp_sz = compressBound((uLong)data_len);

    if ((cmp_buf = (Bytef*)malloc(cmp_sz))) {
        compressed = compress2(cmp_buf, &cmp_sz, (Bytef*)char_data,
            (uLong)data_len, compress_power);
    }

    if (compressed == Z_OK && cmp_sz < data_len) {
        sprintf(myquery, "UPDATE %s SET "
            "size = '%u', play_time = '%d', lastip = '%s', data = '",
            CHARACTER, (unsigned)data_len, play_time, lastip);

        psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)cmp_buf,
            cmp_sz);
    }
    else {
        sprintf(myquery, "UPDATE %s SET "
            "size = '0', play_time = '%d', lastip = '%s', data = '",
            CHARACTER, play_time, lastip);

        psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)char_data,
            data_len);
    }

    snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "' WHERE guildcard = '%" PRIu32 "' AND "
        "slot = '%" PRIu8 "'", gc, slot);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):%s", CHARACTER, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -6;
    }

    /* 更新基础玩家数据 */
    if (db_update_character(char_data, CHARACTER, gc, slot)) {
        SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):%s", CHARACTER, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -7;
    }

    free(cmp_buf);

    return 0;
}

/* 插入玩家角色数据 */
int db_insert_char_data(psocn_bb_db_char_t* char_data, uint32_t gc, uint8_t slot) {
    char ipstr[INET6_ADDRSTRLEN];

    memcpy(&ipstr, db_get_auth_ip(gc, slot), sizeof(ipstr));

    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    char timestamp[28];
    sprintf(timestamp, "%u-%02u-%02u %02u:%02u:%02u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond);

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO %s ("
        "guildcard, slot, size, ip, create_time, "
        "`character2`, bank, quest_data1, guildcard_desc, "
        "autoreply, infoboard, b_records, c_records, tech_menu, mode_quest_data, "
        "data"
        ") VALUES ("
        "'%" PRIu32 "', '%" PRIu8 "', '0', '%s', '%s', '"
        , CHARACTER
        , gc, slot, ipstr, timestamp
    );

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->character,
        PSOCN_STLENGTH_BB_CHAR2);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->bank,
        PSOCN_STLENGTH_BANK);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->quest_data1,
        PSOCN_STLENGTH_BB_DB_QUEST_DATA1);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->guildcard_desc,
        88);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->autoreply,
        172);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->infoboard,
        172);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->b_records,
        PSOCN_STLENGTH_BATTLE_RECORDS);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->c_records,
        PSOCN_STLENGTH_BB_CHALLENGE_RECORDS);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->tech_menu,
        PSOCN_STLENGTH_BB_DB_TECH_MENU);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)&char_data->mode_quest_data.all,
        PSOCN_DATALENGTH_BB_DB_MODE_QUEST_DATA);

    SAFE_STRCAT(myquery, "', '");

    psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)char_data,
        PSOCN_STLENGTH_BB_DB_CHAR);

    SAFE_STRCAT(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法创建玩家数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -2;
    }

    return 0;
}

/* 删除角色数据 */
int db_delete_bb_char_data(uint32_t gc, uint8_t slot) {
    static char query[PSOCN_STLENGTH_BB_DB_CHAR * 2 + 256];

    sprintf(query, "DELETE FROM %s WHERE "
        "guildcard = '%" PRIu32 "' AND slot='%" PRIu8 "'", 
        CHARACTER, 
        gc, slot);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

/* 角色备份功能 */
int db_backup_bb_char_data(uint32_t gc, uint8_t slot) {
    static char query[PSOCN_STLENGTH_BB_DB_CHAR * 2 + 256];
    psocn_bb_db_char_t* char_data;
    uint32_t data_size;
    unsigned long* len;
    void* result;
    char** row;

    sprintf(query, "SELECT data, size, ip FROM %s WHERE guildcard='%"
        PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER, gc, slot);

    /* Backup the old data... */
    if (!psocn_db_real_query(&conn, query)) {
        if (!(result = psocn_db_result_store(&conn))) {
            SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", gc, slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            return -4;
        }

        if (!(row = psocn_db_result_fetch(result))) {
            //SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            //    "槽位 %" PRIu8 "):\n%s", gc, slot,
            //    psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            psocn_db_result_free(result);
            return 0;
        }

        if (row != NULL) {
            /* Grab the length of the character data */
            if (!(len = psocn_db_result_lengths(result))) {
                //SQLERR_LOG("无法获取角色数据的长度");
                //SQLERR_LOG("%s", psocn_db_error(&conn));
                psocn_db_result_free(result);
                return 0;
            }

            data_size = (uint32_t)strtoul(row[1], NULL, 10);

            char_data = db_uncompress_char_data(len, row, data_size);

            sprintf(query, "INSERT INTO %s (guildcard, slot, size, deleteip, data) "
                "VALUES ('%" PRIu32 "', '%" PRIu8 "', '0', '%s', '", CHARACTER_DELETE
                , gc, slot, row[2]);
            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)char_data,
                PSOCN_STLENGTH_BB_DB_CHAR);
            SAFE_STRCAT(query, "')");

            if (psocn_db_real_query(&conn, query)) {
                ERR_LOG("无法备份已删除的玩家数据 (gc=%"
                    PRIu32 ", slot=%" PRIu8 "):\n%s", gc, slot,
                    psocn_db_error(&conn));
                /* XXXX: 未完成给客户端发送一个错误信息 */
                return -1;
            }
        }
        psocn_db_result_free(result);
    }

    return 0;
}