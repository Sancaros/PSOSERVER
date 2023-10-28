/*
    梦幻之星中国 舰船服务器 62指令处理
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
#include <SFMT.h>
#include <items.h>

#include "subcmd.h"
#include "subcmd_send.h"
#include "shop.h"
#include "pmtdata.h"
#include "clients.h"
#include "ship_packets.h"
#include "utils.h"
#include "handle_player_items.h"
#include "word_select.h"
#include "scripts.h"
#include "shipgate.h"
#include "quest_functions.h"
#include "mag_bb.h"
#include "pso_items_black_paper_reward_list.h"
#include "ptdata.h"
#include "enemy_type.h"

#include "subcmd_handle.h"
#include "admin.h"
#include "lobby_f.h"
#include <pso_items_coren_reward_list.h>

static int handle_itemreq_gm(ship_client_t* src,
    subcmd_itemreq_t* req) {
    subcmd_itemgen_t gen = { 0 };
    int r = LE16(req->request_id);
    int i;
    lobby_t* l = src->cur_lobby;

    /* 填充数据并准备发送. */
    gen.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    gen.hdr.flags = 0;
    gen.hdr.pkt_len = LE16(0x30);
    gen.shdr.type = SUBCMD60_ITEM_DROP_BOX_ENEMY;
    gen.shdr.size = 0x0B;
    gen.shdr.unused = 0x0000;
    gen.data.area = req->area;
    gen.data.from_enemy = 0x02;
    gen.data.request_id = req->request_id;
    gen.data.x = req->x;
    gen.data.z = req->z;
    gen.data.unk1 = LE16(0x00000010);

    gen.data.item.datal[0] = LE32(src->new_item.datal[0]);
    gen.data.item.datal[1] = LE32(src->new_item.datal[1]);
    gen.data.item.datal[2] = LE32(src->new_item.datal[2]);
    gen.data.item.data2l = LE32(src->new_item.data2l);
    gen.data.item2 = LE32(0x00000002);

    /* Obviously not "right", but it works though, so we'll go with it. */
    gen.data.item.item_id = LE32((r | 0x06010100));

    /* Send the packet to every client in the lobby. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]) {
            send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&gen);
        }
    }

    /* Clear this out. */
    clear_inv_item(&src->new_item);

    return 0;
}

static int handle_itemreq_quest(ship_client_t* src, ship_client_t* dest,
    subcmd_itemreq_t* req) {
    uint32_t mid = LE16(req->request_id);
    uint32_t pti = req->pt_index;
    lobby_t* l = src->cur_lobby;
    uint32_t qdrop = 0xFFFFFFFF;

    if (pti != 0x30 && l->mids)
        qdrop = quest_search_enemy_list(mid, l->mids, l->num_mids, 0);
    if (qdrop == 0xFFFFFFFF && l->mtypes)
        qdrop = quest_search_enemy_list(pti, l->mtypes, l->num_mtypes, 0);

    /* If we found something, the version matters here. Basically, we only care
       about the none option on DC/PC, as rares do not drop in quests. On GC,
       we have to block drops on all options other than free, since we have no
       control over the drop once we send it to the leader. */
    if (qdrop != PSOCN_QUEST_ENDROP_FREE && qdrop != 0xFFFFFFFF) {
        switch (l->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
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
    return send_pkt_dc(dest, (dc_pkt_hdr_t*)req);
}

int sub62_06_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_dc_gcsend_t* pkt) {

    ERR_LOG(
        "handle_dc_gcsend %d 类型 = %d 版本识别 = %d",
        dest->sock, pkt->hdr.pkt_type, dest->version);

    /* Make sure the recipient is not ignoring the sender... */
    if (client_has_ignored(dest, src->guildcard))
        return 0;

    /* This differs based on the destination client's version. */
    switch (dest->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
        return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    {
        subcmd_gc_gcsend_t gc;

        memset(&gc, 0, sizeof(gc));

        /* Copy the name and text over. */
        memcpy(gc.name, pkt->name, 24);
        memcpy(gc.guildcard_desc, pkt->guildcard_desc, 88);

        /* Copy the rest over. */
        gc.hdr.pkt_type = pkt->hdr.pkt_type;
        gc.hdr.flags = pkt->hdr.flags;
        gc.hdr.pkt_len = LE16(0x0098);
        gc.shdr.type = pkt->shdr.type;
        gc.shdr.size = 0x25;
        gc.shdr.unused = 0x0000;
        gc.player_tag = pkt->player_tag;
        gc.guildcard = pkt->guildcard;
        gc.padding = 0;
        gc.disable_udp = 1;
        gc.language = pkt->language;
        gc.section = pkt->section;
        gc.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&gc);
    }

    case CLIENT_VERSION_XBOX:
    {
        subcmd_xb_gcsend_t xb;
        uint32_t guildcard = LE32(pkt->guildcard);

        memset(&xb, 0, sizeof(xb));

        /* Copy the name and text over. */
        memcpy(xb.name, pkt->name, 24);
        memcpy(xb.guildcard_desc, pkt->guildcard_desc, 88);

        /* Copy the rest over. */
        xb.hdr.pkt_type = pkt->hdr.pkt_type;
        xb.hdr.flags = pkt->hdr.flags;
        xb.hdr.pkt_len = LE16(0x0234);
        xb.shdr.type = pkt->shdr.type;
        xb.shdr.size = 0x8C;
        xb.shdr.unused = LE16(0xFB0D);
        xb.player_tag = pkt->player_tag;
        xb.guildcard = pkt->guildcard;
        xb.xbl_userid = LE64(guildcard);
        xb.disable_udp = 1;
        xb.language = pkt->language;
        xb.section = pkt->section;
        xb.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&xb);
    }

    case CLIENT_VERSION_PC:
    {
        subcmd_pc_gcsend_t pc;
        size_t in, out;
        char* inptr;
        char* outptr;

        /* Don't allow guild cards to be sent to PC NTE, as it doesn't
           support them. */
        if ((dest->flags & CLIENT_FLAG_IS_NTE)) {
            if (src)
                return send_txt(src, "%s", __(src, "\tE\tC7Cannot send Guild\n"
                    "Card to that user."));
            else
                return 0;
        }

        memset(&pc, 0, sizeof(pc));

        /* Convert the name (ASCII -> UTF-16). */
        in = 24;
        out = 48;
        inptr = pkt->name;
        outptr = (char*)pc.name;
        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

        /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
        in = 88;
        out = 176;
        inptr = pkt->guildcard_desc;
        outptr = (char*)pc.guildcard_desc;

        if (pkt->guildcard_desc[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        pc.hdr.pkt_type = pkt->hdr.pkt_type;
        pc.hdr.flags = pkt->hdr.flags;
        pc.hdr.pkt_len = LE16(0x00F8);
        pc.shdr.type = pkt->shdr.type;
        pc.shdr.size = 0x3D;
        pc.shdr.unused = 0x0000;
        pc.player_tag = pkt->player_tag;
        pc.guildcard = pkt->guildcard;
        pc.padding = 0;
        pc.disable_udp = 1;
        pc.language = pkt->language;
        pc.section = pkt->section;
        pc.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&pc);
    }

    case CLIENT_VERSION_BB:
    {
        subcmd_bb_gcsend_t bb;
        size_t in, out;
        char* inptr;
        char* outptr;

        memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));

        /* Convert the name (ASCII -> UTF-16). */
        bb.name[0] = LE16('\t');
        bb.name[1] = LE16('E');
        in = 24;
        out = 44;
        inptr = pkt->name;
        outptr = (char*)&bb.name[2];
        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

        /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
        in = 88;
        out = 176;
        inptr = pkt->guildcard_desc;
        outptr = (char*)bb.guildcard_desc;

        if (pkt->guildcard_desc[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        bb.hdr.pkt_len = LE16(0x0114);
        bb.hdr.pkt_type = LE16(GAME_SUBCMD62_TYPE);
        bb.hdr.flags = LE32(dest->client_id);
        bb.shdr.type = SUBCMD62_GUILDCARD;
        bb.shdr.size = 0x43;
        bb.shdr.unused = 0x0000;
        bb.guildcard = pkt->guildcard;
        bb.disable_udp = 1;
        bb.language = pkt->language;
        bb.section = pkt->section;
        bb.ch_class = pkt->ch_class;

        return send_pkt_bb(dest, (bb_pkt_hdr_t*)&bb);
    }
    }

    return 0;
}

int sub62_06_pc(ship_client_t* src, ship_client_t* dest,
    subcmd_pc_gcsend_t* pkt) {

    /* Make sure the recipient is not ignoring the sender... */
    if (client_has_ignored(dest, src->guildcard))
        return 0;

    /* This differs based on the destination client's version. */
    switch (dest->version) {
    case CLIENT_VERSION_PC:
        /* Don't allow guild cards to be sent to PC NTE, as it doesn't
           support them. */
        if ((dest->flags & CLIENT_FLAG_IS_NTE))
            return send_txt(src, "%s", __(src, "\tE\tC7无法发送GC至该玩家."));

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);

    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    {
        subcmd_dc_gcsend_t dc;
        size_t in, out;
        char* inptr;
        char* outptr;

        memset(&dc, 0, sizeof(dc));

        /* Convert the name (UTF-16 -> ASCII). */
        in = 48;
        out = 24;
        inptr = (char*)pkt->name;
        outptr = dc.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)pkt->guildcard_desc;
        outptr = dc.guildcard_desc;

        if (pkt->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        dc.hdr.pkt_type = pkt->hdr.pkt_type;
        dc.hdr.flags = pkt->hdr.flags;
        dc.hdr.pkt_len = LE16(0x0088);
        dc.shdr.type = pkt->shdr.type;
        dc.shdr.size = 0x21;
        dc.shdr.unused = 0x0000;
        dc.player_tag = pkt->player_tag;
        dc.guildcard = pkt->guildcard;
        dc.unused2 = 0;
        dc.disable_udp = 1;
        dc.language = pkt->language;
        dc.section = pkt->section;
        dc.ch_class = pkt->ch_class;
        dc.padding[0] = dc.padding[1] = dc.padding[2] = 0;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&dc);
    }

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    {
        subcmd_gc_gcsend_t gc;
        size_t in, out;
        char* inptr;
        char* outptr;

        memset(&gc, 0, sizeof(gc));

        /* Convert the name (UTF-16 -> ASCII). */
        in = 48;
        out = 24;
        inptr = (char*)pkt->name;
        outptr = gc.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)pkt->guildcard_desc;
        outptr = gc.guildcard_desc;

        if (pkt->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        gc.hdr.pkt_type = pkt->hdr.pkt_type;
        gc.hdr.flags = pkt->hdr.flags;
        gc.hdr.pkt_len = LE16(0x0098);
        gc.shdr.type = pkt->shdr.type;
        gc.shdr.size = 0x25;
        gc.shdr.unused = 0x0000;
        gc.player_tag = pkt->player_tag;
        gc.guildcard = pkt->guildcard;
        gc.padding = 0;
        gc.disable_udp = 1;
        gc.language = pkt->language;
        gc.section = pkt->section;
        gc.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&gc);
    }

    case CLIENT_VERSION_XBOX:
    {
        subcmd_xb_gcsend_t xb;
        uint32_t guildcard = LE32(pkt->guildcard);
        size_t in, out;
        char* inptr;
        char* outptr;

        memset(&xb, 0, sizeof(xb));

        /* Convert the name (UTF-16 -> ASCII). */
        in = 48;
        out = 24;
        inptr = (char*)pkt->name;
        outptr = xb.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 512;
        inptr = (char*)pkt->guildcard_desc;
        outptr = xb.guildcard_desc;

        if (pkt->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        xb.hdr.pkt_type = pkt->hdr.pkt_type;
        xb.hdr.flags = pkt->hdr.flags;
        xb.hdr.pkt_len = LE16(0x0234);
        xb.shdr.type = pkt->shdr.type;
        xb.shdr.size = 0x8C;
        xb.shdr.unused = LE16(0xFB0D);
        xb.player_tag = pkt->player_tag;
        xb.guildcard = pkt->guildcard;
        xb.xbl_userid = LE64(guildcard);
        xb.disable_udp = 1;
        xb.language = pkt->language;
        xb.section = pkt->section;
        xb.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&xb);
    }

    case CLIENT_VERSION_BB:
    {
        subcmd_bb_gcsend_t bb;

        /* 填充数据并准备发送.. */
        memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));
        bb.hdr.pkt_len = LE16(0x0114);
        bb.hdr.pkt_type = LE16(GAME_SUBCMD62_TYPE);
        bb.hdr.flags = LE32(dest->client_id);
        bb.shdr.type = SUBCMD62_GUILDCARD;
        bb.shdr.size = 0x43;
        bb.shdr.unused = 0x0000;
        bb.guildcard = pkt->guildcard;
        bb.name[0] = LE16('\t');
        bb.name[1] = LE16('E');
        memcpy(&bb.name[2], pkt->name, 28);
        memcpy(bb.guildcard_desc, pkt->guildcard_desc, 176);
        bb.disable_udp = 1;
        bb.language = pkt->language;
        bb.section = pkt->section;
        bb.ch_class = pkt->ch_class;

        return send_pkt_bb(dest, (bb_pkt_hdr_t*)&bb);
    }
    }

    return 0;
}

int sub62_06_gc(ship_client_t* src, ship_client_t* dest,
    subcmd_gc_gcsend_t* pkt) {

    /* Make sure the recipient is not ignoring the sender... */
    if (client_has_ignored(dest, src->guildcard))
        return 0;

    /* This differs based on the destination client's version. */
    switch (dest->version) {
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
        return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);

    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    {
        subcmd_dc_gcsend_t dc;

        memset(&dc, 0, sizeof(dc));

        /* Copy the name and text over. */
        memcpy(dc.name, pkt->name, 24);
        memcpy(dc.guildcard_desc, pkt->guildcard_desc, 88);

        /* Copy the rest over. */
        dc.hdr.pkt_type = pkt->hdr.pkt_type;
        dc.hdr.flags = pkt->hdr.flags;
        dc.hdr.pkt_len = LE16(0x0088);
        dc.shdr.type = pkt->shdr.type;
        dc.shdr.size = 0x21;
        dc.shdr.unused = 0x0000;
        dc.player_tag = pkt->player_tag;
        dc.guildcard = pkt->guildcard;
        dc.unused2 = 0;
        dc.disable_udp = 1;
        dc.language = pkt->language;
        dc.section = pkt->section;
        dc.ch_class = pkt->ch_class;
        dc.padding[0] = dc.padding[1] = dc.padding[2] = 0;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&dc);
    }

    case CLIENT_VERSION_XBOX:
    {
        subcmd_xb_gcsend_t xb;
        uint32_t guildcard = LE32(pkt->guildcard);

        memset(&xb, 0, sizeof(xb));

        /* Copy the name and text over. */
        memcpy(xb.name, pkt->name, 24);
        memcpy(xb.guildcard_desc, pkt->guildcard_desc, 104);

        /* Copy the rest over. */
        xb.hdr.pkt_type = pkt->hdr.pkt_type;
        xb.hdr.flags = pkt->hdr.flags;
        xb.hdr.pkt_len = LE16(0x0234);
        xb.shdr.type = pkt->shdr.type;
        xb.shdr.size = 0x8C;
        xb.shdr.unused = LE16(0xFB0D);
        xb.player_tag = pkt->player_tag;
        xb.guildcard = pkt->guildcard;
        xb.xbl_userid = LE64(guildcard);
        xb.disable_udp = 1;
        xb.language = pkt->language;
        xb.section = pkt->section;
        xb.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&xb);
    }

    case CLIENT_VERSION_PC:
    {
        subcmd_pc_gcsend_t pc;
        size_t in, out;
        char* inptr;
        char* outptr;

        /* Don't allow guild cards to be sent to PC NTE, as it doesn't
           support them. */
        if ((dest->flags & CLIENT_FLAG_IS_NTE))
            return send_txt(src, "%s", __(src, "\tE\tC无法发送GC至该玩家."));

        memset(&pc, 0, sizeof(pc));

        /* Convert the name (ASCII -> UTF-16). */
        in = 24;
        out = 48;
        inptr = pkt->name;
        outptr = (char*)pc.name;
        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

        /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
        in = 88;
        out = 176;
        inptr = pkt->guildcard_desc;
        outptr = (char*)pc.guildcard_desc;

        if (pkt->guildcard_desc[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        pc.hdr.pkt_type = pkt->hdr.pkt_type;
        pc.hdr.flags = pkt->hdr.flags;
        pc.hdr.pkt_len = LE16(0x00F8);
        pc.shdr.type = pkt->shdr.type;
        pc.shdr.size = 0x3D;
        pc.shdr.unused = 0x0000;
        pc.player_tag = pkt->player_tag;
        pc.guildcard = pkt->guildcard;
        pc.padding = 0;
        pc.disable_udp = 1;
        pc.language = pkt->language;
        pc.section = pkt->section;
        pc.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&pc);
    }

    case CLIENT_VERSION_BB:
    {
        subcmd_bb_gcsend_t bb;
        size_t in, out;
        char* inptr;
        char* outptr;

        memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));

        /* Convert the name (ASCII -> UTF-16). */
        bb.name[0] = LE16('\t');
        bb.name[1] = LE16('E');
        in = 24;
        out = 44;
        inptr = pkt->name;
        outptr = (char*)&bb.name[2];
        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

        /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
        in = 88;
        out = 176;
        inptr = pkt->guildcard_desc;
        outptr = (char*)bb.guildcard_desc;

        if (pkt->guildcard_desc[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        bb.hdr.pkt_len = LE16(0x0114);
        bb.hdr.pkt_type = LE16(GAME_SUBCMD62_TYPE);
        bb.hdr.flags = LE32(dest->client_id);
        bb.shdr.type = SUBCMD62_GUILDCARD;
        bb.shdr.size = 0x43;
        bb.shdr.unused = 0x0000;
        bb.guildcard = pkt->guildcard;
        bb.disable_udp = 1;
        bb.language = pkt->language;
        bb.section = pkt->section;
        bb.ch_class = pkt->ch_class;

        return send_pkt_bb(dest, (bb_pkt_hdr_t*)&bb);
    }
    }

    return 0;
}

int sub62_06_xb(ship_client_t* src, ship_client_t* dest,
    subcmd_xb_gcsend_t* pkt) {

    /* Make sure the recipient is not ignoring the sender... */
    if (client_has_ignored(dest, src->guildcard))
        return 0;

    /* This differs based on the destination client's version. */
    switch (dest->version) {
    case CLIENT_VERSION_XBOX:
        return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);

    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    {
        subcmd_dc_gcsend_t dc;

        memset(&dc, 0, sizeof(dc));

        /* Copy the name and text over. */
        memcpy(dc.name, pkt->name, 24);
        memcpy(dc.guildcard_desc, pkt->guildcard_desc, 88);

        /* Copy the rest over. */
        dc.hdr.pkt_type = pkt->hdr.pkt_type;
        dc.hdr.flags = pkt->hdr.flags;
        dc.hdr.pkt_len = LE16(0x0088);
        dc.shdr.type = pkt->shdr.type;
        dc.shdr.size = 0x21;
        dc.shdr.unused = 0x0000;
        dc.player_tag = pkt->player_tag;
        dc.guildcard = pkt->guildcard;
        dc.unused2 = 0;
        dc.disable_udp = 1;
        dc.language = pkt->language;
        dc.section = pkt->section;
        dc.ch_class = pkt->ch_class;
        dc.padding[0] = dc.padding[1] = dc.padding[2] = 0;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&dc);
    }

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    {
        subcmd_gc_gcsend_t gc;

        memset(&gc, 0, sizeof(gc));

        /* Copy the name and text over. */
        memcpy(gc.name, pkt->name, 24);
        memcpy(gc.guildcard_desc, pkt->guildcard_desc, 88);

        /* Copy the rest over. */
        gc.hdr.pkt_type = pkt->hdr.pkt_type;
        gc.hdr.flags = pkt->hdr.flags;
        gc.hdr.pkt_len = LE16(0x0098);
        gc.shdr.type = pkt->shdr.type;
        gc.shdr.size = 0x25;
        gc.shdr.unused = 0x0000;
        gc.player_tag = pkt->player_tag;
        gc.guildcard = pkt->guildcard;
        gc.padding = 0;
        gc.disable_udp = 1;
        gc.language = pkt->language;
        gc.section = pkt->section;
        gc.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&gc);
    }

    case CLIENT_VERSION_PC:
    {
        subcmd_pc_gcsend_t pc;
        size_t in, out;
        char* inptr;
        char* outptr;

        /* Don't allow guild cards to be sent to PC NTE, as it doesn't
           support them. */
        if ((dest->flags & CLIENT_FLAG_IS_NTE)) {
            if (src)
                return send_txt(src, "%s", __(src, "\tE\tC7无法发送GC至该玩家."));
            else
                return 0;
        }

        memset(&pc, 0, sizeof(pc));

        /* Convert the name (ASCII -> UTF-16). */
        in = 24;
        out = 48;
        inptr = pkt->name;
        outptr = (char*)pc.name;
        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

        /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
        in = 88;
        out = 176;
        inptr = pkt->guildcard_desc;
        outptr = (char*)pc.guildcard_desc;

        if (pkt->guildcard_desc[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        pc.hdr.pkt_type = pkt->hdr.pkt_type;
        pc.hdr.flags = pkt->hdr.flags;
        pc.hdr.pkt_len = LE16(0x00F8);
        pc.shdr.type = pkt->shdr.type;
        pc.shdr.size = 0x3D;
        pc.shdr.unused = 0x0000;
        pc.player_tag = pkt->player_tag;
        pc.guildcard = pkt->guildcard;
        pc.padding = 0;
        pc.disable_udp = 1;
        pc.language = pkt->language;
        pc.section = pkt->section;
        pc.ch_class = pkt->ch_class;

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&pc);
    }

    case CLIENT_VERSION_BB:
    {
        subcmd_bb_gcsend_t bb;
        size_t in, out;
        char* inptr;
        char* outptr;

        memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));

        /* Convert the name (ASCII -> UTF-16). */
        bb.name[0] = LE16('\t');
        bb.name[1] = LE16('E');
        in = 24;
        out = 44;
        inptr = pkt->name;
        outptr = (char*)&bb.name[2];
        iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

        /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
        in = 88;
        out = 176;
        inptr = pkt->guildcard_desc;
        outptr = (char*)bb.guildcard_desc;

        if (pkt->guildcard_desc[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        bb.hdr.pkt_len = LE16(0x0114);
        bb.hdr.pkt_type = LE16(GAME_SUBCMD62_TYPE);
        bb.hdr.flags = LE32(dest->client_id);
        bb.shdr.type = SUBCMD62_GUILDCARD;
        bb.shdr.size = 0x43;
        bb.shdr.unused = 0x0000;
        bb.guildcard = pkt->guildcard;
        bb.disable_udp = 1;
        bb.language = pkt->language;
        bb.section = pkt->section;
        bb.ch_class = pkt->ch_class;

        return send_pkt_bb(dest, (bb_pkt_hdr_t*)&bb);
    }
    }

    return 0;
}

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
        //memset(&dc.name, '-', 16);
        //in = 48;
        //out = 24;
        //inptr = (char*)&src->pl->bb.character.name;
        //outptr = dc.name;
        //iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        istrncpy16_raw(ic_utf16_to_ascii, dc.name, &src->pl->bb.character.name, 24, 12);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)src->bb_pl->guildcard_desc;
        outptr = dc.guildcard_desc;

        if (src->bb_pl->guildcard_desc[1] == LE16('J')) {
            istrncpy16_raw(ic_utf16_to_sjis, dc.guildcard_desc, &src->bb_pl->guildcard_desc, 88, 88);
            //iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            istrncpy16_raw(ic_utf16_to_8859, dc.guildcard_desc, &src->bb_pl->guildcard_desc, 88, 88);
            //iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        dc.hdr.pkt_type = GAME_SUBCMD62_TYPE;
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
        memcpy(pc.name, &src->pl->bb.character.name.name_tag, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
        memcpy(pc.guildcard_desc, src->bb_pl->guildcard_desc, sizeof(src->bb_pl->guildcard_desc));

        /* Copy the rest over. */
        pc.hdr.pkt_type = GAME_SUBCMD62_TYPE;
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
        gc.hdr.pkt_type = GAME_SUBCMD62_TYPE;
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
        xb.hdr.pkt_type = GAME_SUBCMD62_TYPE;
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
        bb.hdr.pkt_type = LE16(GAME_SUBCMD62_TYPE);
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

int sub62_5A_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pick_up_item_req_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->item_id;
    uint16_t area = pkt->area;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_pick_up_item_req_t), 0x03))
        return -2;

    if (src->cur_area != area) {
        ERR_LOG("%s picked up item in area they are "
            "not currently in!", get_player_describe(src));
    }

    /* Clear the list of dropped items. */
    if (src->cur_area == 0) {
        memset(src->p2_drops, 0, sizeof(src->p2_drops));
        src->p2_drops_max = 0;
    }

    /* Maybe do more in the future with inventory tracking? */
    return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
}

int sub62_5A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pick_up_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->item_id;
    uint16_t area = pkt->area;
    int pick_count;
    item_t item_data = { 0 };

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_pick_up_t), 0x03))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("%s 发送错误的拾取数据!", get_player_describe(src));
        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
        return -1;
    }

    /* 尝试从大厅物品栏中移除... */
    pick_count = remove_litem_locked(l, item_id, &item_data);
    if (pick_count < 0) {
        return -1;
    }
    else if (pick_count > 0) {
        PICKS_LOG("%s 尝试拾取不存在的地面物品", get_player_describe(src));
        PRINT_HEX_LOG(PICKS_LOG, pkt, pkt->hdr.pkt_len);
        /* 假设别人已经捡到了, 就忽略它... */
        return 0;
    }
    else {
        //PICKS_LOG("-----------------物品ID %d 个拾取情况----------------- ", pkt->item_id);
        //PICKS_LOG("%s %s 区域 %d 拾取情况!"
        //    , get_player_describe(src)
        //    , get_section_describe(src, get_player_section(src), true)
        //    , pkt->area
        //);
        //PICKS_LOG("%s", get_lobby_describe(l));
        //print_item_data(&iitem_data.data, l->version);
        LOBBY_PICKITEM_LOG(src, item_id, area, &item_data);
        /* Add the item to the client's inventory. */
        if (!add_invitem(src, item_data)) {
            ERR_LOG("%s 拾取物品出错", get_player_describe(src));
            print_item_data(&item_data, src->version);
            return -1;
        }
        //size_t id = destroy_item_id(l, src->client_id);
        //DBG_LOG("id %u", id);
    }

    /* 让所有人都知道是该客户端捡到的，并将其从所有人视线中删除. */
    subcmd_send_lobby_bb_create_inv_item(src, item_data, stack_size(&item_data), true);

    return subcmd_send_bb_del_map_item(src, area, item_data.item_id);
}

int sub62_60_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (src->new_item.datal[0] &&
        !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        return  handle_itemreq_gm(src, (subcmd_itemreq_t*)pkt);
    }
    else if (l->dropfunc &&
        (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
        return  l->dropfunc(src, l, pkt);
    }
    else if ((l->num_mtypes || l->num_mids) &&
        (l->flags & LOBBY_FLAG_QUESTING)) {
        return handle_itemreq_quest(src, dest, (subcmd_itemreq_t*)pkt);
    }
    else {
        return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
    }
}

int sub62_60_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_itemreq_t* pkt) {
    lobby_t* l = src->cur_lobby;
    item_t item = { 0 };
    int rv = 0;
    uint8_t section = 0, pt_index = get_pt_index(l->episode, pkt->pt_index), drop_area = pkt->area, pt_area = get_pt_data_area_bb(l->episode, drop_area);
    uint16_t mid = LE16(pkt->entity_id);

    if (!in_game(src))
        return -1;

    /* 房间判断是否取消掉落 */
    if (l->drops_disabled)
        return 0;

    if (mid > l->map_enemies->enemy_count) {
        ITEM_LOG("%s 请求无效敌人掉落 (%d -- max: %d, 任务=%" PRIu32 ")!", get_player_describe(src), mid,
            l->map_enemies->enemy_count, l->qid);
        return 0;
    }

    /* Grab the map enemy to make sure it hasn't already dropped something. */
    game_enemy_t* enemy = &l->map_enemies->enemies[mid];
    if (enemy->drop_done)
        return 0;

    /* TODO */
    if(!l->drop_pso2)
        enemy->drop_done = 1;

    //pt_index = enemy->rt_index;

    //uint32_t expected_rt_index = rare_table_index_for_enemy_type(enemy->rt_index);

    /* 独立掉落模式 */
    if (l->drop_pso2) {
        /* Send the packet to every connected client. */
        for (int i = 0; i < l->max_clients; ++i) {
            if (!l->clients[i])
                continue;

            ship_client_t* p2 = l->clients[i];
            pthread_mutex_lock(&p2->mutex);
            psocn_bb_char_t* p2_char = get_client_char_bb(p2);
            section = p2_char->dress_data.section;
            /* 复用模式 独立 随机颜色ID */
            if (l->drop_psocn) {
                section = sfmt_genrand_uint32(&p2->sfmt_rng) % 10;
            }

            item = on_monster_item_drop(l, &p2->sfmt_rng, pt_index, drop_area, pt_area, section);

            LOBBY_MOB_DROPITEM_LOG(p2, mid, pt_index, drop_area, pt_area, &item);
            if (is_item_empty(&item)) {
                pthread_mutex_unlock(&p2->mutex);
                continue;
            }

            litem_t* lt = add_lobby_litem_locked(l, &item, drop_area, pkt->x, pkt->z, true);
            if (!lt) {
                pthread_mutex_unlock(&p2->mutex);
                ERR_LOG("%s 无法将物品添加至游戏房间!", get_player_describe(p2));
                print_item_data(&item, l->version);
                continue;
            }
#ifdef DEBUG
            print_item_data(&lt->item, l->version);
#endif // DEBUG

            rv = subcmd_send_bb_drop_box_or_enemy_item(p2, pkt, &lt->item);
            pthread_mutex_unlock(&p2->mutex);
        }
    }
    else {
        pthread_mutex_lock(&src->mutex);
        psocn_bb_char_t* character = get_client_char_bb(src);
        section = get_lobby_leader_section(l);

        if (!l->map_enemies) {
            ERR_LOG("%s 游戏并未载入地图敌人数据", get_player_describe(src));
        }

        item = on_monster_item_drop(l, &src->sfmt_rng, pt_index, drop_area, pt_area, section);

        LOBBY_MOB_DROPITEM_LOG(src, mid, pt_index, drop_area, pt_area, &item);
        if (is_item_empty(&item)) {
            pthread_mutex_unlock(&src->mutex);
            return 0;
        }

        litem_t* lt = add_lobby_litem_locked(l, &item, drop_area, pkt->x, pkt->z, true);
        if (!lt) {
            pthread_mutex_unlock(&src->mutex);
            ERR_LOG("%s 无法将物品添加至游戏房间!", get_player_describe(src));
            print_item_data(&item, l->version);
            return 0;
        }

        rv = subcmd_send_lobby_bb_drop_box_or_enemy_item(src, NULL, pkt, &lt->item);
        pthread_mutex_unlock(&src->mutex);
    }

    return rv;
    //return l->dropfunc(src, l, pkt);
}

int sub62_6F_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {

    if (dest->version == CLIENT_VERSION_BB)
        send_bb_quest_data1(dest);

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_6F_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {

    return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
}

int sub62_71_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_71_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {

    return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
}

int sub62_A2_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (src->new_item.datal[0] &&
        !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        return  handle_itemreq_gm(src, (subcmd_itemreq_t*)pkt);
    }
    else if (l->dropfunc &&
        (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
        return  l->dropfunc(src, l, pkt);
    }
    else if ((l->num_mtypes || l->num_mids) &&
        (l->flags & LOBBY_FLAG_QUESTING)) {
        return handle_itemreq_quest(src, dest, (subcmd_itemreq_t*)pkt);
    }
    else {
        return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
    }
}

int sub62_A2_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_bitemreq_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;
    uint8_t drop_area = pkt->area, pt_area = get_pt_data_area_bb(l->episode, drop_area), section = 0, pt_index = pkt->pt_index;                   /* Always 0x30 */
    uint16_t request_id = pkt->request_id, ignore_def = pkt->ignore_def;
    item_t item = { 0 };

    if (!in_game(src))
        return -1;

    /* 房间判断是否取消掉落 */
    if (l->drops_disabled)
        return 0;

    if (l->drop_pso2) {
        /* Send the packet to every connected client. */
        for (int i = 0; i < l->max_clients; ++i) {
            if (!l->clients[i])
                continue;

            ship_client_t* p2 = l->clients[i];
            pthread_mutex_lock(&p2->mutex);
            psocn_bb_char_t* p2_char = get_client_char_bb(p2);
            section = p2_char->dress_data.section;
            if (l->drop_psocn) {
                section = sfmt_genrand_uint32(&p2->sfmt_rng) % 10;
            }

            if (ignore_def) {
                item = on_box_item_drop(l, &p2->sfmt_rng, drop_area, pt_area, section);
            }
            else
                item = on_specialized_box_item_drop(l, &p2->sfmt_rng, pt_area,
                    pkt->def[0], pkt->def[1], pkt->def[2]);

            LOBBY_BOX_DROPITEM_LOG(p2, request_id, pt_index, ignore_def, drop_area
                , pt_area, &item);
            if (is_item_empty(&item)) {
                pthread_mutex_unlock(&p2->mutex);
                continue;
            }

            litem_t* lt = add_lobby_litem_locked(l, &item, pkt->area, pkt->x, pkt->z, true);
            if (!lt) {
                pthread_mutex_unlock(&p2->mutex);
                ERR_LOG("%s 无法将物品添加至游戏房间! pkt->ignore_def %d", get_player_describe(p2), pkt->ignore_def);
                print_item_data(&item, l->version);
                continue;
            }

            rv = subcmd_send_bb_drop_box_or_enemy_item(p2, (subcmd_bb_itemreq_t*)pkt, &lt->item);
            pthread_mutex_unlock(&p2->mutex);
        }

    }
    else {
        pthread_mutex_lock(&src->mutex);
        psocn_bb_char_t* character = get_client_char_bb(src);
        section = get_lobby_leader_section(l);
        //drop_area = src->cur_area;
        //PRINT_HEX_LOG(DBG_LOG, pkt, pkt->hdr.pkt_len);

        if (ignore_def) {
            item = on_box_item_drop(l, &src->sfmt_rng, drop_area, pt_area, section);
        }
        else
            item = on_specialized_box_item_drop(l, &src->sfmt_rng, pt_area,
                pkt->def[0], pkt->def[1], pkt->def[2]);

        LOBBY_BOX_DROPITEM_LOG(src, request_id, pt_index, ignore_def, drop_area
            , pt_area, &item);
        if (is_item_empty(&item)) {
            pthread_mutex_unlock(&src->mutex);
            return 0;
        }

        litem_t* lt = add_lobby_litem_locked(l, &item, pkt->area, pkt->x, pkt->z, true);
        if (!lt) {
            pthread_mutex_unlock(&src->mutex);
            ERR_LOG("%s 无法将物品添加至游戏房间!", get_player_describe(src));
            print_item_data(&item, l->version);
            return 0;
        }

        rv = subcmd_send_lobby_bb_drop_box_or_enemy_item(src, NULL, (subcmd_bb_itemreq_t*)pkt, &lt->item);
        pthread_mutex_unlock(&src->mutex);
    }

    return rv;

    //return l->dropfunc(src, l, pkt);
}

int sub62_A6_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_trade_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t trade_type = pkt->trade_type, trade_stage = pkt->trade_stage;
    int rv = -1;
    size_t item_id = 0;
    item_t trade_i = { 0 };

    if (!in_game(src))
        return rv;

    if (!check_pkt_size(src, pkt, 0, 0x04))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("%s 发送损坏的数据指令 0x%02X! 数据大小 %02X", get_player_describe(src), pkt->shdr.type, pkt->shdr.size);
        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
        return rv;
    }

    //send_msg(dest, BB_SCROLL_MSG_TYPE, "%s", __(dest, "\tE\tC6交易功能存在故障，暂时关闭"));
    //send_msg(src, BB_SCROLL_MSG_TYPE, "%s", __(src, "\tE\tC6交易功能存在故障，暂时关闭"));

    psocn_bb_char_t* player = get_client_char_bb(src);
    trade_inv_t* trade_inv_src = get_client_trade_inv_bb(src);
    trade_inv_t* trade_inv_dest = get_client_trade_inv_bb(dest);

    //DBG_LOG("GC %u sclient_id 0x%02X -> %u dclient_id 0x%02X ", src->guildcard, src->client_id, dest->guildcard, dest->client_id);

    //PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);

    switch (trade_type) {
    case 0x00:

        switch (trade_stage) {
        case 0x00:
            //交易开始
        //[2023年08月14日 20:03:54:452] 调试(subcmd_handle_62.c 1316): GC 10000001 -> 10000000
        //( 00000000 )   18 00 62 00 00 00 00 00   A6 04 01 00 00 00 AC 00  ..b.....?....?
        //( 00000010 )   82 00 00 00 0E 87 01 00                           ?...?.
            trade_inv_src = player_tinv_init(src);
            trade_inv_src->other_client_id = dest->client_id;
        case 0x02:
            //交易开始确认
        //[2023年08月14日 20:07:19:815] 调试(subcmd_handle_62.c 1316): GC 10000000 -> 10000001
        //( 00000000 )   18 00 62 00 01 00 00 00   A6 04 00 00 00 02 11 0F  ..b.....?......
        //( 00000010 )   30 47 11 0F C7 C9 0C 00                           0G..巧..
            trade_inv_src = player_tinv_init(src);
            trade_inv_src->other_client_id = dest->client_id;
        case 0x03:
            //对方拒绝交易
        //[2023年08月14日 20:12:57:651] 调试(subcmd_handle_62.c 1316): GC 10000000 -> 10000001
        //( 00000000 )   18 00 62 00 01 00 00 00   A6 04 00 00 00 03 11 0F  ..b.....?......
        //( 00000010 )   30 47 11 0F C8 75 81 00                           0G..萿?
            trade_inv_src = player_tinv_init(src);
            trade_inv_dest = player_tinv_init(dest);
            break;
        }
        break;

    case 0x01:
        switch (trade_stage) {
        case 0x00:
            //需要一个以双方的id来储存的一个物品ID容器
            //被交易方加入物品栏ID 0x00010000 物品
        //[2023年08月14日 20:15:20:984] 调试(subcmd_handle_62.c 1316): GC 10000000 -> 10000001
        //( 00000000 )   18 00 62 00 01 00 00 00   A6 04 00 00 01 00 71 00  ..b.....?....q.
        //( 00000010 )   00 00 01 00 01 00 00 00                           ........
            //交易方加入物品栏ID 0x00210000 物品
        //[2023年08月14日 20:17:58:493] 调试(subcmd_handle_62.c 1316): GC 10000001 -> 10000000
        //( 00000000 )   18 00 62 00 00 00 00 00   A6 04 01 00 01 00 71 00  ..b.....?....q.
        //( 00000010 )   00 00 21 00 01 00 00 00                           ..!.....

            //要判定加入得物品是否是堆叠得 如果是堆叠得物品 则相同交易物品的数量+1
            //0xFFFFFF01 是美赛塔？？？？ 不应该是FFFFFFFF吗
            if (pkt->item_id == 0xFFFFFF01) {
                if (player->disp.meseta < pkt->amount) {
                    player->disp.meseta = 0;
                }

                trade_inv_src->trade_item_count++;
                trade_inv_src->meseta = min(pkt->amount, 999999);
                trade_i.datab[0] = 0x04;
                trade_i.item_id = 0xFFFFFFFF;
                trade_i.data2l = trade_inv_src->meseta;
            }
            else {

                item_id = find_iitem_index(&player->inv, pkt->item_id);
                /* 如果找不到该物品，则将用户从船上推下. */
                if (item_id < 0) {
                    ERR_LOG("%s 交易无效物品! 错误码 %d", get_player_describe(src), item_id);
                    return item_id;
                }

                trade_i = player->inv.iitems[item_id].data;

                if (pkt->amount && is_stackable(&trade_i) &&
                    (pkt->amount < trade_i.datab[5])) {
                    trade_i.datab[5] = pkt->amount;
                }
            }

            if (!add_titem(trade_inv_src, trade_i)) {
                ERR_LOG("%s 无法添加交易物品!", get_player_describe(src));
                return -3;
            }
            break;
        }
        break;

    case 0x02:
        //被交易方取消物品栏ID 0x00010000 物品
    //[2023年08月14日 20:19:21:886] 调试(subcmd_handle_62.c 1316): GC 10000000 -> 10000001
    //( 00000000 )   18 00 62 00 01 00 00 00   A6 04 00 00 02 00 71 00  ..b.....?....q.
    //( 00000010 )   00 00 01 00 01 00 00 00                           ........
        //减掉对应的交易物品 从客户端的“交易物品栏”中删除对应的物品ID

        if (pkt->item_id == 0xFFFFFF01) {
            if (pkt->amount <= trade_inv_src->meseta) {
                trade_inv_src->meseta -= pkt->amount;
            }
            else {
                trade_inv_src->meseta = 0;
                trade_inv_src->trade_item_count--;
            }

            trade_i.datab[0] = 0x04;
            trade_i.item_id = 0xFFFFFFFF;
        }
        else {

            item_id = check_titem_id(trade_inv_src, pkt->item_id);
            if (item_id != pkt->item_id) {
                ERR_LOG("%s 交易无效物品! 错误码 %d", get_player_describe(src), item_id);
                return item_id;
            }

            trade_i.item_id = item_id;
        }

        item_t tmp = remove_titem(trade_inv_src, trade_i.item_id, pkt->amount);
        if (item_not_identification_bb(tmp.datal[0], tmp.datal[1])) {
            ERR_LOG("%s 移除非法交易物品!", get_player_describe(src));
            print_item_data(&tmp, src->version);
            return -4;
        }

        break;

    case 0x03:
        switch (trade_stage) {
        case 0x00:
            /* 完成确认 */
            trade_inv_src->confirmed = true;
            //交易方确认
        //[2023年08月14日 20:53:33:705] 调试(subcmd_handle_62.c 1316): GC 10000001 -> 10000000
        //( 00000000 )   18 00 62 00 00 00 00 00   A6 04 01 00 03 00 8F 00  ..b.....?....?
        //( 00000010 )   FF FF FF FF 00 00 00 00                           ....

            //被交易方确认
        //[2023年08月14日 20:40:33:430] 调试(subcmd_handle_62.c 1316): GC 10000000 -> 10000001
        //( 00000000 )   18 00 62 00 01 00 00 00   A6 04 00 00 03 00 8F 00  ..b.....?....?
        //( 00000010 )   FF FF FF FF 00 00 00 00                           ....
            break;

        case 0x04:
            /* 完成确认 */
            trade_inv_src->confirmed = false;
            //交易方取消确认
//[2023年09月22日 00:00:42:249] 调试(subcmd_handle_62.c 1447): GC 10000000 sclient_id 0x00 -> 10000001 dclient_id 0x01
//[2023年09月22日 00:00:42:252] 调试(f_logs.c 0140): 数据包如下:
//(00000000) 18 00 62 00 01 00 00 00  A6 04 00 00 03 04 82 00    ..b.............
//(00000010) D9 5B 82 00 60 70 5D 0D     .[..`p].

            //被交易方取消确认
//[2023年09月22日 00:00:44:415] 调试(subcmd_handle_62.c 1447): GC 10000001 sclient_id 0x01 -> 10000000 dclient_id 0x00
//[2023年09月22日 00:00:44:419] 调试(f_logs.c 0140): 数据包如下:
//(00000000) 18 00 62 00 00 00 00 00  A6 04 01 00 03 04 82 00    ..b.............
//(00000010) D9 5B 82 00 60 50 5F 0D     .[..`P_.
            break;
        }
        break;
    case 0x04:
        switch (trade_stage) {
        case 0x00:
            if (!trade_inv_src->confirmed)
                ERR_LOG("GC %u 根本没有确认", get_player_describe(src));

            trade_inv_src->confirmed = true;
            //交易方确认交易
//[2023年08月14日 21:01:06:843] 调试(subcmd_handle_62.c 1316): GC 10000001 -> 10000000
//( 00000000 )   18 00 62 00 00 00 00 00   A6 04 01 00 04 00 1C 0F  ..b.....?......
//( 00000010 )   40 AF 1C 0F 40 AF 1C 0F                           @?.@?.
            //被交易方确认交易
//[2023年08月14日 21:02:39:912] 调试(subcmd_handle_62.c 1316): GC 10000000 -> 10000001
//( 00000000 )   18 00 62 00 01 00 00 00   A6 04 00 00 04 00 11 0F  ..b.....?......
//( 00000010 )   40 3F 11 0F 40 3F 11 0F                           @?..@?..
            break;
        }
        break;
    case 0x05:
        switch (trade_stage) {
        case 0x04:
            //交易终止
    //[2023年08月14日 20:12:03:817] 调试(subcmd_handle_62.c 1316): GC 10000000 -> 10000001
    //( 00000000 )   18 00 62 00 01 00 00 00   A6 04 00 00 05 04 00 00  ..b.....?......
    //( 00000010 )   9D 3A 71 00 01 00 00 00                           ?q.....
            trade_inv_src = player_tinv_init(src);
            trade_inv_dest = player_tinv_init(dest);
            break;
        }
        break;

    case 0x07:
        /* 其中一位玩家移动到了无法交易的区域 */
        /*[2023年09月21日 22:41:03:355] 错误(subcmd_handle_62.c 1572): 交易数据未处理 trade_type 0x07 trade_stage 0x00
[2023年09月21日 22:41:03:357] 调试(f_logs.c 0140): 数据包如下:
(00000000) 18 00 62 00 01 00 00 00  A6 04 00 00 07 00 01 00    ..b.............
(00000010) FF FF FF FF 00 00 00 00     ........*/
        trade_inv_src = player_tinv_init(src);
        trade_inv_dest = player_tinv_init(dest);
        break;

    default:
        ERR_LOG("交易数据未处理 trade_type 0x%02X trade_stage 0x%02X", trade_type, trade_stage);
        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
        break;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_AE_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_send_lobby_chair_state_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = -1;

    if (!check_pkt_size(src, pkt, 0, 0x04))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("%s 发送损坏的数据指令 0x%02X! 数据大小 %02X", get_player_describe(src), pkt->shdr.type, pkt->shdr.size);
        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
        return rv;
    }

    return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
}
//
//int sub62_AE_pc(ship_client_t* src, ship_client_t* dest,
//    subcmd_pc_send_lobby_chair_state_t* pkt) {
//    lobby_t* l = src->cur_lobby;
//    int rv = -1;
//
//    if (!check_pkt_size(src, pkt, 0, 0x04))
//        return -2;
//
//    if (pkt->shdr.client_id != src->client_id) {
//        ERR_LOG("%s 发送损坏的数据指令 0x%02X! 数据大小 %02X", get_player_describe(src), pkt->shdr.type, pkt->shdr.size);
//        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
//        return rv;
//    }
//
//    return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
//}

int sub62_AE_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_send_lobby_chair_state_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = -1;

    if (!check_pkt_size(src, pkt, 0, 0x04))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("%s 发送损坏的数据指令 0x%02X! 数据大小 %02X", get_player_describe(src), pkt->shdr.type, pkt->shdr.size);
        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
        return rv;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_B5_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_shop_req_t* pkt) {
    lobby_t* l = src->cur_lobby;
    block_t* b = src->cur_block;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_shop_req_t), 0x02))
        return -2;

    uint32_t shop_type = LE32(pkt->shop_type);
    uint8_t num_item_count = 10;
    uint32_t player_level = get_player_level(src);
    uint8_t random_shop = src->game_data->random_shop;

    if (player_level < 11) {
        num_item_count = 10;
    }
    else if (player_level < 43) {
        num_item_count = 12;
    }
    else {
        num_item_count = 16;
    }

    size_t shop_item_count = ARRAYSIZE(src->game_data->shop_items[random_shop][shop_type]);
    bool create = true;
    uint8_t i = 0;
    uint32_t uniqueNumbers[2][3][40] = { 0 };  // 用于保存已生成的唯一数据

    sfmt_t sfmt_rng = { 0 };
    sfmt_init_gen_rand(&sfmt_rng, (uint32_t)srv_time);

    sfmt_t* 随机因子 = NULL;

    if (random_shop)
        随机因子 = &sfmt_rng;
    else
        随机因子 = &src->sfmt_rng;

    for (size_t x = 0; x < 0x14; x++) {
        clear_inv_item(&src->game_data->shop_items[random_shop][shop_type][x]);
        src->game_data->shop_items_price[random_shop][shop_type][x] = 0;
    }

    while (i < num_item_count) {
        create = true;
        if (num_item_count > shop_item_count) {
            ERR_LOG("%s 商店物品生成错误 num_items %d > shop_size %d", get_player_describe(src), num_item_count, shop_item_count);
            create = false;
            break;
        }

        item_t item = create_bb_shop_items(player_level, random_shop, shop_type, l->difficulty, i, 随机因子);
        if (item_not_identification_bb(item.datal[0], item.datal[1])) {
            create = false;
            send_msg(src, MSG1_TYPE, "%s", __(src, "\tE\tC4商店生成错误,菜单类型缺失,请联系管理员处理!"));
            return -1;
        }

        pmt_weapon_bb_t tmp_wp = { 0 };
        pmt_guard_bb_t tmp_guard = { 0 };

        switch (item.datab[0]) {
        case ITEM_TYPE_WEAPON:
            if (pmt_lookup_weapon_bb(item.datal[0], &tmp_wp)) {
                create = false;
                continue;
            }

            if (!item_check_equip(tmp_wp.equip_flag, src->equip_flags)) {create = false;
                continue;
            }
            break;

        case ITEM_TYPE_GUARD:
            switch (item.datab[1]) {
            case ITEM_SUBTYPE_FRAME:
                if (pmt_lookup_guard_bb(item.datal[0], &tmp_guard)) {create = false;
                    continue;
                }

                if (!item_check_equip(tmp_guard.equip_flag, src->equip_flags)) {create = false;
                    continue;
                }
                break;

            case ITEM_SUBTYPE_BARRIER:
                if (pmt_lookup_guard_bb(item.datal[0], &tmp_guard)) {
                    create = false;
                    continue;
                }

                if (!item_check_equip(tmp_guard.equip_flag, src->equip_flags)) {create = false;
                    continue;
                }
                break;

            case ITEM_SUBTYPE_UNIT:
                if (char_class_is_android(src->equip_flags)) {
                    for (size_t x = 4; x < 7;x++) {
                        if (item.datab[2] == x) {
                            create = false;
                            continue;
                        }
                    }
                }
                break;
            }
            break;
        }

        if (create) {
            if (isUnique(uniqueNumbers[random_shop][shop_type], i, item.datal[0])) {  // 检查是否已经生成过该数
                uniqueNumbers[random_shop][shop_type][i] = item.datal[0];  // 将生成的唯一数据添加到数组中
                size_t shop_price = price_for_item(&item);
                if (shop_price <= 0) {
                    ERR_LOG("%s:%d 生成 ID 0x%08X %s(0x%08X) 发生错误 shop_price = %d"
                        , get_player_describe(src), src->sec_data.slot, item.item_id, item_get_name(&item, src->version, 0), item.datal[0], shop_price);
                    create = false;
                    continue;
                }

                item.item_id = generate_item_id(l, src->client_id);
                src->game_data->shop_items[random_shop][shop_type][i] = item;
                src->game_data->shop_items_price[random_shop][shop_type][i] = shop_price;

                i++;
            }
        }
    }

    if (!create) {
        ERR_LOG("菜单类型缺失 shop_type = %d", shop_type);
        return -4;
    }

    return subcmd_bb_send_shop(src, random_shop, shop_type, num_item_count, create);
}

int sub62_B7_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_shop_buy_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int found = -1;
    uint32_t new_inv_item_id = pkt->new_inv_item_id;
    uint8_t random_shop = src->game_data->random_shop, 
        shop_type = pkt->shop_type, 
        shop_item_index = pkt->shop_item_index, 
        num_bought = pkt->num_bought;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_shop_buy_t), 0x03))
        return -2;

    psocn_bb_char_t* character = get_client_char_bb(src);

#ifdef DEBUG

    DBG_LOG("购买 %d 个物品 %02X %04X", num_bought, pkt->unknown_a1, pkt->shdr.unused);

#endif // DEBUG

    /* 填充物品数据 */
    item_t item = src->game_data->shop_items[random_shop][shop_type][shop_item_index];

    /* 如果是堆叠物品 */
    if (is_stackable(&item)) {
        if (num_bought <= max_stack_size(&item)) {
            item.datab[5] = num_bought;
        }
        else {
            ERR_LOG("%s 发送损坏的物品购买数据!",
                get_player_describe(src));
            PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
            print_item_data(&item, src->version);
            return -3;
        }
    }

    item.item_id = new_inv_item_id;
    item.datab[5] = get_item_amount(&item, num_bought);

    uint32_t price = src->game_data->shop_items_price[random_shop][shop_type][shop_item_index] * num_bought;

    if (character->disp.meseta < price) {
        ERR_LOG("%s 发送损坏的数据! 0x%02X MESETA %d PRICE %d",
            get_player_describe(src), pkt->shdr.type, character->disp.meseta, price);
        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
        print_item_data(&item, src->version);
        return -4;
    }

    if (!add_invitem(src, item)) {
        ERR_LOG("%s 背包空间不足, 无法获得物品!",
            get_player_describe(src));
        return -5;
    }

    subcmd_send_lobby_bb_delete_meseta(src, character, price, false);

#ifdef DEBUG

    DBG_LOG("扣款后 meseta %d price %d", character->disp.meseta, price);

#endif // DEBUG

    return subcmd_send_lobby_bb_create_inv_item(src, item, num_bought, false);
}

int sub62_B8_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_tekk_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    sfmt_t* rng = &src->sfmt_rng;
    uint32_t id_item_index = 0;
    uint32_t item_id = pkt->item_id;
    char percent_mod = 0;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_tekk_item_t), 0x02))
        return -2;

    psocn_bb_char_t* character = get_client_char_bb(src);

    if (character->disp.meseta < 100) {
        DBG_LOG("sub62_B8_bb 玩家没钱了 %d", character->disp.meseta);
        return send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC4你没钱啦"));
    }

    id_item_index = find_iitem_index(&character->inv, item_id);
    if (id_item_index < 0) {
        ERR_LOG("%s 鉴定的物品无效! 错误码 %d", get_player_describe(src), id_item_index);
        return id_item_index;
    }

    if (character->inv.iitems[id_item_index].data.datab[0] != ITEM_TYPE_WEAPON) {
        ERR_LOG("%s 发送无法鉴定的物品!",
            get_player_describe(src));
        send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC4鉴定物品出错 -3"));
        return -2;
    }

    subcmd_send_lobby_bb_delete_meseta(src, character, 100, false);
    iitem_t* id_result = &character->inv.iitems[id_item_index];

    if (id_result->data.item_id == EMPTY_STRING) {
        ERR_LOG("%s 未发送需要鉴定的物品!",
            get_player_describe(src));
        send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC4鉴定物品出错 -4"));
        return -3;
    }

    if (id_result->data.item_id != item_id) {
        ERR_LOG("%s 接受的物品ID与以前请求的物品ID不匹配 !",
            get_player_describe(src));
        send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC4鉴定物品出错 -5"));
        return -4;
    }

    /* 获取鉴定的物品结果 */
    src->game_data->identify_result = *id_result;

    //if (player_tekker_item(src, &src->sfmt_rng, &src->game_data->identify_result.data)) {
    //    ERR_LOG("%s 发送无法鉴定的物品!",
    //        get_player_describe(src));
    //    send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC4鉴定物品出错 -3"));
    //    return -5;
    //}

    ssize_t luck = player_tekker_item(src, &src->sfmt_rng, &src->game_data->identify_result.data);

    switch (luck) {
    case 1:
        break;

    case 2:
        if (is_item_rare(&src->game_data->identify_result.data))
            announce_message(src, false, BB_SCROLL_MSG_TYPE, "[奸商鉴定]: "
                "恭喜 %s \tE\tC8有点幸运\tE\tC7,鉴定出了 "
                "\tE\tC6%s.",
                get_player_name(src->pl, src->version, false),
                get_item_describe(&src->game_data->identify_result.data, src->version)
            );
        break;

    case 3:
        if (is_item_rare(&src->game_data->identify_result.data))
            announce_message(src, false, BB_SCROLL_MSG_TYPE, "[奸商鉴定]: "
                "恭喜 %s \tE\tC6最佳运气\tE\tC7,鉴定出了 "
                "\tE\tC6%s.",
                get_player_name(src->pl, src->version, false),
                get_item_describe(&src->game_data->identify_result.data, src->version)
            );
        break;

    default:
        break;
    }

    src->drop_item_id = src->game_data->identify_result.data.item_id;
    src->drop_amt = 1;

    LOBBY_TEKKITEM_LOG(src, item_id, src->cur_area, &src->game_data->identify_result.data);

    if (src->game_data->gm_debug)
        print_item_data(&src->game_data->identify_result.data, src->version);

    return subcmd_send_bb_create_tekk_item(src, src->game_data->identify_result.data);
}

int sub62_BA_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_accept_item_identification_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t i = 0;
    int found = -1;

    if (!in_game(src))
        return -1;

    item_t id_result = src->game_data->identify_result.data;

    if (!id_result.item_id) {
        ERR_LOG("%s 未获取到鉴定结果", get_player_describe(src));
        return -1;
    }

    if (id_result.item_id != pkt->item_id) {
        ERR_LOG("%s 鉴定结果 item_id != 数据包 item_id", get_player_describe(src));
        return -1;
    }

    LOBBY_TEKKITEM_LOG(src, pkt->item_id, src->cur_area, &id_result);

    if (!add_invitem(src, id_result)) {
        ERR_LOG("%s 背包空间不足, 无法获得物品!",
            get_player_describe(src));
        return -1;
    }

    subcmd_send_lobby_bb_create_inv_item(src, id_result, stack_size(&id_result), true);

    /* 初始化临时鉴定的物品数据 */
    memset(&src->game_data->identify_result, 0, PSOCN_STLENGTH_IITEM);

    src->drop_item_id = 0xFFFFFFFF;
    src->drop_amt = 0;

    return 0;
}

int sub62_BB_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_bank_open_t* req) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, req, sizeof(subcmd_bb_bank_open_t), 0x02))
        return -2;

    psocn_bank_t* bank = get_client_bank_bb(src);

    send_msg(src, BB_SCROLL_MSG_TYPE, "%s 数量 %u 美赛塔 %u", src->bank_type ?
        __(src, "\tE\tC6公共仓库") : __(src, "\tE\tC6角色仓库")
        , bank->item_count
        , bank->meseta
    );

    /* Clean up the user's bank first... */
    regenerate_bank_item_id(src->client_id, bank, src->bank_type);

    return subcmd_send_bb_bank(src, bank);
}

int sub62_BD_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_bank_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = LE32(pkt->item_id);
    uint8_t action = pkt->action;  // 0 = deposit, 1 = take
    item_t item = { 0 };
    bitem_t bitem = { 0 };

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_bank_act_t), 0x04))
        return -2;

    psocn_bank_t* bank = get_client_bank_bb(src);
    psocn_bb_char_t* character = get_client_char_bb(src);

    uint32_t amt = LE32(pkt->meseta_amount),
        bank_amt = LE32(bank->meseta),
        c_mst_amt = LE32(character->disp.meseta),
        pkt_item_amt = LE32(pkt->item_amount);
    uint16_t pkt_bitem_index = pkt->bitem_index;

#ifdef DEBUG

    DBG_LOG("pkt_item_amt %d bitem_index %d", pkt_item_amt, pkt->bitem_index);

#endif // DEBUG

    switch (action) {
    case SUBCMD62_BANK_ACT_CLOSE:
        send_txt(src, "%s", src->bank_type ?
            __(src, "\tE\tC6公共仓库") : __(src, "\tE\tC6角色仓库")
        );
    case SUBCMD62_BANK_ACT_DONE:
        break;

    case SUBCMD62_BANK_ACT_DEPOSIT:
        bank = get_client_bank_bb(src);
        character = get_client_char_bb(src);
        amt = LE32(pkt->meseta_amount);
        bank_amt = LE32(bank->meseta);
        c_mst_amt = LE32(character->disp.meseta);
        pkt_item_amt = LE32(pkt->item_amount);
        pkt_bitem_index = pkt->bitem_index;

        /* Are they depositing meseta or an item? */
        if (item_id == 0xFFFFFFFF) {
            /* Make sure they aren't trying to do something naughty... */
            if (amt > c_mst_amt) {
                ERR_LOG("%s 存入银行的美赛塔超出他所拥有的!amt %d c_mst_amt %d", get_player_describe(src), amt, c_mst_amt);
                return -1;
            }

            bank_amt = LE32(bank->meseta);
            if ((amt + bank_amt) > 999999) {
                ERR_LOG("%s 存入银行的美赛塔超出限制!amt %d bank_amt %d", get_player_describe(src), amt, bank_amt);
                return -2;
            }

            bank->meseta += amt;
            character->disp.meseta -= amt;
            src->pl->bb.character.disp.meseta = character->disp.meseta;
        }
        else {
            item = remove_invitem(src, item_id, pkt_item_amt, src->version != CLIENT_VERSION_BB);
            LOBBY_BANK_DEPOSIT_ITEM_LOG(src, item_id, src->cur_area, &item);
            if (item.datal[0] == 0 && item.data2l == 0) {
                ERR_LOG("%s 移除了不存在于背包的物品!", get_player_describe(src));
                return -3;
            }

            /* 已获得背包的物品数据, 将其添加至银行数据中... */
            bitem = player_bitem_init(item);

            /* 存入! */
            if (!add_bitem(src, bitem)) {
                ERR_LOG("%s 存物品进银行错误!", get_player_describe(src));
                return -5;
            }

            subcmd_send_lobby_bb_destroy_item(src, item.item_id,
                pkt_item_amt);

            sort_client_bank(bank);
        }

        break;

    case SUBCMD62_BANK_ACT_TAKE:
        bank = get_client_bank_bb(src);
        character = get_client_char_bb(src);
        amt = LE32(pkt->meseta_amount);
        bank_amt = LE32(bank->meseta);
        c_mst_amt = LE32(character->disp.meseta);
        pkt_item_amt = LE32(pkt->item_amount);
        pkt_bitem_index = pkt->bitem_index;

        /* Are they taking meseta or an item? */
        if (item_id == ITEM_ID_MESETA) {
            if (amt > bank_amt) {
                ERR_LOG("%s 从银行取出的美赛塔超出了银行库存!amt %d bank_amt %d", get_player_describe(src), amt, bank_amt);
                return -6;
            }

            /* Make sure they aren't trying to do something naughty... */
            if ((amt + c_mst_amt) > 999999) {
                ERR_LOG("%s 从银行取出的美赛塔超出了存储限制!amt %d c_mst_amt %d", get_player_describe(src), amt, c_mst_amt);
                return -7;
            }

            bank_amt = LE32(bank->meseta);

            bank->meseta -= amt;
            character->disp.meseta += amt;
            src->pl->bb.character.disp.meseta = character->disp.meseta;
        }
        else {
            if (pkt_bitem_index >= bank->item_count) {
                ERR_LOG("%s 银行物品索引有误! pkt_bitem_index == %d", get_player_describe(src), pkt_bitem_index);
                return -8;
            }

            /* 尝试从银行中取出物品. */
            bitem = remove_bitem(src, item_id, pkt_bitem_index, pkt_item_amt);
            LOBBY_BANK_TAKE_ITEM_LOG(src, item_id, src->cur_area, &bitem.data);
            if (&bitem == NULL) {
                ERR_LOG("%s 从银行中取出无效物品!", get_player_describe(src));
                return -9;
            }

            bitem.data.item_id = generate_item_id(l, src->client_id);
            /* 已获得银行的物品数据, 将其添加至临时背包数据中... */
            item = bitem.data;

            /* 新增至玩家背包中... */
            if (!add_invitem(src, item)) {
                ERR_LOG("%s 物品从玩家银行取出失败!",
                    get_player_describe(src));
                return -11;
            }

            /* 发送至房间中的客户端. */
            subcmd_send_lobby_bb_create_inv_item(src, item, pkt_item_amt, true);
        }

        break;

    default:
        ERR_LOG("%s 发送未知银行操作: %d!", get_player_describe(src), action);
        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
        break;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_C1_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_guild_invite_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t invite_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_guild_invite_t), 0x17))
        return -2;

#ifdef DEBUG
    char guild_name_text[24];
    char inviter_name_text[24];

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gb18030, inviter_name_text, &pkt->inviter_name[2], 24, 12);

    TEST_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ", type, invite_cmd, src->guildcard, dest->guildcard, target_guildcard);
    PRINT_HEX_LOG(ERR_LOG, pkt, len);
#endif // DEBUG

    switch (invite_cmd)
    {
        /* 公会邀请起始 检测双方的 公会情况 */
    case 0x00:
        if (dest->bb_guild->data.guild_id != 0) {
            dest->guild_accept = false;
            DBG_LOG("被邀请方 GUILD ID %u", dest->bb_guild->data.guild_id);
            /* 到这就没了, 获取对方已经属于某个公会. */
            send_msg(src, TEXT_MSG_TYPE, "%s\n\n%s", __(src, "\tE\tC4无法邀请玩家!"),
                __(src, "\tC7对方已在公会中."));
        }
        else
            dest->guild_accept = true;

        break;

        /* 未知功能 */
    case 0x01:
    default:
        ERR_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ", type, invite_cmd, src->guildcard, dest->guildcard, target_guildcard);
        PRINT_HEX_LOG(ERR_LOG, pkt, len);
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
    src->guild_accept = false;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_guild_invite_t), 0x17))
        return -2;

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gb18030, inviter_name_text, &pkt->inviter_name[2], 24, 12);

#ifdef DEBUG
    TEST_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ",
        type, invite_cmd, src->guildcard, d->guildcard, target_guildcard);
    PRINT_HEX_LOG(ERR_LOG, pkt, len);
#endif // DEBUG

    switch (invite_cmd)
    {
        /* 公会邀请成功 检测对方的 公会情况 */
    case 0x02:
        if (src->guildcard == target_guildcard)
            if (src->bb_guild->data.guild_id != 0) {
                DBG_LOG("被邀请方 GUILD ID %u", dest->bb_guild->data.guild_id);
                /* 到这就没了, 获取对方已经属于某个公会. */
                send_msg(src, TEXT_MSG_TYPE, "%s\n\n%s", __(src, "\tE\tC4无法邀请玩家!"),
                    __(src, "\tC7对方已在公会中."));
            }
            else
                src->guild_accept = true;

        break;

        /* 对方拒绝加入公会 */
    case 0x03:
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s",
            __(dest, "\tE\tC4对方拒绝加入公会."), inviter_name_text, guild_name_text);
        break;

        /* 公会邀请失败 给双方返回错误信息 */
    case 0x04:
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s",
            __(dest, "\tE\tC4公会邀请失败."), inviter_name_text, guild_name_text);
        break;

    default:
        ERR_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ",
            type, invite_cmd, src->guildcard, dest->guildcard, target_guildcard);
        PRINT_HEX_LOG(ERR_LOG, pkt, len);
        break;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_C9_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_quest_reward_meseta_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int meseta_amount = pkt->amount;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_quest_reward_meseta_t), 0x02))
        return -2;

    psocn_bb_char_t* character = get_client_char_bb(src);

    if (meseta_amount < 0) {
        meseta_amount = -meseta_amount;

        if (remove_character_meseta(character, meseta_amount, src->version != CLIENT_VERSION_BB)) {
            ERR_LOG("玩家拥有的美赛塔不足 %d < %d", character->disp.meseta, meseta_amount);
            return -1;
        }
    }
    else {
        item_t item;
        clear_inv_item(&item);
        item.datab[0] = ITEM_TYPE_MESETA;
        item.data2l = meseta_amount;
        item.item_id = generate_item_id(l, 0xFF);

        if (!add_invitem(src, item)) {
            ERR_LOG("%s 背包空间不足, 无法获得物品!",
                get_player_describe(src));
            return -1;
        }

        return subcmd_send_lobby_bb_create_inv_item(src, item, meseta_amount, true);
    }

    return 0;
}

int sub62_CA_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_quest_reward_item_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_quest_reward_item_t), 0x06)) {
        return -2;
    }

    //( 00000000 )   20 00 62 00 00 00 00 00   CA 06 FF FF 03 03 00 00   .b.....?....
    //( 00000010 )   00 00 00 00 00 00 00 00   13 00 01 00 00 00 00 00  ................
    //( 00000000 )   20 00 62 00 00 00 00 00   CA 06 FF FF 03 03 00 00   .b.....?....
    //( 00000010 )   00 00 00 00 00 00 00 00   14 00 01 00 00 00 00 00  ................
    //( 00000000 )   20 00 62 00 00 00 00 00   CA 06 FF FF 03 03 00 00   .b.....?....
    //( 00000010 )   00 00 00 00 00 00 00 00   15 00 01 00 00 00 00 00  ................
    //( 00000000 )   20 00 62 00 00 00 00 00   CA 06 FF FF 03 03 00 00   .b.....?....
    //( 00000010 )   00 00 00 00 00 00 00 00   16 00 01 00 00 00 00 00  ................
    //( 00000000 )   20 00 62 00 00 00 00 00   CA 06 FF FF 03 03 00 00   .b.....?....
    //( 00000010 )   00 00 00 00 00 00 00 00   17 00 01 00 00 00 00 00  ................
    item_t item;

    clear_inv_item(&item);

    item = pkt->item_data;
    item.item_id = generate_item_id(l, 0xFF);

    if (!add_invitem(src, item)) {
        ERR_LOG("%s 背包空间不足, 无法获得物品!",
            get_player_describe(src));
        return -1;
    }

    return subcmd_send_lobby_bb_create_inv_item(src, item, stack_size(&item), true);
}

int sub62_CD_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_guild_master_trans1_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t trans_cmd = pkt->trans_cmd;
    uint32_t client_id_sender = pkt->client_id_sender;
    char guild_name_text[24];
    char master_name_text[24];

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_guild_master_trans1_t), 0x17)) {
        return -2;
    }

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gb18030, master_name_text, &pkt->master_name[2], 24, 12);

    //TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS1 0x%02X 0x%08X c %u d %u", type, trans_cmd, src->guildcard, dest->guildcard);
    //PRINT_HEX_LOG(ERR_LOG, pkt, len);

    if (src->bb_guild->data.guild_priv_level != BB_GUILD_PRIV_LEVEL_MASTER) {
        ERR_LOG("GC %u 公会权限不足", get_player_describe(src));
        send_msg(src, TEXT_MSG_TYPE, "%s\n\n%s", __(src, "\tE\tC4公会权限不足!"),
            __(src, "\tC7您无权进行此操作."));
        return -1;
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
    dest->guild_master_exfer = false;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_guild_master_trans2_t), 0x17)) {
        return -2;
    }

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gb18030, master_name_text, &pkt->master_name[2], 24, 12);

    //TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS2 0x%02X 0x%08X c %u d %u", type, trans_cmd, src->guildcard, dest->guildcard);
    //PRINT_HEX_LOG(ERR_LOG, pkt, len);

    switch (trans_cmd)
    {
    case 0x01:
        dest->guild_master_exfer = true;
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(dest, "\tE\tC4会长变更中."), master_name_text, guild_name_text);
        break;

        /* 公会转移成功 TODO 通知其他会员 */
    case 0x02:
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(dest, "\tE\tC4会长已变更."), master_name_text, guild_name_text);
        break;

        /* 对方拒绝成为会长 */
    case 0x03:
        send_msg(dest, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(dest, "\tE\tC4对方拒绝成为会长."), master_name_text, guild_name_text);
        break;

        /* 公会转移失败 给双方返回错误信息 */
    case 0x04:
        send_msg(src, TEXT_MSG_TYPE, "%s\n\tC6邀请人:%s\n\tC8公会名称:%s", __(src, "\tE\tC4公会转移失败."), master_name_text, guild_name_text);
        break;

    default:
        ERR_LOG("SUBCMD62_GUILD_MASTER_TRANS 0x%02X 0x%08X c %u d %u", type, trans_cmd, src->guildcard, dest->guildcard);
        break;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_D0_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_battle_mode_level_up_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if ((l->battle) && (l->flags && LOBBY_FLAG_QUESTING))
    {
        if ((pkt->shdr.client_id < 4) && (l->clients[pkt->shdr.client_id]))
        {
            uint16_t target_lv;

            ship_client_t* lClient = l->clients[pkt->shdr.client_id];
            target_lv = lClient->mode_pl->bb.disp.level;
            target_lv += pkt->num_levels;

            if (target_lv > 199)
                target_lv = 199;

            client_give_level(lClient, target_lv);
        }
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_D1_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_c_mode_grave_drop_req_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    float x = pkt->x, z = pkt->z;

    if (!in_game(src))
        return -1;

    PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);

    /* Make sure there's something set with /item */
    if (!pkt->drop_amount) {
        return send_txt(src, "%s", __(src, "\tE\tC7墓碑掉落无效."));
    }

    /* TODO 错了 是掉落和玩家背包中ID一致的物品 */

    //[2023年07月31日 04:42 : 58 : 142] 调试(subcmd_handle_62.c 2515) : 未知 0x62 指令 : 0xD1
    //(00000000)   1C 00 62 00 00 00 00 00   D1 05 00 00 01 00 05 00  ..b..... ? ......
    //(00000010)   B0 FF 07 C4 99 E8 3E C2   00 00 00 00 ? .臋 ? ? ...
//[2023年10月21日 02:10:59:946] 错误(f_logs.h 0587): 数据包如下:
//(00000000) 1C 00 62 00 00 00 00 00  D1 05 00 00 01 00 05 00    ..b.............
//(00000010) 77 B7 09 C4 D2 1E 66 C3  03 00 00 00    w.....f.....
    item_t drop_item = { 0 };
    drop_item.datab[0] = pkt->item[0];
    drop_item.datab[1] = pkt->item_subtype;
    drop_item.datab[5] = pkt->drop_amount;

    litem_t* new_litem = add_lobby_litem_locked(l, &drop_item, src->cur_area, x, z, true);

    if (new_litem == NULL) {
        return send_txt(src, "%s", __(src, "\tE\tC7新物品空间不足或生成失败."));
    }

    return subcmd_send_lobby_drop_stack_bb(src, 0xFBFF, NULL, new_litem);
}

int sub62_D6_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_warp_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    item_t item_data = pkt->item_data;

    if (!in_game(src))
        return -1;

    item_t backup_item = remove_invitem(src, item_data.item_id, 1, src->version != CLIENT_VERSION_BB);
    if (item_not_identification_bb(backup_item.datal[0],backup_item.datal[1])) {
        ERR_LOG("%s 转换物品ID %d 失败!", get_player_describe(src), item_data.item_id);
        return -1;
    }

    wrap(&backup_item);

    /* 将物品新增至背包. */
    if (!add_invitem(src, backup_item)) {
        return send_txt(src, __(src, "\tE\tC4转换物品失败"));
    }

    subcmd_send_lobby_bb_create_inv_item(src, backup_item, stack_size(&backup_item), true);

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_DF_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_black_paper_deal_photon_drop_exchange_t* pkt) {
    lobby_t* l = src->cur_lobby;

//[2023年10月16日 13:27:38:107] 调试(f_logs.h 0612): 数据包如下:
//(00000000) 0C 00 62 00 00 00 00 00  DF 01 00 00    ..b.........

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_black_paper_deal_photon_drop_exchange_t), 0x01)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("%s ID不一致, 错误的黑页扣除PD", get_player_describe(src));
        return -3;
    }

    l->drops_disabled = false;
    l->questE0_done = false;

    if ((!l->oneperson) && !(l->flags & LOBBY_FLAG_QUESTING) && (l->drops_disabled)) {
        return -3;
    }

    inventory_t* inv = get_client_inv(src);
    if (!inv) {
        ERR_LOG("获取角色背包失败");
        return -3;
    }

    size_t item_id = find_iitem_code_stack_item_id(inv, BBItem_Photon_Crystal);
    /* 如果找不到该物品，则将用户从船上推下. */
    if (item_id == 0) {
        ERR_LOG("%s 没有兑换所需物品!", get_player_describe(src));
        return -4;
    }

    item_t item = remove_invitem(src, item_id, 1, src->version != CLIENT_VERSION_BB);
    if (item_not_identification_bb(item.datal[0], item.datal[1])) {
        ERR_LOG("0x%08X 是未识别物品", item_id);
        return -5;
    }

    l->drops_disabled = true;

    return subcmd_send_lobby_bb_destroy_item(src, item_id, 1);
}

int sub62_E0_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_black_paper_deal_reward_t* pkt) {
    lobby_t* l = src->cur_lobby;
    sfmt_t* rng = &l->block->sfmt_rng;
    uint8_t area = pkt->area, bp_type = pkt->bp_type;
    float x = pkt->drop_x, z = pkt->drop_z;
    bp_reward_list_t* bp_reward_list = NULL;
    size_t reward_list_count = 0;
    uint8_t tmp_value = 0;
    errno_t err = 0;

    //普通难度 黑页2
//[2023年10月15日 21:55:38:535] 舰船服务器 错误(f_logs.h 0599): 数据包如下:
//(00000000) 18 00 62 00 00 00 00 00  E0 04 00 00 08 04 15 00    ..b.............
//(00000010) 00 60 60 45 00 00 CD C3     .``E....
//
//[2023年10月15日 22:05:00:586] 舰船服务器 错误(f_logs.h 0599): 数据包如下:
//(00000000) 18 00 62 00 00 00 00 00  E0 04 00 00 08 04 15 00    ..b.............
//(00000010) 00 60 60 45 00 00 CD C3     .``E....
// 
//[2023年10月16日 11:17:45:150] 调试(f_logs.h 0612): 数据包如下:
//(00000000) 18 00 62 00 00 00 00 00  E0 04 00 00 08 04 15 00    ..b.............
//(00000010) 00 60 60 45 00 00 CD C3     .``E....
// 
//[2023年10月16日 13:36:36:942] 调试(f_logs.h 0612): 数据包如下:
//(00000000) 18 00 62 00 00 00 00 00  E0 04 00 00 08 04 15 00    ..b.............
//(00000010) 00 60 60 45 00 00 CD C3     .``E....
//     //极限难度 黑页2
//[2023年10月16日 15:13:46:066] 调试(f_logs.h 0612): 数据包如下:
//(00000000) 18 00 62 00 00 00 00 00  E0 04 00 00 08 04 15 00    ..b.............
//(00000010) 00 60 60 45 00 00 CD C3     .``E....
//[2023年10月16日 15:31:00:903] 调试(f_logs.h 0612): 数据包如下:
//(00000000) 18 00 62 00 00 00 00 00  E0 04 00 00 08 04 15 00    ..b.............
//(00000010) 00 60 60 45 00 00 CD C3     .``E....


    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_black_paper_deal_reward_t), 0x04)) {
        return -2;
    }

    if ((!l->oneperson) && (!l->flags & LOBBY_FLAG_QUESTING) && (!l->drops_disabled) && (l->questE0_done)) {
        ERR_LOG("%s 黑页掉落失败", get_player_describe(src));
        return 0;
    }

    for (int i = 0; i < (bp_type > 0x03 ? (l->difficulty <= 0x01 ? 1 : 2) : l->difficulty + 1); i++) {
        uint32_t reward_item = 0;

        switch (bp_type) {
        case BP_REWARD_BP1_DORPHON:
            bp_reward_list = bp1_dorphon[l->difficulty];
            reward_list_count = count_element_int((void**)bp1_dorphon[l->difficulty]);
            break;

        case BP_REWARD_BP1_RAPPY:
            bp_reward_list = bp1_rappy[l->difficulty];
            reward_list_count = count_element_int((void**)bp1_rappy[l->difficulty]);
            break;

        case BP_REWARD_BP1_ZU:
            bp_reward_list = bp1_zu[l->difficulty];
            reward_list_count = count_element_int((void**)bp1_zu[l->difficulty]);
            break;

        case BP_REWARD_BP2:
            bp_reward_list = bp2[l->difficulty];
            reward_list_count = count_element_int((void**)bp2[l->difficulty]);
            break;
        }

        reward_item = bp_reward_list[(sfmt_genrand_uint32(rng) % reward_list_count)].reward;

        if (reward_item == 0)
            reward_item = BBItem_Meseta;

        l->questE0_done = true;

        item_t item = { 0 };

        item.datal[0] = reward_item;

        /* 填充物品数据 */
        if (item.datal[0] == BBItem_Meseta) {
            pt_bb_entry_t* ent = get_pt_data_bb(l->episode, l->challenge, l->difficulty, src->bb_pl->character.dress_data.section);
            if (!ent) {
                ERR_LOG("%s Item_PT 不存在难度 %d 颜色 %d 的掉落", client_type[src->version].ver_name, l->difficulty, src->bb_pl->character.dress_data.section);
                return 0;
            }
            item.data2l = get_random_value(rng, ent->enemy_meseta_ranges[0x2E]) + (sfmt_genrand_uint32(rng) % 100);
        }
        else {
            /* 检索物品类型 */
            switch (item.datab[0]) {
            case ITEM_TYPE_WEAPON: // 武器
                /* 打磨值 0 - 10*/
                item.datab[3] = sfmt_genrand_uint32(rng) % 11;
                /* 特殊攻击 0 - 10 配合难度 0 - 3 33%几率 获得特殊EX */
                if ((sfmt_genrand_uint32(rng) % 3) == 1) {
                    item.datab[4] = sfmt_genrand_uint32(rng) % 11 + l->difficulty;
                }
                /* datab[5] 在这里不涉及 礼物 未鉴定*/

                /* 如果掉落的是武器 则未鉴定 */
                item.datab[4] |= 0x80;

                /* 生成属性*/
                size_t num_percentages = 0;
                while (num_percentages < 3) {
                    /*0-5 涵盖所有属性*/
                    for (size_t x = 0; x < 6; x++) {
                        /* 后期设置调整 生成属性几率 TODO */
                        if ((sfmt_genrand_uint32(rng) % 6) == 1) {
                            /*+6 对应属性槽（结果分别为 6 8 10） +7对应数值（结果分别为 随机数1-20 1-35 1-45 1-50）*/
                            item.datab[(num_percentages * 2) + 6] = (uint8_t)x;
                            tmp_value = /*sfmt_genrand_uint32(rng) % 6 + */weapon_bonus_values[sfmt_genrand_uint32(rng) % 21];/* 0 - 5 % 0 - 19*/

                            //if (tmp_value > 50)
                            //    tmp_value = 50;

                            //if (tmp_value < -50)
                            //    tmp_value = -50;

                            item.datab[(num_percentages * 2) + 7] = /*(sfmt_genrand_uint32(rng) % 50 + 1 ) +*/ tmp_value;
                            num_percentages++;
                        }
                    }
                }

                break;

            case ITEM_TYPE_GUARD: // 装甲
                pmt_guard_bb_t pmt_guard = { 0 };
                pmt_unit_bb_t pmt_unit = { 0 };

                switch (item.datab[1]) {
                case ITEM_SUBTYPE_FRAME://护甲
                    if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
                        ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                        return -1;
                    }

                    /*随机槽位 0 - 4 33几率新增槽位 */
                    if ((sfmt_genrand_uint32(rng) % 3) == 1)
                        item.datab[5] = sfmt_genrand_uint32(rng) % 4 + 1;

                    /* DFP值 */
                    if (pmt_guard.dfp_range) {
                        tmp_value = sfmt_genrand_uint32(rng) % (pmt_guard.dfp_range + 1);
                        if (tmp_value < 0)
                            tmp_value = 0;
                        item.datab[6] = tmp_value;
                    }

                    /* EVP值 */
                    if (pmt_guard.evp_range) {
                        tmp_value = sfmt_genrand_uint32(rng) % (pmt_guard.evp_range + 1);
                        if (tmp_value < 0)
                            tmp_value = 0;
                        item.datab[8] = tmp_value;
                    }
                    break;

                case ITEM_SUBTYPE_BARRIER://护盾
                    if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
                        ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                        return -1;
                    }

                    /* DFP值 */
                    if (pmt_guard.dfp_range) {
                        tmp_value = sfmt_genrand_uint32(rng) % (pmt_guard.dfp_range + 1);
                        if (tmp_value < 0)
                            tmp_value = 0;
                        item.datab[6] = tmp_value;
                    }

                    /* EVP值 */
                    if (pmt_guard.evp_range) {
                        tmp_value = sfmt_genrand_uint32(rng) % (pmt_guard.evp_range + 1);
                        if (tmp_value < 0)
                            tmp_value = 0;
                        item.datab[8] = tmp_value;
                    }
                    break;

                case ITEM_SUBTYPE_UNIT://插件
                    if (err = pmt_lookup_unit_bb(item.datal[0], &pmt_unit)) {
                        ERR_LOG("pmt_lookup_unit_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                        UNLOCK_CMUTEX(src);
                        return -1;
                    }

                    tmp_value = sfmt_genrand_uint32(rng) % 5;
                    item.datab[6] = unit_bonus_values[tmp_value][0];
                    item.datab[7] = unit_bonus_values[tmp_value][1];

                    break;
                }
                break;

            case ITEM_TYPE_MAG:
                magitemstat_t stats;
                ItemMagStats_init(&stats, get_player_msg_color_set(src));
                assign_mag_stats(&item, &stats);
                break;

            case ITEM_TYPE_TOOL: // 药品工具
                item.datab[5] = get_item_amount(&item, 1);
                break;
            }
        }

        litem_t* litem = add_lobby_litem_locked(l, &item, area, x, z, true);
        if (!litem) {
            return send_txt(src, "%s", __(src, "\tE\tC7新物品空间不足."));
        }
        subcmd_send_drop_stack_bb(src, 0xFBFF, litem);
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_E2_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_coren_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    sfmt_t* rng = &src->sfmt_rng;
    item_t result_item = { 0 };
    uint32_t menu_choice = pkt->menu_choice, reward_percent[3] = { 0 };
    Coren_Reward_List_t reward_list = { 0 };
    uint8_t tmp_value = 0;
    item_t item = { 0 };
    errno_t err = 0;
    uint8_t 难度 = l->difficulty, 章节 = l->episode, 挑战 = l->challenge;
    int rv = 0;

    static const uint8_t max_quantity[4] = { 1,  1,  1,  1 };
    static const uint8_t max_tech_lvl[4] = { 4,  7, 10, 15 };
    static const uint8_t max_anti_lvl[4] = { 2,  4,  6,  7 };

    if (!in_game(src)) {
        return -1;
    }

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_coren_act_t), 0x04)) {
        return -2;
    }

    //PRINT_HEX_LOG(DBG_LOG, pkt, pkt->hdr.pkt_len);

    psocn_bb_char_t* character = get_client_char_bb(src);

    // 获取当前系统时间
    SYSTEMTIME time;
    GetLocalTime(&time);

    // 获取当前是星期几（星期天 = 0, 星期一 = 1, 星期二 = 2, ..., 星期六 = 6）
    reward_list.wday = time.wDayOfWeek/*sfmt_genrand_uint32(&src->sfmt_rng) % 7*/;

#ifdef DEBUG

    // 将数字转换为对应的星期几文本
    const char* weekDays[] = { "星期天", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };
    const char* currentDayOfWeek = weekDays[reward_list.wday];

    // 打印当前是星期几
    DBG_LOG("今天是 %s.", currentDayOfWeek);

#endif // DEBUG

    // 将数字转换为对应的星期几文本
    const char* weekDays[] = { "星期天", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };
    const char* currentDayOfWeek = weekDays[reward_list.wday];

    reward_percent[0] = weekly_reward_percent[menu_choice][reward_list.wday][0];
    reward_percent[1] = weekly_reward_percent[menu_choice][reward_list.wday][1];
    reward_percent[2] = weekly_reward_percent[menu_choice][reward_list.wday][2];
    reward_list.rewards = day_reward_list[reward_list.wday][menu_choice];

    clear_inv_item(&item);

    /* 必须获取 1-100 大于0的数 这样就不会出现0这个数字了*/
    if (src->game_data->gm_debug) {
        result_item.datal[0] = reward_list.rewards[lottery_num(rng, TOTAL_COREN_ITEMS)];
    }
    else {
        uint32_t rng_value = sfmt_genrand_uint32(rng) % 100 + 1;
        size_t tmp_choice = menu_choice + 1;
        if (tmp_choice > 3)
            tmp_choice = 3;

        for (size_t i = 0; i < tmp_choice; i++) {
            if ((rng_value <= reward_percent[i]) && (reward_percent[i] != 0)) {
                result_item.datal[0] = reward_list.rewards[lottery_num(rng, TOTAL_COREN_ITEMS)];
                break;
            }
        }

        if (is_item_empty(&result_item)) {

#ifdef DEBUG
            DBG_LOG("美赛塔奖励 menu_choice 0x%08X rng_value %d", menu_choice, rng_value);

            PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.pkt_len);
#endif // DEBUG

            uint32_t amount = (sfmt_genrand_uint32(rng) % menu_choice_price[menu_choice] + 1) / 2;

            result_item.datal[0] = BBItem_Meseta;
            get_item_amount(&result_item, amount);

            item = result_item;

            if (!add_invitem(src, item)) {
                ERR_LOG("%s 背包空间不足, 无法获得物品!",
                    get_player_describe(src));
                return -1;
            }

            rv |= subcmd_send_bb_create_inv_item(src, item, amount);

            //rv |= send_msg(src, BB_SCROLL_MSG_TYPE, "[%s轮盘赌]:\tE\tC4 %d \tE\tC7美赛塔档次\n很抱歉 %s 本次未获得物品奖励\n安慰奖: %d 美赛塔.",
            //    currentDayOfWeek,
            //    menu_choice_price[menu_choice],
            //    get_player_name(src->pl, src->version, false),
            //    amount
            //);

            rv |= subcmd_bb_send_coren_reward(src, 1, result_item);
            return rv;
        }

    }

    item.datal[0] = result_item.datal[0];

    /* 填充物品数据 */
    if (item.datal[0] == BBItem_Meseta) {
        item.item_id = 0xFFFFFFFF;
        get_item_amount(&item, menu_choice_price[menu_choice] * (sfmt_genrand_uint32(rng) % 2 + 1));
    }
    else {
        /* 检索物品类型 */
        switch (item.datab[0]) {
        case ITEM_TYPE_WEAPON: // 武器
            pmt_weapon_bb_t pmt_weapon = { 0 };

            if (err = pmt_lookup_weapon_bb(item.datal[0], &pmt_weapon)) {
                ERR_LOG("pmt_lookup_weapon_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                return -1;
            }

            /* 打磨值 0 - 10*/
            if (pmt_weapon.max_grind) {
                item.datab[3] = sfmt_genrand_uint32(rng) % pmt_weapon.max_grind;
            }

            /* 特殊攻击 0 - 10 配合难度 0 - 3 33%几率 获得特殊EX */
            if (!pmt_weapon.special_type) {
                if ((sfmt_genrand_uint32(rng) % 3) == 1) {
                    item.datab[4] = sfmt_genrand_uint32(rng) % 11 + 难度;
                }
            }

            /* datab[5] 在这里不涉及 礼物 未鉴定*/

            /* 生成属性*/
            size_t attrib_slot = 0;
            while (attrib_slot < 3) {
                /*0-5 涵盖所有属性*/
                for (size_t x = 0; x < 6; x++) {
                    /* 后期设置调整 生成属性几率 TODO */
                    if ((sfmt_genrand_uint32(rng) % 6) == 1) {
                        /*+6 对应属性槽（结果分别为 6 8 10） +7对应数值（结果分别为 随机数1-20 1-35 1-45 1-50）*/
                        item.datab[(attrib_slot * 2) + 6] = (uint8_t)x;
                        tmp_value = /*sfmt_genrand_uint32(rng) % 6 + */weapon_bonus_values[sfmt_genrand_uint32(rng) % 21];/* 0 - 5 % 0 - 19*/

                        //if (tmp_value > 50)
                        //    tmp_value = 50;

                        //if (tmp_value < -50)
                        //    tmp_value = -50;

                        item.datab[(attrib_slot * 2) + 7] = /*(sfmt_genrand_uint32(rng) % 50 + 1 ) +*/ tmp_value;
                        attrib_slot++;
                    }
                }
            }

            break;

        case ITEM_TYPE_GUARD: // 装甲
            pmt_guard_bb_t pmt_guard = { 0 };
            pmt_unit_bb_t pmt_unit = { 0 };

            switch (item.datab[1]) {
            case ITEM_SUBTYPE_FRAME://护甲
                if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
                    ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                    return -1;
                }

                /*随机槽位 0 - 4 33几率新增槽位 */
                if ((sfmt_genrand_uint32(rng) % 3) == 1)
                    item.datab[5] = sfmt_genrand_uint32(rng) % 4 + 1;

                /* DFP值 */
                if (pmt_guard.dfp_range) {
                    tmp_value = sfmt_genrand_uint32(rng) % (pmt_guard.dfp_range + 1);
                    if (tmp_value < 0)
                        tmp_value = 0;
                    item.datab[6] = tmp_value;
                }

                /* EVP值 */
                if (pmt_guard.evp_range) {
                    tmp_value = sfmt_genrand_uint32(rng) % (pmt_guard.evp_range + 1);
                    if (tmp_value < 0)
                        tmp_value = 0;
                    item.datab[8] = tmp_value;
                }
                break;

            case ITEM_SUBTYPE_BARRIER://护盾
                if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
                    ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                    return -1;
                }

                /* DFP值 */
                if (pmt_guard.dfp_range) {
                    tmp_value = sfmt_genrand_uint32(rng) % (pmt_guard.dfp_range + 1);
                    if (tmp_value < 0)
                        tmp_value = 0;
                    item.datab[6] = tmp_value;
                }

                /* EVP值 */
                if (pmt_guard.evp_range) {
                    tmp_value = sfmt_genrand_uint32(rng) % (pmt_guard.evp_range + 1);
                    if (tmp_value < 0)
                        tmp_value = 0;
                    item.datab[8] = tmp_value;
                }
                break;

            case ITEM_SUBTYPE_UNIT://插件
                if (err = pmt_lookup_unit_bb(item.datal[0], &pmt_unit)) {
                    ERR_LOG("pmt_lookup_unit_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                    UNLOCK_CMUTEX(src);
                    return -1;
                }

                tmp_value = sfmt_genrand_uint32(rng) % 5;
                item.datab[6] = unit_bonus_values[tmp_value][0];
                item.datab[7] = unit_bonus_values[tmp_value][1];

                break;
            }
            break;

        case ITEM_TYPE_MAG:
            magitemstat_t stats;
            ItemMagStats_init(&stats, get_player_msg_color_set(src));
            assign_mag_stats(&item, &stats);
            if (!check_mag_has_pb(&item)) {
                uint32_t rng = rand_int(&src->sfmt_rng, _countof(smrpb));
                item.datab[3] = smrpb[rng].datab3;
                item.data2b[2] = smrpb[rng].data2b2;
                item.data2b[3] = get_player_msg_color_set(src);
            }
            break;

        case ITEM_TYPE_TOOL: // 药品工具
            item.datab[5] = get_item_amount(&item, 1);
            break;
        }

        item.item_id = generate_item_id(l, src->client_id);
    }

    //print_item_data(&iitem.data, src->version);

    if (!add_invitem(src, item)) {
        ERR_LOG("%s 背包空间不足, 无法获得物品!",
            get_player_describe(src));
        return -1;
    }

    rv |= subcmd_send_bb_create_inv_item(src, item, 1);

    //rv |= send_msg(src, BB_SCROLL_MSG_TYPE, "[%s轮盘赌]:\tE\tC4 %d \tE\tC7美赛塔档次\n恭喜 %s 抽奖获得了\n\tE\tC6%s.",
    //    currentDayOfWeek,
    //    menu_choice_price[menu_choice],
    //    get_player_name(src->pl, src->version, false),
    //    get_item_describe(&iitem.data, src->version)
    //);

    if (is_item_rare(&item))
        rv |= announce_message(src, false, BB_SCROLL_MSG_TYPE, "[%s轮盘赌]: "
            "恭喜 %s 在 \tE\tC4%d \tE\tC7美赛塔档次抽奖获得了 "
            "\tE\tC6%s.",
            currentDayOfWeek,
            get_player_name(src->pl, src->version, false),
            menu_choice_price[menu_choice],
            get_item_describe(&item, src->version)
        );

    rv |= subcmd_bb_send_coren_reward(src, 1, result_item);

    return rv;
}

// 定义函数指针数组
subcmd_handle_func_t subcmd62_handler[] = {
    //    cmd_type                         DC           GC           EP3          XBOX         PC           BB
    { SUBCMD62_GUILDCARD                 , sub62_06_dc, sub62_06_gc, NULL,        sub62_06_xb, sub62_06_pc, sub62_06_bb },
    { SUBCMD62_PICK_UP                   , sub62_5A_dc, sub62_5A_dc, sub62_5A_dc, sub62_5A_dc, sub62_5A_dc, sub62_5A_bb },
    { SUBCMD62_ITEM_DROP_REQ             , sub62_60_dc, sub62_60_dc, sub62_60_dc, sub62_60_dc, sub62_60_dc, sub62_60_bb },
    { SUBCMD62_BURST5                    , sub62_6F_dc, sub62_6F_dc, sub62_6F_dc, sub62_6F_dc, sub62_6F_dc, sub62_6F_bb },
    { SUBCMD62_BURST6                    , sub62_71_dc, sub62_71_dc, sub62_71_dc, sub62_71_dc, sub62_71_dc, sub62_71_bb },
    { SUBCMD62_ITEM_BOXDROP_REQ          , sub62_A2_dc, sub62_A2_dc, sub62_A2_dc, sub62_A2_dc, sub62_A2_dc, sub62_A2_bb },
    { SUBCMD62_TRADE                     , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_A6_bb },
    { SUBCMD62_CHAIR_STATE               , sub62_AE_dc, sub62_AE_dc, sub62_AE_dc, sub62_AE_dc, sub62_AE_dc, sub62_AE_bb },
    { SUBCMD62_SHOP_REQ                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B5_bb },
    { SUBCMD62_SHOP_BUY                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B7_bb },
    { SUBCMD62_TEKKING                   , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B8_bb },
    { SUBCMD62_TEKKED                    , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_BA_bb },
    { SUBCMD62_BANK_OPEN                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_BB_bb },
    { SUBCMD62_BANK_ACT                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_BD_bb },
    { SUBCMD62_GUILD_INVITE1             , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_C1_bb },
    { SUBCMD62_GUILD_INVITE2             , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_C2_bb },
    { SUBCMD62_QUEST_REWARD_MESETA       , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_C9_bb },
    { SUBCMD62_ITEM_QUEST_REWARD         , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_CA_bb },
    { SUBCMD62_GUILD_MASTER_TRANS1       , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_CD_bb },
    { SUBCMD62_GUILD_MASTER_TRANS2       , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_CE_bb },
    { SUBCMD62_BATTLE_CHAR_LEVEL_FIX     , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_D0_bb },
    { SUBCMD62_CH_GRAVE_DROP_REQ         , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_D1_bb },
    { SUBCMD62_ITEM_WARP                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_D6_bb },
    { SUBCMD62_QUEST_BP_PHOTON_EX        , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_DF_bb },
    { SUBCMD62_QUEST_BP_REWARD           , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_E0_bb },
    { SUBCMD62_COREN_ACT                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_E2_bb },
};

/* 处理 DC GC PC V1 V2 0x62 来自客户端的数据包. */
int subcmd_handle_62(ship_client_t* src, subcmd_pkt_t* pkt) {
    __try {
        lobby_t* l = src->cur_lobby;
        ship_client_t* dest;
        uint16_t len = pkt->hdr.dc.pkt_len, hdr_type = pkt->hdr.dc.pkt_type;
        uint8_t type = pkt->type;
        int rv = -1;

        /* The DC NTE must be treated specially, so deal with that elsewhere... */
        if (src->version == CLIENT_VERSION_DCV1 && (src->flags & CLIENT_FLAG_IS_NTE))
            return subcmd_dcnte_handle_one(src, pkt);

        /* 如果客户端不在大厅或者队伍中则忽略数据包. */
        if (!l)
            return 0;

        pthread_mutex_lock(&l->mutex);

        /* 搜索目标客户端. */
        dest = l->clients[pkt->hdr.dc.flags];

        /* 目标客户端已离线，将不再发送数据包. */
        if (!dest) {
            pthread_mutex_unlock(&l->mutex);
            return 0;
        }

        l->subcmd_handle = subcmd_get_handler(hdr_type, type, src->version);
        if (!l->subcmd_handle) {

#ifdef BB_LOG_UNKNOWN_SUBS
            DBG_LOG("未知 0x%02X 指令: 0x%02X", hdr_type, type);
            PRINT_HEX_LOG(ERR_LOG, pkt, len);
#endif /* BB_LOG_UNKNOWN_SUBS */

            rv = send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
            pthread_mutex_unlock(&l->mutex);
            return rv;
        }

        /* If there's a burst going on in the lobby, delay most packets */
        if (l->flags & LOBBY_FLAG_BURSTING) {
            rv = 0;

            switch (type) {
            case SUBCMD62_BURST5:
            case SUBCMD62_BURST6:
                rv |= l->subcmd_handle(src, dest, pkt);
                break;

            default:
                rv = lobby_enqueue_pkt(l, src, (dc_pkt_hdr_t*)pkt);
            }
        } else {
            rv = l->subcmd_handle(src, dest, pkt);
        }


        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

/* 处理BB 0x62 数据包. */
int subcmd_bb_handle_62(ship_client_t* src, subcmd_bb_pkt_t* pkt) {
    __try {
        lobby_t* l = src->cur_lobby;
        ship_client_t* dest;
        uint16_t len = pkt->hdr.pkt_len;
        uint16_t hdr_type = pkt->hdr.pkt_type;
        uint8_t type = pkt->type;
        uint8_t size = pkt->size;
        int rv = -1;
        uint32_t dnum = LE32(pkt->hdr.flags);

        /* 如果客户端不在大厅或者队伍中则忽略数据包. */
        if (!l)
            return 0;

        pthread_mutex_lock(&l->mutex);

        /* 搜索目标客户端. */
        dest = l->clients[dnum];

        /* 目标客户端已离线，将不再发送数据包. */
        if (!dest) {
            pthread_mutex_unlock(&l->mutex);
            return 0;
        }

#ifdef DEBUG

        DBG_LOG("src %d GC%u dest %d GC%u 0x%02X 指令: 0x%02X", c->client_id, c->guildcard, dnum, l->clients[dnum]->guildcard, hdr_type, type);

#endif // DEBUG

        l->subcmd_handle = subcmd_get_handler(hdr_type, type, src->version);
        if (!l->subcmd_handle) {

#ifdef BB_LOG_UNKNOWN_SUBS
            DBG_LOG("未知 0x%02X 指令: 0x%02X", hdr_type, type);
            PRINT_HEX_LOG(ERR_LOG, pkt, len);
#endif /* BB_LOG_UNKNOWN_SUBS */

            rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            pthread_mutex_unlock(&l->mutex);
            return rv;
        }

        /* If there's a burst going on in the lobby, delay most packets */
        if (l->flags & LOBBY_FLAG_BURSTING) {
            rv = 0;

            switch (type) {
            case SUBCMD62_BURST5://0x62 6F //其他大厅跃迁进房时触发 5
            case SUBCMD62_BURST6://0x62 71 //其他大厅跃迁进房时触发 6
                rv |= l->subcmd_handle(src, dest, pkt);
                break;

            default:
                rv = lobby_enqueue_pkt_bb(l, src, (bb_pkt_hdr_t*)pkt);
            }
        } else {
            rv = l->subcmd_handle(src, dest, pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}
