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
#include "f_iconv.h"
#include "handle_player_items.h"

/* 读取玩家最大科技魔法的等级表 */
int read_player_max_tech_level_table_bb(bb_max_tech_level_t* bb_max_tech_level) {
    char query[256];
    void* result;
    char** row;
    int i, j;
    long long row_count = 0;

    for (i = 0; i < MAX_PLAYER_TECHNIQUES; i++) {
        for (j = 0; j < MAX_PLAYER_CLASS_BB; j++) {
            sprintf(query, "SELECT * FROM %s WHERE id = '%" PRIu32 "'",
                PLAYER_MAX_TECH_LEVEL_TABLE_BB, i);

            if (psocn_db_real_query(&conn, query)) {
                ERR_LOG("Couldn't fetch monsters from database!");
                free_safe(bb_max_tech_level);
                return -1;
            }

            if ((result = psocn_db_result_store(&conn)) == NULL) {
                ERR_LOG("Could not store results of monster select!");
                free_safe(bb_max_tech_level);
                return -1;
            }

            /* Make sure we have something. */
            row_count = psocn_db_result_rows(result);

            if (row_count < 0) {
                ERR_LOG("无法获取职业法术设置!");
                psocn_db_result_free(result);
                return -1;
            }
            else if (!row_count) {
                ERR_LOG("职业法术设置数据库为空.");
                psocn_db_result_free(result);
                return 0;
            }

            row = psocn_db_result_fetch(result);
            if (!row) {
                ERR_LOG("无法获取到数组!");
                free_safe(bb_max_tech_level);
                return -1;
            }

            //memcpy(&bb_max_tech_level[i].tech_cn_name, (char*)row[1], sizeof(bb_max_tech_level[i].tech_cn_name));
            istrncpy(ic_utf8_to_gbk, bb_max_tech_level[i].tech_cn_name, (char*)row[1], sizeof(bb_max_tech_level[i].tech_cn_name));
            memcpy(&bb_max_tech_level[i].tech_name, (char*)row[2], sizeof(bb_max_tech_level[i].tech_name));
            bb_max_tech_level[i].tech_name[11] = 0x00;
            bb_max_tech_level[i].max_lvl[j] = (uint8_t)strtoul(row[j + 3], NULL, 10);
#ifdef DEBUG
            DBG_LOG("法术 %d.%s 职业 %d 等级 %d", i, bb_max_tech_level[i].tech_name, j, bb_max_tech_level[i].max_lvl[j]);
#endif // DEBUG
            psocn_db_result_free(result);
        }
    }

    return 0;
}

/* 读取BB玩家等级列表 */
int read_player_level_table_bb(bb_level_table_t* bb_char_stats) {
    char query[256];
    void* result;
    char** row;
    int i, j;
    long long row_count;

    for (i = 0; i < MAX_PLAYER_CLASS_BB; i++) {
        sprintf(query, "SELECT * FROM %s WHERE id = '%" PRIu32 "'",
            PLAYER_LEVEL_START_STATS_TABLE_BB, i);

        if (psocn_db_real_query(&conn, query)) {
            ERR_LOG("Couldn't fetch monsters from database!");
            free_safe(bb_char_stats);
            return -1;
        }

        if ((result = psocn_db_result_store(&conn)) == NULL) {
            ERR_LOG("Could not store results of monster select!");
            free_safe(bb_char_stats);
            return -1;
        }

        /* Make sure we have something. */
        row_count = psocn_db_result_rows(result);

        if (row_count < 0) {
            ERR_LOG("无法获取设置数据!");
            psocn_db_result_free(result);
            return -1;
        }
        else if (!row_count) {
            ERR_LOG("设置数据库为空.");
            psocn_db_result_free(result);
            return 0;
        }

        row = psocn_db_result_fetch(result);
        if (!row) {
            ERR_LOG("无法获取到数组!");
            free_safe(bb_char_stats);
            return -1;
        }

        bb_char_stats->start_stats[i].atp = (uint16_t)strtoul(row[4], NULL, 10);
        bb_char_stats->start_stats[i].mst = (uint16_t)strtoul(row[5], NULL, 10);
        bb_char_stats->start_stats[i].evp = (uint16_t)strtoul(row[6], NULL, 10);
        bb_char_stats->start_stats[i].hp = (uint16_t)strtoul(row[7], NULL, 10);
        bb_char_stats->start_stats[i].dfp = (uint16_t)strtoul(row[8], NULL, 10);
        bb_char_stats->start_stats[i].ata = (uint16_t)strtoul(row[9], NULL, 10);
        bb_char_stats->start_stats[i].lck = (uint16_t)strtoul(row[10], NULL, 10);
        bb_char_stats->start_stats_index[i] = (uint32_t)strtoul(row[3], NULL, 10);
        //DBG_LOG("ATP数值 %d", bb_char_stats->start_stats[i].atp);

        psocn_db_next_result_free(&conn, result);
    }

    for (j = 0; j < MAX_PLAYER_CLASS_BB; j++) {
        for (i = 0; i < MAX_PLAYER_LEVEL; i++) {
            sprintf(query, "SELECT * FROM %s%d WHERE level = '%" PRIu32 "'",
                PLAYER_LEVEL_TABLE_BB_, pso_class[j].class_code, i);

            if (psocn_db_real_query(&conn, query)) {
                ERR_LOG("无法查询法术数据表!");
                free_safe(bb_char_stats);
                return -1;
            }

            if ((result = psocn_db_result_store(&conn)) == NULL) {
                ERR_LOG("无法获取法术数据表!");
                free_safe(bb_char_stats);
                return -1;
            }

            /* Make sure we have something. */
            row_count = psocn_db_result_rows(result);

            if (row_count < 0) {
                ERR_LOG("无法获取设置数据!");
                psocn_db_result_free(result);
                return -1;
            }
            else if (!row_count) {
                ERR_LOG("设置数据库为空.");
                psocn_db_result_free(result);
                return 0;
            }

            row = psocn_db_result_fetch(result);
            if (!row) {
                ERR_LOG("无法获取到数组!");
                free_safe(bb_char_stats);
                return -1;
            }

            bb_char_stats->levels[j][i].atp = (uint8_t)strtoul(row[3], NULL, 10);
            bb_char_stats->levels[j][i].mst = (uint8_t)strtoul(row[4], NULL, 10);
            bb_char_stats->levels[j][i].evp = (uint8_t)strtoul(row[5], NULL, 10);
            bb_char_stats->levels[j][i].hp = (uint8_t)strtoul(row[6], NULL, 10);
            bb_char_stats->levels[j][i].dfp = (uint8_t)strtoul(row[7], NULL, 10);
            bb_char_stats->levels[j][i].ata = (uint8_t)strtoul(row[8], NULL, 10);
            bb_char_stats->levels[j][i].lck = (uint8_t)strtoul(row[9], NULL, 10);
            bb_char_stats->levels[j][i].tp = (uint8_t)strtoul(row[10], NULL, 10);
            bb_char_stats->levels[j][i].exp = (uint32_t)strtoul(row[2], NULL, 10);

            //DBG_LOG("职业 %d 等级 %d 经验数值 %d", j, i, bb_char_stats->levels[j][i].exp);

            psocn_db_next_result_free(&conn, result);
        }
    }
    return 0;
}

/* 检测玩家是否在线 */
int db_check_gc_online(uint32_t gc) {
    char query[256];
    void* result;
    char** row;

    /* Fill in the query. */
    sprintf(query, "SELECT guildcard FROM %s WHERE guildcard='%u'", 
        SERVER_CLIENTS_ONLINE, gc);

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

int db_update_gc_char_last_block(uint32_t gc, uint32_t char_slot, uint32_t block_num) {
    char query[512];

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
int db_update_gc_login_state(uint32_t gc, int islogged, int char_slot, char* char_name) {
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

    if (char_slot >= 0) {
        sprintf_s(query, _countof(query), "UPDATE %s SET lastchar_slot = '%d'"
            " where guildcard = '%u'",
            AUTH_ACCOUNT, char_slot, gc);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("更新GC %u 在线数据信息错误:\n %s", gc, psocn_db_error(&conn));
            return 2;
        }
    }
    else if (char_slot == -2) {

        sprintf_s(query, _countof(query), "UPDATE %s SET islogged = '%d'"
            " where guildcard = '%u'",
            CHARACTER, islogged, gc);
        if (psocn_db_real_query(&conn, query)) {
            SQLERR_LOG("更新GC %u 在线数据信息错误:\n %s", gc, psocn_db_error(&conn));
            return 1;
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
    ship_id = atoi(row[0]);
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

char* db_get_auth_ip(uint32_t gc, uint8_t slot) {
    void* result;
    char** row;

    memset(myquery, 0, sizeof(myquery));

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

int db_delete_online_ships(char* ship_name, uint16_t id) {

    memset(myquery, 0, sizeof(myquery));

    /* Remove the ship from the server_online_ships table. */
    sprintf(myquery, "DELETE FROM %s WHERE ship_id='%hu'"
        , SERVER_SHIPS_ONLINE, id);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理 %s 在线舰船数据库表 %s", ship_name,
            SERVER_SHIPS_ONLINE);
        return -1;
    }

    return 0;
}

int db_delete_online_clients(char* ship_name, uint16_t ship_id) {
    void* result;
    char** row;
    uint32_t guildcard;

    memset(myquery, 0, sizeof(myquery));


    /* Build the query asking for the data. */
    sprintf(myquery, "SELECT guildcard FROM %s WHERE ship_id = '%hu'", SERVER_CLIENTS_ONLINE, ship_id);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -1;
    }

    /* Grab the data we got. */
    if ((result = psocn_db_result_store(&conn)) == NULL) {
        SQLERR_LOG("%s", psocn_db_error(&conn));
        return -2;
    }

    while ((row = psocn_db_result_fetch(result)) != NULL) {
        guildcard = (uint32_t)strtoul(row[0], NULL, 10);
        if(guildcard)
            db_update_gc_login_state(guildcard, 0, -2, NULL);
    }

    psocn_db_result_free(result);

    /* Remove the client from the server_online_clients table. */
    sprintf(myquery, "DELETE FROM %s WHERE ship_id='%hu'"
        , SERVER_CLIENTS_ONLINE, ship_id);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理 %s 在线玩家数据库表 %s", ship_name,
            SERVER_CLIENTS_ONLINE);
        return -2;
    }

    return 0;
}

int db_delete_transient_clients(char* ship_name, uint16_t id) {

    memset(myquery, 0, sizeof(myquery));

    /* Remove the client from the server_transient_clients table. */
    sprintf(myquery, "DELETE FROM %s WHERE ship_id='%hu'"
        , SERVER_CLIENTS_TRANSIENT, id);

    if (psocn_db_real_query(&conn, myquery)) {
        SQLERR_LOG("无法清理 %s 临时玩家数据库表 %s", ship_name,
            SERVER_CLIENTS_TRANSIENT);
        return -1;
    }

    return 0;
}