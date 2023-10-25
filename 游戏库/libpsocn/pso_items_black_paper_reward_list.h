/*
	梦幻之星中国 舰船服务器 黑夜奖励物品列表
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

#ifndef BB_ITEMS_BP_REWARD_LIST
#define BB_ITEMS_BP_REWARD_LIST
#include <stdint.h>

#define BP_REWARD_BP1_DORPHON				0x00
#define BP_REWARD_BP1_RAPPY					0x01
#define BP_REWARD_BP1_ZU					0x02
#define BP_REWARD_BP2						0x04

typedef struct bp_reward_list {
	uint32_t reward;
}bp_reward_list_t;

/* 拉比 */
static bp_reward_list_t bp1_rappy[4][25] = {
	//普通
	{
	BBItem_BUNNY_EARS, BBItem_TRIPOLIC_SHIELD, BBItem_SMOKING_PLATE, BBItem_God_Power,
	BBItem_God_Arm, BBItem_Rappys_Beak, BBItem_RABBIT_WAND, BBItem_Resist_Storm,
	BBItem_Resist_Devil, BBItem_Resist_Holy, BBItem_Resist_Burning, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//困难
	{
	BBItem_CAT_EARS, BBItem_INVISIBLE_GUARD, BBItem_YATA_MIRROR, BBItem_RED_COAT,
	BBItem_Cure_Slow, BBItem_Cure_Freeze, BBItem_Cure_Confuse, BBItem_Cure_Shock,
	BBItem_Rappys_Beak,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极难
	{
	BBItem_TYRELLS_PARASOL, BBItem_MADAMS_PARASOL, BBItem_HITOGATA, BBItem_KASAMI_BRACER,
	BBItem_WINDMILL, BBItem_ALIVE_AQHU, BBItem_CLUB_OF_ZUMIURAN, BBItem_CLUB_OF_LACONIUM,
	BBItem_RABBIT_WAND, BBItem_STING_TIP, BBItem_MADAMS_UMBRELLA, BBItem_Rappys_Beak,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极限
	{
	BBItem_EVIL_CURST, BBItem_DIVINE_PROTECTION, BBItem_God_Ability, BBItem_God_Technique,
	BBItem_STANDSTILL_SHIELD, BBItem_Rappys_Beak, BBItem_BATTLE_VERGE, BBItem_CLUB_OF_ZUMIURAN,
	BBItem_PLANTAIN_LEAF, BBItem_MADAMS_UMBRELLA, BBItem_STING_TIP, BBItem_MACE_OF_ADAMAN,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	}
};

/* 火犀鸟 */
static bp_reward_list_t bp1_zu[4][25] = {
	//普通
	{
	BBItem_ELECTRO_FRAME, BBItem_BLACK_KING_BAR, BBItem_VIVIENNE, BBItem_TWIN_BLAZE,
	BBItem_TWIN_BRAND, BBItem_METEOR_CUDGEL, BBItem_PARTISAN_of_LIGHTNING, BBItem_DEMOLITION_COMET,
	BBItem_MONKEY_KING_BAR, BBItem_God_Body,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//困难
	{
	BBItem_CANNON_ROUGE, BBItem_ANO_BAZOOKA, BBItem_NUG2000_BAZOOKA, BBItem_FLAME_GARMENT,
	BBItem_PHOTON_LAUNCHER, BBItem_FINAL_IMPACT, BBItem_PANZER_FAUST, BBItem_MASER_BEAM,
	BBItem_FLAME_VISIT, BBItem_RED_SCORPIO,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极难
	{
	BBItem_SLICER_OF_FANATIC, BBItem_THIRTEEN, BBItem_INFANTRY_GEAR, BBItem_REGENE_GEAR_ADV,
	BBItem_FLIGHT_CUTTER, BBItem_RED_SLICER, BBItem_DISKA_OF_BRAVEMAN, BBItem_DISKA_OF_LIBERATOR,
	BBItem_SLICER_OF_ASSASSIN, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极限
	{
	BBItem_OPHELIE_SEIZE, BBItem_HANDGUN_MILLA, BBItem_SMARTLINK, BBItem_TANEGASHIMA,
	BBItem_ANGEL_HARP, BBItem_YASMINKOV_9000M, BBItem_FROZEN_SHOOTER, BBItem_YASMINKOV_7000V,
	BBItem_SPREAD_NEEDLE, BBItem_YASMINKOV_3000R, BBItem_STANDSTILL_SHIELD, BBItem_HOLY_RAY,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	}
};

/* 铁甲巨象 */
static bp_reward_list_t bp1_dorphon[4][25] = {
	//普通
	{
	BBItem_DBS_SABER_3062, BBItem_DBS_SABER_3067, BBItem_DBS_SABER_3069_2, BBItem_DBS_SABER_3064,
	BBItem_DBS_SABER_3069_4, BBItem_DBS_SABER_3073, BBItem_DBS_SABER_3070, BBItem_DBS_SABER_3075,
	BBItem_DBS_SABER_3077, BBItem_KUSANAGI, BBItem_OFFICER_UNIFORM, BBItem_God_Mind,
	BBItem_God_Battle,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//困难
	{
	BBItem_FLAMBERGE, BBItem_RED_SWORD, BBItem_Spread, BBItem_DBS_SABER_3069_2,
	BBItem_DBS_SABER_3075, BBItem_ELYSION, BBItem_RED_SABER, BBItem_SECURE_FEET,
	BBItem_KALADBOLG, BBItem_DBS_SABER,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极难
	{
	BBItem_GUREN, BBItem_YASHA, BBItem_AGITO_1975, BBItem_AGITO_1983,
	BBItem_AGITO_2001, BBItem_AGITO_1991, BBItem_AGITO_1977, BBItem_AGITO_1980,
	BBItem_ANCIENT_SABER, BBItem_DURANDAL,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极限
	{
	BBItem_SHOUREN, BBItem_AGITO_1975, BBItem_AGITO_1983, BBItem_AGITO_2001,
	BBItem_AGITO_1991, BBItem_AGITO_1977, BBItem_AGITO_1980, 0x2900,
	BBItem_SANGE, BBItem_KAMUI,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	}
};

/* 黑页2 */
static bp_reward_list_t bp2[4][25] = {
	//普通
	{
	BBItem_YUNCHANG, BBItem_PHOENIX_CLAW, BBItem_LAST_SWAN, BBItem_RIANOV_303SNR,
	BBItem_MASTER_RAVEN, BBItem_DECALOG, BBItem_Photon_Drop, BBItem_UNION_GUARD_95,
	BBItem_DF_SHIELD, BBItem_DE_ROL_LE_SHIELD
	},
	//困难
	{
	BBItem_SNAKE_SPIRE, BBItem_PHOENIX_CLAW, BBItem_SHOUREN, BBItem_MASTER_RAVEN,
	BBItem_RIANOV_303SNR, BBItem_DECALOG, BBItem_BLACK_HOUND_CUIRASS, BBItem_YATA_MIRROR,
	BBItem_STINK_SHIELD, BBItem_SMARTLINK, BBItem_Centurion_Ability, BBItem_DIVINE_PROTECTION,
	BBItem_Photon_Drop, BBItem_Liberta_Kit
	},
	//极难
	{
	BBItem_YUNCHANG, BBItem_KUSANAGI, BBItem_PHOENIX_CLAW, BBItem_GUREN,
	BBItem_VIVIENNE, BBItem_RIANOV_303SNR, BBItem_LAST_SWAN, BBItem_DECALOG,
	BBItem_BLACK_HOUND_CUIRASS, BBItem_YATA_MIRROR, BBItem_STINK_SHIELD, BBItem_GRATIA,
	BBItem_YASAKANI_MAGATAMA, BBItem_SMARTLINK, BBItem_Centurion_Ability, BBItem_DIVINE_PROTECTION,
	BBItem_Photon_Drop
	},
	//极限
	{
	BBItem_YUNCHANG, BBItem_KUSANAGI, BBItem_PHOENIX_CLAW, BBItem_GUREN,
	BBItem_VIVIENNE, BBItem_RIANOV_303SNR, BBItem_LAST_SWAN, BBItem_DECALOG,
	BBItem_BLACK_HOUND_CUIRASS, BBItem_YATA_MIRROR, BBItem_STINK_SHIELD, BBItem_GRATIA,
	BBItem_YASAKANI_MAGATAMA, BBItem_SMARTLINK, BBItem_Centurion_Ability, BBItem_DIVINE_PROTECTION
	}
};

#endif // !BB_ITEMS_BP_REWARD_LIST

