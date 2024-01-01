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
#include <iconv.h>
#include <string.h>
#include <pthread.h>

#include <f_logs.h>
#include <debug.h>
#include <f_iconv.h>
#include <SFMT.h>
#include <items.h>

#include "subcmd.h"
#include "clients.h"
#include "ship_packets.h"
#include "utils.h"
#include "handle_player_items.h"
#include "shop.h"
#include "word_select.h"
#include "scripts.h"
#include "shipgate.h"
#include "quest_functions.h"
#include "subcmd_handle.h"

// 当客户端发送游戏命令时, 调用此文件中的函数
// 指令集
// (60, 62, 6C, 6D, C9, CB).

/* 处理EP3 0xC9/0xCB 数据包. */
int subcmd_handle_ep3_bcast(ship_client_t* c, subcmd_pkt_t* pkt) {
    __try {
        lobby_t* l = c->cur_lobby;
        int rv;

        /* 如果客户端不在大厅或者队伍中则忽略数据包. */
        if (!l)
            return 0;

        pthread_mutex_lock(&l->mutex);

        /* We don't do anything special with these just yet... */
        rv = lobby_send_pkt_ep3(l, c, (dc_pkt_hdr_t*)pkt);

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

int subcmd_send_lobby_item(lobby_t* l, subcmd_itemreq_t* req,
    const uint32_t item[4]) {
    subcmd_itemgen_t gen = { 0 };
    int i;
    uint32_t tmp = /*LE32(req->unk2[0]) & 0x0000FFFF*/LE32(req->unk1);

    /* 填充数据并准备发送. */
    gen.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    gen.hdr.flags = 0;
    gen.hdr.pkt_len = LE16(0x0030);
    gen.shdr.type = SUBCMD60_ITEM_DROP_BOX_ENEMY;
    gen.shdr.size = 0x0B;
    gen.shdr.unused = 0x0000;
    gen.data.area = req->area;
    gen.data.from_enemy = req->pt_index;   /* Probably not right... but whatever. */
    gen.data.request_id = req->request_id;
    gen.data.x = req->x;
    gen.data.z = req->z;
    gen.data.unk1 = LE32(tmp);       /* ??? */

    gen.data.item.data1l[0] = LE32(item[0]);
    gen.data.item.data1l[1] = LE32(item[1]);
    gen.data.item.data1l[2] = LE32(item[2]);
    gen.data.item.data2l = LE32(item[3]);
    gen.data.item2 = LE32(0x00000002);

    gen.data.item.item_id = LE32(l->item_lobby_id);
    ++l->item_lobby_id;

    /* Send the packet to every client in the lobby. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]) {
            send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&gen);
        }
    }

    return 0;
}

int subcmd_send_lobby_dc(lobby_t* l, ship_client_t* c, subcmd_pkt_t* pkt,
    int ignore_check) {
    int i;

    /* Send the packet to every connected client. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            /* If we're supposed to check the ignore list, and this client is on
               it, don't send the packet. */
            if (ignore_check && client_has_ignored(l->clients[i], c->guildcard)) {
                continue;
            }

            if (l->clients[i]->version != CLIENT_VERSION_DCV1 ||
                !(l->clients[i]->flags & CLIENT_FLAG_IS_NTE))
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
            else
                subcmd_translate_dc_to_nte(l->clients[i], pkt);
        }
    }

    return 0;
}

int subcmd_send_pos(ship_client_t* dest, ship_client_t* src) {
    subcmd_set_pos_t dc = { 0 };
    subcmd_bb_set_pos_t bb = { 0 };

    if (dest->version == CLIENT_VERSION_BB) {
        bb.hdr.pkt_len = LE16(0x0020);
        bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
        bb.hdr.flags = 0;

        bb.shdr.type = SUBCMD60_SET_AREA_20;
        bb.shdr.size = 0x06;
        bb.shdr.client_id = src->client_id;

        bb.area = LE32(0x0000000F);            /* Area */
        bb.w = src->x;                         /* X */
        bb.x = 0.0f;                           /* Y */
        bb.y = src->z;                         /* Z */
        bb.z = 0.0f;                           /* Facing, perhaps? */

        return send_pkt_bb(dest, (bb_pkt_hdr_t*)&bb);
    }
    else {
        dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
        dc.hdr.flags = 0;
        dc.hdr.pkt_len = LE16(0x001C);

        if (dest->version == CLIENT_VERSION_DCV1 &&
            (dest->flags & CLIENT_FLAG_IS_NTE))
            dc.shdr.type = SUBCMD60_DESTORY_NPC;
        else
            dc.shdr.type = SUBCMD60_SET_AREA_20;

        dc.shdr.size = 0x06;
        dc.shdr.client_id = src->client_id;

        dc.area = LE32(0x0000000F);            /* Area */
        dc.w = src->x;                         /* X */
        dc.x = 0.0f;                           /* Y */
        dc.y = src->z;                         /* Z */
        dc.z = 0.0f;                           /* Facing, perhaps? */

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&dc);
    }
}
