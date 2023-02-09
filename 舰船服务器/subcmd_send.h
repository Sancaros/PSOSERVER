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

int bb_reg_sync_index(lobby_t* l, uint16_t regnum);

/* 假设所有参数都已按小端序排列. */
int subcmd_send_bb_drop_stack(ship_client_t* c, uint32_t area, float x,
    float z, iitem_t* item);

int subcmd_send_bb_pick_item(ship_client_t* c, uint32_t item_id, uint32_t area);

int subcmd_send_bb_create_item(ship_client_t* c, item_t item, int 发送给其他客户端);

int subcmd_send_bb_destroy_map_item(ship_client_t* c, uint16_t area,
    uint32_t item_id);

int subcmd_send_bb_destroy_item(ship_client_t* c, uint32_t item_id,
    uint8_t amt);

//从客户端移除美赛塔
int subcmd_send_bb_delete_meseta(ship_client_t* c, uint32_t count, uint32_t drop);

int subcmd_send_bb_gm_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req);

int subcmd_send_bb_quest_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req, ship_client_t* dest);

int subcmd_send_bb_lobby_item(lobby_t* l, subcmd_bb_itemreq_t* req,
    const iitem_t* item);

int subcmd_send_bb_enemy_item_req(lobby_t* l, subcmd_bb_itemreq_t* req,
    const iitem_t* item);

int subcmd_send_bb_exp(ship_client_t* c, uint32_t exp_amount);

int subcmd_send_bb_set_exp_rate(ship_client_t* c, uint32_t exp_rate);

int subcmd_send_bb_level(ship_client_t* c);

// 向玩家发送货物清单
int subcmd_bb_send_shop(ship_client_t* c, uint8_t shop_type, uint8_t num_items);

int subcmd_bb_60size_check(ship_client_t* c, subcmd_bb_pkt_t* pkt);

int subcmd_bb_626Dsize_check(ship_client_t* c, subcmd_bb_pkt_t* pkt);