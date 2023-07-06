/*
    梦幻之星中国 舰船服务器 副指令发包
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
#include <stdio.h>
#include <stdbool.h>

#include "clients.h"
#include "subcmd.h"
#include "ship_packets.h"

/* 0x5D SUBCMD60_DROP_STACK BB 掉落堆叠物品*/
int subcmd_send_lobby_drop_stack(ship_client_t* c, uint32_t area, float x,
    float z, iitem_t* item);

/* 0x59 SUBCMD60_DEL_MAP_ITEM BB 拾取物品 */
int subcmd_send_bb_del_map_item(ship_client_t* c, uint32_t area, uint32_t item_id);

/* 0xBE SUBCMD60_CREATE_ITEM BB 单人获得物品 */
int subcmd_send_bb_create_inv_item(ship_client_t* c, item_t item);

/* 0xBE SUBCMD60_CREATE_ITEM BB 发送给大厅玩家物品 用于SHOP类型的获取 */
int subcmd_send_lobby_bb_create_inv_item(ship_client_t* src, item_t item, bool send_to_src);

/* 0xB9 SUBCMD62_TEKKED_RESULT BB 单人获得鉴定物品 */
int subcmd_send_bb_create_tekk_item(ship_client_t* c, item_t item);

/* 0x29 SUBCMD60_DELETE_ITEM BB 消除物品 */
int subcmd_send_bb_destroy_item(ship_client_t* c, uint32_t item_id,
    uint8_t amt);

/* BB 从客户端移除美赛塔 */
int subcmd_send_bb_delete_meseta(ship_client_t* c, uint32_t count, uint32_t drop);

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB 怪物掉落物品 */
int subcmd_send_bb_gm_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req);

int subcmd_send_bb_quest_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req, ship_client_t* dest);

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB 怪物掉落物品 */
int subcmd_send_bb_lobby_item(lobby_t* l, subcmd_bb_itemreq_t* req, const iitem_t* item);

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB 怪物掉落物品 */
int subcmd_send_bb_enemy_item_req(lobby_t* l, subcmd_bb_itemreq_t* req, const iitem_t* item);

/* 0xBF SUBCMD60_GIVE_EXP BB 玩家获得经验 */
int subcmd_send_bb_exp(ship_client_t* c, uint32_t exp_amount);

/* 0xDD SUBCMD60_SET_EXP_RATE BB 设置游戏经验倍率 */
int subcmd_send_bb_set_exp_rate(ship_client_t* c, uint32_t exp_rate);

/* 0x30 SUBCMD60_LEVEL_UP BB 玩家升级数值变化 */
int subcmd_send_bb_level(ship_client_t* c);

/* 0xB6 SUBCMD60_SHOP_INV BB 向玩家发送货物清单 */
int subcmd_bb_send_shop(ship_client_t* c, uint8_t shop_type, uint8_t num_items);

int subcmd_bb_60size_check(ship_client_t* c, subcmd_bb_pkt_t* pkt);

int subcmd_bb_626Dsize_check(ship_client_t* c, subcmd_bb_pkt_t* pkt);