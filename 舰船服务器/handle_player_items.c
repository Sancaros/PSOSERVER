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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <windows.h>
#include <wincrypt.h>

#include <f_logs.h>
#include <pso_text.h>
#include <SFMT.h>

#include "handle_player_items.h"
#include "subcmd_send.h"

/* We need LE32 down below... so get it from packets.h */
#define PACKETS_H_HEADERS_ONLY
#include "packets.h"
#include "pmtdata.h"
#include "ptdata.h"
#include "rtdata.h"
#include "mag_bb.h"
#include "max_tech_level.h"
#include "shipgate.h"
#include "commands_gm.h"

iitem_t fix_iitem[MAX_PLAYER_INV_ITEMS];
bitem_t fix_bitem[MAX_PLAYER_BANK_ITEMS];
extern bb_max_tech_level_t max_tech_level[MAX_PLAYER_TECHNIQUES];

////////////////////////////////////////////////////////////////////////////////////////////////////
////游戏房间操作

bool check_structs_equal(const void* s1, const void* s2, size_t sz) {
    if (s1 == NULL || s2 == NULL) {
        return false;
    }

    // 使用 memcmp 比较二进制数据
    if (memcmp(s1, s2, sz) != 0) {
        ERR_LOG("不一致 ");
        print_item_data((item_t*)s1, 5);
        print_item_data((item_t*)s2, 5);
        return false;
    }

    return true;
}

/* 生成物品ID */
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

/* 修复玩家背包数据 */
void regenerate_lobby_item_id(lobby_t* l, ship_client_t* c) {
    uint32_t id;
    int i;

    if (c->version == CLIENT_VERSION_BB) {
        inventory_t* inv = get_client_inv(c);

        /* 在新房间中修正玩家背包ID */
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
        /* 在新房间中修正玩家背包ID */
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

/* 新增一件物品至大厅背包中. 调用者在调用这个之前必须持有大厅的互斥锁.
如果大厅的库存中没有新物品的空间,则返回NULL. */
litem_t* add_lobby_litem_locked(lobby_t* l, item_t* item, uint8_t area, float x, float z, bool is_new_item) {
    /* 合理性检查... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    if (item_not_identification_bb(item->datal[0], item->datal[1])) {
        ERR_LOG("0x%08X 是未识别物品", item->datal[0]);
        print_item_data( item, l->version);
        return NULL;
    }

    litem_t* litem = (litem_t*)malloc(sizeof(litem_t));
    if (!litem) {
        ERR_LOG("malloc");
        return NULL;
    }

    memset(litem, 0, sizeof(litem_t));

    /* Copy the item data in. */
    memcpy(&litem->item, item, PSOCN_STLENGTH_ITEM);
    litem->x = x;
    litem->z = z;
    litem->area = area;
    litem->amount = get_item_amount(item, item->datab[5]);

#ifdef DEBUG

    print_iitem_data(&item->d, 0, l->version);

#endif // DEBUG

    /* 如果是新物品,则赋值新ID  玩家丢出的 取出的 购买的物品时 无需再生成新的ID */
    if (is_new_item) {
        litem->item.item_id = LE32(l->item_lobby_id);
        generate_item_id(l, 0xFF);
    }

    TAILQ_INSERT_HEAD(&l->item_queue, litem, qentry);
    return litem;
}

int remove_litem_locked(lobby_t* l, uint32_t item_id, item_t* rv) {

    if (l->version != CLIENT_VERSION_BB)
        return -1;

    clear_inv_item(rv);

    litem_t* litem = TAILQ_FIRST(&l->item_queue);
    while (litem) {
        litem_t* tmp = TAILQ_NEXT(litem, qentry);

        if (litem->item.item_id == item_id) {
            memcpy(rv, &litem->item, PSOCN_STLENGTH_ITEM);
            TAILQ_REMOVE(&l->item_queue, litem, qentry);
            free_safe(litem);
            return 0;
        }

        litem = tmp;
    }

    return 1;
}

size_t find_litem_index(lobby_t* l, item_t* item) {
    size_t index = 0;
    litem_t* litem;

    TAILQ_FOREACH(litem, &l->item_queue, qentry) {
        if (&litem->item == item) {
            return index;
        }

        index++;
    }

    return -1;
}

bool find_mag_and_feed_item(const inventory_t* inv,
    const uint32_t mag_id,
    const uint32_t item_id,
    size_t* mag_item_index,
    size_t* feed_item_index) {

    if (inv->item_count > MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("背包物品数量超出限制 %d", inv->item_count);
        return false;
    }

    for (size_t i = 0; i < inv->item_count; i++) {
        if (inv->iitems[i].data.item_id == mag_id) {
            /* 找到玛古 */
            if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_MAG) &&
                (inv->iitems[i].data.datab[1] <= Mag_Agastya)) {

                *mag_item_index = i;
                for (size_t j = 0; j < inv->item_count; j++) {
                    if (inv->iitems[j].data.item_id == item_id) {
                        /* 找到喂养的物品 */
                        if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_TOOL) &&
                            (inv->iitems[j].data.datab[1] < ITEM_SUBTYPE_TELEPIPE) &&
                            (inv->iitems[j].data.datab[1] != ITEM_SUBTYPE_DISK) &&
                            (inv->iitems[j].data.datab[5] > 0x00)) {

                            *feed_item_index = j;
                            // 找到魔法装备和喂养物品，返回true
                            return true;
                        }
                        break;
                    }
                }
            }
        }
    }

    // 没有找到魔法装备和喂养物品，返回false
    return false;
}


int find_iitem_index(const inventory_t* inv, const uint32_t item_id) {
    int x = 0;

    if(inv->item_count > MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("背包物品数量超出限制 %d", inv->item_count);
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
        ERR_LOG("未从背包中找到ID 0x%08X 物品", item_id);
        return -2;
    }

    return -3;
}

int find_titem_index(const trade_inv_t* trade, const uint32_t item_id) {
    int x = 0;
    int y = trade->trade_item_count;

    if (y > 0x20) {
        ERR_LOG("交易物品数量超出限制 %d", y);
        return -1;
    }

    for (x = 0; x < y; x++) {
        if (trade->items[x].item_id != item_id)
            continue;

        return x;
    }

#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    if (x == y) {
        ERR_LOG("未从交易背包中找到ID 0x%08X 物品", item_id);
        return -2;
    }

    return -3;
}

int check_titem_id(const trade_inv_t* trade, const uint32_t item_id) {
    int x = 0;
    int y = trade->trade_item_count;

    if (y > 0x20) {
        ERR_LOG("交易物品数量超出限制 %d", y);
        return -1;
    }

    for (x = 0; x < y; x++) {
        if (trade->items[x].item_id != item_id)
            continue;

        return trade->items[x].item_id;
    }

#ifdef DEBUG

    for (x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, 5);
    }

#endif // DEBUG

    if (x == y) {
        ERR_LOG("未从交易背包中找到ID 0x%08X 物品", item_id);
        return -2;
    }

    return -3;
}

int find_bitem_index(const psocn_bank_t* bank, const uint32_t item_id) {
    int x = -1;

    if (bank->item_count > MAX_PLAYER_BANK_ITEMS) {
        ERR_LOG("银行物品数量超出限制 %d", bank->item_count);
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
        ERR_LOG("未从银行中找到ID 0x%08X 物品", item_id);
        return -1;
    }

    return -2;
}

/* 仅用于PD PC 等独立堆叠物品 不可用于单独物品 */
size_t find_iitem_stack_item_id(const inventory_t* inv, const item_t* item) {
    uint32_t pid = primary_identifier(item);
    size_t x = 0;

    if (inv->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("背包物品数量超出限制 %d", inv->item_count);
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
        ERR_LOG("未从背包中找到ID 0x%08X 物品", pid);
        return 0;
    }

    return 0;
}

/* 仅用于PD PC 等独立堆叠物品 不可用于单独物品 */
size_t find_iitem_code_stack_item_id(const inventory_t* inv, const uint32_t code) {
    uint32_t pid = primary_code_identifier(code);
    size_t x = 0;

    if (inv->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("背包物品数量超出限制 %d", inv->item_count);
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
        ERR_LOG("未从背包中找到ID 0x%08X 物品", code);
        return 0;
    }

    return 0;
}

size_t find_iitem_pid(const inventory_t* inv, const item_t* item) {
    uint32_t pid = primary_identifier(item);
    int x = 0;

    if (inv->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("背包物品数量超出限制 %d", inv->item_count);
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
        ERR_LOG("未从背包中找到ID 0x%08X 物品", pid);
        return -1;
    }

    return -2;
}

int find_iitem_pid_index(const inventory_t* inv, const item_t* item) {
    uint32_t pid = primary_identifier(item);
    int x = 0;

    if (inv->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("背包物品数量超出限制 %d", inv->item_count);
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
        ERR_LOG("未从背包中找到ID 0x%08X 物品", pid);
        return -1;
    }

    return -2;
}

int find_bitem_pid_index(const psocn_bank_t* bank, const bitem_t* bitem) {
    uint32_t pid = primary_identifier(&bitem->data);
    int x = 0;

    if (bank->item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("背包物品数量超出限制 %d", bank->item_count);
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
        ERR_LOG("未从背包中找到ID 0x%08X 物品", pid);
        return -1;
    }

    return -2;
}

/* 获取背包中目标物品所在槽位 */
int find_equipped_weapon(const inventory_t* inv){
    int ret = -1;
    for (int y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & EQUIP_FLAGS)) {
            continue;
        }
        if (inv->iitems[y].data.datab[0] != ITEM_TYPE_WEAPON) {
            continue;
        }
        if (ret < 0) {
            ret = y;
        }
        else {
            ERR_LOG("找到装备多件武器");
        }
    }
    if (ret < 0) {
        ERR_LOG("未从背包中找已装备的武器");
    }
    return ret;
}

/* 获取背包中目标物品所在槽位 */
int find_equipped_armor(const inventory_t* inv) {
    int ret = -1;
    for (int y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & EQUIP_FLAGS)) {
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
            ERR_LOG("找到装备多件盔甲");
        }
    }
    if (ret < 0) {
        ERR_LOG("未从背包中找已装备的盔甲");
    }
    return ret;
}

/* 获取背包中目标物品所在槽位 */
int find_equipped_mag(const inventory_t* inv) {
    int ret = -1;
    for (int y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & EQUIP_FLAGS)) {
            continue;
        }
        if (inv->iitems[y].data.datab[0] != ITEM_TYPE_MAG) {
            continue;
        }
        if (ret < 0) {
            ret = y;
        }
        else {
            ERR_LOG("找到装备多件玛古");
            ret = -2;
        }
    }
#ifdef DEBUG

    DBG_LOG("ret %u", ret);
#endif // DEBUG

    return ret;
}

/* 用于移除物品时 移除物品原有的装备标志 */
void remove_iitem_equiped_flags(inventory_t* inv, const item_t* item) {
    size_t i = find_iitem_index(inv, item->item_id);
    /* 如果找不到该物品，则将用户从船上推下. */
    if (i < 0) {
        ERR_LOG("卸除无效装备物品! 错误码 %d", i);
        return;
    }

    if (inv->iitems[i].flags & EQUIP_FLAGS) {
        switch (item->datab[0]) {
        case ITEM_TYPE_WEAPON:
            inv->iitems[i].flags &= UNEQUIP_FLAGS;
            break;

        case ITEM_TYPE_GUARD:
            switch (item->datab[1]) {
            case ITEM_SUBTYPE_FRAME:
                for (size_t j = 0; j < inv->item_count; j++) {
                    if (inv->iitems[i].data.datab[0] == ITEM_TYPE_GUARD &&
                        inv->iitems[i].data.datab[1] == ITEM_SUBTYPE_FRAME) {
                        if (inv->iitems[j].data.datab[0] == ITEM_TYPE_GUARD &&
                            inv->iitems[j].data.datab[1] == ITEM_SUBTYPE_UNIT) {
                            inv->iitems[j].flags &= UNEQUIP_FLAGS;
                        }

                    }
                }
                inv->iitems[i].flags &= UNEQUIP_FLAGS;
                break;

            case ITEM_SUBTYPE_BARRIER:
                inv->iitems[i].flags &= UNEQUIP_FLAGS;
                break;

            case ITEM_SUBTYPE_UNIT:
                inv->iitems[i].flags &= UNEQUIP_FLAGS;
                break;

            }
            break;

        case ITEM_TYPE_MAG:
            inv->iitems[i].flags &= UNEQUIP_FLAGS;
            break;

        }
    }
}

void bswap_data2_if_mag(item_t* item) {
    if (item->datab[0] == ITEM_TYPE_MAG) {
        item->data2l = bswap32(item->data2l);
    }
}

int add_character_meseta(psocn_bb_char_t* character, uint32_t amount) {
    uint32_t max_meseta = MAX_PLAYER_MESETA;
#ifdef DEBUG

    if (character->disp.meseta + amount > max_meseta) {
        ERR_LOG("玩家美赛塔已满");
        return -1;
    }

#endif // DEBUG

    character->disp.meseta = min(character->disp.meseta + amount, max_meseta);

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

/* 移除背包物品操作 */
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

item_t remove_invitem(ship_client_t* src, uint32_t item_id, uint32_t amount, 
    bool allow_meseta_overdraft) {
    item_t ret = { 0 };
    psocn_bb_char_t* character = get_client_char_bb(src);

    if (item_id == ITEM_ID_MESETA) {
        if (remove_character_meseta(character, amount, allow_meseta_overdraft)) {
            ERR_LOG("玩家拥有的美赛塔不足 %d < %d", character->disp.meseta, amount);
            return ret;
        }
        ret.datab[0] = ITEM_TYPE_MESETA;
        ret.data2l = amount;
        return ret;
    }

    int index = find_iitem_index(&character->inv, item_id);
    if (index < 0) {
        ERR_LOG("移除物品发生错误 错误码 %d", index);
        return ret;
    }
    item_t* item = &character->inv.iitems[index].data;

    if (amount && is_stackable(item) && (amount < item->datab[5])) {
        ret = *item;
        ret.datab[5] = amount;
        ret.item_id = EMPTY_STRING;
        item->datab[5] -= amount;
        return ret;
    }

    remove_iitem_equiped_flags(&character->inv, item);

    ret = *item;
    character->inv.item_count--;
    for (int x = index; x < character->inv.item_count; x++) {
        character->inv.iitems[x] = character->inv.iitems[x + 1];
    }
    clear_iitem(&character->inv.iitems[character->inv.item_count]);
    return ret;
}

item_t remove_titem(trade_inv_t* trade, uint32_t item_id, uint32_t amount) {
    item_t ret = { 0 };

    if (item_id == ITEM_ID_MESETA) {
        trade->meseta = max(trade->meseta - amount, 0);
        ret.datab[0] = ITEM_TYPE_MESETA;
        ret.data2l = amount;
        return ret;
    }

    int index = find_titem_index(trade, item_id);
    if (index < 0) {
        ERR_LOG("移除物品发生错误 错误码 %d", index);
        return ret;
    }

    item_t* trade_item = &trade->items[index];

    if (amount && is_stackable(trade_item) && (amount < trade_item->datab[5])) {
        ret = *trade_item;
        ret.datab[5] = amount;
        ret.item_id = EMPTY_STRING;
        trade_item->datab[5] -= amount;
        return ret;
    }

    // If we get here, then it's not meseta, and either it's not a combine item or
    // we're removing the entire stack. Delete the item from the inventory slot
    // and return the deleted item.
    //memcpy(&ret, trade_item, sizeof(iitem_t));
    ret = *trade_item;
    trade->trade_item_count--;
    for (size_t x = index; x < trade->trade_item_count; x++) {
        trade->items[x] = trade->items[x + 1];
    }
    clear_inv_item(&trade->items[trade->trade_item_count]);
    return ret;
}

bitem_t remove_bitem(ship_client_t* src, uint32_t item_id, uint16_t bitem_index, uint32_t amount) {
    bitem_t ret = { 0 };
    psocn_bank_t* bank = get_client_bank_bb(src);
    
    // 检查是否超出美赛塔数量
    if (item_id == ITEM_ID_MESETA) {
        if (amount > bank->meseta) {
            ERR_LOG("%s 移除的美赛塔超出所拥有的",
                get_player_describe(src));
            return ret;
        }
        ret.data.datab[0] = ITEM_TYPE_MESETA;
        ret.data.data2l = amount;
        bank->meseta -= amount;
        return ret;
    }

    //// 查找银行物品的索引
    //size_t index = find_bitem_index(bank, item_id);

    if (bitem_index == bank->item_count) {
        ERR_LOG("%s 银行物品索引超出限制",
            get_player_describe(src));
        return ret;
    }

    bitem_t* bank_item = &bank->bitems[bitem_index];

    // 检查是否超出物品可堆叠的数量
    if (amount && is_stackable(&bank_item->data) &&
        (amount < bank_item->data.datab[5])) {
        ret = *bank_item;
        ret.data.datab[5] = amount;
        ret.amount = amount;
        bank_item->data.datab[5] -= amount;
        bank_item->amount -= amount;
        return ret;
    }

    ret = *bank_item;
    //memcpy(&ret, bank_item, sizeof(bitem_t));
    // 移除银行物品
    bank->item_count--;
    for (size_t x = bitem_index; x < bank->item_count; x++) {
        bank->bitems[x] = bank->bitems[x + 1];
    }
    clear_bitem(&bank->bitems[bank->item_count]);
    return ret;
}

/* 已支持非堆叠物品 新增物品ID */
bool add_invitem(ship_client_t* src, const item_t item) {
    lobby_t* l = src->cur_lobby;
    uint32_t pid = primary_identifier(&item);
    psocn_bb_char_t* character = get_client_char_bb(src);

    // 检查是否为meseta，如果是，则修改统计数据中的meseta值
    if (pid == MESETA_IDENTIFIER) {
        add_character_meseta(character, item.data2l);
        return true;
    }

    // 处理可合并的物品
    size_t combine_max = max_stack_size(&item);
    if (combine_max > 1) {
        // 如果玩家库存中已经存在相同物品的堆叠，获取该物品的索引
        size_t y;
        for (y = 0; y < character->inv.item_count; y++) {
            if (primary_identifier(&character->inv.iitems[y].data) == pid) {
                break;
            }
        }

        // 如果存在堆叠，则将数量相加，并限制最大堆叠数量
        if (y < character->inv.item_count) {
            character->inv.iitems[y].data.datab[5] += item.datab[5];
            if (character->inv.iitems[y].data.datab[5] > (uint8_t)combine_max) {
                character->inv.iitems[y].data.datab[5] = (uint8_t)combine_max;
            }
            return true;
        }
    }

    // 如果执行到这里，既不是meseta也不是可合并物品，因此需要放入一个空的库存槽位
    if (character->inv.item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("%s 背包物品数量超出最大值,当前 %d 个物品",
            get_player_describe(src), character->inv.item_count);
        return false;
    }
    character->inv.iitems[character->inv.item_count].data = item;
    character->inv.iitems[character->inv.item_count].present = LE16(0x0001);
    character->inv.iitems[character->inv.item_count].flags = 0;
    character->inv.item_count++;
    return true;
}

bool add_titem(trade_inv_t* trade, const item_t item) {
    uint32_t pid = primary_identifier(&item);

    // 检查是否为meseta，如果是，则修改统计数据中的meseta值
    if (pid == MESETA_IDENTIFIER) {
        trade->meseta = min(trade->meseta + item.data2l, 999999);
        return true;
    }

    // 处理可合并的物品
    size_t combine_max = max_stack_size(&item);
    if (combine_max > 1) {
        // 如果玩家库存中已经存在相同物品的堆叠，获取该物品的索引
        size_t y;
        for (y = 0; y < trade->trade_item_count; y++) {
            if (primary_identifier(&trade->items[y]) == pid) {
                break;
            }
        }

        // 如果存在堆叠，则将数量相加，并限制最大堆叠数量
        if (y < trade->trade_item_count) {
            trade->items[y].datab[5] += item.datab[5];
            if (trade->items[y].datab[5] > (uint8_t)combine_max) {
                trade->items[y].datab[5] = (uint8_t)combine_max;
            }
            return true;
        }
    }

    // 如果执行到这里，既不是meseta也不是可合并物品，因此需要放入一个空的库存槽位
    if (trade->trade_item_count >= 0x20) {
        ERR_LOG("交易物品数量超出最大值,当前 %d 个物品", trade->trade_item_count);
        return false;
    }
    trade->items[trade->trade_item_count] = item;
    trade->item_ids[trade->trade_item_count] = item.item_id;
    trade->trade_item_count++;
    return true;
}

bool add_bitem(ship_client_t* src, const bitem_t bitem) {
    uint32_t pid = primary_identifier(&bitem.data);
    psocn_bank_t* bank = get_client_bank_bb(src);
    
    if (pid == MESETA_IDENTIFIER) {
        bank->meseta += bitem.data.data2l;
        if (bank->meseta > 999999) {
            bank->meseta = 999999;
        }
        return true;
    }
    
    size_t combine_max = max_stack_size(&bitem.data);
    if (combine_max > 1) {
        size_t y;
        for (y = 0; y < bank->item_count; y++) {
            if (primary_identifier(&bank->bitems[y].data) == pid) {
                break;
            }
        }

        if (y < bank->item_count) {
            bank->bitems[y].data.datab[5] += bitem.data.datab[5];
            if (bank->bitems[y].data.datab[5] > (uint8_t)combine_max) {
                bank->bitems[y].data.datab[5] = (uint8_t)combine_max;
            }
            bank->bitems[y].amount = bank->bitems[y].data.datab[5];
            return true;
        }
    }

    if (bank->item_count >= MAX_PLAYER_BANK_ITEMS) {
        ERR_LOG("%s 银行物品数量超出最大值",
            get_player_describe(src));
        return false;
    }
    bank->bitems[bank->item_count] = bitem;
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

    ERR_LOG("无效物品数据 0x%02X", item->datab[0]);
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
        ERR_LOG("无效 unwrap 物品类型");
    }
}

int player_use_item(ship_client_t* src, uint32_t item_id) {
    lobby_t* l = src->cur_lobby;
    sfmt_t* rng = &src->cur_block->sfmt_rng;
    iitem_t* weapon = { 0 };
    iitem_t* armor = { 0 };
    iitem_t* mag = { 0 };
    // On PC (and presumably DC), the client sends a 6x29 after this to delete the
    // used item. On GC and later versions, this does not happen, so we should
    // delete the item here.
    bool should_delete_item = (src->version != CLIENT_VERSION_DCV2) && (src->version != CLIENT_VERSION_PC);
    errno_t err = 0;
    
    inventory_t* inv = get_client_inv(src);

    int index = find_iitem_index(inv, item_id);
    if (index < 0) {
        err = index;
        ERR_LOG("%s 使用物品发生错误 错误码 %d", get_player_describe(src), index);
        return err;
    }
    item_t* item = &inv->iitems[index].data;

    LOBBY_USE_ITEM_LOG(src, item_id, src->cur_area, item);

    if (is_common_consumable(primary_identifier(item))) { // Monomate, etc.
        // Nothing to do (it should be deleted)
        goto done;
    }
    else if (is_wrapped(item)) {
        // Unwrap present
        unwrap(item);
        should_delete_item = false;
        goto done;
    }

    switch (item->datab[0])
    {
    case ITEM_TYPE_WEAPON:
        switch (item->datab[1]) {
        case 0x33:
            // Unseal Sealed J-Sword => Tsumikiri J-Sword
            item->datab[1] = 0x32;
            should_delete_item = false;
            break;

        case 0xAB:
            // Unseal Lame d'Argent => Excalibur
            item->datab[1] = 0xAC;
            should_delete_item = false;
            break;

        default:
            goto combintion_other;
        }
        break;

    case ITEM_TYPE_GUARD:
        switch (item->datab[1]) {
        case ITEM_SUBTYPE_UNIT:
            switch (item->datab[2]) {
            case 0x4D:
                // Unseal Limiter => Adept
                item->datab[2] = 0x4E;
                should_delete_item = false;
                break;

            case 0x4F:
                // Unseal Swordsman Lore => Proof of Sword-Saint
                item->datab[2] = 0x50;
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
        switch (item->datab[1]) {
        case 0x2B:
            weapon = &inv->iitems[find_equipped_weapon(inv)];
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
            armor = &inv->iitems[find_equipped_armor(inv)];
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
        switch (item->datab[1]) {
        case ITEM_SUBTYPE_TELEPIPE:
            if (src->cur_area == 0) {
                ERR_LOG("%s 尝试在先驱者2号释放传送门", get_player_describe(src));
                return -10;
            }

            break;

        case ITEM_SUBTYPE_DISK: // Technique disk
            if (item->datab[2] + 1 > get_bb_max_tech_level(src->bb_pl->character.dress_data.ch_class, item->datab[4])) {
                ERR_LOG("%s 法术科技光碟等级高于职业可用等级", get_player_describe(src));
                return -1;
            }

            //DBG_LOG("类型 %d 等级 %d", item->datab[4], item->datab[2]);

            set_technique_level(get_player_v1_tech(src), inv, item->datab[4], item->datab[2]);
            break;

        case ITEM_SUBTYPE_GRINDER: // Grinder
            if (item->datab[2] > 2) {
                ERR_LOG("%s 无效打磨物品值", get_player_describe(src));
                return -2;
            }
            weapon = &inv->iitems[find_equipped_weapon(inv)];
            pmt_weapon_bb_t weapon_def = { 0 };
            if (pmt_lookup_weapon_bb(weapon->data.datal[0], &weapon_def)) {
                ERR_LOG("%s 装备了不存在的物品数据!", get_player_describe(src));
                return -3;
            }

            weapon->data.datab[3] += (item->datab[2] + 1);

            if (weapon->data.datab[3] > weapon_def.max_grind)
                weapon->data.datab[3] = weapon_def.max_grind;
            break;

        case ITEM_SUBTYPE_MATERIAL:
            switch (item->datab[2]) {
            case ITEM_SUBTYPE_MATERIAL_POWER: // Power Material
                set_material_usage(inv, MATERIAL_POWER, get_material_usage(inv, MATERIAL_POWER) + 1);
                get_player_stats(src)->atp += 2;
                break;

            case ITEM_SUBTYPE_MATERIAL_MIND: // Mind Material
                set_material_usage(inv, MATERIAL_MIND, get_material_usage(inv, MATERIAL_MIND) + 1);
                get_player_stats(src)->mst += 2;
                break;

            case ITEM_SUBTYPE_MATERIAL_EVADE: // Evade Material
                set_material_usage(inv, MATERIAL_EVADE, get_material_usage(inv, MATERIAL_EVADE) + 1);
                get_player_stats(src)->evp += 2;
                break;

            case ITEM_SUBTYPE_MATERIAL_HP: // HP Material
                set_material_usage(inv, MATERIAL_HP, get_material_usage(inv, MATERIAL_HP) + 1);
                break;

            case ITEM_SUBTYPE_MATERIAL_TP: // TP Material
                set_material_usage(inv, MATERIAL_TP, get_material_usage(inv, MATERIAL_TP) + 1);
                break;

            case ITEM_SUBTYPE_MATERIAL_DEF: // Def Material
                set_material_usage(inv, MATERIAL_DEF, get_material_usage(inv, MATERIAL_DEF) + 1);
                get_player_stats(src)->dfp += 2;
                break;

            case ITEM_SUBTYPE_MATERIAL_LUCK: // Luck Material
                set_material_usage(inv, MATERIAL_LUCK, get_material_usage(inv, MATERIAL_LUCK) + 1);
                get_player_stats(src)->lck += 2;
                break;

            default:
                ERR_LOG("%s 未知药物 0x%08X", get_player_describe(src), item->datal[0]);
                return -5;
            }
            break;

        case ITEM_SUBTYPE_MAG_CELL1:
            int mag_index = find_equipped_mag(inv);
            if (mag_index == -1) {
                ERR_LOG("%s 没有装备玛古,玛古细胞 0x%08X", get_player_describe(src), item->datal[0]);
                break;
            }
            mag = &inv->iitems[mag_index];

            switch (item->datab[2]) {
            case 0x00:
                // Cell of MAG 502
                mag->data.datab[1] = (get_player_section(src) & SID_Greennill) ? 0x1D : 0x21;
                break;

            case 0x01:
                // Cell of MAG 213
                mag->data.datab[1] = (get_player_section(src) & SID_Greennill) ? 0x27 : 0x22;
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
                ERR_LOG("%s 未知玛古细胞 0x%08X", get_player_describe(src), item->datal[0]);
                return -5;
            }
            break;

        case ITEM_SUBTYPE_ADD_SLOT:
            armor = &inv->iitems[find_equipped_armor(inv)];

            if (armor->data.datab[5] >= 4) {
                ERR_LOG("%s 物品已达最大插槽数量", get_player_describe(src));
                return -6;
            }
            armor->data.datab[5]++;
            break;

        case ITEM_SUBTYPE_SERVER_ITEM1:
            size_t eq_wep = find_equipped_weapon(inv);
            weapon = &inv->iitems[eq_wep];

            //アイテムIDの5, 6文字目が1, 2のアイテムの場合（0x0312 * *のとき）
            switch (item->datab[2]) {//スイッチ文。「使用」するアイテムIDの7, 8文字目をスイッチに使う。
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
                DBG_LOG("%s Aluminum Weapons Badge", get_player_describe(src));
                break;

            case 0x06:
                DBG_LOG("%s Leather Weapons Badge", get_player_describe(src));
                break;

            case 0x07: // weapons bone
                //アイテムIDの7,8文字目が0,7のとき。（0x031207のとき　つまりウェポンズバッジ骨のとき）
                if (eq_wep != -1) {    //詳細不明、武器装備時？
                    switch (weapon->data.datab[1]) { //スイッチにキャラクターの装備している武器のアイテムIDの3,4文字目をスイッチに使う

                    case 0x22:
                        //キャラクターの装備している武器のアイテムIDの3,4文字目が2,2のとき（0x002200のとき　つまりカジューシース）
                        if (weapon->data.datab[3] == 0x09)
                            //もしキャラクターの装備している武器のアイテムIDの7,8文字目が0,9だったら（つまり強化値が + 9だったら）
                        {
                            weapon->data.datab[2] = 0x01; // melcrius
                            //キャラクターの装備している武器のアイテムIDの5,6文字目を0,1に変更する
                            //（つまりカジューシース + 9（0x00220009）→メルクリウスロッド + 9（0x00220109）にする）
                            weapon->data.datab[3] = 0x00; // Not grinded
                            //強化値を０にする（つまりメルクリウスロッド + 9（0x00220109）→メルクリウスロッド（0x00220100）にする）
                            weapon->data.datab[4] = 0x00;
                            //Send_Item(inv->iitems[eq_wep].data.item_id, client);
                            // 詳細不明。装備している武器をインベントリの最終行に送る？
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
            litem_t* new_litem;

            switch (item->datab[2]) {
                /* 情人节巧克力 */
                /* 暂未解析该生成什么物品 */
            case ITEM_SUBTYPE_SERVER_ITEM2_CHOCOLATE:
            case ITEM_SUBTYPE_SERVER_ITEM2_CANDY:
            case ITEM_SUBTYPE_SERVER_ITEM2_CAKE:
                break;

                /* 银徽章 增加 100000 经验 */
            case ITEM_SUBTYPE_SERVER_ITEM2_W_SILVER:
                //Add Exp
                if (get_player_level(src) + 1 < 200) {
                    client_give_exp(src, 100000);
                    send_txt(src, "%s", __(src, "\tE\tC6增加经验 100000 点"));
                }
                break;

                /* 金徽章 获得一个精神玛古 露可敏  */
            case ITEM_SUBTYPE_SERVER_ITEM2_W_GOLD:

                new_item.datal[0] = 0xA3963C02;
                new_item.datal[1] = 0;
                new_item.datal[2] = 0x3A980000;
                new_item.data2l = 0x0407C878;

                new_litem = add_lobby_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z, true);
                if (!new_litem) {
                    /* *Gulp* The lobby is probably toast... At least make sure this user is
                       still (mostly) safe... */
                    ERR_LOG("%s 无法将物品新增游戏房间背包!", get_player_describe(src));
                    return -1;
                }

                subcmd_send_lobby_drop_stack_bb(src, 0xFBFF, NULL, new_litem);

                send_txt(src, "%s", __(src, "\tE\tC6增加经验 100000 点"));

                break;

                /* 水晶徽章 DB套装 DB剑 铠 盾 */
            case ITEM_SUBTYPE_SERVER_ITEM2_W_CRYSTAL:

                //DB鎧 スロット4
                new_item.datal[0] = 0x00280101;
                new_item.datal[1] = 0x00000400;
                new_item.datal[2] = 0x00000000;
                new_item.data2l = 0x00000000;

                new_litem = add_lobby_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z, true);
                if (!new_litem) {
                    /* *Gulp* The lobby is probably toast... At least make sure this user is
                       still (mostly) safe... */
                    ERR_LOG("%s 无法将物品新增游戏房间背包!", get_player_describe(src));
                    return -1;
                }

                subcmd_send_lobby_drop_stack_bb(src, 0xFBFF, NULL, new_litem);

                //DB剣 Machine 100% Ruin 100% Hit 50%
                new_item.datal[0] = 0x2C050100;
                new_item.datal[1] = 0x32050000;
                new_item.datal[2] = 0x64046403;
                new_item.data2l = 0x00000000;

                new_litem = add_lobby_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z, true);
                if (!new_litem) {
                    /* *Gulp* The lobby is probably toast... At least make sure this user is
                       still (mostly) safe... */
                    ERR_LOG("%s 无法将物品新增游戏房间背包!", get_player_describe(src));
                    return -1;
                }

                subcmd_send_lobby_drop_stack_bb(src, 0xFBFF, NULL, new_litem);

                //DB盾
                new_item.datal[0] = 0x00260201;
                new_item.datal[1] = 0x00000000;
                new_item.datal[2] = 0x00000000;
                new_item.data2l = 0x00000000;

                new_litem = add_lobby_litem_locked(src->cur_lobby, &new_item, src->cur_area, src->x, src->z, true);
                if (!new_litem) {
                    /* *Gulp* The lobby is probably toast... At least make sure this user is
                       still (mostly) safe... */
                    ERR_LOG("%s 无法将物品新增游戏房间背包!", get_player_describe(src));
                    return -1;
                }

                subcmd_send_lobby_drop_stack_bb(src, 0xFBFF, NULL, new_litem);

                break;

                /* 暂未解析该生成什么物品 */
            case ITEM_SUBTYPE_SERVER_ITEM2_W_STEEL:
            case ITEM_SUBTYPE_SERVER_ITEM2_W_ALUMINUM:
            case ITEM_SUBTYPE_SERVER_ITEM2_W_LEATHER:
            case ITEM_SUBTYPE_SERVER_ITEM2_W_BONE:
            case ITEM_SUBTYPE_SERVER_ITEM2_BOUQUET:
            case ITEM_SUBTYPE_SERVER_ITEM2_DECOCTION:
                break;

            default:
                goto combintion_other;
            }
            break;

        case ITEM_SUBTYPE_PRESENT_EVENT:
            // Present, etc. - use unwrap_table + probabilities therein
            size_t sum = 0, z = 0;
            pmt_eventitem_bb_t entry;

            for (z = 0; z < get_num_eventitems_bb(item->datab[2]); z++) {
                if (pmt_lookup_eventitem_bb(item->datal[0], z, &entry)) {
                    DBG_LOG("%s 使用圣诞礼物 0x%08X 出错", get_player_describe(src), item->datal[0]);
                    break;
                }

                sum += entry.probability;
            }

            if (sum == 0) {
                ERR_LOG("%s 节日事件没有可用的礼包结果", get_player_describe(src));
                return 0;
            }

            size_t det = sfmt_genrand_uint32(rng) % sum;

            for (z = 0; z < get_num_eventitems_bb(item->datab[2]); z++) {
                if (pmt_lookup_eventitem_bb(item->datal[0], z, &entry)) {
                    DBG_LOG("%s 使用圣诞礼物 0x%08X 出错", get_player_describe(src), item->datal[0]);
                    break;
                }
                if (det > entry.probability) {
                    det -= entry.probability;
                    continue;
                }

                item_t event_item = { 0 };
                clear_inv_item(&event_item);
                event_item.datab[0] = entry.item[0];
                event_item.datab[1] = entry.item[1];
                event_item.datab[2] = entry.item[2];
                event_item.datab[5] = get_item_amount(&event_item, 1);
                event_item.item_id = generate_item_id(l, src->client_id);
                should_delete_item = true;

                /* TODO 出现的武器 装甲 增加随机属性 */
                switch (item->datab[2]) {
                case ITEM_SUBTYPE_PRESENT_EVENT_CHRISTMAS:
                    DBG_LOG("%s 使用圣诞礼物", get_player_describe(src));
                    break;

                case ITEM_SUBTYPE_PRESENT_EVENT_EASTER:
                    DBG_LOG("%s 使用复活节礼物", get_player_describe(src));
                    break;

                case ITEM_SUBTYPE_PRESENT_EVENT_LANTERN:
                    DBG_LOG("%s 使用万圣节礼物", get_player_describe(src));
                    break;
                }

                if (!add_invitem(src, event_item)) {
                    ERR_LOG("%s 背包空间不足, 无法获得物品!", get_player_describe(src));
                    return -3;
                }

                subcmd_send_lobby_bb_create_inv_item(src, event_item, stack_size(&event_item), true);
                break;
            }

            break;

        case ITEM_SUBTYPE_DISK_MUSIC:
            should_delete_item = true;
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

        for (size_t z = 0; z < inv->item_count; z++) {
            iitem_t* inv_item = &inv->iitems[z];
            if (!(inv_item->flags & EQUIP_FLAGS)) {
                continue;
            }

            __try {
                if (err = pmt_lookup_itemcombination_bb(item->datal[0], inv_item->data.datal[0], &combo)) {
#ifdef DEBUG
                    ERR_LOG("%s pmt_lookup_itemcombination_bb 不存在数据! 错误码 %d", get_char_describe(src), err);
#endif // DEBUG
                    continue;
                }

                if (combo.char_class != 0xFF && combo.char_class != get_player_class(src)) {
                    ERR_LOG("%s 物品合成需要特定的玩家职业", get_player_describe(src));
                    ERR_LOG("combo.class %d player %d", combo.char_class, get_player_class(src));
                }
                if (combo.mag_level != 0xFF) {
                    if (inv_item->data.datab[0] != ITEM_TYPE_MAG && find_equipped_mag(inv) == -1) {
                        ERR_LOG("%s 物品合成适用于mag级别要求,但装备的物品不是mag", get_player_describe(src));
                        ERR_LOG("datab[0] 0x%02X", inv_item->data.datab[0]);
                        return -1;
                    }
                    if (compute_mag_level(&inv_item->data) < combo.mag_level) {
                        ERR_LOG("%s 物品合成适用于mag等级要求,但装备的mag等级过低", get_player_describe(src));
                        return -2;
                    }
                }
                if (combo.grind != 0xFF) {
                    if (inv_item->data.datab[0] != ITEM_TYPE_WEAPON && find_equipped_weapon(inv) == -1) {
                        ERR_LOG("%s 物品合成适用于研磨要求,但装备的物品不是武器", get_player_describe(src));
                        return -3;
                    }
                    if (inv_item->data.datab[3] < combo.grind) {
                        ERR_LOG("%s 物品合成适用于研磨要求,但装备的武器研磨过低", get_player_describe(src));
                        return -4;
                    }
                }
                if (combo.level != 0xFF && get_player_level(src) + 1 < combo.level) {
#ifdef DEBUG
                    ERR_LOG("%s 物品合成适用于等级要求,但玩家等级过低", get_player_describe(src));
#endif // DEBUG
                    return -5;
                }
                // If we get here, then the combo applies
#ifdef DEBUG
                if (combo_applied) {
                    DBG_LOG("%s multiple combinations apply", get_char_describe(src));
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
                // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。
                CRASH_LOG("%s 使用物品合成出现错误.", get_player_describe(src));
            }
        }


        if (!combo_applied) {
            ERR_LOG("%s 不适用任何合成", get_player_describe(src));
            print_item_data(item, src->version);
        }
        break;
    }

done:
    if (should_delete_item) {
        // Allow overdrafting meseta if the client is not BB, since the server isn't
        // informed when meseta is added or removed from the bank.
        item_t delete_item = remove_invitem(src, item->item_id, 1, src->version != CLIENT_VERSION_BB);
        if (item_not_identification_bb(delete_item.datal[0], delete_item.datal[1])) {
            ERR_LOG("%s 物品 ID 0x%08X 已不存在", get_player_describe(src), item->item_id);
        }
    }

    return err;
}

//int player_tekker_item(ship_client_t* src, sfmt_t* rng, item_t* item) {
//    char percent_mod = 0;
//    uint8_t attrib = item->datab[4];
//    int8_t tmp_value = 0;
//
//    if (item->datab[0] != ITEM_TYPE_WEAPON) {
//        ERR_LOG("%s 尝试鉴定非武器物品 %s", get_player_describe(src), get_item_describe(item, src->version));
//        return -1;
//    }
//
//    // 技能属性提取和随机数处理
//    if (attrib < ITEM_TYPE_WEAPON_SPECIAL_NUM) {
//        item->datab[4] = tekker_attributes[(attrib * 3) + 1];
//        if ((sfmt_genrand_uint32(rng) % 100) > 70)
//            item->datab[4] += sfmt_genrand_uint32(rng) % ((tekker_attributes[(attrib * 3) + 2] - tekker_attributes[(attrib * 3) + 1]) + 1);
//    }
//    else if ((sfmt_genrand_uint32(rng) % 5 == 1)) {
//        item->datab[4] = sfmt_genrand_uint32(rng) % 10;
//    }else
//        item->datab[4] = 0;
//
//    // 各属性值修正处理
//    static const uint8_t delta_table[5] = { -10, -5, 0, 5, 10 };
//    for (size_t x = 6; x < 0x0B; x += 2) {
//        // 百分比修正处理
//        uint32_t mt_index = sfmt_genrand_uint32(rng) % 5;
//
//        tmp_value = delta_table[mt_index];
//
//        if (tmp_value > 10)
//            tmp_value = 10;
//
//        if (tmp_value < -10)
//            tmp_value = -10;
//
//        //percent_mod += tmp_value;
//
//        //if (mt_index > 5)
//        //    percent_mod += delta_table[mt_index];
//        //else
//        //    percent_mod -= delta_table[mt_index];
//
//        if (!(item->datab[x] & 128) && (item->datab[x + 1] > 0))
//            (char)item->datab[x + 1] += tmp_value;
//    }
//
//    return 0;
//}

bool check_tekke_favored_weapon_by_section_id(uint8_t section, item_t* item) {
    bool favored = false;

    /* 根据颜色ID预设武器鉴定加成 */
    static const uint8_t tekke_favored_weapon_by_section_id[10][2] = {
        {0x09, 0x09}, //铬绿 火箭炮类
        {0x07, 0x07}, //翠绿 来复枪 狙击枪类
        {0x02, 0x02}, //天青 剑类
        {0x04, 0x04}, //纯蓝 长柄类
        {0x08, 0x08}, //淡紫 机枪类
        {0x0A, 0x0A}, //粉红 杖类
        {0x01, 0x06}, //真红 单手剑类 手枪类
        {0x03, 0x03}, //橙黄 匕首类
        {0x0B, 0x0C}, //金黄 长杖类 魔杖类
        {0x05, 0x05}  //羽白 投刃类
    };

    for (size_t i = 0; i < 2; i++) {
        if (item->datab[1] == tekke_favored_weapon_by_section_id[section][i])
            favored = true;
    }

    return favored;
}

ssize_t player_tekker_item(ship_client_t* src, sfmt_t* rng, item_t* item) {
    errno_t err = 0;

    if (item->datab[0] != ITEM_TYPE_WEAPON) {
        ERR_LOG("%s 尝试鉴定非武器物品 %s", get_player_describe(src), get_item_describe(item, src->version));
        return -1;
    }

    item->datab[4] &= 0x7F;

    /* 随机表 提升或降低 11种情况 */
    static const int8_t delta_table[11] = { -10, -5, -3, -2, -1, 0, 1, 2, 3, 5, 10 };
    uint8_t delta_index = 0;
    int8_t delta = 0;
    
    /* 对应颜色ID的装备类型加成 */
    bool favored = check_tekke_favored_weapon_by_section_id(get_player_section(src), item);
    ssize_t luck = 0;

    /* 预设加成 */
    uint8_t favored_add = 0;
    if (favored)
        favored_add = 5;

#ifdef DEBUG

    DBG_LOG("%d %d", favored, favored_add);

#endif // DEBUG

    // 调整武器的特殊能力
    {
        delta_index = (uint8_t)(sfmt_genrand_uint32(rng) % 11);
        delta = delta_table[delta_index] + favored_add;
#ifdef DEBUG
        DBG_LOG("%d", delta);
#endif // DEBUG
        // 注意：原始代码在这里专门检查了-1和+1，但是数据文件中只包括delta_indexes为4、5、6（对应-1、0、1），
        // 因此我们只检查正数和负数即可。使用原始的JudgeItem.rel文件时，行为应该是相同的，但这样更准确。
        uint8_t new_special = 0;
        if (delta < 0) {
            new_special = item->datab[4] - 1;
        }
        else if (delta > 0) {
            new_special = item->datab[4] + 1;
        }
        else {
            new_special = item->datab[4];
        }

        // 如果新的特殊能力仍然属于同一类别，则更新特殊能力
        if (get_special_type_bb(item->datab[4]) && get_special_type_bb(new_special)) {
            if ((new_special != item->datab[4]) &&
                (get_special_type_bb(item->datab[4])->type ==
                    get_special_type_bb(new_special)->type)) {
                item->datab[4] = new_special;
            }
        }

        if (delta > 0)
            luck += 1;
    }

    // 如果武器不是稀有的，则调整武器的磨损度
    if (!is_item_rare(item)) {
        pmt_weapon_bb_t pmt_weapon = { 0 };

        if (err = pmt_lookup_weapon_bb(item->datal[0], &pmt_weapon)) {
            ERR_LOG("pmt_lookup_weapon_bb 不存在数据! 错误码 %d", err);
            return 0;
        }
        delta_index = (uint8_t)(sfmt_genrand_uint32(rng) % 11);
        delta = delta_table[delta_index] + favored_add;
        int16_t new_grind = (int16_t)(item->datab[3]) + (int16_t)(delta);
        item->datab[3] = (uint8_t)clamp(new_grind, 0, pmt_weapon.max_grind);

        if (delta > 0)
            luck += 1;
    }

    // 调整武器的加成
    {
        // 注意：原始代码确实对所有三个加成使用相同的delta。
        // 注意：原始代码在增加值之前不会检查每个槽位是否实际存在加成。
        // 可能后面有检查会清除任何无效的加成，但我们没有这样的检查，因此需要在这里检查每个加成是否实际存在。
        delta_index = (uint8_t)(sfmt_genrand_uint32(rng) % 11);
        delta = delta_table[delta_index] + favored_add;

        for (size_t z = 6; z <= 10; z += 2) {
            if (!(item->datab[z] & 128) && item->datab[z + 1] > 0) {
                item->datab[z + 1] = (uint8_t)min(item->datab[z + 1] + delta, 100);
            }
        }

        if (delta > 0)
            luck += 1;
    }

    return luck;

}

iitem_t player_iitem_init(const item_t item) {
    iitem_t iitem = { LE16(0x0001), 0, 0, 0, item };
    return iitem;
}

trade_inv_t* player_tinv_init(ship_client_t* src) {
    trade_inv_t* tinv = NULL;
    if (!src->game_data) {
        ERR_LOG("GC %u 玩家游戏数据没有申请内存", src->guildcard);
        return tinv;
    }

    tinv = &src->game_data->pending_item_trade;
    tinv->other_client_id = LE16(0);
    tinv->confirmed = false;
    tinv->meseta = 0;
    tinv->trade_item_count = 0;
    for (size_t x = 0; x < 0x20; x++) {
        tinv->item_ids[x] = 0xFFFFFFFF;
        clear_inv_item(&tinv->items[x]);
    }
    return tinv;
}

bitem_t player_bitem_init(const item_t item) {
    bitem_t bitem = { item, (uint16_t)stack_size(&item), LE16(0x0001) };
    return bitem;
}

/* 整理背包物品操作 */
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

/* 整理银行物品操作 */
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

//检查装备穿戴标记item_equip_flags
bool item_check_equip(uint8_t 装备标签, uint8_t 客户端装备标签) {
    for (size_t i = 0; i < PLAYER_EQUIP_FLAGS_MAX; i++) {
        if ((客户端装备标签 & (1 << i)) && (!(装备标签 & (1 << i)))) {
            return PLAYER_EQUIP_FLAGS_NONE;
        }
    }

    return PLAYER_EQUIP_FLAGS_OK;
}

/* 物品装备函数 */
int player_equip_item(ship_client_t* src, uint32_t item_id) {
    pmt_weapon_bb_t tmp_wp = { 0 };
    pmt_guard_bb_t tmp_guard = { 0 };
    uint32_t found = 0, j = 0;
    uint8_t found_slot = 0, slot[4] = { 0 };

    psocn_bb_char_t* character = get_client_char_bb(src);
    inventory_t* inv = &character->inv;

    for (size_t i = 0; i < inv->item_count; i++) {
        if (inv->iitems[i].data.item_id == item_id) {
            item_t* found_item = &inv->iitems[i].data;

            switch (found_item->datab[0]) {
            case ITEM_TYPE_WEAPON:
                if (pmt_lookup_weapon_bb(found_item->datal[0], &tmp_wp)) {
                    ERR_LOG("%s 装备了不存在的物品数据!", get_player_describe(src));
                    return -3;
                }

                if (!item_check_equip(tmp_wp.equip_flag, src->equip_flags)) {
                    ERR_LOG("%s 装备了不属于该职业的物品数据!", get_player_describe(src));
                    return -4;
                }
                else {
                    // 解除角色上任何其他武器的装备。（防止堆叠） 
                    for (j = 0; j < inv->item_count; j++)
                        if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_WEAPON) &&
                            (inv->iitems[j].flags & EQUIP_FLAGS)) {
                            inv->iitems[j].flags &= UNEQUIP_FLAGS;
                            //DBG_LOG("卸载武器");
                        }
                    //DBG_LOG("武器识别 %02X", tmp_wp.equip_flag);
                }

                found = 1;
                break;

            case ITEM_TYPE_GUARD:
                switch (found_item->datab[1]) {
                case ITEM_SUBTYPE_FRAME:
                    if (pmt_lookup_guard_bb(found_item->datal[0], &tmp_guard)) {
                        ERR_LOG("%s 装备了不存在的物品数据!", get_player_describe(src));
                        return -5;
                    }

                    if (character->disp.level < tmp_guard.level_req) {
                        ERR_LOG("%s 等级不足, 不应该装备该物品数据!", get_player_describe(src));
                        return -6;
                    }

                    if (!item_check_equip(tmp_guard.equip_flag, src->equip_flags)) {
                        ERR_LOG("%s 装备了不属于该职业的物品数据!", get_player_describe(src));
                        return -7;
                    }
                    else {
                        //DBG_LOG("装甲识别");
                        // 移除其他装甲和插槽
                        for (j = 0; j < inv->item_count; ++j) {
                            if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_GUARD) &&
                                (inv->iitems[j].data.datab[1] != ITEM_SUBTYPE_BARRIER) &&
                                (inv->iitems[j].flags & EQUIP_FLAGS)) {
                                //DBG_LOG("卸载装甲");
                                inv->iitems[j].flags &= UNEQUIP_FLAGS;
                                inv->iitems[j].data.datab[4] = 0x00;
                            }
                        }
                    }

                    found = 1;
                    break;

                case ITEM_SUBTYPE_BARRIER: // Check barrier equip requirements 检测护盾装备请求
                    if (pmt_lookup_guard_bb(found_item->datal[0], &tmp_guard)) {
                        ERR_LOG("%s 装备了不存在的物品数据!", get_player_describe(src));
                        return -8;
                    }

                    if (character->disp.level < tmp_guard.level_req) {
                        ERR_LOG("%s 等级不足, 不应该装备该物品数据!", get_player_describe(src));
                        return -9;
                    }

                    if (!item_check_equip(tmp_guard.equip_flag, src->equip_flags)) {
                        ERR_LOG("%s 装备了不属于该职业的物品数据!", get_player_describe(src));
                        return -10;
                    }
                    else {
                        //DBG_LOG("护盾识别");
                        // Remove any other barrier
                        for (j = 0; j < inv->item_count; ++j) {
                            if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_GUARD) &&
                                (inv->iitems[j].data.datab[1] == ITEM_SUBTYPE_BARRIER) &&
                                (inv->iitems[j].flags & EQUIP_FLAGS)) {
                                //DBG_LOG("卸载护盾");
                                inv->iitems[j].flags &= UNEQUIP_FLAGS;
                                inv->iitems[j].data.datab[4] = 0x00;
                            }
                        }
                    }

                    found = 1;
                    break;

                case ITEM_SUBTYPE_UNIT:// Assign unit a slot
                    //DBG_LOG("插槽识别");
                    /* 先初始化插槽临时存储 */
                    for (j = 0; j < 4; j++)
                        slot[j] = 0;

                    for (j = 0; j < inv->item_count; j++) {
                        // Another loop ;(
                        if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_GUARD) &&
                            (inv->iitems[j].data.datab[1] == ITEM_SUBTYPE_UNIT) &&
                            (inv->iitems[j].flags & EQUIP_FLAGS)) {
                            if (inv->iitems[j].data.datab[4] < 0x04) {
                                slot[inv->iitems[j].data.datab[4]] = 1;
                                //DBG_LOG("插槽 %d 已安装", inv->iitems[j].data.datab[4]);
                            }
                        }
                    }

                    for (j = 0; j < 4; j++) {
                        if (slot[j] == 0) {
                            found_slot = j + 1;
                            break;
                        }
                    }

                    //DBG_LOG("0x%02X", found_slot);
                    if (found_slot) {
                        found_slot--;
                        inv->iitems[i].data.datab[4] = found_slot;
                        //DBG_LOG("0x%02X", found_slot);
                        found = 1;
                    }
                    else {//缺失 TODO
                        for (j = 0; j < inv->item_count; j++) {
                            // Another loop ;(
                            if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_GUARD) &&
                                (inv->iitems[j].data.datab[1] == ITEM_SUBTYPE_UNIT) &&
                                (inv->iitems[j].flags & EQUIP_FLAGS)) {
                                ERR_LOG("%s 装备了 %s 作弊的 %d 插槽物品数据!"
                                    , get_player_describe(src)
                                    , get_item_describe(&inv->iitems[j].data, src->version)
                                    , inv->iitems[j].data.datab[4]
                                );
                                inv->iitems[j].data.datab[4] = 0x00;
                                inv->iitems[i].flags &= UNEQUIP_FLAGS;
                            }
                        }
                    }
                    break;
                }
                break;

            case ITEM_TYPE_MAG:
                //DBG_LOG("玛古识别");
                // Remove equipped mag
                for (j = 0; j < inv->item_count; j++)
                    if ((inv->iitems[j].data.datab[0] == ITEM_TYPE_MAG) &&
                        (inv->iitems[j].flags & EQUIP_FLAGS)) {

                        inv->iitems[j].flags &= UNEQUIP_FLAGS;
                        //DBG_LOG("卸载玛古");
                    }
                found = 1;
                break;
            }

            if (!found) {

                ERR_LOG("%s 完成装备, 但是未识别成功", get_player_describe(src));
                return -11;
            }

            /* TODO: Should really make sure we can equip it first... */
            inv->iitems[i].flags |= EQUIP_FLAGS;
        }
    }

    if (!found)
        return -1;

    return 0;
}

/* 物品卸装函数 */
int player_unequip_item(ship_client_t* src, uint32_t item_id) {
    int found_item = 0;
    size_t x = 0, i = 0;

    inventory_t* inv = get_client_inv(src);

    for (x = 0; x < inv->item_count; x++) {
        if (inv->iitems[x].data.item_id == item_id) {
            found_item = 1;
            inv->iitems[x].flags &= UNEQUIP_FLAGS;
            switch (inv->iitems[x].data.datab[0]) {
            case ITEM_TYPE_WEAPON:
                // Remove any other weapon (in case of a glitch... or stacking)
                for (i = 0; i < inv->item_count; i++) {
                    if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_WEAPON) &&
                        (inv->iitems[i].flags & EQUIP_FLAGS))
                        inv->iitems[i].flags &= UNEQUIP_FLAGS;
                }
                break;
            case ITEM_TYPE_GUARD:
                switch (inv->iitems[x].data.datab[1]) {
                case ITEM_SUBTYPE_FRAME:
                    // Remove any other armor (stacking?) and equipped slot items.
                    for (i = 0; i < inv->item_count; ++i) {
                        if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_GUARD) &&
                            (inv->iitems[i].data.datab[1] != ITEM_SUBTYPE_BARRIER) &&
                            (inv->iitems[i].flags & EQUIP_FLAGS)) {
                            //DBG_LOG("卸载装甲");
                            inv->iitems[i].flags &= UNEQUIP_FLAGS;
                            inv->iitems[i].data.datab[4] = 0x00;
                        }
                    }
                    break;

                case ITEM_SUBTYPE_BARRIER:
                    // Remove any other barrier (stacking?)
                    for (i = 0; i < inv->item_count; i++)
                    {
                        if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_GUARD) &&
                            (inv->iitems[i].data.datab[1] == ITEM_SUBTYPE_BARRIER) &&
                            (inv->iitems[i].flags & EQUIP_FLAGS)) {
                            inv->iitems[i].flags &= UNEQUIP_FLAGS;
                            inv->iitems[i].data.datab[4] = 0x00;
                        }
                    }
                    break;

                case ITEM_SUBTYPE_UNIT:
                    // Remove unit from slot
                    inv->iitems[x].data.datab[4] = 0x00;
                    break;
                }
                break;
            case ITEM_TYPE_MAG:
                // Remove any other mags (stacking?)
                for (i = 0; i < inv->item_count; i++)
                    if ((inv->iitems[i].data.datab[0] == ITEM_TYPE_MAG) &&
                        (inv->iitems[i].flags & EQUIP_FLAGS))
                        inv->iitems[i].flags &= UNEQUIP_FLAGS;
                break;
            }
            break;
        }
    }

    if (x >= inv->item_count && !found_item)
        return -1;

    return 0;
}

/* 物品整理函数 */
int player_sort_inv_by_id(ship_client_t* src, uint32_t* id_arr, int id_count) {
    inventory_t* inv = get_client_inv(src);
    iitem_t iitem1, iitem2, swap_item = { 0 };

    // 如果物品数量小于等于 1，则不需要排序
    if (inv->item_count < 1) {
        ERR_LOG("%s 身上没有物品可以排序", get_player_describe(src));
        return -1;
    }

    // 冒泡排序
    for (int i = 0; i < id_count - 1; i++) {
        memcpy(&iitem1, &inv->iitems[i], sizeof(iitem_t));
        int index1 = -1;
        for (int j = 0; j < id_count; j++) {
            if (id_arr[j] == iitem1.data.item_id) {
                index1 = j;
                break;
            }
        }

        if (index1 < 0) {  // 物品1的 ID 不在指定 ID 数组中，跳过本次循环
            continue;
        }

        for (int j = i + 1; j < id_count; j++) {
            memcpy(&iitem2, &inv->iitems[j], sizeof(iitem_t));
            int index2 = -1;
            for (int k = 0; k < id_count; k++) {
                if (id_arr[k] == iitem2.data.item_id) {
                    index2 = k;
                    break;
                }
            }

            if (index2 < 0) {  // 物品2的 ID 不在指定 ID 数组中，跳过本次循环
                continue;
            }

            if (index2 < index1) {  // 交换两个物品的位置
                memcpy(&swap_item, &inv->iitems[i], sizeof(iitem_t));
                memcpy(&inv->iitems[i], &inv->iitems[j], sizeof(iitem_t));
                memcpy(&inv->iitems[j], &swap_item, sizeof(iitem_t));
                index1 = index2;
            }
        }
    }

    return 0;
}

/* 给客户端标记可穿戴职业装备的标签 */
void player_class_tag_item_equip_flag(ship_client_t* c) {
    psocn_bb_char_t* character = get_client_char_bb(c);
    c->equip_flags = class_equip_flags[get_player_class(c)];
}

void remove_titem_equip_flags(iitem_t* trade_item) {
    if (trade_item->flags & EQUIP_FLAGS) {
        switch (trade_item->data.datab[0])
        {
        case ITEM_TYPE_WEAPON:
        case ITEM_TYPE_GUARD:
        case ITEM_TYPE_MAG:
            trade_item->flags &= UNEQUIP_FLAGS;
            break;
        }
    }
}

//修复背包银行数据错误的物品代码
void fix_inv_bank_item(item_t* i) {
    uint32_t ch3;
    pmt_guard_bb_t tmp_guard = { 0 };

    switch (i->datab[0]) {
    case ITEM_TYPE_WEAPON:// 修正武器的值
        int8_t percent_table[6] = { 0 };
        int8_t percent = 0;
        uint32_t max_percents = 0, num_percents = 0;
        int32_t srank = 0;

        if ((i->datab[1] == 0x33) ||  // SJS和Lame最大2%
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
                // 百分比高于100或低于-100则设为0
                percent = (int8_t)i->datab[ch3 + 1];

                if ((percent > 100) || (percent < -100))
                    percent = 0;
                // 保存百分比
                percent_table[i->datab[ch3]] = percent;
            }
        }

        if (!srank) {
            for (ch3 = 6; ch3 <= 4 + (max_percents * 2); ch3 += 2) {
                // 重置 %s
                i->datab[ch3] = 0;
                i->datab[ch3 + 1] = 0;
            }

            for (ch3 = 1; ch3 <= 5; ch3++) {
                // 重建 %s
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

    case ITEM_TYPE_GUARD:// 修正装甲和护盾的值
        switch (i->datab[1])
        {
        case ITEM_SUBTYPE_FRAME:

            if (pmt_lookup_guard_bb(i->datal[0], &tmp_guard)) {
                ERR_LOG("未从PMT获取到 0x%04X 的数据!", i->datal[0]);
                break;
            }

            if (i->datab[6] > tmp_guard.dfp_range && tmp_guard.dfp_range)
                i->datab[6] = tmp_guard.dfp_range;
            if (i->datab[8] > tmp_guard.evp_range && tmp_guard.evp_range)
                i->datab[8] = tmp_guard.evp_range;
            break;

        case ITEM_SUBTYPE_BARRIER:

            if (pmt_lookup_guard_bb(i->datal[0], &tmp_guard)) {
                ERR_LOG("未从PMT获取到 0x%04X 的数据!", i->datal[0]);
                break;
            }

            if (i->datab[6] > tmp_guard.dfp_range && tmp_guard.dfp_range)
                i->datab[6] = tmp_guard.dfp_range;
            if (i->datab[8] > tmp_guard.evp_range && tmp_guard.evp_range)
                i->datab[8] = tmp_guard.evp_range;
            break;
        }
        break;

    case ITEM_TYPE_MAG:// 玛古
        int16_t mag_def, mag_pow, mag_dex, mag_mind;
        int32_t total_levels;

        magitem_t* playermag = (magitem_t*)&i->datab[0];

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
            // 玛古修正失败,则初始化所有数据
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

//整理仓库物品
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