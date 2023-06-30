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

#include "subcmd_bb_handle_60.h"

static int handle_bb_cmd_check_client_id(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (pkt->param != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 在触发了游戏房间内 %s 指令! 且Client ID不一致",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    //UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_lobby(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅 %s 指令!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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

    display_packet(pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static inline int tlindex(uint8_t l) {
    switch (l) {
    case 0: case 1: case 2: case 3: case 4: return 0;
    case 5: case 6: case 7: case 8: case 9: return 1;
    case 10: case 11: case 12: case 13: case 14: return 2;
    case 15: case 16: case 17: case 18: case 19: return 3;
    case 20: case 21: case 22: case 23: case 25: return 4;
    default: return 5;
    }
}

static void update_bb_qpos(ship_client_t* c, lobby_t* l) {
    uint8_t r;

    if ((r = l->qpos_regs[c->client_id][0])) {
        send_sync_register(l->clients[0], r, (uint32_t)c->x);
        send_sync_register(l->clients[0], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[0], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[0], r + 3, (uint32_t)c->cur_area);
    }
    if ((r = l->qpos_regs[c->client_id][1])) {
        send_sync_register(l->clients[1], r, (uint32_t)c->x);
        send_sync_register(l->clients[1], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[1], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[1], r + 3, (uint32_t)c->cur_area);
    }
    if ((r = l->qpos_regs[c->client_id][2])) {
        send_sync_register(l->clients[2], r, (uint32_t)c->x);
        send_sync_register(l->clients[2], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[2], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[2], r + 3, (uint32_t)c->cur_area);
    }
    if ((r = l->qpos_regs[c->client_id][3])) {
        send_sync_register(l->clients[3], r, (uint32_t)c->x);
        send_sync_register(l->clients[3], r + 1, (uint32_t)c->y);
        send_sync_register(l->clients[3], r + 2, (uint32_t)c->z);
        send_sync_register(l->clients[3], r + 3, (uint32_t)c->cur_area);
    }
}

#define BARTA_TIMING 1500
#define GIBARTA_TIMING 2200

static const uint16_t gifoie_timing[6] = { 5000, 6000, 7000, 8000, 9000, 10000 };
static const uint16_t gizonde_timing[6] = { 1000, 900, 700, 700, 700, 700 };
static const uint16_t rafoie_timing[6] = { 1500, 1400, 1300, 1200, 1100, 1100 };
static const uint16_t razonde_timing[6] = { 1200, 1100, 950, 800, 750, 750 };
static const uint16_t rabarta_timing[6] = { 1200, 1100, 950, 800, 750, 750 };

static void handle_bb_objhit_common(ship_client_t* c, lobby_t* l, uint16_t bid) {
    uint32_t obj_type;

    /* What type of object was hit? */
    if ((bid & 0xF000) == 0x4000) {
        /* An object was hit */
        bid &= 0x0FFF;

        /* Make sure the object is in range. */
        if (bid > l->map_objs->count) {
            ERR_LOG("GC %" PRIu32 " 攻击了无效的实例 "
                "(%d -- 地图实例数量: %d)!\n"
                "章节: %d, 层级: %d, 地图: (%d, %d)", c->guildcard,
                bid, l->map_objs->count, l->episode, c->cur_area,
                l->maps[c->cur_area << 1],
                l->maps[(c->cur_area << 1) + 1]);

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
            script_execute(ScriptActionBoxBreak, c, SCRIPT_ARG_PTR, c,
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

int check_aoe_timer(ship_client_t* c, subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t tech_level = 0;

    /* 合理性检查... Does the character have that level of technique? */
    tech_level = c->pl->bb.character.techniques[pkt->technique_number];
    if (tech_level == 0xFF) {
        /* 如果用户在团队中学习一项新技术，则可能会发生这种情况。
        在我们有真正的库存跟踪之前，我们将不得不篡改这一点。
        一旦我们有了真实的、完整的库存跟踪，这种情况可能会断开客户端的连接 */
        tech_level = pkt->level;
    }

    if (c->version >= CLIENT_VERSION_DCV2)
        tech_level += c->pl->bb.inv.iitems[pkt->technique_number].tech;

    if (tech_level < pkt->level) {
        /* 如果用户在团队中学习一项新技术，则可能会发生这种情况。
        在我们有真正的库存跟踪之前，我们将不得不篡改这一点。
        一旦我们有了真实的、完整的库存跟踪，这种情况可能会断开客户端的连接 */
        tech_level = pkt->level;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* See what technique was used... */
    switch (pkt->technique_number) {
        /* These work just like physical hits and can only hit one target, so
           handle them the simple way... */
    case TECHNIQUE_FOIE:
    case TECHNIQUE_ZONDE:
    case TECHNIQUE_GRANTS:
        if (pkt->shdr.size == 3)
            handle_bb_objhit_common(c, l, LE16(pkt->objects[0].obj_id));
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
        SYSTEMTIME aoetime;
        GetLocalTime(&aoetime);
        printf("%03u\n", aoetime.wMilliseconds);
        c->aoe_timer = get_ms_time() + BARTA_TIMING;
        break;

    case TECHNIQUE_GIBARTA:
        c->aoe_timer = get_ms_time() + GIBARTA_TIMING;
        break;

    case TECHNIQUE_GIFOIE:
        c->aoe_timer = get_ms_time() + gifoie_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RAFOIE:
        c->aoe_timer = get_ms_time() + rafoie_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_GIZONDE:
        c->aoe_timer = get_ms_time() + gizonde_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RAZONDE:
        c->aoe_timer = get_ms_time() + razonde_timing[tlindex(tech_level)];
        break;

    case TECHNIQUE_RABARTA:
        c->aoe_timer = get_ms_time() + rabarta_timing[tlindex(tech_level)];
        break;
    }

    return 0;
}

static int handle_bb_switch_changed(ship_client_t* c, subcmd_bb_switch_changed_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅开启了机关!",
            c->guildcard);
        rv = -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->data.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            c->guildcard, pkt->data.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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
        if ((l->flags & LOBBY_TYPE_CHEATS_ENABLED) && c->options.switch_assist &&
            (c->last_switch_enabled_command.type == 0x05)) {
            DBG_LOG("[机关助手] 重复启动上一个命令");
            subcmd_bb_switch_changed_pkt_t* gem = pkt;
            gem->data = c->last_switch_enabled_command;
            rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)gem, 0);
        }
        c->last_switch_enabled_command = pkt->data;
    }

    return rv;
}

static int handle_bb_symbol_chat(ship_client_t* c, subcmd_bb_symbol_chat_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT)
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));

    /* Don't send chats for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU))
        return 0;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 1);
}

static int handle_bb_mhit(ship_client_t* c, subcmd_bb_mhit_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t enemy_id2, enemy_id, dmg;
    game_enemy_t* en;
    uint32_t flags;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了怪物!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的怪物攻击数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Make sure the enemy is in range. */
    enemy_id = LE16(pkt->shdr.enemy_id);
    enemy_id2 = LE16(pkt->enemy_id2);
    dmg = LE16(pkt->damage);
    flags = LE32(pkt->flags);

    /* Swap the flags on the packet if the user is on GC... Looks like Sega
       decided that it should be the opposite order as it is on DC/PC/BB. */
    if (c->version == CLIENT_VERSION_GC)
        flags = SWAP32(flags);

    if (enemy_id2 > l->map_enemies->count) {
        ERR_LOG("GC %" PRIu32 " 攻击了无效的怪物 (%d -- 地图怪物数量: "
            "%d)!", c->guildcard, enemy_id2, l->map_enemies->count);
        return -1;
    }

    /* Save the hit, assuming the enemy isn't already dead. */
    en = &l->map_enemies->enemies[enemy_id2];
    if (!(en->clients_hit & 0x80)) {
        en->clients_hit |= (1 << c->client_id);
        en->last_client = c->client_id;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_objhit_phys(ship_client_t* c, subcmd_bb_objhit_phys_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t pkt_size = pkt->hdr.pkt_len;
    uint8_t size = pkt->shdr.size;
    uint32_t hit_count = pkt->hit_count;
    uint8_t i;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了实例!",
            c->guildcard);
        return -1;
    }

    /* 甚至不要试图在战斗或挑战模式中处理这些问题. */
    if (l->challenge || l->battle)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    if (pkt->hdr.pkt_len == LE16(0x0010)) {
        /* 对空气进行攻击 */
        return 0;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt_size != (sizeof(bb_pkt_hdr_t) + (size << 2)) || size < 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的普通攻击数据! %d %d hit_count %d",
            c->guildcard, pkt_size, (sizeof(bb_pkt_hdr_t) + (size << 2)), hit_count);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Handle each thing that was hit */
    for (i = 0; i < pkt->shdr.size - 2; ++i)
        handle_bb_objhit_common(c, l, LE16(pkt->objects[i].obj_id));

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

/* TODO */
static int handle_bb_objhit_tech(ship_client_t* c, subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅用法术攻击实例!",
            c->guildcard);
        return -1;
    }

    /* 甚至不要试图在战斗或挑战模式中处理这些问题. */
    if (l->challenge || l->battle)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.client_id != c->client_id ||
        pkt->technique_number > TECHNIQUE_MEGID ||
        c->equip_flags & EQUIP_FLAGS_DROID
        ) {
        ERR_LOG("GC %" PRIu32 " 职业 %s 发送损坏的 %s 法术攻击数据!",
            c->guildcard, pso_class[c->pl->bb.character.dress_data.ch_class].cn_name,
            max_tech_level[pkt->technique_number].tech_name);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    //display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    //printf("ch_class %d techniques %d = %d max_lvl %d\n", c->pl->bb.character.dress_data.ch_class,
    //    c->pl->bb.character.techniques[pkt->technique_number],
    //    c->bb_pl->character.techniques[pkt->technique_number], 
    //    max_tech_level[pkt->technique_number].max_lvl[c->pl->bb.character.dress_data.ch_class]);

    //if (c->version <= CLIENT_VERSION_BB) {
    check_aoe_timer(c, pkt);
    //}

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_objhit(ship_client_t* c, subcmd_bb_bhit_pkt_t* pkt) {
    uint64_t now = get_ms_time();
    //SYSTEMTIME aoetime;
    //GetLocalTime(&aoetime);
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅攻击了箱子!",
            c->guildcard);
        return -1;
    }

    /* 甚至不要试图在战斗或挑战模式中处理这些问题. */
    if (l->challenge || l->battle)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    //printf("wMilliseconds = %d\n", aoetime.wMilliseconds);

    //printf("aoe_timer = %d now = %d\n", (uint32_t)c->aoe_timer, (uint32_t)now);

    /* We only care about these if the AoE timer is set on the sender. */
    if (c->aoe_timer < now)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Check the type of the object that was hit. As the AoE timer can't be set
       if the objects aren't loaded, this shouldn't ever trip up... */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Handle the object marked as hit, if appropriate. */
    handle_bb_objhit_common(c, l, LE16(pkt->shdr.object_id));

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_condition(ship_client_t* c, subcmd_bb_add_or_remove_condition_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            c->guildcard, pkt->hdr.pkt_type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_dragon_act(ship_client_t* c, subcmd_bb_dragon_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int v = c->version, i;
    subcmd_bb_dragon_act_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " reported Dragon action in "
            "lobby!\n", c->guildcard);
        return -1;
    }

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_bb_dragon_act_t));
    tr.x.b = SWAP32(tr.x.b);
    tr.z.b = SWAP32(tr.z.b);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
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

static int handle_bb_set_area_1F(ship_client_t* c, subcmd_bb_set_area_1F_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if (c->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            c->cur_area, SCRIPT_ARG_END);

        // 测试DC代码
        /* Clear the list of dropped items. */
        if (c->cur_area == 0) {
            memset(c->p2_drops, 0, sizeof(c->p2_drops));
            c->p2_drops_max = 0;
        }

        c->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_set_area_20(ship_client_t* c, subcmd_bb_set_area_20_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if (c->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            c->cur_area, SCRIPT_ARG_END);

        // 测试DC代码
        /* Clear the list of dropped items. */
        if (c->cur_area == 0) {
            memset(c->p2_drops, 0, sizeof(c->p2_drops));
            c->p2_drops_max = 0;
        }

        c->cur_area = pkt->area;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        //更换场景 客户端之间更新位置
        //DBG_LOG("客户端区域 %d （x:%f y:%f z:%f)", c->cur_area, c->x, c->y, c->z);
        //客户端区域 0 （x:228.995331 y:0.000000 z:253.998901)

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_inter_level_warp(ship_client_t* c, subcmd_bb_inter_level_warp_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        return -1;
    }

    /* Save the new area and move along */
    if (c->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, c, SCRIPT_ARG_PTR, c,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            c->cur_area, SCRIPT_ARG_END);
        c->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_load_22(ship_client_t* c, subcmd_bb_set_player_visibility_6x22_6x23_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int i, rv;

    if (pkt->shdr.client_id != c->client_id) {
        DBG_LOG("SUBCMD60_LOAD_22 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    if (l->type == LOBBY_TYPE_LOBBY) {
        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != c &&
                subcmd_send_pos(c, l->clients[i])) {
                rv = -1;
                break;
            }
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_finish_load(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int i, rv;

    if (l->type != LOBBY_TYPE_GAME) {
        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != c &&
                subcmd_send_pos(c, l->clients[i])) {
                rv = -1;
                break;
            }
        }

        /* 将房间中的玩家公会数据发送至新进入的客户端 */
        send_bb_guild_cmd(c, BB_GUILD_FULL_DATA);
        send_bb_guild_cmd(c, BB_GUILD_INITIALIZATION_DATA);
    }

    rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    return rv;
}

static int handle_bb_set_pos_0x24(ship_client_t* c, subcmd_bb_set_pos_0x24_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x001C) || pkt->shdr.size != 0x05) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
#ifdef DEBUG
        DBG_LOG("GC %u %d %d (0x%X 0x%X) X轴:%f Y轴:%f Z轴:%f", c->guildcard,
            c->client_id, pkt->shdr.client_id, pkt->unk1, pkt->unused, pkt->x, pkt->y, pkt->z);
#endif // DEBUG
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_equip(ship_client_t* c, subcmd_bb_equip_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t /*inv, */item_id = pkt->item_id, found_item = 0/*, found_slot = 0, j = 0, slot[4] = { 0 }*/;
    int i = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅更换装备!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误装备数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    found_item = item_check_equip_flags(c, item_id);

    /* 是否存在物品背包中? */
    if (!found_item) {
        ERR_LOG("GC %" PRIu32 " 装备了未存在的物品数据!",
            c->guildcard);
        return -1;
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_unequip(ship_client_t* c, subcmd_bb_equip_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t inv, i, isframe = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅卸除装备!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误卸除装备数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Find the item and remove the equip flag. */
    inv = c->bb_pl->inv.item_count;

    i = find_iitem_slot(&c->bb_pl->inv, pkt->item_id);

    if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
        c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);

        /* If its a frame, we have to make sure to unequip any units that
           may be equipped as well. */
        if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
            c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_FRAME) {
            isframe = 1;
        }
    }

    /* Did we find something to equip? */
    if (i >= inv) {
        ERR_LOG("GC %" PRIu32 " 卸除了未存在的物品数据!",
            c->guildcard);
        return -1;
    }

    /* Clear any units if we unequipped a frame. */
    if (isframe) {
        for (i = 0; i < inv; ++i) {
            if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_UNIT) {
                c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_use_item(ship_client_t* c, subcmd_bb_use_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int num;

    /* We can't get these in default lobbies without someone messing with
       something that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅使用物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x02)
        return -1;

    if (!(c->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* 从玩家的背包中移除该物品. */
    if ((num = item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
        pkt->item_id, 1)) < 1) {
        ERR_LOG("无法从玩家背包中移除物品!");
        return -1;
    }

    c->item_count -= num;
    c->bb_pl->inv.item_count -= num;
    c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;


send_pkt:
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_feed_mag(ship_client_t* c, subcmd_bb_feed_mag_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t item_id = pkt->item_id, mag_id = pkt->mag_id;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发游戏指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送错误数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    DBG_LOG("GC %" PRIu32 " 使用物品ID 0x%04X 喂养玛古 ID 0x%04X!",
        c->guildcard, item_id, mag_id);

    if (mag_bb_feed(c, item_id, mag_id)) {
        ERR_LOG("GC %" PRIu32 " 喂养玛古发生错误!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_destroy_item(ship_client_t* c, subcmd_bb_destroy_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1;
    uint32_t i, tmp, tmp2;
    iitem_t item_data;
    iitem_t* it;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中掉落物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的物品掉落数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* TODO 完成挑战模式的物品掉落 */
    if (l->challenge || l->battle) {
        /* 数据包完成, 发送至游戏房间. */
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    if (pkt->item_id != 0xFFFFFFFF) {
        /* 查找用户库存中的物品. */
        for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
            if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
                found = i;
                break;
            }
        }

        /* 如果找不到该物品，则将用户从船上推下. */
        if (found == -1) {
            ERR_LOG("GC %" PRIu32 " 掉落无效的堆叠物品!", c->guildcard);
            return -1;
        }

        /* 从客户端结构的库存中获取物品并设置拆分 */
        item_data = c->iitems[found];
        item_data.data.item_id = LE32((++l->item_player_id[c->client_id]));
        item_data.data.data_b[5] = (uint8_t)(LE32(pkt->amount));
    }
    else {
        item_data.data.data_l[0] = LE32(Item_Meseta);
        item_data.data.data_l[1] = item_data.data.data_l[2] = 0;
        item_data.data.item_id = LE32((++l->item_player_id[c->client_id]));
        item_data.data.data2_l = pkt->amount;
    }

    /* Make sure the item id and amount match the most recent 0xC3. */
    if (pkt->item_id != c->drop_item_id || pkt->amount != c->drop_amt) {
        ERR_LOG("GC %" PRIu32 " 掉了不同的堆叠物品!",
            c->guildcard);
        ERR_LOG("pkt->item_id %d c->drop_item %d pkt->amount %d c->drop_amt %d",
            pkt->item_id, c->drop_item_id, pkt->amount, c->drop_amt);
        return -1;
    }

    /* We have the item... Add it to the lobby's inventory. */
    if (!(it = lobby_add_item_locked(l, &item_data))) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("无法将物品添加至游戏房间!");
        return -1;
    }

    if (pkt->item_id != 0xFFFFFFFF) {
        /* 从玩家的背包中移除该物品. */
        found = item_remove_from_inv(c->bb_pl->inv.iitems,
            c->bb_pl->inv.item_count, pkt->item_id,
            LE32(pkt->amount));
        if (found < 0) {
            ERR_LOG("无法从玩家背包中移除物品!");
            return -1;
        }

        c->bb_pl->inv.item_count -= found;
    }
    else {
        /* Remove the meseta from the character data */
        tmp = LE32(pkt->amount);
        tmp2 = LE32(c->bb_pl->character.disp.meseta);

        if (tmp > tmp2) {
            ERR_LOG("GC %" PRIu32 " 掉落的美赛塔超出所拥有的",
                c->guildcard);
            return -1;
        }

        c->bb_pl->character.disp.meseta = LE32(tmp2 - tmp);
        c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
    }

    /* 现在我们有两个数据包要发送.首先,发送一个数据包,告诉每个人有一个物品掉落.
    然后,发送一个从客户端的库存中删除物品的人.第一个必须发给每个人,
    第二个必须发给除了最初发送这个包裹的人以外的所有人. */
    subcmd_send_bb_lobby_drop_stack(c, c->drop_area, c->drop_x, c->drop_z, it);

    /* 数据包完成, 发送至游戏房间. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_drop_item(ship_client_t* c, subcmd_bb_drop_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1, isframe = 0;
    uint32_t i, inv;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅中掉落物品!",
            c->guildcard);
        return -1;
    }

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if ((c->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("GC %" PRIu32 " 尝试在商店中掉落物品!",
            c->guildcard);
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的物品掉落数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* TODO 完成挑战模式的物品掉落 */
    if (l->challenge || l->battle) {
        /* 数据包完成, 发送至游戏房间. */
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Look for the item in the user's inventory. */
    inv = c->bb_pl->inv.item_count;

    for (i = 0; i < inv; ++i) {
        if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
            found = i;

            /* If it is an equipped frame, we need to unequip all the units
               that are attached to it.
               如果它是一个装备好的护甲，我们需要取消与它相连的所有装备的插件 */
            if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_FRAME &&
                (c->bb_pl->inv.iitems[i].flags & LE32(0x00000008))) {
                isframe = 1;
            }

            break;
        }
    }

    /* If the item isn't found, then punt the user from the ship. */
    if (found == -1) {
        ERR_LOG("GC %" PRIu32 " 掉落了无效的物品 ID!",
            c->guildcard);
        return -1;
    }

    /* Clear the equipped flag.
    清理已装备的标签*/
    c->bb_pl->inv.iitems[found].flags &= LE32(0xFFFFFFF7);

    /* Unequip any units, if the item was equipped and a frame.
    卸掉所有已插入在这件装备的插件*/
    if (isframe) {
        for (i = 0; i < inv; ++i) {
            if (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_GUARD &&
                c->bb_pl->inv.iitems[i].data.data_b[1] == ITEM_SUBTYPE_UNIT) {
                c->bb_pl->inv.iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* We have the item... Add it to the lobby's inventory.
    我们有这个物品…把它添加到大厅的背包中*/
    if (!lobby_add_item_locked(l, &c->bb_pl->inv.iitems[found])) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("无法将物品新增游戏房间背包!");
        return -1;
    }

    /* 从玩家的背包中移除该物品. */
    if (item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
        pkt->item_id, EMPTY_STRING) < 1) {
        ERR_LOG("无法从玩家背包中移除物品!");
        return -1;
    }

    --c->bb_pl->inv.item_count;

    /* 数据包完成, 发送至游戏房间. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_talk_npc(ship_client_t* c, subcmd_bb_talk_npc_t* pkt) {
    lobby_t* l = c->cur_lobby;

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
            c->guildcard, pkt->shdr.type);
        display_packet((uint8_t*)pkt, pkt->hdr.pkt_len);
        return -1;
    }

    /* Clear the list of dropped items. */
    if (pkt->unk == 0xFFFF && c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_done_talk_npc(ship_client_t* c, subcmd_bb_end_talk_to_npc_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x01) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        display_packet((uint8_t*)pkt, pkt->hdr.pkt_len);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_hit_by_enemy(ship_client_t* c, subcmd_bb_hit_by_enemy_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t type = LE16(pkt->hdr.pkt_type);

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发游戏房间指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* TODO 测试GM DBUG 无限血量 */
    if (c->game_data->gm_debug)
        send_lobby_mod_stat(l, c, SUBCMD60_STAT_HPUP, 2040);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_3a(ship_client_t* c, subcmd_bb_cmd_3a_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    display_packet(pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_load_3b(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            c->guildcard);
        return -1;
    }

    subcmd_send_bb_set_exp_rate(c, 3000);

    c->need_save_data = 1;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_set_pos(ship_client_t* c, subcmd_bb_set_pos_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->w = pkt->w;
        c->x = pkt->x;
        c->y = pkt->y;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_move(ship_client_t* c, subcmd_bb_move_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* Save the new position and move along */
    if (c->client_id == pkt->shdr.client_id) {
        c->x = pkt->x;
        c->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(c, l);

        if (c->game_data->death) {
            c->game_data->death = 0;
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_normal_attack(ship_client_t* c, subcmd_bb_natk_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发普通攻击指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Save the new area and move along */
    if (c->client_id == pkt->shdr.client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(c, l);
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_used_tech(ship_client_t* c, subcmd_bb_used_tech_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 释放了违规的法术!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(c->flags & CLIENT_FLAG_INFINITE_TP)) {
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD60_STAT_TPUP, 255);
    //return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_defense_damage(ship_client_t* c, subcmd_bb_defense_damage_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* This aught to do it... */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_take_damage(ship_client_t* c, subcmd_bb_take_damage_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY || pkt->shdr.client_id != c->client_id) {
        return -1;
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(c->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, c, SUBCMD60_STAT_HPUP, 2000);
}

static int handle_bb_death_sync(ship_client_t* c, subcmd_bb_death_sync_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int i;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    c->game_data->death = 1;

    for (i = 0; i < c->bb_pl->inv.item_count; i++) {
        if ((c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_MAG) &&
            (c->bb_pl->inv.iitems[i].flags & LE32(0x00000008))) {
            if (c->bb_pl->inv.iitems[i].data.data2_b[0] >= 5)
                c->bb_pl->inv.iitems[i].data.data2_b[0] -= 5;
            else
                c->bb_pl->inv.iitems[i].data.data2_b[0] = 0;
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_4e(ship_client_t* c, subcmd_bb_cmd_4e_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令! 0x%02X",
            c->guildcard, pkt->shdr.type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_player_saved(ship_client_t* c, subcmd_bb_player_saved_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->shdr.client_id != c->client_id) {
        DBG_LOG("错误 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_switch_req(ship_client_t* c, subcmd_bb_switch_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了开关!",
            c->guildcard);
        return -1;
    }
    //(00000000)   10 00 60 00 00 00 00 00  50 02 00 00 A3 A6 00 00    ..`.....P.......
    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_menu_req(ship_client_t* c, subcmd_bb_menu_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We don't care about these in lobbies. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        //ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Clear the list of dropped items. */
    if (c->cur_area == 0) {
        memset(c->p2_drops, 0, sizeof(c->p2_drops));
        c->p2_drops_max = 0;
    }

    /* Flip the shopping flag, since this packet is sent both for talking to the
       shop in the first place and when the client exits the shop
       翻转购物标志，因为该数据包在第一时间和客户离开商店时都会发送给商店. */
    c->flags ^= CLIENT_FLAG_SHOPPING;
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_map_warp_55(ship_client_t* c, subcmd_bb_map_warp_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0028) || pkt->shdr.size != 0x08 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    if (c->client_id == pkt->shdr.client_id) {

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

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_lobby_act(ship_client_t* c, subcmd_bb_lobby_act_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    DBG_LOG("动作ID %d", pkt->act_id);

    // pkt->act_id 大厅动作的对应ID

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_level_up_req(ship_client_t* c, subcmd_bb_levelup_req_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014)) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    //ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_destroy_ground_item(ship_client_t* c, subcmd_bb_destory_ground_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅房间指令!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_create_pipe(ship_client_t* c, subcmd_bb_pipe_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅使用传送门!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0024) || pkt->shdr.size != 0x07) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据指令 0x%02X!",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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
        if (pkt->area != c->cur_area) {
            ERR_LOG("GC %" PRIu32 " 尝试从传送门传送至不存在的区域 (玩家层级: %d, 传送层级: %d).", c->guildcard,
                c->cur_area, (int)pkt->area);
            return -1;
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_spawn_npc(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅生成了NPC!",
            c->guildcard);
        return -1;
    }

    /* The only quests that allow NPCs to be loaded are those that require there
       to only be one player, so limit that here. Also, we only allow /npc in
       single-player teams, so that'll fall into line too. */
    if (l->num_clients > 1) {
        ERR_LOG("GC %" PRIu32 " to spawn NPC in multi-"
            "player team!", c->guildcard);
        return -1;
    }

    /* Either this is a legitimate request to spawn a quest NPC, or the player
       is doing something stupid like trying to NOL himself. We don't care if
       someone is stupid enough to NOL themselves, so send the packet on now. */
    return subcmd_send_lobby_bb(l, c, pkt, 0);
}

static int handle_bb_subcmd_6a(ship_client_t* c, subcmd_bb_Unknown_6x6A_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_word_select(ship_client_t* c, subcmd_bb_word_select_t* pkt) {
    subcmd_word_select_t gc = { 0 };

    /* Don't send the message if they have the protection flag on. */
    if (c->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(c, __(c, "\tE\tC7您必须登录后才可以进行操作."));
    }

    /* Don't send chats for a STFUed client. */
    if ((c->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    memcpy(&gc, pkt, sizeof(subcmd_word_select_t) - sizeof(dc_pkt_hdr_t));
    gc.client_id_gc = (uint8_t)pkt->shdr.client_id;
    gc.hdr.pkt_type = (uint8_t)(LE16(pkt->hdr.pkt_type));
    gc.hdr.flags = (uint8_t)pkt->hdr.flags;
    gc.hdr.pkt_len = LE16((LE16(pkt->hdr.pkt_len) - 4));

    return word_select_send_gc(c, &gc);
}

static int handle_bb_set_flag(ship_client_t* c, subcmd_bb_set_flag_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t flag = pkt->flag;
    uint16_t checked = pkt->checked;
    uint16_t difficulty = pkt->difficulty;
    int rv = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (!l || l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发SET_FLAG指令!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    DBG_LOG("GC %" PRIu32 " 触发SET_FLAG指令! flag = 0x%02X checked = 0x%02X episode = 0x%02X difficulty = 0x%02X",
        c->guildcard, flag, checked, l->episode, difficulty);

    if (!checked) {
        if (flag < 0x400)
            c->bb_pl->quest_data1.data[((uint32_t)l->difficulty * 0x80) + (flag >> 3)] |= 1 << (7 - (flag & 0x07));
    }

    bool should_send_boss_drop_req = false;
    if (difficulty == l->difficulty) {
        if ((l->episode == GAME_TYPE_EPISODE_1) && (c->cur_area == 0x0E)) {
            // 在正常情况下，黑暗佛没有第三阶段，所以在第二阶段结束后发送掉落请求
            // 其他困难模式则在第三阶段结束后发送
            if (((l->difficulty == 0) && (flag == 0x00000035)) ||
                ((l->difficulty != 0) && (flag == 0x00000037))) {
                should_send_boss_drop_req = true;
            }
        }
        else if ((l->episode == GAME_TYPE_EPISODE_2) && (flag == 0x00000057) && (c->cur_area == 0x0D)) {
            should_send_boss_drop_req = true;
        }
    }

    if (should_send_boss_drop_req) {
        ship_client_t* s = l->clients[l->leader_id];
        if (s) {
            subcmd_bb_itemreq_t bd = { 0 };

            /* 填充数据头 */
            bd.hdr.pkt_len = LE16(sizeof(subcmd_bb_itemreq_t));
            bd.hdr.pkt_type = GAME_COMMAND2_TYPE;
            bd.hdr.flags = 0x00000000;
            bd.shdr.type = SUBCMD62_ITEMREQ;
            bd.shdr.size = 0x06;
            bd.shdr.unused = 0x0000;

            /* 填充数据 */
            bd.area = (uint8_t)c->cur_area;
            bd.pt_index = (uint8_t)((l->episode == GAME_TYPE_EPISODE_2) ? 0x4E : 0x2F);
            bd.request_id = LE16(0x0B4F);
            bd.x = (l->episode == 2) ? -9999.0f : 10160.58984375f;
            bd.z = 0;
            bd.unk1 = LE16(0x0002);
            bd.unk2 = LE16(0x0000);
            bd.unk3 = LE32(0xE0AEDC01);

            return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
        }
    }

    rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    return rv;
}

static int handle_bb_killed_monster(ship_client_t* c, subcmd_bb_killed_monster_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中杀掉了怪物!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的杀怪数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static inline int reg_sync_index_bb(lobby_t* l, uint16_t regnum) {
    int i;

    if (!(l->q_flags & LOBBY_QFLAG_SYNC_REGS))
        return -1;

    for (i = 0; i < l->num_syncregs; ++i) {
        if (regnum == l->syncregs[i])
            return i;
    }

    return -1;
}

static int handle_bb_sync_reg(ship_client_t* c, subcmd_bb_sync_reg_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t val = LE32(pkt->value);
    int done = 0, idx;
    uint32_t ctl;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了游戏指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的同步数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* TODO: Probably should do some checking here... */
    /* Run the register sync script, if one is set. If the script returns
       non-zero, then assume that it has adequately handled the sync. */
    if ((script_execute(ScriptActionQuestSyncRegister, c, SCRIPT_ARG_PTR, c,
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
            send_sync_register(c, pkt->register_number, 0x8000FFFE);
        }
        /* Make sure we don't have anything with any reserved ctl bits set
           (unless a script has already handled the sync). */
        else if ((val & 0x17000000)) {
            DBG_LOG("Quest set flag register with reserved ctl!\n");
            send_sync_register(c, pkt->register_number, 0x8000FFFE);
        }
        else if ((val & 0x08000000)) {
            /* Delete the flag... */
            shipgate_send_qflag(&ship->sg, c, 1, ((val >> 16) & 0xFF),
                c->cur_lobby->qid, 0, QFLAG_DELETE_FLAG);
        }
        else {
            /* Send the request to the shipgate... */
            shipgate_send_qflag(&ship->sg, c, ctl & 0x01, (val >> 16) & 0xFF,
                c->cur_lobby->qid, val & 0xFFFF, 0);
        }

        done = 1;
    }

    /* Does this quest use server data calls? If so, deal with it... */
    if ((l->q_flags & LOBBY_QFLAG_DATA) && !done) {
        if (pkt->register_number == l->q_data_reg) {
            if (c->q_stack_top < CLIENT_MAX_QSTACK) {
                if (!(c->flags & CLIENT_FLAG_QSTACK_LOCK)) {
                    c->q_stack[c->q_stack_top++] = val;

                    /* Check if we've got everything we expected... */
                    if (c->q_stack_top >= 3 &&
                        c->q_stack_top == 3 + c->q_stack[1] + c->q_stack[2]) {
                        /* Call the function requested and reset the stack. */
                        ctl = quest_function_dispatch(c, l);

                        if (ctl != QUEST_FUNC_RET_NOT_YET) {
                            send_sync_register(c, pkt->register_number, ctl);
                            c->q_stack_top = 0;
                        }
                    }
                }
                else {
                    /* The stack is locked, ignore the write and report the
                       error. */
                    send_sync_register(c, pkt->register_number,
                        QUEST_FUNC_RET_STACK_LOCKED);
                }
            }
            else if (c->q_stack_top == CLIENT_MAX_QSTACK) {
                /* Eat the stack push and report an error. */
                send_sync_register(c, pkt->register_number,
                    QUEST_FUNC_RET_STACK_OVERFLOW);
            }

            done = 1;
        }
        else if (pkt->register_number == l->q_ctl_reg) {
            /* For now, the only reason we'll have one of these is to reset the
               stack. There might be other reasons later, but this will do, for
               the time being... */
            c->q_stack_top = 0;
            done = 1;
        }
    }

    /* Does this register have to be synced? */
    if ((idx = reg_sync_index_bb(l, pkt->register_number)) != -1) {
        l->regvals[idx] = val;
    }

    if (!done)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    return 0;
}

static int handle_bb_gogo_ball(ship_client_t* c, subcmd_bb_gogo_ball_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    // pkt->act_id 大厅动作的对应ID

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_battle_mode(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int ch, ch2;
    ship_client_t* lc = { 0 };

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            c->guildcard);
        return -1;
    }

    if ((l->battle) && (c->mode)) {
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
                            if (lc->pl->bb.inv.iitems[ch2].data.data_b[0] == 0x02)
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

static int handle_bb_challenge_mode_grave(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    int i;
    lobby_t* l = c->cur_lobby;
    subcmd_pc_grave_t pc = { { 0 } };
    subcmd_dc_grave_t dc = { { 0 } };
    subcmd_bb_grave_t bb = { { 0 } };
    size_t in, out;
    char* inptr;
    char* outptr;

    /* Deal with converting the different versions... */
    switch (c->version) {
    case CLIENT_VERSION_DCV2:
        memcpy(&dc, pkt, sizeof(subcmd_dc_grave_t));

        /* Make a copy to send to PC players... */
        pc.hdr.pkt_type = GAME_COMMAND0_TYPE;
        pc.hdr.pkt_len = LE16(0x00E4);

        pc.shdr.type = SUBCMD60_CMODE_GRAVE;
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

        dc.shdr.type = SUBCMD60_CMODE_GRAVE;
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
        display_packet((unsigned char*)&bb, LE16(pkt->hdr.pkt_len));
        break;

    default:
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Send the packet to everyone in the lobby */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
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

static int handle_bb_game_mode(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            c->guildcard);
        return -1;
    }

    if ((l->battle) && (c->mode))
        handle_bb_battle_mode(c, pkt);
    else 
        handle_bb_challenge_mode_grave(c, pkt);

    return 0;
}

static int handle_bb_arrow_change(ship_client_t* c, subcmd_bb_arrow_change_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->shdr.size != 0x01 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    c->arrow_color = pkt->shdr.client_id;
    return send_lobby_arrows(l);
}

static int handle_bb_player_died(ship_client_t* c, subcmd_bb_player_died_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间指令! 0x%02X",
            c->guildcard, pkt->shdr.type);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了损坏的死亡数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_Unknown_6x8A(ship_client_t* c, subcmd_bb_Unknown_6x8A_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        //return -1;
    }

    UDONE_CSPD(pkt->hdr.pkt_type, c->version, pkt);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_set_technique_level_override(ship_client_t* c, subcmd_bb_set_technique_level_override_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中释放了法术!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 释放了违规的法术!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        //return -1;
    }

    /*uint8_t tmp_level = pkt->level_upgrade;

    pkt->level_upgrade = tmp_level+100;*/

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_timed_switch_activated(ship_client_t* c, subcmd_bb_timed_switch_activated_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发了房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " 发送了错误的数据包!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        //return -1;
    }

    display_packet(pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_save_player_act(ship_client_t* c, subcmd_bb_save_player_act_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (pkt->shdr.client_id != c->client_id) {
        DBG_LOG("错误 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_lobby_chair(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅房间指令!",
            c->guildcard);
        return -1;
    }

    switch (type)
    {
        /* 0xAB */
    case SUBCMD60_CHAIR_CREATE:
        subcmd_bb_create_lobby_chair_t* pktab = (subcmd_bb_create_lobby_chair_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_CREATE %u %u ", pktab->unknown_a1, pktab->unknown_a2);

        break;

        /* 0xAE */
    case SUBCMD60_CHAIR_STATE:
        subcmd_bb_set_lobby_chair_state_t* pktae = (subcmd_bb_set_lobby_chair_state_t*)pkt;
        //DBG_LOG("SUBCMD60_CHAIR_STATE %u %u %u %u", pktae->unknown_a1, pktae->unknown_a2, pktae->unknown_a3, pktae->unknown_a4);

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

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_sell_item(ship_client_t* c, subcmd_bb_sell_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t i;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅触发了游戏房间的指令!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    for (i = 0; i < c->bb_pl->inv.item_count; i++) {
        /* Look for the item in question. */
        if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {

            if ((pkt->sell_num > 1) && (c->bb_pl->inv.iitems[i].data.data_b[0] != 0x03)) {
                DBG_LOG("handle_bb_sell_item %d 0x%02X", pkt->sell_num, c->bb_pl->inv.iitems[i].data.data_b[0]);
                return -1;
            }
            else
            {
                uint32_t shop_price = get_bb_shop_price(&c->bb_pl->inv.iitems[i]) * pkt->sell_num;

                /* 从玩家的背包中移除该物品. */
                if (item_remove_from_inv(c->bb_pl->inv.iitems, c->bb_pl->inv.item_count,
                    pkt->item_id, 0xFFFFFFFF) < 1) {
                    ERR_LOG("无法从玩家GC %u 背包中移除ID %u 物品!", c->guildcard, pkt->item_id);
                    return -1;
                }

                --c->bb_pl->inv.item_count;
                c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

                c->bb_pl->character.disp.meseta += shop_price;

                if (c->bb_pl->character.disp.meseta > 999999)
                    c->bb_pl->character.disp.meseta = 999999;

                c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;
            }

            return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
        }
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_drop_split_stacked_item(ship_client_t* c, subcmd_bb_drop_split_stacked_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int found = -1;
    uint32_t i, meseta, amt;
    iitem_t iitem = { 0 };

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅丢弃了物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06 ||
        pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Look for the item in the user's inventory. */
    if (pkt->item_id != 0xFFFFFFFF) {
        for (i = 0; i < c->bb_pl->inv.item_count; ++i) {
            if (c->bb_pl->inv.iitems[i].data.item_id == pkt->item_id) {
                found = i;
                break;
            }
        }

        /* If the item isn't found, then punt the user from the ship. */
        if (found == -1) {
            ERR_LOG("GC %" PRIu32 " 掉落的物品ID无效!",
                c->guildcard);
            return -1;
        }
    }
    else {
        meseta = LE32(c->bb_pl->character.disp.meseta);
        amt = LE32(pkt->amount);

        if (meseta < amt) {
            ERR_LOG("GC %" PRIu32 " 掉落太多美赛塔!\n",
                c->guildcard);
            return -1;
        }
    }

    /* We have the item... Record the information for use with the subcommand
    0x29 that should follow. */
    c->drop_x = pkt->x;
    c->drop_z = pkt->z;
    c->drop_area = pkt->area;
    c->drop_item_id = pkt->item_id;
    c->drop_amt = pkt->amount;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_sort_inv(ship_client_t* c, subcmd_bb_sort_inv_t* pkt) {
    lobby_t* l = c->cur_lobby;
    inventory_t sorted = { 0 };
    size_t x = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅整理了物品!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0084) || pkt->shdr.size != 0x1F) {
        ERR_LOG("GC %" PRIu32 " 发送错误的整理物品数据包!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    memset(&sorted, 0, sizeof(inventory_t));

    for (x = 0; x < MAX_PLAYER_INV_ITEMS; x++) {
        if (pkt->item_ids[x] == 0xFFFFFFFF) {
            sorted.iitems[x].data.data_b[1] = 0xFF;
            sorted.iitems[x].data.item_id = 0xFFFFFFFF;
        }
        else {
            size_t index = find_iitem_slot(&c->bb_pl->inv, pkt->item_ids[x]);
            sorted.iitems[x] = c->bb_pl->inv.iitems[index];
        }
    }

    sorted.item_count = c->bb_pl->inv.item_count;
    sorted.hpmats_used = c->bb_pl->inv.hpmats_used;
    sorted.tpmats_used = c->bb_pl->inv.tpmats_used;
    sorted.language = c->bb_pl->inv.language;

    c->bb_pl->inv = sorted;
    c->pl->bb.inv = sorted;

    /* Nobody else really needs to care about this one... */
    return 0;
}

static int handle_bb_medic(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " used medical center in lobby!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x000C) || pkt->size != 0x01 ||
        pkt->data[0] != c->client_id) {
        ERR_LOG("GC %" PRIu32 " sent bad medic message!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Subtract 10 meseta from the client. */
    c->bb_pl->character.disp.meseta -= 10;
    c->pl->bb.character.disp.meseta = c->bb_pl->character.disp.meseta;

    /* Send it along to the rest of the lobby. */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_steal_exp(ship_client_t* c, subcmd_bb_steal_exp_t* pkt) {
    lobby_t* l = c->cur_lobby;
    pmt_weapon_bb_t tmp_wp = { 0 };
    game_enemy_t* en;
    subcmd_bb_steal_exp_t* pk = (subcmd_bb_steal_exp_t*)pkt;
    uint32_t exp_percent = 0;
    uint32_t exp_to_add;
    uint8_t special = 0;
    uint16_t mid;
    uint32_t bp, exp_amount;
    int i;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 尝试在大厅触发游戏指令!",
            c->guildcard);
        return -1;
    }

    mid = LE16(pk->shdr.enemy_id);
    mid &= 0xFFF;

    if (mid < 0xB50) {
        for (i = 0; i < c->bb_pl->inv.item_count; i++) {
            if ((c->bb_pl->inv.iitems[i].flags & LE32(0x00000008)) &&
                (c->bb_pl->inv.iitems[i].data.data_b[0] == ITEM_TYPE_WEAPON)) {
                if ((c->bb_pl->inv.iitems[i].data.data_b[1] < 0x0A) &&
                    (c->bb_pl->inv.iitems[i].data.data_b[2] < 0x05)) {
                    special = (c->bb_pl->inv.iitems[i].data.data_b[4] & 0x1F);
                }
                else {
                    if ((c->bb_pl->inv.iitems[i].data.data_b[1] < 0x0D) &&
                        (c->bb_pl->inv.iitems[i].data.data_b[2] < 0x04))
                        special = (c->bb_pl->inv.iitems[i].data.data_b[4] & 0x1F);
                    else {
                        if (pmt_lookup_weapon_bb(c->bb_pl->inv.iitems[i].data.data_l[0], &tmp_wp)) {
                            ERR_LOG("GC %" PRIu32 " 装备了不存在的物品数据!",
                                c->guildcard);
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
                        (c->equip_flags & EQUIP_FLAGS_DROID))
                        exp_percent += 30;
                    break;
                }

                break;
            }
        }

        if (exp_percent) {
            /* Make sure the enemy is in range. */
            mid = LE16(pk->shdr.enemy_id);
            //DBG_LOG("怪物编号原值 %02X %02X", mid, pkt->enemy_id2);
            mid &= 0xFFF;
            //DBG_LOG("怪物编号新值 %02X map_enemies->count %02X", mid, l->map_enemies->count);

            if (mid > l->map_enemies->count) {
                ERR_LOG("GC %" PRIu32 " 杀掉了无效的敌人 (%d -- "
                    "敌人数量: %d)!", c->guildcard, mid, l->map_enemies->count);
                return -1;
            }

            /* Make sure this client actually hit the enemy and that the client didn't
               already claim their experience. */
            en = &l->map_enemies->enemies[mid];

            if (!(en->clients_hit & (1 << c->client_id))) {
                return 0;
            }

            /* Set that the client already got their experience and that the monster is
               indeed dead. */
            en->clients_hit = (en->clients_hit & (~(1 << c->client_id))) | 0x80;

            /* Give the client their experience! */
            bp = en->bp_entry;

            //新增经验倍率提升参数 exp_mult expboost 11.18
            exp_amount = (l->bb_params[bp].exp * exp_percent) / 100L;

            if (exp_amount > 80)  // Limit the amount of exp stolen to 80
                exp_amount = 80;

            if ((c->game_data->expboost) && (l->exp_mult > 0)) {
                exp_to_add = exp_amount;
                exp_amount = exp_to_add + (l->bb_params[bp].exp * l->exp_mult);
            }

            if (!exp_amount) {
                ERR_LOG("未获取到经验新值 %d bp %d 倍率 %d", exp_amount, bp, l->exp_mult);
                return client_give_exp(c, 1);
            }

            return client_give_exp(c, exp_amount);
        }
    }
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

/* 从怪物获取的经验倍率未进行调整 */
static int handle_bb_req_exp(ship_client_t* c, subcmd_bb_req_exp_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint16_t mid;
    uint32_t bp, exp_amount;
    game_enemy_t* en;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中获取经验!",
            c->guildcard);
        return -1;
    }

    /* 合理性检查... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的经验获取数据包!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Make sure the enemy is in range. */
    mid = LE16(pkt->enemy_id2);
    //DBG_LOG("怪物编号原值 %02X %02X", mid, pkt->enemy_id2);
    mid &= 0xFFF;
    //DBG_LOG("怪物编号新值 %02X map_enemies->count %02X", mid, l->map_enemies->count);

    if (mid > l->map_enemies->count) {
        ERR_LOG("GC %" PRIu32 " 杀掉了无效的敌人 (%d -- "
            "敌人数量: %d)!", c->guildcard, mid, l->map_enemies->count);
        return -1;
    }

    /* Make sure this client actually hit the enemy and that the client didn't
       already claim their experience. */
    en = &l->map_enemies->enemies[mid];

    if (!(en->clients_hit & (1 << c->client_id))) {
        return 0;
    }

    /* Set that the client already got their experience and that the monster is
       indeed dead. */
    en->clients_hit = (en->clients_hit & (~(1 << c->client_id))) | 0x80;

    /* Give the client their experience! */
    bp = en->bp_entry;
    exp_amount = l->bb_params[bp].exp + 100000;

    //DBG_LOG("经验原值 %d bp %d", exp_amount, bp);

    if ((c->game_data->expboost) && (l->exp_mult > 0))
        exp_amount = l->bb_params[bp].exp * l->exp_mult;

    if (!exp_amount)
        ERR_LOG("未获取到经验新值 %d bp %d 倍率 %d", exp_amount, bp, l->exp_mult);

    // TODO 新增房间共享经验 分别向其余3个玩家发送数值不等的经验值
    if (!pkt->last_hitter) {
        exp_amount = (exp_amount * 80) / 100;
    }

    return client_give_exp(c, exp_amount);
}

static int handle_bb_guild_ex_item(ship_client_t* c, subcmd_bb_guild_ex_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
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

    if (pkt->shdr.client_id != c->client_id || ex_amount == 0) {
        DBG_LOG("错误 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* TODO 完成挑战模式的物品掉落 */
    if (l->challenge) {
        send_msg(c, MSG1_TYPE, "%s", __(c, "\t挑战模式不支持转换物品!"));
        /* 数据包完成, 发送至游戏房间. */
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
    }

    if (ex_item_id == EMPTY_STRING) {
        DBG_LOG("错误 0x60 指令: 0x%02X", pkt->hdr.pkt_type);
        UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* 从玩家的背包中移除该物品. */
    found = item_remove_from_inv(c->bb_pl->inv.iitems,
        c->bb_pl->inv.item_count, ex_item_id,
        LE32(ex_amount));

    if (found < 0) {
        ERR_LOG("无法从玩家背包中移除物品!");
        return -1;
    }

    send_msg(c, MSG1_TYPE, "%s", __(c, "\t物品转换成功!"));

    c->bb_pl->inv.item_count -= found;
    c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_gallon_area(ship_client_t* c, subcmd_bb_gallon_area_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t quest_offset = pkt->quest_offset;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在大厅中触发任务指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
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
        *(uint32_t*)&c->bb_pl->quest_data2[quest_offset] = *(uint32_t*)&pkt->data[0];
    }

    return send_pkt_bb(c, (bb_pkt_hdr_t*)pkt);

}











// 定义函数指针数组
subcmd60_bb_handle_t subcmd60_bb_handle[0x100] = {
    /* 60指令 0x00 */ NULL,
    /* 60指令 0x01 */ NULL,
    /* 60指令 0x02 */ NULL,
    /* 60指令 0x03 */ NULL,
    /* 60指令 0x04 */ NULL,
    /* 60指令 0x05 */ handle_bb_switch_changed,
    /* 60指令 0x06 */ NULL,
    /* 60指令 0x07 */ handle_bb_symbol_chat,
    /* 60指令 0x08 */ NULL,
    /* 60指令 0x09 */ NULL,
    /* 60指令 0x0A */ handle_bb_mhit,
    /* 60指令 0x0B */ handle_bb_objhit,
    /* 60指令 0x0C */ handle_bb_condition,
    /* 60指令 0x0D */ handle_bb_condition,
    /* 60指令 0x0E */ NULL,
    /* 60指令 0x0F */ NULL,
    /* 60指令 0x10 */ NULL,
    /* 60指令 0x11 */ NULL,
    /* 60指令 0x12 */ handle_bb_dragon_act,
    /* 60指令 0x13 */ NULL,
    /* 60指令 0x14 */ NULL,
    /* 60指令 0x15 */ NULL,
    /* 60指令 0x16 */ NULL,
    /* 60指令 0x17 */ NULL,
    /* 60指令 0x18 */ NULL,
    /* 60指令 0x19 */ NULL,
    /* 60指令 0x1A */ NULL,
    /* 60指令 0x1B */ NULL,
    /* 60指令 0x1C */ NULL,
    /* 60指令 0x1D */ NULL,
    /* 60指令 0x1E */ NULL,
    /* 60指令 0x1F */ handle_bb_set_area_1F,
    /* 60指令 0x20 */ handle_bb_set_area_20,
    /* 60指令 0x21 */ handle_bb_inter_level_warp,
    /* 60指令 0x22 */ handle_bb_load_22,
    /* 60指令 0x23 */ handle_bb_finish_load,
    /* 60指令 0x24 */ handle_bb_set_pos_0x24,
    /* 60指令 0x25 */ handle_bb_equip,
    /* 60指令 0x26 */ handle_bb_unequip,
    /* 60指令 0x27 */ handle_bb_use_item,
    /* 60指令 0x28 */ handle_bb_feed_mag,
    /* 60指令 0x29 */ handle_bb_destroy_item,
    /* 60指令 0x2A */ handle_bb_drop_item,
    /* 60指令 0x2B */ NULL,
    /* 60指令 0x2C */ handle_bb_talk_npc,
    /* 60指令 0x2D */ handle_bb_done_talk_npc,
    /* 60指令 0x2E */ NULL,
    /* 60指令 0x2F */ handle_bb_hit_by_enemy,
    /* 60指令 0x30 */ NULL,
    /* 60指令 0x31 */ handle_bb_cmd_check_client_id,
    /* 60指令 0x32 */ handle_bb_cmd_check_client_id,
    /* 60指令 0x33 */ NULL,
    /* 60指令 0x34 */ NULL,
    /* 60指令 0x35 */ NULL,
    /* 60指令 0x36 */ NULL,
    /* 60指令 0x37 */ NULL,
    /* 60指令 0x38 */ NULL,
    /* 60指令 0x39 */ NULL,
    /* 60指令 0x3A */ handle_bb_cmd_3a,
    /* 60指令 0x3B */ handle_bb_load_3b,
    /* 60指令 0x3C */ NULL,
    /* 60指令 0x3D */ NULL,
    /* 60指令 0x3E */ handle_bb_set_pos,
    /* 60指令 0x3F */ handle_bb_set_pos,
    /* 60指令 0x40 */ handle_bb_move,
    /* 60指令 0x41 */ NULL,
    /* 60指令 0x42 */ handle_bb_move,
    /* 60指令 0x43 */ handle_bb_normal_attack,
    /* 60指令 0x44 */ handle_bb_normal_attack,
    /* 60指令 0x45 */ handle_bb_normal_attack,
    /* 60指令 0x46 */ handle_bb_objhit_phys,
    /* 60指令 0x47 */ handle_bb_objhit_tech,
    /* 60指令 0x48 */ handle_bb_used_tech,
    /* 60指令 0x49 */ NULL,
    /* 60指令 0x4A */ handle_bb_defense_damage,
    /* 60指令 0x4B */ handle_bb_take_damage,
    /* 60指令 0x4C */ handle_bb_take_damage,
    /* 60指令 0x4D */ handle_bb_death_sync,
    /* 60指令 0x4E */ handle_bb_cmd_4e,
    /* 60指令 0x4F */ handle_bb_player_saved,
    /* 60指令 0x50 */ handle_bb_switch_req,
    /* 60指令 0x51 */ NULL,
    /* 60指令 0x52 */ handle_bb_menu_req,
    /* 60指令 0x53 */ NULL,
    /* 60指令 0x54 */ NULL,
    /* 60指令 0x55 */ handle_bb_map_warp_55,
    /* 60指令 0x56 */ NULL,
    /* 60指令 0x57 */ NULL,
    /* 60指令 0x58 */ handle_bb_lobby_act,
    /* 60指令 0x59 */ NULL,
    /* 60指令 0x5A */ NULL,
    /* 60指令 0x5B */ NULL,
    /* 60指令 0x5C */ NULL,
    /* 60指令 0x5D */ NULL,
    /* 60指令 0x5E */ NULL,
    /* 60指令 0x5F */ NULL,
    /* 60指令 0x60 */ NULL,
    /* 60指令 0x61 */ handle_bb_level_up_req,
    /* 60指令 0x62 */ NULL,
    /* 60指令 0x63 */ handle_bb_destroy_ground_item,
    /* 60指令 0x64 */ NULL,
    /* 60指令 0x65 */ NULL,
    /* 60指令 0x66 */ NULL,
    /* 60指令 0x67 */ handle_bb_cmd_check_lobby,
    /* 60指令 0x68 */ handle_bb_create_pipe,
    /* 60指令 0x69 */ handle_bb_spawn_npc,
    /* 60指令 0x6A */ handle_bb_subcmd_6a,
    /* 60指令 0x6B */ NULL,
    /* 60指令 0x6C */ NULL,
    /* 60指令 0x6D */ NULL,
    /* 60指令 0x6E */ NULL,
    /* 60指令 0x6F */ NULL,
    /* 60指令 0x70 */ NULL,
    /* 60指令 0x71 */ NULL,
    /* 60指令 0x72 */ NULL,
    /* 60指令 0x73 */ NULL,
    /* 60指令 0x74 */ handle_bb_word_select,
    /* 60指令 0x75 */ handle_bb_set_flag,
    /* 60指令 0x76 */ handle_bb_killed_monster,
    /* 60指令 0x77 */ handle_bb_sync_reg,
    /* 60指令 0x78 */ NULL,
    /* 60指令 0x79 */ handle_bb_gogo_ball,
    /* 60指令 0x7A */ NULL,
    /* 60指令 0x7B */ NULL,
    /* 60指令 0x7C */ handle_bb_game_mode,
    /* 60指令 0x7D */ NULL,
    /* 60指令 0x7E */ NULL,
    /* 60指令 0x7F */ NULL,
    /* 60指令 0x80 */ NULL,
    /* 60指令 0x81 */ NULL,
    /* 60指令 0x82 */ NULL,
    /* 60指令 0x83 */ NULL,
    /* 60指令 0x84 */ NULL,
    /* 60指令 0x85 */ NULL,
    /* 60指令 0x86 */ NULL,
    /* 60指令 0x87 */ NULL,
    /* 60指令 0x88 */ handle_bb_arrow_change,
    /* 60指令 0x89 */ handle_bb_player_died,
    /* 60指令 0x8A */ handle_bb_Unknown_6x8A,
    /* 60指令 0x8B */ NULL,
    /* 60指令 0x8C */ NULL,
    /* 60指令 0x8D */ handle_bb_set_technique_level_override,
    /* 60指令 0x8E */ NULL,
    /* 60指令 0x8F */ NULL,
    /* 60指令 0x90 */ NULL,
    /* 60指令 0x91 */ NULL,
    /* 60指令 0x92 */ NULL,
    /* 60指令 0x93 */ handle_bb_timed_switch_activated,
    /* 60指令 0x94 */ NULL,
    /* 60指令 0x95 */ NULL,
    /* 60指令 0x96 */ NULL,
    /* 60指令 0x97 */ NULL,
    /* 60指令 0x98 */ NULL,
    /* 60指令 0x99 */ NULL,
    /* 60指令 0x9A */ NULL,
    /* 60指令 0x9B */ NULL,
    /* 60指令 0x9C */ NULL,
    /* 60指令 0x9D */ NULL,
    /* 60指令 0x9E */ NULL,
    /* 60指令 0x9F */ NULL,
    /* 60指令 0xA0 */ NULL,
    /* 60指令 0xA1 */ handle_bb_save_player_act,
    /* 60指令 0xA2 */ NULL,
    /* 60指令 0xA3 */ NULL,
    /* 60指令 0xA4 */ NULL,
    /* 60指令 0xA5 */ NULL,
    /* 60指令 0xA6 */ NULL,
    /* 60指令 0xA7 */ NULL,
    /* 60指令 0xA8 */ NULL,
    /* 60指令 0xA9 */ NULL,
    /* 60指令 0xAA */ NULL,
    /* 60指令 0xAB */ handle_bb_lobby_chair,
    /* 60指令 0xAC */ NULL,
    /* 60指令 0xAD */ NULL,
    /* 60指令 0xAE */ handle_bb_lobby_chair,
    /* 60指令 0xAF */ handle_bb_lobby_chair,
    /* 60指令 0xB0 */ handle_bb_lobby_chair,
    /* 60指令 0xB1 */ NULL,
    /* 60指令 0xB2 */ NULL,
    /* 60指令 0xB3 */ NULL,
    /* 60指令 0xB4 */ NULL,
    /* 60指令 0xB5 */ NULL,
    /* 60指令 0xB6 */ NULL,
    /* 60指令 0xB7 */ NULL,
    /* 60指令 0xB8 */ NULL,
    /* 60指令 0xB9 */ NULL,
    /* 60指令 0xBA */ NULL,
    /* 60指令 0xBB */ NULL,
    /* 60指令 0xBC */ NULL,
    /* 60指令 0xBD */ NULL,
    /* 60指令 0xBE */ NULL,
    /* 60指令 0xBF */ NULL,
    /* 60指令 0xC0 */ handle_bb_sell_item,
    /* 60指令 0xC1 */ NULL,
    /* 60指令 0xC2 */ NULL,
    /* 60指令 0xC3 */ handle_bb_drop_split_stacked_item,
    /* 60指令 0xC4 */ handle_bb_sort_inv,
    /* 60指令 0xC5 */ handle_bb_medic,
    /* 60指令 0xC6 */ handle_bb_steal_exp,
    /* 60指令 0xC7 */ NULL,
    /* 60指令 0xC8 */ handle_bb_req_exp,
    /* 60指令 0xC9 */ NULL,
    /* 60指令 0xCA */ NULL,
    /* 60指令 0xCB */ NULL,
    /* 60指令 0xCC */ handle_bb_guild_ex_item,
    /* 60指令 0xCD */ NULL,
    /* 60指令 0xCE */ NULL,
    /* 60指令 0xCF */ NULL,
    /* 60指令 0xD0 */ NULL,
    /* 60指令 0xD1 */ NULL,
    /* 60指令 0xD2 */ handle_bb_gallon_area,
    /* 60指令 0xD3 */ NULL,
    /* 60指令 0xD4 */ NULL,
    /* 60指令 0xD5 */ NULL,
    /* 60指令 0xD6 */ NULL,
    /* 60指令 0xD7 */ NULL,
    /* 60指令 0xD8 */ NULL,
    /* 60指令 0xD9 */ NULL,
    /* 60指令 0xDA */ NULL,
    /* 60指令 0xDB */ NULL,
    /* 60指令 0xDC */ NULL,
    /* 60指令 0xDD */ NULL,
    /* 60指令 0xDE */ NULL,
    /* 60指令 0xDF */ NULL,
    /* 60指令 0xE0 */ NULL,
    /* 60指令 0xE1 */ NULL,
    /* 60指令 0xE2 */ NULL,
    /* 60指令 0xE3 */ NULL,
    /* 60指令 0xE4 */ NULL,
    /* 60指令 0xE5 */ NULL,
    /* 60指令 0xE6 */ NULL,
    /* 60指令 0xE7 */ NULL,
    /* 60指令 0xE8 */ NULL,
    /* 60指令 0xE9 */ NULL,
    /* 60指令 0xEA */ NULL,
    /* 60指令 0xEB */ NULL,
    /* 60指令 0xEC */ NULL,
    /* 60指令 0xED */ NULL,
    /* 60指令 0xEE */ NULL,
    /* 60指令 0xEF */ NULL,
    /* 60指令 0xF0 */ NULL,
    /* 60指令 0xF1 */ NULL,
    /* 60指令 0xF2 */ NULL,
    /* 60指令 0xF3 */ NULL,
    /* 60指令 0xF4 */ NULL,
    /* 60指令 0xF5 */ NULL,
    /* 60指令 0xF6 */ NULL,
    /* 60指令 0xF7 */ NULL,
    /* 60指令 0xF8 */ NULL,
    /* 60指令 0xF9 */ NULL,
    /* 60指令 0xFA */ NULL,
    /* 60指令 0xFB */ NULL,
    /* 60指令 0xFC */ NULL,
    /* 60指令 0xFD */ NULL,
    /* 60指令 0xFE */ NULL,
    /* 60指令 0xFF */ NULL,
};
