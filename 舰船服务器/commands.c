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
#include "player_handle_iitem.h"
#include "bans.h"
#include "admin.h"
#include "ptdata.h"
#include "pmtdata.h"
#include "mapdata.h"
#include "rtdata.h"
#include "scripts.h"

extern int sub62_06_dc(ship_client_t *s, ship_client_t *d,
                     subcmd_dc_gcsend_t *pkt);

typedef struct command {
    char trigger[10];
    int (*hnd)(ship_client_t *c, const char *params);
} command_t;

/* 用法: /debug [0/1]*/
static int handle_gmdebug(ship_client_t* c, const char* params) {
    unsigned long param;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the floor requested */
    errno = 0;
    param = strtoul(params, NULL, 10);

    if (errno) {
        /* Send a message saying invalid area */
        return send_txt(c, "%s", __(c, "\tE\tC7未选择开启或关闭 1或0."));
    }

    if (param == 1) {
        c->game_data->gm_debug = 1;
        return send_txt(c, "%s", __(c, "\tE\tC6DEBUG模式开启."));
    }
    else {
        c->game_data->gm_debug = 0;
        return send_txt(c, "%s", __(c, "\tE\tC4DEBUG模式关闭."));
    }
}

/* 用法: /swarp area */
static int handle_shipwarp(ship_client_t* c, const char* params) {
    unsigned long area;
    lobby_t* l = c->cur_lobby;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Figure out the floor requested */
    errno = 0;
    area = strtoul(params, NULL, 10);

    if (errno) {
        /* Send a message saying invalid area */
        return send_txt(c, "%s", __(c, "\tE\tC7无效区域."));
    }

    if (area > 17) {
        /* Area too large, give up */
        return send_txt(c, "%s", __(c, "\tE\tC7无效区域."));
    }

    /* Send the person to the requested place */
    return send_warp(c, (uint8_t)area, false);
}

/* 用法: /warp area */
static int handle_warp(ship_client_t *c, const char *params) {
    unsigned long area;
    lobby_t *l = c->cur_lobby;

    ///* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Figure out the floor requested */
    errno = 0;
    area = strtoul(params, NULL, 10);

    if(errno) {
        /* Send a message saying invalid area */
        return send_txt(c, "%s", __(c, "\tE\tC7无效区域."));
    }

    if(area > 17) {
        /* Area too large, give up */
        return send_txt(c, "%s", __(c, "\tE\tC7无效区域."));
    }

    /* Send the person to the requested place */
    return send_warp(c, (uint8_t)area, true);
}

/* 用法: /warpall area */
static int handle_warpall(ship_client_t *c, const char *params) {
    unsigned long area;
    lobby_t *l = c->cur_lobby;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Figure out the floor requested */
    errno = 0;
    area = strtoul(params, NULL, 10);

    if(errno) {
        /* Send a message saying invalid area */
        return send_txt(c, "%s", __(c, "\tE\tC7无效区域."));
    }

    if(area > 17) {
        /* Area too large, give up */
        return send_txt(c, "%s", __(c, "\tE\tC7无效区域."));
    }

    /* Send the person to the requested place */
    return send_lobby_warp(l, (uint8_t)area);
}

/* 用法: /kill guildcard reason */
static int handle_kill(ship_client_t *c, const char *params) {
    uint32_t gc;
    char *reason;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC."));
    }

    /* Hand it off to the admin functionality to do the dirty work... */
    if(strlen(reason) > 1) {
        return kill_guildcard(c, gc, reason + 1);
    }
    else {
        return kill_guildcard(c, gc, NULL);
    }
}

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

/* 用法: /refresh [quests, gms, or limits] */
static int handle_refresh(ship_client_t *c, const char *params) {
    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Not valid for Blue Burst clients */
    if (c->version == CLIENT_VERSION_BB) {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7Blue Burst 不支持该指令."));
    }

    if(!strcmp(params, "quests")) {
        return refresh_quests(c, send_msg);
    }
    else if(!strcmp(params, "gms")) {
        /* Make sure the requester is a local root. */
        if(!LOCAL_ROOT(c)) {
            return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7权限不足."));
        }

        return refresh_gms(c, send_msg);
    }
    else if(!strcmp(params, "limits")) {
        return refresh_limits(c, send_msg);
    }
    else {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7Unknown item to refresh."));
    }
}

/* 用法: /save slot */
static int handle_save(ship_client_t *c, const char *params) {
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
        return send_txt(c, "%s", __(c, "\tE\tC7无效角色插槽."));
    }

    /* Adjust so we don't go into the Blue Burst character data */
    slot += 4;

    /* Send the character data to the shipgate */
    if(shipgate_send_cdata(&ship->sg, c->guildcard, slot, c->pl, 1052,
                           c->cur_block->b)) {
        /* Send a message saying we couldn't save */
        return send_txt(c, "%s", __(c, "\tE\tC7无法保存角色数据."));
    }

    /* An error or success message will be sent when the shipgate gets its
       response. */
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

/* 用法 /bcast message */
static int handle_bcast(ship_client_t *c, const char *params) {
    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    return broadcast_message(c, params, 1);
}
/* 用法 /tmsg typecode,message */
static int handle_tmsg(ship_client_t* c, const char* params) {
    uint16_t typecode = 0;
    char message[4096] = { 0 };
    int count;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(c)) {
        return send_msg(c, TEXT_MSG_TYPE, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%hu,%s", &typecode, &message[0]);

    if (count == EOF || count == 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7无效参数代码."));
    }

    return test_message(c, typecode, message, 1);
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

/* 用法 /item item1,item2,item3,item4 */
static int handle_item(ship_client_t *src, const char *params) {
    uint32_t item[4] = {0, 0, 0, 0};
    int count;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(src)) {
        return send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC7权限不足."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X,%X,%X,%X", &item[0], &item[1], &item[2],
        &item[3]);

    if(count == EOF || count == 0) {
        return send_txt(src, "%s", __(src, "\tE\tC7无效物品代码."));
    }

    clear_item(&src->new_item);

    /* Copy over the item data. */
    src->new_item.datal[0] = SWAP32(item[0]);
    src->new_item.datal[1] = SWAP32(item[1]);
    src->new_item.datal[2] = SWAP32(item[2]);
    src->new_item.data2l = SWAP32(item[3]);

    print_item_data(&src->new_item, src->version);

    return send_txt(src, "%s %s %s",
        __(src, "\tE\tC8物品:"),
        item_get_name(&src->new_item, src->version),
        __(src, "\tE\tC6 new_item 设置成功."));
}

/* 用法 /item4 item4 */
static int handle_item4(ship_client_t *src, const char *params) {
    uint32_t item;
    int count;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(src)) {
        return send_txt(src, "%s", __(src, "\tE\tC7权限不足."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X", &item);

    if(count == EOF || count == 0) {
        return send_txt(src, "%s", __(src, "\tE\tC7无效物品代码."));
    }

    src->new_item.data2l = LE32(item);

    return send_txt(src, "%s", __(src, "\tE\tC7next_item 设置成功."));
}

/* 用法: /makeitem */
static int handle_makeitem(ship_client_t* src, const char* params) {
    lobby_t* l = src->cur_lobby;
    iitem_t* iitem;
    subcmd_drop_stack_t dc = { 0 };
    subcmd_bb_drop_stack_t bb = { 0 };

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return send_txt(src, "%s", __(src, "\tE\tC7权限不足."));
    }

    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s", __(src, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure there's something set with /item */
    if (!src->new_item.datal[0]) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s\n%s", __(src, "\tE\tC7请先输入物品的ID."), 
            __(src, "\tE\tC7/item code1,code2,code3,code4."));
    }

    /* If we're on Blue Burst, add the item to the lobby's inventory first. */
    if (l->version == CLIENT_VERSION_BB) {
        iitem = add_new_litem_locked(l, &src->new_item, src->cur_area, src->x, src->z);

        if (!iitem) {
            pthread_mutex_unlock(&l->mutex);
            return send_txt(src, "%s", __(src, "\tE\tC7新物品空间不足."));
        }
    }
    else {
        ++l->item_player_id[src->client_id];
    }

    /* Generate the packet to drop the item */
    dc.hdr.pkt_type = GAME_COMMAND0_TYPE;
    dc.hdr.pkt_len = LE16(sizeof(subcmd_drop_stack_t));
    dc.hdr.flags = 0;

    dc.shdr.type = SUBCMD60_DROP_STACK;
    dc.shdr.size = 0x0A;
    dc.shdr.client_id = src->client_id;


    bb.hdr.pkt_len = LE16(sizeof(subcmd_bb_drop_stack_t));
    bb.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    bb.hdr.flags = 0;

    bb.shdr.type = SUBCMD60_DROP_STACK;
    bb.shdr.size = 0x09;
    bb.shdr.client_id = src->client_id;

    dc.area = LE16(src->cur_area);
    bb.area = LE32(src->cur_area);
    bb.x = dc.x = src->x;
    bb.z = dc.z = src->z;
    bb.data = dc.data = src->new_item;
    bb.data.item_id = dc.data.item_id = LE32((l->item_lobby_id - 1));
    bb.two = dc.two = LE32(0x00000002);

    /* Clear the set item */
    //clear_item(&src->new_item);

    /* Send the packet to everyone in the lobby */
    pthread_mutex_unlock(&l->mutex);

    switch (src->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        return lobby_send_pkt_dc(l, NULL, (dc_pkt_hdr_t*)&dc, 0);

    case CLIENT_VERSION_BB:
        return lobby_send_pkt_bb(l, NULL, (bb_pkt_hdr_t*)&bb, 0);

    default:
        return 0;
    }
}

/* 用法: /event number */
static int handle_event(ship_client_t *c, const char *params) {
    int event;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Grab the event number */
    event = atoi(params);

    if(event < 0 || event > 14) {
        return send_txt(c, "%s", __(c, "\tE\tC7无效节日事件."));
    }

    ship->lobby_event = event;
    update_lobby_event();

    return send_txt(c, "%s", __(c, "\tE\tC7节日事件设置成功."));
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
    gcpkt.hdr.pkt_type = GAME_COMMAND2_TYPE;
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

/* 用法 /clinfo client_id */
static int handle_clinfo(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int id, count;
    ship_client_t *cl;
    char ip[INET6_ADDRSTRLEN];

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%d", &id);

    if(count == EOF || count == 0 || id >= l->max_clients || id < 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7无效 Client ID."));
    }

    /* Make sure there is such a client. */
    if(!(cl = l->clients[id])) {
        return send_txt(c, "%s", __(c, "\tE\tC7No such client."));
    }

    /* Fill in the client's info. */
    my_ntop(&cl->ip_addr, ip);
    return send_txt(c, "\tE\tC7名称: %s\nIP: %s\nGC: %u\n%s Lv.%d",
                    cl->pl->v1.character.dress_data.guildcard_str.string, ip, cl->guildcard,
                    pso_class[cl->pl->v1.character.dress_data.ch_class].cn_name, cl->pl->v1.character.disp.level + 1);
}

/* 用法: /gban:d guildcard reason */
static int handle_gban_d(ship_client_t *c, const char *params) {
    uint32_t gc;
    char *reason;

    /* Make sure the requester is a global GM. */
    if(!GLOBAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
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
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
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
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
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
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
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

/* 用法: /list parameters (there's too much to put here) */
static int handle_list(ship_client_t *c, const char *params) {
    /* Make sure the requester is a local GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Pass off to the player list code... */
    return send_player_list(c, params);
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
            return send_txt(c, "%s", __(c, "\tE\tC7Not valid on Episode 3."));

        case CLIENT_VERSION_BB:
            return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst 不支持该指令."));

        default:
            return -1;
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_LEGIT;
        return send_txt(c, "%s", __(c, "\tE\tC7Legit mode off\n"
                                    "for any new teams."));
    }

    /* XXXX: Select the appropriate limits file. */
    pthread_rwlock_rdlock(&ship->llock);
    if(!ship->def_limits) {
        pthread_rwlock_unlock(&ship->llock);
        return send_txt(c, "%s", __(c, "\tE\tC7Legit mode not\n"
                                    "available on this\n"
                                    "ship."));
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
            return send_txt(c, "%s", __(c, "\tE\tC7You failed the legit "
                                           "check."));
        }
    }

    /* Set the flag and retain the limits list on the client. */
    c->flags |= CLIENT_FLAG_LEGIT;
    c->limits = retain(limits);

    pthread_rwlock_unlock(&ship->llock);

    return send_txt(c, "%s", __(c, "\tE\tC7Legit mode on\n"
                                "for your next team."));
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
        return send_txt(c, "%s", __(c, "\tE\tC7Already in normal mode."));
    }

    /* Clear the flag */
    l->flags &= ~(LOBBY_FLAG_LEGIT_MODE);

    /* Let everyone know legit mode has been turned off. */
    for(i = 0; i < l->max_clients; ++i) {
        if(l->clients[i]) {
            send_txt(l->clients[i], "%s",
                     __(l->clients[i], "\tE\tC7Legit mode deactivated."));
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
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
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
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
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

/* 用法: /forgegc guildcard name */
static int handle_forgegc(ship_client_t *c, const char *params) {
    uint32_t gc;
    char *name = NULL;
    subcmd_dc_gcsend_t gcpkt = { 0 };

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &name, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Make sure a name was given */
    if(!name || name[0] != ' ' || name[1] == '\0') {
        return send_txt(c, "%s", __(c, "\tE\tC7未获取到玩家名称."));
    }

    /* Forge the guildcard send */
    gcpkt.hdr.pkt_type = GAME_COMMAND2_TYPE;
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

/* 用法: /invuln [off] */
static int handle_invuln(ship_client_t *c, const char *params) {
    pthread_mutex_lock(&c->mutex);

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        pthread_mutex_unlock(&c->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_INVULNERABLE;
        pthread_mutex_unlock(&c->mutex);

        return send_txt(c, "%s", __(c, "\tE\tC7无敌 关闭."));
    }

    /* Set the flag since we're turning it on. */
    c->flags |= CLIENT_FLAG_INVULNERABLE;

    pthread_mutex_unlock(&c->mutex);
    return send_txt(c, "%s", __(c, "\tE\tC7无敌 开启."));
}

/* 用法: /inftp [off] */
static int handle_inftp(ship_client_t *c, const char *params) {
    pthread_mutex_lock(&c->mutex);

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        pthread_mutex_unlock(&c->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_INFINITE_TP;
        pthread_mutex_unlock(&c->mutex);

        return send_txt(c, "%s", __(c, "\tE\tC7无限 TP 关闭."));
    }

    /* Set the flag since we're turning it on. */
    c->flags |= CLIENT_FLAG_INFINITE_TP;

    pthread_mutex_unlock(&c->mutex);
    return send_txt(c, "%s", __(c, "\tE\tC7无限 TP 开启."));
}

/* 用法: /smite clientid hp tp */
static int handle_smite(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int count, id, hp, tp;
    ship_client_t *cl;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%d %d %d", &id, &hp, &tp);

    if(count == EOF || count < 3 || id >= l->max_clients || id < 0 || hp < 0 ||
       tp < 0 || hp > 2040 || tp > 2040) {
        return send_txt(c, "%s", __(c, "\tE\tC7无效参数.\n/smite clientid hp tp"));
    }

    pthread_mutex_lock(&l->mutex);

    /* Make sure there is such a client. */
    if(!(cl = l->clients[id])) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7未找到该客户端ID."));
    }

    /* Smite the client */
    count = 0;

    if(hp) {
        send_lobby_mod_stat(l, cl, SUBCMD60_STAT_HPDOWN, hp);
        ++count;
    }

    if(tp) {
        send_lobby_mod_stat(l, cl, SUBCMD60_STAT_TPDOWN, tp);
        ++count;
    }

    /* Finish up */
    pthread_mutex_unlock(&l->mutex);

    if(count) {
        send_txt(cl, "%s", __(c, "\tE\tC7You have been smitten."));
        return send_txt(c, "%s", __(c, "\tE\tC7Client smitten."));
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7Nothing to do."));
    }
}

/* 用法: /teleport client */
static int handle_teleport(ship_client_t *c, const char *params) {
    int client;
    lobby_t *l = c->cur_lobby;
    ship_client_t *c2;
    subcmd_teleport_t p2 = { 0 };

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Figure out the user requested */
    errno = 0;
    client = strtoul(params, NULL, 10);

    if(errno) {
        /* Send a message saying invalid client ID */
        return send_txt(c, "%s", __(c, "\tE\tC7无效客户端 ID."));
    }

    if(client > l->max_clients) {
        /* Client ID too large, give up */
        return send_txt(c, "%s", __(c, "\tE\tC7无效客户端 ID."));
    }

    if(!(c2 = l->clients[client])) {
        /* Client doesn't exist */
        return send_txt(c, "%s", __(c, "\tE\tC7无效客户端 ID."));
    }

    /* See if we need to warp first */
    if(c2->cur_area != c->cur_area) {
        /* Send the person to the other user's area */
        return send_warp(c, (uint8_t)c2->cur_area, true);
    }
    else {
        /* Now, set up the teleport packet */
        p2.hdr.pkt_type = GAME_COMMAND0_TYPE;
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
        return lobby_send_pkt_dc(l, NULL, (dc_pkt_hdr_t *)&p2, 0);
    }
}

static void dumpinv_internal(ship_client_t *src) {
    //char name[64];
    psocn_v1v2v3pc_char_t* player_v1 = { 0 };
    psocn_bb_char_t* player_bb = { 0 };
    int i;
    int v = src->version;

    if(v != CLIENT_VERSION_BB) {
        player_v1 = &src->pl->v1.character;

        if (src->mode)
            player_v1 = &src->mode_pl->nobb;

        ITEM_LOG("------------------------------------------------------------");
        ITEM_LOG("玩家: %s (%d:%d) 背包数据转储", 
            get_player_name(src->pl, src->version, false), 
            src->guildcard, src->sec_data.slot);
        ITEM_LOG("职业: %s 房间模式: %s", pso_class[player_v1->dress_data.ch_class].cn_name, src->mode ? "模式" : "普通");
        ITEM_LOG("背包物品数量: %u", player_v1->inv.item_count);

        for(i = 0; i < player_v1->inv.item_count; ++i) {
            print_iitem_data(&player_v1->inv.iitems[i], i, src->version);
        }
        ITEM_LOG("------------------------------------------------------------");
    }
    else {
        player_bb = &src->bb_pl->character;

        if (src->mode)
            player_bb = &src->mode_pl->bb;

        /*istrncpy16_raw(ic_utf16_to_gb18030, name, &src->bb_pl->character.name.char_name[0], 64,
            BB_CHARACTER_CHAR_NAME_WLENGTH);*/
        ITEM_LOG("------------------------------------------------------------");
        ITEM_LOG("玩家: %s (%d:%d) 背包数据转储", 
            get_player_name(src->pl, src->version, false), 
            src->guildcard, src->sec_data.slot);
        ITEM_LOG("职业: %s 房间模式: %s", pso_class[player_bb->dress_data.ch_class].cn_name, src->mode ? "模式" : "普通");
        ITEM_LOG("背包物品数量: %u", player_bb->inv.item_count);

        for (i = 0; i < player_bb->inv.item_count; ++i) {
            print_iitem_data(&player_bb->inv.iitems[i], i, src->version);
        }
        ITEM_LOG("------------------------------------------------------------");
    }
    
}

/* 用法: /dbginv [l/client_id/guildcard] */
static int handle_dbginv(ship_client_t* src, const char* params) {
    lobby_t* l = src->cur_lobby;
    lobby_item_t* j;
    int do_lobby;
    uint32_t client;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return send_txt(src, "%s", __(src, "\tE\tC7权限不足."));
    }

    do_lobby = params && !strcmp(params, "l");

    /* If the arguments say "lobby", then dump the lobby's inventory. */
    if (do_lobby) {
        pthread_mutex_lock(&l->mutex);

        if (l->type != LOBBY_TYPE_GAME || l->version != CLIENT_VERSION_BB) {
            pthread_mutex_unlock(&l->mutex);
            return send_txt(src, "%s", __(src, "\tE\tC7无效请求或非BB版本客户端."));
        }

        SHIPS_LOG("------------------------------------------------------------");
        SHIPS_LOG("房间大厅背包数据转储 %s (%" PRIu32 ")", l->name,
            l->lobby_id);

        TAILQ_FOREACH(j, &l->item_queue, qentry) {
            print_item_data(&j->data.data, src->version);
            //SHIPS_LOG("%08x: %08x %08x %08x %08x: %s",
            //    LE32(j->d.data.item_id), LE32(j->d.data.data_l[0]),
            //    LE32(j->d.data.data_l[1]), LE32(j->d.data.data_l[2]),
            //    LE32(j->d.data.data2_l), item_get_name(&j->d.data, l->version));
        }
        SHIPS_LOG("------------------------------------------------------------");

        pthread_mutex_unlock(&l->mutex);
    }
    /* If there's no arguments, then dump the caller's inventory. */
    else if (!params || params[0] == '\0') {
        dumpinv_internal(src);
    }
    /* Otherwise, try to parse the arguments. */
    else {
        /* Figure out the user requested */
        errno = 0;
        client = (int)strtoul(params, NULL, 10);

        if (errno) {
            return send_txt(src, "%s", __(src, "\tE\tC7无效目标."));
        }

        /* See if we have a client ID or a guild card number... */
        if (client < 12) {
            pthread_mutex_lock(&l->mutex);

            if (client >= 4 && l->type == LOBBY_TYPE_GAME) {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(src, "%s", __(src, "\tE\tC7无效 Client ID."));
            }

            if (l->clients[client]) {
                dumpinv_internal(l->clients[client]);
            }
            else {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(src, "%s", __(src, "\tE\tC7无效 Client ID."));
            }

            pthread_mutex_unlock(&l->mutex);
        }
        /* Otherwise, assume we're looking for a guild card number. */
        else {
            ship_client_t* target = block_find_client(src->cur_block, client);

            if (!target) {
                return send_txt(src, "%s", __(src, "\tE\tC7未找到请求的玩家."));
            }

            dumpinv_internal(target);
        }
    }

    return send_txt(src, "%s", __(src, "\tE\tC7已转储的背包数据到日志文件."));
}

static void dumpbank_internal(ship_client_t* c) {
    //char name[64];
    size_t i;
    int v = c->version;

    if (v == CLIENT_VERSION_BB) {
        psocn_bb_char_t* player = &c->bb_pl->character;

        //istrncpy16_raw(ic_utf16_to_gb18030, name, &c->bb_pl->character.name.char_name[0], 64,
        //    BB_CHARACTER_CHAR_NAME_WLENGTH);
        ITEM_LOG("////////////////////////////////////////////////////////////");
        ITEM_LOG("玩家 %s (%d:%d) 银行数据转储", get_player_name(c->pl, c->version, false), c->guildcard, c->sec_data.slot);
        ITEM_LOG("职业: %s 房间模式: %s", pso_class[player->dress_data.ch_class].cn_name, c->mode ? "模式" : "普通");
        ITEM_LOG("银行物品数量: %u", c->bb_pl->bank.item_count);

        for (i = 0; i < c->bb_pl->bank.item_count; ++i) {
            print_bitem_data(&c->bb_pl->bank.bitems[i], i, c->version);
        }
    }
    else {
        send_txt(c, "%s", __(c, "\tE\tC7仅限于BB使用."));
    }
}

/* 用法: /dbgbank [clientid/guildcard] */
static int handle_dbgbank(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    uint32_t client;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }
    
    if(!params || params[0] == '\0') {
        dumpbank_internal(c);
    }
    /* Otherwise, try to parse the arguments. */
    else {
        /* Figure out the user requested */
        errno = 0;
        client = (int)strtoul(params, NULL, 10);

        if(errno) {
            return send_txt(c, "%s", __(c, "\tE\tC7无效目标."));
        }

        /* See if we have a client ID or a guild card number... */
        if(client < 12) {
            pthread_mutex_lock(&l->mutex);

            if(client >= 4 && l->type == LOBBY_TYPE_GAME) {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(c, "%s", __(c, "\tE\tC7无效 Client ID."));
            }

            if(l->clients[client]) {
                dumpbank_internal(l->clients[client]);
            }
            else {
                pthread_mutex_unlock(&l->mutex);
                return send_txt(c, "%s", __(c, "\tE\tC7无效 Client ID."));
            }

            pthread_mutex_unlock(&l->mutex);
        }
        /* Otherwise, assume we're looking for a guild card number. */
        else {
            ship_client_t *target = block_find_client(c->cur_block, client);

            if(!target) {
                return send_txt(c, "%s", __(c, "\tE\tC7未找到请求的玩家."));
            }

            dumpbank_internal(target);
        }
    }

    return send_txt(c, "%s", __(c, "\tE\tC7已转储的背包数据到日志文件."));
}

/* 用法: /showdcpc [off] */
static int handle_showdcpc(ship_client_t *c, const char *params) {
    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Check if the client is on PSOGC */
    if(c->version != CLIENT_VERSION_GC) {
        return send_txt(c, "%s", __(c, "\tE\tC7Only valid on Gamecube."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_SHOW_DCPC_ON_GC;
        return send_txt(c, "%s", __(c, "\tE\tC7DC/PC games hidden."));
    }

    /* Set the flag, and tell the client that its been set. */
    c->flags |= CLIENT_FLAG_SHOW_DCPC_ON_GC;
    return send_txt(c, "%s", __(c, "\tE\tC7DC/PC games visible."));
}

/* 用法: /allowgc [off] */
static int handle_allowgc(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;

    /* Lock the lobby mutex... we've got some work to do. */
    pthread_mutex_lock(&l->mutex);

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

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
        l->flags &= ~LOBBY_FLAG_GC_ALLOWED;
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7GameCube 不允许进入."));
    }

    /* Make sure there's no conflicting flags */
    if((l->flags & LOBBY_FLAG_DCONLY) || (l->flags & LOBBY_FLAG_PCONLY) ||
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

/* 用法 /ws item1,item2,item3,item4 */
static int handle_ws(ship_client_t *c, const char *params) {
    uint32_t ws[4] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
    int count;
    uint8_t tmp[0x24] = { 0 };
    subcmd_pkt_t *p = (subcmd_pkt_t *)tmp;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X,%X,%X,%X", ws + 0, ws + 1, ws + 2, ws + 3);

    if(count == EOF || count == 0) {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid WS code."));
    }

    /* 填充数据头 */
    memset(p, 0, 0x24);
    memset(&tmp[0x0C], 0xFF, 0x10);
    p->hdr.dc.pkt_type = GAME_COMMAND0_TYPE;
    p->hdr.dc.pkt_len = LE16(0x24);
    p->type = SUBCMD60_WORD_SELECT;
    p->size = 0x08;

    if(c->version != CLIENT_VERSION_GC) {
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
    send_pkt_dc(c, (dc_pkt_hdr_t *)p);
    return subcmd_handle_60(c, p);
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
                                i, c2->pl->v1.character.dress_data.guildcard_str.string, c2->guildcard);
            }
            else {
                len += snprintf(str + len, 511 - len, "%d: None   ", i);
            }

            if((i + 1) < l->max_clients) {
                if((c2 = l->clients[i + 1])) {
                    len += snprintf(str + len, 511 - len, "%d: %s (%" PRIu32
                                    ")\n", i + 1, c2->pl->v1.character.dress_data.guildcard_str.string,
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
                                c2->pl->v1.character.dress_data.guildcard_str.string);
            }
            else {
                len += snprintf(str + len, 511 - len, "%d: None   ", i);
            }

            if((i + 1) < l->max_clients) {
                if((c2 = l->clients[i + 1])) {
                    len += snprintf(str + len, 511 - len, "%d: %s\n", i + 1,
                                    c2->pl->v1.character.dress_data.guildcard_str.string);
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
        return send_txt(c, "%s", __(c, "\tE\tC7Not valid in battle or\n"
                                    "challenge modes."));
    }

    /* Make sure we're not in legit mode. */
    if((l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(c, "%s", __(c, "\tE\tC7Not valid in legit\n"
                                    "mode."));
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
    p->hdr.dc.pkt_type = GAME_COMMAND0_TYPE;
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

/* 用法: /stfu guildcard */
static int handle_stfu(ship_client_t *c, const char *params) {
    uint32_t gc;
    block_t *b = c->cur_block;
    ship_client_t *i;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Look for the requested user and STFU them (only on this block). */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if(i->guildcard == gc && i->privilege < c->privilege) {
            i->flags |= CLIENT_FLAG_STFU;
            return send_txt(c, "%s", __(c, "\tE\tC7Client STFUed."));
        }
    }

    /* The person isn't here... There's nothing left to do. */
    return send_txt(c, "%s", __(c, "\tE\tC7Guildcard not found"));
}

/* 用法: /unstfu guildcard */
static int handle_unstfu(ship_client_t *c, const char *params) {
    uint32_t gc;
    block_t *b = c->cur_block;
    ship_client_t *i;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Look for the requested user and un-STFU them (only on this block). */
    TAILQ_FOREACH(i, b->clients, qentry) {
        if(i->guildcard == gc) {
            i->flags &= ~CLIENT_FLAG_STFU;
            return send_txt(c, "%s", __(c, "\tE\tC7Client un-STFUed."));
        }
    }

    /* The person isn't here... There's nothing left to do. */
    return send_txt(c, "%s", __(c, "\tE\tC7Guildcard not found."));
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
                            cl->pl->v1.character.dress_data.guildcard_str.string, __(c, "Entry"), i);
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

/* 用法: /gameevent number */
static int handle_gameevent(ship_client_t *c, const char *params) {
    int event;

    /* Make sure the requester is a GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Grab the event number */
    event = atoi(params);

    if(event < 0 || event > 6) {
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid event code."));
    }

    ship->game_event = event;

    return send_txt(c, "%s", __(c, "\tE\tC7Game Event set."));
}

/* 用法: /ban:d guildcard reason */
static int handle_ban_d(ship_client_t *c, const char *params) {
    uint32_t gc;
    block_t *b;
    ship_client_t *i;
    char *reason;
    uint32_t j;

    /* Make sure the requester is a local GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Set the ban in the list (86,400s = 7 days) */
    if(ban_guildcard(ship, time(NULL) + 86400, c->guildcard, gc, reason + 1)) {
        return send_txt(c, "%s", __(c, "\tE\tC7设置封禁发生错误."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for(j = 0; j < ship->cfg->blocks; ++j) {
        if((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if(i->guildcard == gc) {
                    if(strlen(reason) > 1) {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s\n%s\n%s",
                                         __(i, "\tE您已被GM封禁并逐出舰船."), __(i, "封禁时长:"),
                                         __(i, "1 天"), __(i, "封禁理由:"),
                                         reason + 1);
                    }
                    else {
                        send_msg(i, MSG_BOX_TYPE, "%s\n%s %s",
                                         __(i, "\tE您已被GM封禁并逐出舰船."), __(i, "封禁时长:"),
                                         __(i, "1 天"));
                    }

                    i->flags |= CLIENT_FLAG_DISCONNECTED;
                }
            }

            pthread_rwlock_unlock(&b->lock);
        }
    }

    return send_txt(c, "%s", __(c, "\tE\tC7已成功封禁该玩家."));
}

/* 用法: /ban:w guildcard reason */
static int handle_ban_w(ship_client_t *c, const char *params) {
    uint32_t gc;
    block_t *b;
    ship_client_t *i;
    char *reason;
    uint32_t j;

    /* Make sure the requester is a local GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Set the ban in the list (604,800s = 7 days) */
    if(ban_guildcard(ship, time(NULL) + 604800, c->guildcard, gc, reason + 1)) {
        return send_txt(c, "%s", __(c, "\tE\tC7Error setting ban."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for(j = 0; j < ship->cfg->blocks; ++j) {
        if((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if(i->guildcard == gc) {
                    if(strlen(reason) > 1) {
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

/* 用法: /ban:m guildcard reason */
static int handle_ban_m(ship_client_t *c, const char *params) {
    uint32_t gc;
    block_t *b;
    ship_client_t *i;
    char *reason;
    uint32_t j;

    /* Make sure the requester is a local GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Set the ban in the list (2,592,000s = 30 days) */
    if(ban_guildcard(ship, time(NULL) + 2592000, c->guildcard, gc,
                     reason + 1)) {
        return send_txt(c, "%s", __(c, "\tE\tC7Error setting ban."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for(j = 0; j < ship->cfg->blocks; ++j) {
        if((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if(i->guildcard == gc) {
                    if(strlen(reason) > 1) {
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

/* 用法: /ban:p guildcard reason */
static int handle_ban_p(ship_client_t *c, const char *params) {
    uint32_t gc;
    block_t *b;
    ship_client_t *i;
    uint32_t j;
    char *reason;

    /* Make sure the requester is a local GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, &reason, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Set the ban in the list. An end time of -1 = forever */
    if(ban_guildcard(ship, (time_t)-1, c->guildcard, gc, reason + 1)) {
        return send_txt(c, "%s", __(c, "\tE\tC7Error setting ban."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for(j = 0; j < ship->cfg->blocks; ++j) {
        if((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                if(i->guildcard == gc) {
                    if(strlen(reason) > 1) {
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

/* 用法: /unban guildcard */
static int handle_unban(ship_client_t *c, const char *params) {
    uint32_t gc;
    int rv;

    /* Make sure the requester is a local GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Figure out the user requested */
    errno = 0;
    gc = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid guildcard number */
        return send_txt(c, "%s", __(c, "\tE\tC7无效 GC 编号."));
    }

    /* Attempt to lift the ban */
    rv = ban_lift_guildcard_ban(ship, gc);

    /* Did we succeed? */
    if(!rv) {
        return send_txt(c, "%s", __(c, "\tE\tC7Lifted ban."));
    }
    else if(rv == -1) {
        return send_txt(c, "%s", __(c, "\tE\tC7User not banned."));
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7Error lifting ban."));
    }
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

/* 用法: /gbc message */
static int handle_gbc(ship_client_t *c, const char *params) {
    /* Make sure the requester is a Global GM. */
    if(!GLOBAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Make sure there's a message to send */
    if(!strlen(params)) {
        return send_txt(c, "%s", __(c, "\tE\tC7缺少参数?"));
    }

    return shipgate_send_global_msg(&ship->sg, c->guildcard, params);
}

/* 用法: /logout */
static int handle_logout(ship_client_t *c, const char *params) {
    /* See if they're logged in first */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7未登录管理后台."));
    }

    /* Clear the logged in status. */
    c->flags &= ~(CLIENT_FLAG_LOGGED_IN | CLIENT_FLAG_OVERRIDE_GAME);
    c->privilege &= ~(CLIENT_PRIV_LOCAL_GM | CLIENT_PRIV_LOCAL_ROOT);

    return send_txt(c, "%s", __(c, "\tE\tC7已登出管理后台."));
}

/* 用法: /override */
static int handle_override(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;

    /* Make sure the requester is a GM */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Make sure that the requester is in a lobby, not a team */
    if(l->type != LOBBY_TYPE_LOBBY) {
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));
    }

    /* Set the flag so we know when they join a lobby */
    c->flags |= CLIENT_FLAG_OVERRIDE_GAME;

    return send_txt(c, "%s", __(c, "\tE\tC7Lobby restriction override on."));
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

/* 用法: /gm */
static int handle_gm(ship_client_t *c, const char *params) {
    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    return send_gm_menu(c, MENU_ID_GM);
}

/* 用法: /maps [numeric string] */
static int handle_maps(ship_client_t *c, const char *params) {
    uint32_t maps[32] = { 0 };
    int i = 0;

    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Read in the maps string, one character at a time. Any undefined entries
       at the end will give you a 0 in their place. */
    while(*params) {
        if(!isdigit(*params)) {
            return send_txt(c, "%s", __(c, "\tE\tC7Invalid map entry."));
        }
        else if(i > 31) {
            return send_txt(c, "%s", __(c, "\tE\tC7Too many entries."));
        }

        maps[i++] = *params - '0';
        params++;
    }

    /* Free any old set, if there is one. */
    if(c->next_maps) {
        free_safe(c->next_maps);
    }

    /* Save the maps string into the client's struct. */
    c->next_maps = (uint32_t *)malloc(sizeof(uint32_t) * 32);
    if(!c->next_maps) {
        return send_txt(c, "%s", __(c, "\tE\tC7Unknown error."));
    }

    memcpy(c->next_maps, maps, sizeof(uint32_t) * 32);

    /* We're done. */
    return send_txt(c, "%s", __(c, "\tE\tC7Set maps for next team."));
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
    return send_txt(c, "%s\n%s", __(c, "\tE\tC7Maps in use:"), string);
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
        strncpy((char*)c->game_info.name, c->pl->bb.character.dress_data.guildcard_str.string, sizeof(c->game_info.name));
        c->game_info.name[31] = 0;

    }
    else {
        strncpy((char*)c->game_info.name, c->pl->v1.character.dress_data.guildcard_str.string, sizeof(c->game_info.name));
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
        return send_txt(c, "%s", __(c, "\tE\tC7You must be logged in to "
                                    "use this command."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
                           USER_OPT_ENABLE_BACKUP, 1, &enable);
    c->flags |= CLIENT_FLAG_AUTO_BACKUP;
    return send_txt(c, "%s", __(c, "\tE\tC7Character backups enabled."));
}

/* 用法 /disablebk */
static int handle_disablebk(ship_client_t *c, const char *params) {
    uint8_t enable = 0;

    /* Make sure the user is logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7You must be logged in to "
                                    "use this command."));
    }

    /* Send the message to the shipgate */
    shipgate_send_user_opt(&ship->sg, c->guildcard, c->cur_block->b,
                           USER_OPT_ENABLE_BACKUP, 1, &enable);
    c->flags &= ~(CLIENT_FLAG_AUTO_BACKUP);
    return send_txt(c, "%s", __(c, "\tE\tC7Character backups disabled."));
}

/* 用法: /exp amount */
static int handle_exp(ship_client_t *c, const char *params) {
    uint32_t amt;
    lobby_t *l = c->cur_lobby;

    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    /* Make sure that the requester is in a team, not a lobby */
    if(l->type != LOBBY_TYPE_GAME) {
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure the requester is on Blue Burst */
    if(c->version != CLIENT_VERSION_BB) {
        return send_txt(c, "%s", __(c, "\tE\tC7Only valid on Blue Burst."));
    }

    /* Figure out the amount requested */
    errno = 0;
    amt = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid amount */
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid amount."));
    }

    return client_give_exp(c, amt);
}

/* 用法: /level [destination level, or blank for one level up] */
static int handle_level(ship_client_t *c, const char *params) {
    uint32_t amt;
    lobby_t *l = c->cur_lobby;

    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    if(c->version == CLIENT_VERSION_BB) {
        /* Make sure that the requester is in a team, not a lobby */
        if(l->type != LOBBY_TYPE_GAME) {
            return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));
        }

        /* Figure out the level requested */
        if(params && strlen(params)) {
            errno = 0;
            amt = (uint32_t)strtoul(params, NULL, 10);

            if(errno != 0) {
                /* Send a message saying invalid amount */
                return send_txt(c, "%s", __(c, "\tE\tC7无效等级."));
            }

            amt -= 1;
        }
        else {
            amt = LE32(c->bb_pl->character.disp.level) + 1;
        }

        /* If the level is too high, let them know. */
        if(amt > 199) {
            return send_txt(c, "%s", __(c, "\tE\tC7无效等级."));
        }

        return client_give_level(c, amt);
    }
    else if(c->version == CLIENT_VERSION_DCV2 ||
            c->version == CLIENT_VERSION_PC) {
        /* Make sure that the requester is in a lobby, not a team */
        if(l->type == LOBBY_TYPE_GAME) {
            return send_txt(c, "%s", __(c, "\tE\tC7Not valid in a team."));
        }

        /* Figure out the level requested */
        if(params && strlen(params)) {
            errno = 0;
            amt = (uint32_t)strtoul(params, NULL, 10);

            if(errno != 0) {
                /* Send a message saying invalid amount */
                return send_txt(c, "%s", __(c, "\tE\tC7无效等级."));
            }

            amt -= 1;
        }
        else {
            amt = LE32(c->pl->v1.character.disp.level) + 1;
        }

        /* If the level is too high, let them know. */
        if(amt > 199) {
            return send_txt(c, "%s", __(c, "\tE\tC7无效等级."));
        }

        return client_give_level_v2(c, amt);
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7Not valid on your version."));
    }
}

/* 用法: /sdrops [off] */
static int handle_sdrops(ship_client_t *c, const char *params) {
    /* See if we can enable server-side drops or not on this ship. */
    switch(c->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
            if(!pt_v2_enabled() || !map_have_v2_maps() || !pmt_v2_enabled() ||
               !rt_v2_enabled())
                return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                                            "suported on this ship for\n"
                                            "this client version."));
            break;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            /* XXXX: GM-only until they're fixed... */
            if(!pt_gc_enabled() || !map_have_gc_maps() || !pmt_gc_enabled() ||
               !rt_gc_enabled() || !LOCAL_GM(c))
                return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                                            "suported on this ship for\n"
                                            "this client version."));
            break;

        case CLIENT_VERSION_EP3:
            return send_txt(c, "%s", __(c, "\tE\tC7不支持 Episode 3 版本."));

        case CLIENT_VERSION_BB:
            /* XXXX: GM-only until they're fixed... */
            if (!pt_bb_enabled() || !map_have_bb_maps() || !pmt_bb_enabled() ||
                !rt_bb_enabled() || !LOCAL_GM(c))
                return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                    "suported on this ship for\n"
                    "this client version."));
            /* 对 Blue Burst 无效(BB采用网络服务器掉落) */
            //return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst 不支持该指令."));
    }

    /* See if we're turning the flag off. */
    if(!strcmp(params, "off")) {
        c->flags &= ~CLIENT_FLAG_SERVER_DROPS;
        return send_txt(c, "%s", __(c, "\tE\tC7服务器掉落模式关闭."));
    }

    c->flags |= CLIENT_FLAG_SERVER_DROPS;
    return send_txt(c, "%s", __(c, "\tE\tC7新房间开启服务器掉落模式."));
}

/* 用法: /gcprotect [off] */
static int handle_gcprotect(ship_client_t *c, const char *params) {
    uint8_t enable = 1;

    /* Make sure they're logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7You must be logged in to "
                                    "use this command."));
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

/* 用法: /trackinv */
static int handle_trackinv(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;

    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    /* Make sure that the requester is in a plain old lobby. */
    if(l->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));

    /* Set the flag... */
    c->flags |= CLIENT_FLAG_TRACK_INVENTORY;

    return send_txt(c, "%s", __(c, "\tE\tC7Flag set."));
}

/* 用法 /trackkill [off] */
static int handle_trackkill(ship_client_t *c, const char *params) {
    uint8_t enable = 1;

    /* Make sure they're logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7You must be logged in to "
                                       "use this command."));
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
    return send_txt(c, "%s", __(c, "\tE\tC7Kill tracking enabled."));
}

/* 用法: /ep3music value */
static int handle_ep3music(ship_client_t *c, const char *params) {
    uint32_t song;
    lobby_t *l = c->cur_lobby;
    uint8_t rawpkt[12] = { 0 };
    subcmd_change_lobby_music_GC_Ep3_t *pkt = (subcmd_change_lobby_music_GC_Ep3_t *)rawpkt;

    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    /* Make sure that the requester is in a lobby, not a team */
    if(l->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));

    /* Figure out the level requested */
    if(!params || !strlen(params))
        return send_txt(c, "%s", __(c, "\tE\tC7请指定歌曲名称."));

    errno = 0;
    song = (uint32_t)strtoul(params, NULL, 10);

    if(errno != 0)
        return send_txt(c, "%s", __(c, "\tE\tC7无效歌曲名称."));

    /* Prepare the packet. */
    pkt->hdr.dc.pkt_type = GAME_COMMAND0_TYPE;
    pkt->hdr.dc.flags = 0x00;
    pkt->hdr.dc.pkt_len = LE16(0x000C);
    pkt->shdr.type = SUBCMD60_JUKEBOX;
    pkt->shdr.size = 0x02;
    pkt->shdr.unused = 0x0000;
    pkt->song_number = song;

    /* Send it. */
    subcmd_send_lobby_dc(l, NULL, (subcmd_pkt_t *)pkt, 0);
    return 0;
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

/* 用法: /dsdrops version difficulty section episode */
static int handle_dsdrops(ship_client_t *c, const char *params) {
#ifdef DEBUG
    uint8_t ver, diff, section, ep;
    char *sver, *sdiff, *ssection, *sep;
    char *tok, *str;
#endif

    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    if(c->version == CLIENT_VERSION_BB)
        return send_txt(c, "%s", __(c, "\tE\tC7Blue Burst 不支持该指令."));

#ifndef DEBUG
    return send_txt(c, "%s", __(c, "\tE\tC7Debug support not compiled in."));
#else
    if(!(str = _strdup(params)))
        return send_txt(c, "%s", __(c, "\tE\tC7内部服务器错误."));

    /* Grab each token we're expecting... */
    sver = strtok_s(str, " ,", &tok);
    sdiff = strtok_s(NULL, " ,", &tok);
    ssection = strtok_s(NULL, " ,", &tok);
    sep = strtok_s(NULL, " ,", &tok);

    if(!ssection || !sdiff || !sver || !sep) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7指令参数缺失."));
    }

    /* Parse the arguments */
    if(!strcmp(sver, "v2")) {
        if(!pt_v2_enabled() || !map_have_v2_maps() || !pmt_v2_enabled() ||
           !rt_v2_enabled()) {
            free_safe(str);
            return send_txt(c, "%s", __(c, "\tE\tC7Server-side drops not\n"
                                        "suported on this ship for\n"
                                        "this client version."));
        }

        ver = CLIENT_VERSION_DCV2;
    }
    else if(!strcmp(sver, "gc")) {
        if(!pt_gc_enabled() || !map_have_gc_maps() || !pmt_gc_enabled() ||
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

    if(errno || diff > 3) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid difficulty."));
    }

    section = (uint8_t)strtoul(ssection, NULL, 10);

    if(errno || section > 9) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid section ID."));
    }

    ep = (uint8_t)strtoul(sep, NULL, 10);

    if(errno || (ep != 1 && ep != 2) ||
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

/* 用法: /noevent */
static int handle_noevent(ship_client_t *c, const char *params) {
    if(c->version < CLIENT_VERSION_GC)
        return send_txt(c, "%s", __(c, "\tE\tC7游戏版本不支持."));

    /* Make sure that the requester is in a lobby, not a game */
    if(c->cur_lobby->type != LOBBY_TYPE_LOBBY)
        return send_txt(c, "%s", __(c, "\tE\tC7无法在游戏房间中使用."));

    return send_simple(c, LOBBY_EVENT_TYPE, LOBBY_EVENT_NONE);
}

/* 用法: /lflags */
static int handle_lflags(ship_client_t *c, const char *params) {
    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    return send_txt(c, "\tE\tC7%08x", c->cur_lobby->flags);
}

/* 用法: /cflags */
static int handle_cflags(ship_client_t *c, const char *params) {
    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    return send_txt(c, "\tE\tC7%08x", c->flags);
}

/* 用法: /showpos */
static int handle_showpos(ship_client_t *c, const char *params) {
    return send_txt(c, "\tE\tC7(%f, %f, %f)", c->x, c->y, c->z);
}

/* 用法: /t x, y, z */
static int handle_t(ship_client_t *c, const char *params) {
    char *str, *tok = { 0 }, *xs, *ys, *zs;
    float x, y, z;
    subcmd_teleport_t p2 = { 0 };
    lobby_t *l = c->cur_lobby;

    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    if(!(str = _strdup(params)))
        return send_txt(c, "%s", __(c, "\tE\tC7内部服务器错误."));

    /* Grab each token we're expecting... */
    xs = strtok_s(str, " ,", &tok);
    ys = strtok_s(NULL, " ,", &tok);
    zs = strtok_s(NULL, " ,", &tok);

    if(!xs || !ys || !zs) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7指令参数缺失."));
    }

    /* Parse out the numerical values... */
    errno = 0;
    x = strtof(xs, NULL);
    y = strtof(ys, NULL);
    z = strtof(zs, NULL);
    free_safe(str);

    /* Did they all parse ok? */
    if(errno)
        return send_txt(c, "%s", __(c, "\tE\tC7无效方位角."));

    /* Hopefully they know what they're doing... Send the packet. */
    p2.hdr.pkt_type = GAME_COMMAND0_TYPE;
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
    return lobby_send_pkt_dc(l, NULL, (dc_pkt_hdr_t *)&p2, 0);
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

/* 用法: /quest id */
static int handle_quest(ship_client_t *c, const char *params) {
    char* str, * tok = { 0 }, * qid;
    lobby_t *l = c->cur_lobby;
    uint32_t quest_id;
    int rv;

    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME)
        return send_txt(c, "%s", __(c, "\tE\tC7只在游戏房间中有效."));

    if(l->flags & LOBBY_FLAG_BURSTING)
        return send_txt(c, "%s", __(c, "\tE\tC4跃迁中,请稍后再试."));

    if(!(str = _strdup(params)))
        return send_txt(c, "%s", __(c, "\tE\tC7内部服务器错误."));

    /* Grab each token we're expecting... */
    qid = strtok_s(str, " ,", &tok);

    /* Attempt to parse out the quest id... */
    errno = 0;
    quest_id = (uint32_t)strtoul(qid, NULL, 10);

    if(errno) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7无效任务ID."));
    }

    /* Done with this... */
    free_safe(str);

    pthread_rwlock_rdlock(&ship->qlock);

    /* Do we have quests configured? */
    if(!TAILQ_EMPTY(&ship->qmap)) {
        /* Find the quest first, since someone might be doing something
           stupid... */
           quest_map_elem_t *e = quest_lookup(&ship->qmap, quest_id);

           /* If the quest isn't found, bail out now. */
           if(!e) {
               rv = send_txt(c, __(c, "\tE\tC7无效任务ID."));
               pthread_rwlock_unlock(&ship->qlock);
               return rv;
           }

           rv = lobby_setup_quest(l, c, quest_id, CLIENT_LANG_ENGLISH);
    }
    else {
        rv = send_txt(c, "%s", __(c, "\tE\tC4Quests not\nconfigured."));
    }

    pthread_rwlock_unlock(&ship->qlock);

    return rv;
}

/* 用法 /autolegit [off] */
static int handle_autolegit(ship_client_t *c, const char *params) {
    uint8_t enable = 1;
    psocn_limits_t *limits;

    /* Make sure they're logged in */
    if(!(c->flags & CLIENT_FLAG_LOGGED_IN)) {
        return send_txt(c, "%s", __(c, "\tE\tC7You must be logged in to "
                                       "use this command."));
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

/* 用法: /teamlog */
static int handle_teamlog(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int rv;

    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME)
        return send_txt(c, "%s", __(c, "\tE\tC7Only valid in a team."));

    rv = team_log_start(l);

    if(!rv) {
        return send_txt(c, "%s", __(c, "\tE\tC7Logging started."));
    }
    else if(rv == -1) {
        return send_txt(c, "%s", __(c, "\tE\tC7The team is already\n"
                                    "being logged."));
    }
    else {
        return send_txt(c, "%s", __(c, "\tE\tC7Cannot create log file."));
    }
}

/* 用法: /eteamlog */
static int handle_eteamlog(ship_client_t *c, const char *params) {
    lobby_t *l = c->cur_lobby;
    int rv;

    /* Make sure the requester is a local GM, at least. */
    if(!LOCAL_GM(c))
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

    /* Make sure that the requester is in a team, not a lobby. */
    if(l->type != LOBBY_TYPE_GAME)
        return send_txt(c, "%s", __(c, "\tE\tC7Only valid in a team."));

    rv = team_log_stop(l);

    if(!rv) {
        return send_txt(c, "%s", __(c, "\tE\tC7Logging ended."));
    }
    else {
        return send_txt(c, "%s", __(c,"\tE\tC7The team is not\n"
                                    "being logged."));
    }
}

/* 用法: /ib days ip reason */
static int handle_ib(ship_client_t *c, const char *params) {
    struct sockaddr_storage addr = { 0 }, netmask = { 0 };
    struct sockaddr_in *ip = (struct sockaddr_in *)&addr;
    struct sockaddr_in *nm = (struct sockaddr_in *)&netmask;
    block_t *b;
    ship_client_t *i;
    char *slen, *sip, *reason;
    char *str, *tok = { 0 };
    uint32_t j;
    uint32_t len;

    /* Make sure the requester is a local GM. */
    if(!LOCAL_GM(c)) {
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));
    }

    if(!(str = _strdup(params))) {
        return send_txt(c, "%s", __(c, "\tE\tC7内部服务器错误."));
    }

    /* Grab each token we're expecting... */
    slen = strtok_s(str, " ,", &tok);
    sip = strtok_s(NULL, " ,", &tok);
    reason = strtok_s(NULL, "", &tok);

    /* Figure out the user requested */
    errno = 0;
    len = (uint32_t)strtoul(slen, NULL, 10);

    if(errno != 0) {
        /* Send a message saying invalid length */
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid ban length."));
    }

    /* Parse the IP address. We only support IPv4 here. */
    if(!my_pton(PF_INET, sip, &addr)) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Invalid IP address."));
    }

    nm->sin_addr.s_addr = EMPTY_STRING;
    addr.ss_family = netmask.ss_family = PF_INET;

    /* Set the ban in the list (86,400s = 1 day) */
    if(ban_ip(ship, time(NULL) + 86400 * (time_t)len, c->guildcard, &addr, &netmask,
              reason)) {
        free_safe(str);
        return send_txt(c, "%s", __(c, "\tE\tC7Error setting ban."));
    }

    /* Look for the requested user and kick them if they're on the ship. */
    for(j = 0; j < ship->cfg->blocks; ++j) {
        if((b = ship->blocks[j])) {
            pthread_rwlock_rdlock(&b->lock);

            TAILQ_FOREACH(i, b->clients, qentry) {
                /* Disconnect them if we find them */
                nm = (struct sockaddr_in *)&i->ip_addr;
                if(nm->sin_family == PF_INET &&
                   ip->sin_addr.s_addr == nm->sin_addr.s_addr) {
                    if(reason && strlen(reason)) {
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
        return send_txt(c, "%s", __(c, "\tE\tC7权限不足."));

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

/* 用法: /pso2 item1,item2,item3,item4*/
static int handle_pso2(ship_client_t* src, const char* params) {
    lobby_t* l = src->cur_lobby;
    iitem_t* iitem;
    subcmd_drop_stack_t dc = { 0 };
    subcmd_bb_drop_stack_t bb = { 0 };
    uint32_t item[4] = { 0, 0, 0, 0 };

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return send_txt(src, "%s", __(src, "\tE\tC7权限不足."));
    }

    int count;

    /* Make sure the requester is a GM. */
    if (!LOCAL_GM(src)) {
        return send_msg(src, TEXT_MSG_TYPE, "%s", __(src, "\tE\tC7权限不足."));
    }

    /* Copy over the item data. */
    count = sscanf(params, "%X,%X,%X,%X", &item[0], &item[1], &item[2],
        &item[3]);

    if (count == EOF || count == 0) {
        return send_txt(src, "%s", __(src, "\tE\tC7无效物品代码."));
    }

    clear_item(&src->new_item);

    /* Copy over the item data. */
    src->new_item.datal[0] = SWAP32(item[0]);
    src->new_item.datal[1] = SWAP32(item[1]);
    src->new_item.datal[2] = SWAP32(item[2]);
    src->new_item.data2l = SWAP32(item[3]);

    print_item_data(&src->new_item, src->version);

    send_txt(src, "%s %s %s",
        __(src, "\tE\tC8物品:"),
        item_get_name(&src->new_item, src->version),
        __(src, "\tE\tC6 new_item 设置成功."));

    pthread_mutex_lock(&l->mutex);

    /* Make sure that the requester is in a team, not a lobby. */
    if (l->type != LOBBY_TYPE_GAME) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s", __(src, "\tE\tC7只在游戏房间中有效."));
    }

    /* Make sure there's something set with /item */
    if (!src->new_item.datal[0]) {
        pthread_mutex_unlock(&l->mutex);
        return send_txt(src, "%s\n%s", __(src, "\tE\tC7请先输入物品的ID."),
            __(src, "\tE\tC7/item code1,code2,code3,code4."));
    }

    /* If we're on Blue Burst, add the item to the lobby's inventory first. */
    if (l->version == CLIENT_VERSION_BB) {
        iitem = add_new_litem_locked(l, &src->new_item, src->cur_area, src->x, src->z);

        if (!iitem) {
            pthread_mutex_unlock(&l->mutex);
            return send_txt(src, "%s", __(src, "\tE\tC7新物品空间不足."));
        }
    }
    else {
        ++l->item_player_id[src->client_id];
    }

    /* Generate the packet to drop the item */
    dc.hdr.pkt_type = GAME_COMMAND0_TYPE;
    dc.hdr.pkt_len = LE16(sizeof(subcmd_drop_stack_t));
    dc.hdr.flags = 0;

    dc.shdr.type = SUBCMD60_DROP_STACK;
    dc.shdr.size = 0x0A;
    dc.shdr.client_id = src->client_id;


    bb.hdr.pkt_len = LE16(sizeof(subcmd_bb_drop_stack_t));
    bb.hdr.pkt_type = LE16(GAME_COMMAND0_TYPE);
    bb.hdr.flags = 0;

    bb.shdr.type = SUBCMD60_DROP_STACK;
    bb.shdr.size = 0x09;
    bb.shdr.client_id = src->client_id;

    dc.area = LE16(src->cur_area);
    bb.area = LE32(src->cur_area);
    bb.x = dc.x = src->x;
    bb.z = dc.z = src->z;
    bb.data = dc.data = src->new_item;
    bb.data.item_id = dc.data.item_id = LE32((l->item_lobby_id - 1));
    bb.two = dc.two = LE32(0x00000002);

    /* Clear the set item */
    clear_item(&src->new_item);

    /* Send the packet to everyone in the lobby */
    pthread_mutex_unlock(&l->mutex);

    switch (src->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        return lobby_send_pkt_dc(l, NULL, (dc_pkt_hdr_t*)&dc, 0);

    case CLIENT_VERSION_BB:
        return lobby_send_pkt_bb(l, NULL, (bb_pkt_hdr_t*)&bb, 0);

    default:
        return 0;
    }
}

static command_t cmds[] = {
    { "debug"    , handle_gmdebug   },
    { "swarp"    , handle_shipwarp  },
    { "warp"     , handle_warp      },
    { "kill"     , handle_kill      },
    { "minlvl"   , handle_min_level },
    { "maxlvl"   , handle_max_level },
    { "refresh"  , handle_refresh   },
    { "save"     , handle_save      },
    { "restore"  , handle_restore   },
    { "bstat"    , handle_bstat     },
    { "bcast"    , handle_bcast     },
    { "tmsg"     , handle_tmsg      },
    { "arrow"    , handle_arrow     },
    { "login"    , handle_login     },
    { "item"     , handle_item      },
    { "item4"    , handle_item4     },
    { "makeitem" , handle_makeitem  },
    { "event"    , handle_event     },
    { "passwd"   , handle_passwd    },
    { "lname"    , handle_lname     },
    { "warpall"  , handle_warpall   },
    { "bug"      , handle_bug       },
    { "clinfo"   , handle_clinfo    },
    { "gban:d"   , handle_gban_d    },
    { "gban:w"   , handle_gban_w    },
    { "gban:m"   , handle_gban_m    },
    { "gban:p"   , handle_gban_p    },
    { "list"     , handle_list      },
    { "legit"    , handle_legit     },
    { "normal"   , handle_normal    },
    { "shutdown" , handle_shutdown  },
    { "log"      , handle_log       },
    { "endlog"   , handle_endlog    },
    { "motd"     , handle_motd      },
    { "friendadd", handle_friendadd },
    { "frienddel", handle_frienddel },
    { "dconly"   , handle_dconly    },
    { "v1only"   , handle_v1only    },
    { "forgegc"  , handle_forgegc   },
    { "invuln"   , handle_invuln    },
    { "inftp"    , handle_inftp     },
    { "smite"    , handle_smite     },
    { "teleport" , handle_teleport  },
    { "dbginv"   , handle_dbginv    },
    { "dbgbank"  , handle_dbgbank   },
    { "showdcpc" , handle_showdcpc  },
    { "allowgc"  , handle_allowgc   },
    { "ws"       , handle_ws        },
    { "ll"       , handle_ll        },
    { "npc"      , handle_npc       },
    { "stfu"     , handle_stfu      },
    { "unstfu"   , handle_unstfu    },
    { "ignore"   , handle_ignore    },
    { "unignore" , handle_unignore  },
    { "quit"     , handle_quit      },
    { "gameevent", handle_gameevent },
    { "ban:d"    , handle_ban_d     },
    { "ban:w"    , handle_ban_w     },
    { "ban:m"    , handle_ban_m     },
    { "ban:p"    , handle_ban_p     },
    { "unban"    , handle_unban     },
    { "cc"       , handle_cc        },
    { "qlang"    , handle_qlang     },
    { "friends"  , handle_friends   },
    { "gbc"      , handle_gbc       },
    { "logout"   , handle_logout    },
    { "override" , handle_override  },
    { "ver"      , handle_ver       },
    { "restart"  , handle_restart   },
    { "search"   , handle_search    },
    { "gm"       , handle_gm        },
    { "maps"     , handle_maps      },
    { "showmaps" , handle_showmaps  },
    { "restorebk", handle_restorebk },
    { "enablebk" , handle_enablebk  },
    { "disablebk", handle_disablebk },
    { "exp"      , handle_exp       },
    { "level"    , handle_level     },
    { "sdrops"   , handle_sdrops    },
    { "gcprotect", handle_gcprotect },
    { "trackinv" , handle_trackinv  },
    { "trackkill", handle_trackkill },
    { "ep3music" , handle_ep3music  },
    { "tlogin"   , handle_tlogin    },
    { "dsdrops"  , handle_dsdrops   },
    { "noevent"  , handle_noevent   },
    { "lflags"   , handle_lflags    },
    { "cflags"   , handle_cflags    },
    { "stalk"    , handle_teleport  },    /* Happy, Aleron Ives? */
    { "showpos"  , handle_showpos   },
    { "t"        , handle_t         },    /* Short command = more precision. */
    { "info"     , handle_info      },
    { "quest"    , handle_quest     },
    { "autolegit", handle_autolegit },
    { "censor"   , handle_censor    },
    { "teamlog"  , handle_teamlog   },
    { "eteamlog" , handle_eteamlog  },
    { "ib"       , handle_ib        },
    { "xblink"   , handle_xblink    },
    { "logme"    , handle_logme     },
    { "clean"    , handle_clean     },
    { ""         , NULL             }     /* End marker -- DO NOT DELETE */
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
