/*
    �λ�֮���й� ����������
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <iconv.h>
#include <time.h>
#include <ctype.h>

#include <Software_Defines.h>
#include <WinSock_Defines.h>
#include <psomemory.h>
#include <f_logs.h>
#include <f_iconv.h>
#include <pso_menu.h>

#include "ship_packets.h"
#include "lobby.h"
#include "subcmd.h"
#include "utils.h"
#include "shipgate.h"
#include "handle_player_items.h"
#include "bans.h"
#include "admin.h"
#include "ptdata.h"
#include "pmtdata.h"
#include "mapdata.h"
#include "rtdata.h"
#include "scripts.h"
#include "subcmd_send.h"

extern int sub62_06_dc(ship_client_t* s, ship_client_t* d,
    subcmd_dc_gcsend_t* pkt);

/* �÷�: /gm */
static int handle_gm(ship_client_t* c, const char* params) {
    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    if (in_lobby(c))
        return send_gm_menu(c, MENU_ID_GM);

    return send_txt(c, "%s", __(c, "\tE\tC4�޷��ڷ���ʹ�ø�ָ��."));
}

/* �÷�: /debug [0/1]*/
static int handle_gmdebug(ship_client_t* c, const char* params) {
    unsigned long param;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the floor requested */
    errno = 0;
    param = strtoul(params, NULL, 10);

    if (errno) {
        /* Send a message saying invalid area */
        return send_txt(c, "%s", __(c, "\tE\tC7δѡ������ر� 1��0."));
    }

    if (param == 1) {
        c->game_data->gm_debug = 1;
        return send_txt(c, "%s", __(c, "\tE\tC6DEBUGģʽ����."));
    }
    else {
        c->game_data->gm_debug = 0;
        return send_txt(c, "%s", __(c, "\tE\tC4DEBUGģʽ�ر�."));
    }
}

/* �÷�: /swarp area */
static int handle_shipwarp(ship_client_t* c, const char* params) {
    unsigned long area;
    lobby_t* l = c->cur_lobby;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Figure out the floor requested */
    errno = 0;
    area = strtoul(params, NULL, 10);

    if (errno) {
        /* Send a message saying invalid area */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����."));
    }

    if (area > 17) {
        /* Area too large, give up */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����."));
    }

    /* Send the person to the requested place */
    return send_warp(c, (uint8_t)area, false);
}

/* �÷�: /warp area */
static int handle_warp(ship_client_t* c, const char* params) {
    unsigned long area;
    lobby_t* l = c->cur_lobby;

    ///* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Figure out the floor requested */
    errno = 0;
    area = strtoul(params, NULL, 10);

    if (errno) {
        /* Send a message saying invalid area */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����."));
    }

    if (area > 17) {
        /* Area too large, give up */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����."));
    }

    /* Send the person to the requested place */
    return send_warp(c, (uint8_t)area, true);
}

/* �÷�: /warpall area */
static int handle_warpall(ship_client_t* c, const char* params) {
    unsigned long area;
    lobby_t* l = c->cur_lobby;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Figure out the floor requested */
    errno = 0;
    area = strtoul(params, NULL, 10);

    if (errno) {
        /* Send a message saying invalid area */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����."));
    }

    if (area > 17) {
        /* Area too large, give up */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����."));
    }

    /* Send the person to the requested place */
    return send_lobby_warp(l, (uint8_t)area);
}

/* �÷�: /kill guildcard reason */
static int handle_kill(ship_client_t* c, const char* params) {
    uint32_t gc;
    char* reason;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC."));
    }

    /* Hand it off to the admin functionality to do the dirty work... */
    if (strlen(reason) > 1) {
        return kill_guildcard(c, gc, reason + 1);
    }
    else {
        return kill_guildcard(c, gc, NULL);
    }
}

/* �÷�: /refresh [quests, gms, or limits] */
static int handle_refresh(ship_client_t* c, const char* params) {
    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Not valid for Blue Burst clients */
    if (c->version == CLIENT_VERSION_BB) {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7Blue Burst ��֧�ָ�ָ��."));
    }

    if (!strcmp(params, "quests")) {
        return refresh_quests(c, send_msg);
    }
    else if (!strcmp(params, "gms")) {
        /* Make sure the requester is a local root. */
        if (!LOCAL_ROOT(c)) {
            return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7Ȩ�޲���."));
        }

        return refresh_gms(c, send_msg);
    }
    else if (!strcmp(params, "limits")) {
        return refresh_limits(c, send_msg);
    }
    else {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7Unknown item to refresh."));
    }
}

/* �÷� /bcast message */
static int handle_bcast(ship_client_t* c, const char* params) {
    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    return broadcast_message(c, params, 1);
}

/* �÷� /tmsg typecode,message */
static int handle_tmsg(ship_client_t* c, const char* params) {
    uint16_t typecode = 0;
    char message[4096] = { 0 };
    int count;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Copy over the item data. */
    count = sscanf(params, "%hu,%s", &typecode, &message[0]);

    if (count == EOF || count == 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч��������."));
    }

    return global_message(c, typecode, 1, message);
}

/* �÷�: /event number */
static int handle_event(ship_client_t* c, const char* params) {
    int event;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Grab the event number */
    event = atoi(params);

    if (event < 0 || event > 14) {
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�����¼�."));
    }

    ship->lobby_event = event;
    update_lobby_event();

    return send_txt(c, "%s", __(c, "\tE\tC7�����¼����óɹ�."));
}

/* �÷� /clinfo client_id */
static int handle_clinfo(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int id, count;
    ship_client_t* cl;
    char ip[INET6_ADDRSTRLEN];

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Copy over the item data. */
    count = sscanf(params, "%d", &id);

    if (count == EOF || count == 0 || id >= l->max_clients || id < 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч Client ID."));
    }

    /* Make sure there is such a client. */
    if (!(cl = l->clients[id])) {
        return send_txt(c, "%s", __(c, "\tE\tC7No such client."));
    }

    /* Fill in the client's info. */
    my_ntop(&cl->ip_addr, ip);
    return send_txt(c, "\tE\tC7����: %s\nIP: %s\nGC: %u\n%s Lv.%d",
        cl->pl->v1.character.dress_data.gc_string, ip, cl->guildcard,
        pso_class[cl->pl->v1.character.dress_data.ch_class].cn_name, cl->pl->v1.character.disp.level + 1);
}

/* �÷�: /list parameters (there's too much to put here) */
static int handle_list(ship_client_t* c, const char* params) {
    /* Make sure the requester is a local GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Pass off to the player list code... */
    return send_player_list(c, params);
}

/* �÷�: /forgegc guildcard name */
static int handle_forgegc(ship_client_t* c, const char* params) {
    uint32_t gc;
    char* name = NULL;
    subcmd_dc_gcsend_t gcpkt = { 0 };

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &name, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Make sure a name was given */
    if (!name || name[0] != ' ' || name[1] == '\0') {
        return send_txt(c, "%s", __(c, "\tE\tC7δ��ȡ���������."));
    }

    /* Forge the guildcard send */
    gcpkt.hdr.pkt_type = GAME_SUBCMD62_TYPE;
    gcpkt.hdr.flags = c->client_id;
    gcpkt.hdr.pkt_len = LE16(0x0088);
    gcpkt.shdr.type = SUBCMD62_GUILDCARD;
    gcpkt.shdr.size = 0x21;
    gcpkt.shdr.unused = 0x0000;
    gcpkt.player_tag = LE32(0x00010000);
    gcpkt.guildcard = LE32(gc);
    strncpy(gcpkt.name, name + 1, 16);
    gcpkt.name[15] = 0;
    memset(gcpkt.guildcard_desc, 0, 88);
    gcpkt.unused2 = 0;
    gcpkt.disable_udp = 1;
    gcpkt.language = CLIENT_LANG_ENGLISH;
    gcpkt.section = 0;
    gcpkt.ch_class = 8;
    gcpkt.padding[0] = gcpkt.padding[1] = gcpkt.padding[2] = 0;

    /* Send the packet */
    return sub62_06_dc(NULL, c, &gcpkt);
}

/* �÷�: /invuln [off] */
static int handle_invuln(ship_client_t* c, const char* params) {
    pthread_mutex_lock(&c->mutex);

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        pthread_mutex_unlock(&c->mutex);
        return get_gm_priv(c);
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_INVULNERABLE;
        pthread_mutex_unlock(&c->mutex);

        return send_txt(c, "%s", __(c, "\tE\tC7�޵� �ر�."));
    }

    /* Set the flag since we're turning it on. */
    c->flags |= CLIENT_FLAG_INVULNERABLE;

    pthread_mutex_unlock(&c->mutex);
    return send_txt(c, "%s", __(c, "\tE\tC7�޵� ����."));
}

/* �÷�: /inftp [off] */
static int handle_inftp(ship_client_t* c, const char* params) {
    pthread_mutex_lock(&c->mutex);

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        pthread_mutex_unlock(&c->mutex);
        return get_gm_priv(c);
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_INFINITE_TP;
        pthread_mutex_unlock(&c->mutex);

        return send_txt(c, "%s", __(c, "\tE\tC7���� TP �ر�."));
    }

    /* Set the flag since we're turning it on. */
    c->flags |= CLIENT_FLAG_INFINITE_TP;

    pthread_mutex_unlock(&c->mutex);
    return send_txt(c, "%s", __(c, "\tE\tC7���� TP ����."));
}

/* �÷�: /smite clientid hp tp */
static int handle_smite(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int count, id, hp, tp;
    ship_client_t* cl;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%d %d %d", &id, &hp, &tp);

    if (count == EOF || count < 3 || id >= l->max_clients || id < 0 || hp < 0 ||
        tp < 0 || hp > 2040 || tp > 2040) {
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����.\n/smite clientid hp tp"));
    }

    pthread_mutex_lock(&l->mutex);

    /* Make sure there is such a client. */
    if (!(cl = l->clients[id])) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7δ�ҵ��ÿͻ���ID."));
    }

    /* Smite the client */
    count = 0;

    if (hp) {
        send_lobby_mod_stat(l, cl, SUBCMD60_STAT_HPDOWN, hp);
        ++count;
    }

    if (tp) {
        send_lobby_mod_stat(l, cl, SUBCMD60_STAT_TPDOWN, tp);
        ++count;
    }

    /* Finish up */
    pthread_mutex_unlock(&l->mutex);

    if (count) {
        send_txt(cl, "%s", __(c, "\tE\tC7You have been smitten."));
        return send_txt(c, "%s", __(c, "\tE\tC7Client smitten."));
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7Nothing to do."));
    }
}

/* �÷�: /tp client */
static int handle_teleport(ship_client_t* c, const char* params) {
    int client;
    lobby_t* l = c->cur_lobby;
    ship_client_t* c2;
    subcmd_teleport_t p2 = { 0 };

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Figure out the user requested */
    errno = 0;
    client = strtoul(params, NULL, 10);

    if (errno) {
        /* Send a message saying invalid client ID */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ͻ��� ID."));
    }

    if (client > l->max_clients) {
        /* Client ID too large, give up */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ͻ��� ID."));
    }

    if (!(c2 = l->clients[client])) {
        /* Client doesn't exist */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ͻ��� ID."));
    }

    /* See if we need to warp first */
    if (c2->cur_area != c->cur_area) {
        /* Send the person to the other user's area */
        return send_warp(c, (uint8_t)c2->cur_area, true);
    }
    else {
        /* Now, set up the teleport packet */
        p2.hdr.pkt_type = GAME_SUBCMD60_TYPE;
        p2.hdr.pkt_len = sizeof(subcmd_teleport_t);
        p2.hdr.flags = 0;
        p2.shdr.type = SUBCMD60_TELEPORT;
        p2.shdr.size = 5;
        p2.shdr.client_id = c->client_id;
        p2.x = c2->x;
        p2.y = c2->y;
        p2.z = c2->z;
        p2.w = c2->w;

        /* Update the teleporter's position. */
        c->x = c2->x;
        c->y = c2->y;
        c->z = c2->z;
        c->w = c2->w;

        /* Send the packet to everyone in the lobby */
        return lobby_send_pkt_dc(l, NULL, (dc_pkt_hdr_t*)&p2, 0);
    }
}

/* �÷�: /showdcpc [off] */
static int handle_showdcpc(ship_client_t* c, const char* params) {
    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Check if the client is on PSOGC */
    if (c->version != CLIENT_VERSION_GC) {
        return send_txt(c, "%s", __(c, "\tE\tC7Only valid on Gamecube."));
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_SHOW_DCPC_ON_GC;
        return send_txt(c, "%s", __(c, "\tE\tC7DC/PC games hidden."));
    }

    /* Set the flag, and tell the client that its been set. */
    c->flags |= CLIENT_FLAG_SHOW_DCPC_ON_GC;
    return send_txt(c, "%s", __(c, "\tE\tC7DC/PC games visible."));
}

/* �÷�: /allowgc [off] */
static int handle_allowgc(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    /* Lock the lobby mutex... we've got some work to do. */
    pthread_mutex_lock(&l->mutex);

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        pthread_mutex_unlock(&l->mutex);
        return get_gm_priv(c);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is the leader of the team. */
    if (l->leader_id != c->client_id) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s",
            __(c, "\tE\tC7ֻ������Ϸ����ķ����ſ���ʹ��."));
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        l->flags &= ~LOBBY_FLAG_GC_ALLOWED;
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7GameCube ���������."));
    }

    /* Make sure there's no conflicting flags */
    if ((l->flags & LOBBY_FLAG_DCONLY) || (l->flags & LOBBY_FLAG_PCONLY) ||
        (l->flags & LOBBY_FLAG_V1ONLY)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7Game flag conflict."));
    }

    /* We passed the check, set the flag and unlock the lobby. */
    l->flags |= LOBBY_FLAG_GC_ALLOWED;
    pthread_mutex_unlock(&l->mutex);

    /* Tell the leader that the command has been activated. */
    return send_txt(c, "%s", __(c, "\tE\tC7Gamecube allowed."));
}

/* �÷� /ws item1,item2,item3,item4 */
static int handle_ws(ship_client_t* c, const char* params) {
    uint32_t ws[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    int count;
    uint8_t tmp[0x24] = { 0 };
    subcmd_pkt_t* p = (subcmd_pkt_t*)tmp;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X,%X,%X,%X", ws + 0, ws + 1, ws + 2, ws + 3);

    if (count == EOF || count == 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid WS code."));
    }

    /* �������ͷ */
    memset(p, 0, 0x24);
    memset(&tmp[0x0C], 0xFF, 0x10);
    p->hdr.dc.pkt_type = GAME_SUBCMD60_TYPE;
    p->hdr.dc.pkt_len = LE16(0x24);
    p->type = SUBCMD60_WORD_SELECT;
    p->size = 0x08;

    if (c->version != CLIENT_VERSION_GC) {
        tmp[6] = c->client_id;
    }
    else {
        tmp[7] = c->client_id;
    }

    tmp[8] = count;
    tmp[10] = 1;
    tmp[12] = (uint8_t)(ws[0]);
    tmp[13] = (uint8_t)(ws[0] >> 8);
    tmp[14] = (uint8_t)(ws[1]);
    tmp[15] = (uint8_t)(ws[1] >> 8);
    tmp[16] = (uint8_t)(ws[2]);
    tmp[17] = (uint8_t)(ws[2] >> 8);
    tmp[18] = (uint8_t)(ws[3]);
    tmp[19] = (uint8_t)(ws[3] >> 8);

    /* Send the packet to everyone, including the person sending the request. */
    send_pkt_dc(c, (dc_pkt_hdr_t*)p);
    return subcmd_handle_60(c, p);
}

/* �÷� /item item1,item2,item3,item4 */
static int handle_item(ship_client_t* src, const char* params) {
    uint32_t item[4] = { 0, 0, 0, 0 };
    int count;
    lobby_t* l = src->cur_lobby;
    litem_t* litem;
    subcmd_drop_stack_t dc = { 0 };
    subcmd_bb_drop_stack_t bb = { 0 };

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return get_gm_priv(src);
    }

    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s", __(src, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X,%X,%X,%X", &item[0], &item[1], &item[2],
        &item[3]);

    if (count == EOF || count == 0) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s", __(src, "\tE\tC7��Ч��Ʒ����."));
    }

    /* Clear the set item */
    clear_inv_item(&src->new_item);

    /* Copy over the item data. */
    src->new_item.datal[0] = SWAP32(item[0]);
    src->new_item.datal[1] = SWAP32(item[1]);
    src->new_item.datal[2] = SWAP32(item[2]);
    src->new_item.data2l = SWAP32(item[3]);

    pmt_item_base_check_t item_base_check = get_item_definition_bb(src->new_item.datal[0], src->new_item.datal[1]);
    if (item_base_check.err) {
        clear_inv_item(&src->new_item);
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s \n������ %d", __(src, "\tE\tC4��Ч��Ʒ����,������Ʒ������."), item_base_check.err);
    }

    /* If we're on Blue Burst, add the item to the lobby's inventory first. */
    if (l->version == CLIENT_VERSION_BB) {
        litem = add_new_litem_locked(l, &src->new_item, src->cur_area, src->x, src->z);

        if (!litem) {
            pthread_mutex_unlock(&l->mutex);
            return send_txt(src, "%s", __(src, "\tE\tC4����Ʒ�ռ䲻��򲻴��ڸ���Ʒ."));
        }

        /* Generate the packet to drop the item */
        subcmd_send_lobby_drop_stack_bb(src, 0xFBFF, NULL, litem);
        pthread_mutex_unlock(&l->mutex);
    }
    else {
        ++l->item_player_id[src->client_id];

        /* Generate the packet to drop the item */
        subcmd_send_lobby_drop_stack_dc(src, 0xFBFF, NULL, src->new_item, src->cur_area, src->x, src->z);
        pthread_mutex_unlock(&l->mutex);
    }

    send_txt(src, "%s %s %s",
        __(src, "\tE\tC8��Ʒ:"),
        item_get_name(&src->new_item, src->version, 0),
        __(src, "\tE\tC6���óɹ�, ��������."));

    GM_LOG("%s ʹ��Ȩ������:", get_player_describe(src));
    print_item_data(&src->new_item, l->version);

    return 0;
}

/* �÷� /item1 item1 */
static int handle_item1(ship_client_t* src, const char* params) {
    uint32_t item;
    lobby_t* l = src->cur_lobby;
    int count;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return get_gm_priv(src);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(src, "%s", __(src, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X", &item);

    if (count == EOF || count == 0) {
        return send_txt(src, "%s", __(src, "\tE\tC7��Ч item1 ��Ʒ����."));
    }

    pmt_item_base_check_t item_base_check = get_item_definition_bb(item, src->new_item.datal[0]);
    if (item_base_check.err) {
        clear_inv_item(&src->new_item);
        return send_txt(src, "%s \n������ %d", __(src, "\tE\tC4��Ч��Ʒ����,������Ʒ������."), item_base_check.err);
    }

    src->new_item.datal[0] = SWAP32(item);

    GM_LOG("%s ʹ��Ȩ���޸� item1:", get_player_describe(src));
    print_item_data(&src->new_item, src->version);

    return send_txt(src, "%s %s %s",
        __(src, "\tE\tC8��Ʒ:"),
        item_get_name(&src->new_item, src->version, 0),
        __(src, "\tE\tC6 new_item item1 ���óɹ�, ��������."));
}

/* �÷� /item2 item2 */
static int handle_item2(ship_client_t* src, const char* params) {
    uint32_t item;
    lobby_t* l = src->cur_lobby;
    int count;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return get_gm_priv(src);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(src, "%s", __(src, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X", &item);

    if (count == EOF || count == 0) {
        return send_txt(src, "%s", __(src, "\tE\tC7��Ч item2 ��Ʒ����."));
    }

    src->new_item.datal[1] = SWAP32(item);

    GM_LOG("%s ʹ��Ȩ���޸� item2:", get_player_describe(src));
    print_item_data(&src->new_item, src->version);

    return send_txt(src, "%s", __(src, "\tE\tC7next_item item2 ���óɹ�."));
}

/* �÷� /item3 item3 */
static int handle_item3(ship_client_t* src, const char* params) {
    uint32_t item;
    lobby_t* l = src->cur_lobby;
    int count;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return get_gm_priv(src);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(src, "%s", __(src, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X", &item);

    if (count == EOF || count == 0) {
        return send_txt(src, "%s", __(src, "\tE\tC7��Ч item3 ��Ʒ����."));
    }

    src->new_item.datal[2] = SWAP32(item);

    GM_LOG("%s ʹ��Ȩ���޸� item3:", get_player_describe(src));
    print_item_data(&src->new_item, src->version);

    return send_txt(src, "%s", __(src, "\tE\tC7next_item item3 ���óɹ�."));
}

/* �÷� /item4 item4 */
static int handle_item4(ship_client_t* src, const char* params) {
    uint32_t item;
    lobby_t* l = src->cur_lobby;
    int count;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return get_gm_priv(src);
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(src, "%s", __(src, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X", &item);

    if (count == EOF || count == 0) {
        return send_txt(src, "%s", __(src, "\tE\tC7��Ч item4 ��Ʒ����."));
    }

    src->new_item.data2l = SWAP32(item);

    GM_LOG("%s ʹ��Ȩ���޸� item4:", get_player_describe(src));
    print_item_data(&src->new_item, src->version);

    return send_txt(src, "%s", __(src, "\tE\tC7next_item item4 ���óɹ�."));
}

/* �÷�: /miitem */
static int handle_miitem(ship_client_t* src, const char* params) {
    lobby_t* l = src->cur_lobby;
    litem_t* litem;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return get_gm_priv(src);
    }

    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s", __(src, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Make sure there's something set with /item */
    if (!src->new_item.datal[0]) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s\n%s", __(src, "\tE\tC7����������Ʒ��ID."),
            __(src, "\tE\tC7/item code1,code2,code3,code4."));
    }

    /* If we're on Blue Burst, add the item to the lobby's inventory first. */
    if (l->version == CLIENT_VERSION_BB) {
        litem = add_new_litem_locked(l, &src->new_item, src->cur_area, src->x, src->z);

        if (!litem) {
            pthread_mutex_unlock(&l->mutex);
            return send_txt(src, "%s", __(src, "\tE\tC4����Ʒ�ռ䲻��򲻴��ڸ���Ʒ."));
        }

        /* Generate the packet to drop the item */
        subcmd_send_lobby_drop_stack_bb(src, 0xFBFF, NULL, litem);
        pthread_mutex_unlock(&l->mutex);
    }
    else {
        ++l->item_player_id[src->client_id];

        /* Generate the packet to drop the item */
        subcmd_send_lobby_drop_stack_dc(src, 0xFBFF, NULL, src->new_item, src->cur_area, src->x, src->z);
        pthread_mutex_unlock(&l->mutex);
    }

    send_txt(src, "%s %s %s",
        __(src, "\tE\tC8��Ʒ:"),
        item_get_name(&src->new_item, src->version, 0),
        __(src, "\tE\tC6���ɳɹ�."));

    GM_LOG("%s ʹ��Ȩ������:", get_player_describe(src));
    print_item_data(&src->new_item, l->version);

    return 0;
}

static void dumpinv_internal(ship_client_t* src) {
    int i;
    int v = src->version;

    if (v != CLIENT_VERSION_BB) {
        psocn_v1v2v3pc_char_t* character_v1 = get_client_char_nobb(src);

        GM_LOG("////////////////////////////////////////////////////////////");
        GM_LOG("���: %s ��������ת��", get_player_describe(src));
        GM_LOG("ְҵ: %s ����ģʽ: %s", pso_class[character_v1->dress_data.ch_class].cn_name, src->mode ? "ģʽ" : "��ͨ");
        GM_LOG("�ȼ�: %u ����: %u", character_v1->disp.level + 1, character_v1->disp.exp);
        GM_LOG("Ǯ��: %u", character_v1->disp.meseta);
        GM_LOG("����: %u", character_v1->inv.item_count);

        GM_LOG("------------------------------------------------------------");
        for (i = 0; i < character_v1->inv.item_count; ++i) {
            print_iitem_data(&character_v1->inv.iitems[i], i, src->version);
        }
        GM_LOG("////////////////////////////////////////////////////////////");
    }
    else {
        psocn_bb_char_t* character_bb = get_client_char_bb(src);

        GM_LOG("////////////////////////////////////////////////////////////");
        GM_LOG("���: %s ��������ת��", get_player_describe(src));
        GM_LOG("ְҵ: %s ����ģʽ: %s", pso_class[character_bb->dress_data.ch_class].cn_name, src->mode ? "ģʽ" : "��ͨ");
        GM_LOG("�ȼ�: %u ����: %u", character_bb->disp.level + 1, character_bb->disp.exp);
        GM_LOG("Ǯ��: %u", character_bb->disp.meseta);
        GM_LOG("����: %u", character_bb->inv.item_count);
        GM_LOG("HPҩ: %u TPҩ: %u ����: %u", character_bb->inv.hpmats_used, character_bb->inv.tpmats_used, character_bb->inv.language);

        GM_LOG("------------------------------------------------------------");
        for (i = 0; i < character_bb->inv.item_count; ++i) {
            print_iitem_data(&character_bb->inv.iitems[i], i, src->version);
        }
        GM_LOG("////////////////////////////////////////////////////////////");
    }

}

/* �÷�: /dbginv [l/client_id/guildcard] */
static int handle_dbginv(ship_client_t* src, const char* params) {
    lobby_t* l = src->cur_lobby;
    litem_t* j;
    int do_lobby;
    uint32_t client;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return get_gm_priv(src);
    }

    do_lobby = params && !strcmp(params, "l");

    /* If the arguments say "lobby", then dump the lobby's inventory. */
    if (do_lobby) {
        pthread_mutex_lock(&l->mutex);

        if (l->type != LOBBY_TYPE_GAME || l->version != CLIENT_VERSION_BB) {
            pthread_mutex_unlock(&l->mutex);
            return send_txt(src, "%s", __(src, "\tE\tC7��Ч������BB�汾�ͻ���."));
        }

        GM_LOG("////////////////////////////////////////////////////////////");
        GM_LOG("���������������ת�� %s (%" PRIu32 ")", l->name,
            l->lobby_id);

        GM_LOG("------------------------------------------------------------");
        TAILQ_FOREACH(j, &l->item_queue, qentry) {
            GM_LOG("λ�ò���: x:%f z:%f area:%u", j->x, j->z, j->area);
            print_iitem_data(&j->iitem, find_litem_index(l, &j->iitem.data), src->version);
        }
        GM_LOG("------------------------------------------------------------");

        pthread_mutex_unlock(&l->mutex);
    }
    /* If there's no arguments, then dump the caller's inventory. */
    else if (!params || params[0] == '\0') {
        dumpinv_internal(src);

        return send_msg(src, TEXT_MSG_TYPE, "%s%s"
            , get_player_describe(src), __(src, "\tE\tC7�ı��������Ѵ洢����־."));
    }
    /* Otherwise, try to parse the arguments. */
    else {
        /* Figure out the user requested */
        errno = 0;
        client = (int)strtoul(params, NULL, 10);

        if (errno) {
            return send_txt(src, "%s", __(src, "\tE\tC7��ЧĿ��."));
        }

        /* See if we have a client ID or a guild card number... */
        if (client < 12) {
            pthread_mutex_lock(&l->mutex);

            if (client >= 4 && l->type == LOBBY_TYPE_GAME) {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(src, "%s", __(src, "\tE\tC7��Ч Client ID."));
            }

            if (l->clients[client]) {
                dumpinv_internal(l->clients[client]);
            }
            else {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(src, "%s", __(src, "\tE\tC7��Ч Client ID."));
            }

            pthread_mutex_unlock(&l->mutex);
        }
        /* Otherwise, assume we're looking for a guild card number. */
        else {
            ship_client_t* target = block_find_client(src->cur_block, client);

            if (!target) {
                return send_txt(src, "%s", __(src, "\tE\tC7δ�ҵ���������."));
            }

            dumpinv_internal(target);

            return send_msg(src, TEXT_MSG_TYPE, "%s%s"
                , get_player_describe(target), __(src, "\tE\tC7�ı��������Ѵ洢����־."));
        }
    }

    return send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC4�������ݴ洢ʧ��."));
}

static void dumpbank_internal(ship_client_t* src) {
    //char name[64];
    size_t i;
    int v = src->version;

    if (v == CLIENT_VERSION_BB) {
        psocn_bb_char_t* character = get_client_char_bb(src);
        psocn_bank_t* bank = get_client_bank_bb(src);

        //istrncpy16_raw(ic_utf16_to_gb18030, name, &c->bb_pl->character.name.char_name[0], 64,
        //    BB_CHARACTER_CHAR_NAME_WLENGTH);
        GM_LOG("////////////////////////////////////////////////////////////");
        GM_LOG("���: %s ��������ת��", get_player_describe(src));
        GM_LOG("ְҵ: %s ����: %s ģʽ: %s", pso_class[character->dress_data.ch_class].cn_name, src->bank_type ? "����" : "��ɫ", src->mode ? "ģʽ" : "��ͨ");
        GM_LOG("����: %u ������ %u", bank->item_count, bank->meseta);

        GM_LOG("------------------------------------------------------------");
        for (i = 0; i < bank->item_count; ++i) {
            print_bitem_data(&bank->bitems[i], i, src->version);
        }
        GM_LOG("////////////////////////////////////////////////////////////");
    }
    else {
        send_txt(src, "%s", __(src, "\tE\tC7������BBʹ��."));
    }
}

/* �÷�: /dbgbank [clientid/guildcard] */
static int handle_dbgbank(ship_client_t* src, const char* params) {
    lobby_t* l = src->cur_lobby;
    uint32_t client;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return get_gm_priv(src);
    }

    if (!params || params[0] == '\0') {
        dumpbank_internal(src);

        return send_msg(src, TEXT_MSG_TYPE, "%s%s"
            , get_player_describe(src), __(src, "\tE\tC7�����������Ѵ洢����־."));
    }
    /* Otherwise, try to parse the arguments. */
    else {
        /* Figure out the user requested */
        errno = 0;
        client = (int)strtoul(params, NULL, 10);

        if (errno) {
            return send_txt(src, "%s", __(src, "\tE\tC7��ЧĿ��."));
        }

        /* See if we have a client ID or a guild card number... */
        if (client < 12) {
            pthread_mutex_lock(&l->mutex);

            if (client >= 4 && l->type == LOBBY_TYPE_GAME) {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(src, "%s", __(src, "\tE\tC7��Ч Client ID."));
            }

            if (l->clients[client]) {
                dumpbank_internal(l->clients[client]);
            }
            else {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(src, "%s", __(src, "\tE\tC7��Ч Client ID."));
            }

            pthread_mutex_unlock(&l->mutex);
        }
        /* Otherwise, assume we're looking for a guild card number. */
        else {
            ship_client_t* target = block_find_client(src->cur_block, client);

            if (!target) {
                return send_txt(src, "%s", __(src, "\tE\tC7δ�ҵ���������."));
            }

            dumpbank_internal(target);

            return send_msg(src, TEXT_MSG_TYPE, "%s%s"
                , get_player_describe(target), __(src, "\tE\tC7�����������Ѵ洢����־."));
        }
    }

    return send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC4�������ݴ洢ʧ��."));
}

/* �÷�: /stfu guildcard */
static int handle_stfu(ship_client_t* c, const char* params) {
    uint32_t gc;
    block_t* b = c->cur_block;
    ship_client_t* i;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Look for the requested user and STFU them (only on this block). */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc && i->privilege < c->privilege) {
            i->flags |= CLIENT_FLAG_STFU;
            return send_txt(c, "%s", __(c, "\tE\tC7Client STFUed."));
        }
    }

    /* The person isn't here... There's nothing left to do. */
    return send_txt(c, "%s", __(c, "\tE\tC7Guildcard not found"));
}

/* �÷�: /unstfu guildcard */
static int handle_unstfu(ship_client_t* c, const char* params) {
    uint32_t gc;
    block_t* b = c->cur_block;
    ship_client_t* i;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Look for the requested user and un-STFU them (only on this block). */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if (i->guildcard == gc) {
            i->flags &= ~CLIENT_FLAG_STFU;
            return send_txt(c, "%s", __(c, "\tE\tC7Client un-STFUed."));
        }
    }

    /* The person isn't here... There's nothing left to do. */
    return send_txt(c, "%s", __(c, "\tE\tC7Guildcard not found."));
}

/* �÷�: /gameevent number */
static int handle_gameevent(ship_client_t* c, const char* params) {
    int event;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Grab the event number */
    event = atoi(params);

    if (event < 0 || event > 6) {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid event code."));
    }

    ship->game_event = event;

    return send_txt(c, "%s", __(c, "\tE\tC7Game Event set."));
}

/* �÷�: /ban:d guildcard reason */
static int handle_ban_d(ship_client_t* c, const char* params) {
    uint32_t gc;
    block_t* b;
    ship_client_t* i;
    char* reason;
    uint32_t j;

    /* Make sure the requester is a local GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Set the ban in the list (86,400s = 7 days) */
    if (ban_guildcard(ship, time(NULL) + 86400, c->guildcard, gc, reason + 1)) {
        return send_txt(c, "%s", __(c, "\tE\tC7���÷����������."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for (j = 0; j < ship->cfg->blocks; ++j) {
        if ((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if (i->guildcard == gc) {
                    if (strlen(reason) > 1) {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s\n%s\n%s",
                            __(i, "\tE���ѱ�GM������������."), __(i, "���ʱ��:"),
                            __(i, "1 ��"), __(i, "�������:"),
                            reason + 1);
                    }
                    else {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s",
                            __(i, "\tE���ѱ�GM������������."), __(i, "���ʱ��:"),
                            __(i, "1 ��"));
                    }

                    i->flags |= CLIENT_FLAG_DISCONNECTED;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return send_txt(c, "%s", __(c, "\tE\tC7�ѳɹ���������."));
}

/* �÷�: /ban:w guildcard reason */
static int handle_ban_w(ship_client_t* c, const char* params) {
    uint32_t gc;
    block_t* b;
    ship_client_t* i;
    char* reason;
    uint32_t j;

    /* Make sure the requester is a local GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Set the ban in the list (604,800s = 7 days) */
    if (ban_guildcard(ship, time(NULL) + 604800, c->guildcard, gc, reason + 1)) {
        return send_txt(c, "%s", __(c, "\tE\tC7Error setting ban."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for (j = 0; j < ship->cfg->blocks; ++j) {
        if ((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if (i->guildcard == gc) {
                    if (strlen(reason) > 1) {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s\n%s\n%s",
                            __(i, "\tEYou have been banned from "
                                "this ship."), __(i, "Ban Length:"),
                            __(i, "1 week"), __(i, "Reason:"),
                            reason + 1);
                    }
                    else {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s",
                            __(i, "\tEYou have been banned from "
                                "this ship."), __(i, "Ban Length:"),
                            __(i, "1 week"));
                    }

                    i->flags |= CLIENT_FLAG_DISCONNECTED;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return send_txt(c, "%s", __(c, "\tE\tC7Successfully set ban."));
}

/* �÷�: /ban:m guildcard reason */
static int handle_ban_m(ship_client_t* c, const char* params) {
    uint32_t gc;
    block_t* b;
    ship_client_t* i;
    char* reason;
    uint32_t j;

    /* Make sure the requester is a local GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Set the ban in the list (2,592,000s = 30 days) */
    if (ban_guildcard(ship, time(NULL) + 2592000, c->guildcard, gc,
        reason + 1)) {
        return send_txt(c, "%s", __(c, "\tE\tC7Error setting ban."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for (j = 0; j < ship->cfg->blocks; ++j) {
        if ((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if (i->guildcard == gc) {
                    if (strlen(reason) > 1) {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s\n%s\n%s",
                            __(i, "\tEYou have been banned from "
                                "this ship."), __(i, "Ban Length:"),
                            __(i, "30 days"), __(i, "Reason:"),
                            reason + 1);
                    }
                    else {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s",
                            __(i, "\tEYou have been banned from "
                                "this ship."), __(i, "Ban Length:"),
                            __(i, "30 days"));
                    }

                    i->flags |= CLIENT_FLAG_DISCONNECTED;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return send_txt(c, "%s", __(c, "\tE\tC7Successfully set ban."));
}

/* �÷�: /ban:p guildcard reason */
static int handle_ban_p(ship_client_t* c, const char* params) {
    uint32_t gc;
    block_t* b;
    ship_client_t* i;
    uint32_t j;
    char* reason;

    /* Make sure the requester is a local GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Set the ban in the list. An end time of -1 = forever */
    if (ban_guildcard(ship, (time_t)-1, c->guildcard, gc, reason + 1)) {
        return send_txt(c, "%s", __(c, "\tE\tC7Error setting ban."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for (j = 0; j < ship->cfg->blocks; ++j) {
        if ((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if (i->guildcard == gc) {
                    if (strlen(reason) > 1) {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s\n%s\n%s",
                            __(i, "\tEYou have been banned from "
                                "this ship."), __(i, "Ban Length:"),
                            __(i, "Forever"), __(i, "Reason:"),
                            reason + 1);
                    }
                    else {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s",
                            __(i, "\tEYou have been banned from "
                                "this ship."), __(i, "Ban Length:"),
                            __(i, "Forever"));
                    }

                    i->flags |= CLIENT_FLAG_DISCONNECTED;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return send_txt(c, "%s", __(c, "\tE\tC7Successfully set ban."));
}

/* �÷�: /unban guildcard */
static int handle_unban(ship_client_t* c, const char* params) {
    uint32_t gc;
    int rv;

    /* Make sure the requester is a local GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Attempt to lift the ban */
    rv = ban_lift_guildcard_ban(ship, gc);

    /* Did we succeed? */
    if (!rv) {
        return send_txt(c, "%s", __(c, "\tE\tC7Lifted ban."));
    }
    else if (rv == -1) {
        return send_txt(c, "%s", __(c, "\tE\tC7User not banned."));
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7Error lifting ban."));
    }
}

/* �÷�: /gbc message */
static int handle_gbc(ship_client_t* c, const char* params) {
    /* Make sure the requester is a Global GM. */
    if (!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Make sure there's a message to send */
    if (!strlen(params)) {
        return send_txt(c, "%s", __(c, "\tE\tC7ȱ�ٲ���?"));
    }

    return shipgate_send_global_msg(&ship->sg, c->guildcard, params);
}

/* �÷�: /logout */
static int handle_logout(ship_client_t* c, const char* params) {
    /* See if they're logged in first */
    if (!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7δ��¼�����̨."));
    }

    /* Clear the logged in status. */
    c->flags &= ~(CLIENT_FLAG_LOGGED_IN | CLIENT_FLAG_OVERRIDE_GAME);
    c->privilege &= ~(CLIENT_PRIV_LOCAL_GM | CLIENT_PRIV_LOCAL_ROOT);

    return send_txt(c, "%s", __(c, "\tE\tC7�ѵǳ������̨."));
}

/* �÷�: /override */
static int handle_override(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the requester is a GM */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Make sure that the requester is in a lobby, not a team */
    if (l->type != LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7�޷�����Ϸ������ʹ��."));
    }

    /* Set the flag so we know when they join a lobby */
    c->flags |= CLIENT_FLAG_OVERRIDE_GAME;

    return send_txt(c, "%s", __(c, "\tE\tC7Lobby restriction override on."));
}

/* �÷�: /maps [numeric string] */
static int handle_maps(ship_client_t* c, const char* params) {
    uint32_t maps[32] = { 0 };
    int i = 0;

    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Read in the maps string, one character at a time. Any undefined entries
       at the end will give you a 0 in their place. */
    while (*params) {
        if (!isdigit(*params)) {
            return send_txt(c, "%s", __(c, "\tE\tC7��Ч��ͼ����."));
        }
        else if (i > 31) {
            return send_txt(c, "%s", __(c, "\tE\tC7��ͼ����������������."));
        }

        maps[i++] = *params - '0';
        params++;
    }

    /* Free any old set, if there is one. */
    if (c->next_maps) {
        free_safe(c->next_maps);
    }

    /* Save the maps string into the client's struct. */
    c->next_maps = (uint32_t*)malloc(sizeof(uint32_t) * 32);
    if (!c->next_maps) {
        return send_txt(c, "%s", __(c, "\tE\tC7δ֪����.!c->next_maps"));
    }

    memcpy(c->next_maps, maps, sizeof(uint32_t) * 32);

    /* We're done. */
    return send_txt(c, "%s", __(c, "\tE\tC7��������һ����ͼ."));
}

/* �÷�: /exp amount */
static int handle_exp(ship_client_t* c, const char* params) {
    uint32_t amt;
    lobby_t* l = c->cur_lobby;

    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Make sure that the requester is in a team, not a lobby */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is on Blue Burst */
    if (c->version != CLIENT_VERSION_BB) {
        return send_txt(c, "%s", __(c, "\tE\tC7��ָ��ֻ������ Blue Burst."));
    }

    /* Figure out the amount requested */
    errno = 0;
    amt = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid amount */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч������ֵ."));
    }

    return client_give_exp(c, amt);
}

/* �÷�: /level [destination level, or blank for one level up] */
static int handle_level(ship_client_t* c, const char* params) {
    uint32_t amt;
    lobby_t* l = c->cur_lobby;

    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    if (c->version == CLIENT_VERSION_BB) {
        /* Make sure that the requester is in a team, not a lobby */
        if (l->type != LOBBY_TYPE_GAME) {
            return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));
        }

        /* Figure out the level requested */
        if (params && strlen(params)) {
            errno = 0;
            amt = (uint32_t)strtoul(params, NULL, 10);

            if (errno != 0) {
                /* Send a message saying invalid amount */
                return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ȼ�."));
            }

            amt -= 1;
        }
        else {
            psocn_bb_char_t* character = get_client_char_bb(c);
            amt = LE32(character->disp.level) + 1;
        }

        /* If the level is too high, let them know. */
        if (amt > 199) {
            return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ȼ�."));
        }

        return client_give_level(c, amt);
    }
    else if (c->version == CLIENT_VERSION_DCV2 ||
        c->version == CLIENT_VERSION_PC) {
        /* Make sure that the requester is in a lobby, not a team */
        if (l->type == LOBBY_TYPE_GAME) {
            return send_txt(c, "%s", __(c, "\tE\tC7Not valid in a team."));
        }

        /* Figure out the level requested */
        if (params && strlen(params)) {
            errno = 0;
            amt = (uint32_t)strtoul(params, NULL, 10);

            if (errno != 0) {
                /* Send a message saying invalid amount */
                return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ȼ�."));
            }

            amt -= 1;
        }
        else {
            psocn_v1v2v3pc_char_t* character = get_client_char_nobb(c);
            amt = LE32(character->disp.level) + 1;
        }

        /* If the level is too high, let them know. */
        if (amt > 199) {
            return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ȼ�."));
        }

        return client_give_level_v2(c, amt);
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7Not valid on your version."));
    }
}

/* �÷�: /sdrops [off] */
static int handle_sdrops(ship_client_t* c, const char* params) {
    /* See if we can enable server-side drops or not on this ship. */
    switch (c->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
        if (!pt_v2_enabled() || !map_have_v2_maps() || !pmt_enabled_v2() ||
            !rt_v2_enabled())
            return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                "suported on this ship for\n"
                "this client version."));
        break;

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_XBOX:
        /* XXXX: GM-only until they're fixed... */
        if (!pt_gc_enabled() || !map_have_gc_maps() || !pmt_enabled_gc() ||
            !rt_gc_enabled() || !LOCAL_GM(c))
            return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                "suported on this ship for\n"
                "this client version."));
        break;

    case CLIENT_VERSION_EP3:
        return send_txt(c, "%s", __(c, "\tE\tC7��֧�� Episode 3 �汾."));

    case CLIENT_VERSION_BB:
        /* XXXX: GM-only until they're fixed... */
        if (!pt_bb_enabled() || !map_have_bb_maps() || !pmt_enabled_bb() ||
            !rt_bb_enabled() || !LOCAL_GM(c))
            return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                "suported on this ship for\n"
                "this client version."));
        /* �� Blue Burst ��Ч(BB�����������������) */
        //return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst ��֧�ָ�ָ��."));
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_SERVER_DROPS;
        return send_txt(c, "%s", __(c, "\tE\tC7����������ģʽ�ر�."));
    }

    c->flags |= CLIENT_FLAG_SERVER_DROPS;
    return send_txt(c, "%s", __(c, "\tE\tC7�·��俪������������ģʽ."));
}

/* �÷�: /trackinv */
static int handle_trackinv(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    /* Make sure that the requester is in a plain old lobby. */
    if (l->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7�޷�����Ϸ������ʹ��."));

    /* Set the flag... */
    c->flags |= CLIENT_FLAG_TRACK_INVENTORY;

    return send_txt(c, "%s", __(c, "\tE\tC7������Ʒ����׷���ѿ���."));
}

/* �÷�: /ep3music value */
static int handle_ep3music(ship_client_t* c, const char* params) {
    uint32_t song;
    lobby_t* l = c->cur_lobby;
    uint8_t rawpkt[12] = { 0 };
    subcmd_change_lobby_music_GC_Ep3_t* pkt = (subcmd_change_lobby_music_GC_Ep3_t*)rawpkt;

    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    /* Make sure that the requester is in a lobby, not a team */
    if (l->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7�޷�����Ϸ������ʹ��."));

    /* Figure out the level requested */
    if (!params || !strlen(params))
        return send_txt(c, "%s", __(c, "\tE\tC7��ָ����������."));

    errno = 0;
    song = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0)
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч��������."));

    /* Prepare the packet. */
    pkt->hdr.dc.pkt_type = GAME_SUBCMD60_TYPE;
    pkt->hdr.dc.flags = 0x00;
    pkt->hdr.dc.pkt_len = LE16(0x000C);
    pkt->shdr.type = SUBCMD60_JUKEBOX;
    pkt->shdr.size = 0x02;
    pkt->shdr.unused = 0x0000;
    pkt->song_number = song;

    /* Send it. */
    subcmd_send_lobby_dc(l, NULL, (subcmd_pkt_t*)pkt, 0);
    return 0;
}

/* �÷�: /dsdrops version difficulty section episode */
static int handle_dsdrops(ship_client_t* c, const char* params) {
#ifdef DEBUG
    uint8_t ver, diff, section, ep;
    char* sver, * sdiff, * ssection, * sep;
    char* tok, * str;
#endif

    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    if (c->version == CLIENT_VERSION_BB)
        return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst ��֧�ָ�ָ��."));

#ifndef DEBUG
    return send_txt(c, "%s", __(c, "\tE\tC7Debug support not compiled in."));
#else
    if (!(str = _strdup(params)))
        return send_txt(c, "%s", __(c, "\tE\tC7�ڲ�����������."));

    /* Grab each token we're expecting... */
    sver = strtok_s(str, " ,", &tok);
    sdiff = strtok_s(NULL, " ,", &tok);
    ssection = strtok_s(NULL, " ,", &tok);
    sep = strtok_s(NULL, " ,", &tok);

    if (!ssection || !sdiff || !sver || !sep) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7ָ�����ȱʧ."));
    }

    /* Parse the arguments */
    if (!strcmp(sver, "v2")) {
        if (!pt_v2_enabled() || !map_have_v2_maps() || !pmt_v2_enabled() ||
            !rt_v2_enabled()) {
            free_safe(str);
            return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                "suported on this ship for\n"
                "this client version."));
        }

        ver = CLIENT_VERSION_DCV2;
    }
    else if (!strcmp(sver, "gc")) {
        if (!pt_gc_enabled() || !map_have_gc_maps() || !pmt_gc_enabled() ||
            !rt_gc_enabled()) {
            free_safe(str);
            return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                "suported on this ship for\n"
                "this client version."));
        }
        ver = CLIENT_VERSION_GC;
    }
    else {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid version."));
    }

    errno = 0;
    diff = (uint8_t)strtoul(sdiff, NULL, 10);

    if (errno || diff > 3) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid difficulty."));
    }

    section = (uint8_t)strtoul(ssection, NULL, 10);

    if (errno || section > 9) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid section ID."));
    }

    ep = (uint8_t)strtoul(sep, NULL, 10);

    if (errno || (ep != 1 && ep != 2) ||
        (ver == CLIENT_VERSION_DCV2 && ep != 1)) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid episode."));
    }

    /* We got everything, save it in the client structure and we're done. */
    c->sdrops_ver = ver;
    c->sdrops_diff = diff;
    c->sdrops_section = section;
    c->sdrops_ep = ep;
    c->flags |= CLIENT_FLAG_DBG_SDROPS;
    free_safe(str);

    return send_txt(c, "%s", __(c, "\tE\tC7Enabled server-side drop debug."));
#endif
}

/* �÷�: /lflags */
static int handle_lflags(ship_client_t* c, const char* params) {
    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    return send_txt(c, "\tE\tC7%08x", c->cur_lobby->flags);
}

/* �÷�: /cflags */
static int handle_cflags(ship_client_t* c, const char* params) {
    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    return send_txt(c, "\tE\tC7%08x", c->flags);
}

/* �÷�: /t x, y, z */
static int handle_t(ship_client_t* c, const char* params) {
    char* str, * tok = { 0 }, * xs, * ys, * zs;
    float x, y, z;
    subcmd_teleport_t p2 = { 0 };
    lobby_t* l = c->cur_lobby;

    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    if (!(str = _strdup(params)))
        return send_txt(c, "%s", __(c, "\tE\tC7�ڲ�����������."));

    /* Grab each token we're expecting... */
    xs = strtok_s(str, " ,", &tok);
    ys = strtok_s(NULL, " ,", &tok);
    zs = strtok_s(NULL, " ,", &tok);

    if (!xs || !ys || !zs) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7ָ�����ȱʧ."));
    }

    /* Parse out the numerical values... */
    errno = 0;
    x = strtof(xs, NULL);
    y = strtof(ys, NULL);
    z = strtof(zs, NULL);
    free_safe(str);

    /* Did they all parse ok? */
    if (errno)
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч��λ��."));

    /* Hopefully they know what they're doing... Send the packet. */
    p2.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    p2.hdr.pkt_len = sizeof(subcmd_teleport_t);
    p2.hdr.flags = 0;
    p2.shdr.type = SUBCMD60_TELEPORT;
    p2.shdr.size = 5;
    p2.shdr.client_id = c->client_id;
    p2.x = x;
    p2.y = y;
    p2.z = z;
    p2.w = c->w;

    c->x = x;
    c->y = y;
    c->z = z;

    /* Send the packet to everyone in the lobby */
    return lobby_send_pkt_dc(l, NULL, (dc_pkt_hdr_t*)&p2, 0);
}

/* �÷�: /quest id */
static int handle_quest(ship_client_t* c, const char* params) {
    char* str, * tok = { 0 }, * qid;
    lobby_t* l = c->cur_lobby;
    uint32_t quest_id;
    int rv;

    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME)
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����Ϸ��������Ч."));

    if (l->flags & LOBBY_FLAG_BURSTING)
        return send_txt(c, "%s", __(c, "\tE\tC4ԾǨ��,���Ժ�����."));

    if (!(str = _strdup(params)))
        return send_txt(c, "%s", __(c, "\tE\tC7�ڲ�����������."));

    /* Grab each token we're expecting... */
    qid = strtok_s(str, " ,", &tok);

    /* Attempt to parse out the quest id... */
    errno = 0;
    quest_id = (uint32_t)strtoul(qid, NULL, 10);

    if (errno) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����ID."));
    }

    /* Done with this... */
    free_safe(str);

    pthread_rwlock_rdlock(&ship->qlock);

    /* Do we have quests configured? */
    if (!TAILQ_EMPTY(&ship->qmap)) {
        /* Find the quest first, since someone might be doing something
           stupid... */
        quest_map_elem_t* e = quest_lookup(&ship->qmap, quest_id);

        /* If the quest isn't found, bail out now. */
        if (!e) {
            rv = send_txt(c, __(c, "\tE\tC7��Ч����ID."));
            pthread_rwlock_unlock(&ship->qlock);
            return rv;
        }

        rv = lobby_setup_quest(l, c, quest_id, CLIENT_LANG_CHINESE_S);
    }
    else {
        rv = send_txt(c, "%s", __(c, "\tE\tC4����δ����."));
    }

    pthread_rwlock_unlock(&ship->qlock);

    return rv;
}

/* �÷�: /teamlog */
static int handle_teamlog(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int rv;

    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME)
        return send_txt(c, "%s", __(c, "\tE\tC7Only valid in a team."));

    rv = team_log_start(l);

    if (!rv) {
        return send_txt(c, "%s", __(c, "\tE\tC7Logging started."));
    }
    else if (rv == -1) {
        return send_txt(c, "%s", __(c, "\tE\tC7The team is already\n"
            "being logged."));
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7Cannot create log file."));
    }
}

/* �÷�: /eteamlog */
static int handle_eteamlog(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int rv;

    /* Make sure the requester is a local GM, at least. */
    if (!LOCAL_GM(c))
        return get_gm_priv(c);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME)
        return send_txt(c, "%s", __(c, "\tE\tC7Only valid in a team."));

    rv = team_log_stop(l);

    if (!rv) {
        return send_txt(c, "%s", __(c, "\tE\tC7Logging ended."));
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7The team is not\n"
            "being logged."));
    }
}

/* �÷�: /ib days ip reason */
static int handle_ib(ship_client_t* c, const char* params) {
    struct sockaddr_storage addr = { 0 }, netmask = { 0 };
    struct sockaddr_in* ip = (struct sockaddr_in*)&addr;
    struct sockaddr_in* nm = (struct sockaddr_in*)&netmask;
    block_t* b;
    ship_client_t* i;
    char* slen, * sip, * reason;
    char* str, * tok = { 0 };
    uint32_t j;
    uint32_t len;

    /* Make sure the requester is a local GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    if (!(str = _strdup(params))) {
        return send_txt(c, "%s", __(c, "\tE\tC7�ڲ�����������."));
    }

    /* Grab each token we're expecting... */
    slen = strtok_s(str, " ,", &tok);
    sip = strtok_s(NULL, " ,", &tok);
    reason = strtok_s(NULL, "", &tok);

    /* Figure out the user requested */
    errno = 0;
    len = (uint32_t)strtoul(slen, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid length */
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid ban length."));
    }

    /* Parse the IP address. We only support IPv4 here. */
    if (!my_pton(PF_INET, sip, &addr)) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid IP address."));
    }

    nm->sin_addr.s_addr = EMPTY_STRING;
    addr.ss_family = netmask.ss_family = PF_INET;

    /* Set the ban in the list (86,400s = 1 day) */
    if (ban_ip(ship, time(NULL) + 86400 * (time_t)len, c->guildcard, &addr, &netmask,
        reason)) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Error setting ban."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for (j = 0; j < ship->cfg->blocks; ++j) {
        if ((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                nm = (struct sockaddr_in*)&i->ip_addr;
                if (nm->sin_family == PF_INET &&
                    ip->sin_addr.s_addr == nm->sin_addr.s_addr) {
                    if (reason && strlen(reason)) {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s\n%s",
                            __(i, "\tEYou have been banned from "
                                "this ship."), __(i, "Reason:"),
                            reason);
                    }
                    else {
                        send_msg(i, MSG_BOX_TYPE, "%s",
                            __(i, "\tEYou have been banned from "
                                "this ship."));
                    }

                    i->flags |= CLIENT_FLAG_DISCONNECTED;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    free_safe(str);
    return send_txt(c, "%s", __(c, "\tE\tC7Successfully set ban."));
}

/* �÷�: /cheat [off] */
static int handle_cheat(ship_client_t* c, const char* params) {
    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        c->options.infinite_hp = false;
        c->options.infinite_tp = false;
        c->options.switch_assist = false;
        c->flags &= ~CLIENT_FLAG_INFINITE_TP;
        c->flags &= ~CLIENT_FLAG_INVULNERABLE;
        c->cur_lobby->flags &= ~LOBBY_TYPE_CHEATS_ENABLED;
        return send_txt(c, "%s", __(c, "\tE\tC4����ģʽ�ر�."));
    }
    else if (!strcmp(params, "on")) {

        /* Set the flag since we're turning it on. */
        c->options.infinite_hp = true;
        c->options.infinite_tp = true;
        c->options.switch_assist = true;
        c->flags |= CLIENT_FLAG_INFINITE_TP;
        c->flags |= CLIENT_FLAG_INVULNERABLE;
        c->cur_lobby->flags |= LOBBY_TYPE_CHEATS_ENABLED;
        return send_txt(c, "%s", __(c, "\tE\tC6����ģʽ����."));
    }

    return send_txt(c, "%s", __(c, "\tE\tC4����ģʽ����:on /off."));
}

/* �÷�: /cmdc [opcode1, opcode2] */
static int handle_cmdcheck(ship_client_t* src, const char* params) {
    int count;
    uint16_t opcode[2] = { 0 };
    uint8_t data[512] = { 0 };

    pthread_mutex_lock(&src->mutex);

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        pthread_mutex_unlock(&src->mutex);
        return get_gm_priv(src);
    }

    /* Copy over the item data. */
    count = sscanf(params, "%hX, %hX, %[0-9A-Fa-f]", &opcode[0], &opcode[1], data);

    if (count == EOF || count == 0) {
        return send_txt(src, "%s", __(src, "\tE\tC7��Ч��������."));
    }

    /* See if we're turning the flag off. */
    if (count == 1) {

        DBG_LOG("����ָ�� 0x%04X %s GC %u",
            opcode[0], c_cmd_name(opcode[0], 0), src->guildcard);

        send_bb_cmd_test(src, opcode[0]);

        send_msg(src, TEXT_MSG_TYPE, "%s \n0x%04X", __(src, "\tE\tC6cmd������ִ��1."), opcode[0]);

        pthread_mutex_unlock(&src->mutex);

        return 0;
    }

    /* See if we're turning the flag off. */
    if (opcode[0] && data[0]) {

        DBG_LOG("����ָ�� 0x%04X %s GC %u",
            opcode[0], c_cmd_name(opcode[0], 0), src->guildcard);

        send_bb_cmd_test2(src, opcode[0], data);

        send_msg(src, TEXT_MSG_TYPE, "%s \n0x%04X", __(src, "\tE\tC4cmd������ִ��2."), opcode[0]);

        pthread_mutex_unlock(&src->mutex);

        return 0;
    }

    /* See if we're turning the flag off. */
    if (opcode[0] && opcode[1]) {

        DBG_LOG("����ָ�� 0x%04X %s 0x%04X GC %u",
            opcode[0], c_cmd_name(opcode[0], 0), isEmptyInt(opcode[1]) ? 0 : opcode[1], src->guildcard);

        send_bb_subcmd_test(src, opcode[0], opcode[1]);

        send_msg(src, TEXT_MSG_TYPE, "%s \n0x%04X 0x%04X", __(src, "\tE\tC3subcmd������ִ��3."), opcode[0], opcode[1]);

        pthread_mutex_unlock(&src->mutex);

        return 0;
    }

    pthread_mutex_unlock(&src->mutex);
    return send_txt(src, "%s", __(src, "\tE\tC6ָ���ȷ\n����Ϊ[opcodes]."));
}

/* �÷�: /rsquset TODO δ���*/
static int handle_resetquest(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return get_gm_priv(c);
    }

    if (l->flags & LOBBY_FLAG_QUESTING) {

        if (l->oneperson) {
            c->reset_quest = true;

            /* Attempt to change the player's lobby. */
            join_game(c, l);

            c->reset_quest = false;
            return 0;
        }

        c->reset_quest = true;

        /* Attempt to change the player's lobby. */
        join_game(c, l);

        c->reset_quest = false;
        return 0;
    }

    return send_txt(c, "%s", __(c, "\tE\tC6��ǰ����������."));
}

/* �÷� /login username password */
static int handle_login(ship_client_t* c, const char* params) {
    char username[32] = { 0 }, password[32] = { 0 };
    int len = 0;
    const char* ch = params;

    /* Make sure the user isn't doing something stupid. */
    if (!*params) {
        return send_txt(c, "%s", __(c, "\tE\tC7������:\n"
            "/login �û��� ����\n"
            "���GM��¼��֤."));
    }

    /* Copy over the username/password. */
    while (*ch != ' ' && len < 32) {
        username[len++] = *ch++;
    }

    if (len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�û���."));

    username[len] = '\0';

    len = 0;
    ++ch;

    while (*ch != ' ' && *ch != '\0' && len < 32) {
        password[len++] = *ch++;
    }

    if (len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����."));

    password[len] = '\0';

    /* We'll get success/failure later from the shipgate. */
    return shipgate_send_usrlogin(&ship->sg, c->guildcard, c->cur_block->b,
        username, password, 0, 0);
}
