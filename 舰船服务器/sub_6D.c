/*
    �λ�֮���й� ���������� 62ָ���
    ��Ȩ (C) 2022, 2023 Sancaros

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
#include "mag_bb.h"

#include "subcmd_handle.h"

/* �Ѹ�Ϊ���ݿ����� */
extern bb_max_tech_level_t max_tech_level[MAX_PLAYER_TECHNIQUES];

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
    subcmd_bb_sync_game_state_header_6x6B_6x6C_6x6D_6x6E_t* pkt) {
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

    if (!in_game(src))
        return -1;

    if ((src->version == CLIENT_VERSION_XBOX && dest->version == CLIENT_VERSION_GC) ||
        (dest->version == CLIENT_VERSION_XBOX && src->version == CLIENT_VERSION_GC)) {
        /* Scan the inventory and fix any mags before sending it along. */
        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* If the item is a mag, then we have to swap the last dword of the
                data. Otherwise colors and stats get messed up. */
            encode_if_mag(&item->data, src->version);
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

    if (!in_game(src))
        return -1;
//( 00000000 )   C8 04 6D 00 01 00 00 00   70 00 00 00 C0 04 00 00  ?m.....p...?..
//( 00000010 )   00 00 00 00 00 00 00 10   CE FE 64 43 00 00 00 00  ........��dC....
//( 00000020 )   B8 FF 7D 43 00 00 00 00   00 C0 FF FF 00 00 00 00  ?}C.....?....
//( 00000030 )   01 00 48 02 00 00 00 00   00 00 00 00 00 00 00 00  ..H.............
//( 00000040 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 00000050 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 00000060 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 00000070 )   00 00 00 00 00 00 00 00   00 00 01 00 80 96 98 00  ............���.
//( 00000080 )   00 00 00 00 00 00 00 00   FF FF 00 00 00 00 00 00  ..............
//( 00000090 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 000000A0 )   FF FF 00 00 00 00 00 00   00 00 00 00 0F 00 00 00  ..............
//( 000000B0 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 000000C0 )   00 10 42 01 FF FF FF FF   FF FF FF FF FF FF FF FF  ..B.
//( 000000D0 )   FF FF FF FF FF FF FF BC   20 20 31 30 30 30 30 30  ? 100000
//( 000000E0 )   30 30 00 00 32 35 35 00   00 32 35 35 00 32 35 35  00..255..255.255
//( 000000F0 )   FF FF FF FF 00 00 00 30   00 30 00 39 00 30 00 30  ...0.0.9.0.0
//( 00000100 )   00 00 00 00 00 00 00 00   09 00 00 03 25 00 00 00  ............%...
//( 00000110 )   00 00 01 00 00 00 00 00   00 00 85 00 49 00 29 00  ..........?I.).
//( 00000120 )   9F AA AA 3E 00 00 00 3F   09 00 42 00 BA 4E 7B 7C  ��?...?..B.�N{|
//( 00000130 )   37 75 0E 73 BA 4E 00 00   00 00 00 00 00 00 00 00  7u.s�N..........
//( 00000140 )   00 00 00 00 12 E7 00 00   E6 01 18 01 62 01 B7 00  .....?.?..b.?
//( 00000150 )   6E 00 F8 01 0A 00 28 00   00 00 8C 41 00 00 20 41  n.?..(...�A.. A
//( 00000160 )   6D 00 00 00 BD DC 29 00   14 82 03 00 1E 00 00 00  m...��)..?.....
//( 00000170 )   02 A7 00 00 4C 00 00 00   00 01 00 00 00 00 00 00  .?.L...........
//( 00000180 )   00 00 00 00 00 00 01 00   00 00 00 00 02 B0 00 00  .............?.
//( 00000190 )   4C 00 00 00 01 01 00 00   00 00 00 00 00 00 00 00  L...............
//( 000001A0 )   01 00 01 00 00 00 00 00   02 A7 00 00 4C 00 00 00  .........?.L...
//( 000001B0 )   02 00 05 00 F4 01 00 00   00 00 00 00 02 00 01 00  ....?..........
//( 000001C0 )   12 00 00 09 01 B0 00 00   50 00 00 00 03 00 00 00  .....?.P.......
//( 000001D0 )   00 04 00 00 00 00 00 00   03 00 01 00 00 00 00 00  ................
//( 000001E0 )   01 A7 00 30 50 00 00 00   03 01 00 00 00 06 00 00  .?0P...........
//( 00000200 )   50 00 00 00 03 0A 02 00   00 04 00 00 00 00 00 00  P...............
//( 00000210 )   05 00 01 00 00 00 00 00   01 00 00 00 44 00 00 00  ............D...
//( 00000220 )   00 01 00 00 00 00 00 00   00 00 00 00 06 00 01 00  ................
//( 00000230 )   00 00 00 00 01 2B 00 3D   50 00 00 00 03 00 02 00  .....+.=P.......
//( 00000240 )   00 04 00 00 00 00 00 00   07 00 01 00 00 00 00 00  ................
//( 00000250 )   01 15 00 00 44 00 00 00   00 0A 00 00 00 00 00 00  ....D...........
//( 00000260 )   00 00 00 00 08 00 01 00   00 00 00 00 01 FF 00 00  ...............
//( 00000270 )   44 00 00 00 00 01 00 00   00 00 01 55 00 00 00 00  D..........U....
//( 00000280 )   09 00 01 00 00 00 00 00   01 3F 00 00 44 00 00 00  .........?..D...
//( 00000290 )   00 0A 00 00 00 00 00 00   00 00 00 00 0A 00 01 00  ................
//( 000002A0 )   00 00 00 00 01 4F 00 00   44 00 00 00 01 01 00 00  .....O..D.......
//( 000002B0 )   00 00 00 00 01 00 00 00   0B 00 01 00 00 00 00 00  ................
//( 000002C0 )   01 00 00 00 50 00 00 00   03 16 03 00 00 01 00 00  ....P...........
//( 000002D0 )   00 00 00 00 0C 00 01 00   00 00 00 00 01 B9 00 00  .............?.
//( 000002E0 )   50 00 00 00 03 0A 00 00   00 02 00 00 00 00 00 00  P...............
//( 000002F0 )   0D 00 01 00 00 00 00 00   01 A3 00 00 44 00 00 00  .........?.D...
//( 00000300 )   01 03 00 00 00 00 00 00   00 00 00 00 0E 00 01 00  ................
//( 00000310 )   00 00 00 00 01 8F 00 00   50 00 00 00 03 00 01 00  .....?.P.......
//( 00000320 )   00 04 00 00 00 00 00 00   0F 00 01 00 00 00 00 00  ................
//( 00000330 )   01 9F 00 9F 50 00 00 00   03 01 01 00 00 01 00 00  .?�P...........
//( 00000350 )   44 00 00 00 01 03 08 00   00 00 00 00 00 00 00 00  D...............
//( 00000360 )   11 00 01 00 00 00 00 00   01 5D 00 00 44 00 00 00  .........]..D...
//( 00000370 )   00 34 00 00 00 00 00 00   00 00 00 00 12 00 01 00  .4..............
//( 00000380 )   00 00 00 00 01 47 CF 59   44 00 00 00 00 01 03 00  .....G�YD.......
//( 00000390 )   00 00 00 00 00 00 00 00   13 00 01 00 00 00 00 00  ................
//( 000003A0 )   01 31 DF 43 44 00 00 00   00 0A 00 00 00 00 00 00  .1�CD...........
//( 000003C0 )   44 00 00 00 01 01 00 00   00 01 01 00 00 00 00 00  D...............
//( 000003D0 )   15 00 01 00 00 00 00 00   01 FF 17 FF 50 00 00 00  ..........P...
//( 000003E0 )   03 0B 01 00 00 01 00 00   00 00 00 00 16 00 01 00  ................
//( 000003F0 )   00 00 00 00 01 00 00 00   50 00 00 00 03 0B 06 00  ........P.......
//( 00000400 )   00 01 00 00 00 00 00 00   17 00 01 00 00 00 00 00  ................
//( 00000410 )   01 00 00 00 50 00 00 00   03 0B 05 00 00 01 00 00  ....P...........
//( 00000420 )   00 00 00 00 18 00 01 00   00 00 00 00 01 00 00 00  ................
//( 00000430 )   44 00 00 00 01 02 30 00   00 00 00 00 00 00 00 00  D.....0.........
//( 00000440 )   19 00 01 00 00 00 00 00   02 00 00 00 4C 00 00 00  ............L...
//( 00000450 )   01 02 30 00 00 00 00 00   00 00 00 00 1A 00 01 00  ..0.............
//( 00000460 )   00 00 00 00 01 00 00 00   44 00 00 00 01 02 88 00  ........D.....?
//( 00000470 )   00 00 03 00 00 00 00 00   1B 00 01 00 00 00 00 00  ................
//( 00000480 )   01 00 00 00 40 00 00 00   03 09 00 00 00 00 00 00  ....@...........
//( 00000490 )   00 00 00 00 1C 00 01 00   00 00 00 00 01 00 00 00  ................
//( 000004A0 )   50 00 00 00 03 07 00 00   00 01 00 00 00 00 00 00  P...............
//( 000004B0 )   1D 00 01 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 000004C0 )   00 00 00 00 00 00 00 00                           ........
    //display_packet_old(pkt, pkt->hdr.pkt_len);

    if ((src->version == CLIENT_VERSION_XBOX && dest->version == CLIENT_VERSION_GC) ||
        (dest->version == CLIENT_VERSION_XBOX && src->version == CLIENT_VERSION_GC)) {
        /* ɨ���沢�ڷ���֮ǰ�޸�����mag. */


        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* �����Ŀ��mag,��ô���Ǳ��뽻�����ݵ����һ��dword.����,��ɫ��ͳ�����ݻ���һ���� */
            encode_if_mag(&item->data, src->version);
        }

        return send_pkt_dc(dest, (dc_pkt_hdr_t*)&pkt);
    }

    //psocn_bb_char_t* character = get_client_char_bb(src);
    //psocn_bb_char_t* character2 = get_client_char_bb(dest);

    //DBG_LOG("%s ID %d | %s ID %d",get_player_describe(src), src->client_id, get_player_describe(dest), dest->client_id);

    ////�����ܵ�����Ҵ��͡�
    ////�����Ǿ����ܶ���ؽ�0x70��
    ////
    ////������������������Ϣ

    //subcmd_bb_burst_pldata_t pkt2 = { 0 };

    //memcpy(&pkt2, pkt, pkt->hdr.pkt_len);

    //pkt2.hdr.pkt_type = pkt->hdr.pkt_type;
    //pkt2.hdr.pkt_len = pkt->hdr.pkt_len;
    //pkt2.hdr.flags = pkt->hdr.flags;

    ///* ȷ����ҵ�GC���� */
    //pkt2.guildcard = src->guildcard;

    /* Fall through... */
    rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);

    /* �����ҵ�ħ���Ƿ�Ϲ� */
    fix_player_max_tech_level(&src->bb_pl->character);

    ///* ������Լ���ɫ�ĺϷ�������� */
    //memcpy(&pkt2.dress_data.gc_string[0], &character->dress_data.gc_string[0], sizeof(psocn_dress_data_t));

    ///* ���������NPCƤ�� ��ȡ�������ڵ���ʾ ��ʱ�� ��ֹ���� */
    //if (character->dress_data.v2flags)
    //    memset(&pkt2.dress_data.costume, 0, 10);

    ///* ������ҵ��������� */
    //for (i = 0; i < 0x0C; i++) {
    //    pkt2.name.all[i] = character->name.all[i];
    //}

    ///* ������Լ����ҵĻ������� */
    //pkt2.disp.stats.ata = character->disp.stats.ata;
    //pkt2.disp.stats.mst = character->disp.stats.mst;
    //pkt2.disp.stats.evp = character->disp.stats.evp;
    //pkt2.disp.stats.hp = character->disp.stats.hp;
    //pkt2.disp.stats.dfp = character->disp.stats.dfp;
    //pkt2.disp.stats.ata = character->disp.stats.ata;
    //pkt2.disp.stats.lck = character->disp.stats.lck;
    ///* δ֪�Ķ��� */
    //pkt2.disp.unknown_a1 = character->disp.unknown_a1;
    //pkt2.disp.unknown_a2 = character->disp.unknown_a2;
    //pkt2.disp.unknown_a3 = character->disp.unknown_a3;
    ///* �ȼ� ���� ������ */
    //pkt2.disp.level = character->disp.level;
    //pkt2.disp.exp = character->disp.exp;
    //pkt2.disp.meseta = character->disp.meseta;

    ///* ��ʼ��鱳����Ʒ */
    //pkt2.inv.item_count = character->inv.item_count;
    //pkt2.inv.hpmats_used = character->inv.hpmats_used;
    //pkt2.inv.tpmats_used = character->inv.tpmats_used;
    //pkt2.inv.language = character->inv.language;

    //for (i = 0; i < MAX_PLAYER_INV_ITEMS; i++) {
    //    pkt2.inv.iitems[i] = character->inv.iitems[i];
    //}

    return rv;
}

// ���庯��ָ������
subcmd_handle_func_t subcmd6D_handler[] = {
    //    cmd_type                         DC           GC           EP3          XBOX         PC           BB
    { SUBCMD6D_BURST1                    , sub6D_6D_dc, sub6D_6D_dc, sub6D_6D_dc, sub6D_6D_dc, sub6D_6D_dc, sub6D_6D_bb },
    { SUBCMD6D_BURST2                    , sub6D_6B_dc, sub6D_6B_dc, sub6D_6B_dc, sub6D_6B_dc, sub6D_6B_dc, sub6D_6B_bb },
    { SUBCMD6D_BURST3                    , sub6D_6C_dc, sub6D_6C_dc, sub6D_6C_dc, sub6D_6C_dc, sub6D_6C_dc, sub6D_6C_bb },
    { SUBCMD6D_BURST4                    , sub6D_6E_dc, sub6D_6E_dc, sub6D_6E_dc, sub6D_6E_dc, sub6D_6E_dc, sub6D_6E_bb },
    { SUBCMD6D_BURST_PLDATA              , sub6D_70_dc, sub6D_70_dc, sub6D_70_dc, sub6D_70_dc, sub6D_70_dc, sub6D_70_bb },
};

/* ���� DC GC PC V1 V2 0x6D ���Կͻ��˵����ݰ�. */
int subcmd_handle_6D(ship_client_t* src, subcmd_pkt_t* pkt) {
    __try {
        lobby_t* l = src->cur_lobby;
        ship_client_t* dest;
        uint16_t hdr_type = pkt->hdr.dc.pkt_type;
        uint8_t type = pkt->type;
        int rv = -1;

        /* ����ͻ��˲��ڴ������߶�������������ݰ�. */
        if (!l)
            return 0;

        pthread_mutex_lock(&l->mutex);

        /* ����Ŀ��ͻ���. */
        dest = l->clients[pkt->hdr.dc.flags];

        /* Ŀ��ͻ��������ߣ������ٷ������ݰ�. */
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
        } else if (l->subcmd_handle == NULL) {

#ifdef BB_LOG_UNKNOWN_SUBS
            DBG_LOG("δ֪ 0x%02X ָ��: 0x%02X", hdr_type, type);
            print_ascii_hex(dbgl, pkt, LE16(pkt->hdr.dc.pkt_len));
#endif /* BB_LOG_UNKNOWN_SUBS */

            rv = send_pkt_dc(dest, (dc_pkt_hdr_t*)pkt);
        }
        else {
            rv = l->subcmd_handle(src, dest, pkt);
        }


        pthread_mutex_unlock(&l->mutex);
        return rv;

    }

    __except (crash_handler(GetExceptionInformation())) {
        // ������ִ���쳣�������߼��������ӡ������Ϣ���ṩ�û��Ѻõ���ʾ��

        CRASH_LOG("���ִ���, �����˳�.");
        (void)getchar();
        return -4;
    }
}

/* ����BB 0x6D ���ݰ�. */
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

        /* ����ͻ��˲��ڴ������߶�������������ݰ�. */
        if (!l)
            return 0;

        pthread_mutex_lock(&l->mutex);

        /* ����Ŀ��ͻ���. */
        dest = l->clients[dnum];

        /* Ŀ��ͻ��������ߣ������ٷ������ݰ�. */
        if (!dest) {
            pthread_mutex_unlock(&l->mutex);
            return 0;
        }
#ifdef DEBUG

        DBG_LOG("GC %u Ŀ��ͻ���-> %d GC %u 0x%02X ָ��: 0x%02X", src->guildcard, dnum, l->clients[dnum]->guildcard, hdr_type, type);

#endif // DEBUG

        //subcmd_bb_626Dsize_check(c, pkt);

        l->subcmd_handle = subcmd_get_handler(hdr_type, type, src->version);
        if (!l->subcmd_handle) {

#ifdef BB_LOG_UNKNOWN_SUBS
            DBG_LOG("δ֪ 0x%02X ָ��: 0x%02X", hdr_type, type);
            print_ascii_hex(dbgl, pkt, len);
#endif /* BB_LOG_UNKNOWN_SUBS */

            rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            pthread_mutex_unlock(&l->mutex);
            return rv;
        }

        /* If there's a burst going on in the lobby, delay most packets */
        if (l->flags & LOBBY_FLAG_BURSTING) {
            rv = 0;

            switch (type) {
            case SUBCMD6D_BURST1://0x6D 6D //��������ԾǨ����ʱ���� 1
            case SUBCMD6D_BURST2://0x6D 6B //��������ԾǨ����ʱ���� 2
            case SUBCMD6D_BURST3://0x6D 6C //��������ԾǨ����ʱ���� 3
            case SUBCMD6D_BURST4://0x6D 6E //��������ԾǨ����ʱ���� 4
            case SUBCMD6D_BURST_PLDATA://0x6D 70 //��������ԾǨ����ʱ���� 7
                rv = l->subcmd_handle(src, dest, pkt);
                break;

            default:
                rv = lobby_enqueue_pkt_bb(l, src, (bb_pkt_hdr_t*)pkt);
            }
        } else {
            rv = l->subcmd_handle(src, dest, pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // ������ִ���쳣�������߼��������ӡ������Ϣ���ṩ�û��Ѻõ���ʾ��

        CRASH_LOG("���ִ���, �����˳�.");
        (void)getchar();
        return -4;
    }
}
