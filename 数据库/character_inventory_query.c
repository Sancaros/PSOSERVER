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
#include "handle_player_items.h"
#include "pso_player.h"

static int db_insert_inv_param(inventory_t* inv, uint32_t gc, uint8_t slot) {
	uint32_t inv_crc32 = psocn_crc32((uint8_t*)inv, PSOCN_STLENGTH_INV);

	memset(myquery, 0, sizeof(myquery));

	// 插入玩家数据
	_snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
		"guildcard, slot, "
		"item_count, hpmats_used, tpmats_used, language, inv_check_num, "
		"inventory"
		") VALUES ("
		"'%" PRIu32 "', '%" PRIu8 "', "
		"'%" PRIu8 "', '%" PRIu8 "', '%" PRIu8 "', '%" PRIu8 "','%" PRIu32 "', '"
		/*")"*/,
		CHARACTER_INVENTORY,
		gc, slot,
		inv->item_count, inv->hpmats_used, inv->tpmats_used, inv->language, inv_crc32
	);

	psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)inv,
		PSOCN_STLENGTH_INV);

	SAFE_STRCAT(myquery, "')");

	if (psocn_db_real_query(&conn, myquery)) {
		//SQLERR_LOG("psocn_db_real_query() 失败: %s", psocn_db_error(&conn));
		return -1;
	}

	return 0;
}
//
//static int db_insert_inv_ext_param(inventory_t* inv, uint32_t gc, uint8_t slot, const char* table_name, const char* extension_field) {
//	memset(myquery, 0, sizeof(myquery));
//
//	// 插入玩家数据
//	_snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
//		"guildcard, slot, change_time, "
//		"%s_0, %s_1, %s_2, %s_3, %s_4, "
//		"%s_5, %s_6, %s_7, %s_8, %s_9, "
//		"%s_10, %s_11, %s_12, %s_13, %s_14, "
//		"%s_15, %s_16, %s_17, %s_18, %s_19, "
//		"%s_20, %s_21, %s_22, %s_23, %s_24, "
//		"%s_25, %s_26, %s_27, %s_28, %s_29"
//		") VALUES ("
//		"'%" PRIu32 "', '%" PRIu8 "', NOW(), "
//		"'%02X', '%02X', '%02X', '%02X', '%02X', "
//		"'%02X', '%02X', '%02X', '%02X', '%02X', "
//		"'%02X', '%02X', '%02X', '%02X', '%02X', "
//		"'%02X', '%02X', '%02X', '%02X', '%02X', "
//		"'%02X', '%02X', '%02X', '%02X', '%02X', "
//		"'%02X', '%02X', '%02X', '%02X', '%02X'"
//		")",
//		table_name,
//		extension_field, extension_field, extension_field, extension_field, extension_field,
//		extension_field, extension_field, extension_field, extension_field, extension_field,
//		extension_field, extension_field, extension_field, extension_field, extension_field,
//		extension_field, extension_field, extension_field, extension_field, extension_field,
//		extension_field, extension_field, extension_field, extension_field, extension_field,
//		extension_field, extension_field, extension_field, extension_field, extension_field,
//		gc, slot
//		, inv->iitems[0].extension_data1, inv->iitems[1].extension_data1, inv->iitems[2].extension_data1, inv->iitems[3].extension_data1, inv->iitems[4].extension_data1
//		, inv->iitems[5].extension_data1, inv->iitems[6].extension_data1, inv->iitems[7].extension_data1, inv->iitems[8].extension_data1, inv->iitems[9].extension_data1
//		, inv->iitems[10].extension_data1, inv->iitems[11].extension_data1, inv->iitems[12].extension_data1, inv->iitems[13].extension_data1, inv->iitems[14].extension_data1
//		, inv->iitems[15].extension_data1, inv->iitems[16].extension_data1, inv->iitems[17].extension_data1, inv->iitems[18].extension_data1, inv->iitems[19].extension_data1
//		, inv->iitems[20].extension_data1, inv->iitems[21].extension_data1, inv->iitems[22].extension_data1, inv->iitems[23].extension_data1, inv->iitems[24].extension_data1
//		, inv->iitems[25].extension_data1, inv->iitems[26].extension_data1, inv->iitems[27].extension_data1, inv->iitems[28].extension_data1, inv->iitems[29].extension_data1
//	);
//
//	if (psocn_db_real_query(&conn, myquery)) {
//		SQLERR_LOG("无法插入角色背包额外数据 (GC%" PRIu32 ":%" PRIu8 "槽)", gc, slot);
//		return -1;
//	}
//
//	return 0;
//}

static int db_insert_inv_ext1_param(inventory_t* inv, uint32_t gc, uint8_t slot) {
	memset(myquery, 0, sizeof(myquery));

	// 插入玩家数据
	_snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
		"guildcard, slot, change_time, "
		"ext1_0, ext1_1, ext1_2, ext1_3, ext1_4, "
		"ext1_5, ext1_6, ext1_7, ext1_8, ext1_9, "
		"ext1_10, ext1_11, ext1_12, ext1_13, ext1_14, "
		"ext1_15, ext1_16, ext1_17, ext1_18, ext1_19, "
		"ext1_20, ext1_21, ext1_22, ext1_23, ext1_24, "
		"ext1_25, ext1_26, ext1_27, ext1_28, ext1_29"
		") VALUES ("
		"'%" PRIu32 "', '%" PRIu8 "', NOW(), "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X'"
		")",
		CHARACTER_INVENTORY_ITEMS_EXT1,
		gc, slot
		, inv->iitems[0].extension_data1, inv->iitems[1].extension_data1, inv->iitems[2].extension_data1, inv->iitems[3].extension_data1, inv->iitems[4].extension_data1
		, inv->iitems[5].extension_data1, inv->iitems[6].extension_data1, inv->iitems[7].extension_data1, inv->iitems[8].extension_data1, inv->iitems[9].extension_data1
		, inv->iitems[10].extension_data1, inv->iitems[11].extension_data1, inv->iitems[12].extension_data1, inv->iitems[13].extension_data1, inv->iitems[14].extension_data1
		, inv->iitems[15].extension_data1, inv->iitems[16].extension_data1, inv->iitems[17].extension_data1, inv->iitems[18].extension_data1, inv->iitems[19].extension_data1
		, inv->iitems[20].extension_data1, inv->iitems[21].extension_data1, inv->iitems[22].extension_data1, inv->iitems[23].extension_data1, inv->iitems[24].extension_data1
		, inv->iitems[25].extension_data1, inv->iitems[26].extension_data1, inv->iitems[27].extension_data1, inv->iitems[28].extension_data1, inv->iitems[29].extension_data1
	);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法插入角色背包额外1数据 (GC%" PRIu32 ":%" PRIu8 "槽)", gc, slot);
		return -1;
	}

	return 0;
}

static int db_insert_inv_ext2_param(inventory_t* inv, uint32_t gc, uint8_t slot) {
	memset(myquery, 0, sizeof(myquery));

	// 插入玩家数据
	_snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
		"guildcard, slot, change_time, "
		"ext2_0, ext2_1, ext2_2, ext2_3, ext2_4, "
		"ext2_5, ext2_6, ext2_7, ext2_8, ext2_9, "
		"ext2_10, ext2_11, ext2_12, ext2_13, ext2_14, "
		"ext2_15, ext2_16, ext2_17, ext2_18, ext2_19, "
		"ext2_20, ext2_21, ext2_22, ext2_23, ext2_24, "
		"ext2_25, ext2_26, ext2_27, ext2_28, ext2_29"
		") VALUES ("
		"'%" PRIu32 "', '%" PRIu8 "', NOW(), "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', '%02X'"
		")",
		CHARACTER_INVENTORY_ITEMS_EXT2,
		gc, slot
		, inv->iitems[0].extension_data2, inv->iitems[1].extension_data2, inv->iitems[2].extension_data2, inv->iitems[3].extension_data2, inv->iitems[4].extension_data2
		, inv->iitems[5].extension_data2, inv->iitems[6].extension_data2, inv->iitems[7].extension_data2, inv->iitems[8].extension_data2, inv->iitems[9].extension_data2
		, inv->iitems[10].extension_data2, inv->iitems[11].extension_data2, inv->iitems[12].extension_data2, inv->iitems[13].extension_data2, inv->iitems[14].extension_data2
		, inv->iitems[15].extension_data2, inv->iitems[16].extension_data2, inv->iitems[17].extension_data2, inv->iitems[18].extension_data2, inv->iitems[19].extension_data2
		, inv->iitems[20].extension_data2, inv->iitems[21].extension_data2, inv->iitems[22].extension_data2, inv->iitems[23].extension_data2, inv->iitems[24].extension_data2
		, inv->iitems[25].extension_data2, inv->iitems[26].extension_data2, inv->iitems[27].extension_data2, inv->iitems[28].extension_data2, inv->iitems[29].extension_data2
	);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法插入角色背包额外1数据 (GC%" PRIu32 ":%" PRIu8 "槽)", gc, slot);
		return -1;
	}

	return 0;
}

static int db_insert_inv_backup_param(inventory_t* inv, uint32_t gc, uint8_t slot) {
	uint32_t inv_crc32 = psocn_crc32((uint8_t*)inv, PSOCN_STLENGTH_INV);

	memset(myquery, 0, sizeof(myquery));

	// 插入玩家数据
	_snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
		"guildcard, slot, "
		"item_count, hpmats_used, tpmats_used, language, inv_check_num, change_time, "
		"inventory"
		") VALUES ("
		"'%" PRIu32 "', '%" PRIu8 "', "
		"'%" PRIu8 "', '%" PRIu8 "', '%" PRIu8 "', '%" PRIu8 "','%" PRIu32 "', NOW(), '"
		/*")"*/,
		CHARACTER_INVENTORY_FULL_DATA,
		gc, slot,
		inv->item_count, inv->hpmats_used, inv->tpmats_used, inv->language, inv_crc32
	);

	psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)inv,
		PSOCN_STLENGTH_INV);

	SAFE_STRCAT(myquery, "')");

	if (psocn_db_real_query(&conn, myquery)) {
		//SQLERR_LOG("psocn_db_real_query() 失败: %s", psocn_db_error(&conn));
		return -1;
	}

	return 0;
}

static int db_update_inv_param(inventory_t* inv, uint32_t gc, uint8_t slot) {
	uint32_t inv_crc32 = psocn_crc32((uint8_t*)inv, PSOCN_STLENGTH_INV);
	memset(myquery, 0, sizeof(myquery));

	_snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
		"item_count = '%" PRIu8 "', hpmats_used = '%" PRIu8 "', tpmats_used = '%" PRIu8 "', language = '%" PRIu8 "', inv_check_num = '%" PRIu32 "', "
		"inventory = '",
		CHARACTER_INVENTORY,
		inv->item_count, inv->hpmats_used, inv->tpmats_used, inv->language, inv_crc32
	);

	psocn_db_escape_str(&conn, myquery + strlen(myquery), (char*)inv,
		PSOCN_STLENGTH_INV);

	snprintf(myquery + strlen(myquery), sizeof(myquery) - strlen(myquery), "' WHERE guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'", gc, slot);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("psocn_db_real_query() 失败: %s", psocn_db_error(&conn));
		return -1;
	}

	return 0;
}

static int db_update_inv_ext_param(inventory_t* inv, uint32_t gc, uint8_t slot, int isext2) {
	memset(myquery, 0, sizeof(myquery));
	const char* filed = "ext1";
	const char* tbl_nm = CHARACTER_INVENTORY_ITEMS_EXT1;

	memset(myquery, 0, sizeof(myquery));

	if (isext2) {
		filed = "ext2";
		tbl_nm = CHARACTER_INVENTORY_ITEMS_EXT2;
	}
	else {
		filed = "ext1";
		tbl_nm = CHARACTER_INVENTORY_ITEMS_EXT1;
	}

	//_snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
	//	"guildcard, slot, change_time, "
	//	"%s_0, %s_1, %s_2, %s_3, %s_4, "
	//	"%s_5, %s_6, %s_7, %s_8, %s_9, "
	//	"%s_10, %s_11, %s_12, %s_13, %s_14, "
	//	"%s_15, %s_16, %s_17, %s_18, %s_19, "
	//	"%s_20, %s_21, %s_22, %s_23, %s_24, "
	//	"%s_25, %s_26, %s_27, %s_28, %s_29"
	//	") VALUES ("
	//	"'%" PRIu32 "', '%" PRIu8 "', NOW(), "
	//	"'%02X', '%02X', '%02X', '%02X', '%02X', "
	//	"'%02X', '%02X', '%02X', '%02X', '%02X', "
	//	"'%02X', '%02X', '%02X', '%02X', '%02X', "
	//	"'%02X', '%02X', '%02X', '%02X', '%02X', "
	//	"'%02X', '%02X', '%02X', '%02X', '%02X', "
	//	"'%02X', '%02X', '%02X', '%02X', '%02X'"
	//	")"
	//	, tbl_nm
	//	, filed, filed, filed, filed, filed
	//	, filed, filed, filed, filed, filed
	//	, filed, filed, filed, filed, filed
	//	, filed, filed, filed, filed, filed
	//	, filed, filed, filed, filed, filed
	//	, filed, filed, filed, filed, filed
	//	, gc, slot
	//	, inv->iitems[0].extension_data1, inv->iitems[1].extension_data1, inv->iitems[2].extension_data1, inv->iitems[3].extension_data1, inv->iitems[4].extension_data1
	//	, inv->iitems[5].extension_data1, inv->iitems[6].extension_data1, inv->iitems[7].extension_data1, inv->iitems[8].extension_data1, inv->iitems[9].extension_data1
	//	, inv->iitems[10].extension_data1, inv->iitems[11].extension_data1, inv->iitems[12].extension_data1, inv->iitems[13].extension_data1, inv->iitems[14].extension_data1
	//	, inv->iitems[15].extension_data1, inv->iitems[16].extension_data1, inv->iitems[17].extension_data1, inv->iitems[18].extension_data1, inv->iitems[19].extension_data1
	//	, inv->iitems[20].extension_data1, inv->iitems[21].extension_data1, inv->iitems[22].extension_data1, inv->iitems[23].extension_data1, inv->iitems[24].extension_data1
	//	, inv->iitems[25].extension_data1, inv->iitems[26].extension_data1, inv->iitems[27].extension_data1, inv->iitems[28].extension_data1, inv->iitems[29].extension_data1
	//);

	//if (psocn_db_real_query(&conn, myquery)) {
	//	SQLERR_LOG("无法更新角色背包额外 %d 数据 (GC%" PRIu32 ":%" PRIu8 "槽)", isext2 + 1, gc, slot);
	//	return -1;
	//}

	for (size_t i = 0; i < MAX_PLAYER_INV_ITEMS; i++) {
		_snprintf(myquery, sizeof(myquery), "UPDATE %s SET %s_%d = '%02X' WHERE guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'"
			, tbl_nm
			, filed
			, i
			, isext2 ? inv->iitems[i].extension_data2 : inv->iitems[i].extension_data1
			, gc
			, slot
		);

		if (psocn_db_real_query(&conn, myquery)) {
			SQLERR_LOG("无法更新角色背包额外%d 第%d数据 (GC%" PRIu32 ":%" PRIu8 "槽)", isext2 + 1, i, gc, slot);
			return -1;
		}
	}

	return 0;
}

/* 优先获取背包数据库中的物品数量 */
static int db_get_char_inv_param(uint32_t gc, uint8_t slot, inventory_t* inv, int check) {
	void* result;
	char** row;

	memset(myquery, 0, sizeof(myquery));

	/* Build the query asking for the data. */
	sprintf(myquery, "SELECT "
		"item_count, hpmats_used, tpmats_used, language"
		" FROM "
		"%s"
		" WHERE "
		"guildcard = '%" PRIu32 "'"
		" AND "
		"slot = '%u'",
		CHARACTER_INVENTORY,
		gc,
		slot
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

	inv->item_count = (uint8_t)strtoul(row[0], NULL, 10);
	inv->hpmats_used = (uint8_t)strtoul(row[1], NULL, 10);
	inv->tpmats_used = (uint8_t)strtoul(row[2], NULL, 10);
	inv->language = (uint8_t)strtoul(row[3], NULL, 10);

	if (inv->item_count > MAX_PLAYER_INV_ITEMS) {
		inv->item_count = MAX_PLAYER_INV_ITEMS;
	}

	psocn_db_result_free(result);

	return 0;
}

static int db_insert_inv_items(iitem_t* item, uint32_t gc, uint8_t slot, int item_index) {
	char item_name_text[32];

	istrncpy(ic_gbk_to_utf8, item_name_text, item_get_name(&item->data, 5, 0), sizeof(item_name_text));

	memset(myquery, 0, sizeof(myquery));

	_snprintf(myquery, sizeof(myquery), "INSERT INTO %s ("
		"data_b0, data_b1, data_b2, data_b3, "
		"data_b4, data_b5, data_b6, data_b7, "
		"data_b8, data_b9, data_b10, data_b11, "
		"item_id, "
		"data2_b0, data2_b1, data2_b2, data2_b3, "
		"item_index, present, flags, "
		"item_name, "
		"guildcard, slot"
		") VALUES ("
		"'%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', "
		"'%02X', '%02X', '%02X', '%02X', "
		"'%08X', "
		"'%02X', '%02X', '%02X', '%02X', "
		"'%d', '%04X', '%08X', "
		"\'%s\', "
		"'%" PRIu32 "', '%" PRIu8 "'"
		")",
		CHARACTER_INVENTORY_ITEMS,
		item->data.datab[0], item->data.datab[1], item->data.datab[2], item->data.datab[3],
		item->data.datab[4], item->data.datab[5], item->data.datab[6], item->data.datab[7],
		item->data.datab[8], item->data.datab[9], item->data.datab[10], item->data.datab[11],
		item->data.item_id,
		item->data.data2b[0], item->data.data2b[1], item->data.data2b[2], item->data.data2b[3],
		item_index, item->present, item->flags,
		item_name_text,
		gc, slot
	);

	if (psocn_db_real_query(&conn, myquery)) {
		//SQLERR_LOG("无法新增玩家背包数据 (GC %"
		//    PRIu32 ", 槽位 %" PRIu8 "):\n%s", gc, slot,
		//    psocn_db_error(&conn));
		/* XXXX: 未完成给客户端发送一个错误信息 */
		return -1;
	}

	return 0;
}

static int db_update_inv_items(iitem_t* item, uint32_t gc, uint8_t slot, int item_index) {
	char item_name_text[64];

	istrncpy(ic_gbk_to_utf8, item_name_text, item_get_name(&item->data, 5, 0), sizeof(item_name_text));

	memset(myquery, 0, sizeof(myquery));

	_snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
		"data_b0 = '%02X', data_b1 = '%02X', data_b2 = '%02X', data_b3 = '%02X', "
		"data_b4 = '%02X', data_b5 = '%02X', data_b6 = '%02X', data_b7 = '%02X', "
		"data_b8 = '%02X', data_b9 = '%02X', data_b10 = '%02X', data_b11 = '%02X', "
		"item_id = '%08X', "
		"data2_b0 = '%02X', data2_b1 = '%02X', data2_b2 = '%02X', data2_b3 = '%02X', "
		"present = '%04X', flags = '%08X', "
		"item_name = \'%s\'"
		" WHERE "
		"(guildcard = '%" PRIu32 "') AND (slot = '%" PRIu8 "') AND (item_index = '%d')",
		CHARACTER_INVENTORY_ITEMS,
		item->data.datab[0], item->data.datab[1], item->data.datab[2], item->data.datab[3],
		item->data.datab[4], item->data.datab[5], item->data.datab[6], item->data.datab[7],
		item->data.datab[8], item->data.datab[9], item->data.datab[10], item->data.datab[11],
		item->data.item_id,
		item->data.data2b[0], item->data.data2b[1], item->data.data2b[2], item->data.data2b[3],
		item->present, item->flags,
		item_name_text,
		gc, slot, item_index
	);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("psocn_db_real_query() 失败: %s", psocn_db_error(&conn));
		return -1;
	}

	return 0;
}

static int db_del_inv_items(uint32_t gc, uint8_t slot, int item_index, int del_count) {
	memset(myquery, 0, sizeof(myquery));
	void* result;
	char** row;

	sprintf(myquery, "SELECT * FROM %s WHERE "
		"guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'",
		CHARACTER_INVENTORY_ITEMS,
		gc, slot
	);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法查询角色背包数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -1;
	}

	/* Grab the data we got. */
	if (result = psocn_db_result_store(&conn)) {
		//DBG_LOG("正在修改 '%" PRIu32 "' '%" PRIu8 "' %d\n", gc, slot, item_index);
		if ((row = mysql_fetch_row((MYSQL_RES*)result))) {

			sprintf(myquery, "INSERT INTO %s SELECT * FROM %s WHERE "
				"guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'",
				CHARACTER_INVENTORY_ITEMS_BACKUP, CHARACTER_INVENTORY_ITEMS,
				gc, slot
			);

			psocn_db_real_query(&conn, myquery);

			for (item_index; item_index < del_count; item_index++) {

				sprintf(myquery, "DELETE FROM %s WHERE "
					"guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "' AND item_index = '%" PRIu32 "'",
					CHARACTER_INVENTORY_ITEMS,
					gc, slot, item_index
				);

				if (psocn_db_real_query(&conn, myquery)) {
					SQLERR_LOG("无法清理旧玩家数据 (GC %"
						PRIu32 ", 槽位 %" PRIu8 "):\n%s", gc, slot,
						psocn_db_error(&conn));
					/* XXXX: 未完成给客户端发送一个错误信息 */
					return -1;
				}
			}

			//DBG_LOG("正在修改 %s\n", row[2]);

		}
		psocn_db_result_free(result);
	}


	return 0;
}

static int db_get_char_inv_itemdata(uint32_t gc, uint8_t slot, inventory_t* inv) {
	void* result;
	char** row;
	char* endptr;

	memset(myquery, 0, sizeof(myquery));

	/* Build the query asking for the data. */
	sprintf(myquery, "SELECT "
		"*"
		" FROM "
		"%s"
		" WHERE "
		"guildcard = '%" PRIu32 "' AND slot = '%" PRIu8 "'",
		CHARACTER_INVENTORY_ITEMS,
		gc, slot
	);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法查询角色数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -1;
	}

	/* Grab the data we got. */
	if ((result = psocn_db_result_use(&conn)) == NULL) {
		SQLERR_LOG("未获取到角色数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -2;
	}

	int k = 0;

	while ((row = psocn_db_result_fetch(result)) != NULL) {
		if (!isEmptyInt((uint16_t)strtoul(row[4], &endptr, 16))) {
			int i = 4;
			inv->iitems[k].present = (uint16_t)strtoul(row[i], &endptr, 16);
			i++;
			//DBG_LOG("extension_data1 %d 0x%02X", k, inv->iitems[k].extension_data1);
			//DBG_LOG("extension_data2 %d 0x%02X", k, inv->iitems[k].extension_data2);
			inv->iitems[k].flags = (uint32_t)strtoul(row[i], &endptr, 16);
			i++;
			//DBG_LOG("0x%08X", inv->iitems[k].flags);

			inv->iitems[k].data.datab[0] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[1] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[2] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[3] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[4] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[5] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[6] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[7] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[8] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[9] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[10] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.datab[11] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;

			inv->iitems[k].data.item_id = (uint32_t)strtoul(row[i], &endptr, 16);
			i++;

			inv->iitems[k].data.data2b[0] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.data2b[1] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.data2b[2] = (uint8_t)strtoul(row[i], &endptr, 16);
			i++;
			inv->iitems[k].data.data2b[3] = (uint8_t)strtoul(row[i], &endptr, 16);

			k++;
		}
	}

	psocn_db_result_free(result);

	return k;
}

/* 优先获取银行数据库中的物品数量 */
static int db_get_char_inv_full_data(uint32_t gc, uint8_t slot, inventory_t* inv, int backup) {
	void* result;
	char** row;
	const char* tbl_nm = CHARACTER_INVENTORY;

	memset(myquery, 0, sizeof(myquery));

	if (backup)
		tbl_nm = CHARACTER_INVENTORY_FULL_DATA;

	/* Build the query asking for the data. */
	sprintf(myquery, "SELECT inventory FROM "
		"%s "
		"WHERE "
		"guildcard = '%" PRIu32 "'"
		" AND "
		"slot = '%u' "
		"ORDER BY change_time DESC "
		"LIMIT 1",
		tbl_nm,
		gc, slot
	);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法查询角色背包数据 (GC%" PRIu32 ":%" PRIu8 "槽)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -1;
	}

	/* Grab the data we got. */
	if ((result = psocn_db_result_store(&conn)) == NULL) {
		SQLERR_LOG("未获取到角色背包数据 (GC%" PRIu32 ":%" PRIu8 "槽)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -2;
	}

	if ((row = psocn_db_result_fetch(result)) == NULL) {
		psocn_db_result_free(result);
		SQLERR_LOG("未找到保存的角色背包数据 (GC%" PRIu32 ":%" PRIu8 "槽)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -3;
	}

	if (isPacketEmpty(row[0], sizeof(inventory_t))) {
		psocn_db_result_free(result);

		SQLERR_LOG("保存的角色背包数据为空 (GC%" PRIu32 ":%" PRIu8 "槽)", gc, slot);
		return -4;
	}

	memcpy(&inv->item_count, row[0], sizeof(inventory_t));

	psocn_db_result_free(result);

	return 0;
}

static int db_get_char_ext_data(uint32_t gc, uint8_t slot, inventory_t* inv, int isext2) {
	void* result;
	char** row;
	const char* tbl_nm = CHARACTER_INVENTORY_ITEMS_EXT1;

	memset(myquery, 0, sizeof(myquery));

	if (isext2)
		tbl_nm = CHARACTER_INVENTORY_ITEMS_EXT2;

	/* Build the query asking for the data. */
	sprintf(myquery, "SELECT * FROM "
		"%s "
		"WHERE "
		"guildcard = '%" PRIu32 "'"
		" AND "
		"slot = '%u'"
		, tbl_nm
		, gc
		, slot
	);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法查询角色背包额外%d数据 (GC%" PRIu32 ":%" PRIu8 "槽)", isext2 + 1, gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -1;
	}

	/* Grab the data we got. */
	if ((result = psocn_db_result_store(&conn)) == NULL) {
		SQLERR_LOG("未获取到角色背包额外%d数据 (GC%" PRIu32 ":%" PRIu8 "槽)", isext2 + 1, gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -2;
	}

	if ((row = psocn_db_result_fetch(result)) == NULL) {
		psocn_db_result_free(result);
		SQLERR_LOG("未找到保存的角色背包额外%d数据 (GC%" PRIu32 ":%" PRIu8 "槽)", isext2 + 1, gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -3;
	}

	if (isPacketEmpty(row[0], sizeof(inventory_t))) {
		psocn_db_result_free(result);

		SQLERR_LOG("保存的角色背包额外%d数据为空 (GC%" PRIu32 ":%" PRIu8 "槽)", isext2 + 1, gc, slot);
		return -4;
	}

	int i2 = 3;
	for (int i = 0; i < MAX_PLAYER_INV_ITEMS; i++) {
		if (!isext2) {
			inv->iitems[i].extension_data1 = (uint8_t)strtoul(row[i2], NULL, 16);
			//DBG_LOG("extension_data1 %d 0x%02X 数据 i2 %d", i, inv->iitems[i].extension_data1, i2);
		}
		else {
			inv->iitems[i].extension_data2 = (uint8_t)strtoul(row[i2], NULL, 16);
			//DBG_LOG("extension_data2 %d 0x%02X 数据 i2 %d", i, inv->iitems[i].extension_data2, i2);
		}

		i2++;
	}

	psocn_db_result_free(result);

	return 0;
}

/* 优先获取背包数据库中的数量 */
static uint8_t db_get_char_inv_item_count(uint32_t gc, uint8_t slot) {
	uint8_t item_count = 0;
	void* result;
	char** row;

	memset(myquery, 0, sizeof(myquery));

	/* Build the query asking for the data. */
	sprintf(myquery, "SELECT COUNT(*) FROM %s WHERE guildcard = '%" PRIu32 "' "
		"AND slot = '%u'", CHARACTER_INVENTORY_ITEMS, gc, slot);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法查询角色背包数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return 0;
	}

	/* Grab the data we got. */
	if ((result = psocn_db_result_store(&conn)) == NULL) {
		SQLERR_LOG("未获取到角色背包数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return 0;
	}

	if ((row = psocn_db_result_fetch(result)) == NULL) {
		psocn_db_result_free(result);
		SQLERR_LOG("未找到保存的角色背包数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return 0;
	}

	item_count = (uint8_t)strtoul(row[0], NULL, 10);

	if (item_count > MAX_PLAYER_INV_ITEMS)
		item_count = MAX_PLAYER_INV_ITEMS;

	psocn_db_result_free(result);

	return item_count;
}

/* 优先获取背包数据checkum */
static uint32_t db_get_char_inv_checkum(uint32_t gc, uint8_t slot) {
	uint32_t inv_crc32 = 0;
	void* result;
	char** row;

	memset(myquery, 0, sizeof(myquery));

	/* Build the query asking for the data. */
	sprintf(myquery, "SELECT inv_check_num FROM %s WHERE guildcard = '%" PRIu32 "' "
		"AND slot = '%u'", CHARACTER_INVENTORY, gc, slot);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法查询角色背包数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -1;
	}

	/* Grab the data we got. */
	if ((result = psocn_db_result_store(&conn)) == NULL) {
		SQLERR_LOG("未获取到角色背包数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -2;
	}

	if ((row = psocn_db_result_fetch(result)) == NULL) {
		psocn_db_result_free(result);
		SQLERR_LOG("未找到保存的角色背包数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -3;
	}

	inv_crc32 = (uint32_t)strtoul(row[0], NULL, 10);

	psocn_db_result_free(result);

	return inv_crc32;
}

void clean_up_char_inv(inventory_t* inv, int item_index, int del_count) {
	for (item_index; item_index < del_count; item_index++) {
		inv->iitems[item_index].present = 0;
		inv->iitems[item_index].flags = 0;

		inv->iitems[item_index].data.datal[0] = 0;
		inv->iitems[item_index].data.datal[1] = 0;
		inv->iitems[item_index].data.datal[2] = 0;
		inv->iitems[item_index].data.item_id = 0xFFFFFFFF;
		inv->iitems[item_index].data.data2l = 0;
	}
}

/* 新增玩家背包数据至数据库 */
int db_insert_char_inv(inventory_t* inv, uint32_t gc, uint8_t slot) {
	size_t i = 0;

	if (db_insert_inv_param(inv, gc, slot)) {
		db_update_inv_param(inv, gc, slot);
	}

	if (db_insert_inv_ext1_param(inv, gc, slot)) {
		db_update_inv_ext_param(inv, gc, slot, 0);
	}

	if (db_insert_inv_ext2_param(inv, gc, slot)) {
		db_update_inv_ext_param(inv, gc, slot, 1);
	}

	// 遍历背包数据，插入到数据库中
	for (i; i < inv->item_count; i++) {
		if (db_insert_inv_items(&inv->iitems[i], gc, slot, i)) {
			db_update_inv_items(&inv->iitems[i], gc, slot, i);
		}
	}

	clean_up_char_inv(inv, inv->item_count, MAX_PLAYER_INV_ITEMS);

	if (db_del_inv_items(gc, slot, inv->item_count, MAX_PLAYER_INV_ITEMS))
		return -1;

	return 0;
}

int db_update_char_inv(inventory_t* inv, uint32_t gc, uint8_t slot) {
	uint8_t db_item_count = 0;
	size_t i = 0;
	uint8_t ic = inv->item_count;

	if (ic > MAX_PLAYER_INV_ITEMS)
		ic = db_get_char_inv_item_count(gc, slot);

	inv->item_count = ic;

	db_insert_inv_backup_param(inv, gc, slot);

	if (db_update_inv_param(inv, gc, slot)) {
		SQLERR_LOG("无法更新(GC%" PRIu32 ":%" PRIu8 "槽)角色背包参数数据", gc, slot);
		return -1;
	}

	if (db_update_inv_ext_param(inv, gc, slot, 0)) {
		SQLERR_LOG("无法更新(GC%" PRIu32 ":%" PRIu8 "槽)角色背包额外1参数数据", gc, slot);
		return -2;
	}

	if (db_update_inv_ext_param(inv, gc, slot, 1)) {
		SQLERR_LOG("无法更新(GC%" PRIu32 ":%" PRIu8 "槽)角色背包额外2参数数据", gc, slot);
		return -3;
	}

	// 遍历背包数据，插入到数据库中
	for (i = 0; i < ic; i++) {
		if (db_insert_inv_items(&inv->iitems[i], gc, slot, i)) {
			if (db_update_inv_items(&inv->iitems[i], gc, slot, i)) {
				SQLERR_LOG("无法新增(GC%" PRIu32 ":%" PRIu8 "槽)角色背包物品数据", gc, slot);
				return -4;
			}
		}
	}

	clean_up_char_inv(inv, ic, MAX_PLAYER_INV_ITEMS);

	if (db_del_inv_items(gc, slot, ic, MAX_PLAYER_INV_ITEMS))
		return -5;

	return 0;
}

/* 获取玩家角色背包数据数据项 */
int db_get_char_inv(uint32_t gc, uint8_t slot, psocn_bb_char_t* character, int check) {
	size_t i = 0;
	int rv = 0;

	inventory_t* inv = &character->inv;

	if (rv = db_get_char_inv_param(gc, slot, inv, 1)) {
		if (rv) {
			SQLERR_LOG("获取(GC%" PRIu32 ":%" PRIu8 "槽)角色背包数据为空,执行获取总数据计划.", gc, slot);
			if (db_get_char_inv_full_data(gc, slot, inv, 0)) {
				SQLERR_LOG("获取(GC%" PRIu32 ":%" PRIu8 "槽)角色背包原始数据为空,执行获取备份数据计划.", gc, slot);
				if (db_get_char_inv_full_data(gc, slot, inv, 1)) {
					SQLERR_LOG("获取(GC%" PRIu32 ":%" PRIu8 "槽)角色背包备份数据失败,执行初始化1.", gc, slot);
					db_insert_inv_param(inv, gc, slot);
					db_insert_inv_backup_param(inv, gc, slot);

				}
				else {
					SQLERR_LOG("获取(GC%" PRIu32 ":%" PRIu8 "槽)角色背包数据成功,执行初始化2", gc, slot);
					db_insert_inv_param(inv, gc, slot);

				}

				for (i = 0; i < inv->item_count; i++) {
					if (!db_insert_inv_items(&inv->iitems[i], gc, slot, i)) {
						SQLERR_LOG("插入(GC%" PRIu32 ":%" PRIu8 "槽)角色背包数据失败", gc, slot);
						return -1;
					}
				}
			}
		}
	} else {
		inv->item_count = db_get_char_inv_itemdata(gc, slot, inv);

		clean_up_char_inv(inv, inv->item_count, MAX_PLAYER_INV_ITEMS);

		if (db_del_inv_items(gc, slot, inv->item_count, MAX_PLAYER_INV_ITEMS))
			return -1;
	}

	if (db_get_char_ext_data(gc, slot, inv, 0))
		db_insert_inv_ext1_param(inv, gc, slot);


	if (db_get_char_ext_data(gc, slot, inv, 1))
		db_insert_inv_ext2_param(inv, gc, slot);

	return 0;
}