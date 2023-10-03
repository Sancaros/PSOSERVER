/*
	�λ�֮���й� ���������� ��������
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
							"N/A", "�񱩾�Գ", "�񱩰�Գ", "��Ѫ����", "��Ѫ���ó�", "��ɫ������", "��ɫ������", "����", "������", "�ؿ���",
							"�ƿ���", "�Ͽ���", "�����", "������", "��ɫ����", "����", "����ħ", "����ħ", "����ħ", "��ʷ��ķ",
							"��ʷ��ķ", "�����", "������", "�𽣹�", "���ܻ���", "���ױ���", "��ɫ��������", "��ɫ��������", "��צ", "���ĵ�צ",
							"��ħ", "���編ʦ", "��֮ˮ��", "ʥ֮ˮ��", "������̨", "������̨", "������ʿ", "���ھ�����", "צ��", "����צ��",
							"����צħ", "��ħ��", "��ħ��", "��ħ��", "�������", "�� ��� ��", "��³ŷ��", "���ڷ�", "����֮��", "���ܿ�����",
							"���л���", "��֮������", "��÷��", "��÷��", "����", "�� ���", "��ɫЫ��", "��ɫЫ��", "��ɫЫ��", "��Գ",
							"��Գ", "����Գ", "ɭ���ײ�", "�����ײ�", "������", "������", "������", "����ħ��", "è�͹�", "տ������",
							"糺����", "����ħ��", "��������", "�Ͷ�����", "������", "������", "���뱩��", "�Ӷ�ʨ��", "�¶�٤ ������", "ʥ��������",
							"�Ϲ�������", "�ʵ�������", "��������", "������", "������¡", "������", "������", "��������", "N/A", "N/A",
							"N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A"
};
const static char* mobnames_ult_cn[] = {
							"N/A", "�ױ���Գ", "���ھ�Գ", "��Ѫ����", "��Ѫ���ó�", "��������", "�߲�������", "����", "������", "��ħ��",
							"��ħ��", "ħ���", "��Ѫ�����", "��������", "��������", "ǿ������", "������", "������", "��������", "��ʷ��ķ",
							"��ʷ��ķ", "ǿ�������", "������", "�𽣹�", "���ܻ���", "�����ױ���", "��ɫ��������", "��ɫ��������", "ǿ����צ",
							"ǿ�����ĵ�צ", "����ħ��ʿ", "����ʦ", "��֮ˮ��", "ʥ֮ˮ��", "ǿ��������̨", "ǿ��������̨", "������ʿ", "����Զ�ž�����",
							"צ��", "����צ��", "����צħ", "��ħ��", "��ħ��", "��ħ��", "��˪����", "��� �� ��", "��³ŷ�� ��", "���ڷ�",
							"����֮��", "���ܿ�����", "���λ���", "��֮������", "ǿ����÷��", "ǿ����÷��", "����", "�� ���", "������Ы��", "����Ы��",
							"������Ы��", "�ױ���Գ", "��֮��Գ", "������Գ", "ǿ��ɭ���ײ�", "ǿ�������ײ�", "����������", "���������", "ǿ��������",
							"ǿ��������", "è�͹�", "ǿ��������", "ǿ�������", "����ħ��", "����ħ����", "�Ͷ�����", "������", "������", "ǿ�����뱩��",
							"Ұ���Ӷ�ʨ��", "�¶�٤ ������", "ʥ��������", "�Ϲ�������", "�ʵ�������", "��������", "������", "������¡", "������", "������", "��������", "N/A", "N/A",
							"N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A"
};

const static char* mobnames_ep4_cn[] = {
						  "N/A", "��׾����", "ɰ������", "ɳ������", "��Ӱɳħ ��", "��Ӱɳħ ����", "����ħ�۾޻�", "���ͻ�Ϭ��", "���������", "������", "��Ӱ������",
						  "�޽Ǳ�����", "���׾���", "�������׾���", "���ڵ�ħ", "���ھ�ħ��", "���ڿ�ħ", "ɳ��������", "����������", "ʥħ��",
						  "ɳ����", "�ռ���", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
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