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
#include "subcmd_send_bb.h"
#include "subcmd_bb_handle_60.h"
#include "shop.h"
#include "pmtdata.h"
#include "clients.h"
#include "ship_packets.h"
#include "utils.h"
#include "iitems.h"
#include "word_select.h"
#include "scripts.h"
#include "shipgate.h"
#include "quest_functions.h"
#include "mag_bb.h"

// 当客户端发送游戏命令时, 调用此文件中的函数
// 指令集
// (60, 62, 6C, 6D, C9, CB).

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

static int handle_bb_cmd_check_size(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! size %02X",
            c->guildcard, pkt->type, size);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

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
        inptr = (char*)&src->pl->bb.character.name;
        outptr = dc.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)src->bb_pl->guildcard_desc;
        outptr = dc.guildcard_desc;

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
        dc.section = src->pl->bb.character.dress_data.section;
        dc.ch_class = src->pl->bb.character.dress_data.ch_class;
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
        memcpy(pc.name, &src->pl->bb.character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
        memcpy(pc.guildcard_desc, src->bb_pl->guildcard_desc, sizeof(src->bb_pl->guildcard_desc));

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
        pc.section = src->pl->bb.character.dress_data.section;
        pc.ch_class = src->pl->bb.character.dress_data.ch_class;

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
        out = 24;
        inptr = (char*)&src->pl->bb.character.name.char_name[0];
        outptr = gc.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)src->bb_pl->guildcard_desc;
        outptr = gc.guildcard_desc;

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
        gc.section = src->pl->bb.character.dress_data.section;
        gc.ch_class = src->pl->bb.character.dress_data.ch_class;

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
        out = 24;
        inptr = (char*)&src->pl->bb.character.name;
        outptr = xb.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 512;
        inptr = (char*)src->bb_pl->guildcard_desc;
        outptr = xb.guildcard_desc;

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
        xb.section = src->pl->bb.character.dress_data.section;
        xb.ch_class = src->pl->bb.character.dress_data.ch_class;

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
        memcpy(bb.name, &src->pl->bb.character.name, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
        memcpy(bb.guild_name, src->bb_opts->guild_name, sizeof(src->bb_opts->guild_name));
        memcpy(bb.guildcard_desc, src->bb_pl->guildcard_desc, sizeof(src->bb_pl->guildcard_desc));
        bb.disable_udp = 1;
        bb.language = src->language_code;
        bb.section = src->pl->bb.character.dress_data.section;
        bb.ch_class = src->pl->bb.character.dress_data.ch_class;

        return send_pkt_bb(dest, (bb_pkt_hdr_t*)&bb);
    }
    }

    return 0;
}

static int handle_bb_pick_up(ship_client_t* c, subcmd_bb_pick_up_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int pick_count;
    uint32_t item, tmp;
    iitem_t iitem_data = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中拾取了物品!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x14) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误的拾取数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* 尝试从大厅物品栏中移除... */
    pick_count = lobby_remove_item_locked(l, pkt->item_id, &iitem_data);
    if (pick_count < 0) {
        return -1;
    }
    else if (pick_count > 0) {
        /* 假设别人已经捡到了, 就忽略它... */
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

            /* Add the item to the client's inventory. */
            pick_count = add_item_to_client(c, &iitem_data);

            if (pick_count == -1)
                return 0;

            //c->bb_pl->inv.item_count += pick_count;
            //c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;
        }
    }

    /* 让所有人都知道是该客户端捡到的，并将其从所有人视线中删除. */
    subcmd_send_lobby_bb_create_inv_item(c, iitem_data.data, 1);

    return subcmd_send_bb_pick_item(c, pkt->area, iitem_data.data.item_id);
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
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return rv;
    }

    send_msg(d, BB_SCROLL_MSG_TYPE, "%s", __(c, "\tE\tC6交易功能存在故障，暂时关闭"));
    send_msg(c, BB_SCROLL_MSG_TYPE, "%s", __(c, "\tE\tC6交易功能存在故障，暂时关闭"));

    DBG_LOG("GC %u -> %u", c->guildcard, d->guildcard);

    display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

static int handle_bb_shop_req(ship_client_t* c, subcmd_bb_shop_req_t* req) {
    lobby_t* l = c->cur_lobby;
    block_t* b = c->cur_block;
    item_t item_data;
    uint32_t shop_type = LE32(req->shop_type);
    uint8_t num_items = 9 + (mt19937_genrand_int32(&b->rng) % 4);

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    for (uint8_t i = 0; i < num_items; ++i) {
        memset(&c->game_data->shop_items[i], 0, sizeof(item_t));
        memset(&item_data, 0, sizeof(item_t));

        switch (shop_type) {
        case BB_SHOPTYPE_TOOL:// 工具商店
            item_data = create_bb_shop_item(l->difficulty, 3, &b->rng);
            break;

        case BB_SHOPTYPE_WEAPON:// 武器商店
            item_data = create_bb_shop_item(l->difficulty, 0, &b->rng);
            break;

        case BB_SHOPTYPE_ARMOR:// 装甲商店
            item_data = create_bb_shop_item(l->difficulty, 1, &b->rng);
            break;

        default:
            ERR_LOG("菜单类型缺失 shop_type = %d", shop_type);
            return -1;
        }

        item_data.item_id = generate_item_id(l, c->client_id);

        memcpy(&c->game_data->shop_items[i], &item_data, sizeof(item_t));
    }

    return subcmd_bb_send_shop(c, shop_type, num_items);
}

static int handle_bb_shop_buy(ship_client_t* c, subcmd_bb_shop_buy_t* pkt) {
    lobby_t* l = c->cur_lobby;
    iitem_t ii = { 0 };
    int found = -1;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品购买数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    DBG_LOG("购买 %d 个物品 %02X %04X", pkt->num_bought, pkt->unknown_a1, pkt->shdr.unused);

    /* 填充物品数据头 */
    ii.present = LE16(1);
    ii.tech = LE16(0);
    ii.flags = LE32(0);

    /* 填充物品数据 */
    memcpy(&ii.data.data_b[0], &c->game_data->shop_items[pkt->shop_item_index].data_b[0], sizeof(item_t));

    /* 如果是堆叠物品 */
    if (pkt->num_bought <= stack_size_for_item(ii.data)) {
        ii.data.data_b[5] = pkt->num_bought;
    }
    else {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品购买数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return 0;
    }

    l->item_player_id[c->client_id] = pkt->new_inv_item_id;
    ii.data.item_id = l->item_player_id[c->client_id]++;

    print_iitem_data(&ii, 0, c->version);

    found = add_item_to_client(c, &ii);

    if (found == -1) {
        ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法获得物品!",
            c->guildcard);
        return -1;
    }

    //c->bb_pl->inv.item_count += found;
    //c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

    uint32_t price = ii.data.data2_l * pkt->num_bought;
    subcmd_send_bb_delete_meseta(c, price, 0);

    return subcmd_send_lobby_bb_create_inv_item(c, ii.data, 1);
}

static int handle_bb_item_tekk(ship_client_t* c, subcmd_bb_tekk_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    block_t* b = c->cur_block;
    subcmd_bb_tekk_identify_result_t i_res = { 0 };
    uint32_t tek_item_slot = 0, attrib = 0;
    uint8_t percent_mod = 0;

    if (c->version == CLIENT_VERSION_BB) {
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
            ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
            return -2;
        }

        tek_item_slot = find_iitem_slot(&c->bb_pl->inv, pkt->item_id);

        /* 获取鉴定物品的内存指针 */
        iitem_t* id_result = &(c->game_data->identify_result);

        /* 获取背包鉴定物品所在位置的数据 */
        id_result->data = c->bb_pl->inv.iitems[tek_item_slot].data;

        if (c->game_data->gm_debug)
            print_iitem_data(id_result, tek_item_slot, c->version);

        if (id_result->data.data_b[0] != ITEM_TYPE_WEAPON) {
            ERR_LOG("GC %" PRIu32 " 发送无法鉴定的物品!",
                c->guildcard);
            return -3;
        }

        // 技能属性提取和随机数处理
        attrib = id_result->data.data_b[4] & ~(0x80);

        if (attrib < 0x29) {
            id_result->data.data_b[4] = tekker_attributes[(attrib * 3) + 1];
            uint32_t mt_result_1 = mt19937_genrand_int32(&b->rng) % 100;
            if (mt_result_1 > 70)
                id_result->data.data_b[4] += mt_result_1 % ((tekker_attributes[(attrib * 3) + 2] - tekker_attributes[(attrib * 3) + 1]) + 1);
        } else
            id_result->data.data_b[4] = 0;

        // 百分比修正处理
        uint32_t mt_result_2 = mt19937_genrand_int32(&b->rng) % 10;

        switch (mt_result_2)
        {
        case 0:
        case 1:
            percent_mod = -10;
            break;
        case 2:
        case 3:
            percent_mod = -5;
            break;
        case 4:
        case 5:
            percent_mod = 5;
            break;
        case 6:
        case 7:
            percent_mod = 10;
            break;
        }

        // 各属性值修正处理
        if (!(id_result->data.data_b[6] & 128) && (id_result->data.data_b[7] > 0))
            id_result->data.data_b[7] += percent_mod;

        if (!(id_result->data.data_b[8] & 128) && (id_result->data.data_b[9] > 0))
            id_result->data.data_b[9] += percent_mod;

        if (!(id_result->data.data_b[10] & 128) && (id_result->data.data_b[11] > 0))
            id_result->data.data_b[11] += percent_mod;

        if (!id_result->data.item_id) {
            ERR_LOG("GC %" PRIu32 " 未发送需要鉴定的物品!",
                c->guildcard);
            return -4;
        }

        if (id_result->data.item_id != pkt->item_id) {
            ERR_LOG("GC %" PRIu32 " 接受的物品ID与以前请求的物品ID不匹配 !",
                c->guildcard);
            return -5;
        }

        subcmd_send_bb_delete_meseta(c, 100, 0);

        c->drop_item_id = id_result->data.item_id;
        c->drop_amt = 1;

        return subcmd_send_bb_create_tekk_item(c, id_result->data);
    } else
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

}

static int handle_bb_item_tekked(ship_client_t* c, subcmd_bb_accept_item_identification_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t i = 0;
    int found = -1;

    if (c->version == CLIENT_VERSION_BB) {

        iitem_t* id_result = &(c->game_data->identify_result);

        if (!id_result->data.item_id) {
            ERR_LOG("no identify result present");
            return -1;
        }

        if (id_result->data.item_id != pkt->item_id) {
            ERR_LOG("identify_result.data.item_id != pkt->item_id");
            return -1;
        }

        for (i = 0; i < c->pl->bb.inv.item_count; i++)
            if (c->pl->bb.inv.iitems[i].data.item_id == id_result->data.item_id) {
                ERR_LOG("GC %" PRIu32 " 没有鉴定任何装备或该装备来自非法所得!",
                    c->guildcard);
                return -1;
            }

        //for (i = 0; i < l->item_count; i++) {
        //    uint32_t item_index = l->item_list[i];
        //    if (l->item_id_to_lobby_item[item_index].inv_item.data.item_id == id_result->data.item_id) {
        //        memset(&l->item_id_to_lobby_item[item_index], 0, sizeof(fitem_t));
        //        l->item_list[i] = EMPTY_STRING;
        //        break;
        //    }
        //}

        //cleanup_lobby_item(l);

        /* 尝试从大厅物品栏中移除... */
        found = lobby_remove_item_locked(l, pkt->item_id, id_result);
        if (found < 0) {
            /* 未找到需要移除的物品... */
            ERR_LOG("GC %" PRIu32 " 鉴定的物品不存在!",
                c->guildcard);
            return -1;
        }
        else if (found > 0) {
            /* 假设别人已经捡到了, 就忽略它... */
            ERR_LOG("GC %" PRIu32 " 鉴定的物品已被拾取!",
                c->guildcard);
            return 0;
        }
        else {

            if (add_item_to_client(c, id_result) == -1) {
                ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法获得物品!",
                    c->guildcard);
                return -1;
            }

            /* 初始化临时鉴定的物品数据 */
            memset(&c->game_data->identify_result, 0, sizeof(iitem_t));

            c->drop_item_id = 0xFFFFFFFF;
            c->drop_amt = 0;

            return subcmd_send_lobby_bb_create_inv_item(c, id_result->data, 0);
        }

    } else
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
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
        ERR_LOG("GC %" PRIu32 " 在大厅中操作银行!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0018) || pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的银行操作数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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

            stack = stack_size_for_item(iitem.data);

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
            c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

            /* Fill in the bank item. */
            if (stack) {
                iitem.data.data_b[5] = pkt->item_amount;
                bitem.amount = LE16(pkt->item_amount);
            }
            else {
                bitem.amount = LE16(1);
            }

            bitem.show_flags = LE16(1);
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
                ERR_LOG("GC %" PRIu32 " 从银行中取出无效物品!", c->guildcard);
                return -1;
            }

            /* 已获得银行的物品数据, 将其添加至临时背包数据中... */
            iitem.present = LE16(0x0001);
            iitem.tech = LE16(0x0000);
            iitem.flags = LE32(0);
            ic[0] = iitem.data.data_l[0] = bitem.data.data_l[0];
            ic[1] = iitem.data.data_l[1] = bitem.data.data_l[1];
            ic[2] = iitem.data.data_l[2] = bitem.data.data_l[2];
            iitem.data.item_id = LE32(l->bitem_player_id[c->client_id]);
            iitem.data.data2_l = bitem.data.data2_l;
            ++l->bitem_player_id[c->client_id];

            /* 新增至玩家背包中... */
            found = add_item_to_client(c, &iitem);

            if (found == -1) {
                /* Uh oh... Guess we should put it back in the bank... */
                item_deposit_to_bank(c, &bitem);
                return -1;
            }

            //c->bb_pl->inv.item_count += found;
            //c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

            /* 发送至房间中的客户端. */
            return subcmd_send_lobby_bb_create_inv_item(c, iitem.data, 1);
        }

    default:
        ERR_LOG("GC %" PRIu32 " 发送未知银行操作: %d!",
            c->guildcard, pkt->action);
        display_packet((uint8_t*)pkt, 0x18);
        return -1;
    }
}

static int handle_bb_guild_invite(ship_client_t* c, ship_client_t* d, subcmd_bb_guild_invite_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t invite_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;
    char guild_name_text[24];
    char inviter_name_text[24];

    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " 发送错误的公会邀请数据包!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gbk, guild_name_text, &pkt->guild_name[2], 24, sizeof(pkt->guild_name) - 4);
    istrncpy16_raw(ic_utf16_to_gbk, inviter_name_text, &pkt->inviter_name[2], 24, sizeof(pkt->inviter_name) - 4);

#ifdef DEBUG
    TEST_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ", type, invite_cmd, c->guildcard, d->guildcard, target_guildcard);
    display_packet((uint8_t*)pkt, len);
#endif // DEBUG

    switch (type)
    {
    case SUBCMD62_GUILD_INVITE1:

        switch (invite_cmd)
        {
            /* 公会邀请起始 检测双方的 公会情况 */
        case 0x00:
                if (d->bb_guild->data.guild_id != 0) {
                    d->guild_accept = 0;
                    DBG_LOG("被邀请方 GUILD ID %u", d->bb_guild->data.guild_id);
                    /* 到这就没了, 获取对方已经属于某个公会. */
                    send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法邀请玩家!"),
                        __(c, "\tC7对方已在公会中."));
                }
                else
                    d->guild_accept = 1;

            break;

            /* 未知功能 */
        case 0x01:
        default:
            ERR_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ", type, invite_cmd, c->guildcard, d->guildcard, target_guildcard);
            display_packet((uint8_t*)pkt, len);
            break;
        }

        break;

    case SUBCMD62_GUILD_INVITE2:

        switch (invite_cmd)
        {

            /* 公会邀请成功 检测对方的 公会情况 */
        case 0x02:
            if (c->guildcard == target_guildcard)
                if (c->bb_guild->data.guild_id != 0) {
                    c->guild_accept = 0;
                    DBG_LOG("被邀请方 GUILD ID %u", d->bb_guild->data.guild_id);
                    /* 到这就没了, 获取对方已经属于某个公会. */
                    send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4无法邀请玩家!"),
                        __(c, "\tC7对方已在公会中."));
                }
                else
                    c->guild_accept = 1;

            break;

            /* 对方拒绝加入公会 */
        case 0x03:
            c->guild_accept = 0;
            send_msg(d, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(d, "\tE\tC4对方拒绝加入公会."), inviter_name_text, guild_name_text);
            break;

            /* 公会邀请失败 给双方返回错误信息 */
        case 0x04:
            c->guild_accept = 0;
            send_msg(c, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(c, "\tE\tC4公会邀请失败."), inviter_name_text, guild_name_text);
            break;

        default:
            ERR_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ", type, invite_cmd, c->guildcard, d->guildcard, target_guildcard);
            display_packet((uint8_t*)pkt, len);
            break;
        }

        break;
    }

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

static int handle_bb_guild_trans(ship_client_t* c, ship_client_t* d, subcmd_bb_guild_master_trans_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t trans_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;
    char guild_name_text[24];
    char master_name_text[24];

    // 公会邀请 C1 & C2, 会长转让 CD & CE.                             ...y
//[2023年02月13日 19:20:13:191] 测试(subcmd-bb.c 1495): SUBCMD62_GUILD_MASTER_TRANS1
//( 00000000 )   64 00 62 00 00 00 00 00  CD 17 14 04 01 00 00 00    d.b.............
//( 00000010 )   00 00 00 00 09 00 42 00  31 00 32 00 33 00 00 00    .... .B.1.2.3...
//( 00000020 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000030 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000040 )   00 00 00 00 09 00 42 00  F8 76 B2 4E F8 76 31 72    .... .B..v.N.v1r
//( 00000050 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000060 )   00 00 BB 79                                         ...y
//[2023年02月13日 19:20:29:038] 测试(subcmd-bb.c 1501): SUBCMD62_GUILD_MASTER_TRANS2
//( 00000000 )   64 00 62 00 01 00 00 00  CE 17 85 00 60 EE 80 02    d.b.........`...
//( 00000010 )   01 00 00 00 09 00 42 00  31 00 00 00 00 00 00 00    .... .B.1.......
//( 00000020 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000030 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000040 )   00 00 00 00 09 00 42 00  F8 76 B2 4E F8 76 31 72    .... .B..v.N.v1r
//( 00000050 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000060 )   00 00 BB 79                                         ...y
    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " 发送错误的公会转让数据包!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gbk, guild_name_text, &pkt->guild_name[2], 24, sizeof(pkt->guild_name) - 4);
    istrncpy16_raw(ic_utf16_to_gbk, master_name_text, &pkt->master_name[2], 24, sizeof(pkt->master_name) - 4);

#ifdef DEBUG
    TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS 0x%02X 0x%08X c %u d %u", type, trans_cmd, c->guildcard, d->guildcard);
    display_packet((uint8_t*)pkt, len);
#endif // DEBUG

    switch (type) {
    case SUBCMD62_GUILD_MASTER_TRANS1:

        //switch (trans_cmd)
        //{
        //case 0x00:
        //case 0x01:
        //    if (c->bb_guild->data.guild_data.guild_priv_level != BB_GUILD_PRIV_LEVEL_MASTER) {
        //        ERR_LOG("GC %u 公会权限不足", c->guildcard);
        //        return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4公会权限不足!"),
        //            __(c, "\tC7您无权进行此操作."));
        //    }

        //    c->guild_master_exfer = trans_cmd;
        //    break;

        //default:
        //    ERR_LOG("SUBCMD62_GUILD_MASTER_TRANS 0x%02X 0x%08X c %u d %u", type, trans_cmd, c->guildcard, d->guildcard);
        //    break;
        //}

        if (c->bb_guild->data.guild_priv_level != BB_GUILD_PRIV_LEVEL_MASTER) {
            ERR_LOG("GC %u 公会权限不足", c->guildcard);
            return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4公会权限不足!"),
                __(c, "\tC7您无权进行此操作."));
        }

        c->guild_master_exfer = trans_cmd;
        break;

    case SUBCMD62_GUILD_MASTER_TRANS2:

        switch (trans_cmd)
        {
        case 0x01:
            d->guild_master_exfer = 1;
            send_msg(d, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(d, "\tE\tC4会长变更中."), master_name_text, guild_name_text);
            break;

            /* 公会转移成功 TODO 通知其他会员 */
        case 0x02:
            d->guild_master_exfer = 0;
            send_msg(d, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(d, "\tE\tC4会长已变更."), master_name_text, guild_name_text);
            break;

            /* 对方拒绝成为会长 */
        case 0x03:
            d->guild_master_exfer = 0;
            send_msg(d, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(d, "\tE\tC4对方拒绝成为会长."), master_name_text, guild_name_text);
            break;

            /* 公会转移失败 给双方返回错误信息 */
        case 0x04:
            d->guild_master_exfer = 0;
            send_msg(c, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(c, "\tE\tC4公会转移失败."), master_name_text, guild_name_text);
            break;

        default:
            ERR_LOG("SUBCMD62_GUILD_MASTER_TRANS 0x%02X 0x%08X c %u d %u", type, trans_cmd, c->guildcard, d->guildcard);
            break;
        }

        break;
    }

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
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

int handle_bb_burst_pldata(ship_client_t* c, ship_client_t* d,
    subcmd_bb_burst_pldata_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t ch_class = c->bb_pl->character.dress_data.ch_class;
    iitem_t* item;
    int i, rv = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅中触发传送中的玩家数据!", c->guildcard);
        return -1;
    }

    if ((c->version == CLIENT_VERSION_XBOX && d->version == CLIENT_VERSION_GC) ||
        (d->version == CLIENT_VERSION_XBOX && c->version == CLIENT_VERSION_GC)) {
        /* 扫描库存并在发送之前修复所有mag. */


        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* 如果项目是mag,那么我们必须交换数据的最后一个dword.否则,颜色和统计数据会变得一团糟 */
            if (item->data.data_b[0] == ITEM_TYPE_MAG) {
                item->data.data2_l = SWAP32(item->data.data2_l);
            }
        }
    }
    else {
        //pkt->guildcard = c->guildcard;

        //// Check techniques...检查魔法,如果是机器人就不该有魔法
        //if (!(c->equip_flags & EQUIP_FLAGS_DROID)) {
        //    for (i = 0; i < BB_MAX_TECH_LEVEL; i++) {
        //        //if (pkt->techniques[i] == 0xFF)
        //            //pkt->techniques[i] = 0x00;

        //        if (pkt->techniques[i] > max_tech_level[i].max_lvl[ch_class])
        //            rv = -1;

        //        if (rv) {
        //            ERR_LOG("GC %u 不该有 Lv%d.%s 魔法! 正常值 Lv%d.%s", pkt->guildcard,
        //                pkt->techniques[i], max_tech_level[i].tech_name, 
        //                max_tech_level[i].max_lvl[ch_class], max_tech_level[i].tech_name);

        //            /* 规范化数据包的魔法等级 */
        //            //pkt->techniques[i] = max_tech_level[i].max_lvl[ch_class];
        //            rv = 0;
        //        }
        //    }
        //}

        ////for (i = 0; i < BB_MAX_TECH_LEVEL; i++) {
        ////    // 学会所有技能 TODO 增加作弊开关
        ////    //c->bb_pl->character.techniques[i] = max_tech_level[i].max_lvl[ch_class];
        ////    pkt->techniques[i] = c->bb_pl->character.techniques[i];
        ////}

        //// 检测玩家角色结构
        //pkt->dress_data = c->bb_pl->character.dress_data;

        //memcpy(&pkt->name[0], &c->bb_pl->character.name[0], sizeof(c->bb_pl->character.name));

        //// Prevent crashing with NPC skins... 防止NPC皮肤崩溃
        //if (c->bb_pl->character.dress_data.v2flags) {
        //    pkt->dress_data.v2flags = 0x00;
        //    pkt->dress_data.version = 0x00;
        //    pkt->dress_data.v1flags = LE32(0x00000000);
        //    pkt->dress_data.costume = LE16(0x0000);
        //    pkt->dress_data.skin = LE16(0x0000);
        //}

        ///* 检测人物基础数值 */
        //pkt->stats.atp = c->bb_pl->character.disp.stats.atp;
        //pkt->stats.mst = c->bb_pl->character.disp.stats.mst;
        //pkt->stats.evp = c->bb_pl->character.disp.stats.evp;
        //pkt->stats.hp = c->bb_pl->character.disp.stats.hp;
        //pkt->stats.dfp = c->bb_pl->character.disp.stats.dfp;
        //pkt->stats.ata = c->bb_pl->character.disp.stats.ata;
        //pkt->stats.lck = c->bb_pl->character.disp.stats.lck;

        //for (i = 0; i < 10; i++)
        //    pkt->opt_flag[i] = c->bb_pl->character.disp.opt_flag[i];

        //pkt->level = c->bb_pl->character.disp.level;
        //pkt->exp = c->bb_pl->character.disp.exp;
        //pkt->meseta = c->bb_pl->character.disp.meseta;


        //// Could check inventory here 查看背包
        //pkt->inv.item_count = c->bb_pl->inv.item_count;

        //for (i = 0; i < MAX_PLAYER_INV_ITEMS; i++)
        //    memcpy(&pkt->inv.iitems[i], &c->bb_pl->inv.iitems[i], sizeof(iitem_t));

        //for (i = 0; i < sizeof(uint32_t); i++)
        //    memset(&pkt->unused[i], 0, sizeof(uint32_t));

        //pkt->shdr.size = pkt->hdr.pkt_len / 4;
    }

    //printf("%" PRIu32 " - %" PRIu32 "  %s \n", c->guildcard, pkt->guildcard, pkt->dress_data.guildcard_string);

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

static int handle_bb_warp_item(ship_client_t* c, subcmd_bb_warp_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t iitem_count, found, i, stack;
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
        stack = stack_size_for_item(backup_item.data);

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
        found = add_item_to_client(c, &backup_item);

        if (found == -1)
            return 0;
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
        c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

        if (!found) {
            l->drops_disabled = 1;
            return -1;
        }
    }

    return 0;
}

static int handle_bb_quest_reward_meseta(ship_client_t* c, subcmd_bb_quest_reward_meseta_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int meseta;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 尝试获取错误的任务美赛塔奖励!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    meseta = pkt->amount;

    if (meseta < 0) {
        meseta = -meseta;

        if (meseta > (int)c->bb_pl->character.disp.meseta) {
            c->bb_pl->character.disp.meseta = 0;
        }
        else
            c->bb_pl->character.disp.meseta -= meseta;

        return 0;

    }
    else {
        iitem_t ii;
        memset(&ii, 0, sizeof(iitem_t));
        ii.data.data_b[0] = ITEM_TYPE_MESETA;
        ii.data.data2_l = meseta;
        ii.data.item_id = generate_item_id(l, 0xFF);

        if (add_item_to_client(c, &ii) == -1) {
            ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法获得物品!",
                c->guildcard);
            return -1;
        }

        return subcmd_send_lobby_bb_create_inv_item(c, ii.data, 0);
    }
}

static int handle_bb_quest_reward_item(ship_client_t* c, subcmd_bb_quest_reward_item_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    iitem_t ii;
    memset(&ii, 0, sizeof(iitem_t));
    ii.data = pkt->item_data;
    ii.data.item_id = generate_item_id(l, 0xFF);

    if (add_item_to_client(c, &ii) == -1) {
        ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法获得物品!",
            c->guildcard);
        return -1;
    }

    return subcmd_send_lobby_bb_create_inv_item(c, ii.data, 0);
}

/* 处理BB 0x62/0x6D 数据包. */
int subcmd_bb_handle_one(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* dest;
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->type;
    uint8_t size = pkt->size;
    int rv = -1;
    uint32_t dnum = LE32(pkt->hdr.flags);

    /* Ignore these if the client isn't in a lobby. */
    if (!l)
        return 0;

    //DBG_LOG("client num %d 0x62/0x6D 指令: 0x%02X", dnum, type);

    pthread_mutex_lock(&l->mutex);

    /* Find the destination. */
    dest = l->clients[dnum];

    /* The destination is now offline, don't bother sending it. */
    if (!dest) {
        pthread_mutex_unlock(&l->mutex);
        return 0;
    }

    //subcmd_bb_626Dsize_check(c, pkt);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;
        switch (type) {
        case SUBCMD62_BURST1://6D //其他大厅跃迁进房时触发 1
        case SUBCMD62_BURST2://6B //其他大厅跃迁进房时触发 2
        case SUBCMD62_BURST3://6C //其他大厅跃迁进房时触发 3
        case SUBCMD62_BURST4://6E //其他大厅跃迁进房时触发 4
            if (l->flags & LOBBY_FLAG_QUESTING)
                rv = lobby_enqueue_burst_bb(l, c, (bb_pkt_hdr_t*)pkt);
            /* Fall through... */

        case SUBCMD62_BURST5://6F //其他大厅跃迁进房时触发 5
        case SUBCMD62_BURST6://71 //其他大厅跃迁进房时触发 6
            rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            break;

        case SUBCMD62_BURST_PLDATA://70 //其他大厅跃迁进房时触发 7
            rv = handle_bb_burst_pldata(c, dest, (subcmd_bb_burst_pldata_t*)pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x62 指令: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
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
        break;

    case SUBCMD62_OPEN_BANK:
        rv = handle_bb_bank(c, (subcmd_bb_bank_open_t*)pkt);
        break;

    case SUBCMD62_BANK_ACTION:
        rv = handle_bb_bank_action(c, (subcmd_bb_bank_act_t*)pkt);
        break;

    case SUBCMD62_GUILD_INVITE1:
    case SUBCMD62_GUILD_INVITE2:
        rv = handle_bb_guild_invite(c, dest, (subcmd_bb_guild_invite_t*)pkt);
        break;

    case SUBCMD62_GUILD_MASTER_TRANS1:
    case SUBCMD62_GUILD_MASTER_TRANS2:
        rv = handle_bb_guild_trans(c, dest, (subcmd_bb_guild_master_trans_t*)pkt);
        break;

    case SUBCMD62_CH_GRAVE_DATA:
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

    case SUBCMD62_QUEST_REWARD_MESETA:
        rv = handle_bb_quest_reward_meseta(c, (subcmd_bb_quest_reward_meseta_t*)pkt);
        break;

    case SUBCMD62_QUEST_REWARD_ITEM:
        rv = handle_bb_quest_reward_item(c, (subcmd_bb_quest_reward_item_t*)pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x62/0x6D 指令: 0x%02X", type);
        display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
#endif /* BB_LOG_UNKNOWN_SUBS */

    case SUBCMD62_BATTLE_CHAR_LEVEL_FIX:
    case SUBCMD62_GANBLING:
        UNK_CSPD(type, c->version, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

///////////////////////////////////////////////////////////////////////////////////////////////

int handle_bb_gol_dragon_act(ship_client_t* c, subcmd_bb_gol_dragon_act_t* pkt) {
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
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

/* 处理BB 0x60 数据包. */
int subcmd_bb_handle_bcast(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0, sent = 1, i = 0;

    /* 如果客户端不在大厅或者队伍中则忽略数据包. */
    if (!l)
        return 0;

#ifdef DEBUG_60

    DBG_LOG("玩家 0x60 指令: 0x%02X", type);

#endif // DEBUG_60

    pthread_mutex_lock(&l->mutex);

    subcmd_bb_60size_check(c, pkt);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        switch (type) {
        case SUBCMD60_SET_POS_3F://大厅跃迁时触发 1

        case SUBCMD60_SET_AREA_1F://大厅跃迁时触发 2

        case SUBCMD60_LOAD_3B://大厅跃迁时触发 3
        case SUBCMD60_BURST_DONE:
            /* TODO 将这个函数融合进房间函数中 */

            /* 0x7C 挑战模式 进入房间游戏未开始前触发*/
        case SUBCMD60_CMODE_GRAVE:
            rv = subcmd60_bb_handle[type](c, pkt);
            break;

        default:
            DBG_LOG("LOBBY_FLAG_BURSTING 0x60 指令: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {

        /* 0x05 此函数正常载入 */
    case SUBCMD60_SWITCH_CHANGED:

        /* 0x07 玩家聊天 */
    case SUBCMD60_SYMBOL_CHAT:

        /* 0x0A 此函数正常载入 */
    case SUBCMD60_HIT_MONSTER:

        /* 0x0B 此函数正常载入 */
    case SUBCMD60_HIT_OBJ:

        /* 0x0C 此函数正常载入 */
    case SUBCMD60_CONDITION_ADD:

        /* 0x0D 此函数正常载入 */
    case SUBCMD60_CONDITION_REMOVE:

        /* 0x12 EP1龙BOSS的动作 */
    case SUBCMD60_DRAGON_ACT:// Dragon actions

        /* 0x1F 此函数正常载入 */
    case SUBCMD60_SET_AREA_1F:

        /* 0x20 此函数正常载入 */
    case SUBCMD60_SET_AREA_20:

        /* 0x21 进入下一层传送 */
    case SUBCMD60_INTER_LEVEL_WARP:

        /* 0x22 设置玩家出现 */
    case SUBCMD60_LOAD_22://subcmd_set_player_visibility_6x22_6x23_t

        /* 0x23 玩家完成所有数据载入 */
    case SUBCMD60_FINISH_LOAD:

        /* 0x24 此函数正常载入 */
    case SUBCMD60_SET_POS_24:

        /* 0x25 此函数正常载入 */
    case SUBCMD60_EQUIP:

        /* 0x26 此函数正常载入 */
    case SUBCMD60_REMOVE_EQUIP:

        /* 0x27 使用物品 */
    case SUBCMD60_USE_ITEM:

        /* 0x28 喂养MAG */
    case SUBCMD60_FEED_MAG:

        /* 0x29 此函数正常载入 */
    case SUBCMD60_DELETE_ITEM:

        /* 0x2A 此函数正常载入 */
    case SUBCMD60_DROP_ITEM:

        /* 0x2C 与NPC交谈 */
    case SUBCMD60_TALK_NPC:

        /* 0x2D 与NPC交谈完成 */
    case SUBCMD60_DONE_NPC:

        /* 0x2F 被敌人攻击 */
    case SUBCMD60_HIT_BY_ENEMY:

        /* 0x31 医院治疗请求 */
    case SUBCMD60_MEDIC_REQ:

        /* 0x32 完成医院治疗 */
    case SUBCMD60_MEDIC_DONE:

        /* 0x3A TODO 未知 */
    case SUBCMD60_UNKNOW_3A:

        /* 0x3B TODO 地图载入时触发 */
    case SUBCMD60_LOAD_3B:

        /* 0x3E 此函数正常载入 */
    case SUBCMD60_SET_POS_3E:
        /* 0x3F 此函数正常载入 */
    case SUBCMD60_SET_POS_3F:

        /* 0x40 此函数正常载入 */
    case SUBCMD60_MOVE_SLOW:
        /* 0x42 此函数正常载入 */
    case SUBCMD60_MOVE_FAST:

        /* 0x43 此函数正常载入 */
    case SUBCMD60_ATTACK1:
        /* 0x44 此函数正常载入 */
    case SUBCMD60_ATTACK2:
        /* 0x45 此函数正常载入 */
    case SUBCMD60_ATTACK3:

        /* 0x46 此函数正常载入 */
    case SUBCMD60_OBJHIT_PHYS:
        /* 0x47 此函数正常载入 */
    case SUBCMD60_OBJHIT_TECH:

        /* 0x48 此函数正常载入 */
    case SUBCMD60_USED_TECH:

        /* 0x4A 此函数正常载入 */
    case SUBCMD60_DEFENSE_DAMAGE:

        /* 0x4B 此函数正常载入 */
    case SUBCMD60_TAKE_DAMAGE1:
        /* 0x4C 此函数正常载入 */
    case SUBCMD60_TAKE_DAMAGE2:

        /* 0x4D SUBCMD60_DEATH_SYNC */
    case SUBCMD60_DEATH_SYNC:

        /* 0x4E 未知 */
    case SUBCMD60_UNKNOW_4E:

        /* 0x4F SUBCMD60_PLAYER_SAVED */
    case SUBCMD60_PLAYER_SAVED:

        /* 0x50 游戏开关请求 */
    case SUBCMD60_SWITCH_REQ:

        /* 0x52 游戏菜单请求 */
    case SUBCMD60_MENU_REQ:

        /* 0x55 地图传送变化触发 */
    case SUBCMD60_WARP_55:

        /* 0x58 大厅动作 */
    case SUBCMD60_LOBBY_ACTION:

        /* 0x61 玩家升级请求 */
    case SUBCMD60_LEVEL_UP_REQ:

        /* 0x63 摧毁地面物品（当房间物品过多时） */
    case SUBCMD60_DESTROY_GROUND_ITEM:

        /* 0x67 此函数正常载入 */
    case SUBCMD60_CREATE_ENEMY_SET:

        /* 0x68 生成传送门 */
    case SUBCMD60_CREATE_PIPE:

        /* 0x69 生成NPC */
    case SUBCMD60_SPAWN_NPC:

        /* 0x6A TODO 未知 */
    case SUBCMD60_UNKNOW_6A:

        /* 0x74 此函数正常载入 */
    case SUBCMD60_WORD_SELECT:

        /* 0x75 设置任务标记 */
    case SUBCMD60_SET_FLAG:

        /* 0x76 此函数正常载入 */
    case SUBCMD60_KILL_MONSTER:

        /* 0x77 此函数正常载入 */
    case SUBCMD60_SYNC_REG:

        /* 0x79 动感足球函数 */
    case SUBCMD60_GOGO_BALL:

        /* 0x88 玩家箭头更换 */
    case SUBCMD60_ARROW_CHANGE:

        /* 0x89 玩家死亡 */
    case SUBCMD60_PLAYER_DIED:

        /* 0x8A 挑战模式 触发*/
    case SUBCMD60_UNKNOW_CH_8A:

        /* 0x8D 此函数正常载入 */
    case SUBCMD60_SET_TECH_LEVEL_OVERRIDE:

        /* 0x93 挑战模式 触发*/
    case SUBCMD60_TIMED_SWITCH_ACTIVATED:

        /* 0xA1 SUBCMD60_SAVE_PLAYER_ACT */
    case SUBCMD60_SAVE_PLAYER_ACT:

        /* 0xAB 座椅状态 触发*/
    case SUBCMD60_CHAIR_CREATE:
        /* 0xAE 座椅状态 触发*/
    case SUBCMD60_CHAIR_STATE:
        /* 0xAF 座椅状态 触发*/
    case SUBCMD60_CHAIR_TURN:
        /* 0xB0 座椅状态 触发*/
    case SUBCMD60_CHAIR_MOVE:

        /* 0xC0 售卖物品 */
    case SUBCMD60_SELL_ITEM:

        /* 0xC3 此函数正常载入 */
    case SUBCMD60_DROP_SPLIT_STACKED_ITEM:

        /* 0xC4 此函数正常载入 */
    case SUBCMD60_SORT_INV:

        /* 0xC5 此函数正常载入 */
    case SUBCMD60_MEDIC:

        /* 0xC6 偷取经验 */
    case SUBCMD60_STEAL_EXP:
        /* 0xC8 此函数正常载入 */
    case SUBCMD60_EXP_REQ:

        /* 0xCC 公会转换物品点数 */
    case SUBCMD60_GUILD_EX_ITEM:

        /* 0xD2 此函数正常载入 */
    case SUBCMD60_GALLON_AREA:
        rv = subcmd60_bb_handle[type](c, pkt);
        break;


        /* 0xD9 Momoka物品交换 */
    case SUBCMD60_EX_ITEM_MK:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        //DBG_LOG("暂未完成 0x60: 0x%02X\n", type);
        //display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
        sent = 0;
        break;

    }

    /* Broadcast anything we don't care to check anything about. */
    if (!sent)
        rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    printf("%d\n", rv);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}