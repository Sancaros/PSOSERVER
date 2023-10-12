/*
	梦幻之星中国 玩家数据结构
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

#include <f_iconv.h>

#include "f_logs.h"

#include "pso_player.h"
#include "pso_items.h"

/* Possible values for the version field of ship_client_t */
#define CLIENT_VERSION_DCV1     0
#define CLIENT_VERSION_DCV2     1
#define CLIENT_VERSION_PC       2
#define CLIENT_VERSION_GC       3
#define CLIENT_VERSION_EP3      4
#define CLIENT_VERSION_BB       5
#define CLIENT_VERSION_XBOX     6

bool char_class_is_male(uint8_t equip_flags) {
	return equip_flags & PLAYER_EQUIP_FLAGS_MALE;
}

bool char_class_is_female(uint8_t equip_flags) {
	return equip_flags & PLAYER_EQUIP_FLAGS_FEMALE;
}

bool char_class_is_human(uint8_t equip_flags) {
	return equip_flags & PLAYER_EQUIP_FLAGS_HUMAN;
}

bool char_class_is_newman(uint8_t equip_flags) {
	return equip_flags & PLAYER_EQUIP_FLAGS_NEWMAN;
}

bool char_class_is_android(uint8_t equip_flags) {
	return equip_flags & PLAYER_EQUIP_FLAGS_DROID;
}

bool char_class_is_hunter(uint8_t equip_flags) {
	return equip_flags & PLAYER_EQUIP_FLAGS_HUNTER;
}

bool char_class_is_ranger(uint8_t equip_flags) {
	return equip_flags & PLAYER_EQUIP_FLAGS_RANGER;
}

bool char_class_is_force(uint8_t equip_flags) {
	return equip_flags & PLAYER_EQUIP_FLAGS_FORCE;
}

int player_bb_name_cpy(psocn_bb_char_name_t* dst, psocn_bb_char_name_t* src) {
	removeWhitespace_w(src->char_name);
	size_t dst_name_len = sizeof(dst->char_name);
	size_t src_name_len = sizeof(src->char_name);

	dst->name_tag = src->name_tag;
	dst->name_tag2 = src->name_tag2;

	memcpy_s(&dst->char_name, dst_name_len, &src->char_name, src_name_len);

	if (!dst->char_name[0]) {
		ERR_LOG("玩家名称 %s 拷贝失败", src->char_name);
		return -1;
	}

	return 0;
}

char* get_player_name(player_t* pl, int version, bool raw) {

	if (!pl)
		return "玩家名称不存在";

	switch (version)
	{
	case CLIENT_VERSION_DCV1:
	case CLIENT_VERSION_DCV2:
	case CLIENT_VERSION_PC:
	case CLIENT_VERSION_GC:
	case CLIENT_VERSION_EP3:
	case CLIENT_VERSION_XBOX:
		istrncpy16_raw(ic_utf8_to_gb18030, player_name,
			&pl->v1.character.dress_data.gc_string, 24, 16);
		break;

	case CLIENT_VERSION_BB:
		removeWhitespace_w(pl->bb.character.name.char_name);
		if (raw)
			istrncpy16_raw(ic_utf16_to_gb18030, player_name,
				&pl->bb.character.name, 24, BB_CHARACTER_NAME_LENGTH);
		else
			istrncpy16_raw(ic_utf16_to_gb18030, player_name,
				&pl->bb.character.name.char_name[0], 24, BB_CHARACTER_CHAR_NAME_LENGTH);
		break;
	}

	if (player_name == NULL)
		ERR_LOG("GC %s 玩家名称为空", pl->v1.character.dress_data.gc_string);

	return player_name;
}

void set_technique_level(techniques_t* technique_levels_v1, inventory_t* inv, uint8_t which, uint8_t level) {
	if (level == TECHNIQUE_UNLEARN) {
		technique_levels_v1->all[which] = TECHNIQUE_UNLEARN;
		inv->iitems[which].extension_data1 = 0x00;
	}
	else if (level <= TECHNIQUE_V1_MAX_LEVEL) {
		technique_levels_v1->all[which] = level;
		inv->iitems[which].extension_data1 = 0x00;
	} else {
		technique_levels_v1->all[which] = TECHNIQUE_V1_MAX_LEVEL;
		inv->iitems[which].extension_data1 = level - TECHNIQUE_V1_MAX_LEVEL;
	}
}

uint8_t get_technique_level(techniques_t* technique_levels_v1, inventory_t* inv, uint8_t which) {
	return (technique_levels_v1->all[which] == TECHNIQUE_UNLEARN)
		? TECHNIQUE_UNLEARN
		: (technique_levels_v1->all[which] + inv->iitems[which].extension_data1);
}

uint8_t show_technique_level(psocn_bb_char_t* character, uint8_t which) {

	if (character->technique_levels_v1.all[which] == TECHNIQUE_UNLEARN)
		return TECHNIQUE_UNLEARN;

	switch (which) {
		/*这两个法术永远是1级*/
	case TECHNIQUE_RYUKER:
	case TECHNIQUE_REVERSER:
		return character->technique_levels_v1.all[which];

		/*这个法术最高7级*/
	case TECHNIQUE_ANTI:
		return character->technique_levels_v1.all[which];

	case TECHNIQUE_FOIE:
	case TECHNIQUE_GIFOIE:
	case TECHNIQUE_RAFOIE:
	case TECHNIQUE_BARTA:
	case TECHNIQUE_GIBARTA:
	case TECHNIQUE_RABARTA:
	case TECHNIQUE_ZONDE:
	case TECHNIQUE_GIZONDE:
	case TECHNIQUE_RAZONDE:
	case TECHNIQUE_GRANTS:
	case TECHNIQUE_DEBAND:
	case TECHNIQUE_JELLEN:
	case TECHNIQUE_ZALURE:
	case TECHNIQUE_SHIFTA:
	case TECHNIQUE_RESTA:
	case TECHNIQUE_MEGID:
		return character->technique_levels_v1.all[which] + character->inv.iitems[which].extension_data1;
	}

	return TECHNIQUE_UNLEARN;
}

uint8_t get_material_usage(inventory_t* inv, MaterialType which) {
	if (inv) {

		switch (which) {
		case MATERIAL_HP:
			return inv->hpmats_used;

		case MATERIAL_TP:
			return inv->tpmats_used;

		case MATERIAL_POWER:
		case MATERIAL_MIND:
		case MATERIAL_EVADE:
		case MATERIAL_DEF:
		case MATERIAL_LUCK:
			return inv->iitems[8 + (uint8_t)which].extension_data2;

		default:
			ERR_LOG("玩家吃药类型不支持 %d", which);
			return 0xFF;
		}
	}
	else {
		ERR_LOG("玩家数据不存在");
		return 0;
	}

}

void set_material_usage(inventory_t* inv, MaterialType which, uint8_t usage) {
	if (inv) {

		switch (which) {
		case MATERIAL_HP:
			inv->hpmats_used = usage;
			break;

		case MATERIAL_TP:
			inv->tpmats_used = usage;
			break;

		case MATERIAL_POWER:
		case MATERIAL_MIND:
		case MATERIAL_EVADE:
		case MATERIAL_DEF:
		case MATERIAL_LUCK:
			inv->iitems[8 + (uint8_t)which].extension_data2 = usage;
			break;

		default:
			ERR_LOG("玩家吃药类型不支持 %d", which);
		}
	}
	else {
		ERR_LOG("玩家数据不存在");
	}
}