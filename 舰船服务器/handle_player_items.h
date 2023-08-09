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

/* ������ƷID */
size_t generate_item_id(lobby_t* l, size_t client_id);

/* �޸���ұ������� */
void regenerate_lobby_item_id(lobby_t* l, ship_client_t* c);

////////////////////////////////////////////////////////////////////////////////////////////////////
////��Ϸ�������

/* ����һ����Ʒ������������. �������ڵ������֮ǰ������д����Ļ�����.
��������Ŀ����û������Ʒ�Ŀռ�,�򷵻�NULL. */
iitem_t* add_new_litem_locked(lobby_t* l, item_t* new_item, uint8_t area, float x, float z);
iitem_t* add_litem_locked(lobby_t* l, iitem_t* item);

int remove_litem_locked(lobby_t* l, uint32_t item_id, iitem_t* rv);

/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
size_t find_iitem_index(inventory_t* inv, uint32_t item_id);
size_t find_bitem_index(psocn_bank_t* bank, uint32_t item_id);
size_t find_iitem_stack_item_id(inventory_t* inv, iitem_t* item);
size_t find_iitem_pid(inventory_t* inv, iitem_t* item);
size_t find_iitem_pid_index(inventory_t* inv, iitem_t* item);
size_t find_equipped_weapon(inventory_t* inv);
size_t find_equipped_armor(inventory_t* inv);
size_t find_equipped_mag(inventory_t* inv);

/* �Ƴ�������Ʒ���� */
int remove_iitem_v1(iitem_t *inv, int inv_count, uint32_t item_id, uint32_t amt);
iitem_t remove_iitem(ship_client_t* src, uint32_t item_id, uint32_t amount, bool allow_meseta_overdraft);
bitem_t remove_bitem(ship_client_t* src, uint32_t item_id, uint32_t amount);
bool add_iitem(ship_client_t* src, iitem_t* item);
bool add_bitem(ship_client_t* src, bitem_t* item);
int player_use_item(ship_client_t* src, size_t item_index);

/* ��սģʽר�� */
int initialize_cmode_iitem(ship_client_t* dest);

/* ��ɫ�������й��� */
void player_iitem_init(iitem_t* item, const bitem_t* src);
void player_bitem_init(bitem_t* item, const iitem_t* src);
void cleanup_bb_bank(ship_client_t *c, psocn_bank_t* bank, bool comoon_bank);

/* ��Ʒ���װ����ǩ */
int item_check_equip(uint8_t װ����ǩ, uint8_t �ͻ���װ����ǩ);
int item_check_equip_flags(uint32_t gc, uint32_t target_level, uint8_t equip_flags, inventory_t* inv, uint32_t item_id);
/* ���ͻ��˱�ǿɴ���ְҵװ���ı�ǩ */
int item_class_tag_equip_flag(ship_client_t* c);

//�޸������������ݴ������Ʒ����
void fix_inv_bank_item(item_t* i);

//�޸�����װ�����ݴ������Ʒ����
void fix_equip_item(inventory_t* inv);

/* ��������Ʒ */
void fix_client_inv(inventory_t* inv);
void sort_client_inv(inventory_t* inv);
void fix_client_bank(psocn_bank_t* bank);
void sort_client_bank(psocn_bank_t* bank);

#endif /* !IITEMS_H */
