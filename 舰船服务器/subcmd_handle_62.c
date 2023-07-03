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

/* 处理BB 0x62 数据包. */
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

    l->subcmd62_handle = subcmd_get_handler(hdr_type, type, c->version);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;

        switch (type) {
        case SUBCMD62_BURST5://0x62 6F //其他大厅跃迁进房时触发 5
        case SUBCMD62_BURST6://0x62 71 //其他大厅跃迁进房时触发 6
            //send_bb_quest_data1(dest, &c->bb_pl->quest_data1);

            rv |= l->subcmd62_handle(c, dest, pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x62 指令: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

    }
    else {
        switch (type) {
        case SUBCMD62_ITEMREQ:
        case SUBCMD62_BITEMREQ:
            rv = l->dropfunc(c, l, pkt);
            pthread_mutex_unlock(&l->mutex);
            return rv;
        }

        if (l->subcmd62_handle == NULL) {
#ifdef BB_LOG_UNKNOWN_SUBS
            DBG_LOG("未知 0x%02X 指令: 0x%02X", hdr_type, type);
            display_packet(pkt, len);
            //UNK_CSPD(type, c->version, pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
            rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
        }
        else
            rv = l->subcmd62_handle(c, dest, pkt);
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}
