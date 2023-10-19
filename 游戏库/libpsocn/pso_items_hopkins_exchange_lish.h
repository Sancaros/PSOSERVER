/*
	梦幻之星中国 舰船服务器 霍普金斯兑换物品列表
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

#ifndef PSOCN_HOPKINS_EXCHANGE_LIST_H
#define PSOCN_HOPKINS_EXCHANGE_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include "f_logs.h"

#include "pso_items.h"

// Gallon's商店"Hopkins的爸爸"物品列表+每个物品所需的Photon Drops
typedef struct gallons_shop_hopkins_pd_item_exchange {
    uint32_t item_datal;  // 物品ID
    uint8_t pd_num;  // 所需的Photon Drops数量
} gallons_shop_hopkins_pd_item_exchange_t;

const static gallons_shop_hopkins_pd_item_exchange_t gallons_shop_hopkins[] = {
    {BBItem_Monomate,               1},  // 单体恢复药水，需要1个Photon Drops
    {BBItem_Monofluid,              1},  // 单体补充魔法药水，需要1个Photon Drops
    {BBItem_AddSlot,                1},  // 增加物品栏位，需要1个Photon Drops
    {BBItem_RECOVERY_BARRIER,       1},  // 恢复屏障，需要1个Photon Drops
    {BBItem_ASSIST__BARRIER,        1},  // 辅助屏障，需要1个Photon Drops
    {BBItem_Magic_Water,            20},  // 魔法之水，需要20个Photon Drops
    {BBItem_Parasitic_cell_Type_D,  20},  // 寄生细胞D型，需要20个Photon Drops
    {BBItem_Berill_Photon,          20},  // 贝里尔光子，需要20个Photon Drops
    {BBItem_RED_BARRIER,            20},  // 红色屏障，需要20个Photon Drops
    {BBItem_BLUE_BARRIER,           20},  // 蓝色屏障，需要20个Photon Drops
    {BBItem_YELLOW_BARRIER,         20},  // 黄色屏障，需要20个Photon Drops
    {BBItem_NEIS_CLAW_2,            20},  // 奈斯之爪2，需要20个Photon Drops
    {BBItem_BROOM,                  20},  // 扫帚，需要20个Photon Drops
    {BBItem_FLOWER_CANE,            20},  // 花杖，需要20个Photon Drops
    {BBItem_Blue_black_stone,       50},  // 蓝黑之石，需要50个Photon Drops
    {BBItem_magic_rock_Heart_Key,   50},  // 魔石-心之钥，需要50个Photon Drops
    {BBItem_Photon_Booster,         50},  // 光子增幅器，需要50个Photon Drops
    {BBItem_Photon_Sphere,          99}  // 光子球，需要99个Photon Drops
};

// Gallon's商店"Hopkins的爸爸"所需的S级属性的Photon Drops数量
typedef struct gallons_shop_hopkins_pd_srank_exchange {
    uint8_t special_type;  // 特殊类型
    uint8_t pd_num;  // 所需的Photon Drops数量
} gallons_shop_hopkins_pd_srank_exchange_t;

const static gallons_shop_hopkins_pd_srank_exchange_t gallon_srank_pds[] = {
    {Weapon_Srank_Attr_Jellen,      60},  // 特殊类型1，需要60个Photon Drops
    {Weapon_Srank_Attr_Zalure,      60},  // 特殊类型2，需要60个Photon Drops
    {Weapon_Srank_Attr_HP_Regen,    20},  // 特殊类型3，需要20个Photon Drops
    {Weapon_Srank_Attr_TP_Regen,    20},  // 特殊类型4，需要20个Photon Drops
    {Weapon_Srank_Attr_Burning,     30},  // 特殊类型5，需要30个Photon Drops
    {Weapon_Srank_Attr_Tempest,     30},  // 特殊类型6，需要30个Photon Drops
    {Weapon_Srank_Attr_Blizzard,    30},  // 特殊类型7，需要30个Photon Drops
    {Weapon_Srank_Attr_Arrest,      50},  // 特殊类型8，需要50个Photon Drops
    {Weapon_Srank_Attr_Chaos,       40},  // 特殊类型9，需要40个Photon Drops
    {Weapon_Srank_Attr_Hell,        50},  // 特殊类型10，需要50个Photon Drops
    {Weapon_Srank_Attr_Spirit,      40},  // 特殊类型11，需要40个Photon Drops
    {Weapon_Srank_Attr_Berserk,     40},  // 特殊类型12，需要40个Photon Drops
    {Weapon_Srank_Attr_Demons,      50},  // 特殊类型13，需要50个Photon Drops
    {Weapon_Srank_Attr_Gush,        40},  // 特殊类型14，需要40个Photon Drops
    {Weapon_Srank_Attr_Geist,       40},  // 特殊类型15，需要40个Photon Drops
    {Weapon_Srank_Attr_Kings,       40}  // 特殊类型16，需要40个Photon Drops
};
#endif // !PSOCN_HOPKINS_EXCHANGE_LIST_H