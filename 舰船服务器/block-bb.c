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
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <errno.h>
#include <time.h>

#include <WinSock_Defines.h>
#include <psopipe.h>
#include <f_logs.h>
#include <f_iconv.h>
#include <psoconfig.h>
#include <mtwist.h>
#include <psomemory.h>
#include <pso_menu.h>
//#include <pso_version.h>

#include "block.h"
#include "ship.h"
#include "clients.h"
#include "lobby.h"
#include "ship_packets.h"
#include "utils.h"
#include "shipgate.h"
#include "commands.h"
#include "gm.h"
#include "subcmd.h"
#include "scripts.h"
#include "admin.h"
#include "smutdata.h"
#include "items.h"

extern int enable_ipv6;
extern uint32_t ship_ip4;
extern uint8_t ship_ip6[16];
extern time_t srv_time;

/* Process a chat packet from a Blue Burst client. */
static int bb_join_game(ship_client_t* c, lobby_t* l) {
    int rv;

    /* Make sure they don't have the protection flag on */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7请先登录\n才可加入游戏."));
        return LOBBY_FLAG_ERROR_ADD_CLIENT;
    }

    item_class_tag_equip_flag(c);

    /* See if they can change lobbies... */
    rv = lobby_change_lobby(c, l);

    if (rv == LOBBY_FLAG_ERROR_GAME_V1_CLASS) {
        /* HUcaseal, FOmar, or RAmarl trying to join a v1 game */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Your class is\nnot allowed in a\n"
                "PSOv1 game."));
    }
    if (rv == LOBBY_FLAG_ERROR_SINGLEPLAYER) {
        /* Single player mode */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\nin single player\nmode."));
    }
    else if (rv == LOBBY_FLAG_ERROR_PC_ONLY) {
        /* PC only */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\nfor PSOPC only."));
    }
    else if (rv == LOBBY_FLAG_ERROR_V1_ONLY) {
        /* V1 only */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\nfor PSOv1 only."));
    }
    else if (rv == LOBBY_FLAG_ERROR_DC_ONLY) {
        /* DC only */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\nfor PSODC only."));
    }
    else if (rv == LOBBY_FLAG_ERROR_LEGIT_TEMP_UNAVAIL) {
        /* Temporarily unavailable */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7The game is\ntemporarily\nunavailable."));
    }
    else if (rv == LOBBY_FLAG_ERROR_LEGIT_MODE) {
        /* Legit check failed */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Game mode is set\nto legit and you\n"
                "failed the legit\ncheck!"));
    }
    else if (rv == LOBBY_FLAG_ERROR_QUESTSEL) {
        /* Quest selection in progress */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Quest selection\nis in progress"));
    }
    else if (rv == LOBBY_FLAG_ERROR_QUESTING) {
        /* Questing in progress */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7A quest is in\nprogress."));
    }
    else if (rv == LOBBY_FLAG_ERROR_DCV2_ONLY) {
        /* V1 client attempting to join a V2 only game */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7This game is for\nVersion 2 only."));
    }
    else if (rv == LOBBY_FLAG_ERROR_MAX_LEVEL) {
        /* Level is too high */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Your level is\ntoo high."));
    }
    else if (rv == LOBBY_FLAG_ERROR_MIN_LEVEL) {
        /* Level is too high */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7Your level is\ntoo low."));
    }
    else if (rv == LOBBY_FLAG_ERROR_BURSTING) {
        /* A client is bursting. */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7玩家正在跃迁中"));
    }
    else if (rv == LOBBY_FLAG_ERROR_REMOVE_CLIENT) {
        /* The lobby has disappeared. */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7This game is\nnon-existant."));
    }
    else if (rv == LOBBY_FLAG_ERROR_ADD_CLIENT) {
        /* The lobby is full. */
        send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
            __(c, "\tC7游戏房间人数已满."));
    }
    else {
        /* Clear their legit mode flag... */
        if ((c->flags & CLIENT_FLAG_LEGIT)) {
            c->flags &= ~CLIENT_FLAG_LEGIT;
            if (c->limits)
                release(c->limits);
        }

        fix_up_pl_iitem(l, c);
    }

    c->game_info.guildcard = c->guildcard;
    c->game_info.slot = c->sec_data.slot;
    c->game_info.block = c->cur_block->b;
    c->game_info.c_version = c->version;

    strncpy((char*)c->game_info.name, c->pl->bb.character.disp.dress_data.guildcard_string, sizeof(c->game_info.name));
    c->game_info.name[31] = 0;

    /* 备份临时数据 TODO BB版本未完成 */
    if (c->version != CLIENT_VERSION_BB &&
        (c->flags & CLIENT_FLAG_AUTO_BACKUP)) {
        if (shipgate_send_cbkup(&ship->sg, &c->game_info, &c->pl->bb, sizeof(psocn_bb_db_char_t))) {
            DBG_LOG("备份临时数据 版本 %d", c->version);
            return rv;
        }
    }

    return rv;
}

static int bb_process_chat(ship_client_t* c, bb_chat_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    size_t len = LE16(pkt->hdr.pkt_len) - 16;
    size_t i;
    char* u8msg, * cmsg;

    /* 合理性检查... this shouldn't happen. */
    if (!l)
        return -1;

    /* 填充彩色聊天内容的转义符 */
    if (c->cc_char) {
        for (i = 0; i < len; i += 2) {
            /* Only accept it if it has a C right after, since that means we
               should have a color code... Also, make sure there's at least one
               character after the C, or we get junk... */
            if (pkt->msg[i] == c->cc_char && pkt->msg[i + 1] == '\0' &&
                pkt->msg[i + 2] == 'C' && pkt->msg[i + 3] == '\0' &&
                pkt->msg[i + 4] != '\0') {
                pkt->msg[i] = '\t';
            }
        }
    }

#ifndef DISABLE_CHAT_COMMANDS
    /* Check for commands. */
    if (pkt->msg[2] == LE16('/')) {
        return bbcommand_parse(c, pkt);
    }
#endif

    /* Convert it to UTF-8. */
    if (!(u8msg = (char*)malloc((len + 1) << 1)))
        return -1;

    memset(u8msg, 0, (len + 1) << 1);

    istrncpy16(ic_utf16_to_gbk, u8msg, pkt->msg, (len + 1) << 1);

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以聊天."));
    }

    cmsg = smutdata_censor_string(u8msg, SMUTDATA_BOTH);

    /* Send the message to the lobby. */
    i = send_lobby_chat(l, c, u8msg, cmsg);

    /* Clean up. */
    free_safe(cmsg);
    free_safe(u8msg);

    /* Send the message to the lobby. */
    //return send_lobby_bbchat(l, c, pkt->msg, len);
    return i;
}

static int bb_process_info_req(ship_client_t* c, bb_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);

    /* What kind of information do they want? */
    switch (menu_id & 0xFF) {
        /* Block */
    case MENU_ID_BLOCK:
        if (item_id == -1)
            return 0;

        return block_info_reply(c, item_id);

        /* Game List */
    case MENU_ID_GAME:
        return lobby_info_reply(c, item_id);

        /* Quest */
    case MENU_ID_QUEST:
    {
        int lang = (menu_id >> 24) & 0xFF;
        int rv;

        pthread_rwlock_rdlock(&ship->qlock);

        /* Do we have quests configured? */
        if (!TAILQ_EMPTY(&ship->qmap)) {
            rv = send_quest_info(c->cur_lobby, item_id, lang);
        }
        else {
            rv = send_msg1(c, "%s", __(c, "\tE\tC4任务未加载."));
        }

        pthread_rwlock_unlock(&ship->qlock);
        return rv;
    }

    /* Ship */
    case MENU_ID_SHIP:
    {
        miniship_t* i;

        /* Find the ship if its still online */
        TAILQ_FOREACH(i, &ship->ships, qentry) {
            if (i->ship_id == item_id) {
                char string[256];
                char tmp[3] = { (char)i->menu_code,
                    (char)(i->menu_code >> 8), 0 };

                sprintf(string, "%02x:%s%s%s\n%d %s\n%d %s", i->ship_number,
                    tmp, tmp[0] ? "/" : "", i->name, i->clients,
                    __(c, "玩家"), i->games, __(c, "房间"));
                return send_info_reply(c, string);
            }
        }

        return 0;
    }

    default:
        UNK_CPD(pkt->hdr.pkt_type, c->version, (uint8_t)pkt);
        return -1;
    }
}

static int bb_process_game_type(ship_client_t* c, uint32_t menu_id, uint32_t item_id) {
    lobby_t* l = c->create_lobby;

    if (l) {
        switch (item_id) {
        case 0:
            l->version = CLIENT_VERSION_BB;
            DBG_LOG("PSOBB 独享");
            break;

        case 1:
            DBG_LOG("允许所有版本");
            break;

        case 2:
            l->v2 = 0;
            l->version = CLIENT_VERSION_DCV1;
            DBG_LOG("允许PSO V1");
            break;

        case 3:
            DBG_LOG("允许PSO V2");
            break;

        case 4:
            DBG_LOG("允许PSO DC");
            break;

        case 5:
            l->flags |= LOBBY_FLAG_GC_ALLOWED;
            DBG_LOG("允许PSO GC");
            break;

        default:
            pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
            lobby_destroy(l);
            pthread_rwlock_unlock(&c->cur_block->lobby_lock);
            DBG_LOG("返回上一级");
            return send_bb_game_create(c);
        }

        //TODO 这里要判断类型

        /* All's well in the world if we get here. */
        return send_bb_game_game_drop_set(c);
    }

    return send_msg1(c, "%s", __(c, "\tE\tC4发生错误!请联系程序员!"));
}

static int bb_process_game_drop_set(ship_client_t* c, uint32_t menu_id, uint32_t item_id) {
    lobby_t* l = c->create_lobby;

    if (l) {
        switch (item_id) {
        case 0:
            //l->version = CLIENT_VERSION_BB;
            DBG_LOG("默认掉落模式");
            break;

        case 1:
            DBG_LOG("PSO2掉落模式");
            break;

        case 2:
            //l->v2 = 0;
            //l->version = CLIENT_VERSION_DCV1;
            DBG_LOG("随机掉落模式");
            break;

        default:
            DBG_LOG("返回上一级");
            return send_bb_game_type_sel(c);
        }

        c->create_lobby = NULL;

        /* Add the user to the lobby... */
        if (bb_join_game(c, l)) {
            /* Something broke, destroy the created lobby before anyone
               tries to join it. */
            pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
            lobby_destroy(l);
            pthread_rwlock_unlock(&c->cur_block->lobby_lock);
        }

        /* All's well in the world if we get here. */
        return 0;
    }

    return send_msg1(c, "%s", __(c, "\tE\tC4发生错误!请联系程序员!"));
}

static int bb_process_gm_menu(ship_client_t* c, uint32_t menu_id, uint32_t item_id) {
    if (!LOCAL_GM(c)) {
        return send_msg1(c, "%s", __(c, "\tE\tC7Nice try."));
    }

    switch (menu_id) {
    case MENU_ID_GM:
        switch (item_id) {
        case ITEM_ID_GM_REF_QUESTS:
            return refresh_quests(c, send_msg1);

        case ITEM_ID_GM_REF_GMS:
            return refresh_gms(c, send_msg1);

        case ITEM_ID_GM_REF_LIMITS:
            return refresh_limits(c, send_msg1);

        case ITEM_ID_GM_RESTART:
        case ITEM_ID_GM_SHUTDOWN:
        case ITEM_ID_GM_GAME_EVENT:
        case ITEM_ID_GM_LOBBY_EVENT:
            return send_gm_menu(c, MENU_ID_GM | (item_id << 8));
        }

        break;

    case MENU_ID_GM_SHUTDOWN:
    case MENU_ID_GM_RESTART:
        return schedule_shutdown(c, item_id, menu_id == MENU_ID_GM_RESTART,
            send_msg1);

    case MENU_ID_GM_GAME_EVENT:
        if (item_id < 7) {
            ship->game_event = item_id;
            return send_msg1(c, "%s", __(c, "\tE\tC7Game Event set."));
        }

        break;

    case MENU_ID_GM_LOBBY_EVENT:
        if (item_id < 15) {
            ship->lobby_event = item_id;
            update_lobby_event();
            return send_msg1(c, "%s", __(c, "\tE\tC7Event set."));
        }

        break;
    }

    return send_msg1(c, "%s", __(c, "\tE\tC4发生错误!请联系程序员!"));
}

static int bb_process_menu(ship_client_t* c, bb_select_pkt* pkt) {
    uint32_t menu_id = LE32(pkt->menu_id);
    uint32_t item_id = LE32(pkt->item_id);
    uint8_t* bp = (uint8_t*)pkt + 0x10;
    uint16_t len = LE16(pkt->hdr.pkt_len) - 0x10;

    //DBG_LOG("bb_process_menu指令: 0x%08X item_id %d", menu_id & 0xFF, item_id);

    //return process_menu(c, menu_id, item_id, bp + 0x10, len);

        /* Figure out what the client is selecting. */
    switch (menu_id & 0xFF) {
        /* Lobby Information Desk */
    case MENU_ID_INFODESK:
        return send_info_file(c, ship, item_id);

        /* Blocks */
    case MENU_ID_BLOCK:
    {
        uint16_t port;

        /* See if it's the "Ship Select" entry */
        if (item_id == ITEM_ID_LAST) {
            return send_ship_list(c, ship, ship->cfg->menu_code);
        }

        /* Make sure the block selected is in range. */
        if (item_id > ship->cfg->blocks) {
            return -1;
        }

        /* Make sure that block is up and running. */
        if (ship->blocks[item_id - 1] == NULL ||
            ship->blocks[item_id - 1]->run == 0) {
            return -2;
        }

        switch (c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            port = ship->blocks[item_id - 1]->dc_port;
            break;

        case CLIENT_VERSION_PC:
            port = ship->blocks[item_id - 1]->pc_port;
            break;

        case CLIENT_VERSION_GC:
            port = ship->blocks[item_id - 1]->gc_port;
            break;

        case CLIENT_VERSION_EP3:
            port = ship->blocks[item_id - 1]->ep3_port;
            break;

        case CLIENT_VERSION_BB:
            port = ship->blocks[item_id - 1]->bb_port;
            break;

        case CLIENT_VERSION_XBOX:
            port = ship->blocks[item_id - 1]->xb_port;
            break;

        default:
            return -1;
        }

        /* Redirect the client where we want them to go. */
#ifdef PSOCN_ENABLE_IPV6
        if (c->flags & CLIENT_FLAG_IPV6) {
            return send_redirect6(c, ship_ip6, port);
        }
        else {
            return send_redirect(c, ship_ip4, port);
        }
#else
        return send_redirect(c, ship_ip4, port);
#endif
    }

    /* Game Selection */
    case MENU_ID_GAME:
    {
        char passwd_cmp[17] = { 0 };
        lobby_t* l;
        int override = c->flags & CLIENT_FLAG_OVERRIDE_GAME;

        memset(passwd_cmp, 0, 17);

        /* Read the password, if the client provided one. */
        if (c->version == CLIENT_VERSION_PC ||
            c->version == CLIENT_VERSION_BB) {
            char tmp[32];

            if (len > 0x20) {
                return -1;
            }

            memset(tmp, 0, 32);
            memcpy(tmp, bp, len);
            istrncpy16(ic_utf16_to_ascii, passwd_cmp, (uint16_t*)tmp, 16);
        }
        else {
            if (len > 0x10) {
                return -1;
            }

            memcpy(passwd_cmp, bp, len);
        }

        /* The client is selecting a game to join. */
        l = block_get_lobby(c->cur_block, item_id);

        if (!l) {
            /* The lobby has disappeared. */
            send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法加入游戏!"),
                __(c, "\tC7This game is\nnon-existant."));
            return 0;
        }

        /* Check the provided password (if any). */
        if (!override) {
            if (l->passwd[0] && strcmp(passwd_cmp, l->passwd)) {
                send_msg1(c, "%s\n\n%s",
                    __(c, "\tE\tC4无法加入游戏!"),
                    __(c, "\tC7密码错误."));
                return 0;
            }
        }

        /* Attempt to change the player's lobby. */
        bb_join_game(c, l);

        return 0;
    }

    /* Quest category */
    case MENU_ID_QCATEGORY:
    {
        int rv;
        int lang;

        pthread_rwlock_rdlock(&ship->qlock);

        /* Do we have quests configured? */
        if (!TAILQ_EMPTY(&ship->qmap)) {
            lang = (menu_id >> 24) & 0xFF;
            rv = send_quest_list(c, (int)item_id, lang);
        }
        else {
            rv = send_msg1(c, "%s", __(c, "\tE\tC4未读取任务."));
        }
        pthread_rwlock_unlock(&ship->qlock);
        return rv;
    }

    /* Quest */
    case MENU_ID_QUEST:
    {
        int lang = (menu_id >> 24) & 0xFF;
        lobby_t* l = c->cur_lobby;

        if (l->flags & LOBBY_FLAG_BURSTING) {
            return send_msg1(c, "%s",
                __(c, "\tE\tC4请等待一会后操作."));
        }

        return lobby_setup_quest(l, c, item_id, lang);
    }

    /* Ship */
    case MENU_ID_SHIP:
    {
        miniship_t* i;
        int off = 0;

        /* See if the user picked a Ship List item */
        if (item_id == 0) {
            return send_ship_list(c, ship, (uint16_t)(menu_id >> 8));
        }

        switch (c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            off = 0;
            break;

        case CLIENT_VERSION_PC:
            off = 1;
            break;

        case CLIENT_VERSION_GC:
            off = 2;
            break;

        case CLIENT_VERSION_EP3:
            off = 3;
            break;

        case CLIENT_VERSION_BB:
            off = 4;
            break;

        case CLIENT_VERSION_XBOX:
            off = 5;
            break;
        }

        /* Go through all the ships that we know about looking for the one
           that the user has requested. */
        TAILQ_FOREACH(i, &ship->ships, qentry) {
            if (i->ship_id == item_id) {
#ifdef PSOCN_ENABLE_IPV6
                if (c->flags & CLIENT_FLAG_IPV6 && i->ship_addr6[0]) {
                    return send_redirect6(c, i->ship_addr6,
                        i->ship_port + off);
                }
                else {
                    return send_redirect(c, i->ship_addr,
                        i->ship_port + off);
                }
#else
                return send_redirect(c, i->ship_addr, i->ship_port + off);
#endif
            }
        }

        /* We didn't find it, punt. */
        return send_msg1(c, "%s",
            __(c, "\tE\tC4当前选择的舰船\n已离线."));
    }

    /* Game type (PSOBB only) */
    case MENU_ID_GAME_TYPE:
        return bb_process_game_type(c, menu_id, item_id);

    /* GM Menu */
    case MENU_ID_GM:
        return bb_process_gm_menu(c, menu_id, item_id);

        /* Game type (PSOBB only) */
    case MENU_ID_GAME_DROP:
        return bb_process_game_drop_set(c, menu_id, item_id);

    default:
        ERR_LOG("bb_process_menu menu_id & 0xFF = %u", menu_id & 0xFF);
        if (script_execute(ScriptActionUnknownMenu, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_UINT32, menu_id, SCRIPT_ARG_UINT32,
            item_id, SCRIPT_ARG_END) > 0)
            return 0;

        return send_msg1(c, "%s",
            __(c, "\tE\tC4菜单错误, 请联系管理员."));
    }

    return -1;
}

static int bb_process_ping(ship_client_t* c) {
    int rv;
    if (!(c->flags & CLIENT_FLAG_SENT_MOTD)) {
        send_motd(c);
        c->flags |= CLIENT_FLAG_SENT_MOTD;
    }

    /* If they've got the always legit flag set, but we haven't run the
       legit check yet, then do so now. The reason we wait until now is
       so that if they fail the legit check, we can actually inform them
       of that. */
    if ((c->flags & CLIENT_FLAG_ALWAYS_LEGIT) &&
        !(c->flags & CLIENT_FLAG_LEGIT)) {
        psocn_limits_t* limits;

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
        }

        pthread_rwlock_unlock(&ship->llock);
    }

    if (c->flags & CLIENT_FLAG_WAIT_QPING) {
        lobby_t* l = c->cur_lobby;

        if (!l)
            return -1;

        pthread_mutex_lock(&l->mutex);
        l->flags &= ~LOBBY_FLAG_BURSTING;
        c->flags &= ~(CLIENT_FLAG_BURSTING | CLIENT_FLAG_WAIT_QPING);

        rv = lobby_resend_burst_bb(l, c);
        rv = send_simple(c, PING_TYPE, 0) |
            lobby_handle_done_burst_bb(l, c);
        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    c->last_message = time(NULL);

    return 0;
}

static int bb_process_guild_search(ship_client_t* c, bb_guild_search_pkt* pkt) {
    uint32_t i;
    ship_client_t* it;
    uint32_t gc = LE32(pkt->gc_target);
    int done = 0, rv = -1;
    uint32_t flags = 0;

    /* Don't allow this if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't allow guild searches for any reserved guild card numbers. */
    if (gc < 1000)
        return 0;

    /* Search the local ship first. */
    for (i = 0; i < ship->cfg->blocks && !done; ++i) {
        if (!ship->blocks[i] || !ship->blocks[i]->run) {
            continue;
        }

        pthread_rwlock_rdlock(&ship->blocks[i]->lock);

        /* Look through all clients on that block. */
        TAILQ_FOREACH(it, ship->blocks[i]->clients, qentry) {
            /* Check if this is the target and the target has player
               data. */
            if (it->guildcard == gc && it->pl) {
                pthread_mutex_lock(&it->mutex);
#ifdef PSOCN_ENABLE_IPV6
                if ((c->flags & CLIENT_FLAG_IPV6)) {
                    rv = send_guild_reply6(c, it);
                }
                else {
                    rv = send_guild_reply(c, it);
                }
#else
                rv = send_guild_reply(c, it);
#endif
                done = 1;
                pthread_mutex_unlock(&it->mutex);
            }
            else if (it->guildcard == gc) {
                /* If they're on but don't have data, we're not going to
                   find them anywhere else, return success. */
                rv = 0;
                done = 1;
            }

            if (done)
                break;
        }

        pthread_rwlock_unlock(&ship->blocks[i]->lock);
    }

    /* 如果我们到了这里, we didn't find it locally. Send to the shipgate to
       continue searching. */
    if (!done) {
#ifdef PSOCN_ENABLE_IPV6
        if ((c->flags & CLIENT_FLAG_IPV6)) {
            flags |= FW_FLAG_PREFER_IPV6;
        }
#endif

        return shipgate_fw_bb(&ship->sg, pkt, flags, c);
    }

    return rv;
}

static int bb_process_char(ship_client_t* c, bb_char_data_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);
    uint32_t version = c->version;// pkt->hdr.flags;
    uint32_t v;
    int i;

    pthread_mutex_lock(&c->mutex);

    /* 要完善更合理的角色数据检测...
       TODO: This should probably be more thorough and done as part of the
       client_check_character() function. */
    /* If they already had character data, then check if it's still sane. */
    if (c->pl->bb.character.name) {
        i = client_check_character(c, &pkt->data, version);
        if (i) {
            ERR_LOG("%s(%d): 角色数据检查失败 GC %" PRIu32
                " 错误码 %d", ship->cfg->name, c->cur_block->b,
                c->guildcard, i);
            if (c->cur_lobby) {
                ERR_LOG("        房间: %s (类型: 难度:%d,对战模式:%d,挑战模式:%d,V2:%d)",
                    c->cur_lobby->name, c->cur_lobby->difficulty,
                    c->cur_lobby->battle, c->cur_lobby->challenge,
                    c->cur_lobby->v2);
            }
        }
       /* else
            ERR_LOG("%s(%d): 角色数据检测 GC %" PRIu32
                " 版本 = %d 未找到对应角色", ship->cfg->name, c->cur_block->b,
                c->guildcard, c->version);*/
    }
    else {
        ERR_LOG("%s(%d): 角色数据检测 GC %" PRIu32
            " 版本 = %d 未找到角色名", ship->cfg->name, c->cur_block->b,
            c->guildcard, c->version);
    }

    v = LE32(pkt->data.bb.character.disp.level + 1);
    if (v > MAX_PLAYER_LEVEL) {
        send_msg_box(c, __(c, "\tE不允许作弊玩家进入\n"
            "这个服务器.\n\n"
            "这条信息将会上报至\n"
            "管理员处."));
        ERR_LOG("%s(%d): 检测到无效级别的角色!\n"
            "        GC %" PRIu32 ", 等级: %" PRIu32 "",
            ship->cfg->name, c->cur_block->b,
            c->guildcard, v);
        return -1;
    }

    if (pkt->data.bb.autoreply[0]) {
        /* 复制自动回复数据 */
        client_set_autoreply(c, pkt->data.bb.autoreply,
            len - 4 - sizeof(bb_player_t));
    }

    /* 复制玩家数据至统一结构, 并设置指针. */
    memcpy(c->pl, &pkt->data, sizeof(bb_player_t));
    c->infoboard = (char*)c->pl->bb.infoboard;
    c->c_rank = c->pl->bb.c_rank.c_rank.all;
    memcpy(c->blacklist, c->pl->bb.blacklist, 30 * sizeof(uint32_t));

    /* 将背包数据复制至玩家数据结构中 */
    memcpy(c->iitems, c->pl->bb.inv.iitems, sizeof(iitem_t) * 30);
    c->item_count = (int)c->pl->bb.inv.item_count;

    /* 重新对库存数据进行编号, 以便后期进行数据交换 */
    for (i = 0; i < c->item_count; ++i) {
        v = 0x00210000 | i;
        c->iitems[i].data.item_id = LE32(v);
    }

    /* If this packet is coming after the client has left a game, then don't
       do anything else here, they'll take care of it by sending an 0x84
        如果此数据包是在客户端退出游戏后发出的，则不要在此处执行任何其他操作
        ，他们将通过发送0x84来处理此数据包 . */
    if (type == LEAVE_GAME_PL_DATA_TYPE) {
        /* Remove the client from the lobby they're in, which will force the
           0x84 sent later to act like we're adding them to any lobby
            将客户端从其所在的大厅中删除，
            这将强制稍后发送的0x84表现为我们正在将其添加到任何大厅中 . */
        pthread_mutex_unlock(&c->mutex);
        return lobby_remove_player(c);
    }

    /* If the client isn't in a lobby already, then add them to the first
       available default lobby 如果客户机不在大厅中，则将其添加到第一个可用的默认大厅中. */
    if (!c->cur_lobby) {
        if (lobby_add_to_any(c, c->lobby_req)) {
            pthread_mutex_unlock(&c->mutex);
            return -1;
        }

        if (send_lobby_join(c, c->cur_lobby)) {
            pthread_mutex_unlock(&c->mutex);
            return -2;
        }

        if (send_lobby_add_player(c->cur_lobby, c)) {
            pthread_mutex_unlock(&c->mutex);
            return -3;
        }

        /* Do a few things that should only be done once per session... */
        if (!(c->flags & CLIENT_FLAG_SENT_MOTD)) {
            script_execute(ScriptActionClientBlockLogin, c, SCRIPT_ARG_PTR,
                c, SCRIPT_ARG_END);

            uint16_t bbname[BB_CHARACTER_NAME_LENGTH + 1];

            memcpy(bbname, c->bb_pl->character.name, BB_CHARACTER_NAME_LENGTH);
            bbname[BB_CHARACTER_NAME_LENGTH] = 0;

            /* Notify the shipgate */
            shipgate_send_block_login_bb(&ship->sg, 1, c->guildcard, c->sec_data.slot,
                c->cur_block->b, bbname);

            if (c->cur_lobby)
                shipgate_send_lobby_chg(&ship->sg, c->guildcard,
                    c->cur_lobby->lobby_id, c->cur_lobby->name);
            else
                ERR_LOG("shipgate_send_lobby_chg 错误");

            c->flags |= CLIENT_FLAG_SENT_MOTD;
        }
        else {
            if (c->cur_lobby)
                shipgate_send_lobby_chg(&ship->sg, c->guildcard,
                    c->cur_lobby->lobby_id, c->cur_lobby->name);
            else
                ERR_LOG("shipgate_send_lobby_chg 错误");
        }

        //print_payload((uint8_t*)&c->bb_pl->quest_data1[0], sizeof(c->bb_pl->quest_data1));

        send_bb_quest_data1(c, c->bb_pl->quest_data1);

        if (c->cur_lobby)
            send_lobby_pkt(c->cur_lobby, c, build_guild_full_data_pkt(c), 1);
        else
            ERR_LOG("大厅玩家数量 %d %d", c->cur_lobby->max_clients, c->cur_lobby->num_clients);

        /* Send a ping so we know when they're done loading in. This is useful
           for sending the MOTD as well as enforcing always-legit mode. */
        send_simple(c, PING_TYPE, 0);
    }

    pthread_mutex_unlock(&c->mutex);

    return 0;
}

/* Process a client's done bursting signal. */
static int bb_process_done_burst(ship_client_t* c, bb_done_burst_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv;
    //uint16_t flag;

        /* 合理性检查... Is the client in a game lobby? */
    if (!l || l->type == LOBBY_TYPE_LOBBY) {
        return -1;
    }

    /* Lock the lobby, clear its bursting flag, send the resume game packet to
       the rest of the lobby, and continue on. */
    pthread_mutex_lock(&l->mutex);

    /* Handle the end of burst stuff with the lobby */
    if (!(l->flags & LOBBY_FLAG_QUESTING)) {
        l->flags &= ~LOBBY_FLAG_BURSTING;
        c->flags &= ~CLIENT_FLAG_BURSTING;

        if (l->version == CLIENT_VERSION_BB) {
            send_lobby_end_burst(l);
        }

        rv = send_simple(c, PING_TYPE, 0) | lobby_handle_done_burst_bb(l, c);
    }
    else {
        rv = send_quest_one(l, c, l->qid, l->qlang);
        c->flags |= CLIENT_FLAG_WAIT_QPING;
        rv |= send_simple(c, PING_TYPE, 0);

    }

    pthread_mutex_unlock(&l->mutex);

    return rv;
}

/* 处理客户端获得任务后. */
static int bb_process_done_quest_burst(ship_client_t* c, bb_done_quest_burst_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;
    //uint16_t flag;

        /* 合理性检查... Is the client in a game lobby? */
    if (!l || l->type == LOBBY_TYPE_LOBBY)
        return -1;

    /* Lock the lobby, clear its bursting flag, send the resume game packet to
       the rest of the lobby, and continue on. */
    pthread_mutex_lock(&l->mutex);

    send_bb_rare_monster_data(c);

    if (l->version == CLIENT_VERSION_BB) {
        send_lobby_end_burst(l);

        //TEST_LOG("bb_process_done_quest_burst");
        send_lobby_pkt(l, c, build_guild_full_data_pkt(c), 1);
    }

    pthread_mutex_unlock(&l->mutex);

    return rv;
}

static int bb_process_mail(ship_client_t* c, simple_mail_pkt* pkt) {
    uint32_t i;
    ship_client_t* it;
    uint32_t gc = LE32(pkt->bbmaildata.gc_dest);
    int done = 0, rv = -1;

    /* Don't send mail if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录游戏才可以发送邮件."));
    }

    /* Don't send mail for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    /* First check if this is to the bug report "character". */
    if (gc == BUG_REPORT_GC) {
        bb_bug_report(c, pkt);
        return 0;
    }

    /* Don't allow mails for any reserved guild card numbers other than the bug
       report one. */
    if (gc < 1000)
        return 0;

    /* Search the local ship first. */
    for (i = 0; i < ship->cfg->blocks && !done; ++i) {
        if (!ship->blocks[i] || !ship->blocks[i]->run) {
            continue;
        }

        pthread_rwlock_rdlock(&ship->blocks[i]->lock);

        /* Look through all clients on that block. */
        TAILQ_FOREACH(it, ship->blocks[i]->clients, qentry) {
            /* Check if this is the target and the target has player
               data. */
            if (it->guildcard == gc && it->pl) {
                pthread_mutex_lock(&it->mutex);

                /* Make sure the user hasn't blacklisted the sender. */
                if (client_has_blacklisted(it, c->guildcard) ||
                    client_has_ignored(it, c->guildcard)) {
                    done = 1;
                    pthread_mutex_unlock(&it->mutex);
                    rv = 0;
                    break;
                }

                /* Check if the user has an autoreply set. */
                if (it->autoreply_on) {
                    send_mail_autoreply(c, it);
                }

                rv = send_bb_simple_mail(it, pkt);
                pthread_mutex_unlock(&it->mutex);
                done = 1;
                break;
            }
            else if (it->guildcard == gc) {
                /* If they're on but don't have data, we're not going to
                   find them anywhere else, return success. */
                rv = 0;
                done = 1;
                break;
            }
        }

        pthread_rwlock_unlock(&ship->blocks[i]->lock);
    }

    if (!done) {
        /* 如果我们到了这里, we didn't find it locally. Send to the shipgate to
           continue searching. */
        return shipgate_fw_bb(&ship->sg, pkt, 0, c);
    }

    return rv;
}

static int bb_process_change_lobby(ship_client_t* c, bb_select_pkt* pkt) {
    uint32_t item_id = LE32(pkt->item_id);

    return process_change_lobby(c, item_id);
}

/* Process a client's arrow update request. */
static int bb_process_arrow(ship_client_t* c, uint8_t flag) {
    c->arrow_color = flag;
    return send_lobby_arrows(c->cur_lobby);
}

static int bb_process_login(ship_client_t* c, bb_login_93_pkt* pkt) {
    uint32_t guild_id;
    char ipstr[INET6_ADDRSTRLEN];
    char* ban_reason;
    time_t ban_end;

    /* Make sure PSOBB is allowed on this ship. */
    if ((ship->cfg->shipgate_flags & LOGIN_FLAG_NOBB)) {
        send_msg_box(c, "%s", __(c, "\tE梦幻之星 蓝色脉冲\n 客户端无法登录此船.\n\n"
            "正在断开连接."));
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        return 0;
    }

    /* See if the user is banned */
    if (is_guildcard_banned(ship, c->guildcard, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }
    else if (is_ip_banned(ship, &c->ip_addr, &ban_reason, &ban_end)) {
        send_ban_msg(c, ban_end, ban_reason);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
        free_safe(ban_reason);
        return 0;
    }

    c->guildcard = LE32(pkt->guildcard);
    guild_id = LE32(pkt->guild_id);
    c->menu_id = pkt->menu_id;
    c->preferred_lobby_id = pkt->preferred_lobby_id;

    /* See if this person is a GM. */
    c->privilege = is_gm(c->guildcard, ship);

    /* Copy in the security data */
    memcpy(&c->sec_data, &pkt->var.new_clients.cfg, sizeof(bb_client_config_pkt));

    if (c->sec_data.cfg.magic != CLIENT_CONFIG_MAGIC) {
        send_bb_security(c, 0, LOGIN_93BB_FORCED_DISCONNECT, 0, NULL, 0);
        return -1;
    }

    /* Send the security data packet */
    if (send_bb_security(c, c->guildcard, LOGIN_93BB_OK, guild_id,
        &c->sec_data, sizeof(bb_client_config_pkt))) {
        return -2;
    }

    /* Request the character data from the shipgate */
    if (shipgate_send_creq(&ship->sg, c->guildcard, c->sec_data.slot)) {
        return -3;
    }

    /* Request the user options from the shipgate */
    if (shipgate_send_bb_opt_req(&ship->sg, c->guildcard, c->cur_block->b)) {
        return -4;
    }

    /* Log the connection. */
    my_ntop(&c->ip_addr, ipstr);
    BLOCK_LOG("%s(%d): 版本 Blue Burst GC %d 已连接 IP %s",
        ship->cfg->name, c->cur_block->b, c->guildcard, ipstr);

    return 0;
}

static int process_bb_qload_done(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* c2;
    int i;

    if (!l || l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " 的房间载入任务完成出错!",
            c->guildcard);
        return -1;
    }

    c->flags |= CLIENT_FLAG_QLOAD_DONE;

    /* 检测所有成员是否完成任务载入. */
    for (i = 0; i < l->max_clients; ++i) {
        if ((c2 = l->clients[i])) {
            /* 该客户端未完成载入, 结束循环并执行下一步. */
            if (!(c2->flags & CLIENT_FLAG_QLOAD_DONE) &&
                c2->version >= CLIENT_VERSION_GC)
                return 0;
        }
    }

    /* 如果我们到了这里, 所有客户端载入完成. 向每一个客户端发送载入完成指令. */
    for (i = 0; i < l->max_clients; ++i) {
        if ((c2 = l->clients[i]) && c2->version >= CLIENT_VERSION_GC) {
            if (send_simple(c2, QUEST_LOAD_DONE_TYPE, 0))
                c2->flags |= CLIENT_FLAG_DISCONNECTED;
        }
    }

    return 0;
}

static int process_bb_qlist(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;
    int rv;

    if (!l || l->type != LOBBY_TYPE_GAME)
        return -1;

    pthread_rwlock_rdlock(&ship->qlock);
    pthread_mutex_lock(&l->mutex);

    /* Do we have quests configured? */
    if (!TAILQ_EMPTY(&ship->qmap)) {
        l->flags |= LOBBY_FLAG_QUESTSEL;
        rv = send_quest_categories(c, c->q_lang);
    }
    else {
        rv = send_msg1(c, "%s", __(c, "\tE\tC4未设置任务."));
    }

    pthread_mutex_unlock(&l->mutex);
    pthread_rwlock_unlock(&ship->qlock);

    return rv;
}

static int process_bb_qlist_end(ship_client_t* c) {
    lobby_t* l = c->cur_lobby;

    if (!l || l->type != LOBBY_TYPE_GAME)
        return -1;

    pthread_mutex_lock(&l->mutex);
    c->cur_lobby->flags &= ~LOBBY_FLAG_QUESTSEL;
    pthread_mutex_unlock(&l->mutex);

    return 0;
}

/* Process a 0xAA packet. This command is used in Maximum Attack 2 */
static int process_bb_update_quest_stats(ship_client_t* c,
    bb_update_quest_stats_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);
    lobby_t* l = c->cur_lobby;

    print_payload((unsigned char*)pkt, len);

    if (!l || !(l->flags & LOBBY_FLAG_QUESTING))
        return -1;

    if (c->flags & 0x00002000)
        ERR_LOG("trial edition client sent update quest stats command.");

    DBG_LOG("process_dc_update_quest_stats qid %d  quest_internal_id %d", l->qid, pkt->quest_internal_id);

    if (l->qid != pkt->quest_internal_id) {
        ERR_LOG("l->qid != pkt->quest_internal_id.");
        return -1;
    }

    return send_bb_confirm_update_quest_statistics(c, pkt->request_token);
}

static int bb_process_game_create(ship_client_t* c, bb_game_create_pkt* pkt) {
    lobby_t* l;
    uint8_t event = ship->game_event;
    char name[65], passwd[65];
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    //UNK_CPD(type, c->version, pkt);

    /* 转换房间 名称/密码 为 UTF-8格式 */
    istrncpy16_raw(ic_utf16_to_gbk, name, pkt->name, 64, 16);
    istrncpy16_raw(ic_utf16_to_gbk, passwd, pkt->password, 64, 16);

#ifdef DEBUG
    LOBBY_LOG("创建游戏房间 章节 %d 难度 %d 人物等级 %d 需求等级 %d", pkt->episode,
        pkt->difficulty, c->pl->bb.character.disp.level + 1,
        bb_game_required_level[pkt->episode - 1][pkt->difficulty]);
#endif // DEBUG

    /* Check the user's ability to create a game of that difficulty
    检查用户创建该难度游戏的能力. */
    if (!(c->flags & CLIENT_FLAG_OVERRIDE_GAME)) {
        if ((c->pl->bb.character.disp.level + 1) < bb_game_required_level[pkt->episode - 1][pkt->difficulty]) {
            return send_msg1(c, "%s\n %s\n%s\n\n%s\n%s%d",
                __(c, "\tE房间名称: "),
                name,
                __(c, "\tC4创建失败!"),
                __(c, "\tC2您的等级太低."),
                __(c, "\tC2需求等级: Lv.\tC6"),
                bb_game_required_level[pkt->episode - 1][pkt->difficulty]);
        }
    }

    if (pkt->episode == LOBBY_EPISODE_4 && (pkt->challenge || pkt->battle)) {
        send_msg1(c, __(c, "\tE\tC4EP4暂不支持挑战模式或对战模式"));
        return 0;
    }

    /* 创建游戏房间结构. */
    l = lobby_create_game(c->cur_block, name, passwd, pkt->difficulty,
        pkt->battle, pkt->challenge, 0, c->version,
        c->pl->bb.character.disp.dress_data.section, event, pkt->episode, c,
        pkt->single_player, 1);

    /* If we don't have a game, something went wrong... tell the user. */
    if (!l) {
        return send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4创建游戏失败!"),
            __(c, "\tC7请稍后重试."));
    }

    /* If its a non-challenge, non-battle, non-ultimate game, ask the user if
       they want v1 compatibility or not. */
    if (!pkt->battle && !pkt->challenge && pkt->difficulty != 4 &&
        !(c->flags & CLIENT_FLAG_IS_NTE)) {
        c->create_lobby = l;
        return send_bb_game_type_sel(c);
    }

    /* We've got a new game, but nobody's in it yet... Lets put the requester
       in the game. */
    if (bb_join_game(c, l)) {
        /* Something broke, destroy the created lobby before anyone tries to
           join it. */
        pthread_rwlock_wrlock(&c->cur_block->lobby_lock);
        lobby_destroy(l);
        pthread_rwlock_unlock(&c->cur_block->lobby_lock);
    }

    /* All is good in the world. */
    return 0;
}

static int bb_process_blacklist(ship_client_t* c,
    bb_blacklist_update_pkt* pkt) {
    memcpy(c->blacklist, pkt->list, 30 * sizeof(uint32_t));
    memcpy(c->pl->bb.blacklist, pkt->list, 30 * sizeof(uint32_t));
    memcpy(c->bb_opts->black_gc_list, pkt->list, 30 * sizeof(uint32_t));
    return send_txt(c, "%s", __(c, "\tE\tC7黑名单已更新."));
}

/* Process a blue burst client's trade request. */
static int bb_process_trade(ship_client_t* c, bb_trade_D0_D3_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t target_client_id = pkt->target_client_id;
    ship_client_t* c2 = { 0 };
    int i;

    if (!l || l->type != LOBBY_TYPE_GAME)
        return 0;

    if (&c->game_data->pending_item_trade) {
        ERR_LOG("玩家在一笔交易尚未完成时开始交易");
        return 0;
    }

    if (pkt->item_count > 0x20) {
        ERR_LOG("GC %" PRIu32 " 尝试交易的物品超出限制!",
            c->guildcard);
        return 0;
    }

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]->client_id == target_client_id) {
            c2 = l->clients[i];
            break;
        }
    }

    if (c2 == NULL)
        return send_msg1(c, "%s",
            __(c, "\tE未找到需要交易的玩家."));

    /* Find the destination. */
    if (!c2) {
        ERR_LOG("GC %" PRIu32 " 尝试与不存在的玩家交易!",
            c->guildcard);
        return 0;
    }

    DBG_LOG("GC %" PRIu32 " 尝试交易 %d 件物品 给 %" PRIu32 "!",
        c->guildcard, pkt->item_count, c2->guildcard);

    print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));

    memset(&c->game_data->pending_item_trade, 0, sizeof(client_trade_item_t));
    c->game_data->pending_item_trade->other_client_id = target_client_id;
    for (size_t x = 0; x < pkt->item_count; x++) {
        c->game_data->pending_item_trade->items[x] = pkt->items[x];
    }

    DBG_LOG("GC %" PRIu32 " 尝试交易 %d 件物品 给 %" PRIu32 "!",
        c->guildcard, pkt->item_count, c2->guildcard);

    send_simple(c2, TRADE_1_TYPE, 0x00);

    if (c2->game_data->pending_item_trade)
        send_simple(c, TRADE_1_TYPE, 0x00);

    return 0;
}

static int bb_process_trade_excute(ship_client_t* c, bb_trade_D0_D3_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t target_client_id = pkt->target_client_id;
    ship_client_t* c2 = { 0 };
    int i;

    if (!l || l->type != LOBBY_TYPE_GAME)
        return 0;

    if (!c->game_data->pending_item_trade) {
        ERR_LOG("GC %" PRIu32 " 未产生交易!",
            c->guildcard);
        return 0;
    }

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]->client_id == target_client_id) {
            c2 = l->clients[i];
            break;
        }
    }

    if (!c2 || !c2->game_data->pending_item_trade)
        return send_msg1(c, "%s",
            __(c, "\tE未找到需要交易的玩家."));

    print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
    c->game_data->pending_item_trade->confirmed = true;
    if (c2->game_data->pending_item_trade->confirmed) {
        send_bb_execute_item_trade(c, c2->game_data->pending_item_trade->items);
        send_bb_execute_item_trade(c2, c->game_data->pending_item_trade->items);
        send_simple(c, TRADE_4_TYPE, 0x01);
        send_simple(c2, TRADE_4_TYPE, 0x01);
        memset(c->game_data->pending_item_trade, 0, sizeof(client_trade_item_t));
        memset(c2->game_data->pending_item_trade, 0, sizeof(client_trade_item_t));
    }

    return 0;
}

static int bb_process_trade_error(ship_client_t* c, bb_trade_D0_D3_pkt* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* c2;
    int rv = -1;

    if (!l || l->type != LOBBY_TYPE_GAME)
        return -1;

    if (!c->game_data->pending_item_trade)
        return 0;

    uint16_t other_client_id = c->game_data->pending_item_trade->other_client_id;
    memset(&c->game_data->pending_item_trade, 0, sizeof(client_trade_item_t));
    send_simple(c, TRADE_4_TYPE, 0);

    /* Find the destination. */
    c2 = l->clients[other_client_id];
    if (!c2) {
        ERR_LOG("GC %" PRIu32 " 尝试与不存在的玩家交易!",
            c->guildcard);
        return -1;
    }else if (!c2->game_data->pending_item_trade) {
        return -1;
    }
    else {
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

        memset(c2->game_data->pending_item_trade, 0, sizeof(client_trade_item_t));
        send_simple(c2, TRADE_4_TYPE, 0);
    }

    return 0;
}

static int bb_process_infoboard(ship_client_t* c, bb_write_info_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len) - sizeof(bb_pkt_hdr_t);
    uint16_t type = LE16(pkt->hdr.pkt_type);

    if (!c->infoboard || len > sizeof(c->bb_pl->infoboard)) {
        ERR_LOG("GC %" PRIu32 " 尝试设置的信息板数据超出限制!",
            c->guildcard);
        return -1;
    }

    /* BB has this in two places for now... */
    memcpy(c->infoboard, pkt->msg, len);
    /* 剩余数据设置为0 以免出现乱码 */
    memset(c->infoboard + len, 0, sizeof(c->bb_pl->infoboard) - len);

    memcpy(c->bb_pl->infoboard, pkt->msg, len);
    /* 剩余数据设置为0 以免出现乱码 */
    memset(((uint8_t*)c->bb_pl->infoboard) + len, 0, sizeof(c->bb_pl->infoboard) - len);

    return 0;
}

static int bb_process_full_char(ship_client_t* c, bb_full_char_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);
    psocn_bb_full_char_t char_data = pkt->data;

    if (c->version != CLIENT_VERSION_BB)
        return -1;

    if (!c->bb_pl || len > BB_FULL_CHARACTER_DATA_LENGTH) {
        return -1;
    }

    /* No need if they've already maxed out. */
    if (c->bb_pl->character.disp.level + 1 > MAX_PLAYER_LEVEL) {
        ERR_LOG("GC %d 等级超过 200 (%d)", c->guildcard, c->bb_pl->character.disp.level + 1);
        return -1;
    }

    ///* BB has this in two places for now... */
    //memcpy(c->bb_pl->quest_data1, char_data.quest_data1, sizeof(c->bb_pl->quest_data1));
    //memcpy(c->bb_pl->guildcard_desc, char_data.gc_data.guildcard_desc, sizeof(c->bb_pl->guildcard_desc));
    //memcpy(c->bb_pl->autoreply, char_data.autoreply, sizeof(c->bb_pl->autoreply));
    //memcpy(c->bb_pl->infoboard, char_data.infoboard, sizeof(c->bb_pl->infoboard));
    //memcpy(c->bb_pl->challenge_data, char_data.challenge_data, sizeof(c->bb_pl->challenge_data));
    //memcpy(c->bb_pl->tech_menu, char_data.tech_menu, sizeof(c->bb_pl->tech_menu));
    //memcpy(c->bb_pl->quest_data2, char_data.quest_data2, sizeof(c->bb_pl->quest_data2));

    //memcpy(&c->bb_guild->guild_data, &char_data.guild_data, sizeof(bb_guild_t));

    /* BB has this in two places for now... */
    memcpy(&c->bb_pl->inv, &char_data.inv, sizeof(inventory_t));//
    memcpy(&c->bb_pl->character, &char_data.character, sizeof(psocn_bb_char_t));//
    memcpy(c->bb_pl->quest_data1, char_data.quest_data1, sizeof(c->bb_pl->quest_data1));
    memcpy(&c->bb_pl->bank, &char_data.bank, sizeof(psocn_bank_t));//
    memcpy(c->bb_pl->guildcard_desc, char_data.gc_data.guildcard_desc, sizeof(c->bb_pl->guildcard_desc));
    memcpy(c->bb_pl->autoreply, char_data.autoreply, sizeof(c->bb_pl->autoreply));
    memcpy(c->bb_pl->infoboard, char_data.infoboard, sizeof(c->bb_pl->infoboard));
    memcpy(c->bb_pl->challenge_data, char_data.challenge_data, sizeof(c->bb_pl->challenge_data));
    memcpy(c->bb_pl->tech_menu, char_data.tech_menu, sizeof(c->bb_pl->tech_menu));
    memcpy(c->bb_pl->quest_data2, char_data.quest_data2, sizeof(c->bb_pl->quest_data2));

    memcpy(&c->bb_guild->guild_data, &char_data.guild_data, sizeof(bb_guild_t));

    memcpy(&c->bb_opts->key_cfg, &char_data.key_cfg, sizeof(bb_key_config_t));
    c->bb_opts->option_flags = char_data.option_flags;
    memcpy(c->bb_opts->shortcuts, &char_data.shortcuts, 0x0A40);
    memcpy(c->bb_opts->symbol_chats, &char_data.symbol_chats, 0x04E0);
    memcpy(c->bb_opts->guild_name, &char_data.gc_data2.guild_name, sizeof(char_data.gc_data2.guild_name));

    return 0;
}

/* Process a Blue Burst options update */
static int bb_process_opt_flags(ship_client_t* c, bb_options_update_option_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_options_update_option_pkt)) {
        ERR_LOG("BB选项设置更新的数据大小无效 (数据大小:%d)", len);
        return -1;
    }

    c->bb_opts->option_flags = pkt->option;
    c->option_flags = c->bb_opts->option_flags;

    return 0;
}

static int bb_process_symbols(ship_client_t* c, bb_options_update_symbol_chats_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_options_update_symbol_chats_pkt)) {
        ERR_LOG("BB表情设置更新的数据大小无效 (数据大小:%d)", len);
        return -1;
    }

    memcpy(c->bb_opts->symbol_chats, pkt->symbol_chats, sizeof(pkt->symbol_chats));
    return 0;
}

static int bb_process_shortcuts(ship_client_t* c, bb_options_update_chat_shortcuts_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_options_update_chat_shortcuts_pkt)) {
        ERR_LOG("BB聊天快捷方式更新的数据大小无效 (数据大小:%d)", len);
        return -1;
    }

    memcpy(c->bb_opts->shortcuts, pkt->chat_shortcuts, sizeof(pkt->chat_shortcuts));
    return 0;
}

static int bb_process_keys(ship_client_t* c, bb_options_update_key_config_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_options_update_key_config_pkt)) {
        ERR_LOG("BB键位设置更新的数据大小无效 (数据大小:%d)", len);
        return -1;
    }

    memcpy(c->bb_opts->key_cfg.key_config, pkt->key_config, sizeof(pkt->key_config));
    return 0;
}

static int bb_process_pad(ship_client_t* c, bb_options_update_pad_config_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_options_update_pad_config_pkt)) {
        ERR_LOG("BB键盘设置更新的数据大小无效 (数据大小:%d)", len);
        return -1;
    }

    memcpy(c->bb_opts->key_cfg.joystick_config, pkt->pad_config, sizeof(pkt->pad_config));
    return 0;
}

static int bb_process_techs(ship_client_t* c, bb_options_update_tech_menu_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_options_update_tech_menu_pkt)) {
        ERR_LOG("BB法术设置更新的数据大小无效 (数据大小:%d)", len);
        return -1;
    }

    memcpy(c->bb_pl->tech_menu, pkt->tech_menu, sizeof(pkt->tech_menu));
    return 0;
}

static int bb_process_config(ship_client_t* c, bb_options_update_customize_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_options_update_customize_pkt)) {
        ERR_LOG("BB设置更新的数据大小无效 (数据大小:%d)", len);
        return -1;
    }

    memcpy(c->bb_pl->character.config, pkt->customize, sizeof(pkt->customize));
    return 0;
}

static int bb_process_ch_mode_config(ship_client_t* c, bb_options_update_challenge_battle_config_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_options_update_challenge_battle_config_pkt)) {
        ERR_LOG("无效 BB 挑战模式更新数据包 (数据大小:%d)", len);
        return -1;
    }

    memcpy(c->bb_pl->challenge_data, pkt->challenge_battle_config, sizeof(pkt->challenge_battle_config));
    return 0;
}

static int bb_set_guild_text(ship_client_t* c, bb_guildcard_set_txt_pkt* pkt) {
    uint16_t len = LE16(pkt->hdr.pkt_len);
    size_t data_len = sizeof(bb_guildcard_set_txt_pkt);

    if (len != sizeof(bb_guildcard_set_txt_pkt)) {
        ERR_LOG("无效 BB GC 文本描述更新数据包大小 (%d 数据大小:%d)", len, data_len);
        return -1;
    }

    memcpy(c->bb_pl->guildcard_desc, pkt->gc_data.guildcard_desc, sizeof(bb_guildcard_set_txt_pkt));
    return 0;
}

/* BB公会功能 */
static int process_bb_guild_create(ship_client_t* c, bb_guild_create_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_create_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    if (c->bb_guild->guild_data.guild_id > 0) {
        send_msg1(c, "%s\n%s", __(c, "\tE\tC4您已经在公会中了!"),
            __(c, "\tC7请切换舰船刷新数据."));
        return 0;
    }

    pkt->guildcard = c->guildcard;

    //print_payload((uint8_t*)pkt, len);

    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_unk_02EA(ship_client_t* c, bb_guild_unk_02EA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_02EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);

    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_member_add(ship_client_t* c, bb_guild_member_add_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);
    ship_client_t* c2 = { 0 };
    uint32_t  i = 0, target_gc = pkt->target_guildcard;
    int done = 0, rv = -1;

    if (len != sizeof(bb_guild_member_add_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    if (c->bb_guild->guild_data.guild_id <= 0)
        return 0;

    if (c->bb_guild->guild_data.guild_priv_level < 0x30)
        return 0;

    /* Search the local ship first. */
    for (i = 0; i < ship->cfg->blocks && !done; ++i) {
        if (!ship->blocks[i] || !ship->blocks[i]->run) {
            continue;
        }

        pthread_rwlock_rdlock(&ship->blocks[i]->lock);

        /* Look through all clients on that block. */
        TAILQ_FOREACH(c2, ship->blocks[i]->clients, qentry) {
            /* Check if this is the target and the target has player
               data. */
            if (c2->guildcard == target_gc && c2->bb_guild->guild_data.guild_id <= 0 && c2->guild_accept == 1) {
                pthread_mutex_lock(&c2->mutex);

                shipgate_fw_bb(&ship->sg, pkt, c->bb_guild->guild_data.guild_id, c2);

                c2->guild_accept = 0;

                memset(&c2->bb_guild->guild_data, 0, sizeof(bb_guild_t));

                c2->bb_guild->guild_data.guildcard = c2->bb_guild->guild_data.guildcard;
                c2->bb_guild->guild_data.guild_id = c->bb_guild->guild_data.guild_id;
                memcpy(&c2->bb_guild->guild_data.guild_info[0], &c->bb_guild->guild_data.guild_info[0], sizeof(c->bb_guild->guild_data.guild_info));
                c2->bb_guild->guild_data.guild_priv_level = 0;

                /* 赋予名称颜色代码 */
                memcpy(&c2->bb_guild->guild_data.guild_name, &c->bb_guild->guild_data.guild_name[0], sizeof(c->bb_guild->guild_data.guild_name));
                c2->bb_guild->guild_data.guild_name[0] = 0x0009;
                c2->bb_guild->guild_data.guild_name[1] = 0x0045;

                /* TODO 公会等级未实现 */
                c2->bb_guild->guild_data.guild_rank = c->bb_guild->guild_data.guild_rank;

                memcpy(&c2->bb_guild->guild_data.guild_flag, &c->bb_guild->guild_data.guild_flag[0], sizeof(c->bb_guild->guild_data.guild_flag));
                c2->bb_guild->guild_data.guild_rewards[0] = c->bb_guild->guild_data.guild_rewards[0];
                c2->bb_guild->guild_data.guild_rewards[1] = c->bb_guild->guild_data.guild_rewards[1];

                rv = send_bb_guild_cmd(c2, BB_GUILD_FULL_DATA);
                rv = send_bb_guild_cmd(c2, BB_GUILD_INITIALIZATION_DATA);
                rv = send_bb_guild_cmd(c, BB_GUILD_UNK_04EA);
                rv = send_bb_guild_cmd(c2, BB_GUILD_UNK_04EA);

                pthread_mutex_unlock(&c2->mutex);
                done = 1;
                break;
            }
            else if (c2->guildcard == target_gc) {
                /* 到这就没了, 获取对方已经属于某个公会. */
                send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4无法邀请玩家!"),
                    __(c, "\tC7对方已在公会中."));

                rv = 0;
                done = 1;
                break;
            }
        }

        pthread_rwlock_unlock(&ship->blocks[i]->lock);
    }

    if (!done) {
        /* 如果我们到了这里, we didn't find it locally. Send to the shipgate to
           continue searching. */
        return shipgate_fw_bb(&ship->sg, pkt, 0, c);
    }

    return 0;
}

static int process_bb_guild_unk_04EA(ship_client_t* c, bb_guild_unk_04EA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_04EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_member_remove(ship_client_t* c, bb_guild_member_remove_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);
    lobby_t* l = c->cur_lobby;
    uint32_t guild_id = c->bb_guild->guild_data.guild_id;
    uint32_t target_gc = pkt->target_guildcard;
    int i;
    ship_client_t* c2 = { 0 };
    char guild_name[30] = { 0 };

    if (len != sizeof(bb_guild_member_remove_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    if (guild_id <= 0) {
        return send_msg_box(c, "%s",
            __(c, "\tE您不在任何一个公会中."));
    }

    if (target_gc != c->guildcard && c->bb_guild->guild_data.guild_priv_level == 0x40) {
        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i]->guildcard == target_gc) {
                c2 = l->clients[i];

                break;
            }
        }

        if(!c2 || c2->bb_guild->guild_data.guild_id != guild_id)
            return send_msg_box(c, "%s",
                __(c, "\tE未找到该玩家."));

        shipgate_fw_bb(&ship->sg, pkt, guild_id, c);
        send_bb_guild_cmd(c, BB_GUILD_UNK_06EA);

        if (c2->bb_guild->guild_data.guild_priv_level < c->bb_guild->guild_data.guild_priv_level) {
            memset(&c2->bb_guild->guild_data, 0, sizeof(psocn_bb_db_guild_t));
            send_bb_guild_cmd(c2, BB_GUILD_INITIALIZATION_DATA);
            send_bb_guild_cmd(c2, BB_GUILD_FULL_DATA);

            istrncpy16_raw(ic_utf16_to_gbk, guild_name, &c->bb_guild->guild_data.guild_name[2], 30, 0x1C);

            send_msg_box(c2, "%s%s%s",
                __(c2, "\tE您已被 "),
                guild_name,
                __(c2, "\tE 公会开除."));
            return send_msg_box(c, "%s",
                __(c, "\tE会员已移除."));
        }
        else
            return send_msg_box(c, "%s",
                __(c, "\tE您的公会权限不足."));
    }

    if(target_gc == c->guildcard) {
        /* 操作数据库 */
        shipgate_fw_bb(&ship->sg, pkt, guild_id, c);
        /* 初始化对应GC的公会数据 */
        memset(&c->bb_guild->guild_data, 0, sizeof(psocn_bb_db_guild_t));
        return send_bb_guild_cmd(c, BB_GUILD_INITIALIZATION_DATA);
    }

    return send_msg_box(c, "%s",
        __(c, "\tE公会开除玩家发生错误,请联系管理员处理."));
}

static int process_bb_guild_06EA(ship_client_t* c, bb_guild_unk_06EA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_06EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_member_chat(ship_client_t* c, bb_guild_member_chat_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);
    uint32_t guild_id = c->bb_guild->guild_data.guild_id;

    if (c->bb_guild->guild_data.guild_id <= 0) {
        return send_msg1(c, "%s\n\n%s", __(c, "\tE\tC4您不在公会中!"),
            __(c, "\tC7您无权进行此操作."));
    }

    //if (add_color_tag((uint16_t*)&pkt->chat)) {
    //    ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
    //    print_payload((uint8_t*)pkt, len);
    //    return 0;
    //}

    pkt->guildcard = c->guildcard;
    pkt->guild_id = c->bb_guild->guild_data.guild_id;

    return shipgate_fw_bb(&ship->sg, pkt, guild_id, c);
}

static int process_bb_guild_member_setting(ship_client_t* c, bb_guild_member_setting_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (c->bb_guild->guild_data.guild_id != 0) {
        //print_payload((uint8_t*)pkt, len);
        return shipgate_fw_bb(&ship->sg, pkt, c->bb_guild->guild_data.guild_id, c);
    }

    return 0;
}

static int process_bb_guild_unk_09EA(ship_client_t* c, bb_guild_unk_09EA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_09EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);

    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_unk_0AEA(ship_client_t* c, bb_guild_unk_0AEA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_0AEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_unk_0BEA(ship_client_t* c, bb_guild_unk_0BEA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_0BEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);

    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_unk_0CEA(ship_client_t* c, bb_guild_unk_0CEA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_0CEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_invite_0DEA(ship_client_t* c, bb_guild_invite_0DEA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    //print_payload((uint8_t*)pkt, len);

    return send_bb_guild_cmd(c, BB_GUILD_UNK_0EEA);
}

static int process_bb_guild_unk_0EEA(ship_client_t* c, bb_guild_unk_0EEA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    print_payload((uint8_t*)pkt, len);
    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_member_flag_setting(ship_client_t* c, bb_guild_member_flag_setting_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_member_flag_setting_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    if ((c->bb_guild->guild_data.guild_priv_level == 0x40) && (c->bb_guild->guild_data.guild_id != 0)) {
        pkt->hdr.flags = c->bb_guild->guild_data.guild_id;
        return shipgate_fw_bb(&ship->sg, pkt, 0, c);
    }

    return 0;
}

static int process_bb_guild_dissolve(ship_client_t* c, bb_guild_dissolve_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);
    lobby_t* l = c->cur_lobby;

    if (!l)
        return 0;

    if (len != sizeof(bb_guild_dissolve_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    DBG_LOG("%d %d %d", c->bb_guild->guild_data.guild_priv_level, c->bb_guild->guild_data.guild_id, c->guildcard);

    if ((c->bb_guild->guild_data.guild_priv_level == 0x40) && (c->bb_guild->guild_data.guild_id != 0)) {
        print_payload((uint8_t*)pkt, len);
        //pkt->guild_id = c->bb_guild->guild_data.guild_id;
        shipgate_fw_bb(&ship->sg, pkt, c->bb_guild->guild_data.guild_id, c);
        send_bb_guild_cmd(c, BB_GUILD_DISSOLVE);
        memset(&c->bb_guild->guild_data, 0, sizeof(c->bb_guild->guild_data));
        send_lobby_pkt(l, NULL, build_guild_full_data_pkt(c), 1);
        send_bb_guild_cmd(c, BB_GUILD_INITIALIZATION_DATA);
        send_msg_box(c, "%s", __(c, "\tE\tC4公会已成功解散!"));
    }

    print_payload((uint8_t*)pkt, len);
    return 0;
}

static int process_bb_guild_member_promote(ship_client_t* c, bb_guild_member_promote_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);
    uint32_t guild_priv_level = pkt->hdr.flags;
    uint32_t target_gc = pkt->target_guildcard;
    uint32_t guild_id = c->bb_guild->guild_data.guild_id;
    lobby_t* l = c->cur_lobby;
    ship_client_t* c2 = { 0 };
    int i;

    if (!l)
        return -1;

    if (len != sizeof(bb_guild_member_promote_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return send_msg1(c, "%s %s", c_cmd_name(type, 0),
            __(c, "\tE数据错误."));
    }

    if (c->bb_guild->guild_data.guild_id <= 0) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return 0;
    }

    if (c->guildcard == target_gc) {
        ERR_LOG("错误 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return send_msg1(c, "%s",
            __(c, "\tE您无法提升自己的权限了."));
    }

    if (c->bb_guild->guild_data.guild_priv_level != 0x40) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);

        return send_msg1(c, "%s",
            __(c, "\tE您的权限不足."));
    }

    DBG_LOG("转换GC %u", target_gc);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i]->guildcard == target_gc && !l->clients[i]->guild_master_exfer) {
            c2 = l->clients[i];

            DBG_LOG("非会长转换GC %u", c2->guildcard);

            break;
        }
    }

    if(c2 == NULL)
        return send_msg1(c, "%s",
            __(c, "\tE未找到需要提升的公会玩家."));

    shipgate_fw_bb(&ship->sg, pkt, guild_id, c);

    if (guild_priv_level == 0x00000040 && c->guild_master_exfer) {
        //会长转让
        shipgate_fw_bb(&ship->sg, pkt, guild_id, c);
        c->bb_guild->guild_data.guild_priv_level = 0x00000030;
        send_bb_guild_cmd(c, BB_GUILD_INITIALIZATION_DATA);
        send_bb_guild_cmd(c, BB_GUILD_FULL_DATA);
        DBG_LOG("会长转换GC %u", c->guildcard);
        //send_bb_guild_cmd(c, BB_GUILD_FULL_DATA);

        //send_lobby_pkt(l, NULL, build_guild_full_data_pkt(c), 1);
    }

    if (c2 != NULL) {
        if (c2->bb_guild->guild_data.guild_priv_level != guild_priv_level) {
            c2->bb_guild->guild_data.guild_priv_level = guild_priv_level;
            send_bb_guild_cmd(c2, BB_GUILD_INITIALIZATION_DATA);
            send_bb_guild_cmd(c2, BB_GUILD_FULL_DATA);
            //send_bb_guild_cmd(c2, BB_GUILD_FULL_DATA);
            //send_lobby_pkt(c2->cur_lobby, NULL, build_guild_full_data_pkt(c2), 1);
        }

        send_bb_guild_cmd(c, BB_GUILD_MEMBER_PROMOTE);
    }

    return 0;
}

static int process_bb_guild_unk_12EA(ship_client_t* c, bb_guild_unk_12EA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_12EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_lobby_setting(ship_client_t* c, bb_guild_lobby_setting_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_lobby_setting_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    return send_bb_guild_cmd(c, BB_GUILD_LOBBY_SETTING);
}

static int process_bb_guild_member_tittle(ship_client_t* c, bb_guild_member_tittle_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_member_tittle_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);
    //return send_bb_guild_cmd(c, BB_GUILD_MEMBER_TITLE);
    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_full_data(ship_client_t* c, bb_guild_full_data_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);
    lobby_t* l = c->cur_lobby;

    if (!l)
        return 0;

    if (len != sizeof(bb_guild_full_data_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);

    return send_lobby_pkt(l, NULL, build_guild_full_data_pkt(c), 0);
}

static int process_bb_guild_unk_16EA(ship_client_t* c, bb_guild_unk_16EA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_16EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_unk_17EA(ship_client_t* c, bb_guild_unk_17EA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_17EA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);

    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_buy_privilege_and_point_info(ship_client_t* c, bb_guild_buy_privilege_and_point_info_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    print_payload((uint8_t*)pkt, len);
    return send_bb_guild_cmd(c, BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO);
    //return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_privilege_list(ship_client_t* c, bb_guild_privilege_list_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_privilege_list_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);

    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_buy_special_item(ship_client_t* c, bb_guild_buy_special_item_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_buy_special_item_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return send_bb_guild_cmd(c, BB_GUILD_BUY_SPECIAL_ITEM);
    //return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_unk_1BEA(ship_client_t* c, bb_guild_unk_1BEA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_1BEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);

    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int process_bb_guild_rank_list(ship_client_t* c, bb_guild_rank_list_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_rank_list_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    print_payload((uint8_t*)pkt, len);

    if (c->bb_guild->guild_data.guild_id <= 0)
        return 0;

    return shipgate_fw_bb(&ship->sg, pkt, c->bb_guild->guild_data.guild_id, c);
}

static int process_bb_guild_unk_1DEA(ship_client_t* c, bb_guild_unk_1DEA_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != sizeof(bb_guild_unk_1DEA_pkt)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        //return -1;
    }

    print_payload((uint8_t*)pkt, len);

    return shipgate_fw_bb(&ship->sg, pkt, 0, c);
}

static int bb_process_guild(ship_client_t* c, uint8_t* pkt) {
    bb_pkt_hdr_t* hdr = (bb_pkt_hdr_t*)pkt;
    bb_guild_pkt_pkt* gpkt = (bb_guild_pkt_pkt*)pkt;
    uint16_t type = LE16(hdr->pkt_type);
    uint16_t len = LE16(hdr->pkt_len);

    DBG_LOG("舰仓：BB 公会功能指令 0x%04X %s (长度%d)", type, c_cmd_name(type, 0), len);

    switch (type) {
    case BB_GUILD_CREATE:
        return process_bb_guild_create(c, (bb_guild_create_pkt*)pkt);

    case BB_GUILD_UNK_02EA:
        return process_bb_guild_unk_02EA(c, (bb_guild_unk_02EA_pkt*)pkt);

    case BB_GUILD_MEMBER_ADD:
        return process_bb_guild_member_add(c, (bb_guild_member_add_pkt*)pkt);

    case BB_GUILD_UNK_04EA:
        return process_bb_guild_unk_04EA(c, (bb_guild_unk_04EA_pkt*)pkt);

    case BB_GUILD_MEMBER_REMOVE:
        return process_bb_guild_member_remove(c, (bb_guild_member_remove_pkt*)pkt);

    case BB_GUILD_UNK_06EA:
        return process_bb_guild_06EA(c, (bb_guild_unk_06EA_pkt*)pkt);

    case BB_GUILD_CHAT:
        return process_bb_guild_member_chat(c, (bb_guild_member_chat_pkt*)pkt);

    case BB_GUILD_MEMBER_SETTING:
        return process_bb_guild_member_setting(c, (bb_guild_member_setting_pkt*)pkt);

    case BB_GUILD_UNK_09EA:
        return process_bb_guild_unk_09EA(c, (bb_guild_unk_09EA_pkt*)pkt);

    case BB_GUILD_UNK_0AEA:
        return process_bb_guild_unk_0AEA(c, (bb_guild_unk_0AEA_pkt*)pkt);

    case BB_GUILD_UNK_0BEA:
        return process_bb_guild_unk_0BEA(c, (bb_guild_unk_0BEA_pkt*)pkt);

    case BB_GUILD_UNK_0CEA:
        return process_bb_guild_unk_0CEA(c, (bb_guild_unk_0CEA_pkt*)pkt);

    case BB_GUILD_INVITE:
        return process_bb_guild_invite_0DEA(c, (bb_guild_invite_0DEA_pkt*)pkt);

    case BB_GUILD_UNK_0EEA:
        return process_bb_guild_unk_0EEA(c, (bb_guild_unk_0EEA_pkt*)pkt);

    case BB_GUILD_MEMBER_FLAG_SETTING:
        return process_bb_guild_member_flag_setting(c, (bb_guild_member_flag_setting_pkt*)pkt);

    case BB_GUILD_DISSOLVE:
        return process_bb_guild_dissolve(c, (bb_guild_dissolve_pkt*)pkt);

    case BB_GUILD_MEMBER_PROMOTE:
        return process_bb_guild_member_promote(c, (bb_guild_member_promote_pkt*)pkt);

    case BB_GUILD_INITIALIZATION_DATA:
        return process_bb_guild_unk_12EA(c, (bb_guild_unk_12EA_pkt*)pkt);

    case BB_GUILD_LOBBY_SETTING:
        return process_bb_guild_lobby_setting(c, (bb_guild_lobby_setting_pkt*)pkt);

    case BB_GUILD_MEMBER_TITLE:
        return process_bb_guild_member_tittle(c, (bb_guild_member_tittle_pkt*)pkt);

    case BB_GUILD_FULL_DATA:
        return process_bb_guild_full_data(c, (bb_guild_full_data_pkt*)pkt);

    case BB_GUILD_UNK_16EA:
        return process_bb_guild_unk_16EA(c, (bb_guild_unk_16EA_pkt*)pkt);

    case BB_GUILD_UNK_17EA:
        return process_bb_guild_unk_17EA(c, (bb_guild_unk_17EA_pkt*)pkt);

    case BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO:
        return process_bb_guild_buy_privilege_and_point_info(c, (bb_guild_buy_privilege_and_point_info_pkt*)pkt);

    case BB_GUILD_PRIVILEGE_LIST:
        return process_bb_guild_privilege_list(c, (bb_guild_privilege_list_pkt*)pkt);

    case BB_GUILD_BUY_SPECIAL_ITEM:
        return process_bb_guild_buy_special_item(c, (bb_guild_buy_special_item_pkt*)pkt);

    case BB_GUILD_UNK_1BEA:
        return process_bb_guild_unk_1BEA(c, (bb_guild_unk_1BEA_pkt*)pkt);

    case BB_GUILD_RANKING_LIST:
        return process_bb_guild_rank_list(c, (bb_guild_rank_list_pkt*)pkt);

    case BB_GUILD_UNK_1DEA:
        return process_bb_guild_unk_1DEA(c, (bb_guild_unk_1DEA_pkt*)pkt);

    default:
        UDONE_CPD(type, c->version, pkt);
        break;
    }
    return 0;
}

/* BB挑战模式功能 */
static int process_bb_challenge_01DF(ship_client_t* c, bb_challenge_01df_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != LE16(0x000C)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return 0;
}

static int process_bb_challenge_02DF(ship_client_t* c, bb_challenge_02df_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != LE16(0x000C)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return 0;
}

static int process_bb_challenge_03DF(ship_client_t* c, bb_challenge_03df_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != LE16(0x000C)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return 0;
}

static int process_bb_challenge_04DF(ship_client_t* c, bb_challenge_04df_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != LE16(0x000C)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return 0;
}

static int process_bb_challenge_05DF(ship_client_t* c, bb_challenge_05df_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != LE16(0x0024)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return 0;
}

static int process_bb_challenge_06DF(ship_client_t* c, bb_challenge_06df_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    if (len != LE16(0x0014)) {
        ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
        print_payload((uint8_t*)pkt, len);
        return -1;
    }

    print_payload((uint8_t*)pkt, len);
    return 0;
}

static int process_bb_challenge_07DF(ship_client_t* c, bb_challenge_07df_pkt* pkt) {
    uint16_t type = LE16(pkt->hdr.pkt_type);
    uint16_t len = LE16(pkt->hdr.pkt_len);

    //if (len != LE16(0x0014)) {
    //    ERR_LOG("无效 BB %s 数据包 (%d)", c_cmd_name(type, 0), len);
    //    print_payload((uint8_t*)pkt, len);
    //    return -1;
    //}

    print_payload((uint8_t*)pkt, len);
    return 0;
}

static int bb_process_challenge(ship_client_t* c, uint8_t* pkt) {
    bb_pkt_hdr_t* hdr = (bb_pkt_hdr_t*)pkt;
    bb_guild_pkt_pkt* gpkt = (bb_guild_pkt_pkt*)pkt;
    uint16_t type = LE16(hdr->pkt_type);
    uint16_t len = LE16(hdr->pkt_len);

    DBG_LOG("舰仓：挑战模式功能指令 0x%04X %s", type, c_cmd_name(type, 0));

    switch (type) {
    case BB_CHALLENGE_01DF:
        return process_bb_challenge_01DF(c, (bb_challenge_01df_pkt*)pkt);

    case BB_CHALLENGE_02DF:
        return process_bb_challenge_02DF(c, (bb_challenge_02df_pkt*)pkt);

    case BB_CHALLENGE_03DF:
        return process_bb_challenge_03DF(c, (bb_challenge_03df_pkt*)pkt);

    case BB_CHALLENGE_04DF:
        return process_bb_challenge_04DF(c, (bb_challenge_04df_pkt*)pkt);

    case BB_CHALLENGE_05DF:
        return process_bb_challenge_05DF(c, (bb_challenge_05df_pkt*)pkt);

    case BB_CHALLENGE_06DF:
        return process_bb_challenge_06DF(c, (bb_challenge_06df_pkt*)pkt);

    case BB_CHALLENGE_07DF:
        return process_bb_challenge_07DF(c, (bb_challenge_07df_pkt*)pkt);

    default:
        UDONE_CPD(type, c->version, pkt);
        break;
    }
    return 0;
}

typedef void (*process_command_t)(ship_t s, ship_client_t* c,
    uint16_t command, uint32_t flag, uint8_t* data);

int bb_process_pkt(ship_client_t* c, uint8_t* pkt) {
    bb_pkt_hdr_t* hdr = (bb_pkt_hdr_t*)pkt;
    uint16_t type = LE16(hdr->pkt_type);
    uint16_t len = LE16(hdr->pkt_len);
    uint32_t flags = LE32(hdr->flags);

#ifdef DEBUG
    DBG_LOG("舰仓：BB处理数据 指令 0x%04X %s 长度 %d 字节 标志 %d GC %u",
        type, c_cmd_name(type, 0), len, flags, c->guildcard);
    //print_payload((unsigned char*)pkt, len);
#endif // DEBUG

    /* 整合为综合指令集 */
    switch (type & 0x00FF) {
        /* 0x00DF 挑战模式 */
    case BB_CHALLENGE_DF:
        return bb_process_challenge(c, pkt);

        /* 0x00EA 公会功能 */
    case BB_GUILD_COMMAND:
        return bb_process_guild(c, pkt);

    default:
        break;
    }

    switch (type) {
        /* 0x0005 5*/
    case BURSTING_TYPE:
        /* If we've already gotten one of these, disconnect the client. */
        if (c->flags & CLIENT_FLAG_GOT_05) {
            c->flags |= CLIENT_FLAG_DISCONNECTED;
        }

        c->flags |= CLIENT_FLAG_GOT_05;
        //c->flags |= CLIENT_FLAG_DISCONNECTED;

        //DBG_LOG("BURSTING_TYPE: 0x%02X\n", type);

        send_lobby_pkt(c->cur_lobby, c, build_guild_full_data_pkt(c), 1);
        return 0;

        /* 0x0006 6*/
    case CHAT_TYPE:
        if(time_check(c->cmd_cooldown[type], 2))
            return bb_process_chat(c, (bb_chat_pkt*)pkt);
        else
            return send_txt(c, "%s",
                __(c, "\tE\tC6说话太快了哟~."));

        /* 0x0008 8*/
    case GAME_LIST_TYPE:
        return send_game_list(c, c->cur_block);

        /* 0x0009 9*/
    case INFO_REQUEST_TYPE:
        return bb_process_info_req(c, (bb_select_pkt*)pkt);

        /* 0x0010 16*/
    case MENU_SELECT_TYPE:
        return bb_process_menu(c, (bb_select_pkt*)pkt);

        /* 0x0013 19*/
    case QUEST_CHUNK_TYPE:
        //print_payload((unsigned char*)pkt, len);
        /* Uhh... Ignore these for now, we've already sent it by the time we
           get this packet from the client.
           嗯…暂时忽略这些，当我们从客户端收到这个数据包时，我们已经发送了 */
        return 0;

        /* 0x001D 29*/
    case PING_TYPE:
        return bb_process_ping(c);

        /* 0x001F 31*/
    case LOBBY_INFO_TYPE:
        return send_info_list(c, ship);

        /* 0x0040 64*/
    case GUILD_SEARCH_TYPE:
        if (time_check(c->cmd_cooldown[type], 1))
            return bb_process_guild_search(c, (bb_guild_search_pkt*)pkt);
        else
            return 0;

        /* 0x0044 68*/
    case QUEST_FILE_TYPE:
        //print_payload((unsigned char*)pkt, len);
        /* Uhh... Ignore these for now, we've already sent it by the time we
           get this packet from the client.
           嗯…暂时忽略这些，当我们从客户端收到这个数据包时，我们已经发送了 */
        return 0;

        /* 0x0060 96*/
    case GAME_COMMAND0_TYPE:
        return subcmd_bb_handle_bcast_o(c, (subcmd_bb_pkt_t*)pkt);

        /* 0x0061 97*/
    case CHAR_DATA_TYPE:
        return bb_process_char(c, (bb_char_data_pkt*)pkt);

        /* 0x0062 98*/
    case GAME_COMMAND2_TYPE:
        /* 0x006C 108*/
    case GAME_COMMANDC_TYPE:
        /* 0x006D 109*/
    case GAME_COMMANDD_TYPE:
        return subcmd_bb_handle_one(c, (subcmd_bb_pkt_t*)pkt);

        /* 0x006F 111*/
    case DONE_BURSTING_TYPE:
        return bb_process_done_burst(c, (bb_done_burst_pkt*)pkt);

        /* 0x016F 367*/
    case DONE_BURSTING_TYPE01:
        return bb_process_done_quest_burst(c, (bb_done_quest_burst_pkt*)pkt);

        /* 0x0081 129*/
    case SIMPLE_MAIL_TYPE:
        return bb_process_mail(c, (simple_mail_pkt*)pkt);

        /* 0x0084 132*/
    case LOBBY_CHANGE_TYPE:
        return bb_process_change_lobby(c, (bb_select_pkt*)pkt);

        /* 0x0089 137*/
    case LOBBY_ARROW_CHANGE_TYPE:
        return bb_process_arrow(c, (uint8_t)flags);

        /* 0x008A 138*/
    case LOBBY_NAME_TYPE:
        return send_lobby_name(c, c->cur_lobby);

        /* 0x0093 147*/
    case LOGIN_93_TYPE:
        return bb_process_login(c, (bb_login_93_pkt*)pkt);

        /* 0x0098 152*/
    case LEAVE_GAME_PL_DATA_TYPE:
        return bb_process_char(c, (bb_char_data_pkt*)pkt);

        /* 0x00A0 160*/
    case SHIP_LIST_TYPE:
        return send_ship_list(c, ship, ship->cfg->menu_code);

        /* 0x00A1 161*/
    case BLOCK_LIST_REQ_TYPE:
        return send_block_list(c, ship);

        /* 0x00A2 162*/
    case QUEST_LIST_TYPE:
        return process_bb_qlist(c);

        /* 0x00A9 169*/
    case QUEST_END_LIST_TYPE:
        return process_bb_qlist_end(c);

    case QUEST_STATS_TYPE:
        return process_bb_update_quest_stats(c, (bb_update_quest_stats_pkt*)pkt);

        /* 0x00AC 172*/
    case QUEST_LOAD_DONE_TYPE:
        return process_bb_qload_done(c);
        /* XXXX: This isn't right... we need to synchronize this. */
        //return send_simple(c, QUEST_LOAD_DONE_TYPE, 0);

        /* 0x00C0 192*/
    case CHOICE_OPTION_TYPE:
        //print_payload((unsigned char*)pkt, len);
        //return 0;
        return send_choice_search(c);

        /* 0x00C1 193*/
    case GAME_CREATE_TYPE:
        return bb_process_game_create(c, (bb_game_create_pkt*)pkt);

        /* 0x00C6 198*/
    case BLACKLIST_TYPE:
        return bb_process_blacklist(c, (bb_blacklist_update_pkt*)pkt);

        /* 0x00C7 199*/
    case AUTOREPLY_SET_TYPE:
        return client_set_autoreply(c, ((bb_autoreply_set_pkt*)pkt)->msg,
            len - 8);

        /* 0x00C8 200*/
    case AUTOREPLY_CLEAR_TYPE:
        return client_disable_autoreply(c);

        /* 0x00D0 208*/
    case TRADE_0_TYPE:
        return bb_process_trade(c, (bb_trade_D0_D3_pkt*)pkt);

        /* 0x00D2 210*/
    case TRADE_2_TYPE:
        return bb_process_trade_excute(c, (bb_trade_D0_D3_pkt*)pkt);

        /* 0x00D4 212*/
    case TRADE_4_TYPE:
        return bb_process_trade_error(c, (bb_trade_D0_D3_pkt*)pkt);

        /* 0x00D8 216*/
    case INFOBOARD_TYPE:
        return send_infoboard(c, c->cur_lobby);

        /* 0x00D9 217*/
    case INFOBOARD_WRITE_TYPE:
        return bb_process_infoboard(c, (bb_write_info_pkt*)pkt);

        /* 0x00E7 231*/
    case BB_FULL_CHARACTER_TYPE:
        //UDONE_CPD(type,pkt);
        //print_payload((unsigned char*)pkt, len);
        /* Ignore for now... */
        return bb_process_full_char(c, (bb_full_char_pkt*)pkt);

        /* 0x01ED 493*/
    case BB_UPDATE_OPTION_FLAGS:
        return bb_process_opt_flags(c, (bb_options_update_option_pkt*)pkt);

        /* 0x02ED 749*/
    case BB_UPDATE_SYMBOL_CHAT:
        return bb_process_symbols(c, (bb_options_update_symbol_chats_pkt*)pkt);

        /* 0x03ED 1005*/
    case BB_UPDATE_SHORTCUTS:
        return bb_process_shortcuts(c, (bb_options_update_chat_shortcuts_pkt*)pkt);

        /* 0x04ED 1261*/
    case BB_UPDATE_KEY_CONFIG:
        return bb_process_keys(c, (bb_options_update_key_config_pkt*)pkt);

        /* 0x05ED 1517*/
    case BB_UPDATE_PAD_CONFIG:
        return bb_process_pad(c, (bb_options_update_pad_config_pkt*)pkt);

        /* 0x06ED 1773*/
    case BB_UPDATE_TECH_MENU:
        return bb_process_techs(c, (bb_options_update_tech_menu_pkt*)pkt);

        /* 0x07ED 2029*/
    case BB_UPDATE_CONFIG:
        return bb_process_config(c, (bb_options_update_customize_pkt*)pkt);

        /* 0x08ED 2285*/
    case BB_UPDATE_C_MODE_CONFIG:
        return bb_process_ch_mode_config(c, (bb_options_update_challenge_battle_config_pkt*)pkt);

        /* 0x04E8 1256*/
    case BB_ADD_GUILDCARD_TYPE:
        /* 0x05E8 1512*/
    case BB_DEL_GUILDCARD_TYPE:
        /* 0x07E8 2024*/
    case BB_ADD_BLOCKED_USER_TYPE:
        /* 0x08E8 2280*/
    case BB_DEL_BLOCKED_USER_TYPE:
        /* 0x09E8 2536*/
    case BB_SET_GUILDCARD_COMMENT_TYPE:
        /* 0x0AE8 2792*/
    case BB_SORT_GUILDCARD_TYPE:
        /* Let the shipgate deal with these... */
        return shipgate_fw_bb(&ship->sg, pkt, 0, c);

        /* 0x06E8 1768*/
    case BB_SET_GUILDCARD_TEXT_TYPE:
        return bb_set_guild_text(c, (bb_guildcard_set_txt_pkt*)pkt);

    default:
        DBG_LOG("BB未知数据! 指令 0x%04X", type);
        UNK_CPD(type, c->version, (uint8_t*)pkt);
        //if (!script_execute_pkt(ScriptActionUnknownBlockPacket, c, pkt,
        //    len)) {
        //    DBG_LOG("BB未知数据! 指令 0x%04X", type);
        //    print_payload((unsigned char*)pkt, len);
        //    return -3;
        //}
        return 0;
    }
}
