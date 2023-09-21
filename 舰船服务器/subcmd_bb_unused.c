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
#include <SFMT.h>
#include <items.h>

#include "subcmd.h"
#include "subcmd_send.h"
#include "subcmd_handle.h"
#include "clients.h"
#include "ship_packets.h"
#include "utils.h"
#include "handle_player_items.h"
#include "word_select.h"
#include "scripts.h"
#include "shipgate.h"
#include "quest_functions.h"
#include "mag_bb.h"

// 当客户端发送游戏命令时, 调用此文件中的函数
// 指令集
// (60, 62, 6C, 6D, C9, CB).

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

    print_ascii_hex((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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
    
    if (c->new_item.datal[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
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

    if (c->new_item.datal[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
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