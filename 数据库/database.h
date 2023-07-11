/*
    梦幻之星中国 数据库
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

#ifndef PSOCN_DATABASE_H
#define PSOCN_DATABASE_H

#include "psoconfig.h"
#include <mysql.h>

/* NOTE: 数据写入必须为UTF8字符串 */

#pragma comment(lib , "libmariadb.lib")
#pragma comment(lib , "mariadbclient.lib")

#define MAX_QUERY_SIZE 0x10000

static char myquery[MAX_QUERY_SIZE] = { 0 };

#define AUTH_ACCOUNT "auth_account"
#define AUTH_ACCOUNT_BLUEBURST "auth_account_blueburst"
#define AUTH_ACCOUNT_DREAMCAST "auth_account_dreamcast"
#define AUTH_ACCOUNT_DREAMCAST_NTE "auth_account_dreamcast_nte"
#define AUTH_ACCOUNT_GAMECUBE "auth_account_gamecube"
#define AUTH_ACCOUNT_GUILDCARDS "auth_account_guildcards"
#define AUTH_ACCOUNT_PC "auth_account_pc"
#define AUTH_ACCOUNT_XBOX "auth_account_xbox"

#define AUTH_BANS "auth_bans"
#define AUTH_BANS_GC "auth_bans_gc"
#define AUTH_BANS_HW "auth_bans_hw"
#define AUTH_BANS_IP "auth_bans_ip" //IP封禁功能还未完成
#define AUTH_BANS_IP6 "auth_bans_ip6" //IP封禁功能还未完成

#define AUTH_LOGIN_TOKENS "auth_login_tokens"
#define AUTH_SECURITY "auth_security"
#define AUTH_SERVER_LIST "auth_server_list"
//#define AUTH_QUESTITEM "auth_questitem"

#define CHARACTER "character_"
#define CHARACTER_BACKUP "character_backup"
#define CHARACTER_BANK "character_bank"
#define CHARACTER_BANK_ITEMS "character_bank_items"
#define CHARACTER_BANK_ITEMS_BACKUP "character_bank_items_backup"
#define CHARACTER_BLACKLIST "character_blacklist"
#define CHARACTER_DEFAULT "character_default"
#define CHARACTER_DELETE "character_delete"
#define CHARACTER_DISP "character_disp"
#define CHARACTER_DRESS "character_dress"
#define CHARACTER_FRIENDLIST "character_friendlist"
#define CHARACTER_GCDATA "character_gcdata"
#define CHARACTER_INVENTORY "character_inventory"
#define CHARACTER_INVENTORY_ITEMS "character_inventory_items"
#define CHARACTER_INVENTORY_ITEMS_BACKUP "character_inventory_items_backup"
#define CHARACTER_QUEST_DATA1 "character_quest_data1"
#define CHARACTER_QUEST_DATA2 "character_quest_data2"
#define CHARACTER_RECORDS_BATTLE "character_records_battle"
#define CHARACTER_RECORDS_CHALLENGE "character_records_challenge"
#define CHARACTER_TECHNIQUES "character_techniques"
#define CHARACTER_TRADE "character_trade"

#define CLIENTS_GUILD "clients_guild"
#define CLIENTS_GUILDCARDS "clients_guildcards"
#define CLIENTS_OPTION "clients_option"
#define CLIENTS_OPTION_BLUEBURST "clients_option_blueburst"

#define PLAYER_LEVEL_START_STATS_TABLE_BB "player_level_start_stats_table_bb"
//全职业数据总表
#define PLAYER_LEVEL_TABLE_BB "player_level_table_bb"
//用于读取职业分表 player_level_table_bb_id id是索引 0-11
#define PLAYER_LEVEL_TABLE_BB_ "player_level_table_bb_"

#define PLAYER_MAX_TECH_LEVEL_TABLE_BB "player_max_tech_level_table_bb"

#define QUEST_FLAGS_LONG "quest_flags_long"
#define QUEST_FLAGS_SHORT "quest_flags_short"

//#define SERVER_CLIENTS_BLUEBURST "server_clients_blueburst"
#define SERVER_BLOCKS_LIST "server_block_list"
#define SERVER_CLIENTS_COUNT "server_clients_count"
#define SERVER_CLIENTS_ONLINE "server_clients_online"
#define SERVER_CLIENTS_TRANSIENT "server_clients_transient"
#define SERVER_EVENT_MONSTER "server_event_monster"
#define SERVER_EVENT_MONSTER_DISQ "server_event_monster_disq"
#define SERVER_EVENT_MONSTER_KILLS "server_event_monster_kills"
#define SERVER_EVENTS "server_events"
#define SERVER_SHIPS "server_ships"
#define SERVER_SHIPS_METADATA "server_ships_metadata"
#define SERVER_SHIPS_ONLINE "server_ships_online"
#define SERVER_SIMPLE_MAIL "server_simple_mail"

//以下参数表未启用
#define PARAM_ATTACK "param_attack"
#define PARAM_BATTLE "param_battle"
#define PARAM_MOVEMENT "param_movement"
#define PARAM_RESISTANCE "param_resistance"

#define PMT_ARMOR "pmt_armor"
#define PMT_EVENT_ITEM "pmt_event_item"
#define PMT_MAG "pmt_mag"
#define PMT_MAG_FEED "pmt_mag_feed"
#define PMT_SHIELD "pmt_shield"
#define PMT_TECH "pmt_tech"
#define PMT_TOOL "pmt_tool"
#define PMT_UNIT "pmt_unit"
#define PMT_UNSEALABLE_ITEM "pmt_unsealable_item"
#define PMT_WEAPON "pmt_weapon"

#define SHIP_KILLCOUNT "ship_killcount"
#define SHIP_SHOP "ship_shop"
#define SHIP_MONSTER_EVENTS "ship_monster_events"
#define SHIP_MONSTER_EVENTS_DISQ "ship_monster_events_disq"

enum mysql_config {
    MYSQL_CONFIG_HOST = 0x00,
    MYSQL_CONFIG_USER,
    MYSQL_CONFIG_PASS,
    MYSQL_CONFIG_DATABASE,
    MYSQL_CONFIG_PORT,
    MYSQL_CONFIG_CHARACTER_SET,
    MYSQL_CONFIG_AUTO_RECONNECT,
    MYSQL_CONFIG_WAITTIMEOUT,
    MYSQL_CONFIG_MAX
};

typedef struct psocn_dbconn {
    void* conndata;
} psocn_dbconn_t;

// 参数类型枚举
typedef enum {
    PARAM_TYPE_UINT8,
    PARAM_TYPE_UINT16,
    PARAM_TYPE_UINT32,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_CHAR,
    PARAM_TYPE_WCHAR,
    PARAM_TYPE_STRING,
    PARAM_TYPE_STRUCT,
    PARAM_TYPE_BLOB,
    PARAM_TYPE_UNKNOWN
} psocn_param_type_t;

// 数据库参数结构体
typedef struct {
    psocn_param_type_t type;  // 参数类型（MYSQL_TYPE_*）
    void* value;            // 参数值的指针
    unsigned long length;   // 参数值的长度
} psocn_db_param_t;

extern int psocn_db_open(psocn_dbconfig_t* dbcfg,
    psocn_dbconn_t* conn);
extern void psocn_db_close(psocn_dbconn_t* conn);

/* 普通查询 */
extern int psocn_db_query(psocn_dbconn_t* conn, const char* str);
/* 支持二进制查询 */
extern int psocn_db_real_query(psocn_dbconn_t* conn, const char* str);

extern void* psocn_db_result_store(psocn_dbconn_t* conn);
extern void* psocn_db_result_use(psocn_dbconn_t* conn);
extern void psocn_db_result_free(void* result);
extern void psocn_db_next_result_free(psocn_dbconn_t* conn, void* result);

extern long long int psocn_db_result_rows(void* result);
extern unsigned int psocn_db_result_fields(psocn_dbconn_t* conn);
extern char** psocn_db_result_fetch(void* result);
unsigned long* psocn_db_result_lengths(void* result);

extern long long int psocn_db_insert_id(psocn_dbconn_t* conn);
extern unsigned long psocn_db_escape_str(psocn_dbconn_t* conn, char* to,
    const char* from,
    unsigned long len);
/* 静态数据库字符串 */
extern unsigned long psocn_db_str(psocn_dbconn_t* conn, char* q, const char* str, unsigned long len);
extern const char* psocn_db_error(psocn_dbconn_t* conn);

extern int psocn_db_commit(psocn_dbconn_t* conn);
extern unsigned long long psocn_db_affected_rows(psocn_dbconn_t* conn);

/// <参数化查询>
/// //////////////////////////////////////////////////////////////////////////////////////////
/// </参数化查询>

int psocn_db_stmt_query(psocn_dbconn_t* conn, MYSQL_STMT* stmt, const char* str, unsigned long length, MYSQL_BIND* params);
int psocn_db_squery(psocn_dbconn_t* conn, const char* str, MYSQL_BIND* params);
int psocn_db_real_squery(psocn_dbconn_t* conn, const char* str, MYSQL_BIND* params);
char** psocn_db_result_sfetch(void* result, int num_fields);

// 销毁参数对象
extern void psocn_destroy_db_param(psocn_db_param_t* param);
extern int psocn_set_db_param_value(psocn_db_param_t* param, psocn_param_type_t type, void* value, size_t length);

extern void psocn_set_uint8_param(psocn_db_param_t* param, uint8_t value);
extern void psocn_set_uint16_param(psocn_db_param_t* param, uint16_t value);
extern void psocn_set_uint32_param(psocn_db_param_t* param, uint32_t value);
extern void psocn_set_float_param(psocn_db_param_t* param, float value);
extern void psocn_set_char_param(psocn_db_param_t* param, char value);
extern void psocn_set_wchar_param(psocn_db_param_t* param, wchar_t* value);
extern void psocn_set_string_param(psocn_db_param_t* param, const char* value);
extern void psocn_set_struct_param(psocn_db_param_t* param, void* value, size_t size);
extern void psocn_set_blob_param(psocn_db_param_t* param, void* value, size_t size);
extern void psocn_set_unknown_param(psocn_db_param_t* param, void* value, size_t size);

// 输出参数值
extern void psocn_output_param_value(const psocn_db_param_t* param);

extern int psocn_db_param_query(psocn_dbconn_t* conn, const char* query, const psocn_db_param_t* params, int param_count);

#include "database_query.h"

#endif /* !PSOCN_DATABASE_H */
