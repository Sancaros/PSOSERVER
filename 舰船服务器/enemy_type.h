/*
	梦幻之星中国 舰船服务器 敌人类型
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

#ifndef ENEMY_TYPE_H
#define ENEMY_TYPE_H

typedef enum {
	UNKNOWN = -1,
	NONE = 0,
	AL_RAPPY,
	ASTARK,
	BA_BOOTA,
	BARBA_RAY,
	BARBAROUS_WOLF,
	BEE_L,
	BEE_R,
	BOOMA,
	BOOTA,
	BULCLAW,
	CANADINE,
	CANADINE_GROUP,
	CANANE,
	CHAOS_BRINGER,
	CHAOS_SORCERER,
	CLAW,
	DARK_BELRA,
	DARK_FALZ_1,
	DARK_FALZ_2,
	DARK_FALZ_3,
	DARK_GUNNER,
	DARVANT,
	DARVANT_ULTIMATE,
	DE_ROL_LE,
	DE_ROL_LE_BODY,
	DE_ROL_LE_MINE,
	DEATH_GUNNER,
	DEL_LILY,
	DEL_RAPPY,
	DEL_RAPPY_ALT,
	DELBITER,
	DELDEPTH,
	DELSABER,
	DIMENIAN,
	DOLMDARL,
	DOLMOLM,
	DORPHON,
	DORPHON_ECLAIR,
	DRAGON,
	DUBCHIC,
	DUBWITCH,// Has no entry in battle params
	EGG_RAPPY,
	EPSIGUARD,
	EPSILON,
	EVIL_SHARK,
	GAEL,
	GAL_GRYPHON,
	GARANZ,
	GEE,
	GI_GUE,
	GIBBLES,
	GIGOBOOMA,
	GILLCHIC,
	GIRTABLULU,
	GOBOOMA,
	GOL_DRAGON,
	GORAN,
	GORAN_DETONATOR,
	GRASS_ASSASSIN,
	GUIL_SHARK,
	HALLO_RAPPY,
	HIDOOM,
	HILDEBEAR,
	HILDEBLUE,
	ILL_GILL,
	KONDRIEU,
	LA_DIMENIAN,
	LOVE_RAPPY,
	MERICAROL,
	MERICUS,
	MERIKLE,
	MERILLIA,
	MERILTAS,
	MERISSA_A,
	MERISSA_AA,
	MIGIUM,
	MONEST,
	MORFOS,
	MOTHMANT,
	NANO_DRAGON,
	NAR_LILY,
	OLGA_FLOW_1,
	OLGA_FLOW_2,
	PAL_SHARK,
	PAN_ARMS,
	PAZUZU,
	PAZUZU_ALT,
	PIG_RAY,
	POFUILLY_SLIME,
	POUILLY_SLIME,
	POISON_LILY,
	PYRO_GORAN,
	RAG_RAPPY,
	RECOBOX,
	RECON,
	SAINT_MILLION,
	SAINT_RAPPY,
	SAND_RAPPY,
	SAND_RAPPY_ALT,
	SATELLITE_LIZARD,
	SATELLITE_LIZARD_ALT,
	SAVAGE_WOLF,
	SHAMBERTIN,
	SINOW_BEAT,
	SINOW_BERILL,
	SINOW_GOLD,
	SINOW_SPIGELL,
	SINOW_ZELE,
	SINOW_ZOA,
	SO_DIMENIAN,
	UL_GIBBON,
	VOL_OPT_1,
	VOL_OPT_2,
	VOL_OPT_AMP,
	VOL_OPT_CORE,
	VOL_OPT_MONITOR,
	VOL_OPT_PILLAR,
	YOWIE,
	YOWIE_ALT,
	ZE_BOOTA,
	ZOL_GIBBON,
	ZU,
	ZU_ALT,
	MAX_ENEMY_TYPE,
} EnemyType;

typedef struct {
	const char* name;
	EnemyType type;
} EnemyTypeMapping;

typedef enum{
	NORMAL = 0,
	EP1 = 1,
	EP2 = 2,
	EP3 = 3,
	EP4 = 4,
} Episode;

const char* name_for_enum(EnemyType type);

EnemyType name_for_enemy_type(const char* name);

bool enemy_type_valid_for_episode(Episode episode, EnemyType enemy_type);

uint8_t battle_param_index_for_enemy_type(Episode episode, EnemyType enemy_type);

uint8_t rare_table_index_for_enemy_type(EnemyType enemy_type);

#endif // !ENEMY_TYPE_H