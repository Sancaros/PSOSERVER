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

uint32_t bp_rappy_normal[] = {
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
};

uint32_t bp_rappy_hard[] = {
	BBItem_CAT_EARS, BBItem_INVISIBLE_GUARD, BBItem_YATA_MIRROR, 0x400101,
	0x440301, 0x460301, 0x450301, 0x470301,
	BBItem_Rappys_Beak, 
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_rappy_vhard[] =
{
	0xCB00, 0x3A00, 0x028C00, 0x2B0201,
	0x5000, 0x060B00, 0x060A00, 0x040A00,
	BBItem_RABBIT_WAND, 0x2300, 0x3B00, BBItem_Rappys_Beak,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_rappy_ultimate[] =
{
	0x5100, 0x520301, 0x200301, 0x3E0301,
	0x290201, BBItem_Rappys_Beak, 0x040B00, 0x060A00,
	0x5600, 0x3B00, 0x2300, 0x050A00,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_zu_normal[] =
{
	0x320101, 0x012F00, 0xB300, 0x5E00,
	0x020E00, 0x2E00, 0x9500, 0x9A00,
	0x2F00, 0x1B0301, 
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_zu_hard[] =
{
	0xC000, 0xD200, 0x8D00, 0x2E0101,
	0x8B00, 0x070900, 0x4E00, 0x6D00,
	0x1500, 0x028B00, 
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_zu_vhard[] =
{
	0xAA00, 0x410101, 0x510101, 0x230201,
	0x3F00, 0x4100, 0x070500, 0x060500,
	0x050500, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_zu_ultimate[] =
{
	0xAF00, 0x4300, 0x510301, 0xCD00,
	0x9900, 0x6C00, 0x4500, 0x6B00,
	0x1200, 0x6500, 0x290201, 0x1300,
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_dorphon_normal[] =
{
	0x9000, 0x019000, 0x029000, 0x039000,
	0x049000, 0x059000, 0x069000, 0x079000,
	0x089000, 0xB400, 0x4E0101, 0x070301,
	0x410301, 
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_dorphon_hard[] =
{
	0xB900, 0x3400, 0x010900, 0x029000,
	0x079000, 0x2C00, 0x2D00, 0x350201,
	0x060100, 0x050100, 
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_dorphon_vhard[] =
{
	0xB600, 0x018A00, 0x011000, 0x021000,
	0x031000, 0x041000, 0x051000, 0x061000,
	0x2700, 0x070100, 
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};

uint32_t bp_dorphon_ultimate[] =
{
	0xB700, 0x011000, 0x021000, 0x031000,
	0x041000, 0x051000, 0x061000, 0x2900,
	0x8A00, 0x028A00, 
	BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
	BBItem_Meseta, BBItem_Meseta, BBItem_Meseta, BBItem_Meseta,
};


// Dangerous Deal with Black Paper 2 reward lists
uint32_t bp2_normal[] =
{
	0x00BA00, 0x030D00, 0x014300, 0x080700,
	0x014200, 0x00c900, 0x001003, 0x950201,
	0x8F0201, 0x910201
};

uint32_t bp2_hard[] =
{
	0x00BB00, 0x030D00, 0x00B700, 0x014200,
	0x080700, 0x00c900, 0x360101, 0x8A0201,
	0x990201, 0x510301, 0x5B0301, 0x520301,
	0x001003, 0x0A1803
};

uint32_t bp2_vhard[] =
{
	0x00BA00, 0x00B400, 0x030D00, 0x00B600,
	0x00B300, 0x080700, 0x014300, 0x00c900,
	0x360101, 0x8A0201, 0x990201, 0x850201,
	0x480301, 0x510301, 0x5B0301, 0x520301,
	0x001003
};


uint32_t bp2_ultimate[] =
{
	0x00BA00, 0x00B400, 0x030D00, 0x00B600,
	0x00B300, 0x080700, 0x014300, 0x00c900,
	0x360101, 0x8A0201, 0x990201, 0x850201,
	0x480301, 0x510301, 0x5B0301, 0x520301
};

#endif // !BB_ITEMS_BP_REWARD_LIST

