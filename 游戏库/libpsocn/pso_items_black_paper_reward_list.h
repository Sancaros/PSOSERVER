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

typedef struct bp_reward_list {
	uint32_t reward[25];
}bp_reward_list_t;

/* 拉比 */
static const bp_reward_list_t bp1_rappy[4] = {
	//普通
	{
	BBItem_BUNNY_EARS,
	BBItem_TRIPOLIC_SHIELD,
	BBItem_SMOKING_PLATE,
	BBItem_God_Power,
	BBItem_God_Arm,
	BBItem_Rappys_Beak,
	BBItem_RABBIT_WAND,
	BBItem_Resist_Storm,
	BBItem_Resist_Devil,
	BBItem_Resist_Holy,
	BBItem_Resist_Burning,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//困难
	{
	BBItem_CAT_EARS,
	BBItem_INVISIBLE_GUARD,
	BBItem_YATA_MIRROR,
	BBItem_RED_COAT,
	BBItem_Cure_Slow,
	BBItem_Cure_Freeze,
	BBItem_Cure_Confuse,
	BBItem_Cure_Shock,
	BBItem_Rappys_Beak,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极难
	{
	BBItem_TYRELLS_PARASOL,
	BBItem_MADAMS_PARASOL,
	BBItem_HITOGATA,
	BBItem_KASAMI_BRACER,
	BBItem_WINDMILL, 0x060B00, 0x060A00, 0x040A00,
	BBItem_RABBIT_WAND, 0x2300, 0x3B00, BBItem_Rappys_Beak,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极限
	{
	0x5100, BBItem_DIVINE_PROTECTION, 0x200301, 0x3E0301,
	0x290201, BBItem_Rappys_Beak, 0x040B00, 0x060A00,
	0x5600, 0x3B00, 0x2300, 0x050A00,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	}
};

/* 火犀鸟 */
static const bp_reward_list_t bp1_zu[4] = {
	//普通
	{
	0x320101, 0x012F00, 0xB300, 0x5E00,
	0x020E00, 0x2E00, 0x9500, 0x9A00,
	0x2F00, 0x1B0301,
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
	0xAA00, 0x410101, 0x510101, 0x230201,
	0x3F00, 0x4100, 0x070500, 0x060500,
	0x050500, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极限
	{
	0xAF00, 0x4300, BBItem_SMARTLINK, 0xCD00,
	0x9900, 0x6C00, 0x4500, 0x6B00,
	0x1200, 0x6500, 0x290201, 0x1300,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	}
};

/* 铁甲巨象 */
static const bp_reward_list_t bp1_dorphon[4] = {
	//普通
	{
	BBItem_DBS_SABER_3062, BBItem_DBS_SABER_3067, BBItem_DBS_SABER_3069_2, BBItem_DBS_SABER_3064,
	BBItem_DBS_SABER_3069_4, BBItem_DBS_SABER_3073, BBItem_DBS_SABER_3070, BBItem_DBS_SABER_3075,
	BBItem_DBS_SABER_3077, BBItem_KUSANAGI, 0x4E0101, 0x070301,
	0x410301,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//困难
	{
	BBItem_FLAMBERGE, BBItem_RED_SWORD, BBItem_Spread, BBItem_DBS_SABER_3069_2,
	BBItem_DBS_SABER_3075, 0x2C00, 0x2D00, 0x350201,
	0x060100, 0x050100,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极难
	{
	0xB600, 0x018A00, 0x011000, 0x021000,
	0x031000, 0x041000, 0x051000, 0x061000,
	0x2700, 0x070100,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	},
	//极限
	{
	0xB700, 0x011000, 0x021000, 0x031000,
	0x041000, 0x051000, 0x061000, 0x2900,
	0x8A00, 0x028A00,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	}
};

/* 黑页2 */
static const bp_reward_list_t bp2[4] = {
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

