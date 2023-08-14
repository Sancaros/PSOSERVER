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
#include <mtwist.h>

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

/* �޸���ұ������� */
void regenerate_lobby_item_id(lobby_t* l, ship_client_t* c) {
    uint32_t id;
    int i;

    inventory_t* inv = &c->bb_pl->character.inv;

    if (c->mode)
        inv = &c->mode_pl->bb.inv;

    if (c->version == CLIENT_VERSION_BB) {
        /* ���·�����������ұ���ID */
        id = 0x00010000 | (c->client_id << 21) | (l->item_player_id[c->client_id]);

        for (i = 0; i < inv->item_count; ++i, ++id) {
            inv->iitems[i].data.item_id = LE32(id);
        }

        --id;
        l->item_player_id[c->client_id] = id;
    }
    else {
        /* Fix up the inventory for their new lobby */
        id = 0x00010000 | (c->client_id << 21) | (l->item_player_id[c->client_id]);

        for (i = 0; i < c->item_count; ++i, ++id) {
            c->iitems[i].data.item_id = LE32(id);
        }

        --id;
        l->item_player_id[c->client_id] = id;
    }
}

/* ����һ����Ʒ������������. �������ڵ������֮ǰ������д����Ļ�����.
��������Ŀ����û������Ʒ�Ŀռ�,�򷵻�NULL. */
iitem_t* add_new_litem_locked(lobby_t* l, item_t* new_item, uint8_t area, float x, float z) {
    lobby_item_t* item;

    /* �����Լ��... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    item = (lobby_item_t*)malloc(sizeof(lobby_item_t));

    if (!item)
        return NULL;

    memset(item, 0, sizeof(lobby_item_t));

    /* Copy the item data in. */
    item->data.present = LE16(0x0001);
    item->data.extension_data1 = 0;
    item->data.extension_data2 = 0;
    item->data.flags = LE32(0);

    item->data.data.datal[0] = LE32(new_item->datal[0]);
    item->data.data.datal[1] = LE32(new_item->datal[1]);
    item->data.data.datal[2] = LE32(new_item->datal[2]);
    item->data.data.item_id = LE32(l->item_lobby_id);
    item->data.data.data2l = LE32(new_item->data2l);

    item->x = x;
    item->z = z;
    item->area = area;

#ifdef DEBUG

    print_iitem_data(&item->d, 0, l->version);

#endif // DEBUG

    /* Increment the item ID, add it to the queue, and return the new item */
    ++l->item_lobby_id;
    TAILQ_INSERT_HEAD(&l->item_queue, item, qentry);
    return &item->data;
}

/* ��Ҷ����� ȡ���� �������Ʒ */
iitem_t* add_litem_locked(lobby_t* l, iitem_t* it) {
    lobby_item_t* item;

    /* �����Լ��... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    item = (lobby_item_t*)malloc(sizeof(lobby_item_t));

    if (!item)
        return NULL;

    memset(item, 0, sizeof(lobby_item_t));

    /* Copy the item data in. */
    memcpy(&item->data, it, PSOCN_STLENGTH_IITEM);

    /* Add it to the queue, and return the new item */
    TAILQ_INSERT_HEAD(&l->item_queue, item, qentry);
    return &item->data;
}

int remove_litem_locked(lobby_t* l, uint32_t item_id, iitem_t* rv) {
    lobby_item_t* i, * tmp;

    if (l->version != CLIENT_VERSION_BB)
        return -1;

    clear_iitem(rv);

    //memset(rv, 0, PSOCN_STLENGTH_IITEM);
    //rv->data.datal[0] = LE32(Item_NoSuchItem);

    i = TAILQ_FIRST(&l->item_queue);
    while (i) {
        tmp = TAILQ_NEXT(i, qentry);

        if (i->data.data.item_id == item_id) {
            memcpy(rv, &i->data, PSOCN_STLENGTH_IITEM);
            TAILQ_REMOVE(&l->item_queue, i, qentry);
            free_safe(i);
            return 0;
        }

        i = tmp;
    }

    return 1;
}

size_t find_iitem_index(inventory_t* inv, uint32_t item_id) {
    size_t x;

    for (x = 0; x < inv->item_count; x++) {
        if (inv->iitems[x].data.item_id == item_id) {
            return x;
        }
    }

    ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", item_id);
#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    return -1;
}

size_t find_bitem_index(psocn_bank_t* bank, uint32_t item_id) {
    size_t x;

    for (x = 0; x < bank->item_count; x++) {
        if (bank->bitems[x].data.item_id == item_id) {
            return x;
        }
    }

    ERR_LOG("δ���������ҵ�ID 0x%08X ��Ʒ", item_id);

#ifdef DEBUG

    for (x = 0; x < bank->item_count; x++) {
        print_bitem_data(&bank->bitems[x], x, 5);
    }

#endif // DEBUG

    return -1;
}

/* ������PD PC �ȶ����ѵ���Ʒ �������ڵ�����Ʒ */
size_t find_iitem_stack_item_id(inventory_t* inv, iitem_t* item) {
    uint32_t pid = primary_identifier(&item->data);
    size_t x;

    for (x = 0; x < inv->item_count; x++) {
        if (primary_identifier(&inv->iitems[x].data) == pid) {
            return inv->iitems[x].data.item_id;
        }
    }

    ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", pid);
#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    return -1;
}

size_t find_iitem_pid(inventory_t* inv, iitem_t* item) {
    uint32_t pid = primary_identifier(&item->data);
    size_t x;

    for (x = 0; x < inv->item_count; x++) {
        if (primary_identifier(&inv->iitems[x].data) == pid) {
            return primary_identifier(&inv->iitems[x].data);
        }
    }

    ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", pid);
#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    return -1;
}

size_t find_iitem_pid_index(inventory_t* inv, iitem_t* item) {
    uint32_t pid = primary_identifier(&item->data);
    size_t x;

    for (x = 0; x < inv->item_count; x++) {
        if (primary_identifier(&inv->iitems[x].data) == pid) {
            return x;
        }
    }

    ERR_LOG("δ�ӱ������ҵ�ID 0x%08X ��Ʒ", pid);
#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    return -1;
}

bool is_wrapped(item_t* this) {
    switch (this->datab[0]) {
    case ITEM_TYPE_WEAPON:
    case ITEM_TYPE_GUARD:
        return this->datab[4] & 0x40;
    case ITEM_TYPE_MAG:
        return this->data2b[2] & 0x40;
    case ITEM_TYPE_TOOL:
        return !is_stackable(this) && (this->datab[3] & 0x40);
    case ITEM_TYPE_MESETA:
        return false;
    }

    ERR_LOG("��Ч��Ʒ���� 0x%02X", this->datab[0]);
    return false;
}

void unwrap(item_t* this) {
    switch (this->datab[0]) {
    case ITEM_TYPE_WEAPON:
    case ITEM_TYPE_GUARD:
        this->datab[4] &= 0xBF;
        break;
    case ITEM_TYPE_MAG:
        this->data2b[2] &= 0xBF;
        break;
    case ITEM_TYPE_TOOL:
        if (!is_stackable(this)) {
            this->datab[3] &= 0xBF;
        }
        break;
    case 4:
        break;
    default:
        ERR_LOG("��Ч unwrap ��Ʒ����");
    }
}

/* ��ȡ������Ŀ����Ʒ���ڲ�λ */
size_t find_equipped_weapon(inventory_t* inv){
    ssize_t ret = -1;
    for (size_t y = 0; y < inv->item_count; y++) {
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
size_t find_equipped_armor(inventory_t* inv) {
    ssize_t ret = -1;
    for (size_t y = 0; y < inv->item_count; y++) {
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
size_t find_equipped_mag(inventory_t* inv) {
    ssize_t ret = -1;
    for (size_t y = 0; y < inv->item_count; y++) {
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
        }
    }
    if (ret < 0) {
        ERR_LOG("δ�ӱ���������װ�������");
    }
    return ret;
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
    psocn_bb_char_t* character = &src->bb_pl->character;

    if (src->mode)
        character = &src->mode_pl->bb;

    if (item_id == 0xFFFFFFFF) {
        if (amount <= character->disp.meseta) {
            character->disp.meseta -= amount;
        }
        else if (allow_meseta_overdraft) {
            character->disp.meseta = 0;
        }
        else {
            ERR_LOG("GC %" PRIu32 " �����������������ӵ�е�",
                src->guildcard);
            return ret;
        }
        ret.data.datab[0] = 0x04;
        ret.data.data2l = amount;
        return ret;
    }

    size_t index = find_iitem_index(&character->inv, item_id);
    iitem_t* inventory_item = &character->inv.iitems[index];

    if (amount && (stack_size(&inventory_item->data) > 1) && (amount < inventory_item->data.datab[5])) {
        ret = *inventory_item;
        ret.data.datab[5] = amount;
        ret.data.item_id = 0xFFFFFFFF;
        inventory_item->data.datab[5] -= amount;
        return ret;
    }

    // If we get here, then it's not meseta, and either it's not a combine item or
    // we're removing the entire stack. Delete the item from the inventory slot
    // and return the deleted item.
    memcpy(&ret, inventory_item, sizeof(iitem_t));
    //ret = inventory_item;
    character->inv.item_count--;
    for (size_t x = index; x < character->inv.item_count; x++) {
        character->inv.iitems[x] = character->inv.iitems[x + 1];
    }
    clear_iitem(&character->inv.iitems[character->inv.item_count]);
    return ret;
}

bitem_t remove_bitem(ship_client_t* src, uint32_t item_id, uint32_t amount) {
    bitem_t ret = { 0 };
    psocn_bank_t* bank = &src->bb_pl->bank;

    if (src->bank_type)
        bank = src->common_bank;
    
    // ����Ƿ񳬳�����������
    if (item_id == 0xFFFFFFFF) {
        if (amount > bank->meseta) {
            ERR_LOG("GC %" PRIu32 " �Ƴ���������������ӵ�е�",
                src->guildcard);
            return ret;
        }
        ret.data.datab[0] = 0x04;
        ret.data.data2l = amount;
        bank->meseta -= amount;
        return ret;
    }

    // ����������Ʒ������
    size_t index = find_bitem_index(bank, item_id);

    if (index == bank->item_count) {
        ERR_LOG("GC %" PRIu32 " ������Ʒ������������",
            src->guildcard);
        return ret;
    }

    bitem_t* bank_item = &bank->bitems[index];

    // ����Ƿ񳬳���Ʒ�ɶѵ�������
    if (amount && (stack_size(&bank_item->data) > 1) &&
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
    for (size_t x = index; x < bank->item_count; x++) {
        bank->bitems[x] = bank->bitems[x + 1];
    }
    clear_bitem(&bank->bitems[bank->item_count]);
    return ret;
}

bool add_iitem(ship_client_t* src, iitem_t* item) {
    uint32_t pid = primary_identifier(&item->data);
    psocn_bb_char_t* player = &src->bb_pl->character;

    if (src->mode)
        player = &src->mode_pl->bb;

    // ����Ƿ�Ϊmeseta������ǣ����޸�ͳ�������е�mesetaֵ
    if (pid == MESETA_IDENTIFIER) {
        player->disp.meseta += item->data.data2l;
        if (player->disp.meseta > 999999) {
            player->disp.meseta = 999999;
        }
        return true;
    }

    // ����ɺϲ�����Ʒ
    size_t combine_max = max_stack_size(&item->data);
    if (combine_max > 1) {
        // �����ҿ�����Ѿ�������ͬ��Ʒ�Ķѵ�����ȡ����Ʒ������
        size_t y;
        for (y = 0; y < player->inv.item_count; y++) {
            if (primary_identifier(&player->inv.iitems[y].data) == pid) {
                break;
            }
        }

        // ������ڶѵ�����������ӣ����������ѵ�����
        if (y < player->inv.item_count) {
            player->inv.iitems[y].data.datab[5] += item->data.datab[5];
            if (player->inv.iitems[y].data.datab[5] > (uint8_t)combine_max) {
                player->inv.iitems[y].data.datab[5] = (uint8_t)combine_max;
            }
            return true;
        }
    }

    // ���ִ�е�����Ȳ���mesetaҲ���ǿɺϲ���Ʒ�������Ҫ����һ���յĿ���λ
    if (player->inv.item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("GC %" PRIu32 " ������Ʒ�����������ֵ",
            src->guildcard);
        return false;
    }
    player->inv.iitems[player->inv.item_count] = *item;
    player->inv.item_count++;
    return true;
}

bool add_bitem(ship_client_t* src, bitem_t* item) {
    uint32_t pid = primary_identifier(&item->data);
    psocn_bank_t* bank = &src->bb_pl->bank;

    if (src->bank_type)
        bank = src->common_bank;

    if (pid == MESETA_IDENTIFIER) {
        bank->meseta += item->data.data2l;
        if (bank->meseta > 999999) {
            bank->meseta = 999999;
        }
        return true;
    }
    
    size_t combine_max = max_stack_size(&item->data);
    if (combine_max > 1) {
        size_t y;
        for (y = 0; y < bank->item_count; y++) {
            if (primary_identifier(&bank->bitems[y].data) == pid) {
                break;
            }
        }

        if (y < bank->item_count) {
            bank->bitems[y].data.datab[5] += item->data.datab[5];
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
    bank->bitems[bank->item_count] = *item;
    bank->item_count++;
    return true;
}

int player_use_item(ship_client_t* src, uint32_t item_id) {
    struct mt19937_state* rng = &src->cur_block->rng;
    iitem_t* weapon = { 0 };
    iitem_t* armor = { 0 };
    iitem_t* mag = { 0 };
    // On PC (and presumably DC), the client sends a 6x29 after this to delete the
    // used item. On GC and later versions, this does not happen, so we should
    // delete the item here.
    bool should_delete_item = (src->version != CLIENT_VERSION_DCV2) && (src->version != CLIENT_VERSION_PC);
    errno_t err = 0;

    psocn_bb_char_t* character = &src->bb_pl->character;

    if (src->mode)
        character = &src->mode_pl->bb;

    iitem_t* iitem = &character->inv.iitems[find_iitem_index(&character->inv, item_id)];

    if (is_common_consumable(primary_identifier(&iitem->data))) { // Monomate, etc.
        // Nothing to do (it should be deleted)
        goto done;
    }
    else if (is_wrapped(&iitem->data)) {
        // Unwrap present
        unwrap(&iitem->data);
        should_delete_item = false;

    }
    else if (iitem->data.datab[0] == ITEM_TYPE_WEAPON) {
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
        }

    }
    else if (iitem->data.datab[0] == ITEM_TYPE_GUARD) {
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
            }
            break;
        }
    }
    /* �Ѿ��޸�ITEMPMT�Ķ�ȡ˳�� ���Ǳ��� ���ڿ��Խ�һ������ ??? */
    else if (iitem->data.datab[0] == ITEM_TYPE_MAG) {
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
        }
    }
    else if (iitem->data.datab[0] == ITEM_TYPE_TOOL) {
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
            mag = &character->inv.iitems[find_equipped_mag(&character->inv)];

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
                    } else {
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
                    } else {
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
                    } else {
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
                    } else {
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
                    } else {
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
                            //Send_Item(client->character.inventory[eq_wep].item.item_id, client);
                            // Ԕ��������װ�䤷�Ƥ��������򥤥�٥�ȥ����K�Ф��ͤ룿
                        }
                        break;
                    }
                }
                break;
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
                new_item.data2l   = 0x0407C878;

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
                new_item.data2l   = 0x00000000;

                new_iitem = add_new_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z);

                subcmd_send_lobby_drop_stack(src, 0xFBFF, NULL, src->cur_area, src->x, src->z, new_iitem->data, 1);

                //DB�� Machine 100% Ruin 100% Hit 50%
                new_item.datal[0] = 0x2C050100;
                new_item.datal[1] = 0x32050000;
                new_item.datal[2] = 0x64046403;
                new_item.data2l   = 0x00000000;

                new_iitem = add_new_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z);

                subcmd_send_lobby_drop_stack(src, 0xFBFF, NULL, src->cur_area, src->x, src->z, new_iitem->data, 1);

                //DB��
                new_item.datal[0] = 0x00260201;
                new_item.datal[1] = 0x00000000;
                new_item.datal[2] = 0x00000000;
                new_item.data2l   = 0x00000000;

                new_iitem = add_new_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z);

                subcmd_send_lobby_drop_stack(src, 0xFBFF, NULL, src->cur_area, src->x, src->z, new_iitem->data, 1);

                pthread_mutex_unlock(&src->mutex);
                break;
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

            size_t det = mt19937_genrand_int32(rng) % sum;

            for (z = 0; z < num_eventitems_bb[iitem->data.datab[2]]; z++) {
                pmt_eventitem_bb_t entry = eventitem_bb[iitem->data.datab[2]][z];
                if (det > entry.probability) {
                    det -= entry.probability;
                }
                else {
                    iitem->data.data2l = 0;
                    iitem->data.datab[0] = entry.item[0];
                    iitem->data.datab[1] = entry.item[1];
                    iitem->data.datab[2] = entry.item[2];
                    iitem->data.datab[3] = 0;
                    iitem->data.datab[4] = 0;
                    iitem->data.datab[5] = 0;
                    should_delete_item = false;

                    subcmd_send_lobby_bb_create_inv_item(src, iitem->data, 1, true);
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
        }
    }
    else {
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

                inv_item->data.datab[0] = combo.result_item[0];
                inv_item->data.datab[1] = combo.result_item[1];
                inv_item->data.datab[2] = combo.result_item[2];
                inv_item->data.datab[3] = 0; // Grind
                inv_item->data.datab[4] = 0; // Flags + special
            }

            __except (crash_handler(GetExceptionInformation())) {
                // ������ִ���쳣�������߼��������ӡ������Ϣ���ṩ�û��Ѻõ���ʾ��
                ERR_LOG("ʹ����Ʒ�ϳɳ��ִ���.");
            }
        }


        if (!combo_applied) {
            ERR_LOG("�������κκϳ�");
        }
    }

done:
    if (should_delete_item) {
        // Allow overdrafting meseta if the client is not BB, since the server isn't
        // informed when meseta is added or removed from the bank.
        remove_iitem(src, iitem->data.item_id, 1, src->version != CLIENT_VERSION_BB);
    }

    fix_client_inv(&character->inv);

    return err;
}

int initialize_cmode_iitem(ship_client_t* dest) {
    size_t x;
    lobby_t* l = dest->cur_lobby;

    psocn_bb_char_t* player = &dest->mode_pl->bb;

    for (x = 0; x < MAX_PLAYER_INV_ITEMS; x++) {
        clear_iitem(&dest->mode_pl->bb.inv.iitems[x]);
    }

    switch (l->episode)
    {
    case GAME_TYPE_EPISODE_1:
        switch (player->dress_data.ch_class) {
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


    return 0;
}

void player_iitem_init(iitem_t* item, const bitem_t* src) {
    item->present = 1;
    item->extension_data1 = 0;
    item->extension_data2 = 0;
    item->flags = 0;
    item->data = src->data;
}

void player_bitem_init(bitem_t* item, const iitem_t* src) {
    item->data = src->data;
    item->amount = (uint16_t)stack_size(&item->data);
    item->show_flags = 1;
}

/* ����������Ʒ���� */
void cleanup_bb_bank(ship_client_t *c, psocn_bank_t* bank, bool comoon_bank) {
    uint32_t item_id = 0x80010000 | (c->client_id << 21);
    if (comoon_bank)
        item_id = 0x80110000 | (c->client_id << 21);

    uint32_t count = LE32(bank->item_count), i;

    for(i = 0; i < count; ++i) {
        bank->bitems[i].data.item_id = LE32(item_id);
        ++item_id;
    }

    /* Clear all the rest of them... */
    for(; i < MAX_PLAYER_BANK_ITEMS; ++i) {
        clear_bitem(&bank->bitems[i]);
        /*memset(&bank->bitems[i], 0, PSOCN_STLENGTH_BITEM);
        bank->bitems[i].data.item_id = EMPTY_STRING;*/
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
    item_t found_item = { 0 };

    i = find_iitem_index(inv, item_id);
#ifdef DEBUG
    DBG_LOG("ʶ���λ %d ������ƷID %d ������ƷID %d", i, inv->iitems[i].data.item_id, item_id);
    print_item_data(&inv->iitems[i].data, c->version);
#endif // DEBUG

    /* ����Ҳ�������Ʒ�����û��Ӵ�������. */
    if (i == -1) {
        ERR_LOG("GC %" PRIu32 " ������Ч�Ķѵ���Ʒ!", gc);
        return -1;
    }

    found_item = inv->iitems[i].data;

    if (found_item.item_id == item_id) {
        found = 1;
        inv_count = inv->item_count;

        switch (found_item.datab[0])
        {
        case ITEM_TYPE_WEAPON:
            if (pmt_lookup_weapon_bb(found_item.datal[0], &tmp_wp)) {
                ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                    gc);
                return -1;
            }

            if (item_check_equip(tmp_wp.equip_flag, equip_flags)) {
                ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                    gc);
                return -2;
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
            switch (found_item.datab[1]) {
            case ITEM_SUBTYPE_FRAME:
                if (pmt_lookup_guard_bb(found_item.datal[0], &tmp_guard)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                        gc);
                    return -3;
                }

                if (target_level < tmp_guard.level_req) {
                    ERR_LOG("GC %" PRIu32 " �ȼ�����, ��Ӧ��װ������Ʒ����!",
                        gc);
                    return -4;
                }

                if (item_check_equip(tmp_guard.equip_flag, equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                        gc);
                    return -5;
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
                if (pmt_lookup_guard_bb(found_item.datal[0], &tmp_guard)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                        gc);
                    return -3;
                }

                if (target_level < tmp_guard.level_req) {
                    ERR_LOG("GC %" PRIu32 " �ȼ�����, ��Ӧ��װ������Ʒ����!",
                        gc);
                    return -4;
                }

                if (item_check_equip(tmp_guard.equip_flag, equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " װ���˲����ڸ�ְҵ����Ʒ����!",
                        gc);
                    return -5;
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
                    return -1;
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

    return found;
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

static iitem_t fix_iitem[30];
static bitem_t fix_bitem[200];

/* ��������Ʒ */
void fix_client_inv(inventory_t* inv) {
    uint8_t i, j = 0;

    memset(&fix_iitem[0], 0, PSOCN_STLENGTH_IITEM * inv->item_count);

    for (i = 0; i < inv->item_count; i++)
        if (inv->iitems[i].present && inv->iitems[i].data.datal[0])
            fix_iitem[j++] = inv->iitems[i];

    inv->item_count = j;

    for (i = 0; i < inv->item_count; i++)
        inv->iitems[i] = fix_iitem[i];
}

void sort_client_inv(inventory_t* inv) {
    size_t i, j, k, l, item_id;

    j = 0x0C;

    memset(&fix_iitem[0], 0, PSOCN_STLENGTH_IITEM * MAX_PLAYER_INV_ITEMS);

    for (l = 0; l < MAX_PLAYER_INV_ITEMS; l++) {
        fix_iitem[l].data.datab[1] = 0xFF;
        fix_iitem[l].data.item_id = EMPTY_STRING;
    }

    l = 0;

    for (i = 0; i < MAX_PLAYER_INV_ITEMS; i++) {
        item_id = inv->iitems[j].data.item_id;
        j += 4;
        if (item_id != EMPTY_STRING) {
            for (k = 0; k < inv->item_count; k++) {
                if ((inv->iitems[k].present) && (inv->iitems[k].data.item_id == item_id)) {
                    fix_iitem[l++] = inv->iitems[k];
                    break;
                }
            }
        }
    }

    for (i = 0; i < MAX_PLAYER_INV_ITEMS; i++)
        inv->iitems[i] = fix_iitem[i];

}

void fix_client_bank(psocn_bank_t* bank) {
    size_t i, j = 0;

    memset(&fix_bitem[0], 0, PSOCN_STLENGTH_BITEM * bank->item_count);

    for (i = 0; i < bank->item_count; i++)
        if (bank->bitems[i].show_flags && bank->bitems[i].amount && bank->bitems[i].data.datal[0])
            fix_bitem[j++] = bank->bitems[i];

    bank->item_count = j;

    for (i = 0; i < bank->item_count; i++)
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

