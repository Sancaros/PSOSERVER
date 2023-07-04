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

#ifndef IITEMS_H
#define IITEMS_H

#include <pso_character.h>

#include <pso_player.h>
#include <pso_items.h>
#include "clients.h"
#include "shop.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////物品操作

/* 初始化物品数据 */
void clear_item(item_t* item);
/* 生成物品ID */
uint32_t generate_item_id(lobby_t* l, uint8_t client_id);

uint32_t primary_identifier(item_t* i);

/* 堆叠物检测 */
bool is_stackable(const item_t* item);
size_t stack_size(const item_t* item);
size_t max_stack_size(const item_t* item);
size_t max_stack_size_for_item(uint8_t data0, uint8_t data1);
//
//// TODO: Eliminate duplication between this function and the parallel function
//// in PlayerBank
//size_t stack_size_for_item(item_t item);

////////////////////////////////////////////////////////////////////////////////////////////////////
////玩家背包操作

/* 初始化背包物品数据 */
void clear_iitem(iitem_t* iitem);

/* 修复玩家背包数据 */
void fix_up_pl_iitem(lobby_t* l, ship_client_t* c);

////////////////////////////////////////////////////////////////////////////////////////////////////
////银行背包操作

/* 初始化银行物品数据 */
void clear_bitem(bitem_t* bitem);


////////////////////////////////////////////////////////////////////////////////////////////////////
////游戏房间操作

/* 新增一件物品至大厅背包中. 调用者在调用这个之前必须持有大厅的互斥锁.
如果大厅的库存中没有新物品的空间,则返回NULL. */
iitem_t* lobby_add_new_item_locked(lobby_t* l, item_t* new_item);
iitem_t* lobby_add_item_locked(lobby_t* l, iitem_t* item);

int lobby_remove_item_locked(lobby_t* l, uint32_t item_id, iitem_t* rv);

/* 获取背包中目标物品所在槽位 */
int find_iitem_slot(inventory_t* inv, uint32_t item_id);
size_t find_equipped_weapon(inventory_t* inv);
size_t find_equipped_armor(inventory_t* inv);
size_t find_equipped_mag(inventory_t* inv);

/* 移除背包物品操作 */
int item_remove_from_inv(iitem_t *inv, int inv_count, uint32_t item_id,
                         uint32_t amt);
iitem_t remove_item(ship_client_t* src, uint32_t item_id, uint32_t amount, bool allow_meseta_overdraft);

/* 蓝色脉冲银行管理 */
void cleanup_bb_bank(ship_client_t *c);
int item_deposit_to_bank(ship_client_t *c, bitem_t *it);
int item_take_from_bank(ship_client_t *c, uint32_t item_id, uint8_t amt,
                        bitem_t *rv);

/* 物品检测装备标签 */
int item_check_equip(uint8_t 装备标签, uint8_t 客户端装备标签);
int item_check_equip_flags(ship_client_t* c, uint32_t item_id);
/* 给客户端标记可穿戴职业装备的标签 */
int item_class_tag_equip_flag(ship_client_t* c);

/* 增加背包物品 */
int item_add_to_inv(ship_client_t* c, iitem_t* iitem);

/* 增加物品至客户端 */
int add_item_to_client(ship_client_t* c, iitem_t* iitem);

//修复背包银行数据错误的物品代码
void fix_inv_bank_item(item_t* i);

//修复背包装备数据错误的物品代码
void fix_equip_item(inventory_t* inv);

/* 清理背包物品 */
void clean_up_inv(inventory_t* inv);
void sort_client_inv(inventory_t* inv);
void clean_up_bank(psocn_bank_t* bank);

#endif /* !IITEMS_H */
