/*
    �λ�֮���й� ���������� 62ָ���
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
#include "subcmd_send_bb.h"
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

#include "subcmd_handle.h"

int sub62_06_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_gcsend_t* pkt) {
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

        /* ������ݲ�׼������.. */
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

int sub62_5A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pick_up_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int pick_count;
    uint32_t item, tmp;
    iitem_t iitem_data = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����ʰȡ����Ʒ!",
            src->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x14) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " ���ʹ����ʰȡ����!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* ���ԴӴ�����Ʒ�����Ƴ�... */
    pick_count = lobby_remove_item_locked(l, pkt->item_id, &iitem_data);
    if (pick_count < 0) {
        return -1;
    }
    else if (pick_count > 0) {
        /* ��������Ѿ�����, �ͺ�����... */
        return 0;
    }
    else {
        item = LE32(iitem_data.data.data_l[0]);

        /* Is it meseta, or an item? */
        if (item == Item_Meseta) {
            tmp = LE32(iitem_data.data.data2_l) + LE32(src->bb_pl->character.disp.meseta);

            /* Cap at 999,999 meseta. */
            if (tmp > 999999)
                tmp = 999999;

            src->bb_pl->character.disp.meseta = LE32(tmp);
            src->pl->bb.character.disp.meseta = src->bb_pl->character.disp.meseta;
        }
        else {
            iitem_data.present = LE16(0x0001);
            iitem_data.tech = 0;
            iitem_data.flags = 0;


            /* Add the item to the client's inventory. */
            pick_count = add_item_to_client(src, &iitem_data);

            if (pick_count == -1)
                return 0;

            //src->bb_pl->inv.item_count += pick_count;
            //src->pl->bb.inv.item_count = src->bb_pl->inv.item_count;
        }
    }

    /* �������˶�֪���Ǹÿͻ��˼񵽵ģ��������������������ɾ��. */
    subcmd_send_lobby_bb_create_inv_item(src, iitem_data.data, 1);

    return subcmd_send_bb_pick_item(src, pkt->area, iitem_data.data.item_id);
}

int sub62_6F_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {

    send_bb_quest_data1(dest, &src->bb_pl->quest_data1);

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_71_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_A6_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_trade_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = -1;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            src->guildcard);
        return rv;
    }

    if (pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! ���ݴ�С %02X",
            src->guildcard, pkt->shdr.type, pkt->shdr.size);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return rv;
    }

    send_msg(dest, BB_SCROLL_MSG_TYPE, "%s", __(dest, "\tE\tC6���׹��ܴ��ڹ��ϣ���ʱ�ر�"));
    send_msg(src, BB_SCROLL_MSG_TYPE, "%s", __(src, "\tE\tC6���׹��ܴ��ڹ��ϣ���ʱ�ر�"));

    DBG_LOG("GC %u -> %u", src->guildcard, dest->guildcard);

    display_packet(pkt, pkt->hdr.pkt_len);

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_AE_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_send_lobby_chair_state_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = -1;

    if (pkt->shdr.size != 0x04 || pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! ���ݴ�С %02X",
            src->guildcard, pkt->shdr.type, pkt->shdr.size);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return rv;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_B5_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_shop_req_t* req) {
    lobby_t* l = src->cur_lobby;
    block_t* b = src->cur_block;
    item_t item_data;
    uint32_t shop_type = LE32(req->shop_type);
    uint8_t num_items = 9 + (mt19937_genrand_int32(&b->rng) % 4);

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            src->guildcard);
        return -1;
    }

    for (uint8_t i = 0; i < num_items; ++i) {
        memset(&src->game_data->shop_items[i], 0, sizeof(item_t));
        memset(&item_data, 0, sizeof(item_t));

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

        item_data.item_id = generate_item_id(l, src->client_id);

        memcpy(&src->game_data->shop_items[i], &item_data, sizeof(item_t));
    }

    return subcmd_bb_send_shop(src, shop_type, num_items);
}

int sub62_B7_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_shop_buy_t* pkt) {
    lobby_t* l = src->cur_lobby;
    iitem_t ii = { 0 };
    int found = -1;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " �����𻵵���Ʒ��������!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    DBG_LOG("���� %d ����Ʒ %02X %04X", pkt->num_bought, pkt->unknown_a1, pkt->shdr.unused);

    /* �����Ʒ����ͷ */
    ii.present = LE16(1);
    ii.tech = LE16(0);
    ii.flags = LE32(0);

    /* �����Ʒ���� */
    memcpy(&ii.data.data_b[0], &src->game_data->shop_items[pkt->shop_item_index].data_b[0], sizeof(item_t));

    /* ����Ƕѵ���Ʒ */
    if (pkt->num_bought <= max_stack_size(&ii.data)) {
        ii.data.data_b[5] = pkt->num_bought;
    }
    else {
        ERR_LOG("GC %" PRIu32 " �����𻵵���Ʒ��������!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return 0;
    }

    l->item_player_id[src->client_id] = pkt->new_inv_item_id;
    ii.data.item_id = l->item_player_id[src->client_id]++;

    print_iitem_data(&ii, 0, src->version);

    found = add_item_to_client(src, &ii);

    if (found == -1) {
        ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷������Ʒ!",
            src->guildcard);
        return -1;
    }

    //src->bb_pl->inv.item_count += found;
    //src->pl->bb.inv.item_count = src->bb_pl->inv.item_count;

    uint32_t price = ii.data.data2_l * pkt->num_bought;
    subcmd_send_bb_delete_meseta(src, price, 0);

    return subcmd_send_lobby_bb_create_inv_item(src, ii.data, 1);
}

int sub62_B8_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_tekk_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    block_t* b = src->cur_block;
    subcmd_bb_tekk_identify_result_t i_res = { 0 };
    uint32_t tek_item_slot = 0, attrib = 0;
    uint8_t percent_mod = 0;

    if (src->version == CLIENT_VERSION_BB) {
        if (src->bb_pl->character.disp.meseta < 100)
            return 0;

        if (l->type == LOBBY_TYPE_LOBBY) {
            ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
                src->guildcard);
            return -1;
        }

        if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
            ERR_LOG("GC %" PRIu32 " �����𻵵���Ʒ��������!",
                src->guildcard);
            ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
            return -2;
        }

        tek_item_slot = find_iitem_slot(&src->bb_pl->inv, pkt->item_id);

        /* ��ȡ������Ʒ���ڴ�ָ�� */
        iitem_t* id_result = &(src->game_data->identify_result);

        /* ��ȡ����������Ʒ����λ�õ����� */
        id_result->data = src->bb_pl->inv.iitems[tek_item_slot].data;

        if (src->game_data->gm_debug)
            print_iitem_data(id_result, tek_item_slot, src->version);

        if (id_result->data.data_b[0] != ITEM_TYPE_WEAPON) {
            ERR_LOG("GC %" PRIu32 " �����޷���������Ʒ!",
                src->guildcard);
            return -3;
        }

        // ����������ȡ�����������
        attrib = id_result->data.data_b[4] & ~(0x80);

        if (attrib < 0x29) {
            id_result->data.data_b[4] = tekker_attributes[(attrib * 3) + 1];
            uint32_t mt_result_1 = mt19937_genrand_int32(&b->rng) % 100;
            if (mt_result_1 > 70)
                id_result->data.data_b[4] += mt_result_1 % ((tekker_attributes[(attrib * 3) + 2] - tekker_attributes[(attrib * 3) + 1]) + 1);
        }
        else
            id_result->data.data_b[4] = 0;

        // �ٷֱ���������
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

        // ������ֵ��������
        if (!(id_result->data.data_b[6] & 128) && (id_result->data.data_b[7] > 0))
            id_result->data.data_b[7] += percent_mod;

        if (!(id_result->data.data_b[8] & 128) && (id_result->data.data_b[9] > 0))
            id_result->data.data_b[9] += percent_mod;

        if (!(id_result->data.data_b[10] & 128) && (id_result->data.data_b[11] > 0))
            id_result->data.data_b[11] += percent_mod;

        if (!id_result->data.item_id) {
            ERR_LOG("GC %" PRIu32 " δ������Ҫ��������Ʒ!",
                src->guildcard);
            return -4;
        }

        if (id_result->data.item_id != pkt->item_id) {
            ERR_LOG("GC %" PRIu32 " ���ܵ���ƷID����ǰ�������ƷID��ƥ�� !",
                src->guildcard);
            return -5;
        }

        subcmd_send_bb_delete_meseta(src, 100, 0);

        src->drop_item_id = id_result->data.item_id;
        src->drop_amt = 1;

        return subcmd_send_bb_create_tekk_item(src, id_result->data);
    }
    else
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

}

int sub62_BA_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_accept_item_identification_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t i = 0;
    int found = -1;

    if (src->version == CLIENT_VERSION_BB) {

        iitem_t* id_result = &(src->game_data->identify_result);

        if (!id_result->data.item_id) {
            ERR_LOG("no identify result present");
            return -1;
        }

        if (id_result->data.item_id != pkt->item_id) {
            ERR_LOG("identify_result.data.item_id != pkt->item_id");
            return -1;
        }

        for (i = 0; i < src->pl->bb.inv.item_count; i++)
            if (src->pl->bb.inv.iitems[i].data.item_id == id_result->data.item_id) {
                ERR_LOG("GC %" PRIu32 " û�м����κ�װ�����װ�����ԷǷ�����!",
                    src->guildcard);
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

        /* ���ԴӴ�����Ʒ�����Ƴ�... */
        found = lobby_remove_item_locked(l, pkt->item_id, id_result);
        if (found < 0) {
            /* δ�ҵ���Ҫ�Ƴ�����Ʒ... */
            ERR_LOG("GC %" PRIu32 " ��������Ʒ������!",
                src->guildcard);
            return -1;
        }
        else if (found > 0) {
            /* ��������Ѿ�����, �ͺ�����... */
            ERR_LOG("GC %" PRIu32 " ��������Ʒ�ѱ�ʰȡ!",
                src->guildcard);
            return 0;
        }
        else {

            if (add_item_to_client(src, id_result) == -1) {
                ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷������Ʒ!",
                    src->guildcard);
                return -1;
            }

            /* ��ʼ����ʱ��������Ʒ���� */
            memset(&src->game_data->identify_result, 0, sizeof(iitem_t));

            src->drop_item_id = 0xFFFFFFFF;
            src->drop_amt = 0;

            return subcmd_send_lobby_bb_create_inv_item(src, id_result->data, 0);
        }

    }
    else
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub62_BB_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_bank_open_t* req) {
    lobby_t* l = src->cur_lobby;
    uint8_t* sendbuf = get_sendbuf();
    subcmd_bb_bank_inv_t* pkt = (subcmd_bb_bank_inv_t*)sendbuf;
    uint32_t num_items = LE32(src->bb_pl->bank.item_count);
    uint16_t size = sizeof(subcmd_bb_bank_inv_t) + num_items *
        sizeof(bitem_t);
    block_t* b = src->cur_block;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������!",
            src->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (req->hdr.pkt_len != LE16(0x10) || req->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " ���ʹ�����������ݰ�!",
            src->guildcard);
        return -1;
    }

    /* Clean up the user's bank first... */
    cleanup_bb_bank(src);

    /* ������ݲ�׼������ */
    pkt->hdr.pkt_len = LE16(size);
    pkt->hdr.pkt_type = LE16(GAME_COMMANDC_TYPE);
    pkt->hdr.flags = 0;
    pkt->shdr.type = SUBCMD60_BANK_INV;
    pkt->shdr.size = pkt->shdr.size;
    pkt->shdr.unused = 0x0000;
    pkt->size = LE32(size);
    pkt->checksum = mt19937_genrand_int32(&b->rng); /* Client doesn't care */
    memcpy(&pkt->item_count, &src->bb_pl->bank, sizeof(psocn_bank_t));

    return crypt_send(src, (int)size, sendbuf);
}

int sub62_BD_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_bank_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id;
    uint32_t amt, bank, iitem_count, i;
    int found = -1, stack, isframe = 0;
    iitem_t iitem = { 0 };
    bitem_t bitem = { 0 };
    uint32_t ic[3] = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����в�������!",
            src->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0018) || pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " �����𻵵����в�������!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
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
            iitem_count = LE32(src->bb_pl->character.disp.meseta);

            /* Make sure they aren't trying to do something naughty... */
            if (amt > iitem_count) {
                ERR_LOG("GC %" PRIu32 " depositing more "
                    "meseta than they have!", src->guildcard);
                return -1;
            }

            bank = LE32(src->bb_pl->bank.meseta);
            if (amt + bank > 999999) {
                ERR_LOG("GC %" PRIu32 " depositing too much "
                    "money at the bank!", src->guildcard);
                return -1;
            }

            src->bb_pl->character.disp.meseta = LE32((iitem_count - amt));
            src->pl->bb.character.disp.meseta = src->bb_pl->character.disp.meseta;
            src->bb_pl->bank.meseta = LE32((bank + amt));

            /* No need to tell everyone else, I guess? */
            return 0;
        }
        else {
            /* Look for the item in the user's inventory. */
            iitem_count = src->bb_pl->inv.item_count;
            for (i = 0; i < iitem_count; ++i) {
                if (src->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
                    iitem = src->bb_pl->inv.iitems[i];
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
                    "they do not have!", src->guildcard);
                return -1;
            }

            stack = is_stackable(&iitem.data);

            if (!stack && pkt->item_amount > 1) {
                ERR_LOG("GC %" PRIu32 " banking multiple of "
                    "a non-stackable item!", src->guildcard);
                return -1;
            }

            found = item_remove_from_inv(src->bb_pl->inv.iitems, iitem_count,
                pkt->item_id, pkt->item_amount);

            if (found < 0 || found > 1) {
                ERR_LOG("GC %u Error removing item from inventory for "
                    "banking!", src->guildcard);
                return -1;
            }

            src->bb_pl->inv.item_count = (iitem_count -= found);
            src->pl->bb.inv.item_count = src->bb_pl->inv.item_count;

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
                    iitem = src->bb_pl->inv.iitems[i];
                    if (iitem.data.data_b[0] == ITEM_TYPE_GUARD &&
                        iitem.data.data_b[1] == ITEM_SUBTYPE_UNIT) {
                        src->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
                    }
                }
            }

            /* ����! */
            if (item_deposit_to_bank(src, &bitem) < 0) {
                ERR_LOG("Error depositing to bank for guildcard %"
                    PRIu32 "!", src->guildcard);
                return -1;
            }

            return subcmd_send_bb_destroy_item(src, iitem.data.item_id,
                pkt->item_amount);
        }

    case SUBCMD62_BANK_ACT_TAKE:
        item_id = LE32(pkt->item_id);

        /* Are they taking meseta or an item? */
        if (item_id == 0xFFFFFFFF) {
            amt = LE32(pkt->meseta_amount);
            iitem_count = LE32(src->bb_pl->character.disp.meseta);

            /* Make sure they aren't trying to do something naughty... */
            if (amt + iitem_count > 999999) {
                ERR_LOG("GC %" PRIu32 " ������ȡ���������������˴洢����!", src->guildcard);
                return -1;
            }

            bank = LE32(src->bb_pl->bank.meseta);

            if (amt > bank) {
                ERR_LOG("GC %" PRIu32 " ������ȡ�������������������п��!", src->guildcard);
                return -1;
            }

            src->bb_pl->character.disp.meseta = LE32((iitem_count + amt));
            src->pl->bb.character.disp.meseta = src->bb_pl->character.disp.meseta;
            src->bb_pl->bank.meseta = LE32((bank - amt));

            /* ��ȡ���������ø�֪�����ͻ���... */
            return 0;
        }
        else {
            /* ���Դ�������ȡ����Ʒ. */
            found = item_take_from_bank(src, pkt->item_id, pkt->item_amount, &bitem);

            if (found < 0) {
                ERR_LOG("GC %" PRIu32 " ��������ȡ����Ч��Ʒ!", src->guildcard);
                return -1;
            }

            /* �ѻ�����е���Ʒ����, �����������ʱ����������... */
            iitem.present = LE16(0x0001);
            iitem.tech = LE16(0x0000);
            iitem.flags = LE32(0);
            ic[0] = iitem.data.data_l[0] = bitem.data.data_l[0];
            ic[1] = iitem.data.data_l[1] = bitem.data.data_l[1];
            ic[2] = iitem.data.data_l[2] = bitem.data.data_l[2];
            iitem.data.item_id = LE32(l->bitem_player_id[src->client_id]);
            iitem.data.data2_l = bitem.data.data2_l;
            ++l->bitem_player_id[src->client_id];

            /* ��������ұ�����... */
            found = add_item_to_client(src, &iitem);

            if (found == -1) {
                /* Uh oh... Guess we should put it back in the bank... */
                item_deposit_to_bank(src, &bitem);
                return -1;
            }

            //src->bb_pl->inv.item_count += found;
            //src->pl->bb.inv.item_count = src->bb_pl->inv.item_count;

            /* �����������еĿͻ���. */
            return subcmd_send_lobby_bb_create_inv_item(src, iitem.data, 1);
        }

    default:
        ERR_LOG("GC %" PRIu32 " ����δ֪���в���: %d!",
            src->guildcard, pkt->action);
        display_packet((uint8_t*)pkt, 0x18);
        return -1;
    }
}

int sub62_C1_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_guild_invite_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t invite_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;
    char guild_name_text[24];
    char inviter_name_text[24];

    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " ���ʹ���Ĺ����������ݰ�!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gbk, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gbk, inviter_name_text, &pkt->inviter_name[2], 24, 12);

#ifdef DEBUG
    TEST_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u Ŀ��GC %u ", type, invite_cmd, src->guildcard, dest->guildcard, target_guildcard);
    display_packet((uint8_t*)pkt, len);
#endif // DEBUG

    switch (invite_cmd)
    {
        /* ����������ʼ ���˫���� ������� */
    case 0x00:
        if (dest->bb_guild->data.guild_id != 0) {
            dest->guild_accept = 0;
            DBG_LOG("�����뷽 GUILD ID %u", dest->bb_guild->data.guild_id);
            /* �����û��, ��ȡ�Է��Ѿ�����ĳ������. */
            send_msg(src, MSG1_TYPE, "%s\n\n%s", __(src, "\tE\tC4�޷��������!"),
                __(src, "\tC7�Է����ڹ�����."));
        }
        else
            dest->guild_accept = 1;

        break;

        /* δ֪���� */
    case 0x01:
    default:
        ERR_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u Ŀ��GC %u ", type, invite_cmd, src->guildcard, dest->guildcard, target_guildcard);
        display_packet((uint8_t*)pkt, len);
        break;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_C2_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_guild_invite_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t invite_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;
    char guild_name_text[24];
    char inviter_name_text[24];
    src->guild_accept = 0;

    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " ���ʹ���Ĺ����������ݰ�!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gbk, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gbk, inviter_name_text, &pkt->inviter_name[2], 24, 12);

#ifdef DEBUG
    TEST_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u Ŀ��GC %u ", type, invite_cmd, src->guildcard, d->guildcard, target_guildcard);
    display_packet((uint8_t*)pkt, len);
#endif // DEBUG

    switch (invite_cmd)
    {
        /* ��������ɹ� ���Է��� ������� */
    case 0x02:
        if (src->guildcard == target_guildcard)
            if (src->bb_guild->data.guild_id != 0) {
                DBG_LOG("�����뷽 GUILD ID %u", dest->bb_guild->data.guild_id);
                /* �����û��, ��ȡ�Է��Ѿ�����ĳ������. */
                send_msg(src, MSG1_TYPE, "%s\n\n%s", __(src, "\tE\tC4�޷��������!"),
                    __(src, "\tC7�Է����ڹ�����."));
            }
            else
                src->guild_accept = 1;

        break;

        /* �Է��ܾ����빫�� */
    case 0x03:
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6������:%s\n\tC8��������:%s", __(dest, "\tE\tC4�Է��ܾ����빫��."), inviter_name_text, guild_name_text);
        break;

        /* ��������ʧ�� ��˫�����ش�����Ϣ */
    case 0x04:
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6������:%s\n\tC8��������:%s", __(dest, "\tE\tC4��������ʧ��."), inviter_name_text, guild_name_text);
        break;

    default:
        ERR_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u Ŀ��GC %u ", type, invite_cmd, src->guildcard, dest->guildcard, target_guildcard);
        display_packet((uint8_t*)pkt, len);
        break;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_C9_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_quest_reward_meseta_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int meseta;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " ���Ի�ȡ�������������������!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    meseta = pkt->amount;

    if (meseta < 0) {
        meseta = -meseta;

        if (meseta > (int)src->bb_pl->character.disp.meseta) {
            src->bb_pl->character.disp.meseta = 0;
        }
        else
            src->bb_pl->character.disp.meseta -= meseta;

        return 0;

    }
    else {
        iitem_t ii;
        memset(&ii, 0, sizeof(iitem_t));
        ii.data.data_b[0] = ITEM_TYPE_MESETA;
        ii.data.data2_l = meseta;
        ii.data.item_id = generate_item_id(l, 0xFF);

        if (add_item_to_client(src, &ii) == -1) {
            ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷������Ʒ!",
                src->guildcard);
            return -1;
        }

        return subcmd_send_lobby_bb_create_inv_item(src, ii.data, 0);
    }
}

int sub62_CA_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_quest_reward_item_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            src->guildcard);
        return -1;
    }

    display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    iitem_t ii;
    memset(&ii, 0, sizeof(iitem_t));
    ii.data = pkt->item_data;
    ii.data.item_id = generate_item_id(l, 0xFF);

    if (add_item_to_client(src, &ii) == -1) {
        ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷������Ʒ!",
            src->guildcard);
        return -1;
    }

    return subcmd_send_lobby_bb_create_inv_item(src, ii.data, 0);
}

int sub62_CD_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_guild_master_trans1_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t trans_cmd = pkt->trans_cmd;
    uint32_t client_id_sender = pkt->client_id_sender;
    char guild_name_text[24];
    char master_name_text[24];

    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " ���ʹ���Ĺ���ת�����ݰ�!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gbk, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gbk, master_name_text, &pkt->master_name[2], 24, 12);

    TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS1 0x%02X 0x%08X c %u d %u", type, trans_cmd, src->guildcard, dest->guildcard);
    display_packet((uint8_t*)pkt, len);

    if (src->bb_guild->data.guild_priv_level != BB_GUILD_PRIV_LEVEL_MASTER) {
        ERR_LOG("GC %u ����Ȩ�޲���", src->guildcard);
        return send_msg(src, MSG1_TYPE, "%s\n\n%s", __(src, "\tE\tC4����Ȩ�޲���!"),
            __(src, "\tC7����Ȩ���д˲���."));
    }

    src->guild_master_exfer = trans_cmd;

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_CE_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_guild_master_trans2_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t trans_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;
    char guild_name_text[24];
    char master_name_text[24];
    dest->guild_master_exfer = 0;

    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " ���ʹ���Ĺ���ת�����ݰ�!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gbk, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gbk, master_name_text, &pkt->master_name[2], 24, 12);

    TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS2 0x%02X 0x%08X c %u d %u", type, trans_cmd, src->guildcard, dest->guildcard);
    display_packet((uint8_t*)pkt, len);

    switch (trans_cmd)
    {
    case 0x01:
        dest->guild_master_exfer = 1;
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6������:%s\n\tC8��������:%s", __(dest, "\tE\tC4�᳤�����."), master_name_text, guild_name_text);
        break;

        /* ����ת�Ƴɹ� TODO ֪ͨ������Ա */
    case 0x02:
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6������:%s\n\tC8��������:%s", __(dest, "\tE\tC4�᳤�ѱ��."), master_name_text, guild_name_text);
        break;

        /* �Է��ܾ���Ϊ�᳤ */
    case 0x03:
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6������:%s\n\tC8��������:%s", __(dest, "\tE\tC4�Է��ܾ���Ϊ�᳤."), master_name_text, guild_name_text);
        break;

        /* ����ת��ʧ�� ��˫�����ش�����Ϣ */
    case 0x04:
        send_msg(src, TEXT_MSG_TYPE, "%s\n\tC6������:%s\n\tC8��������:%s", __(src, "\tE\tC4����ת��ʧ��."), master_name_text, guild_name_text);
        break;

    default:
        ERR_LOG("SUBCMD62_GUILD_MASTER_TRANS 0x%02X 0x%08X c %u d %u", type, trans_cmd, src->guildcard, dest->guildcard);
        break;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_D6_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_warp_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t iitem_count, found, i, stack;
    uint32_t wrap_id;
    iitem_t backup_item;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            src->guildcard);
        return -1;
    }

    memset(&backup_item, 0, sizeof(iitem_t));
    wrap_id = *(uint32_t*)&pkt->data[0x0C];

    for (i = 0; i < src->bb_pl->inv.item_count; i++) {
        if (src->bb_pl->inv.iitems[i].data.item_id == wrap_id) {
            memcpy(&backup_item, &src->bb_pl->inv.iitems[i], sizeof(iitem_t));
            break;
        }
    }

    if (backup_item.data.item_id) {
        stack = is_stackable(&backup_item.data);

        //if (!stack && pkt->item_amount > 1) {
        //    ERR_LOG("GC %" PRIu32 " banking multiple of "
        //        "a non-stackable item!", c->guildcard);
        //    return -1;
        //}

        iitem_count = src->bb_pl->inv.item_count;

        if ((found = item_remove_from_inv(src->bb_pl->inv.iitems, iitem_count,
            backup_item.data.item_id, 1)) < 1) {
            ERR_LOG("�޷�����ұ������Ƴ���Ʒ!");
            return -1;
        }

        if (found < 0 || found > 1) {
            ERR_LOG("GC %u Error removing item from inventory for "
                "banking!", src->guildcard);
            return -1;
        }

        src->bb_pl->inv.item_count = (iitem_count -= found);

        if (backup_item.data.data_b[0] == 0x02)
            backup_item.data.data2_b[2] |= 0x40; // Wrap a mag
        else
            backup_item.data.data_b[4] |= 0x40; // Wrap other

        /* ����Ʒ����������. */
        found = add_item_to_client(src, &backup_item);

        if (found == -1)
            return 0;
    }
    else {
        ERR_LOG("GC %" PRIu32 " ������ƷID %d ʧ��!",
            src->guildcard, wrap_id);
        return -1;
    }

    return 0;
}

int sub62_DF_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_quest_oneperson_set_ex_pc_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t iitem_count, found;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            src->guildcard);
        return -1;
    }

    if ((l->oneperson) && (l->flags & LOBBY_FLAG_QUESTING) && (!l->drops_disabled)) {
        iitem_t tmp_iitem;
        iitem_count = src->bb_pl->inv.item_count;

        memset(&tmp_iitem, 0, sizeof(iitem_t));
        tmp_iitem.data.data_b[0] = 0x03;
        tmp_iitem.data.data_b[1] = 0x10;
        tmp_iitem.data.data_b[2] = 0x02;

        found = item_remove_from_inv(src->bb_pl->inv.iitems, iitem_count, 0, 1);

        if (found < 0 || found > 1) {
            ERR_LOG("GC %u Error handle_bb_quest_oneperson_set_ex_pc!", src->guildcard);
            return -1;
        }

        src->bb_pl->inv.item_count = (iitem_count -= found);
        src->pl->bb.inv.item_count = src->bb_pl->inv.item_count;

        if (!found) {
            l->drops_disabled = 1;
            return -1;
        }
    }

    return 0;
}

// ���庯��ָ������
subcmd_handle_func_t subcmd62_handler[] = {
    //    cmd_type                         DC           GC           EP3          XBOX         PC           BB
    { SUBCMD62_GUILDCARD                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_06_bb },
    { SUBCMD62_PICK_UP                   , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_5A_bb },
    { SUBCMD62_BURST5                    , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_6F_bb },
    { SUBCMD62_BURST6                    , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_71_bb },
    { SUBCMD62_TRADE                     , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_A6_bb },
    { SUBCMD62_CHAIR_STATE               , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_AE_bb },
    { SUBCMD62_SHOP_REQ                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B5_bb },
    { SUBCMD62_SHOP_BUY                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B7_bb },
    { SUBCMD62_TEKKING                   , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B8_bb },
    { SUBCMD62_TEKKED                    , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_BA_bb },
    { SUBCMD62_OPEN_BANK                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_BB_bb },
    { SUBCMD62_BANK_ACTION               , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_BD_bb },
    { SUBCMD62_GUILD_INVITE1             , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_C1_bb },
    { SUBCMD62_GUILD_INVITE2             , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_C2_bb },
    { SUBCMD62_QUEST_REWARD_MESETA       , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_C9_bb },
    { SUBCMD62_QUEST_REWARD_ITEM         , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_CA_bb },
    { SUBCMD62_GUILD_MASTER_TRANS1       , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_CD_bb },
    { SUBCMD62_GUILD_MASTER_TRANS2       , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_CE_bb },
    { SUBCMD62_WARP_ITEM                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_D6_bb },
    { SUBCMD62_QUEST_ONEPERSON_SET_EX_PC , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_DF_bb },
};

/* ����BB 0x62 ���ݰ�. */
int subcmd_bb_handle_62(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* dest;
    uint16_t len = pkt->hdr.pkt_len;
    uint16_t hdr_type = pkt->hdr.pkt_type;
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

#ifdef DEBUG

    DBG_LOG("src %d GC%u dest %d GC%u 0x%02X ָ��: 0x%02X", c->client_id, c->guildcard, dnum, l->clients[dnum]->guildcard, hdr_type, type);

#endif // DEBUG

    l->subcmd62_handle = subcmd_get_handler(hdr_type, type, c->version);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;

        switch (type) {
        case SUBCMD62_BURST5://0x62 6F //��������ԾǨ����ʱ���� 5
        case SUBCMD62_BURST6://0x62 71 //��������ԾǨ����ʱ���� 6
            //send_bb_quest_data1(dest, &c->bb_pl->quest_data1);

            rv |= l->subcmd62_handle(c, dest, pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x62 ָ��: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

    }
    else {
        switch (type) {
        case SUBCMD62_ITEMREQ:
        case SUBCMD62_BITEMREQ:
            rv = l->dropfunc(c, l, pkt);
            pthread_mutex_unlock(&l->mutex);
            return rv;
        }

        if (l->subcmd62_handle == NULL) {
#ifdef BB_LOG_UNKNOWN_SUBS
            DBG_LOG("δ֪ 0x%02X ָ��: 0x%02X", hdr_type, type);
            display_packet(pkt, len);
            //UNK_CSPD(type, c->version, pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
            rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        }
        else
            rv = l->subcmd62_handle(c, dest, pkt);
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}
