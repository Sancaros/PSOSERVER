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

#include <stdint.h>

#include <pso_character.h>
#include <f_logs.h>
#include <items.h>

#include "mag_bb.h"
#include "clients.h"
#include "ship_packets.h"
#include "items.h"

//玛古同步概率的代码
void mag_bb_add_pb(uint8_t* flags, uint8_t* blasts, uint8_t pb)
{
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

//玛古的代码
int32_t mag_bb_alignment(mag_t* m)
{
	int32_t v1, v2, v3, v4, v5, v6;

	v4 = 0;
	v3 = m->power;
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

int32_t mag_bb_special_evolution(mag_t* m, uint8_t sectionID, uint8_t type, int32_t EvolutionClass)
{
	uint8_t oldType;
	int16_t mDefense, mPower, mDex, mMind;

	oldType = m->mtype;

	if (m->level >= 100)
	{
		mDefense = m->defense / 100;
		mPower = m->power / 100;
		mDex = m->dex / 100;
		mMind = m->mind / 100;

		switch (sectionID)
		{
		case ID_Viridia:
		case ID_Bluefull:
		case ID_Redria:
		case ID_Whitill:
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
		case ID_Skyly:
		case ID_Pinkal:
		case ID_Yellowboze:
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
		case ID_Greennill:
		case ID_Oran:
		case ID_Purplenum:
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

void mag_bb_lv50_evolution(mag_t* m, uint8_t sectionID, uint8_t type, int32_t EvolutionClass)
{
	int32_t v10, v11, v12, v13;

	int32_t Alignment = mag_bb_alignment(m);

	if (EvolutionClass > 3) // Don't bother to check if a special mag.
		return;

	v10 = m->power / 100;
	v11 = m->dex / 100;
	v12 = m->mind / 100;
	v13 = m->defense / 100;

	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108)
		{
			if (sectionID & 1)
			{
				if (v12 > v11)
				{
					m->mtype = Mag_Apsaras;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
				}
				else
				{
					m->mtype = Mag_Kama;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
				}
			}
			else
			{
				if (v12 > v11)
				{
					m->mtype = Mag_Bhirava;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
				}
			}
		}
		else
		{
			if (Alignment & 0x10)
			{
				if (sectionID & 1)
				{
					if (v10 > v12)
					{
						m->mtype = Mag_Garuda;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Yaksa;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
					}
				}
				else
				{
					if (v10 > v12)
					{
						m->mtype = Mag_Ila;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Nandin;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
					}
				}
			}
			else
			{
				if (Alignment & 0x20)
				{
					if (sectionID & 1)
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Soma;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Bana;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Ushasu;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
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
			if (sectionID & 1)
			{
				if (v10 > v12)
				{
					m->mtype = Mag_Kaitabha;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
				}
				else
				{
					m->mtype = Mag_Varaha;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
				}
			}
			else
			{
				if (v10 > v12)
				{
					m->mtype = Mag_Bhirava;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
				}
				else
				{
					m->mtype = Mag_Kama;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
				}
			}
		}
		else
		{
			if (Alignment & 0x08)
			{
				if (sectionID & 1)
				{
					if (v12 > v11)
					{
						m->mtype = Mag_Kaitabha;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Madhu;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
				}
				else
				{
					if (v12 > v11)
					{
						m->mtype = Mag_Bhirava;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
					}
					else
					{
						m->mtype = Mag_Kama;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
					}
				}
			}
			else
			{
				if (Alignment & 0x20)
				{
					if (sectionID & 1)
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Durga;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Kabanda;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
					}
					else
					{
						if (v11 > v10)
						{
							m->mtype = Mag_Apsaras;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
						}
						else
						{
							m->mtype = Mag_Varaha;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
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
				mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
			}
			else
			{
				if (sectionID & 1)
				{
					if (v11 > v10)
					{
						m->mtype = Mag_Ila;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Kumara;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
					}
				}
				else
				{
					if (v11 > v10)
					{
						m->mtype = Mag_Kabanda;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					}
					else
					{
						m->mtype = Mag_Naga;
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
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
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
				}
				else
				{
					if (sectionID & 1)
					{
						if (v12 > v11)
						{
							m->mtype = Mag_Naga;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
						}
						else
						{
							m->mtype = Mag_Marica;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
						}
					}
					else
					{
						if (v12 > v11)
						{
							m->mtype = Mag_Ravana;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Farlla);
						}
						else
						{
							m->mtype = Mag_Naraka;
							mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
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
						mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
					}
					else
					{
						if (sectionID & 1)
						{
							if (v10 > v12)
							{
								m->mtype = Mag_Garuda;
								mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
							}
							else
							{
								m->mtype = Mag_Bhirava;
								mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
							}
						}
						else
						{
							if (v10 > v12)
							{
								m->mtype = Mag_Ribhava;
								mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Farlla);
							}
							else
							{
								m->mtype = Mag_Sita;
								mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
							}
						}
					}
				}
			}
		}
		break;
	}
}

void mag_bb_lv35_evolution(mag_t* m, uint8_t sectionID, uint8_t type, int32_t EvolutionClass)
{
	int32_t Alignment = mag_bb_alignment(m);

	if (EvolutionClass > 3) // Don't bother to check if a special mag.
		return;

	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		if (Alignment & 0x108)
		{
			m->mtype = Mag_Rudra;
			mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
			return;
		}
		else
		{
			if (Alignment & 0x10)
			{
				m->mtype = Mag_Marutah;
				mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
				return;
			}
			else
			{
				if (Alignment & 0x20)
				{
					m->mtype = Mag_Vayu;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					return;
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
			m->mtype = Mag_Mitra;
			mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
			return;
		}
		else
		{
			if (Alignment & 0x08)
			{
				m->mtype = Mag_Surya;
				mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
				return;
			}
			else
			{
				if (Alignment & 0x20)
				{
					m->mtype = Mag_Tapas;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
					return;
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
			m->mtype = Mag_Namuci;
			mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Mylla_Youlla);
			return;
		}
		else
		{
			if (Alignment & 0x08)
			{
				m->mtype = Mag_Sumba;
				mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Golla);
				return;
			}
			else
			{
				if (Alignment & 0x10)
				{
					m->mtype = Mag_Ashvinau;
					mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Pilla);
					return;
				}
			}
		}
		break;
	}
}

void mag_bb_lv10_evolution(mag_t* m, uint8_t sectionID, uint8_t type, int32_t EvolutionClass)
{
	switch (type)
	{
	case CLASS_HUMAR:
	case CLASS_HUNEWEARL:
	case CLASS_HUCAST:
	case CLASS_HUCASEAL:
		m->mtype = Mag_Varuna;
		mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Farlla);
		break;
	case CLASS_RAMAR:
	case CLASS_RACAST:
	case CLASS_RACASEAL:
	case CLASS_RAMARL:
		m->mtype = Mag_Kalki;
		mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Estlla);
		break;
	case CLASS_FONEWM:
	case CLASS_FONEWEARL:
	case CLASS_FOMARL:
	case CLASS_FOMAR:
		m->mtype = Mag_Vritra;
		mag_bb_add_pb(&m->PBflags, &m->blasts, PB_Leilla);
		break;
	}
}

void mag_bb_check_evolution(mag_t* m, uint8_t sectionID, uint8_t type, int32_t EvolutionClass)
{
	if ((m->level < 10) || (m->level >= 35))
	{
		if ((m->level < 35) || (m->level >= 50))
		{
			if (m->level >= 50)
			{
				if (!(m->level % 5)) // Divisible by 5 with no remainder.
				{
					if (EvolutionClass <= 3)
					{
						if (!mag_bb_special_evolution(m, sectionID, type, EvolutionClass))
							mag_bb_lv50_evolution(m, sectionID, type, EvolutionClass);
					}
				}
			}
		}
		else
		{
			if (EvolutionClass < 2)
				mag_bb_lv35_evolution(m, sectionID, type, EvolutionClass);
		}
	}
	else
	{
		if (EvolutionClass <= 0)
			mag_bb_lv10_evolution(m, sectionID, type, EvolutionClass);
	}
}

//喂养玛古
int mag_bb_feed(ship_client_t* c, uint32_t itemid, uint32_t magid) {
	lobby_t* l = c->cur_lobby;

	if (!l || l->type != LOBBY_TYPE_GAME)
		return -1;

	if (l->challenge || l->battle)
		return -2;

	int32_t found_mag = -1;
	int32_t found_item = -1;
	uint32_t ch, ch2, mt_index;
	int32_t EvolutionClass = 0;
	mag_t* mag;
	uint16_t* ft;
	int16_t mIQ, mDefense, mPower, mDex, mMind;

	for (ch = 0; ch < c->pl->bb.inv.item_count; ch++)
	{
		if (c->pl->bb.inv.iitems[ch].data.item_id == magid)
		{
			// Found mag
			if ((c->pl->bb.inv.iitems[ch].data.data_b[0] == 0x02) &&
				(c->pl->bb.inv.iitems[ch].data.data_b[1] <= Mag_Agastya))
			{
				found_mag = ch;
				mag = (mag_t*)&c->pl->bb.inv.iitems[ch].data.data_b[0];
				for (ch2 = 0; ch2 < c->pl->bb.inv.item_count; ch2++)
				{
					if (c->pl->bb.inv.iitems[ch2].data.item_id == itemid)
					{
						// Found item to feed
						if ((c->pl->bb.inv.iitems[ch2].data.data_b[0] == 0x03) &&
							(c->pl->bb.inv.iitems[ch2].data.data_b[1] < 0x07) &&
							(c->pl->bb.inv.iitems[ch2].data.data_b[1] != 0x02) &&
							(c->pl->bb.inv.iitems[ch2].data.data_b[5] > 0x00))
						{
							found_item = ch2;
							switch (c->pl->bb.inv.iitems[ch2].data.data_b[1])
							{
							case 0x00:
								mt_index = c->pl->bb.inv.iitems[ch2].data.data_b[2];
								break;
							case 0x01:
								mt_index = 3 + c->pl->bb.inv.iitems[ch2].data.data_b[2];
								break;
							case 0x03:
							case 0x04:
							case 0x05:
								mt_index = 5 + c->pl->bb.inv.iitems[ch2].data.data_b[1];
								break;
							case 0x06:
								mt_index = 6 + c->pl->bb.inv.iitems[ch2].data.data_b[2];
								break;
							}
						}
						break;
					}
				}
			}
			break;
		}
	}

	//send_msg_box(c, "找到需要喂养的玛古或者需要喂养的物品. %d %d", found_mag, found_item);

	//代码缺失 Sancaros 修复挑战模式喂养玛古的报错断线
	if ((found_mag == -1) || (found_item == -1))
	{
		send_msg_box(c, "无法找到需要喂养的玛古或者需要喂养的物品.");
		return -3;
	}
	else
	{
		/* Remove the item from the user's inventory. */
		if (item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
			itemid, 0xFFFFFFFF) < 1) {
			ERR_LOG("无法从玩家背包中移除物品!");
			return -4;
		}

		--c->bb_pl->inv.item_count;

		//Delete_Item_From_Client(itemid, 1, 0, c);

		// Rescan to update Mag pointer (if changed due to clean up) 重新扫描以更新磁指针（如果由于清理而更改） 
		for (ch = 0; ch < c->pl->bb.inv.item_count; ch++)
		{
			if (c->pl->bb.inv.iitems[ch].data.item_id == magid)
			{
				// Found mag (again) 再次搜寻玛古
				if ((c->pl->bb.inv.iitems[ch].data.data_b[0] == 0x02) &&
					(c->pl->bb.inv.iitems[ch].data.data_b[1] <= Mag_Agastya))
				{
					found_mag = ch;

					mag = (mag_t*)&c->pl->bb.inv.iitems[ch].data.data_b[0];
					break;
				}
			}
		}

		// Feed that mag (Updates to code by Lee from schtserv.com)
		switch (mag->mtype)
		{
		case Mag_Mag:
			ft = &mag_feed_t[0][0];
			EvolutionClass = 0;
			break;
		case Mag_Varuna:
		case Mag_Vritra:
		case Mag_Kalki:
			EvolutionClass = 1;
			ft = &mag_feed_t[1][0];
			break;
		case Mag_Ashvinau:
		case Mag_Sumba:
		case Mag_Namuci:
		case Mag_Marutah:
		case Mag_Rudra:
			ft = &mag_feed_t[2][0];
			EvolutionClass = 2;
			break;
		case Mag_Surya:
		case Mag_Tapas:
		case Mag_Mitra:
			ft = &mag_feed_t[3][0];
			EvolutionClass = 2;
			break;
		case Mag_Apsaras:
		case Mag_Vayu:
		case Mag_Varaha:
		case Mag_Ushasu:
		case Mag_Kama:
		case Mag_Kaitabha:
		case Mag_Kumara:
		case Mag_Bhirava:
			EvolutionClass = 3;
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
			EvolutionClass = 3;
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
			EvolutionClass = 3;
			ft = &mag_feed_t[6][0];
			break;
		case Mag_Deva:
		case Mag_Rukmin:
		case Mag_Sato:
			ft = &mag_feed_t[5][0];
			EvolutionClass = 4;
			break;
		case Mag_Rati:
		case Mag_Pushan:
		case Mag_Bhima:
			ft = &mag_feed_t[6][0];
			EvolutionClass = 4;
			break;
		default:
			ft = &mag_feed_t[7][0];
			EvolutionClass = 4;
			break;
		}
		mt_index *= 6;
		mag->synchro += ft[mt_index];
		if (mag->synchro < 0)
			mag->synchro = 0;
		if (mag->synchro > 120)
			mag->synchro = 120;
		mIQ = mag->IQ;
		mIQ += ft[mt_index + 1];
		if (mIQ < 0)
			mIQ = 0;
		if (mIQ > 200)
			mIQ = 200;
		mag->IQ = (uint8_t)mIQ;

		// Add Defense

		mDefense = mag->defense % 100;
		mDefense += ft[mt_index + 2];

		if (mDefense < 0)
			mDefense = 0;

		if (mDefense >= 100)
		{
			if (mag->level == MAX_PLAYER_LEVEL)
				mDefense = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->defense = ((mag->defense / 100) * 100) + mDefense;
			mag_bb_check_evolution(mag, c->pl->bb.character.disp.dress_data.section, c->pl->bb.character.disp.dress_data.ch_class, EvolutionClass);
		}
		else
			mag->defense = ((mag->defense / 100) * 100) + mDefense;

		// Add Power

		mPower = mag->power % 100;
		mPower += ft[mt_index + 3];

		if (mPower < 0)
			mPower = 0;

		if (mPower >= 100)
		{
			if (mag->level == MAX_PLAYER_LEVEL)
				mPower = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->power = ((mag->power / 100) * 100) + mPower;
			mag_bb_check_evolution(mag, c->pl->bb.character.disp.dress_data.section, c->pl->bb.character.disp.dress_data.ch_class, EvolutionClass);
		}
		else
			mag->power = ((mag->power / 100) * 100) + mPower;

		// Add Dex

		mDex = mag->dex % 100;
		mDex += ft[mt_index + 4];

		if (mDex < 0)
			mDex = 0;

		if (mDex >= 100)
		{
			if (mag->level == MAX_PLAYER_LEVEL)
				mDex = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->dex = ((mag->dex / 100) * 100) + mDex;
			mag_bb_check_evolution(mag, c->pl->bb.character.disp.dress_data.section, c->pl->bb.character.disp.dress_data.ch_class, EvolutionClass);
		}
		else
			mag->dex = ((mag->dex / 100) * 100) + mDex;

		// Add Mind

		mMind = mag->mind % 100;
		mMind += ft[mt_index + 5];

		if (mMind < 0)
			mMind = 0;

		if (mMind >= 100)
		{
			if (mag->level == MAX_PLAYER_LEVEL)
				mMind = 99; // Don't go above level 200
			else
				mag->level++; // Level up!
			mag->mind = ((mag->mind / 100) * 100) + mMind;
			mag_bb_check_evolution(mag, c->pl->bb.character.disp.dress_data.section, c->pl->bb.character.disp.dress_data.ch_class, EvolutionClass);
		}
		else
			mag->mind = ((mag->mind / 100) * 100) + mMind;
	}

	return 0;
}
