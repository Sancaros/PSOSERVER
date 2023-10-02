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
#include <string.h>

#include <f_logs.h>

#include "clients.h"
#include "lobby.h"
#include "subcmd.h"
#include "utils.h"
#include "ship_packets.h"
#include "word_select.h"
#include "word_select-dc.h"
#include "word_select-pc.h"
#include "word_select-gc.h"
#include "word_select-bb.h"

int word_select_send_dc(ship_client_t *c, subcmd_word_select_t *pkt) {
    lobby_t *l = c->cur_lobby;
    int i;
    subcmd_word_select_t pc = { 0 }, gc = { 0 }, xb = { 0 };
    subcmd_bb_word_select_t bb = { 0 };
    int pcusers = 0, gcusers = 0;
    int pcuntrans = 0, gcuntrans = 0;
    uint16_t dcw, pcw, gcw;

    /* Fill in the translated packets */
    pc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    pc.hdr.flags = pkt->hdr.flags;
    pc.hdr.pkt_len = LE16(0x0024);
    pc.type = SUBCMD60_WORD_SELECT;
    pc.size = 0x08;
    pc.client_id = pkt->client_id;
    pc.client_id_gc = 0;
    pc.data.num_words = pkt->data.num_words;
    pc.data.ws_type = pkt->data.ws_type;

    gc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    gc.hdr.flags = pkt->hdr.flags;
    gc.hdr.pkt_len = LE16(0x0024);
    gc.type = SUBCMD60_WORD_SELECT;
    gc.size = 0x08;
    gc.client_id = 0;
    gc.client_id_gc = pkt->client_id;
    gc.data.num_words = pkt->data.num_words;
    gc.data.ws_type = pkt->data.ws_type;

    xb.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    xb.hdr.flags = pkt->hdr.flags;
    xb.hdr.pkt_len = LE16(0x0024);
    xb.type = SUBCMD60_WORD_SELECT;
    xb.size = 0x08;
    xb.client_id = pkt->client_id;
    xb.client_id_gc = 0;
    xb.data.num_words = pkt->data.num_words;
    xb.data.ws_type = pkt->data.ws_type;

    bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    bb.hdr.flags = LE32(pkt->hdr.flags);
    bb.hdr.pkt_len = LE16(0x0028);
    bb.shdr.type = SUBCMD60_WORD_SELECT;
    bb.shdr.size = 0x08;
    bb.shdr.client_id = pkt->client_id;
    bb.data.num_words = pkt->data.num_words;
    bb.data.ws_type = pkt->data.ws_type;

    /* No versions other than PSODC sport the lovely LIST ALL menu. Oh well, I
       guess I can't go around saying "HELL HELL HELL" to everyone. */
    if(pkt->data.ws_type == 6) {
        pcuntrans = 1;
        gcuntrans = 1;
    }

    for(i = 0; i < 8; ++i) {
        dcw = LE16(pkt->data.words[i]);

        /* Make sure each word is valid */
        if(dcw > WORD_SELECT_DC_MAX && dcw != 0xFFFF) {
            return send_txt(c, __(c, "\tE\tC7Invalid word select."));
        }

        /* Grab the words from the map */
        if(dcw != 0xFFFF) {
            pcw = word_select_dc_map[dcw][0];
            gcw = word_select_dc_map[dcw][1];
        }
        else {
            pcw = gcw = 0xFFFF;
        }

        /* See if we have an untranslateable word */
        if(pcw == 0xFFFF && dcw != 0xFFFF) {
            pcuntrans = 1;
        }

        if(gcw == 0xFFFF && dcw != 0xFFFF) {
            gcuntrans = 1;
        }

        /* Throw them into the packets */
        pc.data.words[i] = LE16(pcw);
        gc.data.words[i] = xb.data.words[i] = LE16(gcw);
        bb.data.words[i] = LE16(gcw);
    }

    /* Deal with amounts and such... */
    pc.data.words[8] = pkt->data.words[8];
    pc.data.words[9] = pkt->data.words[9];
    pc.data.words[10] = pkt->data.words[10];
    pc.data.words[11] = pkt->data.words[11];
    gc.data.words[8] = pkt->data.words[8];
    gc.data.words[9] = pkt->data.words[9];
    gc.data.words[10] = pkt->data.words[10];
    gc.data.words[11] = pkt->data.words[11];
    xb.data.words[8] = pkt->data.words[8];
    xb.data.words[9] = pkt->data.words[9];
    xb.data.words[10] = pkt->data.words[10];
    xb.data.words[11] = pkt->data.words[11];
    bb.data.words[8] = pkt->data.words[8];
    bb.data.words[9] = pkt->data.words[9];
    bb.data.words[10] = pkt->data.words[10];
    bb.data.words[11] = pkt->data.words[11];

    /* Send the packet to everyone we can */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] && l->clients[i] != c &&
           !client_has_ignored(l->clients[i], c->guildcard)) {
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)pkt);
                    break;

                case CLIENT_VERSION_PC:
                    if(!pcuntrans) {
                        send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&pc);
                    }

                    pcusers = 1;
                    break;

                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                    if(!gcuntrans) {
                        send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&gc);
                    }

                    gcusers = 1;
                    break;

                case CLIENT_VERSION_XBOX:
                    if (!gcuntrans) {
                        send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&xb);
                    }

                    gcusers = 1;
                    break;

                case CLIENT_VERSION_BB:
                    if(!gcuntrans) {
                        send_pkt_bb(l->clients[i], (bb_pkt_hdr_t *)&bb);
                    }

                    gcusers = 1;
                    break;
            }
        }
    }

    /* See if we had anyone that we couldn't send it to */
    if((pcusers && pcuntrans) || (gcusers && gcuntrans)) {
        send_txt(c, __(c, "\tE\tC7Some clients did not\n"
                          "receive your last word\nselect."));
    }

    return 0;
}

int word_select_send_pc(ship_client_t *c, subcmd_word_select_t *pkt) {
    lobby_t *l = c->cur_lobby;
    int i;
    subcmd_word_select_t dc = { 0 }, gc = { 0 }, xb = { 0 };
    subcmd_bb_word_select_t bb = { 0 };
    int dcusers = 0, gcusers = 0;
    int dcuntrans = 0, gcuntrans = 0;
    uint16_t dcw, pcw, gcw;

    /* Fill in the translated packets */
    dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    dc.hdr.flags = pkt->hdr.flags;
    dc.hdr.pkt_len = LE16(0x0024);
    dc.type = SUBCMD60_WORD_SELECT;
    dc.size = 0x08;
    dc.client_id = pkt->client_id;
    dc.client_id_gc = 0;
    dc.data.num_words = pkt->data.num_words;
    dc.data.ws_type = pkt->data.ws_type;

    gc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    gc.hdr.flags = pkt->hdr.flags;
    gc.hdr.pkt_len = LE16(0x0024);
    gc.type = SUBCMD60_WORD_SELECT;
    gc.size = 0x08;
    gc.client_id = 0;
    gc.client_id_gc = pkt->client_id;
    gc.data.num_words = pkt->data.num_words;
    gc.data.ws_type = pkt->data.ws_type;

    xb.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    xb.hdr.flags = pkt->hdr.flags;
    xb.hdr.pkt_len = LE16(0x0024);
    xb.type = SUBCMD60_WORD_SELECT;
    xb.size = 0x08;
    xb.client_id = pkt->client_id;
    xb.client_id_gc = 0;
    xb.data.num_words = pkt->data.num_words;
    xb.data.ws_type = pkt->data.ws_type;

    bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    bb.hdr.flags = LE32(pkt->hdr.flags);
    bb.hdr.pkt_len = LE16(0x0028);
    bb.shdr.type = SUBCMD60_WORD_SELECT;
    bb.shdr.size = 0x08;
    bb.shdr.client_id = pkt->client_id;
    bb.data.num_words = pkt->data.num_words;
    bb.data.ws_type = pkt->data.ws_type;

    for(i = 0; i < 8; ++i) {
        pcw = LE16(pkt->data.words[i]);

        /* Make sure each word is valid */
        if(pcw > WORD_SELECT_PC_MAX && pcw != 0xFFFF) {
            return send_txt(c, __(c, "\tE\tC7Invalid word select."));
        }

        /* Grab the words from the map */
        if(pcw != 0xFFFF && pcw <= WORD_SELECT_PC_MAX) {
            dcw = word_select_pc_map[pcw][0];
            gcw = word_select_pc_map[pcw][1];
        }
        else {
            dcw = gcw = 0xFFFF;
        }

        /* See if we have an untranslateable word */
        if(dcw == 0xFFFF && pcw != 0xFFFF) {
            dcuntrans = 1;
        }

        if(gcw == 0xFFFF && pcw != 0xFFFF) {
            gcuntrans = 1;
        }

        /* Throw them into the packets */
        dc.data.words[i] = LE16(dcw);
        gc.data.words[i] = xb.data.words[i] = LE16(gcw);
        bb.data.words[i] = LE16(gcw);
    }

    /* Deal with amounts and such... */
    dc.data.words[8] = pkt->data.words[8];
    dc.data.words[9] = pkt->data.words[9];
    dc.data.words[10] = pkt->data.words[10];
    dc.data.words[11] = pkt->data.words[11];
    gc.data.words[8] = pkt->data.words[8];
    gc.data.words[9] = pkt->data.words[9];
    gc.data.words[10] = pkt->data.words[10];
    gc.data.words[11] = pkt->data.words[11];
    xb.data.words[8] = pkt->data.words[8];
    xb.data.words[9] = pkt->data.words[9];
    xb.data.words[10] = pkt->data.words[10];
    xb.data.words[11] = pkt->data.words[11];
    bb.data.words[8] = pkt->data.words[8];
    bb.data.words[9] = pkt->data.words[9];
    bb.data.words[10] = pkt->data.words[10];
    bb.data.words[11] = pkt->data.words[11];

    /* Send the packet to everyone we can */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] && l->clients[i] != c &&
           !client_has_ignored(l->clients[i], c->guildcard)) {
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                    if(!dcuntrans) {
                        send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&dc);
                    }

                    dcusers = 1;
                    break;

                case CLIENT_VERSION_PC:
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)pkt);
                    break;

                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                    if(!gcuntrans) {
                        send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&gc);
                    }

                    gcusers = 1;
                    break;

                case CLIENT_VERSION_XBOX:
                    if (!gcuntrans) {
                        send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&xb);
                    }

                    gcusers = 1;
                    break;

                case CLIENT_VERSION_BB:
                    if(!gcuntrans) {
                        send_pkt_bb(l->clients[i], (bb_pkt_hdr_t *)&bb);
                    }

                    gcusers = 1;
                    break;
            }
        }
    }

    /* See if we had anyone that we couldn't send it to */
    if((dcusers && dcuntrans) || (gcusers && gcuntrans)) {
        send_txt(c, __(c, "\tE\tC7Some clients did not\n"
                       "receive your last word\nselect."));
    }

    return 0;
}

int word_select_send_gc(ship_client_t *c, subcmd_word_select_t *pkt) {
    lobby_t *l = c->cur_lobby;
    int i;
    subcmd_word_select_t pc = { 0 }, dc = { 0 }, xb = { 0 }, gc = { 0 };
    subcmd_bb_word_select_t bb = { 0 };
    int pcusers = 0, dcusers = 0;
    int pcuntrans = 0, dcuntrans = 0;
    uint16_t dcw, pcw, gcw;
    uint8_t client_id;

    if (c->version == CLIENT_VERSION_XBOX) {
        client_id = pkt->client_id;
        memcpy(&xb, pkt, sizeof(subcmd_word_select_t));
        memcpy(&gc, pkt, sizeof(subcmd_word_select_t));
        gc.client_id = 0;
        gc.client_id_gc = client_id;
    }
    else {
        client_id = pkt->client_id_gc;
        memcpy(&xb, pkt, sizeof(subcmd_word_select_t));
        memcpy(&gc, pkt, sizeof(subcmd_word_select_t));
        xb.client_id = client_id;
        xb.client_id_gc = 0;
    }

    /* Fill in the translated packets */
    pc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    pc.hdr.flags = pkt->hdr.flags;
    pc.hdr.pkt_len = LE16(0x0024);
    pc.type = SUBCMD60_WORD_SELECT;
    pc.size = 0x08;
    pc.client_id = client_id;
    pc.client_id_gc = 0;
    pc.data.num_words = pkt->data.num_words;
    pc.data.ws_type = pkt->data.ws_type;

    dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    dc.hdr.flags = pkt->hdr.flags;
    dc.hdr.pkt_len = LE16(0x0024);
    dc.type = SUBCMD60_WORD_SELECT;
    dc.size = 0x08;
    dc.client_id = client_id;
    dc.client_id_gc = 0;
    dc.data.num_words = pkt->data.num_words;
    dc.data.ws_type = pkt->data.ws_type;

    bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    bb.hdr.flags = LE32(pkt->hdr.flags);
    bb.hdr.pkt_len = LE16(0x0028);
    bb.shdr.type = SUBCMD60_WORD_SELECT;
    bb.shdr.size = 0x08;
    bb.shdr.client_id = client_id;
    bb.data.num_words = pkt->data.num_words;
    bb.data.ws_type = pkt->data.ws_type;

    for(i = 0; i < 8; ++i) {
        gcw = LE16(pkt->data.words[i]);

        /* Make sure each word is valid */
        if(gcw > WORD_SELECT_GC_MAX && gcw != 0xFFFF) {
            return send_txt(c, __(c, "\tE\tC7Invalid word select."));
        }

        /* Grab the words from the map */
        if(gcw != 0xFFFF) {
            dcw = word_select_gc_map[gcw][0];
            pcw = word_select_gc_map[gcw][1];
        }
        else {
            pcw = dcw = 0xFFFF;
        }

        /* See if we have an untranslateable word */
        if(pcw == 0xFFFF && gcw != 0xFFFF) {
            pcuntrans = 1;
        }

        if(dcw == 0xFFFF && gcw != 0xFFFF) {
            dcuntrans = 1;
        }

        /* Throw them into the packets */
        pc.data.words[i] = LE16(pcw);
        dc.data.words[i] = LE16(dcw);
        bb.data.words[i] = LE16(gcw);
    }

    /* Deal with amounts and such... */
    dc.data.words[8] = pkt->data.words[8];
    dc.data.words[9] = pkt->data.words[9];
    dc.data.words[10] = pkt->data.words[10];
    dc.data.words[11] = pkt->data.words[11];
    pc.data.words[8] = pkt->data.words[8];
    pc.data.words[9] = pkt->data.words[9];
    pc.data.words[10] = pkt->data.words[10];
    pc.data.words[11] = pkt->data.words[11];
    bb.data.words[8] = pkt->data.words[8];
    bb.data.words[9] = pkt->data.words[9];
    bb.data.words[10] = pkt->data.words[10];
    bb.data.words[11] = pkt->data.words[11];

    /* Send the packet to everyone we can */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i] && l->clients[i] != c &&
           !client_has_ignored(l->clients[i], c->guildcard)) {
            switch(l->clients[i]->version) {
                case CLIENT_VERSION_DCV1:
                case CLIENT_VERSION_DCV2:
                    if(!dcuntrans) {
                        send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&dc);
                    }

                    dcusers = 1;
                    break;

                case CLIENT_VERSION_PC:
                    if(!pcuntrans) {
                        send_pkt_dc(l->clients[i], (dc_pkt_hdr_t *)&pc);
                    }

                    pcusers = 1;
                    break;

                case CLIENT_VERSION_GC:
                case CLIENT_VERSION_EP3:
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&gc);
                    break;

                case CLIENT_VERSION_XBOX:
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&xb);
                    break;

                case CLIENT_VERSION_BB:
                    send_pkt_bb(l->clients[i], (bb_pkt_hdr_t *)&bb);
                    break;
            }
        }
    }

    /* See if we had anyone that we couldn't send it to */
    if((pcusers && pcuntrans) || (dcusers && dcuntrans)) {
        send_txt(c, __(c, "\tE\tC7Some clients did not\n"
                       "receive your last word\nselect."));
    }

    return 0;
}

int word_select_send_bb(ship_client_t* c, subcmd_bb_word_select_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int i;
    subcmd_word_select_t pc = { 0 }, dc = { 0 }, xb = { 0 }, gc = { 0 };
    subcmd_bb_word_select_t bb = { 0 };
    int pcusers = 0, dcusers = 0, gcusers = 0;
    int pcuntrans = 0, dcuntrans = 0, gcuntrans = 0;
    uint16_t dcw, pcw, gcw;
    uint8_t client_id;

    if (c->version == CLIENT_VERSION_XBOX) {
        client_id = (uint8_t)pkt->shdr.client_id;
        memcpy(&xb.data, &pkt->data, sizeof(word_select_t));
        memcpy(&gc.data, &pkt->data, sizeof(word_select_t));
        gc.client_id = 0;
        gc.client_id_gc = client_id;
    }
    else {
        client_id = (uint8_t)pkt->shdr.client_id;
        memcpy(&xb.data, &pkt->data, sizeof(word_select_t));
        memcpy(&gc.data, &pkt->data, sizeof(word_select_t));
        xb.client_id = client_id;
        xb.client_id_gc = 0;
    }

    /* Fill in the translated packets */
    pc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    pc.hdr.flags = pkt->hdr.flags;
    pc.hdr.pkt_len = LE16(0x0024);
    pc.type = SUBCMD60_WORD_SELECT;
    pc.size = 0x08;
    pc.client_id = client_id;
    pc.client_id_gc = 0;
    pc.data.num_words = pkt->data.num_words;
    pc.data.ws_type = pkt->data.ws_type;

    dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    dc.hdr.flags = pkt->hdr.flags;
    dc.hdr.pkt_len = LE16(0x0024);
    dc.type = SUBCMD60_WORD_SELECT;
    dc.size = 0x08;
    dc.client_id = client_id;
    dc.client_id_gc = 0;
    dc.data.num_words = pkt->data.num_words;
    dc.data.ws_type = pkt->data.ws_type;

    gc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    gc.hdr.flags = pkt->hdr.flags;
    gc.hdr.pkt_len = LE16(0x0024);
    gc.type = SUBCMD60_WORD_SELECT;
    gc.size = 0x08;
    gc.client_id = 0;
    gc.client_id_gc = client_id;
    gc.data.num_words = pkt->data.num_words;
    gc.data.ws_type = pkt->data.ws_type;

    xb.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    xb.hdr.flags = pkt->hdr.flags;
    xb.hdr.pkt_len = LE16(0x0024);
    xb.type = SUBCMD60_WORD_SELECT;
    xb.size = 0x08;
    xb.client_id = client_id;
    xb.client_id_gc = 0;
    xb.data.num_words = pkt->data.num_words;
    xb.data.ws_type = pkt->data.ws_type;

    bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    bb.hdr.flags = LE32(pkt->hdr.flags);
    bb.hdr.pkt_len = LE16(0x0028);
    bb.shdr.type = SUBCMD60_WORD_SELECT;
    bb.shdr.size = 0x08;
    bb.shdr.client_id = client_id;
    bb.data.num_words = pkt->data.num_words;
    bb.data.ws_type = pkt->data.ws_type;


    /* No versions other than PSODC sport the lovely LIST ALL menu. Oh well, I
       guess I can't go around saying "HELL HELL HELL" to everyone. */
    if (pkt->data.ws_type == 6) {
        pcuntrans = 1;
        gcuntrans = 1;
    }

    for (i = 0; i < 8; ++i) {
        gcw = LE16(pkt->data.words[i]);

        /* Make sure each word is valid */
        if (gcw > WORD_SELECT_BB_MAX && gcw != 0xFFFF) {
            return send_txt(c, __(c, "\tE\tC7无效快捷语."));
        }

        /* Grab the words from the map */
        if (gcw != 0xFFFF) {
            dcw = word_select_gc_map[gcw][0];
            pcw = word_select_gc_map[gcw][1];
            gcw = word_select_dc_map[dcw][1];
        }
        else {
            pcw = dcw = 0xFFFF;
        }

        /* See if we have an untranslateable word */
        if (pcw == 0xFFFF && gcw != 0xFFFF) {
            pcuntrans = 1;
        }

        if (dcw == 0xFFFF && gcw != 0xFFFF) {
            dcuntrans = 1;
        }

        if (gcw == 0xFFFF && dcw != 0xFFFF) {
            gcuntrans = 1;
        }

        /* Throw them into the packets */
        pc.data.words[i] = LE16(pcw);
        dc.data.words[i] = LE16(dcw);
        gc.data.words[i] = xb.data.words[i] = LE16(gcw);
        bb.data.words[i] = LE16(gcw);
    }

    /* Deal with amounts and such... */
    dc.data.words[8] = pkt->data.words[8];
    dc.data.words[9] = pkt->data.words[9];
    dc.data.words[10] = pkt->data.words[10];
    dc.data.words[11] = pkt->data.words[11];
    pc.data.words[8] = pkt->data.words[8];
    pc.data.words[9] = pkt->data.words[9];
    pc.data.words[10] = pkt->data.words[10];
    pc.data.words[11] = pkt->data.words[11];
    gc.data.words[8] = pkt->data.words[8];
    gc.data.words[9] = pkt->data.words[9];
    gc.data.words[10] = pkt->data.words[10];
    gc.data.words[11] = pkt->data.words[11];
    xb.data.words[8] = pkt->data.words[8];
    xb.data.words[9] = pkt->data.words[9];
    xb.data.words[10] = pkt->data.words[10];
    xb.data.words[11] = pkt->data.words[11];
    bb.data.words[8] = pkt->data.words[8];
    bb.data.words[9] = pkt->data.words[9];
    bb.data.words[10] = pkt->data.words[10];
    bb.data.words[11] = pkt->data.words[11];

    /* Send the packet to everyone we can */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c &&
            !client_has_ignored(l->clients[i], c->guildcard)) {
            switch (l->clients[i]->version) {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
                if (!dcuntrans) {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&dc);
                }

                dcusers = 1;
                break;

            case CLIENT_VERSION_PC:
                if (!pcuntrans) {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&pc);
                }

                pcusers = 1;
                break;

            case CLIENT_VERSION_GC:
            case CLIENT_VERSION_EP3:
                if (!gcuntrans) {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&gc);
                }

                gcusers = 1;
                break;

            case CLIENT_VERSION_XBOX:
                if (!gcuntrans) {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&xb);
                }

                gcusers = 1;
                break;

            case CLIENT_VERSION_BB:
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&bb);
                break;
            }
        }
    }

    /* See if we had anyone that we couldn't send it to */
    if ((pcusers && pcuntrans) || (dcusers && dcuntrans) || (gcusers && gcuntrans)) {
        send_txt(c, __(c, "\tE\tC7Some clients did not\n"
            "receive your last word\nselect."));
    }

    return 0;
}