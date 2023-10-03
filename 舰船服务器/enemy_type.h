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

#include "mapdata.h"

typedef enum {
	ENEMY_UNKNOWN = -1,
	ENEMY_NONE = 0,
	ENEMY_AL_RAPPY,
	ENEMY_ASTARK,
	ENEMY_BA_BOOTA,
	ENEMY_BARBA_RAY,
	ENEMY_BARBAROUS_WOLF,
	ENEMY_BEE_L,
	ENEMY_BEE_R,
	ENEMY_BOOMA,
	ENEMY_BOOTA,
	ENEMY_BULCLAW,
	ENEMY_CANADINE,
	ENEMY_CANADINE_GROUP,
	ENEMY_CANANE,
	ENEMY_CHAOS_BRINGER,
	ENEMY_CHAOS_SORCERER,
	ENEMY_CLAW,
	ENEMY_DARK_BELRA,
	ENEMY_DARK_FALZ_1,
	ENEMY_DARK_FALZ_2,
	ENEMY_DARK_FALZ_3,
	ENEMY_DARK_GUNNER,
	ENEMY_DARVANT,
	ENEMY_DARVANT_ULTIMATE,
	ENEMY_DE_ROL_LE,
	ENEMY_DE_ROL_LE_BODY,
	ENEMY_DE_ROL_LE_MINE,
	ENEMY_DEATH_GUNNER,
	ENEMY_DEL_LILY,
	ENEMY_DEL_RAPPY,
	ENEMY_DEL_RAPPY_ALT,
	ENEMY_DELBITER,
	ENEMY_DELDEPTH,
	ENEMY_DELSABER,
	ENEMY_DIMENIAN,
	ENEMY_DOLMDARL,
	ENEMY_DOLMOLM,
	ENEMY_DORPHON,
	ENEMY_DORPHON_ECLAIR,
	ENEMY_DRAGON,
	ENEMY_DUBCHIC,
	ENEMY_DUBWITCH, // Has no entry in battle params
	ENEMY_EGG_RAPPY,
	ENEMY_EPSIGUARD,
	ENEMY_EPSILON,
	ENEMY_EVIL_SHARK,
	ENEMY_GAEL,
	ENEMY_GAL_GRYPHON,
	ENEMY_GARANZ,
	ENEMY_GEE,
	ENEMY_GI_GUE,
	ENEMY_GIBBLES,
	ENEMY_GIGOBOOMA,
	ENEMY_GILLCHIC,
	ENEMY_GIRTABLULU,
	ENEMY_GOBOOMA,
	ENEMY_GOL_DRAGON,
	ENEMY_GORAN,
	ENEMY_GORAN_DETONATOR,
	ENEMY_GRASS_ASSASSIN,
	ENEMY_GUIL_SHARK,
	ENEMY_HALLO_RAPPY,
	ENEMY_HIDOOM,
	ENEMY_HILDEBEAR,
	ENEMY_HILDEBLUE,
	ENEMY_ILL_GILL,
	ENEMY_KONDRIEU,
	ENEMY_LA_DIMENIAN,
	ENEMY_LOVE_RAPPY,
	ENEMY_MERICAROL,
	ENEMY_MERICUS,
	ENEMY_MERIKLE,
	ENEMY_MERILLIA,
	ENEMY_MERILTAS,
	ENEMY_MERISSA_A,
	ENEMY_MERISSA_AA,
	ENEMY_MIGIUM,
	ENEMY_MONEST,
	ENEMY_MORFOS,
	ENEMY_MOTHMANT,
	ENEMY_NANO_DRAGON,
	ENEMY_NAR_LILY,
	ENEMY_OLGA_FLOW_1,
	ENEMY_OLGA_FLOW_2,
	ENEMY_PAL_SHARK,
	ENEMY_PAN_ARMS,
	ENEMY_PAZUZU,
	ENEMY_PAZUZU_ALT,
	ENEMY_PIG_RAY,
	ENEMY_POFUILLY_SLIME,
	ENEMY_POUILLY_SLIME,
	ENEMY_POISON_LILY,
	ENEMY_PYRO_GORAN,
	ENEMY_RAG_RAPPY,
	ENEMY_RECOBOX,
	ENEMY_RECON,
	ENEMY_SAINT_MILLION,
	ENEMY_SAINT_RAPPY,
	ENEMY_SAND_RAPPY,
	ENEMY_SAND_RAPPY_ALT,
	ENEMY_SATELLITE_LIZARD,
	ENEMY_SATELLITE_LIZARD_ALT,
	ENEMY_SAVAGE_WOLF,
	ENEMY_SHAMBERTIN,
	ENEMY_SINOW_BEAT,
	ENEMY_SINOW_BERILL,
	ENEMY_SINOW_GOLD,
	ENEMY_SINOW_SPIGELL,
	ENEMY_SINOW_ZELE,
	ENEMY_SINOW_ZOA,
	ENEMY_SO_DIMENIAN,
	ENEMY_UL_GIBBON,
	ENEMY_VOL_OPT_1,
	ENEMY_VOL_OPT_2,
	ENEMY_VOL_OPT_AMP,
	ENEMY_VOL_OPT_CORE,
	ENEMY_VOL_OPT_MONITOR,
	ENEMY_VOL_OPT_PILLAR,
	ENEMY_YOWIE,
	ENEMY_YOWIE_ALT,
	ENEMY_ZE_BOOTA,
	ENEMY_ZOL_GIBBON,
	ENEMY_ZU,
	ENEMY_ZU_ALT,
	ENEMY_MAX
} EnemyType;

const static char* mobnames[] = {
						"N/A", "Hildebear", "Hildeblue", "Mothmant", "Monest", "Rag Rappy", "Al Rappy", "Savage Wolf", "Barbarous Wolf", "Booma",
						"Gobooma", "Gigobooma", "Grass Assassin", "Poison Lily", "Nar Lily", "Nano Dragon", "Evil Shark", "Pal Shark", "Guil Shark", "Pofuilly Slime",
						"Pouilly Slime", "Pan Arms", "Migium", "Hidoom", "Dubchic", "Garanz", "Sinow Beat", "Sinow Gold", "Canadine", "Canane",
						"Delsaber", "Chaos Sorcerer", "Bee R", "Bee L", "Dark Gunner", "Death Gunner", "Chaos Bringer", "Dark Belra", "Claw", "Bulk",
						"Bulclaw", "Dimenian", "La Dimenian", "So Dimenian", "Dragon", "De Rol Le", "Vol Opt", "Dark Falz", "Container", "Dubwitch",
						"Gillchic", "Love Rappy", "Merillia", "Meriltas", "Gee", "Gi Gue", "Mericarol", "Merikle", "Mericus", "Ul Gibbon",
						"Zol Gibbon", "Gibbles", "Sinow Berill", "Sinow Spigell", "Dolmolm", "Dolmdarl", "Morfos", "Recobox", "Recon", "Sinow Zoa",
						"Sinow Zele", "Deldepth", "Delbiter", "Barba Ray", "Pig Ray", "Ul Ray", "Gol Dragon", "Gal Gryphon", "Olga Flow", "St Rappy",
						"Hallo Rappy", "Egg Rappy", "Ill Gill", "Del Lily", "Epsilon", "Gael", "Giel", "Epsigard", "N/A", "N/A"
						"N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A"
};

const static char* mobnames_ult[] = {
						"N/A", "Hildelt", "Hildetorr", "Mothvert", "Mothvist", "El Rappy", "Pal Rappy", "Gulgus", "Gulgus-gue", "Bartle",
						"Barble", "Tollaw", "Crimson Assassin", "Ob Lily", "Mil Lily", "Nano Dragon", "Vulmer", "Govulmer", "Melqueek", "Pofuilly Slime",
						"Pouilly Slime", "Pan Arms", "Migium", "Hidoom", "Dubchich", "Baranz", "Sinow Blue", "Sinow Red", "Canabin", "Canune",
						"Delsaber", "Gran Sorcerer", "Gee R", "Gee L", "Dark Gunner", "Death Gunner", "Dark Bringer", "Indi Belra", "Claw", "Bulk",
						"Bulclaw", "Arlan", "Merlan", "Del-D", "Sil Dragon", "Dal Ra Lie", "Vol Opt ver.2", "Dark Falz", "Container", "Duvuik",
						"Gillchich", "Love Rappy", "Merillia", "Meriltas", "Gee", "Gi Gue", "Mericarol", "Merikle", "Mericus", "Ul Gibbon",
						"Zol Gibbon", "Gibbles", "Sinow Berill", "Sinow Spigell", "Dolmolm", "Dolmdarl", "Morfos", "Recobox", "Recon", "Sinow Zoa",
						"Sinow Zele", "Deldepth", "Delbiter", "Barba Ray", "Pig Ray", "Ul Ray", "Gol Dragon", "Gal Gryphon", "Olga Flow", "St Rappy",
						"Hallo Rappy", "Egg Rappy", "Ill Gill", "Del Lily", "Epsilon", "Gael", "Giel", "Epsigard", "N/A", "N/A",
						"N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A"
};


const static char* mobnames_ep4[] = {
						  "N/A","Astark", "Yowie", "Satellite Lizard", "Merissa A", "Merissa AA", "Girtablulu", "Zu", "Pazuzu", "Boota", "Za Boota",
						  "Ba Boota", "Dorphon", "Dorphon Eclair", "Goran", "Goran Detonator", "Pyro Goran", "Sand Rappy", "Del Rappy", "Saint Million",
						  "Shambertin", "Kondrieu", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
};

const static char* mobnames_cn[] = {
							"N/A", "狂暴巨猿", "狂暴白猿", "吸血巨蚊", "吸血巨蚊巢", "黄色拉比鸟", "蓝色拉比鸟", "狂狼", "狂狼王", "棕狂熊",
							"黄狂熊", "紫狂熊", "巨螳螂", "毒铃兰", "红色铃兰", "翼龙", "绿鲨魔", "紫鲨魔", "狂鲨魔", "冰史莱姆",
							"炎史莱姆", "合体怪", "冰剑怪", "火剑怪", "傀儡机兵", "机甲堡垒", "蓝色机甲忍者", "金色机甲忍者", "电爪", "核心电爪",
							"剑魔", "混沌法师", "暗之水晶", "圣之水晶", "暗黑炮台", "死亡炮台", "混沌骑士", "暗黑巨神像", "爪虫", "合体爪虫",
							"合体爪魔", "刀魔兵", "刀魔将", "刀魔王", "赤焰巨龙", "迪 洛尔 雷", "波鲁欧普", "暗黑佛", "禁锢之钟", "傀儡控制器",
							"步行机兵", "爱之拉比鸟", "毒梅兰", "狂梅兰", "姬蜂", "姬 蜂后", "红色蝎花", "蓝色蝎花", "绿色蝎花", "幽猿",
							"泽猿", "蛮甲猿", "森隐雷藏", "雾隐雷藏", "青鱿鱼", "赤鱿鱼", "暗腐蝶", "爆防魔箱", "猫型怪", "湛蓝光武",
							"绯红光武", "暗黑魔精", "暗黑麒麟", "巴尔巴雷", "碧涟虫", "幽涟虫", "数码暴龙", "加尔狮鹫", "奥尔伽 弗罗文", "圣诞拉比鸟",
							"南瓜拉比鸟", "彩蛋拉比鸟", "恶镰死神", "暗铃兰", "厄普西隆", "堕天灵", "怵天灵", "厄普西盾", "N/A", "N/A",
							"N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A"
};
const static char* mobnames_ult_cn[] = {
							"N/A", "雷爆巨猿", "暗黑巨猿", "嗜血巨蚊", "嗜血巨蚊巢", "白拉比鸟", "七彩拉比鸟", "角狼", "角狼王", "棕魔龟",
							"黄魔龟", "魔螳螂", "嗜血巨螳螂", "变种铃兰", "异种铃兰", "强化翼龙", "棕鲨熊", "紫鲨熊", "变种鲨熊", "冰史莱姆",
							"炎史莱姆", "强化合体怪", "冰剑怪", "火剑怪", "傀儡机将", "超机甲堡垒", "紫色机甲忍者", "红色机甲忍者", "强化电爪",
							"强化核心电爪", "暗黑魔剑士", "毁灭法师", "暗之水晶", "圣之水晶", "强化暗黑炮台", "强化死亡炮台", "暗黑骑士", "暗黑远古巨神像",
							"爪虫", "合体爪虫", "合体爪魔", "异魔兵", "异魔将", "异魔王", "冰霜巨龙", "达尔 拉 利", "波鲁欧普 改", "暗黑佛",
							"禁锢之钟", "傀儡控制器", "浮游机将", "爱之拉比鸟", "强化毒梅兰", "强化狂梅兰", "姬蜂", "姬 蜂后", "致命红蝎花", "蓝狂蝎花",
							"地狱绿蝎花", "雷爆幽猿", "暗之泽猿", "超蛮甲猿", "强化森隐雷藏", "强化雾隐雷藏", "变异青鱿鱼", "变异赤鱿鱼", "强化暗腐蝶",
							"强化爆防机", "猫型怪", "强化蓝光武", "强化红光武", "暗黑魔精", "暗黑魔麒麟", "巴尔巴雷", "碧涟虫", "幽涟虫", "强化数码暴龙",
							"野生加尔狮鹫", "奥尔伽 弗罗文", "圣诞拉比鸟", "南瓜拉比鸟", "彩蛋拉比鸟", "恶镰死神", "暗铃兰", "厄普西隆", "堕天灵", "怵天灵", "厄普西盾", "N/A", "N/A",
							"N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A"
};

const static char* mobnames_ep4_cn[] = {
						  "N/A", "异甲巨螳螂", "砂岩蜥蜴", "沙暴蜥蜴", "媚影沙魔 Ａ", "媚影沙魔 ＡＡ", "暗黑魔眼巨花", "巨型火犀鸟", "变异羚角鸟", "变异猪", "狂影变异猪",
						  "巨角变异猪", "铁甲巨象", "暗黑铁甲巨象", "暗黑刀魔", "暗黑巨魔王", "暗黑狂刀魔", "沙地拉比鸟", "暗黑拉比鸟", "圣魔龙",
						  "沙柏龙", "空间龙", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
						  "N/A", "N/A", "N/A", "N/A" };

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

char enemy_desc[128];

const char* name_for_enum(EnemyType type);

char* enemy_str(game_enemy_t* enemy);

EnemyType name_for_enemy_type(const char* name);

uint8_t rare_table_index_for_enemy_type(EnemyType enemy_type);

bool enemy_type_valid_for_episode(Episode episode, EnemyType enemy_type);

uint8_t battle_param_index_for_enemy_type(Episode episode, EnemyType enemy_type);

const char* get_enemy_name(uint8_t episode, uint8_t difficulty, int rt_index);

#endif // !ENEMY_TYPE_H