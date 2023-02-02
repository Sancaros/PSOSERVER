/*
    梦幻之星中国 舰船服务器
    版权 (C) 2022 Sancaros

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
#include <iconv.h>
#include <string.h>
#include <pthread.h>

#include <f_logs.h>
#include <debug.h>
#include <f_iconv.h>
#include <mtwist.h>
#include <items.h>

#include "subcmd.h"
#include "clients.h"
#include "ship_packets.h"
#include "utils.h"
#include "items.h"
#include "shopdata.h"
#include "word_select.h"
#include "scripts.h"
#include "shipgate.h"
#include "quest_functions.h"

/* Handle a GC send packet. */
int handle_dc_gcsend(ship_client_t *s, ship_client_t *d, subcmd_dc_gcsend_t *pkt) {

    ERR_LOG(
        "handle_dc_gcsend %d 类型 = %d 版本识别 = %d",
        d->sock, pkt->hdr.pkt_type, d->version);

    /* This differs based on the destination client's version. */
    switch(d->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            return send_pkt_dc(d, (dc_pkt_hdr_t *)pkt);

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        {
            subcmd_gc_gcsend_t gc;

            memset(&gc, 0, sizeof(gc));

            /* Copy the name and text over. */
            memcpy(gc.name, pkt->name, 24);
            memcpy(gc.text, pkt->text, 88);

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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&gc);
        }

        case CLIENT_VERSION_XBOX:
        {
            subcmd_xb_gcsend_t xb;
            uint32_t guildcard = LE32(pkt->guildcard);

            memset(&xb, 0, sizeof(xb));

            /* Copy the name and text over. */
            memcpy(xb.name, pkt->name, 24);
            memcpy(xb.text, pkt->text, 88);

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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&xb);
        }

        case CLIENT_VERSION_PC:
        {
            subcmd_pc_gcsend_t pc;
            size_t in, out;
            char *inptr;
            char *outptr;

            /* Don't allow guild cards to be sent to PC NTE, as it doesn't
               support them. */
            if((d->flags & CLIENT_FLAG_IS_NTE)) {
                if(s)
                    return send_txt(s, "%s", __(s, "\tE\tC7Cannot send Guild\n"
                                                   "Card to that user."));
                else
                    return 0;
            }

            memset(&pc, 0, sizeof(pc));

            /* Convert the name (ASCII -> UTF-16). */
            in = 24;
            out = 48;
            inptr = pkt->name;
            outptr = (char *)pc.name;
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

            /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
            in = 88;
            out = 176;
            inptr = pkt->text;
            outptr = (char *)pc.text;

            if(pkt->text[1] == 'J') {
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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&pc);
        }

        case CLIENT_VERSION_BB:
        {
            subcmd_bb_gcsend_t bb;
            size_t in, out;
            char *inptr;
            char *outptr;

            memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));

            /* Convert the name (ASCII -> UTF-16). */
            bb.name[0] = LE16('\t');
            bb.name[1] = LE16('E');
            in = 24;
            out = 44;
            inptr = pkt->name;
            outptr = (char *)&bb.name[2];
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

            /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
            in = 88;
            out = 176;
            inptr = pkt->text;
            outptr = (char *)bb.text;

            if(pkt->text[1] == 'J') {
                iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
            }

            /* Copy the rest over. */
            bb.hdr.pkt_len = LE16(0x0114);
            bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
            bb.hdr.flags = LE32(d->client_id);
            bb.shdr.type = SUBCMD62_GUILDCARD;
            bb.shdr.size = 0x43;
            bb.shdr.unused = 0x0000;
            bb.guildcard = pkt->guildcard;
            bb.disable_udp = 1;
            bb.language = pkt->language;
            bb.section = pkt->section;
            bb.ch_class = pkt->ch_class;

            return send_pkt_bb(d, (bb_pkt_hdr_t *)&bb);
        }
    }

    return 0;
}

static int handle_pc_gcsend(ship_client_t *s, ship_client_t *d, subcmd_pc_gcsend_t *pkt) {

    /*ERR_LOG(
        "handle_pc_gcsend %d 类型 = %d 版本识别 = %d",
        d->sock, pkt->hdr.pkt_type, d->version);*/

    /* This differs based on the destination client's version. */
    switch(d->version) {
        case CLIENT_VERSION_PC:
            /* Don't allow guild cards to be sent to PC NTE, as it doesn't
               support them. */
            if((d->flags & CLIENT_FLAG_IS_NTE))
                return send_txt(s, "%s", __(s, "\tE\tC7无法发送GC至该玩家."));

            return send_pkt_dc(d, (dc_pkt_hdr_t *)pkt);

        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        {
            subcmd_dc_gcsend_t dc;
            size_t in, out;
            char *inptr;
            char *outptr;

            memset(&dc, 0, sizeof(dc));

            /* Convert the name (UTF-16 -> ASCII). */
            in = 48;
            out = 24;
            inptr = (char *)pkt->name;
            outptr = dc.name;
            iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

            /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
            in = 176;
            out = 88;
            inptr = (char *)pkt->text;
            outptr = dc.text;

            if(pkt->text[1] == LE16('J')) {
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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&dc);
        }

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        {
            subcmd_gc_gcsend_t gc;
            size_t in, out;
            char *inptr;
            char *outptr;

            memset(&gc, 0, sizeof(gc));

            /* Convert the name (UTF-16 -> ASCII). */
            in = 48;
            out = 24;
            inptr = (char *)pkt->name;
            outptr = gc.name;
            iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

            /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
            in = 176;
            out = 88;
            inptr = (char *)pkt->text;
            outptr = gc.text;

            if(pkt->text[1] == LE16('J')) {
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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&gc);
        }

        case CLIENT_VERSION_XBOX:
        {
            subcmd_xb_gcsend_t xb;
            uint32_t guildcard = LE32(pkt->guildcard);
            size_t in, out;
            char *inptr;
            char *outptr;

            memset(&xb, 0, sizeof(xb));

            /* Convert the name (UTF-16 -> ASCII). */
            in = 48;
            out = 24;
            inptr = (char *)pkt->name;
            outptr = xb.name;
            iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

            /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
            in = 176;
            out = 512;
            inptr = (char *)pkt->text;
            outptr = xb.text;

            if(pkt->text[1] == LE16('J')) {
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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&xb);
        }

        case CLIENT_VERSION_BB:
        {
            subcmd_bb_gcsend_t bb;

            /* 填充数据并准备发送.. */
            memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));
            bb.hdr.pkt_len = LE16(0x0114);
            bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
            bb.hdr.flags = LE32(d->client_id);
            bb.shdr.type = SUBCMD62_GUILDCARD;
            bb.shdr.size = 0x43;
            bb.shdr.unused = 0x0000;
            bb.guildcard = pkt->guildcard;
            bb.name[0] = LE16('\t');
            bb.name[1] = LE16('E');
            memcpy(&bb.name[2], pkt->name, 28);
            memcpy(bb.text, pkt->text, 176);
            bb.disable_udp = 1;
            bb.language = pkt->language;
            bb.section = pkt->section;
            bb.ch_class = pkt->ch_class;

            return send_pkt_bb(d, (bb_pkt_hdr_t *)&bb);
        }
    }

    return 0;
}

static int handle_gc_gcsend(ship_client_t *s, ship_client_t *d, subcmd_gc_gcsend_t *pkt) {

    /*ERR_LOG(
        "handle_gc_gcsend %d 类型 = %d 版本识别 = %d",
        d->sock, pkt->hdr.pkt_type, d->version);*/

    /* This differs based on the destination client's version. */
    switch(d->version) {
        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
            return send_pkt_dc(d, (dc_pkt_hdr_t *)pkt);

        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        {
            subcmd_dc_gcsend_t dc;

            memset(&dc, 0, sizeof(dc));

            /* Copy the name and text over. */
            memcpy(dc.name, pkt->name, 24);
            memcpy(dc.text, pkt->text, 88);

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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&dc);
        }

        case CLIENT_VERSION_XBOX:
        {
            subcmd_xb_gcsend_t xb;
            uint32_t guildcard = LE32(pkt->guildcard);

            memset(&xb, 0, sizeof(xb));

            /* Copy the name and text over. */
            memcpy(xb.name, pkt->name, 24);
            memcpy(xb.text, pkt->text, 104);

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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&xb);
        }

        case CLIENT_VERSION_PC:
        {
            subcmd_pc_gcsend_t pc;
            size_t in, out;
            char *inptr;
            char *outptr;

            /* Don't allow guild cards to be sent to PC NTE, as it doesn't
               support them. */
            if((d->flags & CLIENT_FLAG_IS_NTE))
                return send_txt(s, "%s", __(s, "\tE\tC无法发送GC至该玩家."));

            memset(&pc, 0, sizeof(pc));

            /* Convert the name (ASCII -> UTF-16). */
            in = 24;
            out = 48;
            inptr = pkt->name;
            outptr = (char *)pc.name;
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

            /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
            in = 88;
            out = 176;
            inptr = pkt->text;
            outptr = (char *)pc.text;

            if(pkt->text[1] == 'J') {
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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&pc);
        }

        case CLIENT_VERSION_BB:
        {
            subcmd_bb_gcsend_t bb;
            size_t in, out;
            char *inptr;
            char *outptr;

            memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));

            /* Convert the name (ASCII -> UTF-16). */
            bb.name[0] = LE16('\t');
            bb.name[1] = LE16('E');
            in = 24;
            out = 44;
            inptr = pkt->name;
            outptr = (char *)&bb.name[2];
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

            /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
            in = 88;
            out = 176;
            inptr = pkt->text;
            outptr = (char *)bb.text;

            if(pkt->text[1] == 'J') {
                iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
            }

            /* Copy the rest over. */
            bb.hdr.pkt_len = LE16(0x0114);
            bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
            bb.hdr.flags = LE32(d->client_id);
            bb.shdr.type = SUBCMD62_GUILDCARD;
            bb.shdr.size = 0x43;
            bb.shdr.unused = 0x0000;
            bb.guildcard = pkt->guildcard;
            bb.disable_udp = 1;
            bb.language = pkt->language;
            bb.section = pkt->section;
            bb.ch_class = pkt->ch_class;

            return send_pkt_bb(d, (bb_pkt_hdr_t *)&bb);
        }
    }

    return 0;
}

int handle_xb_gcsend(ship_client_t *s, ship_client_t *d, subcmd_xb_gcsend_t *pkt) {

    /*ERR_LOG(
        "handle_xb_gcsend %d 类型 = %d 版本识别 = %d",
        d->sock, pkt->hdr.pkt_type, d->version);*/

    /* This differs based on the destination client's version. */
    switch(d->version) {
        case CLIENT_VERSION_XBOX:
            return send_pkt_dc(d, (dc_pkt_hdr_t *)pkt);

        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        {
            subcmd_dc_gcsend_t dc;

            memset(&dc, 0, sizeof(dc));

            /* Copy the name and text over. */
            memcpy(dc.name, pkt->name, 24);
            memcpy(dc.text, pkt->text, 88);

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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&dc);
        }

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        {
            subcmd_gc_gcsend_t gc;

            memset(&gc, 0, sizeof(gc));

            /* Copy the name and text over. */
            memcpy(gc.name, pkt->name, 24);
            memcpy(gc.text, pkt->text, 88);

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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&gc);
        }

        case CLIENT_VERSION_PC:
        {
            subcmd_pc_gcsend_t pc;
            size_t in, out;
            char *inptr;
            char *outptr;

            /* Don't allow guild cards to be sent to PC NTE, as it doesn't
               support them. */
            if((d->flags & CLIENT_FLAG_IS_NTE)) {
                if(s)
                    return send_txt(s, "%s", __(s, "\tE\tC7无法发送GC至该玩家."));
                else
                    return 0;
            }

            memset(&pc, 0, sizeof(pc));

            /* Convert the name (ASCII -> UTF-16). */
            in = 24;
            out = 48;
            inptr = pkt->name;
            outptr = (char *)pc.name;
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

            /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
            in = 88;
            out = 176;
            inptr = pkt->text;
            outptr = (char *)pc.text;

            if(pkt->text[1] == 'J') {
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

            return send_pkt_dc(d, (dc_pkt_hdr_t *)&pc);
        }

        case CLIENT_VERSION_BB:
        {
            subcmd_bb_gcsend_t bb;
            size_t in, out;
            char *inptr;
            char *outptr;

            memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));

            /* Convert the name (ASCII -> UTF-16). */
            bb.name[0] = LE16('\t');
            bb.name[1] = LE16('E');
            in = 24;
            out = 44;
            inptr = pkt->name;
            outptr = (char *)&bb.name[2];
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);

            /* Convert the text (ISO-8859-1 or SHIFT-JIS -> UTF-16). */
            in = 88;
            out = 176;
            inptr = pkt->text;
            outptr = (char *)bb.text;

            if(pkt->text[1] == 'J') {
                iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
            }

            /* Copy the rest over. */
            bb.hdr.pkt_len = LE16(0x0114);
            bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
            bb.hdr.flags = LE32(d->client_id);
            bb.shdr.type = SUBCMD62_GUILDCARD;
            bb.shdr.size = 0x43;
            bb.shdr.unused = 0x0000;
            bb.guildcard = pkt->guildcard;
            bb.disable_udp = 1;
            bb.language = pkt->language;
            bb.section = pkt->section;
            bb.ch_class = pkt->ch_class;

            return send_pkt_bb(d, (bb_pkt_hdr_t *)&bb);
        }
    }

    return 0;
}

static int handle_gm_itemreq(ship_client_t *c, subcmd_itemreq_t *req) {
    subcmd_itemgen_t gen = { 0 };
    int r = LE16(req->request_id);
    int i;
    lobby_t *l = c->cur_lobby;

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
    gen.data.unk1 = LE16(0x00000010);

    gen.data.item.data_l[0] = LE32(c->next_item[0]);
    gen.data.item.data_l[1] = LE32(c->next_item[1]);
    gen.data.item.data_l[2] = LE32(c->next_item[2]);
    gen.data.item.data2_l = LE32(c->next_item[3]);
    gen.data.item2 = LE32(0x00000002);

    /* Obviously not "right", but it works though, so we'll go with it. */
    gen.data.item.item_id = LE32((r | 0x06010100));

    /* Send the packet to every client in the lobby. */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&gen);
        }
    }

    /* Clear this out. */
    c->next_item[0] = c->next_item[1] = c->next_item[2] = c->next_item[3] = 0;

    return 0;
}

static int handle_quest_itemreq(ship_client_t *c, subcmd_itemreq_t *req, ship_client_t *dest) {
    uint32_t mid = LE16(req->request_id);
    uint32_t pti = req->pt_index;
    lobby_t *l = c->cur_lobby;
    uint32_t qdrop = 0xFFFFFFFF;

    if(pti != 0x30 && l->mids)
        qdrop = quest_search_enemy_list(mid, l->mids, l->num_mids, 0);
    if(qdrop == 0xFFFFFFFF && l->mtypes)
        qdrop = quest_search_enemy_list(pti, l->mtypes, l->num_mtypes, 0);

    /* If we found something, the version matters here. Basically, we only care
       about the none option on DC/PC, as rares do not drop in quests. On GC,
       we have to block drops on all options other than free, since we have no
       control over the drop once we send it to the leader. */
    if(qdrop != PSOCN_QUEST_ENDROP_FREE && qdrop != 0xFFFFFFFF) {
        switch(l->version) {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
            case CLIENT_VERSION_PC:
                if(qdrop == PSOCN_QUEST_ENDROP_NONE)
                    return 0;
                break;

            case CLIENT_VERSION_GC:
            case CLIENT_VERSION_XBOX:
                return 0;
        }
    }

    /* If we haven't handled it, we're not supposed to, so send it on to the
       leader. */
    return send_pkt_dc(dest, (dc_pkt_hdr_t *)req);
}

static int handle_level_up(ship_client_t *c, subcmd_level_up_t*pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in a lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if(pkt->shdr.size != 0x05 || pkt->shdr.client_id != c->client_id) {
        return -1;
    }

    /* Copy over the new data to the client's character structure... */
    c->pl->v1.character.disp.stats.atp = pkt->atp;
    c->pl->v1.character.disp.stats.mst = pkt->mst;
    c->pl->v1.character.disp.stats.evp = pkt->evp;
    c->pl->v1.character.disp.stats.hp = pkt->hp;
    c->pl->v1.character.disp.stats.dfp = pkt->dfp;
    c->pl->v1.character.disp.stats.ata = pkt->ata;
    c->pl->v1.character.disp.level = pkt->level;

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_take_item(ship_client_t *c, subcmd_take_item_t *pkt) {
    lobby_t *l = c->cur_lobby;
    iitem_t item = { 0 };
    uint32_t v;
    int i;
    subcmd_take_item_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY)
        return -1;

    /* Buggy PSO version is buggy... */
    if(c->version == CLIENT_VERSION_DCV1 && pkt->shdr.size == 0x06)
        pkt->shdr.size = 0x07;

    /* Sanity check... Make sure the size of the subcommand is valid, and
       disconnect the client if it isn't. */
    if(pkt->shdr.size != 0x07)
        return -1;

    /* If we have multiple clients in the team, make sure that the client id in
       the packet matches the user sending the packet.
       Note: We don't do this in single player teams because NPCs do weird
       things if you change their equipment in quests. */
    if(l->num_clients != 1 && pkt->shdr.client_id != c->client_id)
        return -1;

    /* Outside of quests, we shouldn't be able to get these unless the shopping
       flag is set... Log any we get. */
    if(!(l->flags & LOBBY_FLAG_QUESTING) &&
       !(c->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("GC %" PRIu32 " 尝试在未打开银行的情况下取出物品!", c->guildcard);
    }

    /* Run the bank action script, if any. */
    if(script_execute(ScriptActionBankAction, c, SCRIPT_ARG_PTR, c,
                      SCRIPT_ARG_INT, 1, SCRIPT_ARG_UINT32, pkt->data.data_l[0],
                      SCRIPT_ARG_UINT32, pkt->data.data_l[1], SCRIPT_ARG_UINT32,
                      pkt->data.data_l[2], SCRIPT_ARG_UINT32, pkt->data.data2_l,
                      SCRIPT_ARG_UINT32, pkt->data.item_id, SCRIPT_ARG_END) < 0) {
        return -1;
    }

    /* If we're in legit mode, we need to check the newly taken item. */
    if((l->flags & LOBBY_FLAG_LEGIT_MODE) && l->limits_list) {
        switch(c->version) {
            case CLIENT_VERSION_DCV1:
                v = ITEM_VERSION_V1;
                break;

            case CLIENT_VERSION_DCV2:
            case CLIENT_VERSION_PC:
                v = ITEM_VERSION_V2;
                break;

            case CLIENT_VERSION_GC:
            case CLIENT_VERSION_XBOX:
                v = ITEM_VERSION_GC;
                break;

            default:
                return -1;
        }

        /* Fill in the item structure so we can check it. */
        memcpy(&item.data.data_l[0], &pkt->data.data_l[0], sizeof(uint32_t) * 5);

        if(!psocn_limits_check_item(l->limits_list, &item, v)) {
            DBG_LOG("Potentially non-legit item in legit mode:\n"
                  "%08x %08x %08x %08x", LE32(pkt->data.data_l[0]),
                  LE32(pkt->data.data_l[1]), LE32(pkt->data.data_l[2]),
                  LE32(pkt->data.data2_l));

            /* The item failed the check, so kick the user. */
            send_msg_box(c, "%s\n\n%s\n%s",
                             __(c, "\tE您已被踢出服务器."),
                             __(c, "原因:"),
                             __(c, "Attempt to remove a non-legit item from\n"
                                "the bank in a legit-mode game."));
            return -1;
        }
    }

    /* If we get here, either the game is not in legit mode, or the item is
       actually legit, so make a note of the ID, add it to the inventory and
       forward the packet on. */
    l->highest_item[c->client_id] = (uint16_t)LE32(pkt->data.item_id);
    v = LE32(pkt->data.data_l[0]);

    if(!(c->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* See if its a stackable item, since we have to treat them differently. */
    if(item_is_stackable(v)) {
        /* Its stackable, so see if we have any in the inventory already */
        for(i = 0; i < c->item_count; ++i) {
            /* Found it, add what we're adding in */
            if(c->iitems[i].data.data_l[0] == pkt->data.data_l[0]) {
                c->iitems[i].data.data_l[1] += pkt->data.data_l[1];
                goto send_pkt;
            }
        }
    }

    memcpy(&c->iitems[c->item_count++].data.data_l[0], &pkt->data.data_l[0],
           sizeof(uint32_t) * 5);

send_pkt:
    /* If the item isn't a mag, or the client isn't Xbox or GC, then just
       send the packet away now. */
    if ((v & 0xff) != ITEM_TYPE_MAG ||
        (c->version != CLIENT_VERSION_XBOX && c->version != CLIENT_VERSION_GC)) {
        return subcmd_send_lobby_dc(c->cur_lobby, c, (subcmd_pkt_t*)pkt, 0);
    }
    else {
        /* If we have a mag and the user is on GC or Xbox, we have to swap the
           last dword when sending to the other of those two versions to make
           things work correctly in cross-play teams. */
        memcpy(&tr, pkt, sizeof(subcmd_take_item_t));
        tr.data.data2_l = SWAP32(tr.data.data2_l);

        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != c) {
                if (l->clients[i]->version == c->version) {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
                }
                else {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&tr);
                }
            }
        }

        return 0;
    }
}

static int handle_itemdrop(ship_client_t* c, subcmd_itemgen_t* pkt) {
    lobby_t* l = c->cur_lobby;
    iitem_t item = { 0 };
    uint32_t v;
    int i;
    ship_client_t* c2;
    const char* name;
    subcmd_destroy_item_t dp;
    subcmd_itemgen_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. We accept two different sizes here
       0x0B for v2 and later, and 0x0A for v1. */
    if (pkt->shdr.size != 0x0B && pkt->shdr.size != 0x0A) {
        return -1;
    }

    /* If we're in legit mode, we need to check the item. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) && l->limits_list) {
        switch (c->version) {
        case CLIENT_VERSION_DCV1:
            v = ITEM_VERSION_V1;
            break;

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
            v = ITEM_VERSION_V2;
            break;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            v = ITEM_VERSION_GC;
            break;

        default:
            return -1;
        }

        /* Fill in the item structure so we can check it. */
        memcpy(&item.data.data_l[0], &pkt->data.item.data_l[0], 5 * sizeof(uint32_t));

        if (!psocn_limits_check_item(l->limits_list, &item, v)) {
            /* The item failed the check, deal with it. */
            DBG_LOG("Potentially non-legit item dropped in legit mode:\n"
                "%08x %08x %08x %08x %08x", LE32(pkt->data.item.data_l[0]),
                LE32(pkt->data.item.data_l[1]), LE32(pkt->data.item.data_l[2]),
                LE32(pkt->data.item.data2_l), LE32(pkt->data.item2));

            /* Grab the item name, if we can find it. */
            name = iitem_get_name((iitem_t*)&item, v);

            /* Fill in the destroy item packet. */
            memset(&dp, 0, sizeof(subcmd_destroy_item_t));
            dp.hdr.pkt_type = GAME_COMMAND0_TYPE;
            dp.hdr.pkt_len = LE16(0x0010);
            dp.shdr.type = SUBCMD60_DESTROY_ITEM;
            dp.shdr.size = 0x03;
            dp.item_id = pkt->data.item.item_id;

            /* Send out a warning message, followed by the drop, followed by a
               packet deleting the drop from the game (to prevent any desync) */
            for (i = 0; i < l->max_clients; ++i) {
                if ((c2 = l->clients[i])) {
                    if (name) {
                        send_txt(c2, "%s: %s",
                            __(c2, "\tE\tC7Potentially hacked drop\n"
                                "detected."), name);
                    }
                    else {
                        send_txt(c2, "%s",
                            __(c2, "\tE\tC7Potentially hacked drop\n"
                                "detected."));
                    }

                    /* Send out the drop item packet. This doesn't go to the
                       person who originated the drop (the team leader). */
                    if (c != c2) {
                        send_pkt_dc(c2, (dc_pkt_hdr_t*)pkt);
                    }

                    /* Send out the destroy drop packet. */
                    send_pkt_dc(c2, (dc_pkt_hdr_t*)&dp);
                }
            }

            /* We're done */
            return 0;
        }
    }

    /* If we end up here, then the item is legit... */
    v = LE32(pkt->data.item.data_l[0]) & 0xff;

    /* If the item isn't a mag, or the client isn't Xbox or GC, then just
       send the packet away now. */
    if ((v & 0xff) != ITEM_TYPE_MAG ||
        (c->version != CLIENT_VERSION_XBOX && c->version != CLIENT_VERSION_GC)) {
        return subcmd_send_lobby_dc(c->cur_lobby, c, (subcmd_pkt_t*)pkt, 0);
    }
    else {
        /* If we have a mag and the user is on GC or Xbox, we have to swap the
           last dword when sending to the other of those two versions to make
           things work correctly in cross-play teams. */
        memcpy(&tr, pkt, sizeof(subcmd_itemgen_t));
        tr.data.item.data2_l = SWAP32(tr.data.item.data2_l);

        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != c) {
                if (l->clients[i]->version == c->version) {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
                }
                else {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&tr);
                }
            }
        }

        return 0;
    }
}

static int handle_take_damage(ship_client_t *c, subcmd_take_damage_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY || pkt->shdr.client_id != c->client_id) {
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
       !(c->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD60_STAT_HPUP, 2000);
}

static int handle_used_tech(ship_client_t *c, subcmd_used_tech_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅使用法术!",
              c->guildcard);
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
       !(c->flags & CLIENT_FLAG_INFINITE_TP)) {
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD60_STAT_TPUP, 255);
}

static void update_qpos(ship_client_t *c, lobby_t *l) {
    uint8_t r;

    if((r = l->qpos_regs[c->client_id][0])) {
        send_sync_register(l->clients[0], r, (uint32_t)c->x);
        send_sync_register(l->clients[0], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[0], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[0], r + 3, (uint32_t)c->cur_area);
    }
    if((r = l->qpos_regs[c->client_id][1])) {
        send_sync_register(l->clients[1], r, (uint32_t)c->x);
        send_sync_register(l->clients[1], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[1], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[1], r + 3, (uint32_t)c->cur_area);
    }
    if((r = l->qpos_regs[c->client_id][2])) {
        send_sync_register(l->clients[2], r, (uint32_t)c->x);
        send_sync_register(l->clients[2], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[2], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[2], r + 3, (uint32_t)c->cur_area);
    }
    if((r = l->qpos_regs[c->client_id][3])) {
        send_sync_register(l->clients[3], r, (uint32_t)c->x);
        send_sync_register(l->clients[3], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[3], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[3], r + 3, (uint32_t)c->cur_area);
    }
}

static int handle_set_area(ship_client_t *c, subcmd_set_area_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* Make sure the area is valid */
    if(pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if(c->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, c, SCRIPT_ARG_PTR, c,
                       SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
                       c->cur_area, SCRIPT_ARG_END);

        /* Clear the list of dropped items. */
        if(c->cur_area == 0) {
            memset(c->p2_drops, 0, sizeof(c->p2_drops));
            c->p2_drops_max = 0;
        }

        c->cur_area = pkt->area;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(c, l);
    }

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_inter_level_warp(ship_client_t* c, subcmd_inter_level_warp_t* pkt) {
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
            update_qpos(c, l);
    }

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t*)pkt, 0);
}

static int handle_set_pos(ship_client_t *c, subcmd_set_pos_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* Save the new position and move along */
    if(c->client_id == pkt->shdr.client_id) {
        c->w = pkt->w;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(c, l);
    }

    /* Clear this, in case we're at the lobby counter */
    c->last_info_req = 0;

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_move(ship_client_t *c, subcmd_move_t*pkt) {
    lobby_t *l = c->cur_lobby;

    /* Save the new position and move along */
    if(c->client_id == pkt->shdr.client_id) {
        c->x = pkt->x;
        c->z = pkt->z;

        if((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(c, l);
    }

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_delete_inv(ship_client_t *c, subcmd_destroy_item_t *pkt) {
    lobby_t *l = c->cur_lobby;
    int num;
    uint32_t i, item_id;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY)
        return -1;

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if(pkt->shdr.size != 0x03)
        return -1;

    /* Ignore meseta */
    if(pkt->item_id != 0xFFFFFFFF) {
        /* Has the user recently dropped this item without picking it up? If so,
           they can't possibly have it in their inventory. */
        item_id = LE32(pkt->item_id);

        for(i = 0; i < c->p2_drops_max; ++i) {
            if(c->p2_drops[i] == item_id) {
                ERR_LOG("GC %" PRIu32 " appears to be duping "
                      "item with id %" PRIu32 "!", c->guildcard, item_id);
            }
        }
    }

    if(!(c->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* Ignore meseta */
    if(pkt->item_id != 0xFFFFFFFF) {
        /* Remove the item from the user's inventory */
        num = item_remove_from_inv(c->iitems, c->item_count, pkt->item_id,
                                   LE32(pkt->amount));
        if(num < 0) {
            ERR_LOG("Couldn't remove item from inventory!\n");
        }
        else {
            c->item_count -= num;
        }
    }

send_pkt:
    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_buy(ship_client_t *c, subcmd_buy_t *pkt) {
    lobby_t *l = c->cur_lobby;
    uint32_t ic;
    int i;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY)
        return -1;

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if(pkt->shdr.size != 0x06 || pkt->shdr.client_id != c->client_id)
        return -1;

    /* Make a note of the item ID, and add to the inventory */
    l->highest_item[c->client_id] = LE32(pkt->data.sitem_id);

    if(!(c->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    ic = LE32(pkt->data.data_l[0]);

    /* See if its a stackable item, since we have to treat them differently. */
    if(item_is_stackable(ic)) {
        /* Its stackable, so see if we have any in the inventory already */
        for(i = 0; i < c->item_count; ++i) {
            /* Found it, add what we're adding in */
            if(c->iitems[i].data.data_l[0] == pkt->data.data_l[0]) {
                c->iitems[i].data.data_l[1] += pkt->data.data_l[1];
                goto send_pkt;
            }
        }
    }

    memcpy(&c->iitems[c->item_count].data.data_l[0], &pkt->data.data_l[0],
           sizeof(uint32_t) * 4);
    c->iitems[c->item_count++].data.data2_l = 0;

send_pkt:
    return subcmd_send_lobby_dc(c->cur_lobby, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_use_item(ship_client_t *c, subcmd_use_item_t *pkt) {
    lobby_t *l = c->cur_lobby;
    int num;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " used item in lobby!",
              c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if(pkt->shdr.size != 0x02)
        return -1;

    if(!(c->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* Remove the item from the user's inventory */
    num = item_remove_from_inv(c->iitems, c->item_count, pkt->item_id, 1);
    if(num < 0)
        ERR_LOG("Couldn't remove item from inventory!\n");
    else
        c->item_count -= num;

send_pkt:
    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_word_select(ship_client_t *c, subcmd_word_select_t *pkt) {
    /* Don't send the message if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send chats for a STFUed client. */
    if((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            return word_select_send_dc(c, pkt);

        case CLIENT_VERSION_PC:
            return word_select_send_pc(c, pkt);

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_BB:
        case CLIENT_VERSION_XBOX:
            return word_select_send_gc(c, pkt);
    }

    return 0;
}

static int handle_symbol_chat(ship_client_t *c, subcmd_symbol_chat_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* Don't send the message if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send chats for a STFUed client. */
    if((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 1);
}

static int handle_cmode_grave(ship_client_t *c, subcmd_pkt_t *pkt) {
    int i;
    lobby_t *l = c->cur_lobby;
    subcmd_pc_grave_t pc = { { 0 } };
    subcmd_dc_grave_t dc = { { 0 } };
    subcmd_bb_grave_t bb = { { 0 } };
    size_t in, out;
    char *inptr;
    char *outptr;

    /* Deal with converting the different versions... */
    switch(c->version) {
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

            for(i = 0; i < 0x0C; ++i) {
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
            outptr = (char *)pc.team;

            if(dc.team[1] == 'J') {
                iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
            }

            /* Convert the message */
            in = 24;
            out = 48;
            inptr = dc.message;
            outptr = (char *)pc.message;

            if(dc.message[1] == 'J') {
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

            for(i = 0; i < 0x0C; ++i) {
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
            inptr = (char *)pc.team;
            outptr = dc.team;

            if(pc.team[1] == LE16('J')) {
                iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
            }
            else {
                iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
            }

            /* Convert the message */
            in = 48;
            out = 24;
            inptr = (char *)pc.message;
            outptr = dc.message;

            if(pc.message[1] == LE16('J')) {
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
            break;

        default:
            return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
    }

    /* Send the packet to everyone in the lobby */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] && l->clients[i] != c) {
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV2:
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&dc);
                    break;

                case CLIENT_VERSION_PC:
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&pc);
                    break;

                case CLIENT_VERSION_BB:
                    send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&bb);
                    break;
            }
        }
    }

    return 0;
}

/* XXXX: We need to handle the b0rked nature of the Gamecube packet for this one
   still (for more than just kill tracking). */
static int handle_mhit(ship_client_t *c, subcmd_mhit_pkt_t *pkt) {
    lobby_t *l = c->cur_lobby;
    uint16_t mid, mid2, dmg;
    game_enemy_t *en;
    uint32_t flags;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了怪物!",
              c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if(pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的怪物攻击数据!",
              c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Grab relevant information from the packet */
    mid2 = LE16(pkt->shdr.enemy_id);
    mid = LE16(pkt->enemy_id2);
    dmg = LE16(pkt->damage);
    flags = LE32(pkt->flags);

    /* Swap the flags on the packet if the user is on GC... Looks like Sega
       decided that it should be the opposite order as it is on DC/PC/BB. */
    if(c->version == CLIENT_VERSION_GC)
        flags = SWAP32(flags);

    /* Bail out now if we don't have any enemy data on the team. */
    if(!l->map_enemies || l->challenge || l->battle) {
        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
                       SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        if(flags & 0x00000800)
            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                           SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        return send_lobby_mhit(l, c, mid, mid2, dmg, flags);
    }

    /* Make sure the enemy is in range. */
    if(mid > l->map_enemies->count) {
#ifdef DEBUG
        ERR_LOG("GC %" PRIu32 " hit invalid enemy (%d -- max: "
              "%d)!\n"
              "Episode: %d, Floor: %d, Map: (%d, %d)", c->guildcard, mid,
              l->map_enemies->count, l->episode, c->cur_area,
              l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);

        if((l->flags & LOBBY_FLAG_QUESTING))
            ERR_LOG("Quest ID: %d, Version: %d", l->qid, l->version);
#endif

        if(l->logfp) {
            fdebug(l->logfp, DBG_WARN, "GC %" PRIu32 " hit invalid "
                   "enemy (%d -- max: %d)!\n"
                   "Episode: %d, Floor: %d, Map: (%d, %d)\n", c->guildcard, mid,
                   l->map_enemies->count, l->episode, c->cur_area,
                   l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);

            if((l->flags & LOBBY_FLAG_QUESTING))
                fdebug(l->logfp, DBG_WARN, "Quest ID: %d, Version: %d\n",
                       l->qid, l->version);
        }

        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
                       SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        if(flags & 0x00000800)
            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                           SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        /* If server-side drops aren't on, then just send it on and hope for the
           best. We've probably got a bug somewhere on our end anyway... */
        if(!(l->flags & LOBBY_FLAG_SERVER_DROPS))
            return send_lobby_mhit(l, c, mid, mid2, dmg, flags);

        return -1;
    }

    /* Make sure it looks like they're in the right area for this... */
    /* XXXX: There are some issues still with Episode 2, so only spit this out
       for now on Episode 1. */
#ifdef DEBUG
    if(c->cur_area != l->map_enemies->enemies[mid].area && l->episode == 1 &&
       !(l->flags & LOBBY_FLAG_QUESTING)) {
        ERR_LOG("GC %" PRIu32 " hit enemy in wrong area "
              "(%d -- max: %d)!\n Episode: %d, Area: %d, Enemy Area: %d "
              "Map: (%d, %d)", c->guildcard, mid, l->map_enemies->count,
              l->episode, c->cur_area, l->map_enemies->enemies[mid].area,
              l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);
    }
#endif

    if(l->logfp && c->cur_area != l->map_enemies->enemies[mid].area &&
       !(l->flags & LOBBY_FLAG_QUESTING)) {
        fdebug(l->logfp, DBG_WARN, "GC %" PRIu32 " hit enemy in wrong "
               "area (%d -- max: %d)!\n Episode: %d, Area: %d, Enemy Area: %d "
               "Map: (%d, %d)", c->guildcard, mid, l->map_enemies->count,
               l->episode, c->cur_area, l->map_enemies->enemies[mid].area,
               l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);
    }

    /* Make sure the person's allowed to be on this floor in the first place. */
    if((l->flags & LOBBY_FLAG_ONLY_ONE) && !(l->flags & LOBBY_FLAG_QUESTING)) {
        if(l->episode == 1) {
            switch(c->cur_area) {
                case 5:     /* Cave 3 */
                case 12:    /* De Rol Le */
                    ERR_LOG("GC %" PRIu32 " hit enemy in area "
                          "impossible to\nreach in a single-player team (%d)\n"
                          "Team Flags: %08" PRIx32 "",
                          c->guildcard, c->cur_area, l->flags);
                    break;
            }
        }
    }

    /* Save the hit, assuming the enemy isn't already dead. */
    en = &l->map_enemies->enemies[mid];
    if(!(en->clients_hit & 0x80)) {
        en->clients_hit |= (1 << c->client_id);
        en->last_client = c->client_id;

        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
                       SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_UINT32, en->bp_entry,
                       SCRIPT_ARG_UINT8, en->rt_index, SCRIPT_ARG_UINT8,
                       en->clients_hit, SCRIPT_ARG_END);

        /* If the kill flag is set, mark it as dead and update the client's
           counter. */
        if(flags & 0x00000800) {
            en->clients_hit |= 0x80;

            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                           SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_UINT32,
                           en->bp_entry, SCRIPT_ARG_UINT8, en->rt_index,
                           SCRIPT_ARG_UINT8, en->clients_hit, SCRIPT_ARG_END);

            if(en->bp_entry < 0x60 && !(l->flags & LOBBY_FLAG_HAS_NPC))
                ++c->enemy_kills[en->bp_entry];
        }
    }

    return send_lobby_mhit(l, c, mid, mid2, dmg, flags);
}

static void handle_objhit_common(ship_client_t *c, lobby_t *l, uint16_t bid) {
    uint32_t obj_type;

    /* What type of object was hit? */
    if((bid & 0xF000) == 0x4000) {
        /* An object was hit */
        bid &= 0x0FFF;

        /* Make sure the object is in range. */
        if(bid > l->map_objs->count) {
            ERR_LOG("GC %" PRIu32 " hit invalid object "
                  "(%d -- max: %d)!\n"
                  "Episode: %d, Floor: %d, Map: (%d, %d)", c->guildcard,
                  bid, l->map_objs->count, l->episode, c->cur_area,
                  l->maps[c->cur_area << 1],
                  l->maps[(c->cur_area << 1) + 1]);

            if((l->flags & LOBBY_FLAG_QUESTING))
                ERR_LOG("Quest ID: %d, Version: %d", l->qid,
                      l->version);

            /* Just continue on and hope for the best. We've probably got a
               bug somewhere on our end anyway... */
            return;
        }

        /* Make sure it isn't marked as hit already. */
        if((l->map_objs->objs[bid].flags & 0x80000000))
            return;

        /* Now, see if we care about the type of the object that was hit. */
        obj_type = l->map_objs->objs[bid].data.skin & 0xFFFF;

        /* We'll probably want to do a bit more with this at some point, but
           for now this will do. */
        switch(obj_type) {
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
    else if((bid & 0xF000) == 0x1000) {
        /* An enemy was hit. We don't really do anything with these here,
           as there is a better packet for handling them. */
        return;
    }
    else if((bid & 0xF000) == 0x0000) {
        /* An ally was hit. We don't really care to do anything here. */
        return;
    }
    else {
        DBG_LOG("Unknown object type hit: %04" PRIx16 "", bid);
    }
}

static int handle_objhit_phys(ship_client_t *c, subcmd_objhit_phys_t *pkt) {
    lobby_t *l = c->cur_lobby;
    uint8_t i;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " hit object in lobby!",
              c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if(LE16(pkt->hdr.pkt_len) != (sizeof(pkt->hdr) + (pkt->shdr.size << 2)) || pkt->shdr.size < 0x02) {
        ERR_LOG("GC %" PRIu32 " sent bad objhit message!",
              c->guildcard);
        print_payload((unsigned char *)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if(!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);

    /* Handle each thing that was hit */
    for(i = 0; i < pkt->shdr.size - 2; ++i) {
        handle_objhit_common(c, l, LE16(pkt->objects[i].obj_id));
    }

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static inline int tlindex(uint8_t l) {
    switch(l) {
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

static int handle_objhit_tech(ship_client_t *c, subcmd_objhit_tech_t *pkt) {
    lobby_t *l = c->cur_lobby;
    uint8_t tech_level;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " hit object in lobby!",
              c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if(LE16(pkt->hdr.pkt_len) != (4 + (pkt->shdr.size << 2)) || pkt->shdr.size < 0x02) {
        ERR_LOG("GC %" PRIu32 " sent bad objhit message!",
              c->guildcard);
        print_payload((unsigned char *)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Sanity check... Does the character have that level of technique? */
    tech_level = c->pl->v1.character.techniques[pkt->tech];
    if(tech_level == 0xFF) {
        /* This might happen if the user learns a new tech in a team. Until we
           have real inventory tracking, we'll have to fudge this. Once we have
           real, full inventory tracking, this condition should probably
           disconnect the client */
        tech_level = pkt->level;
    }

    if(c->version >= CLIENT_VERSION_DCV2)
        tech_level += c->pl->v1.inv.iitems[pkt->tech].tech;

    if(tech_level < pkt->level) {
        /* This might happen if the user learns a new tech in a team. Until we
           have real inventory tracking, we'll have to fudge this. Once we have
           real, full inventory tracking, this condition should probably
           disconnect the client */
        tech_level = pkt->level;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if(!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);

    /* See what technique was used... */
    switch(pkt->tech) {
        /* These work just like physical hits and can only hit one target, so
           handle them the simple way... */
        case TECHNIQUE_FOIE:
        case TECHNIQUE_ZONDE:
        case TECHNIQUE_GRANTS:
            if(pkt->shdr.size == 3)
                handle_objhit_common(c, l, LE16(pkt->objects[0].obj_id));
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
            ERR_LOG("GC %" PRIu32 " used bad technique: %d",
                  c->guildcard, (int)pkt->tech);
            return -1;
    }

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_objhit(ship_client_t *c, subcmd_bhit_pkt_t *pkt) {
    uint64_t now = get_ms_time();
    lobby_t *l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了箱子!",
              c->guildcard);
        return -1;
    }

    /* We only care about these if the AoE timer is set on the sender. */
    if(c->aoe_timer < now)
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);

    /* Check the type of the object that was hit. As the AoE timer can't be set
       if the objects aren't loaded, this shouldn't ever trip up... */
    if(!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);

    /* Handle the object marked as hit, if appropriate. */
    handle_objhit_common(c, l, LE16(pkt->shdr.object_id));

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_spawn_npc(ship_client_t *c, subcmd_pkt_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("Attempt by GC %" PRIu32 " to spawn NPC in lobby!",
              c->guildcard);
        return -1;
    }

    /* The only quests that allow NPCs to be loaded are those that require there
       to only be one player, so limit that here. Also, we only allow /npc in
       single-player teams, so that'll fall into line too. */
    if(l->num_clients > 1) {
        ERR_LOG("Attempt by GC %" PRIu32 " to spawn NPC in multi-"
              "player team!", c->guildcard);
        return -1;
    }

    /* Either this is a legitimate request to spawn a quest NPC, or the player
       is doing something stupid like trying to NOL himself. We don't care if
       someone is stupid enough to NOL themselves, so send the packet on now. */
    return subcmd_send_lobby_dc(l, c, pkt, 0);
}

static int handle_create_pipe(ship_client_t *c, subcmd_pipe_pkt_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("Attempt by GC %" PRIu32 " to spawn pipe in lobby!",
              c->guildcard);
        return -1;
    }

    /* See if the user is creating a pipe or destroying it. Destroying a pipe
       always matches the created pipe, but sets the area to 0. We could keep
       track of all of the pipe data, but that's probably overkill. For now,
       blindly accept any pipes where the area is 0. */
    if(pkt->area != 0) {
        /* Make sure the user is sending a pipe from the area he or she is
           currently in. */
        if(pkt->area != c->cur_area) {
            ERR_LOG("Attempt by GC %" PRIu32 " to spawn pipe to area "
                "he/she is not in (in: %d, pipe: %d).", c->guildcard,
                c->cur_area, (int)pkt->area);
            return -1;
        }
    }

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static inline int reg_sync_index(lobby_t *l, uint16_t regnum) {
    int i;

    if(!(l->q_flags & LOBBY_QFLAG_SYNC_REGS))
        return -1;

    for(i = 0; i < l->num_syncregs; ++i) {
        if(regnum == l->syncregs[i])
            return i;
    }

    return -1;
}

static int handle_sync_reg(ship_client_t *c, subcmd_sync_reg_t *pkt) {
    lobby_t *l = c->cur_lobby;
    uint32_t val = LE32(pkt->value);
    int done = 0, idx;
    uint32_t ctl;

    /* XXXX: Probably should do some checking here... */
    /* Run the register sync script, if one is set. If the script returns
       non-zero, then assume that it has adequately handled the sync. */
    if((script_execute(ScriptActionQuestSyncRegister, c, SCRIPT_ARG_PTR, c,
                        SCRIPT_ARG_PTR, l, SCRIPT_ARG_UINT8, pkt->register_number,
                        SCRIPT_ARG_UINT32, val, SCRIPT_ARG_END))) {
        done = 1;
    }

    /* Does this quest use global flags? If so, then deal with them... */
    if((l->q_flags & LOBBY_QFLAG_SHORT) && pkt->register_number == l->q_shortflag_reg &&
       !done) {
        /* Check the control bits for sensibility... */
        ctl = (val >> 29) & 0x07;

        /* Make sure the error or response bits aren't set. */
        if((ctl & 0x06)) {
            DBG_LOG("Quest set flag register with illegal ctl!\n");
            send_sync_register(c, pkt->register_number, 0x8000FFFE);
        }
        /* Make sure we don't have anything with any reserved ctl bits set
           (unless a script has already handled the sync). */
        else if((val & 0x17000000)) {
            DBG_LOG("Quest set flag register with reserved ctl!\n");
            send_sync_register(c, pkt->register_number, 0x8000FFFE);
        }
        else if((val & 0x08000000)) {
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
    if((l->q_flags & LOBBY_QFLAG_DATA) && !done) {
        if(pkt->register_number == l->q_data_reg) {
            if(c->q_stack_top < CLIENT_MAX_QSTACK) {
                if(!(c->flags & CLIENT_FLAG_QSTACK_LOCK)) {
                    c->q_stack[c->q_stack_top++] = val;

                    /* Check if we've got everything we expected... */
                    if(c->q_stack_top >= 3 &&
                       c->q_stack_top == 3 + c->q_stack[1] + c->q_stack[2]) {
                        /* Call the function requested and reset the stack. */
                        ctl = quest_function_dispatch(c, l);

                        if(ctl != QUEST_FUNC_RET_NOT_YET) {
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
            else if(c->q_stack_top == CLIENT_MAX_QSTACK) {
                /* Eat the stack push and report an error. */
                send_sync_register(c, pkt->register_number,
                                   QUEST_FUNC_RET_STACK_OVERFLOW);
            }

            done = 1;
        }
        else if(pkt->register_number == l->q_ctl_reg) {
            /* For now, the only reason we'll have one of these is to reset the
               stack. There might be other reasons later, but this will do, for
               the time being... */
            c->q_stack_top = 0;
            done = 1;
        }
    }

    /* Does this register have to be synced? */
    if((idx = reg_sync_index(l, pkt->register_number)) != -1) {
        l->regvals[idx] = val;
    }

    if(!done)
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);

    return 0;
}

static int handle_set_pos24(ship_client_t *c, subcmd_pkt_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* Ugh... For some reason, v1 really likes to send these at the start of
       quests. And by "really likes to send these", I mean that everybody sends
       one of these for themselves and everybody else in the team... That can
       cause some interesting problems if clients are out of sync at the
       beginning of a quest for any reason (like a PSOPC player playing with
       a v1 player might be, for instance). Thus, we have to ignore these sent
       by v1 players with other players' client IDs at the beginning of a
       quest. */
    if(c->version == CLIENT_VERSION_DCV1) {
        /* Sanity check... */
        if(pkt->hdr.dc.pkt_len != LE16(0x0018) || pkt->size != 0x05) {
            ERR_LOG("Client %" PRIu32 " sent invalid setpos24!",
                  c->guildcard);
            return -1;
        }

        /* Oh look, misusing other portions of the client structure so that I
           don't have to make a new field... */
        if(c->autoreply_len && pkt->data[0] != c->client_id) {
            /* Silently drop the packet. */
            --c->autoreply_len;
            return 0;
        }
    }

    return subcmd_send_lobby_dc(l, c, pkt, 0);
}

static int handle_menu_req(ship_client_t *c, subcmd_pkt_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We don't care about these in lobbies. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
    }

    /* Clear the list of dropped items. */
    if(c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    /* Flip the shopping flag, since this packet is sent both for talking to the
       shop in the first place and when the client exits the shop. */
    c->flags ^= CLIENT_FLAG_SHOPPING;
    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_drop_item(ship_client_t *c, subcmd_drop_item_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " dropped item in lobby!",
              c->guildcard);
        return -1;
    }

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if((c->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("GC %" PRIu32 " dropped item while shopping!",
              c->guildcard);
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if(pkt->shdr.size != 0x06)
        return -1;

    /* Are we on Pioneer 2? If so, record the item they just dropped. */
    if(c->cur_area == 0) {
        if(c->p2_drops_max < 30) {
            c->p2_drops[c->p2_drops_max++] = LE32(pkt->item_id);
        }
        else {
            ERR_LOG("GC %" PRIu32 " dropped too many items!"
                  "This is possibly a bug in the server!\n");
        }
    }

    /* Perhaps do more with this at some point when we do inventory tracking? */
    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_drop_stack(ship_client_t *c, subcmd_drop_stack_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " dropped stack in lobby!",
              c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. Note that v1 is missing the stupid
       extra "two" field from the packet. */
    if(pkt->shdr.size != 0x0A && pkt->shdr.size != 0x09)
        return -1;

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if((c->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("GC %" PRIu32 " dropped stack while shopping!",
              c->guildcard);
    }

    /* Perhaps do more with this at some point when we do inventory tracking? */
    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_talk_npc(ship_client_t *c, subcmd_talk_npc_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " talked to NPC in lobby!",
              c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if(pkt->shdr.size != 0x05)
        return -1;

    /* Clear the list of dropped items. */
    if(pkt->unk == 0xFFFF && c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_done_talk_npc(ship_client_t *c, subcmd_end_talk_to_npc_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " finished NPC talk in lobby!",
              c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if(pkt->shdr.size != 0x01)
        return -1;

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
}

static int handle_pick_up(ship_client_t *c, ship_client_t *d, subcmd_pick_up_t *pkt) {
    lobby_t *l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " picked up item in lobby!",
              c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if(pkt->shdr.size != 0x03)
        return -1;

    if(c->cur_area != pkt->area) {
        ERR_LOG("GC %" PRIu32 " picked up item in area they are "
              "not currently in!", c->guildcard);
    }

    /* Clear the list of dropped items. */
    if(c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    /* Maybe do more in the future with inventory tracking? */
    return send_pkt_dc(d, (dc_pkt_hdr_t *)pkt);
}

int handle_burst_pldata(ship_client_t* c, ship_client_t* d,
    subcmd_burst_pldata_t* pkt) {
    int i;
    iitem_t* item;
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " sent burst player data in "
            "lobby!\n", c->guildcard);
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

    return send_pkt_dc(d, (dc_pkt_hdr_t*)pkt);
}

int handle_dragon_act(ship_client_t* c, subcmd_dragon_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int v = c->version, i;
    subcmd_dragon_act_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " reported Dragon action in "
            "lobby!\n", c->guildcard);
        return -1;
    }

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_dragon_act_t));
    tr.x.b = SWAP32(tr.x.b);
    tr.z.b = SWAP32(tr.z.b);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            if (l->clients[i]->version == v) {
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
            }
            else {
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&tr);
            }
        }
    }

    return 0;
}

int handle_gol_dragon_act(ship_client_t* c, subcmd_gol_dragon_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int v = c->version, i;
    subcmd_gol_dragon_act_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " reported Gol Dragon action in "
            "lobby!\n", c->guildcard);
        return -1;
    }

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_gol_dragon_act_t));
    tr.x.b = SWAP32(tr.x.b);
    tr.z.b = SWAP32(tr.z.b);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            if (l->clients[i]->version == v) {
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
            }
            else {
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&tr);
            }
        }
    }

    return 0;
}

/* 处理DC 0x62/0x6D 数据包. */
int subcmd_handle_one(ship_client_t *c, subcmd_pkt_t *pkt) {
    lobby_t *l = c->cur_lobby;
    ship_client_t *dest;
    uint8_t type = pkt->type;
    int rv = -1;

    /* Ignore these if the client isn't in a lobby or team. */
    if(!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    /* Find the destination. */
    dest = l->clients[pkt->hdr.dc.flags];

    /* The destination is now offline, don't bother sending it. */
    if(!dest) {
        pthread_mutex_unlock(&l->mutex);
        return 0;
    }

    /* If there's a burst going on in the lobby, delay most packets */
    if(l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;

        switch(type) {
            case SUBCMD62_BURST1:
            case SUBCMD62_BURST2:
            case SUBCMD62_BURST3:
            case SUBCMD62_BURST4:
                if(l->flags & LOBBY_FLAG_QUESTING)
                    rv = lobby_enqueue_burst(l, c, (dc_pkt_hdr_t *)pkt);
                /* Fall through... */

            case SUBCMD62_BURST5:
            case SUBCMD62_BURST6:
                rv |= send_pkt_dc(dest, (dc_pkt_hdr_t *)pkt);
                break;

            case SUBCMD62_BURST_PLDATA:
                rv = handle_burst_pldata(c, dest, (subcmd_burst_pldata_t*)pkt);
                break;

            default:
                rv = lobby_enqueue_pkt(l, c, (dc_pkt_hdr_t *)pkt);
                break;
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch(type) {
        case SUBCMD62_GUILDCARD:
            /* Make sure the recipient is not ignoring the sender... */
            if(client_has_ignored(dest, c->guildcard)) {
                rv = 0;
                break;
            }

            switch(c->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                    rv = handle_dc_gcsend(c, dest, (subcmd_dc_gcsend_t *)pkt);
                    break;

                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                    rv = handle_gc_gcsend(c, dest, (subcmd_gc_gcsend_t *)pkt);
                    break;

                case CLIENT_VERSION_XBOX:
                    rv = handle_xb_gcsend(c, dest, (subcmd_xb_gcsend_t *)pkt);
                    break;

                case CLIENT_VERSION_PC:
                    rv = handle_pc_gcsend(c, dest, (subcmd_pc_gcsend_t *)pkt);
                    break;
            }
            break;

        case SUBCMD62_ITEMREQ:
        case SUBCMD62_BITEMREQ:
            /* There's only three ways we pay attention to this one: First, if
               the lobby is not in legit mode and a GM has used /item. Second,
               if the lobby has a drop function (for server-side drops). Third,
               if there is a quest going on with modified drops
               我们只有三种方法可以注意到这一点：
               首先，如果大厅不处于合法模式， 并且GM使用了/item。
               第二，如果大厅具有投递功能（用于服务器端投递）。
               第三，如果有一个任务正在进行修改的下降. */
            if(c->next_item[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
                rv = handle_gm_itemreq(c, (subcmd_itemreq_t *)pkt);
            }
            else if(l->dropfunc && (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
                rv = l->dropfunc(c, l, pkt);
            }
            else if((l->num_mtypes || l->num_mids) &&
                    (l->flags & LOBBY_FLAG_QUESTING)) {
                rv = handle_quest_itemreq(c, (subcmd_itemreq_t *)pkt, dest);
            }
            else {
                rv = send_pkt_dc(dest, (dc_pkt_hdr_t *)pkt);
            }
            break;

        case SUBCMD62_PICK_UP:
            rv = handle_pick_up(c, dest, (subcmd_pick_up_t *)pkt);
            break;

        default:
#ifdef LOG_UNKNOWN_SUBS
            DBG_LOG("未知 0x62/0x6D 指令: 0x%02X", type);
            UNK_CSPD(type, c->version, pkt);
            //print_payload((unsigned char *)pkt, LE16(pkt->hdr.dc.pkt_len));
#endif /* LOG_UNKNOWN_SUBS */
            /* Forward the packet unchanged to the destination. */
            rv = send_pkt_dc(dest, (dc_pkt_hdr_t *)pkt);
            break;
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

/* 处理DC 0x60 数据包. */
int subcmd_handle_bcast(ship_client_t *c, subcmd_pkt_t *pkt) {
    uint8_t type = pkt->type;
    lobby_t *l = c->cur_lobby;
    int rv, sent = 1, i;

    /* The DC NTE must be treated specially, so deal with that elsewhere... */
    if(c->version == CLIENT_VERSION_DCV1 && (c->flags & CLIENT_FLAG_IS_NTE))
        return subcmd_dcnte_handle_bcast(c, pkt);

    /* Ignore these if the client isn't in a lobby. */
    if(!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    /* If there's a burst going on in the lobby, delay most packets */
    if(l->flags & LOBBY_FLAG_BURSTING) {
        switch(type) {
            case SUBCMD60_LOAD_3B:
            case SUBCMD60_BURST_DONE:
                rv = subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);
                break;

            case SUBCMD60_SET_AREA:
                rv = handle_set_area(c, (subcmd_set_area_t *)pkt);
                break;

            case SUBCMD60_SET_POS_3F:
                rv = handle_set_pos(c, (subcmd_set_pos_t *)pkt);
                break;

            case SUBCMD60_CMODE_GRAVE:
                rv = handle_cmode_grave(c, pkt);
                break;

            default:
                rv = lobby_enqueue_pkt(l, c, (dc_pkt_hdr_t *)pkt);
                break;
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch(type) {
        case SUBCMD60_TAKE_ITEM:
            rv = handle_take_item(c, (subcmd_take_item_t *)pkt);
            break;

        case SUBCMD60_LEVEL_UP:
            rv = handle_level_up(c, (subcmd_level_up_t*)pkt);
            break;

        case SUBCMD60_USED_TECH:
            rv = handle_used_tech(c, (subcmd_used_tech_t *)pkt);
            break;

        case SUBCMD60_TAKE_DAMAGE1:
        case SUBCMD60_TAKE_DAMAGE2:
            rv = handle_take_damage(c, (subcmd_take_damage_t *)pkt);
            break;

        case SUBCMD60_BOX_ENEMY_ITEM_DROP:
            rv = handle_itemdrop(c, (subcmd_itemgen_t *)pkt);
            break;

        case SUBCMD60_SET_AREA:
            rv = handle_set_area(c, (subcmd_set_area_t*)pkt);
            break;

        case SUBCMD60_INTER_LEVEL_WARP:
            rv = handle_inter_level_warp(c, (subcmd_inter_level_warp_t*)pkt);
            break;

        case SUBCMD60_SET_POS_3E:
        case SUBCMD60_SET_POS_3F:
            rv = handle_set_pos(c, (subcmd_set_pos_t *)pkt);
            break;

        case SUBCMD60_MOVE_SLOW:
        case SUBCMD60_MOVE_FAST:
            rv = handle_move(c, (subcmd_move_t *)pkt);
            break;

        case SUBCMD60_DELETE_ITEM:
            rv = handle_delete_inv(c, (subcmd_destroy_item_t *)pkt);
            break;

        case SUBCMD60_BUY:
            rv = handle_buy(c, (subcmd_buy_t *)pkt);
            break;

        case SUBCMD60_USE_ITEM:
            rv = handle_use_item(c, (subcmd_use_item_t *)pkt);
            break;

        case SUBCMD60_WORD_SELECT:
            rv = handle_word_select(c, (subcmd_word_select_t *)pkt);
            break;

        case SUBCMD60_SYMBOL_CHAT:
            rv = handle_symbol_chat(c, (subcmd_symbol_chat_t *)pkt);
            break;

        case SUBCMD60_CMODE_GRAVE:
            rv = handle_cmode_grave(c, pkt);
            break;

        case SUBCMD60_HIT_MONSTER:
            rv = handle_mhit(c, (subcmd_mhit_pkt_t *)pkt);
            break;

        case SUBCMD60_OBJHIT_PHYS:
            /* Don't even try to deal with these in battle or challenge mode. */
            if(l->challenge || l->battle) {
                sent = 0;
                break;
            }

            rv = handle_objhit_phys(c, (subcmd_objhit_phys_t *)pkt);
            break;

        case SUBCMD60_OBJHIT_TECH:
            /* Don't even try to deal with these in battle or challenge mode. */
            if(l->challenge || l->battle) {
                sent = 0;
                break;
            }

            rv = handle_objhit_tech(c, (subcmd_objhit_tech_t *)pkt);
            break;

        case SUBCMD60_HIT_OBJ:
            /* Don't even try to deal with these in battle or challenge mode. */
            if(l->challenge || l->battle) {
                sent = 0;
                break;
            }

            rv = handle_objhit(c, (subcmd_bhit_pkt_t *)pkt);
            break;

        case SUBCMD60_SPAWN_NPC:
            rv = handle_spawn_npc(c, pkt);
            break;

        case SUBCMD60_CREATE_PIPE:
            rv = handle_create_pipe(c, (subcmd_pipe_pkt_t *)pkt);
            break;

        case SUBCMD60_SYNC_REG:
            rv = handle_sync_reg(c, (subcmd_sync_reg_t *)pkt);
            break;

        case SUBCMD60_SET_POS_24:
            rv = handle_set_pos24(c, pkt);
            break;

        case SUBCMD60_MENU_REQ:
            rv = handle_menu_req(c, pkt);
            break;

        case SUBCMD60_DROP_ITEM:
            rv = handle_drop_item(c, (subcmd_drop_item_t *)pkt);
            break;

        case SUBCMD60_DROP_STACK:
            rv = handle_drop_stack(c, (subcmd_drop_stack_t *)pkt);
            break;

        case SUBCMD60_TALK_NPC:
            rv = handle_talk_npc(c, (subcmd_talk_npc_t *)pkt);
            break;

        case SUBCMD60_DONE_NPC:
            rv = handle_done_talk_npc(c, (subcmd_end_talk_to_npc_t*)pkt);
            break;

        case SUBCMD60_DRAGON_ACT:
            rv = handle_dragon_act(c, (subcmd_dragon_act_t*)pkt);
            break;

        case SUBCMD60_GDRAGON_ACT:
            rv = handle_gol_dragon_act(c, (subcmd_gol_dragon_act_t*)pkt);
            break;

        default:
#ifdef LOG_UNKNOWN_SUBS
            DBG_LOG("未知 0x60 指令: 0x%02X", type);
            UNK_CSPD(type, c->version, pkt);
            //print_payload((unsigned char *)pkt, LE16(pkt->hdr.dc.pkt_len));
#endif /* LOG_UNKNOWN_SUBS */
            sent = 0;
            break;

        case SUBCMD60_FINISH_LOAD:
            if(l->type == LOBBY_TYPE_LOBBY) {
                for(i = 0; i < l->max_clients; ++i) {
                    if(l->clients[i] && l->clients[i] != c &&
                       subcmd_send_pos(c, l->clients[i])) {
                        rv = -1;
                        break;
                    }
                }
            }

        case SUBCMD60_LOAD_22:
        case SUBCMD60_LOAD_3B:
        case SUBCMD60_WARP_55:
        case SUBCMD60_LOBBY_ACTION:
        case SUBCMD60_GOGO_BALL:
        case SUBCMD60_LOBBY_CHAIR:
        case SUBCMD60_CHAIR_DIR:
        case SUBCMD60_CHAIR_MOVE:
            sent = 0;
            break;
    }

    /* Broadcast anything we don't care to check anything about. */
    if(!sent)
        rv = subcmd_send_lobby_dc(l, c, (subcmd_pkt_t *)pkt, 0);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

/* 处理EP3 0xC9/0xCB 数据包. */
int subcmd_handle_ep3_bcast(ship_client_t *c, subcmd_pkt_t *pkt) {
    lobby_t *l = c->cur_lobby;
    int rv;

    /* Ignore these if the client isn't in a lobby or team. */
    if(!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    /* We don't do anything special with these just yet... */
    rv = lobby_send_pkt_ep3(l, c, (dc_pkt_hdr_t *)pkt);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

int subcmd_send_lobby_item(lobby_t *l, subcmd_itemreq_t *req,
                           const uint32_t item[4]) {
    subcmd_itemgen_t gen = { 0 };
    int i;
    uint32_t tmp = LE32(req->unk2[0]) & 0x0000FFFF;

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

    gen.data.item.data_l[0] = LE32(item[0]);
    gen.data.item.data_l[1] = LE32(item[1]);
    gen.data.item.data_l[2] = LE32(item[2]);
    gen.data.item.data2_l = LE32(item[3]);
    gen.data.item2 = LE32(0x00000002);

    gen.data.item.item_id = LE32(l->next_game_item_id);
    ++l->next_game_item_id;

    /* Send the packet to every client in the lobby. */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&gen);
        }
    }

    return 0;
}

int subcmd_send_lobby_dc(lobby_t *l, ship_client_t *c, subcmd_pkt_t *pkt,
                         int igcheck) {
    int i;

    /* Send the packet to every connected client. */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] && l->clients[i] != c) {
            /* If we're supposed to check the ignore list, and this client is on
               it, don't send the packet. */
            if(igcheck && client_has_ignored(l->clients[i], c->guildcard)) {
                continue;
            }

            if(l->clients[i]->version != CLIENT_VERSION_DCV1 ||
               !(l->clients[i]->flags & CLIENT_FLAG_IS_NTE))
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)pkt);
            else
                subcmd_translate_dc_to_nte(l->clients[i], pkt);
        }
    }

    return 0;
}

int subcmd_send_pos(ship_client_t *dst, ship_client_t *src) {
    subcmd_set_pos_t dc = { 0 };
    subcmd_bb_set_pos_t bb = { 0 };

    if(dst->version == CLIENT_VERSION_BB) {
        bb.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
        bb.hdr.flags = 0;
        bb.hdr.pkt_len = LE16(0x0020);
        bb.shdr.type = 0x20;
        bb.shdr.size = 6;
        bb.shdr.client_id = src->client_id;
        dc.shdr.size = 6;
        dc.shdr.client_id = src->client_id;
        bb.area = LE32(0x0000000F);         /* Area */
        bb.w = src->x;                      /* X */
        bb.x = 0;                           /* Y */
        bb.y = src->z;                      /* Z */
        bb.z = 0;                           /* Facing, perhaps? */

        return send_pkt_bb(dst, (bb_pkt_hdr_t *)&bb);
    }
    else {
        dc.hdr.pkt_type = GAME_COMMAND0_TYPE;
        dc.hdr.flags = 0;
        dc.hdr.pkt_len = LE16(0x001C);

        if(dst->version == CLIENT_VERSION_DCV1 &&
           (dst->flags & CLIENT_FLAG_IS_NTE))
            dc.shdr.type = 0x1C;
        else
            dc.shdr.type = 0x20;

        dc.shdr.size = 6;
        dc.shdr.client_id = src->client_id;
        dc.area = LE32(0x0000000F);         /* Area */
        dc.w = src->x;                      /* X */
        dc.x = 0;                           /* Y */
        dc.y = src->z;                      /* Z */
        dc.z = 0;                           /* Facing, perhaps? */

        return send_pkt_dc(dst, (dc_pkt_hdr_t*)&dc);
    }
}
