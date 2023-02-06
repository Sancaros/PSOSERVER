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
#include <stdbool.h>
#include <iconv.h>
#include <string.h>
#include <pthread.h>

#include <f_logs.h>
#include <debug.h>
#include <f_iconv.h>
#include <mtwist.h>
#include <items.h>

#include "subcmd.h"
#include "subcmd_size_table.h"
#include "shopdata.h"
#include "pmtdata.h"
#include "clients.h"
#include "ship_packets.h"
#include "utils.h"
#include "items.h"
#include "word_select.h"
#include "scripts.h"
#include "shipgate.h"
#include "quest_functions.h"
#include "mag_bb.h"

// 当客户端发送游戏命令时, 调用此文件中的函数
// 指令集
// (60, 62, 6C, 6D, C9, CB).
/* Forward declarations */
static int subcmd_send_bb_delete_meseta(ship_client_t* c, uint32_t count, uint32_t drop);
static int subcmd_send_bb_drop_stack(ship_client_t* c, uint32_t area, float x,
    float z, iitem_t* item);

static int subcmd_send_bb_create_item(ship_client_t* c, item_t item, int 发送给其他客户端);
static int subcmd_send_bb_destroy_map_item(ship_client_t* c, uint16_t area,
    uint32_t item_id);
static int subcmd_send_bb_destroy_item(ship_client_t* c, uint32_t item_id,
    uint8_t amt);

// subcmd 直接发送指令至客户端
static inline int bb_reg_sync_index(lobby_t* l, uint16_t regnum) {
    int i;

    if (!(l->q_flags & LOBBY_QFLAG_SYNC_REGS))
        return -1;

    for (i = 0; i < l->num_syncregs; ++i) {
        if (regnum == l->syncregs[i])
            return i;
    }

    return -1;
}

/* It is assumed that all parameters are already in little-endian order. */
static int subcmd_send_bb_drop_stack(ship_client_t* c, uint32_t area, float x,
    float z, iitem_t* item) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_drop_stack_t drop = { 0 };

    if (!l)
        return -1;

    /* 填充数据并准备发送.. */
    drop.hdr.pkt_len = LE16(0x002C);
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

static int subcmd_send_bb_create_item(ship_client_t* c, item_t item, int 发送给其他客户端) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_create_item_t new_item = { 0 };

    if (!l)
        return -1;

    /* 填充数据并准备发送 */
    new_item.hdr.pkt_len = LE16(sizeof(subcmd_bb_create_item_t));
    new_item.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    new_item.hdr.flags = 0;
    new_item.shdr.type = SUBCMD60_CREATE_ITEM;
    new_item.shdr.size = 0x07;
    new_item.shdr.client_id = c->client_id;

    /* 填充剩余数据 */
    new_item.item = item;
    new_item.unused2 = 0;

    if (发送给其他客户端)
        return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&new_item, 0);
    else
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)&new_item, 0);
}

static int subcmd_send_bb_destroy_map_item(ship_client_t* c, uint16_t area,
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

static int subcmd_send_bb_destroy_item(ship_client_t* c, uint32_t item_id,
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
static int subcmd_send_bb_delete_meseta(ship_client_t* c, uint32_t count, uint32_t drop) {
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

static int subcmd_send_bb_gm_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req) {
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

static int subcmd_send_bb_quest_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req, ship_client_t* dest) {
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

    if (c->game_data.expboost <= 0)
        c->game_data.expboost = 1;

    exp_r = LE16(exp_rate * c->game_data.expboost);

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
        l->exp_mult = LE32(exp_r);

        DBG_LOG("房间经验倍率为 %d 倍", l->exp_mult);
    }
    else
        ERR_LOG("GC %" PRIu32 " 设置房间经验 %d 倍失败!",
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
        shop->items[i] = c->game_data.shop_items[i];
    }

    shop->hdr.pkt_len = LE16(len); //236 - 220 11 * 20 = 16 + num_items * sizeof(sitem_t)
    shop->hdr.pkt_type = LE16(GAME_COMMANDC_TYPE);
    shop->hdr.flags = 0;
    shop->shdr.type = SUBCMD60_SHOP_INV;
    shop->shdr.size = (uint8_t)sizeof(c->game_data.shop_items);
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

    if (type != SUBCMD60_MOVE_FAST && type != SUBCMD60_MOVE_SLOW) {
        switch (l->type) {

        case LOBBY_TYPE_LOBBY:
            DBG_LOG("BB处理大厅 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);
            break;

        case LOBBY_TYPE_GAME:
            if (l->flags & LOBBY_FLAG_QUESTING) {
                DBG_LOG("BB处理任务 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);
            }
            else
                DBG_LOG("BB处理普通游戏 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);
            break;

        default:
            DBG_LOG("BB处理通用 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);
            break;
        }
    }

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

    DBG_LOG("BB处理 GC %" PRIu32 " 62/6D指令: 0x%02X", c->guildcard, type);

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

static int handle_bb_cmd_check_size(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! size %02X",
            c->guildcard, pkt->type, size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_lobby_size(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅 %s 指令!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! size %02X",
            c->guildcard, pkt->type, size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_game_size(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间内 %s 指令!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_client_id(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (pkt->data[2] != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间内 %s 指令!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //UNK_CSPD(pkt->type, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

static int handle_bb_gcsend(ship_client_t* src, ship_client_t* dest) {
    size_t in, out;
    char* inptr;
    char* outptr;

    /* Make sure the recipient is not ignoring the sender... */
    if (client_has_ignored(dest, src->guildcard))
        return 0;

        /* This differs based on the destination client's version. */
    switch (dest->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    {
        subcmd_dc_gcsend_t dc;

        memset(&dc, 0, sizeof(dc));

        /* Convert the name (UTF-16 -> ASCII). */
        memset(&dc.name, '-', 16);
        in = 48;
        out = 24;
        inptr = (char*)&src->pl->bb.character.name[2];
        outptr = dc.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)src->bb_pl->guildcard_desc;
        outptr = dc.text;

        if (src->bb_pl->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        dc.hdr.pkt_type = GAME_COMMAND2_TYPE;
        dc.hdr.flags = (uint8_t)dest->client_id;
        dc.hdr.pkt_len = LE16(0x0088);
        dc.shdr.type = SUBCMD62_GUILDCARD;
        dc.shdr.size = 0x21;
        dc.shdr.unused = 0x0000;
        dc.player_tag = LE32(0x00010000);
        dc.guildcard = LE32(src->guildcard);
        dc.unused2 = 0;
        dc.disable_udp = 1;
        dc.language = src->language_code;
        dc.section = src->pl->bb.character.disp.dress_data.section;
        dc.ch_class = src->pl->bb.character.disp.dress_data.ch_class;
        dc.padding[0] = dc.padding[1] = dc.padding[2] = 0;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&dc);
    }

    case CLIENT_VERSION_PC:
    {
        subcmd_pc_gcsend_t pc;

        /* Don't allow guild cards to be sent to PC NTE, as it doesn't
           support them. */
        if ((dest->flags & CLIENT_FLAG_IS_NTE))
            return send_txt(src, "%s", __(src, "\tE\tC7Cannot send Guild\n"
                "Card to that user."));

        memset(&pc, 0, sizeof(pc));

        /* First the name and text... */
        memcpy(pc.name, &src->pl->bb.character.name[2], BB_CHARACTER_NAME_LENGTH * 2 - 4);
        memcpy(pc.text, src->bb_pl->guildcard_desc, sizeof(src->bb_pl->guildcard_desc));

        /* Copy the rest over. */
        pc.hdr.pkt_type = GAME_COMMAND2_TYPE;
        pc.hdr.flags = (uint8_t)dest->client_id;
        pc.hdr.pkt_len = LE16(0x00F8);
        pc.shdr.type = SUBCMD62_GUILDCARD;
        pc.shdr.size = 0x3D;
        pc.shdr.unused = 0x0000;
        pc.player_tag = LE32(0x00010000);
        pc.guildcard = LE32(src->guildcard);
        pc.padding = 0;
        pc.disable_udp = 1;
        pc.language = src->language_code;
        pc.section = src->pl->bb.character.disp.dress_data.section;
        pc.ch_class = src->pl->bb.character.disp.dress_data.ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&pc);
    }

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    {
        subcmd_gc_gcsend_t gc;

        memset(&gc, 0, sizeof(gc));

        /* Convert the name (UTF-16 -> ASCII). */
        memset(&gc.name, '-', 16);
        in = 48;
        out = BB_CHARACTER_NAME_LENGTH * 2;
        inptr = (char*)&src->pl->bb.character.name[2];
        outptr = gc.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)src->bb_pl->guildcard_desc;
        outptr = gc.text;

        if (src->bb_pl->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        gc.hdr.pkt_type = GAME_COMMAND2_TYPE;
        gc.hdr.flags = (uint8_t)dest->client_id;
        gc.hdr.pkt_len = LE16(0x0098);
        gc.shdr.type = SUBCMD62_GUILDCARD;
        gc.shdr.size = 0x25;
        gc.shdr.unused = 0x0000;
        gc.player_tag = LE32(0x00010000);
        gc.guildcard = LE32(src->guildcard);
        gc.padding = 0;
        gc.disable_udp = 1;
        gc.language = src->language_code;
        gc.section = src->pl->bb.character.disp.dress_data.section;
        gc.ch_class = src->pl->bb.character.disp.dress_data.ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&gc);
    }

    case CLIENT_VERSION_XBOX:
    {
        subcmd_xb_gcsend_t xb;
        size_t in, out;
        char* inptr;
        char* outptr;

        memset(&xb, 0, sizeof(xb));

        /* Convert the name (UTF-16 -> ASCII). */
        memset(&xb.name, '-', 16);
        in = 48;
        out = BB_CHARACTER_NAME_LENGTH * 2;
        inptr = (char*)&src->pl->bb.character.name[2];
        outptr = xb.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 512;
        inptr = (char*)src->bb_pl->guildcard_desc;
        outptr = xb.text;

        if (src->bb_pl->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        xb.hdr.pkt_type = GAME_COMMAND2_TYPE;
        xb.hdr.flags = (uint8_t)dest->client_id;
        xb.hdr.pkt_len = LE16(0x0234);
        xb.shdr.type = SUBCMD62_GUILDCARD;
        xb.shdr.size = 0x8C;
        xb.shdr.unused = LE16(0xFB0D);
        xb.player_tag = LE32(0x00010000);
        xb.guildcard = LE32(src->guildcard);
        xb.xbl_userid = LE64(src->guildcard);
        xb.disable_udp = 1;
        xb.language = src->language_code;
        xb.section = src->pl->bb.character.disp.dress_data.section;
        xb.ch_class = src->pl->bb.character.disp.dress_data.ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&xb);
    }

    case CLIENT_VERSION_BB:
    {
        subcmd_bb_gcsend_t bb;

        /* 填充数据并准备发送.. */
        memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));
        bb.hdr.pkt_len = LE16(0x0114);
        bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
        bb.hdr.flags = LE32(dest->client_id);
        bb.shdr.type = SUBCMD62_GUILDCARD;
        bb.shdr.size = 0x43;
        bb.shdr.unused = 0x0000;
        bb.guildcard = LE32(src->guildcard);
        memcpy(bb.name, src->pl->bb.character.name, BB_CHARACTER_NAME_LENGTH * 2);
        memcpy(bb.guild_name, src->bb_opts->guild_name, sizeof(src->bb_opts->guild_name));
        memcpy(bb.text, src->bb_pl->guildcard_desc, sizeof(src->bb_pl->guildcard_desc));
        bb.disable_udp = 1;
        bb.language = src->language_code;
        bb.section = src->pl->bb.character.disp.dress_data.section;
        bb.ch_class = src->pl->bb.character.disp.dress_data.ch_class;

        return send_pkt_bb(dest, (bb_pkt_hdr_t*)&bb);
    }
    }

    return 0;
}

static int handle_bb_pick_up(ship_client_t* c, subcmd_bb_pick_up_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found;
    uint32_t item, tmp;
    iitem_t iitem_data;
    //uint32_t ic[3];

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中拾取了物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误的拾取数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Try to remove the item from the lobby... */
    found = lobby_remove_item_locked(l, pkt->item_id, &iitem_data);

    if (found < 0) {
        return -1;
    }
    else if (found > 0) {
        /* Assume someone else already picked it up, and just ignore it... */
        return 0;
    }
    else {
        item = LE32(iitem_data.data.data_l[0]);

        /* Is it meseta, or an item? */
        if (item == Item_Meseta) {
            tmp = LE32(iitem_data.data.data2_l) + LE32(c->bb_pl->character.disp.meseta);

            /* Cap at 999,999 meseta. */
            if (tmp > 999999)
                tmp = 999999;

            c->bb_pl->character.disp.meseta = LE32(tmp);
            c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
        }
        else {
            iitem_data.flags = 0;
            iitem_data.present = LE16(1);
            iitem_data.tech = 0;

            /* 将物品新增至背包. */
            found = item_add_to_inv(c->bb_pl->inv.iitems,
                c->bb_pl->inv.item_count, &iitem_data);

            if (found == -1) {
                ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法拾取!",
                    c->guildcard);
                //return -1;
            }

            c->bb_pl->inv.item_count += found;
            memcpy(&c->pl->bb.inv, &c->bb_pl->inv, sizeof(iitem_t));
        }
    }

    /* Let everybody know that the client picked it up, and remove it from the
       view. */
    //memcpy(ic, iitem_data.data.data_l, 3 * sizeof(uint32_t));
    subcmd_send_bb_create_item(c, /*ic, */iitem_data.data/*, iitem_data.data2_l*/, 1);

    return subcmd_send_bb_destroy_map_item(c, pkt->area, iitem_data.data.item_id);
}

static int handle_bb_item_req(ship_client_t* c, ship_client_t* d, subcmd_bb_itemreq_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            c->guildcard, pkt->shdr.type, pkt->shdr.size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }
    
    if (c->new_item.data_l[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        rv = subcmd_send_bb_gm_itemreq(c, (subcmd_bb_itemreq_t*)pkt);
    }
    else if (l->dropfunc && (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
        rv = l->dropfunc(c, l, pkt);
    }
    else if ((l->num_mtypes || l->num_mids) &&
        (l->flags & LOBBY_FLAG_QUESTING)) {
        rv = subcmd_send_bb_quest_itemreq(c, (subcmd_bb_itemreq_t*)pkt, d);
    }
    else
        rv = send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);

    return rv;
}

static int handle_bb_bitem_req(ship_client_t* c, ship_client_t* d, subcmd_bb_bitemreq_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0030) || pkt->shdr.size != 0x0A) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            c->guildcard, pkt->shdr.type, pkt->shdr.size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    if (c->new_item.data_l[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        rv = subcmd_send_bb_gm_itemreq(c, (subcmd_bb_itemreq_t*)pkt);
    }
    else if (l->dropfunc && (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
        rv = l->dropfunc(c, l, pkt);
    }
    else if ((l->num_mtypes || l->num_mids) &&
        (l->flags & LOBBY_FLAG_QUESTING)) {
        rv = subcmd_send_bb_quest_itemreq(c, (subcmd_bb_itemreq_t*)pkt, d);
    }
    else
        rv = send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);

    return rv;
}

static int handle_bb_trade(ship_client_t* c, ship_client_t* d, subcmd_bb_trade_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = -1;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return rv;
    }

    if (pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            c->guildcard, pkt->shdr.type, pkt->shdr.size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return rv;
    }
    send_simple(d, TRADE_5_TYPE, 0x01);
    send_msg(d, BB_SCROLL_MSG_TYPE, "%s", __(c, "\tE\tC6交易功能存在故障，暂时关闭"));
    send_simple(c, TRADE_5_TYPE, 0x01);
    send_msg(c, BB_SCROLL_MSG_TYPE, "%s", __(c, "\tE\tC6交易功能存在故障，暂时关闭"));

    //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    //switch (pkt->menu.menu_id) {
    //case 0x0000:
    //    switch (pkt->menu.menu_pos)
    //    {
    //    case 0x0000:
    //        printf("交易错误\n");
    //        break;

    //    case 0x0082:
    //        printf("开始交易 \n");
    //        break;

    //    default:
    //        break;
    //    }
    //    break;

    //case 0xFFFF:
    //    switch (pkt->menu.menu_pos)
    //    {
    //    case 0xFFFF:
    //        d->game_data.pending_item_trade->confirmed = true;
    //        printf("确认交易 \n");
    //        break;

    //    case 0xFF01:
    //        printf("梅塞塔交易 数量 %d\n", pkt->trade_num);
    //        break;

    //    default:
    //        break;
    //    }
    //    break;

    //case 0x0085:
    //    switch (pkt->menu.menu_pos)
    //    {
    //    case 0xB4F4:
    //        printf("取消交易 \n");
    //        break;

    //    default:
    //        break;
    //    }
    //    break;

    //case 0x41A0:
    //    printf("交易故障 \n");
    //    break;

    //case 0x21F2:
    //    switch (pkt->menu.menu_pos)
    //    {
    //    case 0x9F40:
    //        if (c->game_data.pending_item_trade->confirmed && d->game_data.pending_item_trade->confirmed) {
    //            printf("双方确认，结束交易 \n");
    //            rv = send_simple(d, TRADE_4_TYPE, 0x01);
    //            rv = send_simple(c, TRADE_4_TYPE, 0x01);
    //            memset(d->game_data.pending_item_trade, 0, sizeof(client_trade_item_t));
    //            memset(c->game_data.pending_item_trade, 0, sizeof(client_trade_item_t));
    //        }
    //        break;
    //    case 0xA730:
    //        rv = send_simple(d, TRADE_2_TYPE, 0x01);
    //        rv = send_simple(c, TRADE_2_TYPE, 0x01);
    //        break;

    //    default:
    //        break;
    //    }
    //    printf("对方同意交易 \n");
    //    break;

    //case 0x21F6:
    //    switch (pkt->menu.menu_pos)
    //    {
    //    case 0xEF40:
    //        rv = send_simple(d, TRADE_3_TYPE, 0x01);
    //        rv = send_simple(c, TRADE_3_TYPE, 0x01);
    //        printf("确认所有物品 \n");
    //        break;

    //    case 0xEC70:
    //        rv = send_simple(d, TRADE_4_TYPE, 0x01);
    //        rv = send_simple(c, TRADE_4_TYPE, 0x01);
    //        break;

    //    case 0xF730:
    //    case 0xC730:
    //        printf("0xF730 \n");
    //        rv = send_simple(d, TRADE_5_TYPE, 0x01);
    //        rv = send_simple(c, TRADE_5_TYPE, 0x01);
    //        break;

    //    default:
    //        break;
    //    }
    //    break;

    //case 0x0021:
    //    uint16_t inv_item_pos = pkt->menu.menu_pos;
    //    printf("加入交易物品 位置 %04X  \n", pkt->menu.menu_pos);
    //    break;

    //default:
    //    //             物品背包位置   
    //    //pkt->menu_id 06          00 21 00
    //    printf("未确认菜单 ID 0x%04X POS 0x%04X\n", pkt->menu.menu_id, pkt->menu.menu_pos);
    //    print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
    //    break;
    //}

    rv = send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
    //if (pkt->menu_id == LE32(0x00000082)) {
    //    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
    //}

    //if (pkt->menu_id == LE32(0x21F6F730)) {
    //    return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
    //}

    //t2 = c->game_data.pending_item_trade.other_client_id = (uint8_t)pkt->target_id;
    //t1 = d->game_data.pending_item_trade.other_client_id = (uint8_t)pkt->req_id;

    //if (t1 == d->game_data.pending_item_trade.other_client_id && pkt->menu_id == 0xFFFFFFFF) {
    //    c->game_data.pending_item_trade.confirmed = 1;
    //    rv = send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
    //    return rv;
    //}
    //else if (t2 == c->game_data.pending_item_trade.other_client_id && pkt->menu_id == 0xFFFFFFFF) {
    //    d->game_data.pending_item_trade.confirmed = 1;
    //    rv = send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
    //    return rv;
    //}

    //if (pkt->menu_id == LE32(0x21F29F40)) {
    //    memset(&c->game_data.pending_item_trade, 0, sizeof(client_trade_item_t));
    //    rv = send_simple(c, TRADE_5_TYPE, 0);
    //    memset(&d->game_data.pending_item_trade, 0, sizeof(client_trade_item_t));
    //    rv = send_simple(d, TRADE_5_TYPE, 0);
    //}

    //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
    return rv;
}

static int handle_bb_shop_req(ship_client_t* c, subcmd_bb_shop_req_t* req) {
    lobby_t* l = c->cur_lobby;
    block_t* b = c->cur_block;
    sitem_t item_data = { 0 };

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    uint32_t shop_type = LE32(req->shop_type);

    //print_payload((unsigned char*)req, LE16(req->hdr.pkt_len));

    if (c->version == CLIENT_VERSION_BB) {
        uint8_t num_items = 9 + (mt19937_genrand_int32(&b->rng) % 4);

        //printf("%d \n", num_items);

        for (uint8_t i = 0; i < num_items; ++i) {
            memset(&c->game_data.shop_items[i], 0, sizeof(sitem_t));

            switch (shop_type) {
            case 0:// 工具商店
                item_data = create_bb_shop_item(l->difficulty, 3, &b->rng);
                break;
            case 1:// 武器商店
                item_data = create_bb_shop_item(l->difficulty, 0, &b->rng);
                break;
            case 2:// 装甲商店
                item_data = create_bb_shop_item(l->difficulty, 1, &b->rng);
                break;

            default:
                ERR_LOG("菜单类型缺失 shop_type = %d", shop_type);
                return -1;
                break;
            }

            item_data.sitem_id = generate_item_id(l, c->client_id);

            memcpy(&c->game_data.shop_items[i], &item_data, sizeof(sitem_t));
        }

        return subcmd_bb_send_shop(c, shop_type, num_items);
    }
    else {
        subcmd_bb_shop_inv_t shop;
        int i;
        block_t* b = c->cur_block;

        memset(&shop, 0, sizeof(shop));

        shop.hdr.pkt_len = LE16(0x00EC);
        shop.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
        shop.hdr.flags = 0;
        shop.shdr.type = SUBCMD60_SHOP_INV;
        shop.shdr.size = 0x3B;
        shop.shdr.params = 0x037F;

        shop.shop_type = req->shop_type;
        shop.num_items = 0x0B;

        for (i = 0; i < 0x0B; ++i) {
            shop.items[i].data_l[0] = LE32((0x03 | (i << 8)));
            shop.items[i].sitem_id = 0xFFFFFFFF;
            shop.items[i].costl = LE32((mt19937_genrand_int32(&b->rng) % 255));
        }

        return send_pkt_bb(c, (bb_pkt_hdr_t*)&shop);
    }

    return 0;
}

static int handle_bb_shop_buy(ship_client_t* c, subcmd_bb_shop_buy_t* pkt) {
    lobby_t* l = c->cur_lobby;
    sitem_t *shopi;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品购买数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //DBG_LOG("购买物品");
    shopi = (sitem_t*)malloc(sizeof(sitem_t));

    if (!shopi) {
        ERR_LOG("无法分配商店物品数据内存空间");
        ERR_LOG("%s", strerror(errno));
    }

    DBG_LOG("购买物品");
    memcpy(shopi, &c->game_data.shop_items[pkt->shop_item_index], sizeof(sitem_t));

    print_item_data(c, (item_t*)shopi);


    //if ((pkt->item_id > 1) && (shopi->data_b[0] != 0x03)) {
    //    ERR_LOG("GC %" PRIu32 " 购买函数有故障", c->guildcard);
    //    free(shopi);
    //    //return -1;
    //}
    //else
        //if (c->bb_pl->character.disp.meseta < shopi->costl)
        //{
        //    send_txt(c, "您的金钱不足以购买此商品.");
        //    ERR_LOG("GC %" PRIu32 " 的金钱不足以购买此商品 价格 %d 背包金钱 %d", c->guildcard, shopi->costl, c->bb_pl->character.disp.meseta);
        //    //free(shopi);
        //    //return -1;
        //}
        //else
        //{
            //DBG_LOG("购买物品");
            iitem_t *ii;

            ii = (iitem_t*)malloc(sizeof(iitem_t));

            memcpy(&ii->data.data_l[0], &shopi->data_l[0],
                sizeof(sitem_t));


            //DBG_LOG("购买物品");

            //subcmd_send_bb_create_item(c, /*ic, */ii->data/*, iitem_data.data2_l*/, 1);
            //DBG_LOG("购买物品");
            // Update player item ID
            l->item_player_id[c->client_id] = pkt->new_inv_item_id;
            //DBG_LOG("购买物品");
            ii->data.item_id = l->item_player_id[c->client_id]++;

            //DBG_LOG("购买物品");
            item_add_to_inv(c->bb_pl->inv.iitems,
                c->bb_pl->inv.item_count, ii);
            //DBG_LOG("购买物品");
            //Add_To_Inventory(&i, client->decrypt_buf_code[0x12], 1, client);
            subcmd_send_bb_delete_meseta(c, shopi->costl, 0);
            //DBG_LOG("购买物品");
        //}

    //DBG_LOG("购买物品");
    free_safe(shopi);
    free_safe(ii);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_item_tekk(ship_client_t* c, subcmd_bb_tekk_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    block_t* b = c->cur_block;
    subcmd_bb_tekk_identify_result_t i_res = { 0 };
    uint32_t tek_item_slot = 0, attrib = 0;
    uint8_t percent_mod;

    if (l->version == CLIENT_VERSION_BB) {
        if (c->bb_pl->character.disp.meseta < 100)
            return 0;

        if (l->type == LOBBY_TYPE_LOBBY) {
            ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
                c->guildcard);
            return -1;
        }

        if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
            ERR_LOG("GC %" PRIu32 " 发送损坏的物品鉴定数据!",
                c->guildcard);
            print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
            return -2;
        }

        tek_item_slot = item_get_inv_item_slot(c->bb_pl->inv, pkt->item_id);

        DBG_LOG("%04X %04X", tek_item_slot, pkt->item_id);

        print_item_data(c, &c->bb_pl->inv.iitems[tek_item_slot].data);

        if (c->bb_pl->inv.iitems[tek_item_slot].data.data_b[0] != 0x00) {
            ERR_LOG("GC %" PRIu32 " 发送无法鉴定的物品!",
                c->guildcard);
            return -3;
        }

        c->game_data.identify_result = c->bb_pl->inv.iitems[tek_item_slot];
        attrib = c->game_data.identify_result.data.data_b[4] & ~(0x80);

        if (attrib < 0x29)
        {
            c->game_data.identify_result.data.data_b[4] = tekker_attributes[(attrib * 3) + 1];
            if ((mt19937_genrand_int32(&b->rng) % 100) > 70)
                c->game_data.identify_result.data.data_b[4] += mt19937_genrand_int32(&b->rng) % ((tekker_attributes[(attrib * 3) + 2] - tekker_attributes[(attrib * 3) + 1]) + 1);
        }
        else
            c->game_data.identify_result.data.data_b[4] = 0;

        if ((mt19937_genrand_int32(&b->rng) % 10) < 2) percent_mod = -10;
        else
            if ((mt19937_genrand_int32(&b->rng) % 10) < 2) percent_mod = -5;
            else
                if ((mt19937_genrand_int32(&b->rng) % 10) < 2) percent_mod = 5;
                else
                    if ((mt19937_genrand_int32(&b->rng) % 10) < 2) percent_mod = 10;
                    else
                        percent_mod = 0;

        if ((!(c->bb_pl->inv.iitems[tek_item_slot].data.data_b[6] & 128)) && (c->bb_pl->inv.iitems[tek_item_slot].data.data_b[7] > 0))
            c->game_data.identify_result.data.data_b[7] += percent_mod;

        if ((!(c->bb_pl->inv.iitems[tek_item_slot].data.data_b[8] & 128)) && (c->bb_pl->inv.iitems[tek_item_slot].data.data_b[9] > 0))
            c->game_data.identify_result.data.data_b[9] += percent_mod;

        if ((!(c->bb_pl->inv.iitems[tek_item_slot].data.data_b[10] & 128)) && (c->bb_pl->inv.iitems[tek_item_slot].data.data_b[11] > 0))
            c->game_data.identify_result.data.data_b[11] += percent_mod;

        if (!c->game_data.identify_result.data.item_id) {
            ERR_LOG("GC %" PRIu32 " 未发送需要鉴定的物品!",
                c->guildcard);
            return -4;
        }

        if (c->game_data.identify_result.data.item_id != pkt->item_id) {
            ERR_LOG("GC %" PRIu32 " 接受的物品ID与以前请求的物品ID不匹配 !",
                c->guildcard);
            return -5;
        }

        subcmd_send_bb_delete_meseta(c, 100, 0);

        /* 填充数据头 */
        i_res.hdr.pkt_type = GAME_COMMAND0_TYPE;
        i_res.hdr.pkt_len = LE16(sizeof(i_res));
        i_res.hdr.flags = 0x00000000;
        i_res.shdr.type = SUBCMD60_UNKNOW_B9;
        i_res.shdr.size = sizeof(i_res) / 4;
        i_res.shdr.client_id = c->client_id;

        /* 填充数据 */
        i_res.item = c->game_data.identify_result.data;

        c->drop_item = i_res.item.item_id;
        c->drop_amt = 1;

        return send_pkt_bb(c, (bb_pkt_hdr_t*)&i_res);
    }
    else
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

}

static int handle_bb_item_tekked(ship_client_t* c, subcmd_bb_accept_item_identification_t* pkt) {
    lobby_t* l = c->cur_lobby;
    block_t* b = c->cur_block;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (!c->game_data.identify_result.data.item_id) {
        ERR_LOG("GC %" PRIu32 " 没有鉴定任何装备!",
            c->guildcard);
        return -1;
    }

    return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

    /* TODO 未完成*/
    uint32_t ch2, ch;

    for (ch = 0; ch < 4; ch++) {
        if (/*(l->slot_use[ch]) && (*/l->clients[ch]/*)*/) {
            for (ch2 = 0; ch2 < l->clients[ch]->pl->bb.inv.item_count; ch2++)
                if (l->clients[ch]->pl->bb.inv.iitems[ch2].data.item_id == c->game_data.identify_result.data.item_id) {
                    //Message_Box(L"", L"Item duplication attempt!", client, BigMes_Pkt1A, NULL, 94);
                    ERR_LOG("GC %" PRIu32 " 没有鉴定任何装备或该装备来自非法所得!",
                        c->guildcard);
                    return -1;
                }
        }
    }

    for (ch = 0; ch < l->item_count; l++) {
        uint32_t itemNum = l->item_list[ch];
        if (l->item_id_to_lobby_item[itemNum].inv_item.data.item_id == c->game_data.identify_result.data.item_id) {
            // Added to the game's inventory by the client...
            // Delete it and avoid duping...
            //被客户端添加到游戏目录中。。。
            //删除它并避免复制。。
            memset(&l->item_id_to_lobby_item[itemNum], 0, sizeof(fitem_t));
            l->item_list[ch] = EMPTY_STRING;
            break;
        }
    }

    clear_lobby_item(l);
    subcmd_send_bb_create_item(c, c->game_data.identify_result.data, 1);
    //Add_To_Inventory(&client->tekked, 1, 1, client);
    memset(&c->game_data.identify_result, 0, sizeof(iitem_t));

    return 0;
}

static int handle_bb_bank(ship_client_t* c, subcmd_bb_bank_open_t* req) {
    lobby_t* l = c->cur_lobby;
    uint8_t* sendbuf = get_sendbuf();
    subcmd_bb_bank_inv_t* pkt = (subcmd_bb_bank_inv_t*)sendbuf;
    uint32_t num_items = LE32(c->bb_pl->bank.item_count);
    uint16_t size = sizeof(subcmd_bb_bank_inv_t) + num_items *
        sizeof(bitem_t);
    block_t* b = c->cur_block;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅打开了银行!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (req->hdr.pkt_len != LE16(0x10) || req->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送错误的银行数据包!",
            c->guildcard);
        return -1;
    }

    /* Clean up the user's bank first... */
    cleanup_bb_bank(c);

    /* 填充数据并准备发送 */
    pkt->hdr.pkt_len = LE16(size);
    pkt->hdr.pkt_type = LE16(GAME_COMMANDC_TYPE);
    pkt->hdr.flags = 0;
    pkt->shdr.type = SUBCMD60_BANK_INV;
    pkt->shdr.size = pkt->shdr.size;
    pkt->shdr.unused = 0x0000;
    pkt->size = LE32(size);
    pkt->checksum = mt19937_genrand_int32(&b->rng); /* Client doesn't care */
    memcpy(&pkt->item_count, &c->bb_pl->bank, sizeof(psocn_bank_t));

    return crypt_send(c, (int)size, sendbuf);
}

static int handle_bb_bank_action(ship_client_t* c, subcmd_bb_bank_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t item_id;
    uint32_t amt, bank, iitem_count, i;
    int found = -1, stack, isframe = 0;
    iitem_t iitem = { 0 };
    bitem_t bitem = { 0 };
    uint32_t ic[3] = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " did bank action in lobby!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0018) || pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " sent bad bank action!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    switch (pkt->action) {
    case SUBCMD62_BANK_ACT_CLOSE:
    case SUBCMD62_BANK_ACT_DONE:
        return 0;

    case SUBCMD62_BANK_ACT_DEPOSIT:
        item_id = LE32(pkt->item_id);

        /* Are they depositing meseta or an item? */
        if (item_id == 0xFFFFFFFF) {
            amt = LE32(pkt->meseta_amount);
            iitem_count = LE32(c->bb_pl->character.disp.meseta);

            /* Make sure they aren't trying to do something naughty... */
            if (amt > iitem_count) {
                ERR_LOG("GC %" PRIu32 " depositing more "
                    "meseta than they have!", c->guildcard);
                return -1;
            }

            bank = LE32(c->bb_pl->bank.meseta);
            if (amt + bank > 999999) {
                ERR_LOG("GC %" PRIu32 " depositing too much "
                    "money at the bank!", c->guildcard);
                return -1;
            }

            c->bb_pl->character.disp.meseta = LE32((iitem_count - amt));
            c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
            c->bb_pl->bank.meseta = LE32((bank + amt));

            /* No need to tell everyone else, I guess? */
            return 0;
        }
        else {
            /* Look for the item in the user's inventory. */
            iitem_count = c->bb_pl->inv.item_count;
            for (i = 0; i < iitem_count; ++i) {
                if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
                    iitem = c->bb_pl->inv.iitems[i];
                    found = i;

                    /* If it is an equipped frame, we need to unequip all
                       the units that are attached to it. */
                    if (iitem.data.data_b[0] == ITEM_TYPE_GUARD &&
                        iitem.data.data_b[1] == ITEM_SUBTYPE_FRAME &&
                        (iitem.flags & LE32(0x00000008))) {
                        isframe = 1;
                    }

                    break;
                }
            }

            /* If the item isn't found, then punt the user from the ship. */
            if (found == -1) {
                ERR_LOG("GC %" PRIu32 " banked item that "
                    "they do not have!", c->guildcard);
                return -1;
            }

            stack = item_is_stackable(iitem.data.data_l[0]);

            if (!stack && pkt->item_amount > 1) {
                ERR_LOG("GC %" PRIu32 " banking multiple of "
                    "a non-stackable item!", c->guildcard);
                return -1;
            }

            found = item_remove_from_inv(c->bb_pl->inv.iitems, iitem_count,
                pkt->item_id, pkt->item_amount);

            if (found < 0 || found > 1) {
                ERR_LOG("GC %u Error removing item from inventory for "
                    "banking!", c->guildcard);
                return -1;
            }

            c->bb_pl->inv.item_count = (iitem_count -= found);

            /* Fill in the bank item. */
            if (stack) {
                iitem.data.data_b[5] = pkt->item_amount;
                bitem.amount = LE16(pkt->item_amount);
            }
            else {
                bitem.amount = LE16(1);
            }

            bitem.flags = LE16(1);
            bitem.data.data_l[0] = iitem.data.data_l[0];
            bitem.data.data_l[1] = iitem.data.data_l[1];
            bitem.data.data_l[2] = iitem.data.data_l[2];
            bitem.data.item_id = iitem.data.item_id;
            bitem.data.data2_l = iitem.data.data2_l;

            /* Unequip any units, if the item was equipped and a frame. */
            if (isframe) {
                for (i = 0; i < iitem_count; ++i) {
                    iitem = c->bb_pl->inv.iitems[i];
                    if (iitem.data.data_b[0] == ITEM_TYPE_GUARD &&
                        iitem.data.data_b[1] == ITEM_SUBTYPE_UNIT) {
                        c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
                    }
                }
            }

            /* 存入! */
            if (item_deposit_to_bank(c, &bitem) < 0) {
                ERR_LOG("Error depositing to bank for guildcard %"
                    PRIu32 "!", c->guildcard);
                return -1;
            }

            return subcmd_send_bb_destroy_item(c, iitem.data.item_id,
                pkt->item_amount);
        }

    case SUBCMD62_BANK_ACT_TAKE:
        item_id = LE32(pkt->item_id);

        /* Are they taking meseta or an item? */
        if (item_id == 0xFFFFFFFF) {
            amt = LE32(pkt->meseta_amount);
            iitem_count = LE32(c->bb_pl->character.disp.meseta);

            /* Make sure they aren't trying to do something naughty... */
            if (amt + iitem_count > 999999) {
                ERR_LOG("GC %" PRIu32 " 从银行取出的美赛塔超出了存储限制!", c->guildcard);
                return -1;
            }

            bank = LE32(c->bb_pl->bank.meseta);

            if (amt > bank) {
                ERR_LOG("GC %" PRIu32 " 从银行取出的美赛塔超出了银行库存!", c->guildcard);
                return -1;
            }

            c->bb_pl->character.disp.meseta = LE32((iitem_count + amt));
            c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
            c->bb_pl->bank.meseta = LE32((bank - amt));

            /* 存取美赛塔不用告知其他客户端... */
            return 0;
        }
        else {
            /* 尝试从银行中取出物品. */
            found = item_take_from_bank(c, pkt->item_id, pkt->item_amount, &bitem);

            if (found < 0) {
                ERR_LOG("GC %" PRIu32 " taking invalid item "
                    "from bank!", c->guildcard);
                return -1;
            }

            /* 已获得银行的物品数据, 将其添加至临时背包数据中... */
            iitem.present = LE16(0x0001);
            iitem.tech = LE16(0x0000);
            iitem.flags = 0;
            ic[0] = iitem.data.data_l[0] = bitem.data.data_l[0];
            ic[1] = iitem.data.data_l[1] = bitem.data.data_l[1];
            ic[2] = iitem.data.data_l[2] = bitem.data.data_l[2];
            iitem.data.item_id = LE32(l->item_next_lobby_id);
            iitem.data.data2_l = bitem.data.data2_l;
            ++l->item_next_lobby_id;

            /* 新增至玩家背包中... */
            found = item_add_to_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count, &iitem);

            if (found < 0) {
                /* Uh oh... Guess we should put it back in the bank... */
                item_deposit_to_bank(c, &bitem);
                return -1;
            }

            c->bb_pl->inv.item_count += found;

            /* 发送至房间中的客户端. */
            return subcmd_send_bb_create_item(c, iitem.data, 1);
        }

    default:
        ERR_LOG("GC %" PRIu32 " 发送未知银行操作: %d!",
            c->guildcard, pkt->action);
        print_payload((unsigned char*)pkt, 0x18);
        return -1;
    }
}

static int handle_bb_62_check_game_loading(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间内指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size)
        return -1;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_burst_pldata(ship_client_t* c, ship_client_t* d,
    subcmd_bb_burst_pldata_t* pkt) {
    int i, rv = 0;
    iitem_t* item;
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅中触发传送中的玩家数据!", c->guildcard);
        return -1;
    }

    if ((c->version == CLIENT_VERSION_XBOX && d->version == CLIENT_VERSION_GC) ||
        (d->version == CLIENT_VERSION_XBOX && c->version == CLIENT_VERSION_GC)) {
        /* Scan the inventory and fix any mags before sending it along. */


        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* If the item is a mag, then we have to swap the last dword of the
                data. Otherwise colors and stats get messed up. */
            if (item->data.data_b[0] == ITEM_TYPE_MAG) {
                item->data.data2_l = SWAP32(item->data.data2_l);
            }
        }
    }
    else {

        pkt->shdr.size = pkt->hdr.pkt_len / 4;

        pkt->guildcard = c->guildcard;

        // Check techniques...检查魔法,如果是机器人就不该有魔法
        if (!(c->equip_flags & EQUIP_FLAGS_DROID)) {
            for (i = 0; i < 19; i++) {
                if (pkt->techniques[i] > max_tech_level[i][c->bb_pl->character.disp.dress_data.ch_class]) {
                    (char)c->bb_pl->character.techniques[i] = -1; // Unlearn broken technique.忘掉损坏的技能
                    rv = -1;
                }
            }

            if (rv)
                ERR_LOG("GC %u 是机器人就不该有魔法! 标签值 %02X 错误码 %d", c->equip_flags, c->guildcard, rv);
        }

        memcpy(&pkt->techniques[0], &c->bb_pl->character.techniques, sizeof(c->bb_pl->character.techniques));

        // Could check character structure here 可以查看角色结构
        //memcpy(&pkt->dress_data, &c->bb_pl->character.disp.dress_data, sizeof(psocn_dress_data_t));
        pkt->dress_data = c->bb_pl->character.disp.dress_data;

        memcpy(&pkt->name[0], &c->bb_pl->character.name[0], sizeof(c->bb_pl->character.name));

        // Prevent crashing with NPC skins... 防止NPC皮肤崩溃
        if (c->bb_pl->character.disp.dress_data.v2flags) {
            pkt->dress_data.v2flags = 0x00;
            pkt->dress_data.version = 0x00;
            pkt->dress_data.v1flags = LE32(0x00000000);
            pkt->dress_data.costume = LE16(0x0000);
            pkt->dress_data.skin = LE16(0x0000);
        }

        /* 检测人物基础数值 */
        pkt->stats.atp = c->pl->bb.character.disp.stats.atp;
        pkt->stats.mst = c->pl->bb.character.disp.stats.mst;
        pkt->stats.evp = c->pl->bb.character.disp.stats.evp;
        pkt->stats.hp = c->pl->bb.character.disp.stats.hp;
        pkt->stats.dfp = c->pl->bb.character.disp.stats.dfp;
        pkt->stats.ata = c->pl->bb.character.disp.stats.ata;
        pkt->stats.lck = c->pl->bb.character.disp.stats.lck;

        for (i = 0; i < 10; i++)
            pkt->opt_flag[i] = c->bb_pl->character.disp.opt_flag[i];

        pkt->level = c->pl->bb.character.disp.level;
        pkt->exp = c->pl->bb.character.disp.exp;
        pkt->meseta = c->pl->bb.character.disp.meseta;


        // Could check inventory here 查看背包
        pkt->inv.item_count = c->bb_pl->inv.item_count;

        for (i = 0; i < 30; i++)
            memcpy(&pkt->inv.iitems[i], &c->bb_pl->inv.iitems[i], sizeof(iitem_t));

        for (i = 0; i < 4; i++)
            memset(&pkt->unused[i], 0, sizeof(uint32_t));
    }

    printf("%" PRIu32 " - %" PRIu32 "  %s \n", c->guildcard, pkt->guildcard, pkt->dress_data.guildcard_string);

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

static int handle_bb_warp_item(ship_client_t* c, subcmd_bb_warp_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t iitem_count, found, i;
    uint32_t wrap_id;
    iitem_t backup_item;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    memset(&backup_item, 0, sizeof(iitem_t));
    wrap_id = *(uint32_t*)&pkt->data[0x0C];

    for (i = 0; i < c->bb_pl->inv.item_count; i++) {
        if (c->bb_pl->inv.iitems[i].data.item_id == wrap_id) {
            memcpy(&backup_item, &c->bb_pl->inv.iitems[i], sizeof(iitem_t));
            break;
        }
    }

    if (backup_item.data.item_id) {
        //stack = item_is_stackable(backup_item.data.data_l[0]);

        //if (!stack && pkt->item_amount > 1) {
        //    ERR_LOG("GC %" PRIu32 " banking multiple of "
        //        "a non-stackable item!", c->guildcard);
        //    return -1;
        //}

        iitem_count = c->bb_pl->inv.item_count;

        if ((found = item_remove_from_inv(c->bb_pl->inv.iitems, iitem_count,
            backup_item.data.item_id, 1)) < 1) {
            ERR_LOG("无法从玩家背包中移除物品!");
            return -1;
        }

        if (found < 0 || found > 1) {
            ERR_LOG("GC %u Error removing item from inventory for "
                "banking!", c->guildcard);
            return -1;
        }

        c->bb_pl->inv.item_count = (iitem_count -= found);

        if (backup_item.data.data_b[0] == 0x02)
            backup_item.data.data2_b[2] |= 0x40; // Wrap a mag
        else
            backup_item.data.data_b[4] |= 0x40; // Wrap other

        /* 将物品新增至背包. */
        found = item_add_to_inv(c->bb_pl->inv.iitems,
            c->bb_pl->inv.item_count, &backup_item);

        if (found == -1) {
            ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法拾取!",
                c->guildcard);
            //return -1;
        }

        c->bb_pl->inv.item_count += found;
        memcpy(&c->pl->bb.inv, &c->bb_pl->inv, sizeof(iitem_t));
    }
    else {
        ERR_LOG("GC %" PRIu32 " 传送物品ID %d 失败!",
            c->guildcard, wrap_id);
        return -1;
    }

    return 0;
}

static int handle_bb_quest_oneperson_set_ex_pc(ship_client_t* c, subcmd_bb_quest_oneperson_set_ex_pc_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t iitem_count, found;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if ((l->oneperson) && (l->flags & LOBBY_FLAG_QUESTING) && (!l->drops_disabled)) {
        iitem_t tmp_iitem;
        iitem_count = c->bb_pl->inv.item_count;

        memset(&tmp_iitem, 0, sizeof(iitem_t));
        tmp_iitem.data.data_b[0] = 0x03;
        tmp_iitem.data.data_b[1] = 0x10;
        tmp_iitem.data.data_b[2] = 0x02;
        
        found = item_remove_from_inv(c->bb_pl->inv.iitems, iitem_count, 0, 1);

        if (found < 0 || found > 1) {
            ERR_LOG("GC %u Error handle_bb_quest_oneperson_set_ex_pc!", c->guildcard);
            return -1;
        }

        c->bb_pl->inv.item_count = (iitem_count -= found);

        if (!found) {
            l->drops_disabled = 1;
            return -1;
        }
    }

    return 0;
}

/* 处理BB 0x62/0x6D 数据包. */
int subcmd_bb_handle_one(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* dest;
    uint8_t type = pkt->type;
    uint8_t size = pkt->size;
    int rv = -1;
    uint32_t dnum = LE32(pkt->hdr.flags);

    /* Ignore these if the client isn't in a lobby. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    /* Find the destination. */
    dest = l->clients[dnum];

    /* The destination is now offline, don't bother sending it. */
    if (!dest) {
        pthread_mutex_unlock(&l->mutex);
        return 0;
    }

    subcmd_bb_626Dsize_check(c, pkt);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;

        switch (type) {
        case SUBCMD62_BURST1://6D
        case SUBCMD62_BURST2://6B
        case SUBCMD62_BURST3://6C
        case SUBCMD62_BURST4://6E
            if (l->flags & LOBBY_FLAG_QUESTING)
                rv = lobby_enqueue_burst_bb(l, c, (bb_pkt_hdr_t*)pkt);
            /* Fall through... */

        case SUBCMD62_BURST5://6F
        case SUBCMD62_BURST6://71
        //case SUBCMD62_BURST_PLDATA://70
            rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            break;

        case SUBCMD62_BURST_PLDATA://70
            rv = handle_bb_burst_pldata(c, dest, (subcmd_bb_burst_pldata_t*)pkt);
            break;

        default:
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {
    case SUBCMD62_GUILDCARD:
        rv = handle_bb_gcsend(c, dest);
        break;

    case SUBCMD62_PICK_UP:
        rv = handle_bb_pick_up(c, (subcmd_bb_pick_up_t*)pkt);
        break;

    case SUBCMD62_ITEMREQ:
        rv = l->dropfunc(c, l, pkt);
        //rv = handle_bb_item_req(c, dest, (subcmd_bb_itemreq_t*)pkt);
        break;

    case SUBCMD62_BITEMREQ:
        rv = l->dropfunc(c, l, pkt);
        //rv = handle_bb_bitem_req(c, dest, (subcmd_bb_bitemreq_t*)pkt);
        break;

    case SUBCMD62_TRADE:
        rv = handle_bb_trade(c, dest, (subcmd_bb_trade_t*)pkt);
        break;

        // Barba Ray actions
    case SUBCMD62_UNKNOW_A9:
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD62_CHARACTER_INFO:
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD62_SHOP_REQ:
        rv = handle_bb_shop_req(c, (subcmd_bb_shop_req_t*)pkt);
        break;

    case SUBCMD62_SHOP_BUY:
        rv = handle_bb_shop_buy(c, (subcmd_bb_shop_buy_t*)pkt);
        break;

    case SUBCMD62_TEKKING:
        rv = handle_bb_item_tekk(c, (subcmd_bb_tekk_item_t*)pkt);
        break;

    case SUBCMD62_TEKKED:
        rv = handle_bb_item_tekked(c, (subcmd_bb_accept_item_identification_t*)pkt);
        //rv = send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD62_OPEN_BANK:
        rv = handle_bb_bank(c, (subcmd_bb_bank_open_t*)pkt);
        break;

    case SUBCMD62_BANK_ACTION:
        rv = handle_bb_bank_action(c, (subcmd_bb_bank_act_t*)pkt);
        break;

    case SUBCMD62_GUILD_INVITE1:
    case SUBCMD62_GUILD_INVITE2:
    case SUBCMD62_GUILD_MASTER_TRANS1:
    case SUBCMD62_GUILD_MASTER_TRANS2:
        // guild invite for C1 & C2, Master Transfer for CD & CE.
        if (size == 0x64)
            rv = 0;

        if (type == SUBCMD62_GUILD_INVITE2)//判断是公会邀请
        {
            uint32_t gcn;

            gcn = *(uint32_t*)&pkt->data[0x02];

            if ((pkt->data[0x06] == 0x02) && //0x02 应该时接受公会邀请指令
                (c->guildcard == gcn))
                c->guild_accept = 1;
        }

        if (type == SUBCMD62_GUILD_MASTER_TRANS1)//判断是会长转让功能
        {
            if (c->bb_guild->guild_data.guild_priv_level != 0x40) {
                rv = 1;
                DBG_LOG("GC %u 公会权限不足", c->guildcard);
                //Message_Box(L"", L"You aren't the master of your team.", client, BRMes_Pkt01, NULL, 133);
            }
            else
                c->guild_master_exfer = 1;
        }
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD62_CH_GRAVE_DATA:
        UNK_CSPD(type, c->version, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD60_SRANK_ATTR:
        UNK_CSPD(type, c->version, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD60_EX_ITEM_MK:
        UNK_CSPD(type, c->version, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD62_QUEST_ONEPERSON_SET_BP:
        UNK_CSPD(type, c->version, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD62_WARP_ITEM:
        rv = handle_bb_warp_item(dest, (subcmd_bb_warp_item_t*)pkt);
        break;


    case SUBCMD62_QUEST_ONEPERSON_SET_EX_PC:
        rv = handle_bb_quest_oneperson_set_ex_pc(dest, (subcmd_bb_quest_oneperson_set_ex_pc_t*)pkt);
        break;


    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x62/0x6D 指令: 0x%02X", type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
#endif /* BB_LOG_UNKNOWN_SUBS */
        /* Forward the packet unchanged to the destination. */
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);

    case SUBCMD62_QUEST_ITEM_UNKNOW1:
    case SUBCMD62_QUEST_ITEM_RECEIVE:
    case SUBCMD62_BATTLE_CHAR_LEVEL_FIX:
    case SUBCMD62_GANBLING:
        //UNK_CSPD(type, c->version, pkt);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

///////////////////////////////////////////////////////////////////////////////////////////////

static int handle_bb_switch_changed(ship_client_t* c, subcmd_bb_switch_changed_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅开启了机关!",
            c->guildcard);
        rv = -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->data.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            c->guildcard, pkt->data.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    rv = subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);

    if (pkt->data.flags && pkt->data.object_id != 0xFFFF) {
        if ((l->flags & LOBBY_TYPE_CHEATS_ENABLED) && c->options.switch_assist &&
            (c->last_switch_enabled_command.type == 0x05)) {
            DBG_LOG("[机关助手] 重复启动上一个命令");
            subcmd_bb_switch_changed_pkt_t* gem = pkt;
            gem->data = c->last_switch_enabled_command;
            rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)gem, 0);
        }
        c->last_switch_enabled_command = pkt->data;
    }

    return rv;
}

static void handle_bb_objhit_common(ship_client_t* c, lobby_t* l, uint16_t bid) {
    uint32_t obj_type;

    /* What type of object was hit? */
    if ((bid & 0xF000) == 0x4000) {
        /* An object was hit */
        bid &= 0x0FFF;

        /* Make sure the object is in range. */
        if (bid > l->map_objs->count) {
            ERR_LOG("GC %" PRIu32 " 攻击了无效的实例 "
                "(%d -- 地图实例数量: %d)!\n"
                "章节: %d, 层级: %d, 地图: (%d, %d)", c->guildcard,
                bid, l->map_objs->count, l->episode, c->cur_area,
                l->maps[c->cur_area << 1],
                l->maps[(c->cur_area << 1) + 1]);

            if ((l->flags & LOBBY_FLAG_QUESTING))
                ERR_LOG("任务 ID: %d, 版本: %d", l->qid,
                    l->version);

            /* Just continue on and hope for the best. We've probably got a
               bug somewhere on our end anyway... */
            return;
        }

        /* Make sure it isn't marked as hit already. */
        if ((l->map_objs->objs[bid].flags & 0x80000000))
            return;

        /* Now, see if we care about the type of the object that was hit. */
        obj_type = l->map_objs->objs[bid].data.skin & 0xFFFF;

        /* We'll probably want to do a bit more with this at some point, but
           for now this will do. */
        switch (obj_type) {
        case OBJ_SKIN_REG_BOX:
        case OBJ_SKIN_FIXED_BOX:
        case OBJ_SKIN_RUINS_REG_BOX:
        case OBJ_SKIN_RUINS_FIXED_BOX:
        case OBJ_SKIN_CCA_REG_BOX:
        case OBJ_SKIN_CCA_FIXED_BOX:
            /* Run the box broken script. */
            script_execute(ScriptActionBoxBreak, c, SCRIPT_ARG_PTR, c,
                SCRIPT_ARG_UINT16, bid, SCRIPT_ARG_UINT16,
                obj_type, SCRIPT_ARG_END);
            break;
        }

        /* Mark it as hit. */
        l->map_objs->objs[bid].flags |= 0x80000000;
    }
    else if ((bid & 0xF000) == 0x1000) {
        /* An enemy was hit. We don't really do anything with these here,
           as there is a better packet for handling them. */
        return;
    }
    else if ((bid & 0xF000) == 0x0000) {
        /* An ally was hit. We don't really care to do anything here. */
        return;
    }
    else {
        DBG_LOG("Unknown object type hit: %04" PRIx16 "", bid);
    }
}

static int handle_bb_objhit_phys(ship_client_t* c, subcmd_bb_objhit_phys_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t pkt_size = pkt->hdr.pkt_len;
    uint8_t size = pkt->shdr.size;
    uint32_t hit_count = pkt->hit_count;
    uint8_t i;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了实例!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len == LE16(0x0010)) {
        /* 对空气进行攻击 */
        return 0;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt_size != (sizeof(bb_pkt_hdr_t) + (size << 2)) || size < 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的普通攻击数据! %d %d hit_count %d",
            c->guildcard, pkt_size, (sizeof(bb_pkt_hdr_t) + (size << 2)), hit_count);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Handle each thing that was hit */
    for (i = 0; i < pkt->shdr.size - 2; ++i)
        handle_bb_objhit_common(c, l, LE16(pkt->objects[i].obj_id));

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static inline int tlindex(uint8_t l) {
    switch (l) {
    case 0: case 1: case 2: case 3: case 4: return 0;
    case 5: case 6: case 7: case 8: case 9: return 1;
    case 10: case 11: case 12: case 13: case 14: return 2;
    case 15: case 16: case 17: case 18: case 19: return 3;
    case 20: case 21: case 22: case 23: case 25: return 4;
    default: return 5;
    }
}

#define BARTA_TIMING 1500
#define GIBARTA_TIMING 2200

static const uint16_t gifoie_timing[6] = { 5000, 6000, 7000, 8000, 9000, 10000 };
static const uint16_t gizonde_timing[6] = { 1000, 900, 700, 700, 700, 700 };
static const uint16_t rafoie_timing[6] = { 1500, 1400, 1300, 1200, 1100, 1100 };
static const uint16_t razonde_timing[6] = { 1200, 1100, 950, 800, 750, 750 };
static const uint16_t rabarta_timing[6] = { 1200, 1100, 950, 800, 750, 750 };

static int handle_bb_objhit_tech(ship_client_t* c, subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t tech_level;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅用法术攻击实例!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len == LE16(0x0010)) {
        /* 对空气进行攻击 */
        return 0;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的法术攻击数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* 合理性检查... Does the character have that level of technique? */
    tech_level = c->pl->v1.character.techniques[pkt->tech];
    if (tech_level == 0xFF) {
        /* This might happen if the user learns a new tech in a team. Until we
           have real inventory tracking, we'll have to fudge this. Once we have
           real, full inventory tracking, this condition should probably
           disconnect the client */
        tech_level = pkt->level;
    }

    if (c->version >= CLIENT_VERSION_DCV2)
        tech_level += c->pl->v1.inv.iitems[pkt->tech].tech;

    if (tech_level < pkt->level) {
        /* This might happen if the user learns a new tech in a team. Until we
           have real inventory tracking, we'll have to fudge this. Once we have
           real, full inventory tracking, this condition should probably
           disconnect the client */
        tech_level = pkt->level;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* See what technique was used... */
    switch (pkt->tech) {
        /* These work just like physical hits and can only hit one target, so
           handle them the simple way... */
    case TECHNIQUE_FOIE:
    case TECHNIQUE_ZONDE:
    case TECHNIQUE_GRANTS:
        if (pkt->shdr.size == 3)
            handle_bb_objhit_common(c, l, LE16(pkt->objects[0].obj_id));
        break;

        /* None of these can hit boxes (which is all we care about right now), so
           don't do anything special with them. */
    case TECHNIQUE_DEBAND:
    case TECHNIQUE_JELLEN:
    case TECHNIQUE_ZALURE:
    case TECHNIQUE_SHIFTA:
    case TECHNIQUE_RYUKER:
    case TECHNIQUE_RESTA:
    case TECHNIQUE_ANTI:
    case TECHNIQUE_REVERSER:
    case TECHNIQUE_MEGID:
        break;

        /* AoE spells are... special (why, Sega?). They never have more than one
           item hit in the packet, and just act in a broken manner in general. We
           have to do some annoying stuff to handle them here. */
    case TECHNIQUE_BARTA:
        c->aoe_timer = get_ms_time() + BARTA_TIMING;
        break;

    case TECHNIQUE_GIBARTA:
        c->aoe_timer = get_ms_time() + GIBARTA_TIMING;
        break;

    case TECHNIQUE_GIFOIE:
        c->aoe_timer = get_ms_time() + gifoie_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RAFOIE:
        c->aoe_timer = get_ms_time() + rafoie_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_GIZONDE:
        c->aoe_timer = get_ms_time() + gizonde_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RAZONDE:
        c->aoe_timer = get_ms_time() + razonde_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RABARTA:
        c->aoe_timer = get_ms_time() + rabarta_timing[tlindex(tech_level)];
        break;

    default:
        ERR_LOG("GC %" PRIu32 " 使用了损坏的法术: %d",
            c->guildcard, (int)pkt->tech);
        return -1;
    }

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_objhit(ship_client_t* c, subcmd_bb_bhit_pkt_t* pkt) {
    uint64_t now = get_ms_time();
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了箱子!",
            c->guildcard);
        return -1;
    }

    /* We only care about these if the AoE timer is set on the sender. */
    if (c->aoe_timer < now)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Check the type of the object that was hit. As the AoE timer can't be set
       if the objects aren't loaded, this shouldn't ever trip up... */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Handle the object marked as hit, if appropriate. */
    handle_bb_objhit_common(c, l, LE16(pkt->shdr.object_id));

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_symbol_chat(ship_client_t* c, subcmd_bb_symbol_chat_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT)
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));

    /* Don't send chats for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU))
        return 0;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 1);
}

static int handle_bb_dragon_act(ship_client_t* c, subcmd_bb_dragon_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int v = c->version, i;
    subcmd_bb_dragon_act_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " reported Dragon action in "
            "lobby!\n", c->guildcard);
        return -1;
    }

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_bb_dragon_act_t));
    tr.x.b = SWAP32(tr.x.b);
    tr.z.b = SWAP32(tr.z.b);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            if (l->clients[i]->version == v) {
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)pkt);
            }
            else {
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&tr);
            }
        }
    }

    return 0;
}

static int handle_bb_gol_dragon_act(ship_client_t* c, subcmd_bb_gol_dragon_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int v = c->version, i;
    subcmd_bb_gol_dragon_act_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " reported Gol Dragon action in "
            "lobby!\n", c->guildcard);
        return -1;
    }

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_bb_gol_dragon_act_t));
    tr.x.b = SWAP32(tr.x.b);
    tr.z.b = SWAP32(tr.z.b);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            if (l->clients[i]->version == v) {
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)pkt);
            }
            else {
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&tr);
            }
        }
    }

    return 0;
}

static int handle_bb_charge_act(ship_client_t* c, subcmd_bb_charge_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int32_t meseta = *(int32_t*)pkt->mst;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中获取经验!",
            c->guildcard);
        return -1;
    }

    if (meseta > 0)
    {
        if (c->bb_pl->character.disp.meseta >= (uint32_t)meseta)
            subcmd_send_bb_delete_meseta(c, meseta, 0);
        else
            subcmd_send_bb_delete_meseta(c, c->bb_pl->character.disp.meseta, 0);
    }
    else
    {
        meseta = -meseta;
        c->bb_pl->character.disp.meseta += (uint32_t)meseta;
        if (c->bb_pl->character.disp.meseta > 999999)
            c->bb_pl->character.disp.meseta = 999999;
    }

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);
}

/* 从怪物获取的经验倍率未进行调整 */
static int handle_bb_req_exp(ship_client_t* c, subcmd_bb_req_exp_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t mid;
    uint32_t bp, exp_amount;
    game_enemy_t* en;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中获取经验!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的经验获取数据包!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Make sure the enemy is in range. */
    mid = LE16(pkt->enemy_id2);
    //DBG_LOG("怪物编号原值 %02X %02X", mid, pkt->enemy_id2);
    mid &= 0xFFF;
    //DBG_LOG("怪物编号新值 %02X map_enemies->count %02X", mid, l->map_enemies->count);

    if (mid > l->map_enemies->count) {
        ERR_LOG("GC %" PRIu32 " 杀掉了无效的敌人 (%d -- "
            "敌人数量: %d)!", c->guildcard, mid, l->map_enemies->count);
        return -1;
    }

    /* Make sure this client actually hit the enemy and that the client didn't
       already claim their experience. */
    en = &l->map_enemies->enemies[mid];

    if (!(en->clients_hit & (1 << c->client_id))) {
        return 0;
    }

    /* Set that the client already got their experience and that the monster is
       indeed dead. */
    en->clients_hit = (en->clients_hit & (~(1 << c->client_id))) | 0x80;

    /* Give the client their experience! */
    bp = en->bp_entry;
    exp_amount = l->bb_params[bp].exp + 100000;

    //DBG_LOG("经验原值 %d bp %d", exp_amount, bp);

    if ((c->game_data.expboost) && (l->exp_mult > 0))
        exp_amount = l->bb_params[bp].exp * l->exp_mult;

    if (!exp_amount)
        ERR_LOG("未获取到经验新值 %d bp %d 倍率 %d", exp_amount, bp, l->exp_mult);

    // TODO 新增房间共享经验 分别向其余3个玩家发送数值不等的经验值
    if (!pkt->last_hitter) {
        exp_amount = (exp_amount * 80) / 100;
    }

    return client_give_exp(c, exp_amount);
}

static int handle_bb_gallon_area(ship_client_t* c, subcmd_bb_gallon_area_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t quest_offset = *(uint32_t*)&pkt->pos[0];

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发任务指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    if (quest_offset >= 23) {
        UDONE_CSPD(pkt->hdr.pkt_type, c->version, pkt);
        return 0;
    }

    quest_offset *= 4;

    *(uint32_t*)&c->bb_pl->quest_data2[quest_offset] = *(uint32_t*)&pkt->pos[0];

    return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

}

static int handle_bb_mhit(ship_client_t* c, subcmd_bb_mhit_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t enemy_id2, enemy_id, dmg;
    game_enemy_t* en;
    uint32_t flags;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了怪物!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的怪物攻击数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Make sure the enemy is in range. */
    enemy_id = LE16(pkt->shdr.enemy_id);
    enemy_id2 = LE16(pkt->enemy_id2);
    dmg = LE16(pkt->damage);
    flags = LE32(pkt->flags);

    /* Swap the flags on the packet if the user is on GC... Looks like Sega
       decided that it should be the opposite order as it is on DC/PC/BB. */
    if (c->version == CLIENT_VERSION_GC)
        flags = SWAP32(flags);

    /* Bail out now if we don't have any enemy data on the team. */
    if (!l->map_enemies || l->challenge || l->battle) {
        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_UINT16, enemy_id2, SCRIPT_ARG_END);

        if (flags & 0x00000800)
            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                SCRIPT_ARG_UINT16, enemy_id2, SCRIPT_ARG_END);

        return send_lobby_mhit(l, c, enemy_id2, enemy_id, dmg, flags);
    }

    if (enemy_id2 > l->map_enemies->count) {
#ifdef DEBUG
        ERR_LOG("GC %" PRIu32 " 攻击了无效的怪物 (%d -- 地图怪物数量: "
            "%d)!"
            "章节: %d, 层级: %d, 地图: (%d, %d)", c->guildcard, enemy_id2,
            l->map_enemies->count, l->episode, c->cur_area,
            l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);

        if ((l->flags & LOBBY_FLAG_QUESTING))
            ERR_LOG("任务 ID: %d, 版本: %d", l->qid, l->version);
#endif

        if (l->logfp) {
            fdebug(l->logfp, DBG_WARN, "GC %" PRIu32 " 攻击了无效的怪物 (%d -- 地图怪物数量: %d)!\n"
                "章节: %d, 层级: %d, 地图: (%d, %d)\n", c->guildcard, enemy_id2,
                l->map_enemies->count, l->episode, c->cur_area,
                l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);

            if ((l->flags & LOBBY_FLAG_QUESTING))
                fdebug(l->logfp, DBG_WARN, "任务 ID: %d, 版本: %d\n",
                    l->qid, l->version);
        }

        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_UINT16, enemy_id2, SCRIPT_ARG_END);

        if (flags & 0x00000800)
            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                SCRIPT_ARG_UINT16, enemy_id2, SCRIPT_ARG_END);

        /* If server-side drops aren't on, then just send it on and hope for the
           best. We've probably got a bug somewhere on our end anyway... */
        if (!(l->flags & LOBBY_FLAG_SERVER_DROPS))
            return send_lobby_mhit(l, c, enemy_id2, enemy_id, dmg, flags);

        ERR_LOG("GC %" PRIu32 " 攻击了无效的怪物 (%d -- 地图怪物数量: "
            "%d)!", c->guildcard, enemy_id2, l->map_enemies->count);
        return -1;
    }

    /* Make sure it looks like they're in the right area for this... */
    /* XXXX: There are some issues still with Episode 2, so only spit this out
       for now on Episode 1. */
#ifdef DEBUG
    if (c->cur_area != l->map_enemies->enemies[enemy_id2].area && l->episode == 1 &&
        !(l->flags & LOBBY_FLAG_QUESTING)) {
        ERR_LOG("GC %" PRIu32 " 在无效区域攻击了怪物 "
            "(%d -- 地图怪物数量: %d)!\n 章节: %d, 区域: %d, 敌人数据区域: %d "
            "地图: (%d, %d)", c->guildcard, enemy_id2, l->map_enemies->count,
            l->episode, c->cur_area, l->map_enemies->enemies[enemy_id2].area,
            l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);
    }
#endif

    if (l->logfp && c->cur_area != l->map_enemies->enemies[enemy_id2].area &&
        !(l->flags & LOBBY_FLAG_QUESTING)) {
        fdebug(l->logfp, DBG_WARN, "GC %" PRIu32 " 在无效区域攻击了怪物 "
            "(%d -- 地图怪物数量: %d)!\n 章节: %d, 区域: %d, 敌人数据区域: %d "
            "地图: (%d, %d)", c->guildcard, enemy_id2, l->map_enemies->count,
            l->episode, c->cur_area, l->map_enemies->enemies[enemy_id2].area,
            l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);
    }

    /* Make sure the person's allowed to be on this floor in the first place. */
    if ((l->flags & LOBBY_FLAG_ONLY_ONE) && !(l->flags & LOBBY_FLAG_QUESTING)) {
        if (l->episode == 1) {
            switch (c->cur_area) {
            case 5:     /* Cave 3 */
            case 12:    /* De Rol Le */
                ERR_LOG("GC %" PRIu32 " 在无法接近的区域内击中敌人 单人模式 (区域 %d X:%f Y:%f) "
                    "房间 Flags: %08" PRIx32 "",
                    c->guildcard, c->cur_area, c->x, c->z, l->flags);
                break;
            }
        }
    }

    /* Save the hit, assuming the enemy isn't already dead. */
    en = &l->map_enemies->enemies[enemy_id2];
    if (!(en->clients_hit & 0x80)) {
        en->clients_hit |= (1 << c->client_id);
        en->last_client = c->client_id;

        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_UINT16, enemy_id2, SCRIPT_ARG_UINT32, en->bp_entry,
            SCRIPT_ARG_UINT8, en->rt_index, SCRIPT_ARG_UINT8,
            en->clients_hit, SCRIPT_ARG_END);

        /* If the kill flag is set, mark it as dead and update the client's
           counter. */
        if (flags & 0x00000800) {
            en->clients_hit |= 0x80;

            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                SCRIPT_ARG_UINT16, enemy_id2, SCRIPT_ARG_UINT32,
                en->bp_entry, SCRIPT_ARG_UINT8, en->rt_index,
                SCRIPT_ARG_UINT8, en->clients_hit, SCRIPT_ARG_END);

            if (en->bp_entry < 0x60 && !(l->flags & LOBBY_FLAG_HAS_NPC))
                ++c->enemy_kills[en->bp_entry];
        }
    }

    return send_lobby_mhit(l, c, enemy_id2, enemy_id, dmg, flags);
}

static int handle_bb_feed_mag(ship_client_t* c, subcmd_bb_feed_mag_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t item_id = pkt->item_id, mag_id = pkt->mag_id;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发游戏指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    DBG_LOG("GC %" PRIu32 " 使用物品ID 0x%04X 喂养玛古 ID 0x%04X!",
        c->guildcard, item_id, mag_id);

    if (mag_bb_feed(c, item_id, mag_id)) {
        ERR_LOG("GC %" PRIu32 " 喂养玛古发生错误!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_equip(ship_client_t* c, subcmd_bb_equip_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t inv, item_id = 0, found_item = 0, found_slot = 0, j = 0, slot[4] = { 0 };
    int i = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅更换装备!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误装备数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //DBG_LOG("物品数量 %d %d", c->bb_pl->inv.item_count, c->pl->bb.inv.item_count);
    /* Find the item and equip it. */
    inv = c->bb_pl->inv.item_count;
    item_id = pkt->item_id;

    i = item_get_inv_item_slot(c->bb_pl->inv, pkt->item_id);

    print_item_data(c, &c->bb_pl->inv.iitems[i].data);

    if (c->bb_pl->inv.iitems[i].data.item_id == item_id) {
        found_item = 1;

        switch (c->bb_pl->inv.iitems[i].data.data_b[0])
        {
        case ITEM_TYPE_WEAPON:
            DBG_LOG("武器识别");
            if (item_check_equip(
                pmt_weapon_bb[c->bb_pl->inv.iitems[i].data.data_b[1]][c->bb_pl->inv.iitems[i].data.data_b[2]]->equip_flag,
                c->equip_flags)) {
                ERR_LOG("GC %" PRIu32 " 装备了不属于该职业的物品数据!",
                    c->guildcard);
                send_msg1(c, "装备了不属于该职业的物品数据.");
                //return -1;
            }
            else  {
            // De-equip any other weapon on character. (Prevent stacking) 解除角色上任何其他武器的装备。（防止堆叠） 
            for (j = 0; j < inv; j++)
                if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_WEAPON) &&
                    (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {

                    c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                    //DBG_LOG("卸载武器");
                }
        }
            break;
        case ITEM_TYPE_GUARD:
            switch (c->bb_pl->inv.iitems[i].data.data_b[1])
            {
            case ITEM_SUBTYPE_FRAME:
                //DBG_LOG("装甲识别");
                /*if ((item_check_equip(pmt_armor_bb[c->bb_pl->inv.iitems[i].data_b[2]]->equip_flag, c->equip_flags)) ||
                    (c->bb_pl->character.level < pmt_armor_bb[c->bb_pl->inv.iitems[i].data_b[2]]->level_req))
                {
                    ERR_LOG("GC %" PRIu32 " 装备了作弊物品数据!",
                        c->guildcard);
                    send_msg1(c, "\"作弊装备\" 是不允许的.");
                    return -1;
                } else */ {
                // Remove any other armor and equipped slot items.移除其他装甲和插槽
                for (j = 0; j < inv; ++j)
                {
                    if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                        (c->bb_pl->inv.iitems[j].data.data_b[1] != ITEM_SUBTYPE_BARRIER) &&
                        (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008)))
                    {
                        //DBG_LOG("卸载装甲");
                        c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                        c->bb_pl->inv.iitems[j].data.data_b[4] = 0x00;
                    }
                }
                break;
            }
                break;
            case ITEM_SUBTYPE_BARRIER: // Check barrier equip requirements 检测护盾装备请求
                //DBG_LOG("护盾识别");
                if ((item_check_equip(pmt_shield_bb[c->bb_pl->inv.iitems[i].data.data_b[2]]->equip_flag & c->equip_flags, c->equip_flags)) ||
                    (c->bb_pl->character.disp.level < pmt_shield_bb[c->bb_pl->inv.iitems[i].data.data_b[2]]->level_req))
                {
                    ERR_LOG("GC %" PRIu32 " 装备了作弊物品数据!",
                        c->guildcard);
                    send_msg1(c, "\"作弊装备\" 是不允许的.");
                    //return -1;
                } else  {
                // Remove any other barrier
                for (j = 0; j < inv; ++j) {
                    if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                        (c->bb_pl->inv.iitems[j].data.data_b[1] == ITEM_SUBTYPE_BARRIER) &&
                        (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008)))
                    {
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

                for (j = 0; j < inv; j++)
                {
                    // Another loop ;(
                    if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_GUARD) &&
                        (c->bb_pl->inv.iitems[j].data.data_b[1] == ITEM_SUBTYPE_UNIT))
                    {
                        //DBG_LOG("插槽 %d 识别", j);
                        if ((c->bb_pl->inv.iitems[j].flags & LE32(0x00000008)) &&
                            (c->bb_pl->inv.iitems[j].data.data_b[4] < 0x04)) {

                            slot[c->bb_pl->inv.iitems[j].data.data_b[4]] = 1;
                            //DBG_LOG("插槽 %d 卸载", j);
                        }
                    }
                }

                for (j = 0; j < 4; j++)
                {
                    if (slot[j] == 0)
                    {
                        found_slot = j + 1;
                        break;
                    }
                }

                if (found_slot && (c->mode > 0)) {
                    found_slot--;
                    c->bb_pl->inv.iitems[j].data.data_b[4] = (uint8_t)(found_slot);
                    //DBG_LOG("111111111111111");
                    //client->Full_Char.inventory[i].item.item_data1[4] = (uint8_t)(found_slot);
                }
                else {
                    if (found_slot)
                    {
                        found_slot--;
                        c->bb_pl->inv.iitems[j].data.data_b[4] = (uint8_t)(found_slot);
                        //DBG_LOG("111111111111111");
                    }
                    else
                    {//缺失 Sancaros

                        //DBG_LOG("111111111111111");
                        c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                        ERR_LOG("GC %" PRIu32 " 装备了作弊物品数据!",
                            c->guildcard);
                        send_msg1(c, "\"装甲没有插槽\" 是不允许的.");
                        return -1;
                    }
                }
                break;
            }
            break;
        case ITEM_TYPE_MAG:
            //DBG_LOG("玛古识别");
            // Remove equipped mag
            for (j = 0; j < inv; j++)
                if ((c->bb_pl->inv.iitems[j].data.data_b[0] == ITEM_TYPE_MAG) &&
                    (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {

                    c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                    //DBG_LOG("卸载玛古");
                }
            break;
        }

        //DBG_LOG("完成卸载, 但是未识别成功");

        /* XXXX: Should really make sure we can equip it first... */
        c->bb_pl->inv.iitems[i].flags |= LE32(0x00000008);
    }

    /* 是否存在物品背包中? */
    if (!found_item) {
        ERR_LOG("GC %" PRIu32 " 装备了未存在的物品数据!",
            c->guildcard);
        //return -1;
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_unequip(ship_client_t* c, subcmd_bb_equip_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t inv, i, isframe = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅卸除装备!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误卸除装备数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Find the item and remove the equip flag. */
    inv = c->bb_pl->inv.item_count;
    for (i = 0; i < inv; ++i) {
        if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
            c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);

            /* If its a frame, we have to make sure to unequip any units that
               may be equipped as well. */
            if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_FRAME) {
                isframe = 1;
            }

            break;
        }
    }

    /* Did we find something to equip? */
    if (i >= inv) {
        ERR_LOG("GC %" PRIu32 " 卸除了未存在的物品数据!",
            c->guildcard);
        return -1;
    }

    /* Clear any units if we unequipped a frame. */
    if (isframe) {
        for (i = 0; i < inv; ++i) {
            if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_UNIT) {
                c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_use_item(ship_client_t* c, subcmd_bb_use_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int num;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅使用物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x02)
        return -1;

    if (!(c->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* 从玩家的背包中移除该物品. */
    if ((num = item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
        pkt->item_id, 1)) < 1) {
        ERR_LOG("无法从玩家背包中移除物品!");
        return -1;
    }

    c->item_count -= num;
    c->bb_pl->inv.item_count -= num;
    c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;


send_pkt:
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_medic(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " used medical center in lobby!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->size != 0x01 ||
        pkt->data[0] != c->client_id) {
        ERR_LOG("GC %" PRIu32 " sent bad medic message!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Subtract 10 meseta from the client. */
    c->bb_pl->character.disp.meseta -= 10;
    c->pl->bb.character.disp.meseta -= 10;

    /* Send it along to the rest of the lobby. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_3a(ship_client_t* c, subcmd_bb_cmd_3a_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_sort_inv(ship_client_t* c, subcmd_bb_sort_inv_t* pkt) {
    lobby_t* l = c->cur_lobby;
    inventory_t inv = { 0 };
    inventory_t mode_inv = { 0 };
    int i, j, found = 0;
    int item_used[30] = { 0 };
    int mode_item_used[30] = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅整理了物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0084) || pkt->shdr.size != 0x1F) {
        ERR_LOG("GC %" PRIu32 " 发送错误的整理物品数据包!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Copy over the beginning of the inventory... */
    inv.item_count = c->bb_pl->inv.item_count;
    inv.hpmats_used = c->bb_pl->inv.hpmats_used;
    inv.tpmats_used = c->bb_pl->inv.tpmats_used;
    inv.language = c->bb_pl->inv.language;

    mode_inv.item_count = c->pl->bb.inv.item_count;
    mode_inv.hpmats_used = c->pl->bb.inv.hpmats_used;
    mode_inv.tpmats_used = c->pl->bb.inv.tpmats_used;
    mode_inv.language = c->pl->bb.inv.language;
    
    /* Copy over each item as its in the sorted list. */
    for (i = 0; i < MAX_PLAYER_INV_ITEMS; ++i) {
        /* Have we reached the end of the list? */
        if (pkt->item_ids[i] == 0xFFFFFFFF){
            inv.iitems[i].data.item_id = 0xFFFFFFFF;
            break;
        }

        /* Look for the item in question. */
        for (j = 0; j < inv.item_count; ++j) {
            if (c->bb_pl->inv.iitems[j].data.item_id == pkt->item_ids[i]) {
                /* Make sure they haven't used this one yet. */
                if (item_used[j]) {
                    ERR_LOG("GC %" PRIu32 " 在背包整理中列出了两次同样的物品!", c->guildcard);
                    return -1;
                }

                /* Copy the item and set it as used in the list. */
                memcpy(&inv.iitems[i], &c->bb_pl->inv.iitems[j], sizeof(iitem_t));
                item_used[j] = 1;
                break;
            }
        }

        //ERR_LOG("GC %" PRIu32 " 整理了未知的物品! 数据背包 %d 当前背包 %d",
        //    c->guildcard, j, inv.item_count);

        /* Look for the item in question. */
        for (j = 0; j < mode_inv.item_count; ++j) {
            if (c->pl->bb.inv.iitems[j].data.item_id == pkt->item_ids[i]) {
                /* Make sure they haven't used this one yet. */
                if (mode_item_used[j]) {
                    ERR_LOG("GC %" PRIu32 " 在背包整理中列出了两次同样的物品!", c->guildcard);
                    return -1;
                }

                /* Copy the item and set it as used in the list. */
                memcpy(&mode_inv.iitems[i], &c->pl->bb.inv.iitems[j], sizeof(iitem_t));
                mode_item_used[j] = 1;
                break;
            }
        }

        //ERR_LOG("GC %" PRIu32 " 整理了未知的物品! 数据背包 %d 当前背包 %d",
        //    c->guildcard, j, mode_inv.item_count);

        /* Make sure the item got sorted in right. */
        if (j > inv.item_count) {
            ERR_LOG("GC %" PRIu32 " 整理了未知的物品! 数据背包 %d 当前背包 %d",
                c->guildcard, j, inv.item_count);
            return -1;
        }
        found++;
    }

    if (!i) {
        ERR_LOG("GC %" PRIu32 " 未找到需要整理的物品! %d %d", c->guildcard, i, c->bb_pl->inv.item_count);
        return -1;
    }

    //DBG_LOG("found %d (%d %d) i = %d", found, c->bb_pl->inv.item_count, c->pl->bb.inv.item_count, i);

    /* Make sure we got everything... */
    if (i != c->pl->bb.inv.item_count) {
        ERR_LOG("GC %" PRIu32 " 忘了物品还在排序中! %d %d", c->guildcard, i, c->bb_pl->inv.item_count);
        mode_inv.item_count = i;
        inv.item_count = i;
        //return -1;
    }

    /* We're good, so copy the inventory into the client's data. */
    memcpy(&c->bb_pl->inv, &inv, sizeof(inventory_t));
    memcpy(&c->pl->bb.inv, &mode_inv, sizeof(inventory_t));

    //DBG_LOG("found %d (%d %d) i = %d", found, c->bb_pl->inv.item_count, c->pl->bb.inv.item_count, i);

    /* Nobody else really needs to care about this one... */
    return 0;
}

static int handle_bb_create_pipe(ship_client_t* c, subcmd_bb_pipe_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅使用传送门!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0024) || pkt->shdr.size != 0x07) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* See if the user is creating a pipe or destroying it. Destroying a pipe
       always matches the created pipe, but sets the area to 0. We could keep
       track of all of the pipe data, but that's probably overkill. For now,
       blindly accept any pipes where the area is 0
       查看用户是创建管道还是销毁管道。销毁管道总是与创建的管道匹配，
       但会将区域设置为0。我们可以跟踪所有管道数据，
       但这可能太过分了。目前，盲目接受面积为0的任何管道。. */
    if (pkt->area != 0) {
        /* Make sure the user is sending a pipe from the area he or she is
           currently in. */
        if (pkt->area != c->cur_area) {
            ERR_LOG("GC %" PRIu32 " 尝试从传送门传送至不存在的区域 (玩家层级: %d, 传送层级: %d).", c->guildcard,
                c->cur_area, (int)pkt->area);
            return -1;
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_spawn_npc(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅生成了NPC!",
            c->guildcard);
        return -1;
    }

    /* The only quests that allow NPCs to be loaded are those that require there
       to only be one player, so limit that here. Also, we only allow /npc in
       single-player teams, so that'll fall into line too. */
    if (l->num_clients > 1) {
        ERR_LOG("GC %" PRIu32 " to spawn NPC in multi-"
            "player team!", c->guildcard);
        return -1;
    }

    /* Either this is a legitimate request to spawn a quest NPC, or the player
       is doing something stupid like trying to NOL himself. We don't care if
       someone is stupid enough to NOL themselves, so send the packet on now. */
    return subcmd_send_lobby_bb(l, c, pkt, 0);
}

/* 任务的完成度检测 */
int quest_flag_check(uint8_t* flag_data, uint32_t flag, uint32_t difficulty) {
    if (flag_data[(difficulty * 0x80) + (flag >> 3)] & (1 << (7 - (flag & 0x07))))
        return 1;
    else
        return 0;
}

/* EP1 单人任务的完成度检测 */
int quest_flag_check_ep1_solo(uint8_t* flag_data, uint32_t difficulty) {
    int i;
    uint32_t quest_flag;

    for (i = 1; i <= 25; i++) {
        quest_flag = 0x63 + (i << 1);
        if (!quest_flag_check(flag_data, quest_flag, difficulty)) 
            return 0;
    }

    return 1;
}

/* TODO 挑战任务的完成度检测 */
int quest_flag_check_cmode(uint8_t* flag_data, uint32_t difficulty) {
    int i;
    uint32_t quest_flag;

    for (i = 1; i <= 14; i++) {
        quest_flag = 65535 + (i << 1);
        if (!quest_flag_check(flag_data, quest_flag, difficulty)) 
            return 0;
    }

    return 1;
}

static int handle_bb_set_flag(ship_client_t* c, subcmd_bb_set_flag_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t flag;
    int rv = 0;

    //if (!l->is_game()) {
    //    return;
    //}

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (!l || l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        return -1;
    }

    //const auto* p = check_size_sc(data, sizeof(PSOSubcommand), 0xFFFF);
    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    DBG_LOG("GC %" PRIu32 " 触发SET_FLAG指令!",
        c->guildcard);

    flag = pkt->flag;
    if (flag < 0x400)
        c->bb_pl->quest_data1[((uint32_t)l->difficulty * 0x80) + (flag >> 3)] |= 1 << (7 - (flag & 0x07));

    bool should_send_boss_drop_req = false;
    if (pkt->difficulty == l->difficulty) {
        if ((l->episode == 1) && (c->cur_area == 0x0E)) {
            // 在正常情况下，黑暗佛没有第三阶段，所以在第二阶段结束后发送掉落请求
            // 其他困难模式则在第三阶段结束后发送
            if (((l->difficulty == 0) && (flag == 0x00000035)) ||
                ((l->difficulty != 0) && (flag == 0x00000037))) {
                should_send_boss_drop_req = true;
            }
        }
        else if ((l->episode == 2) && (flag == 0x00000057) && (c->cur_area == 0x0D)) {
            should_send_boss_drop_req = true;
        }
    }

    if (should_send_boss_drop_req) {
        ship_client_t* s = l->clients[l->leader_id];
        if (s) {
            subcmd_bb_itemreq_t bd = { 0 };

            /* 填充数据头 */
            bd.hdr.pkt_len = LE16(sizeof(subcmd_bb_itemreq_t));
            bd.hdr.pkt_type = GAME_COMMAND2_TYPE;
            bd.hdr.flags = 0x00000000;
            bd.shdr.type = SUBCMD62_ITEMREQ;
            bd.shdr.size = 0x06;
            bd.shdr.unused = 0x0000;

            /* 填充数据 */
            bd.area = (uint8_t)c->cur_area;
            bd.pt_index = (uint8_t)((l->episode == 2) ? 0x4E : 0x2F);
            bd.request_id = LE16(0x0B4F);
            bd.x = (l->episode == 2) ? -9999.0f : 10160.58984375f;
            bd.z = 0.0f;
            bd.unk1 = LE16(0x0002);
            bd.unk2 = LE16(0x0000);
            bd.unk3 = LE32(0xE0AEDC01);

            return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
        }
    }

    rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    //forward_subcommand(l, c, command, flag, data);

    return rv;
}

static int handle_bb_killed_monster(ship_client_t* c, subcmd_bb_killed_monster_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中杀掉了怪物!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的杀怪数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_sync_reg(ship_client_t* c, subcmd_bb_sync_reg_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t val = LE32(pkt->value);
    int done = 0, idx;
    uint32_t ctl;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的同步数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* XXXX: Probably should do some checking here... */
    /* Run the register sync script, if one is set. If the script returns
       non-zero, then assume that it has adequately handled the sync. */
    if ((script_execute(ScriptActionQuestSyncRegister, c, SCRIPT_ARG_PTR, c,
        SCRIPT_ARG_PTR, l, SCRIPT_ARG_UINT8, pkt->register_number,
        SCRIPT_ARG_UINT32, val, SCRIPT_ARG_END))) {
        done = 1;
    }

    /* Does this quest use global flags? If so, then deal with them... */
    if ((l->q_flags & LOBBY_QFLAG_SHORT) && pkt->register_number == l->q_shortflag_reg &&
        !done) {
        /* Check the control bits for sensibility... */
        ctl = (val >> 29) & 0x07;

        /* Make sure the error or response bits aren't set. */
        if ((ctl & 0x06)) {
            DBG_LOG("Quest set flag register with illegal ctl!\n");
            send_sync_register(c, pkt->register_number, 0x8000FFFE);
        }
        /* Make sure we don't have anything with any reserved ctl bits set
           (unless a script has already handled the sync). */
        else if ((val & 0x17000000)) {
            DBG_LOG("Quest set flag register with reserved ctl!\n");
            send_sync_register(c, pkt->register_number, 0x8000FFFE);
        }
        else if ((val & 0x08000000)) {
            /* Delete the flag... */
            shipgate_send_qflag(&ship->sg, c, 1, ((val >> 16) & 0xFF),
                c->cur_lobby->qid, 0, QFLAG_DELETE_FLAG);
        }
        else {
            /* Send the request to the shipgate... */
            shipgate_send_qflag(&ship->sg, c, ctl & 0x01, (val >> 16) & 0xFF,
                c->cur_lobby->qid, val & 0xFFFF, 0);
        }

        done = 1;
    }

    /* Does this quest use server data calls? If so, deal with it... */
    if ((l->q_flags & LOBBY_QFLAG_DATA) && !done) {
        if (pkt->register_number == l->q_data_reg) {
            if (c->q_stack_top < CLIENT_MAX_QSTACK) {
                if (!(c->flags & CLIENT_FLAG_QSTACK_LOCK)) {
                    c->q_stack[c->q_stack_top++] = val;

                    /* Check if we've got everything we expected... */
                    if (c->q_stack_top >= 3 &&
                        c->q_stack_top == 3 + c->q_stack[1] + c->q_stack[2]) {
                        /* Call the function requested and reset the stack. */
                        ctl = quest_function_dispatch(c, l);

                        if (ctl != QUEST_FUNC_RET_NOT_YET) {
                            send_sync_register(c, pkt->register_number, ctl);
                            c->q_stack_top = 0;
                        }
                    }
                }
                else {
                    /* The stack is locked, ignore the write and report the
                       error. */
                    send_sync_register(c, pkt->register_number,
                        QUEST_FUNC_RET_STACK_LOCKED);
                }
            }
            else if (c->q_stack_top == CLIENT_MAX_QSTACK) {
                /* Eat the stack push and report an error. */
                send_sync_register(c, pkt->register_number,
                    QUEST_FUNC_RET_STACK_OVERFLOW);
            }

            done = 1;
        }
        else if (pkt->register_number == l->q_ctl_reg) {
            /* For now, the only reason we'll have one of these is to reset the
               stack. There might be other reasons later, but this will do, for
               the time being... */
            c->q_stack_top = 0;
            done = 1;
        }
    }

    /* Does this register have to be synced? */
    if ((idx = bb_reg_sync_index(l, pkt->register_number)) != -1) {
        l->regvals[idx] = val;
    }

    if (!done)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    return 0;
}

static int handle_bb_player_died(ship_client_t* c, subcmd_bb_player_died_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令! 0x%02X",
            c->guildcard, pkt->shdr.type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的死亡数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_used_tech(ship_client_t* c, subcmd_bb_used_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 释放了违规的法术!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(c->flags & CLIENT_FLAG_INFINITE_TP)) {
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD60_STAT_TPUP, 255);
    //return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_Unknown_6x88(ship_client_t* c, subcmd_bb_Unknown_6x88_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    UDONE_CSPD(pkt->hdr.pkt_type, c->version, pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_Unknown_6x8A(ship_client_t* c, subcmd_bb_Unknown_6x8A_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        //return -1;
    }

    UDONE_CSPD(pkt->hdr.pkt_type, c->version, pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_set_technique_level_override(ship_client_t* c, subcmd_bb_set_technique_level_override_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 释放了违规的法术!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        //return -1;
    }

    /*uint8_t tmp_level = pkt->level_upgrade;

    pkt->level_upgrade = tmp_level+100;*/

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_timed_switch_activated(ship_client_t* c, subcmd_bb_timed_switch_activated_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        //return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_shop_inv(ship_client_t* c, subcmd_bb_shop_inv_t* pkt) {
    block_t* b = c->cur_block;
    lobby_t* l = c->cur_lobby;
 
    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    /* This aught to do it... */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_death_sync(ship_client_t* c, subcmd_bb_death_sync_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int i;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    c->game_data.death = 1;

    for (i = 0; i < c->bb_pl->inv.item_count; i++) {
        if ((c->bb_pl->inv.iitems[i].data.data_b[0] == 0x02) &&
            (c->bb_pl->inv.iitems[i].flags & 0x08))
        {
            if (c->bb_pl->inv.iitems[i].data.data2_b[0] >= 5)
                c->bb_pl->inv.iitems[i].data.data2_b[0] -= 5;
            else
                c->bb_pl->inv.iitems[i].data.data2_b[0] = 0;
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_4e(ship_client_t* c, subcmd_bb_cmd_4e_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令! 0x%02X",
            c->guildcard, pkt->shdr.type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_switch_req(ship_client_t* c, subcmd_bb_switch_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_take_damage(ship_client_t* c, subcmd_bb_take_damage_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY || pkt->shdr.client_id != c->client_id) {
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(c->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD60_STAT_HPUP, 2000);
}

static int handle_bb_menu_req(ship_client_t* c, subcmd_bb_menu_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We don't care about these in lobbies. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Clear the list of dropped items. */
    if (c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    /* Flip the shopping flag, since this packet is sent both for talking to the
       shop in the first place and when the client exits the shop
       翻转购物标志，因为该数据包在第一时间和客户离开商店时都会发送给商店. */
    c->flags ^= CLIENT_FLAG_SHOPPING;
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static void update_bb_qpos(ship_client_t* c, lobby_t* l) {
    uint8_t r;

    if ((r = l->qpos_regs[c->client_id][0])) {
        send_sync_register(l->clients[0], r, (uint32_t)c->x);
        send_sync_register(l->clients[0], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[0], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[0], r + 3, (uint32_t)c->cur_area);
    }
    if ((r = l->qpos_regs[c->client_id][1])) {
        send_sync_register(l->clients[1], r, (uint32_t)c->x);
        send_sync_register(l->clients[1], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[1], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[1], r + 3, (uint32_t)c->cur_area);
    }
    if ((r = l->qpos_regs[c->client_id][2])) {
        send_sync_register(l->clients[2], r, (uint32_t)c->x);
        send_sync_register(l->clients[2], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[2], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[2], r + 3, (uint32_t)c->cur_area);
    }
    if ((r = l->qpos_regs[c->client_id][3])) {
        send_sync_register(l->clients[3], r, (uint32_t)c->x);
        send_sync_register(l->clients[3], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[3], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[3], r + 3, (uint32_t)c->cur_area);
    }
}

static int handle_bb_set_area(ship_client_t* c, subcmd_bb_set_area_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if (c->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            c->cur_area, SCRIPT_ARG_END);

        // 测试DC代码
        /* Clear the list of dropped items. */
        if (c->cur_area == 0) {
            memset(c->p2_drops, 0, sizeof(c->p2_drops));
            c->p2_drops_max = 0;
        }

        c->cur_area = pkt->area;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        //更换场景 客户端之间更新位置
        //DBG_LOG("客户端区域 %d （x:%f y:%f z:%f)", c->cur_area, c->x, c->y, c->z);
        //客户端区域 0 （x:228.995331 y:0.000000 z:253.998901)

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_inter_level_warp(ship_client_t* c, subcmd_bb_inter_level_warp_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if (c->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            c->cur_area, SCRIPT_ARG_END);
        c->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_set_pos_0x24(ship_client_t* c, subcmd_bb_set_pos_0x24_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x001C) || pkt->shdr.size != 0x05) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);

        DBG_LOG("GC %" PRIu32 " %d %d %02X %02X X轴:%f Y轴:%f Z轴:%f", c->guildcard, 
            c->client_id, pkt->shdr.client_id, pkt->unk1, pkt->unused, pkt->x, pkt->y, pkt->z);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_normal_attack(ship_client_t* c, subcmd_bb_natk_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发普通攻击指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Save the new area and move along */
    if (c->client_id == pkt->shdr.client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_set_pos(ship_client_t* c, subcmd_bb_set_pos_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->w = pkt->w;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_move(ship_client_t* c, subcmd_bb_move_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->x = pkt->x;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);

        c->game_data.death = 0;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_take_item(ship_client_t* c, subcmd_bb_take_item_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    // BB should never send this command - inventory items should only be
    // created by the server in response to shop buy / bank withdraw / etc. reqs

    UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);

    /* 数据包完成, 发送至游戏房间. */
    return 0;
}

static int handle_bb_drop_item(ship_client_t* c, subcmd_bb_drop_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1, isframe = 0;
    uint32_t i, inv;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅中掉落物品!",
            c->guildcard);
        return -1;
    }

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if ((c->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("GC %" PRIu32 " 尝试在商店中掉落物品!",
            c->guildcard);
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品掉落数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* TODO 完成挑战模式的物品掉落 */
    if (l->challenge) {
        /* 数据包完成, 发送至游戏房间. */
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Look for the item in the user's inventory. */
    inv = c->bb_pl->inv.item_count;

    for (i = 0; i < inv; ++i) {
        if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
            found = i;

            /* If it is an equipped frame, we need to unequip all the units
               that are attached to it.
               如果它是一个装备好的护甲，我们需要取消与它相连的所有单元的装备*/
            if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_FRAME &&
                (c->bb_pl->inv.iitems[i].flags & LE32(0x00000008))) {
                isframe = 1;
            }

            break;
        }
    }

    /* If the item isn't found, then punt the user from the ship. */
    if (found == -1) {
        ERR_LOG("GC %" PRIu32 " 掉落了无效的物品 ID!",
            c->guildcard);
        return -1;
    }

    /* Clear the equipped flag. */
    c->bb_pl->inv.iitems[found].flags &= LE32(0xFFFFFFF7);

    /* Unequip any units, if the item was equipped and a frame. */
    if (isframe) {
        for (i = 0; i < inv; ++i) {
            if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_UNIT) {
                c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* We have the item... Add it to the lobby's inventory.
    我们有这个物品…把它添加到大厅的背包中*/
    if (!lobby_add_item_locked(l, &c->bb_pl->inv.iitems[found])) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("Couldn't add item to lobby inventory!\n");
        return -1;
    }

    /* 从玩家的背包中移除该物品. */
    if (item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
        pkt->item_id, 0xFFFFFFFF) < 1) {
        ERR_LOG("无法从玩家背包中移除物品!");
        return -1;
    }

    --c->bb_pl->inv.item_count;
    c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

    /* 数据包完成, 发送至游戏房间. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_drop_pos(ship_client_t* c, subcmd_bb_drop_pos_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1;
    uint32_t i, meseta, amt;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅丢弃了物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Look for the item in the user's inventory. */
    if (pkt->item_id != 0xFFFFFFFF) {
        for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
            if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
                found = i;
                break;
            }
        }

        if (c->mode > 0) {
            for (i = 0; i < c->pl->bb.inv.item_count; ++i) {
                if (c->pl->bb.inv.iitems[i].data.item_id == pkt->item_id) {
                    found = i;
                    break;
                }
            }
        }

        /* If the item isn't found, then punt the user from the ship. */
        if (found == -1) {
            ERR_LOG("GC %" PRIu32 " 丢弃了无效的物品ID!",
                c->guildcard);
            return -1;
        }
    }
    else {
        meseta = LE32(c->bb_pl->character.disp.meseta);
        amt = LE32(pkt->amount);

        if (meseta < amt) {
            ERR_LOG("GC %" PRIu32 " 丢弃的美赛塔超出所拥有的!",
                c->guildcard);
            return -1;
        }
    }

    /* We have the item... Record the information for use with the subcommand
       0x29 that should follow. */
    c->drop_x = pkt->x;
    c->drop_z = pkt->z;
    c->drop_area = pkt->area;
    c->drop_item = pkt->item_id;
    c->drop_amt = pkt->amount;

    /* 数据包完成, 发送至游戏房间. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_destroy_item(ship_client_t* c, subcmd_bb_destroy_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1;
    uint32_t i, tmp, tmp2;
    iitem_t item_data;
    iitem_t* it;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中掉落堆叠物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的堆叠物品掉落数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* TODO 完成挑战模式的物品掉落 */
    if (l->challenge) {
        /* 数据包完成, 发送至游戏房间. */
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    if (pkt->item_id != 0xFFFFFFFF) {
        /* 查找用户库存中的物品. */
        for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
            if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
                found = i;
                break;
            }
        }

        /* 如果找不到该物品，则将用户从船上推下. */
        if (found == -1) {
            ERR_LOG("GC %" PRIu32 " 掉落无效的堆叠物品!", c->guildcard);
            return -1;
        }

        /* 从客户端结构的库存中获取物品并设置拆分 */
        item_data = c->iitems[found];
        item_data.data.item_id = LE32((++l->item_player_id[c->client_id]));
        item_data.data.data_b[5] = (uint8_t)(LE32(pkt->amount));
    }
    else {
        item_data.data.data_l[0] = LE32(Item_Meseta);
        item_data.data.data_l[1] = item_data.data.data_l[2] = 0;
        item_data.data.item_id = LE32((++l->item_player_id[c->client_id]));
        item_data.data.data2_l = pkt->amount;
    }

    /* Make sure the item id and amount match the most recent 0xC3. */
    if (pkt->item_id != c->drop_item || pkt->amount != c->drop_amt) {
        ERR_LOG("GC %" PRIu32 " 掉了不同的堆叠物品!",
            c->guildcard);
        ERR_LOG("pkt->item_id %d c->drop_item %d pkt->amount %d c->drop_amt %d", pkt->item_id, c->drop_item, pkt->amount, c->drop_amt);
        return -1;
    }

    /* We have the item... Add it to the lobby's inventory. */
    if (!(it = lobby_add_item_locked(l, &item_data))) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("无法将物品添加至游戏房间!\n");
        return -1;
    }

    if (pkt->item_id != 0xFFFFFFFF) {
        /* 从玩家的背包中移除该物品. */
        found = item_remove_from_inv(c->bb_pl->inv.iitems,
            c->bb_pl->inv.item_count, pkt->item_id,
            LE32(pkt->amount));
        if (found < 0) {
            ERR_LOG("无法从玩家背包中移除物品!");
            return -1;
        }

        c->bb_pl->inv.item_count -= found;
        //c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;
    }
    else {
        /* Remove the meseta from the character data */
        tmp = LE32(pkt->amount);
        tmp2 = LE32(c->bb_pl->character.disp.meseta);

        if (tmp > tmp2) {
            ERR_LOG("GC %" PRIu32 " 掉落的美赛塔超出所拥有的",
                c->guildcard);
            return -1;
        }

        c->bb_pl->character.disp.meseta = LE32(tmp2 - tmp);
        c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
    }

    /* 现在我们有两个数据包要发送.首先,发送一个数据包,告诉每个人有一个物品掉落.
    然后,发送一个从客户端的库存中删除物品的人.第一个必须发给每个人,
    第二个必须发给除了最初发送这个包裹的人以外的所有人. */
    subcmd_send_bb_drop_stack(c, c->drop_area, c->drop_x, c->drop_z, it);

    /* 数据包完成, 发送至游戏房间. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_hit_by_enemy(ship_client_t* c, subcmd_bb_hit_by_enemy_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t type = LE16(pkt->hdr.pkt_type);

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发游戏房间指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    UDONE_CSPD(type, c->version, pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_talk_npc(ship_client_t* c, subcmd_bb_talk_npc_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅与游戏房间中的NPC交谈!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x05)
        return -1;

    /* Clear the list of dropped items. */
    if (pkt->unk == 0xFFFF && c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_done_talk_npc(ship_client_t* c, subcmd_bb_end_talk_to_npc_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅与游戏房间中的NPC完成交谈!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x01)
        return -1;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_level_up(ship_client_t* c, subcmd_bb_level_up_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x05 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Copy over the new data to the client's character structure... */
    c->pl->bb.character.disp.stats.atp = pkt->atp;
    c->pl->bb.character.disp.stats.mst = pkt->mst;
    c->pl->bb.character.disp.stats.evp = pkt->evp;
    c->pl->bb.character.disp.stats.hp = pkt->hp;
    c->pl->bb.character.disp.stats.dfp = pkt->dfp;
    c->pl->bb.character.disp.stats.ata = pkt->ata;
    c->pl->bb.character.disp.level = pkt->level;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_word_select(ship_client_t* c, subcmd_bb_word_select_t* pkt) {
    subcmd_word_select_t gc = { 0 };

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send chats for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    memcpy(&gc, pkt, sizeof(subcmd_word_select_t) - sizeof(dc_pkt_hdr_t));
    gc.client_id_gc = (uint8_t)pkt->shdr.client_id;
    gc.hdr.pkt_type = (uint8_t)(LE16(pkt->hdr.pkt_type));
    gc.hdr.flags = (uint8_t)pkt->hdr.flags;
    gc.hdr.pkt_len = LE16((LE16(pkt->hdr.pkt_len) - 4));

    return word_select_send_gc(c, &gc);
}

static int handle_bb_chair_dir(ship_client_t* c, subcmd_bb_chair_dir_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmode_grave(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    int i;
    lobby_t* l = c->cur_lobby;
    subcmd_pc_grave_t pc = { { 0 } };
    subcmd_dc_grave_t dc = { { 0 } };
    subcmd_bb_grave_t bb = { { 0 } };
    size_t in, out;
    char* inptr;
    char* outptr;

    /* Deal with converting the different versions... */
    switch (c->version) {
    case CLIENT_VERSION_DCV2:
        memcpy(&dc, pkt, sizeof(subcmd_dc_grave_t));

        /* Make a copy to send to PC players... */
        pc.hdr.pkt_type = GAME_COMMAND0_TYPE;
        pc.hdr.pkt_len = LE16(0x00E4);

        pc.shdr.type = SUBCMD60_CMODE_GRAVE;
        pc.shdr.size = 0x38;
        pc.shdr.client_id = dc.shdr.client_id;
        pc.client_id2 = dc.client_id2;
        //pc.unk0 = dc.unk0;
        //pc.unk1 = dc.unk1;

        for (i = 0; i < 0x0C; ++i) {
            pc.string[i] = LE16(dc.string[i]);
        }

        memcpy(pc.unk2, dc.unk2, 0x24);
        pc.grave_unk4 = dc.grave_unk4;
        pc.deaths = dc.deaths;
        pc.coords_time[0] = dc.coords_time[0];
        pc.coords_time[1] = dc.coords_time[1];
        pc.coords_time[2] = dc.coords_time[2];
        pc.coords_time[3] = dc.coords_time[3];
        pc.coords_time[4] = dc.coords_time[4];

        /* Convert the team name */
        in = 20;
        out = 40;
        inptr = dc.team;
        outptr = (char*)pc.team;

        if (dc.team[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        /* Convert the message */
        in = 24;
        out = 48;
        inptr = dc.message;
        outptr = (char*)pc.message;

        if (dc.message[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        memcpy(pc.times, dc.times, 36);
        pc.unk = dc.unk;
        break;

    case CLIENT_VERSION_PC:
        memcpy(&pc, pkt, sizeof(subcmd_pc_grave_t));

        /* Make a copy to send to DC players... */
        dc.hdr.pkt_type = GAME_COMMAND0_TYPE;
        dc.hdr.pkt_len = LE16(0x00AC);

        dc.shdr.type = SUBCMD60_CMODE_GRAVE;
        dc.shdr.size = 0x2A;
        dc.shdr.client_id = pc.shdr.client_id;
        dc.client_id2 = pc.client_id2;
        //dc.unk0 = pc.unk0;
        //dc.unk1 = pc.unk1;

        for (i = 0; i < 0x0C; ++i) {
            dc.string[i] = (char)LE16(dc.string[i]);
        }

        memcpy(dc.unk2, pc.unk2, 0x24);
        dc.grave_unk4 = pc.grave_unk4;
        dc.deaths = pc.deaths;
        dc.coords_time[0] = pc.coords_time[0];
        dc.coords_time[1] = pc.coords_time[1];
        dc.coords_time[2] = pc.coords_time[2];
        dc.coords_time[3] = pc.coords_time[3];
        dc.coords_time[4] = pc.coords_time[4];

        /* Convert the team name */
        in = 40;
        out = 20;
        inptr = (char*)pc.team;
        outptr = dc.team;

        if (pc.team[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Convert the message */
        in = 48;
        out = 24;
        inptr = (char*)pc.message;
        outptr = dc.message;

        if (pc.message[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        memcpy(dc.times, pc.times, 36);
        dc.unk = pc.unk;
        break;

    case CLIENT_VERSION_BB:
        memcpy(&bb, pkt, sizeof(subcmd_bb_grave_t));
        print_payload((unsigned char*)&bb, LE16(pkt->hdr.pkt_len));
        break;

    default:
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Send the packet to everyone in the lobby */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            switch (l->clients[i]->version) {
            case CLIENT_VERSION_DCV2:
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&dc);
                break;

            case CLIENT_VERSION_PC:
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&pc);
                break;

            case CLIENT_VERSION_BB:
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&bb);
                break;
            }
        }
    }

    return 0;
}

static int handle_bb_trade_done(ship_client_t* c, subcmd_bb_trade_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x08) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_sell_item(ship_client_t* c, subcmd_bb_sell_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t i;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    for (i = 0;i < c->bb_pl->inv.item_count;i++) {
        /* Look for the item in question. */
        if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {

            if ((pkt->sell_num > 1) && (c->bb_pl->inv.iitems[i].data.data_b[0] != 0x03)) {
                DBG_LOG("handle_bb_sell_item %d 0x%02X", pkt->sell_num, c->bb_pl->inv.iitems[i].data.data_b[0]);
                return -1;
            }
            else
            {
                uint32_t shop_price = get_bb_shop_price(&c->bb_pl->inv.iitems[i]) * pkt->sell_num;

                /* 从玩家的背包中移除该物品. */
                if (item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
                    pkt->item_id, 0xFFFFFFFF) < 1) {
                    ERR_LOG("无法从玩家背包中移除物品!");
                    break;
                }

                --c->bb_pl->inv.item_count;
                c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

                c->bb_pl->character.disp.meseta += shop_price;

                if (c->bb_pl->character.disp.meseta > 999999)
                    c->bb_pl->character.disp.meseta = 999999;
            }

            return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_Unknown_6x53(ship_client_t* c, subcmd_bb_Unknown_6x53_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        //return -1;
    }

    UDONE_CSPD(pkt->hdr.pkt_type, c->version, pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_map_warp_55(ship_client_t* c, subcmd_bb_map_warp_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0028) || pkt->shdr.size != 0x08 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    if (c->client_id == pkt->shdr.client_id) {
        //c->x = pkt->x;
        //c->y = pkt->y;
        //c->z = pkt->z;

        //if ((l->flags & LOBBY_FLAG_QUESTING))
        //    update_bb_qpos(c, l);

        DBG_LOG("这里缺总督府任务识别");
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_level_up_req(ship_client_t* c, subcmd_bb_levelup_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014)) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_lobby_act(ship_client_t* c, subcmd_bb_lobby_act_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    DBG_LOG("动作ID %d", pkt->act_id);

    // pkt->act_id 大厅动作的对应ID

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_gogo_ball(ship_client_t* c, subcmd_bb_gogo_ball_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    // pkt->act_id 大厅动作的对应ID

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

/* 处理BB 0x60 数据包. */
int subcmd_bb_handle_bcast(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0, sent = 1, i;

    subcmd_bb_60size_check(c, pkt);

    /* Ignore these if the client isn't in a lobby. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        switch (type) {
        case SUBCMD60_LOAD_3B:
        case SUBCMD60_BURST_DONE:
            /* TODO 将这个函数融合进房间函数中 */
            subcmd_send_bb_set_exp_rate(c, 100000);
            rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
            break;

        case SUBCMD60_SET_AREA:
            rv = handle_bb_set_area(c, (subcmd_bb_set_area_t*)pkt);
            break;

        case SUBCMD60_SET_POS_3F:
            rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
            break;

        case SUBCMD60_CMODE_GRAVE:
            UNK_CSPD(type, c->version, (uint8_t*)pkt);
            rv = handle_bb_cmode_grave(c, pkt);
            break;

        default:
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {
    case SUBCMD60_SWITCH_CHANGED:
        rv = handle_bb_switch_changed(c, (subcmd_bb_switch_changed_pkt_t*)pkt);
        break;

    case SUBCMD60_SYMBOL_CHAT: //间隔1秒可以正常发送
        if (time_check(c->subcmd_cooldown[type], 1))
            rv = handle_bb_symbol_chat(c, (subcmd_bb_symbol_chat_t*)pkt);
        else
            sent = 1;
        break;

    case SUBCMD60_HIT_MONSTER:
        rv = handle_bb_mhit(c, (subcmd_bb_mhit_pkt_t*)pkt);
        break;

    case SUBCMD60_HIT_OBJ:
        /* Don't even try to deal with these in battle or challenge mode
        不要尝试在战斗或挑战模式中处理这些问题. */
        if (l->challenge || l->battle) {
            sent = 0;
            break;
        }

        rv = handle_bb_objhit(c, (subcmd_bb_bhit_pkt_t*)pkt);
        break;

    case SUBCMD60_CONDITION_ADD:
        rv = handle_bb_cmd_check_size(c, pkt, 0x03);
        break;

    case SUBCMD60_CONDITION_REMOVE:
        rv = handle_bb_cmd_check_size(c, pkt, 0x03);
        break;

    case SUBCMD60_DRAGON_ACT:// Dragon actions
        rv = handle_bb_dragon_act(c, (subcmd_bb_dragon_act_t*)pkt);
        break;

    case SUBCMD60_ACTION_DE_ROl_LE:// De Rol Le actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_ACTION_DE_ROl_LE2:// De Rol Le actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_ACTION_VOL_OPT:// Vol Opt actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_ACTION_VOL_OPT2:// Vol Opt actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_TELEPORT:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_UNKNOW_18:// Dragon special actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_ACTION_DARK_FALZ:// Dark Falz actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_UNKNOW_1C:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_SET_AREA:
    case SUBCMD60_SET_AREA_20:
        rv = handle_bb_set_area(c, (subcmd_bb_set_area_t*)pkt);
        break;

    case SUBCMD60_INTER_LEVEL_WARP:
        rv = handle_bb_inter_level_warp(c, (subcmd_bb_inter_level_warp_t*)pkt);
        break;

    case SUBCMD60_LOAD_22://subcmd_set_player_visibility_6x22_6x23_t
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_SET_POS_24:
        rv = handle_bb_set_pos_0x24(c, (subcmd_bb_set_pos_0x24_t*)pkt);
        break;

    case SUBCMD60_EQUIP:
        rv = handle_bb_equip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD60_REMOVE_EQUIP:
        rv = handle_bb_unequip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD60_USE_ITEM:
        rv = handle_bb_use_item(c, (subcmd_bb_use_item_t*)pkt);
        break;

    case SUBCMD60_FEED_MAG:
        rv = handle_bb_feed_mag(c, (subcmd_bb_feed_mag_t*)pkt);
        break;

    case SUBCMD60_DELETE_ITEM:
        rv = handle_bb_destroy_item(c, (subcmd_bb_destroy_item_t*)pkt);
        break;

    case SUBCMD60_DROP_ITEM:
        rv = handle_bb_drop_item(c, (subcmd_bb_drop_item_t*)pkt);
        break;

    case SUBCMD60_TAKE_ITEM:
        rv = handle_bb_take_item(c, (subcmd_bb_take_item_t*)pkt);
        break;

    case SUBCMD60_TALK_NPC:
        rv = handle_bb_talk_npc(c, (subcmd_bb_talk_npc_t*)pkt);
        break;

    case SUBCMD60_DONE_NPC:
        rv = handle_bb_done_talk_npc(c, (subcmd_bb_end_talk_to_npc_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_2E:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_HIT_BY_ENEMY:
        rv = handle_bb_hit_by_enemy(c, (subcmd_bb_hit_by_enemy_t*)pkt);
        break;

    case SUBCMD60_LEVEL_UP:
        rv = handle_bb_level_up(c, (subcmd_bb_level_up_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_MEDIC_31:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_UNKNOW_MEDIC_32:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_UNKNOW_33:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_34:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_35:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_36:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_37:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_38:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_39:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_3A:
        rv = handle_bb_cmd_3a(c, (subcmd_bb_cmd_3a_t*)pkt);
        break;

    case SUBCMD60_LOAD_3B:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_UNKNOW_3C:
    case SUBCMD60_UNKNOW_3D:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_SET_POS_3E:
    case SUBCMD60_SET_POS_3F:
        rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_41:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_MOVE_SLOW:
    case SUBCMD60_MOVE_FAST:
        rv = handle_bb_move(c, (subcmd_bb_move_t*)pkt);
        break;

    case SUBCMD60_ATTACK1:
    case SUBCMD60_ATTACK2:
    case SUBCMD60_ATTACK3:
        rv = handle_bb_normal_attack(c, (subcmd_bb_natk_t*)pkt);
        break;

    case SUBCMD60_OBJHIT_PHYS:
        /* Don't even try to deal with these in battle or challenge mode. */
        if (l->challenge || l->battle) {
            sent = 0;
            break;
        }

        rv = handle_bb_objhit_phys(c, (subcmd_bb_objhit_phys_t*)pkt);
        break;

    case SUBCMD60_OBJHIT_TECH:
        /* Don't even try to deal with these in battle or challenge mode. */
        if (l->challenge || l->battle) {
            sent = 0;
            break;
        }

        rv = handle_bb_objhit_tech(c, (subcmd_bb_objhit_tech_t*)pkt);
        break;

    case SUBCMD60_USED_TECH:
        rv = handle_bb_used_tech(c, (subcmd_bb_used_tech_t*)pkt);
        break;

    case SUBCMD60_TAKE_DAMAGE:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_DEATH_SYNC:
        rv = handle_bb_death_sync(c, (subcmd_bb_death_sync_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_4E:
        rv = handle_bb_cmd_4e(c, (subcmd_bb_cmd_4e_t*)pkt);
        break;

    case SUBCMD60_REQ_SWITCH:
        rv = handle_bb_switch_req(c, (subcmd_bb_switch_req_t*)pkt);
        break;

    case SUBCMD60_WORD_SELECT:
        rv = handle_bb_word_select(c, (subcmd_bb_word_select_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_88:
        rv = handle_bb_Unknown_6x88(c, (subcmd_bb_Unknown_6x88_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_89:
        rv = handle_bb_player_died(c, (subcmd_bb_player_died_t*)pkt);
        break;

    case SUBCMD60_SELL_ITEM:
        rv = handle_bb_sell_item(c, (subcmd_bb_sell_item_t*)pkt);
        break;

    case SUBCMD60_DROP_POS:
        rv = handle_bb_drop_pos(c, (subcmd_bb_drop_pos_t*)pkt);
        break;

    case SUBCMD60_SORT_INV:
        rv = handle_bb_sort_inv(c, (subcmd_bb_sort_inv_t*)pkt);
        break;

    case SUBCMD60_MEDIC:
        rv = handle_bb_medic(c, (subcmd_bb_pkt_t*)pkt);
        break;

    case SUBCMD60_STEAL_EXP:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_CHARGE_ACT: // Charge action 充能动作
        rv = handle_bb_charge_act(c, (subcmd_bb_charge_act_t *)pkt);
        break;

    case SUBCMD60_REQ_EXP:
        rv = handle_bb_req_exp(c, (subcmd_bb_req_exp_pkt_t*)pkt);
        break;

    case SUBCMD60_EX_ITEM_TEAM:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_BATTLEMODE:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD60_GALLON_AREA:
        rv = handle_bb_gallon_area(c, (subcmd_bb_gallon_area_pkt_t*)pkt);
        break;

    case SUBCMD60_TAKE_DAMAGE1:
    case SUBCMD60_TAKE_DAMAGE2:
        rv = handle_bb_take_damage(c, (subcmd_bb_take_damage_t*)pkt);
        break;

    case SUBCMD60_MENU_REQ:
        rv = handle_bb_menu_req(c, (subcmd_bb_menu_req_t*)pkt);
        break;

    case SUBCMD60_CREATE_ENEMY_SET:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x04);
        break;

    case SUBCMD60_CREATE_PIPE:
        rv = handle_bb_create_pipe(c, (subcmd_bb_pipe_pkt_t*)pkt);
        break;

    case SUBCMD60_SPAWN_NPC:
        rv = handle_bb_spawn_npc(c, pkt);
        break;

    case SUBCMD60_UNKNOW_6A:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_SET_FLAG:
        rv = handle_bb_set_flag(c, (subcmd_bb_set_flag_t*)pkt);
        break;

    case SUBCMD60_KILL_MONSTER:
        rv = handle_bb_killed_monster(c, (subcmd_bb_killed_monster_t*)pkt);
        break;

    case SUBCMD60_SYNC_REG:
        rv = handle_bb_sync_reg(c, (subcmd_bb_sync_reg_t*)pkt);
        break;

    case SUBCMD60_CMODE_GRAVE:
        rv = handle_bb_cmode_grave(c, pkt);
        break;

        /*挑战模式 触发*/
    case SUBCMD60_UNKNOW_7D:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        //rv = handle_bb_cmd_check_game_size(c, pkt, 0x06);
        break;

        /*挑战模式 触发*/
    case SUBCMD60_UNKNOW_8A:
        rv = handle_bb_Unknown_6x8A(c, (subcmd_bb_Unknown_6x8A_t*)pkt);
        break;

    case SUBCMD60_SET_TECH_LEVEL_OVERRIDE:
        rv = handle_bb_set_technique_level_override(c, (subcmd_bb_set_technique_level_override_t*)pkt);
        break;

        /*挑战模式 触发*/
    case SUBCMD60_TIMED_SWITCH_ACTIVATED:
        rv = handle_bb_timed_switch_activated(c, (subcmd_bb_timed_switch_activated_t*)pkt);
        break;

    case SUBCMD60_SHOP_INV:
        rv = handle_bb_shop_inv(c, (subcmd_bb_shop_inv_t*)pkt);
        break;

        // 0x53: Unknown (指令生效范围; 仅限游戏)
    case SUBCMD60_UNKNOW_53:
        rv = handle_bb_Unknown_6x53(c, (subcmd_bb_Unknown_6x53_t*)pkt);
        break;

    case SUBCMD60_WARP_55:
        rv = handle_bb_map_warp_55(c, (subcmd_bb_map_warp_t*)pkt);
        break;

    case SUBCMD60_LOBBY_ACTION:
        rv = handle_bb_lobby_act(c, (subcmd_bb_lobby_act_t*)pkt);
        break;

    case SUBCMD60_GOGO_BALL:
        rv = handle_bb_gogo_ball(c, (subcmd_bb_gogo_ball_t*)pkt);
        break;

    case SUBCMD60_GDRAGON_ACT:
        rv = handle_bb_gol_dragon_act(c, (subcmd_bb_gol_dragon_act_t*)pkt);
        break;

    case SUBCMD60_LOBBY_CHAIR:
        rv = handle_bb_chair_dir(c, (subcmd_bb_chair_dir_t*)pkt);
        break;

    case SUBCMD60_CHAIR_DIR:
        rv = handle_bb_chair_dir(c, (subcmd_bb_chair_dir_t*)pkt);
        break;

    case SUBCMD60_CHAIR_MOVE:
        rv = handle_bb_chair_dir(c, (subcmd_bb_chair_dir_t*)pkt);
        break;

    case SUBCMD60_TRADE_DONE:
        rv = handle_bb_trade_done(c, (subcmd_bb_trade_t*)pkt);
        break;

    case SUBCMD60_LEVEL_UP_REQ:
        rv = handle_bb_level_up_req(c, (subcmd_bb_levelup_req_t*)pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x60 指令: 0x%02X", type);
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
        sent = 0;
        break;

    case SUBCMD60_FINISH_LOAD:
        if (l->type == LOBBY_TYPE_LOBBY) {
            for (i = 0; i < l->max_clients; ++i) {
                if (l->clients[i] && l->clients[i] != c &&
                    subcmd_send_pos(c, l->clients[i])) {
                    rv = -1;
                    break;
                }
            }
        }
    }

    /* Broadcast anything we don't care to check anything about. */
    if (!sent)
        rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/* 原始 62 6D 指令函数 */
int subcmd_bb_handle_one2(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* dest;
    uint8_t type = pkt->type;
    int rv = -1;
    uint32_t dnum = LE32(pkt->hdr.flags);

    /* Ignore these if the client isn't in a lobby or team. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    /* Find the destination. */
    dest = l->clients[dnum];

    /* The destination is now offline, don't bother sending it. */
    if (!dest) {
        pthread_mutex_unlock(&l->mutex);
        return 0;
    }

    switch (type) {
    case SUBCMD62_GUILDCARD:
        /* Make sure the recipient is not ignoring the sender... */
        if (client_has_ignored(dest, c->guildcard)) {
            rv = 0;
            break;
        }

        rv = handle_bb_gcsend(c, dest);
        break;

    case SUBCMD62_PICK_UP:
        rv = handle_bb_pick_up(c, (subcmd_bb_pick_up_t*)pkt);
        break;

    case SUBCMD62_SHOP_REQ:
        rv = handle_bb_shop_req(c, (subcmd_bb_shop_req_t*)pkt);
        break;

    case SUBCMD62_OPEN_BANK:
        rv = handle_bb_bank(c, (subcmd_bb_bank_open_t*)pkt);
        break;

    case SUBCMD62_BANK_ACTION:
        rv = handle_bb_bank_action(c, (subcmd_bb_bank_act_t*)pkt);
        break;

    case SUBCMD62_ITEMREQ:
    case SUBCMD62_BITEMREQ:
        /* Unlike earlier versions, we have to handle this here... */
        rv = l->dropfunc(c, l, pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x62 指令: 0x%02X", type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
#endif /* BB_LOG_UNKNOWN_SUBS */
        /* Forward the packet unchanged to the destination. */
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

/* 原始 60 指令函数 */
int subcmd_bb_handle_bcast2(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0, sent = 1, i;

    /* Ignore these if the client isn't in a lobby or team. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    switch (type) {
    case SUBCMD60_SYMBOL_CHAT://间隔1秒可以正常发送
        if (time_check(c->subcmd_cooldown[type], 1))
            rv = handle_bb_symbol_chat(c, (subcmd_bb_symbol_chat_t*)pkt);
        else
            sent = 1;
        break;

    case SUBCMD60_HIT_MONSTER:
        rv = handle_bb_mhit(c, (subcmd_bb_mhit_pkt_t*)pkt);
        break;

    case SUBCMD60_SET_AREA:
    case SUBCMD60_INTER_LEVEL_WARP:
        rv = handle_bb_set_area(c, (subcmd_bb_set_area_t*)pkt);
        break;

    case SUBCMD60_EQUIP:
        rv = handle_bb_equip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD60_REMOVE_EQUIP:
        rv = handle_bb_unequip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD60_DROP_ITEM:
        rv = handle_bb_drop_item(c, (subcmd_bb_drop_item_t*)pkt);
        break;

    case SUBCMD60_SET_POS_3E:
    case SUBCMD60_SET_POS_3F:
        rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
        break;

    case SUBCMD60_MOVE_SLOW:
    case SUBCMD60_MOVE_FAST:
        rv = handle_bb_move(c, (subcmd_bb_move_t*)pkt);
        break;

    case SUBCMD60_DELETE_ITEM:
        rv = handle_bb_destroy_item(c, (subcmd_bb_destroy_item_t*)pkt);
        break;

    case SUBCMD60_WORD_SELECT:
        rv = handle_bb_word_select(c, (subcmd_bb_word_select_t*)pkt);
        break;

    case SUBCMD60_DROP_POS:
        rv = handle_bb_drop_pos(c, (subcmd_bb_drop_pos_t*)pkt);
        break;

    case SUBCMD60_SORT_INV:
        rv = handle_bb_sort_inv(c, (subcmd_bb_sort_inv_t*)pkt);
        break;

    case SUBCMD60_MEDIC:
        rv = handle_bb_medic(c, (subcmd_bb_pkt_t*)pkt);
        break;

    case SUBCMD60_REQ_EXP:
        rv = handle_bb_req_exp(c, (subcmd_bb_req_exp_pkt_t*)pkt);
        break;

    case SUBCMD60_USED_TECH:
        rv = handle_bb_used_tech(c, (subcmd_bb_used_tech_t*)pkt);
        break;

    case SUBCMD60_TAKE_DAMAGE1:
    case SUBCMD60_TAKE_DAMAGE2:
        rv = handle_bb_take_damage(c, (subcmd_bb_take_damage_t*)pkt);
        break;

    case SUBCMD60_SPAWN_NPC:
        rv = handle_bb_spawn_npc(c, pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x60 指令: 0x%02X", type);
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
        sent = 0;
        break;

    case SUBCMD60_FINISH_LOAD:
        if (l->type == LOBBY_TYPE_LOBBY) {
            for (i = 0; i < l->max_clients; ++i) {
                if (l->clients[i] && l->clients[i] != c &&
                    subcmd_send_pos(c, l->clients[i])) {
                    rv = -1;
                    break;
                }
            }
        }

    case SUBCMD60_LOAD_22:
    case SUBCMD60_TALK_NPC:
    case SUBCMD60_DONE_NPC:
    case SUBCMD60_LOAD_3B:
    case SUBCMD60_MENU_REQ:
    case SUBCMD60_WARP_55:
    case SUBCMD60_LOBBY_ACTION:
    case SUBCMD60_GOGO_BALL:
    case SUBCMD60_LOBBY_CHAIR:
    case SUBCMD60_CHAIR_DIR:
    case SUBCMD60_CHAIR_MOVE:
        sent = 0;
    }

    /* Broadcast anything we don't care to check anything about. */
    if (!sent)
        rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    pthread_mutex_unlock(&l->mutex);

    return rv;
}
