/*
    梦幻之星中国 数据库操作 头文件
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

#ifndef PSOCN_DATABASE_QUERY
#define PSOCN_DATABASE_QUERY

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <inttypes.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include <pso_character.h>
#include <WinSock_Defines.h>

#include <f_logs.h>
#include <f_iconv.h>

#include <zlib.h>

#include <debug.h>
#include <f_logs.h>
#include <mtwist.h>
#include "max_tech_level.h"

#define PSOCN_DB_LOAD_CHAR              0x00000000
#define PSOCN_DB_SAVE_CHAR              0x00000001
#define PSOCN_DB_UPDATA_CHAR            0x00000002

/* 通用查询 */
/* 获取BB职业最大法术数据 */
int read_player_max_tech_level_table_bb(bb_max_tech_level_t* bb_max_tech_level);

/* 获取BB职业等级数据 */
int read_player_level_table_bb(bb_level_table_t* bb_char_stats);

/* 检测玩家是否在线 */
int db_check_gc_online(uint32_t gc);

/* */
uint32_t db_remove_account_login_state_from_ship(uint16_t key_idx);

/* 更新玩家数据在线情况 */
int db_remove_gc_char_login_state(uint32_t gc);
int db_update_gc_char_login_state(uint32_t gc, uint32_t char_slot,
    uint32_t islogged, uint32_t block_num);

/* 更新玩家账户在线情况 */
int db_update_gc_login_state(uint32_t gc,
    uint32_t islogged, uint32_t char_slot, char* char_name);

/* 获取角色所在舰船ID */
int db_get_char_ship_id(uint32_t gc);

/* 更新角色安全数据至数据库 */
int db_updata_char_security(uint32_t play_time, uint32_t gc, uint8_t slot);

/* 更新角色游戏时间至数据库 */
int db_updata_char_play_time(uint32_t play_time, uint32_t gc, uint8_t slot);

/* 获取认证IP数据 */
char* db_get_auth_ip(uint32_t gc, uint8_t slot);

/* 获取角色创建时间 */
char* db_get_char_create_time(uint32_t gc, uint8_t slot);

/* 解压角色数据 */
psocn_bb_db_char_t *db_uncompress_char_data(unsigned long* len, char** row, uint32_t data_size);

/* 获取并解压未结构化的角色数据 */
int db_uncompress_char_data_raw(uint8_t* data, char* raw_data, uint32_t data_legth, uint32_t data_size);

/* 获取解压角色数据 */
psocn_bb_db_char_t* db_get_uncompress_char_data(uint32_t gc, uint8_t slot);

/* 压缩并更新角色数据 */
int db_compress_char_data(psocn_bb_db_char_t *char_data, uint16_t data_len, uint32_t gc, uint8_t slot);

/* 角色备份功能 */
int db_backup_bb_char_data(uint32_t gc, uint8_t slot);

/* 删除角色数据 */
int db_delete_bb_char_data(uint32_t gc, uint8_t slot);

/* 更新角色更衣室数据 */
int db_update_char_dress_data(psocn_dress_data_t* dress_data, uint32_t gc, uint8_t slot, uint32_t flag);

/* 获取玩家角色外观数据项 */
int db_get_dress_data(uint32_t gc, uint8_t slot, psocn_dress_data_t* dress_data, int check);

/* 插入玩家角色数据 */
int db_insert_char_data(psocn_bb_db_char_t *char_data, uint32_t gc, uint8_t slot);

int db_update_char_disp(psocn_disp_char_t *disp_data, uint32_t gc, uint8_t slot, uint32_t flag);

/* 初始化在线舰船数据表 */
int db_delete_online_ships(char* ship_name, uint16_t id);

/* 初始化在线角色数据表 */
int db_delete_online_clients(char* ship_name, uint16_t id);

/* 初始化临时角色数据表 */
int db_delete_transient_clients(char* ship_name, uint16_t id);

/* 更新玩家背包数据至数据库 */
void db_insert_inventory(inventory_t* inv, uint32_t gc, uint8_t slot);

/* 获取玩家角色背包数据数据项 */
int db_get_char_inv(uint32_t gc, uint8_t slot, inventory_t* inv, int check);

/* 更新角色更衣室数据 暂时弃用*/
int db_update_char_inventory(inventory_t* inv, uint32_t gc, uint8_t slot, uint32_t flag);

///////////////////////////////////////////////////////
/* 认证查询 */

int db_upload_temp_data(void* data, size_t size);

/* 获取BB角色选项数据 */
psocn_bb_db_opts_t db_get_bb_char_option(uint32_t gc);

int db_updata_bb_char_guild_data(uint32_t guild_id, uint32_t gc);

/* 获取BB角色公会数据 */
psocn_bb_db_guild_t db_get_bb_char_guild(uint32_t gc);

/* 更新BB角色认证数据*/
int db_update_char_auth_msg(char ipstr[INET6_ADDRSTRLEN], uint32_t gc, uint8_t slot);

/* 更新BB角色更衣室数据*/
int db_update_char_dressflag(uint32_t gc, uint32_t flags);

/* 更新认证服务器列表*/
int db_update_auth_server_list(psocn_srvconfig_t* cfg);

/* 获取认证服务器列表*/
psocn_srvconfig_t db_get_auth_server_list(uint32_t id);

///////////////////////////////////////////////////////
/* 船闸查询 */

/* 处理BB 公会创建 */
int db_insert_bb_char_guild(uint16_t* guild_name, uint8_t* default_guild_flag, uint32_t gc, uint32_t version);

/* 获取玩家角色数据长度 */
uint32_t db_get_char_data_length(uint32_t gc, uint8_t slot);

/* 获取玩家角色数据大小 */
uint32_t db_get_char_data_size(uint32_t gc, uint8_t slot);

/* 获取玩家角色数据项 */
char* db_get_char_raw_data(uint32_t gc, uint8_t slot, int check);

/* 获取玩家角色数值数据项 */
int db_get_char_disp(uint32_t gc, uint8_t slot, psocn_disp_char_t* data, int check);

/* 注意 一定要确保更衣室数据存在 才可以使用此函数 */
int db_updata_bb_char_create_code(uint32_t code,
    uint32_t gc, uint8_t slot);

uint32_t db_get_char_data_play_time(uint32_t gc, uint8_t slot);

/* 初始化舰闸所需的数据表 */
int db_initialize();

/* 更新舰船数据表的IPv4 */
int db_update_ship_ipv4(uint32_t ip, uint16_t key_idx);

///////////////////////////////////////////////////////////

/* 更新BB角色公会数据 */
int db_update_bb_char_guild(psocn_bb_db_guild_t guild, uint32_t gc);

/* 公会新增BB角色 */
int db_update_bb_guild_member_add(uint32_t guild_id, uint32_t gc_target);

/* 更新BB角色公会标志 */
int db_update_bb_guild_flag(uint8_t* guild_flag, uint32_t guild_id);

/* 解散BB角色公会 */
int db_dissolve_bb_guild(uint32_t guild_id);

///////////////////////////////////////////////////////////

/* 更新BB角色选项数据 */
int db_update_bb_char_option(psocn_bb_db_opts_t opts, uint32_t gc);

/* 更新BB角色挑战数据 */
int db_update_char_challenge(psocn_bb_db_char_t* char_data, uint32_t gc, uint8_t slot, uint32_t flag);
///////////////////////////////////////////////////////






#endif /* !PSOCN_DATABASE_QUERY */
