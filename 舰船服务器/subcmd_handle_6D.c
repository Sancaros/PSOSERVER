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
#include "iitems.h"
#include "word_select.h"
#include "scripts.h"
#include "shipgate.h"
#include "quest_functions.h"
#include "mag_bb.h"

#include "subcmd_handle.h"

int sub6D_70_bb2(ship_client_t* c, ship_client_t* d,
    subcmd_bb_burst_pldata_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t ch_class = c->bb_pl->character.dress_data.ch_class;
    iitem_t* item;
    int i, rv = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅中触发传送中的玩家数据!", c->guildcard);
        return -1;
    }

    if ((c->version == CLIENT_VERSION_XBOX && d->version == CLIENT_VERSION_GC) ||
        (d->version == CLIENT_VERSION_XBOX && c->version == CLIENT_VERSION_GC)) {
        /* 扫描库存并在发送之前修复所有mag. */


        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* 如果项目是mag,那么我们必须交换数据的最后一个dword.否则,颜色和统计数据会变得一团糟 */
            if (item->data.data_b[0] == ITEM_TYPE_MAG) {
                item->data.data2_l = SWAP32(item->data.data2_l);
            }
        }
    }

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

int subcmd_bb_handle_6D(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
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

    DBG_LOG("GC %u 目标客户端-> %d GC %u 0x%02X 指令: 0x%02X",c->guildcard, dnum, l->clients[dnum]->guildcard, hdr_type, type);

    //subcmd_bb_626Dsize_check(c, pkt);

    l->subcmd6D_handle = subcmd_get_handler(hdr_type, type, c->version);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;

        switch (type) {
        case SUBCMD6D_BURST1://0x6D 6D //其他大厅跃迁进房时触发 1
        case SUBCMD6D_BURST2://0x6D 6B //其他大厅跃迁进房时触发 2
        case SUBCMD6D_BURST3://0x6D 6C //其他大厅跃迁进房时触发 3
        case SUBCMD6D_BURST4://0x6D 6E //其他大厅跃迁进房时触发 4
            //if (l->flags & LOBBY_FLAG_QUESTING)
            //    rv = lobby_enqueue_burst_bb(l, c, (bb_pkt_hdr_t*)pkt);

            ///* Fall through... */
            //rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            //break;

        case SUBCMD6D_BURST_PLDATA://0x6D 70 //其他大厅跃迁进房时触发 7
            rv = l->subcmd6D_handle(c, dest, pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x62 指令: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

    }
    else {

        if (l->subcmd6D_handle == NULL) {
#ifdef BB_LOG_UNKNOWN_SUBS
            DBG_LOG("未知 0x%02X 指令: 0x%02X", hdr_type, type);
            display_packet(pkt, len);
#endif /* BB_LOG_UNKNOWN_SUBS */
            rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        }else
            rv = l->subcmd6D_handle(c, dest, pkt);
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}
