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

/* �÷�: /minlvl level */
static int handle_min_level(ship_client_t* c, const char* params) {
    int lvl;
    lobby_t* l = c->cur_lobby;

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is the leader of the team. */
    if (l->leader_id != c->client_id) {
        return send_txt(c, "%s",
            __(c, "\tE\tC7ֻ������Ϸ����ķ����ſ���ʹ��."));
    }

    /* Figure out the level requested */
    errno = 0;
    lvl = (int)strtoul(params, NULL, 10);

    if (errno || lvl > MAX_PLAYER_LEVEL || lvl < 1) {
        /* Send a message saying invalid level */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ȼ�."));
    }

    /* Make sure the requested level is greater than or equal to the value for
       the game's difficulty. */
    if (lvl < game_required_level[l->difficulty]) {
        return send_txt(c, "%s",
            __(c, "\tE\tC7Invalid level for this difficulty."));
    }

    /* Make sure the requested level is less than or equal to the game's maximum
       level. */
    if (lvl > (int)l->max_level) {
        return send_txt(c, "%s",
            __(c, "\tE\tC7Minimum level must be <= maximum."));
    }

    /* Set the value in the structure, and be on our way. */
    l->min_level = lvl;

    return send_txt(c, "%s", __(c, "\tE\tC7Minimum level set."));
}

/* �÷�: /maxlvl level */
static int handle_max_level(ship_client_t* c, const char* params) {
    int lvl;
    lobby_t* l = c->cur_lobby;

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is the leader of the team. */
    if (l->leader_id != c->client_id) {
        return send_txt(c, "%s",
            __(c, "\tE\tC7ֻ������Ϸ����ķ����ſ���ʹ��."));
    }

    /* Figure out the level requested */
    errno = 0;
    lvl = (int)strtoul(params, NULL, 10);

    if (errno || lvl > MAX_PLAYER_LEVEL || lvl < 1) {
        /* Send a message saying invalid level */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ȼ�."));
    }

    /* Make sure the requested level is greater than or equal to the value for
       the game's minimum level. */
    if (lvl < (int)l->min_level) {
        return send_txt(c, "%s",
            __(c, "\tE\tC7Maximum level must be >= minimum."));
    }

    /* Set the value in the structure, and be on our way. */
    l->max_level = lvl;

    return send_txt(c, "%s", __(c, "\tE\tC7Maximum level set."));
}

/* �÷� /arrow color_number */
static int handle_arrow(ship_client_t* c, const char* params) {
    int i;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Set the arrow color and send the packet to the lobby. */
    i = atoi(params);
    c->arrow_color = i;

    send_txt(c, "%s", __(c, "\tE\tC7��ͷ�����ɫ�Ѹı�."));

    return send_lobby_arrows(c->cur_lobby);
}

/* �÷�: /passwd newpass */
static int handle_passwd(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int len = strlen(params), i;

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is the leader of the team. */
    if (l->leader_id != c->client_id) {
        return send_txt(c, "%s",
            __(c, "\tE\tC7ֻ������Ϸ����ķ����ſ���ʹ��."));
    }

    /* Check the length of the provided password. */
    if (len > 16) {
        return send_txt(c, "%s", __(c, "\tE\tC7Password too long."));
    }

    /* Make sure the password only has ASCII characters */
    for (i = 0; i < len; ++i) {
        if (params[i] < 0x20 || params[i] >= 0x7F) {
            return send_txt(c, "%s",
                __(c, "\tE\tC7Illegal character in password."));
        }
    }

    pthread_mutex_lock(&l->mutex);

    /* Copy the new password in. */
    strcpy(l->passwd, params);

    pthread_mutex_unlock(&l->mutex);

    return send_txt(c, "%s", __(c, "\tE\tC7Password set."));
}

/* �÷�: /lname newname */
static int handle_lname(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is the leader of the team. */
    if (l->leader_id != c->client_id) {
        return send_txt(c, "%s",
            __(c, "\tE\tC7ֻ������Ϸ����ķ����ſ���ʹ��."));
    }

    /* Check the length of the provided lobby name. */
    if (strlen(params) > 16) {
        return send_txt(c, "%s", __(c, "\tE\tC7�������ƹ���,����������."));
    }

    pthread_mutex_lock(&l->mutex);

    /* Copy the new name in. */
    strcpy(l->name, params);

    pthread_mutex_unlock(&l->mutex);

    return send_txt(c, "%s\n%s", __(c, "\tE\tC7�����÷���������:"), l->name);
}

/* �÷�: /bug */
static int handle_bug(ship_client_t* c, const char* params) {
    subcmd_dc_gcsend_t gcpkt = { 0 };

    /* Forge a guildcard send packet. */
    gcpkt.hdr.pkt_type = GAME_SUBCMD62_TYPE;
    gcpkt.hdr.flags = c->client_id;
    gcpkt.hdr.pkt_len = LE16(0x88);

    gcpkt.shdr.type = SUBCMD62_GUILDCARD;
    gcpkt.shdr.size = 0x21;
    gcpkt.shdr.unused = 0x0000;

    gcpkt.player_tag = LE32(0x00010000);
    gcpkt.guildcard = LE32(BUG_REPORT_GC);
    gcpkt.unused2 = 0;
    gcpkt.disable_udp = 1;
    gcpkt.language = CLIENT_LANG_CHINESE_S;
    gcpkt.section = 0;
    gcpkt.ch_class = 8;
    gcpkt.padding[0] = gcpkt.padding[1] = gcpkt.padding[2] = 0;

    sprintf(gcpkt.name, "%s", __(c, "���󱨸�"));
    sprintf(gcpkt.guildcard_desc, "%s", __(c, "���GC�ϱ�һ�����󱨸�."));

    send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7�����ʼ���\n"
        "'���󱨸�' ��Ҳ��ϱ�\n"
        "һ��BUG."));

    return sub62_06_dc(NULL, c, &gcpkt);
}

/* �÷�: /gban:d guildcard reason */
static int handle_gban_d(ship_client_t* c, const char* params) {
    uint32_t gc;
    char* reason;

    /* Make sure the requester is a global GM. */
    if (!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Set the ban (86,400s = 1 day). */
    if (strlen(reason) > 1) {
        return global_ban(c, gc, 86400, reason + 1);
    }
    else {
        return global_ban(c, gc, 86400, NULL);
    }
}

/* �÷�: /gban:w guildcard reason */
static int handle_gban_w(ship_client_t* c, const char* params) {
    uint32_t gc;
    char* reason;

    /* Make sure the requester is a global GM. */
    if (!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Set the ban (604,800s = 1 week). */
    if (strlen(reason) > 1) {
        return global_ban(c, gc, 604800, reason + 1);
    }
    else {
        return global_ban(c, gc, 604800, NULL);
    }
}

/* �÷�: /gban:m guildcard reason */
static int handle_gban_m(ship_client_t* c, const char* params) {
    uint32_t gc;
    char* reason;

    /* Make sure the requester is a global GM. */
    if (!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Set the ban (2,592,000s = 30 days). */
    if (strlen(reason) > 1) {
        return global_ban(c, gc, 2592000, reason + 1);
    }
    else {
        return global_ban(c, gc, 2592000, NULL);
    }
}

/* �÷�: /gban:p guildcard reason */
static int handle_gban_p(ship_client_t* c, const char* params) {
    uint32_t gc;
    char* reason;

    /* Make sure the requester is a global GM. */
    if (!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Set the ban (0xFFFFFFFF = forever (or close enough for now)). */
    if (strlen(reason) > 1) {
        return global_ban(c, gc, EMPTY_STRING, reason + 1);
    }
    else {
        return global_ban(c, gc, EMPTY_STRING, NULL);
    }
}

/* �÷�: /normal */
static int handle_normal(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int i;

    /* Lock the lobby mutex... we've got some work to do. */
    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is the leader of the team. */
    if (l->leader_id != c->client_id) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s",
            __(c, "\tE\tC7ֻ������Ϸ����ķ����ſ���ʹ��."));
    }

    /* If we're not in legit mode, then this command doesn't do anything... */
    if (!(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7�Ѿ��ǺϷ�ģʽ,�޷�ʹ������ģʽ."));
    }

    /* Clear the flag */
    l->flags &= ~(LOBBY_FLAG_LEGIT_MODE);

    /* Let everyone know legit mode has been turned off. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]) {
            send_txt(l->clients[i], "%s",
                __(l->clients[i], "\tE\tC7�Ϸ�ģʽ�ѹر�."));
        }
    }

    /* Unlock, we're done. */
    pthread_mutex_unlock(&l->mutex);

    return 0;
}

/* �÷�: /log guildcard */
static int handle_log(ship_client_t* c, const char* params) {
    uint32_t gc;
    block_t* b = c->cur_block;
    ship_client_t* i;
    int rv;

    /* Make sure the requester is a local root. */
    if (!LOCAL_ROOT(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Look for the requested user and start the log */
    TAILQ_FOREACH(i, b->clients, qentry) {
        /* Start logging them if we find them */
        if (i->guildcard == gc) {
            rv = pkt_log_start(i);

            if (!rv) {
                return send_txt(c, "%s", __(c, "\tE\tC7Logging started."));
            }
            else if (rv == -1) {
                return send_txt(c, "%s", __(c, "\tE\tC7The user is already\n"
                    "being logged."));
            }
            else if (rv == -2) {
                return send_txt(c, "%s",
                    __(c, "\tE\tC7Cannot create log file."));
            }
        }
    }

    /* The person isn't here... There's nothing left to do. */
    return send_txt(c, "%s", __(c, "\tE\tC7Requested user not\nfound."));
}

/* �÷�: /endlog guildcard */
static int handle_endlog(ship_client_t* c, const char* params) {
    uint32_t gc;
    block_t* b = c->cur_block;
    ship_client_t* i;
    int rv;

    /* Make sure the requester is a local root. */
    if (!LOCAL_ROOT(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Look for the requested user and end the log */
    TAILQ_FOREACH(i, b->clients, qentry) {
        /* Finish logging them if we find them */
        if (i->guildcard == gc) {
            rv = pkt_log_stop(i);

            if (!rv) {
                return send_txt(c, "%s", __(c, "\tE\tC7Logging ended."));
            }
            else if (rv == -1) {
                return send_txt(c, "%s", __(c, "\tE\tC7The user is not\n"
                    "being logged."));
            }
        }
    }

    /* The person isn't here... There's nothing left to do. */
    return send_txt(c, "%s", __(c, "\tE\tC7Requested user not\nfound."));
}

/* �÷�: /motd */
static int handle_motd(ship_client_t* c, const char* params) {
    return send_motd(c);
}

/* �÷�: /friendadd guildcard nickname */
static int handle_friendadd(ship_client_t* c, const char* params) {
    uint32_t gc;
    char* nick;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &nick, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Make sure the nickname is valid. */
    if (!nick || nick[0] != ' ' || nick[1] == '\0') {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid Nickname."));
    }

    /* Send a request to the shipgate to do the rest */
    shipgate_send_friend_add(&ship->sg, c->guildcard, gc, nick + 1);

    /* Any further messages will be handled by the shipgate handler */
    return 0;
}

/* �÷�: /frienddel guildcard */
static int handle_frienddel(ship_client_t* c, const char* params) {
    uint32_t gc;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Send a request to the shipgate to do the rest */
    shipgate_send_friend_del(&ship->sg, c->guildcard, gc);

    /* Any further messages will be handled by the shipgate handler */
    return 0;
}

/* �÷�: /dconly [off] */
static int handle_dconly(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int i;

    /* Lock the lobby mutex... we've got some work to do. */
    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is the leader of the team. */
    if (l->leader_id != c->client_id) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s",
            __(c, "\tE\tC7ֻ������Ϸ����ķ����ſ���ʹ��."));
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        l->flags &= ~LOBBY_FLAG_DCONLY;
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7Dreamcast-only mode off."));
    }

    /* Check to see if all players are on a Dreamcast version */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]) {
            if (l->clients[i]->version != CLIENT_VERSION_DCV1 &&
                l->clients[i]->version != CLIENT_VERSION_DCV2) {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(c, "%s", __(c, "\tE\tC7At least one "
                    "non-Dreamcast player is in the "
                    "game."));
            }
        }
    }

    /* We passed the check, set the flag and unlock the lobby. */
    l->flags |= LOBBY_FLAG_DCONLY;
    pthread_mutex_unlock(&l->mutex);

    /* Tell the leader that the command has been activated. */
    return send_txt(c, "%s", __(c, "\tE\tC7Dreamcast-only mode on."));
}

/* �÷�: /v1only [off] */
static int handle_v1only(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int i;

    /* Lock the lobby mutex... we've got some work to do. */
    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* Make sure the requester is the leader of the team. */
    if (l->leader_id != c->client_id) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s",
            __(c, "\tE\tC7ֻ������Ϸ����ķ����ſ���ʹ��."));
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        l->flags &= ~LOBBY_FLAG_V1ONLY;
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7V1-only mode off."));
    }

    /* Check to see if all players are on V1 */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]) {
            if (l->clients[i]->version != CLIENT_VERSION_DCV1) {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(c, "%s", __(c, "\tE\tC7At least one "
                    "non-PSOv1 player is in the "
                    "game."));
            }
        }
    }

    /* We passed the check, set the flag and unlock the lobby. */
    l->flags |= LOBBY_FLAG_V1ONLY;
    pthread_mutex_unlock(&l->mutex);

    /* Tell the leader that the command has been activated. */
    return send_txt(c, "%s", __(c, "\tE\tC7V1-only mode on."));
}

/* �÷�: /ll */
static int handle_ll(ship_client_t* c, const char* params) {
    char str[512] = { 0 };
    lobby_t* l = c->cur_lobby;
    int i;
    ship_client_t* c2;
    size_t len;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    str[0] = '\t';
    str[1] = 'E';
    len = 2;

    if (LOCAL_GM(c)) {
        for (i = 0; i < l->max_clients; i += 2) {
            if ((c2 = l->clients[i])) {
                len += snprintf(str + len, 511 - len, "%d: %s (%" PRIu32 ")   ",
                    i, c2->pl->v1.character.dress_data.gc_string, c2->guildcard);
            }
            else {
                len += snprintf(str + len, 511 - len, "%d: None   ", i);
            }

            if ((i + 1) < l->max_clients) {
                if ((c2 = l->clients[i + 1])) {
                    len += snprintf(str + len, 511 - len, "%d: %s (%" PRIu32
                        ")\n", i + 1, c2->pl->v1.character.dress_data.gc_string,
                        c2->guildcard);
                }
                else {
                    len += snprintf(str + len, 511 - len, "%d: None\n", i + 1);
                }
            }
        }
    }
    else {
        for (i = 0; i < l->max_clients; i += 2) {
            if ((c2 = l->clients[i])) {
                len += snprintf(str + len, 511 - len, "%d: %s   ", i,
                    c2->pl->v1.character.dress_data.gc_string);
            }
            else {
                len += snprintf(str + len, 511 - len, "%d: None   ", i);
            }

            if ((i + 1) < l->max_clients) {
                if ((c2 = l->clients[i + 1])) {
                    len += snprintf(str + len, 511 - len, "%d: %s\n", i + 1,
                        c2->pl->v1.character.dress_data.gc_string);
                }
                else {
                    len += snprintf(str + len, 511 - len, "%d: None\n", i + 1);
                }
            }
        }
    }

    /* Make sure the string is terminated properly. */
    str[511] = '\0';

    /* �����ݰ����ͳ�ȥ */
    return send_msg(c, MSG_BOX_TYPE, "%s", str);
}

/* �÷� /npc number,client_id,follow_id */
static int handle_npc(ship_client_t* c, const char* params) {
    int count, npcnum, client_id, follow;
    uint8_t tmp[0x10] = { 0 };
    subcmd_pkt_t* p = (subcmd_pkt_t*)tmp;
    lobby_t* l = c->cur_lobby;

    pthread_mutex_lock(&l->mutex);

    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* For now, limit only to lobbies with a single person in them... */
    if (l->num_clients != 1) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ�ڵ�����Ϸ��������Ч."));
    }

    /* Also, make sure its not a battle or challenge lobby. */
    if (l->battle || l->challenge) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7��սģʽ����սģʽ�޷�ʹ��."));
    }

    /* Make sure we're not in legit mode. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7�Ϸ�ģʽ���޷���Ч."));
    }

    /* Make sure we're not in a quest. */
    if ((l->flags & LOBBY_FLAG_QUESTING)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7������ʱ�޷�ʹ��."));
    }

    /* Make sure we're on Pioneer 2. */
    if (c->cur_area != 0) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7ֻ����������2����ʹ��."));
    }

    /* Figure out what we're supposed to do. */
    count = sscanf(params, "%d,%d,%d", &npcnum, &client_id, &follow);

    if (count == EOF || count == 0) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч NPC ����ID.\n�÷� /npc number,client_id,follow_id"));
    }

    /* Fill in sane defaults. */
    if (count == 1) {
        /* Find an open slot... */
        for (client_id = l->max_clients - 1; client_id >= 0; --client_id) {
            if (!l->clients[client_id]) {
                break;
            }
        }

        follow = c->client_id;
    }
    else if (count == 2) {
        follow = c->client_id;
    }

    /* Check the validity of arguments */
    if (npcnum < 0 || npcnum > 63) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч NPC ���."));
    }

    if (client_id < 0 || client_id >= l->max_clients || l->clients[client_id]) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�ͻ��� ID,\n"
            "��δ�����Ŀͻ��˲�λ."));
    }

    if (follow < 0 || follow >= l->max_clients || !l->clients[follow]) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч����ͻ���ID."));
    }

    /* This command, for now anyway, locks us down to one player mode. */
    l->flags |= LOBBY_FLAG_SINGLEPLAYER | LOBBY_FLAG_HAS_NPC;

    /* We're done with the lobby data now... */
    pthread_mutex_unlock(&l->mutex);

    /* �������ͷ */
    memset(p, 0, 0x10);
    p->hdr.dc.pkt_type = GAME_SUBCMD60_TYPE;
    p->hdr.dc.pkt_len = LE16(0x0010);
    p->type = SUBCMD60_SPAWN_NPC;
    p->size = 0x03;

    tmp[6] = 0x01;
    tmp[7] = 0x01;
    tmp[8] = follow;
    tmp[10] = client_id;
    tmp[14] = npcnum;

    /* Send the packet to everyone, including the person sending the request. */
    send_pkt_dc(c, (dc_pkt_hdr_t*)p);
    return subcmd_handle_60(c, p);
}

/* �÷�: /ignore client_id */
static int handle_ignore(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    int id, i;
    ship_client_t* cl;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Copy over the ID. */
    i = sscanf(params, "%d", &id);

    if (i == EOF || i == 0 || id >= l->max_clients || id < 0) {
        return send_txt(c, "%s", __(c, "\tE\tC4��Ч�ͻ��� ID ����."));
    }

    /* Lock the lobby so we don't mess anything up in grabbing this... */
    pthread_mutex_lock(&l->mutex);

    /* Make sure there is such a client. */
    if (!(cl = l->clients[id])) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC4δ�ҵ��ÿͻ���."));
    }

    /* Find an empty spot to put this in. */
    for (i = 0; i < CLIENT_IGNORE_LIST_SIZE; ++i) {
        if (!c->ignore_list[i]) {
            c->ignore_list[i] = cl->guildcard;
            pthread_mutex_unlock(&l->mutex);
            return send_txt(c, "%s %s\n%s %d", __(c, "\tE\tC7�Ѻ���"),
                cl->pl->v1.character.dress_data.gc_string, __(c, "Entry"), i);
        }
    }

    /* ������ǵ�������, the ignore list is full, report that to the user... */
    pthread_mutex_unlock(&l->mutex);
    return send_txt(c, "%s", __(c, "\tE\tC7�����б�����."));
}

/* �÷�: /unignore entry_number */
static int handle_unignore(ship_client_t* c, const char* params) {
    int id, i;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Copy over the ID */
    i = sscanf(params, "%d", &id);

    if (i == EOF || i == 0 || id >= CLIENT_IGNORE_LIST_SIZE || id < 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid Entry Number."));
    }

    /* Clear that entry of the ignore list */
    c->ignore_list[id] = 0;

    return send_txt(c, "%s", __(c, "\tE\tC7Ignore list entry cleared."));
}

/* �÷�: /quit */
static int handle_quit(ship_client_t* c, const char* params) {
    c->flags |= CLIENT_FLAG_DISCONNECTED;
    return 0;
}

/* �÷�: /cc [any ascii char] or /cc off */
static int handle_cc(ship_client_t* c, const char* params) {
    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Are we turning it off? */
    if (!strcmp(params, "off")) {
        c->cc_char = 0;
        return send_txt(c, "%s", __(c, "\tE\tC4��ɫ����ر�."));
    }

    /* Make sure they only gave one character */
    if (strlen(params) != 1) {
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�Ĵ������ַ�."));
    }

    /* Set the char in the client struct */
    c->cc_char = params[0];
    return send_txt(c, "%s", __(c, "\tE\tCG��ɫ���쿪��."));
}

/* �÷�: /qlang [2 character language code] */
static int handle_qlang(ship_client_t* c, const char* params) {
    int i;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Make sure they only gave one character */
    if (strlen(params) != 2) {
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч���Դ���."));
    }

    /* Look for the specified language code */
    for (i = 0; i < CLIENT_LANG_ALL; ++i) {
        if (!strcmp(language_codes[i], params)) {
            c->q_lang = i;

#ifdef DEBUG

            DBG_LOG("���� %d %d", c->q_lang, language_codes[i]);

#endif // DEBUG

            shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
                USER_OPT_QUEST_LANG, 1, &c->q_lang);

            return send_txt(c, "%s", __(c, "\tE\tC7��������������."));
        }
    }

    return send_txt(c, "%s", __(c, "\tE\tC7��Ч���Դ���."));
}

/* �÷�: /friends page */
static int handle_friends(ship_client_t* c, const char* params) {
    block_t* b = c->cur_block;
    uint32_t page;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Figure out the user requested */
    errno = 0;
    page = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid page number */
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid Page."));
    }

    /* Send the request to the shipgate */
    return shipgate_send_frlist_req(&ship->sg, c->guildcard, b->b, page * 5);
}

/* �÷�: /ver */
static int handle_ver(ship_client_t* c, const char* params) {
    return send_txt(c, "%s: %s\n%s: %s", __(c, "\tE\tC7Git Build"),
        "1", __(c, "Changeset"), "1");
}

/* �÷�: /restart minutes */
static int handle_restart(ship_client_t* c, const char* params) {
    uint32_t when;

    /* Make sure the requester is a local root. */
    if (!LOCAL_ROOT(c)) {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7Ȩ�޲���."));
    }

    /* Figure out when we're supposed to shut down. */
    errno = 0;
    when = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid time */
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7ʱ�������Ч."));
    }

    /* Give everyone at least a minute */
    if (when < 1) {
        when = 1;
    }

    return schedule_shutdown(c, when, 1, send_msg);
}

/* �÷�: /search guildcard */
static int handle_search(ship_client_t* c, const char* params) {
    uint32_t gc;
    dc_guild_search_pkt dc = { 0 };
    bb_guild_search_pkt bb = { 0 };

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7��Ч GC ���."));
    }

    /* Hacky? Maybe a bit, but this will work just fine. */
    if (c->version == CLIENT_VERSION_BB) {
        bb.hdr.pkt_len = LE16(0x14);
        bb.hdr.pkt_type = LE16(GUILD_SEARCH_TYPE);
        bb.hdr.flags = 0;
        bb.player_tag = LE32(0x00010000);
        bb.gc_searcher = LE32(c->guildcard);
        bb.gc_target = LE32(gc);

        return block_process_pkt(c, (uint8_t*)&bb);
    }
    else if (c->version == CLIENT_VERSION_PC) {
        dc.hdr.pc.pkt_len = LE16(0x10);
        dc.hdr.pc.pkt_type = GUILD_SEARCH_TYPE;
        dc.hdr.pc.flags = 0;
        dc.player_tag = LE32(0x00010000);
        dc.gc_searcher = LE32(c->guildcard);
        dc.gc_target = LE32(gc);

        return block_process_pkt(c, (uint8_t*)&dc);
    }
    else {
        dc.hdr.dc.pkt_len = LE16(0x10);
        dc.hdr.dc.pkt_type = GUILD_SEARCH_TYPE;
        dc.hdr.dc.flags = 0;
        dc.player_tag = LE32(0x00010000);
        dc.gc_searcher = LE32(c->guildcard);
        dc.gc_target = LE32(gc);

        return block_process_pkt(c, (uint8_t*)&dc);
    }
}

/* �÷�: /showmaps */
static int handle_showmaps(ship_client_t* c, const char* params) {
    char string[33] = { 0 };
    int i;
    lobby_t* l = c->cur_lobby;

    pthread_mutex_lock(&l->mutex);

    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ����Ϸ��������Ч."));
    }

    /* Build the string from the map list */
    for (i = 0; i < 32; ++i) {
        string[i] = l->maps[i] + '0';
    }

    string[32] = 0;

    pthread_mutex_unlock(&l->mutex);
    return send_txt(c, "%s\n%s", __(c, "\tE\tC7��ǰ��ͼ:"), string);
}

/* �÷�: /gcprotect [off] */
static int handle_gcprotect(ship_client_t* c, const char* params) {
    uint8_t enable = 1;

    /* Make sure they're logged in */
    if (!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7����Ҫ��¼"
            "�ſ���ʹ�ø�ָ��."));
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        enable = 0;
        shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
            USER_OPT_GC_PROTECT, 1, &enable);
        return send_txt(c, "%s", __(c, "\tE\tC7Guildcard protection "
            "disabled."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
        USER_OPT_GC_PROTECT, 1, &enable);
    return send_txt(c, "%s", __(c, "\tE\tC7Guildcard protection enabled."));
}

/* �÷� /trackkill [on/off] */
static int handle_trackkill(ship_client_t* c, const char* params) {
    uint8_t enable = 1;

    /* Make sure they're logged in */
    if (!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7����Ҫ��¼"
            "�ſ���ʹ�ø�ָ��."));
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        enable = 0;
        shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
            USER_OPT_TRACK_KILLS, 1, &enable);
        c->flags &= ~CLIENT_FLAG_TRACK_KILLS;
        return send_txt(c, "%s", __(c, "\tE\tC7ɱ���������ѹر�."));
    }
    else if (!strcmp(params, "on")) {
        /* Send the message to the shipgate */
        shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
            USER_OPT_TRACK_KILLS, 1, &enable);
        c->flags |= CLIENT_FLAG_TRACK_KILLS;
        return send_txt(c, "%s", __(c, "\tE\tC7������ɱ��������."));
    }

    return send_txt(c, "%s", __(c, "\tE\tC4��������,/trackkill [on/off]."));
}

/* �÷�: /tlogin username token */
static int handle_tlogin(ship_client_t* c, const char* params) {
    char username[32] = { 0 }, token[32] = { 0 };
    int len = 0;
    const char* ch = params;

    /* Make sure the user isn't doing something stupid. */
    if (!*params) {
        return send_txt(c, "%s", __(c, "\tE\tC7You must specify\n"
            "your username and\n"
            "website token."));
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
        token[len++] = *ch++;
    }

    if (len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid token."));

    token[len] = '\0';

    /* We'll get success/failure later from the shipgate. */
    return shipgate_send_usrlogin(&ship->sg, c->guildcard, c->cur_block->b,
        username, token, 1, 0);
}

/* �÷�: /showpos */
static int handle_showpos(ship_client_t* c, const char* params) {
    return send_txt(c, "\tE\tC7(%f, %f, %f)", c->x, c->y, c->z);
}

/* �÷�: /info */
static int handle_info(ship_client_t* c, const char* params) {
    /* Don't let certain versions even try... */
    if (c->version == CLIENT_VERSION_EP3)
        return send_txt(c, "%s", __(c, "\tE\tC7Episode III ��֧�ָ�ָ��."));
    else if (c->version == CLIENT_VERSION_DCV1 &&
        (c->flags & CLIENT_FLAG_IS_NTE))
        return send_txt(c, "%s", __(c, "\tE\tC7DC NTE ��֧�ָ�ָ��."));
    else if ((c->version == CLIENT_VERSION_GC ||
        c->version == CLIENT_VERSION_XBOX) &&
        !(c->flags & CLIENT_FLAG_GC_MSG_BOXES))
        return send_txt(c, "%s", __(c, "\tE\tC7��Ϸ�汾��֧��."));

    return send_info_list(c, ship);
}

/* �÷� /autolegit [off] */
static int handle_autolegit(ship_client_t* c, const char* params) {
    uint8_t enable = 1;
    psocn_limits_t* limits;

    /* Make sure they're logged in */
    if (!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7����Ҫ��¼"
            "�ſ���ʹ�ø�ָ��."));
    }

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        enable = 0;
        shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
            USER_OPT_LEGIT_ALWAYS, 1, &enable);
        c->flags &= ~CLIENT_FLAG_ALWAYS_LEGIT;
        return send_txt(c, "%s", __(c, "\tE\tC7Automatic legit\n"
            "mode disabled."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
        USER_OPT_LEGIT_ALWAYS, 1, &enable);
    c->flags |= CLIENT_FLAG_ALWAYS_LEGIT;
    send_txt(c, "%s", __(c, "\tE\tC7Automatic legit\nmode enabled."));

    /* Check them now, since they shouldn't have to re-connect to get the
       flag set... */
    pthread_rwlock_rdlock(&ship->llock);
    if (!ship->def_limits) {
        pthread_rwlock_unlock(&ship->llock);
        c->flags &= ~CLIENT_FLAG_ALWAYS_LEGIT;
        send_txt(c, "%s", __(c, "\tE\tC7Legit mode not\n"
            "available on this\n"
            "ship."));
        return 0;
    }

    limits = ship->def_limits;

    if (client_legit_check(c, limits)) {
        c->flags &= ~CLIENT_FLAG_ALWAYS_LEGIT;
        send_txt(c, "%s", __(c, "\tE\tC7You failed the legit "
            "check."));
    }
    else {
        /* Set the flag and retain the limits list on the client. */
        c->flags |= CLIENT_FLAG_LEGIT;
        c->limits = retain(limits);
        send_txt(c, "%s", __(c, "\tE\tC7Legit check passed."));
    }

    pthread_rwlock_unlock(&ship->llock);
    return 0;
}

/* �÷�: /censor [off] */
static int handle_censor(ship_client_t* c, const char* params) {
    uint8_t enable = 1;
    pthread_mutex_lock(&c->mutex);

    /* See if we're turning the flag off. */
    if (!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_WORD_CENSOR;
        enable = 0;
        shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
            USER_OPT_WORD_CENSOR, 1, &enable);

        pthread_mutex_unlock(&c->mutex);

        return send_txt(c, "%s", __(c, "\tE\tC7Word censor off."));
    }

    /* Set the flag since we're turning it on. */
    c->flags |= CLIENT_FLAG_WORD_CENSOR;

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
        USER_OPT_WORD_CENSOR, 1, &enable);

    pthread_mutex_unlock(&c->mutex);
    return send_txt(c, "%s", __(c, "\tE\tC7Word censor on."));
}

/* �÷�: /xblink username token */
static int handle_xblink(ship_client_t* c, const char* params) {
    char username[32] = { 0 }, token[32] = { 0 };
    int len = 0;
    const char* ch = params;

    if (c->version != CLIENT_VERSION_XBOX)
        return send_txt(c, "%s", __(c, "\tE\tC7��Ϸ�汾��֧��."));

    /* Make sure the user isn't doing something stupid. */
    if (!*params) {
        return send_txt(c, "%s", __(c, "\tE\tC7You must specify\n"
            "your username and\n"
            "website token."));
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
        token[len++] = *ch++;
    }

    if (len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid token."));

    token[len] = '\0';

    /* We'll get success/failure later from the shipgate. */
    return shipgate_send_usrlogin(&ship->sg, c->guildcard, c->cur_block->b,
        username, token, 1, TLOGIN_VER_XBOX);
}

/* �÷�: /logme [off] */
static int handle_logme(ship_client_t* c, const char* params) {
#ifndef DEBUG
    return send_txt(c, "%s", __(c, "\tE\tC7��Чָ��."));
#else
    /* Make sure the requester has permission to do this */
    if (!IS_TESTER(c))
        return get_gm_priv(c);

    if (*params) {
        if (!strcmp(params, "off")) {
            pkt_log_stop(c);
            return send_txt(c, "%s", __(c, "\tE\tC7DEBUG��־����."));
        }
        else {
            return send_txt(c, "%s", __(c, "\tE\tC7��ЧLOG����."));
        }
    }
    else {
        pkt_log_start(c);
        return send_txt(c, "%s", __(c, "\tE\tC7DEBUG��־��ʼ."));
    }
#endif /* DEBUG */
}

/* �÷�: /clean [inv/bank/quest1/quest2] ������������ ����*/
static int handle_clean(ship_client_t* c, const char* params) {

    if (c->cur_lobby->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC4ֻ�����ڴ���ʹ��."));

    if (c->version != CLIENT_VERSION_BB)
        return send_txt(c, "%s", __(c, "\tE\tC4��Ϸ�汾��֧��."));

    if (*params) {
        if (!strcmp(params, "inv")) {

            int size = sizeof(c->bb_pl->character.inv.iitems) / PSOCN_STLENGTH_IITEM;

            for (int i = 0; i < size; i++) {
                memset(&c->bb_pl->character.inv.iitems[i], 0, PSOCN_STLENGTH_IITEM);
            }

            c->bb_pl->character.inv.item_count = 0;
            c->pl->bb.character.inv = c->bb_pl->character.inv;

            send_txt(c, "%s", __(c, "\tE\tC4�������������."));
            return send_block_list(c, ship);
        }

        if (!strcmp(params, "bank")) {

            int size = sizeof(c->bb_pl->bank.bitems) / PSOCN_STLENGTH_BITEM;

            for (int i = 0; i < size; i++) {
                memset(&c->bb_pl->bank.bitems[i], 0, PSOCN_STLENGTH_IITEM);
            }

            c->bb_pl->bank.item_count = 0;
            c->bb_pl->bank.meseta = 0;

            send_txt(c, "%s", __(c, "\tE\tC4�������������."));
            return send_block_list(c, ship);
        }

        if (!strcmp(params, "quest1")) {

            for (int i = 0; i < PSOCN_STLENGTH_BB_DB_QUEST_DATA1; i++) {
                c->bb_pl->quest_data1[i] = 0x00;
            }
            send_txt(c, "%s", __(c, "\tE\tC4����1���������."));
            return send_block_list(c, ship);
        }

        if (!strcmp(params, "quest2")) {

            for (int i = 0; i < PSOCN_STLENGTH_BB_DB_QUEST_DATA2; i++) {
                c->bb_pl->quest_data2[i] = 0x00;
            }

            send_txt(c, "%s", __(c, "\tE\tC4����2���������."));
            return send_block_list(c, ship);
        }

        return send_txt(c, "%s", __(c, "\tE\tC7��Ч�������."));
    }

    return send_txt(c, "%s", __(c, "\tE\tC6���ָ���ȷ\n����Ϊ[inv/bank/quest1/quest2]."));
}

/* �÷�: /bank */
static int handle_bank(ship_client_t* c, const char* params) {

    if (!c->bank_type) {
        c->bank_type = true;
        return send_txt(c, "%s", __(c, "\tE\tC8�����ֿ�."));
    }

    c->bank_type = false;
    return send_txt(c, "%s", __(c, "\tE\tC6��ɫ�ֿ�."));
}

/* �÷�: /rshop */
static int handle_rshop(ship_client_t* c, const char* params) {

    if(get_player_level(c) + 1 < MAX_PLAYER_LEVEL)
        return send_txt(c, "%s", __(c, "\tE\tC4����������޷�ʹ��."));

    if (!c->game_data->random_shop) {
        c->game_data->random_shop = 1;
        return send_txt(c, "%s", __(c, "\tE\tC6����̵��ѿ���."));
    }

    c->game_data->random_shop = 0;
    return send_txt(c, "%s", __(c, "\tE\tC7����̵��ѹر�."));
}

/* �÷�: /qr */
static int handle_quick_return(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    if (l->flags & LOBBY_FLAG_QUESTING) {
        return send_txt(c, "%s", __(c, "\tE\tC4�޷���������ʹ�ø�ָ��."));
    }
    else if (l->type == LOBBY_TYPE_GAME) {
        if (c->cur_area > 0)
            return send_warp(c, 0, true);
        else
            return send_txt(c, "%s", __(c, "\tE\tC6���Ѿ���������2����."));
    }

    send_ship_list(c, ship, ship->cfg->menu_code);
    return send_txt(c, "%s", __(c, "\tE\tC4��ѡ����Ҫǰ���Ľ���."));
}

/* �÷�: /lb */
static int handle_lobby(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_GAME) {
        c->game_data->lobby_return = true;
        return send_warp(c, 0, false);
    }

    send_block_list(c, ship);
    return send_txt(c, "%s", __(c, "\tE\tC4��ѡ����Ҫǰ���Ĵ���."));
}

/* �÷�: /matuse */
static int handle_matuse(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    psocn_bb_char_t* character = get_client_char_bb(c);
    inventory_t* inv = &character->inv;

    return send_txt(c, "�ҩ���:\nHPҩ:%u TPҩ:%u \n��ҩ:%u ��ҩ:%u ��ҩ:%u \n��ҩ:%u ��ҩ:%u"
        , get_material_usage(inv, MATERIAL_HP)
        , get_material_usage(inv, MATERIAL_TP)
        , get_material_usage(inv, MATERIAL_POWER)
        , get_material_usage(inv, MATERIAL_MIND)
        , get_material_usage(inv, MATERIAL_EVADE)
        , get_material_usage(inv, MATERIAL_DEF)
        , get_material_usage(inv, MATERIAL_LUCK)
    );
}

/* �÷�: /npcskin 0 - 5*/
static int handle_npcskin(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    uint32_t skinid = 0;

    if (l->type == LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC4�޷�����Ϸ������ʹ��."));
    }

    /* Figure out the user requested */
    errno = 0;
    skinid = (uint32_t)strtoul(params, NULL, 10);

    if (errno != 0 || skinid > 5) {
        /* Send a message saying invalid page number */
        return send_txt(c, "%s", __(c, "\tE\tC4��ЧƤ������.(��Χ0 - 5, 0Ϊ��ɫƤ��)"));
    }

    if (skinid > 5) {
        skinid = 5;
    }

    if (skinid == 0) {
        c->bb_pl->character.dress_data.model = 0x00;
        c->bb_pl->character.dress_data.v2flags = 0x00;
    }
    else {
        c->bb_pl->character.dress_data.model = skinid - 1;
        c->bb_pl->character.dress_data.v2flags = 0x02;
    }

    send_block_list(c, ship);
    return send_txt(c, "Ƥ���л�Ϊ \tE\tC%d%s\n%s", skinid, npcskin_desc[skinid], __(c, "\tE\tC4���л�����������Ч."));
}

/* �÷�: /susi itemslot*/
static int handle_susi(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;
    size_t item_slot = 0;

    /* Figure out the user requested */
    errno = 0;
    item_slot = (size_t)strtoul(params, NULL, 10);

    if (errno != 0 || item_slot > MAX_PLAYER_INV_ITEMS) {
        /* Send a message saying invalid page number */
        return send_txt(c, "%s", __(c, "\tE\tC4��Ч��Ʒ��λ,��ұ������30����Ʒ"));
    }

    inventory_t* inv = get_player_inv(c);

    if (!item_slot) {
        char tmp_msg[4096] = { 0 };
        char data_str[100] = { 0 };

        for (size_t i = 0; i < inv->item_count; i++) {
            if (is_unsealable_item(&inv->iitems[i].data)) {
                // ��Ҫд������ݸ�ʽ��Ϊ�ַ���
                sprintf_s(data_str, sizeof(data_str), "%d.%s", i
                    , get_item_unsealable_describe(&inv->iitems[i].data, c->version)
                );

                // ����ʣ����ÿռ�
                size_t remaining_space = sizeof(tmp_msg) - strlen(tmp_msg);

                // ���ʣ��ռ��Ƿ��㹻
                if (strlen(data_str) + 1 > remaining_space) {  // +1 ��Ϊ�˿��ǽ�β�� null �ַ�
                    break;  // ֹͣ׷������
                }

                strcat_s(data_str, sizeof(data_str), " | ");

                // ����ʽ���������׷�ӵ� tmp_msg �ַ�������
                strcat_s(tmp_msg, sizeof(tmp_msg), data_str);
            }
        }

        return send_msg(c, BB_SCROLL_MSG_TYPE, "%s %s", __(c, "\tE\tC7δ�����Ʒ���:"), tmp_msg);
    }
    else {
        item_t* item = &inv->iitems[item_slot - 1].data;

        if (is_unsealable_item(item)) {
            return send_txt(c, "%s\n%s", __(c, "\tE\tC7δ�����Ʒ���:"), get_item_unsealable_describe(item, c->version));
        }

    }

    return send_txt(c, "\tE\tC4δ�ҵ� %d ��δ�����Ʒ", item_slot);
}
