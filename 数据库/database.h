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
#define AUTH_BANNED "auth_banned"
#define AUTH_GUILD "auth_guild"
#define AUTH_QUESTITEM "auth_questitem"
#define AUTH_SECURITY "auth_security"
#define AUTH_BANS "auth_bans"
#define AUTH_BANS_GC "auth_bans_gc"
#define AUTH_BANS_HW "auth_bans_hw"
#define AUTH_BANS_IP "auth_bans_ip" //IP封禁功能还未完成
#define AUTH_BANS_IP6 "auth_bans_ip6" //IP封禁功能还未完成
#define AUTH_LOGIN_TOKENS "auth_login_tokens"

#define CHARACTER "character"
//#define CHARACTER_KEY "character_key"
#define CHARACTER_BANK "character_bank"
#define CHARACTER_BACKUP "character_backup"
#define CHARACTER_BATTLE "character_battle"
#define CHARACTER_BLACKLIST "character_blacklist"
#define CHARACTER_CHALLENGE "character_challenge"
#define CHARACTER_DELETE "character_delete"
#define CHARACTER_DRESS_DATA "character_dress_data"
#define CHARACTER_FRIENDLIST "character_friendlist"
//#define CHARACTER_GCCARDS "character_gccards"
#define CHARACTER_STATS "character_stats"
//#define CHARACTER_TRADE "character_trade"

#define CLIENTS_BLUEBURST "clients_blueburst"
#define CLIENTS_BLUEBURST_GUILD "clients_blueburst_guild"
#define CLIENTS_BLUEBURST_GUILDCARDS "clients_blueburst_guildcards"
#define CLIENTS_BLUEBURST_OPTION "clients_blueburst_option"
#define CLIENTS_DREAMCAST "clients_dreamcast"
#define CLIENTS_DREAMCAST_NTE "clients_dreamcast_nte"
#define CLIENTS_GAMECUBE "clients_gamecube"
#define CLIENTS_PC "clients_pc"
#define CLIENTS_XBOX "clients_xbox"
#define CLIENTS_GUILDCARDS "clients_guildcards"
#define CLIENTS_OPTIONS "clients_options"

#define QUEST_FLAGS_LONG "quest_flags_long"
#define QUEST_FLAGS_SHORT "quest_flags_short"

//#define SERVER_CLIENTS_BLUEBURST "server_clients_blueburst"
#define SERVER_BLOCKS_LIST "server_block_list"
#define SERVER_CLIENTS_COUNT "server_clients_count"
#define SERVER_CLIENTS_ONLINE "server_clients_online"
#define SERVER_CLIENTS_TRANSIENT "server_clients_transient"
#define SERVER_EVENT_MONSTER "server_event_monster"
#define SERVER_EVENT_MONSTER_KILLS "server_event_monster_kills"
#define SERVER_EVENT_MONSTER_DISQ "server_event_monster_disq"
#define SERVER_EVENTS "server_events"
#define SERVER_SHIPS_DATA "server_ships"
#define SERVER_SHIPS_METADATA "server_ships_metadata"
#define SERVER_SHIPS_ONLINE "server_ships_online"
#define SERVER_SIMPLE_MAIL "server_simple_mail"

#define PARAM_DATA_ATTACK "param_data_attack"
#define PARAM_DATA_BATTLE "param_data_battle"
#define PARAM_DATA_MOVEMENT "param_data_movement"
#define PARAM_DATA_RESISTANCE "param_data_resistance"

#define PMT_DATA_ARMOR "pmt_data_armor"
#define PMT_DATA_EVENT_ITEM "pmt_data_event_item"
#define PMT_DATA_MAG "pmt_data_mag"
#define PMT_DATA_MAG_FEED "pmt_data_mag_feed"
#define PMT_DATA_SHIELD "pmt_data_shield"
#define PMT_DATA_TECH "pmt_data_tech"
#define PMT_DATA_TOOL "pmt_data_tool"
#define PMT_DATA_UNIT "pmt_data_unit"
#define PMT_DATA_UNSEALABLE_ITEM "pmt_data_unsealable_item"
#define PMT_DATA_WEAPON "pmt_data_weapon"

#define SHIP_DATA_KILLCOUNT "ship_data_killcount"
#define SHIP_DATA_SHOP "ship_data_shop"
#define SHIP_MONSTER_EVENTS "ship_monster_events"
#define SHIP_MONSTER_EVENTS_DISQ "ship_monster_events_disq"

typedef enum Mysql_Config {
    MYSQL_CONFIG_HOST = 0x00,
    MYSQL_CONFIG_USER,
    MYSQL_CONFIG_PASS,
    MYSQL_CONFIG_DATABASE,
    MYSQL_CONFIG_PORT,
    MYSQL_CONFIG_CHARACTER_SET,
    MYSQL_CONFIG_AUTO_RECONNECT,
    MYSQL_CONFIG_WAITTIMEOUT,
    MYSQL_CONFIG_MAX
}MYSQL_CONFIG;

typedef struct psocn_dbconn {
    void* conndata;
} psocn_dbconn_t;

extern int psocn_db_open(psocn_dbconfig_t* dbcfg,
    psocn_dbconn_t* conn);
extern void psocn_db_close(psocn_dbconn_t* conn);

/* 普通查询 */
extern int psocn_db_query(psocn_dbconn_t* conn, const char* str);
/* 支持二进制查询 */
extern int psocn_db_real_query(psocn_dbconn_t* conn, const char* str);

extern void* psocn_db_result_store(psocn_dbconn_t* conn);
extern void psocn_db_result_free(void* result);

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

#include "database_query.h"

#endif /* !PSOCN_DATABASE_H */
