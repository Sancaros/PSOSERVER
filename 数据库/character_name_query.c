/*
	梦幻之星中国 数据库操作
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

#include <f_iconv.h>
#include <pso_text.h>

int db_update_char_name(psocn_bb_char_name_t* name, uint32_t gc, uint8_t slot) {
	char tmp_name[20] = { 0 };

	istrncpy16_raw(ic_utf16_to_utf8, tmp_name, (char*)&name->char_name, 20, 10);

	if (name->name_tag != 0x0009)
		name->name_tag = 0x0009;

	if (name->name_tag2 != 0x0042)
		name->name_tag2 = 0x0042;

	memset(myquery, 0, sizeof(myquery));

	snprintf(myquery, sizeof(myquery), "UPDATE %s SET "
		"name_tag = '%04X', name_tag2 = '%04X', char_name = '%s' WHERE guildcard = '%" PRIu32 "' AND "
		"slot = '%" PRIu8 "'",
		CHARACTER,
		name->name_tag, name->name_tag2, tmp_name,
		gc, slot
	);

	if (psocn_db_real_query(&conn, myquery)) {
		SQLERR_LOG("无法更新角色 %s 名字数据!", CHARACTER);
		return -1;
	}

	return 0;
}

int db_get_char_name(uint32_t gc, uint8_t slot, psocn_bb_char_name_t* name) {
	char tmp_name[20] = { 0 };
	void* result;
	char** row;
	size_t in, out;
	char* inptr;
	char* outptr;
	int i = 0, j;

	memset(myquery, 0, sizeof(myquery));

	/* Build the query asking for the data. */
	sprintf(myquery, "SELECT "
		"name_tag, name_tag2, char_name"
		" FROM "
		"%s"
		" WHERE "
		"guildcard = '%" PRIu32 "' "
		"AND slot = '%" PRIu8 "'"
		, CHARACTER
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

		SQLERR_LOG("未找到保存的角色数据 (%" PRIu32 ": %u)", gc, slot);
		SQLERR_LOG("%s", psocn_db_error(&conn));
		return -3;
	}

	j = 0;

	if (isPacketEmpty(row[j], 2)) {
		psocn_db_result_free(result);

		SQLERR_LOG("保存的角色数据为空 (%" PRIu32 ": %u)", gc, slot);
		return -4;
	}

	name->name_tag = (uint16_t)strtoul(row[j], NULL, 16);

	j++;

	if (isPacketEmpty(row[j], 2)) {
		psocn_db_result_free(result);

		SQLERR_LOG("保存的角色数据为空 (%" PRIu32 ": %u)", gc, slot);
		return -5;
	}

	name->name_tag2 = (uint16_t)strtoul(row[j], NULL, 16);

	j++;

	if (isPacketEmpty(row[j], 2)) {
		psocn_db_result_free(result);

		SQLERR_LOG("保存的角色数据为空 (%" PRIu32 ": %u)", gc, slot);
		return -6;
	}

	memcpy(&tmp_name[0], row[j], 20);

	memset(name->char_name, 0, sizeof(name->char_name));

	in = strlen(tmp_name);
	inptr = tmp_name;
	out = 0x14;
	outptr = (char*)&name->char_name[0];

	iconv(ic_utf8_to_utf16, &inptr, &in, &outptr, &out);

	psocn_db_result_free(result);

	return 0;
}