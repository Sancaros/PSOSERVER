/*
	�λ�֮���й� ���������� ������Ʒ�б�
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

#ifndef PSO_GUILD_SPECIAL_ITEM_LIST_H
#define PSO_GUILD_SPECIAL_ITEM_LIST_H

#include <stdint.h>

typedef struct bb_guild_special_items {
    char item_name[64];
    char item_desc[128];
    uint32_t point_amount;
} bb_guild_special_items_t;

static const bb_guild_special_items_t guild_special_items[] = {
    {"����ר��", "\tE\tC6��������Ĺ���.", 0},
    {"�����־", "\tE\tC6��������ı�־����,ֻ�л᳤���Ը��Ĺ����־.", 1000},
    {"������", "\tE\tC6��������ĸ����ҹ���.", 1000},
    {"������", "\tE\tC6��������ĸ�������.", 1000},
    {"�������� 20", "\tE\tC6������������Ϊ20��,���᳤�����޶�Ϊ3��.", 1000},
    {"�������� 40", "\tE\tC6������������Ϊ40��,���᳤�����޶�Ϊ5��.", 2000},
    {"�������� 70", "\tE\tC6������������Ϊ70��,���᳤�����޶�Ϊ8��.", 5000},
    {"�������� 100", "\tE\tC6������������Ϊ70��,���᳤�����޶�Ϊ8��.", 10000},
    {"�᳤ָ�ӹٴ�", "\tE\tC6����᳤ָ�ӹٴ�", 5000},
    {"�������", "\tE\tC6���򹫻����", 100},
    {"�������", "\tE\tC6���򹫻����", 100},
    {"������� 500", "\tE\tC6�һ�������� 500", 500},
    {"������� 1000", "\tE\tC6�һ�������� 1000", 1000},
    {"������� 5000", "\tE\tC6�һ�������� 5000", 5000},
    {"������� 10000", "\tE\tC6�һ�������� 10000", 10000},
    {"��������: ������������", "\tE\tC6������������", 1500},
};

#endif // !PSO_GUILD_SPECIAL_ITEM_LIST_H
