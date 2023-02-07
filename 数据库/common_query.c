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

/* 初始化数据库连接 */
extern psocn_dbconn_t conn;

/* 检测玩家是否在线 */
int db_check_gc_online(uint32_t gc) {
    char query[256];
    void* result;
    char** row;

    /* Fill in the query. */
    sprintf(query, "SELECT guildcard FROM %s WHERE guildcard='%u'", 
        SERVER_CLIENTS_ONLINE, (unsigned int)gc);

    /* If we can't query the database, fail. */
    if (psocn_db_real_query(&conn, query)) {
        return 0;
    }

    /* Grab the results. */
    result = psocn_db_result_store(&conn);
    if (!result) {
        return 0;
    }

    row = psocn_db_result_fetch(result);
    if (!row) {
        psocn_db_result_free(result);
        return 0;
    }

    /* If there is a result, then the user is already online. */
    psocn_db_result_free(result);

    return 1;
}

/* TODO */
uint32_t db_remove_account_login_state_from_ship(uint16_t key_idx) {
    char query[256];
    void* result;
    char** row;
    uint32_t gc = -1;
    int i = 0;

    printf("%d", i);

    /* Fill in the query. */
    sprintf(query, "SELECT guildcard FROM %s WHERE ship_id='%hu'",
        SERVER_SHIPS_ONLINE, key_idx);

    /* If we can't query the database, fail. */
    if (psocn_db_real_query(&conn, query)) {
        return -1;
    }

    /* Grab the results. */
    result = psocn_db_result_store(&conn);
    if (!result) {
        return -1;
    }

    row = psocn_db_result_fetch(result);
    if (!row) {
        psocn_db_result_free(result);
        return 0;
    }

    while (!row) {
        i++;
    }

    printf("%d", i);

    gc = (uint32_t)strtoul(row[0], NULL, 0);

    return gc;
}

int db_remove_gc_char_login_state(uint32_t gc) {
    char query[512];

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '0' "
        "WHERE guildcard = '%u' AND islogged = '1'",
        CHARACTER, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("初始化 %s 数据表错误,请检查数据库", CHARACTER);
        return 1;
    }

    return 0;
}

int db_update_gc_char_login_state(uint32_t gc, uint8_t char_slot,
    uint32_t islogged, uint32_t block_num) {
    char query[512];

    if (!(char_slot < 0)) {
        sprintf_s(query, _countof(query), "UPDATE %s SET "
            "islogged = '%d' "
            "WHERE guildcard = '%u' AND slot = '%u'", 
            CHARACTER,
            islogged, 
            gc, char_slot);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("更新GC %u 数据错误:\n %s", gc, psocn_db_error(&conn));
            return 1;
        }
    }

    if (block_num > 0) {
        sprintf_s(query, _countof(query), "UPDATE %s SET "
            "lastblock = '%d' "
            "WHERE guildcard = '%u' AND slot = '%u'",
            CHARACTER,
            block_num,
            gc, char_slot);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("更新GC %u 数据错误:\n %s", gc, psocn_db_error(&conn));
            return 1;
        }
    }

    return 0;
}

/**/
int db_update_gc_login_state(uint32_t gc, 
    uint32_t islogged, uint32_t char_slot, char* char_name) {
    char query[512];
    char lastchar_name[64];
    uint8_t lastchar_blob[4098] = { 0 };

    sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '%d'"
        " where guildcard = '%u'",
        AUTH_ACCOUNT, islogged, gc);
    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("更新GC %u 在线数据信息错误:\n %s", gc, psocn_db_error(&conn));
        return 1;
    }

    if (!(char_slot < 0)) {
        sprintf_s(query, _countof(query), "UPDATE %s SET lastchar_slot = '%d'"
            " where guildcard = '%u'",
            AUTH_ACCOUNT, char_slot, gc);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("更新GC %u 在线数据信息错误:\n %s", gc, psocn_db_error(&conn));
            return 2;
        }
    }

    if (char_name != NULL) {
        istrncpy16_raw(ic_utf16_to_utf8, lastchar_name, char_name, 64, BB_CHARACTER_NAME_LENGTH);

        psocn_db_escape_str(&conn, (char*)&lastchar_blob[0], char_name, BB_CHARACTER_NAME_LENGTH * 2);

        sprintf_s(query, _countof(query), "UPDATE %s SET lastchar_blob = '%s', lastchar_name = '%s'"
            " WHERE guildcard = '%" PRIu32 "'",
            AUTH_ACCOUNT, (char*)&lastchar_blob[0], lastchar_name,
            gc);

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("更新GC %u 在线数据信息错误:\n %s", gc, psocn_db_error(&conn));
            return 3;
        }
    }

    return 0;
}

/* 获取玩家所在舰船ID */
int db_get_char_ship_id(uint32_t gc) {
    void* result;
    char** row;
    int ship_id;

    memset(myquery, 0, sizeof(myquery));

    /* Figure out where the user requested is */
    sprintf(myquery, "SELECT ship_id FROM %s WHERE guildcard='%u'", SERVER_CLIENTS_ONLINE,
        gc);
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("获取角色舰船ID发生错误: %s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("无法获取角色舰船ID数据库结果: %s",
            psocn_db_error(&conn));
        return -2;
    }

    if (!(row = psocn_db_result_fetch(result))) {
        /* The user's not online, see if we should save it. */
        psocn_db_result_free(result);
        return -3;
    }

    /* Grab the data from the result */
    errno = 0;
    ship_id = atoi(row[0]);/* (uint16_t)strtoul(row[0], NULL, 0);*/
    psocn_db_result_free(result);

    if (errno) {
        ERR_LOG("无法分析获取角色舰船ID: %s", strerror(errno));
        return -4;
    }

    return ship_id;
}

int db_updata_char_security(uint32_t play_time, uint32_t gc, uint8_t slot) {
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

char* db_get_auth_ip(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Delete any character data already exising in that slot. */
    sprintf(myquery, "SELECT lastip FROM %s WHERE guildcard='%"PRIu32"'", AUTH_ACCOUNT, gc);

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

char* db_get_char_create_time(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

    /* Delete any character data already exising in that slot. */
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

psocn_bb_db_char_t *db_uncompress_char_data(unsigned long* len, char** row, uint32_t data_size) {
    psocn_bb_db_char_t *char_data;
    int sz;
    uLong sz2, csz;

    char_data = (psocn_bb_db_char_t*)malloc(sizeof(psocn_bb_db_char_t));

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

        if (len[0] != sizeof(psocn_bb_db_char_t)) {
            ERR_LOG("无效(未知)角色数据,长度不一致!");
            //psocn_db_result_free(result);
            free(char_data);
        }else
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

psocn_bb_db_char_t *db_get_uncompress_char_data(uint32_t gc, uint8_t slot) {
    psocn_bb_db_char_t *char_data;
    uint32_t* len;
    char** row;
    uint32_t data_size;
    void* result;

    char_data = (psocn_bb_db_char_t*)malloc(sizeof(psocn_bb_db_char_t));

    if (!char_data) {
        ERR_LOG("无法分配角色数据内存空间");
        ERR_LOG("%s", strerror(errno));
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
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
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

    data_size = (uint32_t)strtoul(row[1], NULL, 0);

    char_data = db_uncompress_char_data(len, row, data_size);

    return char_data;

err:
    free(char_data);
    return NULL;
}

int db_compress_char_data(psocn_bb_db_char_t *char_data, uint16_t data_len, uint32_t gc, uint8_t slot) {
    Bytef* cmp_buf;
    uLong cmp_sz;
    int compressed = ~Z_OK;
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

    //data_size = (uint32_t)strtoul(row[1], NULL, 0);

    /* Is it a Blue Burst character or not? */
    if (data_len > 1056) {
        data_len = sizeof(psocn_bb_db_char_t);
    }
    else {
        data_len = 1052;
    }

    /* 压缩角色数据 */
    cmp_sz = compressBound((uLong)data_len);

    if ((cmp_buf = (Bytef*)malloc(cmp_sz))) {
        compressed = compress2(cmp_buf, &cmp_sz, (Bytef*)char_data,
            (uLong)data_len, 9);
    }

    if (compressed == Z_OK && cmp_sz < data_len) {
        sprintf(myquery, "UPDATE %s SET size = '%u', play_time = '%d', lastip = '%s', data = '", 
            CHARACTER, (unsigned)data_len, play_time, lastip);
        psocn_db_escape_str(&conn, myquery + strlen(myquery),
            (char*)cmp_buf,
            cmp_sz);
        sprintf(myquery + strlen(myquery), "' WHERE guildcard = '%" PRIu32 "' AND "
            "slot = '%" PRIu8 "'", gc, slot);
    }
    else {
        sprintf(myquery, "UPDATE %s SET size = '0', play_time = '%d', lastip = '%s', data = '", 
            CHARACTER, play_time, lastip);
        psocn_db_escape_str(&conn, myquery + strlen(myquery),
            (char*)char_data,
            data_len);
        sprintf(myquery + strlen(myquery), "' WHERE guildcard = '%" PRIu32 "' AND "
            "slot = '%" PRIu8 "'", gc, slot);
    }

    //printf("%s \n", query);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):%s", CHARACTER, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -6;
    }

    free(cmp_buf);

    return 0;
}

/* 角色备份功能 */
int db_backup_bb_char_data(uint32_t gc, uint8_t slot) {
    static char query[sizeof(psocn_bb_db_char_t) * 2 + 256];
    psocn_bb_db_char_t *char_data;
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

            data_size = (uint32_t)strtoul(row[1], NULL, 0);

            char_data = db_uncompress_char_data(len, row, data_size);

            sprintf(query, "INSERT INTO %s (guildcard, slot, size, deleteip, data) "
                "VALUES ('%" PRIu32 "', '%" PRIu8 "', '0', '%s', '", CHARACTER_DELETE
                , gc, slot, row[2]);
            psocn_db_escape_str(&conn, query + strlen(query),
                (char*)char_data,
                sizeof(psocn_bb_db_char_t));
            strcat(query, "')");

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

/* 删除角色数据 */
int db_delete_bb_char_data(uint32_t gc, uint8_t slot) {
    static char query[sizeof(psocn_bb_db_char_t) * 2 + 256];

    sprintf(query, "DELETE FROM %s WHERE guildcard="
        "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER, gc,
        slot);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    sprintf(query, "DELETE FROM %s WHERE guildcard="
        "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_STATS, gc,
        slot);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法清理旧玩家 %s 人物数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_STATS, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    sprintf(query, "DELETE FROM %s WHERE guildcard="
        "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_DRESS, gc,
        slot);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法清理旧玩家 %s 人物数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    sprintf(query, "DELETE FROM %s WHERE guildcard="
        "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_CHALLENGE, gc,
        slot);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法清理旧玩家 %s 挑战数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_CHALLENGE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    sprintf(query, "DELETE FROM %s WHERE guildcard="
        "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_BATTLE, gc,
        slot);

    if (psocn_db_real_query(&conn, query)) {
        SQLERR_LOG("无法清理旧玩家 %s 对战数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_BATTLE, gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -1;
    }

    return 0;
}

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

/* 更新玩家更衣室数据至数据库 */
int db_update_char_dress_data(psocn_dress_data_t dress_data, uint32_t gc, uint8_t slot, uint32_t flag) {

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
            CHARACTER_DRESS, 
            gc, slot, 
            dress_data.guildcard_string, 
            dress_data.dress_unk1, dress_data.dress_unk2, dress_data.name_color_b, dress_data.name_color_g, dress_data.name_color_r,
            dress_data.name_color_transparency, dress_data.model, (char*)dress_data.dress_unk3, dress_data.create_code, dress_data.name_color_checksum, dress_data.section,
            dress_data.ch_class, dress_data.v2flags, dress_data.version, dress_data.v1flags, dress_data.costume,
            dress_data.skin, dress_data.face, dress_data.head, dress_data.hair, 
            dress_data.hair_r, dress_data.hair_g, dress_data.hair_b, dress_data.prop_x, dress_data.prop_y
        );

        //DBG_LOG("保存角色更衣室数据 %d", dress_data.create_code);

        if (psocn_db_real_query(&conn, myquery)) {
            SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
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
    //        dress_data.costume, 
    //        dress_data.hair, dress_data.hair_r, dress_data.hair_g, dress_data.hair_b,
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
            CHARACTER_DRESS, gc, slot,  
            dress_data.guildcard_string, dress_data.dress_unk1, dress_data.dress_unk2, 
            dress_data.name_color_b, dress_data.name_color_g, dress_data.name_color_r, dress_data.name_color_transparency, 
            dress_data.model, (char*)dress_data.dress_unk3, dress_data.create_code, dress_data.name_color_checksum, dress_data.section,
            dress_data.ch_class, dress_data.v2flags, dress_data.version, dress_data.v1flags, 
            dress_data.costume, dress_data.skin, dress_data.face, dress_data.head, 
            dress_data.hair, dress_data.hair_r, dress_data.hair_g, dress_data.hair_b, 
            dress_data.prop_x, dress_data.prop_y
            , gc, slot
        );

        //DBG_LOG("更新角色更衣室数据");

        if (psocn_db_real_query(&conn, myquery)) {
            SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            return -6;
        }
        else {
            //DBG_LOG("更新角色更衣室数据");

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
                CHARACTER_DRESS,
                gc, slot, 
                dress_data.guildcard_string,
                dress_data.dress_unk1, dress_data.dress_unk2, dress_data.name_color_b, dress_data.name_color_g, dress_data.name_color_r,
                dress_data.name_color_transparency, dress_data.model, (char*)dress_data.dress_unk3, dress_data.create_code, dress_data.name_color_checksum, dress_data.section,
                dress_data.ch_class, dress_data.v2flags, dress_data.version, dress_data.v1flags, dress_data.costume,
                dress_data.skin, dress_data.face, dress_data.head, dress_data.hair,
                dress_data.hair_r, dress_data.hair_g, dress_data.hair_b, dress_data.prop_x, dress_data.prop_y
            );

            if (psocn_db_real_query(&conn, myquery)) {
                SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 "):\n%s", CHARACTER_DRESS, gc, slot,
                    psocn_db_error(&conn));
                /* XXXX: 未完成给客户端发送一个错误信息 */
                return -6;
            }
        }
    }

    return 0;

}

/* 获取玩家更衣室数据 */
psocn_dress_data_t db_get_char_dress_data(uint32_t gc, uint8_t slot) {
    psocn_dress_data_t dress_data;
    void* result;
    char** row;

    memset(&dress_data, 0, sizeof(psocn_dress_data_t));

    memset(myquery, 0, sizeof(myquery));

    /* 查询数据库并获取数据 */
    sprintf(myquery, "SELECT * FROM %s WHERE guildcard = '%"
        PRIu32 "' AND slot = '%"PRIu8"'", CHARACTER_DRESS, gc, slot);

    /* 获取旧玩家数据... */
    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
    }

    if (!(result = psocn_db_result_store(&conn))) {
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
    }

    if (!(row = psocn_db_result_fetch(result))) {
        SQLERR_LOG("无法获取玩家数据 (GC %" PRIu32 ", "
            "槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        psocn_db_result_free(result);
    }

    if (row) {
        int i = 2;
        memcpy(&dress_data.guildcard_string, row[i], sizeof(dress_data.guildcard_string));
        i++;
        dress_data.dress_unk1 = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.dress_unk2 = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.name_color_b = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.name_color_g = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.name_color_r = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.name_color_transparency = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.model = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        memcpy(&dress_data.dress_unk3, row[i], sizeof(dress_data.dress_unk3));
        i++;
        dress_data.create_code = (uint32_t)strtoul(row[i], NULL, 0);
        //DBG_LOG("%d %d %s", dress_data.create_code, (uint32_t)strtof(row[i], NULL), row[i]);
        i++;
        dress_data.name_color_checksum = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.section = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.ch_class = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.v2flags = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.version = (uint8_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.v1flags = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.costume = (uint16_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.skin = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.face = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.head = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.hair = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.hair_r = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.hair_g = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.hair_b = (uint32_t)strtoul(row[i], NULL, 0);
        i++;
        dress_data.prop_x = strtof(row[i], NULL);
        //memcpy(&dress_data.prop_x, row[i], sizeof(dress_data.prop_x));
        //printf("%f %f %s\n", dress_data.prop_x, strtof(row[i], NULL), row[i]);

        i++;
        dress_data.prop_y = strtof(row[i], NULL);
        //memcpy(&dress_data.prop_y, row[i], sizeof(dress_data.prop_y));

        //printf("获取了 %d 个更衣室数据 \n", i);
    }

    return dress_data;
}

/* 插入玩家角色数据 */
int db_insert_char_data(psocn_bb_db_char_t *char_data, uint32_t gc, uint8_t slot) {
    char ipstr[INET6_ADDRSTRLEN];

    memcpy(&ipstr, db_get_auth_ip(gc, slot), sizeof(ipstr));

    SYSTEMTIME rawtime;
    GetLocalTime(&rawtime);
    char timestamp[28];
    sprintf(timestamp, "%u-%02u-%02u %02u:%02u:%02u",
        rawtime.wYear, rawtime.wMonth, rawtime.wDay,
        rawtime.wHour, rawtime.wMinute, rawtime.wSecond);

    memset(myquery, 0, sizeof(myquery));

    sprintf(myquery, "INSERT INTO %s (guildcard, slot, size, ip, create_time, data) "
        "VALUES ('%" PRIu32 "', '%" PRIu8 "', '0', '%s', '%s', '", CHARACTER, gc,
        slot, ipstr, timestamp);
    psocn_db_escape_str(&conn, myquery + strlen(myquery),
        (char*)char_data,
        sizeof(psocn_bb_db_char_t));
    strcat(myquery, "')");

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法创建玩家数据 (GC %"
            PRIu32 ", 槽位 %" PRIu8 "):\n%s", gc, slot,
            psocn_db_error(&conn));
        /* XXXX: 未完成给客户端发送一个错误信息 */
        return -2;
    }

    return 0;
}

/* 更新玩家基础数据至数据库 */
int db_update_char_stat(psocn_bb_db_char_t* char_data, 
    uint32_t gc, uint8_t slot, uint32_t flag) {
    static char query[sizeof(psocn_bb_db_char_t) * 2 + 256];
    char name[64];
    char class_name[64];

    istrncpy16_raw(ic_utf16_to_utf8, name, &char_data->character.name[2], 64, BB_CHARACTER_NAME_LENGTH);

    istrncpy(ic_gbk_to_utf8, class_name, pso_class[char_data->character.disp.dress_data.ch_class].cn_name, 64);

    if (flag & PSOCN_DB_SAVE_CHAR) {
        sprintf(query, "INSERT INTO %s (guildcard, slot, name, class, class_name, meseta"
            ", level, xp, section, skin, hpmats_used"
            ", tpmats_used, inventoryUse, lang, atp, mst"
            ", evp, hp, dfp, ata, lck) "
            "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%d', '%s', '%d'"
            ", '%d', '%d', '%d', '%d', '%d'"
            ", '%d', '%d', '%d', '%d', '%d'"
            ", '%d', '%d', '%d', '%d', '%d')", CHARACTER_STATS, gc,
            slot
            , name, char_data->character.disp.dress_data.ch_class, class_name, char_data->character.disp.meseta
            , char_data->character.disp.level + 1, char_data->character.disp.exp, char_data->character.disp.dress_data.section, char_data->character.disp.dress_data.skin, char_data->inv.hpmats_used
            , char_data->inv.tpmats_used, char_data->inv.item_count, char_data->inv.language, char_data->character.disp.stats.atp, char_data->character.disp.stats.mst
            , char_data->character.disp.stats.evp, char_data->character.disp.stats.hp, char_data->character.disp.stats.dfp, char_data->character.disp.stats.ata, char_data->character.disp.stats.lck
        );

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", CHARACTER_STATS, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            return -6;
        }
    }
    else if (flag & PSOCN_DB_UPDATA_CHAR) {
        sprintf(query, "UPDATE %s SET name='%s', class='%d', class_name='%s', meseta='%d'"
            ", level='%d', xp='%d',section='%d', skin='%d', hpmats_used='%d'"
            ", tpmats_used='%d', inventoryUse='%d', lang='%d', atp='%d', mst='%d'"
            ", evp='%d', hp='%d', dfp='%d', ata='%d', lck='%d'"
            " WHERE guildcard='%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_STATS
            , name, char_data->character.disp.dress_data.ch_class, class_name, char_data->character.disp.meseta
            , char_data->character.disp.level + 1, char_data->character.disp.exp, char_data->character.disp.dress_data.section, char_data->character.disp.dress_data.skin, char_data->inv.hpmats_used
            , char_data->inv.tpmats_used, char_data->inv.item_count, char_data->inv.language, char_data->character.disp.stats.atp, char_data->character.disp.stats.mst
            , char_data->character.disp.stats.evp, char_data->character.disp.stats.hp, char_data->character.disp.stats.dfp, char_data->character.disp.stats.ata, char_data->character.disp.stats.lck
            , gc, slot
        );

        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("无法更新数据表 %s (GC %" PRIu32 ", "
                "槽位 %" PRIu8 "):\n%s", CHARACTER_STATS, gc, slot,
                psocn_db_error(&conn));
            /* XXXX: 未完成给客户端发送一个错误信息 */
            return -6;
        }
        else {
            sprintf(query, "DELETE FROM %s WHERE guildcard="
                "'%" PRIu32 "' AND slot='%" PRIu8 "'", CHARACTER_STATS, gc,
                slot);

            if (psocn_db_real_query(&conn, query)) {
                SQLERR_LOG("无法清理旧玩家 %s 数据 (GC %"
                    PRIu32 ", 槽位 %" PRIu8 "):\n%s", CHARACTER_STATS, gc, slot,
                    psocn_db_error(&conn));
                /* XXXX: 未完成给客户端发送一个错误信息 */
                return -1;
            }

            sprintf(query, "INSERT INTO %s (guildcard, slot, name, class, class_name, meseta"
                ", level, xp, section, skin, hpmats_used"
                ", tpmats_used, inventoryUse, lang, atp, mst"
                ", evp, hp, dfp, ata, lck) "
                "VALUES ('%" PRIu32 "', '%" PRIu8 "', '%s', '%d', '%s', '%d'"
                ", '%d', '%d', '%d', '%d', '%d'"
                ", '%d', '%d', '%d', '%d', '%d'"
                ", '%d', '%d', '%d', '%d', '%d')", CHARACTER_STATS, gc, slot
                , name, char_data->character.disp.dress_data.ch_class, class_name, char_data->character.disp.meseta
                , char_data->character.disp.level + 1, char_data->character.disp.exp, char_data->character.disp.dress_data.section, char_data->character.disp.dress_data.skin, char_data->inv.hpmats_used
                , char_data->inv.tpmats_used, char_data->inv.item_count, char_data->inv.language, char_data->character.disp.stats.atp, char_data->character.disp.stats.mst
                , char_data->character.disp.stats.evp, char_data->character.disp.stats.hp, char_data->character.disp.stats.dfp, char_data->character.disp.stats.ata, char_data->character.disp.stats.lck);

            if (psocn_db_real_query(&conn, query)) {
                SQLERR_LOG("无法创建数据表 %s (GC %" PRIu32 ", "
                    "槽位 %" PRIu8 "):\n%s", CHARACTER_STATS, gc, slot,
                    psocn_db_error(&conn));
                /* XXXX: 未完成给客户端发送一个错误信息 */
                return -6;
            }
        }
    }

    return 0;
}
