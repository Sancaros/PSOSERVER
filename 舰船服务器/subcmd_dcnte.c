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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <f_logs.h>

#include "subcmd.h"
#include "clients.h"
#include "ship_packets.h"
#include "utils.h"

// 当客户端发送游戏命令时, 调用此文件中的函数
// 指令集
// (60, 62, 6C, 6D, C9, CB).

static int handle_set_area_1D(ship_client_t* c, subcmd_set_area_1D_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17)
        return -1;

    /* Save the new area and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->cur_area = pkt->area;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;
    }

    return subcmd_send_lobby_dcnte(l, c, (subcmd_pkt_t*)pkt, 0);
}

static int handle_set_pos(ship_client_t* c, subcmd_set_pos_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->w = pkt->w;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;
    }

    return subcmd_send_lobby_dcnte(l, c, (subcmd_pkt_t*)pkt, 0);
}

static int handle_move(ship_client_t* c, subcmd_move_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->x = pkt->x;
        c->z = pkt->z;
    }

    return subcmd_send_lobby_dcnte(l, c, (subcmd_pkt_t*)pkt, 0);
}


int subcmd_dcnte_handle_bcast(ship_client_t* c, subcmd_pkt_t* pkt) {
    __try {
        uint8_t type = pkt->type;
        lobby_t* l = c->cur_lobby;
        int rv, sent = 1, i;

        /* 如果客户端不在大厅或者队伍中则忽略数据包. */
        if (!l)
            return 0;

        pthread_mutex_lock(&l->mutex);

        switch (type) {
        case SUBCMD60_DCNTE_SET_AREA:
            rv = handle_set_area_1D(c, (subcmd_set_area_1D_t*)pkt);
            break;

        case SUBCMD60_DCNTE_SET_POS:
            rv = handle_set_pos(c, (subcmd_set_pos_t*)pkt);
            break;

        case SUBCMD60_DCNTE_MOVE_SLOW:
        case SUBCMD60_DCNTE_MOVE_FAST:
            rv = handle_move(c, (subcmd_move_t*)pkt);
            break;

        default:
#ifdef LOG_UNKNOWN_SUBS
            DBG_LOG("未知 0x60 指令: 0x%02X %s", type, c_cmd_name(type, 0));
            print_ascii_hex((unsigned char*)pkt, LE16(pkt->hdr.dc.pkt_len));
#endif /* LOG_UNKNOWN_SUBS */
            sent = 0;
            break;

        case SUBCMD60_SET_AREA_1F:
            if (l->type == LOBBY_TYPE_LOBBY) {
                for (i = 0; i < l->max_clients; ++i) {
                    if (l->clients[i] && l->clients[i] != c &&
                        subcmd_send_pos(c, l->clients[i])) {
                        rv = -1;
                        break;
                    }
                }
            }
            sent = 0;
            break;
        }

        /* Broadcast anything we don't care to check anything about. */
        if (!sent)
            rv = subcmd_send_lobby_dcnte(l, c, pkt, 0);

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

int subcmd_translate_dc_to_nte(ship_client_t* c, subcmd_pkt_t* pkt) {
    __try {
        uint8_t* sendbuf;
        uint8_t newtype = 0xFF;
        uint16_t len = LE16(pkt->hdr.dc.pkt_len);
        int rv;

        switch (pkt->type) {
        case SUBCMD60_INTER_LEVEL_WARP:
            newtype = SUBCMD60_DCNTE_SET_AREA;
            break;

        case SUBCMD60_FINISH_LOAD:
            newtype = SUBCMD60_DCNTE_FINISH_LOAD;
            break;

        case SUBCMD60_SET_POS_3F:
            newtype = SUBCMD60_DCNTE_SET_POS;
            break;

        case SUBCMD60_MOVE_SLOW:
            newtype = SUBCMD60_DCNTE_MOVE_SLOW;
            break;

        case SUBCMD60_MOVE_FAST:
            newtype = SUBCMD60_DCNTE_MOVE_FAST;
            break;

        case SUBCMD60_MENU_REQ:
            newtype = SUBCMD60_DCNTE_TALK_SHOP;
            break;

        default:
#ifdef LOG_UNKNOWN_SUBS
            SHIPS_LOG("Cannot translate DC->NTE packet, dropping");
            print_ascii_hex((unsigned char*)pkt, len);
#endif /* LOG_UNKNOWN_SUBS */
            return 0;
        }

        if (!(sendbuf = (uint8_t*)malloc(len))) {
            ERR_LOG("sendbuf malloc error");
            return -1;
        }

        memcpy(sendbuf, pkt, len);
        pkt = (subcmd_pkt_t*)sendbuf;
        pkt->type = newtype;

        rv = send_pkt_dc(c, (dc_pkt_hdr_t*)sendbuf);
        free_safe(sendbuf);
        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

int subcmd_translate_bb_to_nte(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    __try {
        uint8_t* sendbuf;
        uint8_t newtype = 0xFF;
        uint16_t len = LE16(pkt->hdr.pkt_len);
        int rv;

        switch (pkt->type) {
        case SUBCMD60_INTER_LEVEL_WARP:
            newtype = SUBCMD60_DCNTE_SET_AREA;
            break;

        case SUBCMD60_FINISH_LOAD:
            newtype = SUBCMD60_DCNTE_FINISH_LOAD;
            break;

        case SUBCMD60_SET_POS_3F:
            newtype = SUBCMD60_DCNTE_SET_POS;
            break;

        case SUBCMD60_MOVE_SLOW:
            newtype = SUBCMD60_DCNTE_MOVE_SLOW;
            break;

        case SUBCMD60_MOVE_FAST:
            newtype = SUBCMD60_DCNTE_MOVE_FAST;
            break;

        case SUBCMD60_MENU_REQ:
            newtype = SUBCMD60_DCNTE_TALK_SHOP;
            break;

        default:
#ifdef LOG_UNKNOWN_SUBS
            SHIPS_LOG("Cannot translate BB->NTE packet, dropping");
            print_ascii_hex((unsigned char*)pkt, len);
#endif /* LOG_UNKNOWN_SUBS */
            return 0;
        }

        if (!(sendbuf = (uint8_t*)malloc(len))) {
            ERR_LOG("sendbuf malloc error");
            return -1;
        }

        memcpy(sendbuf, pkt, len);
        pkt = (subcmd_bb_pkt_t*)sendbuf;
        pkt->type = newtype;

        rv = send_pkt_bb(c, (bb_pkt_hdr_t*)sendbuf);
        free_safe(sendbuf);
        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

int subcmd_translate_nte_to_dc(ship_client_t* c, subcmd_pkt_t* pkt) {
    __try {
        uint8_t* sendbuf;
        uint8_t newtype = 0xFF;
        uint16_t len = LE16(pkt->hdr.dc.pkt_len);
        int rv;

        switch (pkt->type) {
        case SUBCMD60_DCNTE_SET_AREA:
            newtype = SUBCMD60_INTER_LEVEL_WARP;
            break;

        case SUBCMD60_DCNTE_FINISH_LOAD:
            newtype = SUBCMD60_FINISH_LOAD;
            break;

        case SUBCMD60_DCNTE_SET_POS:
            newtype = SUBCMD60_SET_POS_3F;
            break;

        case SUBCMD60_DCNTE_MOVE_SLOW:
            newtype = SUBCMD60_MOVE_SLOW;
            break;

        case SUBCMD60_DCNTE_MOVE_FAST:
            newtype = SUBCMD60_MOVE_FAST;
            break;

        case SUBCMD60_DCNTE_TALK_SHOP:
            newtype = SUBCMD60_MENU_REQ;
            break;

        default:
#ifdef LOG_UNKNOWN_SUBS
            SHIPS_LOG("Cannot translate NTE->DC packet, dropping");
            print_ascii_hex((unsigned char*)pkt, len);
#endif /* LOG_UNKNOWN_SUBS */
            return 0;
        }

        if (!(sendbuf = (uint8_t*)malloc(len))) {
            ERR_LOG("sendbuf malloc error");
            return -1;
        }

        memcpy(sendbuf, pkt, len);
        pkt = (subcmd_pkt_t*)sendbuf;
        pkt->type = newtype;

        rv = send_pkt_dc(c, (dc_pkt_hdr_t*)sendbuf);
        free_safe(sendbuf);
        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

int subcmd_send_lobby_dcnte(lobby_t* l, ship_client_t* c, subcmd_pkt_t* pkt,
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

            if (l->clients[i]->version == CLIENT_VERSION_DCV1 &&
                (l->clients[i]->flags & CLIENT_FLAG_IS_NTE))
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
            else
                subcmd_translate_nte_to_dc(l->clients[i], pkt);
        }
    }

    return 0;
}
