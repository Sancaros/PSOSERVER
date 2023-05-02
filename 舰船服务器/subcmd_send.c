/*
    梦幻之星中国 舰船服务器 副指令发送
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

#include "subcmd_send.h"
#include "subcmd_size_table.h"

#include "items.h"
#include "f_logs.h"


// subcmd 直接发送指令至客户端
/* 发送副指令数据包至房间 ignore_check 是否忽略客户端忽略的玩家 c 是否不发给自己*/
int subcmd_send_lobby_bb(lobby_t* l, ship_client_t* c, subcmd_bb_pkt_t* pkt, int ignore_check) {
    int i;

    /* Send the packet to every connected client. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            /* If we're supposed to check the ignore list, and this client is on
               it, don't send the packet
               如果我们要检查忽略列表，并且该客户端在其中，请不要发送数据包. */
            if (ignore_check && client_has_ignored(l->clients[i], c->guildcard)) {
                continue;
            }

            if (l->clients[i]->version != CLIENT_VERSION_DCV1 ||
                !(l->clients[i]->flags & CLIENT_FLAG_IS_NTE))
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)pkt);
            else
                subcmd_translate_bb_to_nte(l->clients[i], pkt);
        }
    }

    return 0;
}

/* It is assumed that all parameters are already in little-endian order. */
int subcmd_send_bb_drop_stack(ship_client_t* c, uint32_t area, float x,
    float z, iitem_t* item) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_drop_stack_t drop = { 0 };

    if (!l)
        return -1;

    /* 填充数据并准备发送.. */
    drop.hdr.pkt_len = LE16(sizeof(subcmd_bb_drop_stack_t));
    drop.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    drop.hdr.flags = 0;
    drop.shdr.type = SUBCMD60_DROP_STACK;
    drop.shdr.size = 0x09;
    drop.shdr.client_id = c->client_id;

    /* 填充剩余数据 */
    drop.area = area;
    drop.x = x;
    drop.z = z;
    drop.data.data_l[0] = item->data.data_l[0];
    drop.data.data_l[1] = item->data.data_l[1];
    drop.data.data_l[2] = item->data.data_l[2];
    drop.data.item_id = item->data.item_id;
    drop.data.data2_l = item->data.data2_l;
    drop.two = 0x00000000;

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&drop, 0);
}

int subcmd_send_bb_pick_item(ship_client_t* c, uint32_t item_id, uint32_t area) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_destroy_map_item_t pkt = { 0 };

    if (!l)
        return -1;

    /* 填充数据并准备发送 */
    pkt.hdr.pkt_len = LE16(sizeof(subcmd_bb_destroy_map_item_t));
    pkt.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt.hdr.flags = 0;
    pkt.shdr.type = SUBCMD60_DEL_MAP_ITEM;
    pkt.shdr.size = 0x03;
    pkt.shdr.client_id = c->client_id;

    /* 填充剩余数据 */
    pkt.client_id2 = c->client_id;
    pkt.item_id = item_id;
    pkt.area = area;

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
}

int subcmd_send_bb_create_item(ship_client_t* c, item_t item, int 发送给其他客户端) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_create_item_t pkt = { 0 };

    if (!l)
        return -1;

    /* 填充数据并准备发送 */
    pkt.hdr.pkt_len = LE16(sizeof(subcmd_bb_create_item_t));
    pkt.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt.hdr.flags = 0;
    pkt.shdr.type = SUBCMD60_CREATE_ITEM;
    pkt.shdr.size = 0x07;
    pkt.shdr.client_id = c->client_id;

    /* 填充剩余数据 */
    pkt.item = item;
    pkt.unused2 = 0;

    if (发送给其他客户端)
        return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
    else
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)&pkt, 0);
}

int subcmd_send_bb_destroy_map_item(ship_client_t* c, uint16_t area,
    uint32_t item_id) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_destroy_map_item_t d = { 0 };

    if (!l)
        return -1;

    /* 填充数据并准备发送 */
    d.hdr.pkt_len = LE16(0x0014);
    d.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    d.hdr.flags = 0;
    d.shdr.type = SUBCMD60_DEL_MAP_ITEM;
    d.shdr.size = 0x03;
    d.shdr.client_id = c->client_id;

    /* 填充剩余数据 */
    d.client_id2 = c->client_id;
    d.area = area;
    d.item_id = item_id;

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&d, 0);
}

int subcmd_send_bb_destroy_item(ship_client_t* c, uint32_t item_id,
    uint8_t amt) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_destroy_item_t d = { 0 };

    if (!l)
        return -1;

    /* 填充数据并准备发送 */
    d.hdr.pkt_len = LE16(0x0014);
    d.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    d.hdr.flags = 0;
    d.shdr.type = SUBCMD60_DELETE_ITEM;
    d.shdr.size = 0x03;
    d.shdr.client_id = c->client_id;

    /* 填充剩余数据 */
    d.item_id = item_id;
    d.amount = LE32(amt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)&d, 0);
}

//从客户端移除美赛塔
int subcmd_send_bb_delete_meseta(ship_client_t* c, uint32_t count, uint32_t drop) {
    uint32_t stack_count;
    uint32_t tmp;
    iitem_t tmp_meseta = { 0 };
    iitem_t* meseta = { 0 };
    lobby_t* l = c->cur_lobby;

    if (!l)
        return -1;

    stack_count = c->bb_pl->character.disp.meseta;

    if (stack_count < count) {
        c->bb_pl->character.disp.meseta = 0;
        count = stack_count;
    }
    else
        c->bb_pl->character.disp.meseta -= count;

    if (drop) {
        tmp_meseta.data.data_l[0] = LE32(Item_Meseta);
        tmp_meseta.data.data_l[1] = tmp_meseta.data.data_l[2] = 0;
        tmp_meseta.data.item_id = LE32((++l->item_player_id[c->client_id]));
        tmp_meseta.data.data2_l = count;

        /* 当获得物品... 将其新增入房间物品背包. */
        if (!(meseta = lobby_add_item_locked(l, &tmp_meseta))) {
            /* *大厅里可能是烤面包... 至少确保该用户仍然（大部分）安全... */
            ERR_LOG("无法将物品添加至游戏房间!\n");
            return -1;
        }

        /* Remove the meseta from the character data */
        tmp = LE32(c->bb_pl->character.disp.meseta);

        if (count > tmp) {
            ERR_LOG("GC %" PRIu32 " 掉落的美赛塔超出所拥有的",
                c->guildcard);
            return -1;
        }

        c->bb_pl->character.disp.meseta = LE32(tmp - count);
        c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;

        /* 现在我们有两个数据包要发送.首先,发送一个数据包,告诉每个人有一个物品掉落.
        然后,发送一个从客户端的库存中删除物品的人.第一个必须发给每个人,
        第二个必须发给除了最初发送这个包裹的人以外的所有人. */
        if (subcmd_send_bb_drop_stack(c, c->drop_area, c->x, c->z, meseta))
            return -1;
    }

    return 0;
}

int subcmd_send_bb_gm_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req) {
    subcmd_bb_itemgen_t gen = { 0 };
    int r = LE16(req->request_id);
    lobby_t* l = c->cur_lobby;

    /* 填充数据并准备发送. */
    gen.hdr.pkt_type = GAME_COMMAND0_TYPE;
    gen.hdr.flags = 0;
    gen.hdr.pkt_len = LE16(0x30);
    gen.shdr.type = SUBCMD60_BOX_ENEMY_ITEM_DROP;
    gen.shdr.size = 0x0B;
    gen.shdr.unused = 0x0000;
    gen.data.area = req->area;
    gen.data.from_enemy = 0x02;
    gen.data.request_id = req->request_id;
    gen.data.x = req->x;
    gen.data.z = req->z;
    gen.data.unk1 = LE32(0x00000010);

    gen.data.item.data_l[0] = LE32(c->new_item.data_l[0]);
    gen.data.item.data_l[1] = LE32(c->new_item.data_l[1]);
    gen.data.item.data_l[2] = LE32(c->new_item.data_l[2]);
    gen.data.item.data2_l = LE32(c->new_item.data2_l);
    gen.data.item2 = LE32(0x00000002);

    /* Obviously not "right", but it works though, so we'll go with it. */
    gen.data.item.item_id = LE32((r | 0x06010100));

    /* Send the packet to every client in the lobby. */
    subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&gen, 0);

    /* Clear this out. */
    clear_item(&c->new_item);

    return 0;
}

int subcmd_send_bb_quest_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req, ship_client_t* dest) {
    uint32_t mid = LE16(req->request_id);
    uint32_t pti = req->pt_index;
    lobby_t* l = c->cur_lobby;
    uint32_t qdrop = 0xFFFFFFFF;

    if (pti != 0x30 && l->mids)
        qdrop = quest_search_enemy_list(mid, l->mids, l->num_mids, 0);
    if (qdrop == 0xFFFFFFFF && l->mtypes)
        qdrop = quest_search_enemy_list(pti, l->mtypes, l->num_mtypes, 0);

    /* If we found something, the version matters here. Basically, we only care
       about the none option on DC/PC, as rares do not drop in quests. On GC,
       we have to block drops on all options other than free, since we have no
       control over the drop once we send it to the leader.
       如果我们发现了什么，版本在这里很重要。基本上，我们只关心DC/PC上的“无”选项，
       因为稀有物种不会放弃任务。在GC上，我们必须阻止除免费外的所有选项上的丢弃，
       因为一旦我们将丢弃发送给领导者，我们就无法控制丢弃。*/
    if (qdrop != PSOCN_QUEST_ENDROP_FREE && qdrop != 0xFFFFFFFF) {
        switch (l->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_BB:
            if (qdrop == PSOCN_QUEST_ENDROP_NONE)
                return 0;
            break;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            return 0;
        }
    }

    /* If we haven't handled it, we're not supposed to, so send it on to the
       leader. */
    return send_pkt_bb(dest, (bb_pkt_hdr_t*)req);
}

int subcmd_send_bb_lobby_item(lobby_t* l, subcmd_bb_itemreq_t* req,
    const iitem_t* item) {
    subcmd_bb_itemgen_t gen = { 0 };
    uint32_t tmp = LE32(req->unk1)/* & 0x0000FFFF*/;

    /* 填充数据并准备发送. */
    gen.hdr.pkt_type = GAME_COMMAND0_TYPE;
    gen.hdr.flags = 0;
    gen.hdr.pkt_len = LE16(0x0030);
    gen.shdr.type = SUBCMD60_BOX_ENEMY_ITEM_DROP;
    gen.shdr.size = 0x0B;
    gen.shdr.unused = 0x0000;
    gen.data.area = req->area;
    gen.data.from_enemy = req->pt_index;   /* Probably not right... but whatever. */
    gen.data.request_id = req->request_id;
    gen.data.x = req->x;
    gen.data.z = req->z;
    gen.data.unk1 = LE32(tmp);       /* ??? */

    gen.data.item.data_l[0] = LE32(item->data.data_l[0]);
    gen.data.item.data_l[1] = LE32(item->data.data_l[1]);
    gen.data.item.data_l[2] = LE32(item->data.data_l[2]);
    gen.data.item.data2_l = LE32(item->data.data2_l);

    gen.data.item.item_id = LE32(item->data.item_id);

    /* Send the packet to every client in the lobby. */
    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&gen, 0);
}

int subcmd_send_bb_enemy_item_req(lobby_t* l, subcmd_bb_itemreq_t* req,
    const iitem_t* item) {
    subcmd_bb_itemgen_t gen = { 0 };
    uint32_t tmp = LE32(req->unk1)/* & 0x0000FFFF*/;

    if (!l)
        return -1;

    /* 填充数据并准备发送. */
    gen.hdr.pkt_type = GAME_COMMAND0_TYPE;
    gen.hdr.flags = 0;
    gen.hdr.pkt_len = LE16(0x0030);
    gen.shdr.type = SUBCMD60_ENEMY_ITEM_DROP_REQ;
    gen.shdr.size = 0x06;
    gen.shdr.unused = 0x0000;

    gen.data.area = req->area;
    gen.data.from_enemy = req->pt_index;   /* Probably not right... but whatever. */
    gen.data.request_id = req->request_id;
    gen.data.x = req->x;
    gen.data.z = req->z;
    gen.data.unk1 = LE32(tmp);       /* ??? */

    gen.data.item.data_l[0] = LE32(item->data.data_l[0]);
    gen.data.item.data_l[1] = LE32(item->data.data_l[1]);
    gen.data.item.data_l[2] = LE32(item->data.data_l[2]);
    gen.data.item.data2_l = LE32(item->data.data2_l);

    gen.data.item.item_id = LE32(item->data.item_id);

    /* Send the packet to every client in the lobby. */
    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&gen, 0);
}

int subcmd_send_bb_exp(ship_client_t* c, uint32_t exp_amount) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_exp_t pkt = { 0 };

    if (!l)
        return -1;

    /* 填充数据并准备发送 */
    pkt.hdr.pkt_len = LE16(0x0010);
    pkt.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt.hdr.flags = 0;
    pkt.shdr.type = SUBCMD60_GIVE_EXP;
    pkt.shdr.size = 0x02;
    pkt.shdr.client_id = c->client_id;
    pkt.exp_amount = LE32(exp_amount);

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
}

int subcmd_send_bb_set_exp_rate(ship_client_t* c, uint32_t exp_rate) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_set_exp_rate_t pkt = { 0 };
    uint16_t exp_r;
    int rv = 0;

    if (!l)
        return -1;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (c->game_data->expboost <= 0)
        c->game_data->expboost = 1;

    exp_r = LE16(exp_rate * c->game_data->expboost);

    if (exp_r <= 1)
        exp_r = 1;

    /* 填充数据并准备发送 */
    pkt.hdr.pkt_len = LE16(0x000C);
    pkt.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt.hdr.flags = 0;
    pkt.shdr.type = SUBCMD60_SET_EXP_RATE;
    pkt.shdr.size = 0x03;
    pkt.shdr.params = LE16(exp_r);

    rv = send_pkt_bb(c, (bb_pkt_hdr_t*)&pkt);

    if (!rv) {
        if (l->exp_mult < LE32(exp_r)) {
            l->exp_mult = LE32(exp_r);
            DBG_LOG("GC %" PRIu32 " 的房间经验倍率提升为 %u 倍", c->guildcard, l->exp_mult);
        }
    }
    else
        ERR_LOG("GC %" PRIu32 " 设置房间经验 %u 倍发生错误!",
            c->guildcard, exp_r);

    return rv;
}

int subcmd_send_bb_level(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_level_up_t pkt = { 0 };
    int i;
    uint16_t base, mag;

    if (!l)
        return -1;

    /* 填充数据并准备发送 */
    pkt.hdr.pkt_len = LE16(0x001C);
    pkt.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt.hdr.flags = 0;
    pkt.shdr.type = SUBCMD60_LEVEL_UP;
    pkt.shdr.size = 0x05;
    pkt.shdr.client_id = c->client_id;

    /* 填充人物基础数据. 均为 little-endian 字符串. */
    pkt.atp = c->bb_pl->character.disp.stats.atp;
    pkt.mst = c->bb_pl->character.disp.stats.mst;
    pkt.evp = c->bb_pl->character.disp.stats.evp;
    pkt.hp = c->bb_pl->character.disp.stats.hp;
    pkt.dfp = c->bb_pl->character.disp.stats.dfp;
    pkt.ata = c->bb_pl->character.disp.stats.ata;
    pkt.level = c->bb_pl->character.disp.level;

    /* 增加MAG的升级奖励. */
    for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
        if ((c->bb_pl->inv.iitems[i].flags & LE32(0x00000008)) &&
            c->bb_pl->inv.iitems[i].data.data_b[0] == 0x02) {
            base = LE16(pkt.dfp);
            mag = LE16(c->bb_pl->inv.iitems[i].data.data_w[2]) / 100;
            pkt.dfp = LE16((base + mag));

            base = LE16(pkt.atp);
            mag = LE16(c->bb_pl->inv.iitems[i].data.data_w[3]) / 50;
            pkt.atp = LE16((base + mag));

            base = LE16(pkt.ata);
            mag = LE16(c->bb_pl->inv.iitems[i].data.data_w[4]) / 200;
            pkt.ata = LE16((base + mag));

            base = LE16(pkt.mst);
            mag = LE16(c->bb_pl->inv.iitems[i].data.data_w[5]) / 50;
            pkt.mst = LE16((base + mag));

            break;
        }
    }

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
}

// 向玩家发送货物清单
int subcmd_bb_send_shop(ship_client_t* c, uint8_t shop_type, uint8_t num_items) {
    block_t* b = c->cur_block;
    subcmd_bb_shop_inv_t* shop = { 0 };
    int rv = 0;

    shop = (subcmd_bb_shop_inv_t*)malloc(sizeof(subcmd_bb_shop_inv_t));

    if (!shop) {
        ERR_LOG("无法分配商店物品清单内存");
        rv = -1;
    }

    if (num_items > sizeof(shop->items) / sizeof(shop->items[0])) {
        ERR_LOG("GC %" PRIu32 " 获取商店物品超出限制! %d %d",
            c->guildcard, num_items, sizeof(shop->items) / sizeof(shop->items[0]));
        rv = -1;
    }

    uint16_t len = LE16(0x0016) + num_items * sizeof(sitem_t);

    for (uint8_t i = 0; i < num_items; ++i) {
        memset(&shop->items[i], 0, sizeof(sitem_t));
        shop->items[i] = c->game_data->shop_items[i];
    }

    shop->hdr.pkt_len = LE16(len); //236 - 220 11 * 20 = 16 + num_items * sizeof(sitem_t)
    shop->hdr.pkt_type = LE16(GAME_COMMANDC_TYPE);
    shop->hdr.flags = 0;
    shop->shdr.type = SUBCMD60_SHOP_INV;
    shop->shdr.size = (uint8_t)sizeof(c->game_data->shop_items);
    shop->shdr.params = LE16(0x037F);
    shop->shop_type = shop_type;
    shop->num_items = num_items;

    //print_payload((unsigned char*)&shop, LE16(shop.hdr.pkt_len));

    rv = send_pkt_bb(c, (bb_pkt_hdr_t*)shop);
    free_safe(shop);
    return rv;
}

int subcmd_bb_60size_check(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int sent = 0;
    uint16_t size = pkt->hdr.pkt_len;
    uint16_t sizecheck = pkt->size;

    //if (type != SUBCMD60_MOVE_FAST && type != SUBCMD60_MOVE_SLOW) {
    //    switch (l->type) {

    //    case LOBBY_TYPE_LOBBY:
    //        DBG_LOG("BB处理大厅 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);
    //        break;

    //    case LOBBY_TYPE_GAME:
    //        if (l->flags & LOBBY_FLAG_QUESTING) {
    //            DBG_LOG("BB处理任务 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);
    //        }
    //        else
    //            DBG_LOG("BB处理普通游戏 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);
    //        break;

    //    default:
    //        DBG_LOG("BB处理通用 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);
    //        break;
    //    }
    //}

    sizecheck *= 4;
    sizecheck += 8;

    if (!l)
        return -1;

    if (size != sizecheck)
    {
        DBG_LOG("客户端发送 0x60 指令数据包 大小不一致 sizecheck != size.");
        DBG_LOG("指令: 0x%02X | 大小: %04X | Sizecheck: %04X", pkt->type,
            size, sizecheck);
        //pkt->size = ((size / 4) - 2);
    }

    if (l->type == LOBBY_TYPE_LOBBY)
    {
        type *= 2;

        if (type == SUBCMD62_GUILDCARD)
            sizecheck = sizeof(subcmd_bb_gcsend_t);
        else
            sizecheck = size_ct[type + 1] + 4;

        if ((size != sizecheck) && (sizecheck > 4))
            sent = 1;

        // No size check packet encountered while in Lobby_Room mode...
        if (sizecheck == 4) {
            DBG_LOG("没有0x60指令 0x%04X 的大小检测信息", type);
            sent = 1;
        }
    }
    else
    {
        if (dont_send_60[(type * 2) + 1] == 1)
        {
            sent = 1;
            UNK_CSPD(type, c->version, (uint8_t*)pkt);
        }
    }

    if ((pkt->data[1] != c->client_id) &&
        (size_ct[(type * 2) + 1] != 0x00) &&
        (type != SUBCMD60_SYMBOL_CHAT) &&
        (type != SUBCMD60_GOGO_BALL))
        sent = 1;

    if ((type == SUBCMD60_SYMBOL_CHAT) &&
        (pkt->data[1] != c->client_id))
        sent = 1;

    if (type == SUBCMD60_BURST_DONE)
        sent = 1;

    return sent;
}

int subcmd_bb_626Dsize_check(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    int sent = 0;
    uint16_t size = pkt->hdr.pkt_len;
    uint16_t sizecheck = pkt->size;

    //DBG_LOG("BB处理 GC %" PRIu32 " 62/6D指令: 0x%02X", c->guildcard, type);

    //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    sizecheck *= 4;
    sizecheck += 8;

    if (size != sizecheck) {
        DBG_LOG("客户端发送 0x6D62 指令数据包 大小不一致.");
        DBG_LOG("指令: 0x%02X | pkt_len: %04X | Sizecheck: %04X", type,
            size, sizecheck);
        sent = 1;
        //pkt->size = (size / 4);
    }

    if (type >= 0x04)
        sent = 1;

    return sent;
}
