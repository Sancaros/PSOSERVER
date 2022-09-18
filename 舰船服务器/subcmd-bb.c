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

/* Forward declarations */
static int subcmd_send_drop_stack(ship_client_t *c, uint32_t area, float x,
                                  float z, iitem_t *item);
static int subcmd_send_create_item(ship_client_t* c, item_t item, int send_to_client);
static int subcmd_send_destroy_map_item(ship_client_t *c, uint8_t area,
                                        uint32_t item_id);
static int subcmd_send_destroy_item(ship_client_t *c, uint32_t item_id,
                                    uint8_t amt);

#define SWAP32(x) (((x >> 24) & 0x00FF) | \
                   ((x >>  8) & 0xFF00) | \
                   ((x & 0xFF00) <<  8) | \
                   ((x & 0x00FF) << 24))

int subcmd_send_lobby_bb(lobby_t* l, ship_client_t* c, bb_subcmd_pkt_t* pkt, int igcheck) {
    int i;

    /* Send the packet to every connected client. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            /* If we're supposed to check the ignore list, and this client is on
               it, don't send the packet
               如果我们要检查忽略列表，并且该客户端在其中，请不要发送数据包. */
            if (igcheck && client_has_ignored(l->clients[i], c->guildcard)) {
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

static int handle_bb_cmd_check_size(ship_client_t* c, bb_subcmd_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! size %02X",
            c->guildcard, pkt->type, size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_lobby_size(ship_client_t* c, bb_subcmd_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅 %s 指令!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! size %02X",
            c->guildcard, pkt->type, size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_game_size(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间内 %s 指令!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //UNK_CPD(pkt->type, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_client_id(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (pkt->data[2] != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间内 %s 指令!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //UNK_CPD(pkt->type, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_gcsend(ship_client_t* s, ship_client_t* d) {
    size_t in, out;
    char* inptr;
    char* outptr;

    /*ERR_LOG(
        "handle_bb_gcsend %d 版本识别 = %d",
        d->sock, d->version);*/

        /* This differs based on the destination client's version. */
    switch (d->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    {
        subcmd_dc_gcsend_t dc;

        memset(&dc, 0, sizeof(dc));

        /* Convert the name (UTF-16 -> ASCII). */
        memset(&dc.name, '-', 16);
        in = 48;
        out = 24;
        inptr = (char*)&s->pl->bb.character.name[2];
        outptr = dc.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)s->bb_pl->guildcard_desc;
        outptr = dc.text;

        if (s->bb_pl->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        dc.hdr.pkt_type = GAME_COMMAND2_TYPE;
        dc.hdr.flags = (uint8_t)d->client_id;
        dc.hdr.pkt_len = LE16(0x0088);
        dc.type = SUBCMD_GUILDCARD;
        dc.size = 0x21;
        dc.unused = 0;
        dc.tag = LE32(0x00010000);
        dc.guildcard = LE32(s->guildcard);
        dc.unused2 = 0;
        dc.one = 1;
        dc.language = s->language_code;
        dc.section = s->pl->bb.character.disp.dress_data.section;
        dc.char_class = s->pl->bb.character.disp.dress_data.ch_class;
        dc.padding[0] = dc.padding[1] = dc.padding[2] = 0;

        return send_pkt_dc(d, (dc_pkt_hdr_t*)&dc);
    }

    case CLIENT_VERSION_PC:
    {
        subcmd_pc_gcsend_t pc;

        /* Don't allow guild cards to be sent to PC NTE, as it doesn't
           support them. */
        if ((d->flags & CLIENT_FLAG_IS_NTE))
            return send_txt(s, "%s", __(s, "\tE\tC7Cannot send Guild\n"
                "Card to that user."));

        memset(&pc, 0, sizeof(pc));

        /* First the name and text... */
        memcpy(pc.name, &s->pl->bb.character.name[2], BB_CHARACTER_NAME_LENGTH * 2 - 4);
        memcpy(pc.text, s->bb_pl->guildcard_desc, 176);

        /* Copy the rest over. */
        pc.hdr.pkt_type = GAME_COMMAND2_TYPE;
        pc.hdr.flags = (uint8_t)d->client_id;
        pc.hdr.pkt_len = LE16(0x00F8);
        pc.type = SUBCMD_GUILDCARD;
        pc.size = 0x3D;
        pc.unused = 0;
        pc.tag = LE32(0x00010000);
        pc.guildcard = LE32(s->guildcard);
        pc.padding = 0;
        pc.one = 1;
        pc.language = s->language_code;
        pc.section = s->pl->bb.character.disp.dress_data.section;
        pc.char_class = s->pl->bb.character.disp.dress_data.ch_class;

        return send_pkt_dc(d, (dc_pkt_hdr_t*)&pc);
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
        inptr = (char*)&s->pl->bb.character.name[2];
        outptr = gc.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 88;
        inptr = (char*)s->bb_pl->guildcard_desc;
        outptr = gc.text;

        if (s->bb_pl->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        gc.hdr.pkt_type = GAME_COMMAND2_TYPE;
        gc.hdr.flags = (uint8_t)d->client_id;
        gc.hdr.pkt_len = LE16(0x0098);
        gc.type = SUBCMD_GUILDCARD;
        gc.size = 0x25;
        gc.unused = 0;
        gc.tag = LE32(0x00010000);
        gc.guildcard = LE32(s->guildcard);
        gc.padding = 0;
        gc.one = 1;
        gc.language = s->language_code;
        gc.section = s->pl->bb.character.disp.dress_data.section;
        gc.char_class = s->pl->bb.character.disp.dress_data.ch_class;

        return send_pkt_dc(d, (dc_pkt_hdr_t*)&gc);
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
        inptr = (char*)&s->pl->bb.character.name[2];
        outptr = xb.name;
        iconv(ic_utf16_to_ascii, &inptr, &in, &outptr, &out);

        /* Convert the text (UTF-16 -> ISO-8859-1 or SHIFT-JIS). */
        in = 176;
        out = 512;
        inptr = (char*)s->bb_pl->guildcard_desc;
        outptr = xb.text;

        if (s->bb_pl->guildcard_desc[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Copy the rest over. */
        xb.hdr.pkt_type = GAME_COMMAND2_TYPE;
        xb.hdr.flags = (uint8_t)d->client_id;
        xb.hdr.pkt_len = LE16(0x0234);
        xb.type = SUBCMD_GUILDCARD;
        xb.size = 0x8C;
        xb.unk = LE16(0xFB0D);
        xb.tag = LE32(0x00010000);
        xb.guildcard = LE32(s->guildcard);
        xb.xbl_userid = LE64(s->guildcard);
        xb.one = 1;
        xb.language = s->language_code;
        xb.section = s->pl->bb.character.disp.dress_data.section;
        xb.char_class = s->pl->bb.character.disp.dress_data.ch_class;

        return send_pkt_dc(d, (dc_pkt_hdr_t*)&xb);
    }

    case CLIENT_VERSION_BB:
    {
        subcmd_bb_gcsend_t bb;

        /* Fill in the packet... */
        memset(&bb, 0, sizeof(subcmd_bb_gcsend_t));
        bb.hdr.pkt_len = LE16(0x0114);
        bb.hdr.pkt_type = LE16(GAME_COMMAND2_TYPE);
        bb.hdr.flags = LE32(d->client_id);
        bb.type = SUBCMD_GUILDCARD;
        bb.size = 0x43;
        bb.guildcard = LE32(s->guildcard);
        memcpy(bb.name, s->pl->bb.character.name, BB_CHARACTER_NAME_LENGTH * 2);
        memcpy(bb.guild_name, s->bb_opts->guild_name, 32);
        memcpy(bb.text, s->bb_pl->guildcard_desc, 176);
        bb.one = 1;
        bb.language = s->language_code;
        bb.section = s->pl->bb.character.disp.dress_data.section;
        bb.char_class = s->pl->bb.character.disp.dress_data.ch_class;

        return send_pkt_bb(d, (bb_pkt_hdr_t*)&bb);
    }
    }

    return 0;
}

static int handle_bb_pick_up(ship_client_t* c, subcmd_bb_pick_up_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found;
    uint32_t item, tmp;
    iitem_t iitem_data;
    uint32_t ic[3];

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中拾取了物品!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03 ||
        pkt->client_id != c->client_id) {
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
        item = LE32(iitem_data.data_l[0]);

        /* Is it meseta, or an item? */
        if (item == Item_Meseta) {
            tmp = LE32(iitem_data.data2_l) + LE32(c->bb_pl->character.disp.meseta);

            /* Cap at 999,999 meseta. */
            if (tmp > 999999)
                tmp = 999999;

            c->bb_pl->character.disp.meseta = LE32(tmp);
            c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
        }
        else {
            iitem_data.flags = 0;
            iitem_data.equipped = LE16(1);
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
    memcpy(ic, iitem_data.data_l, 3 * sizeof(uint32_t));
    subcmd_send_create_item(c, /*ic, */iitem_data.data/*, iitem_data.data2_l*/, 1);

    return subcmd_send_destroy_map_item(c, pkt->area, iitem_data.item_id);
}

static int handle_bb_gm_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req) {
    subcmd_bb_itemgen_t gen;
    int r = LE16(req->req);
    int i;
    lobby_t* l = c->cur_lobby;

    /* Fill in the packet we'll send out. */
    gen.hdr.pkt_type = GAME_COMMAND0_TYPE;
    gen.hdr.flags = 0;
    gen.hdr.pkt_len = LE16(0x30);
    gen.type = SUBCMD_ITEMDROP;
    gen.size = 0x0B;
    gen.params = 0;
    gen.area = req->area;
    gen.what = 0x02;
    gen.req = req->req;
    gen.x = req->x;
    gen.y = req->y;
    gen.unk1 = LE32(0x00000010);

    gen.item.data_l[0] = LE32(c->next_item[0]);
    gen.item.data_l[1] = LE32(c->next_item[1]);
    gen.item.data_l[2] = LE32(c->next_item[2]);
    gen.item.data2_l = LE32(c->next_item[3]);
    /*gen.item2[0] = LE32(c->next_item[3]);
    gen.item2[1] = LE32(0x00000002);*/

    /* Obviously not "right", but it works though, so we'll go with it. */

    gen.item.item_id = LE32((r | 0x06010100));

    /* Send the packet to every client in the lobby. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]) {
            send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&gen);
        }
    }

    /* Clear this out. */
    c->next_item[0] = c->next_item[1] = c->next_item[2] = c->next_item[3] = 0;

    return 0;
}

static int handle_bb_quest_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req, ship_client_t* dest) {
    uint32_t mid = LE16(req->req);
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

static int handle_bb_item_req(ship_client_t* c, ship_client_t* d, lobby_t* l, subcmd_bb_itemreq_t* pkt) {
    //lobby_t* l = c->cur_lobby;
      /*[2022年08月23日 00:55 : 07 : 571] 错误(402) : GC 42004063 发送损坏的数据指令 0x60!数据大小 06
        (00000000)   20 00 62 00 00 00 00 00  60 06 00 00 01 05 72 00.b.....`.....r.
        (00000010)   60 EA 60 43 4B 07 A1 43  0B 00 00 00 01 E7 E7 1E    `.`CK..C........*/
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->size != 0x06) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            c->guildcard, pkt->type, pkt->size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    if (c->next_item[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        return handle_bb_gm_itemreq(c, (subcmd_bb_itemreq_t*)pkt);
    }
    else if (l->dropfunc && (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
        return l->dropfunc(c, l, pkt);
    }
    else if ((l->num_mtypes || l->num_mids) &&
        (l->flags & LOBBY_FLAG_QUESTING)) {
        return handle_bb_quest_itemreq(c, (subcmd_bb_itemreq_t*)pkt, d);
    }
    else {
        return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
    }

    //DBG_LOG("%s", c_cmd_name(pkt->hdr.pkt_type, 0));
    //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
    //return l->dropfunc(c, l, pkt);
    return -1;
}

static int handle_bb_bitem_req(ship_client_t* c, ship_client_t* d, lobby_t* l, subcmd_bb_bitemreq_t* pkt) {
    //lobby_t* l = c->cur_lobby;
      /*[2022年08月23日 00:54 : 27 : 942] 调试(836) : BB处理 GC 42004063 62 / 6D指令 : 0xA2
        ( 00000000 )   30 00 62 00 00 00 00 00  A2 0A 80 3F 01 30 80 00    0.b........ ? .0..
        ( 00000010 )   29 56 EA 43 61 E8 CF 43  10 00 01 00 01 E5 E7 1E    )V.Ca..C........
        ( 00000020 )   00 00 80 3F 00 00 00 00  00 00 00 00 00 00 00 00    ... ? ............

        [2022年08月23日 00:54 : 30 : 647] 调试(836) : BB处理 GC 42004063 62 / 6D指令 : 0xA2
        ( 00000000 )   30 00 62 00 00 00 00 00  A2 0A 80 3F 01 30 7F 00    0.b........ ? .0..
        ( 00000010 )   29 56 E1 43 61 68 D2 43  10 00 01 00 01 E4 E7 1E    )V.Cah.C........
        ( 00000020 )   00 00 80 3F 00 00 00 00  00 00 00 00 00 00 00 00    ... ? ............

        [2022年08月23日 01:03:14:944] 调试(845): BB处理 GC 42004063 62/6D指令: 0xA2
        生成 盾 shield
        ( 00000000 )   30 00 62 00 00 00 00 00  A2 0A 80 3F 01 30 36 00    0.b........?.06.
        ( 00000010 )   76 48 E2 43 85 52 C1 C3  04 00 01 00 01 71 E7 1E    vH.C.R.......q..
        ( 00000020 )   02 00 80 3F FF 00 00 00  00 00 00 00 01 00 00 00    ...?............

        [2022年08月23日 01:03:11:011] 调试(845): BB处理 GC 42004063 62/6D指令: 0xA2
        生成 铠甲 armor
        ( 00000000 )   30 00 62 00 00 00 00 00  A2 0A 80 3F 01 30 37 00    0.b........?.07.
        ( 00000010 )   5F 1C DA 43 57 02 C8 C3  04 00 01 00 01 EB E7 1E    _..CW...........
        ( 00000020 )   02 00 80 3F FF 00 00 00  00 00 00 00 01 00 00 00    ...?............
        */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0030) || pkt->size != 0x0A) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            c->guildcard, pkt->type, pkt->size);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    if (c->next_item[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        return handle_bb_gm_itemreq(c, (subcmd_bb_itemreq_t*)pkt);
    }
    else if (l->dropfunc && (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
        return l->dropfunc(c, l, pkt);
    }
    else if ((l->num_mtypes || l->num_mids) &&
        (l->flags & LOBBY_FLAG_QUESTING)) {
        return handle_bb_quest_itemreq(c, (subcmd_bb_itemreq_t*)pkt, d);
    }
    else {
        return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
    }

    //DBG_LOG("%s", c_cmd_name(pkt->hdr.pkt_type, 0));
    //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
    //return l->dropfunc(c, l, pkt);
    return -1;
}

static int handle_bb_trade(ship_client_t* c, ship_client_t* d, subcmd_bb_trade_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = -1;

    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return rv;
    }

    if (pkt->size != 0x04) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! 数据大小 %02X",
            c->guildcard, pkt->type, pkt->size);
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

// 向玩家发送货物清单
int subcmd_bb_send_shop(ship_client_t* c, uint8_t shop_type, uint8_t num_items) {
    block_t* b = c->cur_block;
    subcmd_bb_shop_inv_t *shop;
    //memset(&shop, 0, sizeof(shop));
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

        /*ITEM_LOG("纯物品字节数据为: %s %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
            sitem_get_name(&c->game_data.shop_items[i], c->version), 
            c->game_data.shop_items[i].data1b[0], c->game_data.shop_items[i].data1b[1], c->game_data.shop_items[i].data1b[2], c->game_data.shop_items[i].data1b[3],
            c->game_data.shop_items[i].data1b[4], c->game_data.shop_items[i].data1b[5], c->game_data.shop_items[i].data1b[6], c->game_data.shop_items[i].data1b[7],
            c->game_data.shop_items[i].data1b[8], c->game_data.shop_items[i].data1b[9], c->game_data.shop_items[i].data1b[10], c->game_data.shop_items[i].data1b[11],
            c->game_data.shop_items[i].costb[0], c->game_data.shop_items[i].costb[1], c->game_data.shop_items[i].costb[2], c->game_data.shop_items[i].costb[3]);*/

        shop->items[i] = c->game_data.shop_items[i];
    }

    shop->hdr.pkt_len = LE16(len); //236 - 220 11 * 20 = 16 + num_items * sizeof(sitem_t)
    shop->hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    shop->hdr.flags = 0;
    shop->type = SUBCMD_SHOP_INV;
    shop->size = 0x2C;
    shop->params = LE16(0x037F);
    shop->shop_type = shop_type;
    shop->num_items = num_items;

    //print_payload((unsigned char*)&shop, LE16(shop.hdr.pkt_len));

    rv = send_pkt_bb(c, (bb_pkt_hdr_t*)shop);
    free_safe(shop);
    return rv;
}

static int handle_bb_shop_req(ship_client_t* c, subcmd_bb_shop_req_t* req) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    uint32_t shop_type = LE32(req->shop_type);

    //print_payload((unsigned char*)req, LE16(req->hdr.pkt_len));

    if (c->version == CLIENT_VERSION_BB) {
        uint8_t num_items = 9 + (rand() % 4);

        //printf("%d \n", num_items);

        for (uint8_t i = 0; i < num_items; ++i) {
            memset(&c->game_data.shop_items[i], 0, sizeof(sitem_t));

            sitem_t item_data;
            if (shop_type == 0) { // 工具商店
                item_data = create_bb_shop_item(l->difficulty, 3);
            }
            else if (shop_type == 1) { // 武器商店
                item_data = create_bb_shop_item(l->difficulty, 0);
            }
            else if (shop_type == 2) { // 装甲商店
                item_data = create_bb_shop_item(l->difficulty, 1);
            }
            else { // 未知商店... 先留空
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
        shop.type = SUBCMD_SHOP_INV;
        shop.size = 0x3B;
        shop.shop_type = req->shop_type;
        shop.num_items = 0x0B;

        for (i = 0; i < 0x0B; ++i) {
            shop.items[i].data1l[0] = LE32((0x03 | (i << 8)));
            shop.items[i].sitem_id = 0xFFFFFFFF;
            shop.items[i].costl = LE32((mt19937_genrand_int32(&b->rng) % 255));
        }

        return send_pkt_bb(c, (bb_pkt_hdr_t*)&shop);
    }

    return 0;
}

//从客户端移除美赛塔
void Delete_Meseta_From_Client(ship_client_t* c, uint32_t count, uint32_t drop)
{
    uint32_t stack_count/*, newItemNum*/;
    lobby_t* l;

    if (!c->cur_lobby)
        return;

    l = c->cur_lobby;

    stack_count = c->bb_pl->character.disp.meseta;
    if (stack_count < count)
    {
        c->bb_pl->character.disp.meseta = 0;
        count = stack_count;
    }
    else
        c->bb_pl->character.disp.meseta -= count;

    //Item_Create(); 相同类似的代码
    //掉落美赛塔 11.18
    //if (drop)
    //{
    //    memset(&PacketData[0x00], 0, 0x2C);
    //    PacketData[0x00] = 0x2C;
    //    PacketData[0x01] = 0x00;
    //    PacketData[0x02] = 0x60;
    //    PacketData[0x03] = 0x00;
    //    PacketData[0x08] = 0x5D;
    //    PacketData[0x09] = 0x09;
    //    PacketData[0x0A] = c->client_id;
    //    //*(uint32_t*)&PacketData[0x0C] = client->drop_area;
    //    //*(int64_t*)&PacketData[0x10] = client->drop_coords;
    //    *(uint32_t*)&PacketData[0x0C] = l->floor[client->clientID];
    //    *(uint32_t*)&PacketData[0x10] = l->clientx[client->clientID];
    //    *(uint32_t*)&PacketData[0x14] = l->clienty[client->clientID];
    //    PacketData[0x18] = 0x04;
    //    PacketData[0x19] = 0x00;
    //    PacketData[0x1A] = 0x00;
    //    PacketData[0x1B] = 0x00;
    //    memset(&PacketData[0x1C], 0, 0x08);
    //    //PacketData[0x1C] = 0x00;
    //    //PacketData[0x1D] = 0x00;
    //    //PacketData[0x1E] = 0x00;
    //    //PacketData[0x1F] = 0x00;
    //    //PacketData[0x20] = 0x00;
    //    //PacketData[0x21] = 0x00;
    //    //PacketData[0x22] = 0x00;
    //    //PacketData[0x23] = 0x00;
    //    *(uint32_t*)&PacketData[0x24] = l->playerItemID[client->clientID];
    //    *(uint32_t*)&PacketData[0x28] = count;
    //    Send_Packet_To_Lobby(client->Lobby_Room, 4, &PacketData[0], 0x2C, 0);

    //    // 生成新的游戏物品...

    //    newItemNum = Free_Game_Item(l);
    //    if (l->gameItemCount < MAX_SAVED_ITEMS)
    //        l->gameItemList[l->gameItemCount++] = newItemNum;
    //    memcpy(&l->gameItem[newItemNum].item.item_data1[0], &PacketData[0x18], 12);
    //    *(uint32_t*)&l->gameItem[newItemNum].item.item_data2[0] = count;
    //    l->gameItem[newItemNum].item.item_id = l->playerItemID[client->clientID];
    //    l->playerItemID[client->clientID]++;
    //}
}

static int handle_bb_shop_buy(ship_client_t* c, subcmd_bb_shop_buy_t* pkt) {
    lobby_t* l = c->cur_lobby;
    sitem_t *shopi;

    print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品购买数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    DBG_LOG("购买物品");
    shopi = (sitem_t*)malloc(sizeof(sitem_t));

    if (!shopi) {
        ERR_LOG("无法分配商店物品数据内存空间");
        ERR_LOG("%s", strerror(errno));
    }

    DBG_LOG("购买物品");
    //shopi = &c->game_data.shop_items[pkt->shop_index];
    memcpy(shopi, &c->game_data.shop_items[pkt->shop_index], sizeof(sitem_t));

    print_payload((unsigned char*)shopi, sizeof(sitem_t));

    //[2022年09月07日 05:23:24:086] 物品(1177): 纯物品字节数据为: 替身人偶     03090000, 00000000, 00000000, 00000000
    //替身人偶
//( 00000000 )   03 09 00 00 00 00 00 00  00 00 00 00 13 00 00 00    .    ..............
//( 00000010 )   00 00 00 00                                         ....

    ITEM_LOG("纯物品字节数据为: %s %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
        sitem_get_name(shopi, c->version),
        shopi->data1b[0], shopi->data1b[1], shopi->data1b[2], shopi->data1b[3],
        shopi->data1b[4], shopi->data1b[5], shopi->data1b[6], shopi->data1b[7],
        shopi->data1b[8], shopi->data1b[9], shopi->data1b[10], shopi->data1b[11],
        shopi->costb[0], shopi->costb[1], shopi->costb[2], shopi->costb[3]);


    //if ((pkt->item_id > 1) && (shopi->data1b[0] != 0x03)) {
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
            DBG_LOG("购买物品");
            iitem_t *ii;

            DBG_LOG("购买物品");

            ii = (iitem_t*)malloc(sizeof(iitem_t));

            //memset(&ii, 0, sizeof(inventory_t));
            DBG_LOG("购买物品");
            //memcpy(&ii.data, &shopi->data, sizeof(ii.data));

            ii->data = shopi->data;



            DBG_LOG("购买物品");

            //subcmd_send_create_item(c, /*ic, */ii->data/*, iitem_data.data2_l*/, 1);
            DBG_LOG("购买物品");
            // Update player item ID
            l->next_item_id[c->client_id] = pkt->item_id;
            DBG_LOG("购买物品");
            ii->item_id = l->next_item_id[c->client_id]++;

            DBG_LOG("购买物品");
            item_add_to_inv(c->bb_pl->inv.iitems,
                c->bb_pl->inv.item_count, ii);
            DBG_LOG("购买物品");
            //Add_To_Inventory(&i, client->decrypt_buf_code[0x12], 1, client);
            Delete_Meseta_From_Client(c, shopi->costl, 0);
            DBG_LOG("购买物品");
        //}

    DBG_LOG("购买物品");
    free_safe(shopi);
    free_safe(ii);

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_item_tekk(ship_client_t* c, subcmd_bb_tekk_item_t* pkt) {
    lobby_t* l = c->cur_lobby;


    if (l->version == CLIENT_VERSION_BB) {

        if (l->type == LOBBY_TYPE_DEFAULT) {
            ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
                c->guildcard);
            return -1;
        }

        //( 00000000 )   10 00 62 00 00 00 00 00  B8 02 FF FF 0E 00 01 00    ..b.............

        if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->size != 0x02/* || pkt->client_id != c->client_id*/) {
            ERR_LOG("GC %" PRIu32 " 发送损坏的物品鉴定数据!",
                c->guildcard);
            print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
            return -1;
        }
        
        /*if (!(l->flags & Lobby::Flag::ITEM_TRACKING_ENABLED)) {
            throw logic_error("item tracking not enabled in BB game");
        }*/

        if (!c->game_data.identify_result.item_id) {
            ERR_LOG("GC %" PRIu32 " 未发送需要鉴定的物品!",
                c->guildcard);
        }
        if (c->game_data.identify_result.item_id != pkt->item.item_id) {
            ERR_LOG("GC %" PRIu32 " accepted item ID does not match previous identify request!",
                c->guildcard);
        }

        //item_add_to_inv(&c->bb_pl->inv, c->bb_pl->inv.item_count, &c->game_data.identify_result);
        c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;
        //c->game_data.player()->add_item(c->game_data.identify_result);
        //subcmd_send_create_inventory_item(l, c, c->game_data.identify_result.data);
        //c->game_data.identify_result.clear();
        clear_iitem(&c->game_data.identify_result);

    }
    else {
        return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
        //forward_subcommand(l, c, command, flag, data);
    }









    print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
}

static int handle_bb_bank(ship_client_t* c, subcmd_bb_bank_open_t* req) {
    lobby_t* l = c->cur_lobby;
    uint8_t* sendbuf = get_sendbuf();
    subcmd_bb_bank_inv_t* pkt = (subcmd_bb_bank_inv_t*)sendbuf;
    uint32_t num_items = LE32(c->bb_pl->bank.item_count);
    uint16_t size = sizeof(subcmd_bb_bank_inv_t) + num_items *
        sizeof(bitem_t);
    block_t* b = c->cur_block;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅打开了银行!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (req->hdr.pkt_len != LE16(0x10) || req->size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送错误的银行数据包!",
            c->guildcard);
        return -1;
    }

    /* Clean up the user's bank first... */
    cleanup_bb_bank(c);

    /* Fill in the packet. */
    pkt->hdr.pkt_len = LE16(size);
    pkt->hdr.pkt_type = LE16(GAME_COMMANDC_TYPE);
    pkt->hdr.flags = 0;
    pkt->type = SUBCMD_BANK_INV;
    pkt->unused[0] = pkt->unused[1] = pkt->unused[2] = 0;
    pkt->size = LE32(size);
    pkt->checksum = mt19937_genrand_int32(&b->rng); /* Client doesn't care */
    memcpy(&pkt->item_count, &c->bb_pl->bank, sizeof(psocn_bank_t));

    return crypt_send(c, (int)size, sendbuf);
}

static int handle_bb_bank_action(ship_client_t* c, subcmd_bb_bank_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t item_id;
    uint32_t amt, bank, inv, i;
    int found = -1, stack, isframe = 0;
    iitem_t iitem;
    bitem_t bitem;
    uint32_t ic[3];

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " did bank action in lobby!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0018) || pkt->size != 0x04) {
        ERR_LOG("GC %" PRIu32 " sent bad bank action!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    switch (pkt->action) {
    case SUBCMD_BANK_ACT_CLOSE:
    case SUBCMD_BANK_ACT_DONE:
        return 0;

    case SUBCMD_BANK_ACT_DEPOSIT:
        item_id = LE32(pkt->item_id);

        /* Are they depositing meseta or an item? */
        if (item_id == 0xFFFFFFFF) {
            amt = LE32(pkt->meseta_amount);
            inv = LE32(c->bb_pl->character.disp.meseta);

            /* Make sure they aren't trying to do something naughty... */
            if (amt > inv) {
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

            c->bb_pl->character.disp.meseta = LE32((inv - amt));
            c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
            c->bb_pl->bank.meseta = LE32((bank + amt));

            /* No need to tell everyone else, I guess? */
            return 0;
        }
        else {
            /* Look for the item in the user's inventory. */
            inv = c->bb_pl->inv.item_count;
            for (i = 0; i < inv; ++i) {
                if (c->bb_pl->inv.iitems[i].item_id == pkt->item_id) {
                    iitem = c->bb_pl->inv.iitems[i];
                    found = i;

                    /* If it is an equipped frame, we need to unequip all
                       the units that are attached to it. */
                    if (iitem.data_b[0] == ITEM_TYPE_GUARD &&
                        iitem.data_b[1] == ITEM_SUBTYPE_FRAME &&
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

            stack = item_is_stackable(iitem.data_l[0]);

            if (!stack && pkt->item_amount > 1) {
                ERR_LOG("GC %" PRIu32 " banking multiple of "
                    "a non-stackable item!", c->guildcard);
                return -1;
            }

            found = item_remove_from_inv(c->bb_pl->inv.iitems, inv,
                pkt->item_id, pkt->item_amount);
            if (found < 0 || found > 1) {
                ERR_LOG("Error removing item from inventory for "
                    "banking!", c->guildcard);
                return -1;
            }

            c->bb_pl->inv.item_count = (inv -= found);

            /* Fill in the bank item. */
            if (stack) {
                iitem.data_b[5] = pkt->item_amount;
                bitem.amount = LE16(pkt->item_amount);
            }
            else {
                bitem.amount = LE16(1);
            }

            bitem.flags = LE16(1);
            bitem.data_l[0] = iitem.data_l[0];
            bitem.data_l[1] = iitem.data_l[1];
            bitem.data_l[2] = iitem.data_l[2];
            bitem.item_id = iitem.item_id;
            bitem.data2_l = iitem.data2_l;

            /* Unequip any units, if the item was equipped and a frame. */
            if (isframe) {
                for (i = 0; i < inv; ++i) {
                    iitem = c->bb_pl->inv.iitems[i];
                    if (iitem.data_b[0] == ITEM_TYPE_GUARD &&
                        iitem.data_b[1] == ITEM_SUBTYPE_UNIT) {
                        c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
                    }
                }
            }

            /* Deposit it! */
            if (item_deposit_to_bank(c, &bitem) < 0) {
                ERR_LOG("Error depositing to bank for guildcard %"
                    PRIu32 "!", c->guildcard);
                return -1;
            }

            return subcmd_send_destroy_item(c, iitem.item_id,
                pkt->item_amount);
        }

    case SUBCMD_BANK_ACT_TAKE:
        item_id = LE32(pkt->item_id);

        /* Are they taking meseta or an item? */
        if (item_id == 0xFFFFFFFF) {
            amt = LE32(pkt->meseta_amount);
            inv = LE32(c->bb_pl->character.disp.meseta);

            /* Make sure they aren't trying to do something naughty... */
            if (amt + inv > 999999) {
                ERR_LOG("GC %" PRIu32 " taking too much "
                    "money out of bank!", c->guildcard);
                return -1;
            }

            bank = LE32(c->bb_pl->bank.meseta);
            if (amt > bank) {
                ERR_LOG("GC %" PRIu32 " taking out more "
                    "money than they have in the bank!", c->guildcard);
                return -1;
            }

            c->bb_pl->character.disp.meseta = LE32((inv + amt));
            c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
            c->bb_pl->bank.meseta = LE32((bank - amt));

            /* No need to tell everyone else... */
            return 0;
        }
        else {
            /* Try to take the item out of the bank. */
            found = item_take_from_bank(c, pkt->item_id, pkt->item_amount,
                &bitem);
            if (found < 0) {
                ERR_LOG("GC %" PRIu32 " taking invalid item "
                    "from bank!", c->guildcard);
                return -1;
            }

            /* Ok, we have the item... Convert the bank item to an inventory
               one... */
            iitem.equipped = LE16(0x0001);
            iitem.tech = LE16(0x0000);
            iitem.flags = 0;
            ic[0] = iitem.data_l[0] = bitem.data_l[0];
            ic[1] = iitem.data_l[1] = bitem.data_l[1];
            ic[2] = iitem.data_l[2] = bitem.data_l[2];
            iitem.item_id = LE32(l->next_game_item_id);
            iitem.data2_l = bitem.data2_l;
            ++l->next_game_item_id;

            /* Time to add it to the inventory... */
            found = item_add_to_inv(c->bb_pl->inv.iitems,
                c->bb_pl->inv.item_count, &iitem);
            if (found < 0) {
                /* Uh oh... Guess we should put it back in the bank... */
                item_deposit_to_bank(c, &bitem);
                return -1;
            }

            c->bb_pl->inv.item_count += found;

            /* Let everyone know about it. */
            return subcmd_send_create_item(c, /*ic, */iitem.data/*, iitem.data2_l*/,
                1);
        }

    default:
        ERR_LOG("GC %" PRIu32 " 发送未知银行操作: %d!",
            c->guildcard, pkt->action);
        print_payload((unsigned char*)pkt, 0x18);
        return -1;
    }
}

static int handle_bb_62_check_game_loading(ship_client_t* c, bb_subcmd_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间内指令!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size)
        return -1;

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

#define BB_LOG_UNKNOWN_SUBS

/* 处理BB 0x62/0x6D 数据包. */
int subcmd_bb_handle_one(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* dest;
    uint8_t type = pkt->type;
    int rv = -1;
    uint32_t dnum = LE32(pkt->hdr.flags);

    //DBG_LOG("BB处理 GC %" PRIu32 " 62/6D指令: 0x%02X", c->guildcard, type);

    //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    /* Ignore these if the client isn't in a lobby. */
    if (!l) {
        return 0;
    }

    pthread_mutex_lock(&l->mutex);

    /* Find the destination. */
    dest = l->clients[dnum];

    /* The destination is now offline, don't bother sending it. */
    if (!dest) {
        pthread_mutex_unlock(&l->mutex);
        return 0;
    }

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;

        switch (type) {
        case SUBCMD0x62_BURST1://6D
        case SUBCMD0x62_BURST2://6B
        case SUBCMD0x62_BURST3://6C
        case SUBCMD0x62_BURST4://6E
            if (l->flags & LOBBY_FLAG_QUESTING)
                rv = lobby_enqueue_burst_bb(l, c, (bb_pkt_hdr_t*)pkt);
            /* Fall through... */

        case SUBCMD0x62_BURST5://6F
        case SUBCMD0x62_BURST6://71
        case SUBCMD0x62_BURST7://70
            rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            break;

        default:
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
            break;
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {
    case SUBCMD_GUILDCARD:
        /* Make sure the recipient is not ignoring the sender... */
        if (client_has_ignored(dest, c->guildcard)) {
            rv = 0;
            break;
        }

        rv = handle_bb_gcsend(c, dest);
        break;

    case SUBCMD_PICK_UP:
        rv = handle_bb_pick_up(c, (subcmd_bb_pick_up_t*)pkt);
        break;

    case SUBCMD_ITEMREQ:
        rv = handle_bb_item_req(c, dest, l, (subcmd_bb_itemreq_t*)pkt);
        break;

    case SUBCMD_BITEMREQ:
        rv = handle_bb_bitem_req(c, dest, l, (subcmd_bb_bitemreq_t*)pkt);
        break;

    case SUBCMD0x62_TRADE:
        rv = handle_bb_trade(c, dest, (subcmd_bb_trade_t*)pkt);
        break;

        // Barba Ray actions
    case SUBCMD0x62_UNKNOW_A9:
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD0x62_CHARACTER_INFO:
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD_SHOP_REQ:
        rv = handle_bb_shop_req(c, (subcmd_bb_shop_req_t*)pkt);
        break;

    case SUBCMD_SHOP_BUY:
        rv = handle_bb_shop_buy(c, (subcmd_bb_shop_buy_t*)pkt);
        break;

    case SUBCMD0x62_TEKKING:
        rv = handle_bb_item_tekk(c, (subcmd_bb_tekk_item_t*)pkt);
        break;

    case SUBCMD0x62_TEKKED:
        rv = send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD_OPEN_BANK:
        rv = handle_bb_bank(c, (subcmd_bb_bank_open_t*)pkt);
        break;

    case SUBCMD_BANK_ACTION:
        rv = handle_bb_bank_action(c, (subcmd_bb_bank_act_t*)pkt);
        break;

    case SUBCMD0x62_CH_GRAVE_DATA:
        UNK_CPD(type, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    case SUBCMD0x62_GUILD_INVITE1:
    case SUBCMD0x62_GUILD_INVITE2:
    case SUBCMD0x62_GUILD_MASTER_TRANS1:
    case SUBCMD0x62_GUILD_MASTER_TRANS2:
    case SUBCMD0x62_QUEST_ITEM_UNKNOW1:
    case SUBCMD0x62_QUEST_ITEM_RECEIVE:
    case SUBCMD0x62_BATTLE_CHAR_LEVEL_FIX:
    case SUBCMD0x62_WARP_ITEM:
    case SUBCMD0x62_QUEST_ONEPERSON_SET_ITEM:
    case SUBCMD0x62_QUEST_ONEPERSON_SET_BP:
    case SUBCMD0x62_GANBLING:
        UNK_CPD(type, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x62/0x6D 指令: 0x%02X", type);
        UNK_CPD(type, pkt);
        //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
#endif /* BB_LOG_UNKNOWN_SUBS */
        /* Forward the packet unchanged to the destination. */
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        break;
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
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅开启了机关!",
            c->guildcard);
        rv = -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);

    if (pkt->enabled && pkt->switch_id  != 0xFFFF) {
        //TODO 作弊开关 开启所有机关
        //if (/*(l->flags | CHEATE) && */c->switch_assist && 
        //    (c->last_switch_enabled_command.type == 0x05)) {

        //    subcmd_send_lobby_bb(l, c, &c->last_switch_enabled_command, 
        //        sizeof(c->last_switch_enabled_command));

        //    //forward_subcommand(l, c, command, flag, &c->last_switch_enabled_command,
        //        //sizeof(c->last_switch_enabled_command));
        //    //send_command_t(c, command, flag, c->last_switch_enabled_command);
        //}
    }

    //if (cmd.enabled && cmd.switch_id != 0xFFFF) {
    //    if ((l->flags & Lobby::Flag::CHEATS_ENABLED) && c->switch_assist &&
    //        (c->last_switch_enabled_command.subcommand == 0x05)) {
    //        c->log.info("[Switch assist] Replaying previous enable command");
    //        forward_subcommand(l, c, command, flag, &c->last_switch_enabled_command,
    //            sizeof(c->last_switch_enabled_command));
    //        send_command_t(c, command, flag, c->last_switch_enabled_command);
    //    }
    //    c->last_switch_enabled_command = cmd;
    //}
    //return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
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
    uint8_t i;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了实例!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len == LE16(0x0010)) {
        /* 对空气进行攻击 */
        return 0;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的普通攻击数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);

    /* Handle each thing that was hit */
    for (i = 0; i < pkt->size - 2; ++i) {
        handle_bb_objhit_common(c, l, LE16(pkt->objects[i].obj_id));
    }

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
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
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅用法术攻击实例!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len == LE16(0x0010)) {
        /* 对空气进行攻击 */
        return 0;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的法术攻击数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Sanity check... Does the character have that level of technique? */
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
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);

    /* See what technique was used... */
    switch (pkt->tech) {
        /* These work just like physical hits and can only hit one target, so
           handle them the simple way... */
    case TECHNIQUE_FOIE:
    case TECHNIQUE_ZONDE:
    case TECHNIQUE_GRANTS:
        if (pkt->size == 3)
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
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_objhit(ship_client_t* c, subcmd_bb_bhit_pkt_t* pkt) {
    uint64_t now = get_ms_time();
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了箱子!",
            c->guildcard);
        return -1;
    }

    /* We only care about these if the AoE timer is set on the sender. */
    if (c->aoe_timer < now)
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);

    /* Check the type of the object that was hit. As the AoE timer can't be set
       if the objects aren't loaded, this shouldn't ever trip up... */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);

    /* Handle the object marked as hit, if appropriate. */
    handle_bb_objhit_common(c, l, LE16(pkt->box_id2));

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_symbol_chat(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send chats for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 1);
}

static int handle_bb_req_exp(ship_client_t* c, subcmd_bb_req_exp_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t mid;
    uint32_t bp, exp;
    game_enemy_t* en;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中获取经验!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的经验获取数据包!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Make sure the enemy is in range. */
    mid = LE16(pkt->enemy_id);
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

    // TODO 这里的经验倍率还未调整
    /* Give the client their experience! */
    bp = en->bp_entry;
    exp = l->bb_params[bp].exp + 100000;

    if (!pkt->last_hitter) {
        exp = (exp * 80) / 100;
    }

    return client_give_exp(c, exp);
}

static int handle_bb_gallon_area(ship_client_t* c, subcmd_bb_gallon_area_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发任务指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_mhit(ship_client_t* c, subcmd_bb_mhit_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t mid, mid2, dmg;
    game_enemy_t* en;
    uint32_t flags;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了怪物!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的怪物攻击数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Make sure the enemy is in range. */
    mid = LE16(pkt->enemy_id);
    mid2 = LE16(pkt->enemy_id2);
    dmg = LE16(pkt->damage);
    flags = LE32(pkt->flags);

    /* Swap the flags on the packet if the user is on GC... Looks like Sega
       decided that it should be the opposite order as it is on DC/PC/BB. */
    if (c->version == CLIENT_VERSION_GC)
        flags = SWAP32(flags);

    /* Bail out now if we don't have any enemy data on the team. */
    if (!l->map_enemies || l->challenge || l->battle) {
        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        if (flags & 0x00000800)
            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        return send_lobby_mhit(l, c, mid, mid2, dmg, flags);
    }

    if (mid > l->map_enemies->count) {
#ifdef DEBUG
        ERR_LOG("GC %" PRIu32 " 攻击了无效的怪物 (%d -- 地图怪物数量: "
            "%d)!"
            "章节: %d, 层级: %d, 地图: (%d, %d)", c->guildcard, mid,
            l->map_enemies->count, l->episode, c->cur_area,
            l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);

        if ((l->flags & LOBBY_FLAG_QUESTING))
            ERR_LOG("任务 ID: %d, 版本: %d", l->qid, l->version);
#endif

        if (l->logfp) {
            fdebug(l->logfp, DBG_WARN, "GC %" PRIu32 " 攻击了无效的怪物 (%d -- 地图怪物数量: %d)!\n"
                "章节: %d, 层级: %d, 地图: (%d, %d)\n", c->guildcard, mid,
                l->map_enemies->count, l->episode, c->cur_area,
                l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);

            if ((l->flags & LOBBY_FLAG_QUESTING))
                fdebug(l->logfp, DBG_WARN, "任务 ID: %d, 版本: %d\n",
                    l->qid, l->version);
        }

        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        if (flags & 0x00000800)
            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        /* If server-side drops aren't on, then just send it on and hope for the
           best. We've probably got a bug somewhere on our end anyway... */
        if (!(l->flags & LOBBY_FLAG_SERVER_DROPS))
            return send_lobby_mhit(l, c, mid, mid2, dmg, flags);

        ERR_LOG("GC %" PRIu32 " 攻击了无效的怪物 (%d -- 地图怪物数量: "
            "%d)!", c->guildcard, mid, l->map_enemies->count);
        return -1;
    }

    /* Make sure it looks like they're in the right area for this... */
    /* XXXX: There are some issues still with Episode 2, so only spit this out
       for now on Episode 1. */
#ifdef DEBUG
    if (c->cur_area != l->map_enemies->enemies[mid].area && l->episode == 1 &&
        !(l->flags & LOBBY_FLAG_QUESTING)) {
        ERR_LOG("GC %" PRIu32 " hit enemy in wrong area "
            "(%d -- max: %d)!\n Episode: %d, Area: %d, Enemy Area: %d "
            "Map: (%d, %d)", c->guildcard, mid, l->map_enemies->count,
            l->episode, c->cur_area, l->map_enemies->enemies[mid].area,
            l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);
    }
#endif

    if (l->logfp && c->cur_area != l->map_enemies->enemies[mid].area &&
        !(l->flags & LOBBY_FLAG_QUESTING)) {
        fdebug(l->logfp, DBG_WARN, "GC %" PRIu32 " 在无效区域攻击了怪物 (%d -- 地图怪物数量: %d)!\n 章节: %d, 区域: %d, 敌人数据区域: %d "
            "地图: (%d, %d)", c->guildcard, mid, l->map_enemies->count,
            l->episode, c->cur_area, l->map_enemies->enemies[mid].area,
            l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);
    }

    /* Make sure the person's allowed to be on this floor in the first place. */
    if ((l->flags & LOBBY_FLAG_ONLY_ONE) && !(l->flags & LOBBY_FLAG_QUESTING)) {
        if (l->episode == 1) {
            switch (c->cur_area) {
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
    if (!(en->clients_hit & 0x80)) {
        en->clients_hit |= (1 << c->client_id);
        en->last_client = c->client_id;

        script_execute(ScriptActionEnemyHit, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_UINT32, en->bp_entry,
            SCRIPT_ARG_UINT8, en->rt_index, SCRIPT_ARG_UINT8,
            en->clients_hit, SCRIPT_ARG_END);

        /* If the kill flag is set, mark it as dead and update the client's
           counter. */
        if (flags & 0x00000800) {
            en->clients_hit |= 0x80;

            script_execute(ScriptActionEnemyKill, c, SCRIPT_ARG_PTR, c,
                SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_UINT32,
                en->bp_entry, SCRIPT_ARG_UINT8, en->rt_index,
                SCRIPT_ARG_UINT8, en->clients_hit, SCRIPT_ARG_END);

            if (en->bp_entry < 0x60 && !(l->flags & LOBBY_FLAG_HAS_NPC))
                ++c->enemy_kills[en->bp_entry];
        }
    }

    return send_lobby_mhit(l, c, mid, mid2, dmg, flags);

    ///* Save the hit, assuming the enemy isn't already dead. */
    //if (!(l->map_enemies->enemies[mid].clients_hit & 0x80)) {
    //    l->map_enemies->enemies[mid].clients_hit |= (1 << c->client_id);
    //    l->map_enemies->enemies[mid].last_client = c->client_id;
    //}

    //return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_feed_mag(ship_client_t* c, subcmd_bb_feed_mag_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t item_id = pkt->item_id, mag_id = pkt->mag_id;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发游戏指令!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03 ||
        pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }
//[2022年08月29日 17:41:23:408] 调试(3222): BB处理 GC 42004064 60指令: 0x3E
//[2022年08月29日 17:41:29:975] 调试(3222): BB处理 GC 42004064 60指令: 0x28
//( 00000000 )   14 00 60 00 00 00 00 00  28 03 00 00 02 00 01 00    ..`.....(.......
//( 00000010 )   03 00 01 00                                         ....
//[2022年08月29日 17:41:30:010] 调试(1875): GC 42004064 使用物品ID 3 喂养玛古 ID 2!
//[2022年08月29日 17:41:30:018] 调试(3222): BB处理 GC 42004064 60指令: 0x61
//[2022年08月29日 17:41:45:642] 调试(3222): BB处理 GC 42004064 60指令: 0x28
//( 00000000 )   14 00 60 00 00 00 00 00  28 03 00 00 02 00 01 00    ..`.....(.......
//( 00000010 )   03 00 01 00                                         ....
//[2022年08月29日 17:41:45:677] 调试(1875): GC 42004064 使用物品ID 3 喂养玛古 ID 2!
//[2022年08月29日 17:41:45:687] 调试(3222): BB处理 GC 42004064 60指令: 0x61
//[2022年08月29日 17:41:49:449] 调试(3222): BB处理 GC 42004064 60指令: 0x28
//( 00000000 )   14 00 60 00 00 00 00 00  28 03 00 00 02 00 01 00    ..`.....(.......
//( 00000010 )   03 00 01 00                                         ....
//[2022年08月29日 17:41:49:485] 调试(1875): GC 42004064 使用物品ID 3 喂养玛古 ID 2!
    print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
    DBG_LOG("GC %" PRIu32 " 使用物品ID %d 喂养玛古 ID %d!",
        c->guildcard, item_id, mag_id);
    //if (mag_bb_feed(c, item_id, mag_id)) {
    //    ERR_LOG("GC %" PRIu32 " 喂养玛古发生错误!",
    //        c->guildcard);
    //    print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
    //    return -1;
    //}

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_equip(ship_client_t* c, subcmd_bb_equip_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t inv, i, item_id = 0, found_item = 0, found_slot = 0, j = 0, slot[4] = { 0 };

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅更换装备!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03 ||
        pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误装备数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    //DBG_LOG("物品数量 %d %d", c->bb_pl->inv.item_count, c->pl->bb.inv.item_count);
    /* Find the item and equip it. */
    inv = c->bb_pl->inv.item_count;
    item_id = pkt->item_id;
    for (i = 0; i < inv; ++i) {
        //DBG_LOG("进入循环");
        if (c->bb_pl->inv.iitems[i].item_id == item_id) {
            found_item = 1;
            /*DBG_LOG("物品ID %08X %08X %08X( %08X %08X %08X - %08X %08X %08X )", c->pl->bb.inv.iitems[i].item_id, c->bb_pl->inv.iitems[i].item_id, pkt->item_id,
                c->bb_pl->inv.iitems[j].data_l[0], c->bb_pl->inv.iitems[j].data_l[1], c->bb_pl->inv.iitems[j].data_l[2],
                c->pl->bb.inv.iitems[i].data_l[0], c->pl->bb.inv.iitems[i].data_l[1], c->pl->bb.inv.iitems[i].data_l[2]
                );*/
            switch (c->bb_pl->inv.iitems[i].data_b[0])
            {
            case ITEM_TYPE_WEAPON:
                //DBG_LOG("武器识别");
               /* if (item_check_equip(
                    pmt_weapon_bb[c->bb_pl->inv.iitems[i].data_b[1]][c->bb_pl->inv.iitems[i].data_b[2]]->equip_flag,
                    c->equip_flags)) {
                    ERR_LOG("GC %" PRIu32 " 装备了不属于该职业的物品数据!",
                        c->guildcard);
                    send_msg1(c, "装备了不属于该职业的物品数据.");
                    return -1;
                }
                else */{
                    // De-equip any other weapon on character. (Prevent stacking) 解除角色上任何其他武器的装备。（防止堆叠） 
                    for (j = 0; j < inv; j++)
                        if ((c->bb_pl->inv.iitems[j].data_b[0] == ITEM_TYPE_WEAPON) &&
                            (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {

                            c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                            //DBG_LOG("卸载武器");
                        }
                }
                break;
            case ITEM_TYPE_GUARD:
                switch (c->bb_pl->inv.iitems[i].data_b[1])
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
                    } else */{
                        // Remove any other armor and equipped slot items.移除其他装甲和插槽
                        for (j = 0; j < inv; ++j)
                        {
                            if ((c->bb_pl->inv.iitems[j].data_b[0] == ITEM_TYPE_GUARD) &&
                                (c->bb_pl->inv.iitems[j].data_b[1] != ITEM_SUBTYPE_BARRIER) &&
                                (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008)))
                            {
                                //DBG_LOG("卸载装甲");
                                c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                                c->bb_pl->inv.iitems[j].data_b[4] = 0x00;
                            }
                        }
                        break;
                    }
                    break;
                case ITEM_SUBTYPE_BARRIER: // Check barrier equip requirements 检测护盾装备请求
                    //DBG_LOG("护盾识别");
                    /*if ((item_check_equip(pmt_shield_bb[c->bb_pl->inv.iitems[i].data_b[2]]->equip_flag & c->equip_flags, c->equip_flags)) ||
                        (c->bb_pl->character.level < pmt_shield_bb[c->bb_pl->inv.iitems[i].data_b[2]]->level_req))
                    {
                        ERR_LOG("GC %" PRIu32 " 装备了作弊物品数据!",
                            c->guildcard);
                        send_msg1(c, "\"作弊装备\" 是不允许的.");
                        return -1;
                    } else */{
                        // Remove any other barrier
                        for (j = 0; j < inv; ++j) {
                            if ((c->bb_pl->inv.iitems[j].data_b[0] == ITEM_TYPE_GUARD) &&
                                (c->bb_pl->inv.iitems[j].data_b[1] == ITEM_SUBTYPE_BARRIER) &&
                                (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008)))
                            {
                                //DBG_LOG("卸载护盾");
                                c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                                c->bb_pl->inv.iitems[j].data_b[4] = 0x00;
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
                        if ((c->bb_pl->inv.iitems[j].data_b[0] == ITEM_TYPE_GUARD) &&
                            (c->bb_pl->inv.iitems[j].data_b[1] == ITEM_SUBTYPE_UNIT))
                        {
                            //DBG_LOG("插槽 %d 识别", j);
                            if ((c->bb_pl->inv.iitems[j].flags & LE32(0x00000008)) &&
                                (c->bb_pl->inv.iitems[j].data_b[4] < 0x04)) {

                                slot[c->bb_pl->inv.iitems[j].data_b[4]] = 1;
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
                        c->bb_pl->inv.iitems[j].data_b[4] = (uint8_t)(found_slot);
                        //DBG_LOG("111111111111111");
                        //client->Full_Char.inventory[i].item.item_data1[4] = (uint8_t)(found_slot);
                    } else {
                        if (found_slot)
                        {
                            found_slot--;
                            c->bb_pl->inv.iitems[j].data_b[4] = (uint8_t)(found_slot);
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
                    if ((c->bb_pl->inv.iitems[j].data_b[0] == ITEM_TYPE_MAG) &&
                        (c->bb_pl->inv.iitems[j].flags & LE32(0x00000008))) {

                        c->bb_pl->inv.iitems[j].flags &= LE32(0xFFFFFFF7);
                        //DBG_LOG("卸载玛古");
                    }
                break;
            }

            //DBG_LOG("完成卸载, 但是未识别成功");

            /* XXXX: Should really make sure we can equip it first... */
            c->bb_pl->inv.iitems[i].flags |= LE32(0x00000008);
            break;
        }
    }

    /* 是否存在物品背包中? */
    if (!found_item) {
        ERR_LOG("GC %" PRIu32 " 装备了未存在的物品数据!",
            c->guildcard);
        //return -1;
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_unequip(ship_client_t* c, subcmd_bb_equip_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t inv, i, isframe = 0;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅卸除装备!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03 ||
        pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误卸除装备数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Find the item and remove the equip flag. */
    inv = c->bb_pl->inv.item_count;
    for (i = 0; i < inv; ++i) {
        if (c->bb_pl->inv.iitems[i].item_id == pkt->item_id) {
            c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);

            /* If its a frame, we have to make sure to unequip any units that
               may be equipped as well. */
            if (c->bb_pl->inv.iitems[i].data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data_b[1] == ITEM_SUBTYPE_FRAME) {
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
            if (c->bb_pl->inv.iitems[i].data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data_b[1] == ITEM_SUBTYPE_UNIT) {
                c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_use_item(ship_client_t* c, subcmd_bb_use_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int num;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅使用物品!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->size != 0x02)
        return -1;

    if (!(c->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    ///* Remove the item from the user's inventory */
    //num = item_remove_from_inv(c->iitems, c->item_count, pkt->item_id, 1);
    //if (num < 0)
    //    ERR_LOG("无法从背包移除物品!");
    //else
    //    c->item_count -= num;

    /* Remove the item from the user's inventory. */
    if ((num = item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
        pkt->item_id, 1)) < 1) {
        ERR_LOG("无法从玩家背包中移除物品!");
        return -1;
    }

    c->item_count -= num;
    c->bb_pl->inv.item_count -= num;
    c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;


send_pkt:
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_medic(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " used medical center in lobby!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
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
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_3a(ship_client_t* c, subcmd_bb_cmd_3a_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->size != 0x01 || c->client_id != pkt->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_sort_inv(ship_client_t* c, subcmd_bb_sort_inv_t* pkt) {
    lobby_t* l = c->cur_lobby;
    inventory_t inv;
    inventory_t mode_inv;
    int i, j, found = 0;
    int item_used[30] = { 0 };
    int mode_item_used[30] = { 0 };

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅整理了物品!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0084) || pkt->size != 0x1F) {
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
            inv.iitems[i].item_id = 0xFFFFFFFF;
            break;
        }

        /* Look for the item in question. */
        for (j = 0; j < inv.item_count; ++j) {
            if (c->bb_pl->inv.iitems[j].item_id == pkt->item_ids[i]) {
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
            if (c->pl->bb.inv.iitems[j].item_id == pkt->item_ids[i]) {
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

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅使用传送门!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0024) || pkt->size != 0x07) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            c->guildcard, pkt->type);
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
    if (pkt->area_id != 0) {
        /* Make sure the user is sending a pipe from the area he or she is
           currently in. */
        if (pkt->area_id != c->cur_area) {
            ERR_LOG("Attempt by GC %" PRIu32 " to spawn pipe to area "
                "he/she is not in (层级: %d, pipe: %d).", c->guildcard,
                c->cur_area, (int)pkt->area_id);
            return -1;
        }
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_spawn_npc(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
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

static int handle_bb_set_flag(ship_client_t* c, subcmd_bb_set_flag_t* pkt) {
    lobby_t* l = c->cur_lobby;

    //if (!l->is_game()) {
    //    return;
    //}

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (!l || l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        return -1;
    }

    //const auto* p = check_size_sc(data, sizeof(PSOSubcommand), 0xFFFF);
    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    //forward_subcommand(l, c, command, flag, data);


    //flag = *(uint16_t*)&client->decrypt_buf_code[0x0C];
    if (pkt->q1flag < 1024)
        c->bb_pl->quest_data1[((uint32_t)l->difficulty * 0x80) + (pkt->q1flag >> 3)] |= 1 << (7 - (pkt->q1flag & 0x07));
        //client->Full_Char.quest_data1[((uint32_t)l->difficulty * 0x80) + (flag >> 3)] |= 1 << (7 - (flag & 0x07));

    //bool should_send_boss_drop_req = false;
    //if (pkt[2].dword == l->difficulty) {
    //    if ((l->episode == 1) && (c->cur_area == 0x0E)) {
    //        // On Normal, Dark Falz does not have a third phase, so send the drop
    //        // request after the end of the second phase. On all other difficulty
    //        // levels, send it after the third phase.
    //        if (((l->difficulty == 0) && (p[1].dword == 0x00000035)) ||
    //            ((l->difficulty != 0) && (p[1].dword == 0x00000037))) {
    //            should_send_boss_drop_req = true;
    //        }
    //    }
    //    else if ((l->episode == 2) && (p[1].dword == 0x00000057) && (c->cur_area == 0x0D)) {
    //        should_send_boss_drop_req = true;
    //    }
    //}

    //if (should_send_boss_drop_req) {
    //    auto c = l->clients.at(l->leader_id);
    //    if (c) {
    //        G_EnemyDropItemRequest_6x60 req = {
    //          0x60,
    //          0x06,
    //          0x1090,
    //          static_cast<uint8_t>(c->area),
    //          static_cast<uint8_t>((l->episode == 2) ? 0x4E : 0x2F),
    //          0x0B4F,
    //          (l->episode == 2) ? -9999.0f : 10160.58984375f,
    //          0.0f,
    //          0xE0AEDC0100000002,
    //        };
    //        send_command_t(c, 0x62, l->leader_id, req);
    //    }
    //}

    /*DBG_LOG("GC %" PRIu32 " 触发SET_FLAG指令! flag = %02X q1flag = %02X",
        c->guildcard, pkt->flag, pkt->q1flag);*/

    return 0;
}

static int handle_bb_killed_monster(ship_client_t* c, subcmd_bb_killed_monster_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中杀掉了怪物!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的杀怪数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static inline int bb_reg_sync_index(lobby_t* l, uint8_t regnum) {
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
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中杀掉了怪物!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的杀怪数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

//[2022年09月04日 15:41:22:305] 舰船服务器 截获(3734): subcmd-bb.c 3734 行 CLIENT_UNKNOW_77 指令 0x0060 未处理. (数据如下)
//
//[2022年09月04日 15:41:22:316] 舰船服务器 截获(3734): 
//( 00000000 )   14 00 60 00 00 00 00 00  77 03 71 00 A4 00 6B 00 ..`.....w.q.?k.
//( 00000010 )   01 00 00 00                                     ....
//
//
//[2022年09月04日 20:25:18:747] 舰船服务器 截获(3734): subcmd-bb.c 3734 行 CLIENT_UNKNOW_77 指令 0x0060 未处理. (数据如下)
//
//[2022年09月04日 20:25:18:758] 舰船服务器 截获(3734): 
//( 00000000 )   14 00 60 00 00 00 00 00  77 03 00 00 00 00 6B 00 ..`.....w.....k.
//( 00000010 )   01 00 00 00                                     ....
//
//
//[2022年09月05日 04:04:11:108] 舰船服务器 截获(3732): subcmd-bb.c 3732 行 CLIENT_UNKNOW_77 指令 0x0060 未处理. (数据如下)
//
//[2022年09月05日 04:04:11:126] 舰船服务器 截获(3732): 
//( 00000000 )   14 00 60 00 00 00 00 00  77 03 00 00 00 00 6B 00 ..`.....w.....k.
//( 00000010 )   01 00 00 00                                     ....
//
//
//[2022年09月05日 22:06:42:522] 舰船服务器 截获(3968): subcmd-bb.c 3968 行 CLIENT_UNKNOW_77 指令 0x0060 未处理. (数据如下)
//
//[2022年09月05日 22:06:42:534] 舰船服务器 截获(3968): 
//( 00000000 )   14 00 60 00 00 00 00 00  77 03 00 00 83 00 6B 00 ..`.....w...?k.
//( 00000010 )   00 00 00 00                                     ....
//

    /* XXXX: Probably should do some checking here... */
    /* Run the register sync script, if one is set. If the script returns
       non-zero, then assume that it has adequately handled the sync. */
    if ((script_execute(ScriptActionQuestSyncRegister, c, SCRIPT_ARG_PTR, c,
        SCRIPT_ARG_PTR, l, SCRIPT_ARG_UINT8, pkt->reg_num,
        SCRIPT_ARG_UINT32, val, SCRIPT_ARG_END))) {
        done = 1;
    }

    /* Does this quest use global flags? If so, then deal with them... */
    if ((l->q_flags & LOBBY_QFLAG_SHORT) && pkt->reg_num == l->q_shortflag_reg &&
        !done) {
        /* Check the control bits for sensibility... */
        ctl = (val >> 29) & 0x07;

        /* Make sure the error or response bits aren't set. */
        if ((ctl & 0x06)) {
            DBG_LOG("Quest set flag register with illegal ctl!\n");
            send_sync_register(c, pkt->reg_num, 0x8000FFFE);
        }
        /* Make sure we don't have anything with any reserved ctl bits set
           (unless a script has already handled the sync). */
        else if ((val & 0x17000000)) {
            DBG_LOG("Quest set flag register with reserved ctl!\n");
            send_sync_register(c, pkt->reg_num, 0x8000FFFE);
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
        if (pkt->reg_num == l->q_data_reg) {
            if (c->q_stack_top < CLIENT_MAX_QSTACK) {
                if (!(c->flags & CLIENT_FLAG_QSTACK_LOCK)) {
                    c->q_stack[c->q_stack_top++] = val;

                    /* Check if we've got everything we expected... */
                    if (c->q_stack_top >= 3 &&
                        c->q_stack_top == 3 + c->q_stack[1] + c->q_stack[2]) {
                        /* Call the function requested and reset the stack. */
                        ctl = quest_function_dispatch(c, l);

                        if (ctl != QUEST_FUNC_RET_NOT_YET) {
                            send_sync_register(c, pkt->reg_num, ctl);
                            c->q_stack_top = 0;
                        }
                    }
                }
                else {
                    /* The stack is locked, ignore the write and report the
                       error. */
                    send_sync_register(c, pkt->reg_num,
                        QUEST_FUNC_RET_STACK_LOCKED);
                }
            }
            else if (c->q_stack_top == CLIENT_MAX_QSTACK) {
                /* Eat the stack push and report an error. */
                send_sync_register(c, pkt->reg_num,
                    QUEST_FUNC_RET_STACK_OVERFLOW);
            }

            done = 1;
        }
        else if (pkt->reg_num == l->q_ctl_reg) {
            /* For now, the only reason we'll have one of these is to reset the
               stack. There might be other reasons later, but this will do, for
               the time being... */
            c->q_stack_top = 0;
            done = 1;
        }
    }

    /* Does this register have to be synced? */
    if ((idx = bb_reg_sync_index(l, pkt->reg_num)) != -1) {
        l->regvals[idx] = val;
    }

    if (!done)
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);

    return 0;
}

static int handle_bb_player_died(ship_client_t* c, subcmd_bb_player_died_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令! 0x%02X",
            c->guildcard, pkt->type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->size != 0x02 || c->client_id != pkt->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的死亡数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_use_tech(ship_client_t* c, subcmd_bb_use_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->size != 0x02 || c->client_id != pkt->client_id) {
        ERR_LOG("GC %" PRIu32 " 释放了违规的法术!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(c->flags & CLIENT_FLAG_INFINITE_TP)) {
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD_STAT_TPUP, 255);
    //return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_shop_inv(ship_client_t* c, subcmd_bb_shop_inv_t* pkt) {
    block_t* b = c->cur_block;
    lobby_t* l = c->cur_lobby;
 
    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    /* This aught to do it... */
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_death_sync(ship_client_t* c, subcmd_bb_death_sync_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->size != 0x02 || c->client_id != pkt->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_4e(ship_client_t* c, subcmd_bb_cmd_4e_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令! 0x%02X",
            c->guildcard, pkt->type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->size != 0x01 || c->client_id != pkt->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_switch_req(ship_client_t* c, subcmd_bb_switch_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->size != 0x02 || c->client_id != pkt->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_take_damage(ship_client_t* c, subcmd_bb_take_damage_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(c->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD_STAT_HPUP, 2000);
}

static int handle_bb_menu_req(ship_client_t* c, subcmd_bb_menu_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    //大厅服务台请求
//[2022年08月29日 16:46:29:312] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 00 00 00 00    ..`.....R.......
//( 00000010 )   00 80 FF FF                                         ....
//[2022年08月29日 16:46:32:113] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 00 00 00 00    ..`.....R.......
//( 00000010 )   00 80 FF FF                                         ....
//[2022年08月29日 16:46:32:149] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x3F
//[2022年08月29日 16:46:32:812] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 00 00 00 00    ..`.....R.......
//( 00000010 )   00 80 FF FF                                         ....
//[2022年08月29日 16:46:33:646] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 00 00 00 00    ..`.....R.......
//( 00000010 )   00 80 FF FF                                         ....
//[2022年08月29日 16:46:33:681] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x3F
//[2022年08月29日 16:46:34:179] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 00 00 00 00    ..`.....R.......
//( 00000010 )   00 80 FF FF                                         ....
    //与防具店对话
//[2022年08月29日 16:48:04:617] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 08 00 00 00    ..`.....R.......
//( 00000010 )   6B DE FF FF                                         k...
    //与工具店对话
//[2022年08月29日 16:49:05:719] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 08 00 00 00    ..`.....R.......
//( 00000010 )   97 EC FF FF                                         ....
    //与武器店对话
//[2022年08月29日 16:50:28:523] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 08 00 00 00    ..`.....R.......
//( 00000010 )   E5 F2 FF FF                                         ....
    //与鉴定对话
//[2022年08月29日 16:53:14:688] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 08 00 00 00    ..`.....R.......
//( 00000010 )   6B 31 00 00                                         k1..
    //与银行对话
//[2022年08月29日 16:54:07:590] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 08 00 00 00    ..`.....R.......
//( 00000010 )   97 D9 FF FF                                         ....
    //与医院对话
//[2022年08月29日 16:55:16:526] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 08 00 00 00    ..`.....R.......
//( 00000010 )   29 7D 00 00                                         )}..
//[2022年08月29日 16:56:17:062] 调试(subcmd-bb.c 3171): BB处理 GC 42004063 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 00 00 08 00 00 00    ..`.....R.......
//( 00000010 )   29 A4 FF FF                                         )...
    //2号人物 任务版对话
//[2022年08月29日 17:22:48:363] 调试(subcmd-bb.c 3171): BB处理 GC 42004064 60指令: 0x52
//( 00000000 )   14 00 60 00 00 00 00 00  52 03 01 00 08 00 00 00    ..`.....R.......
//( 00000010 )   A2 EB FF FF                                         ....
    /* We don't care about these in lobbies. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
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
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_used_tech(ship_client_t* c, subcmd_bb_used_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " hit object in lobby!",
            c->guildcard);
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(c->flags & CLIENT_FLAG_INFINITE_TP)) {
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD_STAT_TPUP, 255);
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
    if (c->client_id == pkt->client_id) {
        script_execute(ScriptActionChangeArea, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            c->cur_area, SCRIPT_ARG_END);
        c->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_normal_attack(ship_client_t* c, subcmd_bb_natk_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发普通攻击指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Save the new area and move along */
    if (c->client_id == pkt->client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_set_pos(ship_client_t* c, subcmd_bb_set_pos_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Save the new position and move along */
    if (c->client_id == pkt->client_id) {
        c->w = pkt->w;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_move(ship_client_t* c, subcmd_bb_move_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Save the new position and move along */
    if (c->client_id == pkt->client_id) {
        c->x = pkt->x;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_drop_item(ship_client_t* c, subcmd_bb_drop_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1, isframe = 0;
    uint32_t i, inv;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅中掉落物品!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->size != 0x06 ||
        pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品掉落数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* TODO 完成挑战模式的物品掉落 */
    if (l->challenge) {
        /* Done. Send the packet on to the lobby. */
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    }

    /* Look for the item in the user's inventory. */
    inv = c->bb_pl->inv.item_count;

    for (i = 0; i < inv; ++i) {
        if (c->bb_pl->inv.iitems[i].item_id == pkt->item_id) {
            found = i;

            /* If it is an equipped frame, we need to unequip all the units
               that are attached to it.
               如果它是一个装备好的护甲，我们需要取消与它相连的所有单元的装备*/
            if (c->bb_pl->inv.iitems[i].data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data_b[1] == ITEM_SUBTYPE_FRAME &&
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
            if (c->bb_pl->inv.iitems[i].data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data_b[1] == ITEM_SUBTYPE_UNIT) {
                c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* We have the item... Add it to the lobby's inventory.
    我们有这个物品…把它添加到大厅的背包中*/
    if (!lobby_add_item2_locked(l, &c->bb_pl->inv.iitems[found])) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("Couldn't add item to lobby inventory!\n");
        return -1;
    }

    /* Remove the item from the user's inventory. */
    if (item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
        pkt->item_id, 0xFFFFFFFF) < 1) {
        ERR_LOG("无法从玩家背包中移除物品!");
        return -1;
    }

    --c->bb_pl->inv.item_count;
    c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

    /* Done. Send the packet on to the lobby. */
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_drop_pos(ship_client_t* c, subcmd_bb_drop_pos_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1;
    uint32_t i, meseta, amt;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅丢弃了物品!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->size != 0x06 ||
        pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Look for the item in the user's inventory. */
    if (pkt->item_id != 0xFFFFFFFF) {
        for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
            if (c->bb_pl->inv.iitems[i].item_id == pkt->item_id) {
                found = i;
                break;
            }
        }

        if (c->mode > 0) {
            for (i = 0; i < c->pl->bb.inv.item_count; ++i) {
                if (c->pl->bb.inv.iitems[i].item_id == pkt->item_id) {
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

    /* Done. Send the packet on to the lobby. */
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_destroy_item(ship_client_t* c, subcmd_bb_destroy_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1;
    uint32_t i, tmp, tmp2;
    iitem_t item_data;
    iitem_t* it;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅中掉落堆叠物品!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03 ||
        pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的堆叠物品掉落数据!",
            c->guildcard);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* TODO 完成挑战模式的物品掉落 */
    if (l->challenge) {
        /* Done. Send the packet on to the lobby. */
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    }

    if (pkt->item_id != 0xFFFFFFFF) {
        /* Look for the item in the user's inventory. */
        for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
            if (c->bb_pl->inv.iitems[i].item_id == pkt->item_id) {
                found = i;
                break;
            }
        }

        /* If the item isn't found, then punt the user from the ship. */
        if (found == -1) {
            ERR_LOG("GC %" PRIu32 " 掉落无效的堆叠物品!", c->guildcard);
            return -1;
        }

        /* Grab the item from the client's inventory and set up the split */
        item_data = c->iitems[found];
        item_data.item_id = LE32((++l->highest_item[c->client_id]));
        item_data.data_b[5] = (uint8_t)(LE32(pkt->amount));
    }
    else {
        item_data.data_l[0] = LE32(Item_Meseta);
        item_data.data_l[1] = item_data.data_l[2] = 0;
        item_data.data2_l = pkt->amount;
        item_data.item_id = LE32((++l->highest_item[c->client_id]));
    }

    /* Make sure the item id and amount match the most recent 0xC3. */
    if (pkt->item_id != c->drop_item || pkt->amount != c->drop_amt) {
        ERR_LOG("GC %" PRIu32 " 掉了不同的堆叠物品!",
            c->guildcard);
        return -1;
    }

    /* We have the item... Add it to the lobby's inventory. */
    if (!(it = lobby_add_item2_locked(l, &item_data))) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("无法将物品添加至游戏房间!\n");
        return -1;
    }

    if (pkt->item_id != 0xFFFFFFFF) {
        /* Remove the item from the user's inventory. */
        found = item_remove_from_inv(c->bb_pl->inv.iitems,
            c->bb_pl->inv.item_count, pkt->item_id,
            LE32(pkt->amount));
        if (found < 0) {
            ERR_LOG("无法从玩家背包中移除物品!");
            return -1;
        }

        c->bb_pl->inv.item_count -= found;
        c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;
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

    /* Now we have two packets to send on. First, send the one telling everyone
       that there's an item dropped. Then, send the one removing the item from
       the client's inventory. The first must go to everybody, the second to
       everybody except the person who sent this packet in the first place. */
    subcmd_send_drop_stack(c, c->drop_area, c->drop_x, c->drop_z, it);

    /* Done. Send the packet on to the lobby. */
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_talk_npc(ship_client_t* c, subcmd_bb_talk_npc_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅与游戏房间中的NPC交谈!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != SUBCMD_DONE_NPC_SIZE)
        return -1;

    /* Clear the list of dropped items. */
    if (pkt->unk == 0xFFFF && c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_done_npc(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅与游戏房间中的NPC完成交谈!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != SUBCMD_TALK_NPC_SIZE)
        return -1;

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_level_up(ship_client_t* c, subcmd_bb_level_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->size != 0x05 || pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
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

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_word_select(ship_client_t* c, subcmd_bb_word_select_t* pkt) {
    subcmd_word_select_t gc;

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send chats for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    memcpy(&gc, pkt, sizeof(subcmd_word_select_t) - sizeof(dc_pkt_hdr_t));
    gc.client_id_gc = pkt->client_id;
    gc.hdr.pkt_type = (uint8_t)(LE16(pkt->hdr.pkt_type));
    gc.hdr.flags = (uint8_t)pkt->hdr.flags;
    gc.hdr.pkt_len = LE16((LE16(pkt->hdr.pkt_len) - 4));

    return word_select_send_gc(c, &gc);
}

static int handle_bb_chair_dir(ship_client_t* c, subcmd_bb_chair_dir_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->size != 0x02 || c->client_id != pkt->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_cmode_grave(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
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

        pc.type = SUBCMD_CMODE_GRAVE;
        pc.size = 0x38;
        pc.unused1 = dc.unused1;
        pc.client_id = dc.client_id;
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

        dc.type = SUBCMD_CMODE_GRAVE;
        dc.size = 0x2A;
        dc.unused1 = pc.unused1;
        dc.client_id = pc.client_id;
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
        return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
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
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->size != 0x08) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

static int handle_bb_sell_item(ship_client_t* c, subcmd_bb_sell_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t i;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    /* Sanity check... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->size != 0x03 || pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    for (i = 0;i < c->bb_pl->inv.item_count;i++) {
        /* Look for the item in question. */
        if (c->bb_pl->inv.iitems[i].item_id == pkt->item_id) {

            if ((pkt->sell_num > 1) && (c->bb_pl->inv.iitems[i].data_b[0] != 0x03)) {
                DBG_LOG("handle_bb_sell_item %d 0x%02X", pkt->sell_num, c->bb_pl->inv.iitems[i].data_b[0]);
                break;
            }
            else
            {
                uint32_t shop_price = get_bb_shop_price(&c->bb_pl->inv.iitems[i]) * pkt->sell_num;
                //Delete_Item_From_Client(itemid, count, 0, client);

                /* Remove the item from the user's inventory. */
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

            return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
        }
    }
    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
    //return -1;
}

static int handle_bb_map_warp_55(ship_client_t* c, subcmd_bb_map_warp_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_DEFAULT) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0028) || pkt->size != 0x08 || pkt->client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    if (c->client_id == pkt->client_id) {
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
}

/* 处理BB 0x60 数据包. */
int subcmd_bb_handle_bcast(ship_client_t* c, bb_subcmd_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0, sent = 1, i;

    //DBG_LOG("BB处理 GC %" PRIu32 " 60指令: 0x%02X", c->guildcard, type);

    //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    /* Ignore these if the client isn't in a lobby. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        switch (type) {
        case SUBCMD_LOAD_3B:
        case SUBCMD_BURST_DONE:
            rv = subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);
            break;

        case SUBCMD_SET_AREA:
            rv = handle_bb_set_area(c, (subcmd_bb_set_area_t*)pkt);
            break;

        case SUBCMD_SET_POS_3F:
            rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
            break;

        case SUBCMD_CMODE_GRAVE:
            UNK_CPD(type, (uint8_t*)pkt);
            rv = handle_bb_cmode_grave(c, pkt);
            break;

        default:
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
            break;
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {
    case SUBCMD0x60_SWITCH_CHANGED:
        rv = handle_bb_switch_changed(c, (subcmd_bb_switch_changed_pkt_t*)pkt);
        //rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x03);
        //print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        //sent = 0;
        break;

    case SUBCMD_SYMBOL_CHAT:
        rv = handle_bb_symbol_chat(c, pkt);
        break;

    case SUBCMD_HIT_MONSTER:
        rv = handle_bb_mhit(c, (subcmd_bb_mhit_pkt_t*)pkt);
        break;

    case SUBCMD_HIT_OBJ:
        /* Don't even try to deal with these in battle or challenge mode
        不要尝试在战斗或挑战模式中处理这些问题. */
        if (l->challenge || l->battle) {
            sent = 0;
            break;
        }

        rv = handle_bb_objhit(c, (subcmd_bb_bhit_pkt_t*)pkt);
        break;

    case SUBCMD_CONDITION_ADD:
        rv = handle_bb_cmd_check_size(c, pkt, 0x03);
        break;

    case SUBCMD_CONDITION_REMOVE:
        rv = handle_bb_cmd_check_size(c, pkt, 0x03);
        break;

    case SUBCMD0x60_ACTION_DRAGON:// Dragon actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD0x60_ACTION_DE_ROl_LE:// De Rol Le actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD0x60_UNKNOW_14:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD0x60_ACTION_VOL_OPT:// Vol Opt actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD0x60_ACTION_VOL_OPT2:// Vol Opt actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD_TELEPORT:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD0x60_TELEPORT_2:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD0x60_ACTION_DARK_FALZ:// Dark Falz actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD0x60_UNKNOW_1C:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD_SET_AREA:
        rv = handle_bb_set_area(c, (subcmd_bb_set_area_t*)pkt);
        break;

    case SUBCMD0x60_SET_AREA_20:
        rv = handle_bb_set_area(c, (subcmd_bb_set_area_t*)pkt);
        break;

    case SUBCMD_SET_AREA_21:
        rv = handle_bb_set_area(c, (subcmd_bb_set_area_t*)pkt);
        break;

    case SUBCMD_LOAD_22:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD_SET_POS_24:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD_EQUIP:
        rv = handle_bb_equip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD_REMOVE_EQUIP:
        rv = handle_bb_unequip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD_USE_ITEM:
        rv = handle_bb_use_item(c, (subcmd_bb_use_item_t*)pkt);
        break;

    case SUBCMD0x60_FEED_MAG:
        rv = handle_bb_feed_mag(c, (subcmd_bb_feed_mag_t*)pkt);
        break;

    case SUBCMD_DELETE_ITEM:
        rv = handle_bb_destroy_item(c, (subcmd_bb_destroy_item_t*)pkt);
        break;

    case SUBCMD_DROP_ITEM:
        rv = handle_bb_drop_item(c, (subcmd_bb_drop_item_t*)pkt);
        break;

    case SUBCMD_TAKE_ITEM:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD_TALK_NPC:
        rv = handle_bb_talk_npc(c, (subcmd_bb_talk_npc_t*)pkt);
        break;

    case SUBCMD_DONE_NPC:
        rv = handle_bb_done_npc(c, (bb_subcmd_pkt_t*)pkt);
        break;

    case SUBCMD0x60_UNKNOW_2E:
    case SUBCMD0x60_UNKNOW_2F:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD_LEVELUP:
        rv = handle_bb_level_up(c, (subcmd_bb_level_t*)pkt);
        break;

    case SUBCMD0x60_UNKNOW_MEDIC_31:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD0x60_UNKNOW_MEDIC_32:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD0x60_UNKNOW_33:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_UNKNOW_34:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_UNKNOW_35:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_UNKNOW_36:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_UNKNOW_37:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_UNKNOW_38:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_UNKNOW_39:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_UNKNOW_3A:
        rv = handle_bb_cmd_3a(c, (subcmd_bb_cmd_3a_t*)pkt);
        break;

    case SUBCMD_LOAD_3B:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, SUBCMD_LOAD_3B_SIZE);
        break;

    case SUBCMD0x60_UNKNOW_3C:
    case SUBCMD0x60_UNKNOW_3D:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD_SET_POS_3E:
    case SUBCMD_SET_POS_3F:
        rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
        break;

    case SUBCMD0x60_UNKNOW_41:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD_MOVE_SLOW:
    case SUBCMD_MOVE_FAST:
        rv = handle_bb_move(c, (subcmd_bb_move_t*)pkt);
        break;

    case SUBCMD0x60_ATTACK1:
        rv = handle_bb_normal_attack(c, (subcmd_bb_natk_t*)pkt);
        break;

    case SUBCMD0x60_ATTACK2:
        rv = handle_bb_normal_attack(c, (subcmd_bb_natk_t*)pkt);
        break;

    case SUBCMD0x60_ATTACK3:
        rv = handle_bb_normal_attack(c, (subcmd_bb_natk_t*)pkt);
        break;

    case SUBCMD_OBJHIT_PHYS:
        /* Don't even try to deal with these in battle or challenge mode. */
        if (l->challenge || l->battle) {
            sent = 0;
            break;
        }

        rv = handle_bb_objhit_phys(c, (subcmd_bb_objhit_phys_t*)pkt);
        break;

    case SUBCMD_OBJHIT_TECH:
        /* Don't even try to deal with these in battle or challenge mode. */
        if (l->challenge || l->battle) {
            sent = 0;
            break;
        }

        rv = handle_bb_objhit_tech(c, (subcmd_bb_objhit_tech_t*)pkt);
        break;

    case SUBCMD_USED_TECH:
        rv = handle_bb_used_tech(c, (subcmd_bb_used_tech_t*)pkt);
        break;

    case SUBCMD0x60_TAKE_DAMAGE:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD0x60_DEATH_SYNC:
        rv = handle_bb_death_sync(c, (subcmd_bb_death_sync_t*)pkt);
        break;

    case SUBCMD0x60_UNKNOW_4E:
        rv = handle_bb_cmd_4e(c, (subcmd_bb_cmd_4e_t*)pkt);
        break;

    case SUBCMD0x60_REQ_SWITCH:
        rv = handle_bb_switch_req(c, (subcmd_bb_switch_req_t*)pkt);
        break;

    case SUBCMD_WORD_SELECT:
        rv = handle_bb_word_select(c, (subcmd_bb_word_select_t*)pkt);
        break;

    case SUBCMD0x60_UNKNOW_89:
        rv = handle_bb_player_died(c, (subcmd_bb_player_died_t*)pkt);
        break;

    case SUBCMD0x60_SELL_ITEM:
        rv = handle_bb_sell_item(c, (subcmd_bb_sell_item_t*)pkt);
        break;

    case SUBCMD_DROP_POS:
        rv = handle_bb_drop_pos(c, (subcmd_bb_drop_pos_t*)pkt);
        break;

    case SUBCMD_SORT_INV:
        rv = handle_bb_sort_inv(c, (subcmd_bb_sort_inv_t*)pkt);
        break;

    case SUBCMD_MEDIC:
        rv = handle_bb_medic(c, (bb_subcmd_pkt_t*)pkt);
        break;

    case SUBCMD0x60_STEAL_EXP:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_CHARGE_ACT:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD_REQ_EXP:
        rv = handle_bb_req_exp(c, (subcmd_bb_req_exp_pkt_t*)pkt);
        break;

    case SUBCMD0x60_EX_ITEM_TEAM:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_BATTLEMODE:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD0x60_GALLON_AREA:
        rv = handle_bb_gallon_area(c, (subcmd_bb_gallon_area_pkt_t*)pkt);
        break;

    case SUBCMD_TAKE_DAMAGE1:
    case SUBCMD_TAKE_DAMAGE2:
        rv = handle_bb_take_damage(c, (subcmd_bb_take_damage_t*)pkt);
        break;

    case SUBCMD0x60_MENU_REQ:
        rv = handle_bb_menu_req(c, (subcmd_bb_menu_req_t*)pkt);
        break;

    case SUBCMD0x60_CREATE_ENEMY_SET:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x04);
        break;

    case SUBCMD_CREATE_PIPE:
        rv = handle_bb_create_pipe(c, (subcmd_bb_pipe_pkt_t*)pkt);
        break;

    case SUBCMD_SPAWN_NPC:
        rv = handle_bb_spawn_npc(c, pkt);
        break;

    case SUBCMD0x60_UNKNOW_6A:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD0x60_SET_FLAG:
        rv = handle_bb_set_flag(c, (subcmd_bb_set_flag_t*)pkt);
        break;

    case SUBCMD_KILL_MONSTER:
        rv = handle_bb_killed_monster(c, (subcmd_bb_killed_monster_t*)pkt);
        break;

    case SUBCMD_SYNC_REG:
        rv = handle_bb_sync_reg(c, (subcmd_bb_sync_reg_t*)pkt);
        break;

    case SUBCMD_CMODE_GRAVE:
        rv = handle_bb_cmode_grave(c, pkt);
        break;

        /*挑战模式 触发*/
    case SUBCMD0x60_UNKNOW_7D:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        //rv = handle_bb_cmd_check_game_size(c, pkt, 0x06);
        break;

        /*挑战模式 触发*/
    case SUBCMD0x60_UNKNOW_8A:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        //rv = handle_bb_cmd_check_game_size(c, pkt, 0x02);
        break;

    case SUBCMD0x60_USE_TECH:
        rv = handle_bb_use_tech(c, (subcmd_bb_use_tech_t*)pkt);
        break;

        /*挑战模式 触发*/
    case SUBCMD0x60_UNKNOW_93:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        //rv = handle_bb_cmd_check_game_size(c, pkt, 0x03);
        break;

    case SUBCMD_SHOP_INV:
        rv = handle_bb_shop_inv(c, (subcmd_bb_shop_inv_t*)pkt);
        break;

    case SUBCMD_WARP_55:
        rv = handle_bb_map_warp_55(c, (subcmd_bb_map_warp_t*)pkt);
        //UNK_CPD(type, (uint8_t*)pkt);
        //sent = 0;
        break;

    case SUBCMD_LOBBY_ACTION:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD_GOGO_BALL:
        UNK_CPD(type, (uint8_t*)pkt);
        sent = 0;
        break;

    case SUBCMD_LOBBY_CHAIR:
        rv = handle_bb_chair_dir(c, (subcmd_bb_chair_dir_t*)pkt);
        break;

    case SUBCMD_CHAIR_DIR:
        rv = handle_bb_chair_dir(c, (subcmd_bb_chair_dir_t*)pkt);
        break;

    case SUBCMD_CHAIR_MOVE:
        rv = handle_bb_chair_dir(c, (subcmd_bb_chair_dir_t*)pkt);
        break;

    case SUBCMD0x60_TRADE_DONE:
        rv = handle_bb_trade_done(c, (subcmd_bb_trade_t*)pkt);
        break;

    case SUBCMD0x60_LEVELUP:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x60 指令: 0x%02X", type);
        UNK_CPD(type, pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
        sent = 0;
        break;

    case SUBCMD_FINISH_LOAD:
        if (l->type == LOBBY_TYPE_DEFAULT) {
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
        rv = subcmd_send_lobby_bb(l, c, (bb_subcmd_pkt_t*)pkt, 0);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int subcmd_send_bb_lobby_item(lobby_t* l, subcmd_bb_itemreq_t* req,
    const iitem_t* item) {
    subcmd_bb_itemgen_t gen;
    int i;
    uint32_t tmp = LE32(req->unk2[0]) & 0x0000FFFF;

    /* Fill in the packet we'll send out. */
    gen.hdr.pkt_type = GAME_COMMAND0_TYPE;
    gen.hdr.flags = 0;
    gen.hdr.pkt_len = LE16(0x0030);
    gen.type = SUBCMD_ITEMDROP;
    gen.size = 0x0B;
    gen.params = 0;
    gen.area = req->area;
    gen.what = req->pt_index;   /* Probably not right... but whatever. */
    gen.req = req->req;
    gen.x = req->x;
    gen.y = req->y;
    gen.unk1 = LE32(tmp);       /* ??? */

    gen.item.data_l[0] = LE32(item->data_l[0]);
    gen.item.data_l[1] = LE32(item->data_l[1]);
    gen.item.data_l[2] = LE32(item->data_l[2]);
    gen.item.data2_l = LE32(item->data2_l);

    gen.item.item_id = LE32(item->item_id);

    /* Send the packet to every client in the lobby. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]) {
            send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&gen);
        }
    }

    return 0;
}

int subcmd_send_bb_exp(ship_client_t* c, uint32_t exp) {
    subcmd_bb_exp_t pkt;

    /* Fill in the packet. */
    pkt.hdr.pkt_len = LE16(0x0010);
    pkt.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt.hdr.flags = 0;
    pkt.type = SUBCMD_GIVE_EXP;
    pkt.size = 0x02;
    pkt.client_id = c->client_id;
    pkt.unused = 0;
    pkt.exp = LE32(exp);

    return subcmd_send_lobby_bb(c->cur_lobby, NULL, (bb_subcmd_pkt_t*)&pkt, 0);
}

int subcmd_send_bb_level(ship_client_t* c) {
    subcmd_bb_level_t pkt;
    int i;
    uint16_t base, mag;

    /* Fill in the packet. */
    pkt.hdr.pkt_len = LE16(0x001C);
    pkt.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    pkt.hdr.flags = 0;
    pkt.type = SUBCMD_LEVELUP;
    pkt.size = 0x05;
    pkt.client_id = c->client_id;
    pkt.unused = 0;

    /* Fill in the base statistics. These are all in little-endian already. */
    pkt.atp = c->bb_pl->character.disp.stats.atp;
    pkt.mst = c->bb_pl->character.disp.stats.mst;
    pkt.evp = c->bb_pl->character.disp.stats.evp;
    pkt.hp = c->bb_pl->character.disp.stats.hp;
    pkt.dfp = c->bb_pl->character.disp.stats.dfp;
    pkt.ata = c->bb_pl->character.disp.stats.ata;
    pkt.level = c->bb_pl->character.disp.level;

    /* Add in the mag's bonus. */
    for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
        if ((c->bb_pl->inv.iitems[i].flags & LE32(0x00000008)) &&
            c->bb_pl->inv.iitems[i].data_b[0] == 0x02) {
            base = LE16(pkt.dfp);
            mag = LE16(c->bb_pl->inv.iitems[i].data_w[2]) / 100;
            pkt.dfp = LE16((base + mag));

            base = LE16(pkt.atp);
            mag = LE16(c->bb_pl->inv.iitems[i].data_w[3]) / 50;
            pkt.atp = LE16((base + mag));

            base = LE16(pkt.ata);
            mag = LE16(c->bb_pl->inv.iitems[i].data_w[4]) / 200;
            pkt.ata = LE16((base + mag));

            base = LE16(pkt.mst);
            mag = LE16(c->bb_pl->inv.iitems[i].data_w[5]) / 50;
            pkt.mst = LE16((base + mag));

            break;
        }
    }

    return subcmd_send_lobby_bb(c->cur_lobby, NULL, (bb_subcmd_pkt_t*)&pkt, 0);
}

/* It is assumed that all parameters are already in little-endian order. */
static int subcmd_send_drop_stack(ship_client_t* c, uint32_t area, float x,
    float z, iitem_t* item) {
    subcmd_bb_drop_stack_t drop;

    /* Fill in the packet... */
    drop.hdr.pkt_len = LE16(0x002C);
    drop.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    drop.hdr.flags = 0;
    drop.type = SUBCMD_DROP_STACK;
    drop.size = 0x09;
    drop.client_id = c->client_id;
    drop.unused = 0;
    drop.area = area;
    drop.x = x;
    drop.z = z;
    drop.item[0] = item->data_l[0];
    drop.item[1] = item->data_l[1];
    drop.item[2] = item->data_l[2];
    drop.item_id = item->item_id;
    drop.item2 = item->data2_l;

    return subcmd_send_lobby_bb(c->cur_lobby, NULL, (bb_subcmd_pkt_t*)&drop,
        0);
}

static int subcmd_send_create_item(ship_client_t* c, item_t item, int send_to_client) {
    subcmd_bb_create_item_t new_item;

    /* Fill in the packet. */
    new_item.hdr.pkt_len = LE16(0x0024);
    new_item.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    new_item.hdr.flags = 0;
    new_item.type = SUBCMD_CREATE_ITEM;
    new_item.size = 0x07;
    new_item.client_id = c->client_id;
    new_item.unused = 0;
    new_item.item = item;
    //pick.item[0] = data_l[0];
    //pick.item[1] = data_l[1];
    //pick.item[2] = data_l[2];
    //pick.item_id = item_id;
    //pick.item2 = data2_l;
    new_item.unused2 = 0;

    if (send_to_client)
        return subcmd_send_lobby_bb(c->cur_lobby, NULL,
            (bb_subcmd_pkt_t*)&new_item, 0);
    else
        return subcmd_send_lobby_bb(c->cur_lobby, c, (bb_subcmd_pkt_t*)&new_item,
            0);
}

static int subcmd_send_destroy_map_item(ship_client_t* c, uint8_t area,
    uint32_t item_id) {
    subcmd_bb_destroy_map_item_t d;

    /* Fill in the packet. */
    d.hdr.pkt_len = LE16(0x0014);
    d.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    d.hdr.flags = 0;
    d.type = SUBCMD_DEL_MAP_ITEM;
    d.size = 0x03;
    d.client_id = c->client_id;
    d.unused = 0;
    d.client_id2 = c->client_id;
    d.unused2 = 0;
    d.area = area;
    d.unused3 = 0;
    d.item_id = item_id;

    return subcmd_send_lobby_bb(c->cur_lobby, NULL, (bb_subcmd_pkt_t*)&d, 0);
}

static int subcmd_send_destroy_item(ship_client_t* c, uint32_t item_id,
    uint8_t amt) {
    subcmd_bb_destroy_item_t d;

    /* Fill in the packet. */
    d.hdr.pkt_len = LE16(0x0014);
    d.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    d.hdr.flags = 0;
    d.type = SUBCMD_DELETE_ITEM;
    d.size = 0x03;
    d.client_id = c->client_id;
    d.unused = 0;
    d.item_id = item_id;
    d.amount = LE32(amt);

    return subcmd_send_lobby_bb(c->cur_lobby, c, (bb_subcmd_pkt_t*)&d, 0);
}

//static int subcmd_send_add_item(ship_client_t* c, uint32_t count, int32_t 是否商店, iitem_t *玩家背包) {
//    uint32_t ch;
//    uint8_t stackable = 0;
//    uint32_t stack_count;
//    uint32_t compare_item1 = 0;
//    uint32_t compare_item2 = 0;
//    uint32_t item_added = 0;
//    uint32_t notsend;
//    lobby_t* l = c->cur_lobby;
//
//    // Adds an item to the client's inventory... (out of thin air)
//    // The new itemid must already be set to i->item.item_id
//    //将项目添加到客户的库存。。。（凭空）
//
//    //新的itemid必须已经设置为i->item.item_id
//
//    if (!l)
//        return 0;
//
//    if (玩家背包->data_b[0] == 0x04)
//    {
//        tmp = LE32(item_data.data2_l) + LE32(c->bb_pl->character.meseta);
//
//        /* Cap at 999,999 meseta. */
//        if (tmp > 999999)
//            tmp = 999999;
//        // Meseta
//        count = *(uint32_t*)&玩家背包->data2_b[0];
//        c->pl->bb .meseta += count;
//        if (client->Full_Char.meseta > 999999)
//            client->Full_Char.meseta = 999999;
//        item_added = 1;
//    }
//    else
//    {
//        if (玩家背包->data_b[0] == 0x03)
//            stackable = item_is_stackable(玩家背包->data_b[1]);
//    }
//
//    if ((!client->todc) && (!item_added))
//    {
//        if (stackable)
//        {
//            if (!count)
//                count = 1;
//            memcpy(&compare_item1, &玩家背包->data_b[0], 3);
//            for (ch = 0; ch < client->Full_Char.inventoryUse; ch++)
//            {
//                memcpy(&compare_item2, &client->Full_Char.inventory[ch].data_b[0], 3);
//                if (compare_item1 == compare_item2)
//                {
//                    stack_count = client->Full_Char.inventory[ch].data_b[5];
//                    if (!stack_count)
//                        stack_count = 1;
//                    if ((stack_count + count) > stackable)
//                    {
//                        count = stackable - stack_count;
//                        client->Full_Char.inventory[ch].data_b[5] = stackable;
//                    }
//                    else
//                        client->Full_Char.inventory[ch].data_b[5] = (uint8_t)(stack_count + count);
//                    item_added = 1;
//                    break;
//                }
//            }
//        }
//
//        if (item_added == 0) // Make sure we don't go over the max inventory确保我们没有超过最大库存
//        {
//            if ((client->Full_Char.inventoryUse >= 30))
//            {//缺失 Sancaros
//                Message_Box(L"", L"inventory limit reached.", client, SideMes_PktB0, NULL, 60);
//                Todc_Check(__LINE__, client); // 暂时移除用于修复挑战模式 11.18
//            }
//            else
//            {
//                // Give item to client...
//                client->Full_Char.inventory[client->Full_Char.inventoryUse].num_items = 0x01;
//                client->Full_Char.inventory[client->Full_Char.inventoryUse].equiped = 0x00;
//                memcpy(&client->Full_Char.inventory[client->Full_Char.inventoryUse].item, &玩家背包->item, sizeof(ITEM_DATA));
//                if (stackable)
//                {
//                    memset(&client->Full_Char.inventory[client->Full_Char.inventoryUse].data_b[4], 0, 4);
//                    client->Full_Char.inventory[client->Full_Char.inventoryUse].data_b[5] = (uint8_t)count;
//                }
//                client->Full_Char.inventoryUse++;
//                item_added = 1;
//            }
//        }
//    }
//
//    if ((!client->todc) && (item_added))
//    {
//        // Let people know the client has a new toy...让人们知道该玩家获得了一个新玩具
//        memset(&client->encrypt_buf_code[0x00], 0, 0x24);
//        client->encrypt_buf_code[0x00] = 0x24;
//        client->encrypt_buf_code[0x02] = 0x60;
//        client->encrypt_buf_code[0x08] = 0xBE;
//        client->encrypt_buf_code[0x09] = 0x09;
//        client->encrypt_buf_code[0x0A] = client->clientID;
//        memcpy(&client->encrypt_buf_code[0x0C], &玩家背包->data_b[0], 12);
//        *(uint32_t*)&client->encrypt_buf_code[0x18] = 玩家背包->data_b;
//        if ((!stackable) || (玩家背包->data_b[0] == 0x04))
//            *(uint32_t*)&client->encrypt_buf_code[0x1C] = *(uint32_t*)&玩家背包->data2_b[0];
//        else
//            client->encrypt_buf_code[0x11] = count;
//        memset(&client->encrypt_buf_code[0x20], 0, 4);
//        if (是否商店)
//            notsend = client->guildcard;
//        else
//            notsend = 0;
//        Send_Packet_To_Lobby(client->Lobby_Room, 4, &client->encrypt_buf_code[0x00], 0x24, notsend);
//    }
//    return item_added;
//}