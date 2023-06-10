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
#include "subcmd_send.h"
#include "shop.h"
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

// ���ͻ��˷�����Ϸ����ʱ, ���ô��ļ��еĺ���
// ָ�
// (60, 62, 6C, 6D, C9, CB).


static int handle_bb_cmd_check_size(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! size %02X",
            c->guildcard, pkt->type, size);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_lobby_size(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " ����Ϸ�д����˴��� %s ָ��!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! size %02X",
            c->guildcard, pkt->type, size);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_game_size(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸ������ %s ָ��!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸ������ %s ָ��!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

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
        memcpy(pc.name, &src->pl->bb.character.name[2], BB_CHARACTER_NAME_LENGTH * 2 + 4);
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

        /* ������ݲ�׼������.. */
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
    iitem_t iitem_data = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����ʰȡ����Ʒ!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x14) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " ���ʹ����ʰȡ����!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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

            /* Add the item to the client's inventory. */
            found = item_add_to_inv(c->bb_pl->inv.iitems,
                c->bb_pl->inv.item_count, &iitem_data);

            if (found == -1) {
                ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷�ʰȡ!",
                    c->guildcard);
                return -1;
            }

            c->bb_pl->inv.item_count += found;
        }
    }

    /* �������˶�֪���Ǹÿͻ��˼񵽵ģ��������������������ɾ��. */
    subcmd_send_bb_create_item(c, iitem_data.data, 1);

    return subcmd_send_bb_destroy_map_item(c, pkt->area, iitem_data.data.item_id);
}

static int handle_bb_item_req(ship_client_t* c, ship_client_t* d, subcmd_bb_itemreq_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! ���ݴ�С %02X",
            c->guildcard, pkt->shdr.type, pkt->shdr.size);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0030) || pkt->shdr.size != 0x0A) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! ���ݴ�С %02X",
            c->guildcard, pkt->shdr.type, pkt->shdr.size);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return rv;
    }

    if (pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! ���ݴ�С %02X",
            c->guildcard, pkt->shdr.type, pkt->shdr.size);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return rv;
    }

    send_msg(d, BB_SCROLL_MSG_TYPE, "%s", __(c, "\tE\tC6���׹��ܴ��ڹ��ϣ���ʱ�ر�"));
    send_msg(c, BB_SCROLL_MSG_TYPE, "%s", __(c, "\tE\tC6���׹��ܴ��ڹ��ϣ���ʱ�ر�"));

    DBG_LOG("GC %u -> %u", c->guildcard, d->guildcard);

    print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

static int handle_bb_shop_req(ship_client_t* c, subcmd_bb_shop_req_t* req) {
    lobby_t* l = c->cur_lobby;
    block_t* b = c->cur_block;
    sitem_t item_data = { 0 };
    uint32_t shop_type = LE32(req->shop_type);
    uint8_t num_items = 9 + (mt19937_genrand_int32(&b->rng) % 4);

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    for (uint8_t i = 0; i < num_items; ++i) {
        memset(&c->game_data->shop_items[i], 0, sizeof(sitem_t));

        switch (shop_type) {
        case BB_SHOPTYPE_TOOL:// �����̵�
            item_data = create_bb_shop_item(l->difficulty, 3, &b->rng);
            break;

        case BB_SHOPTYPE_WEAPON:// �����̵�
            item_data = create_bb_shop_item(l->difficulty, 0, &b->rng);
            break;

        case BB_SHOPTYPE_ARMOR:// װ���̵�
            item_data = create_bb_shop_item(l->difficulty, 1, &b->rng);
            break;

        default:
            ERR_LOG("�˵�����ȱʧ shop_type = %d", shop_type);
            return -1;
        }

        item_data.sitem_id = generate_item_id(l, c->client_id);

        memcpy(&c->game_data->shop_items[i], &item_data, sizeof(sitem_t));
    }

    return subcmd_bb_send_shop(c, shop_type, num_items);
}

static int handle_bb_shop_buy(ship_client_t* c, subcmd_bb_shop_buy_t* pkt) {
    lobby_t* l = c->cur_lobby;
    iitem_t ii = { 0 };

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �����𻵵���Ʒ��������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    DBG_LOG("������Ʒ");

    memcpy(&ii.data.data_l[0], &c->game_data->shop_items[pkt->shop_item_index].data_l[0], sizeof(ii.data));

    print_item_data(c, &ii.data);

    l->item_player_id[c->client_id] = pkt->new_inv_item_id;
    ii.data.item_id = l->item_player_id[c->client_id]++;

    //item_add_to_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count, ii);

    if (add_item(c, ii))
        ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷�ʰȡ!",
            c->guildcard);

    subcmd_send_bb_delete_meseta(c, ii.data.data2_l, 0);

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
            ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
                c->guildcard);
            return -1;
        }

        if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
            ERR_LOG("GC %" PRIu32 " �����𻵵���Ʒ��������!",
                c->guildcard);
            print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
            return -2;
        }

        tek_item_slot = item_get_inv_item_slot(c->bb_pl->inv, pkt->item_id);

        DBG_LOG("%04X %04X", tek_item_slot, pkt->item_id);

        print_item_data(c, &c->bb_pl->inv.iitems[tek_item_slot].data);

        if (c->bb_pl->inv.iitems[tek_item_slot].data.data_b[0] != 0x00) {
            ERR_LOG("GC %" PRIu32 " �����޷���������Ʒ!",
                c->guildcard);
            return -3;
        }

        c->game_data->identify_result = c->bb_pl->inv.iitems[tek_item_slot];
        attrib = c->game_data->identify_result.data.data_b[4] & ~(0x80);

        if (attrib < 0x29)
        {
            c->game_data->identify_result.data.data_b[4] = tekker_attributes[(attrib * 3) + 1];
            if ((mt19937_genrand_int32(&b->rng) % 100) > 70)
                c->game_data->identify_result.data.data_b[4] += mt19937_genrand_int32(&b->rng) % ((tekker_attributes[(attrib * 3) + 2] - tekker_attributes[(attrib * 3) + 1]) + 1);
        }
        else
            c->game_data->identify_result.data.data_b[4] = 0;

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
            c->game_data->identify_result.data.data_b[7] += percent_mod;

        if ((!(c->bb_pl->inv.iitems[tek_item_slot].data.data_b[8] & 128)) && (c->bb_pl->inv.iitems[tek_item_slot].data.data_b[9] > 0))
            c->game_data->identify_result.data.data_b[9] += percent_mod;

        if ((!(c->bb_pl->inv.iitems[tek_item_slot].data.data_b[10] & 128)) && (c->bb_pl->inv.iitems[tek_item_slot].data.data_b[11] > 0))
            c->game_data->identify_result.data.data_b[11] += percent_mod;

        if (!c->game_data->identify_result.data.item_id) {
            ERR_LOG("GC %" PRIu32 " δ������Ҫ��������Ʒ!",
                c->guildcard);
            return -4;
        }

        if (c->game_data->identify_result.data.item_id != pkt->item_id) {
            ERR_LOG("GC %" PRIu32 " ���ܵ���ƷID����ǰ�������ƷID��ƥ�� !",
                c->guildcard);
            return -5;
        }

        subcmd_send_bb_delete_meseta(c, 100, 0);

        /* �������ͷ */
        i_res.hdr.pkt_type = GAME_COMMAND0_TYPE;
        i_res.hdr.pkt_len = LE16(sizeof(i_res));
        i_res.hdr.flags = 0x00000000;
        i_res.shdr.type = SUBCMD60_UNKNOW_B9;
        i_res.shdr.size = sizeof(i_res) / 4;
        i_res.shdr.client_id = c->client_id;

        /* ������� */
        i_res.item = c->game_data->identify_result.data;

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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (!c->game_data->identify_result.data.item_id) {
        ERR_LOG("GC %" PRIu32 " û�м����κ�װ��!",
            c->guildcard);
        return -1;
    }

    return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

    /* TODO δ���*/
    uint32_t ch2, ch;

    for (ch = 0; ch < 4; ch++) {
        if (/*(l->slot_use[ch]) && (*/l->clients[ch]/*)*/) {
            for (ch2 = 0; ch2 < l->clients[ch]->pl->bb.inv.item_count; ch2++)
                if (l->clients[ch]->pl->bb.inv.iitems[ch2].data.item_id == c->game_data->identify_result.data.item_id) {
                    //Message_Box(L"", L"Item duplication attempt!", client, BigMes_Pkt1A, NULL, 94);
                    ERR_LOG("GC %" PRIu32 " û�м����κ�װ�����װ�����ԷǷ�����!",
                        c->guildcard);
                    return -1;
                }
        }
    }

    for (ch = 0; ch < l->item_count; l++) {
        uint32_t itemNum = l->item_list[ch];
        if (l->item_id_to_lobby_item[itemNum].inv_item.data.item_id == c->game_data->identify_result.data.item_id) {
            // Added to the game's inventory by the client...
            // Delete it and avoid duping...
            //���ͻ�����ӵ���ϷĿ¼�С�����
            //ɾ���������⸴�ơ���
            memset(&l->item_id_to_lobby_item[itemNum], 0, sizeof(fitem_t));
            l->item_list[ch] = EMPTY_STRING;
            break;
        }
    }

    clear_lobby_item(l);
    subcmd_send_bb_create_item(c, c->game_data->identify_result.data, 1);
    //Add_To_Inventory(&client->tekked, 1, 1, client);
    memset(&c->game_data->identify_result, 0, sizeof(iitem_t));

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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (req->hdr.pkt_len != LE16(0x10) || req->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " ���ʹ�����������ݰ�!",
            c->guildcard);
        return -1;
    }

    /* Clean up the user's bank first... */
    cleanup_bb_bank(c);

    /* ������ݲ�׼������ */
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

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0018) || pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " sent bad bank action!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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

            /* ����! */
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
                ERR_LOG("GC %" PRIu32 " ������ȡ���������������˴洢����!", c->guildcard);
                return -1;
            }

            bank = LE32(c->bb_pl->bank.meseta);

            if (amt > bank) {
                ERR_LOG("GC %" PRIu32 " ������ȡ�������������������п��!", c->guildcard);
                return -1;
            }

            c->bb_pl->character.disp.meseta = LE32((iitem_count + amt));
            c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
            c->bb_pl->bank.meseta = LE32((bank - amt));

            /* ��ȡ���������ø�֪�����ͻ���... */
            return 0;
        }
        else {
            /* ���Դ�������ȡ����Ʒ. */
            found = item_take_from_bank(c, pkt->item_id, pkt->item_amount, &bitem);

            if (found < 0) {
                ERR_LOG("GC %" PRIu32 " taking invalid item "
                    "from bank!", c->guildcard);
                return -1;
            }

            /* �ѻ�����е���Ʒ����, �����������ʱ����������... */
            iitem.present = LE16(0x0001);
            iitem.tech = LE16(0x0000);
            iitem.flags = 0;
            ic[0] = iitem.data.data_l[0] = bitem.data.data_l[0];
            ic[1] = iitem.data.data_l[1] = bitem.data.data_l[1];
            ic[2] = iitem.data.data_l[2] = bitem.data.data_l[2];
            iitem.data.item_id = LE32(l->bitem_player_id[c->client_id]);
            iitem.data.data2_l = bitem.data.data2_l;
            ++l->bitem_player_id[c->client_id];

            /* ��������ұ�����... */
            found = item_add_to_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count, &iitem);

            if (found < 0) {
                /* Uh oh... Guess we should put it back in the bank... */
                item_deposit_to_bank(c, &bitem);
                return -1;
            }

            c->bb_pl->inv.item_count += found;

            /* �����������еĿͻ���. */
            return subcmd_send_bb_create_item(c, iitem.data, 1);
        }

    default:
        ERR_LOG("GC %" PRIu32 " ����δ֪���в���: %d!",
            c->guildcard, pkt->action);
        print_payload((uint8_t*)pkt, 0x18);
        return -1;
    }
}

static int handle_bb_guild_invite(ship_client_t* c, ship_client_t* d, subcmd_bb_guild_invite_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t invite_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;

    // �������� C1 & C2, �᳤ת�� CD & CE.
//[2023��02��13�� 21:13:20:264] ����(subcmd-bb.c 1521): SUBCMD62_GUILD_INVITE1
//( 00000000 )   64 00 62 00 00 00 00 00  C1 17 80 3F 01 00 00 00    d.b........?....
//( 00000010 )   00 00 00 00 09 00 42 00  31 00 32 00 33 00 00 00    .... .B.1.2.3...
//( 00000020 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000030 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000040 )   00 00 00 00 09 00 42 00  F8 76 B2 4E F8 76 31 72    .... .B..v.N.v1r
//( 00000050 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000060 )   00 00 D9 01                                         ....
//[2023��02��13�� 21:13:25:223] ����(subcmd-bb.c 1533): SUBCMD62_GUILD_INVITE2
//( 00000000 )   64 00 62 00 01 00 00 00  C2 17 E9 6D 60 EE 80 02    d.b........m`...
//( 00000010 )   02 00 00 00 09 00 42 00  31 00 00 00 00 00 00 00    .... .B.1.......
//( 00000020 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000030 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000040 )   00 00 00 00 09 00 42 00  F8 76 B2 4E F8 76 31 72    .... .B..v.N.v1r
//( 00000050 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000060 )   00 00 D9 01                                         ....                                 ...y
    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " ���ʹ���Ĺ����������ݰ�!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //TEST_LOG("SUBCMD62_GUILD_INVITE%d c %u d %u", invite_cmd, c->guildcard, d->guildcard);

    //0x02 Ӧ��ʱ���ܹ�������ָ��
    if ((invite_cmd == 0x02) && (c->guildcard == target_guildcard))
        c->guild_accept = 1;

    //print_payload((uint8_t*)pkt, len);

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

static int handle_bb_guild_trans(ship_client_t* c, ship_client_t* d, subcmd_bb_guild_master_trans_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t trans_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;

    // �������� C1 & C2, �᳤ת�� CD & CE.                             ...y
//[2023��02��13�� 19:20:13:191] ����(subcmd-bb.c 1495): SUBCMD62_GUILD_MASTER_TRANS1
//( 00000000 )   64 00 62 00 00 00 00 00  CD 17 14 04 01 00 00 00    d.b.............
//( 00000010 )   00 00 00 00 09 00 42 00  31 00 32 00 33 00 00 00    .... .B.1.2.3...
//( 00000020 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000030 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000040 )   00 00 00 00 09 00 42 00  F8 76 B2 4E F8 76 31 72    .... .B..v.N.v1r
//( 00000050 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000060 )   00 00 BB 79                                         ...y
//[2023��02��13�� 19:20:29:038] ����(subcmd-bb.c 1501): SUBCMD62_GUILD_MASTER_TRANS2
//( 00000000 )   64 00 62 00 01 00 00 00  CE 17 85 00 60 EE 80 02    d.b.........`...
//( 00000010 )   01 00 00 00 09 00 42 00  31 00 00 00 00 00 00 00    .... .B.1.......
//( 00000020 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000030 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000040 )   00 00 00 00 09 00 42 00  F8 76 B2 4E F8 76 31 72    .... .B..v.N.v1r
//( 00000050 )   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00    ................
//( 00000060 )   00 00 BB 79                                         ...y
    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " ���ʹ���Ĺ���ת�����ݰ�!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS%d c %u d %u", trans_cmd, c->guildcard, d->guildcard);

    switch (type) {
    case SUBCMD62_GUILD_MASTER_TRANS1:

        if (c->bb_guild->guild_data.guild_priv_level != 0x00000040) {
            ERR_LOG("GC %u ����Ȩ�޲���", c->guildcard);
            return send_msg(c, MSG1_TYPE, "%s\n\n%s", __(c, "\tE\tC4����Ȩ�޲���!"),
                __(c, "\tC7����Ȩ���д˲���."));
        }

        c->guild_master_exfer = trans_cmd;

        //print_payload((uint8_t*)pkt, len);
        break;

    case SUBCMD62_GUILD_MASTER_TRANS2:
        TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS2 ָ��%d c %u d %u", trans_cmd, c->guildcard, d->guildcard);

        break;

    //default:
    //    print_payload((uint8_t*)pkt, len);
    //    break;
    }

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

static int handle_bb_62_check_game_loading(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸ������ָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size)
        return -1;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

int handle_bb_burst_pldata(ship_client_t* c, ship_client_t* d,
    subcmd_bb_burst_pldata_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t ch_class = c->bb_pl->character.disp.dress_data.ch_class;
    iitem_t* item;
    int i, rv = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " �ڴ����д��������е��������!", c->guildcard);
        return -1;
    }

    if ((c->version == CLIENT_VERSION_XBOX && d->version == CLIENT_VERSION_GC) ||
        (d->version == CLIENT_VERSION_XBOX && c->version == CLIENT_VERSION_GC)) {
        /* ɨ���沢�ڷ���֮ǰ�޸�����mag. */


        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* �����Ŀ��mag,��ô���Ǳ��뽻�����ݵ����һ��dword.����,��ɫ��ͳ�����ݻ���һ���� */
            if (item->data.data_b[0] == ITEM_TYPE_MAG) {
                item->data.data2_l = SWAP32(item->data.data2_l);
            }
        }
    }
    else {
        //pkt->guildcard = c->guildcard;

        //// Check techniques...���ħ��,����ǻ����˾Ͳ�����ħ��
        //if (!(c->equip_flags & EQUIP_FLAGS_DROID)) {
        //    for (i = 0; i < BB_MAX_TECH_LEVEL; i++) {
        //        //if (pkt->techniques[i] == 0xFF)
        //            //pkt->techniques[i] = 0x00;

        //        if (pkt->techniques[i] > max_tech_level[i].max_lvl[ch_class])
        //            rv = -1;

        //        if (rv) {
        //            ERR_LOG("GC %u ������ Lv%d.%s ħ��! ����ֵ Lv%d.%s", pkt->guildcard,
        //                pkt->techniques[i], max_tech_level[i].tech_name, 
        //                max_tech_level[i].max_lvl[ch_class], max_tech_level[i].tech_name);

        //            /* �淶�����ݰ���ħ���ȼ� */
        //            //pkt->techniques[i] = max_tech_level[i].max_lvl[ch_class];
        //            rv = 0;
        //        }
        //    }
        //}

        ////for (i = 0; i < BB_MAX_TECH_LEVEL; i++) {
        ////    // ѧ�����м��� TODO �������׿���
        ////    //c->bb_pl->character.techniques[i] = max_tech_level[i].max_lvl[ch_class];
        ////    pkt->techniques[i] = c->bb_pl->character.techniques[i];
        ////}

        //// �����ҽ�ɫ�ṹ
        //pkt->dress_data = c->bb_pl->character.disp.dress_data;

        //memcpy(&pkt->name[0], &c->bb_pl->character.name[0], sizeof(c->bb_pl->character.name));

        //// Prevent crashing with NPC skins... ��ֹNPCƤ������
        //if (c->bb_pl->character.disp.dress_data.v2flags) {
        //    pkt->dress_data.v2flags = 0x00;
        //    pkt->dress_data.version = 0x00;
        //    pkt->dress_data.v1flags = LE32(0x00000000);
        //    pkt->dress_data.costume = LE16(0x0000);
        //    pkt->dress_data.skin = LE16(0x0000);
        //}

        ///* ������������ֵ */
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


        //// Could check inventory here �鿴����
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
    uint32_t iitem_count, found, i;
    uint32_t wrap_id;
    iitem_t backup_item;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
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
            ERR_LOG("�޷�����ұ������Ƴ���Ʒ!");
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

        /* ����Ʒ����������. */
        found = item_add_to_inv(c->bb_pl->inv.iitems,
            c->bb_pl->inv.item_count, &backup_item);

        if (found == -1) {
            ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷�ʰȡ!",
                c->guildcard);
            //return -1;
        }

        c->bb_pl->inv.item_count += found;
        memcpy(&c->pl->bb.inv, &c->bb_pl->inv, sizeof(iitem_t));
    }
    else {
        ERR_LOG("GC %" PRIu32 " ������ƷID %d ʧ��!",
            c->guildcard, wrap_id);
        return -1;
    }

    return 0;
}

static int handle_bb_quest_oneperson_set_ex_pc(ship_client_t* c, subcmd_bb_quest_oneperson_set_ex_pc_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t iitem_count, found;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
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

static int handle_bb_quest_reward_meseta(ship_client_t* c, subcmd_bb_quest_reward_meseta_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int meseta;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " ���Ի�ȡ�������������������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    if (l->flags & LOBBY_FLAG_QUESTING) {
        meseta = pkt->amount;

        if (meseta < 0) {
            meseta = -meseta;
            c->bb_pl->character.disp.meseta -= meseta;
        } else {
            meseta += LE32(c->bb_pl->character.disp.meseta);

            /* Cap at 999,999 meseta. */
            if (meseta > 999999)
                meseta = 999999;

            c->bb_pl->character.disp.meseta = meseta;
        }

        c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;

        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
    }

    ERR_LOG("GC %" PRIu32 " ���Ի�ȡ�������������������!",
        c->guildcard);
    return -1;
}

static int handle_bb_quest_reward_item(ship_client_t* c, subcmd_bb_quest_reward_item_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
}

/* ����BB 0x62/0x6D ���ݰ�. */
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
        //DBG_LOG("LOBBY_FLAG_BURSTING 0x62 ָ��: 0x%02X", type);

        switch (type) {
        case SUBCMD62_BURST1://6D //��������ԾǨ����ʱ���� 1
        case SUBCMD62_BURST2://6B //��������ԾǨ����ʱ���� 2
        case SUBCMD62_BURST3://6C //��������ԾǨ����ʱ���� 3
        case SUBCMD62_BURST4://6E //��������ԾǨ����ʱ���� 4
            if (l->flags & LOBBY_FLAG_QUESTING)
                rv = lobby_enqueue_burst_bb(l, c, (bb_pkt_hdr_t*)pkt);
            /* Fall through... */

        case SUBCMD62_BURST5://6F //��������ԾǨ����ʱ���� 5
        case SUBCMD62_BURST6://71 //��������ԾǨ����ʱ���� 6
            rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            break;

        case SUBCMD62_BURST_PLDATA://70 //��������ԾǨ����ʱ���� 7
            rv = handle_bb_burst_pldata(c, dest, (subcmd_bb_burst_pldata_t*)pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x62 ָ��: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        //DBG_LOG("LOBBY_FLAG_BURSTING DONE 0x62 ָ��: 0x%02X", type);
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
        DBG_LOG("δ֪ 0x62/0x6D ָ��: 0x%02X", type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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

static int handle_bb_switch_changed(ship_client_t* c, subcmd_bb_switch_changed_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ��������˻���!",
            c->guildcard);
        rv = -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->data.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X!",
            c->guildcard, pkt->data.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }
//[2023��02��09�� 22:51:33:981] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   04 00 05 03                                         ....
//[2023��02��09�� 22:51:34:017] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   04 00 05 03                                         ....
//[2023��02��09�� 22:51:34:054] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   05 00 05 01                                         ....
//[2023��02��09�� 22:51:34:089] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   06 00 05 01                                         ....
//[2023��02��09�� 22:51:34:124] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   07 00 05 01                                         ....
//[2023��02��09�� 22:51:34:157] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   08 00 05 01                                         ....
//[2023��02��09�� 22:51:34:194] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   09 00 05 01                                              ...
//[2023��02��09�� 22:51:34:228] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   05 00 05 01                                         ....
//[2023��02��09�� 22:51:34:262] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   06 00 05 01                                         ....
//[2023��02��09�� 22:51:34:315] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   07 00 05 01                                         ....
//[2023��02��09�� 22:51:34:354] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   08 00 05 01                                         ....
//[2023��02��09�� 22:51:34:390] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
//
//( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
//( 00000010 )   09 00 05 01                                              ...
    rv = subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);

    if (pkt->data.flags && pkt->data.object_id != 0xFFFF) {
        if ((l->flags & LOBBY_TYPE_CHEATS_ENABLED) && c->options.switch_assist &&
            (c->last_switch_enabled_command.type == 0x05)) {
            DBG_LOG("[��������] �ظ�������һ������");
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
            ERR_LOG("GC %" PRIu32 " ��������Ч��ʵ�� "
                "(%d -- ��ͼʵ������: %d)!\n"
                "�½�: %d, �㼶: %d, ��ͼ: (%d, %d)", c->guildcard,
                bid, l->map_objs->count, l->episode, c->cur_area,
                l->maps[c->cur_area << 1],
                l->maps[(c->cur_area << 1) + 1]);

            if ((l->flags & LOBBY_FLAG_QUESTING))
                ERR_LOG("���� ID: %d, �汾: %d", l->qid,
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
        ERR_LOG("GC %" PRIu32 " �ڴ���������ʵ��!",
            c->guildcard);
        return -1;
    }

    /* ������Ҫ��ͼ��ս������սģʽ�д�����Щ����. */
    if (l->challenge || l->battle)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    if (pkt->hdr.pkt_len == LE16(0x0010)) {
        /* �Կ������й��� */
        return 0;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt_size != (sizeof(bb_pkt_hdr_t) + (size << 2)) || size < 0x02) {
        ERR_LOG("GC %" PRIu32 " �����𻵵���ͨ��������! %d %d hit_count %d",
            c->guildcard, pkt_size, (sizeof(bb_pkt_hdr_t) + (size << 2)), hit_count);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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

int check_aoe_timer(ship_client_t* c, subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t tech_level = 0;

    /* �����Լ��... Does the character have that level of technique? */
    tech_level = c->pl->bb.character.techniques[pkt->technique_number];
    if (tech_level == 0xFF) {
        /* ����û����Ŷ���ѧϰһ���¼���������ܻᷢ�����������
        �������������Ŀ�����֮ǰ�����ǽ����ò��۸���һ�㡣
        һ������������ʵ�ġ������Ŀ����٣�����������ܻ�Ͽ��ͻ��˵����� */
        tech_level = pkt->level;
    }

    if (c->version >= CLIENT_VERSION_DCV2)
        tech_level += c->pl->bb.inv.iitems[pkt->technique_number].tech;

    if (tech_level < pkt->level) {
        /* ����û����Ŷ���ѧϰһ���¼���������ܻᷢ�����������
        �������������Ŀ�����֮ǰ�����ǽ����ò��۸���һ�㡣
        һ������������ʵ�ġ������Ŀ����٣�����������ܻ�Ͽ��ͻ��˵����� */
        tech_level = pkt->level;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* See what technique was used... */
    switch (pkt->technique_number) {
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
        SYSTEMTIME aoetime;
        GetLocalTime(&aoetime);
        printf("%03u\n", aoetime.wMilliseconds);
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
    }

    return 0;
}

/* TODO */
static int handle_bb_objhit_tech(ship_client_t* c, subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����÷�������ʵ��!",
            c->guildcard);
        return -1;
    }

    /* ������Ҫ��ͼ��ս������սģʽ�д�����Щ����. */
    if (l->challenge || l->battle)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.client_id != c->client_id ||
        pkt->technique_number > TECHNIQUE_MEGID ||
        c->equip_flags & EQUIP_FLAGS_DROID
        ) {
        ERR_LOG("GC %" PRIu32 " ְҵ %s �����𻵵� %s ������������!",
            c->guildcard, pso_class[c->pl->bb.character.disp.dress_data.ch_class].cn_name, 
            max_tech_level[pkt->technique_number].tech_name);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    //printf("ch_class %d techniques %d = %d max_lvl %d\n", c->pl->bb.character.disp.dress_data.ch_class,
    //    c->pl->bb.character.techniques[pkt->technique_number],
    //    c->bb_pl->character.techniques[pkt->technique_number], 
    //    max_tech_level[pkt->technique_number].max_lvl[c->pl->bb.character.disp.dress_data.ch_class]);

    //if (c->version <= CLIENT_VERSION_BB) {
        check_aoe_timer(c, pkt);
    //}

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

/* TODO */
static int handle_bb_objhit(ship_client_t* c, subcmd_bb_bhit_pkt_t* pkt) {
    uint64_t now = get_ms_time();
    //SYSTEMTIME aoetime;
    //GetLocalTime(&aoetime);
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�������������!",
            c->guildcard);
        return -1;
    }

    /* ������Ҫ��ͼ��ս������սģʽ�д�����Щ����. */
    if (l->challenge || l->battle)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    //printf("wMilliseconds = %d\n", aoetime.wMilliseconds);

    //printf("aoe_timer = %d now = %d\n", (uint32_t)c->aoe_timer, (uint32_t)now);

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

static int handle_bb_condition(ship_client_t* c, subcmd_bb_add_or_remove_condition_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X!",
            c->guildcard, pkt->hdr.pkt_type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_symbol_chat(ship_client_t* c, subcmd_bb_symbol_chat_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT)
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));

    /* Don't send chats for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU))
        return 0;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 1);
}

int handle_bb_dragon_act(ship_client_t* c, subcmd_bb_dragon_act_t* pkt) {
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
        ERR_LOG("GC %" PRIu32 " �ڴ����л�ȡ����!",
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

/* �ӹ����ȡ�ľ��鱶��δ���е��� */
static int handle_bb_req_exp(ship_client_t* c, subcmd_bb_req_exp_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t mid;
    uint32_t bp, exp_amount;
    game_enemy_t* en;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����л�ȡ����!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �����𻵵ľ����ȡ���ݰ�!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Make sure the enemy is in range. */
    mid = LE16(pkt->enemy_id2);
    //DBG_LOG("������ԭֵ %02X %02X", mid, pkt->enemy_id2);
    mid &= 0xFFF;
    //DBG_LOG("��������ֵ %02X map_enemies->count %02X", mid, l->map_enemies->count);

    if (mid > l->map_enemies->count) {
        ERR_LOG("GC %" PRIu32 " ɱ������Ч�ĵ��� (%d -- "
            "��������: %d)!", c->guildcard, mid, l->map_enemies->count);
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

    //DBG_LOG("����ԭֵ %d bp %d", exp_amount, bp);

    if ((c->game_data->expboost) && (l->exp_mult > 0))
        exp_amount = l->bb_params[bp].exp * l->exp_mult;

    if (!exp_amount)
        ERR_LOG("δ��ȡ��������ֵ %d bp %d ���� %d", exp_amount, bp, l->exp_mult);

    // TODO �������乲���� �ֱ�������3����ҷ�����ֵ���ȵľ���ֵ
    if (!pkt->last_hitter) {
        exp_amount = (exp_amount * 80) / 100;
    }

    return client_give_exp(c, exp_amount);
}

static int handle_bb_gallon_area(ship_client_t* c, subcmd_bb_gallon_area_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t quest_offset = pkt->quest_offset;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }
//[2023��02��09�� 21:50:51:617] ����(subcmd-bb.c 4858): Unknown 0x60: 0xD2
//
//( 00000000 )   14 00 60 00 00 00 00 00  D2 03 FF FF 05 00 00 00    ..`.............
//( 00000010 )   F3 AD 00 00                                         ....
//[2023��02��09�� 21:50:51:653] ����(subcmd-bb.c 4858): Unknown 0x60: 0xD2
//
//( 00000000 )   14 00 60 00 00 00 00 00  D2 03 FF FF 05 00 00 00    ..`.............
//( 00000010 )   9A D7 00 00                                         ....
    if (quest_offset < 23) {
        quest_offset *= 4;
        *(uint32_t*)&c->bb_pl->quest_data2[quest_offset] = *(uint32_t*)&pkt->data[0];
    }

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
        ERR_LOG("GC %" PRIu32 " �ڴ��������˹���!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �����𻵵Ĺ��﹥������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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

    if (enemy_id2 > l->map_enemies->count) {
        ERR_LOG("GC %" PRIu32 " ��������Ч�Ĺ��� (%d -- ��ͼ��������: "
            "%d)!", c->guildcard, enemy_id2, l->map_enemies->count);
        return -1;
    }

    /* Save the hit, assuming the enemy isn't already dead. */
    en = &l->map_enemies->enemies[enemy_id2];
    if (!(en->clients_hit & 0x80)) {
        en->clients_hit |= (1 << c->client_id);
        en->last_client = c->client_id;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_feed_mag(ship_client_t* c, subcmd_bb_feed_mag_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t item_id = pkt->item_id, mag_id = pkt->mag_id;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ���������Ϸָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " ���ʹ�������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    DBG_LOG("GC %" PRIu32 " ʹ����ƷID 0x%04X ι����� ID 0x%04X!",
        c->guildcard, item_id, mag_id);

    if (mag_bb_feed(c, item_id, mag_id)) {
        ERR_LOG("GC %" PRIu32 " ι����ŷ�������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_equip(ship_client_t* c, subcmd_bb_equip_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t /*inv, */item_id = pkt->item_id, found_item = 0/*, found_slot = 0, j = 0, slot[4] = { 0 }*/;
    int i = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�������װ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " ���ʹ���װ������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    found_item = item_check_equip_flags(c, item_id);

    /* �Ƿ������Ʒ������? */
    if (!found_item) {
        ERR_LOG("GC %" PRIu32 " װ����δ���ڵ���Ʒ����!",
            c->guildcard);
        return -1;
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
        ERR_LOG("GC %" PRIu32 " �ڴ���ж��װ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " ���ʹ���ж��װ������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Find the item and remove the equip flag. */
    inv = c->bb_pl->inv.item_count;

    i = item_get_inv_item_slot(c->bb_pl->inv, pkt->item_id);

    if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
        c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);

        /* If its a frame, we have to make sure to unequip any units that
           may be equipped as well. */
        if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
            c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_FRAME) {
            isframe = 1;
        }
    }

    /* Did we find something to equip? */
    if (i >= inv) {
        ERR_LOG("GC %" PRIu32 " ж����δ���ڵ���Ʒ����!",
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
        ERR_LOG("GC %" PRIu32 " �ڴ���ʹ����Ʒ!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x02)
        return -1;

    if (!(c->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* ����ҵı������Ƴ�����Ʒ. */
    if ((num = item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
        pkt->item_id, 1)) < 1) {
        ERR_LOG("�޷�����ұ������Ƴ���Ʒ!");
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

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->size != 0x01 ||
        pkt->data[0] != c->client_id) {
        ERR_LOG("GC %" PRIu32 " sent bad medic message!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Subtract 10 meseta from the client. */
    c->bb_pl->character.disp.meseta -= 10;
    c->pl->bb.character.disp.meseta -= 10;

    /* Send it along to the rest of the lobby. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_subcmd_6a(ship_client_t* c, subcmd_bb_Unknown_6x6A_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_3a(ship_client_t* c, subcmd_bb_cmd_3a_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_sort_inv(ship_client_t* c, subcmd_bb_sort_inv_t* pkt) {
    lobby_t* l = c->cur_lobby;
    inventory_t sorted = {0};
    size_t x = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ʒ!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0084) || pkt->shdr.size != 0x1F) {
        ERR_LOG("GC %" PRIu32 " ���ʹ����������Ʒ���ݰ�!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    for (x = 0; x < MAX_PLAYER_INV_ITEMS; x++) {
        if (pkt->item_ids[x] == 0xFFFFFFFF) {
            sorted.iitems[x].data.item_id = 0xFFFFFFFF;
        }
        else {
            size_t index = item_get_inv_item_slot(c->bb_pl->inv, pkt->item_ids[x]);
            sorted.iitems[x] = c->bb_pl->inv.iitems[index];
        }
    }

    sorted.item_count = c->bb_pl->inv.item_count;
    sorted.hpmats_used = c->bb_pl->inv.hpmats_used;
    sorted.tpmats_used = c->bb_pl->inv.tpmats_used;
    sorted.language = c->bb_pl->inv.language;

    ///* Make sure we got everything... */
    //if (j != sorted.item_count) {
    //    ERR_LOG("GC %" PRIu32 " ������Ʒ����������! %d != %d", c->guildcard, j, sorted.item_count);
    //    return -1;
    //}

    c->bb_pl->inv = sorted;
    c->pl->bb.inv = sorted;

    /* Nobody else really needs to care about this one... */
    return 0;
}

static int handle_bb_create_pipe(ship_client_t* c, subcmd_bb_pipe_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �����ڴ���ʹ�ô�����!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0024) || pkt->shdr.size != 0x07) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X!",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* See if the user is creating a pipe or destroying it. Destroying a pipe
       always matches the created pipe, but sets the area to 0. We could keep
       track of all of the pipe data, but that's probably overkill. For now,
       blindly accept any pipes where the area is 0
       �鿴�û��Ǵ����ܵ��������ٹܵ������ٹܵ������봴���Ĺܵ�ƥ�䣬
       ���Ὣ��������Ϊ0�����ǿ��Ը������йܵ����ݣ�
       �������̫�����ˡ�Ŀǰ��äĿ�������Ϊ0���κιܵ���. */
    if (pkt->area != 0) {
        /* Make sure the user is sending a pipe from the area he or she is
           currently in. */
        if (pkt->area != c->cur_area) {
            ERR_LOG("GC %" PRIu32 " ���ԴӴ����Ŵ����������ڵ����� (��Ҳ㼶: %d, ���Ͳ㼶: %d).", c->guildcard,
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
        ERR_LOG("GC %" PRIu32 " �ڴ���������NPC!",
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

static int handle_bb_set_flag(ship_client_t* c, subcmd_bb_set_flag_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t flag = pkt->flag;
    uint16_t checked = pkt->checked;
    uint16_t difficulty = pkt->difficulty;
    int rv = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (!l || l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " �ڴ�������SET_FLAGָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �ڴ�������SET_FLAGָ��!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    DBG_LOG("GC %" PRIu32 " ����SET_FLAGָ��! flag = 0x%02X checked = 0x%02X episode = 0x%02X difficulty = 0x%02X",
        c->guildcard, flag, checked, l->episode, difficulty);

    if (!checked) {
        if (flag < 0x400)
            c->bb_pl->quest_data1[((uint32_t)l->difficulty * 0x80) + (flag >> 3)] |= 1 << (7 - (flag & 0x07));
    }

    bool should_send_boss_drop_req = false;
    if (difficulty == l->difficulty) {
        if ((l->episode == GAME_TYPE_EPISODE_1) && (c->cur_area == 0x0E)) {
            // ����������£��ڰ���û�е����׶Σ������ڵڶ��׶ν������͵�������
            // ��������ģʽ���ڵ����׶ν�������
            if (((l->difficulty == 0) && (flag == 0x00000035)) ||
                ((l->difficulty != 0) && (flag == 0x00000037))) {
                should_send_boss_drop_req = true;
            }
        }
        else if ((l->episode == GAME_TYPE_EPISODE_2) && (flag == 0x00000057) && (c->cur_area == 0x0D)) {
            should_send_boss_drop_req = true;
        }
    }

    if (should_send_boss_drop_req) {
        ship_client_t* s = l->clients[l->leader_id];
        if (s) {
            subcmd_bb_itemreq_t bd = { 0 };

            /* �������ͷ */
            bd.hdr.pkt_len = LE16(sizeof(subcmd_bb_itemreq_t));
            bd.hdr.pkt_type = GAME_COMMAND2_TYPE;
            bd.hdr.flags = 0x00000000;
            bd.shdr.type = SUBCMD62_ITEMREQ;
            bd.shdr.size = 0x06;
            bd.shdr.unused = 0x0000;

            /* ������� */
            bd.area = (uint8_t)c->cur_area;
            bd.pt_index = (uint8_t)((l->episode == GAME_TYPE_EPISODE_2) ? 0x4E : 0x2F);
            bd.request_id = LE16(0x0B4F);
            bd.x = (l->episode == 2) ? -9999.0f : 10160.58984375f;
            bd.z = 0;
            bd.unk1 = LE16(0x0002);
            bd.unk2 = LE16(0x0000);
            bd.unk3 = LE32(0xE0AEDC01);

            return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
        }
    }

    rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    return rv;
}

static int handle_bb_killed_monster(ship_client_t* c, subcmd_bb_killed_monster_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����ɱ���˹���!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " �������𻵵�ɱ������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static inline int reg_sync_index_bb(lobby_t* l, uint16_t regnum) {
    int i;

    if (!(l->q_flags & LOBBY_QFLAG_SYNC_REGS))
        return -1;

    for (i = 0; i < l->num_syncregs; ++i) {
        if (regnum == l->syncregs[i])
            return i;
    }

    return -1;
}

static int handle_bb_sync_reg(ship_client_t* c, subcmd_bb_sync_reg_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t val = LE32(pkt->value);
    int done = 0, idx;
    uint32_t ctl;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �������𻵵�ͬ������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* TODO: Probably should do some checking here... */
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
    if ((idx = reg_sync_index_bb(l, pkt->register_number)) != -1) {
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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��! 0x%02X",
            c->guildcard, pkt->shdr.type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �������𻵵���������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_used_tech(ship_client_t* c, subcmd_bb_used_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ������ͷ��˷���!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �ͷ���Υ��ķ���!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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

static int handle_bb_arrow_change(ship_client_t* c, subcmd_bb_arrow_change_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д����˷���ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����˴�������ݰ�!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    c->arrow_color = pkt->shdr.client_id;
    return send_lobby_arrows(l);
}

static int handle_bb_Unknown_6x8A(ship_client_t* c, subcmd_bb_Unknown_6x8A_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д����˷���ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����˴�������ݰ�!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ������ͷ��˷���!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �ͷ���Υ��ķ���!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ����д����˷���ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " �����˴�������ݰ�!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
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
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    c->game_data->death = 1;

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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��! 0x%02X",
            c->guildcard, pkt->shdr.type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_switch_req(ship_client_t* c, subcmd_bb_switch_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ������ͷ��˷���!",
            c->guildcard);
        return -1;
    }
    //(00000000)   10 00 60 00 00 00 00 00  50 02 00 00 A3 A6 00 00    ..`.....P.......
    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_defense_damage(ship_client_t* c, subcmd_bb_defense_damage_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ������ͷ��˷���!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* This aught to do it... */
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
        //print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Clear the list of dropped items. */
    if (c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    /* Flip the shopping flag, since this packet is sent both for talking to the
       shop in the first place and when the client exits the shop
       ��ת�����־����Ϊ�����ݰ��ڵ�һʱ��Ϳͻ��뿪�̵�ʱ���ᷢ�͸��̵�. */
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

static int handle_bb_set_area_1F(ship_client_t* c, subcmd_bb_set_area_1F_t* pkt) {
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

        // ����DC����
        /* Clear the list of dropped items. */
        if (c->cur_area == 0) {
            memset(c->p2_drops, 0, sizeof(c->p2_drops));
            c->p2_drops_max = 0;
        }

        c->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_set_area_20(ship_client_t* c, subcmd_bb_set_area_20_t* pkt) {
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

        // ����DC����
        /* Clear the list of dropped items. */
        if (c->cur_area == 0) {
            memset(c->p2_drops, 0, sizeof(c->p2_drops));
            c->p2_drops_max = 0;
        }

        c->cur_area = pkt->area;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        //�������� �ͻ���֮�����λ��
        //DBG_LOG("�ͻ������� %d ��x:%f y:%f z:%f)", c->cur_area, c->x, c->y, c->z);
        //�ͻ������� 0 ��x:228.995331 y:0.000000 z:253.998901)

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
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
#ifdef DEBUG
        DBG_LOG("GC %u %d %d (0x%X 0x%X) X��:%f Y��:%f Z��:%f", c->guildcard,
            c->client_id, pkt->shdr.client_id, pkt->unk1, pkt->unused, pkt->x, pkt->y, pkt->z);
#endif // DEBUG
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_normal_attack(ship_client_t* c, subcmd_bb_natk_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ���������ͨ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " �ڴ�������SET_FLAGָ��!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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

        if (c->game_data->death) {
            c->game_data->death = 0;
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_take_item(ship_client_t* c, subcmd_bb_take_item_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    // BB should never send this command - inventory items should only be
    // created by the server in response to shop buy / bank withdraw / etc. reqs

    UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);

    /* ���ݰ����, ��������Ϸ����. */
    return 0;
}

static int handle_bb_drop_item(ship_client_t* c, subcmd_bb_drop_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1, isframe = 0;
    uint32_t i, inv;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �����ڴ����е�����Ʒ!",
            c->guildcard);
        return -1;
    }

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if ((c->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("GC %" PRIu32 " �������̵��е�����Ʒ!",
            c->guildcard);
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵���Ʒ��������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* TODO �����սģʽ����Ʒ���� */
    if (l->challenge || l->battle) {
        /* ���ݰ����, ��������Ϸ����. */
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Look for the item in the user's inventory. */
    inv = c->bb_pl->inv.item_count;

    for (i = 0; i < inv; ++i) {
        if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
            found = i;

            /* If it is an equipped frame, we need to unequip all the units
               that are attached to it.
               �������һ��װ���õĻ��ף�������Ҫȡ���������������е�Ԫ��װ��*/
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
        ERR_LOG("GC %" PRIu32 " ��������Ч����Ʒ ID!",
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
    �����������Ʒ��������ӵ������ı�����*/
    if (!lobby_add_item_locked(l, &c->bb_pl->inv.iitems[found])) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("Couldn't add item to lobby inventory!\n");
        return -1;
    }

    /* ����ҵı������Ƴ�����Ʒ. */
    if (item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
        pkt->item_id, 0xFFFFFFFF) < 1) {
        ERR_LOG("�޷�����ұ������Ƴ���Ʒ!");
        return -1;
    }

    --c->bb_pl->inv.item_count;
    c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

    /* ���ݰ����, ��������Ϸ����. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_drop_pos(ship_client_t* c, subcmd_bb_drop_pos_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1;
    uint32_t i, meseta, amt;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ʒ!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
            ERR_LOG("GC %" PRIu32 " ��������Ч����ƷID!",
                c->guildcard);
            return -1;
        }
    }
    else {
        meseta = LE32(c->bb_pl->character.disp.meseta);
        amt = LE32(pkt->amount);

        if (meseta < amt) {
            ERR_LOG("GC %" PRIu32 " ������������������ӵ�е�!",
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

    /* ���ݰ����, ��������Ϸ����. */
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
        ERR_LOG("GC %" PRIu32 " �ڴ����е�����Ʒ!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �������𻵵���Ʒ��������!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* TODO �����սģʽ����Ʒ���� */
    if (l->challenge || l->battle) {
        /* ���ݰ����, ��������Ϸ����. */
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    if (pkt->item_id != 0xFFFFFFFF) {
        /* �����û�����е���Ʒ. */
        for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
            if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
                found = i;
                break;
            }
        }

        /* ����Ҳ�������Ʒ�����û��Ӵ�������. */
        if (found == -1) {
            ERR_LOG("GC %" PRIu32 " ������Ч�Ķѵ���Ʒ!", c->guildcard);
            return -1;
        }

        /* �ӿͻ��˽ṹ�Ŀ���л�ȡ��Ʒ�����ò�� */
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
        ERR_LOG("GC %" PRIu32 " ���˲�ͬ�Ķѵ���Ʒ!",
            c->guildcard);
        ERR_LOG("pkt->item_id %d c->drop_item %d pkt->amount %d c->drop_amt %d", pkt->item_id, c->drop_item, pkt->amount, c->drop_amt);
        return -1;
    }

    /* We have the item... Add it to the lobby's inventory. */
    if (!(it = lobby_add_item_locked(l, &item_data))) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("�޷�����Ʒ�������Ϸ����!");
        return -1;
    }

    if (pkt->item_id != 0xFFFFFFFF) {
        /* ����ҵı������Ƴ�����Ʒ. */
        found = item_remove_from_inv(c->bb_pl->inv.iitems,
            c->bb_pl->inv.item_count, pkt->item_id,
            LE32(pkt->amount));
        if (found < 0) {
            ERR_LOG("�޷�����ұ������Ƴ���Ʒ!");
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
            ERR_LOG("GC %" PRIu32 " �����������������ӵ�е�",
                c->guildcard);
            return -1;
        }

        c->bb_pl->character.disp.meseta = LE32(tmp2 - tmp);
        c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
    }

    /* �����������������ݰ�Ҫ����.����,����һ�����ݰ�,����ÿ������һ����Ʒ����.
    Ȼ��,����һ���ӿͻ��˵Ŀ����ɾ����Ʒ����.��һ�����뷢��ÿ����,
    �ڶ������뷢�����������������������������������. */
    subcmd_send_bb_drop_stack(c, c->drop_area, c->drop_x, c->drop_z, it);

    /* ���ݰ����, ��������Ϸ����. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_hit_by_enemy(ship_client_t* c, subcmd_bb_hit_by_enemy_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t type = LE16(pkt->hdr.pkt_type);

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ���������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    UDONE_CSPD(type, c->version, pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_talk_npc(ship_client_t* c, subcmd_bb_talk_npc_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    //if (l->type == LOBBY_TYPE_LOBBY) {
    //    ERR_LOG("GC %" PRIu32 " �ڴ�������Ϸ�����е�NPC��̸!",
    //        c->guildcard);
    //    return -1;
    //}

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x05) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, pkt->hdr.pkt_len);
        return -1;
    }

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
    //if (l->type == LOBBY_TYPE_LOBBY) {
    //    ERR_LOG("GC %" PRIu32 " �ڴ�������Ϸ�����е�NPC��ɽ�̸!",
    //        c->guildcard);
    //    return -1;
    //}

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x01) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, pkt->hdr.pkt_len);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_level_up(ship_client_t* c, subcmd_bb_level_up_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ�����ָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x05 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
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

//����
static int handle_bb_chair_dir(ship_client_t* c, subcmd_bb_create_lobby_chair_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " ����Ϸ�д����˴�������ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ�����ָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x08) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ�����ָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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

                /* ����ҵı������Ƴ�����Ʒ. */
                if (item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
                    pkt->item_id, 0xFFFFFFFF) < 1) {
                    ERR_LOG("�޷�����ұ������Ƴ���Ʒ!");
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
        ERR_LOG("GC %" PRIu32 " �ڴ����д����˷���ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " �����˴�������ݰ�!",
            c->guildcard);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
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
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ�����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0028) || pkt->shdr.size != 0x08 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    if (c->client_id == pkt->shdr.client_id) {

        //DBG_LOG("area = 0x%04X", pkt->area);

        switch (pkt->area)
        {
            /* �ܶ��� ʵ���� */
        case 0x8000:
            //l->govorlab = 1;
            DBG_LOG("�����ܶ�������ʶ��");
            break;

            /* EP1�ɴ� */
        case 0x0000:

            /* EP2�ɴ� */
        case 0xC000:

        default:
            //l->govorlab = 0;
            DBG_LOG("�뿪�ܶ�������ʶ��");
            break;
        }

    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_level_up_req(ship_client_t* c, subcmd_bb_levelup_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ�����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014)) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_lobby_act(ship_client_t* c, subcmd_bb_lobby_act_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    DBG_LOG("����ID %d", pkt->act_id);

    // pkt->act_id ���������Ķ�ӦID

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_gogo_ball(ship_client_t* c, subcmd_bb_gogo_ball_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    // pkt->act_id ���������Ķ�ӦID

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_steal_exp(ship_client_t* c, subcmd_bb_steal_exp_t* pkt) {
    lobby_t* l = c->cur_lobby;
    pmt_weapon_bb_t tmp_wp = { 0 };
    game_enemy_t* en;
    subcmd_bb_steal_exp_t* pk = (subcmd_bb_steal_exp_t*)pkt;
    uint32_t exp_percent = 0;
    uint32_t exp_to_add;
    uint8_t special = 0;
    uint16_t mid;
    uint32_t bp, exp_amount;
    int i;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �����ڴ���������Ϸָ��!",
            c->guildcard);
        return -1;
    }

    mid = LE16(pk->shdr.enemy_id);
    mid &= 0xFFF;

    if (mid < 0xB50) {
        for (i = 0; i < c->bb_pl->inv.item_count; i++) {
            if ((c->bb_pl->inv.iitems[i].flags & 0x08) &&
                (c->bb_pl->inv.iitems[i].data.data_b[0] == 0x00)) {
                if ((c->bb_pl->inv.iitems[i].data.data_b[1] < 0x0A) &&
                    (c->bb_pl->inv.iitems[i].data.data_b[2] < 0x05)) {
                    special = (c->bb_pl->inv.iitems[i].data.data_b[4] & 0x1F);
                }
                else {
                    if ((c->bb_pl->inv.iitems[i].data.data_b[1] < 0x0D) &&
                        (c->bb_pl->inv.iitems[i].data.data_b[2] < 0x04))
                        special = (c->bb_pl->inv.iitems[i].data.data_b[4] & 0x1F);
                    else {
                        if (pmt_lookup_weapon_bb(c->bb_pl->inv.iitems[i].data.data_l[0], &tmp_wp)) {
                            ERR_LOG("GC %" PRIu32 " װ���˲����ڵ���Ʒ����!",
                                c->guildcard);
                            return -1;
                        }

                        special = tmp_wp.special_type;
                    }
                }

                switch (special) {
                case 0x09:
                    // Master's
                    exp_percent = 8;
                    break;

                case 0x0A:
                    // Lord's
                    exp_percent = 10;
                    break;

                case 0x0B:
                    // King's
                    exp_percent = 12;
                    if ((l->difficulty == 0x03) &&
                        (c->equip_flags & EQUIP_FLAGS_DROID))
                        exp_percent += 30;
                    break;
                }

                break;
            }
        }

        if (exp_percent) {
            /* Make sure the enemy is in range. */
            mid = LE16(pk->shdr.enemy_id);
            //DBG_LOG("������ԭֵ %02X %02X", mid, pkt->enemy_id2);
            mid &= 0xFFF;
            //DBG_LOG("��������ֵ %02X map_enemies->count %02X", mid, l->map_enemies->count);

            if (mid > l->map_enemies->count) {
                ERR_LOG("GC %" PRIu32 " ɱ������Ч�ĵ��� (%d -- "
                    "��������: %d)!", c->guildcard, mid, l->map_enemies->count);
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

            //�������鱶���������� exp_mult expboost 11.18
            exp_amount = (l->bb_params[bp].exp * exp_percent) / 100L;

            if (exp_amount > 80)  // Limit the amount of exp stolen to 80
                exp_amount = 80;

            if ((c->game_data->expboost) && (l->exp_mult > 0)) {
                exp_to_add = exp_amount;
                exp_amount = exp_to_add + (l->bb_params[bp].exp * l->exp_mult);
            }

            if (!exp_amount) {
                ERR_LOG("δ��ȡ��������ֵ %d bp %d ���� %d", exp_amount, bp, l->exp_mult);
                return client_give_exp(c, 1);
            }

            return client_give_exp(c, exp_amount);
        }
    }
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_load_3b(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �����ڴ���������Ϸָ��!",
            c->guildcard);
        return -1;
    }

    subcmd_send_bb_set_exp_rate(c, 3000);

    c->need_save_data = 1;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_load_22(ship_client_t* c, subcmd_bb_set_player_visibility_6x22_6x23_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int i, rv;

    if (pkt->shdr.client_id != c->client_id) {
        DBG_LOG("SUBCMD60_LOAD_22 0x60 ָ��: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    if (l->type == LOBBY_TYPE_LOBBY) {
        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != c &&
                subcmd_send_pos(c, l->clients[i])) {
                rv = -1;
                break;
            }
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_guild_ex_item(ship_client_t* c, subcmd_bb_guild_ex_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t ex_item_id = pkt->ex_item_id, ex_amount = pkt->ex_amount;
    int found = 0;

    //[2023��02��14�� 18:07:48:964] ����(subcmd-bb.c 5348): ��δ��� 0x60: 0xCC
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  CC 03 00 00 00 00 01 00    ..`.............
    //( 00000010 )   01 00 00 00                                         ....
    //[2023��02��14�� 18:07:56:864] ����(block-bb.c 2347): ���֣�BB ���Ṧ��ָ�� 0x18EA BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO - 0x18EA (����8)
    //( 00000000 )   08 00 EA 18 00 00 00 00                             ........
    //[2023��02��14�� 18:07:56:890] ����(ship_packets.c 11953): ��GC 42004063 ���͹���ָ�� 0x18EA
    //[2023��02��14�� 18:08:39:462] ����(subcmd-bb.c 5348): ��δ��� 0x60: 0xCC
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  CC 03 00 00 06 00 01 00    ..`.............
    //( 00000010 )   01 00 00 00          
//[2023��02��14�� 18:16:59:989] ����(subcmd-bb.c 5348): ��δ��� 0x60: 0xCC
//
//( 00000000 )   14 00 60 00 00 00 00 00  CC 03 01 00 00 00 81 00    ..`.............
//( 00000010 )   01 00 00 00                                         ....

    if (pkt->shdr.client_id != c->client_id || ex_amount == 0) {
        DBG_LOG("���� 0x60 ָ��: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* TODO �����սģʽ����Ʒ���� */
    if (l->challenge) {
        send_msg(c, MSG1_TYPE, "%s", __(c, "\t��սģʽ��֧��ת����Ʒ!"));
        /* ���ݰ����, ��������Ϸ����. */
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    if (ex_item_id == EMPTY_STRING) {
        DBG_LOG("���� 0x60 ָ��: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* ����ҵı������Ƴ�����Ʒ. */
    found = item_remove_from_inv(c->bb_pl->inv.iitems,
        c->bb_pl->inv.item_count, ex_item_id,
        LE32(ex_amount));

    if (found < 0) {
        ERR_LOG("�޷�����ұ������Ƴ���Ʒ!");
        return -1;
    }

    send_msg(c, MSG1_TYPE, "%s", __(c, "\t��Ʒת���ɹ�!"));

    c->bb_pl->inv.item_count -= found;
    //c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_player_saved(ship_client_t* c, subcmd_bb_player_saved_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->shdr.client_id != c->client_id) {
        DBG_LOG("���� 0x60 ָ��: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_save_player_act(ship_client_t* c, subcmd_bb_save_player_act_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->shdr.client_id != c->client_id) {
        DBG_LOG("���� 0x60 ָ��: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_battle_mode(ship_client_t* c, subcmd_bb_battle_mode_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int ch, ch2;
    ship_client_t* lc = { 0 };

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �����ڴ���������Ϸָ��!",
            c->guildcard);
        return -1;
    }

    if ((l->battle) && (c->mode)) {
        // Battle restarting...
        //
        // If rule #1 we'll copy the character backup to the character array, otherwise
        // we'll reset the character...
        //
        for (ch = 0; ch < l->max_clients; ch++) {
            if ((l->clients_slot[ch]) && (l->clients[ch])) {
                lc = l->clients[ch];
                switch (lc->mode) {
                case 0x01:
                case 0x02:
                    // Copy character backup
                    if (lc->game_data->char_backup && lc->guildcard)
                        memcpy(&lc->pl->bb.character, lc->game_data->char_backup, sizeof(lc->pl->bb.character));

                    if (lc->mode == 0x02) {
                        for (ch2 = 0; ch2 < lc->pl->bb.inv.item_count; ch2++)
                        {
                            if (lc->pl->bb.inv.iitems[ch2].data.data_b[0] == 0x02)
                                lc->pl->bb.inv.iitems[ch2].present = 0;
                        }
                        //CleanUpInventory(lc);
                        lc->pl->bb.character.disp.meseta = 0;
                    }
                    break;
                case 0x03:
                    // Wipe items and reset level.
                    for (ch2 = 0; ch2 < 30; ch2++)
                        lc->pl->bb.inv.iitems[ch2].present = 0;
                    //CleanUpInventory(lc);

                    uint8_t ch_class = lc->pl->bb.character.disp.dress_data.ch_class;

                    lc->pl->bb.character.disp.level = 0;
                    lc->pl->bb.character.disp.exp = 0;

                    lc->pl->bb.character.disp.stats.atp = bb_char_stats.start_stats[ch_class].atp;
                    lc->pl->bb.character.disp.stats.mst = bb_char_stats.start_stats[ch_class].mst;
                    lc->pl->bb.character.disp.stats.evp = bb_char_stats.start_stats[ch_class].evp;
                    lc->pl->bb.character.disp.stats.hp = bb_char_stats.start_stats[ch_class].hp;
                    lc->pl->bb.character.disp.stats.dfp = bb_char_stats.start_stats[ch_class].dfp;
                    lc->pl->bb.character.disp.stats.ata = bb_char_stats.start_stats[ch_class].ata;

                    /*if (l->battle_level > 1)
                        SkipToLevel(l->battle_level - 1, lc, 1);*/
                    lc->pl->bb.character.disp.meseta = 0;
                    break;
                default:
                    // Unknown mode?
                    break;
                }
            }
        }
        // Reset boxes and monsters...
        //memset(&l->boxHit, 0, 0xB50); // Reset box and monster data
        //memset(&l->monsterData, 0, sizeof(l->monsterData));
    }

    return 0;
}

static int handle_bb_lobby_chair(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " ����Ϸ�д����˴�������ָ��!",
            c->guildcard);
        return -1;
    }

    switch (type)
    {
        /* 0xAB */
    case SUBCMD60_CHAIR_CREATE:
        subcmd_bb_create_lobby_chair_t* pktab = (subcmd_bb_create_lobby_chair_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_CREATE %u %u ", pktab->unknown_a1, pktab->unknown_a2);

        break;

        /* 0xAE */
    case SUBCMD60_CHAIR_STATE:
        subcmd_bb_set_lobby_chair_state_t* pktae = (subcmd_bb_set_lobby_chair_state_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_STATE %u %u %u %u", pktae->unknown_a1, pktae->unknown_a2, pktae->unknown_a3, pktae->unknown_a4);

        break;

        /* 0xAF */
    case SUBCMD60_CHAIR_TURN:
        subcmd_bb_turn_lobby_chair_t* pktaf = (subcmd_bb_turn_lobby_chair_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_TURN %02X", pktaf->angle);
        break;

        /* 0xB0 */
    case SUBCMD60_CHAIR_MOVE:
        subcmd_bb_move_lobby_chair_t* pktb0 = (subcmd_bb_move_lobby_chair_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_MOVE %02X", pktb0->angle);
        break;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

/* ����BB 0x60 ���ݰ�. */
int subcmd_bb_handle_bcast(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0, sent = 1, i = 0;

    /* ����ͻ��˲��ڴ������߶�������������ݰ�. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    subcmd_bb_60size_check(c, pkt);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        switch (type) {
        case SUBCMD60_SET_POS_3F://����ԾǨʱ���� 1
            rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
            break;

        case SUBCMD60_SET_AREA_1F://����ԾǨʱ���� 2
            rv = handle_bb_set_area_1F(c, (subcmd_bb_set_area_1F_t*)pkt);
            break;

        case SUBCMD60_LOAD_3B://����ԾǨʱ���� 3
        //case SUBCMD60_BURST_DONE:
            /* TODO ����������ںϽ����亯���� */
            rv = handle_bb_load_3b(c, (subcmd_bb_pkt_t*)pkt);
            break;

        case SUBCMD60_CMODE_GRAVE:
            rv = handle_bb_cmode_grave(c, pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x60 ָ��: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {

        /* ���������� */
    case SUBCMD60_GOGO_BALL:
        rv = handle_bb_gogo_ball(c, (subcmd_bb_gogo_ball_t*)pkt);
        break;

        /* ��ͼ���ͱ仯���� */
    case SUBCMD60_WARP_55:
        rv = handle_bb_map_warp_55(c, (subcmd_bb_map_warp_t*)pkt);
        break;

        /*����״̬ ����*/
    case SUBCMD60_CHAIR_CREATE:
    case SUBCMD60_CHAIR_STATE:
    case SUBCMD60_CHAIR_TURN:
    case SUBCMD60_CHAIR_MOVE:
        rv = handle_bb_lobby_chair(c, (subcmd_bb_pkt_t*)pkt);
        break;

        /*��սģʽ ����*/
    case SUBCMD60_UNKNOW_7D:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        //rv = handle_bb_cmd_check_game_size(c, pkt, 0x06);
        break;

        /*��սģʽ ����*/
    case SUBCMD60_UNKNOW_CH_8A:
        rv = handle_bb_Unknown_6x8A(c, (subcmd_bb_Unknown_6x8A_t*)pkt);
        break;

    case SUBCMD60_BATTLEMODE:
        rv = handle_bb_battle_mode(c, (subcmd_bb_battle_mode_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SAVE_PLAYER_ACT:
        rv = handle_bb_save_player_act(c, (subcmd_bb_save_player_act_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_PLAYER_SAVED:
        rv = handle_bb_player_saved(c, (subcmd_bb_player_saved_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_DEATH_SYNC:
        rv = handle_bb_death_sync(c, (subcmd_bb_death_sync_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_UNKNOW_4E:
        rv = handle_bb_cmd_4e(c, (subcmd_bb_cmd_4e_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_PLAYER_DIED:
        rv = handle_bb_player_died(c, (subcmd_bb_player_died_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SELL_ITEM:
        rv = handle_bb_sell_item(c, (subcmd_bb_sell_item_t*)pkt);
        break;

    case SUBCMD60_EX_ITEM_TEAM:
        rv = handle_bb_guild_ex_item(c, (subcmd_bb_guild_ex_item_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_USE_ITEM:
        rv = handle_bb_use_item(c, (subcmd_bb_use_item_t*)pkt);
        break;

        /* δ������ ����T�� 2023.2.10 */
    case SUBCMD60_STEAL_EXP:
        rv = handle_bb_steal_exp(c, (subcmd_bb_steal_exp_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_LOBBY_ACTION:
        rv = handle_bb_lobby_act(c, (subcmd_bb_lobby_act_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_ARROW_CHANGE:
        rv = handle_bb_arrow_change(c, (subcmd_bb_arrow_change_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_DRAGON_ACT:// Dragon actions
        rv = handle_bb_dragon_act(c, (subcmd_bb_dragon_act_t*)pkt);
        break;

    case SUBCMD60_LOAD_3B:
        rv = handle_bb_load_3b(c, (subcmd_bb_pkt_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_LOAD_22://subcmd_set_player_visibility_6x22_6x23_t
        rv = handle_bb_load_22(c, (subcmd_bb_set_player_visibility_6x22_6x23_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_TALK_NPC:
        rv = handle_bb_talk_npc(c, (subcmd_bb_talk_npc_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_DONE_NPC:
        rv = handle_bb_done_talk_npc(c, (subcmd_bb_end_talk_to_npc_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_MENU_REQ:
        rv = handle_bb_menu_req(c, (subcmd_bb_menu_req_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_LEVEL_UP_REQ:
        rv = handle_bb_level_up_req(c, (subcmd_bb_levelup_req_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_REQ_SWITCH:
        rv = handle_bb_switch_req(c, (subcmd_bb_switch_req_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SET_FLAG:
        rv = handle_bb_set_flag(c, (subcmd_bb_set_flag_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_CREATE_PIPE:
        rv = handle_bb_create_pipe(c, (subcmd_bb_pipe_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_UNKNOW_6A:
        rv = handle_bb_subcmd_6a(c, (subcmd_bb_Unknown_6x6A_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_INTER_LEVEL_WARP:
        rv = handle_bb_inter_level_warp(c, (subcmd_bb_inter_level_warp_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_UNKNOW_3A:
        rv = handle_bb_cmd_3a(c, (subcmd_bb_cmd_3a_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_CONDITION_ADD:
        rv = handle_bb_condition(c, (subcmd_bb_add_or_remove_condition_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_CONDITION_REMOVE:
        rv = handle_bb_condition(c, (subcmd_bb_add_or_remove_condition_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SYNC_REG:
        rv = handle_bb_sync_reg(c, (subcmd_bb_sync_reg_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_OBJHIT_PHYS:
        rv = handle_bb_objhit_phys(c, (subcmd_bb_objhit_phys_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SWITCH_CHANGED:
        rv = handle_bb_switch_changed(c, (subcmd_bb_switch_changed_pkt_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_ATTACK1:
    case SUBCMD60_ATTACK2:
    case SUBCMD60_ATTACK3:
        rv = handle_bb_normal_attack(c, (subcmd_bb_natk_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_DEFENSE_DAMAGE:
        rv = handle_bb_defense_damage(c, (subcmd_bb_defense_damage_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_KILL_MONSTER:
        rv = handle_bb_killed_monster(c, (subcmd_bb_killed_monster_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_HIT_OBJ:
        rv = handle_bb_objhit(c, (subcmd_bb_bhit_pkt_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_CREATE_ENEMY_SET:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x04);
        break;

        /* �˺����������� */
    case SUBCMD60_SET_POS_24:
        rv = handle_bb_set_pos_0x24(c, (subcmd_bb_set_pos_0x24_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_GALLON_AREA:
        rv = handle_bb_gallon_area(c, (subcmd_bb_gallon_area_pkt_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SET_AREA_20:
        rv = handle_bb_set_area_20(c, (subcmd_bb_set_area_20_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SET_TECH_LEVEL_OVERRIDE:
        rv = handle_bb_set_technique_level_override(c, (subcmd_bb_set_technique_level_override_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_OBJHIT_TECH:
        rv = handle_bb_objhit_tech(c, (subcmd_bb_objhit_tech_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SYMBOL_CHAT:
        rv = handle_bb_symbol_chat(c, (subcmd_bb_symbol_chat_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_HIT_MONSTER:
        rv = handle_bb_mhit(c, (subcmd_bb_mhit_pkt_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SET_AREA_1F:
        rv = handle_bb_set_area_1F(c, (subcmd_bb_set_area_1F_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_EQUIP:
        rv = handle_bb_equip(c, (subcmd_bb_equip_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_REMOVE_EQUIP:
        rv = handle_bb_unequip(c, (subcmd_bb_equip_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_DROP_ITEM:
        rv = handle_bb_drop_item(c, (subcmd_bb_drop_item_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SET_POS_3E:
    case SUBCMD60_SET_POS_3F:
        rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_MOVE_SLOW:
    case SUBCMD60_MOVE_FAST:
        rv = handle_bb_move(c, (subcmd_bb_move_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_DELETE_ITEM:
        rv = handle_bb_destroy_item(c, (subcmd_bb_destroy_item_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_WORD_SELECT:
        rv = handle_bb_word_select(c, (subcmd_bb_word_select_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_DROP_POS:
        rv = handle_bb_drop_pos(c, (subcmd_bb_drop_pos_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SORT_INV:
        rv = handle_bb_sort_inv(c, (subcmd_bb_sort_inv_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_MEDIC:
        rv = handle_bb_medic(c, (subcmd_bb_pkt_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_REQ_EXP:
        rv = handle_bb_req_exp(c, (subcmd_bb_req_exp_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_USED_TECH:
        rv = handle_bb_used_tech(c, (subcmd_bb_used_tech_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_TAKE_DAMAGE1:
    case SUBCMD60_TAKE_DAMAGE2:
        rv = handle_bb_take_damage(c, (subcmd_bb_take_damage_t*)pkt);
        break;

        /* �˺����������� */
    case SUBCMD60_SPAWN_NPC:
        rv = handle_bb_spawn_npc(c, pkt);
        break;
        
    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        //DBG_LOG("��δ��� 0x60: 0x%02X\n", type);
        //print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
        sent = 0;
        break;

    case SUBCMD60_FINISH_LOAD:

        if (l->type != LOBBY_TYPE_GAME) {
            for (i = 0; i < l->max_clients; ++i) {
                if (l->clients[i] && l->clients[i] != c &&
                    subcmd_send_pos(c, l->clients[i])) {
                    rv = -1;
                    break;
                }
            }
        }

        rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
        break;
    }

    /* Broadcast anything we don't care to check anything about. */
    if (!sent)
        rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}