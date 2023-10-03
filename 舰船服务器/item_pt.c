/*
	梦幻之星中国 舰船服务器
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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <windows.h>
#include <WinSock_Defines.h>

#include <items.h>
#include <f_logs.h>
#include <debug.h>
#include <SFMT.h>

#include <AFS.h>
#include <GSL.h>

#include "ptdata.h"
#include "pmtdata.h"
#include "rtdata.h"
#include "subcmd.h"
#include "subcmd_send.h"
#include "handle_player_items.h"
#include "utils.h"
#include "quests.h"
#include "mag_bb.h"
#include "itemrt_remap.h"

#define LOG(team, ...) team_log_write(team, TLOG_DROPS, __VA_ARGS__)
#define LOGV(team, ...) team_log_write(team, TLOG_DROPSV, __VA_ARGS__)

int pt_read_v2(const char* fn) {
	pso_afs_read_t* a;
	pso_error_t err;
	ssize_t sz;
	int rv = 0, i, j, k;
#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
	int l;
#endif
	fpt_v2_entry_t* buf;
	pt_v2_entry_t* ent;

	if (!(buf = (fpt_v2_entry_t*)malloc(sizeof(fpt_v2_entry_t)))) {
		ERR_LOG("无法为itempt条目分配内存空间!");
		return -5;
	}

	/* Open up the file and make sure it looks sane enough... */
	if (!(a = pso_afs_read_open(fn, 0, &err))) {
		ERR_LOG("无法读取 %s: %s", fn, pso_strerror(err));
		free_safe(buf);
		return -1;
	}

	/* Make sure the archive has the correct number of entries. */
	if (pso_afs_file_count(a) != 40) {
		ERR_LOG("%s 数据似乎不完整.", fn);
		rv = -2;
		goto out;
	}

	/* Parse each entry... */
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 10; ++j) {
			sz = pso_afs_file_size(a, i * 10 + j);
			if (sz != 0x0940) {
				ERR_LOG("%s 条目大小无效!", fn);
				rv = -3;
				goto out;
			}

			/* Grab the data... */
			sz = sizeof(fpt_v2_entry_t);
			if (pso_afs_file_read(a, i * 10 + j, buf,
				(size_t)sz) != sz) {
				ERR_LOG("无法读取 %s 数据!", fn);
				rv = -4;
				goto out;
			}

			/* Dump it into our nicer (not packed) structure. */
			ent = &v2_ptdata[i][j];
			memcpy(ent, buf, sz);

			//print_ascii_hex(buf, sz);

			//for (int x = 0; x < 12; x++) {
			//	DBG_LOG("weapon_ratio 0x%02X", ent->weapon_ratio[x]);
			//}

			memcpy(ent->weapon_ratio, buf->weapon_ratio, 12);
			memcpy(ent->weapon_minrank, buf->weapon_minrank, 12);
			memcpy(ent->weapon_upgfloor, buf->weapon_upgfloor, 12);
			memcpy(ent->element_ranking, buf->element_ranking, 10);
			memcpy(ent->element_probability, buf->element_probability, 10);
			memcpy(ent->armor_ranking, buf->armor_ranking, 5);
			memcpy(ent->slot_ranking, buf->slot_ranking, 5);
			memcpy(ent->unit_level, buf->unit_level, 10);
			memcpy(ent->enemy_dar, buf->enemy_dar, 100);
			memcpy(ent->enemy_drop, buf->enemy_drop, 100);
			//ent->armor_level = LE32(buf->armor_level);
			ent->armor_level = buf->armor_level;
			for (k = 0; k < 3; ++k) {
				ent->unused2[k] = buf->unused2[k];
			}

			for (k = 0; k < 9; ++k) {
				memcpy(ent->power_pattern[k], buf->power_pattern[k], 4);
			}

			for (k = 0; k < 23; ++k) {
				memcpy(ent->percent_pattern[k], buf->percent_pattern[k], 5);
			}

			for (k = 0; k < 3; ++k) {
				memcpy(ent->area_pattern[k], buf->area_pattern[k], 10);
			}

			for (k = 0; k < 6; ++k) {
				memcpy(ent->percent_attachment[k], buf->percent_attachment[k],
					10);
			}

			for (k = 0; k < 7; ++k) {
				memcpy(ent->box_drop[k], buf->box_drop[k], 10);
			}

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
			for (k = 0; k < 28; ++k) {
				//memcpy(ent->tool_frequency[k], buf->tool_frequency[k], 20);
				for (l = 0; l < 10; ++l) {
					ent->tool_frequency[k][l] = LE16(ent->tool_frequency[k][l]);
				}
			}
#endif

			for (k = 0; k < 19; ++k) {
				memcpy(ent->tech_frequency[k], buf->tech_frequency[k], 10);
				memcpy(ent->tech_levels[k], buf->tech_levels[k], 20);
			}

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
			for (k = 0; k < 100; ++k) {
				//memcpy(ent->enemy_meseta[k], buf->enemy_meseta[k], 4);
				ent->enemy_meseta[k][0] = LE16(ent->enemy_meseta[k][0]);
				ent->enemy_meseta[k][1] = LE16(ent->enemy_meseta[k][1]);
			}
#endif

#if defined(__BIG_ENDIAN__) || defined(WORDS_BIGENDIAN)
			for (k = 0; k < 10; ++k) {
				//memcpy(ent->box_meseta[k], buf->box_meseta[k], 4);
				ent->box_meseta[k][0] = LE16(ent->box_meseta[k][0]);
				ent->box_meseta[k][1] = LE16(ent->box_meseta[k][1]);
			}
#endif
		}
	}

	have_v2pt = 1;

out:
	pso_afs_read_close(a);
	free_safe(buf);
	return rv;
}

int pt_read_v3(const char* fn) {
	pso_gsl_read_t* a;
	const char difficulties[4] = { 'n', 'h', 'v', 'u' };
	const char* episodes[4] = { "", "l" , "c", "cl" };
	char filename[32];
	uint32_t hnd;
	const size_t sz = sizeof(fpt_v3_entry_t);
	pso_error_t err;
	int rv = 0, 章节, 难度, 颜色, l, m;
	fpt_v3_entry_t* buf;
	pt_v3_entry_t* ent;

	if (!(buf = (fpt_v3_entry_t*)malloc(sizeof(fpt_v3_entry_t)))) {
		ERR_LOG("无法为itempt条目分配内存空间!");
		return -6;
	}

	/* Open up the file and make sure it looks sane enough... */
	if (!(a = pso_gsl_read_open(fn, 0, &err))) {
		ERR_LOG("无法读取 %s: %s", fn, pso_strerror(err));
		free_safe(buf);
		return -1;
	}

	/* Make sure the archive has the correct number of entries. */
	if (pso_gsl_file_count(a) != 160) {
		ERR_LOG("%s 数据似乎不完整.", fn);
		rv = -2;
		goto out;
	}

	/* Now, parse each entry... */
	for (章节 = 0; 章节 < 4; ++章节) {
		for (难度 = 0; 难度 < 4; ++难度) {
			for (颜色 = 0; 颜色 < 10; ++颜色) {
				/* Figure out the name of the file in the archive that we're
				   looking for... */
				snprintf(filename, 32, "ItemPT%s%c%d.rel", episodes[章节],
					difficulties[难度], 颜色);

				//printf("%s \n", filename);

				/* Grab a handle to that file. */
				hnd = pso_gsl_file_lookup(a, filename);
				if (hnd == PSOARCHIVE_HND_INVALID) {
					ERR_LOG("%s 缺少 %s 文件!", fn, filename);
					rv = -3;
					goto out;
				}

				/* Make sure the size is correct. */
				if (pso_gsl_file_size(a, hnd) != 0x9E0) {
					ERR_LOG("%s 文件 %s 大小无效!", fn,
						filename);
					rv = -4;
					goto out;
				}

				if (pso_gsl_file_read(a, hnd, (uint8_t*)buf, sz) != sz) {
					ERR_LOG("读取 %s 错误,路径 %s!",
						filename, fn);
					rv = -5;
					goto out;
				}

				/* Dump it into our nicer (not packed) structure. */
				ent = &gc_ptdata[章节][难度][颜色];
				memcpy(ent, buf, sz);

				memcpy(ent->weapon_ratio, buf->weapon_ratio, 12);
				memcpy(ent->weapon_minrank, buf->weapon_minrank, 12);
				memcpy(ent->weapon_upgfloor, buf->weapon_upgfloor, 12);
				memcpy(ent->element_ranking, buf->element_ranking, 10);
				memcpy(ent->element_probability, buf->element_probability, 10);
				memcpy(ent->armor_ranking, buf->armor_ranking, 5);
				memcpy(ent->slot_ranking, buf->slot_ranking, 5);
				memcpy(ent->unit_level, buf->unit_level, 10);
				memcpy(ent->enemy_dar, buf->enemy_dar, 100);
				memcpy(ent->enemy_drop, buf->enemy_drop, 100);
				ent->armor_level = buf->armor_level;
				for (l = 0; l < 3; ++l) {
					ent->unused2[l] = buf->unused2[l];
				}

				for (l = 0; l < 9; ++l) {
					memcpy(ent->power_pattern[l], buf->power_pattern[l], 4);
				}

				for (l = 0; l < 3; ++l) {
					memcpy(ent->area_pattern[l], buf->area_pattern[l], 10);
				}

				for (l = 0; l < 6; ++l) {
					memcpy(ent->percent_attachment[l],
						buf->percent_attachment[l], 10);
				}

				for (l = 0; l < 7; ++l) {
					memcpy(ent->box_drop[l], buf->box_drop[l], 10);
				}

				for (l = 0; l < 19; ++l) {
					memcpy(ent->tech_frequency[l], buf->tech_frequency[l], 10);
					memcpy(ent->tech_levels[l], buf->tech_levels[l], 20);
				}

				for (l = 0; l < 23; ++l) {
					for (m = 0; m < 6; ++m) {
						ent->percent_pattern[l][m] =
							ntohs(buf->percent_pattern[l][m]);
					}
				}

				for (l = 0; l < 28; ++l) {
					for (m = 0; m < 10; ++m) {
						ent->tool_frequency[l][m] =
							ntohs(buf->tool_frequency[l][m]);
					}
				}

				for (l = 0; l < 100; ++l) {
					ent->enemy_meseta[l][0] = ntohs(buf->enemy_meseta[l][0]);
					ent->enemy_meseta[l][1] = ntohs(buf->enemy_meseta[l][1]);
				}

				for (l = 0; l < 10; ++l) {
					ent->box_meseta[l][0] = ntohs(buf->box_meseta[l][0]);
					ent->box_meseta[l][1] = ntohs(buf->box_meseta[l][1]);
				}
			}
		}
	}

	have_gcpt = 1;

out:
	pso_gsl_read_close(a);
	free_safe(buf);
	return rv;
}

int pt_read_bb(const char* fn) {
	pso_gsl_read_t* a;
	const char difficulties[4] = { 'n', 'h', 'v', 'u' };
	//EP1 0  NULL / EP2 1  l /  CHALLENGE1 2 c / CHALLENGE2 3 cl / EP4 4 bb
	const char* game_type[5] = { "", "l" , "c", "cl", "bb" };
	char filename[32];
	uint32_t hnd;
	const size_t sz = sizeof(fpt_bb_entry_t);
	pso_error_t err;
	int rv = 0, 章节, 难度, 颜色, l, m;
	fpt_bb_entry_t* buf;
	pt_bb_entry_t* ent;

	if (!(buf = (fpt_bb_entry_t*)malloc(sizeof(fpt_bb_entry_t)))) {
		ERR_LOG("无法为 ItemPT 数据条目分配内存空间!");
		return -6;
	}

	/* Open up the file and make sure it looks sane enough... */
	if (!(a = pso_gsl_read_open(fn, 0, &err))) {
		ERR_LOG("%d 无法读取 BB %s: %s", a, fn, pso_strerror(err));
		free_safe(buf);
		return -1;
	}

	/* Make sure the archive has the correct number of entries. */
	if (pso_gsl_file_count(a) != 0xC8) {
		ERR_LOG("%s 数据似乎不完整. 读取字节 %d", fn, pso_gsl_file_count(a));
		rv = -2;
		goto out;
	}

	/* Now, parse each entry... */
	for (章节 = 0; 章节 < 5; ++章节) {
		for (难度 = 0; 难度 < 4; ++难度) {
			for (颜色 = 0; 颜色 < 10; ++颜色) {
				/* Figure out the name of the file in the archive that we're
				   looking for... */
				snprintf(filename, 32, "ItemPT%s%c%d.rel", game_type[章节],
					tolower(abbreviation_for_difficulty(难度)), 颜色);

				//printf("%s | ", filename);

				/* Grab a handle to that file. */
				hnd = pso_gsl_file_lookup(a, filename);
				if (hnd == PSOARCHIVE_HND_INVALID) {
					ERR_LOG("%s 缺少 %s 文件!", fn, filename);
					rv = -3;
					goto out;
				}

				/* Make sure the size is correct. */
				if (pso_gsl_file_size(a, hnd) != 0x9E0) {
					ERR_LOG("%s 文件 %s 大小无效!", fn,
						filename);
					rv = -4;
					goto out;
				}

				if (pso_gsl_file_read(a, hnd, buf, sz) != sz) {
					ERR_LOG("读取 %s 错误,路径 %s!",
						filename, fn);
					rv = -5;
					goto out;
				}

				/* Dump it into our nicer (not packed) structure. */
				ent = &bb_ptdata[章节][难度][颜色];
				memcpy(ent, buf, sz);

				memcpy(ent->base_weapon_type_prob_table, buf->weapon_ratio, 12);
				memcpy(ent->subtype_base_table, buf->weapon_minrank, 12);
				memcpy(ent->subtype_area_length_table, buf->weapon_upgfloor, 12);

				for (l = 0; l < 9; ++l) {
					memcpy(ent->grind_prob_tables[l], buf->power_pattern[l], 4);
				}

				for (l = 0; l < 23; ++l) {
					for (m = 0; m < 6; ++m) {
						ent->bonus_value_prob_tables[l][m] =
							ntohs(buf->percent_pattern[l][m]);
					}
				}

				for (l = 0; l < 3; ++l) {
					memcpy(ent->nonrare_bonus_prob_spec[l], buf->area_pattern[l], 10);
				}

				for (l = 0; l < 6; ++l) {
					memcpy(ent->bonus_type_prob_tables[l],
						buf->percent_attachment[l], 10);
				}

				memcpy(ent->special_mult, buf->element_ranking, 10);
				memcpy(ent->special_percent, buf->element_probability, 10);
				memcpy(ent->armor_shield_type_index_prob_table, buf->armor_ranking, 5);
				memcpy(ent->armor_slot_count_prob_table, buf->slot_ranking, 5);
				memcpy(ent->unit_maxes, buf->unit_level, 10);

				for (l = 0; l < 28; ++l) {
					for (m = 0; m < 10; ++m) {
						ent->tool_class_prob_table[l][m] =
							ntohs(buf->tool_frequency[l][m]);
#ifdef DEBUG
						DBG_LOG("ep %d dif %d secid %d l %d m %d value 0x%04X %d", 章节, 难度, 颜色, l, m, ent->tool_class_prob_table[l][m], ent->tool_class_prob_table[l][m]);
#endif // DEBUG
					}
				}

				for (l = 0; l < 19; ++l) {
					memcpy(ent->technique_index_prob_table[l], buf->tech_frequency[l], 10);
					memcpy(ent->technique_level_ranges[l], buf->tech_levels[l], 20);
				}

				memcpy(ent->enemy_type_drop_probs, buf->enemy_dar, 100);

				for (l = 0; l < 100; ++l) {
					ent->enemy_meseta_ranges[l].min = ntohs(buf->enemy_meseta[l][0]);
					ent->enemy_meseta_ranges[l].max = ntohs(buf->enemy_meseta[l][1]);
				}

				memcpy(ent->enemy_item_classes, buf->enemy_drop, 100);

				for (l = 0; l < 10; ++l) {
					ent->box_meseta_ranges[l].min = ntohs(buf->box_meseta[l][0]);
					ent->box_meseta_ranges[l].max = ntohs(buf->box_meseta[l][1]);
				}

				for (l = 0; l < 7; ++l) {
					memcpy(ent->box_drop[l], buf->box_drop[l], 10);
				}

				//ent->armor_level = ntohl(buf->armor_level);
				ent->armor_level = buf->armor_level;
				for (l = 0; l < 3; ++l) {
					ent->unused2[l] = buf->unused2[l];
				}

#ifdef DEBUG
				//memcpy(ent, buf, sz);

				//print_ascii_hex(buf, sz);

				//if (章节 == 4) {
				//	DBG_LOG("%s", filename);
				//	for (int x = 0; x < 100; x++) {
				//		DBG_LOG("enemy_type_drop_probs x %d %d", x, ent->enemy_type_drop_probs[x]);
				//	}

				//	getchar();
				//}
				DBG_LOG("%s", filename);
				for (int x = 0; x < 100; x++) {
					DBG_LOG("enemy_drop x %d 0x%02X", x, ent->enemy_drop[x]);
				}

				getchar();
#endif // DEBUG
			}
		}
	}

	have_bbpt = 1;

out:
	pso_gsl_read_close(a);
	free_safe(buf);
	return rv;
}

pt_bb_entry_t* pt_dynamics_read_bb(const char* fn, int 章节, int 难度, int 颜色) {
	pso_gsl_read_t* a;
	const char difficulties[4] = { 'n', 'h', 'v', 'u' };
	//EP1 0  NULL / EP2 1  l /  CHALLENGE1 2 c / CHALLENGE2 3 cl / EP4 4 bb
	const char* game_type[5] = { "", "l" , "c", "cl", "bb" };
	char filename[32];
	uint32_t hnd;
	const size_t sz = sizeof(fpt_bb_entry_t);
	pso_error_t err;
	int rv = 0, l, m;
	fpt_bb_entry_t* buf;
	pt_bb_entry_t* ent;

	if (!(buf = (fpt_bb_entry_t*)malloc(sizeof(fpt_bb_entry_t)))) {
		ERR_LOG("无法为 ItemPT 数据条目分配内存空间!");
		return NULL;
	}

	/* Open up the file and make sure it looks sane enough... */
	if (!(a = pso_gsl_read_open(fn, 0, &err))) {
		ERR_LOG("%d 无法读取 BB %s: %s", a, fn, pso_strerror(err));
		goto out;
	}

	/* Make sure the archive has the correct number of entries. */
	if (pso_gsl_file_count(a) != 0xC8) {
		ERR_LOG("%s 数据似乎不完整. 读取字节 %d", fn, pso_gsl_file_count(a));
		goto out;
	}

	/* 现在读取特定的章节... */
				/* Figure out the name of the file in the archive that we're
				   looking for... */
	snprintf(filename, 32, "ItemPT%s%c%d.rel", game_type[章节],
		tolower(abbreviation_for_difficulty(难度)), 颜色);

#ifdef DEBUG

	DBG_LOG("%s", filename);

#endif // DEBUG

	/* Grab a handle to that file. */
	hnd = pso_gsl_file_lookup(a, filename);
	if (hnd == PSOARCHIVE_HND_INVALID) {
		ERR_LOG("%s 缺少 %s 文件!", fn, filename);
		goto out;
	}

	/* Make sure the size is correct. */
	if (pso_gsl_file_size(a, hnd) != 0x9E0) {
		ERR_LOG("%s 文件 %s 大小无效!", fn,
			filename);
		goto out;
	}

	if (pso_gsl_file_read(a, hnd, buf, sz) != sz) {
		ERR_LOG("读取 %s 错误,路径 %s!",
			filename, fn);
		goto out;
	}

	/* Dump it into our nicer (not packed) structure. */
	ent = &bb_dymnamic_ptdata[章节][难度][颜色];
	memcpy(ent, buf, sz);

	memcpy(ent->base_weapon_type_prob_table, buf->weapon_ratio, 12);
	memcpy(ent->subtype_base_table, buf->weapon_minrank, 12);
	memcpy(ent->subtype_area_length_table, buf->weapon_upgfloor, 12);

	for (l = 0; l < 9; ++l) {
		memcpy(ent->grind_prob_tables[l], buf->power_pattern[l], 4);
	}

	for (l = 0; l < 23; ++l) {
		for (m = 0; m < 6; ++m) {
			ent->bonus_value_prob_tables[l][m] =
				ntohs(buf->percent_pattern[l][m]);
		}
	}

	for (l = 0; l < 3; ++l) {
		memcpy(ent->nonrare_bonus_prob_spec[l], buf->area_pattern[l], 10);
	}

	for (l = 0; l < 6; ++l) {
		memcpy(ent->bonus_type_prob_tables[l],
			buf->percent_attachment[l], 10);
	}

	memcpy(ent->special_mult, buf->element_ranking, 10);
	memcpy(ent->special_percent, buf->element_probability, 10);
	memcpy(ent->armor_shield_type_index_prob_table, buf->armor_ranking, 5);
	memcpy(ent->armor_slot_count_prob_table, buf->slot_ranking, 5);
	memcpy(ent->unit_maxes, buf->unit_level, 10);

	for (l = 0; l < 28; ++l) {
		for (m = 0; m < 10; ++m) {
			ent->tool_class_prob_table[l][m] =
				ntohs(buf->tool_frequency[l][m]);
#ifdef DEBUG
			DBG_LOG("ep %d dif %d secid %d l %d m %d value 0x%04X %d", 章节, 难度, 颜色, l, m, ent->tool_class_prob_table[l][m], ent->tool_class_prob_table[l][m]);
#endif // DEBUG
		}
	}

	for (l = 0; l < 19; ++l) {
		memcpy(ent->technique_index_prob_table[l], buf->tech_frequency[l], 10);
		memcpy(ent->technique_level_ranges[l], buf->tech_levels[l], 20);
	}

	memcpy(ent->enemy_type_drop_probs, buf->enemy_dar, 100);

	for (l = 0; l < 100; ++l) {
		ent->enemy_meseta_ranges[l].min = ntohs(buf->enemy_meseta[l][0]);
		ent->enemy_meseta_ranges[l].max = ntohs(buf->enemy_meseta[l][1]);
	}

	memcpy(ent->enemy_item_classes, buf->enemy_drop, 100);

	for (l = 0; l < 10; ++l) {
		ent->box_meseta_ranges[l].min = ntohs(buf->box_meseta[l][0]);
		ent->box_meseta_ranges[l].max = ntohs(buf->box_meseta[l][1]);
	}

	for (l = 0; l < 7; ++l) {
		memcpy(ent->box_drop[l], buf->box_drop[l], 10);
	}

	//ent->armor_level = ntohl(buf->armor_level);
	ent->armor_level = buf->armor_level;
	for (l = 0; l < 3; ++l) {
		ent->unused2[l] = buf->unused2[l];
	}


#ifdef DEBUG

	for (int x = 0; x < 12; x++) {
		DBG_LOG("base_weapon_type_prob_table x %d 0x%02X", x, ent->base_weapon_type_prob_table[x]);
	}

	//memcpy(ent, buf, sz);

	//print_ascii_hex(buf, sz);

	//if (章节 == 4) {
	//	DBG_LOG("%s", filename);
	//	for (int x = 0; x < 100; x++) {
	//		DBG_LOG("enemy_type_drop_probs x %d %d", x, ent->enemy_type_drop_probs[x]);
	//	}

	//	getchar();
	//}
	DBG_LOG("%s", filename);
	for (int x = 0; x < 100; x++) {
		DBG_LOG("enemy_drop x %d 0x%02X", x, ent->enemy_drop[x]);
	}

	getchar();
#endif // DEBUG

	have_bbpt = 1;

out:
	pso_gsl_read_close(a);
	free_safe(buf);
	return ent;
}

pt_bb_entry_t* get_pt_data_bb(uint8_t episode, uint8_t challenge, uint8_t difficulty, uint8_t section) {
	uint8_t game_type = 0;

	//EP1 0  NULL / EP2 1  l /  CHALLENGE1 2 c / CHALLENGE2 3 cl / EP4 4 bb

	switch (episode)
	{
	case GAME_TYPE_NORMAL:
	case GAME_TYPE_EPISODE_1:
		if (challenge)
			game_type = 2;
		else
			game_type = 0;
		break;

	case GAME_TYPE_EPISODE_2:
		if (challenge)
			game_type = 3;
		else
			game_type = 1;
		break;

	case GAME_TYPE_EPISODE_3:
	case GAME_TYPE_EPISODE_4:
		game_type = 4;
		break;
	}

	pt_bb_entry_t* tmp = pt_dynamics_read_bb(ship->cfg->bb_ptdata_file, game_type, difficulty, section);
	if (!tmp)
		return &bb_ptdata[game_type][difficulty][section];

	return tmp;
}

int pt_v2_enabled(void) {
	return have_v2pt;
}

int pt_gc_enabled(void) {
	return have_gcpt;
}

int pt_bb_enabled(void) {
	return have_bbpt;
}

size_t get_pt_index(uint8_t episode, size_t pt_index) {
	size_t ep4_pt_index_offset = 0x57;//87 Item_PT EP4 enemy_index 差值
	size_t new_pt_index = pt_index;
	return (episode == GAME_TYPE_EPISODE_3 ? (new_pt_index - ep4_pt_index_offset) : episode == GAME_TYPE_EPISODE_4 ? (new_pt_index - ep4_pt_index_offset) : new_pt_index);
}

int get_pt_data_area_bb(uint8_t episode, int cur_area) {
	int area = 0;

	switch (episode) {
	case GAME_TYPE_NORMAL:
	case GAME_TYPE_EPISODE_1:
	{
		switch (cur_area) {
		case 0:
			area = -1;
			break;

			/* Gal Gryphon -> SeaSide Daytime */
		case 11:
			area = 3;
			break;

			/* Olga Flow -> SeaBed Upper Levels */
		case 12:
			area = 6;
			break;

			/* Barba Ray -> VR Space Ship Alpha */
		case 13:
			area = 8;
			break;

			/* Gol Dragon -> Jungle North */
		case 14:
			area = 10;
			break;

		default:
			area = cur_area;
			///* Everything after Dark Falz -> Ruins 3 */
			//if (area > 14)
			//	area = 10;
			//else
			//	area = 1;// unknown area

			break;

		}
	}
	break;

	case GAME_TYPE_EPISODE_2:
	{
		switch (cur_area) {
		case 0:
			area = -1;
			break;

			/* Gal Gryphon -> SeaSide Daytime */
		case 12:
			area = 9;
			break;

			/* Olga Flow -> SeaBed Upper Levels */
		case 13:
			area = 10;
			break;

			/* Barba Ray -> VR Space Ship Alpha */
		case 14:
			area = 3;
			break;

			/* Gol Dragon -> Jungle North */
		case 15:
			area = 6;
			break;

		default:
			// could be tower
			if (cur_area <= 11)
				area = ep2_rtremap[(cur_area * 2) + 1];
			else
				area = 0x0A;

			///* All others after SeaBed Upper Levels -> SeaBed Upper Levels */
			//if (area > 10)
			//	area = 10; // override_area

			break;
		}
	}
	break;

	case GAME_TYPE_EPISODE_3:
	case GAME_TYPE_EPISODE_4:
		area = cur_area + 1;
		break;
	}

	if (area < 0)
		return 1;

	if (area > 10)
		area = 10;

	/* Subtract one, since we want the index in the box_drop array */
	return --area;
}

/*
   Generate a random weapon, based off of data for PSOv2. This is a rather ugly
   process, so here's a "short" description of how it actually works (note, the
   code itself will probably end up being shorter than the description (at least
   in number of bytes), but I felt this was needed to explain the insanity
   below)...

   First, we have to decide what weapon types are actually going to be available
   to generate. This involves looking at three things, the weapon ratio for the
   weapon type in the PT data, the minimum rank information (also in the PT
   data) and the area we're generating for. If the weapon ratio is zero, we
   don't have to look any further (this is expected to be a somewhat rare
   occurance, hence why its actually checked second in the code). Next, take the
   minimum rank and add the area number to it (where 0 = Forest 1, 9 = Ruins 3
   boss areas use the area that comes immediately after them, except Falz, which
   uses Ruins 3). If that result is greater than or equal to zero, then we can
   actually generate this type of weapon, so we add it to the list of weapons we
   might generate, and add its ratio into the total.

   The next thing we have to do is to decide what rank of weapon will be
   generated for each valid weapon type. To do this, we need to look at three
   pieces of data again: the weapon type's minimum rank, the weapon type's
   upgrade floor value (once again, in the PT data), and the area number. If the
   minimum rank is greater than or equal to zero, start with that value as the
   rank that will be generated. Otherwise, start with 0. Next, calculate the
   effective area for the rest of the calculation. For weapon types where the
   minimum rank is greater than or equal to zero, this will be the area number
   itself. For those where the minimum rank is less than zero, this will be the
   sum of the minimum rank and the area. Next, take the upgrade floor value and
   subtract it from the effective area. If the result is greater than or equal
   to zero, add one to the weapon's rank. Continue doing this until the result
   is less than zero.

   From here, we can actually decide what weapon itself to generate. To do this,
   simply take sum of the chances of all valid weapon types and generate a
   random number between 0 and that sum (including 0, excluding the sum). Then,
   run through and compare with the chances that each type has until you figure
   out what type to generate. Shift the weapon rank that was calculated above
   into place with the weapon type value, and there you go.

   To pick a grind value, take the remainder from the upgrade floor/effective
   area calculation above, and use that as an index into the power patterns in
   the PT data (the second index, not the first one). If this value is greater
   than 3 for some reason, cap it at 3. The power paterns in here should be used
   to decide what grind value to use. Pick a random number between 0 and 100
   (not including 100, but including 0) and see where you end up in the array.
   Whatever index you end up on is the grind value to use.

   To deal with percentages, we have a few things from the PT data to look at.
   For each of the three percentage slots, look at the "area pattern" element
   for the slot and the area the weapon is being generated for. This controls
   which set of items in the percent pattern to look at. If this is less than
   zero, move onto the next possible slot. From there, generate a random number
   between 0 and 100 (not including 100, including 0) and look through that
   column of the matrix formed by the percent pattern to figure out where you
   fall. The value of the percentage will be five times the quantity of whatever
   row of the matrix the random number ends up resulting in being selected,
   minus two. If its zero, you can obviously stop processing here, and move onto
   the next possible slot. Next, generate a new random number between 0 and 100,
   following the same rules as always. For this one, look through the percent
   attachment matrix with the random number (with the area as the column, as
   usual). If you end up in the zeroth row, then you won't actually apply a
   percentage. If not, set the value in the weapon in the appropriate place and
   put the percentage in too. Now you can finally move onto the next slot!
   Wasn't that fun? Yes, you could rearrange the random choices in here to be a
   bit more sane, but that's less fun!

   To pick an element, first check if the area has elements available. This is
   done by looking at the PT data's element ranking and element probability for
   the area. If either is 0, then there shall be no elements on weapons made on
   that floor. If both are non-zero, then generate a random number between 0 and
   100 (not including 100, but including 0). If that value is less than the
   elemental probability for the floor, then look and see how many elements are
   on the ranking level for the floor. Generate a random number to index into
   the elemental grid for that ranking level, and set that as the elemental
   attribute byte. Also, set the high-order bit of that byte so that the user
   has to take the item to the tekker.

   That takes care of all of the tasks with generating a random common weapon!
   If you're still reading this at the end of this comment, I sorta feel a bit
   sorry for you. Just think how I feel while writing the comment and the code
   below. :P
*/
static int generate_weapon_v2(pt_v2_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked, int v1,
	lobby_t* l) {
	uint32_t rnd, upcts = 0, wchance = 0;
	int i, j = 0, k, warea = 0, npcts = 0;
	int wtypes[12] = { 0 }, wranks[12] = { 0 }, gptrn[12] = { 0 };
	uint8_t* item_b = (uint8_t*)item;
	int semirare = 0, rare = 0;

	/* Ugly... but I'm lazy and don't feel like rebalancing things for another
	   level of indentation... */
	if (picked) {
		/* Determine if the item is a rare or a "semi-rare" item. */
		if (item_b[1] > 12)
			rare = semirare = 1;
		else if ((item_b[1] >= 10 && item_b[2] > 3) || item_b[2] > 4)
			semirare = 1;

		/* All RT-generated items use the last grind pattern... */
		warea = 3;
		item[1] = item[2] = item[3] = 0;
		goto already_picked;
	}

	item[0] = item[1] = item[2] = item[3] = 0;

	/* Go through each weapon type to see what ones we actually will have to
	   work with right now... */
	for (i = 0; i < 12; ++i) {
		if ((ent->weapon_minrank[i] + area) >= 0 && ent->weapon_ratio[i] > 0) {
			wtypes[j] = i;
			wchance += ent->weapon_ratio[i];

			if (ent->weapon_minrank[i] >= 0) {
				warea = area;
				wranks[j] = ent->weapon_minrank[i];
			}
			else {
				warea = ent->weapon_minrank[i] + area;
				wranks[j] = 0;
			}

			/* 合理性检查... Make sure this is sane before we go to the loop
			   below, since it will end up being an infinite loop if its not
			   sane... */
			if (ent->weapon_upgfloor[i] <= 0) {
				ITEM_LOG("无效 v2 weapon upgrade floor value for "
					"层级 %d, 武器类型 %d. 请检查您的 ItemPT.afs 文件是否有效!", area, i);
				return -1;
			}

			while ((warea - ent->weapon_upgfloor[i]) >= 0) {
				++wranks[j];
				warea -= ent->weapon_upgfloor[i];
			}

			gptrn[j] = min(warea, 3);
			++j;
		}
	}

	/* 合理性检查... This shouldn't happen! */
	if (!j) {
		ITEM_LOG("在层级 %d 无法生成 v2 版本的 武器. 请检查您的 ItemPT.afs 文件是否有效!", area);
		return -1;
	}

	/* Roll the dice! */
	rnd = sfmt_genrand_uint32(rng) % wchance;
	for (i = 0; i < j; ++i) {
		if ((rnd -= ent->weapon_ratio[wtypes[i]]) > wchance) {
			item[0] = ((wtypes[i] + 1) << 8) | (wranks[i] << 16);

			/* Save off the grind pattern to use... */
			warea = gptrn[i];
			break;
		}
	}

	/* 合理性检查... Once again, this shouldn't happen! */
	if (!item[0]) {
		ITEM_LOG("生成无效 v2 武器. 请通知程序员处理!");
		return -1;
	}

	/* See if we made a "semi-rare" item. */
	if ((item_b[1] >= 10 && item_b[2] > 3) || item_b[2] > 4)
		semirare = 1;

already_picked:
	/* Next up, determine the grind value. */
	rnd = sfmt_genrand_uint32(rng) % 100;
	for (i = 0; i < 9; ++i) {
		if ((rnd -= ent->power_pattern[i][warea]) > 100) {
			item[0] |= (i << 24);
			break;
		}
	}

	/* 合理性检查... */
	if (i >= 9) {
		ITEM_LOG("Invalid power pattern for floor %d, pattern "
			"number %d. 请检查您的 ItemPT.afs 文件是否有效!",
			area, warea);
		return -1;
	}

	/* Let's generate us some percentages, shall we? This isn't necessarily the
	   way I would have designed this, but based on the way the data is laid
	   out in the PT file, this is the implied structure of it... */
	for (i = 0; i < 3; ++i) {
		if (ent->area_pattern[i][area] < 0)
			continue;

		rnd = sfmt_genrand_uint32(rng) % 100;
		warea = ent->area_pattern[i][area];

		for (j = 0; j < 23; ++j) {
			/* See if we're going to generate this one... */
			if ((rnd -= ent->percent_pattern[j][warea]) > 100) {
				/* If it would be 0%, don't bother... */
				if (j == 2) {
					break;
				}

				/* Lets see what type we'll generate now... */
				rnd = sfmt_genrand_uint32(rng) % 100;
				for (k = 0; k < 6; ++k) {
					if ((rnd -= ent->percent_attachment[k][area]) > 100) {
						if (k == 0 || (upcts & (1 << k)))
							break;

						j = (j - 2) * 5;
						item_b[(npcts << 1) + 6] = k;
						item_b[(npcts << 1) + 7] = (uint8_t)j;
						++npcts;
						upcts |= 1 << k;
						break;
					}
				}

				break;
			}
		}
	}

	/* Finally, lets see if there's going to be an elemental attribute applied
	   to this weapon, or if its rare and we need to set the flag. */
	if (!semirare && ent->element_ranking[area]) {
		rnd = sfmt_genrand_uint32(rng) % 100;
		if (rnd < ent->element_probability[area]) {
			rnd = sfmt_genrand_uint32(rng) %
				attr_count[ent->element_ranking[area] - 1];
			item[1] = 0x80 | attr_list[ent->element_ranking[area] - 1][rnd];
		}
	}
	else if (rare || (v1 && semirare)) {
		item[1] = 0x80;
	}

	return 0;
}

static int generate_weapon_v3(pt_v3_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked, int bb,
	lobby_t* l) {
	uint32_t rnd, upcts = 0, wchance = 0;
	int i, j = 0, k, warea = 0, npcts = 0;
	int wtypes[12] = { 0 }, wranks[12] = { 0 }, gptrn[12] = { 0 };
	uint8_t* item_b = (uint8_t*)item;
	int semirare = 0, rare = 0;

	/* Ugly... but I'm lazy and don't feel like rebalancing things for another
	   level of indentation... */
	if (picked) {
		/* Determine if the item is a rare or a "semi-rare" item. */
		if (item_b[1] > 12)
			rare = semirare = 1;
		else if ((item_b[1] >= 10 && item_b[2] > 3) || item_b[2] > 4)
			semirare = 1;

		/* All RT-generated items use the last grind pattern... */
		warea = 3;
		item[1] = item[2] = item[3] = 0;
		goto already_picked;
	}

	item[0] = item[1] = item[2] = item[3] = 0;

	/* Go through each weapon type to see what ones we actually will have to
	   work with right now... */
	for (i = 0; i < 12; ++i) {
		if ((ent->weapon_minrank[i] + area) >= 0 && ent->weapon_ratio[i] > 0) {
			wtypes[j] = i;
			wchance += ent->weapon_ratio[i];

			if (ent->weapon_minrank[i] >= 0) {
				warea = area;
				wranks[j] = ent->weapon_minrank[i];
			}
			else {
				warea = ent->weapon_minrank[i] + area;
				wranks[j] = 0;
			}

			/* 合理性检查... Make sure this is sane before we go to the loop
			   below, since it will end up being an infinite loop if its not
			   sane... */
			if (ent->weapon_upgfloor[i] <= 0) {
				ITEM_LOG("Invalid v3 weapon upgrade floor value for "
					"floor %d, weapon type %d. 请检查您的 ItemPT.gsl "
					"file (%s) 是否有效!", area, i, bb ? "BB" : "GC");
				return -1;
			}

			while ((warea - ent->weapon_upgfloor[i]) >= 0) {
				++wranks[j];
				warea -= ent->weapon_upgfloor[i];
			}

			gptrn[j] = min(warea, 3);
			++j;
		}
	}

	/* 合理性检查... This shouldn't happen! */
	if (!j) {
		ITEM_LOG("No v3 weapon to generate on floor %d, please check "
			"your ItemPT.gsl 文件 (%s) 是否有效!", area,
			bb ? "BB" : "GC");
		return -1;
	}

	/* Roll the dice! */
	rnd = sfmt_genrand_uint32(rng) % wchance;
	for (i = 0; i < j; ++i) {
		if ((rnd -= ent->weapon_ratio[wtypes[i]]) > wchance) {
			item[0] = ((wtypes[i] + 1) << 8) | (wranks[i] << 16);

			/* Save off the grind pattern to use... */
			warea = gptrn[i];
			break;
		}
	}

	/* 合理性检查... Once again, this shouldn't happen! */
	if (!item[0]) {
		ITEM_LOG("生成无效 v3 武器. 请通知程序员处理!");
		return -1;
	}

	/* See if we made a "semi-rare" item. */
	if ((item_b[1] >= 10 && item_b[2] > 3) || item_b[2] > 4)
		semirare = 1;

already_picked:
	/* Next up, determine the grind value. */
	rnd = sfmt_genrand_uint32(rng) % 100;
	for (i = 0; i < 9; ++i) {
		if ((rnd -= ent->power_pattern[i][warea]) > 100) {
			item[0] |= (i << 24);
			break;
		}
	}

	/* 合理性检查... */
	if (i >= 9) {
		ITEM_LOG("Invalid power pattern for floor %d, pattern "
			"number %d. 请检查您的 ItemPT.gsl (%s) 是否有效!",
			area, warea, bb ? "BB" : "GC");
		return -1;
	}

	/* Let's generate us some percentages, shall we? This isn't necessarily the
	   way I would have designed this, but based on the way the data is laid
	   out in the PT file, this is the implied structure of it... */
	for (i = 0; i < 3; ++i) {
		if (ent->area_pattern[i][area] < 0)
			continue;

		rnd = sfmt_genrand_uint32(rng) % 10000;
		warea = ent->area_pattern[i][area];

		for (j = 0; j < 23; ++j) {
			/* See if we're going to generate this one... */
			if ((rnd -= ent->percent_pattern[j][warea]) > 10000) {
				/* If it would be 0%, don't bother... */
				if (j == 2) {
					break;
				}

				/* Lets see what type we'll generate now... */
				rnd = sfmt_genrand_uint32(rng) % 100;
				for (k = 0; k < 6; ++k) {
					if ((rnd -= ent->percent_attachment[k][area]) > 100) {
						if (k == 0 || (upcts & (1 << k)))
							break;

						j = (j - 2) * 5;
						item_b[(npcts << 1) + 6] = k;
						item_b[(npcts << 1) + 7] = (uint8_t)j;
						++npcts;
						upcts |= 1 << k;
						break;
					}
				}

				break;
			}
		}
	}

	/* Finally, lets see if there's going to be an elemental attribute applied
	   to this weapon, or if its rare and we need to set the flag. */
	if (!semirare && ent->element_ranking[area]) {
		rnd = sfmt_genrand_uint32(rng) % 100;
		if (rnd < ent->element_probability[area]) {
			rnd = sfmt_genrand_uint32(rng) %
				attr_count[ent->element_ranking[area] - 1];
			item[1] = 0x80 | attr_list[ent->element_ranking[area] - 1][rnd];
		}
	}
	else if (rare) {
		item[1] = 0x80;
	}

	return 0;
}

static int generate_weapon_bb(pt_bb_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked,
	lobby_t* l) {
	uint32_t rnd, upcts = 0, wchance = 0;
	int i, j = 0, k, warea = 0, npcts = 0;
	int wtypes[12] = { 0 }, wranks[12] = { 0 }, gptrn[12] = { 0 };
	uint8_t* item_b = (uint8_t*)item;
	int semirare = 0, rare = 0;

	/* Ugly... but I'm lazy and don't feel like rebalancing things for another
	   level of indentation...
	   丑陋…但我很懒，不想为了另一个层次的缩进而重新平衡事物。。。*/
	if (picked) {
		/* Determine if the item is a rare or a "semi-rare" item. */
		if (item_b[1] > 12)
			rare = semirare = 1;
		else if ((item_b[1] >= 10 && item_b[2] > 3) || item_b[2] > 4)
			semirare = 1;

		/* All RT-generated items use the last grind pattern...
		所有RT生成的项目都使用最后的研磨模式*/
		warea = 3;
		item[1] = item[2] = item[3] = 0;
		goto already_picked;
	}

	item[0] = item[1] = item[2] = item[3] = 0;

	/* Go through each weapon type to see what ones we actually will have to
	   work with right now...
	   检查每种武器类型，看看我们现在实际需要使用哪些武器*/
	for (i = 0; i < 12; ++i) {
		if ((ent->subtype_base_table[i] + area) >= 0 && ent->base_weapon_type_prob_table[i] > 0) {
			wtypes[j] = i;
			wchance += ent->base_weapon_type_prob_table[i];

			if (ent->subtype_base_table[i] >= 0) {
				warea = area;
				wranks[j] = ent->subtype_base_table[i];
			}
			else {
				warea = ent->subtype_base_table[i] + area;
				wranks[j] = 0;
			}

			/* 合理性检查... Make sure this is sane before we go to the loop
			   below, since it will end up being an infinite loop if its not
			   sane... */
			if (ent->subtype_area_length_table[i] <= 0) {
				ITEM_LOG("无效 BB 武器 upgrade floor value for "
					"floor %d, weapon type %d. 请检查您的 ItemPT.gsl "
					"文件 (BB) 是否有效!", area, i);
				return -1;
			}

			while ((warea - ent->subtype_area_length_table[i]) >= 0) {
				++wranks[j];
				warea -= ent->subtype_area_length_table[i];
			}

			gptrn[j] = min(warea, 3);
			++j;
		}
	}

	/* 合理性检查... This shouldn't happen! */
	if (!j) {
		ITEM_LOG("层级 %d 未生成 BB 武器物品, 请检查 ItemPT.gsl 文件 (BB) 有效性!", area);
		return -1;
	}

	/* Roll the dice! */
	rnd = sfmt_genrand_uint32(rng) % wchance;
	for (i = 0; i < j; ++i) {
		if ((rnd -= ent->base_weapon_type_prob_table[wtypes[i]]) > wchance) {
			item[0] = ((wtypes[i] + 1) << 8) | (wranks[i] << 16);

			/* Save off the grind pattern to use... */
			warea = gptrn[i];
			break;
		}
	}

	/* 合理性检查... Once again, this shouldn't happen! */
	if (!item[0]) {
		ITEM_LOG("生成无效 BB 武器. 请通知程序员处理!");
		return -1;
	}

	//if (l->drop_rare) {
	//	if (item_b[1] < 10) {
	//		// 生成随机值
	//		int min = 0x0A;
	//		int max = 0xED;

	//		item_b[1] = (sfmt_genrand_uint32(rng) % (max - min + 1)) + min;
	//	}

	//	if (item_b[2] <= 4)
	//		item_b[2] = 5;
	//}

	/* See if we made a "semi-rare" item. */
	if ((item_b[1] >= 10 && item_b[2] > 3) || item_b[2] > 4 || l->drop_rare)
		semirare = 1;

already_picked:
	/* Next up, determine the grind value. */
	rnd = sfmt_genrand_uint32(rng) % 100;
	for (i = 0; i < 9; ++i) {
		if ((rnd -= ent->grind_prob_tables[i][warea]) > 100) {
			item[0] |= (i << 24);
			break;
		}
	}

	/* 合理性检查... */
	if (i >= 9) {
		ITEM_LOG("Invalid power pattern for floor %d, pattern "
			"number %d. 请检查您的 ItemPT.gsl (BB) 是否有效!",
			area, warea);
		return -1;
	}

	/* Let's generate us some percentages, shall we? This isn't necessarily the
	   way I would have designed this, but based on the way the data is laid
	   out in the PT file, this is the implied structure of it...
	   让我们产生一些百分比，好吗？这不一定是我设计它的方式，但根据PT文件中数据的布局方式，这是它的隐含结构*/
	for (i = 0; i < 3; ++i) {
		if (ent->nonrare_bonus_prob_spec[i][area] < 0)
			continue;

		rnd = sfmt_genrand_uint32(rng) % 10000;
		warea = ent->nonrare_bonus_prob_spec[i][area];

		for (j = 0; j < 23; ++j) {
			/* See if we're going to generate this one... */
			if ((rnd -= ent->bonus_value_prob_tables[j][warea]) > 10000) {
				/* If it would be 0%, don't bother... */
				if (j == 2) {
					break;
				}

				/* Lets see what type we'll generate now... */
				rnd = sfmt_genrand_uint32(rng) % 100;
				for (k = 0; k < 6; ++k) {
					if ((rnd -= ent->bonus_type_prob_tables[k][area]) > 100) {
						if (k == 0 || (upcts & (1 << k)))
							break;

						j = (j - 2) * 5;
						item_b[(npcts << 1) + 6] = k;
						item_b[(npcts << 1) + 7] = (uint8_t)j;
						++npcts;
						upcts |= 1 << k;
						break;
					}
				}

				break;
			}
		}
	}

	/* Finally, lets see if there's going to be an elemental attribute applied
	   to this weapon, or if its rare and we need to set the flag. */
	if (!semirare && ent->special_mult[area]) {
		rnd = sfmt_genrand_uint32(rng) % 100;
		if (rnd < ent->special_percent[area]) {
			rnd = sfmt_genrand_uint32(rng) %
				attr_count[ent->special_mult[area] - 1];
			item[1] = 0x80 | attr_list[ent->special_mult[area] - 1][rnd];
		}
	}
	else if (rare) {
		item[1] = 0x80;
	}

	return 0;
}

/*
   Generate a random armor, based on data for PSOv2. Luckily, this is a lot
   simpler than the case for weapons, so it needs a lot less explanation.

   Each floor has 5 "virtual slots" for different armor types that can be
   dropped on the floor. The armors that are in these slots depends on two
   things: the index of the floor (0-9) and the armor level value in the ItemPT
   file. The basic pattern for a slot is as follows (for i in [0, 4]):
	 slot[i] = MAX(0, armor_level - 3 + floor + i)
   So, for example, using a Whitill Section ID character on Normal difficulty
   (where the armor level = 0), the slots for Forest 1 would be filled as
   follows: { 0, 0, 0, 0, 1 }. The first three slots all generate negative
   values which get clipped off to 0 by the MAX operator above.

   The values in the slots correspond to the low-order byte of the high-order
   word of the first dword of the item data (or the 3rd byte of the item data,
   in other words). The low-order word of the first dword is always 0x0101, to
   say that it is a frame/armor.

   The probability of getting each of the armors above is controlled by the
   armor ranking array in the ItemPT data. This is just a list of probabilities
   (out of 100) for each slot, much like what is done with the weapon data. I
   won't bother to explain how to pick random numbers here again.

   Each armor has up to 4 unit slots in it. The amount of unit slots is
   determined by looking at the probabilities in the slot ranking array in the
   ItemPT data. Once again, this is a list of probabilities out of 100. The
   index you end up in determines how many unit slots you get, from 0 to 4.

   Random DFP and EVP boosts require data from the ItemPMT.prs file (see the
   pmtdata.[ch] files for more information about that). Basically, they're
   handled by generating a random number in [0, max] where max is the dfp or
   evp range defined in the PMT data.
*/
static int generate_armor_v2(pt_v2_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked,
	lobby_t* l) {
	uint32_t rnd;
	int i, armor = -1;
	uint8_t* item_b = (uint8_t*)item;
	uint16_t* item_w = (uint16_t*)item;
	pmt_guard_v2_t guard;

	if (!picked) {
		/* Go through each slot in the armor rankings to figure out which one
		   that we'll be generating. */
		rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("generate_armor_v2: RNG picked %" PRIu32 "", rnd);
#endif

		for (i = 0; i < 5; ++i) {
#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
					PRIu32 "", i, ent->armor_ranking[i], rnd);
#endif

			if ((rnd -= ent->armor_ranking[i]) > 100) {
				armor = i;
				break;
			}
		}

		/* 合理性检查... */
		if (armor == -1) {
			ITEM_LOG("无法生成 v2 版本的 装甲. 请检查您的 ItemPT.afs 文件是否有效!");
			return -1;
		}

		/* Figure out what the byte we'll use is */
		armor = ((int)ent->armor_level) - 3 + area + armor;

		if (armor < 0)
			armor = 0;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Armor level byte: %d (%d + %d + %d - 3)",
				armor, (int)ent->armor_level, area, i);
#endif

		item[0] = 0x00000101 | (armor << 16);
	}

	item[1] = item[2] = item[3] = 0;

	/* Pick a number of unit slots */
	rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_armor_v2: RNG picked %" PRIu32 " for slots",
			rnd);
#endif

	for (i = 0; i < 5; ++i) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
				PRIu32 "", i, ent->slot_ranking[i], rnd);
#endif

		if ((rnd -= ent->slot_ranking[i]) > 100) {
			item_b[5] = i;
			break;
		}
	}

	/* Look up the item in the ItemPMT data so we can see what boosts we might
	   apply... */
	if (pmt_lookup_guard_v2(item[0], &guard)) {
		ITEM_LOG("ItemPMT.prs 文件版本 v2 似乎缺少了一件 装甲 类型的物品数据 (item[0] 代码 %08X).", item[0]);
		return -2;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_armor_v2: DFP 范围: %d, EVP 范围: %d",
			guard.dfp_range, guard.evp_range);
#endif

	if (guard.dfp_range) {
		rnd = sfmt_genrand_uint32(rng) % (guard.dfp_range + 1);
		item_w[3] = (uint16_t)rnd;
	}

	if (guard.evp_range) {
		rnd = sfmt_genrand_uint32(rng) % (guard.evp_range + 1);
		item_w[4] = (uint16_t)rnd;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("    DFP: %" PRIu16 ", EVP: %" PRIu16 "", item_w[3],
			item_w[4]);
#endif

	return 0;
}

static int generate_armor_v3(pt_v3_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked, int bb,
	lobby_t* l) {
	uint32_t rnd;
	int i, armor = -1;
	uint8_t* item_b = (uint8_t*)item;
	uint16_t* item_w = (uint16_t*)item;
	pmt_guard_gc_t gcg;
	pmt_guard_bb_t bbg;
	uint8_t dfp, evp;

	if (!picked) {
		/* Go through each slot in the armor rankings to figure out which one
		   that we'll be generating. */
		rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("generate_armor_v3: RNG picked %" PRIu32 "", rnd);
#endif

		for (i = 0; i < 5; ++i) {
#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
					PRIu32 "", i, ent->armor_ranking[i], rnd);
#endif

			if ((rnd -= ent->armor_ranking[i]) > 100) {
				armor = i;
				break;
			}
		}

		/* 合理性检查... */
		if (armor == -1) {
			ITEM_LOG("无法生成 %s 版本的 装甲. 请检查您的 ItemPT.gsl 文件是否有效!",
				bb ? "BB" : "GC");
			return -1;
		}

		/* Figure out what the byte we'll use is */
		armor = (ent->armor_level) - 3 + area + armor;

		if (armor < 0)
			armor = 0;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Armor level byte: %d (%d + %d + %d - 3)",
				armor, (int)ent->armor_level, area, i);
#endif


		//这行代码将item列表中的第一个元素设置为一个位运算的结果。
		//首先，0x00000101是一个十六进制数，表示为二进制形式为00000000 00000000 00000001 00000001。
		//然后，将armor的值左移16位，并将结果与0x00000101进行按位或运算。
		//最后，将结果赋值给item列表中的第一个元素。
		item[0] = 0x00000101 | (armor << 16);
	}

	item[1] = item[2] = item[3] = 0;

	/* Pick a number of unit slots */
	rnd = sfmt_genrand_uint32(rng) % 100;
#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_armor_v3: RNG picked %" PRIu32 " for slots",
			rnd);
#endif

	for (i = 0; i < 5; ++i) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
				PRIu32 "", i, ent->slot_ranking[i], rnd);
#endif

		if ((rnd -= ent->slot_ranking[i]) > 100) {
			item_b[5] = i;
			break;
		}
	}

	/* Look up the item in the ItemPMT data so we can see what boosts we might
	   apply... */
	if (!bb) {
		if (pmt_lookup_guard_gc(item[0], &gcg)) {
			ITEM_LOG("ItemPMT.prs 文件版本 GC 似乎缺少了一件 装甲 类型的物品数据 (item[0] 代码 %08X).", item[0]);
			return -2;
		}

		dfp = gcg.dfp_range;
		evp = gcg.evp_range;
	}
	else {
		if (pmt_lookup_guard_bb(item[0], &bbg)) {
			ITEM_LOG("ItemPMT.prs 文件版本 BB 似乎缺少了一件 装甲 类型的物品数据 (item[0] 代码 %08X).", item[0]);
			return -2;
		}

		dfp = bbg.dfp_range;
		evp = bbg.evp_range;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_amor_v3: DFP 范围: %d, EVP 范围: %d",
			dfp, evp);
#endif

	if (dfp) {
		rnd = sfmt_genrand_uint32(rng) % (dfp + 1);
		item_w[3] = (uint16_t)rnd;
	}

	if (evp) {
		rnd = sfmt_genrand_uint32(rng) % (evp + 1);
		item_w[4] = (uint16_t)rnd;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("    DFP: %" PRIu16 ", EVP: %" PRIu16 "", item_w[3],
			item_w[4]);
#endif

	return 0;
}

static int generate_armor_bb(pt_bb_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked,
	lobby_t* l) {
	uint32_t rnd;
	int i, armor = -1;
	uint8_t* item_b = (uint8_t*)item;
	uint16_t* item_w = (uint16_t*)item;
	pmt_guard_bb_t bbg;
	uint8_t dfp, evp;

	if (!picked) {
		/* Go through each slot in the armor rankings to figure out which one
		   that we'll be generating. */
		rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("generate_armor_v3: RNG picked %" PRIu32 "", rnd);
#endif

		for (i = 0; i < 5; ++i) {
#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
					PRIu32 "", i, ent->armor_ranking[i], rnd);
#endif

			if ((rnd -= ent->armor_shield_type_index_prob_table[i]) > 100) {
				armor = i;
				break;
			}
		}

		/* 合理性检查... */
		if (armor == -1) {
			ITEM_LOG("无法生成 BB 版本的 装甲. 请检查您的 ItemPT.gsl 文件是否有效!");
			return -1;
		}

		/* Figure out what the byte we'll use is */
		armor = (ent->armor_level) - 3 + area + armor;

		if (armor < 0 || armor > 0x57)
			armor = 0;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Armor level byte: %d (%d + %d + %d - 3)",
				armor, (int)ent->armor_level, area, i);
#endif

		item[0] = 0x00000101 | (armor << 16);
	}

	item[1] = item[2] = item[3] = 0;

	/* Pick a number of unit slots */
	rnd = sfmt_genrand_uint32(rng) % 100;
#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_armor_bb: RNG picked %" PRIu32 " for slots",
			rnd);
#endif

	for (i = 0; i < 5; ++i) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
				PRIu32 "", i, ent->slot_ranking[i], rnd);
#endif

		if ((rnd -= ent->armor_slot_count_prob_table[i]) > 100) {
			item_b[5] = i;
			break;
		}
	}

	/* Look up the item in the ItemPMT data so we can see what boosts we might
	   apply... */
	if (pmt_lookup_guard_bb(item[0], &bbg)) {
		ITEM_LOG("ItemPMT.prs 文件版本 BB 似乎缺少了一件 装甲 类型的物品数据 (item[0] 代码 %08X).", item[0]);
		return -2;
	}

	dfp = bbg.dfp_range;
	evp = bbg.evp_range;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_amor_v3: DFP 范围: %d, EVP 范围: %d",
			dfp, evp);
#endif

	if (dfp) {
		rnd = sfmt_genrand_uint32(rng) % (dfp + 1);
		item_w[3] = (uint16_t)rnd;
	}

	if (evp) {
		rnd = sfmt_genrand_uint32(rng) % (evp + 1);
		item_w[4] = (uint16_t)rnd;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("    DFP: %" PRIu16 ", EVP: %" PRIu16 "", item_w[3],
			item_w[4]);
#endif

	return 0;
}

/* Generate a random shield, based on data for PSOv2. This is exactly the same
   as the armor version, but without unit slots. */
static int generate_shield_v2(pt_v2_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked,
	lobby_t* l) {
	uint32_t rnd;
	int i, armor = -1;
	uint16_t* item_w = (uint16_t*)item;
	pmt_guard_v2_t guard;

	if (!picked) {
		/* Go through each slot in the armor rankings to figure out which one
		   that we'll be generating. */
		rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("generate_shield_v2: RNG picked %" PRIu32 "", rnd);
#endif

		for (i = 0; i < 5; ++i) {
#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
					PRIu32 "", i, ent->armor_ranking[i], rnd);
#endif

			if ((rnd -= ent->armor_ranking[i]) > 100) {
				armor = i;
				break;
			}
		}

		/* 合理性检查... */
		if (armor == -1) {
			ITEM_LOG("无法生成 v2 版本的 护盾. 请检查您的 ItemPT.afs 文件是否有效!");
			return -1;
		}

		/* Figure out what the byte we'll use is */
		armor = (ent->armor_level) - 3 + area + armor;

		if (armor < 0)
			armor = 0;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Armor level byte: %d (%d + %d + %d - 3)",
				armor, (int)ent->armor_level, area, i);
#endif

		item[0] = 0x00000201 | (armor << 16);
	}

	item[1] = item[2] = item[3] = 0;

	/* Look up the item in the ItemPMT data so we can see what boosts we might
	   apply... */
	if (pmt_lookup_guard_v2(item[0], &guard)) {
		ITEM_LOG("ItemPMT.prs 文件版本 v2 似乎缺少了一件 护盾 类型的物品数据 (item[0] 代码 %08X).", item[0]);
		return -2;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_shield_v2: DFP 范围: %d, EVP 范围: %d",
			guard.dfp_range, guard.evp_range);
#endif

	if (guard.dfp_range) {
		rnd = sfmt_genrand_uint32(rng) % (guard.dfp_range + 1);
		item_w[3] = (uint16_t)rnd;
	}

	if (guard.evp_range) {
		rnd = sfmt_genrand_uint32(rng) % (guard.evp_range + 1);
		item_w[4] = (uint16_t)rnd;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("    DFP: %" PRIu16 ", EVP: %" PRIu16 "", item_w[3],
			item_w[4]);
#endif

	return 0;
}

static int generate_shield_v3(pt_v3_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked, int bb,
	lobby_t* l) {
	uint32_t rnd;
	int i, armor = -1;
	uint16_t* item_w = (uint16_t*)item;
	pmt_guard_gc_t gcg;
	pmt_guard_bb_t bbg;
	uint8_t dfp, evp;

	if (!picked) {
		/* Go through each slot in the armor rankings to figure out which one
		   that we'll be generating. */
		rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("generate_shield_v3: RNG picked %" PRIu32 "", rnd);
#endif

		for (i = 0; i < 5; ++i) {
#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
					PRIu32 "", i, ent->armor_ranking[i], rnd);
#endif

			if ((rnd -= ent->armor_ranking[i]) > 100) {
				armor = i;
				break;
			}
		}

		/* 合理性检查... */
		if (armor == -1) {
			ITEM_LOG("Couldn't find a %s shield to generate. Please "
				"check your ItemPT.gsl 文件 是否有效!",
				bb ? "BB" : "GC");
			return -1;
		}

		/* Figure out what the byte we'll use is */
		armor = (ent->armor_level) - 3 + area + armor;

		if (armor < 0)
			armor = 0;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Armor level byte: %d (%d + %d + %d - 3)",
				armor, (int)ent->armor_level, area, i);
#endif

		item[0] = 0x00000201 | (armor << 16);
	}

	item[1] = item[2] = item[3] = 0;

	/* Look up the item in the ItemPMT data so we can see what boosts we might
	   apply... */
	if (!bb) {
		if (pmt_lookup_guard_gc(item[0], &gcg)) {
			ITEM_LOG("ItemPMT.prs 文件版本 GC 似乎缺少了一件 护盾 类型的物品数据 (item[0] 代码 %08X).", item[0]);
			return -2;
		}

		dfp = gcg.dfp_range;
		evp = gcg.evp_range;
	}
	else {
		if (pmt_lookup_guard_bb(item[0], &bbg)) {
			ITEM_LOG("ItemPMT.prs 文件版本 BB 似乎缺少了一件 护盾 类型的物品数据 (item[0] 代码 %08X).", item[0]);
			return -2;
		}

		dfp = bbg.dfp_range;
		evp = bbg.evp_range;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_shield_v3: DFP 范围: %d, EVP 范围: %d",
			dfp, evp);
#endif

	if (dfp) {
		rnd = sfmt_genrand_uint32(rng) % (dfp + 1);
		item_w[3] = (uint16_t)rnd;
	}

	if (evp) {
		rnd = sfmt_genrand_uint32(rng) % (evp + 1);
		item_w[4] = (uint16_t)rnd;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("    DFP: %" PRIu16 ", EVP: %" PRIu16 "", item_w[3],
			item_w[4]);
#endif

	return 0;
}

static int generate_shield_bb(pt_bb_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, int picked,
	lobby_t* l) {
	uint32_t rnd;
	int i, armor = -1;
	uint16_t* item_w = (uint16_t*)item;
	pmt_guard_bb_t bbg;
	uint8_t dfp, evp;
	errno_t err = 0;

	if (!picked) {
		/* Go through each slot in the armor rankings to figure out which one
		   that we'll be generating. */
		rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("generate_shield_v3: RNG picked %" PRIu32 "", rnd);
#endif

		for (i = 0; i < 5; ++i) {
#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Index %d, value: %" PRId8 ", left: %"
					PRIu32 "", i, ent->armor_ranking[i], rnd);
#endif

			if ((rnd -= ent->armor_shield_type_index_prob_table[i]) > 100) {
				armor = i;
				break;
			}
		}

		/* 合理性检查... */
		if (armor == -1) {
			ITEM_LOG("无法找到可生成的 BB 护盾. 请"
				"检查你的 ItemPT.gsl 文件是否有效!");
			return -1;
		}

		/* Figure out what the byte we'll use is */
		armor = (ent->armor_level) - 3 + area + armor;

		if (armor < 0 || armor > 0xA4)
			armor = 0;

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    Armor level byte: %d (%d + %d + %d - 3)",
				armor, (int)ent->armor_level, area, i);
#endif

		item[0] = 0x00000201 | (armor << 16);
	}

	item[1] = item[2] = item[3] = 0;

	/* Look up the item in the ItemPMT data so we can see what boosts we might
	   apply... */
	err = pmt_lookup_guard_bb(item[0], &bbg);

	if (err) {
		ITEM_LOG("ItemPMT.prs 文件版本 BB 似乎缺少了一件 护盾 类型的物品数据 (item[0] 代码 %08X 错误码 %d).", item[0], err);
		return -2;
	}

	dfp = bbg.dfp_range;
	evp = bbg.evp_range;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_shield_v3: DFP 范围: %d, EVP 范围: %d",
			dfp, evp);
#endif

	if (dfp) {
		rnd = sfmt_genrand_uint32(rng) % (dfp + 1);
		item_w[3] = (uint16_t)rnd;
	}

	if (evp) {
		rnd = sfmt_genrand_uint32(rng) % (evp + 1);
		item_w[4] = (uint16_t)rnd;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("    DFP: %" PRIu16 ", EVP: %" PRIu16 "", item_w[3],
			item_w[4]);
#endif

	return 0;
}

static uint32_t generate_tool_base(uint16_t freqs[28][10], int area,
	sfmt_t* rng, lobby_t* l) {
	uint32_t rnd = sfmt_genrand_uint32(rng) % 10000;
	int i;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS) {
		ITEM_LOG("generate_tool_base: RNG picked %" PRIu32 "", rnd);
	}
#endif

	for (i = 0; i < 28; ++i) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    index: %d, code %08" PRIx32 " chance: %" PRIu16
				" left: %" PRIu32 "", i, tool_base[i], LE16(freqs[i][area]),
				rnd);
#endif
		if ((rnd -= LE16(freqs[i][area])) > 10000) {
#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Generating item.");
#endif

			return tool_base[i];
		}
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("    Can't find item to drop! This shouldn't happen.");
#endif

	return Item_NoSuchItem;
}

/* XXXX: There's something afoot here generating invalid techs. */
static int generate_tech(uint8_t freqs[19][10], int8_t levels[19][20],
	int area, uint32_t item[4],
	sfmt_t* rng, lobby_t* l) {
	uint32_t rnd, tech, level;
	int8_t t1, t2;
	int i;

	rnd = sfmt_genrand_uint32(rng);
	tech = rnd % 1000;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("generate_tech: RNG generated %" PRIu32 " tech part: %"
			PRIu32 "", rnd, tech);
#endif

	rnd /= 1000;

	for (i = 0; i < MAX_PLAYER_TECHNIQUES; ++i) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("    index: %d, chance: %" PRIu8 " left: %" PRIu32
				"", i, freqs[i][area], tech);
#endif

		if ((tech -= freqs[i][area]) > 1000) {
#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Generating tech.");
#endif

			t1 = levels[i][area << 1];
			t2 = levels[i][(area << 1) + 1];

#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Min: %" PRId8 " Max: " PRId8 "", t1, t2);
#endif

			/* Make sure that the minimum level isn't -1 and that the minimum is
			   actually less than the maximum. */
			if (t1 == -1 || t1 > t2) {
				ITEM_LOG("Invalid tech level set for area %d, tech %d",
					area, i);
				return -1;
			}

			/* Cap the levels from the ItemPT data, since Sega's files sometimes
			   have stupid values here. */
			if (t1 >= 30)
				t1 = 29;

			if (t2 >= 30)
				t2 = 29;

			if (t1 < t2)
				level = (rnd % ((t2 + 1) - t1)) + t1;
			else
				level = t1;

#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("    Level selected: %" PRIu32 "", level);
#endif

			item[1] = i;
			item[0] |= (level << 16);
			return 0;
		}
	}

	/* Shouldn't get here... */
	ITEM_LOG("Couldn't find technique to drop for area %d!", area);
	return -1;
}

static int generate_tool_v2(pt_v2_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, lobby_t* l) {
	item[0] = generate_tool_base(ent->tool_frequency, area, rng, l);

	/* Neither of these should happen, but just in case... */
	if (item[0] == Item_Photon_Drop || item[0] == Item_NoSuchItem) {
		ITEM_LOG("生成无效 v2 tool! Please check your "
			"ItemPT.afs 文件 是否有效!");
		return -1;
	}

	/* Clear the rest of the item. */
	item[1] = item[2] = item[3] = 0;

	item_t tmp_item = { 0 };
	if (!create_tmp_item(item, ARRAYSIZE(item), &tmp_item)) {
		ERR_LOG("预设物品参数失败");
		return 0;
	}

	/* If its a stackable item, make sure to give it a quantity of 1 */
	if (is_stackable(&tmp_item)) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Item is stackable. Setting quantity to 1.");
#endif

		item[1] = (1 << 8);
	}

	if (item[0] == Item_Disk_Lv01) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Item is technique disk. Picking technique.");
#endif

		if (generate_tech(ent->tech_frequency, ent->tech_levels, area,
			item, rng, l)) {
			ITEM_LOG("生成无效 technique! Please check "
				"your ItemPT.afs 文件 是否有效!");
			return -1;
		}
	}

	return 0;
}

static int generate_tool_v3(pt_v3_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, lobby_t* l) {
	item[0] = generate_tool_base(ent->tool_frequency, area, rng, l);

	/* This shouldn't happen happen, but just in case... */
	if (item[0] == Item_NoSuchItem) {
		ITEM_LOG("生成无效 v3 tool! Please check your "
			"ItemPT.gsl 文件 是否有效!");
		return -1;
	}

	/* Clear the rest of the item. */
	item[1] = item[2] = item[3] = 0;

	item_t tmp_item = { 0 };
	if (!create_tmp_item(item, ARRAYSIZE(item), &tmp_item)) {
		ERR_LOG("预设物品参数失败");
		return 0;
	}

	/* If its a stackable item, make sure to give it a quantity of 1 */
	if (is_stackable(&tmp_item)) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Item is stackable. Setting quantity to 1.");
#endif

		item[1] = (1 << 8);
	}

	if (item[0] == Item_Disk_Lv01) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Item is technique disk. Picking technique.");
#endif

		if (generate_tech(ent->tech_frequency, ent->tech_levels, area,
			item, rng, l)) {
			ITEM_LOG("生成无效 technique! Please check "
				"your ItemPT.gsl 文件 是否有效!");
			return -1;
		}
	}

	return 0;
}

static int generate_tool_bb(pt_bb_entry_t* ent, int area, uint32_t item[4],
	sfmt_t* rng, lobby_t* l) {
	item[0] = generate_tool_base(ent->tool_class_prob_table, area, rng, l);

	/* This shouldn't happen happen, but just in case... */
	if (item[0] == Item_NoSuchItem) {
		ITEM_LOG("生成无效 BB 工具! 请检查你的 "
			"ItemPT.gsl 文件是否有效! 0x%08X ", item[0]);
		return -1;
	}

	/* 如果生成无效物品 则默认无掉落 */
	if (item_not_identification_bb(item[0], item[1])) {
		//ITEM_LOG("生成无效 BB 工具! 请检查你的 "
		//	"ItemPT.gsl 文件是否有效! 0x%08X ", item[0]);
		return -2;
	}

	/* Clear the rest of the item. */
	item[1] = item[2] = item[3] = 0;

	item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

	/* If its a stackable item, make sure to give it a quantity of 1 */
	if (is_stackable(&tmp_item)) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Item is stackable. Setting quantity to 1.");
#endif

		item[1] = (1 << 8);
	}

	if (item[0] == Item_Disk_Lv01) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Item is technique disk. Picking technique.");
#endif

		if (generate_tech(ent->technique_index_prob_table, ent->technique_level_ranges, area,
			item, rng, l)) {
			ITEM_LOG("生成无效 technique! Please check "
				"your ItemPT.gsl 文件 是否有效!");
			return -1;
		}
	}

	return 0;
}

static int generate_meseta(int min, int max, uint32_t item[4],
	sfmt_t* rng, lobby_t* l) {
	uint32_t rnd;

	if (min < max)
		rnd = (sfmt_genrand_uint32(rng) % ((max + 1) - min)) + min;
	else
		rnd = min;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS) {
		ITEM_LOG("generate_meseta: generated %" PRIu32 " meseta. (min, "
			" max) = (%d, %d)", rnd, min, max);
	}
#endif

	if (rnd) {
		item[0] = 0x00000004;
		item[1] = item[2] = 0x00000000;
		item[3] = rnd;
		return 0;
	}

	return -1;
}

static int check_and_send(ship_client_t* c, lobby_t* l, uint32_t item[4],
	int area, subcmd_itemreq_t* req, int csr) {
	uint32_t v;
	iitem_t iitem = { 0 };
	uint8_t section = get_lobby_leader_section(l);
	uint8_t stars = 0;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("Final dropped item: %08" PRIx32 " %08" PRIx32 " %08"
			PRIx32 " %08" PRIx32 "", item[0], item[1], item[2], item[3]);
#endif

	if (l->limits_list) {
		switch (c->version) {
		case CLIENT_VERSION_DCV1:
			v = ITEM_VERSION_V1;
			break;

		case CLIENT_VERSION_DCV2:
		case CLIENT_VERSION_PC:
			v = ITEM_VERSION_V2;
			break;

		case CLIENT_VERSION_GC:
		case CLIENT_VERSION_XBOX:
			v = ITEM_VERSION_GC;
			break;

		default:
			goto ok;
		}

		/* Fill in the item structure so we can check it. */
		iitem.data.datal[0] = LE32(item[0]);
		iitem.data.datal[1] = LE32(item[1]);
		iitem.data.datal[2] = LE32(item[2]);
		iitem.data.data2l = LE32(item[3]);

		if (!psocn_limits_check_item(l->limits_list, &iitem, v)) {
			ITEM_LOG("发现不合法服务器掉落\n"
				"%08X %08X %08X %08X\n"
				"游戏房间信息: 难度: %d, 角色颜色ID: %d, 房间标签: %08X\n"
				"版本: %d, 房间层数: %d (%d %d)", item[0], item[1], item[2],
				item[3], l->difficulty, section, l->flags, l->version, area,
				l->maps[(area << 1)], l->maps[(area << 1) + 1]);

			/* The item failed the check, so don't send it! */
			return 0;
		}
	}

	/* See it is cool to drop "semi-rare" items. */
	if (csr) {
		switch (l->version) {
		case CLIENT_VERSION_DCV1:
		case CLIENT_VERSION_DCV2:
		case CLIENT_VERSION_PC:
			stars = pmt_lookup_stars_v2(item[0]);
			break;

		case CLIENT_VERSION_GC:
		case CLIENT_VERSION_XBOX:
			stars = pmt_lookup_stars_gc(item[0]);
			break;
		}
	}

	if (stars != (uint8_t)-1 && stars >= 9) {
		/* We aren't supposed to drop rares, and this item qualifies
		   as one (according to Sega's rules), so don't drop it. */
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Item drop blocked by semi-rare restriction. Has %d "
				"stars.", stars);
#endif

		return 0;
	}

ok:
#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("Sending item drop to team.");
#endif

	return subcmd_send_lobby_item(l, req, item);
}

/* 大厅掉落发送数据包 */
static int check_and_send_bb_lobby(ship_client_t* dest, lobby_t* l, uint32_t item[4],
	int area, subcmd_bb_itemreq_t* req, int csr) {
	int rv;
	litem_t* lt;

	if (!l)
		return 0;

	/* See it is cool to drop "semi-rare" items. */
	if (csr) {
		if (pmt_lookup_stars_bb(item[0]) >= 9)
			/* We aren't supposed to drop rares, and this item qualifies
			   as one (according to Sega's rules), so don't drop it
			   我们不应该掉落这些稀有品，而且这件物品符合世嘉规则，所以不要掉落它. */
			return 0;
	}

	dest->new_item.datal[0] = item[0];
	dest->new_item.datal[1] = item[1];
	dest->new_item.datal[2] = item[2];
	dest->new_item.data2l = item[3];

	//print_item_data(&c->new_item, c->version);

	pthread_mutex_lock(&dest->mutex);
	lt = add_new_litem_locked(l, &dest->new_item, req->area, req->x, req->z);
	if (!lt)
		return 0;
	rv = subcmd_send_lobby_bb_drop_item(dest, NULL, req, &lt->iitem);
	pthread_mutex_unlock(&dest->mutex);

	return rv;
}

/* 单独掉落发送数据 */
static int check_and_send_bb(ship_client_t* dest, uint32_t item[4],
	int area, subcmd_bb_itemreq_t* req, int csr) {
	int rv;
	litem_t* lt;
	lobby_t* l = dest->cur_lobby;

	if (!l)
		return 0;

	/* See it is cool to drop "semi-rare" items. */
	if (csr) {
		if (pmt_lookup_stars_bb(item[0]) >= 9)
			/* We aren't supposed to drop rares, and this item qualifies
			   as one (according to Sega's rules), so don't drop it
			   我们不应该掉落这些稀有品，而且这件物品符合世嘉规则，所以不要掉落它. */
			return 0;
	}

	dest->new_item.datal[0] = item[0];
	dest->new_item.datal[1] = item[1];
	dest->new_item.datal[2] = item[2];
	dest->new_item.data2l = item[3];

	//print_item_data(&dest->new_item, dest->version);

	pthread_mutex_lock(&dest->mutex);
	lt = add_new_litem_locked(l, &dest->new_item, req->area, req->x, req->z);
	if (!lt)
		return 0;
	rv = subcmd_send_bb_drop_item(dest, req, &lt->iitem);
	pthread_mutex_unlock(&dest->mutex);

	return rv;
}

/* Generate an item drop from the PT data. This version uses the v2 PT data set,
   and thus is appropriate for any version before PSOGC. */
int pt_generate_v2_drop(ship_client_t* c, lobby_t* l, void* r) {
	subcmd_itemreq_t* req = (subcmd_itemreq_t*)r;
	uint8_t section = get_lobby_leader_section(l);
	pt_v2_entry_t* ent;
	uint32_t rnd;
	uint32_t item[4] = { 0 };
	int area, rarea, do_rare = 1;
	sfmt_t* rng = &c->cur_block->sfmt_rng;
	uint16_t mid;
	game_enemy_t* enemy;
	int csr = 0;
	uint32_t qdrop = 0xFFFFFFFF;

	/* Make sure the PT index in the packet is sane */
	if (req->pt_index > 0x33)
		return -1;

	/* If the PT index is 0x30, this is a box, not an enemy! */
	if (req->pt_index == 0x30)
		return pt_generate_v2_boxdrop(c, l, r);

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ent = &v2_ptdata[l->sdrops_diff][l->sdrops_section];
	else
#endif
		ent = &v2_ptdata[l->difficulty][section];

	/* Figure out the area we'll be worried with */
	rarea = area = c->cur_area;

	/* Dragon -> Cave 1 */
	if (area == 11)
		area = 3;
	/* De Rol Le -> Mine 1 */
	else if (area == 12)
		area = 6;
	/* Vol Opt -> Ruins 1 */
	else if (area == 13)
		area = 8;
	/* Dark Falz -> Ruins 3 */
	else if (area == 14)
		area = 10;
	/* Everything after Dark Falz -> Ruins 3 */
	else if (area > 14)
		area = 10;
	/* Invalid areas... */
	else if (area == 0) {
		ITEM_LOG("GC %" PRIu32 " requested enemy drop on Pioneer "
			"2", c->guildcard);
		LOG(l, "GC %" PRIu32 " requested enemy drop on Pioneer 2\n",
			c->guildcard);
		return -1;
	}

	/* Subtract one, since we want the index in the box_drop array */
	--area;

	/* Make sure the enemy's id is sane... */
	mid = LE16(req->request_id);
	if (mid > l->map_enemies->enemy_count) {
#ifdef DEBUG
		ITEM_LOG("GC %" PRIu32 " requested v2 drop for invalid "
			"enemy (%d -- max: %d, quest=%" PRIu32 ")!", c->guildcard,
			mid, l->map_enemies->count, l->qid);
#endif

		LOG(l, "GC %" PRIu32 " requested v2 drop for invalid enemy (%d "
			"-- max: %d, quest=%" PRIu32 ")!\n", c->guildcard, mid,
			l->map_enemies->enemy_count, l->qid);
		return -1;
	}

	/* Grab the map enemy to make sure it hasn't already dropped something. */
	enemy = &l->map_enemies->enemies[mid];

	LOG(l, "GC %" PRIu32 " requested v2 drop...\n"
		"mid: %d (max: %d), pt: %d (%d), area: %d (%d), quest: %" PRIu32
		"section: %d, difficulty: %d\n",
		c->guildcard, mid, l->map_enemies->enemy_count, req->pt_index,
		enemy->rt_index, area + 1, rarea, l->qid, section, l->difficulty);

	if (enemy->drop_done) {
		LOGV(l, "Drop already done.\n");
		return 0;
	}

	enemy->drop_done = 1;

	/* See if the enemy is going to drop anything at all this time... */
	rnd = sfmt_genrand_uint32(rng) % 100;

	if ((int8_t)rnd >= ent->enemy_dar[req->pt_index]) {
		/* Nope. You get nothing! */
		LOGV(l, "DAR roll failed.\n");
		return 0;
	}

	/* See if we'll do a rare roll. */
	if (l->qid) {
		if (l->mids)
			qdrop = quest_search_enemy_list(mid, l->mids, l->num_mids, 1);
		if (qdrop == 0xFFFFFFFF && l->mtypes)
			qdrop = quest_search_enemy_list(req->pt_index, l->mtypes,
				l->num_mtypes, 1);

		switch (qdrop) {
		case PSOCN_QUEST_ENDROP_NONE:
			LOGV(l, "Enemy fixed to drop nothing by policy.\n");
			return 0;

		case PSOCN_QUEST_ENDROP_NORARE:
			do_rare = 0;
			csr = 1;
			break;

		case PSOCN_QUEST_ENDROP_PARTIAL:
			do_rare = 0;
			csr = 0;
			break;

		case PSOCN_QUEST_ENDROP_FREE:
			do_rare = 1;
			csr = 0;
			break;

		default:
			if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_RARES))
				do_rare = 0;
			if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_SRARES))
				csr = 1;
		}
	}

	/* See if the user is lucky today... */
	if (do_rare && (item[0] = rt_generate_v2_rare(c, l, req->pt_index, 0))) {
		LOGV(l, "Rare roll succeeded, generating %08" PRIx32 "\n", item[0]);

		switch (item[0] & 0xFF) {
		case 0:
			/* Weapon -- add percentages and (potentially) grind values and
			   such... */
			if (generate_weapon_v2(ent, area, item, rng, 1,
				l->version == CLIENT_VERSION_DCV1, l))
				return 0;
			break;

		case 1:
			/* Armor/Shield/Unit */
			switch ((item[0] >> 8) & 0xFF) {
			case 1:
				/* Armor -- Add DFP/EVP boosts and slots */
				if (generate_armor_v2(ent, area, item, rng, 1, l))
					return 0;
				break;

			case 2:
				/* Shield -- Add DFP/EVP boosts */
				if (generate_shield_v2(ent, area, item, rng, 1, l))
					return 0;
				break;

			case 3:
				/* Unit -- Nothing to do here */
				break;

			default:
#ifdef DEBUG
				ITEM_LOG("V2 ItemRT generated an invalid item: "
					"%08" PRIx32 "", item[0]);
#endif

				LOG(l, "V2 ItemRT generated an invalid item: %08"
					PRIx32 "\n", item[0]);
				return 0;
			}
			break;

		case 2:
			/* Mag -- Give it 5 DFP and 40% Synchro and an unset color */
			item[0] = 0x00050002;
			item[1] = 0x000101F4;
			item[2] = 0x00010001;
			item[3] = 0x00280000;
			break;

		case 3:
			/* Tool -- Give it a quantity of 1 if its stackable. */
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);
			break;

		default:
#ifdef DEBUG
			ITEM_LOG("V2 ItemRT generated an invalid item: %08"
				PRIx32 "", item[0]);
#endif

			LOG(l, "V2 ItemRT generated an invalid item: %08" PRIx32 "\n",
				item[0]);
			return 0;
		}

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}

	/* Figure out what type to drop... */
	rnd = sfmt_genrand_uint32(rng) % 3;
	switch (rnd) {
	case 0:
		/* Drop the enemy's designated type of item. */
		switch (ent->enemy_drop[req->pt_index]) {
		case BOX_TYPE_WEAPON:
			/* Drop a weapon */
			if (generate_weapon_v2(ent, area, item, rng, 0,
				l->version == CLIENT_VERSION_DCV1,
				l)) {
				return 0;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);

		case BOX_TYPE_ARMOR:
			/* Drop an armor */
			if (generate_armor_v2(ent, area, item, rng, 0, l)) {
				return 0;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);

		case BOX_TYPE_SHIELD:
			/* Drop a shield */
			if (generate_shield_v2(ent, area, item, rng, 0, l)) {
				return 0;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);

		case BOX_TYPE_UNIT:
			/* Drop a unit */
			if (pmt_random_unit_v2(ent->unit_level[area], item, rng,
				l)) {
				return 0;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);

		case -1:
			/* This shouldn't happen, but if it does, don't drop
			   anything at all. */
			LOGV(l, "指定的掉落类型为无效值.\n");
			return 0;

		default:
#ifdef DEBUG
			ITEM_LOG("Unknown/Invalid v2 enemy drop (%d) for "
				"index %d", ent->enemy_drop[req->pt_index],
				req->pt_index);
#endif

			LOG(l, "Unknown/Invalid v2 enemy drop (%d) for index %d\n",
				ent->enemy_drop[req->pt_index], req->pt_index);
			return 0;
		}

		break;

	case 1:
		/* Drop a tool */
		if (generate_tool_v2(ent, area, item, rng, l)) {
			return 0;
		}

		return check_and_send(c, l, item, c->cur_area, req, csr);

	case 2:
		/* Drop meseta */
		if (generate_meseta(ent->enemy_meseta[req->pt_index][0],
			ent->enemy_meseta[req->pt_index][1],
			item, rng, l)) {
			return 0;
		}

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}

	/* Shouldn't ever get here... */
	return 0;
}

int pt_generate_v2_boxdrop(ship_client_t* c, lobby_t* l, void* r) {
	subcmd_itemreq_t* req = (subcmd_itemreq_t*)r;
	uint8_t section = get_lobby_leader_section(l);
	pt_v2_entry_t* ent;
	uint16_t obj_id;
	game_object_t* gobj;
	map_object_t* obj;
	uint32_t rnd, t1, t2;
	int area, do_rare = 1;
	uint32_t item[4] = { 0 };
	float f1, f2;
	sfmt_t* rng = &c->cur_block->sfmt_rng;
	int csr = 0;
	uint32_t qdrop = 0xFFFFFFFF;

	/* Make sure this is actually a box drop... */
	if (req->pt_index != 0x30)
		return -1;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ent = &v2_ptdata[l->sdrops_diff][l->sdrops_section];
	else
#endif
		ent = &v2_ptdata[l->difficulty][section];

	/* Grab the object ID and make sure its sane, then grab the object itself */
	obj_id = LE16(req->request_id);
	if (obj_id > l->map_objs->obj_count) {
		ITEM_LOG("GC %u requested drop from invalid box",
			c->guildcard);
		return -1;
	}

	/* Don't bother if the box has already been opened */
	gobj = &l->map_objs->objs[obj_id];
	if (gobj->flags & 0x00000001)
		return 0;

	obj = &gobj->mobj_data;

	/* Figure out the area we'll be worried with */
	area = c->cur_area;

	/* Dragon -> Cave 1 */
	if (area == 11)
		area = 3;
	/* De Rol Le -> Mine 1 */
	else if (area == 12)
		area = 6;
	/* Vol Opt -> Ruins 1 */
	else if (area == 13)
		area = 8;
	/* Dark Falz -> Ruins 3 */
	else if (area == 14)
		area = 10;
	/* Everything after Dark Falz -> Ruins 3 */
	else if (area > 14)
		area = 10;
	/* Invalid areas... */
	else if (area == 0) {
		ITEM_LOG("GC %u requested box drop on Pioneer 2",
			c->guildcard);
		return -1;
	}

	/* Subtract one, since we want the index in the box_drop array */
	--area;

	/* Mark the box as spent now... */
	gobj->flags |= 0x00000001;

	/* See if we'll do a rare roll. */
	if (l->qid) {
		if (l->mtypes)
			qdrop = quest_search_enemy_list(0x30, l->mtypes, l->num_mtypes, 1);

		switch (qdrop) {
		case PSOCN_QUEST_ENDROP_NONE:
			return 0;

		case PSOCN_QUEST_ENDROP_NORARE:
			do_rare = 0;
			csr = 1;
			break;

		case PSOCN_QUEST_ENDROP_PARTIAL:
			do_rare = 0;
			csr = 0;
			break;

		case PSOCN_QUEST_ENDROP_FREE:
			do_rare = 1;
			csr = 0;
			break;

		default:
			if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_RARES))
				do_rare = 0;
			if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_SRARES))
				csr = 1;
		}
	}

	/* See if the object is fixed-type box */
	t1 = LE32(obj->dword[0]);
	memcpy(&f1, &t1, sizeof(float));
	if ((obj->base_type == LE32(0x00000092) || obj->base_type == LE32(0x00000161)) &&
		f1 < 1.0f + EPSILON && f1 > 1.0f - EPSILON) {
		/* See if it is a fully-fixed item */
		t2 = LE32(obj->dword[1]);
		memcpy(&f2, &t2, sizeof(float));

		if (f2 < 1.0f + EPSILON && f2 > 1.0f - EPSILON) {
			/* Drop the requested item */
			item[0] = ntohl(obj->dword[2]);
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			/* If its a stackable item, make sure to give it a quantity of 1 */
			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);

			/* This will make the meseta boxes for Vol Opt work... */
			if (item[0] == 0x00000004) {
				t1 = LE32(obj->dword[3]) >> 16;
				item[3] = t1 * 10;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);
		}

		t1 = ntohl(obj->dword[2]);
		switch (t1 & 0xFF) {
		case 0:
			goto generate_weapon;

		case 1:
			goto generate_armor;

		case 3:
			goto generate_tool;

		case 4:
			goto generate_meseta;

		default:
			ITEM_LOG("Invalid type detected from fixed-type box!");
			return 0;
		}
	}

	/* See if the user is lucky today... */
	if (do_rare && (item[0] = rt_generate_v2_rare(c, l, -1, area + 1))) {
		switch (item[0] & 0xFF) {
		case 0:
			/* Weapon -- add percentages and (potentially) grind values and
			   such... */
			if (generate_weapon_v2(ent, area, item, rng, 1,
				l->version == CLIENT_VERSION_DCV1, l))
				return 0;
			break;

		case 1:
			/* Armor/Shield/Unit */
			switch ((item[0] >> 8) & 0xFF) {
			case 1:
				/* Armor -- Add DFP/EVP boosts and slots */
				if (generate_armor_v2(ent, area, item, rng, 1, l))
					return 0;
				break;

			case 2:
				/* Shield -- Add DFP/EVP boosts */
				if (generate_shield_v2(ent, area, item, rng, 1, l))
					return 0;
				break;

			case 3:
				/* Unit -- Nothing to do here */
				break;

			default:
#ifdef DEBUG
				ITEM_LOG("V2 ItemRT generated an invalid item: "
					"%08X", item[0]);
#endif
				LOG(l, "V2 ItemRT generated an invalid box item: "
					"%08X\n", item[0]);
				return 0;
			}

			break;

		case 2:
			/* Mag -- Give it 5 DFP and 40% Synchro and an unset color */
			item[0] = 0x00050002;
			item[1] = 0x000101F4;
			item[2] = 0x00010001;
			item[3] = 0x00280000;
			break;

		case 3:
			/* Tool -- Give it a quantity of 1 if its stackable. */
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);
			break;

		default:
#ifdef DEBUG
			ITEM_LOG("V2 ItemRT generated an invalid item: %08X",
				item[0]);
#endif

			LOG(l, "V2 ItemRT generated an invalid box item: %08X\n",
				item[0]);
			return 0;
		}

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}

	/* Generate an item, according to the PT data */
	rnd = sfmt_genrand_uint32(rng) % 100;

	if ((rnd -= ent->box_drop[BOX_TYPE_WEAPON][area]) > 100) {
	generate_weapon:
		/* Generate a weapon */
		if (generate_weapon_v2(ent, area, item, rng, 0,
			l->version == CLIENT_VERSION_DCV1, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_ARMOR][area]) > 100) {
	generate_armor:
		/* Generate an armor */
		if (generate_armor_v2(ent, area, item, rng, 0, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_SHIELD][area]) > 100) {
		/* Generate a shield */
		if (generate_shield_v2(ent, area, item, rng, 0, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_UNIT][area]) > 100) {
		/* Generate a unit */
		if (pmt_random_unit_v2(ent->unit_level[area], item, rng, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_TOOL][area]) > 100) {
	generate_tool:
		/* Generate a tool */
		if (generate_tool_v2(ent, area, item, rng, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_MESETA][area]) > 100) {
	generate_meseta:
		/* Generate money! */
		if (generate_meseta(ent->box_meseta[area][0], ent->box_meseta[area][1],
			item, rng, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}

	/* You get nothing! */
	return 0;
}

/* Generate an item drop from the PT data. This version uses the v3 PT data set.
   This function only works for PSOGC. */
int pt_generate_gc_drop(ship_client_t* c, lobby_t* l, void* r) {
	subcmd_itemreq_t* req = (subcmd_itemreq_t*)r;
	uint8_t section = get_lobby_leader_section(l);
	pt_v3_entry_t* ent;
	uint32_t rnd;
	uint32_t item[4] = { 0 };
	int area, darea, do_rare = 1;
	sfmt_t* rng = &c->cur_block->sfmt_rng;
	uint16_t mid;
	game_enemy_t* enemy;
	int csr = 0;

	/* Make sure the PT index in the packet is sane */
	//if(req->pt_index > 0x33)
	//    return -1;

	/* If the PT index is 0x30, this is a box, not an enemy! */
	if (req->pt_index == 0x30)
		return pt_generate_gc_boxdrop(c, l, r);

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ent = &gc_ptdata[l->sdrops_ep - 1][l->sdrops_diff][l->sdrops_section];
	else
#endif
		ent = &gc_ptdata[l->episode - 1][l->difficulty][section];

	/* Figure out the area we'll be worried with */
	area = darea = c->cur_area;

	/* Episode II area list:
	   0 = Pioneer 2, 1 = VR Temple Alpha, 2 = VR Temple Beta,
	   3 = VR Space Ship Alpha, 4 = VR Space Ship Beta,
	   5 = Central Control Area, 6 = Jungle North, 7 = Jungle South,
	   8 = Mountains, 9 = SeaSide Daytime, 10 = SeaBed Upper Levels,
	   11 = SeaBed Lower Levels, 12 = Gal Gryphon, 13 = Olga Flow,
	   14 = Barba Ray, 15 = Gol Dragon, 16 = SeaSide Nighttime (battle only),
	   17 = Tower 4th Floor (battle only) */
	switch (l->episode) {
	case 0:
	case 1:
		/* Dragon -> Cave 1 */
		if (area == 11)
			darea = 3;
		/* De Rol Le -> Mine 1 */
		else if (area == 12)
			darea = 6;
		/* Vol Opt -> Ruins 1 */
		else if (area == 13)
			darea = 8;
		/* Dark Falz -> Ruins 3 */
		else if (area == 14)
			darea = 10;
		/* Everything after Dark Falz -> Ruins 3 */
		else if (area > 14)
			darea = 10;
		/* Invalid areas... */
		else if (area == 0) {
			ITEM_LOG("GC %u requested enemy drop on Pioneer "
				"2", c->guildcard);
			return -1;
		}

		break;

	case 2:
		/* Barba Ray -> VR Space Ship Alpha */
		if (area == 14)
			darea = 3;
		/* Gol Dragon -> Jungle North */
		else if (area == 15)
			darea = 6;
		/* Gal Gryphon -> SeaSide Daytime */
		else if (area == 12)
			darea = 9;
		/* Olga Flow -> SeaBed Upper Levels */
		else if (area == 13)
			darea = 10;
		/* All others after SeaBed Upper Levels -> SeaBed Upper Levels */
		else if (area > 10)
			darea = 10;
		else if (area == 0) {
			ITEM_LOG("GC %u requested enemy drop on Pioneer "
				"2", c->guildcard);
			return -1;
		}

		break;
	}

	/* Subtract one, since we want the index in the box_drop array */
	--darea;

	/* Make sure the enemy's id is sane... */
	mid = LE16(req->request_id);

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS) {
		ITEM_LOG("GC %u Debug Drop (GC).\n"
			"        Episode %d, Section: %d, Difficulty: %d,\n"
			"        Area: %d (%d), Enemy ID: %d,\n"
			"        PT Entry Pointer: %p",
			c->guildcard, l->sdrops_ep, l->sdrops_section, l->sdrops_diff,
			area, darea, mid, ent);
	}
#endif

	/* We only really need this separate for debugging... */
	area = darea;

	if (mid > l->map_enemies->enemy_count) {
#ifdef DEBUG
		ITEM_LOG("GC %" PRIu32 " requested GC drop for invalid "
			"enemy (%d -- max: %d, quest=%" PRIu32 ")!", c->guildcard, mid,
			l->map_enemies->count, l->qid);
#endif

		if (l->logfp) {
			fdebug(l->logfp, DBG_WARN, "GC %" PRIu32 " requested GC "
				"drop for invalid enemy (%d -- max: %d, quest=%" PRIu32
				")!\n", c->guildcard, mid, l->map_enemies->enemy_count, l->qid);
		}

		return -1;
	}

	/* Grab the map enemy to make sure it hasn't already dropped something. */
	enemy = &l->map_enemies->enemies[mid];
	if (enemy->drop_done) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Drop already done. Returning no item.");
#endif
		return 0;
	}

	enemy->drop_done = 1;

	/* See if the enemy is going to drop anything at all this time... */
	rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("DAR: %d, Rolled: %d", ent->enemy_dar[req->pt_index],
			rnd);
#endif

	if ((int8_t)rnd >= ent->enemy_dar[req->pt_index]) {
		/* Nope. You get nothing! */
		return 0;
	}

	/* See if we'll do a rare roll. */
	if (l->qid) {
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_RARES))
			do_rare = 0;
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_SRARES))
			csr = 1;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("To do rare roll: %s, Allow quest semi-rares: %s",
			do_rare ? "Yes" : "No", csr ? "No" : "Yes");
#endif

	/* See if the user is lucky today... */
	if (do_rare && (item[0] = rt_generate_gc_rare(c, l, req->pt_index, 0))) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Rare roll succeeded, generating %08X", item[0]);
#endif

		switch (item[0] & 0xFF) {
		case 0:
			/* Weapon -- add percentages and (potentially) grind values and
			   such... */
			if (generate_weapon_v3(ent, area, item, rng, 1, 0, l))
				return 0;
			break;

		case 1:
			/* Armor/Shield/Unit */
			switch ((item[0] >> 8) & 0xFF) {
			case 1:
				/* Armor -- Add DFP/EVP boosts and slots */
				if (generate_armor_v3(ent, area, item, rng, 1, 0, l))
					return 0;
				break;

			case 2:
				/* Shield -- Add DFP/EVP boosts */
				if (generate_shield_v3(ent, area, item, rng, 1, 0, l))
					return 0;
				break;

			case 3:
				/* Unit -- Nothing to do here */
				break;

			default:
#ifdef DEBUG
				ITEM_LOG("GC ItemRT generated an invalid item: "
					"%08X", item[0]);
#endif

				if (l->logfp) {
					fdebug(l->logfp, DBG_WARN, "GC ItemRT generated an "
						"invalid item: %08X\n", item[0]);
				}

				return 0;
			}
			break;

		case 2:
			/* Mag -- Give it 5 DFP and 40% Synchro and an unset color */
			item[0] = 0x00050002;
			item[1] = 0x000001F4;
			item[2] = 0x00000000;
			item[3] = 0x280000FF;
			break;

		case 3:
			/* Tool -- Give it a quantity of 1 if its stackable. */
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);
			break;

		default:
#ifdef DEBUG
			ITEM_LOG("ItemRT generated an invalid item: %08X",
				item[0]);
#endif

			if (l->logfp) {
				fdebug(l->logfp, DBG_WARN, "GC ItemRT generated an invalid "
					"item: %08X\n", item[0]);
			}

			return 0;
		}

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}

	/* Figure out what type to drop... */
	rnd = sfmt_genrand_uint32(rng) % 3;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS) {
		switch (rnd) {
		case 0:
			ITEM_LOG("Drop designated item type: %d.",
				ent->enemy_drop[req->pt_index]);
			break;
		case 1:
			ITEM_LOG("Drop tool");
			break;
		case 2:
			ITEM_LOG("Drop meseta");
			break;
		}
	}
#endif

	switch (rnd) {
	case 0:
		/* Drop the enemy's designated type of item. */
		switch (ent->enemy_drop[req->pt_index]) {
		case BOX_TYPE_WEAPON:
			/* Drop a weapon */
			if (generate_weapon_v3(ent, area, item, rng, 0, 0, l)) {
				return 0;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);

		case BOX_TYPE_ARMOR:
			/* Drop an armor */
			if (generate_armor_v3(ent, area, item, rng, 0, 0, l)) {
				return 0;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);

		case BOX_TYPE_SHIELD:
			/* Drop a shield */
			if (generate_shield_v3(ent, area, item, rng, 0, 0, l)) {
				return 0;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);

		case BOX_TYPE_UNIT:
			/* Drop a unit */
			if (pmt_random_unit_gc(ent->unit_level[area], item, rng,
				l)) {
				return 0;
			}

			return check_and_send(c, l, item, c->cur_area, req, csr);

		case -1:
			/* This shouldn't happen, but if it does, don't drop
			   anything at all. */
			return 0;

		default:
#ifdef DEBUG
			ITEM_LOG("Unknown/Invalid GC enemy drop (%d) for "
				"index %d", ent->enemy_drop[req->pt_index],
				req->pt_index);
#endif

			if (l->logfp) {
				fdebug(l->logfp, DBG_WARN, "Unknown/Invalid GC enemy "
					"drop (%d) for index %d\n",
					ent->enemy_drop[req->pt_index], req->pt_index);
			}

			return 0;
		}

		break;

	case 1:
		/* Drop a tool */
		if (generate_tool_v3(ent, area, item, rng, l)) {
			return 0;
		}

		return check_and_send(c, l, item, c->cur_area, req, csr);

	case 2:
		/* Drop meseta */
		if (generate_meseta(ent->enemy_meseta[req->pt_index][0],
			ent->enemy_meseta[req->pt_index][1],
			item, rng, l)) {
			return 0;
		}

		return check_and_send(c, l, item, c->cur_area, req, csr);
	}

	/* Shouldn't ever get here... */
	return 0;
}

int pt_generate_gc_boxdrop(ship_client_t* c, lobby_t* l, void* r) {
	subcmd_bitemreq_t* req = (subcmd_bitemreq_t*)r;
	uint8_t section = get_lobby_leader_section(l);
	pt_v3_entry_t* ent;
	uint16_t obj_id;
	game_object_t* gobj;
	map_object_t* obj;
	uint32_t rnd, t1, t2;
	int area, darea, do_rare = 1;
	uint32_t item[4] = { 0 };
	float f1, f2;
	sfmt_t* rng = &c->cur_block->sfmt_rng;
	int csr = 0;

	/* Make sure this is actually a box drop... */
	if (req->pt_index != 0x30)
		return -1;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ent = &gc_ptdata[l->sdrops_ep - 1][l->sdrops_diff][l->sdrops_section];
	else
#endif
		ent = &gc_ptdata[l->episode - 1][l->difficulty][section];

	/* Grab the object ID and make sure its sane, then grab the object itself */
	obj_id = LE16(req->request_id);
	if (obj_id > l->map_objs->obj_count) {
		ITEM_LOG("Guildard %u requested drop from invalid box",
			c->guildcard);
		return -1;
	}

	/* Don't bother if the box has already been opened */
	gobj = &l->map_objs->objs[obj_id];
	if (gobj->flags & 0x00000001) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Requested drop from opened box: %d", obj_id);
#endif

		return 0;
	}

	obj = &gobj->mobj_data;

	/* Figure out the area we'll be worried with */
	area = darea = c->cur_area;

	switch (l->episode) {
	case 0:
	case 1:
		/* Dragon -> Cave 1 */
		if (area == 11)
			darea = 3;
		/* De Rol Le -> Mine 1 */
		else if (area == 12)
			darea = 6;
		/* Vol Opt -> Ruins 1 */
		else if (area == 13)
			darea = 8;
		/* Dark Falz -> Ruins 3 */
		else if (area == 14)
			darea = 10;
		/* Everything after Dark Falz -> Ruins 3 */
		else if (area > 14)
			darea = 10;
		/* Invalid areas... */
		else if (area == 0) {
			ITEM_LOG("GC %u requested box drop on Pioneer "
				"2", c->guildcard);
			return -1;
		}

		break;

	case 2:
		/* Barba Ray -> VR Space Ship Alpha */
		if (area == 14)
			darea = 3;
		/* Gol Dragon -> Jungle North */
		else if (area == 15)
			darea = 6;
		/* Gal Gryphon -> SeaSide Daytime */
		else if (area == 12)
			darea = 9;
		/* Olga Flow -> SeaBed Upper Levels */
		else if (area == 13)
			darea = 10;
		/* All others after SeaBed Upper Levels -> SeaBed Upper Levels */
		else if (area > 10)
			darea = 10;
		else if (area == 0) {
			ITEM_LOG("GC %u requested box drop on Pioneer "
				"2", c->guildcard);
			return -1;
		}

		break;
	}

	/* Subtract one, since we want the index in the box_drop array */
	--darea;

	/* Mark the box as spent now... */
	gobj->flags |= 0x00000001;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS) {
		ITEM_LOG("GC %u Debug Box Drop (GC).\n"
			"        Episode %d, Section: %d, Difficulty: %d,\n"
			"        Area: %d (%d), Object ID: %d,\n"
			"        Object Skin: %08" PRIx32 ",\n"
			"        PT Entry Pointer: %p",
			c->guildcard, l->sdrops_ep, l->sdrops_section, l->sdrops_diff,
			area, darea, obj_id, LE32(obj->skin), ent);
	}
#endif

	/* We only really need this separate for debugging... */
	area = darea;

	/* See if we'll do a rare roll. */
	if (l->qid) {
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_RARES))
			do_rare = 0;
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_SRARES))
			csr = 1;
	}

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("To do rare roll: %s, Allow quest semi-rares: %s",
			do_rare ? "Yes" : "No", csr ? "No" : "Yes");
#endif

	/* See if the object is fixed-type box */
	t1 = LE32(obj->dword[0]);
	memcpy(&f1, &t1, sizeof(float));
	if ((obj->base_type == LE32(0x00000092) || obj->base_type == LE32(0x00000161)) &&
		f1 < 1.0f + EPSILON && f1 > 1.0f - EPSILON) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Hit fixed box...");
#endif

		/* See if it is a fully-fixed item */
		t2 = LE32(obj->dword[1]);
		memcpy(&f2, &t2, sizeof(float));

		if (f2 < 1.0f + EPSILON && f2 > 1.0f - EPSILON) {
			/* Drop the requested item */
			item[0] = ntohl(obj->dword[2]);
			item[1] = item[2] = item[3] = 0;

#ifdef DEBUG
			if (l->flags & LOBBY_FLAG_DBG_SDROPS)
				ITEM_LOG("Fully-fixed box. Dropping %08" PRIx32 "",
					ntohl(obj->dword[2]));
#endif

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			/* If its a stackable item, make sure to give it a quantity of 1 */
			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);

			/* This will make the meseta boxes for Vol Opt work... */
			if (item[0] == 0x00000004) {
				t1 = LE32(obj->dword[3]) >> 16;
				item[3] = t1 * 10;
			}

			return check_and_send(c, l, item, c->cur_area,
				(subcmd_itemreq_t*)req, csr);
		}

		t1 = ntohl(obj->dword[2]);

#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Fixed-type box. Generating type %d", t1 & 0xFF);
#endif

		switch (t1 & 0xFF) {
		case 0:
			goto generate_weapon;

		case 1:
			goto generate_armor;

		case 3:
			goto generate_tool;

		case 4:
			goto generate_meseta;

		default:
			ITEM_LOG("Invalid type detected from fixed-type box!");
			return 0;
		}
	}

	/* See if the user is lucky today... */
	if (do_rare && (item[0] = rt_generate_gc_rare(c, l, -1, area + 1))) {
#ifdef DEBUG
		if (l->flags & LOBBY_FLAG_DBG_SDROPS)
			ITEM_LOG("Rare roll succeeded, generating %08X", item[0]);
#endif

		switch (item[0] & 0xFF) {
		case 0:
			/* Weapon -- add percentages and (potentially) grind values and
			   such... */
			if (generate_weapon_v3(ent, area, item, rng, 1, 0, l))
				return 0;
			break;

		case 1:
			/* Armor/Shield/Unit */
			switch ((item[0] >> 8) & 0xFF) {
			case 1:
				/* Armor -- Add DFP/EVP boosts and slots */
				if (generate_armor_v3(ent, area, item, rng, 1, 0, l))
					return 0;
				break;

			case 2:
				/* Shield -- Add DFP/EVP boosts */
				if (generate_shield_v3(ent, area, item, rng, 1, 0, l))
					return 0;
				break;

			case 3:
				/* Unit -- Nothing to do here */
				break;

			default:
#ifdef DEBUG
				ITEM_LOG("GC ItemRT generated an invalid item: "
					"%08X", item[0]);
#endif

				if (l->logfp) {
					fdebug(l->logfp, DBG_WARN, "GC ItemRT generated an "
						"invalid item: %08X\n", item[0]);
				}

				return 0;
			}

			break;

		case 2:
			/* Mag -- Give it 5 DFP and 40% Synchro and an unset color */
			item[0] = 0x00050002;
			item[1] = 0x000001F4;
			item[2] = 0x00000000;
			item[3] = 0x280000FF;
			break;

		case 3:
			/* Tool -- Give it a quantity of 1 if its stackable. */
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);
			break;

		default:
#ifdef DEBUG
			ITEM_LOG("ItemRT generated an invalid item: %08X",
				item[0]);
#endif

			if (l->logfp) {
				fdebug(l->logfp, DBG_WARN, "GC ItemRT generated an invalid "
					"item: %08X\n", item[0]);
			}

			return 0;
		}

		return check_and_send(c, l, item, c->cur_area, (subcmd_itemreq_t*)req,
			csr);
	}

	/* Generate an item, according to the PT data */
	rnd = sfmt_genrand_uint32(rng) % 100;

#ifdef DEBUG
	if (l->flags & LOBBY_FLAG_DBG_SDROPS)
		ITEM_LOG("RNG generated %d", rnd);
#endif

	if ((rnd -= ent->box_drop[BOX_TYPE_WEAPON][area]) > 100) {
	generate_weapon:
		/* Generate a weapon */
		if (generate_weapon_v3(ent, area, item, rng, 0, 0, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, (subcmd_itemreq_t*)req,
			csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_ARMOR][area]) > 100) {
	generate_armor:
		/* Generate an armor */
		if (generate_armor_v3(ent, area, item, rng, 0, 0, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, (subcmd_itemreq_t*)req,
			csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_SHIELD][area]) > 100) {
		/* Generate a shield */
		if (generate_shield_v3(ent, area, item, rng, 0, 0, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, (subcmd_itemreq_t*)req,
			csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_UNIT][area]) > 100) {
		/* Generate a unit */
		if (pmt_random_unit_gc(ent->unit_level[area], item, rng, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, (subcmd_itemreq_t*)req,
			csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_TOOL][area]) > 100) {
	generate_tool:
		/* Generate a tool */
		if (generate_tool_v3(ent, area, item, rng, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, (subcmd_itemreq_t*)req,
			csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_MESETA][area]) > 100) {
	generate_meseta:
		/* Generate money! */
		if (generate_meseta(ent->box_meseta[area][0], ent->box_meseta[area][1],
			item, rng, l))
			return 0;

		return check_and_send(c, l, item, c->cur_area, (subcmd_itemreq_t*)req,
			csr);
	}

	/* You get nothing! */
	return 0;
}

int pt_generate_bb_drop(ship_client_t* src, lobby_t* l, void* r) {
	subcmd_bb_itemreq_t* req = (subcmd_bb_itemreq_t*)r;
	uint8_t section = get_lobby_leader_section(l);
	uint32_t rnd;
	uint32_t item[4] = { 0 };
	int area, do_rare = 1;
	sfmt_t* rng = &src->cur_block->sfmt_rng;
	uint16_t mid;
	game_enemy_t* enemy;
	int csr = 0;
	uint8_t pt_index = req->pt_index;
	size_t ep_pt_index = get_pt_index(l->episode, pt_index);

	pt_bb_entry_t* ent = get_pt_data_bb(l->episode, l->challenge, l->difficulty, section);
	if (!ent) {
		ERR_LOG("%s Item_PT 不存在章节 %d 难度 %d 颜色 %d 的掉落", client_type[src->version].ver_name, l->episode, l->difficulty, section);
		return 0;
	}

#ifdef DEBUG
	print_ascii_hex(ent, sizeof(pt_bb_entry_t));
#endif // DEBUG

	/* If the PT index is 0x30, this is a box, not an enemy! */
	if (pt_index == 0x30) {
		return pt_generate_bb_boxdrop(src, l, r);
	}

	/* Figure out the area we'll be worried with */
	area = get_pt_data_area_bb(l->episode, src->cur_area);

#ifdef DEBUG

	DBG_LOG("area %d l->episode %d pt_index %d ep_pt_index %d", area, l->episode, pt_index, ep_pt_index);

#endif // DEBUG

	/* Make sure the enemy's id is sane... */
	mid = LE16(req->entity_id);
	if (mid > l->map_enemies->enemy_count) {
		ITEM_LOG("GC %" PRIu32 " 请求无效敌人掉落 (%d -- max: %d, 任务=%" PRIu32 ")!", src->guildcard, mid,
			l->map_enemies->enemy_count, l->qid);
		return -1;
	}

	/* Grab the map enemy to make sure it hasn't already dropped something. */
	enemy = &l->map_enemies->enemies[mid];
	if (enemy->drop_done)
		return 0;

	enemy->drop_done = 1;

	/* See if the enemy is going to drop anything at all this time... */
	rnd = sfmt_genrand_uint32(rng) % 100;

	if (!src->game_data->gm_drop_rare) {
		if ((rnd >= ent->enemy_type_drop_probs[ep_pt_index])) {
#ifdef DEBUG
			DBG_LOG("啥也没得到 rnd %d probs %d", rnd, ent->enemy_type_drop_probs[ep_pt_index]);
#endif // DEBUG
			/* Nope. You get nothing! */
			return 0;
		}
	}

	/* See if we'll do a rare roll. */
	if (l->qid) {
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_RARES))
			do_rare = 0;
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_SRARES))
			csr = 1;
	}

	if (src->game_data->gm_drop_rare) {
		do_rare = 1;
		DBG_LOG("全红掉落已开启 %d", do_rare);
	}

#ifdef DEBUG

	DBG_LOG("pt_index %d ep_pt_index %d episode %d challenge %d difficulty %d section %d", pt_index, ep_pt_index, l->episode, l->challenge, l->difficulty, section);

#endif // DEBUG

	item[0] = rt_generate_bb_rare(src, l, ep_pt_index, 0, section);

	/* See if the user is lucky today... */
	if (do_rare && item[0] && !item_not_identification_bb(item[0], item[1])) {

#ifdef DEBUG

		DBG_LOG("GC %" PRIu32 " ITEM数据! 0x%02X", src->guildcard, item[0] & 0xFF);

#endif // DEBUG

		switch (item[0] & 0xFF) {
		case ITEM_TYPE_WEAPON:
			/* Weapon -- add percentages and (potentially) grind values and
			   such... */
			if (generate_weapon_bb(ent, area, item, rng, 1, l))
				return 0;
			break;

		case ITEM_TYPE_GUARD:
			/* Armor/Shield/Unit */
			switch ((item[0] >> 8) & 0xFF) {
			case ITEM_SUBTYPE_FRAME:
				/* Armor -- Add DFP/EVP boosts and slots */
				if (generate_armor_bb(ent, area, item, rng, 1, l))
					return 0;
				break;

			case ITEM_SUBTYPE_BARRIER:
				/* Shield -- Add DFP/EVP boosts */
				if (generate_shield_bb(ent, area, item, rng, 1, l))
					return 0;
				break;

			case ITEM_SUBTYPE_UNIT:
				/* Unit -- Nothing to do here */
				break;

			default:
				ITEM_LOG("ItemRT 生成无效物品: %08X", item[0]);
				return 0;
			}
			break;

		case ITEM_TYPE_MAG:
			/* Mag -- Give it 5 DFP and 40% Synchro and an unset color */
			item[0] = 0x00050002;
			item[1] = 0x000001F4;
			item[2] = 0x00000000;
			item[3] = 0xFF000028;
			break;

		case ITEM_TYPE_TOOL:
			/* Tool -- Give it a quantity of 1 if its stackable. */
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);
			break;

		default:
			ITEM_LOG("ItemRT 生成无效物品: %08X", item[0]);
			return 0;
		}

		return check_and_send_bb_lobby(src, l, item, src->cur_area, req, csr);
	}

	/* Figure out what type to drop... */
	rnd = sfmt_genrand_uint32(rng) % 3;

#ifdef DEBUG

	ITEM_LOG("GC %u 请求章节 %d 难度 %d 区域 %d ptID %d ep_pt_index %d emptID %d 随机 %d 物品掉落", src->guildcard, l->episode, l->difficulty, src->cur_area, req->pt_index, ep_pt_index, ent->enemy_type_drop_probs[ep_pt_index], rnd);

#endif // DEBUG

	switch (rnd) {
	case 0:
		/* Drop the enemy's designated type of item. */
		switch (ent->enemy_item_classes[ep_pt_index]) {
		case BOX_TYPE_WEAPON:
			/* Drop a weapon */
			if (generate_weapon_bb(ent, area, item, rng, 0, l)) {
				return 0;
			}

			return check_and_send_bb_lobby(src, l, item, src->cur_area, req, csr);

		case BOX_TYPE_ARMOR:
			/* Drop an armor */
			if (generate_armor_bb(ent, area, item, rng, 0, l)) {
				return 0;
			}

			return check_and_send_bb_lobby(src, l, item, src->cur_area, req, csr);

		case BOX_TYPE_SHIELD:
			/* Drop a shield */
			if (generate_shield_bb(ent, area, item, rng, 0, l)) {
				return 0;
			}

			return check_and_send_bb_lobby(src, l, item, src->cur_area, req, csr);

		case BOX_TYPE_UNIT:
			/* Drop a unit */
			if (pmt_random_unit_bb(ent->unit_maxes[area], item, rng, l)) {
				return 0;
			}

			return check_and_send_bb_lobby(src, l, item, src->cur_area, req, csr);

		case -1:
			/* This shouldn't happen, but if it does, don't drop
			   anything at all. */
			return 0;

		case 0xFF:
			/* 啥也没有. */
			return 0;

		default:
			ITEM_LOG("未知/无效怪物掉落 (%d) 索引 "
				"%d ep_pt_index %d episode %d", ent->enemy_item_classes[ep_pt_index],
				req->pt_index, ep_pt_index, l->episode);
			return 0;
		}

		break;

	case 1:
		/* Drop a tool */
		if (generate_tool_bb(ent, area, item, rng, l)) {
			return 0;
		}

		return check_and_send_bb_lobby(src, l, item, src->cur_area, req, csr);

	case 2:
		/* Drop meseta */
		if (generate_meseta(ent->enemy_meseta_ranges[ep_pt_index].min,
			ent->enemy_meseta_ranges[ep_pt_index].max,
			item, rng, l)) {
			return 0;
		}

		return check_and_send_bb_lobby(src, l, item, src->cur_area, req, csr);
	}

	/* Shouldn't ever get here... */
	return 0;
}

int pt_generate_bb_boxdrop(ship_client_t* src, lobby_t* l, void* r) {
	subcmd_bb_bitemreq_t* req = (subcmd_bb_bitemreq_t*)r;
	uint8_t section = get_lobby_leader_section(l);
	uint16_t obj_id;
	game_object_t* gobj;
	map_object_t* obj;
	uint32_t rnd, t1, t2;
	int area, do_rare = 1;
	uint32_t item[4] = { 0 };
	float f1, f2;
	sfmt_t* rng = &src->cur_block->sfmt_rng;
	int csr = 0;
	uint8_t pt_index = req->pt_index;

	pt_bb_entry_t* ent = get_pt_data_bb(l->episode, l->challenge, l->difficulty, section);
	if (!ent) {
		ERR_LOG("%s Item_PT 不存在难度 %d 颜色 %d 的掉落", client_type[src->version].ver_name, l->difficulty, section);
		return 0;
	}

	//ITEM_LOG("GC %u 请求章节 %d 难度 %d 物品掉落", c->guildcard, l->episode, l->difficulty);

	/* Make sure this is actually a box drop... */
	if (pt_index != 0x30)
		return -1;

	/* Grab the object ID and make sure its sane, then grab the object itself */
	obj_id = LE16(req->request_id);
	if (obj_id > l->map_objs->obj_count) {
		ITEM_LOG("GC %u 请求的箱子掉落无效",
			src->guildcard);
		return -1;
	}

	/* Don't bother if the box has already been opened */
	gobj = &l->map_objs->objs[obj_id];
	if (gobj->flags & 0x00000001)
		return 0;

	obj = &gobj->mobj_data;

	/* Figure out the area we'll be worried with */
	area = get_pt_data_area_bb(l->episode, src->cur_area);

	/* Mark the box as spent now... */
	gobj->flags |= 0x00000001;

	/* See if we'll do a rare roll. */
	if (l->qid) {
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_RARES))
			do_rare = 0;
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_SRARES))
			csr = 1;
	}

	/* See if the object is fixed-type box */
	t1 = LE32(obj->dword[0]);
	memcpy(&f1, &t1, sizeof(float));
	if ((obj->base_type == LE32(0x00000092) || obj->base_type == LE32(0x00000161)) &&
		f1 < 1.0f + EPSILON && f1 > 1.0f - EPSILON) {
		/* See if it is a fully-fixed item */
		t2 = LE32(obj->dword[1]);
		memcpy(&f2, &t2, sizeof(float));

		if (f2 < 1.0f + EPSILON && f2 > 1.0f - EPSILON) {
			/* Drop the requested item */
			item[0] = ntohl(obj->dword[2]);
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			/* If its a stackable item, make sure to give it a quantity of 1 */
			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);

			/* This will make the meseta boxes for Vol Opt work... */
			if (item[0] == 0x00000004) {
				t1 = LE32(obj->dword[3]) >> 16;
				item[3] = t1 * 10;
			}

			if (item_not_identification_bb(item[0], item[1])) {
				return 0;
			}

			return check_and_send_bb_lobby(src, l, item, src->cur_area,
				(subcmd_bb_itemreq_t*)req, csr);
		}

		t1 = ntohl(obj->dword[2]);
		switch (t1 & 0xFF) {
		case ITEM_TYPE_WEAPON:
			goto generate_weapon;

		case ITEM_TYPE_GUARD:
			goto generate_armor;

		case ITEM_TYPE_TOOL:
			goto generate_tool;

		case ITEM_TYPE_MESETA:
			goto generate_meseta;

		default:
			ITEM_LOG("Invalid type detected from fixed-type box!");
			return 0;
		}
	}

	item[0] = rt_generate_bb_rare(src, l, -1, area + 1, section);

	/* See if the user is lucky today... */
	if (do_rare && item[0] && !item_not_identification_bb(item[0], item[1])) {

		switch (item[0] & 0xFF) {
		case ITEM_TYPE_WEAPON:
			/* Weapon -- add percentages and (potentially) grind values and
			   such... */
			if (generate_weapon_bb(ent, area, item, rng, 1, l))
				return 0;
			break;

		case ITEM_TYPE_GUARD:
			/* Armor/Shield/Unit */
			switch ((item[0] >> 8) & 0xFF) {
			case ITEM_SUBTYPE_FRAME:
				/* Armor -- Add DFP/EVP boosts and slots */
				if (generate_armor_bb(ent, area, item, rng, 1, l))
					return 0;
				break;

			case ITEM_SUBTYPE_BARRIER:
				/* Shield -- Add DFP/EVP boosts */
				if (generate_shield_bb(ent, area, item, rng, 1, l))
					return 0;
				break;

			case ITEM_SUBTYPE_UNIT:
				/* Unit -- Nothing to do here */
				break;

			default:
				ITEM_LOG("BB ItemRT 生成无效物品: "
					"%08X", item[0]);
				return 0;
			}

			break;

		case ITEM_TYPE_MAG:
			/* Mag -- Give it 5 DFP and 40% Synchro and an unset color */
			item[0] = 0x00050002;
			item[1] = 0x000001F4;
			item[2] = 0x00000000;
			item[3] = 0xFF000028;
			break;

		case ITEM_TYPE_TOOL:
			/* Tool -- Give it a quantity of 1 if its stackable. */
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);
			break;

		default:
			ITEM_LOG("BB ItemRT 生成无效物品: %08X",
				item[0]);
			return 0;
		}

		return check_and_send_bb_lobby(src, l, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}

	/* Generate an item, according to the PT data */
	rnd = sfmt_genrand_uint32(rng) % 100;

	if ((rnd -= ent->box_drop[BOX_TYPE_WEAPON][area]) > 100) {
	generate_weapon:
		/* Generate a weapon */
		if (generate_weapon_bb(ent, area, item, rng, 0, l))
			return 0;

		return check_and_send_bb_lobby(src, l, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_ARMOR][area]) > 100) {
	generate_armor:
		/* Generate an armor */
		if (generate_armor_bb(ent, area, item, rng, 0, l))
			return 0;

		return check_and_send_bb_lobby(src, l, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_SHIELD][area]) > 100) {
		/* Generate a shield */
		if (generate_shield_bb(ent, area, item, rng, 0, l))
			return 0;

		return check_and_send_bb_lobby(src, l, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_UNIT][area]) > 100) {
		/* Generate a unit */
		if (pmt_random_unit_bb(ent->unit_maxes[area], item, rng, l))
			return 0;

		return check_and_send_bb_lobby(src, l, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_TOOL][area]) > 100) {
	generate_tool:
		/* Generate a tool */
		if (generate_tool_bb(ent, area, item, rng, l))
			return 0;

		return check_and_send_bb_lobby(src, l, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_MESETA][area]) > 100) {
	generate_meseta:
		/* Generate money! */
		if (generate_meseta(ent->box_meseta_ranges[area].min, ent->box_meseta_ranges[area].max,
			item, rng, l))
			return 0;

		return check_and_send_bb_lobby(src, l, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}

	/* You get nothing! */
	return 0;
}

int pt_generate_bb_pso2_drop_style(ship_client_t* src, lobby_t* l, uint8_t section, subcmd_bb_itemreq_t* req) {
	uint32_t rnd;
	uint32_t item[4] = { 0 };
	int area, do_rare = 1;
	sfmt_t* rng = &src->sfmt_rng;
	uint16_t mid;
	game_enemy_t* enemy;
	int csr = 0;
	uint8_t pt_index = req->pt_index;
	size_t ep_pt_index = get_pt_index(l->episode, pt_index);

	pt_bb_entry_t* ent = get_pt_data_bb(l->episode, l->challenge, l->difficulty, section);
	if (!ent) {
		ERR_LOG("%s Item_PT 不存在章节 %d 难度 %d 颜色 %d 的掉落", client_type[src->version].ver_name, l->episode, l->difficulty, section);
		return 0;
	}

	/* Make sure the PT index in the packet is sane */
	//if(req->pt_index > 0x33)
	//    return -1;

	/* If the PT index is 0x30, this is a box, not an enemy! */
	if (pt_index == 0x30)
		return pt_generate_bb_pso2_boxdrop(src, l, section, req);

	/* Figure out the area we'll be worried with */
	area = get_pt_data_area_bb(l->episode, src->cur_area);

#ifdef DEBUG

	DBG_LOG("area %d l->episode %d", area, l->episode);

#endif // DEBUG

	/* Make sure the enemy's id is sane... */
	mid = LE16(req->entity_id);
	if (mid > l->map_enemies->enemy_count) {
		ITEM_LOG("GC %" PRIu32 " 请求无效敌人掉落 (%d -- max: %d, 任务=%" PRIu32 ")!", src->guildcard, mid,
			l->map_enemies->enemy_count, l->qid);
		return -1;
	}

	/* Grab the map enemy to make sure it hasn't already dropped something. */
	enemy = &l->map_enemies->enemies[mid];
	if (enemy->drop_done)
		return 0;

	enemy->drop_done = 1;

	/* See if the enemy is going to drop anything at all this time... */
	rnd = sfmt_genrand_uint32(rng) % 100;

	if (!src->game_data->gm_drop_rare) {
		if ((rnd >= ent->enemy_type_drop_probs[ep_pt_index]))
			/* Nope. You get nothing! */
			return 0;
	}

	/* See if we'll do a rare roll. */
	if (l->qid) {
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_RARES))
			do_rare = 0;
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_SRARES))
			csr = 1;
	}

	item[0] = rt_generate_bb_rare_style(src, l, ep_pt_index, 0, section);
	/* See if the user is lucky today... */
	if (do_rare && item[0] && !item_not_identification_bb(item[0], item[1])) {

#ifdef DEBUG

		ERR_LOG("GC %" PRIu32 " ITEM数据! 0x%02X", src->guildcard, item[0] & 0xFF);

#endif // DEBUG

		switch (item[0] & 0xFF) {
		case ITEM_TYPE_WEAPON:
			/* Weapon -- add percentages and (potentially) grind values and
			   such... */
			if (generate_weapon_bb(ent, area, item, rng, 1, l))
				return 0;
			break;

		case ITEM_TYPE_GUARD:
			/* Armor/Shield/Unit */
			switch ((item[0] >> 8) & 0xFF) {
			case ITEM_SUBTYPE_FRAME:
				/* Armor -- Add DFP/EVP boosts and slots */
				if (generate_armor_bb(ent, area, item, rng, 1, l))
					return 0;
				break;

			case ITEM_SUBTYPE_BARRIER:
				/* Shield -- Add DFP/EVP boosts */
				if (generate_shield_bb(ent, area, item, rng, 1, l))
					return 0;
				break;

			case ITEM_SUBTYPE_UNIT:
				/* Unit -- Nothing to do here */
				break;

			default:
				ITEM_LOG("ItemRT 生成无效物品: %08X", item[0]);
				return 0;
			}
			break;

		case ITEM_TYPE_MAG:
			/* Mag -- Give it 5 DFP and 40% Synchro and an unset color */
			item[0] = 0x00050002;
			item[1] = 0x000001F4;
			item[2] = 0x00000000;
			item[3] = 0xFF000028;
			break;

		case ITEM_TYPE_TOOL:
			/* Tool -- Give it a quantity of 1 if its stackable. */
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { item[0], item[1], item[2], 0, item[3] };

			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);
			break;

		default:
			ITEM_LOG("ItemRT 生成无效物品: %08X", item[0]);
			return 0;
		}

		return check_and_send_bb(src, item, src->cur_area, req, csr);
	}

	/* Figure out what type to drop... */
	rnd = sfmt_genrand_uint32(rng) % 3;

	//ITEM_LOG("GC %u 请求章节 %d 难度 %d 区域 %d ptID %d emptID %d 随机 %d 物品掉落", c->guildcard, l->episode, l->difficulty, c->cur_area, pt_index, ent->enemy_drop[pt_index], rnd);

	switch (rnd) {
	case 0:
		/* Drop the enemy's designated type of item. */
		switch (ent->enemy_item_classes[ep_pt_index]) {
		case BOX_TYPE_WEAPON:
			/* Drop a weapon */
			if (generate_weapon_bb(ent, area, item, rng, 0, l)) {
				return 0;
			}

			return check_and_send_bb(src, item, src->cur_area, req, csr);

		case BOX_TYPE_ARMOR:
			/* Drop an armor */
			if (generate_armor_bb(ent, area, item, rng, 0, l)) {
				return 0;
			}

			return check_and_send_bb(src, item, src->cur_area, req, csr);

		case BOX_TYPE_SHIELD:
			/* Drop a shield */
			if (generate_shield_bb(ent, area, item, rng, 0, l)) {
				return 0;
			}

			return check_and_send_bb(src, item, src->cur_area, req, csr);

		case BOX_TYPE_UNIT:
			/* Drop a unit */
			if (pmt_random_unit_bb(ent->unit_maxes[area], item, rng, l)) {
				return 0;
			}

			return check_and_send_bb(src, item, src->cur_area, req, csr);

		case -1:
			/* This shouldn't happen, but if it does, don't drop
			   anything at all. */
			return 0;

		case 0xFF:
			/* 啥也没有. */
			return 0;

		default:
			ITEM_LOG("未知/无效怪物掉落 (%d) 索引 "
				"%d", ent->enemy_item_classes[ep_pt_index],
				pt_index);
			return 0;
		}

		break;

	case 1:
		/* Drop a tool */
		if (generate_tool_bb(ent, area, item, rng, l)) {
			return 0;
		}

		return check_and_send_bb(src, item, src->cur_area, req, csr);

	case 2:
		/* Drop meseta */
		if (generate_meseta(ent->enemy_meseta_ranges[ep_pt_index].min,
			ent->enemy_meseta_ranges[ep_pt_index].max,
			item, rng, l)) {
			return 0;
		}

		return check_and_send_bb(src, item, src->cur_area, req, csr);
	}

	/* Shouldn't ever get here... */
	return 0;
}

int pt_generate_bb_pso2_boxdrop(ship_client_t* src, lobby_t* l, uint8_t section, subcmd_bb_itemreq_t* req) {
	uint16_t obj_id;
	game_object_t* gobj;
	map_object_t* obj;
	uint32_t rnd, t1, t2;
	int area, do_rare = 1;
	uint32_t item[4] = { 0 };
	float f1, f2;
	sfmt_t* rng = &src->sfmt_rng;
	int csr = 0;
	uint8_t pt_index = req->pt_index;

	pt_bb_entry_t* ent = get_pt_data_bb(l->episode, l->challenge, l->difficulty, section);
	if (!ent) {
		ERR_LOG("%s Item_PT 不存在难度 %d 颜色 %d 的掉落", client_type[src->version].ver_name, l->difficulty, section);
		return 0;
	}

	ITEM_LOG("GC %u 请求章节 %d 难度 %d 物品 %d 颜色掉落", src->guildcard, l->episode, l->difficulty, section);

	/* Make sure this is actually a box drop... */
	if (pt_index != 0x30)
		return -1;

	/* Grab the object ID and make sure its sane, then grab the object itself */
	obj_id = LE16(req->entity_id);
	if (obj_id > l->map_objs->obj_count) {
		ITEM_LOG("GC %u 请求的箱子掉落无效",
			src->guildcard);
		return -1;
	}

	/* Don't bother if the box has already been opened */
	gobj = &l->map_objs->objs[obj_id];
	if (gobj->flags & 0x00000001) {
		DBG_LOG("箱子已经打开过一次了");
		//return 0;
	}

	obj = &gobj->mobj_data;

	/* Figure out the area we'll be worried with */
	area = get_pt_data_area_bb(l->episode, src->cur_area);

	/* Mark the box as spent now... */
	gobj->flags |= 0x00000001;

	/* See if we'll do a rare roll. */
	if (l->qid) {
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_RARES))
			do_rare = 0;
		if (!(ship->cfg->local_flags & PSOCN_SHIP_QUEST_SRARES))
			csr = 1;
	}

	/* See if the object is fixed-type box */
	t1 = LE32(obj->dword[0]);
	memcpy(&f1, &t1, sizeof(float));
	if ((obj->base_type == LE32(0x00000092) || obj->base_type == LE32(0x00000161)) &&
		f1 < 1.0f + EPSILON && f1 > 1.0f - EPSILON) {
		/* See if it is a fully-fixed item */
		t2 = LE32(obj->dword[1]);
		memcpy(&f2, &t2, sizeof(float));

		if (f2 < 1.0f + EPSILON && f2 > 1.0f - EPSILON) {
			/* Drop the requested item */
			item[0] = ntohl(obj->dword[2]);
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { 0 };
			if (!create_tmp_item(item, ARRAYSIZE(item), &tmp_item)) {
				ERR_LOG("预设物品参数失败");
				return 0;
			}

			/* If its a stackable item, make sure to give it a quantity of 1 */
			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);

			/* This will make the meseta boxes for Vol Opt work... */
			if (item[0] == 0x00000004) {
				t1 = LE32(obj->dword[3]) >> 16;
				item[3] = t1 * 10;
			}

			if (item_not_identification_bb(item[0], item[1])) {
				return 0;
			}

			return check_and_send_bb(src, item, src->cur_area,
				(subcmd_bb_itemreq_t*)req, csr);
		}

		t1 = ntohl(obj->dword[2]);
		switch (t1 & 0xFF) {
		case ITEM_TYPE_WEAPON:
			goto generate_weapon;

		case ITEM_TYPE_GUARD:
			goto generate_armor;

		case ITEM_TYPE_TOOL:
			goto generate_tool;

		case ITEM_TYPE_MESETA:
			goto generate_meseta;

		default:
			ITEM_LOG("Invalid type detected from fixed-type box!");
			return 0;
		}
	}

	item[0] = rt_generate_bb_rare(src, l, -1, area + 1, section);
	/* See if the user is lucky today... */
	if (do_rare && item[0] && !item_not_identification_bb(item[0], item[1])) {
		switch (item[0] & 0xFF) {
		case ITEM_TYPE_WEAPON:
			/* Weapon -- add percentages and (potentially) grind values and
			   such... */
			if (generate_weapon_bb(ent, area, item, rng, 1, l))
				return 0;
			break;

		case ITEM_TYPE_GUARD:
			/* Armor/Shield/Unit */
			switch ((item[0] >> 8) & 0xFF) {
			case ITEM_SUBTYPE_FRAME:
				/* Armor -- Add DFP/EVP boosts and slots */
				if (generate_armor_bb(ent, area, item, rng, 1, l))
					return 0;
				break;

			case ITEM_SUBTYPE_BARRIER:
				/* Shield -- Add DFP/EVP boosts */
				if (generate_shield_bb(ent, area, item, rng, 1, l))
					return 0;
				break;

			case ITEM_SUBTYPE_UNIT:
				/* Unit -- Nothing to do here */
				break;

			default:
				ITEM_LOG("ItemRT 生成了一个无效的物品: "
					"%08X", item[0]);
				return 0;
			}

			break;

		case ITEM_TYPE_MAG:
			/* Mag -- Give it 5 DFP and 40% Synchro and an unset color */
			item[0] = 0x00050002;
			item[1] = 0x000001F4;
			item[2] = 0x00000000;
			item[3] = 0xFF000028;
			break;

		case ITEM_TYPE_TOOL:
			/* Tool -- Give it a quantity of 1 if its stackable. */
			item[1] = item[2] = item[3] = 0;

			item_t tmp_item = { 0 };
			if (!create_tmp_item(item, ARRAYSIZE(item), &tmp_item)) {
				ERR_LOG("预设物品参数失败");
				return 0;
			}

			if (is_stackable(&tmp_item))
				item[1] = (1 << 8);
			break;

		default:
			ITEM_LOG("ItemRT 生成了一个无效的物品: %08X",
				item[0]);
			return 0;
		}

		return check_and_send_bb(src, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}

	/* Generate an item, according to the PT data */
	rnd = sfmt_genrand_uint32(rng) % 100;

	if ((rnd -= ent->box_drop[BOX_TYPE_WEAPON][area]) > 100) {
	generate_weapon:
		/* Generate a weapon */
		if (generate_weapon_bb(ent, area, item, rng, 0, l))
			return 0;

		return check_and_send_bb(src, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_ARMOR][area]) > 100) {
	generate_armor:
		/* Generate an armor */
		if (generate_armor_bb(ent, area, item, rng, 0, l))
			return 0;

		return check_and_send_bb(src, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_SHIELD][area]) > 100) {
		/* Generate a shield */
		if (generate_shield_bb(ent, area, item, rng, 0, l))
			return 0;

		return check_and_send_bb(src, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_UNIT][area]) > 100) {
		/* Generate a unit */
		if (pmt_random_unit_bb(ent->unit_maxes[area], item, rng, l))
			return 0;

		return check_and_send_bb(src, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_TOOL][area]) > 100) {
	generate_tool:
		/* Generate a tool */
		if (generate_tool_bb(ent, area, item, rng, l))
			return 0;

		return check_and_send_bb(src, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}
	else if ((rnd -= ent->box_drop[BOX_TYPE_MESETA][area]) > 100) {
	generate_meseta:
		/* Generate money! */
		if (generate_meseta(ent->box_meseta_ranges[area].min, ent->box_meseta_ranges[area].max,
			item, rng, l))
			return 0;

		return check_and_send_bb(src, item, src->cur_area,
			(subcmd_bb_itemreq_t*)req, csr);
	}

	/* You get nothing! */
	return 0;
}

int pt_generate_bb_pso2_drop(ship_client_t* src, lobby_t* l, void* r) {
	subcmd_bb_itemreq_t* req = (subcmd_bb_itemreq_t*)r;
	uint8_t section;

	for (int i = 0; i < l->max_clients; ++i) {
		if (l->clients[i]) {
			/* If we're supposed to check the ignore list, and this client is on
			   it, don't send the packet. */
			if (client_has_ignored(l->clients[i], src->guildcard)) {
				continue;
			}

			section = l->clients[i]->bb_pl->character.dress_data.section;

			DBG_LOG("%d", section);

			pt_generate_bb_pso2_drop_style(l->clients[i], l, section, req);
		}
	}

	return 0;
}

uint16_t get_random_value(sfmt_t* rng, rang_16bit_t range) {
	uint16_t random_value;
	uint16_t value_range = range.max - range.min + 1;

	// 生成 [0, value_range) 范围内的随机数
	random_value = rand_int(rng, value_range);

	// 将随机数加上 min 得到最终的区间随机值
	return random_value + range.min;
}