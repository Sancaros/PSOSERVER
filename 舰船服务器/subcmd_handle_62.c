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
#include <mtwist.h>
#include <items.h>

#include "subcmd.h"
#include "subcmd_send_bb.h"
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

#include "subcmd_handle.h"

static int handle_itemreq_gm(ship_client_t* src,
    subcmd_itemreq_t* req) {
    subcmd_itemgen_t gen = { 0 };
    int r = LE16(req->request_id);
    int i;
    lobby_t* l = src->cur_lobby;

    /* 填充数据并准备发送. */
    gen.hdr.pkt_type = GAME_COMMAND0_TYPE;
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
    clear_item(&src->new_item);

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
        bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
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
        bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
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
        bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
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
        bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
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
        memcpy(pc.name, &src->pl->bb.character.name.name_tag, BB_CHARACTER_CHAR_TAG_NAME_WLENGTH);
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

int sub62_5A_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pick_up_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " picked up item in lobby!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x03)
        return -1;

    if (src->cur_area != pkt->area) {
        ERR_LOG("GC %" PRIu32 " picked up item in area they are "
            "not currently in!", src->guildcard);
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
    int pick_count;
    uint32_t item, tmp;
    iitem_t iitem_data = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中拾取了物品!",
            src->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x14) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误的拾取数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* 尝试从大厅物品栏中移除... */
    pick_count = remove_litem_locked(l, pkt->item_id, &iitem_data);
    if (pick_count < 0) {
        return -1;
    }
    else if (pick_count > 0) {
        /* 假设别人已经捡到了, 就忽略它... */
        return 0;
    }
    else {

        psocn_bb_char_t* player = &src->bb_pl->character;

        if (src->mode)
            player = &src->mode_pl->bb;

        item = LE32(iitem_data.data.datal[0]);

        /* Is it meseta, or an item? */
        if (item == Item_Meseta) {
            tmp = LE32(iitem_data.data.data2l) + LE32(player->disp.meseta);

            /* Cap at 999,999 meseta. */
            if (tmp > 999999)
                tmp = 999999;

            player->disp.meseta = LE32(tmp);

            if (!src->mode)
                src->pl->bb.character.disp.meseta = player->disp.meseta;
        }
        else {
            iitem_data.present = LE16(0x0001);
            iitem_data.extension_data1 = 0;
            iitem_data.extension_data2 = 0;
            iitem_data.flags = 0;

            //iitem_data.data.item_id = generate_item_id(l, src->client_id);
            iitem_data.data.item_id = pkt->item_id;

            /* Add the item to the client's inventory. */
            if (!add_iitem(src, &iitem_data))
                return 0;
        }
    }

    /* 让所有人都知道是该客户端捡到的，并将其从所有人视线中删除. */
    subcmd_send_lobby_bb_create_inv_item(src, iitem_data.data, 1, true);

    return subcmd_send_bb_del_map_item(src, pkt->area, iitem_data.data.item_id);
}

int sub62_60_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

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
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (src->mode) {
        send_txt(src, __(src, "\tE\tC6暂未探明挑战模式该掉什么东西."));
        return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }

    if (l->battle) {
        send_txt(src, __(src, "\tE\tC6暂未探明对战模式该掉什么东西."));
        return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }

    return l->dropfunc(src, l, pkt);
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
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    subcmd_bb_bitemreq_t* req = (subcmd_bb_bitemreq_t*)pkt;

    if (src->mode) {
        send_txt(src, __(src, "\tE\tC6暂未探明挑战模式该掉什么东西."));
        return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }

    if (l->drop_pso2) {
        l->dropfunc = pt_generate_bb_pso2_drop;
    }

    if (l->drop_psocn) {
        l->dropfunc = pt_generate_bb_pso2_drop;
    }

    return l->dropfunc(src, l, req);
}

int sub62_A6_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_trade_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = -1;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return rv;
    }

    if (pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            src->guildcard, pkt->shdr.type, pkt->shdr.size);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return rv;
    }

    send_msg(dest, BB_SCROLL_MSG_TYPE, "%s", __(dest, "\tE\tC6交易功能存在故障，暂时关闭"));
    send_msg(src, BB_SCROLL_MSG_TYPE, "%s", __(src, "\tE\tC6交易功能存在故障，暂时关闭"));

    DBG_LOG("GC %u -> %u", src->guildcard, dest->guildcard);

    display_packet(pkt, pkt->hdr.pkt_len);

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_AE_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_send_lobby_chair_state_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = -1;

    if (pkt->shdr.size != 0x04 || pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            src->guildcard, pkt->shdr.type, pkt->shdr.size);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return rv;
    }

    return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
}

int sub62_AE_pc(ship_client_t* src, ship_client_t* dest,
    subcmd_pc_send_lobby_chair_state_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = -1;

    if (pkt->shdr.size != 0x04 || pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            src->guildcard, pkt->shdr.type, pkt->shdr.size);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return rv;
    }

    return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
}

int sub62_AE_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_send_lobby_chair_state_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = -1;

    if (pkt->shdr.size != 0x04 || pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
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
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    for (uint8_t i = 0; i < num_items; ++i) {
        memset(&src->game_data->shop_items[i], 0, PSOCN_STLENGTH_ITEM);
        memset(&item_data, 0, PSOCN_STLENGTH_ITEM);

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

        item_data.item_id = generate_item_id(l, src->client_id);

        memcpy(&src->game_data->shop_items[i], &item_data, PSOCN_STLENGTH_ITEM);
    }

    return subcmd_bb_send_shop(src, shop_type, num_items);
}

int sub62_B7_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_shop_buy_t* pkt) {
    lobby_t* l = src->cur_lobby;
    iitem_t ii = { 0 };
    int found = -1;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品购买数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    psocn_bb_char_t* player = &src->bb_pl->character;

    if (src->mode)
        player = &src->mode_pl->bb;

#ifdef DEBUG

    DBG_LOG("购买 %d 个物品 %02X %04X", pkt->num_bought, pkt->unknown_a1, pkt->shdr.unused);

#endif // DEBUG

    /* 填充物品数据头 */
    ii.present = LE16(1);
    ii.extension_data1 = 0;
    ii.extension_data2 = 0;
    ii.flags = LE32(0);

    /* 填充物品数据 */
    //memcpy(&ii.data.data_b[0], &src->game_data->shop_items[pkt->shop_item_index].data_b[0], PSOCN_STLENGTH_ITEM);
    ii.data = src->game_data->shop_items[pkt->shop_item_index];

    /* 如果是堆叠物品 */
    if (is_stackable(&ii.data)) {
        if (pkt->num_bought <= max_stack_size(&ii.data)) {
            ii.data.datab[5] = pkt->num_bought;
        }
        else {
            ERR_LOG("GC %" PRIu32 " 发送损坏的物品购买数据!",
                src->guildcard);
            ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
            return -1;
        }
    }

    ii.data.item_id = pkt->new_inv_item_id;

#ifdef DEBUG

    print_iitem_data(&ii, 0, src->version);

#endif // DEBUG

    if (!add_iitem(src, &ii)) {
        ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法获得物品!",
            src->guildcard);
        return -1;
    }

    uint32_t price = ii.data.data2l * pkt->num_bought;

    if (player->disp.meseta < price) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X MESETA %d PRICE %d",
            src->guildcard, pkt->shdr.type, player->disp.meseta, price);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
    }

    player->disp.meseta -= price;

    if (!src->mode)
        src->pl->bb.character.disp.meseta -= price;

    subcmd_send_bb_delete_meseta(src, price, 0);

    return subcmd_send_lobby_bb_create_inv_item(src, ii.data, price, false);
}

int sub62_B8_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_tekk_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    block_t* b = src->cur_block;
    uint32_t id_item_index = 0, attrib = 0;
    char percent_mod = 0;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品鉴定数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -2;
    }

    if (src->bb_pl->character.disp.meseta < 100)
        return 0;

    id_item_index = find_iitem_index(&src->bb_pl->character.inv, pkt->item_id);

    if (src->bb_pl->character.inv.iitems[id_item_index].data.datab[0] != ITEM_TYPE_WEAPON) {
        ERR_LOG("GC %" PRIu32 " 发送无法鉴定的物品!",
            src->guildcard);
        return -3;
    }

    subcmd_send_bb_delete_meseta(src, 100, 0);
    iitem_t* id_result = &src->bb_pl->character.inv.iitems[id_item_index];
    attrib = id_result->data.datab[4] & ~(0x80);

    src->game_data->identify_result = *id_result;

    // 技能属性提取和随机数处理
    if (attrib < 0x29) {
        src->game_data->identify_result.data.datab[4] = tekker_attributes[(attrib * 3) + 1];
        if ((mt19937_genrand_int32(&b->rng) % 100) > 70)
            src->game_data->identify_result.data.datab[4] += mt19937_genrand_int32(&b->rng) % ((tekker_attributes[(attrib * 3) + 2] - tekker_attributes[(attrib * 3) + 1]) + 1);
    }
    else
        src->game_data->identify_result.data.datab[4] = 0;

    // 百分比修正处理
    uint32_t mt_result_2 = mt19937_genrand_int32(&b->rng) % 10;

    if ((mt_result_2 > 0) && (mt_result_2 <= 7)) {
        if (mt_result_2 > 5) {
            percent_mod = mt19937_genrand_int32(&b->rng) % mt_result_2;
        }
        else if (mt_result_2 <= 5) {
            percent_mod -= mt19937_genrand_int32(&b->rng) % mt_result_2;
        }
    }

    // 各属性值修正处理
    if (!(id_result->data.datab[6] & 128) && (id_result->data.datab[7] > 0))
        (char)src->game_data->identify_result.data.datab[7] += percent_mod;

    if (!(id_result->data.datab[8] & 128) && (id_result->data.datab[9] > 0))
        (char)src->game_data->identify_result.data.datab[9] += percent_mod;

    if (!(id_result->data.datab[10] & 128) && (id_result->data.datab[11] > 0))
        (char)src->game_data->identify_result.data.datab[11] += percent_mod;

    if (!id_result->data.item_id) {
        ERR_LOG("GC %" PRIu32 " 未发送需要鉴定的物品!",
            src->guildcard);
        return -4;
    }

    if (id_result->data.item_id != pkt->item_id) {
        ERR_LOG("GC %" PRIu32 " 接受的物品ID与以前请求的物品ID不匹配 !",
            src->guildcard);
        return -5;
    }

    src->drop_item_id = id_result->data.item_id;
    src->drop_amt = 1;

    if (src->game_data->gm_debug)
        print_item_data(&id_result->data, src->version);

    return subcmd_send_bb_create_tekk_item(src);
}

int sub62_BA_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_accept_item_identification_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t i = 0;
    int found = -1;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    iitem_t* id_result = &src->game_data->identify_result;

    if (!id_result->data.item_id) {
        ERR_LOG("未获取到鉴定结果");
        return -1;
    }

    if (id_result->data.item_id != pkt->item_id) {
        ERR_LOG("鉴定结果 item_id != 数据包 item_id");
        return -1;
    }

    //id_result->data.item_id = generate_item_id(l, src->client_id);

    if (!add_iitem(src, id_result)) {
        ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法获得物品!",
            src->guildcard);
        return -1;
    }

    subcmd_send_lobby_bb_create_inv_item(src, id_result->data, 1, true);

    /* 初始化临时鉴定的物品数据 */
    memset(&src->game_data->identify_result, 0, PSOCN_STLENGTH_IITEM);

    src->drop_item_id = 0xFFFFFFFF;
    src->drop_amt = 0;

    return 0;
}

int sub62_BB_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_bank_open_t* req) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅打开了银行!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (req->hdr.pkt_len != LE16(0x10) || req->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送错误的银行数据包!",
            src->guildcard);
        return -1;
    }

    /* Clean up the user's bank first... */
    cleanup_bb_bank(src);

    return subcmd_send_bb_bank(src);
}

int sub62_BD_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_bank_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id;
    uint32_t amt, bank, iitem_count, i;
    int found = -1, isframe = 0;
    bool is_stack;
    iitem_t iitem = { 0 };
    bitem_t bitem = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中操作银行!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0018) || pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的银行操作数据!",
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
                ERR_LOG("GC %" PRIu32 " 存入银行的美赛塔超出他所拥有的!", src->guildcard);
                return -1;
            }

            bank = LE32(src->bb_pl->bank.meseta);
            if (amt + bank > 999999) {
                ERR_LOG("GC %" PRIu32 " 存入银行的美赛塔超出限制!", src->guildcard);
                return -1;
            }

            src->bb_pl->character.disp.meseta = LE32((iitem_count - amt));
            src->pl->bb.character.disp.meseta = src->bb_pl->character.disp.meseta;
            src->bb_pl->bank.meseta = LE32((bank + amt));

            /* No need to tell everyone else, I guess? */
            return 0;
        }
        else {
            /* 在玩家背包中查找物品. */
            iitem_count = src->bb_pl->character.inv.item_count;
            for (i = 0; i < iitem_count; ++i) {
                if (src->bb_pl->character.inv.iitems[i].data.item_id == pkt->item_id) {
                    iitem = src->bb_pl->character.inv.iitems[i];
                    found = i;

                    /* If it is an equipped frame, we need to unequip all
                       the units that are attached to it. */
                    if (iitem.data.datab[0] == ITEM_TYPE_GUARD &&
                        iitem.data.datab[1] == ITEM_SUBTYPE_FRAME &&
                        (iitem.flags & LE32(0x00000008))) {
                        isframe = 1;
                    }

                    break;
                }
            }

            /* If the item isn't found, then punt the user from the ship. */
            if (found == -1) {
                ERR_LOG("GC %" PRIu32 " 存入的物品不存在!", src->guildcard);
                return -1;
            }

            /* Unequip any units, if the item was equipped and a frame. */
            if (isframe) {
                for (i = 0; i < iitem_count; ++i) {
                    iitem = src->bb_pl->character.inv.iitems[i];
                    if (iitem.data.datab[0] == ITEM_TYPE_GUARD &&
                        iitem.data.datab[1] == ITEM_SUBTYPE_UNIT) {
                        src->bb_pl->character.inv.iitems[i].flags &= LE32(0xFFFFFFF7);
                    }
                }
            }

            is_stack = is_stackable(&iitem.data);

            if (!is_stack && pkt->item_amount > 1) {
                ERR_LOG("GC %" PRIu32 " 存入了多个非堆叠的物品!", src->guildcard);
                return -1;
            }

            iitem = remove_iitem(src, pkt->item_id, pkt->item_amount, src->version != CLIENT_VERSION_BB);

            if (&iitem == NULL) {
                ERR_LOG("GC %" PRIu32 " 移除了不存在于背包的物品!", src->guildcard);
                return -1;
            }

            /* Fill in the bank item. */
            if (is_stack) {
                iitem.data.datab[5] = pkt->item_amount;
                bitem.amount = LE16(pkt->item_amount);
            }
            else {
                bitem.amount = LE16(1);
            }

            /* 已获得背包的物品数据, 将其添加至银行数据中... */
            player_bitem_init(&bitem, &iitem);

            /* 存入! */
            if (!add_bitem(src, &bitem)) {
                ERR_LOG("GC %" PRIu32 " 存物品进银行错误!", src->guildcard);
                add_iitem(src, &iitem);
                return -1;
            }

            sort_client_bank(&src->bb_pl->bank);

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
                ERR_LOG("GC %" PRIu32 " 从银行取出的美赛塔超出了存储限制!", src->guildcard);
                return -1;
            }

            bank = LE32(src->bb_pl->bank.meseta);

            if (amt > bank) {
                ERR_LOG("GC %" PRIu32 " 从银行取出的美赛塔超出了银行库存!", src->guildcard);
                return -1;
            }

            src->bb_pl->character.disp.meseta = LE32((iitem_count + amt));
            src->pl->bb.character.disp.meseta = src->bb_pl->character.disp.meseta;
            src->bb_pl->bank.meseta = LE32((bank - amt));

            /* 存取美赛塔不用告知其他客户端... */
            return 0;
        }
        else {
            /* 尝试从银行中取出物品. */
            bitem = remove_bitem(src, pkt->item_id, pkt->item_amount);

            if (&bitem == NULL) {
                ERR_LOG("GC %" PRIu32 " 从银行中取出无效物品!", src->guildcard);
                return -1;
            }

            /* 已获得银行的物品数据, 将其添加至临时背包数据中... */
            player_iitem_init(&iitem, &bitem);
            iitem.data.item_id = generate_item_id(l, 0xFF);

            /* 新增至玩家背包中... */
            if (!add_iitem(src, &iitem)) {
                /* Uh oh... Guess we should put it back in the bank... */
                add_bitem(src, &bitem);
                return -1;
            }

            sort_client_bank(&src->bb_pl->bank);

            /* 发送至房间中的客户端. */
            return subcmd_send_lobby_bb_create_inv_item(src, iitem.data, 1, true);
        }

    default:
        ERR_LOG("GC %" PRIu32 " 发送未知银行操作: %d!",
            src->guildcard, pkt->action);
        display_packet(pkt, 0x18);
        return -1;
    }
}

int sub62_C1_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_guild_invite_t* pkt) {
    uint16_t len = pkt->hdr.pkt_len;
    uint8_t type = pkt->shdr.type;
    uint32_t invite_cmd = pkt->trans_cmd;
    uint32_t target_guildcard = pkt->traget_guildcard;

    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " 发送错误的公会邀请数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

#ifdef DEBUG
    char guild_name_text[24];
    char inviter_name_text[24];

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gb18030, inviter_name_text, &pkt->inviter_name[2], 24, 12);

    TEST_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ", type, invite_cmd, src->guildcard, dest->guildcard, target_guildcard);
    display_packet(pkt, len);
#endif // DEBUG

    switch (invite_cmd)
    {
        /* 公会邀请起始 检测双方的 公会情况 */
    case 0x00:
        if (dest->bb_guild->data.guild_id != 0) {
            dest->guild_accept = false;
            DBG_LOG("被邀请方 GUILD ID %u", dest->bb_guild->data.guild_id);
            /* 到这就没了, 获取对方已经属于某个公会. */
            send_msg(src, MSG1_TYPE, "%s\n\n%s", __(src, "\tE\tC4无法邀请玩家!"),
                __(src, "\tC7对方已在公会中."));
        }
        else
            dest->guild_accept = true;

        break;

        /* 未知功能 */
    case 0x01:
    default:
        ERR_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ", type, invite_cmd, src->guildcard, dest->guildcard, target_guildcard);
        display_packet(pkt, len);
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

    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " 发送错误的公会邀请数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gb18030, inviter_name_text, &pkt->inviter_name[2], 24, 12);

#ifdef DEBUG
    TEST_LOG("SUBCMD62_GUILD_INVITE 0x%02X 0x%08X c %u d %u 目标GC %u ",
        type, invite_cmd, src->guildcard, d->guildcard, target_guildcard);
    display_packet(pkt, len);
#endif // DEBUG

    switch (invite_cmd)
    {
        /* 公会邀请成功 检测对方的 公会情况 */
    case 0x02:
        if (src->guildcard == target_guildcard)
            if (src->bb_guild->data.guild_id != 0) {
                DBG_LOG("被邀请方 GUILD ID %u", dest->bb_guild->data.guild_id);
                /* 到这就没了, 获取对方已经属于某个公会. */
                send_msg(src, MSG1_TYPE, "%s\n\n%s", __(src, "\tE\tC4无法邀请玩家!"),
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
        display_packet(pkt, len);
        break;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_C9_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_quest_reward_meseta_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int meseta;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 尝试获取错误的任务美赛塔奖励!",
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
        memset(&ii, 0, PSOCN_STLENGTH_IITEM);
        ii.data.datab[0] = ITEM_TYPE_MESETA;
        ii.data.data2l = meseta;
        ii.data.item_id = generate_item_id(l, EMPTY_STRING);

        if (!add_iitem(src, &ii)) {
            ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法获得物品!",
                src->guildcard);
            return -1;
        }

        return subcmd_send_lobby_bb_create_inv_item(src, ii.data, meseta, true);
    }
}

int sub62_CA_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_quest_reward_item_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    display_packet(pkt, LE16(pkt->hdr.pkt_len));

    iitem_t ii;
    memset(&ii, 0, PSOCN_STLENGTH_IITEM);
    ii.data = pkt->item_data;
    ii.data.item_id = generate_item_id(l, 0xFF);

    if (!add_iitem(src, &ii)) {
        ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法获得物品!",
            src->guildcard);
        return -1;
    }

    return subcmd_send_lobby_bb_create_inv_item(src, ii.data, 1, true);
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
        ERR_LOG("GC %" PRIu32 " 发送错误的公会转让数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gb18030, master_name_text, &pkt->master_name[2], 24, 12);

    //TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS1 0x%02X 0x%08X c %u d %u", type, trans_cmd, src->guildcard, dest->guildcard);
    //display_packet(pkt, len);

    if (src->bb_guild->data.guild_priv_level != BB_GUILD_PRIV_LEVEL_MASTER) {
        ERR_LOG("GC %u 公会权限不足", src->guildcard);
        return send_msg(src, MSG1_TYPE, "%s\n\n%s", __(src, "\tE\tC4公会权限不足!"),
            __(src, "\tC7您无权进行此操作."));
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

    if (pkt->hdr.pkt_len != LE16(0x0064) || pkt->shdr.size != 0x17) {
        ERR_LOG("GC %" PRIu32 " 发送错误的公会转让数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    istrncpy16_raw(ic_utf16_to_gb18030, guild_name_text, &pkt->guild_name[2], 24, 12);
    istrncpy16_raw(ic_utf16_to_gb18030, master_name_text, &pkt->master_name[2], 24, 12);

    //TEST_LOG("SUBCMD62_GUILD_MASTER_TRANS2 0x%02X 0x%08X c %u d %u", type, trans_cmd, src->guildcard, dest->guildcard);
    //display_packet(pkt, len);

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

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

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

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    display_packet(pkt, pkt->hdr.pkt_len);

    /* Make sure there's something set with /item */
    if (!pkt->drop_amount) {
        return send_txt(src, "%s", __(src, "\tE\tC7墓碑掉落无效."));
    }

    /* TODO 错了 是掉落和玩家背包中ID一致的物品 */

    //[2023年07月31日 04:42 : 58 : 142] 调试(subcmd_handle_62.c 2515) : 未知 0x62 指令 : 0xD1
    //(00000000)   1C 00 62 00 00 00 00 00   D1 05 00 00 01 00 05 00  ..b..... ? ......
    //(00000010)   B0 FF 07 C4 99 E8 3E C2   00 00 00 00 ? .臋 ? ? ...
    item_t drop_item = { 0 };
    drop_item.datab[0] = ITEM_TYPE_TOOL;
    drop_item.datab[1] = pkt->item_subtype;
    drop_item.datab[5] = pkt->drop_amount;

    iitem_t* new_iitem = add_new_litem_locked(l, &drop_item, src->cur_area, pkt->x, pkt->z);

    if (new_iitem == NULL) {
        return send_txt(src, "%s", __(src, "\tE\tC7新物品空间不足或生成失败."));
    }

    send_pkt_bb(dest, (bb_pkt_hdr_t*)&pkt);

    return subcmd_send_lobby_drop_stack(src, 0xFBFF, NULL, src->cur_area, pkt->x, pkt->z, new_iitem->data, 1);
}

int sub62_D6_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_warp_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    iitem_t backup_item;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    memset(&backup_item, 0, PSOCN_STLENGTH_IITEM);

    backup_item = remove_iitem(src, pkt->item_data.item_id, 1, src->version != CLIENT_VERSION_BB);

    if (&backup_item == NULL) {
        ERR_LOG("GC %" PRIu32 " 传送物品ID %d 失败!",
            src->guildcard, pkt->item_data.item_id);
        return -1;
    }

    if (backup_item.data.datab[0] == 0x02)
        backup_item.data.data2b[2] |= 0x40; // Wrap a mag
    else
        backup_item.data.datab[4] |= 0x40; // Wrap other

    /* 将物品新增至背包. */
    if (!add_iitem(dest, &backup_item)) {
        add_iitem(src, &backup_item);
        return send_txt(src, __(src, "\tE\tC4传送物品失败"));
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_DF_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_black_paper_deal_photon_drop_exchange_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if ((l->oneperson) && (l->flags & LOBBY_FLAG_QUESTING) && (!l->drops_disabled)) {
        iitem_t ex_pc = { 0 };
        ex_pc.data.datal[0] = BBItem_Photon_Crystal;
        size_t item_id = find_iitem_stack_item_id(&src->bb_pl->character.inv, &ex_pc);

        /* 如果找不到该物品，则将用户从船上推下. */
        if (item_id == -1) {
            ERR_LOG("GC %" PRIu32 " 没有兑换所需物品!", src->guildcard);
            return -1;
        }

        iitem_t item = remove_iitem(src, item_id, 1, src->version != CLIENT_VERSION_BB);
        if (&item == NULL)
            l->drops_disabled = true;
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

int sub62_E0_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_black_paper_deal_reward_t* pkt) {
    lobby_t* l = src->cur_lobby;
    struct mt19937_state* rng = &l->block->rng;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    display_packet(pkt, pkt->hdr.pkt_len);

    if ((l->oneperson) && (l->flags & LOBBY_FLAG_QUESTING) && (l->drops_disabled) && (!l->questE0)) {
        uint32_t bp, bp_list_count, new_item;

        if (pkt->area > 0x03)
            bp_list_count = 1;
        else
            bp_list_count = l->difficulty + 1;

        for (bp = 0; bp < bp_list_count; bp++) {
            new_item = 0;

            switch (pkt->area)
            {
            case 0x00:
                // bp1 dorphon route
                switch (l->difficulty)
                {
                case GAME_TYPE_DIFFICULTY_NORMARL:
                    new_item = bp_dorphon_normal[mt19937_genrand_int32(rng) % (sizeof(bp_dorphon_normal) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_HARD:
                    new_item = bp_dorphon_hard[mt19937_genrand_int32(rng) % (sizeof(bp_dorphon_hard) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_VERY_HARD:
                    new_item = bp_dorphon_vhard[mt19937_genrand_int32(rng) % (sizeof(bp_dorphon_vhard) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_ULTIMATE:
                    new_item = bp_dorphon_ultimate[mt19937_genrand_int32(rng) % (sizeof(bp_dorphon_ultimate) / 4)];
                    break;
                }
                break;
            case 0x01:
                // bp1 rappy route
                switch (l->difficulty)
                {
                case GAME_TYPE_DIFFICULTY_NORMARL:
                    new_item = bp_rappy_normal[mt19937_genrand_int32(rng) % (sizeof(bp_rappy_normal) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_HARD:
                    new_item = bp_rappy_hard[mt19937_genrand_int32(rng) % (sizeof(bp_rappy_hard) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_VERY_HARD:
                    new_item = bp_rappy_vhard[mt19937_genrand_int32(rng) % (sizeof(bp_rappy_vhard) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_ULTIMATE:
                    new_item = bp_rappy_ultimate[mt19937_genrand_int32(rng) % (sizeof(bp_rappy_ultimate) / 4)];
                    break;
                }
                break;
            case 0x02:
                // bp1 zu route
                switch (l->difficulty)
                {
                case GAME_TYPE_DIFFICULTY_NORMARL:
                    new_item = bp_zu_normal[mt19937_genrand_int32(rng) % (sizeof(bp_zu_normal) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_HARD:
                    new_item = bp_zu_hard[mt19937_genrand_int32(rng) % (sizeof(bp_zu_hard) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_VERY_HARD:
                    new_item = bp_zu_vhard[mt19937_genrand_int32(rng) % (sizeof(bp_zu_vhard) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_ULTIMATE:
                    new_item = bp_zu_ultimate[mt19937_genrand_int32(rng) % (sizeof(bp_zu_ultimate) / 4)];
                    break;
                }
                break;
            case 0x04:
                // bp2
                switch (l->difficulty)
                {
                case GAME_TYPE_DIFFICULTY_NORMARL:
                    new_item = bp2_normal[mt19937_genrand_int32(rng) % (sizeof(bp2_normal) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_HARD:
                    new_item = bp2_hard[mt19937_genrand_int32(rng) % (sizeof(bp2_hard) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_VERY_HARD:
                    new_item = bp2_vhard[mt19937_genrand_int32(rng) % (sizeof(bp2_vhard) / 4)];
                    break;
                case GAME_TYPE_DIFFICULTY_ULTIMATE:
                    new_item = bp2_ultimate[mt19937_genrand_int32(rng) % (sizeof(bp2_ultimate) / 4)];
                    break;
                }
                break;
            }

            l->questE0 = 1;

            subcmd_bb_drop_stack_t bb = { 0 };

            bb.hdr.pkt_len = LE16(sizeof(subcmd_bb_drop_stack_t));
            bb.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
            bb.hdr.flags = 0;

            bb.shdr.type = SUBCMD60_DROP_STACK;
            bb.shdr.size = 0x09;
            bb.shdr.client_id = 0xFBFF;

            bb.area = pkt->area;
            bb.x = pkt->x;
            bb.z = pkt->z;

            bb.data.datal[0] = new_item;

            if (bb.data.item_id == 0xFFFFFFFF) {
                bb.data.item_id = generate_item_id(l, EMPTY_STRING);
            }

            if (new_item == 0x04) {
                //new_item = pt_tables_ep1[client->character.sectionID][l->difficulty].enemy_meseta[0x2E][0];
                new_item += mt19937_genrand_int32(rng) % 100;
                bb.data.data2l = new_item;
            }

            if (new_item == 0x00)
                bb.data.datab[4] = 0x80;

            iitem_t* litem = add_new_litem_locked(l, &bb.data, pkt->area, pkt->x, pkt->z);

            if (!litem) {
                return send_txt(src, "%s", __(src, "\tE\tC7新物品空间不足."));
            }

            return subcmd_send_drop_stack(src, 0xFBFF, pkt->area, pkt->x, pkt->z, litem->data, 1);
        }

    }

    return 0;
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
    { SUBCMD62_CHAIR_STATE               , sub62_AE_dc, sub62_AE_dc, sub62_AE_dc, sub62_AE_dc, sub62_AE_pc, sub62_AE_bb },
    { SUBCMD62_SHOP_REQ                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B5_bb },
    { SUBCMD62_SHOP_BUY                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B7_bb },
    { SUBCMD62_TEKKING                   , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_B8_bb },
    { SUBCMD62_TEKKED                    , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_BA_bb },
    { SUBCMD62_OPEN_BANK                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub62_BB_bb },
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
};

/* 处理 DC GC PC V1 V2 0x62 来自客户端的数据包. */
int subcmd_handle_62(ship_client_t* src, subcmd_pkt_t* pkt) {
    __try {
        lobby_t* l = src->cur_lobby;
        ship_client_t* dest;
        uint16_t hdr_type = pkt->hdr.dc.pkt_type;
        uint8_t type = pkt->type;
        int rv = -1;

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

        }
        else {

            if (l->subcmd_handle == NULL) {
#ifdef BB_LOG_UNKNOWN_SUBS
                DBG_LOG("未知 0x%02X 指令: 0x%02X", hdr_type, type);
                display_packet(pkt, LE16(pkt->hdr.dc.pkt_len));
                //UNK_CSPD(type, c->version, pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
                rv = send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
            }
            else
                rv = l->subcmd_handle(src, dest, pkt);
        }


        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        ERR_LOG("出现错误, 程序将退出.");
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

        /* If there's a burst going on in the lobby, delay most packets */
        if (l->flags & LOBBY_FLAG_BURSTING) {
            rv = 0;

            switch (type) {
            case SUBCMD62_BURST5://0x62 6F //其他大厅跃迁进房时触发 5
            case SUBCMD62_BURST6://0x62 71 //其他大厅跃迁进房时触发 6

                rv |= l->subcmd_handle(src, dest, pkt);
                break;

            default:
                DBG_LOG("lobby_enqueue_pkt_bb 0x62 指令: 0x%02X", type);
                rv = lobby_enqueue_pkt_bb(l, src, (bb_pkt_hdr_t*)pkt);
            }

        }
        else {

            if (l->subcmd_handle == NULL) {
#ifdef BB_LOG_UNKNOWN_SUBS
                DBG_LOG("未知 0x%02X 指令: 0x%02X", hdr_type, type);
                display_packet(pkt, len);
                //UNK_CSPD(type, c->version, pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
                rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            }
            else
                rv = l->subcmd_handle(src, dest, pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        ERR_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }

}
