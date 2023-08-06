#pragma once
#include <pso_character.h>
#include <pso_player.h>
#include "mag_bb.h"
#include "handle_player_items.h"
#include <f_logs.h>

// 玛古喂养表
static uint16_t mag_feed_t[MAG_MAX_FEED_TABLE][11 * 6] = {

	{
		3, 3, 5, 40, 5, 0,
		3, 3, 10, 45, 5, 0,
		4, 4, 15, 50, 10, 0,
		3, 3, 5, 0, 5, 40,
		3, 3, 10, 0, 5, 45,
		4, 4, 15, 0, 10, 50,
		3, 3, 5, 10, 40, 0,
		3, 3, 5, 0, 44, 10,
		4, 1, 15, 30, 15, 25,
		4, 1, 15, 25, 15, 30,
		6, 5, 25, 25, 25, 25
	},

	{
		0, 0, 5, 10, 0, -1,
		2, 1, 6, 15, 3, -3,
		3, 2, 12, 21, 4, -7,
		0, 0, 5, 0, 0, 8,
		2, 1, 7, 0, 3, 13,
		3, 2, 7, -7, 6, 19,
		0, 1, 0, 5, 15, 0,
		2, 0, -1, 0, 14, 5,
		-2, 2, 10, 11, 8, 0,
		3, -2, 9, 0, 9, 11,
		4, 3, 14, 9, 18, 11
	},

	{
		0, -1, 1, 9, 0, -5,
		3, 0, 1, 13, 0, -10,
		4, 1, 8, 16, 2, -15,
		0, -1, 0, -5, 0, 9,
		3, 0, 4, -10, 0, 13,
		3, 2, 6, -15, 5, 17,
		-1, 1, -5, 4, 12, -5,
		0, 0, -5, -6, 11, 4,
		4, -2, 0, 11, 3, -5,
		-1, 1, 4, -5, 0, 11,
		4, 2, 7, 8, 6, 9
	},

	{
		0, -1, 0, 3, 0, 0,
		2, 0, 5, 7, 0, -5,
		3, 1, 4, 14, 6, -10,
		0, 0, 0, 0, 0, 4,
		0, 1, 4, -5, 0, 8,
		2, 2, 4, -10, 3, 15,
		-3, 3, 0, 0, 7, 0,
		3, 0, -4, -5, 20, -5,
		3, -2, -10, 9, 6, 9,
		-2, 2, 8, 5, -8, 7,
		3, 2, 7, 7, 7, 7
	},

	{
		2, -1, -5, 9, -5, 0,
		2, 0, 0, 11, 0, -10,
		0, 1, 4, 14, 0, -15,
		2, -1, -5, 0, -6, 10,
		2, 0, 0, -10, 0, 11,
		0, 1, 4, -15, 0, 15,
		2, -1, -5, -5, 16, -5,
		-2, 3, 7, -3, 0, -3,
		4, -2, 5, 21, -5, -20,
		3, 0, -5, -20, 5, 21,
		3, 2, 4, 6, 8, 5
	},

	{
		2, -1, -4, 13, -5, -5,
		0, 1, 0, 16, 0, -15,
		2, 0, 3, 19, -2, -18,
		2, -1, -4, -5, -5, 13,
		0, 1, 0, -15, 0, 16,
		2, 0, 3, -20, 0, 19,
		0, 1, 5, -6, 6, -5,
		-1, 1, 0, -4, 14, -10,
		4, -1, 4, 17, -5, -15,
		2, 0, -10, -15, 5, 21,
		3, 2, 2, 8, 3, 6
	},

	{
		-1, 1, -3, 9, -3, -4,
		2, 0, 0, 11, 0, -10,
		2, 0, 2, 15, 0, -16,
		-1, 1, -3, -4, -3, 9,
		2, 0, 0, -10, 0, 11,
		2, 0, -2, -15, 0, 19,
		2, -1, 0, 6, 9, -15,
		-2, 3, 0, -15, 9, 6,
		3, -1, 9, -20, -5, 17,
		0, 2, -5, 20, 5, -20,
		3, 2, 0, 11, 0, 11
	},

	{
		-1, 0, -4, 21, -15, -5, // Fixed the 2 incorrect bytes in table 7 (was cell table)
		0, 1, -1, 27, -10, -16,
		2, 0, 5, 29, -7, -25,
		-1, 0, -10, -5, -10, 21,
		0, 1, -5, -16, -5, 25,
		2, 0, -7, -25, 6, 29,
		-1, 1, -10, -10, 28, -10,
		2, -1, 9, -18, 24, -15,
		2, 1, 19, 18, -15, -20,
		2, 1, -15, -20, 19, 18,
		4, 2, 3, 7, 3, 3
	}
};

size_t get_mag_feed_table_index(iitem_t* mag) {

	switch (mag->data.datab[1])
	{
	case Mag_Mag:
		return 0;

	case Mag_Varuna:
	case Mag_Vritra:
	case Mag_Kalki:
		return 1;

	case Mag_Ashvinau:
	case Mag_Sumba:
	case Mag_Namuci:
	case Mag_Marutah:
	case Mag_Rudra:
		return 2;

	case Mag_Surya:
	case Mag_Tapas:
	case Mag_Mitra:
		return 3;

	case Mag_Apsaras:
	case Mag_Vayu:
	case Mag_Varaha:
	case Mag_Ushasu:
	case Mag_Kama:
	case Mag_Kaitabha:
	case Mag_Kumara:
	case Mag_Bhirava:
		return 4;

	case Mag_Ila:
	case Mag_Garuda:
	case Mag_Sita:
	case Mag_Soma:
	case Mag_Durga:
	case Mag_Nandin:
	case Mag_Yaksa:
	case Mag_Ribhava:
		return 5;

	case Mag_Andhaka:
	case Mag_Kabanda:
	case Mag_Naga:
	case Mag_Naraka:
	case Mag_Bana:
	case Mag_Marica:
	case Mag_Madhu:
	case Mag_Ravana:
		return 6;

	case Mag_Deva:
	case Mag_Rukmin:
	case Mag_Sato:
		return 5;

	case Mag_Rati:
	case Mag_Pushan:
	case Mag_Bhima:
		return 6;

	default:
		return 7;
	}
}

// 玛古喂养表
magfeedresultslist_t mag_feed_table[MAG_MAX_FEED_TABLE][11 * 6] = {

	{
		3, 3, 5, 40, 5, 0,
		3, 3, 10, 45, 5, 0,
		4, 4, 15, 50, 10, 0,
		3, 3, 5, 0, 5, 40,
		3, 3, 10, 0, 5, 45,
		4, 4, 15, 0, 10, 50,
		3, 3, 5, 10, 40, 0,
		3, 3, 5, 0, 44, 10,
		4, 1, 15, 30, 15, 25,
		4, 1, 15, 25, 15, 30,
		6, 5, 25, 25, 25, 25
	},

	{
		0, 0, 5, 10, 0, -1,
		2, 1, 6, 15, 3, -3,
		3, 2, 12, 21, 4, -7,
		0, 0, 5, 0, 0, 8,
		2, 1, 7, 0, 3, 13,
		3, 2, 7, -7, 6, 19,
		0, 1, 0, 5, 15, 0,
		2, 0, -1, 0, 14, 5,
		-2, 2, 10, 11, 8, 0,
		3, -2, 9, 0, 9, 11,
		4, 3, 14, 9, 18, 11
	},

	{
		0, -1, 1, 9, 0, -5,
		3, 0, 1, 13, 0, -10,
		4, 1, 8, 16, 2, -15,
		0, -1, 0, -5, 0, 9,
		3, 0, 4, -10, 0, 13,
		3, 2, 6, -15, 5, 17,
		-1, 1, -5, 4, 12, -5,
		0, 0, -5, -6, 11, 4,
		4, -2, 0, 11, 3, -5,
		-1, 1, 4, -5, 0, 11,
		4, 2, 7, 8, 6, 9
	},

	{
		0, -1, 0, 3, 0, 0,
		2, 0, 5, 7, 0, -5,
		3, 1, 4, 14, 6, -10,
		0, 0, 0, 0, 0, 4,
		0, 1, 4, -5, 0, 8,
		2, 2, 4, -10, 3, 15,
		-3, 3, 0, 0, 7, 0,
		3, 0, -4, -5, 20, -5,
		3, -2, -10, 9, 6, 9,
		-2, 2, 8, 5, -8, 7,
		3, 2, 7, 7, 7, 7
	},

	{
		2, -1, -5, 9, -5, 0,
		2, 0, 0, 11, 0, -10,
		0, 1, 4, 14, 0, -15,
		2, -1, -5, 0, -6, 10,
		2, 0, 0, -10, 0, 11,
		0, 1, 4, -15, 0, 15,
		2, -1, -5, -5, 16, -5,
		-2, 3, 7, -3, 0, -3,
		4, -2, 5, 21, -5, -20,
		3, 0, -5, -20, 5, 21,
		3, 2, 4, 6, 8, 5
	},

	{
		2, -1, -4, 13, -5, -5,
		0, 1, 0, 16, 0, -15,
		2, 0, 3, 19, -2, -18,
		2, -1, -4, -5, -5, 13,
		0, 1, 0, -15, 0, 16,
		2, 0, 3, -20, 0, 19,
		0, 1, 5, -6, 6, -5,
		-1, 1, 0, -4, 14, -10,
		4, -1, 4, 17, -5, -15,
		2, 0, -10, -15, 5, 21,
		3, 2, 2, 8, 3, 6
	},

	{
		-1, 1, -3, 9, -3, -4,
		2, 0, 0, 11, 0, -10,
		2, 0, 2, 15, 0, -16,
		-1, 1, -3, -4, -3, 9,
		2, 0, 0, -10, 0, 11,
		2, 0, -2, -15, 0, 19,
		2, -1, 0, 6, 9, -15,
		-2, 3, 0, -15, 9, 6,
		3, -1, 9, -20, -5, 17,
		0, 2, -5, 20, 5, -20,
		3, 2, 0, 11, 0, 11
	},

	{
		-1, 0, -4, 21, -15, -5, // Fixed the 2 incorrect bytes in table 7 (was cell table)
		0, 1, -1, 27, -10, -16,
		2, 0, 5, 29, -7, -25,
		-1, 0, -10, -5, -10, 21,
		0, 1, -5, -16, -5, 25,
		2, 0, -7, -25, 6, 29,
		-1, 1, -10, -10, 28, -10,
		2, -1, 9, -18, 24, -15,
		2, 1, 19, 18, -15, -20,
		2, 1, -15, -20, 19, 18,
		4, 2, 3, 7, 3, 3
	}
};

magfeedresult_t get_mag_feed_result(size_t table_index, size_t item_index) {
	if (table_index >= 8) {
		ERR_LOG("invalid mag feed table index");
		printf("按任意键停止程序...\n");
		getchar();
		exit(1);
	}
	if (item_index >= 11) {
		ERR_LOG("invalid mag feed item index");
		printf("按任意键停止程序...\n");
		getchar();
		exit(1);
	}

	magfeedresultslist_t* result = mag_feed_table[table_index];
	return result->results[item_index];
}

/* MAG 增加光子爆发参数*/
void mag_bb_add_pb(uint8_t* flags, uint8_t* blasts, uint8_t pb) {
	int32_t pb_exists = 0;
	uint8_t pbv;
	uint32_t pb_slot;

	if ((*flags & 0x01) == 0x01)
	{
		if ((*blasts & 0x07) == pb)
			pb_exists = 1;
	}

	if ((*flags & 0x02) == 0x02)
	{
		if (((*blasts / 8) & 0x07) == pb)
			pb_exists = 1;
	}

	if ((*flags & 0x04) == 0x04)
		pb_exists = 1;

	if (!pb_exists)
	{
		if ((*flags & 0x01) == 0)
			pb_slot = 0;
		else
			if ((*flags & 0x02) == 0)
				pb_slot = 1;
			else
				pb_slot = 2;
		switch (pb_slot)
		{
		case 0x00:
			*blasts &= 0xF8;
			*flags |= 0x01;
			break;
		case 0x01:
			pb *= 8;
			*blasts &= 0xC7;
			*flags |= 0x02;
			break;
		case 0x02:
			pbv = pb;
			if ((*blasts & 0x07) < pb)
				pbv--;
			if (((*blasts / 8) & 0x07) < pb)
				pbv--;
			pb = pbv * 0x40;
			*blasts &= 0x3F;
			*flags |= 0x04;
		}
		*blasts |= pb;
	}
}

/* MAG 增加变量参数*/
int32_t mag_bb_alignment(magitem_t* m) {
	int32_t v1, v2, v3, v4, v5, v6;

	v4 = 0;
	v3 = m->pow;
	v2 = m->dex;
	v1 = m->mind;
	if (v2 < v3)
	{
		if (v1 < v3)
			v4 = 8;
	}
	if (v3 < v2)
	{
		if (v1 < v2)
			v4 |= 0x10u;
	}
	if (v2 < v1)
	{
		if (v3 < v1)
			v4 |= 0x20u;
	}
	v6 = 0;
	v5 = v3;
	if (v3 <= v2)
		v5 = v2;
	if (v5 <= v1)
		v5 = v1;
	if (v5 == v3)
		v6 = 1;
	if (v5 == v2)
		++v6;
	if (v5 == v1)
		++v6;
	if (v6 >= 2)
		v4 |= 0x100u;
	return v4;
}

/* MAG 特殊进化函数 */
int32_t mag_bb_special_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	uint8_t oldType;
	int16_t mDefense, mPower, mDex, mMind;

	oldType = m->mtype;

	if (m->level >= 100)
	{
		mDefense = m->def / 100;
		mPower = m->pow / 100;
		mDex = m->dex / 100;
		mMind = m->mind / 100;

		switch (section_id)
		{
		case SID_Viridia:
		case SID_Bluefull:
		case SID_Redria:
		case SID_Whitill:
			if ((mDefense + mDex) == (mPower + mMind))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Deva;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Sato;
					break;
				default:
					break;
				}
			}
			break;
		case SID_Skyly:
		case SID_Pinkal:
		case SID_Yellowboze:
			if ((mDefense + mPower) == (mDex + mMind))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Dewari;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		case SID_Greennill:
		case SID_Oran:
		case SID_Purplenum:
			if ((mDefense + mMind) == (mPower + mDex))
			{
				switch (type)
				{
				case CLASS_HUMAR:
				case CLASS_HUCAST:
					m->mtype = Mag_Rati;
					break;
				case CLASS_HUNEWEARL:
				case CLASS_HUCASEAL:
					m->mtype = Mag_Savitri;
					break;
				case CLASS_RAMAR:
				case CLASS_RACAST:
					m->mtype = Mag_Pushan;
					break;
				case CLASS_RACASEAL:
				case CLASS_RAMARL:
					m->mtype = Mag_Rukmin;
					break;
				case CLASS_FONEWM:
				case CLASS_FOMAR:
					m->mtype = Mag_Nidra;
					break;
				case CLASS_FONEWEARL:
				case CLASS_FOMARL:
					m->mtype = Mag_Bhima;
					break;
				default:
					break;
				}
			}
			break;
		}
	}
	return (int32_t)(oldType != m->mtype);
}

/* MAG 50级进化函数 */
void mag_bb_lv50_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	int32_t v10, v11, v12, v13;

	int32_t Alignment = mag_bb_alignment(m);

	if (evolution_class > 3) // Don't bother to check if a special mag.
		return;

	v10 = m->pow / 100;
	v11 = m->dex / 100;
	v12 = m->mind / 100;
	v13 = m->def / 100;

	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108)
		{
			if (section_id & 1)
			{
				if (v12 > v11)
				{
					m->mtype = Mag_Apsaras;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
				}
				else
				{
					m->mtype = Mag_Kama;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				}
			}
			else
			{
				if (v12 > v11)
				{
					m->mtype = Mag_Bhirava;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
				}
			}
		}
		else
		{
			if (Alignment & 0x10)
			{
				if (section_id & 1)
				{
					if (v10 > v12)
					{
						m->mtype = Mag_Garuda;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Yaksa;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
					}
				}
				else
				{
					if (v10 > v12)
					{
						m->mtype = Mag_Ila;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Nandin;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
					}
				}
			}
			else
			{
				if (Alignment & 0x20)
				{
					if (section_id & 1)
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Soma;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Bana;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Ushasu;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
						}
					}
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if (Alignment & 0x110)
		{
			if (section_id & 1)
			{
				if (v10 > v12)
				{
					m->mtype = Mag_Kaitabha;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
				}
			}
			else
			{
				if (v10 > v12)
				{
					m->mtype = Mag_Bhirava;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Kama;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				}
			}
		}
		else
		{
			if (Alignment & 0x08)
			{
				if (section_id & 1)
				{
					if (v12 > v11)
					{
						m->mtype = Mag_Kaitabha;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Madhu;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
				}
				else
				{
					if (v12 > v11)
					{
						m->mtype = Mag_Bhirava;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Kama;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
					}
				}
			}
			else
			{
				if (Alignment & 0x20)
				{
					if (section_id & 1)
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Durga;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Apsaras;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Varaha;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
						}
					}
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if (Alignment & 0x120)
		{
			if (v13 > 44)
			{
				m->mtype = Mag_Bana;
				mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
			}
			else
			{
				if (section_id & 1)
				{
					if (v11 > v10)
					{
						m->mtype = Mag_Ila;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Kumara;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
					}
				}
				else
				{
					if (v11 > v10)
					{
						m->mtype = Mag_Kabanda;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Naga;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					}
				}
			}
		}
		else
		{
			if (Alignment & 0x08)
			{
				if (v13 > 44)
				{
					m->mtype = Mag_Andhaka;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
				}
				else
				{
					if (section_id & 1)
					{
						if (v12 > v11)
						{
							m->mtype = Mag_Naga;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
						}
						else
						{
							m->mtype = Mag_Marica;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
						}
					}
					else
					{
						if (v12 > v11)
						{
							m->mtype = Mag_Ravana;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Farlla);
						}
						else
						{
							m->mtype = Mag_Naraka;
							mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
						}
					}
				}
			}
			else
			{
				if (Alignment & 0x10)
				{
					if (v13 > 44)
					{
						m->mtype = Mag_Bana;
						mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
					}
					else
					{
						if (section_id & 1)
						{
							if (v10 > v12)
							{
								m->mtype = Mag_Garuda;
								mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
							}
							else
							{
								m->mtype = Mag_Bhirava;
								mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
							}
						}
						else
						{
							if (v10 > v12)
							{
								m->mtype = Mag_Ribhava;
								mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Farlla);
							}
							else
							{
								m->mtype = Mag_Sita;
								mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
							}
						}
					}
				}
			}
		}
		break;
	}
}

/* MAG 35级进化函数 */
void mag_bb_lv35_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	int32_t Alignment = mag_bb_alignment(m);

	if (evolution_class > 3) // Don't bother to check if a special mag.
		return;

	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108) {
			m->mtype = Mag_Rudra;
			mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
			return;
		}
		else {
			if (Alignment & 0x10) {
				m->mtype = Mag_Marutah;
				mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
				return;
			}
			else {
				if (Alignment & 0x20) {
					m->mtype = Mag_Vayu;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		if (Alignment & 0x110) {
			m->mtype = Mag_Mitra;
			mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
			return;
		}
		else {
			if (Alignment & 0x08) {
				m->mtype = Mag_Surya;
				mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
				return;
			}
			else {
				if (Alignment & 0x20) {
					m->mtype = Mag_Tapas;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
					return;
				}
			}
		}
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		if (Alignment & 0x120) {
			m->mtype = Mag_Namuci;
			mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Mylla_Youlla);
			return;
		}
		else {
			if (Alignment & 0x08) {
				m->mtype = Mag_Sumba;
				mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Golla);
				return;
			}
			else {
				if (Alignment & 0x10) {
					m->mtype = Mag_Ashvinau;
					mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Pilla);
					return;
				}
			}
		}
		break;
	}
}

/* MAG 10级进化函数 */
void mag_bb_lv10_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		m->mtype = Mag_Varuna;
		mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Farlla);
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		m->mtype = Mag_Kalki;
		mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Estlla);
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		m->mtype = Mag_Vritra;
		mag_bb_add_pb(&m->PBflags, &m->photon_blasts, PB_Leilla);
		break;
	}
}

/* MAG 进化检测函数 */
void mag_bb_check_evolution(magitem_t* m, uint8_t section_id, uint8_t type, int32_t evolution_class) {
	if ((m->level < 10) || (m->level >= 35)) {
		if ((m->level < 35) || (m->level >= 50)) {
			if (m->level >= 50) {
				// Divisible by 5 with no remainder.
				if (!(m->level % 5)) {
					if (evolution_class <= 3) {
						if (!mag_bb_special_evolution(m, section_id, type, evolution_class))
							mag_bb_lv50_evolution(m, section_id, type, evolution_class);
					}
				}
			}
		}
		else {
			if (evolution_class < 2)
				mag_bb_lv35_evolution(m, section_id, type, evolution_class);
		}
	}
	else {
		if (evolution_class <= 0)
			mag_bb_lv10_evolution(m, section_id, type, evolution_class);
	}
}

/* MAG 喂养函数 */
int mag_bb_feed(ship_client_t* src, uint32_t mag_item_id, uint32_t fed_item_id) {
	lobby_t* l = src->cur_lobby;

	if (!l || l->type != LOBBY_TYPE_GAME)
		return -1;

	uint32_t i, mt_index = 0;
	int32_t evolution_class = 0;
	magitem_t* mag = { 0 };
	item_t* feed_item = { 0 };
	uint16_t* ft;
	int16_t mag_iq, mag_def, mag_pow, mag_dex, mag_mind;

	psocn_bb_char_t* character = &src->bb_pl->character;

	if (src->mode)
		character = &src->mode_pl->bb;

	if (fed_item_id != EMPTY_STRING) {
		i = find_iitem_index(&character->inv, fed_item_id);

		if (i == -1) {
			ERR_LOG("GC %" PRIu32 "无法找到需要喂养的物品ID %u",
				src->guildcard, fed_item_id);
			return -2;
		}

		feed_item = &character->inv.iitems[i].data;

		i = find_equipped_mag(&character->inv);

		if (i == -1) {
			ERR_LOG("GC %" PRIu32 "无法找到需要喂养的玛古ID %u",
				src->guildcard, mag_item_id);
			return -3;
		}

		mag = (magitem_t*)&character->inv.iitems[i].data;

		if ((feed_item->datab[0] == ITEM_TYPE_TOOL) &&
			(feed_item->datab[1] < ITEM_SUBTYPE_TELEPIPE) &&
			(feed_item->datab[1] != ITEM_SUBTYPE_DISK) &&
			(feed_item->datab[5] > 0x00))
		{
			switch (feed_item->datab[1])
			{
			case ITEM_SUBTYPE_MATE:
				mt_index = feed_item->datab[2];
				break;
			case ITEM_SUBTYPE_FLUID:
				mt_index = 3 + feed_item->datab[2];
				break;
			case ITEM_SUBTYPE_SOL_ATOMIZER:
			case ITEM_SUBTYPE_MOON_ATOMIZER:
			case ITEM_SUBTYPE_STAR_ATOMIZER:
				mt_index = 5 + feed_item->datab[1];
				break;
			case ITEM_SUBTYPE_ANTI:
				mt_index = 6 + feed_item->datab[2];
				break;
			}
		}

		remove_iitem(src, fed_item_id, 1, false);

		// 重新扫描以更新磁指针（如果由于清理而更改） 
		i = find_equipped_mag(&character->inv);

		if (i == -1) {
			ERR_LOG("GC %" PRIu32 "无法找到需要喂养的玛古ID %u",
				src->guildcard, mag_item_id);
			return -4;
		}

		mag = (magitem_t*)&character->inv.iitems[i].data;

		// Feed that mag (Updates to code by Lee from schtserv.com)
		switch (mag->mtype)
		{
		case Mag_Mag:
			ft = &mag_feed_t[0][0];
			evolution_class = 0;
			break;
		case Mag_Varuna:
		case Mag_Vritra:
		case Mag_Kalki:
			evolution_class = 1;
			ft = &mag_feed_t[1][0];
			break;
		case Mag_Ashvinau:
		case Mag_Sumba:
		case Mag_Namuci:
		case Mag_Marutah:
		case Mag_Rudra:
			ft = &mag_feed_t[2][0];
			evolution_class = 2;
			break;
		case Mag_Surya:
		case Mag_Tapas:
		case Mag_Mitra:
			ft = &mag_feed_t[3][0];
			evolution_class = 2;
			break;
		case Mag_Apsaras:
		case Mag_Vayu:
		case Mag_Varaha:
		case Mag_Ushasu:
		case Mag_Kama:
		case Mag_Kaitabha:
		case Mag_Kumara:
		case Mag_Bhirava:
			evolution_class = 3;
			ft = &mag_feed_t[4][0];
			break;
		case Mag_Ila:
		case Mag_Garuda:
		case Mag_Sita:
		case Mag_Soma:
		case Mag_Durga:
		case Mag_Nandin:
		case Mag_Yaksa:
		case Mag_Ribhava:
			evolution_class = 3;
			ft = &mag_feed_t[5][0];
			break;
		case Mag_Andhaka:
		case Mag_Kabanda:
		case Mag_Naga:
		case Mag_Naraka:
		case Mag_Bana:
		case Mag_Marica:
		case Mag_Madhu:
		case Mag_Ravana:
			evolution_class = 3;
			ft = &mag_feed_t[6][0];
			break;
		case Mag_Deva:
		case Mag_Rukmin:
		case Mag_Sato:
			ft = &mag_feed_t[5][0];
			evolution_class = 4;
			break;
		case Mag_Rati:
		case Mag_Pushan:
		case Mag_Bhima:
			ft = &mag_feed_t[6][0];
			evolution_class = 4;
			break;
		default:
			ft = &mag_feed_t[7][0];
			evolution_class = 4;
			break;
		}
		mt_index *= 6;
		mag->synchro += ft[mt_index];
		if (mag->synchro < 0)
			mag->synchro = 0;
		if (mag->synchro > 120)
			mag->synchro = 120;
		mag_iq = mag->IQ;

		mag_iq += ft[mt_index + 1];
		if (mag_iq < 0)
			mag_iq = 0;
		if (mag_iq > 200)
			mag_iq = 200;
		mag->IQ = (uint8_t)mag_iq;

		// Add Defense

		mag_def = mag->def % 100;
		mag_def += ft[mt_index + 2];

		if (mag_def < 0)
			mag_def = 0;

		if (mag_def >= 100) {
			if (mag->level == MAX_PLAYER_LEVEL)
				mag_def = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->def = ((mag->def / 100) * 100) + mag_def;
			mag_bb_check_evolution(mag, character->dress_data.section, character->dress_data.ch_class, evolution_class);
		}
		else
			mag->def = ((mag->def / 100) * 100) + mag_def;

		// Add Power

		mag_pow = mag->pow % 100;
		mag_pow += ft[mt_index + 3];

		if (mag_pow < 0)
			mag_pow = 0;

		if (mag_pow >= 100) {
			if (mag->level == MAX_PLAYER_LEVEL)
				mag_pow = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->pow = ((mag->pow / 100) * 100) + mag_pow;
			mag_bb_check_evolution(mag, character->dress_data.section, character->dress_data.ch_class, evolution_class);
		}
		else
			mag->pow = ((mag->pow / 100) * 100) + mag_pow;

		// Add Dex

		mag_dex = mag->dex % 100;
		mag_dex += ft[mt_index + 4];

		if (mag_dex < 0)
			mag_dex = 0;

		if (mag_dex >= 100)
		{
			if (mag->level == MAX_PLAYER_LEVEL)
				mag_dex = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->dex = ((mag->dex / 100) * 100) + mag_dex;
			mag_bb_check_evolution(mag, character->dress_data.section, character->dress_data.ch_class, evolution_class);
		}
		else
			mag->dex = ((mag->dex / 100) * 100) + mag_dex;

		// Add Mind

		mag_mind = mag->mind % 100;
		mag_mind += ft[mt_index + 5];

		if (mag_mind < 0)
			mag_mind = 0;

		if (mag_mind >= 100)
		{
			if (mag->level == MAX_PLAYER_LEVEL)
				mag_mind = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->mind = ((mag->mind / 100) * 100) + mag_mind;
			mag_bb_check_evolution(mag, character->dress_data.section, character->dress_data.ch_class, evolution_class);
		}
		else
			mag->mind = ((mag->mind / 100) * 100) + mag_mind;
	}

	return 0;
}
