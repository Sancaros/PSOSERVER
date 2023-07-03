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
#include "subcmd_handle.h"
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

/* 处理BB 0x62/0x6D 数据包. */
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

    DBG_LOG("客户端 %d GC %u 0x%02X 指令: 0x%02X", dnum, l->clients[dnum]->guildcard, hdr_type, type);

    l->subcmd62_handle = subcmd62_get_handler(type, c->version);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;
        switch (type) {
        case SUBCMD62_BURST5://0x62 6F //其他大厅跃迁进房时触发 5
        case SUBCMD62_BURST6://0x62 71 //其他大厅跃迁进房时触发 6
            send_bb_quest_data1(dest, &c->bb_pl->quest_data1);

            rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x62 指令: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {
    case SUBCMD62_ITEMREQ:
    case SUBCMD62_BITEMREQ:
        rv = l->dropfunc(c, l, pkt);
        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    rv = l->subcmd62_handle(c, dest, pkt);

#ifdef BB_LOG_UNKNOWN_SUBS
    if (l->subcmd62_handle == NULL && rv == -1) {
        DBG_LOG("未知 0x%02X 指令: 0x%02X", hdr_type, type);
        display_packet(pkt, len);
        //UNK_CSPD(type, c->version, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }
#endif /* BB_LOG_UNKNOWN_SUBS */

    pthread_mutex_unlock(&l->mutex);
    return rv;
}
