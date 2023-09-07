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

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <wincrypt.h>

#include <f_logs.h>
#include <pso_text.h>
#include <SFMT.h>

#include "handle_player_items.h"
#include "subcmd_send_bb.h"

/* We need LE32 down below... so get it from packets.h */
#define PACKETS_H_HEADERS_ONLY
#include "packets.h"
#include "pmtdata.h"
#include "ptdata.h"
#include "rtdata.h"
#include "mag_bb.h"
#include "max_tech_level.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////��Ϸ�������

/* ������ƷID */
size_t generate_item_id(lobby_t* l, size_t client_id) {
    size_t c_id = client_id, l_max_c_id = l->max_clients;

    if (c_id < l_max_c_id)
        return ++l->item_player_id[client_id];

    return ++l->item_lobby_id;
}

size_t destroy_item_id(lobby_t* l, size_t client_id) {
    size_t c_id = client_id, l_max_c_id = l->max_clients;

    if (c_id < l_max_c_id)
        return --l->item_player_id[client_id];

    return --l->item_lobby_id;
}

/* �޸���ұ������� */
void regenerate_lobby_item_id(lobby_t* l, ship_client_t* c) {
    uint32_t id;
    int i;

    inventory_t* inv = get_client_inv_bb(c);

    if (c->version == CLIENT_VERSION_BB) {
        /* ���·�����������ұ���ID */
        id = 0x00010000 | (c->client_id << 21) | (l->item_player_id[c->client_id]);

        if (c->mode) {
            c->mode_semi_item_id = id;
            ++id;
        }

        for (i = 0; i < inv->item_count; ++i, ++id) {
            inv->iitems[i].data.item_id = LE32(id);
        }

        --id;
        l->item_player_id[c->client_id] = id;
    }
    else {
        /* ���·�����������ұ���ID */
        id = 0x00010000 | (c->client_id << 21) | (l->item_player_id[c->client_id]);

        if (c->mode) {
            c->mode_semi_item_id = id;
            ++id;
        }

        for (i = 0; i < c->item_count; ++i, ++id) {
            c->iitems[i].data.item_id = LE32(id);
        }

        --id;
        l->item_player_id[c->client_id] = id;
    }
}

/* ����һ����Ʒ������������. �������ڵ������֮ǰ������д����Ļ�����.
��������Ŀ����û������Ʒ�Ŀռ�,�򷵻�NULL. */
iitem_t* add_new_litem_locked(lobby_t* l, const item_t* new_item, uint8_t area, float x, float z) {
    /* �����Լ��... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    lobby_item_t* litem = (lobby_item_t*)malloc(sizeof(lobby_item_t));

    if (!litem)
        return NULL;

    memset(litem, 0, sizeof(lobby_item_t));

    /* Copy the item data in. */
    litem->iitem.present = LE16(0x0001);
    litem->iitem.extension_data1 = 0;
    litem->iitem.extension_data2 = 0;
    litem->iitem.flags = LE32(0);

    memcpy(&litem->iitem.data, new_item, PSOCN_STLENGTH_IITEM);
    litem->iitem.data.item_id = LE32(l->item_lobby_id);

    litem->x = x;
    litem->z = z;
    litem->area = area;

#ifdef DEBUG

    print_iitem_data(&item->d, 0, l->version);

#endif // DEBUG

    /* Increment the item ID, add it to the queue, and return the new item */
    ++l->item_lobby_id;
    TAILQ_INSERT_HEAD(&l->item_queue, litem, qentry);
    return &litem->iitem;
}

/* ��Ҷ����� ȡ���� �������Ʒ */
iitem_t* add_litem_locked(lobby_t* l, const iitem_t* iitem, uint8_t area, float x, float z) {
    
    /* �����Լ��... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    lobby_item_t* litem = (lobby_item_t*)malloc(sizeof(lobby_item_t));

    if (!litem)
        return NULL;

    memset(litem, 0, sizeof(lobby_item_t));

    /* Copy the item data in. */
    memcpy(&litem->iitem, iitem, PSOCN_STLENGTH_IITEM);
    litem->x = x;
    litem->z = z;
    litem->area = area;

    /* Add it to the queue, and return the new item */
    TAILQ_INSERT_HEAD(&l->item_queue, litem, qentry);
    return &litem->iitem;
}

int remove_litem_locked(lobby_t* l, uint32_t item_id, iitem_t* rv) {

    if (l->version != CLIENT_VERSION_BB)
        return -1;

    clear_iitem(rv);

    //memset(rv, 0, PSOCN_STLENGTH_IITEM);
    //rv->data.datal[0] = LE32(Item_NoSuchItem);

    lobby_item_t* litem = TAILQ_FIRST(&l->item_queue);
    while (litem) {
        lobby_item_t* tmp = TAILQ_NEXT(litem, qentry);

        if (litem->iitem.data.item_id == item_id) {
            memcpy(rv, &litem->iitem, PSOCN_STLENGTH_IITEM);
            TAILQ_REMOVE(&l->item_queue, litem, qentry);
            free_safe(litem);
            return 0;
        }

        litem = tmp;
    }

    return 1;
}

int find_iitem_index(const inventory_t* inv, const uint32_t item_id) {
    int x = 0;

    if(inv->item_count > MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("������Ʒ������������ %d", inv->item_count);
        return -1;
    }

    for (x = 0; x < inv->item_count; x++) {
        if (inv->iitems[x].data.item_id != item_id)
            continue;

        return x;
    }

#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    if (x == inv->item_count) {
        ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", item_id);
        return -2;
    }

    return -3;
}

int find_bitem_index(const psocn_bank_t* bank, const uint32_t item_id) {
    int x = -1;

    if (bank->item_count > MAX_PLAYER_BANK_ITEMS) {
        ERR_LOG("������Ʒ������������ %d", bank->item_count);
        return x;
    }

    for (int y = 0; y < (int)bank->item_count; y++) {
        if (bank->bitems[y].data.item_id != item_id)
            continue;

        x = y;
        break;
    }

#ifdef DEBUG

    for (x = 0; x < bank->item_count; x++) {
        print_bitem_data(&bank->bitems[x], x, 5);
    }

#endif // DEBUG

    if (x == bank->item_count) {
        ERR_LOG("δ���������ҵ�ID 0x%08X ��Ʒ", item_id);
        return -1;
    }

    return -2;
}

/* ������PD PC �ȶ����ѵ���Ʒ �������ڵ�����Ʒ */
size_t find_iitem_stack_item_id(const inventory_t* inv, const iitem_t* iitem) {
    uint32_t pid = primary_identifier(&iitem->data);
    size_t x = 0;

    if (inv->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("������Ʒ������������ %d", inv->item_count);
        return 0;
    }

    for (x = 0; x < inv->item_count; x++) {
        if (primary_identifier(&inv->iitems[x].data) != pid)
            continue;

        return inv->iitems[x].data.item_id;
    }

#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    if (x == inv->item_count) {
        ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", pid);
        return 0;
    }

    return 0;
}

/* ������PD PC �ȶ����ѵ���Ʒ �������ڵ�����Ʒ */
size_t find_iitem_code_stack_item_id(const inventory_t* inv, const uint32_t code) {
    uint32_t pid = primary_code_identifier(code);
    size_t x = 0;

    if (inv->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("������Ʒ������������ %d", inv->item_count);
        return 0;
    }

    for (x = 0; x < inv->item_count; x++) {
        if (primary_identifier(&inv->iitems[x].data) != pid)
            continue;

        return inv->iitems[x].data.item_id;
    }

#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    if (x == inv->item_count) {
        ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", code);
        return 0;
    }

    return 0;
}

size_t find_iitem_pid(const inventory_t* inv, const iitem_t* iitem) {
    uint32_t pid = primary_identifier(&iitem->data);
    int x = 0;

    if (inv->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("������Ʒ������������ %d", inv->item_count);
        return -1;
    }

    for (x = 0; x < inv->item_count; x++) {
        if (primary_identifier(&inv->iitems[x].data) != pid)
            continue;

        return primary_identifier(&inv->iitems[x].data);
    }

#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    if (x == inv->item_count) {
        ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", pid);
        return -1;
    }

    return -2;
}

int find_iitem_pid_index(const inventory_t* inv, const iitem_t* iitem) {
    uint32_t pid = primary_identifier(&iitem->data);
    int x = 0;

    if (inv->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("������Ʒ������������ %d", inv->item_count);
        return -1;
    }

    for (x = 0; x < inv->item_count; x++) {
        if (primary_identifier(&inv->iitems[x].data) != pid)
            continue;

        return x;
    }

#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    if (x == inv->item_count) {
        ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", pid);
        return -1;
    }

    return -2;
}

int find_bitem_pid_index(const psocn_bank_t* bank, const bitem_t* bitem) {
    uint32_t pid = primary_identifier(&bitem->data);
    int x = 0;

    if (bank->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("������Ʒ������������ %d", bank->item_count);
        return -1;
    }

    for (x = 0; x < (int)bank->item_count; x++) {
        if (primary_identifier(&bank->bitems[x].data) != pid)
            continue;

        return x;
    }

#ifdef DEBUG

    for (x = 0; x < bank->item_count; x++) {
        print_bitem_data(&bank->bitems[x], x, 5);
    }

#endif // DEBUG

    if (x == bank->item_count) {
        ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", pid);
        return -1;
    }

    return -2;
}

/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
int find_equipped_weapon(const inventory_t* inv){
    int ret = -1;
    for (int y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & 0x00000008)) {
            continue;
        }
        if (inv->iitems[y].data.datab[0] != ITEM_TYPE_WEAPON) {
            continue;
        }
        if (ret < 0) {
            ret = y;
        }
        else {
            ERR_LOG("�ҵ�װ���������");
        }
    }
    if (ret < 0) {
        ERR_LOG("δ�ӱ���������װ��������");
    }
    return ret;
}

/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
int find_equipped_armor(const inventory_t* inv) {
    int ret = -1;
    for (int y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & 0x00000008)) {
            continue;
        }
        if (inv->iitems[y].data.datab[0] != ITEM_TYPE_GUARD || 
            inv->iitems[y].data.datab[1] != ITEM_SUBTYPE_FRAME) {
            continue;
        }
        if (ret < 0) {
            ret = y;
        }
        else {
            ERR_LOG("�ҵ�װ���������");
        }
    }
    if (ret < 0) {
        ERR_LOG("δ�ӱ���������װ���Ŀ���");
    }
    return ret;
}

/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
int find_equipped_mag(const inventory_t* inv) {
    int ret = -1;
    for (int y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & 0x00000008)) {
            continue;
        }
        if (inv->iitems[y].data.datab[0] != ITEM_TYPE_MAG) {
            continue;
        }
        if (ret < 0) {
            ret = y;
        }
        else {
            ERR_LOG("�ҵ�װ��������");
            ret = -2;
        }
    }
#ifdef DEBUG

    DBG_LOG("ret %u", ret);
#endif // DEBUG

    return ret;
}

void bswap_data2_if_mag(item_t* item) {
    if (item->datab[0] == ITEM_TYPE_MAG) {
        item->data2l = bswap32(item->data2l);
    }
}

int add_character_meseta(psocn_bb_char_t* character, uint32_t amount) {
    uint32_t max_meseta = 999999;
#ifdef DEBUG

    if (character->disp.meseta + amount > max_meseta) {
        ERR_LOG("�������������");
        return -1;
    }

#endif // DEBUG

    character->disp.meseta = MIN(character->disp.meseta + amount, max_meseta);

    return 0;
}

int remove_character_meseta(psocn_bb_char_t* character, uint32_t amount, bool allow_overdraft) {
    if (amount <= character->disp.meseta) {
        character->disp.meseta -= amount;
    }
    else if (allow_overdraft) {
        character->disp.meseta = 0;
    }
    else {
        return -1;
    }

    return 0;
}

/* �Ƴ�������Ʒ���� */
int remove_iitem_v1(iitem_t *inv, int inv_count, uint32_t item_id,
                         uint32_t amt) {
    int i;
    uint32_t tmp;

    /* Look for the item in question */
    for(i = 0; i < inv_count; ++i) {
        if(inv[i].data.item_id == item_id) {
            break;
        }
    }

    /* Did we find it? If not, return error. */
    if(i == inv_count) {
        return -1;
    }

    /* Check if the item is stackable, since we may have to do some stuff
       differently... */
    if(is_stackable(&inv[i].data) && amt != EMPTY_STRING) {
        tmp = inv[i].data.datab[5];

        if(amt < tmp) {
            tmp -= amt;
            inv[i].data.datab[5] = tmp;
            return 0;
        }
    }

    /* Move the rest of the items down to take over the place that the item in
       question used to occupy. */
    memmove(inv + i, inv + i + 1, (inv_count - i - 1) * PSOCN_STLENGTH_IITEM);
    return 1;
}

iitem_t remove_iitem(ship_client_t* src, uint32_t item_id, uint32_t amount, 
    bool allow_meseta_overdraft) {
    iitem_t ret = { 0 };
    psocn_bb_char_t* character = get_client_char_bb(src);

    if (item_id == ITEM_ID_MESETA) {
        if (remove_character_meseta(character, amount, allow_meseta_overdraft)) {
            ERR_LOG("���ӵ�е����������� %d < %d", character->disp.meseta, amount);
            return ret;
        }
        ret.data.datab[0] = ITEM_TYPE_MESETA;
        ret.data.data2l = amount;
        return ret;
    }

    int index = find_iitem_index(&character->inv, item_id);
    if (index < 0) {
        ERR_LOG("�Ƴ���Ʒ�������� ������ %d", index);
        return ret;
    }
    iitem_t* inventory_item = &character->inv.iitems[index];

    if (amount && is_stackable(&inventory_item->data) && (amount < inventory_item->data.datab[5])) {
        ret = *inventory_item;
        ret.data.datab[5] = amount;
        ret.data.item_id = EMPTY_STRING;
        inventory_item->data.datab[5] -= amount;
        return ret;
    }

    // If we get here, then it's not meseta, and either it's not a combine item or
    // we're removing the entire stack. Delete the item from the inventory slot
    // and return the deleted item.
    memcpy(&ret, inventory_item, sizeof(iitem_t));
    //ret = inventory_item;
    character->inv.item_count--;
    for (int x = index; x < character->inv.item_count; x++) {
        character->inv.iitems[x] = character->inv.iitems[x + 1];
    }
    clear_iitem(&character->inv.iitems[character->inv.item_count]);
    fix_client_inv(&character->inv);
    return ret;
}

bitem_t remove_bitem(ship_client_t* src, uint32_t item_id, uint16_t bitem_index, uint32_t amount) {
    bitem_t ret = { 0 };
    psocn_bank_t* bank = get_client_bank_bb(src);
    
    // ����Ƿ񳬳�����������
    if (item_id == ITEM_ID_MESETA) {
        if (amount > bank->meseta) {
            ERR_LOG("GC %" PRIu32 " �Ƴ���������������ӵ�е�",
                src->guildcard);
            return ret;
        }
        ret.data.datab[0] = ITEM_TYPE_MESETA;
        ret.data.data2l = amount;
        bank->meseta -= amount;
        return ret;
    }

    //// ����������Ʒ������
    //size_t index = find_bitem_index(bank, item_id);

    if (bitem_index == bank->item_count) {
        ERR_LOG("GC %" PRIu32 " ������Ʒ������������",
            src->guildcard);
        return ret;
    }

    bitem_t* bank_item = &bank->bitems[bitem_index];

    // ����Ƿ񳬳���Ʒ�ɶѵ�������
    if (amount && is_stackable(&bank_item->data) &&
        (amount < bank_item->data.datab[5])) {
        ret = *bank_item;
        ret.data.datab[5] = amount;
        ret.amount = amount;
        bank_item->data.datab[5] -= amount;
        bank_item->amount -= amount;
        return ret;
    }

    //ret = *bank_item;

    memcpy(&ret, bank_item, sizeof(bitem_t));
    // �Ƴ�������Ʒ
    bank->item_count--;
    for (size_t x = bitem_index; x < bank->item_count; x++) {
        bank->bitems[x] = bank->bitems[x + 1];
    }
    clear_bitem(&bank->bitems[bank->item_count]);
    return ret;
}

bool add_iitem(ship_client_t* src, const iitem_t* iitem) {
    uint32_t pid = primary_identifier(&iitem->data);
    psocn_bb_char_t* character = get_client_char_bb(src);

    // ����Ƿ�Ϊmeseta������ǣ����޸�ͳ�������е�mesetaֵ
    if (pid == MESETA_IDENTIFIER) {
        add_character_meseta(character, iitem->data.data2l);
        return true;
    }

    // ����ɺϲ�����Ʒ
    size_t combine_max = max_stack_size(&iitem->data);
    if (combine_max > 1) {
        // �����ҿ�����Ѿ�������ͬ��Ʒ�Ķѵ�����ȡ����Ʒ������
        size_t y;
        for (y = 0; y < character->inv.item_count; y++) {
            if (primary_identifier(&character->inv.iitems[y].data) == pid) {
                break;
            }
        }

        // ������ڶѵ�����������ӣ����������ѵ�����
        if (y < character->inv.item_count) {
            character->inv.iitems[y].data.datab[5] += iitem->data.datab[5];
            if (character->inv.iitems[y].data.datab[5] > (uint8_t)combine_max) {
                character->inv.iitems[y].data.datab[5] = (uint8_t)combine_max;
            }
            return true;
        }
    }

    // ���ִ�е�����Ȳ���mesetaҲ���ǿɺϲ���Ʒ�������Ҫ����һ���յĿ���λ
    if (character->inv.item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("GC %" PRIu32 " ������Ʒ�����������ֵ,��ǰ %d ����Ʒ",
            src->guildcard, character->inv.item_count);
        return false;
    }
    character->inv.iitems[character->inv.item_count] = *iitem;
    character->inv.item_count++;
    return true;
}

bool add_bitem(ship_client_t* src, const bitem_t* bitem) {
    uint32_t pid = primary_identifier(&bitem->data);
    psocn_bank_t* bank = get_client_bank_bb(src);
    
    if (pid == MESETA_IDENTIFIER) {
        bank->meseta += bitem->data.data2l;
        if (bank->meseta > 999999) {
            bank->meseta = 999999;
        }
        return true;
    }
    
    size_t combine_max = max_stack_size(&bitem->data);
    if (combine_max > 1) {
        size_t y;
        for (y = 0; y < bank->item_count; y++) {
            if (primary_identifier(&bank->bitems[y].data) == pid) {
                break;
            }
        }

        if (y < bank->item_count) {
            bank->bitems[y].data.datab[5] += bitem->data.datab[5];
            if (bank->bitems[y].data.datab[5] > (uint8_t)combine_max) {
                bank->bitems[y].data.datab[5] = (uint8_t)combine_max;
            }
            bank->bitems[y].amount = bank->bitems[y].data.datab[5];
            return true;
        }
    }

    if (bank->item_count >= MAX_PLAYER_BANK_ITEMS) {
        ERR_LOG("GC %" PRIu32 " ������Ʒ�����������ֵ",
            src->guildcard);
        return false;
    }
    bank->bitems[bank->item_count] = *bitem;
    bank->item_count++;
    return true;
}

bool is_wrapped(const item_t* item) {
    switch (item->datab[0]) {
    case ITEM_TYPE_WEAPON:
    case ITEM_TYPE_GUARD:
        return item->datab[4] & 0x40;
    case ITEM_TYPE_MAG:
        return item->data2b[2] & 0x40;
    case ITEM_TYPE_TOOL:
        return !is_stackable(item) && (item->datab[3] & 0x40);
    case ITEM_TYPE_MESETA:
        return false;
    }

    ERR_LOG("��Ч��Ʒ���� 0x%02X", item->datab[0]);
    return false;
}

void unwrap(item_t* item) {
    switch (item->datab[0]) {
    case ITEM_TYPE_WEAPON:
    case ITEM_TYPE_GUARD:
        item->datab[4] &= 0xBF;
        break;
    case ITEM_TYPE_MAG:
        item->data2b[2] &= 0xBF;
        break;
    case ITEM_TYPE_TOOL:
        if (!is_stackable(item)) {
            item->datab[3] &= 0xBF;
        }
        break;
    case 4:
        break;
    default:
        ERR_LOG("��Ч unwrap ��Ʒ����");
    }
}

int player_use_item(ship_client_t* src, uint32_t item_id) {
    sfmt_t* rng = &src->cur_block->sfmt_rng;
    iitem_t* weapon = { 0 };
    iitem_t* armor = { 0 };
    iitem_t* mag = { 0 };
    // On PC (and presumably DC), the client sends a 6x29 after this to delete the
    // used item. On GC and later versions, this does not happen, so we should
    // delete the item here.
    bool should_delete_item = (src->version != CLIENT_VERSION_DCV2) && (src->version != CLIENT_VERSION_PC);
    errno_t err = 0;

    psocn_bb_char_t* character = get_client_char_bb(src);

    int index = find_iitem_index(&character->inv, item_id);
    if (index < 0) {
        err = index;
        ERR_LOG("ʹ����Ʒ�������� ������ %d", index);
        return err;
    }
    iitem_t* iitem = &character->inv.iitems[index];

#ifdef DEBUG

    DBG_LOG("player_use_item");
    print_item_data(&iitem->data, src->version);

#endif // DEBUG

    if (is_common_consumable(primary_identifier(&iitem->data))) { // Monomate, etc.
        // Nothing to do (it should be deleted)
        goto done;
    }
    else if (is_wrapped(&iitem->data)) {
        // Unwrap present
        unwrap(&iitem->data);
        should_delete_item = false;
        goto done;
    }

    switch (iitem->data.datab[0])
    {
    case ITEM_TYPE_WEAPON:
        switch (iitem->data.datab[1]) {
        case 0x33:
            // Unseal Sealed J-Sword => Tsumikiri J-Sword
            iitem->data.datab[1] = 0x32;
            should_delete_item = false;
            break;

        case 0xAB:
            // Unseal Lame d'Argent => Excalibur
            iitem->data.datab[1] = 0xAC;
            should_delete_item = false;
            break;

        default:
            goto combintion_other;
        }
        break;

    case ITEM_TYPE_GUARD:
        switch (iitem->data.datab[1]) {
        case ITEM_SUBTYPE_UNIT:
            switch (iitem->data.datab[2]) {
            case 0x4D:
                // Unseal Limiter => Adept
                iitem->data.datab[2] = 0x4E;
                should_delete_item = false;
                break;

            case 0x4F:
                // Unseal Swordsman Lore => Proof of Sword-Saint
                iitem->data.datab[2] = 0x50;
                should_delete_item = false;
                break;

            default:
                goto combintion_other;
            }
            break;

        default:
            goto combintion_other;
        }
        break;

    case ITEM_TYPE_MAG:
        switch (iitem->data.datab[1]) {
        case 0x2B:
            weapon = &character->inv.iitems[find_equipped_weapon(&character->inv)];
            // Chao Mag used
            if ((weapon->data.datab[1] == 0x68) &&
                (weapon->data.datab[2] == 0x00)) {
                weapon->data.datab[1] = 0x58; // Striker of Chao
                weapon->data.datab[2] = 0x00;
                weapon->data.datab[3] = 0x00;
                weapon->data.datab[4] = 0x00;
            }
            break;

        case 0x2C:
            armor = &character->inv.iitems[find_equipped_armor(&character->inv)];
            // Chu Chu mag used
            if ((armor->data.datab[2] == 0x1C)) {
                armor->data.datab[2] = 0x2C; // Chuchu Fever
            }
            break;

        default:
            goto combintion_other;
        }
        break;

    case ITEM_TYPE_TOOL:
        switch (iitem->data.datab[1]) {
        case ITEM_SUBTYPE_DISK: // Technique disk
            uint8_t max_level = max_tech_level[iitem->data.datab[4]].max_lvl[character->dress_data.ch_class];
            if (iitem->data.datab[2] > max_level) {
                ERR_LOG("�����Ƽ�����ȼ�����ְҵ���õȼ�");
                return -1;
            }
            character->tech.all[iitem->data.datab[4]] = iitem->data.datab[2];
            break;

        case ITEM_SUBTYPE_GRINDER: // Grinder
            if (iitem->data.datab[2] > 2) {
                ERR_LOG("��Ч��ĥ��Ʒֵ");
                return -2;
            }
            weapon = &character->inv.iitems[find_equipped_weapon(&character->inv)];
            pmt_weapon_bb_t weapon_def = { 0 };
            if (pmt_lookup_weapon_bb(weapon->data.datal[0], &weapon_def)) {
                ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                    src->guildcard);
                return -3;
            }
            if (weapon->data.datab[3] >= weapon_def.max_grind) {
                ERR_LOG("�����Ѵ�����ĥֵ");
                return -4;
            }
            weapon->data.datab[3] += (iitem->data.datab[2] + 1);
            break;

        case ITEM_SUBTYPE_MATERIAL:
            switch (iitem->data.datab[2]) {
            case 0x00: // Power Material
                character->disp.stats.atp += 2;
                break;

            case 0x01: // Mind Material
                character->disp.stats.mst += 2;
                break;

            case 0x02: // Evade Material
                character->disp.stats.evp += 2;
                break;

            case 0x03: // HP Material
                character->inv.hpmats_used += 2;
                break;

            case 0x04: // TP Material
                character->inv.tpmats_used += 2;
                break;

            case 0x05: // Def Material
                character->disp.stats.dfp += 2;
                break;

            case 0x06: // Luck Material
                character->disp.stats.lck += 2;
                break;

            default:
                ERR_LOG("δ֪ҩ�� 0x%08X", iitem->data.datal[0]);
                return -5;
            }
            break;

        case ITEM_SUBTYPE_MAG_CELL1:
            int mag_index = find_equipped_mag(&character->inv);
            if (mag_index == -1) {
                ERR_LOG("���û��װ�����,���ϸ�� 0x%08X", iitem->data.datal[0]);
                break;
            }
            mag = &character->inv.iitems[mag_index];

            switch (iitem->data.datab[2]) {
            case 0x00:
                // Cell of MAG 502
                mag->data.datab[1] = (character->dress_data.section & SID_Greennill) ? 0x1D : 0x21;
                break;

            case 0x01:
                // Cell of MAG 213
                mag->data.datab[1] = (character->dress_data.section & SID_Greennill) ? 0x27 : 0x22;
                break;

            case 0x02:
                // Parts of RoboChao
                mag->data.datab[1] = 0x28;
                break;

            case 0x03:
                // Heart of Opa Opa
                mag->data.datab[1] = 0x29;
                break;

            case 0x04:
                // Heart of Pian
                mag->data.datab[1] = 0x2A;
                break;

            case 0x05:
                // Heart of Chao
                mag->data.datab[1] = 0x2B;
                break;

            default:
                ERR_LOG("δ֪���ϸ�� 0x%08X", iitem->data.datal[0]);
                return -5;
            }
            break;

        case ITEM_SUBTYPE_ADD_SLOT:
            armor = &character->inv.iitems[find_equipped_armor(&character->inv)];

            if (armor->data.datab[5] >= 4) {
                ERR_LOG("��Ʒ�Ѵ����������");
                return -6;
            }
            armor->data.datab[5]++;
            break;

        case ITEM_SUBTYPE_SERVER_ITEM1:
            size_t eq_wep = find_equipped_weapon(&character->inv);
            weapon = &character->inv.iitems[eq_wep];

            //�����ƥ�ID��5, 6����Ŀ��1, 2�Υ����ƥ�Έ��ϣ�0x0312 * *�ΤȤ���
            switch (iitem->data.datab[2]) {//�����å��ġ���ʹ�á����륢���ƥ�ID��7, 8����Ŀ�򥹥��å���ʹ����
            case 0x00: // weapons bronze
                if (eq_wep != -1) {
                    uint32_t ai, plustype, num_attribs = 0;
                    plustype = 1;
                    ai = 0;
                    if ((weapon->data.datab[6] > 0x00) &&
                        (!(weapon->data.datab[6] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[6] == plustype)
                            ai = 7;
                    }

                    if ((weapon->data.datab[8] > 0x00) &&
                        (!(weapon->data.datab[8] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[8] == plustype)
                            ai = 9;
                    }

                    if ((weapon->data.datab[10] > 0x00) &&
                        (!(weapon->data.datab[10] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[10] == plustype)
                            ai = 11;
                    }

                    if (ai) {
                        // Attribute already on weapon, increase it
                        weapon->data.datab[ai] += 0x0A;
                        if (weapon->data.datab[ai] > 100)
                            weapon->data.datab[ai] = 100;
                    }
                    else {
                        // Attribute not on weapon, add it if there isn't already 3 attributes
                        if (num_attribs < 3) {
                            weapon->data.datab[6 + (num_attribs * 2)] = 0x01;
                            weapon->data.datab[7 + (num_attribs * 2)] = 0x0A;
                        }
                    }
                }
                break;

            case 0x01: // weapons silver
                if (eq_wep != -1) {
                    uint32_t ai, plustype, num_attribs = 0;
                    plustype = 2;
                    ai = 0;
                    if ((weapon->data.datab[6] > 0x00) &&
                        (!(weapon->data.datab[6] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[6] == plustype)
                            ai = 7;
                    }

                    if ((weapon->data.datab[8] > 0x00) &&
                        (!(weapon->data.datab[8] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[8] == plustype)
                            ai = 9;
                    }

                    if ((weapon->data.datab[10] > 0x00) &&
                        (!(weapon->data.datab[10] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[10] == plustype)
                            ai = 11;
                    }

                    if (ai) {
                        // Attribute already on weapon, increase it
                        weapon->data.datab[ai] += 0x0A;
                        if (weapon->data.datab[ai] > 100)
                            weapon->data.datab[ai] = 100;
                    }
                    else {
                        // Attribute not on weapon, add it if there isn't already 3 attributes
                        if (num_attribs < 3) {
                            weapon->data.datab[6 + (num_attribs * 2)] = 0x02;
                            weapon->data.datab[7 + (num_attribs * 2)] = 0x0A;
                        }
                    }
                }
                break;

            case 0x02: // weapons gold
                if (eq_wep != -1) {
                    uint32_t ai, plustype, num_attribs = 0;
                    plustype = 3;
                    ai = 0;
                    if ((weapon->data.datab[6] > 0x00) &&
                        (!(weapon->data.datab[6] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[6] == plustype)
                            ai = 7;
                    }

                    if ((weapon->data.datab[8] > 0x00) &&
                        (!(weapon->data.datab[8] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[8] == plustype)
                            ai = 9;
                    }

                    if ((weapon->data.datab[10] > 0x00) &&
                        (!(weapon->data.datab[10] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[10] == plustype)
                            ai = 11;
                    }

                    if (ai) {
                        // Attribute already on weapon, increase it
                        weapon->data.datab[ai] += 0x0A;
                        if (weapon->data.datab[ai] > 100)
                            weapon->data.datab[ai] = 100;
                    }
                    else {
                        // Attribute not on weapon, add it if there isn't already 3 attributes
                        if (num_attribs < 3) {
                            weapon->data.datab[6 + (num_attribs * 2)] = 0x03;
                            weapon->data.datab[7 + (num_attribs * 2)] = 0x0A;
                        }
                    }
                }
                break;

            case 0x03: // weapons cristal
                if (eq_wep != -1) {
                    uint32_t ai, plustype, num_attribs = 0;
                    char attrib_add = 10;
                    plustype = 4;
                    ai = 0;
                    if ((weapon->data.datab[6] > 0x00) &&
                        (!(weapon->data.datab[6] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[6] == plustype)
                            ai = 7;
                    }

                    if ((weapon->data.datab[8] > 0x00) &&
                        (!(weapon->data.datab[8] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[8] == plustype)
                            ai = 9;
                    }

                    if ((weapon->data.datab[10] > 0x00) &&
                        (!(weapon->data.datab[10] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[10] == plustype)
                            ai = 11;
                    }

                    if (ai) {
                        // Attribute already on weapon, increase it
                        weapon->data.datab[ai] += 0x0A;
                        if (weapon->data.datab[ai] > 100)
                            weapon->data.datab[ai] = 100;
                    }
                    else {
                        // Attribute not on weapon, add it if there isn't already 3 attributes
                        if (num_attribs < 3) {
                            weapon->data.datab[6 + (num_attribs * 2)] = 0x04;
                            weapon->data.datab[7 + (num_attribs * 2)] = 0x0A;
                        }
                    }
                }
                break;

            case 0x04: // weapons steal
                if (eq_wep != -1) {
                    uint32_t ai, plustype, num_attribs = 0;
                    plustype = 5;
                    ai = 0;
                    if ((weapon->data.datab[6] > 0x00) &&
                        (!(weapon->data.datab[6] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[6] == plustype)
                            ai = 7;
                    }

                    if ((weapon->data.datab[8] > 0x00) &&
                        (!(weapon->data.datab[8] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[8] == plustype)
                            ai = 9;
                    }

                    if ((weapon->data.datab[10] > 0x00) &&
                        (!(weapon->data.datab[10] & 128))) {
                        num_attribs++;
                        if (weapon->data.datab[10] == plustype)
                            ai = 11;
                    }

                    if (ai) {
                        // Attribute already on weapon, increase it
                        weapon->data.datab[ai] += 0x05;
                        if (weapon->data.datab[ai] > 100)
                            weapon->data.datab[ai] = 100;
                    }
                    else {
                        // Attribute not on weapon, add it if there isn't already 3 attributes
                        if (num_attribs < 3) {
                            weapon->data.datab[6 + (num_attribs * 2)] = 0x05;
                            weapon->data.datab[7 + (num_attribs * 2)] = 0x05;
                        }
                    }
                }
                break;

            case 0x05:
                DBG_LOG("Aluminum Weapons Badge");
                break;

            case 0x06:
                DBG_LOG("Leather Weapons Badge");
                break;

            case 0x07: // weapons bone
                //�����ƥ�ID��7,8����Ŀ��0,7�ΤȤ�����0x031207�ΤȤ����Ĥޤꥦ���ݥ󥺥Хå��ǤΤȤ���
                if (eq_wep != -1) {    //Ԕ������������װ��r��
                    switch (weapon->data.datab[1]) { //�����å��˥���饯���`��װ�䤷�Ƥ��������Υ����ƥ�ID��3,4����Ŀ�򥹥��å���ʹ��

                    case 0x22:
                        //����饯���`��װ�䤷�Ƥ��������Υ����ƥ�ID��3,4����Ŀ��2,2�ΤȤ���0x002200�ΤȤ����Ĥޤꥫ����`���`����
                        if (weapon->data.datab[3] == 0x09)
                            //�⤷����饯���`��װ�䤷�Ƥ��������Υ����ƥ�ID��7,8����Ŀ��0,9���ä��飨�Ĥޤꏊ������ + 9���ä��飩
                        {
                            weapon->data.datab[2] = 0x01; // melcrius
                            //����饯���`��װ�䤷�Ƥ��������Υ����ƥ�ID��5,6����Ŀ��0,1�ˉ������
                            //���Ĥޤꥫ����`���`�� + 9��0x00220009������륯�ꥦ����å� + 9��0x00220109���ˤ��룩
                            weapon->data.datab[3] = 0x00; // Not grinded
                            //�������򣰤ˤ��루�Ĥޤ��륯�ꥦ����å� + 9��0x00220109������륯�ꥦ����åɣ�0x00220100���ˤ��룩
                            weapon->data.datab[4] = 0x00;
                            //Send_Item(client->character.inventory[eq_wep].data.item_id, client);
                            // Ԕ��������װ�䤷�Ƥ��������򥤥�٥�ȥ����K�Ф��ͤ룿
                        }
                        break;
                    }
                }
                break;

            default:
                goto combintion_other;
            }
            break;

        case ITEM_SUBTYPE_SERVER_ITEM2:
            item_t new_item = { 0 };
            iitem_t* new_iitem;

            switch (iitem->data.datab[2]) {
            case 0x03: // WeaponsSilverBadge 031403
                //Add Exp
                if (character->disp.level < 200) {
                    client_give_exp(src, 100000);
                }
                break;

            case 0x04: // WeaponseGold���륯�ߥ�M150
                pthread_mutex_lock(&src->mutex);

                new_item.datal[0] = 0xA3963C02;
                new_item.datal[1] = 0;
                new_item.datal[2] = 0x3A980000;
                new_item.data2l = 0x0407C878;

                new_iitem = add_new_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z);

                subcmd_send_lobby_drop_stack(src, 0xFBFF, NULL, src->cur_area, src->x, src->z, new_iitem->data, 1);

                pthread_mutex_unlock(&src->mutex);
                break;

            case 0x05: // WeaponseCrystal��DBȫ���å�
                pthread_mutex_lock(&src->mutex);

                //DB�z ����å�4
                new_item.datal[0] = 0x00280101;
                new_item.datal[1] = 0x00000400;
                new_item.datal[2] = 0x00000000;
                new_item.data2l = 0x00000000;

                new_iitem = add_new_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z);

                subcmd_send_lobby_drop_stack(src, 0xFBFF, NULL, src->cur_area, src->x, src->z, new_iitem->data, 1);

                //DB�� Machine 100% Ruin 100% Hit 50%
                new_item.datal[0] = 0x2C050100;
                new_item.datal[1] = 0x32050000;
                new_item.datal[2] = 0x64046403;
                new_item.data2l = 0x00000000;

                new_iitem = add_new_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z);

                subcmd_send_lobby_drop_stack(src, 0xFBFF, NULL, src->cur_area, src->x, src->z, new_iitem->data, 1);

                //DB��
                new_item.datal[0] = 0x00260201;
                new_item.datal[1] = 0x00000000;
                new_item.datal[2] = 0x00000000;
                new_item.data2l = 0x00000000;

                new_iitem = add_new_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z);

                subcmd_send_lobby_drop_stack(src, 0xFBFF, NULL, src->cur_area, src->x, src->z, new_iitem->data, 1);

                pthread_mutex_unlock(&src->mutex);
                break;

            default:
                goto combintion_other;
            }
            break;

        case ITEM_SUBTYPE_PRESENT_EVENT:
            // Present, etc. - use unwrap_table + probabilities therein
            size_t sum = 0, z = 0;

            for (z = 0; z < num_eventitems_bb[iitem->data.datab[2]]; z++) {
                sum += eventitem_bb[iitem->data.datab[2]][z].probability;
            }

            if (sum == 0) {
                ERR_LOG("�����¼�û�п��õ�������");
                return 0;
            }

            //size_t det = mt19937_genrand_int32(rng) % sum;
            size_t det = sfmt_genrand_uint32(rng) % sum;

            for (z = 0; z < num_eventitems_bb[iitem->data.datab[2]]; z++) {
                pmt_eventitem_bb_t entry = eventitem_bb[iitem->data.datab[2]][z];
                if (det > entry.probability) {
                    det -= entry.probability;
                }
                else {
                    iitem->data.datab[0] = entry.item[0];
                    iitem->data.datab[1] = entry.item[1];
                    iitem->data.datab[2] = entry.item[2];
                    iitem->data.datab[3] = 0;
                    iitem->data.datab[4] = 0;
                    iitem->data.datab[5] = 0;
                    iitem->data.data2l = 0;
                    should_delete_item = false;

                    subcmd_send_lobby_bb_create_inv_item(src, iitem->data, stack_size(&iitem->data), true);
                    break;
                }
            }

            /* TODO ���ֵ����� װ�� ����������� */
            switch (iitem->data.datab[2]) {
            case 0x00:
                DBG_LOG("ʹ��ʥ������");
                break;

            case 0x01:
                DBG_LOG("ʹ�ø��������");
                break;

            case 0x02:
                DBG_LOG("ʹ����ʥ������");
                break;
            }
            break;

        default:
            goto combintion_other;
        }
        break;

    default:
combintion_other:
        // Use item combinations table from ItemPMT
        bool combo_applied = false;
        pmt_itemcombination_bb_t combo = { 0 };

        for (size_t z = 0; z < character->inv.item_count; z++) {
            iitem_t* inv_item = &character->inv.iitems[z];
            if (!(inv_item->flags & 0x00000008)) {
                continue;
            }

            __try {
                if (err = pmt_lookup_itemcombination_bb(iitem->data.datal[0], inv_item->data.datal[0], &combo)) {
#ifdef DEBUG
                    ERR_LOG("pmt_lookup_itemcombination_bb ����������! ������ %d", err);
#endif // DEBUG
                    continue;
                }

                if (combo.char_class != 0xFF && combo.char_class != character->dress_data.ch_class) {
                    ERR_LOG("��Ʒ�ϳ���Ҫ�ض������ְҵ");
                    ERR_LOG("combo.class %d player %d", combo.char_class, character->dress_data.ch_class);
                }
                if (combo.mag_level != 0xFF) {
                    if (inv_item->data.datab[0] != ITEM_TYPE_MAG && find_equipped_mag(&character->inv) == -1) {
                        ERR_LOG("��Ʒ�ϳ�������mag����Ҫ��,��װ������Ʒ����mag");
                        ERR_LOG("datab[0] 0x%02X", inv_item->data.datab[0]);
                        return -1;
                    }
                    if (compute_mag_level(&inv_item->data) < combo.mag_level) {
                        ERR_LOG("��Ʒ�ϳ�������mag�ȼ�Ҫ��,��װ����mag�ȼ�����");
                        return -2;
                    }
                }
                if (combo.grind != 0xFF) {
                    if (inv_item->data.datab[0] != ITEM_TYPE_WEAPON && find_equipped_weapon(&character->inv) == -1) {
                        ERR_LOG("��Ʒ�ϳ���������ĥҪ��,��װ������Ʒ��������");
                        return -3;
                    }
                    if (inv_item->data.datab[3] < combo.grind) {
                        ERR_LOG("��Ʒ�ϳ���������ĥҪ��,��װ����������ĥ����");
                        return -4;
                    }
                }
                if (combo.level != 0xFF && character->disp.level + 1 < combo.level) {
                    ERR_LOG("��Ʒ�ϳ������ڵȼ�Ҫ��,����ҵȼ�����");
                    return -5;
                }
                // If we get here, then the combo applies
#ifdef DEBUG
                if (combo_applied) {
                    DBG_LOG("multiple combinations apply");
                }
#endif // DEBUG
                combo_applied = true;

#ifdef DEBUG

                print_item_data(&inv_item->data, src->version);

#endif // DEBUG
                inv_item->data.datab[0] = combo.result_item[0];
                inv_item->data.datab[1] = combo.result_item[1];
                inv_item->data.datab[2] = combo.result_item[2];
                inv_item->data.datab[3] = 0; // Grind
                inv_item->data.datab[4] = 0; // Flags + special

#ifdef DEBUG
                DBG_LOG("result_item 0x%02X 0x%02X 0x%02X", combo.result_item[0], combo.result_item[1], combo.result_item[2]);
                print_item_data(&inv_item->data, src->version);
#endif // DEBUG
            }

            __except (crash_handler(GetExceptionInformation())) {
                // ������ִ���쳣�������߼��������ӡ������Ϣ���ṩ�û��Ѻõ���ʾ��
                CRASH_LOG("ʹ����Ʒ�ϳɳ��ִ���.");
            }
        }


        if (!combo_applied) {
            ERR_LOG("�������κκϳ�");
        }
        break;
    }

done:
    if (should_delete_item) {
        // Allow overdrafting meseta if the client is not BB, since the server isn't
        // informed when meseta is added or removed from the bank.
        iitem_t delete_item = remove_iitem(src, iitem->data.item_id, 1, src->version != CLIENT_VERSION_BB);
        if (delete_item.data.datal[0] == 0 && delete_item.data.data2l == 0) {
            ERR_LOG("��Ʒ ID 0x%08X �Ѳ�����", iitem->data.item_id);
        }
    }

    return err;
}

int initialize_cmode_iitem(ship_client_t* dest) {
    size_t x = 0;
    lobby_t* l = dest->cur_lobby;

    psocn_bb_char_t* character = &dest->mode_pl->bb;

    for (x = 0; x < MAX_PLAYER_INV_ITEMS; x++) {
        clear_iitem(&character->inv.iitems[x]);
    }

    uint8_t char_class = character->dress_data.ch_class;
    uint16_t costume = character->dress_data.costume;
    uint16_t skin = character->dress_data.skin;
    switch (char_class) {
    case CLASS_HUMAR: //������
    case CLASS_HUNEWEARL: //����Ů
    case CLASS_HUCAST: //������սʿ
    case CLASS_HUCASEAL: //������Ůսʿ
        // Saberսʿ
        character->inv.iitems[0].present = 0x0001;
        character->inv.iitems[0].flags = 0x00000008;
        character->inv.iitems[0].data.datab[1] = 0x01; //��һ���ĵط�
        character->inv.iitems[0].data.item_id = 0x00010000;
        break;
    case CLASS_RAMAR: //������ǹ
    case CLASS_RACAST: //����Ůǹ
    case CLASS_RACASEAL: //��������ǹ
    case CLASS_RAMARL: //������Ůǹ
        // Handgunǹ��
        character->inv.iitems[0].present = 0x0001;
        character->inv.iitems[0].flags = 0x00000008;
        character->inv.iitems[0].data.datab[1] = 0x06; //��һ���ĵط�
        character->inv.iitems[0].data.item_id = 0x00010000;
        break;
    case CLASS_FONEWM: //�������з�ʦ
    case CLASS_FONEWEARL: //������Ů��ʦ
    case CLASS_FOMARL: //����Ů��ʦ
    case CLASS_FOMAR: //�����з�ʦ
        // Cane��ʦ
        character->inv.iitems[0].present = 0x0001;
        character->inv.iitems[0].flags = 0x00000008;
        character->inv.iitems[0].data.datab[1] = 0x0A; //��һ���ĵط�
        character->inv.iitems[0].data.item_id = 0x00010000;
        break;
    default:
        break;
    }
    // Frame ���
    character->inv.iitems[1].present = 0x0001;
    character->inv.iitems[1].flags = 0x00000008;
    character->inv.iitems[1].data.datab[0] = 0x01;
    character->inv.iitems[1].data.datab[1] = 0x01;
    character->inv.iitems[1].data.item_id = 0x00010001; //������ƷID��󳤶�65537

    // Mag ��������޸ĳ�ʼ��ŵ�����
    character->inv.iitems[2].present = 0x0001;
    character->inv.iitems[2].flags = 0x00000008;
    character->inv.iitems[2].data.datab[0] = 0x02;
    character->inv.iitems[2].data.datab[2] = 0x05;
    character->inv.iitems[2].data.datab[4] = 0xF4;
    character->inv.iitems[2].data.datab[5] = 0x01;
    character->inv.iitems[2].data.data2b[0] = 0x14; // 20% synchro 20%ͬ��
    character->inv.iitems[2].data.item_id = 0x00010002;

    if ((char_class == CLASS_HUCAST) || (char_class == CLASS_HUCASEAL) ||
        (char_class == CLASS_RACAST) || (char_class == CLASS_RACASEAL))
        character->inv.iitems[2].data.data2b[3] = (uint8_t)skin;
    else
        character->inv.iitems[2].data.data2b[3] = (uint8_t)costume;

    if (character->inv.iitems[2].data.data2b[3] > 0x11)
        character->inv.iitems[2].data.data2b[3] -= 0x11;

    // Monomates ����
    character->inv.iitems[3].present = 0x0001;
    character->inv.iitems[3].data.datab[0] = 0x03;
    character->inv.iitems[3].data.datab[5] = 0x04;
    character->inv.iitems[3].data.item_id = 0x00010003;

    if ((char_class == CLASS_FONEWM) || (char_class == CLASS_FONEWEARL) ||
        (char_class == CLASS_FOMARL) || (char_class == CLASS_FOMAR))
    { //��Ӧƥ����������ģ��
      // Monofluids �����壿
        character->tech.foie = 0x00;//���������� ������
        character->inv.iitems[4].present = 0x0001;
        character->inv.iitems[4].flags = 0;
        character->inv.iitems[4].data.datab[0] = 0x03;
        character->inv.iitems[4].data.datab[1] = 0x01;
        character->inv.iitems[4].data.datab[2] = 0x00;
        character->inv.iitems[4].data.datab[3] = 0x00;
        memset(&character->inv.iitems[4].data.datab[4], 0x00, 8);
        character->inv.iitems[4].data.datab[5] = 0x04;
        character->inv.iitems[4].data.item_id = 0x00010004;
        memset(&character->inv.iitems[3].data.data2b[0], 0x00, 4);
        character->inv.item_count = 5;
    }
    else {
        character->inv.item_count = 4;
    }

    //����һ�´���λ�� ��ƥ���ֽڽṹ
    character->inv.hpmats_used = 0; //Ѫ��
    character->inv.tpmats_used = 0; //ħ��
    character->inv.language = 0; //����

    ////����հױ��� ����ΪС��30
    //for (ch = Character_NewE7->item_count; ch < 30; ch++)
    //{
    //    player->inv.iitems[ch].num_items = 0x00;
    //    player->inv.iitems[ch].data.datab[1] = 0xFF;
    //    player->inv.iitems[ch].data.item_id = EMPTY_STRING;
    //}

    switch (l->episode)
    {
    case GAME_TYPE_EPISODE_1:
        switch (character->dress_data.ch_class) {
        case CLASS_HUMAR: // ����������
        case CLASS_HUNEWEARL: // ������Ů����
        case CLASS_HUCAST: // ������������
        case CLASS_RAMAR: // ������ǹ��
        case CLASS_RACAST: // ��������ǹ��
        case CLASS_RACASEAL: // ������Ůǹ��
        case CLASS_FOMARL: // ����Ů��ʦ
        case CLASS_FONEWM: // �������з�ʦ
        case CLASS_FONEWEARL: // ������Ů��ʦ
        case CLASS_HUCASEAL: // ������Ů����
        case CLASS_FOMAR: // �����з�ʦ
        case CLASS_RAMARL: // ����Ůǹ��
            break;
        default:
            ERR_LOG("��Ч��ɫְҵ");
            return -2;
        }


        switch (l->challenge)
        {
        default:
            break;
        }
        break;

    case GAME_TYPE_EPISODE_2:
        break;

    default:
        ERR_LOG("δ֪��սģʽ�½� %d", l->episode);
        return -1;
    }

    DBG_LOG("����GC %u ְҵ����", dest->guildcard);

    return 0;
}

iitem_t player_iitem_init(const item_t item) {
    iitem_t iitem = { LE16(0x0001), 0, 0, 0, item };
    return iitem;
}

bitem_t player_bitem_init(const item_t item) {
    bitem_t bitem = { item, (uint16_t)stack_size(&item), LE16(0x0001) };
    return bitem;
}

/* ��������Ʒ���� */
void cleanup_bb_inv(uint32_t client_id, inventory_t* inv) {
    uint32_t item_id = 0x00010000 | (client_id << 21);

    uint32_t count = LE32(inv->item_count), i;

    for (i = 0; i < count; ++i) {
        inv->iitems[i].data.item_id = LE32(item_id);
        ++item_id;
    }

    /* Clear all the rest of them... */
    for (; i < MAX_PLAYER_INV_ITEMS; ++i) {
        clear_iitem(&inv->iitems[i]);
    }
}

/* ����������Ʒ���� */
void regenerate_bank_item_id(uint32_t client_id, psocn_bank_t* bank, bool comoon_bank) {
    uint32_t item_id = 0x80010000 | (client_id << 21);

    if (comoon_bank)
        item_id = 0x80110000 | (client_id << 21);

    uint32_t count = LE32(bank->item_count), i;

    for(i = 0; i < count; ++i) {
        bank->bitems[i].data.item_id = LE32(item_id);
        ++item_id;
    }

    /* Clear all the rest of them... */
    for(; i < MAX_PLAYER_BANK_ITEMS; ++i) {
        clear_bitem(&bank->bitems[i]);
    }
}

//���װ���������item_equip_flags
int item_check_equip(uint8_t װ����ǩ, uint8_t �ͻ���װ����ǩ) {
    int32_t eqOK = EQUIP_FLAGS_OK;
    uint32_t ch;

    for (ch = 0; ch < EQUIP_FLAGS_MAX; ch++) {
        if ((�ͻ���װ����ǩ & (1 << ch)) && (!(װ����ǩ & (1 << ch)))) {
            eqOK = EQUIP_FLAGS_NONE;
            break;
        }
    }

    return eqOK;
}

/* ��Ʒ���װ����ǩ */
int item_check_equip_flags(uint32_t gc, uint32_t target_level, uint8_t equip_flags, inventory_t* inv, uint32_t item_id) {
    pmt_weapon_bb_t tmp_wp = { 0 };
    pmt_guard_bb_t tmp_guard = { 0 };
    uint32_t found = 0, found_slot = 0, j = 0, slot[4] = { 0 }, inv_count = 0;
    int i = 0;

    i = find_iitem_index(inv, item_id);
#ifdef DEBUG
    DBG_LOG("ʶ���λ %d ������ƷID %d ������ƷID %d", i, inv->iitems[i].data.item_id, item_id);
    print_item_data(&inv->iitems[i].data, c->version);
#endif // DEBUG

    /* ����Ҳ�������Ʒ�����û��Ӵ�������. */
    if (i < 0) {
        ERR_LOG("GC %" PRIu32 " װ������Ʒ��Ч! ������ %d", gc, i);
        return -1;
    }

    item_t* found_item = &inv->iitems[i].data;

    if (!found_item) {
        ERR_LOG("GC %" PRIu32 " װ������Ʒ�Ѳ�����!", gc);
        return -2;
    }

    if (found_item->item_id == item_id) {
        found = 1;
        inv_count = inv->item_count;

        switch (found_item->datab[0]) {
        case ITEM_TYPE_WEAPON:
            if (pmt_lookup_weapon_bb(found_item->datal[0], &tmp_wp)) {
                ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                    gc);
                return -3;
            }

            if (item_check_equip(tmp_wp.equip_flag, equip_flags)) {
                ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                    gc);
                return -4;
            }
            else {
                // �����ɫ���κ�����������װ��������ֹ�ѵ��� 
                for (j = 0; j < inv_count; j++)
                    if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_WEAPON) &&
                        (inv->iitems[j].flags & LE32(0x00000008))) {
                        inv->iitems[j].flags &= LE32(0xFFFFFFF7);
                        //DBG_LOG("ж������");
                    }
                //DBG_LOG("����ʶ�� %02X", tmp_wp.equip_flag);
            }
            break;

        case ITEM_TYPE_GUARD:
            switch (found_item->datab[1]) {
            case ITEM_SUBTYPE_FRAME:
                if (pmt_lookup_guard_bb(found_item->datal[0], &tmp_guard)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                        gc);
                    return -5;
                }

                if (target_level < tmp_guard.level_req) {
                    ERR_LOG("GC %" PRIu32 " �ȼ�����, ��Ӧ��װ������Ʒ����!",
                        gc);
                    return -6;
                }

                if (item_check_equip(tmp_guard.equip_flag, equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                        gc);
                    return -7;
                }
                else {
                    //DBG_LOG("װ��ʶ��");
                    // �Ƴ�����װ�׺Ͳ��
                    for (j = 0; j < inv_count; ++j) {
                        if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_GUARD) &&
                            (inv->iitems[j].data.datab[1] != ITEM_SUBTYPE_BARRIER) &&
                            (inv->iitems[j].flags & LE32(0x00000008))) {
                            //DBG_LOG("ж��װ��");
                            inv->iitems[j].flags &= LE32(0xFFFFFFF7);
                            inv->iitems[j].data.datab[4] = 0x00;
                        }
                    }
                    break;
                }
                break;

            case ITEM_SUBTYPE_BARRIER: // Check barrier equip requirements ��⻤��װ������
                if (pmt_lookup_guard_bb(found_item->datal[0], &tmp_guard)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                        gc);
                    return -8;
                }

                if (target_level < tmp_guard.level_req) {
                    ERR_LOG("GC %" PRIu32 " �ȼ�����, ��Ӧ��װ������Ʒ����!",
                        gc);
                    return -9;
                }

                if (item_check_equip(tmp_guard.equip_flag, equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                        gc);
                    return -10;
                }else {
                    //DBG_LOG("����ʶ��");
                    // Remove any other barrier
                    for (j = 0; j < inv_count; ++j) {
                        if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_GUARD) &&
                            (inv->iitems[j].data.datab[1] == ITEM_SUBTYPE_BARRIER) &&
                            (inv->iitems[j].flags & LE32(0x00000008))) {
                            //DBG_LOG("ж�ػ���");
                            inv->iitems[j].flags &= LE32(0xFFFFFFF7);
                            inv->iitems[j].data.datab[4] = 0x00;
                        }
                    }
                }
                break;

            case ITEM_SUBTYPE_UNIT:// Assign unit a slot
                //DBG_LOG("���ʶ��");
                for (j = 0; j < 4; j++)
                    slot[j] = 0;

                for (j = 0; j < inv_count; j++) {
                    // Another loop ;(
                    if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_GUARD) &&
                        (inv->iitems[j].data.datab[1] == ITEM_SUBTYPE_UNIT)) {
                        //DBG_LOG("��� %d ʶ��", j);
                        if ((inv->iitems[j].flags & LE32(0x00000008)) &&
                            (inv->iitems[j].data.datab[4] < 0x04)) {

                            slot[inv->iitems[j].data.datab[4]] = 1;
                            //DBG_LOG("��� %d ж��", j);
                        }
                    }
                }

                for (j = 0; j < 4; j++) {
                    if (slot[j] == 0) {
                        found_slot = j + 1;
                        break;
                    }
                }

                if (found_slot) {
                    found_slot--;
                    inv->iitems[j].data.datab[4] = (uint8_t)(found_slot);
                }
                else {//ȱʧ TODO
                    inv->iitems[j].flags &= LE32(0xFFFFFFF7);
                    ERR_LOG("GC %" PRIu32 " װ�������׵Ĳ����Ʒ����!",
                        gc);
                    return -11;
                }
                break;
            }
            break;

        case ITEM_TYPE_MAG:
            //DBG_LOG("���ʶ��");
            // Remove equipped mag
            for (j = 0; j < inv->item_count; j++)
                if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_MAG) &&
                    (inv->iitems[j].flags & LE32(0x00000008))) {

                    inv->iitems[j].flags &= LE32(0xFFFFFFF7);
                    //DBG_LOG("ж�����");
                }
            break;
        }

        //DBG_LOG("���ж��, ����δʶ��ɹ�");

        /* TODO: Should really make sure we can equip it first... */
        inv->iitems[i].flags |= LE32(0x00000008);
    }

    return 0;
}

/* ���ͻ��˱�ǿɴ���ְҵװ���ı�ǩ */
int item_class_tag_equip_flag(ship_client_t* c) {
    uint8_t c_class = c->bb_pl->character.dress_data.ch_class;

    c->equip_flags = 0;

    switch (c_class)
    {
    case CLASS_HUMAR:
        c->equip_flags |= EQUIP_FLAGS_HUNTER;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_HUNEWEARL:
        c->equip_flags |= EQUIP_FLAGS_HUNTER;
        c->equip_flags |= EQUIP_FLAGS_NEWMAN;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_HUCAST:
        c->equip_flags |= EQUIP_FLAGS_HUNTER;
        c->equip_flags |= EQUIP_FLAGS_DROID;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_HUCASEAL:
        c->equip_flags |= EQUIP_FLAGS_HUNTER;
        c->equip_flags |= EQUIP_FLAGS_DROID;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_RAMAR:
        c->equip_flags |= EQUIP_FLAGS_RANGER;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_RACAST:
        c->equip_flags |= EQUIP_FLAGS_RANGER;
        c->equip_flags |= EQUIP_FLAGS_DROID;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_RACASEAL:
        c->equip_flags |= EQUIP_FLAGS_RANGER;
        c->equip_flags |= EQUIP_FLAGS_DROID;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;
    case CLASS_RAMARL:
        c->equip_flags |= EQUIP_FLAGS_RANGER;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_FONEWM:
        c->equip_flags |= EQUIP_FLAGS_FORCE;
        c->equip_flags |= EQUIP_FLAGS_NEWMAN;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;

    case CLASS_FONEWEARL:
        c->equip_flags |= EQUIP_FLAGS_FORCE;
        c->equip_flags |= EQUIP_FLAGS_NEWMAN;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_FOMARL:
        c->equip_flags |= EQUIP_FLAGS_FORCE;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_FEMALE;
        break;

    case CLASS_FOMAR:
        c->equip_flags |= EQUIP_FLAGS_FORCE;
        c->equip_flags |= EQUIP_FLAGS_HUMAN;
        c->equip_flags |= EQUIP_FLAGS_MALE;
        break;
    }

    return 0;
}

//�޸������������ݴ������Ʒ����
void fix_inv_bank_item(item_t* i) {
    uint32_t ch3;
    pmt_guard_bb_t tmp_guard = { 0 };

    switch (i->datab[0]) {
    case ITEM_TYPE_WEAPON:// ����������ֵ
        int8_t percent_table[6] = { 0 };
        int8_t percent = 0;
        uint32_t max_percents = 0, num_percents = 0;
        int32_t srank = 0;

        if ((i->datab[1] == 0x33) ||  // SJS��Lame���2%
            (i->datab[1] == 0xAB))
            max_percents = 2;
        else
            max_percents = 3;

        memset(&percent_table[0], 0, 6);

        for (ch3 = 6; ch3 <= 4 + (max_percents * 2); ch3 += 2) {
            if (i->datab[ch3] & 128) {
                srank = 1; // S-Rank
                break;
            }

            if ((i->datab[ch3]) && (i->datab[ch3] < 0x06)) {
                // Percents over 100 or under -100 get set to 0 
                // �ٷֱȸ���100�����-100����Ϊ0
                percent = (int8_t)i->datab[ch3 + 1];

                if ((percent > 100) || (percent < -100))
                    percent = 0;
                // ����ٷֱ�
                percent_table[i->datab[ch3]] = percent;
            }
        }

        if (!srank) {
            for (ch3 = 6; ch3 <= 4 + (max_percents * 2); ch3 += 2) {
                // ���� %s
                i->datab[ch3] = 0;
                i->datab[ch3 + 1] = 0;
            }

            for (ch3 = 1; ch3 <= 5; ch3++) {
                // �ؽ� %s
                if (percent_table[ch3]) {
                    i->datab[6 + (num_percents * 2)] = ch3;
                    i->datab[7 + (num_percents * 2)] = (uint8_t)percent_table[ch3];
                    num_percents++;
                    if (num_percents == max_percents)
                        break;
                }
            }
        }

        break;

    case ITEM_TYPE_GUARD:// ����װ�׺ͻ��ܵ�ֵ
        switch (i->datab[1])
        {
        case ITEM_SUBTYPE_FRAME:

            if (pmt_lookup_guard_bb(i->datal[0], &tmp_guard)) {
                ERR_LOG("δ��PMT��ȡ�� 0x%04X ������!", i->datal[0]);
                break;
            }

            if (i->datab[6] > tmp_guard.dfp_range)
                i->datab[6] = tmp_guard.dfp_range;
            if (i->datab[8] > tmp_guard.evp_range)
                i->datab[8] = tmp_guard.evp_range;
            break;

        case ITEM_SUBTYPE_BARRIER:

            if (pmt_lookup_guard_bb(i->datal[0], &tmp_guard)) {
                ERR_LOG("δ��PMT��ȡ�� 0x%04X ������!", i->datal[0]);
                break;
            }

            if (i->datab[6] > tmp_guard.dfp_range)
                i->datab[6] = tmp_guard.dfp_range;
            if (i->datab[8] > tmp_guard.evp_range)
                i->datab[8] = tmp_guard.evp_range;
            break;
        }
        break;

    case ITEM_TYPE_MAG:// ���
        magitem_t* playermag;
        int16_t mag_def, mag_pow, mag_dex, mag_mind;
        int32_t total_levels;

        playermag = (magitem_t*)&i->datab[0];

        if (playermag->synchro > 120)
            playermag->synchro = 120;

        if (playermag->synchro < 0)
            playermag->synchro = 0;

        if (playermag->IQ > 200)
            playermag->IQ = 200;

        if ((playermag->def < 0) || (playermag->pow < 0) || (playermag->dex < 0) || (playermag->mind < 0))
            total_levels = 201; // Auto fail if any stat is under 0...
        else {
            mag_def = playermag->def / 100;
            mag_pow = playermag->pow / 100;
            mag_dex = playermag->dex / 100;
            mag_mind = playermag->mind / 100;
            total_levels = mag_def + mag_pow + mag_dex + mag_mind;
        }

        if ((total_levels > 200) || (playermag->level > 200)) {
            // �������ʧ��,���ʼ����������
            playermag->def = 500;
            playermag->pow = 0;
            playermag->dex = 0;
            playermag->mind = 0;
            playermag->level = 5;
            playermag->photon_blasts = 0;
            playermag->IQ = 0;
            playermag->synchro = 20;
            playermag->mtype = 0;
            playermag->PBflags = 0;
        }
        break;
    }

}

//�޸�����װ�����ݴ������Ʒ����
void fix_equip_item(inventory_t* inv) {
    uint32_t i, eq_weapon = 0, eq_armor = 0, eq_shield = 0, eq_mag = 0;

    /* ���������װ������Ʒ */
    for (i = 0; i < inv->item_count; i++) {
        if (inv->iitems[i].flags & LE32(0x00000008)) {
            switch (inv->iitems[i].data.datab[0])
            {
            case ITEM_TYPE_WEAPON:
                eq_weapon++;
                break;

            case ITEM_TYPE_GUARD:
                switch (inv->iitems[i].data.datab[1])
                {
                case ITEM_SUBTYPE_FRAME:
                    eq_armor++;
                    break;

                case ITEM_SUBTYPE_BARRIER:
                    eq_shield++;
                    break;
                }
                break;

            case ITEM_TYPE_MAG:
                eq_mag++;
                break;
            }
        }
    }

    if (eq_weapon > 1) {
        for (i = 0; i < inv->item_count; i++) {
            // Unequip all weapons when there is more than one equipped.  
            // ��װ���˶������ʱ,ȡ����װ������
            if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_WEAPON) &&
                (inv->iitems[i].flags & LE32(0x00000008)))
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
        }

    }

    if (eq_armor > 1) {
        for (i = 0; i < inv->item_count; i++) {
            // Unequip all armor and slot items when there is more than one armor equipped. 
            // ��װ���˶������ʱ��ȡ��װ�����л��׺Ͳ۵��ߡ� 
            if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_GUARD) &&
                (inv->iitems[i].data.datab[1] == ITEM_SUBTYPE_FRAME) &&
                (inv->iitems[i].flags & LE32(0x00000008))) {

                inv->iitems[i].data.datab[3] = 0x00;
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    if (eq_shield > 1) {
        for (i = 0; i < inv->item_count; i++) {
            // Unequip all shields when there is more than one equipped. 
            // ��װ���˶������ʱ��ȡ��װ�����л��ܡ� 
            if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_GUARD) &&
                (inv->iitems[i].data.datab[1] == ITEM_SUBTYPE_BARRIER) &&
                (inv->iitems[i].flags & LE32(0x00000008))) {

                inv->iitems[i].data.datab[3] = 0x00;
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    if (eq_mag > 1) {
        for (i = 0; i < inv->item_count; i++) {
            // Unequip all mags when there is more than one equipped. 
            // ��װ���˶�����ʱ��ȡ��װ��������š� 
            if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_MAG) &&
                (inv->iitems[i].flags & LE32(0x00000008)))
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
        }
    }
}

/* ��������Ʒ */
void fix_client_inv(inventory_t* inv) {
    uint8_t i, j = 0;
    iitem_t fix_iitem[MAX_PLAYER_INV_ITEMS] = { 0 };

    for (i = 0; i < MAX_PLAYER_INV_ITEMS; i++)
        clear_iitem(&fix_iitem[i]);

    for (i = 0; i < MAX_PLAYER_INV_ITEMS; i++)
        if (inv->iitems[i].present == 0x0001 || inv->iitems[i].present == 0x0002)
            fix_iitem[j++] = inv->iitems[i];

    inv->item_count = j;

    for (i = 0; i < MAX_PLAYER_INV_ITEMS; i++)
        inv->iitems[i] = fix_iitem[i];
}

//����ֿ���Ʒ ������
void sort_client_inv(inventory_t* inv) {
    int i = 0, j = 0;
    uint32_t compare_item1 = 0, compare_item2 = 0;
    uint8_t swap_c = 0;
    iitem_t swap_item = { 0 }, b1 = { 0 }, b2 = { 0 };

    if (inv->item_count > 1) {
        for (i = 0; i < (inv->item_count - 1); i++) {
            memcpy(&b1, &inv->iitems[i], sizeof(iitem_t));
            swap_c = b1.data.datab[0];
            b1.data.datab[0] = b1.data.datab[2];
            b1.data.datab[2] = swap_c;
            memcpy(&compare_item1, &b1.data.datab[0], 3);
            for (j = i + 1; j < inv->item_count; j++) {
                memcpy(&b2, &inv->iitems[j], sizeof(iitem_t));
                swap_c = b2.data.datab[0];
                b2.data.datab[0] = b2.data.datab[2];
                b2.data.datab[2] = swap_c;
                memcpy(&compare_item2, &b2.data.datab[0], 3);
                if (compare_item2 < compare_item1) { // compare_item2 should take compare_item1's place
                    memcpy(&swap_item, &inv->iitems[i], sizeof(iitem_t));
                    memcpy(&inv->iitems[i], &inv->iitems[j], sizeof(iitem_t));
                    memcpy(&inv->iitems[j], &swap_item, sizeof(iitem_t));
                    memcpy(&compare_item1, &compare_item2, 3);
                }
            }
        }
    }
}

void fix_client_bank(psocn_bank_t* bank) {
    size_t i, j = 0;
    bitem_t fix_bitem[MAX_PLAYER_BANK_ITEMS] = { 0 };

    for (j = 0; j < MAX_PLAYER_BANK_ITEMS; j++)
        clear_bitem(&fix_bitem[j]);

    j = 0;

    for (i = 0; i < MAX_PLAYER_BANK_ITEMS; i++)
        if ((bank->bitems[i].show_flags == 0x0001) && bank->bitems[i].amount >= 1)
            fix_bitem[j++] = bank->bitems[i];

    bank->item_count = j;

    for (i = 0; i < MAX_PLAYER_BANK_ITEMS; i++)
        bank->bitems[i] = fix_bitem[i];
}

//����ֿ���Ʒ
void sort_client_bank(psocn_bank_t* bank) {
    size_t i, j;
    uint32_t compare_item1 = 0;
    uint32_t compare_item2 = 0;
    uint8_t swap_c;
    bitem_t swap_item;
    bitem_t b1;
    bitem_t b2;

    if (bank->item_count > 1) {
        for (i = 0; i < bank->item_count - 1; i++) {
            memcpy(&b1, &bank->bitems[i], sizeof(bitem_t));
            swap_c = b1.data.datab[0];
            b1.data.datab[0] = b1.data.datab[2];
            b1.data.datab[2] = swap_c;
            memcpy(&compare_item1, &b1.data.datab[0], 3);
            for (j = i + 1; j < bank->item_count; j++) {
                memcpy(&b2, &bank->bitems[j], sizeof(bitem_t));
                swap_c = b2.data.datab[0];
                b2.data.datab[0] = b2.data.datab[2];
                b2.data.datab[2] = swap_c;
                memcpy(&compare_item2, &b2.data.datab[0], 3);
                if (compare_item2 < compare_item1) { // compare_item2 should take compare_item1's place
                    memcpy(&swap_item, &bank->bitems[i], sizeof(bitem_t));
                    memcpy(&bank->bitems[i], &bank->bitems[j], sizeof(bitem_t));
                    memcpy(&bank->bitems[j], &swap_item, sizeof(bitem_t));
                    memcpy(&compare_item1, &compare_item2, 3);
                }
            }
        }
    }
}