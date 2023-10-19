/*
	�λ�֮���й� ���������� ���ս�˹�һ���Ʒ�б�
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

#ifndef PSOCN_HOPKINS_EXCHANGE_LIST_H
#define PSOCN_HOPKINS_EXCHANGE_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include "f_logs.h"

#include "pso_items.h"

// Gallon's�̵�"Hopkins�İְ�"��Ʒ�б�+ÿ����Ʒ�����Photon Drops
typedef struct gallons_shop_hopkins_pd_item_exchange {
    uint32_t item_datal;  // ��ƷID
    uint8_t pd_num;  // �����Photon Drops����
} gallons_shop_hopkins_pd_item_exchange_t;

const static gallons_shop_hopkins_pd_item_exchange_t gallons_shop_hopkins[] = {
    {BBItem_Monomate,               1},  // ����ָ�ҩˮ����Ҫ1��Photon Drops
    {BBItem_Monofluid,              1},  // ���岹��ħ��ҩˮ����Ҫ1��Photon Drops
    {BBItem_AddSlot,                1},  // ������Ʒ��λ����Ҫ1��Photon Drops
    {BBItem_RECOVERY_BARRIER,       1},  // �ָ����ϣ���Ҫ1��Photon Drops
    {BBItem_ASSIST__BARRIER,        1},  // �������ϣ���Ҫ1��Photon Drops
    {BBItem_Magic_Water,            20},  // ħ��֮ˮ����Ҫ20��Photon Drops
    {BBItem_Parasitic_cell_Type_D,  20},  // ����ϸ��D�ͣ���Ҫ20��Photon Drops
    {BBItem_Berill_Photon,          20},  // ��������ӣ���Ҫ20��Photon Drops
    {BBItem_RED_BARRIER,            20},  // ��ɫ���ϣ���Ҫ20��Photon Drops
    {BBItem_BLUE_BARRIER,           20},  // ��ɫ���ϣ���Ҫ20��Photon Drops
    {BBItem_YELLOW_BARRIER,         20},  // ��ɫ���ϣ���Ҫ20��Photon Drops
    {BBItem_NEIS_CLAW_2,            20},  // ��˹֮צ2����Ҫ20��Photon Drops
    {BBItem_BROOM,                  20},  // ɨ�㣬��Ҫ20��Photon Drops
    {BBItem_FLOWER_CANE,            20},  // ���ȣ���Ҫ20��Photon Drops
    {BBItem_Blue_black_stone,       50},  // ����֮ʯ����Ҫ50��Photon Drops
    {BBItem_magic_rock_Heart_Key,   50},  // ħʯ-��֮Կ����Ҫ50��Photon Drops
    {BBItem_Photon_Booster,         50},  // ��������������Ҫ50��Photon Drops
    {BBItem_Photon_Sphere,          99}  // ��������Ҫ99��Photon Drops
};

// Gallon's�̵�"Hopkins�İְ�"�����S�����Ե�Photon Drops����
typedef struct gallons_shop_hopkins_pd_srank_exchange {
    uint8_t special_type;  // ��������
    uint8_t pd_num;  // �����Photon Drops����
} gallons_shop_hopkins_pd_srank_exchange_t;

const static gallons_shop_hopkins_pd_srank_exchange_t gallon_srank_pds[] = {
    {Weapon_Srank_Attr_Jellen,      60},  // ��������1����Ҫ60��Photon Drops
    {Weapon_Srank_Attr_Zalure,      60},  // ��������2����Ҫ60��Photon Drops
    {Weapon_Srank_Attr_HP_Regen,    20},  // ��������3����Ҫ20��Photon Drops
    {Weapon_Srank_Attr_TP_Regen,    20},  // ��������4����Ҫ20��Photon Drops
    {Weapon_Srank_Attr_Burning,     30},  // ��������5����Ҫ30��Photon Drops
    {Weapon_Srank_Attr_Tempest,     30},  // ��������6����Ҫ30��Photon Drops
    {Weapon_Srank_Attr_Blizzard,    30},  // ��������7����Ҫ30��Photon Drops
    {Weapon_Srank_Attr_Arrest,      50},  // ��������8����Ҫ50��Photon Drops
    {Weapon_Srank_Attr_Chaos,       40},  // ��������9����Ҫ40��Photon Drops
    {Weapon_Srank_Attr_Hell,        50},  // ��������10����Ҫ50��Photon Drops
    {Weapon_Srank_Attr_Spirit,      40},  // ��������11����Ҫ40��Photon Drops
    {Weapon_Srank_Attr_Berserk,     40},  // ��������12����Ҫ40��Photon Drops
    {Weapon_Srank_Attr_Demons,      50},  // ��������13����Ҫ50��Photon Drops
    {Weapon_Srank_Attr_Gush,        40},  // ��������14����Ҫ40��Photon Drops
    {Weapon_Srank_Attr_Geist,       40},  // ��������15����Ҫ40��Photon Drops
    {Weapon_Srank_Attr_Kings,       40}  // ��������16����Ҫ40��Photon Drops
};
#endif // !PSOCN_HOPKINS_EXCHANGE_LIST_H