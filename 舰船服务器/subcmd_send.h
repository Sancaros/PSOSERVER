/*
    �λ�֮���й� ���������� ��ָ���
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

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "clients.h"
#include "subcmd.h"
#include "ship_packets.h"

// subcmd ֱ�ӷ���ָ�����ͻ���
/* ���͸�ָ�����ݰ������� ignore_check �Ƿ���Ա����뱻���Ե���� */
int subcmd_send_lobby_bb(lobby_t* l, ship_client_t* src, subcmd_bb_pkt_t* pkt,
    int ignore_check);

/* 0x5D SUBCMD60_DROP_STACK BB ���˵���ѵ���Ʒ*/
int subcmd_send_drop_stack_bb(ship_client_t* src, uint16_t drop_src_id, litem_t* litem);

/* 0x5D SUBCMD60_DROP_STACK DC ���˵���ѵ���Ʒ*/
int subcmd_send_drop_stack_dc(ship_client_t* src,
    uint16_t drop_src_id, item_t item, uint8_t area, float x, float z);

/* 0x5D SUBCMD60_DROP_STACK BB ��������ѵ���Ʒ*/
int subcmd_send_lobby_drop_stack_bb(ship_client_t* src, uint16_t drop_src_id, ship_client_t* nosend, litem_t* litem);

/* 0x5D SUBCMD60_DROP_STACK DC ��������ѵ���Ʒ*/
int subcmd_send_lobby_drop_stack_dc(ship_client_t* src,
    uint16_t drop_src_id, ship_client_t* nosend, item_t item, uint8_t area, float x, float z);

/* 0x59 SUBCMD60_DEL_MAP_ITEM BB ʰȡ��Ʒ */
int subcmd_send_bb_del_map_item(ship_client_t* c, uint32_t area, uint32_t item_id);

/* 0xBE SUBCMD60_CREATE_ITEM BB ���˻����Ʒ */
int subcmd_send_bb_create_inv_item(ship_client_t* src, item_t item, uint32_t amount);

/* 0xBE SUBCMD60_CREATE_ITEM BB ���͸����������Ʒ ����SHOP���͵Ļ�ȡ */
int subcmd_send_lobby_bb_create_inv_item(ship_client_t* src, item_t item, uint32_t amount, bool send_to_src);

/* 0xB9 SUBCMD62_TEKKED_RESULT BB ���˻�ü�����Ʒ */
int subcmd_send_bb_create_tekk_item(ship_client_t* src, item_t item);

/* 0x29 SUBCMD60_DELETE_ITEM BB ������Ʒ */
int subcmd_send_lobby_bb_destroy_item(ship_client_t* c, uint32_t item_id, uint32_t amt);

/* BB �ӿͻ����Ƴ������� */
int subcmd_send_lobby_bb_delete_meseta(ship_client_t* c, psocn_bb_char_t* character, uint32_t amount, bool drop);

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB ���������Ʒ */
int subcmd_send_lobby_bb_gm_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req);

int subcmd_send_bb_quest_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req, ship_client_t* dest);

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB ���˹��������Ʒ */
int subcmd_send_bb_drop_item(ship_client_t* dest, subcmd_bb_itemreq_t* req, const item_t* item);

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB �������������Ʒ */
int subcmd_send_lobby_bb_drop_item(ship_client_t* src, ship_client_t* nosend, subcmd_bb_itemreq_t* req, const item_t* item);

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB ���������Ʒ */
int subcmd_send_lobby_bb_enemy_item_req(lobby_t* l, subcmd_bb_itemreq_t* req, const item_t* item);

/* 0xBC SUBCMD60_BANK_INV BB ������� */
int subcmd_send_bb_bank(ship_client_t* src, psocn_bank_t* bank);

/* 0xBF SUBCMD60_GIVE_EXP BB ��һ�þ��� */
int subcmd_send_lobby_bb_exp(ship_client_t* dest, uint32_t exp_amount);

/* 0xDD SUBCMD60_SET_EXP_RATE BB ������Ϸ���鱶�� */
int subcmd_send_bb_set_exp_rate(ship_client_t* c, uint32_t exp_rate);

/* 0xDB SUBCMD60_EXCHANGE_ITEM_IN_QUEST BB �����жһ���Ʒ */
int subcmd_send_bb_exchange_item_in_quest(ship_client_t* c, uint32_t item_id, uint32_t amount);

/* 0x30 SUBCMD60_LEVEL_UP BB ���������ֵ�仯 */
int subcmd_send_lobby_bb_level(ship_client_t* dest);

/* 0xB6 SUBCMD60_SHOP_INV BB ����ҷ��ͻ����嵥 */
int subcmd_bb_send_shop(ship_client_t* dest, uint8_t shop_type, uint8_t num_items, bool create);

/* 0xE3 SUBCMD62_COREN_ACT_RESULT BB ����ҷ��Ϳ��׶Ĳ���� */
int subcmd_bb_send_coren_reward(ship_client_t* dest, uint32_t flags, item_t item);

int subcmd_bb_60size_check(ship_client_t* c, subcmd_bb_pkt_t* pkt);

int subcmd_bb_626Dsize_check(ship_client_t* c, subcmd_bb_pkt_t* pkt);