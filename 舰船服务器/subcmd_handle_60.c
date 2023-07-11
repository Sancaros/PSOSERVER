/*
    梦幻之星中国 舰船服务器 60指令处理
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

int sub60_unimplement_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            src->guildcard);
        return -1;
    }

    DBG_LOG("未处理指令 0x%02X", pkt->hdr.pkt_type);

    display_packet(pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_check_client_id_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (pkt->param != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 在触发了游戏房间内 %s 指令! 且Client ID不一致",
            src->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    //UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_check_lobby_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏 %s 指令!",
            src->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    ///* 合理性检查... Make sure the size of the subcommand matches with what we
    //   expect. Disconnect the client if not. */
    //if (pkt->size != size) {
    //    ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X! size %02X",
    //        c->guildcard, pkt->type, size);
    //    ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
    //    return -1;
    //}

    DBG_LOG("玩家 0x%02X 指令: 0x%X", pkt->hdr.pkt_type, pkt->type);
    display_packet(pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

inline int tlindex(uint8_t l) {
    switch (l) {
    case 0: case 1: case 2: case 3: case 4: return 0;
    case 5: case 6: case 7: case 8: case 9: return 1;
    case 10: case 11: case 12: case 13: case 14: return 2;
    case 15: case 16: case 17: case 18: case 19: return 3;
    case 20: case 21: case 22: case 23: case 25: return 4;
    default: return 5;
    }
}

void update_bb_qpos(ship_client_t* src, lobby_t* l) {
    uint8_t r;

    if ((r = l->qpos_regs[src->client_id][0])) {
        send_sync_register(l->clients[0], r, (uint32_t)src->x);
        send_sync_register(l->clients[0], r + 1, (uint32_t)src->y);
        send_sync_register(l->clients[0], r + 2, (uint32_t)src->z);
        send_sync_register(l->clients[0], r + 3, (uint32_t)src->cur_area);
    }
    if ((r = l->qpos_regs[src->client_id][1])) {
        send_sync_register(l->clients[1], r, (uint32_t)src->x);
        send_sync_register(l->clients[1], r + 1, (uint32_t)src->y);
        send_sync_register(l->clients[1], r + 2, (uint32_t)src->z);
        send_sync_register(l->clients[1], r + 3, (uint32_t)src->cur_area);
    }
    if ((r = l->qpos_regs[src->client_id][2])) {
        send_sync_register(l->clients[2], r, (uint32_t)src->x);
        send_sync_register(l->clients[2], r + 1, (uint32_t)src->y);
        send_sync_register(l->clients[2], r + 2, (uint32_t)src->z);
        send_sync_register(l->clients[2], r + 3, (uint32_t)src->cur_area);
    }
    if ((r = l->qpos_regs[src->client_id][3])) {
        send_sync_register(l->clients[3], r, (uint32_t)src->x);
        send_sync_register(l->clients[3], r + 1, (uint32_t)src->y);
        send_sync_register(l->clients[3], r + 2, (uint32_t)src->z);
        send_sync_register(l->clients[3], r + 3, (uint32_t)src->cur_area);
    }
}

void handle_bb_objhit_common(ship_client_t* src, lobby_t* l, uint16_t bid) {
    uint32_t obj_type;

    /* What type of object was hit? */
    if ((bid & 0xF000) == 0x4000) {
        /* An object was hit */
        bid &= 0x0FFF;

        /* Make sure the object is in range. */
        if (bid > l->map_objs->count) {
            ERR_LOG("GC %" PRIu32 " 攻击了无效的实例 "
                "(%d -- 地图实例数量: %d)!\n"
                "章节: %d, 层级: %d, 地图: (%d, %d)", src->guildcard,
                bid, l->map_objs->count, l->episode, src->cur_area,
                l->maps[src->cur_area << 1],
                l->maps[(src->cur_area << 1) + 1]);

            if ((l->flags & LOBBY_FLAG_QUESTING))
                ERR_LOG("任务 ID: %d, 版本: %d", l->qid,
                    l->version);

            /* Just continue on and hope for the best. We've probably got a
               bug somewhere on our end anyway... */
            return;
        }

        /* Make sure it isn't marked as hit already. */
        if ((l->map_objs->objs[bid].flags & 0x80000000))
            return;

        /* Now, see if we care about the type of the object that was hit. */
        obj_type = l->map_objs->objs[bid].data.skin & 0xFFFF;

        /* We'll probably want to do a bit more with this at some point, but
           for now this will do. */
        switch (obj_type) {
        case OBJ_SKIN_REG_BOX:
        case OBJ_SKIN_FIXED_BOX:
        case OBJ_SKIN_RUINS_REG_BOX:
        case OBJ_SKIN_RUINS_FIXED_BOX:
        case OBJ_SKIN_CCA_REG_BOX:
        case OBJ_SKIN_CCA_FIXED_BOX:
            /* Run the box broken script. */
            script_execute(ScriptActionBoxBreak, src, SCRIPT_ARG_PTR, src,
                SCRIPT_ARG_UINT16, bid, SCRIPT_ARG_UINT16,
                obj_type, SCRIPT_ARG_END);
            break;
        }

        /* Mark it as hit. */
        l->map_objs->objs[bid].flags |= 0x80000000;
    }
    else if ((bid & 0xF000) == 0x1000) {
        /* An enemy was hit. We don't really do anything with these here,
           as there is a better packet for handling them. */
        return;
    }
    else if ((bid & 0xF000) == 0x0000) {
        /* An ally was hit. We don't really care to do anything here. */
        return;
    }
    else {
        DBG_LOG("Unknown object type hit: %04" PRIx16 "", bid);
    }
}

#define BARTA_TIMING 1500
#define GIBARTA_TIMING 2200

const uint16_t gifoie_timing[6] = { 5000, 6000, 7000, 8000, 9000, 10000 };
const uint16_t gizonde_timing[6] = { 1000, 900, 700, 700, 700, 700 };
const uint16_t rafoie_timing[6] = { 1500, 1400, 1300, 1200, 1100, 1100 };
const uint16_t razonde_timing[6] = { 1200, 1100, 950, 800, 750, 750 };
const uint16_t rabarta_timing[6] = { 1200, 1100, 950, 800, 750, 750 };

int check_aoe_timer(ship_client_t* src, 
    subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t tech_level = 0;

    /* 合理性检查... Does the character have that level of technique? */
    tech_level = src->pl->bb.character.techniques[pkt->technique_number];
    if (tech_level == 0xFF) {
        /* 如果用户在团队中学习一项新技术，则可能会发生这种情况。
        在我们有真正的库存跟踪之前，我们将不得不篡改这一点。
        一旦我们有了真实的、完整的库存跟踪，这种情况可能会断开客户端的连接 */
        tech_level = pkt->level;
    }

    if (src->version >= CLIENT_VERSION_DCV2)
        tech_level += src->pl->bb.inv.iitems[pkt->technique_number].tech;

    if (tech_level < pkt->level) {
        /* 如果用户在团队中学习一项新技术，则可能会发生这种情况。
        在我们有真正的库存跟踪之前，我们将不得不篡改这一点。
        一旦我们有了真实的、完整的库存跟踪，这种情况可能会断开客户端的连接 */
        tech_level = pkt->level;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    /* See what technique was used... */
    switch (pkt->technique_number) {
        /* These work just like physical hits and can only hit one target, so
           handle them the simple way... */
    case TECHNIQUE_FOIE:
    case TECHNIQUE_ZONDE:
    case TECHNIQUE_GRANTS:
        if (pkt->shdr.size == 3)
            handle_bb_objhit_common(src, l, LE16(pkt->objects[0].obj_id));
        break;

        /* None of these can hit boxes (which is all we care about right now), so
           don't do anything special with them. */
    case TECHNIQUE_DEBAND:
    case TECHNIQUE_JELLEN:
    case TECHNIQUE_ZALURE:
    case TECHNIQUE_SHIFTA:
    case TECHNIQUE_RYUKER:
    case TECHNIQUE_RESTA:
    case TECHNIQUE_ANTI:
    case TECHNIQUE_REVERSER:
    case TECHNIQUE_MEGID:
        break;

        /* AoE spells are... special (why, Sega?). They never have more than one
           item hit in the packet, and just act in a broken manner in general. We
           have to do some annoying stuff to handle them here. */
    case TECHNIQUE_BARTA:
        //SYSTEMTIME aoetime;
        //GetLocalTime(&aoetime);
        //printf("%03u\n", aoetime.wMilliseconds);
        src->aoe_timer = get_ms_time() + BARTA_TIMING;
        break;

    case TECHNIQUE_GIBARTA:
        src->aoe_timer = get_ms_time() + GIBARTA_TIMING;
        break;

    case TECHNIQUE_GIFOIE:
        src->aoe_timer = get_ms_time() + gifoie_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RAFOIE:
        src->aoe_timer = get_ms_time() + rafoie_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_GIZONDE:
        src->aoe_timer = get_ms_time() + gizonde_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RAZONDE:
        src->aoe_timer = get_ms_time() + razonde_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RABARTA:
        src->aoe_timer = get_ms_time() + rabarta_timing[tlindex(tech_level)];
        break;
    }

    return 0;
}

int sub60_05_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_switch_changed_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅开启了机关!",
            src->guildcard);
        rv = -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->data.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            src->guildcard, pkt->data.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }
    //[2023年02月09日 22:51:33:981] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   04 00 05 03                                         ....
    //[2023年02月09日 22:51:34:017] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   04 00 05 03                                         ....
    //[2023年02月09日 22:51:34:054] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   05 00 05 01                                         ....
    //[2023年02月09日 22:51:34:089] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   06 00 05 01                                         ....
    //[2023年02月09日 22:51:34:124] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   07 00 05 01                                         ....
    //[2023年02月09日 22:51:34:157] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   08 00 05 01                                         ....
    //[2023年02月09日 22:51:34:194] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   09 00 05 01                                              ...
    //[2023年02月09日 22:51:34:228] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   05 00 05 01                                         ....
    //[2023年02月09日 22:51:34:262] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   06 00 05 01                                         ....
    //[2023年02月09日 22:51:34:315] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   07 00 05 01                                         ....
    //[2023年02月09日 22:51:34:354] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   08 00 05 01                                         ....
    //[2023年02月09日 22:51:34:390] 调试(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   09 00 05 01                                              ...
    rv = subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);

    if (pkt->data.flags && pkt->data.object_id != 0xFFFF) {
        if ((l->flags & LOBBY_TYPE_CHEATS_ENABLED) && src->options.switch_assist &&
            (src->last_switch_enabled_command.type == 0x05)) {
            DBG_LOG("[机关助手] 重复启动上一个命令");
            subcmd_bb_switch_changed_pkt_t* gem = pkt;
            gem->data = src->last_switch_enabled_command;
            rv = subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)gem, 0);
        }
        src->last_switch_enabled_command = pkt->data;
    }

    return rv;
}

int sub60_07_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_symbol_chat_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Don't send the message if they have the protection flag on. */
    if (src->flags & CLIENT_FLAG_GC_PROTECT)
        return send_txt(src, __(src, "\tE\tC7您必须登录后才可以进行操作."));

    /* Don't send chats for a STFUed client. */
    if ((src->flags & CLIENT_FLAG_STFU))
        return 0;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 1);
}

int sub60_0A_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_mhit_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t enemy_id2, enemy_id, dmg;
    game_enemy_t* en;
    uint32_t flags;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了怪物!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的怪物攻击数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* Make sure the enemy is in range. */
    enemy_id = LE16(pkt->shdr.enemy_id);
    enemy_id2 = LE16(pkt->enemy_id2);
    dmg = LE16(pkt->damage);
    flags = LE32(pkt->flags);

    /* Swap the flags on the packet if the user is on GC... Looks like Sega
       decided that it should be the opposite order as it is on DC/PC/BB. */
    if (src->version == CLIENT_VERSION_GC)
        flags = SWAP32(flags);

    if (enemy_id2 > l->map_enemies->count) {
        ERR_LOG("GC %" PRIu32 " 攻击了无效的怪物 (%d -- 地图怪物数量: "
            "%d)!", src->guildcard, enemy_id2, l->map_enemies->count);
        return -1;
    }

    /* Save the hit, assuming the enemy isn't already dead. */
    en = &l->map_enemies->enemies[enemy_id2];
    if (!(en->clients_hit & 0x80)) {
        en->clients_hit |= (1 << src->client_id);
        en->last_client = src->client_id;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_0B_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_bhit_pkt_t* pkt) {
    uint64_t now = get_ms_time();
    //SYSTEMTIME aoetime;
    //GetLocalTime(&aoetime);
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了箱子!",
            src->guildcard);
        return -1;
    }

    /* We only care about these if the AoE timer is set on the sender. */
    if (src->aoe_timer < now)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    /* Check the type of the object that was hit. As the AoE timer can't be set
       if the objects aren't loaded, this shouldn't ever trip up... */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    /* Handle the object marked as hit, if appropriate. */
    handle_bb_objhit_common(src, l, LE16(pkt->shdr.object_id));

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_0C_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_add_or_remove_condition_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            src->guildcard, pkt->hdr.pkt_type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_12_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_dragon_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int v = src->version, i;
    subcmd_bb_dragon_act_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " reported Dragon action in "
            "lobby!\n", src->guildcard);
        return -1;
    }

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_bb_dragon_act_t));
    tr.x.b = SWAP32(tr.x.b);
    tr.z.b = SWAP32(tr.z.b);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != src) {
            if (l->clients[i]->version == v) {
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)pkt);
            }
            else {
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&tr);
            }
        }
    }

    return 0;
}

int sub60_13_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_de_rolLe_boss_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅触发游戏指令!\n", src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    send_txt(src, "%s\n动作:0x%04X\n阶段:0x%04X.", __(src, "\tE\tC6DR BOSS"), pkt->action, pkt->stage);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_14_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_de_rolLe_boss_sact_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅触发游戏指令!\n", src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    send_txt(src, "%s\n动作:0x%04X\n阶段:0x%04X\n特殊:0x%08X.", __(src, "\tE\tC6DR BOSS2"), 
        pkt->action, pkt->stage, pkt->unused);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_15_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_VolOptBossActions_6x15_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅触发游戏指令!\n", src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    send_txt(src, "%s\n动作:0x%04X\n阶段:0x%04X\n特殊1:0x%04X\n特殊2:0x%04X.", __(src, "\tE\tC6Vol Opt BOSS1"), 
        pkt->unknown_a2, pkt->unknown_a3, pkt->unknown_a4, pkt->unknown_a5);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_16_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_VolOptBossActions_6x16_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅触发游戏指令!\n", src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    send_txt(src, "%s\n动作:0x%04X\n阶段:0x%04X\n特殊1:0x%04X\n特殊2:0x%04X.", __(src, "\tE\tC6Vol Opt BOSS2"),
        pkt->unknown_a2, pkt->unknown_a3, pkt->unknown_a4, pkt->unknown_a5);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_17_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_teleport_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅触发游戏指令!\n", src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x001C) || pkt->shdr.size != 0x05) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_18_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_dragon_special_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅触发游戏指令!\n", src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    send_txt(src, "%s\n参数1:%f\n参数2:%f\n特殊1:0x%08X\n特殊2:0x%08X.", __(src, "\tE\tC6Dragon BOSS"),
        pkt->unknown_a1, pkt->unknown_a2, pkt->unknown_a1, pkt->unknown_a2);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_19_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_DarkFalzActions_6x19_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅触发游戏指令!\n", src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0018) || pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    send_txt(src, "%s\n动作:0x%04X\n阶段:0x%04X\n特殊1:0x%04X\n特殊2:0x%04X.", __(src, "\tE\tC6Dark Falz BOSS"),
        pkt->unknown_a2, pkt->unknown_a3, pkt->unknown_a4, pkt->unused);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_1C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_destory_npc_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " 在大厅触发游戏指令!\n", src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_1F_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_set_area_1F_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, src, SCRIPT_ARG_PTR, src,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            src->cur_area, SCRIPT_ARG_END);

        // 测试DC代码
        /* Clear the list of dropped items. */
        if (src->cur_area == 0) {
            memset(src->p2_drops, 0, sizeof(src->p2_drops));
            src->p2_drops_max = 0;
        }

        src->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_20_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_set_area_20_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, src, SCRIPT_ARG_PTR, src,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            src->cur_area, SCRIPT_ARG_END);

        // 测试DC代码
        /* Clear the list of dropped items. */
        if (src->cur_area == 0) {
            memset(src->p2_drops, 0, sizeof(src->p2_drops));
            src->p2_drops_max = 0;
        }

        src->cur_area = pkt->area;
        src->x = pkt->x;
        src->y = pkt->y;
        src->z = pkt->z;

        //更换场景 客户端之间更新位置
        //DBG_LOG("客户端区域 %d （x:%f y:%f z:%f)", c->cur_area, c->x, c->y, c->z);
        //客户端区域 0 （x:228.995331 y:0.000000 z:253.998901)

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_21_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_inter_level_warp_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, src, SCRIPT_ARG_PTR, src,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            src->cur_area, SCRIPT_ARG_END);
        src->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_22_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_set_player_visibility_6x22_6x23_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int i, rv;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("SUBCMD60_LOAD_22 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    if (l->type == LOBBY_TYPE_LOBBY) {
        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != src &&
                subcmd_send_pos(src, l->clients[i])) {
                rv = -1;
                break;
            }
        }
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_23_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int i, rv;

    if (l->type != LOBBY_TYPE_GAME) {
        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != src &&
                subcmd_send_pos(src, l->clients[i])) {
                rv = -1;
                break;
            }
        }

        /* 将房间中的玩家公会数据发送至新进入的客户端 */
        send_bb_guild_cmd(src, BB_GUILD_FULL_DATA);
        send_bb_guild_cmd(src, BB_GUILD_INITIALIZATION_DATA);
    }

    rv = subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    return rv;
}

int sub60_24_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_set_pos_0x24_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x001C) || pkt->shdr.size != 0x05) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->x = pkt->x;
        src->y = pkt->y;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
#ifdef DEBUG
        DBG_LOG("GC %u %d %d (0x%X 0x%X) X轴:%f Y轴:%f Z轴:%f", c->guildcard,
            c->client_id, pkt->shdr.client_id, pkt->unk1, pkt->unused, pkt->x, pkt->y, pkt->z);
#endif // DEBUG
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_25_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_equip_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t /*inv, */item_id = pkt->item_id, found_item = 0/*, found_slot = 0, j = 0, slot[4] = { 0 }*/;
    int i = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅更换装备!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误装备数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    found_item = item_check_equip_flags(src, item_id);

    /* 是否存在物品背包中? */
    if (!found_item) {
        ERR_LOG("GC %" PRIu32 " 装备了未存在的物品数据!",
            src->guildcard);
        return -1;
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_26_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_equip_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t inv, i, isframe = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅卸除装备!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误卸除装备数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* Find the item and remove the equip flag. */
    inv = src->bb_pl->inv.item_count;

    i = find_iitem_index(&src->bb_pl->inv, pkt->item_id);

    if (src->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
        src->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);

        /* If its a frame, we have to make sure to unequip any units that
           may be equipped as well. */
        if (src->bb_pl->inv.iitems[i].data.datab[0] == ITEM_TYPE_GUARD &&
            src->bb_pl->inv.iitems[i].data.datab[1] == ITEM_SUBTYPE_FRAME) {
            isframe = 1;
        }
    }

    /* Did we find something to equip? */
    if (i >= inv) {
        ERR_LOG("GC %" PRIu32 " 卸除了未存在的物品数据!",
            src->guildcard);
        return -1;
    }

    /* Clear any units if we unequipped a frame. */
    if (isframe) {
        for (i = 0; i < inv; ++i) {
            if (src->bb_pl->inv.iitems[i].data.datab[0] == ITEM_TYPE_GUARD &&
                src->bb_pl->inv.iitems[i].data.datab[1] == ITEM_SUBTYPE_UNIT) {
                src->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_27_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_use_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    size_t index;
    errno_t err;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅使用物品!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x02)
        return -1;

    index = find_iitem_index(&src->bb_pl->inv, pkt->item_id);

    if ((err = player_use_item(src, index))) {
        ERR_LOG("GC %" PRIu32 " 使用物品发生错误! 错误码 %d",
            src->guildcard, err);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_28_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_feed_mag_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->fed_item_id, mag_id = pkt->mag_item_id;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发游戏指令!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    errno_t err = mag_bb_feed(src, pkt->mag_item_id, pkt->fed_item_id);

    if (err) {
        ERR_LOG("GC %" PRIu32 " 发送错误数据! 错误码 %d",
            src->guildcard, err);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_29_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_destroy_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    iitem_t item_data = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中掉落物品!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的物品掉落数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    item_data = remove_iitem(src, pkt->item_id, pkt->amount, src->version != CLIENT_VERSION_BB);

    if (&item_data == NULL) {
        ERR_LOG("GC %" PRIu32 " 掉落堆叠物品失败!",
            src->guildcard);
        return -1;
    }

    /* 数据包完成, 发送至游戏房间. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_2A_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_drop_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int isframe = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅中掉落物品!",
            src->guildcard);
        return -1;
    }

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if ((src->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("GC %" PRIu32 " 尝试在商店中掉落物品!",
            src->guildcard);
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品掉落数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    inventory_t* inv = &src->bb_pl->inv;

    /* 在玩家背包中查找物品. */
    size_t index = find_iitem_index(inv, pkt->item_id);

    /* If the item isn't found, then punt the user from the ship. */
    if (index == -1) {
        ERR_LOG("GC %" PRIu32 " 掉落了的物品 ID 0x%04X 与 数据包 ID 0x%04X 不符!",
            src->guildcard, inv->iitems[index].data.item_id, pkt->item_id);
        return -1;
    }

    if (inv->iitems[index].data.datab[0] == ITEM_TYPE_GUARD &&
        inv->iitems[index].data.datab[1] == ITEM_SUBTYPE_FRAME &&
        (inv->iitems[index].flags & LE32(0x00000008))) {
        isframe = 1;
    }

    /* 清理已装备的标签 */
    inv->iitems[index].flags &= LE32(0xFFFFFFF7);

    /* 卸掉所有已插入在这件装备的插件 */
    if (isframe) {
        for (int i = 0; i < inv->item_count; ++i) {
            if (inv->iitems[i].data.datab[0] == ITEM_TYPE_GUARD &&
                inv->iitems[i].data.datab[1] == ITEM_SUBTYPE_UNIT) {
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* We have the item... Add it to the lobby's inventory.
    我们有这个物品…把它添加到大厅的背包中 */
    if (!add_litem_locked(l, &inv->iitems[index])) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("无法将物品新增游戏房间背包!");
        return -1;
    }

    /* TODO 可以打印丢出的物品信息 */
    remove_iitem(src, pkt->item_id, 0, src->version != CLIENT_VERSION_BB);

    /* 数据包完成, 发送至游戏房间. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_2C_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_select_menu_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
       //if (l->type == LOBBY_TYPE_LOBBY) {
       //    ERR_LOG("GC %" PRIu32 " 在大厅与游戏房间中的NPC交谈!",
       //        c->guildcard);
       //    return -1;
       //}

    /* 合理性检查... Make sure the size of the subcommand matches with what we 
    expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x05) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        display_packet((uint8_t*)pkt, pkt->hdr.pkt_len);
        return -1;
    }

    /* Clear the list of dropped items. */
    if (pkt->unk == 0xFFFF && src->cur_area == 0) {
        memset(src->p2_drops, 0, sizeof(src->p2_drops));
        src->p2_drops_max = 0;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_2D_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_select_done_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x01) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        display_packet((uint8_t*)pkt, pkt->hdr.pkt_len);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_2F_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_hit_by_enemy_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t type = LE16(pkt->hdr.pkt_type);

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发游戏房间指令!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* TODO 测试GM DBUG 无限血量 */
    if (src->game_data->gm_debug)
        send_lobby_mod_stat(l, src, SUBCMD60_STAT_HPUP, 2040);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_39_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_photon_blast_ready_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏 %s 指令!",
            src->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x000c) || pkt->shdr.size != 0x01) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_3A_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_game_client_leave_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || src->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* 这是一个用于通知其他玩家 该玩家离开了游戏  TODO*/
    //display_packet(pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_3B_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            src->guildcard);
        return -1;
    }

    subcmd_send_bb_set_exp_rate(src, 3000);

    src->need_save_data = 1;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_3E_3F_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_set_pos_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->w = pkt->w;
        src->x = pkt->x;
        src->y = pkt->y;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_40_42_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_move_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->x = pkt->x;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);

        if (src->game_data->death) {
            src->game_data->death = 0;
        }
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_43_44_45_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_natk_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发普通攻击指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_46_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_objhit_phys_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t pkt_size = pkt->hdr.pkt_len;
    uint8_t size = pkt->shdr.size;
    uint32_t hit_count = pkt->hit_count;
    uint8_t i;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了实例!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len == LE16(0x0010)) {
        /* 对空气进行攻击 */
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt_size != (sizeof(bb_pkt_hdr_t) + (size << 2)) || size < 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的普通攻击数据! %d %d hit_count %d",
            src->guildcard, pkt_size, (sizeof(bb_pkt_hdr_t) + (size << 2)), hit_count);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    /* Handle each thing that was hit */
    for (i = 0; i < pkt->shdr.size - 2; ++i)
        handle_bb_objhit_common(src, l, LE16(pkt->objects[i].obj_id));

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_47_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅用法术攻击实例!",
            src->guildcard);
        return -1;
    }

    if (pkt->shdr.client_id != src->client_id ||
        pkt->technique_number > TECHNIQUE_MEGID ||
        src->equip_flags & EQUIP_FLAGS_DROID
        ) {
        ERR_LOG("GC %" PRIu32 " 职业 %s 发送损坏的 %s 法术攻击数据!",
            src->guildcard, pso_class[src->pl->bb.character.dress_data.ch_class].cn_name,
            max_tech_level[pkt->technique_number].tech_name);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    size_t allowed_count = MIN(pkt->shdr.size - 2, 10);

    if (pkt->target_count > allowed_count) {
        ERR_LOG("GC %" PRIu32 " 职业 %s 发送损坏的 %s 法术攻击数据!",
            src->guildcard, pso_class[src->pl->bb.character.dress_data.ch_class].cn_name,
            max_tech_level[pkt->technique_number].tech_name);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    check_aoe_timer(src, pkt);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_48_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_used_tech_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || src->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 释放了违规的法术!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INFINITE_TP)) {
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, src, SUBCMD60_STAT_TPUP, 255);
    //return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_4A_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_defense_damage_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* This aught to do it... */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_4B_4C_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_take_damage_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        return -1;
    }

    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    if(src->game_data->gm_debug)
        send_lobby_mod_stat(l, src, SUBCMD60_STAT_HPUP, 2000);

    /* This aught to do it... */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_4D_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_death_sync_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || src->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    inventory_t inventory = src->bb_pl->inv;
    size_t mag_index = find_equipped_mag(&inventory);

    item_t mag = inventory.iitems[mag_index].data;
    mag.data2b[0] = MAX((mag.data2b[0] - 5), 0);

    src->game_data->death = 1;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_4E_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_cmd_4e_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令! 0x%02X",
            src->guildcard, pkt->shdr.type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || src->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_4F_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_player_saved_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("错误 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_50_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_switch_req_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了开关!",
            src->guildcard);
        return -1;
    }
    //(00000000)   10 00 60 00 00 00 00 00  50 02 00 00 A3 A6 00 00    ..`.....P.......
    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || src->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_52_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_menu_req_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We don't care about these in lobbies. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        //ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Clear the list of dropped items. */
    if (src->cur_area == 0) {
        memset(src->p2_drops, 0, sizeof(src->p2_drops));
        src->p2_drops_max = 0;
    }

    /* Flip the shopping flag, since this packet is sent both for talking to the
       shop in the first place and when the client exits the shop
       翻转购物标志，因为该数据包在第一时间和客户离开商店时都会发送给商店. */
    src->flags ^= CLIENT_FLAG_SHOPPING;
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_53_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x53_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || 
        pkt->shdr.size != 0x01 || 
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }
//[2023年07月06日 13:23:16:910] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x53 版本 5 的处理
//[2023年07月06日 13:23:16:927] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x53
//( 00000000 )   0C 00 60 00 00 00 00 00   53 01 00 00             ..`.....S...
//[2023年07月06日 13:23:25:808] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x53 版本 5 的处理
//[2023年07月06日 13:23:25:832] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x53
//( 00000000 )   0C 00 60 00 00 00 00 00   53 01 00 00             ..`.....S...
//[2023年07月06日 13:23:34:356] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x53 版本 5 的处理
//[2023年07月06日 13:23:34:374] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x53
//( 00000000 )   0C 00 60 00 00 00 00 00   53 01 00 00             ..`.....S...
//[2023年07月06日 13:23:40:484] 错误(iitems.c 0428): 未从背包中找已装备的玛古
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_55_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_map_warp_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0028) || pkt->shdr.size != 0x08 || pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    if (src->client_id == pkt->shdr.client_id) {

        //DBG_LOG("area = 0x%04X", pkt->area);

#ifdef DEBUG

        switch (pkt->area)
        {
            /* 总督府 实验室 */
        case 0x8000:
            //l->govorlab = 1;
            DBG_LOG("进入总督府任务识别");
            break;

            /* EP1飞船 */
        case 0x0000:

            /* EP2飞船 */
        case 0xC000:

        default:
            //l->govorlab = 0;
            DBG_LOG("离开总督府任务识别");
            break;
        }

#endif // DEBUG

    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_58_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_lobby_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    DBG_LOG("动作ID %d", pkt->act_id);

    // pkt->act_id 大厅动作的对应ID

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_61_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_levelup_req_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014)) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    //ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_63_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_destory_ground_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅房间指令!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    if (l->flags & LOBBY_ITEM_TRACKING_ENABLED) {
        //auto item = l->remove_item(cmd.item_id);
        //auto name = item.data.name(false);
        //l->log.info("地面物品 %08" PRIX32 " 已被摧毁 (%s)", cmd.item_id.load(),
        //    name.c_str());
        //if (c->options.debug) {
        //    string name = item.data.name(true);
        //    send_text_message_printf(c, "$C5物品: destroy/ground %08" PRIX32 "\n%s",
        //        cmd.item_id.load(), name.c_str());
        //}
        //forward_subcommand(l, c, command, flag, data);
        DBG_LOG("开启物品追踪");
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_66_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_use_star_atomizer_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅激活游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* 用于通知其他玩家此区域生成了什么怪物 */

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_67_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_create_enemy_set_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅激活游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0018) || pkt->shdr.size != 0x04) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* 用于通知其他玩家此区域生成了什么怪物 */

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_68_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pipe_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅使用传送门!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0024) || pkt->shdr.size != 0x07) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* See if the user is creating a pipe or destroying it. Destroying a pipe
       always matches the created pipe, but sets the area to 0. We could keep
       track of all of the pipe data, but that's probably overkill. For now,
       blindly accept any pipes where the area is 0
       查看用户是创建管道还是销毁管道。销毁管道总是与创建的管道匹配，
       但会将区域设置为0。我们可以跟踪所有管道数据，
       但这可能太过分了。目前，盲目接受面积为0的任何管道。. */
    if (pkt->area != 0) {
        /* Make sure the user is sending a pipe from the area he or she is
           currently in. */
        if (pkt->area != src->cur_area) {
            ERR_LOG("GC %" PRIu32 " 尝试从传送门传送至不存在的区域 (玩家层级: %d, 传送层级: %d).", src->guildcard,
                src->cur_area, (int)pkt->area);
            return -1;
        }
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_69_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅生成了NPC!",
            src->guildcard);
        return -1;
    }

    /* The only quests that allow NPCs to be loaded are those that require there
       to only be one player, so limit that here. Also, we only allow /npc in
       single-player teams, so that'll fall into line too. */
    if (l->num_clients > 1) {
        ERR_LOG("GC %" PRIu32 " to spawn NPC in multi-"
            "player team!", src->guildcard);
        return -1;
    }

    /* Either this is a legitimate request to spawn a quest NPC, or the player
       is doing something stupid like trying to NOL himself. We don't care if
       someone is stupid enough to NOL themselves, so send the packet on now. */
    return subcmd_send_lobby_bb(l, src, pkt, 0);
}

int sub60_6A_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_Unknown_6x6A_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_72_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            src->guildcard);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_74_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_word_select_t* pkt) {
    /* Don't send the message if they have the protection flag on. */
    if (src->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(src, __(src, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send chats for a STFUed client. */
    if ((src->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }
//( 00000000 )   28 00 60 00 00 00 00 00   
//                     74 08 00 00
//                     02 00 01 00  (.`.....t.......
//( 00000010 )   46 00 C7 02 FF FF FF FF   FF FF FF FF FF FF FF FF  F.?
//( 00000020 )   00 00 00 00 00 00 00 00                           ........
//
//( 00000000 )   28 00 60 00 00 00 00 00   
//                      74 08 00 00 
//                      01 00 01 00  (.`.....t.......
//( 00000010 )   49 00 C7 02 FF FF FF FF   FF FF FF FF FF FF FF FF  I.?
//( 00000020 )   00 00 00 00 00 00 00 00   
    //display_packet(pkt, pkt->hdr.pkt_len);

    return word_select_send_bb(src, pkt);
}

int sub60_75_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_set_flag_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t flag_index = pkt->flag;
    uint16_t action = pkt->action;
    uint16_t difficulty = pkt->difficulty;
    int rv = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (!l || l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    if (flag_index >= 0x400) {
        return 0;
    }

#ifdef DEBUG

    DBG_LOG("GC %" PRIu32 " 触发SET_FLAG指令! flag = 0x%02X action = 0x%02X episode = 0x%02X difficulty = 0x%02X",
        src->guildcard, flag_index, action, l->episode, difficulty);

#endif // DEBUG

    // The client explicitly checks for both 0 and 1 - any other value means no
    // operation is performed.
    size_t bit_index = (difficulty << 10) + flag_index;
    size_t byte_index = bit_index >> 3;
    uint8_t mask = 0x80 >> (bit_index & 7);
    if (action == 0) {
        src->bb_pl->quest_data1[byte_index] |= mask;
    }
    else if (action == 1) {
        src->bb_pl->quest_data1[byte_index] &= (~mask);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_76_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_killed_monster_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中杀掉了怪物!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的杀怪数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

inline int reg_sync_index_bb(lobby_t* l, uint16_t regnum) {
    int i;

    if (!(l->q_flags & LOBBY_QFLAG_SYNC_REGS))
        return -1;

    for (i = 0; i < l->num_syncregs; ++i) {
        if (regnum == l->syncregs[i])
            return i;
    }

    return -1;
}

int sub60_77_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_sync_reg_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t val = LE32(pkt->value);
    int done = 0, idx;
    uint32_t ctl;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的同步数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* TODO: Probably should do some checking here... */
    /* Run the register sync script, if one is set. If the script returns
       non-zero, then assume that it has adequately handled the sync. */
    if ((script_execute(ScriptActionQuestSyncRegister, src, SCRIPT_ARG_PTR, src,
        SCRIPT_ARG_PTR, l, SCRIPT_ARG_UINT8, pkt->register_number,
        SCRIPT_ARG_UINT32, val, SCRIPT_ARG_END))) {
        done = 1;
    }

    /* Does this quest use global flags? If so, then deal with them... */
    if ((l->q_flags & LOBBY_QFLAG_SHORT) && pkt->register_number == l->q_shortflag_reg &&
        !done) {
        /* Check the control bits for sensibility... */
        ctl = (val >> 29) & 0x07;

        /* Make sure the error or response bits aren't set. */
        if ((ctl & 0x06)) {
            DBG_LOG("Quest set flag register with illegal ctl!\n");
            send_sync_register(src, pkt->register_number, 0x8000FFFE);
        }
        /* Make sure we don't have anything with any reserved ctl bits set
           (unless a script has already handled the sync). */
        else if ((val & 0x17000000)) {
            DBG_LOG("Quest set flag register with reserved ctl!\n");
            send_sync_register(src, pkt->register_number, 0x8000FFFE);
        }
        else if ((val & 0x08000000)) {
            /* Delete the flag... */
            shipgate_send_qflag(&ship->sg, src, 1, ((val >> 16) & 0xFF),
                src->cur_lobby->qid, 0, QFLAG_DELETE_FLAG);
        }
        else {
            /* Send the request to the shipgate... */
            shipgate_send_qflag(&ship->sg, src, ctl & 0x01, (val >> 16) & 0xFF,
                src->cur_lobby->qid, val & 0xFFFF, 0);
        }

        done = 1;
    }

    /* Does this quest use server data calls? If so, deal with it... */
    if ((l->q_flags & LOBBY_QFLAG_DATA) && !done) {
        if (pkt->register_number == l->q_data_reg) {
            if (src->q_stack_top < CLIENT_MAX_QSTACK) {
                if (!(src->flags & CLIENT_FLAG_QSTACK_LOCK)) {
                    src->q_stack[src->q_stack_top++] = val;

                    /* Check if we've got everything we expected... */
                    if (src->q_stack_top >= 3 &&
                        src->q_stack_top == 3 + src->q_stack[1] + src->q_stack[2]) {
                        /* Call the function requested and reset the stack. */
                        ctl = quest_function_dispatch(src, l);

                        if (ctl != QUEST_FUNC_RET_NOT_YET) {
                            send_sync_register(src, pkt->register_number, ctl);
                            src->q_stack_top = 0;
                        }
                    }
                }
                else {
                    /* The stack is locked, ignore the write and report the
                       error. */
                    send_sync_register(src, pkt->register_number,
                        QUEST_FUNC_RET_STACK_LOCKED);
                }
            }
            else if (src->q_stack_top == CLIENT_MAX_QSTACK) {
                /* Eat the stack push and report an error. */
                send_sync_register(src, pkt->register_number,
                    QUEST_FUNC_RET_STACK_OVERFLOW);
            }

            done = 1;
        }
        else if (pkt->register_number == l->q_ctl_reg) {
            /* For now, the only reason we'll have one of these is to reset the
               stack. There might be other reasons later, but this will do, for
               the time being... */
            src->q_stack_top = 0;
            done = 1;
        }
    }

    /* Does this register have to be synced? */
    if ((idx = reg_sync_index_bb(l, pkt->register_number)) != -1) {
        l->regvals[idx] = val;
    }

    if (!done)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    return 0;
}

int sub60_79_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_gogo_ball_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    // pkt->act_id 大厅动作的对应ID

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int handle_bb_battle_mode(ship_client_t* src, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int ch, ch2;
    ship_client_t* lc = { 0 };

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            src->guildcard);
        return -1;
    }

    if ((l->battle) && (src->mode)) {
        // Battle restarting...
        //
        // If rule #1 we'll copy the character backup to the character array, otherwise
        // we'll reset the character...
        //
        for (ch = 0; ch < l->max_clients; ch++) {
            if ((l->clients_slot[ch]) && (l->clients[ch])) {
                lc = l->clients[ch];
                switch (lc->mode) {
                case 0x01:
                case 0x02:
                    // Copy character backup
                    if (lc->game_data->char_backup && lc->guildcard)
                        memcpy(&lc->pl->bb.character, lc->game_data->char_backup, sizeof(lc->pl->bb.character));

                    if (lc->mode == 0x02) {
                        for (ch2 = 0; ch2 < lc->pl->bb.inv.item_count; ch2++)
                        {
                            if (lc->pl->bb.inv.iitems[ch2].data.datab[0] == 0x02)
                                lc->pl->bb.inv.iitems[ch2].present = 0;
                        }
                        //CleanUpInventory(lc);
                        lc->pl->bb.character.disp.meseta = 0;
                    }
                    break;
                case 0x03:
                    // Wipe items and reset level.
                    for (ch2 = 0; ch2 < 30; ch2++)
                        lc->pl->bb.inv.iitems[ch2].present = 0;
                    //CleanUpInventory(lc);

                    uint8_t ch_class = lc->pl->bb.character.dress_data.ch_class;

                    lc->pl->bb.character.disp.level = 0;
                    lc->pl->bb.character.disp.exp = 0;

                    lc->pl->bb.character.disp.stats.atp = bb_char_stats.start_stats[ch_class].atp;
                    lc->pl->bb.character.disp.stats.mst = bb_char_stats.start_stats[ch_class].mst;
                    lc->pl->bb.character.disp.stats.evp = bb_char_stats.start_stats[ch_class].evp;
                    lc->pl->bb.character.disp.stats.hp = bb_char_stats.start_stats[ch_class].hp;
                    lc->pl->bb.character.disp.stats.dfp = bb_char_stats.start_stats[ch_class].dfp;
                    lc->pl->bb.character.disp.stats.ata = bb_char_stats.start_stats[ch_class].ata;

                    /*if (l->battle_level > 1)
                        SkipToLevel(l->battle_level - 1, lc, 1);*/
                    lc->pl->bb.character.disp.meseta = 0;
                    break;
                default:
                    // Unknown mode?
                    break;
                }
            }
        }
        // Reset boxes and monsters...
        //memset(&l->boxHit, 0, 0xB50); // Reset box and monster data
        //memset(&l->monsterData, 0, sizeof(l->monsterData));
    }

    return 0;
}

int handle_bb_challenge_mode_grave(ship_client_t* src, 
    subcmd_bb_pkt_t* pkt) {
    int i;
    lobby_t* l = src->cur_lobby;
    subcmd_pc_grave_t pc = { { 0 } };
    subcmd_dc_grave_t dc = { { 0 } };
    subcmd_bb_grave_t bb = { { 0 } };
    size_t in, out;
    char* inptr;
    char* outptr;

    /* Deal with converting the different versions... */
    switch (src->version) {
    case CLIENT_VERSION_DCV2:
        memcpy(&dc, pkt, sizeof(subcmd_dc_grave_t));

        /* Make a copy to send to PC players... */
        pc.hdr.pkt_type = GAME_COMMAND0_TYPE;
        pc.hdr.pkt_len = LE16(0x00E4);

        pc.shdr.type = SUBCMD60_GAME_MODE;
        pc.shdr.size = 0x38;
        pc.shdr.client_id = dc.shdr.client_id;
        pc.client_id2 = dc.client_id2;
        //pc.unk0 = dc.unk0;
        //pc.unk1 = dc.unk1;

        for (i = 0; i < 0x0C; ++i) {
            pc.string[i] = LE16(dc.string[i]);
        }

        memcpy(pc.unk2, dc.unk2, 0x24);
        pc.grave_unk4 = dc.grave_unk4;
        pc.deaths = dc.deaths;
        pc.coords_time[0] = dc.coords_time[0];
        pc.coords_time[1] = dc.coords_time[1];
        pc.coords_time[2] = dc.coords_time[2];
        pc.coords_time[3] = dc.coords_time[3];
        pc.coords_time[4] = dc.coords_time[4];

        /* Convert the team name */
        in = 20;
        out = 40;
        inptr = dc.team;
        outptr = (char*)pc.team;

        if (dc.team[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        /* Convert the message */
        in = 24;
        out = 48;
        inptr = dc.message;
        outptr = (char*)pc.message;

        if (dc.message[1] == 'J') {
            iconv(ic_sjis_to_utf16, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_8859_to_utf16, &inptr, &in, &outptr, &out);
        }

        memcpy(pc.times, dc.times, 36);
        pc.unk = dc.unk;
        break;

    case CLIENT_VERSION_PC:
        memcpy(&pc, pkt, sizeof(subcmd_pc_grave_t));

        /* Make a copy to send to DC players... */
        dc.hdr.pkt_type = GAME_COMMAND0_TYPE;
        dc.hdr.pkt_len = LE16(0x00AC);

        dc.shdr.type = SUBCMD60_GAME_MODE;
        dc.shdr.size = 0x2A;
        dc.shdr.client_id = pc.shdr.client_id;
        dc.client_id2 = pc.client_id2;
        //dc.unk0 = pc.unk0;
        //dc.unk1 = pc.unk1;

        for (i = 0; i < 0x0C; ++i) {
            dc.string[i] = (char)LE16(dc.string[i]);
        }

        memcpy(dc.unk2, pc.unk2, 0x24);
        dc.grave_unk4 = pc.grave_unk4;
        dc.deaths = pc.deaths;
        dc.coords_time[0] = pc.coords_time[0];
        dc.coords_time[1] = pc.coords_time[1];
        dc.coords_time[2] = pc.coords_time[2];
        dc.coords_time[3] = pc.coords_time[3];
        dc.coords_time[4] = pc.coords_time[4];

        /* Convert the team name */
        in = 40;
        out = 20;
        inptr = (char*)pc.team;
        outptr = dc.team;

        if (pc.team[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        /* Convert the message */
        in = 48;
        out = 24;
        inptr = (char*)pc.message;
        outptr = dc.message;

        if (pc.message[1] == LE16('J')) {
            iconv(ic_utf16_to_sjis, &inptr, &in, &outptr, &out);
        }
        else {
            iconv(ic_utf16_to_8859, &inptr, &in, &outptr, &out);
        }

        memcpy(dc.times, pc.times, 36);
        dc.unk = pc.unk;
        break;

    case CLIENT_VERSION_BB:
        memcpy(&bb, pkt, sizeof(subcmd_bb_grave_t));
        //display_packet((unsigned char*)&bb, LE16(pkt->hdr.pkt_len));
        break;

    default:
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Send the packet to everyone in the lobby */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != src) {
            switch (l->clients[i]->version) {
            case CLIENT_VERSION_DCV2:
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&dc);
                break;

            case CLIENT_VERSION_PC:
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&pc);
                break;

            case CLIENT_VERSION_BB:
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&bb);
                break;
            }
        }
    }

    return 0;
}

int sub60_7C_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            src->guildcard);
        return -1;
    }

    if ((l->battle) && (src->mode))
        handle_bb_battle_mode(src, pkt);
    else 
        handle_bb_challenge_mode_grave(src, pkt);

    return 0;
}

int sub60_80_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_trigger_trap_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) ||
        pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

//[2023年07月06日 13:21:25:848] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x80 版本 5 的处理
//[2023年07月06日 13:21:25:865] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x80
//( 00000000 )   10 00 60 00 00 00 00 00   80 02 FC 47 50 00 02 00  ..`.....�.麲P...

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_83_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_place_trap_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || 
        pkt->shdr.size != 0x02 || 
        src->client_id != pkt->shdr.client_id
        ) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }
//[2023年07月06日 13:21:22:702] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x83 版本 5 的处理
//[2023年07月06日 13:21:22:717] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x83
//( 00000000 )   10 00 60 00 00 00 00 00   83 02 00 00 03 00 14 00  ..`.....?......

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_88_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_arrow_change_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || src->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    src->arrow_color = pkt->shdr.client_id;
    return send_lobby_arrows(l);
}

int sub60_89_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_player_died_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令! 0x%02X",
            src->guildcard, pkt->shdr.type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || src->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的死亡数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_8A_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_ch_game_select_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    src->mode = pkt->mode;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_8D_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_set_technique_level_override_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || src->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 释放了违规的法术!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /*uint8_t tmp_level = pkt->level_upgrade;

    pkt->level_upgrade = tmp_level+100;*/

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_93_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_timed_switch_activated_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    display_packet(pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_97_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_ch_game_cancel_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x001C) || pkt->shdr.size != 0x05) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    //src->mode = pkt->mode;

    ///* Send the packet to every connected client. */
    //for (int i = 0; i < l->max_clients; ++i) {
    //    if (l->clients[i] && l->clients[i] != src) {
    //        l->clients[i]->mode = pkt->mode;
    //    }
    //}

    //UDONE_CSPD(pkt->hdr.pkt_type, src->version, pkt);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_9A_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_update_player_stat_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

//[2023年07月06日 12:31:43:857] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x9A 版本 5 的处理
//[2023年07月06日 12:31:43:869] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x9A
//( 00000000 )   10 00 60 00 00 00 00 00   9A 02 00 00 00 00 03 0D  ..`.....?......

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_9C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x9C_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

//[2023年07月06日 13:15:14:214] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x9C 版本 5 的处理
//[2023年07月06日 13:15:14:233] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x9C
//( 00000000 )   10 00 60 00 00 00 00 00   9C 02 F0 10 10 00 00 00  ..`.....??....
//[2023年07月06日 13:15:14:306] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x9C 版本 5 的处理
//[2023年07月06日 13:15:14:322] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x9C
//( 00000000 )   10 00 60 00 00 00 00 00   9C 02 EF 10 10 00 00 00  ..`.....??....
//[2023年07月06日 13:16:38:651] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x9C 版本 5 的处理
//[2023年07月06日 13:16:38:666] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x9C
//( 00000000 )   10 00 60 00 00 00 00 00   9C 02 12 11 10 00 00 00  ..`.....?......
//[2023年07月06日 13:16:39:680] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x9C 版本 5 的处理
//[2023年07月06日 13:16:39:696] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x9C
//( 00000000 )   10 00 60 00 00 00 00 00   9C 02 13 11 10 00 00 00  ..`.....?......
//[2023年07月06日 13:16:42:718] 错误(subcmd_handle.c 0111): subcmd_get_handler 未完成对 0x60 0x9C 版本 5 的处理
//[2023年07月06日 13:16:42:735] 调试(subcmd_handle_60.c 3061): 未知 0x60 指令: 0x9C
//( 00000000 )   10 00 60 00 00 00 00 00   9C 02 11 11 10 00 00 00  ..`.....?......

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_9D_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_ch_game_failure_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_A1_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_save_player_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("错误 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_AB_AF_B0_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅房间指令!",
            src->guildcard);
        return -1;
    }

    switch (type)
    {
        /* 0xAB */
    case SUBCMD60_CHAIR_CREATE:
        subcmd_bb_create_lobby_chair_t* pktab = (subcmd_bb_create_lobby_chair_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_CREATE %u %u ", pktab->unknown_a1, pktab->unknown_a2);

        break;

        /* 0xAF */
    case SUBCMD60_CHAIR_TURN:
        subcmd_bb_turn_lobby_chair_t* pktaf = (subcmd_bb_turn_lobby_chair_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_TURN %02X", pktaf->angle);
        break;

        /* 0xB0 */
    case SUBCMD60_CHAIR_MOVE:
        subcmd_bb_move_lobby_chair_t* pktb0 = (subcmd_bb_move_lobby_chair_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_MOVE %02X", pktb0->angle);
        break;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_C0_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_sell_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    size_t i;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || 
        pkt->shdr.size != 0x03 || 
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    i = find_iitem_index(&src->bb_pl->inv, pkt->item_id);

    uint32_t shop_price = get_bb_shop_price(&src->bb_pl->inv.iitems[i]) * pkt->sell_amount;

    src->bb_pl->character.disp.meseta = MIN(
        src->bb_pl->character.disp.meseta + shop_price, 999999);

    iitem_t item = remove_iitem(src, pkt->item_id, pkt->sell_amount, src->version != CLIENT_VERSION_BB);
    if (&item == NULL) {
        src->bb_pl->character.disp.meseta -= shop_price;
        ERR_LOG("出售 %d ID 0x%04X 失败", pkt->sell_amount, pkt->item_id);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_C3_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_drop_split_stacked_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    iitem_t* it;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅丢弃了物品!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    iitem_t iitem = remove_iitem(src, pkt->item_id, pkt->amount, src->version != CLIENT_VERSION_BB);

    if (&iitem == NULL) {
        ERR_LOG("GC %" PRIu32 " 掉落堆叠物品失败!",
            src->guildcard);
        return -1;
    }

    if (iitem.data.item_id == 0xFFFFFFFF) {
        iitem.data.item_id = generate_item_id(l, EMPTY_STRING);
    }

    if (!add_iitem(src, &iitem)) {
        ERR_LOG("GC %" PRIu32 " 物品返回玩家背包失败!",
            src->guildcard);
        return -1;
    }

    /* We have the item... Add it to the lobby's inventory. */
    if (!(it = add_litem_locked(l, &iitem))) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("无法将物品添加至游戏房间!");
        return -1;
    }

    return subcmd_send_lobby_drop_stack(src, NULL, pkt->area, pkt->x, pkt->z, it);
}

int sub60_C4_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_sort_inv_t* pkt) {
    lobby_t* l = src->cur_lobby;
    inventory_t sorted = { 0 };
    size_t x = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅整理了物品!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0084) || pkt->shdr.size != 0x1F) {
        ERR_LOG("GC %" PRIu32 " 发送错误的整理物品数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    for (x = 0; x < MAX_PLAYER_INV_ITEMS; x++) {
        if (pkt->item_ids[x] == 0xFFFFFFFF) {
            sorted.iitems[x].data.item_id = 0xFFFFFFFF;
        }
        else {
            int index = find_iitem_index(&src->bb_pl->inv, pkt->item_ids[x]);
            sorted.iitems[x] = src->bb_pl->inv.iitems[index];
        }
    }

    sorted.item_count = src->bb_pl->inv.item_count;
    sorted.hpmats_used = src->bb_pl->inv.hpmats_used;
    sorted.tpmats_used = src->bb_pl->inv.tpmats_used;
    sorted.language = src->bb_pl->inv.language;

    src->bb_pl->inv = sorted;
    src->pl->bb.inv = sorted;

    /* Nobody else really needs to care about this one... */
    return 0;
}

int sub60_C5_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_medical_center_used_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅使用医疗中心!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误的医疗中心数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* Subtract 10 meseta from the client. */
    src->bb_pl->character.disp.meseta -= 10;
    src->pl->bb.character.disp.meseta = src->bb_pl->character.disp.meseta;

    /* Send it along to the rest of the lobby. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_C6_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_steal_exp_t* pkt) {
    lobby_t* l = src->cur_lobby;
    pmt_weapon_bb_t tmp_wp = { 0 };
    game_enemy_t* en;
    uint32_t exp_percent = 0;
    uint32_t exp_to_add;
    uint8_t special = 0;
    uint16_t mid;
    uint32_t bp, exp_amount;
    int i;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            src->guildcard);
        return -1;
    }

    mid = LE16(pkt->shdr.enemy_id);
    mid &= 0xFFF;

    if (mid < 0xB50) {
        for (i = 0; i < src->bb_pl->inv.item_count; i++) {
            if ((src->bb_pl->inv.iitems[i].flags & LE32(0x00000008)) &&
                (src->bb_pl->inv.iitems[i].data.datab[0] == ITEM_TYPE_WEAPON)) {
                if ((src->bb_pl->inv.iitems[i].data.datab[1] < 0x0A) &&
                    (src->bb_pl->inv.iitems[i].data.datab[2] < 0x05)) {
                    special = (src->bb_pl->inv.iitems[i].data.datab[4] & 0x1F);
                }
                else {
                    if ((src->bb_pl->inv.iitems[i].data.datab[1] < 0x0D) &&
                        (src->bb_pl->inv.iitems[i].data.datab[2] < 0x04))
                        special = (src->bb_pl->inv.iitems[i].data.datab[4] & 0x1F);
                    else {
                        if (pmt_lookup_weapon_bb(src->bb_pl->inv.iitems[i].data.datal[0], &tmp_wp)) {
                            ERR_LOG("GC %" PRIu32 " 装备了不存在的物品数据!",
                                src->guildcard);
                            return -1;
                        }

                        special = tmp_wp.special_type;
                    }
                }

                switch (special) {
                case 0x09:
                    // Master's
                    exp_percent = 8;
                    break;

                case 0x0A:
                    // Lord's
                    exp_percent = 10;
                    break;

                case 0x0B:
                    // King's
                    exp_percent = 12;
                    if ((l->difficulty == 0x03) &&
                        (src->equip_flags & EQUIP_FLAGS_DROID))
                        exp_percent += 30;
                    break;
                }

                break;
            }
        }

        if (exp_percent) {
            /* Make sure the enemy is in range. */
            mid = LE16(pkt->shdr.enemy_id);
            //DBG_LOG("怪物编号原值 %02X %02X", mid, pkt->enemy_id2);
            mid &= 0xFFF;
            //DBG_LOG("怪物编号新值 %02X map_enemies->count %02X", mid, l->map_enemies->count);

            if (mid > l->map_enemies->count) {
                ERR_LOG("GC %" PRIu32 " 杀掉了无效的敌人 (%d -- "
                    "敌人数量: %d)!", src->guildcard, mid, l->map_enemies->count);
                return -1;
            }

            /* Make sure this client actually hit the enemy and that the client didn't
               already claim their experience. */
            en = &l->map_enemies->enemies[mid];

            if (!(en->clients_hit & (1 << src->client_id))) {
                return 0;
            }

            /* Set that the client already got their experience and that the monster is
               indeed dead. */
            en->clients_hit = (en->clients_hit & (~(1 << src->client_id))) | 0x80;

            /* Give the client their experience! */
            bp = en->bp_entry;

            //新增经验倍率提升参数 exp_mult expboost 11.18
            exp_amount = (l->bb_params[bp].exp * exp_percent) / 100L;

            if (exp_amount > 80)  // Limit the amount of exp stolen to 80
                exp_amount = 80;

            if ((src->game_data->expboost) && (l->exp_mult > 0)) {
                exp_to_add = exp_amount;
                exp_amount = exp_to_add + (l->bb_params[bp].exp * l->exp_mult);
            }

            if (!exp_amount) {
                ERR_LOG("未获取到经验新值 %d bp %d 倍率 %d", exp_amount, bp, l->exp_mult);
                return client_give_exp(src, 1);
            }

            return client_give_exp(src, exp_amount);
        }
    }
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_C7_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_charge_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发游戏指令!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 ||
        pkt->shdr.client_id != src->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误的数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    psocn_disp_char_t* disp = &src->bb_pl->character.disp;
    if (pkt->meseta_amount > disp->meseta) {
        disp->meseta = 0;
    }
    else {
        disp->meseta -= pkt->meseta_amount;
    }

    /* Send it along to the rest of the lobby. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

/* 从怪物获取的经验倍率未进行调整 */
int sub60_C8_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_req_exp_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t mid;
    uint32_t bp, exp_amount;
    game_enemy_t* en;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中获取经验!",
            src->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的经验获取数据包!",
            src->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    /* Make sure the enemy is in range. */
    mid = LE16(pkt->enemy_id2);
    //DBG_LOG("怪物编号原值 %02X %02X", mid, pkt->enemy_id2);
    mid &= 0xFFF;
    //DBG_LOG("怪物编号新值 %02X map_enemies->count %02X", mid, l->map_enemies->count);

    if (mid > l->map_enemies->count) {
        ERR_LOG("GC %" PRIu32 " 杀掉了无效的敌人 (%d -- "
            "敌人数量: %d)!", src->guildcard, mid, l->map_enemies->count);
        return -1;
    }

    /* Make sure this client actually hit the enemy and that the client didn't
       already claim their experience. */
    en = &l->map_enemies->enemies[mid];

    if (!(en->clients_hit & (1 << src->client_id))) {
        return 0;
    }

    /* Set that the client already got their experience and that the monster is
       indeed dead. */
    en->clients_hit = (en->clients_hit & (~(1 << src->client_id))) | 0x80;

    /* Give the client their experience! */
    bp = en->bp_entry;
    exp_amount = l->bb_params[bp].exp + 100000;

    //DBG_LOG("经验原值 %d bp %d", exp_amount, bp);

    if ((src->game_data->expboost) && (l->exp_mult > 0))
        exp_amount = l->bb_params[bp].exp * l->exp_mult;

    if (!exp_amount)
        ERR_LOG("未获取到经验新值 %d bp %d 倍率 %d", exp_amount, bp, l->exp_mult);

    // TODO 新增房间共享经验 分别向其余3个玩家发送数值不等的经验值
    if (!pkt->last_hitter) {
        exp_amount = (exp_amount * 80) / 100;
    }

    return client_give_exp(src, exp_amount);
}

int sub60_CC_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_guild_ex_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t ex_item_id = pkt->ex_item_id, ex_amount = pkt->ex_amount;
    int found = 0;

    //[2023年02月14日 18:07:48:964] 调试(subcmd-bb.c 5348): 暂未完成 0x60: 0xCC
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  CC 03 00 00 00 00 01 00    ..`.............
    //( 00000010 )   01 00 00 00                                         ....
    //[2023年02月14日 18:07:56:864] 调试(block-bb.c 2347): 舰仓：BB 公会功能指令 0x18EA BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO - 0x18EA (长度8)
    //( 00000000 )   08 00 EA 18 00 00 00 00                             ........
    //[2023年02月14日 18:07:56:890] 调试(ship_packets.c 11953): 向GC 42004063 发送公会指令 0x18EA
    //[2023年02月14日 18:08:39:462] 调试(subcmd-bb.c 5348): 暂未完成 0x60: 0xCC
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  CC 03 00 00 06 00 01 00    ..`.............
    //( 00000010 )   01 00 00 00          
//[2023年02月14日 18:16:59:989] 调试(subcmd-bb.c 5348): 暂未完成 0x60: 0xCC
//
//( 00000000 )   14 00 60 00 00 00 00 00  CC 03 01 00 00 00 81 00    ..`.............
//( 00000010 )   01 00 00 00                                         ....

    if (pkt->shdr.client_id != src->client_id || ex_amount == 0) {
        DBG_LOG("错误 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    if (ex_item_id == EMPTY_STRING) {
        DBG_LOG("错误 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }

    iitem_t item = remove_iitem(src, pkt->ex_item_id, pkt->ex_amount, src->version != CLIENT_VERSION_BB);

    if (&item == NULL) {
        ERR_LOG("无法从玩家背包中移除 %d ID 0x%04X 物品!", pkt->ex_amount, pkt->ex_item_id);
        return -1;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

int sub60_D2_bb(ship_client_t* src, ship_client_t* dest, 
    subcmd_bb_gallon_area_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t quest_offset = pkt->quest_offset;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发任务指令!",
            src->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            src->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
        return -1;
    }
    //[2023年02月09日 21:50:51:617] 调试(subcmd-bb.c 4858): Unknown 0x60: 0xD2
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  D2 03 FF FF 05 00 00 00    ..`.............
    //( 00000010 )   F3 AD 00 00                                         ....
    //[2023年02月09日 21:50:51:653] 调试(subcmd-bb.c 4858): Unknown 0x60: 0xD2
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  D2 03 FF FF 05 00 00 00    ..`.............
    //( 00000010 )   9A D7 00 00                                         ....
    if (quest_offset < 23) {
        quest_offset *= 4;
        *(uint32_t*)&src->bb_pl->quest_data2[quest_offset] = *(uint32_t*)&pkt->data[0];
    }

    return send_pkt_bb(src, (bb_pkt_hdr_t*)pkt);

}

// 定义函数指针数组
subcmd_handle_func_t subcmd60_handler[] = {
    //cmd_type 00 - 0F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_SWITCH_CHANGED         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_05_bb },
    { SUBCMD60_SYMBOL_CHAT            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_07_bb },
    { SUBCMD60_HIT_MONSTER            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_0A_bb },
    { SUBCMD60_HIT_OBJ                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_0B_bb },
    { SUBCMD60_CONDITION_ADD          , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_0C_bb },
    { SUBCMD60_CONDITION_REMOVE       , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_0C_bb },

    //cmd_type 10 - 1F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_DRAGON_ACT             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_12_bb },
    { SUBCMD60_ACTION_DE_ROl_LE       , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_13_bb },
    { SUBCMD60_ACTION_DE_ROl_LE2      , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_14_bb },
    { SUBCMD60_ACTION_VOL_OPT         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_15_bb },
    { SUBCMD60_ACTION_VOL_OPT2        , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_16_bb },
    { SUBCMD60_TELEPORT               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_17_bb },
    { SUBCMD60_UNKNOW_18              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_18_bb },
    { SUBCMD60_ACTION_DARK_FALZ       , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_19_bb },
    { SUBCMD60_DESTORY_NPC            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_1C_bb },
    { SUBCMD60_SET_AREA_1F            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_1F_bb },

    //cmd_type 20 - 2F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_SET_AREA_20            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_20_bb },
    { SUBCMD60_INTER_LEVEL_WARP       , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_21_bb },
    { SUBCMD60_LOAD_22                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_22_bb },
    { SUBCMD60_FINISH_LOAD            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_23_bb },
    { SUBCMD60_SET_POS_24             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_24_bb },
    { SUBCMD60_EQUIP                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_25_bb },
    { SUBCMD60_REMOVE_EQUIP           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_26_bb },
    { SUBCMD60_USE_ITEM               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_27_bb },
    { SUBCMD60_FEED_MAG               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_28_bb },
    { SUBCMD60_DELETE_ITEM            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_29_bb },
    { SUBCMD60_DROP_ITEM              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_2A_bb },
    { SUBCMD60_SELECT_MENU            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_2C_bb },
    { SUBCMD60_SELECT_DONE            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_2D_bb },
    { SUBCMD60_HIT_BY_ENEMY           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_2F_bb },

    //cmd_type 30 - 3F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_MEDIC_REQ              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_check_client_id_bb },
    { SUBCMD60_MEDIC_DONE             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_check_client_id_bb },
    { SUBCMD60_PB_BLAST_READY         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_39_bb },
    { SUBCMD60_GAME_CLIENT_LEAVE      , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_3A_bb },
    { SUBCMD60_LOAD_3B                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_3B_bb },
    { SUBCMD60_SET_POS_3E             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_3E_3F_bb },
    { SUBCMD60_SET_POS_3F             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_3E_3F_bb },

    //cmd_type 40 - 4F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_MOVE_SLOW              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_40_42_bb },
    { SUBCMD60_MOVE_FAST              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_40_42_bb },
    { SUBCMD60_ATTACK1                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_43_44_45_bb },
    { SUBCMD60_ATTACK2                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_43_44_45_bb },
    { SUBCMD60_ATTACK3                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_43_44_45_bb },
    { SUBCMD60_OBJHIT_PHYS            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_46_bb },
    { SUBCMD60_OBJHIT_TECH            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_47_bb },
    { SUBCMD60_USED_TECH              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_48_bb },
    { SUBCMD60_DEFENSE_DAMAGE         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_4A_bb },
    { SUBCMD60_TAKE_DAMAGE1           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_4B_4C_bb },
    { SUBCMD60_TAKE_DAMAGE2           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_4B_4C_bb },
    { SUBCMD60_DEATH_SYNC             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_4D_bb },
    { SUBCMD60_UNKNOW_4E              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_4E_bb },
    { SUBCMD60_PLAYER_SAVED           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_4F_bb },

    //cmd_type 50 - 5F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_SWITCH_REQ             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_50_bb },
    { SUBCMD60_MENU_REQ               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_52_bb },
    { SUBCMD60_UNKNOW_53              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_53_bb },
    { SUBCMD60_WARP_55                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_55_bb },
    { SUBCMD60_LOBBY_ACTION           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_58_bb },

    //cmd_type 60 - 6F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_LEVEL_UP_REQ           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_61_bb },
    { SUBCMD60_DESTROY_GROUND_ITEM    , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_63_bb },
    { SUBCMD60_USE_STAR_ATOMIZER      , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_66_bb },
    { SUBCMD60_CREATE_ENEMY_SET       , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_67_bb },
    { SUBCMD60_CREATE_PIPE            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_68_bb },
    { SUBCMD60_SPAWN_NPC              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_69_bb },
    { SUBCMD60_UNKNOW_6A              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_6A_bb },

    //cmd_type 70 - 7F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_BURST_DONE             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_72_bb },
    { SUBCMD60_WORD_SELECT            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_74_bb },
    { SUBCMD60_SET_FLAG               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_75_bb },
    { SUBCMD60_KILL_MONSTER           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_76_bb },
    { SUBCMD60_SYNC_REG               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_77_bb },
    { SUBCMD60_GOGO_BALL              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_79_bb },
    { SUBCMD60_GAME_MODE              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_7C_bb },

    //cmd_type 80 - 8F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_TRIGGER_TRAP           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_80_bb },
    { SUBCMD60_PLACE_TRAP             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_83_bb },
    { SUBCMD60_ARROW_CHANGE           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_88_bb },
    { SUBCMD60_PLAYER_DIED            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_89_bb },
    { SUBCMD60_CH_GAME_SELECT         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_8A_bb },
    { SUBCMD60_OVERRIDE_TECH_LEVEL    , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_8D_bb },

    //cmd_type 90 - 9F                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_TIMED_SWITCH_ACTIVATED , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_93_bb },
    { SUBCMD60_CH_GAME_CANCEL         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_97_bb },
    { SUBCMD60_CHANGE_STAT            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_9A_bb },
    { SUBCMD60_UNKNOW_9C              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_9C_bb },
    { SUBCMD60_CH_GAME_FINISHED       , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_9D_bb },

    //cmd_type A0 - AF                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_SAVE_PLAYER_ACT        , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_A1_bb },
    { SUBCMD60_CHAIR_CREATE           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_AB_AF_B0_bb },
    { SUBCMD60_CHAIR_TURN             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_AB_AF_B0_bb },

    //cmd_type B0 - BF                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_CHAIR_MOVE             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_AB_AF_B0_bb },

    //cmd_type C0 - CF                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_SELL_ITEM              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C0_bb },
    { SUBCMD60_DROP_SPLIT_ITEM        , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C3_bb },
    { SUBCMD60_SORT_INV               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C4_bb },
    { SUBCMD60_MEDIC                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C5_bb },
    { SUBCMD60_STEAL_EXP              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C6_bb },
    { SUBCMD60_CHARGE_ACT             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C7_bb },
    { SUBCMD60_EXP_REQ                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C8_bb },
    { SUBCMD60_GUILD_EX_ITEM          , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_CC_bb },

    //cmd_type D0 - DF                  DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_GALLON_AREA            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_D2_bb },
    { SUBCMD60_EX_ITEM_MK             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_unimplement_bb },
};
//
//// 使用函数指针直接调用相应的处理函数
//subcmd60_handle_t subcmd60_get_handler(int cmd_type, int version) {
//    for (int i = 0; i < _countof(subcmd60_handle); i++) {
//        if (subcmd60_handle[i].cmd_type == cmd_type) {
//
//            switch (version)
//            {
//            case CLIENT_VERSION_DCV1:
//            case CLIENT_VERSION_DCV2:
//            case CLIENT_VERSION_PC:
//            case CLIENT_VERSION_GC:
//                return subcmd60_handle[i].dc;
//
//            case CLIENT_VERSION_EP3:
//            case CLIENT_VERSION_XBOX:
//                break;
//
//            case CLIENT_VERSION_BB:
//                return subcmd60_handle[i].bb;
//            }
//        }
//    }
//
//    ERR_LOG("subcmd60_get_handler 未完成对 0x60 0x%02X 版本 %d 的处理", cmd_type, version);
//
//    return NULL;
//}

/* 处理BB 0x60 数据包. */
int subcmd_bb_handle_60(ship_client_t* src, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    ship_client_t* dest;
    uint16_t len = pkt->hdr.pkt_len;
    uint16_t hdr_type = pkt->hdr.pkt_type;
    uint8_t type = pkt->type;
    int rv = 0, sent = 1, i = 0;
    uint32_t dnum = LE32(pkt->hdr.flags);

    /* 如果客户端不在大厅或者队伍中则忽略数据包. */
    if (!l)
        return 0;

#ifdef DEBUG_60

    DBG_LOG("玩家 0x%02X 指令: 0x%02X", hdr_type, type);

#endif // DEBUG_60

    pthread_mutex_lock(&l->mutex);

    /* 搜索目标客户端. */
    dest = l->clients[dnum];

    /* 目标客户端已离线，将不再发送数据包. */
    if (!dest) {
        //DBG_LOG("不存在 dest 玩家 0x%02X 指令: 0x%X", hdr_type, type);
        pthread_mutex_unlock(&l->mutex);
        return 0;
    }

    subcmd_bb_60size_check(src, pkt);

    l->subcmd_handle = subcmd_get_handler(hdr_type, type, src->version);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        switch (type) {
        case SUBCMD60_SET_POS_3F://大厅跃迁时触发 1
        case SUBCMD60_SET_AREA_1F://大厅跃迁时触发 2
        case SUBCMD60_LOAD_3B://大厅跃迁时触发 3
        case SUBCMD60_BURST_DONE:
            /* 0x7C 挑战模式 进入房间游戏未开始前触发*/
        case SUBCMD60_GAME_MODE:
            rv = l->subcmd_handle(src, dest, pkt);
            break;

        default:
            DBG_LOG("LOBBY_FLAG_BURSTING 0x60 指令: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, src, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    if (l->subcmd_handle == NULL) {
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x%02X 指令: 0x%02X", hdr_type, type);
        display_packet(pkt, pkt->hdr.pkt_len);
#endif /* BB_LOG_UNKNOWN_SUBS */
        rv = subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    rv = l->subcmd_handle(src, dest, pkt);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}
