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

//释放房间物品内存
uint32_t free_lobby_item(lobby_t* l);
/* 初始化房间物品列表数据 */
void cleanup_lobby_item(lobby_t* l);
/* 修复玩家背包数据 */
void fix_up_pl_iitem(lobby_t* l, ship_client_t* c);
/* 初始化物品数据 */
void clear_item(item_t* item);
/* 初始化背包物品数据 */
void clear_iitem(iitem_t* iitem);
/* 初始化银行物品数据 */
void clear_bitem(bitem_t* bitem);
/* 初始化房间物品数据 */
void clear_fitem(fitem_t* fitem);

/* 新增一件物品至大厅背包中. 调用者在调用这个之前必须持有大厅的互斥锁.
如果大厅的库存中没有新物品的空间,则返回NULL. */
iitem_t* lobby_add_new_item_locked(lobby_t* l, item_t* new_item);
iitem_t* lobby_add_item_locked(lobby_t* l, iitem_t* item);

int lobby_remove_item_locked(lobby_t* l, uint32_t item_id, iitem_t* rv);

/* 生成物品ID */
uint32_t generate_item_id(lobby_t* l, uint8_t client_id);

// TODO: Eliminate duplication between this function and the parallel function
// in PlayerBank
size_t stack_size_for_item(item_t item);

/* 获取背包中目标物品所在槽位 */
size_t find_inv_item_slot(inventory_t* inv, uint32_t item_id);

/* 移除背包物品操作 */
int item_remove_from_inv(iitem_t *inv, int inv_count, uint32_t item_id,
                         uint32_t amt);

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

/* 移除背包物品 */
iitem_t remove_item(ship_client_t* c, uint32_t item_id, uint32_t amount, bool allow_meseta_overdraft);

//修复背包银行数据错误的物品代码
void fix_inv_bank_item(item_t* i);

//修复背包装备数据错误的物品代码
void fix_equip_item(inventory_t* inv);

#endif /* !IITEMS_H */
