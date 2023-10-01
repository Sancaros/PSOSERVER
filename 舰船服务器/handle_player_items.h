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

bool check_structs_equal(const void* s1, const void* s2, size_t sz);

/* 生成物品ID */
size_t generate_item_id(lobby_t* l, size_t client_id);
size_t destroy_item_id(lobby_t* l, size_t client_id);

/* 修复玩家背包数据 */
void regenerate_lobby_item_id(lobby_t* l, ship_client_t* c);

////////////////////////////////////////////////////////////////////////////////////////////////////
////游戏房间操作

/* 新增一件物品至大厅背包中. 调用者在调用这个之前必须持有大厅的互斥锁.
如果大厅的库存中没有新物品的空间,则返回NULL. */
litem_t* add_new_litem_locked(lobby_t* l, item_t* new_item, uint8_t area, float x, float z);
litem_t* add_litem_locked(lobby_t* l, iitem_t* iitem, uint8_t area, float x, float z);

int remove_litem_locked(lobby_t* l, uint32_t item_id, iitem_t* rv);

/* 获取大厅中目标物品所在槽位 */
size_t find_litem_index(lobby_t* l, item_t* item);

/* 获取背包中目标物品所在槽位 */
bool find_mag_and_feed_item(const inventory_t* inv,
    const uint32_t mag_id,
    const uint32_t item_id,
    size_t* mag_item_index,
    size_t* feed_item_index);
int find_iitem_index(const inventory_t* inv, const uint32_t item_id);
int find_titem_index(const trade_inv_t* trade, const uint32_t item_id);
int check_titem_id(const trade_inv_t* trade, const uint32_t item_id);
int find_bitem_index(const psocn_bank_t* bank, const uint32_t item_id);
size_t find_iitem_stack_item_id(const inventory_t* inv, const iitem_t* iitem);
size_t find_iitem_code_stack_item_id(const inventory_t* inv, const uint32_t code);
size_t find_iitem_pid(const inventory_t* inv, const iitem_t* iitem);
int find_iitem_pid_index(const inventory_t* inv, const iitem_t* iitem);
int find_equipped_weapon(const inventory_t* inv);
int find_equipped_armor(const inventory_t* inv);
int find_equipped_mag(const inventory_t* inv);

void bswap_data2_if_mag(item_t* item);

/* 玩家美赛塔操作 内存操作 */
int add_character_meseta(psocn_bb_char_t* character, uint32_t amount);
int remove_character_meseta(psocn_bb_char_t* character, uint32_t amount, bool allow_overdraft);

/* 移除背包物品操作 */
int remove_iitem_v1(iitem_t *inv, int inv_count, uint32_t item_id, uint32_t amt);
iitem_t remove_iitem(ship_client_t* src, uint32_t item_id, uint32_t amount, bool allow_meseta_overdraft);
iitem_t remove_titem(trade_inv_t* trade, uint32_t item_id, uint32_t amount);
bitem_t remove_bitem(ship_client_t* src, uint32_t item_id, uint16_t bitem_index, uint32_t amount);
bool add_iitem(ship_client_t* src, const iitem_t iitem);
bool add_titem(trade_inv_t* trade, const iitem_t iitem);
bool add_bitem(ship_client_t* src, const bitem_t bitem);
int player_use_item(ship_client_t* src, uint32_t item_id);
int player_tekker_item(ship_client_t* src, sfmt_t* rng, item_t* item);

/* 挑战模式专用 */
int initialize_cmode_iitem(ship_client_t* dest);

/* 蓝色脉冲物品管理 */
iitem_t player_iitem_init(const item_t item);
trade_inv_t* player_tinv_init(ship_client_t* src);
bitem_t player_bitem_init(const item_t item);
void cleanup_bb_inv(uint32_t client_id, inventory_t* inv);
void regenerate_bank_item_id(uint32_t client_id, psocn_bank_t* bank, bool comoon_bank);

/* 物品检测装备标签 */
bool item_check_equip(uint8_t 装备标签, uint8_t 客户端装备标签);
int item_check_equip_flags(ship_client_t* src, uint32_t item_id);
/* 给客户端标记可穿戴职业装备的标签 */
void item_class_tag_equip_flag(ship_client_t* c);
void remove_titem_equip_flags(iitem_t* trade_item);

//修复背包银行数据错误的物品代码
void fix_inv_bank_item(item_t* i);

//修复背包装备数据错误的物品代码
void fix_equip_item(inventory_t* inv);

/* 清理背包物品 */
void fix_client_inv(inventory_t* inv);
void sort_client_inv(inventory_t* inv);
void fix_client_bank(psocn_bank_t* bank);
void sort_client_bank(psocn_bank_t* bank);

#endif /* !IITEMS_H */
