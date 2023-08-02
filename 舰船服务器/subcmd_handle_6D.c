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

#include "subcmd_handle.h"

int sub6D_6D_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (l->flags & LOBBY_FLAG_QUESTING)
        rv = lobby_enqueue_burst(l, src, (dc_pkt_hdr_t*)pkt);

    /* Fall through... */
    rv |= send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);

    return rv;
}

int sub6D_6D_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (l->flags & LOBBY_FLAG_QUESTING)
        rv = lobby_enqueue_burst_bb(l, src, (bb_pkt_hdr_t*)pkt);

    /* Fall through... */
    rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);

    return rv;
}

int sub6D_6B_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (l->flags & LOBBY_FLAG_QUESTING)
        rv = lobby_enqueue_burst(l, src, (dc_pkt_hdr_t*)pkt);

    /* Fall through... */
    rv |= send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);

    return rv;
}

int sub6D_6B_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (l->flags & LOBBY_FLAG_QUESTING)
        rv = lobby_enqueue_burst_bb(l, src, (bb_pkt_hdr_t*)pkt);

    /* Fall through... */
    rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);

    return rv;
}

int sub6D_6C_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (l->flags & LOBBY_FLAG_QUESTING)
        rv = lobby_enqueue_burst(l, src, (dc_pkt_hdr_t*)pkt);

    /* Fall through... */
    rv |= send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);

    return rv;
}

int sub6D_6C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (l->flags & LOBBY_FLAG_QUESTING)
        rv = lobby_enqueue_burst_bb(l, src, (bb_pkt_hdr_t*)pkt);

    /* Fall through... */
    rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);

    return rv;
}

int sub6D_6E_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (l->flags & LOBBY_FLAG_QUESTING)
        rv = lobby_enqueue_burst(l, src, (dc_pkt_hdr_t*)pkt);

    /* Fall through... */
    rv |= send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);

    return rv;
}

int sub6D_6E_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (l->flags & LOBBY_FLAG_QUESTING)
        rv = lobby_enqueue_burst_bb(l, src, (bb_pkt_hdr_t*)pkt);

    /* Fall through... */
    rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);

    return rv;
}

int sub6D_70_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_burst_pldata_t* pkt) {
    int i;
    iitem_t* item;
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " sent burst player data in "
            "lobby!\n", src->guildcard);
        return -1;
    }

    if ((src->version == CLIENT_VERSION_XBOX && dest->version == CLIENT_VERSION_GC) ||
        (dest->version == CLIENT_VERSION_XBOX && src->version == CLIENT_VERSION_GC)) {
        /* Scan the inventory and fix any mags before sending it along. */
        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* If the item is a mag, then we have to swap the last dword of the
                data. Otherwise colors and stats get messed up. */
            if (item->data.datab[0] == ITEM_TYPE_MAG) {
                item->data.data2l = SWAP32(item->data.data2l);
            }
        }
    }

    return send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
}

int sub6D_70_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_burst_pldata_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t ch_class = src->bb_pl->character.dress_data.ch_class;
    iitem_t* item;
    int i, rv = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅中触发传送中的玩家数据!", src->guildcard);
        return -1;
    }

    if ((src->version == CLIENT_VERSION_XBOX && dest->version == CLIENT_VERSION_GC) ||
        (dest->version == CLIENT_VERSION_XBOX && src->version == CLIENT_VERSION_GC)) {
        /* 扫描库存并在发送之前修复所有mag. */


        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* 如果项目是mag,那么我们必须交换数据的最后一个dword.否则,颜色和统计数据会变得一团糟 */
            if (item->data.datab[0] == ITEM_TYPE_MAG) {
                item->data.data2l = SWAP32(item->data.data2l);
            }
        }
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
}

// 定义函数指针数组
subcmd_handle_func_t subcmd6D_handler[] = {
    //    cmd_type                         DC           GC           EP3          XBOX         PC           BB
    { SUBCMD6D_BURST1                    , sub6D_6D_dc, sub6D_6D_dc, sub6D_6D_dc, sub6D_6D_dc, sub6D_6D_dc, sub6D_6D_bb },
    { SUBCMD6D_BURST2                    , sub6D_6B_dc, sub6D_6B_dc, sub6D_6B_dc, sub6D_6B_dc, sub6D_6B_dc, sub6D_6B_bb },
    { SUBCMD6D_BURST3                    , sub6D_6C_dc, sub6D_6C_dc, sub6D_6C_dc, sub6D_6C_dc, sub6D_6C_dc, sub6D_6C_bb },
    { SUBCMD6D_BURST4                    , sub6D_6E_dc, sub6D_6E_dc, sub6D_6E_dc, sub6D_6E_dc, sub6D_6E_dc, sub6D_6E_bb },
    { SUBCMD6D_BURST_PLDATA              , sub6D_70_dc, sub6D_70_dc, sub6D_70_dc, sub6D_70_dc, sub6D_70_dc, sub6D_70_bb },
};

/* 处理 DC GC PC V1 V2 0x6D 来自客户端的数据包. */
int subcmd_handle_6D(ship_client_t* src, subcmd_pkt_t* pkt) {
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
        case SUBCMD6D_BURST1:
        case SUBCMD6D_BURST2:
        case SUBCMD6D_BURST3:
        case SUBCMD6D_BURST4:
        case SUBCMD6D_BURST_PLDATA:
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

/* 处理BB 0x6D 数据包. */
int subcmd_bb_handle_6D(ship_client_t* src, subcmd_bb_pkt_t* pkt) {
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

        DBG_LOG("GC %u 目标客户端-> %d GC %u 0x%02X 指令: 0x%02X", src->guildcard, dnum, l->clients[dnum]->guildcard, hdr_type, type);

#endif // DEBUG

        //subcmd_bb_626Dsize_check(c, pkt);

        l->subcmd_handle = subcmd_get_handler(hdr_type, type, src->version);

        /* If there's a burst going on in the lobby, delay most packets */
        if (l->flags & LOBBY_FLAG_BURSTING) {
            rv = 0;

            switch (type) {
            case SUBCMD6D_BURST1://0x6D 6D //其他大厅跃迁进房时触发 1
            case SUBCMD6D_BURST2://0x6D 6B //其他大厅跃迁进房时触发 2
            case SUBCMD6D_BURST3://0x6D 6C //其他大厅跃迁进房时触发 3
            case SUBCMD6D_BURST4://0x6D 6E //其他大厅跃迁进房时触发 4
            case SUBCMD6D_BURST_PLDATA://0x6D 70 //其他大厅跃迁进房时触发 7
                rv = l->subcmd_handle(src, dest, pkt);
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
