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

/* 中途页面 用于区分GM指令的 */
#include "commands_gm.h"

typedef struct command {
    char trigger[10];
    int (*hnd)(ship_client_t *c, const char *params);
} command_t;

/* 用法: /minlvl level */
static int handle_min_level(ship_client_t *c, const char *params) {
    int lvl;
    lobby_t *l = c->cur_lobby;

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure the requester is the leader of the team. */
    if(l->leader_id != c->client_id) {
        return send_txt(c, "%s",
                        __(c, "\tE\tC7只有在游戏房间的房主才可以使用."));
    }

    /* Figure out the level requested */
    errno = 0;
    lvl = (int)strtoul(params, NULL, 10);

    if(errno || lvl > MAX_PLAYER_LEVEL || lvl < 1) {
        /* Send a message saying invalid level */
        return send_txt(c, "%s", __(c, "\tE\tC7无效等级."));
    }

    /* Make sure the requested level is greater than or equal to the value for
       the game's difficulty. */
    if(lvl < game_required_level[l->difficulty]) {
        return send_txt(c, "%s",
                        __(c, "\tE\tC7Invalid level for this difficulty."));
    }

    /* Make sure the requested level is less than or equal to the game's maximum
       level. */
    if(lvl > (int)l->max_level) {
        return send_txt(c, "%s",
                        __(c, "\tE\tC7Minimum level must be <= maximum."));
    }

    /* Set the value in the structure, and be on our way. */
    l->min_level = lvl;

    return send_txt(c, "%s", __(c, "\tE\tC7Minimum level set."));
}

/* 用法: /maxlvl level */
static int handle_max_level(ship_client_t *c, const char *params) {
    int lvl;
    lobby_t *l = c->cur_lobby;

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure the requester is the leader of the team. */
    if(l->leader_id != c->client_id) {
        return send_txt(c, "%s",
                        __(c, "\tE\tC7只有在游戏房间的房主才可以使用."));
    }

    /* Figure out the level requested */
    errno = 0;
    lvl = (int)strtoul(params, NULL, 10);

    if(errno || lvl > MAX_PLAYER_LEVEL || lvl < 1) {
        /* Send a message saying invalid level */
        return send_txt(c, "%s", __(c, "\tE\tC7无效等级."));
    }

    /* Make sure the requested level is greater than or equal to the value for
       the game's minimum level. */
    if(lvl < (int)l->min_level) {
        return send_txt(c, "%s",
                        __(c, "\tE\tC7Maximum level must be >= minimum."));
    }

    /* Set the value in the structure, and be on our way. */
    l->max_level = lvl;

    return send_txt(c, "%s", __(c, "\tE\tC7Maximum level set."));
}

/* 用法: /save slot */
static int handle_save(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    uint32_t slot = 0;
    uint16_t data_len = 0;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Make sure that the requester is in a lobby, not a team */
    if(l->type != LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));
    }

    ///* Not valid for Blue Burst clients */
    //if(c->version == CLIENT_VERSION_BB) {
    //    return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst 不支持该指令."));
    //}

    /* Figure out the slot requested */
    errno = 0;
    if (!params || params[0] == '\0')
        slot = c->sec_data.slot;
    else
        slot = (uint32_t)strtoul(params, NULL, 10);

    if(errno || slot > 4 || slot < 1) {
        /* Send a message saying invalid slot */
        return send_txt(c, "%s", __(c, "\tE\tC7无效角色插槽."));
    }

    if (c->version == CLIENT_VERSION_BB) {
        /* Is it a Blue Burst character or not? */
        if (data_len > 1056) {
            data_len = PSOCN_STLENGTH_BB_DB_CHAR;

            /* Send the character data to the shipgate */
            if (shipgate_send_cdata(&ship->sg, c->guildcard, slot, c->bb_pl, data_len,
                c->cur_block->b)) {
                /* Send a message saying we couldn't save */
                return send_txt(c, "%s", __(c, "\tE\tC7无法保存角色数据."));
            }

            /* 将玩家选项数据存入数据库 */
            if (shipgate_send_bb_opts(&ship->sg, c)) {
                /* Send a message saying we couldn't save */
                return send_txt(c, "%s", __(c, "\tE\tC7无法保存角色选项数据."));
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
            return send_txt(c, "%s", __(c, "\tE\tC7无法保存角色数据."));
        }
    }

    /* An error or success message will be sent when the shipgate gets its
       response. */
    send_txt(c, "%s", __(c, "\tE\tC7保存角色完成."));
    return 0;
}

/* 用法: /restore slot */
static int handle_restore(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    uint32_t slot;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Make sure that the requester is in a lobby, not a team */
    if(l->type != LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));
    }

    /* Not valid for Blue Burst clients */
    if(c->version == CLIENT_VERSION_BB) {
        return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst 不支持该指令."));
    }

    /* Figure out the slot requested */
    errno = 0;
    slot = (uint32_t)strtoul(params, NULL, 10);

    if(errno || slot > 4 || slot < 1) {
        /* Send a message saying invalid slot */
        return send_txt(c, "%s", __(c, "\tE\tC7无效槽位."));
    }

    /* Adjust so we don't go into the Blue Burst character data */
    slot += 4;

    /* Send the request to the shipgate. */
    if(shipgate_send_creq(&ship->sg, c->guildcard, slot)) {
        /* Send a message saying we couldn't request */
        return send_txt(c, "%s",
                        __(c, "\tE\tC7无法获取角色数据."));
    }

    return 0;
}

/* 用法: /bstat */
static int handle_bstat(ship_client_t *c, const char *params) {
    block_t *b = c->cur_block;
    int games, players;

    /* Grab the stats from the block structure */
    pthread_rwlock_rdlock(&b->lobby_lock);
    games = b->num_games;
    pthread_rwlock_unlock(&b->lobby_lock);

    pthread_rwlock_rdlock(&b->lock);
    players = b->num_clients;
    pthread_rwlock_unlock(&b->lock);

    /* Fill in the string. */
    return send_txt(c, "\tE\tC7舰仓%02d:\n%d %s\n%d %s", b->b, players,
                    __(c, "玩家"), games, __(c, "房间"));
}

/* 用法 /arrow color_number */
static int handle_arrow(ship_client_t *c, const char *params) {
    int i;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Set the arrow color and send the packet to the lobby. */
    i = atoi(params);
    c->arrow_color = i;

    send_txt(c, "%s", __(c, "\tE\tC7箭头标记颜色已改变."));

    return send_lobby_arrows(c->cur_lobby);
}

/* 用法 /login username password */
static int handle_login(ship_client_t *c, const char *params) {
    char username[32] = { 0 }, password[32] = { 0 };
    int len = 0;
    const char *ch = params;

    /* Make sure the user isn't doing something stupid. */
    if(!*params) {
        return send_txt(c, "%s", __(c, "\tE\tC7请输入:\n"
                                    "/login 用户名 密码\n"
                                    "完成GM登录验证."));
    }

    /* Copy over the username/password. */
    while(*ch != ' ' && len < 32) {
        username[len++] = *ch++;
    }

    if(len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7无效用户名."));

    username[len] = '\0';

    len = 0;
    ++ch;

    while(*ch != ' ' && *ch != '\0' && len < 32) {
        password[len++] = *ch++;
    }

    if(len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7无效密码."));

    password[len] = '\0';

    /* We'll get success/failure later from the shipgate. */
    return shipgate_send_usrlogin(&ship->sg, c->guildcard, c->cur_block->b,
                                  username, password, 0, 0);
}

/* 用法: /passwd newpass */
static int handle_passwd(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int len = strlen(params), i;

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure the requester is the leader of the team. */
    if(l->leader_id != c->client_id) {
        return send_txt(c, "%s",
                        __(c, "\tE\tC7只有在游戏房间的房主才可以使用."));
    }

    /* Check the length of the provided password. */
    if(len > 16) {
        return send_txt(c, "%s", __(c, "\tE\tC7Password too long."));
    }

    /* Make sure the password only has ASCII characters */
    for(i = 0; i < len; ++i) {
        if(params[i] < 0x20 || params[i] >= 0x7F) {
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

/* 用法: /lname newname */
static int handle_lname(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type == LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure the requester is the leader of the team. */
    if(l->leader_id != c->client_id) {
        return send_txt(c, "%s",
                        __(c, "\tE\tC7只有在游戏房间的房主才可以使用."));
    }

    /* Check the length of the provided lobby name. */
    if(strlen(params) > 16) {
        return send_txt(c, "%s", __(c, "\tE\tC7房间名称过长,请重新设置."));
    }

    pthread_mutex_lock(&l->mutex);

    /* Copy the new name in. */
    strcpy(l->name, params);

    pthread_mutex_unlock(&l->mutex);

    return send_txt(c, "%s\n%s", __(c, "\tE\tC7已设置房间新名称:"), l->name);
}

/* 用法: /bug */
static int handle_bug(ship_client_t *c, const char *params) {
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
    gcpkt.language = CLIENT_LANG_ENGLISH;
    gcpkt.section = 0;
    gcpkt.ch_class = 8;
    gcpkt.padding[0] = gcpkt.padding[1] = gcpkt.padding[2] = 0;

    sprintf(gcpkt.name, "%s", __(c, "错误报告"));
    sprintf(gcpkt.guildcard_desc, "%s", __(c, "向该GC上报一个错误报告."));

    send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7发送邮件至\n"
                         "'错误报告' 玩家并上报\n"
                         "一个BUG."));

    return sub62_06_dc(NULL, c, &gcpkt);
}

/* 用法: /gban:d guildcard reason */
static int handle_gban_d(ship_client_t *c, const char *params) {
    uint32_t gc;
    char *reason;

    /* Make sure the requester is a global GM. */
    if(!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Set the ban (86,400s = 1 day). */
    if(strlen(reason) > 1) {
        return global_ban(c, gc, 86400, reason + 1);
    }
    else {
        return global_ban(c, gc, 86400, NULL);
    }
}

/* 用法: /gban:w guildcard reason */
static int handle_gban_w(ship_client_t *c, const char *params) {
    uint32_t gc;
    char *reason;

    /* Make sure the requester is a global GM. */
    if(!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Set the ban (604,800s = 1 week). */
    if(strlen(reason) > 1) {
        return global_ban(c, gc, 604800, reason + 1);
    }
    else {
        return global_ban(c, gc, 604800, NULL);
    }
}

/* 用法: /gban:m guildcard reason */
static int handle_gban_m(ship_client_t *c, const char *params) {
    uint32_t gc;
    char *reason;

    /* Make sure the requester is a global GM. */
    if(!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Set the ban (2,592,000s = 30 days). */
    if(strlen(reason) > 1) {
        return global_ban(c, gc, 2592000, reason + 1);
    }
    else {
        return global_ban(c, gc, 2592000, NULL);
    }
}

/* 用法: /gban:p guildcard reason */
static int handle_gban_p(ship_client_t *c, const char *params) {
    uint32_t gc;
    char *reason;

    /* Make sure the requester is a global GM. */
    if(!GLOBAL_GM(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Set the ban (0xFFFFFFFF = forever (or close enough for now)). */
    if(strlen(reason) > 1) {
        return global_ban(c, gc, EMPTY_STRING, reason + 1);
    }
    else {
        return global_ban(c, gc, EMPTY_STRING, NULL);
    }
}

/* 用法: /legit [off] */
static int handle_legit(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    uint32_t v;
    iitem_t *item;
    psocn_limits_t *limits;
    int j, irv;

    /* Make sure the requester is in a lobby, not a team. */
    if(l->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));

    /* Figure out what version they're on. */
    switch(c->version) {
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
            return send_txt(c, "%s", __(c, "\tE\tC7合法模式不支持 Episode 3."));

        case CLIENT_VERSION_BB:
            v = ITEM_VERSION_BB;
            break;

        default:
            return -1;
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_LEGIT;
        return send_txt(c, "%s", __(c, "\tE\tC7所有新的房间的合法模式已关闭."));
    }

    /* XXXX: Select the appropriate limits file. */
    pthread_rwlock_rdlock(&ship->llock);
    if(!ship->def_limits) {
        pthread_rwlock_unlock(&ship->llock);
        return send_txt(c, "%s", __(c, "\tE\tC7当前舰船不支持合法模式."));
    }

    limits = ship->def_limits;

    /* Make sure the player qualifies for legit mode... */
    for(j = 0; j < c->pl->v1.character.inv.item_count; ++j) {
        item = (iitem_t *)&c->pl->v1.character.inv.iitems[j];
        irv = psocn_limits_check_item(limits, item, v);

        if(!irv) {
            SHIPS_LOG("Potentially non-legit item in legit mode:\n"
                  "%08x %08x %08x %08x", LE32(item->data.datal[0]),
                  LE32(item->data.datal[1]), LE32(item->data.datal[2]),
                  LE32(item->data.data2l));
            pthread_rwlock_unlock(&ship->llock);
            return send_txt(c, "%s", __(c, "\tE\tC7你的合法模式检查失败."));
        }
    }

    /* Set the flag and retain the limits list on the client. */
    c->flags |= CLIENT_FLAG_LEGIT;
    c->limits = retain(limits);

    pthread_rwlock_unlock(&ship->llock);

    return send_txt(c, "%s", __(c, "\tE\tC7下一个新建房间合法模式将会生效."));
}

/* 用法: /normal */
static int handle_normal(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int i;

    /* Lock the lobby mutex... we've got some work to do. */
    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure the requester is the leader of the team. */
    if(l->leader_id != c->client_id) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s",
                        __(c, "\tE\tC7只有在游戏房间的房主才可以使用."));
    }

    /* If we're not in legit mode, then this command doesn't do anything... */
    if(!(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7已经是合法模式,无法使用作弊模式."));
    }

    /* Clear the flag */
    l->flags &= ~(LOBBY_FLAG_LEGIT_MODE);

    /* Let everyone know legit mode has been turned off. */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            send_txt(l->clients[i], "%s",
                     __(l->clients[i], "\tE\tC7合法模式已关闭."));
        }
    }

    /* Unlock, we're done. */
    pthread_mutex_unlock(&l->mutex);

    return 0;
}

/* 用法: /shutdown minutes */
static int handle_shutdown(ship_client_t *c, const char *params) {
    uint32_t when;

    /* Make sure the requester is a local root. */
    if(!LOCAL_ROOT(c)) {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out when we're supposed to shut down. */
    errno = 0;
    when = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid time */
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7无效时间."));
    }

    /* Give everyone at least a minute */
    if(when < 1) {
        when = 1;
    }

    return schedule_shutdown(c, when, 0, send_msg);
}

/* 用法: /log guildcard */
static int handle_log(ship_client_t *c, const char *params) {
    uint32_t gc;
    block_t *b = c->cur_block;
    ship_client_t *i;
    int rv;

    /* Make sure the requester is a local root. */
    if(!LOCAL_ROOT(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Look for the requested user and start the log */
    TAILQ_FOREACH(i, b->clients, qentry) {
        /* Start logging them if we find them */
        if(i->guildcard == gc) {
            rv = pkt_log_start(i);

            if(!rv) {
                return send_txt(c, "%s", __(c, "\tE\tC7Logging started."));
            }
            else if(rv == -1) {
                return send_txt(c, "%s", __(c, "\tE\tC7The user is already\n"
                                            "being logged."));
            }
            else if(rv == -2) {
                return send_txt(c, "%s",
                                __(c, "\tE\tC7Cannot create log file."));
            }
        }
    }

    /* The person isn't here... There's nothing left to do. */
    return send_txt(c, "%s", __(c, "\tE\tC7Requested user not\nfound."));
}

/* 用法: /endlog guildcard */
static int handle_endlog(ship_client_t *c, const char *params) {
    uint32_t gc;
    block_t *b = c->cur_block;
    ship_client_t *i;
    int rv;

    /* Make sure the requester is a local root. */
    if(!LOCAL_ROOT(c)) {
        return get_gm_priv(c);
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Look for the requested user and end the log */
    TAILQ_FOREACH(i, b->clients, qentry) {
        /* Finish logging them if we find them */
        if(i->guildcard == gc) {
            rv = pkt_log_stop(i);

            if(!rv) {
                return send_txt(c, "%s", __(c, "\tE\tC7Logging ended."));
            }
            else if(rv == -1) {
                return send_txt(c, "%s", __(c,"\tE\tC7The user is not\n"
                                            "being logged."));
            }
        }
    }

    /* The person isn't here... There's nothing left to do. */
    return send_txt(c, "%s", __(c, "\tE\tC7Requested user not\nfound."));
}

/* 用法: /motd */
static int handle_motd(ship_client_t *c, const char *params) {
    return send_motd(c);
}

/* 用法: /friendadd guildcard nickname */
static int handle_friendadd(ship_client_t *c, const char *params) {
    uint32_t gc;
    char *nick;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &nick, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Make sure the nickname is valid. */
    if(!nick || nick[0] != ' ' || nick[1] == '\0') {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid Nickname."));
    }

    /* Send a request to the shipgate to do the rest */
    shipgate_send_friend_add(&ship->sg, c->guildcard, gc, nick + 1);

    /* Any further messages will be handled by the shipgate handler */
    return 0;
}

/* 用法: /frienddel guildcard */
static int handle_frienddel(ship_client_t *c, const char *params) {
    uint32_t gc;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Send a request to the shipgate to do the rest */
    shipgate_send_friend_del(&ship->sg, c->guildcard, gc);

    /* Any further messages will be handled by the shipgate handler */
    return 0;
}

/* 用法: /dconly [off] */
static int handle_dconly(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int i;

    /* Lock the lobby mutex... we've got some work to do. */
    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure the requester is the leader of the team. */
    if(l->leader_id != c->client_id) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s",
                        __(c, "\tE\tC7只有在游戏房间的房主才可以使用."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        l->flags &= ~LOBBY_FLAG_DCONLY;
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7Dreamcast-only mode off."));
    }

    /* Check to see if all players are on a Dreamcast version */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            if(l->clients[i]->version != CLIENT_VERSION_DCV1 &&
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

/* 用法: /v1only [off] */
static int handle_v1only(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int i;

    /* Lock the lobby mutex... we've got some work to do. */
    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure the requester is the leader of the team. */
    if(l->leader_id != c->client_id) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s",
                        __(c, "\tE\tC7只有在游戏房间的房主才可以使用."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        l->flags &= ~LOBBY_FLAG_V1ONLY;
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7V1-only mode off."));
    }

    /* Check to see if all players are on V1 */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            if(l->clients[i]->version != CLIENT_VERSION_DCV1) {
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

/* 用法: /ll */
static int handle_ll(ship_client_t *c, const char *params) {
    char str[512] = { 0 };
    lobby_t *l = c->cur_lobby;
    int i;
    ship_client_t *c2;
    size_t len;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    str[0] = '\t';
    str[1] = 'E';
    len = 2;

    if(LOCAL_GM(c)) {
        for(i = 0; i < l->max_clients; i += 2) {
            if((c2 = l->clients[i])) {
                len += snprintf(str + len, 511 - len, "%d: %s (%" PRIu32 ")   ",
                                i, c2->pl->v1.character.dress_data.gc_string, c2->guildcard);
            }
            else {
                len += snprintf(str + len, 511 - len, "%d: None   ", i);
            }

            if((i + 1) < l->max_clients) {
                if((c2 = l->clients[i + 1])) {
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
        for(i = 0; i < l->max_clients; i += 2) {
            if((c2 = l->clients[i])) {
                len += snprintf(str + len, 511 - len, "%d: %s   ", i,
                                c2->pl->v1.character.dress_data.gc_string);
            }
            else {
                len += snprintf(str + len, 511 - len, "%d: None   ", i);
            }

            if((i + 1) < l->max_clients) {
                if((c2 = l->clients[i + 1])) {
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

    /* 将数据包发送出去 */
    return send_msg(c, MSG_BOX_TYPE, "%s", str);
}

/* 用法 /npc number,client_id,follow_id */
static int handle_npc(ship_client_t *c, const char *params) {
    int count, npcnum, client_id, follow;
    uint8_t tmp[0x10] = { 0 };
    subcmd_pkt_t *p = (subcmd_pkt_t *)tmp;
    lobby_t *l = c->cur_lobby;

    pthread_mutex_lock(&l->mutex);

    if(l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* For now, limit only to lobbies with a single person in them... */
    if(l->num_clients != 1) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7只在单人游戏房间中有效."));
    }

    /* Also, make sure its not a battle or challenge lobby. */
    if(l->battle || l->challenge) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7对战模式和挑战模式无法使用."));
    }

    /* Make sure we're not in legit mode. */
    if((l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7合法模式下无法生效."));
    }

    /* Make sure we're not in a quest. */
    if((l->flags & LOBBY_FLAG_QUESTING)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7在任务时无法使用."));
    }

    /* Make sure we're on Pioneer 2. */
    if(c->cur_area != 0) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7只可在先驱者2号中使用."));
    }

    /* Figure out what we're supposed to do. */
    count = sscanf(params, "%d,%d,%d", &npcnum, &client_id, &follow);

    if(count == EOF || count == 0) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7无效 NPC 数据."));
    }

    /* Fill in sane defaults. */
    if(count == 1) {
        /* Find an open slot... */
        for(client_id = l->max_clients - 1; client_id >= 0; --client_id) {
            if(!l->clients[client_id]) {
                break;
            }
        }

        follow = c->client_id;
    }
    else if(count == 2) {
        follow = c->client_id;
    }

    /* Check the validity of arguments */
    if(npcnum < 0 || npcnum > 63) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7无效 NPC 序号."));
    }

    if(client_id < 0 || client_id >= l->max_clients || l->clients[client_id]) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid client ID given,\n"
                                    "or no open client slots."));
    }

    if(follow < 0 || follow >= l->max_clients || !l->clients[follow]) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid follow client given."));
    }

    /* This command, for now anyway, locks us down to one player mode. */
    l->flags |= LOBBY_FLAG_SINGLEPLAYER | LOBBY_FLAG_HAS_NPC;

    /* We're done with the lobby data now... */
    pthread_mutex_unlock(&l->mutex);

    /* 填充数据头 */
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
    send_pkt_dc(c, (dc_pkt_hdr_t *)p);
    return subcmd_handle_60(c, p);
}

/* 用法: /ignore client_id */
static int handle_ignore(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int id, i;
    ship_client_t *cl;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Copy over the ID. */
    i = sscanf(params, "%d", &id);

    if(i == EOF || i == 0 || id >= l->max_clients || id < 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid Client ID."));
    }

    /* Lock the lobby so we don't mess anything up in grabbing this... */
    pthread_mutex_lock(&l->mutex);

    /* Make sure there is such a client. */
    if(!(cl = l->clients[id])) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7No such client."));
    }

    /* Find an empty spot to put this in. */
    for(i = 0; i < CLIENT_IGNORE_LIST_SIZE; ++i) {
        if(!c->ignore_list[i]) {
            c->ignore_list[i] = cl->guildcard;
            pthread_mutex_unlock(&l->mutex);
            return send_txt(c, "%s %s\n%s %d", __(c, "\tE\tC7Ignoring"),
                            cl->pl->v1.character.dress_data.gc_string, __(c, "Entry"), i);
        }
    }

    /* 如果我们到了这里, the ignore list is full, report that to the user... */
    pthread_mutex_unlock(&l->mutex);
    return send_txt(c, "%s", __(c, "\tE\tC7Ignore list full."));
}

/* 用法: /unignore entry_number */
static int handle_unignore(ship_client_t *c, const char *params) {
    int id, i;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Copy over the ID */
    i = sscanf(params, "%d", &id);

    if(i == EOF || i == 0 || id >= CLIENT_IGNORE_LIST_SIZE || id < 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid Entry Number."));
    }

    /* Clear that entry of the ignore list */
    c->ignore_list[id] = 0;

    return send_txt(c, "%s", __(c, "\tE\tC7Ignore list entry cleared."));
}

/* 用法: /quit */
static int handle_quit(ship_client_t *c, const char *params) {
    c->flags |= CLIENT_FLAG_DISCONNECTED;
    return 0;
}

/* 用法: /cc [any ascii char] or /cc off */
static int handle_cc(ship_client_t *c, const char *params) {
    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Are we turning it off? */
    if(!strcmp(params, "off")) {
        c->cc_char = 0;
        return send_txt(c, "%s", __(c, "\tE\tC7Color Chat off."));
    }

    /* Make sure they only gave one character */
    if(strlen(params) != 1) {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid trigger character."));
    }

    /* Set the char in the client struct */
    c->cc_char = params[0];
    return send_txt(c, "%s", __(c, "\tE\tC7Color Chat on."));
}

/* 用法: /qlang [2 character language code] */
static int handle_qlang(ship_client_t *c, const char *params) {
    int i;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Make sure they only gave one character */
    if(strlen(params) != 2) {
        return send_txt(c, "%s", __(c, "\tE\tC7无效语言代码."));
    }

    /* Look for the specified language code */
    for(i = 0; i < CLIENT_LANG_ALL; ++i) {
        if(!strcmp(language_codes[i], params)) {
            c->q_lang = i;

#ifdef DEBUG

            DBG_LOG("语言 %d %d", c->q_lang, language_codes[i]);

#endif // DEBUG

            shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
                                   USER_OPT_QUEST_LANG, 1, &c->q_lang);

            return send_txt(c, "%s", __(c, "\tE\tC7任务语言已设置."));
        }
    }

    return send_txt(c, "%s", __(c, "\tE\tC7无效语言代码."));
}

/* 用法: /friends page */
static int handle_friends(ship_client_t *c, const char *params) {
    block_t *b = c->cur_block;
    uint32_t page;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Figure out the user requested */
    errno = 0;
    page = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid page number */
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid Page."));
    }

    /* Send the request to the shipgate */
    return shipgate_send_frlist_req(&ship->sg, c->guildcard, b->b, page * 5);
}

/* 用法: /ver */
static int handle_ver(ship_client_t *c, const char *params) {
    return send_txt(c, "%s: %s\n%s: %s", __(c, "\tE\tC7Git Build"),
                    "1", __(c, "Changeset"), "1");
}

/* 用法: /restart minutes */
static int handle_restart(ship_client_t *c, const char *params) {
    uint32_t when;

    /* Make sure the requester is a local root. */
    if(!LOCAL_ROOT(c)) {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out when we're supposed to shut down. */
    errno = 0;
    when = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid time */
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7时间参数无效."));
    }

    /* Give everyone at least a minute */
    if(when < 1) {
        when = 1;
    }

    return schedule_shutdown(c, when, 1, send_msg);
}

/* 用法: /search guildcard */
static int handle_search(ship_client_t *c, const char *params) {
    uint32_t gc;
    dc_guild_search_pkt dc = { 0 };
    bb_guild_search_pkt bb = { 0 };

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Hacky? Maybe a bit, but this will work just fine. */
    if(c->version == CLIENT_VERSION_BB) {
        bb.hdr.pkt_len = LE16(0x14);
        bb.hdr.pkt_type = LE16(GUILD_SEARCH_TYPE);
        bb.hdr.flags = 0;
        bb.player_tag = LE32(0x00010000);
        bb.gc_searcher = LE32(c->guildcard);
        bb.gc_target = LE32(gc);

        return block_process_pkt(c, (uint8_t *)&bb);
    }
    else if(c->version == CLIENT_VERSION_PC) {
        dc.hdr.pc.pkt_len = LE16(0x10);
        dc.hdr.pc.pkt_type = GUILD_SEARCH_TYPE;
        dc.hdr.pc.flags = 0;
        dc.player_tag = LE32(0x00010000);
        dc.gc_searcher = LE32(c->guildcard);
        dc.gc_target = LE32(gc);

        return block_process_pkt(c, (uint8_t *)&dc);
    }
    else {
        dc.hdr.dc.pkt_len = LE16(0x10);
        dc.hdr.dc.pkt_type = GUILD_SEARCH_TYPE;
        dc.hdr.dc.flags = 0;
        dc.player_tag = LE32(0x00010000);
        dc.gc_searcher = LE32(c->guildcard);
        dc.gc_target = LE32(gc);

        return block_process_pkt(c, (uint8_t *)&dc);
    }
}

/* 用法: /showmaps */
static int handle_showmaps(ship_client_t *c, const char *params) {
    char string[33] = { 0 };
    int i;
    lobby_t *l= c->cur_lobby;

    pthread_mutex_lock(&l->mutex);

    if(l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Build the string from the map list */
    for(i = 0; i < 32; ++i) {
        string[i] = l->maps[i] + '0';
    }

    string[32] = 0;

    pthread_mutex_unlock(&l->mutex);
    return send_txt(c, "%s\n%s", __(c, "\tE\tC7当前地图:"), string);
}

/* 用法: /restorebk */
static int handle_restorebk(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;

    /* Don't allow this if they have the protection flag on. */
    if(c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Make sure that the requester is in a lobby, not a team */
    if(l->type != LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));
    }

    c->game_info.guildcard = c->guildcard;
    c->game_info.slot = c->sec_data.slot;
    c->game_info.block = c->cur_block->b;
    c->game_info.c_version = c->version;

    /* Not valid for Blue Burst clients */
    if(c->version == CLIENT_VERSION_BB) {
        //return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst 不支持该指令."));
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
            __(c, "\tE\tC7无法请求恢复角色备用数据."));
    }

    return send_txt(c, "%s",
        __(c, "\tE\tC7恢复角色备用数据成功\n切换舰仓后生效."));

    return 0;
}

/* 用法 /enablebk */
static int handle_enablebk(ship_client_t *c, const char *params) {
    uint8_t enable = 1;

    /* Make sure the user is logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC4你需要登录"
                                    "才可以使用该指令."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
                           USER_OPT_ENABLE_BACKUP, 1, &enable);
    c->flags |= CLIENT_FLAG_AUTO_BACKUP;
    c->game_data->auto_backup = true;
    return send_txt(c, "%s", __(c, "\tE\tC6角色自动备份已开启."));
}

/* 用法 /disablebk */
static int handle_disablebk(ship_client_t *c, const char *params) {
    uint8_t enable = 0;

    /* Make sure the user is logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7你需要登录"
                                    "才可以使用该指令."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
                           USER_OPT_ENABLE_BACKUP, 1, &enable);
    c->flags &= ~(CLIENT_FLAG_AUTO_BACKUP);
    c->game_data->auto_backup = false;
    return send_txt(c, "%s", __(c, "\tE\tC4角色自动备份已关闭."));
}

/* 用法: /gcprotect [off] */
static int handle_gcprotect(ship_client_t *c, const char *params) {
    uint8_t enable = 1;

    /* Make sure they're logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7你需要登录"
                                    "才可以使用该指令."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
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

/* 用法 /trackkill [off] */
static int handle_trackkill(ship_client_t *c, const char *params) {
    uint8_t enable = 1;

    /* Make sure they're logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7你需要登录"
                                       "才可以使用该指令."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        enable = 0;
        shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
                               USER_OPT_TRACK_KILLS, 1, &enable);
        c->flags &= ~CLIENT_FLAG_TRACK_KILLS;
        return send_txt(c, "%s", __(c, "\tE\tC7Kill tracking disabled."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
                           USER_OPT_TRACK_KILLS, 1, &enable);
    c->flags |= CLIENT_FLAG_TRACK_KILLS;
    return send_txt(c, "%s", __(c, "\tE\tC7已启用杀敌数跟踪."));
}

/* 用法: /tlogin username token */
static int handle_tlogin(ship_client_t *c, const char *params) {
    char username[32] = { 0 }, token[32] = { 0 };
    int len = 0;
    const char *ch = params;

    /* Make sure the user isn't doing something stupid. */
    if(!*params) {
        return send_txt(c, "%s", __(c, "\tE\tC7You must specify\n"
                                    "your username and\n"
                                    "website token."));
    }

    /* Copy over the username/password. */
    while(*ch != ' ' && len < 32) {
        username[len++] = *ch++;
    }

    if(len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7无效用户名."));

    username[len] = '\0';

    len = 0;
    ++ch;

    while(*ch != ' ' && *ch != '\0' && len < 32) {
        token[len++] = *ch++;
    }

    if(len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid token."));

    token[len] = '\0';

    /* We'll get success/failure later from the shipgate. */
    return shipgate_send_usrlogin(&ship->sg, c->guildcard, c->cur_block->b,
                                  username, token, 1, 0);
}

/* 用法: /noevent */
static int handle_noevent(ship_client_t *c, const char *params) {
    if(c->version < CLIENT_VERSION_GC)
        return send_txt(c, "%s", __(c, "\tE\tC7游戏版本不支持."));

    /* Make sure that the requester is in a lobby, not a game */
    if(c->cur_lobby->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));

    return send_simple(c, LOBBY_EVENT_TYPE, LOBBY_EVENT_NONE);
}

/* 用法: /showpos */
static int handle_showpos(ship_client_t *c, const char *params) {
    return send_txt(c, "\tE\tC7(%f, %f, %f)", c->x, c->y, c->z);
}

/* 用法: /info */
static int handle_info(ship_client_t *c, const char *params) {
    /* Don't let certain versions even try... */
    if(c->version == CLIENT_VERSION_EP3)
        return send_txt(c, "%s", __(c, "\tE\tC7Episode III 不支持该指令."));
    else if(c->version == CLIENT_VERSION_DCV1 &&
            (c->flags & CLIENT_FLAG_IS_NTE))
        return send_txt(c, "%s", __(c, "\tE\tC7DC NTE 不支持该指令."));
    else if((c->version == CLIENT_VERSION_GC ||
             c->version == CLIENT_VERSION_XBOX) &&
            !(c->flags & CLIENT_FLAG_GC_MSG_BOXES))
        return send_txt(c, "%s", __(c, "\tE\tC7游戏版本不支持."));

    return send_info_list(c, ship);
}

/* 用法 /autolegit [off] */
static int handle_autolegit(ship_client_t *c, const char *params) {
    uint8_t enable = 1;
    psocn_limits_t *limits;

    /* Make sure they're logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7你需要登录"
                                       "才可以使用该指令."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
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
    if(!ship->def_limits) {
        pthread_rwlock_unlock(&ship->llock);
        c->flags &= ~CLIENT_FLAG_ALWAYS_LEGIT;
        send_txt(c, "%s", __(c, "\tE\tC7Legit mode not\n"
                                "available on this\n"
                                "ship."));
        return 0;
    }

    limits = ship->def_limits;

    if(client_legit_check(c, limits)) {
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

/* 用法: /censor [off] */
static int handle_censor(ship_client_t *c, const char *params) {
    uint8_t enable = 1;
    pthread_mutex_lock(&c->mutex);

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
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

/* 用法: /xblink username token */
static int handle_xblink(ship_client_t *c, const char *params) {
    char username[32] = { 0 }, token[32] = { 0 };
    int len = 0;
    const char *ch = params;

    if(c->version != CLIENT_VERSION_XBOX)
        return send_txt(c, "%s", __(c, "\tE\tC7游戏版本不支持."));

    /* Make sure the user isn't doing something stupid. */
    if(!*params) {
        return send_txt(c, "%s", __(c, "\tE\tC7You must specify\n"
                                    "your username and\n"
                                    "website token."));
    }

    /* Copy over the username/password. */
    while(*ch != ' ' && len < 32) {
        username[len++] = *ch++;
    }

    if(len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7无效用户名."));

    username[len] = '\0';

    len = 0;
    ++ch;

    while(*ch != ' ' && *ch != '\0' && len < 32) {
        token[len++] = *ch++;
    }

    if(len == 32)
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid token."));

    token[len] = '\0';

    /* We'll get success/failure later from the shipgate. */
    return shipgate_send_usrlogin(&ship->sg, c->guildcard, c->cur_block->b,
                                  username, token, 1, TLOGIN_VER_XBOX);
}

/* 用法: /logme [off] */
static int handle_logme(ship_client_t *c, const char *params) {
#ifndef DEBUG
    return send_txt(c, "%s", __(c, "\tE\tC7无效指令."));
#else
    /* Make sure the requester has permission to do this */
    if(!IS_TESTER(c))
        return get_gm_priv(c);

    if(*params) {
        if(!strcmp(params, "off")) {
            pkt_log_stop(c);
            return send_txt(c, "%s", __(c, "\tE\tC7DEBUG日志结束."));
        }
        else {
            return send_txt(c, "%s", __(c, "\tE\tC7无效LOG参数."));
        }
    }
    else {
        pkt_log_start(c);
        return send_txt(c, "%s", __(c, "\tE\tC7DEBUG日志开始."));
    }
#endif /* DEBUG */
}

/* 用法: /clean [inv/bank] 用于清理背包 银行数据 急救*/
static int handle_clean(ship_client_t* c, const char* params) {

    if (c->version != CLIENT_VERSION_BB)
        return send_txt(c, "%s", __(c, "\tE\tC7游戏版本不支持."));

    if (*params) {
        if (!strcmp(params, "inv")) {

            int size = sizeof(c->bb_pl->character.inv.iitems) / PSOCN_STLENGTH_IITEM;

            for (int i = 0; i < size; i++) {
                memset(&c->bb_pl->character.inv.iitems[i], 0, PSOCN_STLENGTH_IITEM);
            }

            c->bb_pl->character.inv.item_count = 0;
            c->pl->bb.character.inv = c->bb_pl->character.inv;

            return send_txt(c, "%s", __(c, "\tE\tC4背包数据已清空."));
        }

        if (!strcmp(params, "bank")) {

            int size = sizeof(c->bb_pl->bank.bitems) / PSOCN_STLENGTH_BITEM;

            for (int i = 0; i < size; i++) {
                memset(&c->bb_pl->bank.bitems[i], 0, PSOCN_STLENGTH_IITEM);
            }

            c->bb_pl->bank.item_count = 0;
            c->bb_pl->bank.meseta = 0;

            return send_txt(c, "%s", __(c, "\tE\tC4银行数据已清空."));
        }

        return send_txt(c, "%s", __(c, "\tE\tC7无效清理参数."));
    }

    return send_txt(c, "%s", __(c, "\tE\tC6清空指令不正确\n参数为[inv/bank]."));
}

/* 用法: /bank */
static int handle_bank(ship_client_t* c, const char* params) {

    if (!c->bank_type) {
        c->bank_type = true;
        return send_txt(c, "%s", __(c, "\tE\tC8公共仓库."));
    }

    c->bank_type = false;
    return send_txt(c, "%s", __(c, "\tE\tC6角色仓库."));
}

/* 用法: /qr */
static int handle_quick_return(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    if (l->flags & LOBBY_FLAG_QUESTING) {
        return send_txt(c, "%s", __(c, "\tE\tC4无法在任务中使用该指令."));
    } else if (l->type == LOBBY_TYPE_GAME) {
        if(c->cur_area > 0)
            return send_warp(c, 0, true);
        else
            return send_txt(c, "%s", __(c, "\tE\tC6您已经在先驱者2号了."));
    }

    send_ship_list(c, ship, ship->cfg->menu_code);
    return send_txt(c, "%s", __(c, "\tE\tC4请选择你要前往的舰船."));
}

/* 用法: /lb */
static int handle_lobby(ship_client_t* c, const char* params) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_GAME) {
        c->game_data->lobby_return = true;
        return send_warp(c, 0, false);
    }

    send_block_list(c, ship);
    return send_txt(c, "%s", __(c, "\tE\tC4请选择你要前往的大厅."));
}

/* 用法: /pl */
static int handle_player_menu(ship_client_t* c, const char* params) {
    if (in_lobby(c)) {
        return send_bb_player_menu_list(c);
    }

    return send_txt(c, "%s", __(c, "\tE\tC4无法在房间使用该指令."));
}

static command_t cmds[] = {
    { "debug"    , handle_gmdebug       },
    { "swarp"    , handle_shipwarp      },
    { "warp"     , handle_warp          },
    { "kill"     , handle_kill          },
    { "minlvl"   , handle_min_level     },
    { "maxlvl"   , handle_max_level     },
    { "refresh"  , handle_refresh       },
    { "save"     , handle_save          },
    { "restore"  , handle_restore       },
    { "bstat"    , handle_bstat         },
    { "bcast"    , handle_bcast         },
    { "tmsg"     , handle_tmsg          },
    { "arrow"    , handle_arrow         },
    { "login"    , handle_login         },
    { "item"     , handle_item          },
    { "item1"    , handle_item1         },
    { "item2"    , handle_item2         },
    { "item3"    , handle_item3         },
    { "item4"    , handle_item4         },
    { "miitem"   , handle_miitem        },
    { "event"    , handle_event         },
    { "passwd"   , handle_passwd        },
    { "lname"    , handle_lname         },
    { "warpall"  , handle_warpall       },
    { "bug"      , handle_bug           },
    { "clinfo"   , handle_clinfo        },
    { "gban:d"   , handle_gban_d        },
    { "gban:w"   , handle_gban_w        },
    { "gban:m"   , handle_gban_m        },
    { "gban:p"   , handle_gban_p        },
    { "list"     , handle_list          },
    { "legit"    , handle_legit         },
    { "normal"   , handle_normal        },
    { "shutdown" , handle_shutdown      },
    { "log"      , handle_log           },
    { "endlog"   , handle_endlog        },
    { "motd"     , handle_motd          },
    { "friendadd", handle_friendadd     },
    { "frienddel", handle_frienddel     },
    { "dconly"   , handle_dconly        },
    { "v1only"   , handle_v1only        },
    { "forgegc"  , handle_forgegc       },
    { "invuln"   , handle_invuln        },
    { "inftp"    , handle_inftp         },
    { "smite"    , handle_smite         },
    { "tp"       , handle_teleport      },
    { "dbginv"   , handle_dbginv        },
    { "dbgbank"  , handle_dbgbank       },
    { "showdcpc" , handle_showdcpc      },
    { "allowgc"  , handle_allowgc       },
    { "ws"       , handle_ws            },
    { "ll"       , handle_ll            },
    { "npc"      , handle_npc           },
    { "stfu"     , handle_stfu          },
    { "unstfu"   , handle_unstfu        },
    { "ignore"   , handle_ignore        },
    { "unignore" , handle_unignore      },
    { "quit"     , handle_quit          },
    { "gameevent", handle_gameevent     },
    { "ban:d"    , handle_ban_d         },
    { "ban:w"    , handle_ban_w         },
    { "ban:m"    , handle_ban_m         },
    { "ban:p"    , handle_ban_p         },
    { "unban"    , handle_unban         },
    { "cc"       , handle_cc            },
    { "qlang"    , handle_qlang         },
    { "friends"  , handle_friends       },
    { "gbc"      , handle_gbc           },
    { "logout"   , handle_logout        },
    { "override" , handle_override      },
    { "ver"      , handle_ver           },
    { "restart"  , handle_restart       },
    { "search"   , handle_search        },
    { "gm"       , handle_gm            },
    { "maps"     , handle_maps          },
    { "showmaps" , handle_showmaps      },
    { "restorebk", handle_restorebk     },
    { "enablebk" , handle_enablebk      },
    { "disablebk", handle_disablebk     },
    { "exp"      , handle_exp           },
    { "level"    , handle_level         },
    { "sdrops"   , handle_sdrops        },
    { "gcprotect", handle_gcprotect     },
    { "trackinv" , handle_trackinv      },
    { "trackkill", handle_trackkill     },
    { "ep3music" , handle_ep3music      },
    { "tlogin"   , handle_tlogin        },
    { "dsdrops"  , handle_dsdrops       },
    { "noevent"  , handle_noevent       },
    { "lflags"   , handle_lflags        },
    { "cflags"   , handle_cflags        },
    { "showpos"  , handle_showpos       },
    { "t"        , handle_t             },    /* Short command = more precision. */
    { "info"     , handle_info          },
    { "quest"    , handle_quest         },
    { "autolegit", handle_autolegit     },
    { "censor"   , handle_censor        },
    { "teamlog"  , handle_teamlog       },
    { "eteamlog" , handle_eteamlog      },
    { "ib"       , handle_ib            },
    { "xblink"   , handle_xblink        },
    { "logme"    , handle_logme         },
    { "clean"    , handle_clean         },
    { "cheat"    , handle_cheat         },
    { "cmdc"     , handle_cmdcheck      },
    { "bank"     , handle_bank          },
    { "rsquest"  , handle_resetquest    },
    { "qr"       , handle_quick_return  },
    { "lb"       , handle_lobby         },
    { "pl"       , handle_player_menu   },
    { ""         , NULL                 }     /* End marker -- DO NOT DELETE */
};

static int command_call(ship_client_t *c, const char *txt, size_t len) {
    command_t *i = &cmds[0];
    char cmd[10] = { 0 }, params[512] = { 0 };
    const char *ch = txt + 3;           /* Skip the language code and '/'. */
    int clen = 0;

    /* Figure out what the command the user has requested is */
    while(*ch != ' ' && clen < 9 && *ch) {
        cmd[clen++] = *ch++;
    }

    cmd[clen] = '\0';

    /* Copy the params out for safety... */
    if (params) {
        if (!*ch) {
            memset(params, 0, len);
        }
        else {
            strcpy(params, ch + 1);
        }
    }else
        ERR_LOG("指令参数为空");

    /* Look through the list for the one we want */
    while (i->hnd) {
        /* If this is it, go ahead and handle it */
        if (!strcmp(cmd, i->trigger)) {
            return i->hnd(c, params);
        }

        i++;
    }

    /* Make sure a script isn't set up to respond to the user's command... */
    if (!script_execute(ScriptActionUnknownCommand, c, SCRIPT_ARG_PTR, c,
        SCRIPT_ARG_CSTRING, cmd, SCRIPT_ARG_CSTRING, params,
        SCRIPT_ARG_END)) {
        /* Send the user a message saying invalid command. */
        return send_txt(c, "%s", __(c, "\tE\tC7无效指令."));
    }

    return 0;
}

int command_parse(ship_client_t *c, dc_chat_pkt *pkt) {
    int len = LE16(pkt->hdr.dc.pkt_len), tlen = len - 12;
    size_t in, out;
    char *inptr;
    char *outptr;
    char buf[512] = { 0 };

    /* Convert the text to UTF-8. */
    in = tlen;
    out = tlen * 2;
    inptr = (char *)pkt->msg;
    outptr = buf;

    if(pkt->msg[0] == '\t' && pkt->msg[1] == 'J') {
        iconv(ic_sjis_to_utf8, &inptr, &in, &outptr, &out);
    }
    else {
        iconv(ic_8859_to_utf8, &inptr, &in, &outptr, &out);
    }

    /* Handle the command... */
    return command_call(c, buf, (tlen * 2) - out);
}

int wcommand_parse(ship_client_t *c, dc_chat_pkt *pkt) {
    int len = LE16(pkt->hdr.dc.pkt_len), tlen = len - 12;
    size_t in, out;
    char *inptr;
    char *outptr;
    char buf[512] = { 0 };

    /* Convert the text to UTF-8. */
    in = tlen;
    out = tlen * 2;
    inptr = (char *)pkt->msg;
    outptr = buf;
    iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &out);

    /* Handle the command... */
    return command_call(c, buf, (tlen * 2) - out);
}

int bbcommand_parse(ship_client_t *c, bb_chat_pkt *pkt) {
    int len = LE16(pkt->hdr.pkt_len), tlen = len - 16;
    size_t in, out;
    char *inptr;
    char *outptr;
    char buf[512] = { 0 };

    /* Convert the text to UTF-8. */
    in = tlen;
    out = tlen * 2;
    inptr = (char *)pkt->msg;
    outptr = buf;
    iconv(ic_utf16_to_utf8, &inptr, &in, &outptr, &out);

    /* Handle the command... */
    return command_call(c, buf, (tlen * 2) - out);
}
