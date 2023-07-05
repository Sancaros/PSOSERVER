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
#include <string.h>

#include <f_logs.h>

#include "iitems.h"

/* We need LE32 down below... so get it from packets.h */
#define PACKETS_H_HEADERS_ONLY
#include "packets.h"
#include "pmtdata.h"
#include "ptdata.h"
#include "rtdata.h"
#include "mag_bb.h"
#include "max_tech_level.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////物品操作

/* 初始化物品数据 */
void clear_item(item_t* item) {
    item->data_l[0] = 0;
    item->data_l[1] = 0;
    item->data_l[2] = 0;
    item->item_id = EMPTY_STRING;
    item->data2_l = 0;
}

/* 生成物品ID */
uint32_t generate_item_id(lobby_t* l, uint8_t client_id) {
    if (client_id < l->max_clients) {
        return l->item_player_id[client_id]++;
    }

    return l->item_lobby_id++;
}

uint32_t primary_identifier(item_t* i) {
    // The game treats any item starting with 04 as Meseta, and ignores the rest
    // of data1 (the value is in data2)
    switch (i->data_b[0]) 
    {
    case ITEM_TYPE_MESETA:
        return 0x040000;

    case ITEM_TYPE_TOOL:
        if (i->data_b[1] == ITEM_SUBTYPE_DISK) {
            return 0x030200;// Tech disk (data1[2] is level, so omit it)
        }
        break;

    case ITEM_TYPE_MAG:
        return 0x020000 | (i->data_b[1] << 8); // Mag

    }

    return (i->data_b[0] << 16) | (i->data_b[1] << 8) | i->data_b[2];
}

bool is_stackable(const item_t* item) {
    return max_stack_size(item) > 1;
}

size_t stack_size(const item_t* item) {
    if (max_stack_size_for_item(item->data_b[0], item->data_b[1]) > 1) {
        return item->data_b[5];
    }
    return 1;
}

size_t max_stack_size(const item_t* item) {
    return max_stack_size_for_item(item->data_b[0], item->data_b[1]);
}

size_t max_stack_size_for_item(uint8_t data0, uint8_t data1) {
    if (data0 == ITEM_TYPE_MESETA) {
        return 999999;
    }
    if (data0 == ITEM_TYPE_TOOL) {
        if ((data1 < 9) && (data1 != ITEM_SUBTYPE_DISK)) {
            return 10;
        }
        else if (data1 == ITEM_SUBTYPE_PHOTON) {
            return 99;
        }
    }
    return 1;
}

// TODO 需要客户端支持各种堆叠
//size_t max_stack_size_for_item(uint8_t data0, uint8_t data1) {
//
//    switch (data0) 
//    {
//    case ITEM_TYPE_MESETA:
//        return 999999;
//
//    case ITEM_TYPE_TOOL:
//
//        switch (data1)
//        {
//            /* 支持大量堆叠 */
//        case ITEM_SUBTYPE_MATE:
//        case ITEM_SUBTYPE_FLUID:
//        case ITEM_SUBTYPE_SOL_ATOMIZER:
//        case ITEM_SUBTYPE_MOON_ATOMIZER:
//        case ITEM_SUBTYPE_STAR_ATOMIZER:
//        case ITEM_SUBTYPE_ANTI:
//        case ITEM_SUBTYPE_TELEPIPE:
//        case ITEM_SUBTYPE_TRAP_VISION:
//        case ITEM_SUBTYPE_GRINDER:
//        case ITEM_SUBTYPE_MATERIAL:
//        case ITEM_SUBTYPE_MAG_CELL1:
//        case ITEM_SUBTYPE_MONSTER_LIMBS:
//        case ITEM_SUBTYPE_MAG_CELL2:
//        case ITEM_SUBTYPE_ADD_SLOT:
//        case ITEM_SUBTYPE_PHOTON:
//            return 99;
//
//        case ITEM_SUBTYPE_DISK:
//            return 1;
//
//
//        default:
//            return 10;
//        }
//
//        break;
//
//    default:
//        break;
//    }
//
//    return 1;
//}

bool is_common_consumable(uint32_t primary_identifier) {
    if (primary_identifier == 0x030200) {
        return false;
    }
    return (primary_identifier >= 0x030000) && (primary_identifier < 0x030A00);
}

void clear_after(item_t* this, size_t position, size_t count) {
    for (size_t x = position; x < count; x++) {
        this->data_b[x] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////玩家背包操作

/* 初始化玩家背包数据 */
void clear_iitem(iitem_t* iitem) {
    iitem->present = LE16(0x00FF);
    iitem->tech = 0;
    iitem->flags = 0;
    clear_item(&iitem->data);
}

/* 修复玩家背包数据 */
void fix_up_pl_iitem(lobby_t* l, ship_client_t* c) {
    uint32_t id;
    int i;

    if (c->version == CLIENT_VERSION_BB) {
        /* 在新房间中修正玩家背包ID */
        id = 0x00010000 | (c->client_id << 21) | (l->item_player_id[c->client_id]);

        for (i = 0; i < c->bb_pl->inv.item_count; ++i, ++id) {
            c->bb_pl->inv.iitems[i].data.item_id = LE32(id);
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

////////////////////////////////////////////////////////////////////////////////////////////////////
////银行背包操作

/* 初始化银行背包数据 */
void clear_bitem(bitem_t* bitem) {
    clear_item(&bitem->data);
    bitem->show_flags = 0;
    bitem->amount = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////游戏房间操作

/* 新增一件物品至大厅背包中. 调用者在调用这个之前必须持有大厅的互斥锁.
如果大厅的库存中没有新物品的空间,则返回NULL. */
iitem_t* lobby_add_new_item_locked(lobby_t* l, item_t* new_item) {
    lobby_item_t* item;

    /* 合理性检查... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    item = (lobby_item_t*)malloc(sizeof(lobby_item_t));

    if (!item)
        return NULL;

    memset(item, 0, sizeof(lobby_item_t));

    /* Copy the item data in. */
    item->d.data.data_l[0] = LE32(new_item->data_l[0]);
    item->d.data.data_l[1] = LE32(new_item->data_l[1]);
    item->d.data.data_l[2] = LE32(new_item->data_l[2]);
    item->d.data.item_id = LE32(l->item_lobby_id);
    item->d.data.data2_l = LE32(new_item->data2_l);

#ifdef DEBUG

    print_iitem_data(&item->d, 0, l->version);

#endif // DEBUG

    /* Increment the item ID, add it to the queue, and return the new item */
    ++l->item_lobby_id;
    TAILQ_INSERT_HEAD(&l->item_queue, item, qentry);
    return &item->d;
}

/* 玩家丢出的 取出的 购买的物品 */
iitem_t* lobby_add_item_locked(lobby_t* l, iitem_t* it) {
    lobby_item_t* item;

    /* 合理性检查... */
    if (l->version != CLIENT_VERSION_BB)
        return NULL;

    item = (lobby_item_t*)malloc(sizeof(lobby_item_t));

    if (!item)
        return NULL;

    memset(item, 0, sizeof(lobby_item_t));

    /* Copy the item data in. */
    memcpy(&item->d, it, sizeof(iitem_t));

    /* Add it to the queue, and return the new item */
    TAILQ_INSERT_HEAD(&l->item_queue, item, qentry);
    return &item->d;
}

int lobby_remove_item_locked(lobby_t* l, uint32_t item_id, iitem_t* rv) {
    lobby_item_t* i, * tmp;

    if (l->version != CLIENT_VERSION_BB)
        return -1;

    memset(rv, 0, sizeof(iitem_t));
    rv->data.data_l[0] = LE32(Item_NoSuchItem);

    i = TAILQ_FIRST(&l->item_queue);
    while (i) {
        tmp = TAILQ_NEXT(i, qentry);

        if (i->d.data.item_id == item_id) {
            memcpy(rv, &i->d, sizeof(iitem_t));
            TAILQ_REMOVE(&l->item_queue, i, qentry);
            free_safe(i);
            return 0;
        }

        i = tmp;
    }

    return 1;
}

/* 获取背包中目标物品所在槽位 */
size_t find_iitem_index(inventory_t* inv, uint32_t item_id) {
    for (size_t x = 0; x < inv->item_count; x++) {
        if (inv->iitems[x].data.item_id == item_id) {
            return x;
        }
    }

    ERR_LOG("未从背包中找到ID %u 物品", item_id);
    print_item_data(&inv->iitems->data, 5);

    return -1;
}

bool is_wrapped(item_t* this) {
    switch (this->data_b[0]) {
    case 0:
    case 1:
        return this->data_b[4] & 0x40;
    case 2:
        return this->data2_b[2] & 0x40;
    case 3:
        return !is_stackable(this) && (this->data_b[3] & 0x40);
    case 4:
        return false;
    default:
        ERR_LOG("invalid item data");
    }
}

void unwrap(item_t* this) {
    switch (this->data_b[0]) {
    case 0:
    case 1:
        this->data_b[4] &= 0xBF;
        break;
    case 2:
        this->data2_b[2] &= 0xBF;
        break;
    case 3:
        if (!is_stackable(this)) {
            this->data_b[3] &= 0xBF;
        }
        break;
    case 4:
        break;
    default:
        ERR_LOG("invalid item data");
    }
}

/* 获取背包中目标物品所在槽位 */
size_t find_equipped_weapon(inventory_t* inv){
    ssize_t ret = -1;
    for (size_t y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & 0x00000008)) {
            continue;
        }
        if (inv->iitems[y].data.data_b[0] != ITEM_TYPE_WEAPON) {
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
size_t find_equipped_armor(inventory_t* inv) {
    ssize_t ret = -1;
    for (size_t y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & 0x00000008)) {
            continue;
        }
        if (inv->iitems[y].data.data_b[0] != ITEM_TYPE_GUARD || 
            inv->iitems[y].data.data_b[1] != ITEM_SUBTYPE_FRAME) {
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
size_t find_equipped_mag(inventory_t* inv) {
    ssize_t ret = -1;
    for (size_t y = 0; y < inv->item_count; y++) {
        if (!(inv->iitems[y].flags & 0x00000008)) {
            continue;
        }
        if (inv->iitems[y].data.data_b[0] != ITEM_TYPE_MAG) {
            continue;
        }
        if (ret < 0) {
            ret = y;
        }
        else {
            ERR_LOG("找到装备多件玛古");
        }
    }
    if (ret < 0) {
        ERR_LOG("未从背包中找已装备的玛古");
    }
    return ret;
}

/* 移除背包物品操作 */
int item_remove_from_inv(iitem_t *inv, int inv_count, uint32_t item_id,
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
        tmp = inv[i].data.data_b[5];

        if(amt < tmp) {
            tmp -= amt;
            inv[i].data.data_b[5] = tmp;
            return 0;
        }
    }

    /* Move the rest of the items down to take over the place that the item in
       question used to occupy. */
    memmove(inv + i, inv + i + 1, (inv_count - i - 1) * sizeof(iitem_t));
    return 1;
}

iitem_t remove_item(ship_client_t* src, uint32_t item_id, uint32_t amount, bool allow_meseta_overdraft) {
    iitem_t ret = { 0 };

    if (item_id == 0xFFFFFFFF) {
        if (amount <= src->bb_pl->character.disp.meseta) {
            src->bb_pl->character.disp.meseta -= amount;
        }
        else if (allow_meseta_overdraft) {
            src->bb_pl->character.disp.meseta = 0;
        }
        else {
            ERR_LOG("GC %" PRIu32 " 掉落的美赛塔超出所拥有的",
                src->guildcard);
            return ret;
        }
        ret.data.data_b[0] = 0x04;
        ret.data.data2_l = amount;
        return ret;
    }

    size_t index = find_iitem_index(&src->bb_pl->inv, item_id);
    iitem_t* inventory_item = &src->bb_pl->inv.iitems[index];

    if (amount && (inventory_item->data.data_b[5] > 1) && (amount < inventory_item->data.data_b[5])) {
        ret = *inventory_item;
        ret.data.data_b[5] = amount;
        ret.data.item_id = 0xFFFFFFFF;
        inventory_item->data.data_b[5] -= amount;
        return ret;
    }

    ret = *inventory_item;
    src->bb_pl->inv.item_count--;
    for (size_t x = index; x < src->bb_pl->inv.item_count; x++) {
        src->bb_pl->inv.iitems[x] = src->bb_pl->inv.iitems[x + 1];
    }
    clear_iitem(&src->bb_pl->inv.iitems[src->bb_pl->inv.item_count]);
    return ret;
}

size_t add_iitem(ship_client_t* src, iitem_t* item) {
    uint32_t pid = primary_identifier(&item->data);

    // 检查是否为meseta，如果是，则修改统计数据中的meseta值
    if (pid == MESETA_IDENTIFIER) {
        src->bb_pl->character.disp.meseta += item->data.data2_l;
        if (src->bb_pl->character.disp.meseta > 999999) {
            src->bb_pl->character.disp.meseta = 999999;
        }
        return 0;
    }

    // 处理可合并的物品
    uint32_t combine_max = max_stack_size(&item->data);
    if (combine_max > 1) {
        // 如果玩家库存中已经存在相同物品的堆叠，获取该物品的索引
        size_t y;
        for (y = 0; y < src->bb_pl->inv.item_count; y++) {
            if (primary_identifier(&src->bb_pl->inv.iitems[y].data) == primary_identifier(&item->data)) {
                break;
            }
        }

        // 如果存在堆叠，则将数量相加，并限制最大堆叠数量
        if (y < src->bb_pl->inv.item_count) {
            src->bb_pl->inv.iitems[y].data.data_b[5] += item->data.data_b[5];
            if (src->bb_pl->inv.iitems[y].data.data_b[5] > combine_max) {
                src->bb_pl->inv.iitems[y].data.data_b[5] = combine_max;
            }
            return 0;
        }
    }

    // 如果执行到这里，既不是meseta也不是可合并物品，因此需要放入一个空的库存槽位
    if (src->bb_pl->inv.item_count >= MAX_PLAYER_INV_ITEMS) {
        ERR_LOG("GC %" PRIu32 " 物品数量超出最大值",
            src->guildcard);
        return -1;
    }
    src->bb_pl->inv.iitems[src->bb_pl->inv.item_count] = *item;
    src->bb_pl->inv.item_count++;
    return 0;
}

size_t player_use_item(ship_client_t* src, size_t item_index) {
    // On PC (and presumably DC), the client sends a 6x29 after this to delete the
    // used item. On GC and later versions, this does not happen, so we should
    // delete the item here.
    bool should_delete_item = (src->version != CLIENT_VERSION_DCV2) && (src->version != CLIENT_VERSION_PC);

    psocn_bb_db_char_t* player = src->bb_pl;
    iitem_t item = player->inv.iitems[item_index];
    uint32_t item_identifier = primary_identifier(&item.data);

    iitem_t weapon = { 0 };
    iitem_t armor = { 0 };
    iitem_t mag = { 0 };

    if (is_common_consumable(item_identifier)) { // Monomate, etc.
        // Nothing to do (it should be deleted)

    }
    else if (item_identifier == 0x030200) { // Technique disk
        uint8_t max_level = max_tech_level[item.data.data_b[4]].max_lvl[player->character.dress_data.ch_class];
        if (item.data.data_b[2] > max_level) {
            ERR_LOG("technique level too high");
            return -1;
        }
        player->character.techniques[item.data.data_b[4]] = item.data.data_b[2];

    }
    else if ((item_identifier & 0xFFFF00) == 0x030A00) { // Grinder
        if (item.data.data_b[2] > 2) {
            ERR_LOG("incorrect grinder value");
            return -2;
        }
        weapon = player->inv.iitems[find_equipped_weapon(&player->inv)];
        pmt_weapon_bb_t weapon_def = { 0 };
        if (pmt_lookup_weapon_bb(weapon.data.data_l[0], &weapon_def)) {
            ERR_LOG("GC %" PRIu32 " 装备了不存在的物品数据!",
                src->guildcard);
            return -3;
        }

        if (weapon.data.data_b[3] >= weapon_def.max_grind) {
            ERR_LOG("weapon already at maximum grind");
            return -4;
        }
        weapon.data.data_b[3] += (item.data.data_b[2] + 1);

    }
    else if ((item_identifier & 0xFFFF00) == 0x030B00) { // Material
        switch (item.data.data_b[2]) {
        case 0: // Power Material
            src->bb_pl->character.disp.stats.atp += 2;
            break;
        case 1: // Mind Material
            src->bb_pl->character.disp.stats.mst += 2;
            break;
        case 2: // Evade Material
            src->bb_pl->character.disp.stats.evp += 2;
            break;
        case 3: // HP Material
            src->bb_pl->inv.hpmats_used += 2;
            break;
        case 4: // TP Material
            src->bb_pl->inv.tpmats_used += 2;
            break;
        case 5: // Def Material
            src->bb_pl->character.disp.stats.dfp += 2;
            break;
        case 6: // Luck Material
            src->bb_pl->character.disp.stats.lck += 2;
            break;
        default:
            ERR_LOG("unknown material used");
            return -5;
        }

    }
    else if ((item_identifier & 0xFFFF00) == 0x030F00) { // AddSlot
        armor = player->inv.iitems[find_equipped_armor(&player->inv)];
        if (armor.data.data_b[5] >= 4) {
            ERR_LOG("armor already at maximum slot count");
            return -6;
        }
        armor.data.data_b[5]++;

    }
    else if (is_wrapped(&item.data)) {
        // Unwrap present
        unwrap(&item.data);
        should_delete_item = false;

    }
    else if (item_identifier == 0x003300) {
        // Unseal Sealed J-Sword => Tsumikiri J-Sword
        item.data.data_b[1] = 0x32;
        should_delete_item = false;

    }
    else if (item_identifier == 0x00AB00) {
        // Unseal Lame d'Argent => Excalibur
        item.data.data_b[1] = 0xAC;
        should_delete_item = false;

    }
    else if (item_identifier == 0x01034D) {
        // Unseal Limiter => Adept
        item.data.data_b[2] = 0x4E;
        should_delete_item = false;

    }
    else if (item_identifier == 0x01034F) {
        // Unseal Swordsman Lore => Proof of Sword-Saint
        item.data.data_b[2] = 0x50;
        should_delete_item = false;

    }
    else if (item_identifier == 0x030C00) {
        // Cell of MAG 502
        mag = player->inv.iitems[find_equipped_mag(&player->inv)];
        mag.data.data_b[1] = (player->character.dress_data.section & 1) ? 0x1D : 0x21;

    }
    else if (item_identifier == 0x030C01) {
        // Cell of MAG 213
        mag = player->inv.iitems[find_equipped_mag(&player->inv)];
        mag.data.data_b[1] = (player->character.dress_data.section & 1) ? 0x27 : 0x22;

    }
    else if (item_identifier == 0x030C02) {
        // Parts of RoboChao
        mag = player->inv.iitems[find_equipped_mag(&player->inv)];
        mag.data.data_b[1] = 0x28;

    }
    else if (item_identifier == 0x030C03) {
        // Heart of Opa Opa
        mag = player->inv.iitems[find_equipped_mag(&player->inv)];
        mag.data.data_b[1] = 0x29;

    }
    else if (item_identifier == 0x030C04) {
        // Heart of Pian
        mag = player->inv.iitems[find_equipped_mag(&player->inv)];
        mag.data.data_b[1] = 0x2A;

    }
    else if (item_identifier == 0x030C05) {
        // Heart of Chao
        mag = player->inv.iitems[find_equipped_mag(&player->inv)];
        mag.data.data_b[1] = 0x2B;

    }
    /* TODO */
    //else if ((item_identifier & 0xFFFF00) == 0x031500) {
    //    // Christmas Present, etc. - use unwrap_table + probabilities therein
    //    auto table = s->item_parameter_table->get_event_items(item.data.data_b[2]);
    //    size_t sum = 0;
    //    for (size_t z = 0; z < table.second; z++) {
    //        sum += table.first[z].probability;
    //    }
    //    if (sum == 0) {
    //        ERR_LOG("no unwrap results available for event");
    //    }
    //    size_t det = random_object<size_t>() % sum;
    //    for (size_t z = 0; z < table.second; z++) {
    //        const auto& entry = table.first[z];
    //        if (det > entry.probability) {
    //            det -= entry.probability;
    //        }
    //        else {
    //            item.data.data2_l = 0;
    //            item.data.data_b[0] = entry.item[0];
    //            item.data.data_b[1] = entry.item[1];
    //            item.data.data_b[2] = entry.item[2];
    //            item.data.data_b.clear_after(3);
    //            should_delete_item = false;

    //            lobby_t l = src->cur_lobby;
    //            send_create_inventory_item(l, c, item.data);
    //            break;
    //        }
    //    }

    //}
    //else {
    //    // Use item combinations table from ItemPMT
    //    bool combo_applied = false;
    //    for (size_t z = 0; z < player->inv.item_count; z++) {
    //        iitem_t inv_item = player->inv.iitems[z];
    //        if (!(inv_item.flags & 0x00000008)) {
    //            continue;
    //        }
    //        try {
    //            const auto& combo = s->item_parameter_table->get_item_combination(
    //                item.data, inv_item.data);
    //            if (combo.char_class != 0xFF && combo.char_class != player->character.dress_data.ch_class) {
    //                ERR_LOG("item combination requires specific char_class");
    //            }
    //            if (combo.mag_level != 0xFF) {
    //                if (inv_item.data.data_b[0] != 2) {
    //                    ERR_LOG("item combination applies with mag level requirement, but equipped item is not a mag");
    //                }
    //                if (inv_item.data.compute_mag_level() < combo.mag_level) {
    //                    ERR_LOG("item combination applies with mag level requirement, but equipped mag level is too low");
    //                }
    //            }
    //            if (combo.grind != 0xFF) {
    //                if (inv_item.data.data_b[0] != 0) {
    //                    ERR_LOG("item combination applies with grind requirement, but equipped item is not a weapon");
    //                }
    //                if (inv_item.data.data_b[3] < combo.grind) {
    //                    ERR_LOG("item combination applies with grind requirement, but equipped weapon grind is too low");
    //                }
    //            }
    //            if (combo.level != 0xFF && player->disp.stats.level + 1 < combo.level) {
    //                ERR_LOG("item combination applies with level requirement, but player level is too low");
    //            }
    //            // If we get here, then the combo applies
    //            if (combo_applied) {
    //                ERR_LOG("multiple combinations apply");
    //            }
    //            combo_applied = true;

    //            inv_item.data.data_b[0] = combo.result_item[0];
    //            inv_item.data.data_b[1] = combo.result_item[1];
    //            inv_item.data.data_b[2] = combo.result_item[2];
    //            inv_item.data.data_b[3] = 0; // Grind
    //            inv_item.data.data_b[4] = 0; // Flags + special
    //        }
    //        catch (const out_of_range&) {
    //        }
    //    }

    //    if (!combo_applied) {
    //        ERR_LOG("no combinations apply");
    //    }
    //}

    if (should_delete_item) {
        // Allow overdrafting meseta if the client is not BB, since the server isn't
        // informed when meseta is added or removed from the bank.
        remove_item(src, item.data.item_id, 1, src->version != CLIENT_VERSION_BB);
    }

    return 0;
}

/* 整理银行物品操作 */
void cleanup_bb_bank(ship_client_t *c) {
    uint32_t item_id = 0x80010000 | (c->client_id << 21);
    uint32_t count = LE32(c->bb_pl->bank.item_count), i;

    for(i = 0; i < count; ++i) {
        c->bb_pl->bank.bitems[i].data.item_id = LE32(item_id);
        ++item_id;
    }

    /* Clear all the rest of them... */
    for(; i < MAX_PLAYER_BANK_ITEMS; ++i) {
        memset(&c->bb_pl->bank.bitems[i], 0, sizeof(bitem_t));
        c->bb_pl->bank.bitems[i].data.item_id = EMPTY_STRING;
    }
}

int item_deposit_to_bank(ship_client_t *c, bitem_t *it) {
    uint32_t i, count = LE32(c->bb_pl->bank.item_count);
    int amount;

    /* Make sure there's space first. */
    if(count == MAX_PLAYER_BANK_ITEMS) {
        return -1;
    }

    /* Check if the item is stackable, since we may have to do some stuff
       differently... */
    if(is_stackable(&it->data)) {
        /* Look for anything that matches this item in the inventory. */
        for(i = 0; i < count; ++i) {
            if(c->bb_pl->bank.bitems[i].data.data_l[0] == it->data.data_l[0]) {
                amount = c->bb_pl->bank.bitems[i].data.data_b[5] += it->data.data_b[5];
                c->bb_pl->bank.bitems[i].amount = LE16(amount);
                return 0;
            }
        }
    }

    /* Copy the new item in at the end. */
    c->bb_pl->bank.bitems[count] = *it;
    ++count;
    c->bb_pl->bank.item_count = count;

    return 1;
}

int item_take_from_bank(ship_client_t *c, uint32_t item_id, uint8_t amt,
                        bitem_t *rv) {
    uint32_t i, count = LE32(c->bb_pl->bank.item_count);
    bitem_t *it;

    /* Look for the item in question */
    for(i = 0; i < count; ++i) {
        if(c->bb_pl->bank.bitems[i].data.item_id == item_id) {
            break;
        }
    }

    /* Did we find it? If not, return error. */
    if(i == count) {
        return -1;
    }

    /* Grab the item in question, and copy the data to the return pointer. */
    it = &c->bb_pl->bank.bitems[i];
    *rv = *it;

    /* Check if the item is stackable, since we may have to do some stuff
       differently... */
    if(is_stackable(&it->data)) {
        if(amt < it->data.data_b[5]) {
            it->data.data_b[5] -= amt;
            it->amount = LE16(it->data.data_b[5]);

            /* Fix the amount on the returned value, and return. */
            rv->data.data_b[5] = amt;
            rv->amount = LE16(amt);

            return 0;
        }
        else if(amt > it->data.data_b[5]) {
            return -1;
        }
    }

    if (c->bb_pl != NULL && c->bb_pl->bank.bitems != NULL) {
        if (count > 0 && i >= 0 && i < count) {
            // 确保索引 i 在有效范围内（0 到 count-1）

            if (i <= count - 1) {
                // 计算要移动的字节数
                size_t bytesToMove = (count - i - 1) * sizeof(bitem_t);

                memmove(c->bb_pl->bank.bitems + i, c->bb_pl->bank.bitems + i + 1, bytesToMove);

                // 更新相关变量或进行其他操作
                --count;
                c->bb_pl->bank.item_count = LE32(count);

                return 1;
            }
            else {
                return -1;
            }
        }
        else {
            return -2;
            // i 超出有效范围，进行错误处理
        }
    }
    else {
        return -3;
        // 指针为空，进行错误处理
    }
    ///* Move the rest of the items down to take over the place that the item in
    //   question used to occupy. */
    //memmove(c->bb_pl->bank.bitems + i, c->bb_pl->bank.bitems + i + 1,
    //        (count - i - 1) * sizeof(bitem_t));
    //--count;
    //c->bb_pl->bank.item_count = LE32(count);

    //return 1;
}

//检查装备穿戴标记item_equip_flags
int item_check_equip(uint8_t 装备标签, uint8_t 客户端装备标签) {
    int32_t eqOK = EQUIP_FLAGS_OK;
    uint32_t ch;

    for (ch = 0; ch < EQUIP_FLAGS_MAX; ch++) {
        if ((客户端装备标签 & (1 << ch)) && (!(装备标签 & (1 << ch)))) {
            eqOK = EQUIP_FLAGS_NONE;
            break;
        }
    }

    return eqOK;
}

/* 物品检测装备标签 */
int item_check_equip_flags(ship_client_t* c, uint32_t item_id) {
    pmt_weapon_bb_t tmp_wp = { 0 };
    pmt_guard_bb_t tmp_guard = { 0 };
    uint32_t found = 0, found_slot = 0, j = 0, slot[4] = { 0 }, inv_count = 0;
    int i = 0;
    item_t found_item = { 0 };

    i = find_iitem_index(&c->bb_pl->inv, item_id);
#ifdef DEBUG
    DBG_LOG("识别槽位 %d 背包物品ID %d 数据物品ID %d", i, c->bb_pl->inv.iitems[i].data.item_id, item_id);
    print_item_data(&c->bb_pl->inv.iitems[i].data, c->version);
#endif // DEBUG

    /* 如果找不到该物品，则将用户从船上推下. */
    if (i == -1) {
        ERR_LOG("GC %" PRIu32 " 掉落无效的堆叠物品!", c->guildcard);
        return -1;
    }

    found_item = c->bb_pl->inv.iitems[i].data;

    if (found_item.item_id == item_id) {
        found = 1;
        inv_count = c->bb_pl->inv.item_count;

        switch (found_item.data_b[0])
        {
        case ITEM_TYPE_WEAPON:
            if (pmt_lookup_weapon_bb(found_item.data_l[0], &tmp_wp)) {
                ERR_LOG("GC %" PRIu32 " 装备了不存在的物品数据!",
                    c->guildcard);
                return -1;
            }

            if (item_check_equip(tmp_wp.equip_flag, c->equip_flags)) {
                ERR_LOG("GC %" PRIu32 " 装备了不属于该职业的物品数据!",
                    c->guildcard);
                return -2;
            }
            else {
                // 解除角色上任何其他武器的装备。（防止堆叠） 
                for (j = 0; j < inv_count; j++)
                    if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_WEAPON) &&
                        (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {
                        c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                        //DBG_LOG("卸载武器");
                    }
                //DBG_LOG("武器识别 %02X", tmp_wp.equip_flag);
            }
            break;

        case ITEM_TYPE_GUARD:
            switch (found_item.data_b[1]) {
            case ITEM_SUBTYPE_FRAME:
                if (pmt_lookup_guard_bb(found_item.data_l[0], &tmp_guard)) {
                    ERR_LOG("GC %" PRIu32 " 装备了不存在的物品数据!",
                        c->guildcard);
                    return -3;
                }

                if (c->bb_pl->character.disp.level < tmp_guard.level_req) {
                    ERR_LOG("GC %" PRIu32 " 等级不足, 不应该装备该物品数据!",
                        c->guildcard);
                    return -4;
                }

                if (item_check_equip(tmp_guard.equip_flag, c->equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " 装备了不属于该职业的物品数据!",
                        c->guildcard);
                    return -5;
                }
                else {
                    //DBG_LOG("装甲识别");
                    // 移除其他装甲和插槽
                    for (j = 0; j < inv_count; ++j) {
                        if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                            (c->bb_pl->inv.iitems[j].data.data_b[1] != ITEM_SUBTYPE_BARRIER) &&
                            (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {
                            //DBG_LOG("卸载装甲");
                            c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                            c->bb_pl->inv.iitems[j].data.data_b[4] = 0x00;
                        }
                    }
                    break;
                }
                break;

            case ITEM_SUBTYPE_BARRIER: // Check barrier equip requirements 检测护盾装备请求
                if (pmt_lookup_guard_bb(found_item.data_l[0], &tmp_guard)) {
                    ERR_LOG("GC %" PRIu32 " 装备了不存在的物品数据!",
                        c->guildcard);
                    return -3;
                }

                if (c->bb_pl->character.disp.level < tmp_guard.level_req) {
                    ERR_LOG("GC %" PRIu32 " 等级不足, 不应该装备该物品数据!",
                        c->guildcard);
                    return -4;
                }

                if (item_check_equip(tmp_guard.equip_flag, c->equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " 装备了不属于该职业的物品数据!",
                        c->guildcard);
                    return -5;
                }else {
                    //DBG_LOG("护盾识别");
                    // Remove any other barrier
                    for (j = 0; j < inv_count; ++j) {
                        if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                            (c->bb_pl->inv.iitems[j].data.data_b[1] == ITEM_SUBTYPE_BARRIER) &&
                            (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {
                            //DBG_LOG("卸载护盾");
                            c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                            c->bb_pl->inv.iitems[j].data.data_b[4] = 0x00;
                        }
                    }
                }
                break;

            case ITEM_SUBTYPE_UNIT:// Assign unit a slot
                //DBG_LOG("插槽识别");
                for (j = 0; j < 4; j++)
                    slot[j] = 0;

                for (j = 0; j < inv_count; j++) {
                    // Another loop ;(
                    if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                        (c->bb_pl->inv.iitems[j].data.data_b[1] == ITEM_SUBTYPE_UNIT)) {
                        //DBG_LOG("插槽 %d 识别", j);
                        if ((c->bb_pl->inv.iitems[j].flags & LE32(0x00000008)) &&
                            (c->bb_pl->inv.iitems[j].data.data_b[4] < 0x04)) {

                            slot[c->bb_pl->inv.iitems[j].data.data_b[4]] = 1;
                            //DBG_LOG("插槽 %d 卸载", j);
                        }
                    }
                }

                for (j = 0; j < 4; j++) {
                    if (slot[j] == 0) {
                        found_slot = j + 1;
                        break;
                    }
                }

                if (found_slot && (c->mode > 0)) {
                    found_slot--;
                    c->bb_pl->inv.iitems[j].data.data_b[4] = (uint8_t)(found_slot);
                }
                else {
                    if (found_slot) {
                        found_slot--;
                        c->bb_pl->inv.iitems[j].data.data_b[4] = (uint8_t)(found_slot);
                    }
                    else {//缺失 TODO
                        c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                        ERR_LOG("GC %" PRIu32 " 装备了作弊的插槽物品数据!",
                            c->guildcard);
                        return -1;
                    }
                }
                break;
            }
            break;

        case ITEM_TYPE_MAG:
            //DBG_LOG("玛古识别");
            // Remove equipped mag
            for (j = 0; j < c->bb_pl->inv.item_count; j++)
                if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_MAG) &&
                    (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {

                    c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                    //DBG_LOG("卸载玛古");
                }
            break;
        }

        //DBG_LOG("完成卸载, 但是未识别成功");

        /* TODO: Should really make sure we can equip it first... */
        c->bb_pl->inv.iitems[i].flags |= LE32(0x00000008);
    }

    return found;
}

/* 给客户端标记可穿戴职业装备的标签 */
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

/* 新增背包物品 */
int item_add_to_inv(ship_client_t* c, iitem_t* iitem) {
    uint32_t pid = primary_identifier(&iitem->data);

    // 比较烦的就是, 美赛塔只保存在 disp_data, not in the inventory struct. If the
    // item is meseta, we have to modify disp instead.
    if (pid == MESETA_IDENTIFIER) {
        c->bb_pl->character.disp.meseta += iitem->data.data2_l;
        if (c->bb_pl->character.disp.meseta > 999999) {
            c->bb_pl->character.disp.meseta = 999999;
        }
        return 0;
    }

    // 如果我们到了这里, then it's not meseta and not a combine item, so it needs to
    // go into an empty inventory slot
    if (c->bb_pl->inv.item_count >= 30)
        return -1;

    // 处理堆叠物品
    size_t combine_max = max_stack_size(&iitem->data);
    if (combine_max > 1) {
        //如果玩家的库存中已经有一堆相同的物品,则获取物品索引 
        size_t y;
        for (y = 0; y < c->bb_pl->inv.item_count; y++) {
            if (primary_identifier(&c->bb_pl->inv.iitems[y].data) == primary_identifier(&iitem->data)) {
                break;
            }
        }

        // 如果已经发现存在同类型堆叠物品, 则将其添加至相同物品槽位
        if (y < c->bb_pl->inv.item_count) {
            c->bb_pl->inv.iitems[y].data.data_b[5] += iitem->data.data_b[5];
            if (c->bb_pl->inv.iitems[y].data.data_b[5] > combine_max) {
                c->bb_pl->inv.iitems[y].data.data_b[5] = (uint8_t)combine_max;
            }
            return 0;
        }
    }

    return 1;
}

int add_item_to_client(ship_client_t* c, iitem_t* iitem) {
    int add_count = -1;

    add_count = item_add_to_inv(c, iitem);

    switch (add_count)
    {
        /* 背包空间不足时 返回 -1 */
    case -1:
        return -1;

    case 0:
        /* 如果背包中有堆叠的 则直接新增到堆叠中 包括美赛塔*/
        return 0;

    case 1:
        memcpy(&c->bb_pl->inv.iitems[c->bb_pl->inv.item_count], iitem, sizeof(iitem_t));

        c->bb_pl->inv.item_count += add_count;
        c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

#ifdef DEBUG

        for (int i = 0; i < c->bb_pl->inv.item_count; ++i) {
            print_iitem_data(&c->bb_pl->inv.iitems[i], i, c->version);
        }

#endif // DEBUG

        return 0;

        /* 即使程序不会这么走 我还是写一个默认 */
    default:
        ERR_LOG("GC %" PRIu32 " add_item_to_client 逻辑错误!",
            c->guildcard);
        return -1;
    }

}

//修复背包银行数据错误的物品代码
void fix_inv_bank_item(item_t* i) {
    uint32_t ch3;
    pmt_guard_bb_t tmp_guard = { 0 };

    switch (i->data_b[0])
    {
    case ITEM_TYPE_WEAPON:// 修正武器的值
        int8_t percent_table[6] = { 0 };
        int8_t percent = 0;
        uint32_t max_percents = 0, num_percents = 0;
        int32_t srank = 0;

        if ((i->data_b[1] == 0x33) ||  // SJS和Lame最大2%
            (i->data_b[1] == 0xAB))
            max_percents = 2;
        else
            max_percents = 3;

        memset(&percent_table[0], 0, 6);

        for (ch3 = 6; ch3 <= 4 + (max_percents * 2); ch3 += 2) {
            if (i->data_b[ch3] & 128) {
                srank = 1; // S-Rank
                break;
            }

            if ((i->data_b[ch3]) && (i->data_b[ch3] < 0x06)) {
                // Percents over 100 or under -100 get set to 0 
                // 百分比高于100或低于-100则设为0
                percent = (int8_t)i->data_b[ch3 + 1];

                if ((percent > 100) || (percent < -100))
                    percent = 0;
                // 保存百分比
                percent_table[i->data_b[ch3]] = percent;
            }
        }

        if (!srank) {
            for (ch3 = 6; ch3 <= 4 + (max_percents * 2); ch3 += 2) {
                // 重置 %s
                i->data_b[ch3] = 0;
                i->data_b[ch3 + 1] = 0;
            }

            for (ch3 = 1; ch3 <= 5; ch3++) {
                // 重建 %s
                if (percent_table[ch3]) {
                    i->data_b[6 + (num_percents * 2)] = ch3;
                    i->data_b[7 + (num_percents * 2)] = (uint8_t)percent_table[ch3];
                    num_percents++;
                    if (num_percents == max_percents)
                        break;
                }
            }
        }

        break;

    case ITEM_TYPE_GUARD:// 修正装甲和护盾的值
        switch (i->data_b[1])
        {
        case ITEM_SUBTYPE_FRAME:

            if (pmt_lookup_guard_bb(i->data_l[0], &tmp_guard)) {
                ERR_LOG("未从PMT获取到 0x%04X 的数据!", i->data_l[0]);
                break;
            }

            if (i->data_b[6] > tmp_guard.dfp_range)
                i->data_b[6] = tmp_guard.dfp_range;
            if (i->data_b[8] > tmp_guard.evp_range)
                i->data_b[8] = tmp_guard.evp_range;
            break;

        case ITEM_SUBTYPE_BARRIER:

            if (pmt_lookup_guard_bb(i->data_l[0], &tmp_guard)) {
                ERR_LOG("未从PMT获取到 0x%04X 的数据!", i->data_l[0]);
                break;
            }

            if (i->data_b[6] > tmp_guard.dfp_range)
                i->data_b[6] = tmp_guard.dfp_range;
            if (i->data_b[8] > tmp_guard.evp_range)
                i->data_b[8] = tmp_guard.evp_range;
            break;
        }
        break;
    case ITEM_TYPE_MAG:// 玛古
        magitem_t* playermag;
        int16_t mag_def, mag_pow, mag_dex, mag_mind;
        int32_t total_levels;

        playermag = (magitem_t*)&i->data_b[0];

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

//修复背包装备数据错误的物品代码
void fix_equip_item(inventory_t* inv) {
    uint32_t i, eq_weapon = 0, eq_armor = 0, eq_shield = 0, eq_mag = 0;

    /* 检查所有已装备的物品 */
    for (i = 0; i < inv->item_count; i++) {
        if (inv->iitems[i].flags & LE32(0x00000008)) {
            switch (inv->iitems[i].data.data_b[0])
            {
            case ITEM_TYPE_WEAPON:
                eq_weapon++;
                break;

            case ITEM_TYPE_GUARD:
                switch (inv->iitems[i].data.data_b[1])
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
            // 当装备了多个武器时,取消所装备武器
            if ((inv->iitems[i].data.data_b[0] == 0x00) &&
                (inv->iitems[i].flags & LE32(0x00000008)))
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
        }

    }

    if (eq_armor > 1) {
        for (i = 0; i < inv->item_count; i++) {
            // Unequip all armor and slot items when there is more than one armor equipped. 
            // 当装备了多个护甲时，取消装备所有护甲和槽道具。 
            if ((inv->iitems[i].data.data_b[0] == 0x01) &&
                (inv->iitems[i].data.data_b[1] != 0x02) &&
                (inv->iitems[i].flags & LE32(0x00000008))) {

                inv->iitems[i].data.data_b[3] = 0x00;
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    if (eq_shield > 1) {
        for (i = 0; i < inv->item_count; i++) {
            // Unequip all shields when there is more than one equipped. 
            // 当装备了多个护盾时，取消装备所有护盾。 
            if ((inv->iitems[i].data.data_b[0] == 0x01) &&
                (inv->iitems[i].data.data_b[1] == 0x02) &&
                (inv->iitems[i].flags & LE32(0x00000008))) {

                inv->iitems[i].data.data_b[3] = 0x00;
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    if (eq_mag > 1) {
        for (i = 0; i < inv->item_count; i++) {
            // Unequip all mags when there is more than one equipped. 
            // 当装备了多个玛古时，取消装备所有玛古。 
            if ((inv->iitems[i].data.data_b[0] == 0x02) &&
                (inv->iitems[i].flags & LE32(0x00000008)))
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
        }
    }
}

/* 清理背包物品 */
void clean_up_inv(inventory_t* inv) {
    uint8_t i, j = 0;

    iitem_t* new_data = (iitem_t*)malloc(sizeof(iitem_t) * inv->item_count);

    if (!new_data) {
        ERR_LOG("无法更新背包物品ID,申请内存失败");
        return;
    }

    for (i = 0; i < inv->item_count; i++)
        if (inv->iitems[i].present)
            new_data[j++] = inv->iitems[i];

    inv->item_count = j;

    for (i = 0; i < inv->item_count; i++)
        inv->iitems[i] = new_data[i];

    ERR_LOG("更新背包物品完成");

    /* 释放掉内存 */
    free_safe(new_data);
}

/* 修正玩家背包物品ID数据 */
void reset_item_id(lobby_t* l, ship_client_t* c) {
    uint32_t id;
    int i;

    if (c->version == CLIENT_VERSION_BB) {
        /* 在新房间中修正玩家背包ID */
        id = 0x00010000 | (c->client_id << 21) | (l->item_player_id[c->client_id]);

        for (i = 0; i < c->bb_pl->inv.item_count; ++i, ++id) {
            c->bb_pl->inv.iitems[i].data.item_id = LE32(id);
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


void sort_client_inv(inventory_t* inv) {
    iitem_t sort_data[MAX_PLAYER_INV_ITEMS] = { 0 };
    unsigned ch, ch2, ch3, ch4, itemid;

    ch2 = 0x0C;

    memset(&sort_data[0], 0, sizeof(iitem_t) * MAX_PLAYER_INV_ITEMS);

    for (ch4 = 0; ch4 < MAX_PLAYER_INV_ITEMS; ch4++) {
        sort_data[ch4].data.data_b[1] = 0xFF;
        sort_data[ch4].data.item_id = 0xFFFFFFFF;
    }

    ch4 = 0;

    for (ch = 0; ch < MAX_PLAYER_INV_ITEMS; ch++) {
        itemid = inv->iitems[ch2].data.item_id;
        ch2 += 4;
        if (itemid != 0xFFFFFFFF) {
            for (ch3 = 0; ch3 < inv->item_count; ch3++) {
                if ((inv->iitems[ch3].present) && (inv->iitems[ch3].data.item_id == itemid)) {
                    sort_data[ch4++] = inv->iitems[ch3];
                    break;
                }
            }
        }
    }

    for (ch = 0; ch < MAX_PLAYER_INV_ITEMS; ch++)
        inv->iitems[ch] = sort_data[ch];

}

void clean_up_bank(psocn_bank_t* bank) {
    uint32_t i, j = 0;

    bitem_t* bank_data = (bitem_t*)malloc(sizeof(bitem_t) * bank->item_count);

    if (!bank_data) {
        ERR_LOG("无法更新银行物品ID,申请内存失败");
        return;
    }

    for (i = 0; i < bank->item_count; i++)
        if (bank->bitems[i].data.item_id != 0xFFFFFFFF)
            bank_data[j++] = bank->bitems[i]; 

    bank->item_count = j;

    for (i = 0; i < bank->item_count; i++)
        bank->bitems[i] = bank_data[i];

    ERR_LOG("更新银行物品完成");

    /* 释放掉内存 */
    free_safe(bank_data);
}
