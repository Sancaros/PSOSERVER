/*
    �λ�֮���й� ���������� ����ָ���б�
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

/* �÷�: /pl */
static int handle_player_menu(ship_client_t* c, const char* params) {
    if (in_lobby(c)) {
        return send_bb_player_menu_list(c);
    }

    return send_txt(c, "%s", __(c, "\tE\tC4�޷��ڷ���ʹ�ø�ָ��."));
}

/* �÷�: /save slot */
static int handle_save(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    uint32_t slot = 0;
    uint16_t data_len = 0;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Make sure that the requester is in a lobby, not a team */
    if (l->type != LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7�޷�����Ϸ������ʹ��."));
    }

    ///* Not valid for Blue Burst clients */
    //if(c->version == CLIENT_VERSION_BB) {
    //    return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst ��֧�ָ�ָ��."));
    //}

    /* Figure out the slot requested */
    errno = 0;
    if (!params || params[0] == '\0')
        slot = c->sec_data.slot;
    else
        slot = (uint32_t)strtoul(params, NULL, 10);

    if (errno || slot > 4 || slot < 1) {
        /* Send a message saying invalid slot */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч��ɫ���."));
    }

    if (c->version == CLIENT_VERSION_BB) {
        /* Is it a Blue Burst character or not? */
        if (data_len > 1056) {
            data_len = PSOCN_STLENGTH_BB_DB_CHAR;

            /* Send the character data to the shipgate */
            if (shipgate_send_cdata(&ship->sg, c->guildcard, slot, c->bb_pl, data_len,
                c->cur_block->b)) {
                /* Send a message saying we couldn't save */
                return send_txt(c, "%s", __(c, "\tE\tC7�޷������ɫ����."));
            }

            /* �����ѡ�����ݴ������ݿ� */
            if (shipgate_send_bb_opts(&ship->sg, c)) {
                /* Send a message saying we couldn't save */
                return send_txt(c, "%s", __(c, "\tE\tC7�޷������ɫѡ������."));
            }
        }
    }
    else {
        /* Adjust so we don't go into the Blue Burst character data */
        slot += 4;

        data_len = 1052;

        /* Send the character data to the shipgate */
        if (shipgate_send_cdata(&ship->sg, c->guildcard, slot, c->pl, data_len,
            c->cur_block->b)) {
            /* Send a message saying we couldn't save */
            return send_txt(c, "%s", __(c, "\tE\tC7�޷������ɫ����."));
        }
    }

    /* An error or success message will be sent when the shipgate gets its
       response. */
    send_txt(c, "%s", __(c, "\tE\tC7�����ɫ���."));
    return 0;
}

/* �÷�: /restore slot */
static int handle_restore(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    uint32_t slot;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Make sure that the requester is in a lobby, not a team */
    if (l->type != LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7�޷�����Ϸ������ʹ��."));
    }

    /* Not valid for Blue Burst clients */
    if (c->version == CLIENT_VERSION_BB) {
        return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst ��֧�ָ�ָ��."));
    }

    /* Figure out the slot requested */
    errno = 0;
    slot = (uint32_t)strtoul(params, NULL, 10);

    if (errno || slot > 4 || slot < 1) {
        /* Send a message saying invalid slot */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч��λ."));
    }

    /* Adjust so we don't go into the Blue Burst character data */
    slot += 4;

    /* Send the request to the shipgate. */
    if (shipgate_send_creq(&ship->sg, c->guildcard, slot)) {
        /* Send a message saying we couldn't request */
        return send_txt(c, "%s",
            __(c, "\tE\tC7�޷���ȡ��ɫ����."));
    }

    return 0;
}

/* �÷�: /bstat */
static int handle_bstat(ship_client_t* c, const char* params) {
    block_t* b = c->cur_block;
    int games, players;

    /* Grab the stats from the block structure */
    pthread_rwlock_rdlock(&b->lobby_lock);
    games = b->num_games;
    pthread_rwlock_unlock(&b->lobby_lock);

    pthread_rwlock_rdlock(&b->lock);
    players = b->num_clients;
    pthread_rwlock_unlock(&b->lock);

    /* Fill in the string. */
    return send_txt(c, "\tE\tC7����%02d:\n%d %s\n%d %s", b->b, players,
        __(c, "���"), games, __(c, "����"));
}

/* �÷�: /legit [off] */
static int handle_legit(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    uint32_t v;
    iitem_t* item;
    psocn_limits_t* limits;
    int j, irv;

    /* Make sure the requester is in a lobby, not a team. */
    if (l->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7�޷�����Ϸ������ʹ��."));

    /* Figure out what version they're on. */
    switch (c->version) {
    case CLIENT_VERSION_DCV1:
        v = ITEM_VERSION_V1;
        break;

    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
        v = ITEM_VERSION_V2;
        break;

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_XBOX:
        v = ITEM_VERSION_GC;
        break;

    case CLIENT_VERSION_EP3:
        return send_txt(c, "%s", __(c, "\tE\tC7�Ϸ�ģʽ��֧�� Episode 3."));

    case CLIENT_VERSION_BB:
        v = ITEM_VERSION_BB;
        break;

    default:
        return -1;
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_LEGIT;
        return send_txt(c, "%s", __(c, "\tE\tC7�����µķ���ĺϷ�ģʽ�ѹر�."));
    }

    /* XXXX: Select the appropriate limits file. */
    pthread_rwlock_rdlock(&ship->llock);
    if (!ship->def_limits) {
        pthread_rwlock_unlock(&ship->llock);
        return send_txt(c, "%s", __(c, "\tE\tC7��ǰ������֧�ֺϷ�ģʽ."));
    }

    limits = ship->def_limits;

    /* Make sure the player qualifies for legit mode... */
    for (j = 0; j < c->pl->v1.character.inv.item_count; ++j) {
        item = (iitem_t*)&c->pl->v1.character.inv.iitems[j];
        irv = psocn_limits_check_item(limits, item, v);

        if (!irv) {
            SHIPS_LOG("Potentially non-legit item in legit mode:\n"
                "%08x %08x %08x %08x", LE32(item->data.datal[0]),
                LE32(item->data.datal[1]), LE32(item->data.datal[2]),
                LE32(item->data.data2l));
            pthread_rwlock_unlock(&ship->llock);
            return send_txt(c, "%s", __(c, "\tE\tC7��ĺϷ�ģʽ���ʧ��."));
        }
    }

    /* Set the flag and retain the limits list on the client. */
    c->flags |= CLIENT_FLAG_LEGIT;
    c->limits = retain(limits);

    pthread_rwlock_unlock(&ship->llock);

    return send_txt(c, "%s", __(c, "\tE\tC7��һ���½�����Ϸ�ģʽ������Ч."));
}

/* �÷�: /restorebk */
static int handle_restorebk(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Make sure that the requester is in a lobby, not a team */
    if (l->type != LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7�޷�����Ϸ������ʹ��."));
    }

    c->game_info.guildcard = c->guildcard;
    c->game_info.slot = c->sec_data.slot;
    c->game_info.block = c->cur_block->b;
    c->game_info.c_version = c->version;

    /* Not valid for Blue Burst clients */
    if (c->version == CLIENT_VERSION_BB) {
        //return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst ��֧�ָ�ָ��."));
        /* Send the request to the shipgate. */
        char tmp_name[32] = { 0 };
        istrncpy16_raw(ic_utf16_to_utf8, tmp_name, (char*)&c->pl->bb.character.name.char_name, 32, 10);

        strncpy((char*)c->game_info.name, tmp_name, sizeof(c->game_info.name));
        c->game_info.name[31] = 0;

    }
    else {
        strncpy((char*)c->game_info.name, c->pl->v1.character.dress_data.gc_string, sizeof(c->game_info.name));
        c->game_info.name[31] = 0;
    }

    if (shipgate_send_cbkup_req(&ship->sg, &c->game_info)) {
        /* Send a message saying we couldn't request */
        return send_txt(c, "%s",
            __(c, "\tE\tC7�޷�����ָ���ɫ��������."));
    }

    return send_txt(c, "%s",
        __(c, "\tE\tC7�ָ���ɫ�������ݳɹ�\n�л����ֺ���Ч."));

    return 0;
}

/* �÷� /enablebk */
static int handle_enablebk(ship_client_t* c, const char* params) {
    uint8_t enable = 1;

    /* Make sure the user is logged in */
    if (!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC4����Ҫ��¼"
            "�ſ���ʹ�ø�ָ��."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
        USER_OPT_ENABLE_BACKUP, 1, &enable);
    c->flags |= CLIENT_FLAG_AUTO_BACKUP;
    c->game_data->auto_backup = true;
    return send_txt(c, "%s", __(c, "\tE\tC6��ɫ�Զ������ѿ���."));
}

/* �÷� /disablebk */
static int handle_disablebk(ship_client_t* c, const char* params) {
    uint8_t enable = 0;

    /* Make sure the user is logged in */
    if (!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7����Ҫ��¼"
            "�ſ���ʹ�ø�ָ��."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
        USER_OPT_ENABLE_BACKUP, 1, &enable);
    c->flags &= ~(CLIENT_FLAG_AUTO_BACKUP);
    c->game_data->auto_backup = false;
    return send_txt(c, "%s", __(c, "\tE\tC4��ɫ�Զ������ѹر�."));
}

/* �÷�: /noevent */
static int handle_noevent(ship_client_t* c, const char* params) {
    if (c->version < CLIENT_VERSION_GC)
        return send_txt(c, "%s", __(c, "\tE\tC7��Ϸ�汾��֧��."));

    /* Make sure that the requester is in a lobby, not a game */
    if (c->cur_lobby->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7�޷�����Ϸ������ʹ��."));

    return send_simple(c, LOBBY_EVENT_TYPE, LOBBY_EVENT_NONE);
}
