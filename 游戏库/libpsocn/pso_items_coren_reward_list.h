/*
	�λ�֮���й� ���������� ���׽�����Ʒ�б�
	��Ȩ (C) 2023 Sancaros

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

#ifndef COREN_REWARD_LISH_H
#define COREN_REWARD_LISH_H
#include <stdint.h>

/*[2021��12��22�� 02:35] �ػ�(15466) : ָ�� 0x"e2" δ����Ϸ���н��մ���. (��������)

	[2021��12��22�� 02:35] �ػ�(15466) :
	(0000) 18 00 62 00 00 00 00 00 E2 04 01 00 00 00 00 00 ..b..... ? ......
	(0010) F6 CC 0A C3 70 2F E9 42                         ��.�p / �B


	[2021��12��22�� 02:36] �ػ�(15466) : ָ�� 0x"e2" δ����Ϸ���н��մ���. (��������)

	[2021��12��22�� 02:36] �ػ�(15466) :
								   08 09 0A 0B 0C
	(0000) 18 00 62 00 00 00 00 00 E2 04 01 00 01 00 00 00 ..b..... ? ......
		   10 11 12 13 14 15 16 17
	(0010) F6 CC 0A C3 70 2F E9 42                         ��.�p / �B

	[2023��09��16�� 16:01:27:161] ����(block_bb.c 0517): GC 10000002:2 Ĭ�ϵ���ģʽ
( 00000000 )   18 00 62 00 00 00 00 00   E2 04 00 00 02 00 00 00  ..b.....?......
( 00000010 )   B3 9B 09 C3 13 E6 C0 42                           ��.?��B
		*/
/*
Note:
1.�ͻ��˻���ÿ����ɶĲ��۳����
2.��������Ʒ�������б���
3.����Ҫһ������ʧ�ܵ���Ӧ


*/
typedef struct Coren_Reward_List {
	int wday;
	uint32_t* rewards;
} Coren_Reward_List_t;

/* ѡ����� ûɶ�� */
static uint32_t menu_choice_price[3] = {
	1000, 10000, 100000
};

static uint32_t weekly_reward_percent[3][7][3] = {
	//1000 ������
	{
		{20, 0, 0},  // Day 7
		{20, 0, 0},  // Day 1����һ����Ʒ����Ϊ4%���ڶ�����Ʒ����Ϊ0%����������Ʒ����Ϊ0%
		{20, 0, 0},  // Day 2
		{20, 0, 0},  // Day 3
		{20, 0, 0},  // Day 4
		{20, 0, 0},  // Day 5
		{20, 0, 0}   // Day 6
	},
	//10000 ������
	{
		{40, 20, 0},  // Day 7
		{40, 20, 0},  // Day 1����һ����Ʒ����Ϊ8%���ڶ�����Ʒ����Ϊ8%����������Ʒ����Ϊ0%
		{40, 20, 0},  // Day 2
		{40, 20, 0},  // Day 3
		{40, 20, 0},  // Day 4
		{40, 20, 0},  // Day 5
		{40, 20, 0}   // Day 6
	},
	//100000 ������
	{
		{60, 40, 20},  // Day 7
		{60, 40, 20},  // Day 1����һ����Ʒ����Ϊ12%���ڶ�����Ʒ����Ϊ8%����������Ʒ����Ϊ4%
		{60, 40, 20},  // Day 2
		{60, 40, 20},  // Day 3
		{60, 40, 20},  // Day 4
		{60, 40, 20},  // Day 5
		{60, 40, 20}   // Day 6
	}
};

static uint32_t ����_weekly_reward_percent[3][7][3] = {
	//1000 ������
	{
		{40, 0, 0},  // Day 7
		{40, 0, 0},  // Day 1����һ����Ʒ����Ϊ4%���ڶ�����Ʒ����Ϊ0%����������Ʒ����Ϊ0%
		{40, 0, 0},  // Day 2
		{40, 0, 0},  // Day 3
		{40, 0, 0},  // Day 4
		{40, 0, 0},  // Day 5
		{40, 0, 0}   // Day 6
	},
	//10000 ������
	{
		{50, 40, 0},  // Day 7
		{50, 40, 0},  // Day 1����һ����Ʒ����Ϊ8%���ڶ�����Ʒ����Ϊ8%����������Ʒ����Ϊ0%
		{50, 40, 0},  // Day 2
		{50, 40, 0},  // Day 3
		{50, 40, 0},  // Day 4
		{50, 40, 0},  // Day 5
		{50, 40, 0}   // Day 6
	},
	//100000 ������
	{
		{70, 50, 40},  // Day 7
		{70, 50, 40},  // Day 1����һ����Ʒ����Ϊ12%���ڶ�����Ʒ����Ϊ8%����������Ʒ����Ϊ4%
		{70, 50, 40},  // Day 2
		{70, 50, 40},  // Day 3
		{70, 50, 40},  // Day 4
		{70, 50, 40},  // Day 5
		{70, 50, 40}   // Day 6
	}
};

// �����ܹ�����Ʒ����
#define TOTAL_COREN_ITEMS 25

/* ���׽����б� */
static uint32_t day_reward_list[7][3][TOTAL_COREN_ITEMS] = {
	/* ������ */
	{
		//1000 ������ ����:���� ����� 7��ҩ ��չ��� ��������
		{
			BBItem_Monomate, BBItem_Dimate, BBItem_Trimate,
			BBItem_Monofluid, BBItem_Difluid, BBItem_Trifluid,
			BBItem_Sol_Atomizer, BBItem_Moon_Atomizer, BBItem_Star_Atomizer,
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_AddSlot
		},
		//10000 ������ ����:װ������ ����� ������֮�� ���˷� ǹ�ַ� ħ���� ��ɴ��� ������ver00 ��â����
		{
			BBItem_Frame, BBItem_Armor, BBItem_Psy_Armor,
			BBItem_Hyper_Frame, BBItem_Grand_Armor, BBItem_Shock_Frame,
			BBItem_Kings_Frame, BBItem_Dragon_Frame, BBItem_Absorb_Armor,
			BBItem_Protect_Frame, BBItem_Soul_Frame, BBItem_Cross_Armor,
			BBItem_Solid_Frame, BBItem_Holiness_Armor, BBItem_Guardian_Armor,
			BBItem_Divinity_Armor, BBItem_Ultimate_Frame, BBItem_Celestial_Armor, 
			BBItem_HUNTER_FIELD, BBItem_RANGER_FIELD, BBItem_FORCE_FIELD,
			BBItem_CUSTOM_FRAME_ver_OO, BBItem_ATTRIBUTE_PLATE, BBItem_WEDDING_DRESS,
			BBItem_FLOWENS_FRAME
		},
		//100000 ������ ����:��Ʒ ���ֹ��� ���ϸ�� ��ʹ֮��ϸ�� ��ħ֮��ϸ�� ���ӵ�� ���ܵ�ȯ ��չ��� ����΢�� ���ӽᾧ
		{
			BBItem_Photon_Ticket, BBItem_Item_Ticket, BBItem_AddSlot,
			BBItem_Photon_Sphere, BBItem_Photon_Drop, BBItem_Parts_of_RoboChao,
			BBItem_Cell_of_MAG_502, BBItem_Cell_of_MAG_213, BBItem_DISK_Vol1,
			BBItem_DISK_Vol2, BBItem_DISK_Vol3, BBItem_DISK_Vol4,
			BBItem_DISK_Vol5, BBItem_DISK_Vol6, BBItem_DISK_Vol7,
			BBItem_DISK_Vol8, BBItem_DISK_Vol9, BBItem_DISK_Vol10,
			BBItem_DISK_Vol1, BBItem_DISK_Vol2, BBItem_DISK_Vol3,
			BBItem_Heart_of_Devil, BBItem_Heart_of_Pian, BBItem_Heart_of_Chao,
			BBItem_Heart_of_Angel
		}
	},
	/* ����һ */
	{
		//1000 ������ ���� ��� è������ ��ӡ֮�� ���Ŷ� �ö�����
		{
			BBItem_Monomate, BBItem_Dimate, BBItem_Trimate,
			BBItem_Monofluid, BBItem_Difluid, BBItem_Trifluid,
			BBItem_Sol_Atomizer, BBItem_Moon_Atomizer, BBItem_Star_Atomizer,
			BBItem_Knight_Power, BBItem_Priest_Mind, BBItem_Marksman_Arm,
			BBItem_Thief_Legs, BBItem_Digger_HP, BBItem_Magician_TP,
			BBItem_Cure_Poison, BBItem_Cure_Paralysis, BBItem_Cure_Slow,
			BBItem_Cure_Confuse, BBItem_Cure_Freeze, BBItem_Cure_Shock,
			BBItem_BUNNY_EARS, BBItem_GRATIA, BBItem_THREE_SEALS,
			BBItem_CAT_EARS
		},
		//10000 ������ �������� ��ħ֮�� ʯ�н� ����֮��
		{
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Delsabers_Right_Arm, BBItem_Delsabers_Left_Arm, BBItem_PROOF_OF_SWORD_SAINT,
			BBItem_Sl_JORDSf_WJ_LORE, BBItem_FLAMBERGE, BBItem_LAME_DARGENT,
			BBItem_EXCALIBUR, BBItem_OROTIAGITO_alt, BBItem_AGITO_1980,
			BBItem_DELSABERS_BUSTER
		},
		//100000 ������ �������� PC ն�� ���������� �����Ĵ� ħ���� ��ɫ�� ����ŵ�޽� ˫ʥ����ά˹ �������� �������
		{
			BBItem_Photon_Crystal, BBItem_BLADE_DANCE, BBItem_ZERO_DIVIDE,
			BBItem_DURANDAL, BBItem_TWO_KAMUI, BBItem_GALATINE,
			BBItem_Gladius, BBItem_DBS_SABER, BBItem_KALADBOLG,
			BBItem_DURANDAL, BBItem_GALATINE, BBItem_PHOENIX_CLAW,
			BBItem_DOUBLE_SABER, BBItem_STAG_CUTLERY, BBItem_DELSABERS_BUSTER,
			BBItem_RED_SABER, BBItem_RED_DAGGER, BBItem_FLIGHT_CUTTER,
			BBItem_DRAGON_SLAYER, BBItem_LAST_SURVIVOR, BBItem_LAVIS_BLADE,
			BBItem_ELYSION, BBItem_RED_SWORD, BBItem_S_REDS_BLADE,
			BBItem_FLOWENS_SWORD
		}
	},
	/* ���ڶ� */
	{
		//1000 ������ ��ʴ�Ŵ�����[������]
		{
			BBItem_Monomate, BBItem_Dimate, BBItem_Trimate,
			BBItem_Monofluid, BBItem_Difluid, BBItem_Trifluid,
			BBItem_Sol_Atomizer, BBItem_Moon_Atomizer, BBItem_Star_Atomizer,
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Parasitic_gene_Flow
		},
		//10000 ������ D�ͼ���ϸ�� ������
		{
			BBItem_Rappys_Ming, BBItem_Trap_Vision, BBItem_Telepipe,
			BBItem_Monogrinder, BBItem_Digrinder, BBItem_Trigrinder,
			BBItem_Scape_Doll, BBItem_Amplifier_of_Foie, BBItem_Amplifier_of_Barta,
			BBItem_Amplifier_of_Zonde, BBItem_Amplifier_of_Gifoie, BBItem_Amplifier_of_Gibarta,
			BBItem_Amplifier_of_Gizonde, BBItem_Amplifier_of_Resta, BBItem_Amplifier_of_Anti,
			BBItem_Amplifier_of_Shift, BBItem_Amplifier_of_Deband, BBItem_Star_Amplifier,
			BBItem_Amplifier_of_Rafoie, BBItem_Amplifier_of_Rabarta, BBItem_Amplifier_of_Razonde,
			BBItem_Amplifier_of_Red, BBItem_Amplifier_of_Blue, BBItem_Amplifier_of_Yellow,
			BBItem_Parasitic_cell_Type_D
		},
		//100000 ������ V502 V501 V801 V101 �ϵ۵��ػ� ���������� �޷���
		{
			BBItem_Meseta, BBItem_God_Legs, BBItem_Dragon_HP,
			BBItem_Angel_TP, BBItem_Metal_Body, BBItem_Angel_Luck,
			BBItem_God_Luck, BBItem_God_Body, BBItem_God_Power,
			BBItem_God_Mind, BBItem_God_Arm, BBItem_God_Legs,
			BBItem_God_HP, BBItem_God_TP, BBItem_HP_Ressurection,
			BBItem_TP_Ressurection, BBItem_PB_Increase, BBItem_DIVINE_PROTECTION,
			BBItem_LIMITER, BBItem_ADEPT, BBItem_YASAKANI_MAGATAMA,
			BBItem_V101, BBItem_V501, BBItem_V502,
			BBItem_V801
		}
	},
	/* ������ */
	{
		//1000 ������ MARK3�����
		{
			BBItem_Monomate, BBItem_Dimate, BBItem_Trimate,
			BBItem_Monofluid, BBItem_Difluid, BBItem_Trifluid,
			BBItem_Sol_Atomizer, BBItem_Moon_Atomizer, BBItem_Star_Atomizer,
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Kit_of_MARK3
		},
		//10000 ������ ���֮�� ���ֻظ��Ͳ��
		{
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_HP_Restorate, BBItem_HP_Generate, BBItem_HP_Revival,
			BBItem_TP_Restorate, BBItem_TP_Generate, BBItem_TP_Revival,
			BBItem_PB_Amplifier, BBItem_PB_Generate, BBItem_PB_Create,
			BBItem_SMARTLINK
		},
		//100000 ������ ����ϵ�� ���ﲿ�� ���ϸ��
		{
			BBItem_P_arms_Arms, BBItem_Bringers_Right_Arm, BBItem_Delsabers_Left_Arm,
			BBItem_Delsabers_Right_Arm, BBItem_Heart_of_Opa_Opa, BBItem_Heart_of_Chao,
			BBItem_Heart_of_Pian, BBItem_Parts_of_RoboChao, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Cell_of_MAG_502, BBItem_Cell_of_MAG_213, BBItem_Parts_of_RoboChao,
			BBItem_Photon_Booster, BBItem_AddSlot, BBItem_Photon_Drop,
			BBItem_Photon_Sphere, BBItem_Photon_Crystal, BBItem_Secret_Ticket,
			BBItem_Photon_Ticket
		}
	},
	/* ������ */
	{
		//1000 ������ MASTER SYSTEM�����
		{
			BBItem_Monomate, BBItem_Dimate, BBItem_Trimate,
			BBItem_Monofluid, BBItem_Difluid, BBItem_Trifluid,
			BBItem_Sol_Atomizer, BBItem_Moon_Atomizer, BBItem_Star_Atomizer,
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Kit_of_MASTER_SYSTEM
		},
		//10000 ������ ���������л���� �ܶ����͵���ɡ ��� ������ ���Ӱ� ע���� ������ ʮ�� ����ͨ�� ��ǹ
		{
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_BAMBOO_SPEAR, BBItem_KANEI_TSUHO, BBItem_JITTE,
			BBItem_BUTTERFLY_NET, BBItem_SYRINGE, BBItem_BATTLEDORE,
			BBItem_RACKET, BBItem_HAMMER, BBItem_TYRELLS_PARASOL,
			BBItem_5TH_ANNIV_BLADE
		},
		//100000 ������ ���ָ߼����
		{
			BBItem_V501, BBItem_V502, BBItem_V801,
			BBItem_LIMITER, BBItem_ADEPT, BBItem_DIVINE_PROTECTION,
			BBItem_Heavenly_Battle, BBItem_Heavenly_Power, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Heavenly_Mind, BBItem_Heavenly_Luck, BBItem_Heavenly_Ability,
			BBItem_Heavenly_HP, BBItem_Heavenly_TP, BBItem_Heavenly_Resist,
			BBItem_Heavenly_Technique, BBItem_HP_Ressurection, BBItem_TP_Ressurection,
			BBItem_Heavenly_Ability
		}
	},
	/* ������ */
	{
		//1000 ������ GENESIS�����
		{
			BBItem_Monomate, BBItem_Dimate, BBItem_Trimate,
			BBItem_Monofluid, BBItem_Difluid, BBItem_Trifluid,
			BBItem_Sol_Atomizer, BBItem_Moon_Atomizer, BBItem_Star_Atomizer,
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Kit_of_GENESIS
		},
		//10000 ������ ��������(���) �����飦������(���) ������(���) ������˹(���) ������(���) ����ŵ�(���) ��ħ֮��(���) ��ʹ֮��(���) ��������(���)
		{
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Mag, BBItem_Soniti, BBItem_ANGELS_WING,
			BBItem_DEVILS_WING, BBItem_ELENOR, BBItem_HAMBURGER,
			BBItem_Tellusis, BBItem_Rappy, BBItem_Gael_Giel,
			BBItem_Agastya
		},
		//100000 ������ ����֮���� ǳ�ƽz���zȡ�z ������ ��ɱ���Բʷ� ��ɫ���� KROES SWEATER ���ǵľ����� ϸ������[����������] DF��
		{
			BBItem_DBS_ARMOR, BBItem_GUARD_WAVE, BBItem_DF_FIELD,
			BBItem_LUMINOUS_FIELD, BBItem_CHU_CHU_FEVER, BBItem_LOVE_HEART,
			BBItem_FLAME_GARMENT, BBItem_AURA_FIELD, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_DF_FIELD, BBItem_VIRUS_ARMOR_Lafuteria, BBItem_DIRTY_LIFEJACKET,
			BBItem_KROES_SWEATER, BBItem_SONICTEAM_ARMOR, BBItem_RED_COAT,
			BBItem_THIRTEEN, BBItem_STEALTH_SUIT, BBItem_UNION_FIELD,
			BBItem_SAMURAI_ARMOR
		}
	},
	/* ������ */
	{
		//1000 ������ �������ǵ���� DREAMCAST�����
		{
			BBItem_Monomate, BBItem_Dimate, BBItem_Trimate,
			BBItem_Monofluid, BBItem_Difluid, BBItem_Trifluid,
			BBItem_Sol_Atomizer, BBItem_Moon_Atomizer, BBItem_Star_Atomizer,
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Kit_of_DREAMCAST, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Kit_of_SEGA_SATURN
		},
		//10000 ������ ��ɵ��۾� ��ɵĶ��� ��ɫ����
		{
			BBItem_Antidote, BBItem_Antiparalysis, BBItem_Telepipe,
			BBItem_Trap_Vision, BBItem_Scape_Doll, BBItem_Monogrinder,
			BBItem_Digrinder, BBItem_Trigrinder, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_Valiant_Frame, BBItem_Imperial_Armor, BBItem_Holiness_Armor,
			BBItem_Guardian_Armor, BBItem_Divinity_Armor, BBItem_Ultimate_Frame,
			BBItem_Celestial_Armor, BBItem_RED_RING, BBItem_RIKOS_GLASSES,
			BBItem_RIKOS_EARRING
		},
		//100000 ������ ���Ű¶����� �ֿ���ɫ���� �ֿ��������� ������¡֮�� ��ʹԲ�� ��ɫ���� �ȳ��� ���뾵 ¶�ſ� ��Ԩ����
		{
			BBItem_YATA_MIRROR, BBItem_BLACK_GAUNTLETS, BBItem_EPSIGUARD,
			BBItem_ANGEL_RING, BBItem_Scape_Doll, BBItem_FROM_THE_DEPTHS,
			BBItem_RUPIKA, BBItem_STINK_SHIELD, BBItem_Power_Material,
			BBItem_Mind_Material, BBItem_Evade_Material, BBItem_HP_Material,
			BBItem_TP_Material, BBItem_Def_Material, BBItem_Luck_Material,
			BBItem_TRIPOLIC_REFLECTOR, BBItem_REGENERATE_GEAR_BP, BBItem_WEAPONS_SILVER_SHIELD,
			BBItem_WEAPONS_COPPER_SHIELD, BBItem_YELLOW_RING_63, BBItem_GREEN_RING_5B,
			BBItem_BLUE_RING_53, BBItem_ANTI_DARK_RING, BBItem_ANTI_LIGHT_RING,
			BBItem_RAGOL_RING
		}
	}
};

// �齱����
static inline int lottery_num(sfmt_t* rng) {

	// ����Ȩ���ܺ�
	int totalWeight = 0;
	for (int i = 0; i < TOTAL_COREN_ITEMS; i++) {
		if (i < 7) {  // ǰ7����Ʒռ��70%
			totalWeight += 7;
		}
		else if (i < 17) {  // ��������10����Ʒռ��20%
			totalWeight += 2;
		}
		else {  // ʣ�µ�8����Ʒռ��10%
			totalWeight += 1;
		}
	}

	// �������������Χ��0 �� totalWeight-1��
	int randomNumber = sfmt_genrand_uint32(rng) % totalWeight;

	// ���������ȷ���н���Ʒ
	int winningItem = -1;
	int cumulativeWeight = 0;
	for (int i = 0; i < TOTAL_COREN_ITEMS; i++) {
		if (i < 7) {
			cumulativeWeight += 7;
		}
		else if (i < 17) {
			cumulativeWeight += 2;
		}
		else {
			cumulativeWeight += 1;
		}
		if (randomNumber < cumulativeWeight) {
			winningItem = i;
			break;
		}
	}

	return winningItem;
}

#endif // !COREN_REWARD_LISH_H