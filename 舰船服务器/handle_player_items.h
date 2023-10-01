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

bool check_structs_equal(const void* s1, const void* s2, size_t sz);

/* ������ƷID */
size_t generate_item_id(lobby_t* l, size_t client_id);
size_t destroy_item_id(lobby_t* l, size_t client_id);

/* �޸���ұ������� */
void regenerate_lobby_item_id(lobby_t* l, ship_client_t* c);

////////////////////////////////////////////////////////////////////////////////////////////////////
////��Ϸ�������

/* ����һ����Ʒ������������. �������ڵ������֮ǰ������д����Ļ�����.
��������Ŀ����û������Ʒ�Ŀռ�,�򷵻�NULL. */
litem_t* add_new_litem_locked(lobby_t* l, item_t* new_item, uint8_t area, float x, float z);
litem_t* add_litem_locked(lobby_t* l, iitem_t* iitem, uint8_t area, float x, float z);

int remove_litem_locked(lobby_t* l, uint32_t item_id, iitem_t* rv);

/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
size_t find_litem_index(lobby_t* l, item_t* item);

/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
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

/* ������������� �ڴ���� */
int add_character_meseta(psocn_bb_char_t* character, uint32_t amount);
int remove_character_meseta(psocn_bb_char_t* character, uint32_t amount, bool allow_overdraft);

/* �Ƴ�������Ʒ���� */
int remove_iitem_v1(iitem_t *inv, int inv_count, uint32_t item_id, uint32_t amt);
iitem_t remove_iitem(ship_client_t* src, uint32_t item_id, uint32_t amount, bool allow_meseta_overdraft);
iitem_t remove_titem(trade_inv_t* trade, uint32_t item_id, uint32_t amount);
bitem_t remove_bitem(ship_client_t* src, uint32_t item_id, uint16_t bitem_index, uint32_t amount);
bool add_iitem(ship_client_t* src, const iitem_t iitem);
bool add_titem(trade_inv_t* trade, const iitem_t iitem);
bool add_bitem(ship_client_t* src, const bitem_t bitem);
int player_use_item(ship_client_t* src, uint32_t item_id);
int player_tekker_item(ship_client_t* src, sfmt_t* rng, item_t* item);

/* ��սģʽר�� */
int initialize_cmode_iitem(ship_client_t* dest);

/* ��ɫ������Ʒ���� */
iitem_t player_iitem_init(const item_t item);
trade_inv_t* player_tinv_init(ship_client_t* src);
bitem_t player_bitem_init(const item_t item);
void cleanup_bb_inv(uint32_t client_id, inventory_t* inv);
void regenerate_bank_item_id(uint32_t client_id, psocn_bank_t* bank, bool comoon_bank);

/* ��Ʒ���װ����ǩ */
bool item_check_equip(uint8_t װ����ǩ, uint8_t �ͻ���װ����ǩ);
int item_check_equip_flags(ship_client_t* src, uint32_t item_id);
/* ���ͻ��˱�ǿɴ���ְҵװ���ı�ǩ */
void item_class_tag_equip_flag(ship_client_t* c);
void remove_titem_equip_flags(iitem_t* trade_item);

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
