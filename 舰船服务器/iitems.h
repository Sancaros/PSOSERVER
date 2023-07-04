/*
    �λ�֮���й� ����������
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

#ifndef IITEMS_H
#define IITEMS_H

#include <pso_character.h>

#include <pso_player.h>
#include <pso_items.h>
#include "clients.h"
#include "shop.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////��Ʒ����

/* ��ʼ����Ʒ���� */
void clear_item(item_t* item);
/* ������ƷID */
uint32_t generate_item_id(lobby_t* l, uint8_t client_id);

uint32_t primary_identifier(item_t* i);

/* �ѵ����� */
bool is_stackable(const item_t* item);
size_t stack_size(const item_t* item);
size_t max_stack_size(const item_t* item);
size_t max_stack_size_for_item(uint8_t data0, uint8_t data1);
//
//// TODO: Eliminate duplication between this function and the parallel function
//// in PlayerBank
//size_t stack_size_for_item(item_t item);

////////////////////////////////////////////////////////////////////////////////////////////////////
////��ұ�������

/* ��ʼ��������Ʒ���� */
void clear_iitem(iitem_t* iitem);

/* �޸���ұ������� */
void fix_up_pl_iitem(lobby_t* l, ship_client_t* c);

////////////////////////////////////////////////////////////////////////////////////////////////////
////���б�������

/* ��ʼ��������Ʒ���� */
void clear_bitem(bitem_t* bitem);


////////////////////////////////////////////////////////////////////////////////////////////////////
////��Ϸ�������

/* ����һ����Ʒ������������. �������ڵ������֮ǰ������д����Ļ�����.
��������Ŀ����û������Ʒ�Ŀռ�,�򷵻�NULL. */
iitem_t* lobby_add_new_item_locked(lobby_t* l, item_t* new_item);
iitem_t* lobby_add_item_locked(lobby_t* l, iitem_t* item);

int lobby_remove_item_locked(lobby_t* l, uint32_t item_id, iitem_t* rv);

/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
int find_iitem_slot(inventory_t* inv, uint32_t item_id);
size_t find_equipped_weapon(inventory_t* inv);
size_t find_equipped_armor(inventory_t* inv);
size_t find_equipped_mag(inventory_t* inv);

/* �Ƴ�������Ʒ���� */
int item_remove_from_inv(iitem_t *inv, int inv_count, uint32_t item_id,
                         uint32_t amt);
iitem_t remove_item(ship_client_t* src, uint32_t item_id, uint32_t amount, bool allow_meseta_overdraft);

/* ��ɫ�������й��� */
void cleanup_bb_bank(ship_client_t *c);
int item_deposit_to_bank(ship_client_t *c, bitem_t *it);
int item_take_from_bank(ship_client_t *c, uint32_t item_id, uint8_t amt,
                        bitem_t *rv);

/* ��Ʒ���װ����ǩ */
int item_check_equip(uint8_t װ����ǩ, uint8_t �ͻ���װ����ǩ);
int item_check_equip_flags(ship_client_t* c, uint32_t item_id);
/* ���ͻ��˱�ǿɴ���ְҵװ���ı�ǩ */
int item_class_tag_equip_flag(ship_client_t* c);

/* ���ӱ�����Ʒ */
int item_add_to_inv(ship_client_t* c, iitem_t* iitem);

/* ������Ʒ���ͻ��� */
int add_item_to_client(ship_client_t* c, iitem_t* iitem);

//�޸������������ݴ������Ʒ����
void fix_inv_bank_item(item_t* i);

//�޸�����װ�����ݴ������Ʒ����
void fix_equip_item(inventory_t* inv);

/* ��������Ʒ */
void clean_up_inv(inventory_t* inv);
void sort_client_inv(inventory_t* inv);
void clean_up_bank(psocn_bank_t* bank);

#endif /* !IITEMS_H */
