/*
    梦幻之星中国 舰船服务器 EP3 CA指令处理
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
#include "ship.h"

#include "subcmd_handle.h"

static int subCA_B3_ep3(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    DBG_LOG("Ep3 服务器数据来自 %s (%d)", src->pl->v1.character.dress_data.gc_string,
        src->guildcard);
    PRINT_HEX_LOG(DBG_LOG, pkt, pkt->hdr.dc.pkt_len);
    //[2023年09月24日 16:04:38:035] 调试(block.c 2823): Ep3 服务器数据来自 Sancaros (10000000)
    //[2023年09月24日 16:04:38:037] 调试(f_logs.c 0088): 数据包如下:
    //(00000000) CA 00 14 00 B3 04 42 01  40 00 00 10 FF FF FF FF    ......B.@.......
    //(00000010) 00 00 00 00    ....

    //[2023年10月28日 12:49:08:679] 调试(block.c 2992): Ep3 服务器数据来自 Sancaros (10000000)
    //[2023年10月28日 12:49:08:682] 调试(block.c 2993): 数据包如下:
    //(00000000) CA 00 08 03 B3 C1 1C CE  49 00 00 B8 FF FF FF FF    ........I.......
    //(00000010) 00 00 00 00 EE 00 00 05  27 C4 D4 2B D2 20 A1 27    ........'..+. .'
    //(00000020) BE BE AF 63 E9 98 F9 DE  57 B4 85 9A 03 10 50 94    ...c....W.....P.
    //(00000030) EF AC 5D D2 1B 88 A9 4E  87 A4 35 0A 33 00 01 06    ..]....N..5.3...
    //(00000040) 1D 9E 0D 42 4A 78 59 BE  B7 94 E5 7A 61 F1 B1 76    ...BJxY....za..v
    //(00000050) 4F 8C BD B2 7B 68 09 2E  E5 85 97 EA 93 E0 61 E6    O...{h........a.
    //(00000060) 7F 7C 6D 22 AB 58 B9 9E  17 74 45 5A C3 D0 13 56    .|m".X...tEZ...V
    //(00000070) AF 6E 1C 90 DB 48 69 0C  47 66 F5 CA F1 C0 C1 C6    .n...Hi.Gf......
    //(00000080) DF 5C CD 02 0B 38 19 7E  77 54 A5 3A 23 B0 71 36    .\...8.~wT.:#.q6
    //(00000090) 0D 4C 7D 72 3B 28 C9 EE  A7 44 55 AA 53 A0 21 A6    .L}r;(...DU.S.!.
    //(000000A0) 3F 3C 2F E2 69 18 78 5E  D7 34 05 1B 83 90 D1 16    ?</.i.x^.4......
    //(000000B0) 6F 2C DD 52 9B 08 29 CE  07 24 B5 8B B3 80 81 86    o,.R..)..$......
    //(000000C0) 9F 1C 8D C2 CB F8 D9 3E  37 14 65 FA E3 70 31 F6    .......>7.e..p1.
    //(000000D0) CF 0C 3D 32 FB E8 89 AE  67 04 15 6A 13 62 E3 66    ..=2....g..j.b.f
    //(000000E0) FF FC ED A2 2A D8 39 1E  97 F4 C5 DA 43 50 91 D6    ....*.9.....CP..
    //(000000F0) 2D EC 9D 13 5B C8 E8 8E  C7 E4 75 4A 73 40 41 46    -...[.....uJs@AF
    //(00000100) 5F DC 4D 82 8B B8 99 FE  F7 D4 25 BA A3 30 F1 B6    _.M........0..
    //(00000110) 8F CC FD F2 BB A8 49 6E  27 C4 D5 2A D3 20 A1 26    ......In'..*. .&
    //(00000120) BF BC AD 62 EB 98 F9 DE  57 B4 85 9A 03 10 50 96    ...b....W.....P.
    //(00000130) EF AC 5D D2 1B 88 A9 4E  87 A4 35 0A 33 00 01 06    ..]....N..5.3...
    //(00000140) 1F 9C 0D 42 4B 78 59 BE  B7 94 E5 7A 63 F0 B1 76    ...BKxY....zc..v
    //(00000150) 4F 8C BD B2 7B 68 09 2E  E7 84 95 EA 93 E0 61 E6    O...{h........a.
    //(00000160) 7F 7C 6D 22 AB 58 B9 9E  17 75 45 5A C3 D0 11 56    .|m".X...uEZ...V
    //(00000170) AF 6C 1D 92 DB 48 69 0E  47 64 F5 CA F3 C0 C1 C6    .l...Hi.Gd......
    //(00000180) DF 5C CD 02 0B 38 19 7E  77 54 A5 3A 23 B0 71 36    .\...8.~wT.:#.q6
    //(00000190) 0F 4C 7D 72 3B 28 C9 EE  A7 44 55 AA 53 A0 21 A6    .L}r;(...DU.S.!.
    //(000001A0) 3F 3C 2D E2 6B 18 79 5F  D7 34 05 1A 83 90 D1 16    ?<-.k.y_.4......
    //(000001B0) 6F 2C DD 52 9B 08 29 CE  07 24 B5 88 B3 80 81 86    o,.R..)..$......
    //(000001C0) 9F 1C 8D C2 CB F8 D9 3E  37 14 65 FA E3 70 31 F6    .......>7.e..p1.
    //(000001D0) CF 0C 3D 32 FB E8 89 AE  67 04 15 6A 13 60 E1 66    ..=2....g..j.`.f
    //(000001E0) FF FC ED A2 2B D8 39 1E  97 F4 C5 DA 43 50 91 D6    ....+.9.....CP..
    //(000001F0) 2F EC 9D 12 5B C8 E9 8E  C7 E4 75 4A 73 40 41 46    /...[.....uJs@AF
    //(00000200) 5F DC 4D 82 8B B8 99 FE  F7 D4 25 BA A3 30 F1 B6    _.M........0..
    //(00000210) 8F CC FD F2 BB A8 49 6E  27 C4 D5 2A D3 20 A1 26    ......In'..*. .&
    //(00000220) BF BC AD 62 EB 98 F9 DE  57 B4 85 9A 03 10 51 96    ...b....W.....Q.
    //(00000230) EF AC 5D D2 1B 88 A9 4E  87 A4 35 0A 33 00 01 06    ..]....N..5.3...
    //(00000240) 1F 9C 0D 42 4B 78 59 BE  B7 94 E5 7A 63 F0 B1 76    ...BKxY....zc..v
    //(00000250) 4F 8C BD B2 7B 68 09 2E  E7 84 95 EA 93 E0 60 E6    O...{h........`.
    //(00000260) 7F 7C 6D 22 AB 58 B9 9E  17 74 45 5A C3 D0 11 56    .|m".X...tEZ...V
    //(00000270) AF 6C 1D 92 DB 48 69 0E  47 64 F5 CA F3 C0 C1 C6    .l...Hi.Gd......
    //(00000280) DF 5C CD 02 0B 38 19 7E  77 54 A5 3A 23 B0 71 36    .\...8.~wT.:#.q6
    //(00000290) 0F 4C 7D 72 3B 28 C9 EE  A7 44 55 AA 53 A0 21 A6    .L}r;(...DU.S.!.
    //(000002A0) 3F 3C 2D E2 6B 18 79 5E  D7 34 05 1A 83 90 D1 16    ?<-.k.y^.4......
    //(000002B0) 6F 2C DD 52 9B 08 29 CE  07 24 B5 8A B3 80 81 86    o,.R..)..$......
    //(000002C0) 9F 1C 8D C2 CB F8 D9 3E  37 14 65 FA E3 70 31 F6    .......>7.e..p1.
    //(000002D0) CF 0C 3D 32 FB E8 89 AE  67 04 15 6A 13 60 E1 66    ..=2....g..j.`.f
    //(000002E0) FF FC ED A2 2B D8 39 1E  97 F4 C5 DA 43 50 91 D6    ....+.9.....CP..
    //(000002F0) 2F EC 9D 12 5B C8 E9 8E  C7 E4 75 4A 73 40 41 46    /...[.....uJs@AF
    //(00000300) 5F DC 4D 82 8B B8 99 FE     _.M.....
    if (!in_game(src))
        return -1;

    return 0;
}

// 定义函数指针数组
subcmd_handle_func_t subcmdCA_handler[] = {
    //cmd_type B0 - BF                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMDCA_EP3_SERVER_DATA            , NULL,        NULL,        subCA_B3_ep3,NULL,        NULL,        NULL       },
};

/* 处理 EP3 0xCA 来自客户端的数据包. */
int subcmd_handle_CA(ship_client_t* src, subcmd_pkt_t* pkt) {
    __try {
        uint8_t type = pkt->type;
        lobby_t* l = src->cur_lobby;
        int rv, sent = 1, i = 0;
        ship_client_t* dest;
        uint16_t len = pkt->hdr.dc.pkt_len, hdr_type = pkt->hdr.dc.pkt_type;
        uint8_t dnum = pkt->hdr.dc.flags;

        /* 如果客户端不在大厅或者队伍中则忽略数据包. */
        if (!l)
            return 0;

        pthread_mutex_lock(&l->mutex);

        /* 搜索目标客户端. */
        dest = l->clients[dnum];

#ifdef DEBUG

        DBG_LOG("0x%02X 指令: 0x%02X", pkt->hdr.dc.pkt_type, type);
        DBG_LOG("c version %d", c->version);

        PRINT_HEX_LOG(ERR_LOG, pkt, pkt->hdr.dc.pkt_len);

#endif // DEBUG

        DBG_LOG("0x%02X 指令: 0x%02X", pkt->hdr.dc.pkt_type, type);

        l->subcmd_handle = subcmd_get_handler(hdr_type, type, src->version);
        if (!l->subcmd_handle) {

#ifdef BB_LOG_UNKNOWN_SUBS
            DBG_LOG("未知 %s 0x%02X 指令: 0x%02X", client_type[src->version].ver_name, hdr_type, type);
            PRINT_HEX_LOG(DBG_LOG, pkt, len);
#endif /* BB_LOG_UNKNOWN_SUBS */

            rv = send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
            pthread_mutex_unlock(&l->mutex);
            return rv;
        }

        /* If there's a burst going on in the lobby, delay most packets */
        if (l->flags & LOBBY_FLAG_BURSTING) {
            switch (type) {
            default:
                rv = lobby_enqueue_pkt(l, src, (dc_pkt_hdr_t*)pkt);
            }
        } else {
            rv = l->subcmd_handle(src, dest, pkt);
        }

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
