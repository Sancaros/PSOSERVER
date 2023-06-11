/*
    �λ�֮���й� ���ݿ���� ͷ�ļ�
    ��Ȩ (C) 2022, 2023 Sancaros

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

/* ͨ�ò�ѯ */
/* ��ȡBBְҵ��������� */
int read_player_max_tech_level_table_bb(bb_max_tech_level_t* bb_max_tech_level);

/* ��ȡBBְҵ�ȼ����� */
int read_player_level_table_bb(bb_level_table_t* bb_char_stats);

/* �������Ƿ����� */
int db_check_gc_online(uint32_t gc);

/* */
uint32_t db_remove_account_login_state_from_ship(uint16_t key_idx);

/* ������������������ */
int db_remove_gc_char_login_state(uint32_t gc);
int db_update_gc_char_login_state(uint32_t gc, uint32_t char_slot,
    uint32_t islogged, uint32_t block_num);

/* ��������˻�������� */
int db_update_gc_login_state(uint32_t gc,
    uint32_t islogged, uint32_t char_slot, char* char_name);

/* ��ȡ��ɫ���ڽ���ID */
int db_get_char_ship_id(uint32_t gc);

/* ���½�ɫ��ȫ���������ݿ� */
int db_updata_char_security(uint32_t play_time, uint32_t gc, uint8_t slot);

/* ���½�ɫ��Ϸʱ�������ݿ� */
int db_updata_char_play_time(uint32_t play_time, uint32_t gc, uint8_t slot);

/* ��ȡ��֤IP���� */
char* db_get_auth_ip(uint32_t gc, uint8_t slot);

/* ��ȡ��ɫ����ʱ�� */
char* db_get_char_create_time(uint32_t gc, uint8_t slot);

/* ��ѹ��ɫ���� */
psocn_bb_db_char_t *db_uncompress_char_data(unsigned long* len, char** row, uint32_t data_size);

/* ��ȡ����ѹδ�ṹ���Ľ�ɫ���� */
int db_uncompress_char_data_raw(uint8_t* data, char* raw_data, uint32_t data_legth, uint32_t data_size);

/* ��ȡ��ѹ��ɫ���� */
psocn_bb_db_char_t* db_get_uncompress_char_data(uint32_t gc, uint8_t slot);

/* ѹ�������½�ɫ���� */
int db_compress_char_data(psocn_bb_db_char_t *char_data, uint16_t data_len, uint32_t gc, uint8_t slot);

/* ��ɫ���ݹ��� */
int db_backup_bb_char_data(uint32_t gc, uint8_t slot);

/* ɾ����ɫ���� */
int db_delete_bb_char_data(uint32_t gc, uint8_t slot);

/* ���½�ɫ���������� */
int db_update_char_dress_data(psocn_dress_data_t* dress_data, uint32_t gc, uint8_t slot, uint32_t flag);

/* ��ȡ��ҽ�ɫ��������� */
int db_get_dress_data(uint32_t gc, uint8_t slot, psocn_dress_data_t* dress_data, int check);

/* ������ҽ�ɫ���� */
int db_insert_char_data(psocn_bb_db_char_t *char_data, uint32_t gc, uint8_t slot);

int db_update_char_disp(psocn_disp_char_t *disp_data, uint32_t gc, uint8_t slot, uint32_t flag);

/* ��ʼ�����߽������ݱ� */
int db_delete_online_ships(char* ship_name, uint16_t id);

/* ��ʼ�����߽�ɫ���ݱ� */
int db_delete_online_clients(char* ship_name, uint16_t id);

/* ��ʼ����ʱ��ɫ���ݱ� */
int db_delete_transient_clients(char* ship_name, uint16_t id);

/* ������ұ������������ݿ� */
void db_insert_inventory(inventory_t* inv, uint32_t gc, uint8_t slot);

/* ��ȡ��ҽ�ɫ�������������� */
int db_get_char_inv(uint32_t gc, uint8_t slot, inventory_t* inv, int check);

/* ���½�ɫ���������� ��ʱ����*/
int db_update_char_inventory(inventory_t* inv, uint32_t gc, uint8_t slot, uint32_t flag);

///////////////////////////////////////////////////////
/* ��֤��ѯ */

int db_upload_temp_data(void* data, size_t size);

/* ��ȡBB��ɫѡ������ */
psocn_bb_db_opts_t db_get_bb_char_option(uint32_t gc);

int db_updata_bb_char_guild_data(uint32_t guild_id, uint32_t gc);

/* ��ȡBB��ɫ�������� */
psocn_bb_db_guild_t db_get_bb_char_guild(uint32_t gc);

/* ����BB��ɫ��֤����*/
int db_update_char_auth_msg(char ipstr[INET6_ADDRSTRLEN], uint32_t gc, uint8_t slot);

/* ����BB��ɫ����������*/
int db_update_char_dressflag(uint32_t gc, uint32_t flags);

/* ������֤�������б�*/
int db_update_auth_server_list(psocn_srvconfig_t* cfg);

/* ��ȡ��֤�������б�*/
psocn_srvconfig_t db_get_auth_server_list(uint32_t id);

///////////////////////////////////////////////////////
/* ��բ��ѯ */

/* ����BB ���ᴴ�� */
int db_insert_bb_char_guild(uint16_t* guild_name, uint8_t* default_guild_flag, uint32_t gc, uint32_t version);

/* ��ȡ��ҽ�ɫ���ݳ��� */
uint32_t db_get_char_data_length(uint32_t gc, uint8_t slot);

/* ��ȡ��ҽ�ɫ���ݴ�С */
uint32_t db_get_char_data_size(uint32_t gc, uint8_t slot);

/* ��ȡ��ҽ�ɫ������ */
char* db_get_char_raw_data(uint32_t gc, uint8_t slot, int check);

/* ��ȡ��ҽ�ɫ��ֵ������ */
int db_get_char_disp(uint32_t gc, uint8_t slot, psocn_disp_char_t* data, int check);

/* ע�� һ��Ҫȷ�����������ݴ��� �ſ���ʹ�ô˺��� */
int db_updata_bb_char_create_code(uint32_t code,
    uint32_t gc, uint8_t slot);

uint32_t db_get_char_data_play_time(uint32_t gc, uint8_t slot);

/* ��ʼ����բ��������ݱ� */
int db_initialize();

/* ���½������ݱ��IPv4 */
int db_update_ship_ipv4(uint32_t ip, uint16_t key_idx);

///////////////////////////////////////////////////////////

/* ����BB��ɫ�������� */
int db_update_bb_char_guild(psocn_bb_db_guild_t guild, uint32_t gc);

/* ��������BB��ɫ */
int db_update_bb_guild_member_add(uint32_t guild_id, uint32_t gc_target);

/* ����BB��ɫ�����־ */
int db_update_bb_guild_flag(uint8_t* guild_flag, uint32_t guild_id);

/* ��ɢBB��ɫ���� */
int db_dissolve_bb_guild(uint32_t guild_id);

///////////////////////////////////////////////////////////

/* ����BB��ɫѡ������ */
int db_update_bb_char_option(psocn_bb_db_opts_t opts, uint32_t gc);

/* ����BB��ɫ��ս���� */
int db_update_char_challenge(psocn_bb_db_char_t* char_data, uint32_t gc, uint8_t slot, uint32_t flag);
///////////////////////////////////////////////////////






#endif /* !PSOCN_DATABASE_QUERY */
