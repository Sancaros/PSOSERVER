/*
    �λ�֮���й� ���������� 60ָ���
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
#include "ship.h"

#include "subcmd_handle.h"

extern ship_t* ship;

/* Player levelup data */
extern bb_level_table_t bb_char_stats;
extern v2_level_table_t v2_char_stats;

extern psocn_bb_mode_char_t default_mode_char;
/* �Ѹ�Ϊ���ݿ����� */
extern bb_max_tech_level_t max_tech_level[MAX_PLAYER_TECHNIQUES];

static int sub60_unimplement_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("%s �����ڴ���������Ϸָ��!", get_player_describe(src));
        return -1;
    }

    DBG_LOG("δ����ָ�� 0x%02X", pkt->hdr.pkt_type);

    print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_check_client_id_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (pkt->param != src->client_id) {
        ERR_LOG("%s �ڴ�������Ϸ������ %s ָ��! ��Client ID��һ��", get_player_describe(src), c_cmd_name(pkt->hdr.pkt_type, 0));
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        return -1;
    }

    //UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_check_lobby_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("%s �ڴ�����������Ϸ %s ָ��!", get_player_describe(src), c_cmd_name(pkt->hdr.pkt_type, 0));
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        return -1;
    }

    DBG_LOG("��� 0x%02X ָ��: 0x%X", pkt->hdr.pkt_type, pkt->type);
    print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

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

static void update_qpos(ship_client_t* src, lobby_t* l) {
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

static void update_bb_qpos(ship_client_t* src, lobby_t* l) {
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

static void handle_objhit_common(ship_client_t* src, lobby_t* l, uint16_t bid) {
    uint32_t obj_type;

    /* What type of object was hit? */
    if ((bid & 0xF000) == 0x4000) {
        /* An object was hit */
        bid &= 0x0FFF;

        /* Make sure the object is in range. */
        if (bid > l->map_objs->obj_count) {
            ERR_LOG("%s hit invalid object "
                "(%d -- max: %d)!\n"
                "Episode: %d, Floor: %d, Map: (%d, %d)", get_player_describe(src),
                bid, l->map_objs->obj_count, l->episode, src->cur_area,
                l->maps[src->cur_area << 1],
                l->maps[(src->cur_area << 1) + 1]);

            if ((l->flags & LOBBY_FLAG_QUESTING))
                ERR_LOG("Quest ID: %d, Version: %d", l->qid,
                    l->version);

            /* Just continue on and hope for the best. We've probably got a
               bug somewhere on our end anyway... */
            return;
        }

        /* Make sure it isn't marked as hit already. */
        if ((l->map_objs->objs[bid].flags & 0x80000000))
            return;

        /* Now, see if we care about the type of the object that was hit. */
        obj_type = l->map_objs->objs[bid].mobj_data.base_type & 0xFFFF;

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

static void handle_bb_objhit_common(ship_client_t* src, lobby_t* l, uint16_t bid) {
    uint32_t obj_type;

    /* What type of object was hit? */
    if ((bid & 0xF000) == 0x4000) {
        /* An object was hit */
        bid &= 0x0FFF;

        /* Make sure the object is in range. */
        if (bid > l->map_objs->obj_count) {
#ifdef DEBUG

            DBG_LOG("%s ��������Ч��ʵ�� "
                "(%d -- ��ͼʵ������: %d)!\n"
                "�½�: %d, �㼶: %d, ��ͼ: (%d, %d)", get_char_describe(src),
                bid, l->map_objs->count, l->episode, src->cur_area,
                l->maps[src->cur_area << 1],
                l->maps[(src->cur_area << 1) + 1]);

#endif // DEBUG

            if ((l->flags & LOBBY_FLAG_QUESTING))

#ifdef DEBUG

                DBG_LOG("���� ID: %d, �汾: %d", l->qid,
                    l->version);

#endif // DEBUG

            /* Just continue on and hope for the best. We've probably got a
               bug somewhere on our end anyway... */
            return;
        }

        /* Make sure it isn't marked as hit already. */
        if ((l->map_objs->objs[bid].flags & 0x80000000))
            return;

        /* Now, see if we care about the type of the object that was hit. */
        obj_type = l->map_objs->objs[bid].mobj_data.base_type & 0xFFFF;

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

static int check_aoe_timer(ship_client_t* src,
    subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t tech_level = 0;
    uint16_t technique_number = pkt->technique_number;

    /* �����Լ��... Does the character have that level of technique? */
    tech_level = src->pl->bb.character.tech.all[technique_number];
    if (tech_level == 0xFF) {
        /* ����û����Ŷ���ѧϰһ���¼���������ܻᷢ�����������
        �������������Ŀ�����֮ǰ�����ǽ����ò��۸���һ�㡣
        һ������������ʵ�ġ������Ŀ����٣�����������ܻ�Ͽ��ͻ��˵����� */
        tech_level = pkt->level;
    }

    if (src->version >= CLIENT_VERSION_DCV2)
        tech_level += src->pl->bb.character.inv.iitems[technique_number].extension_data1;

    if (tech_level < pkt->level) {
        /* ����û����Ŷ���ѧϰһ���¼���������ܻᷢ�����������
        �������������Ŀ�����֮ǰ�����ǽ����ò��۸���һ�㡣
        һ������������ʵ�ġ������Ŀ����٣�����������ܻ�Ͽ��ͻ��˵����� */
        tech_level = pkt->level;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    /* See what technique was used... */
    switch (technique_number) {
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

static int sub60_05_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_switch_changed_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_switch_changed_pkt_t), 0x03))
        return -2;

//[2023��09��27�� 19:49:59:564] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x05
//
//[2023��09��27�� 19:49:59:572] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 10 00 05 03 6A 40  00 00 00 00 06 00 01 01    `.....j@........
    rv = subcmd_send_lobby_dc(l, NULL, (subcmd_pkt_t*)pkt, 0);

    if (pkt->data.flags && pkt->data.object_id != 0xFFFF) {
        if ((l->flags & LOBBY_TYPE_CHEATS_ENABLED) && src->options.switch_assist &&
            (src->last_switch_enabled_command.type == 0x05)) {
            DBG_LOG("[��������] �ظ�������һ������");
            subcmd_switch_changed_pkt_t* gem = pkt;
            gem->data = src->last_switch_enabled_command;
            rv = subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)gem, 0);
        }
        src->last_switch_enabled_command = pkt->data;
    }

    return rv;
}

static int sub60_05_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_switch_changed_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (!in_game(src))
        return -1;

    if(!check_pkt_size(src, pkt, sizeof(subcmd_bb_switch_changed_pkt_t), 0x03))
        return -2;

    //[2023��02��09�� 22:51:33:981] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   04 00 05 03                                         ....
    //[2023��02��09�� 22:51:34:017] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   04 00 05 03                                         ....
    //[2023��02��09�� 22:51:34:054] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   05 00 05 01                                         ....
    //[2023��02��09�� 22:51:34:089] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   06 00 05 01                                         ....
    //[2023��02��09�� 22:51:34:124] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   07 00 05 01                                         ....
    //[2023��02��09�� 22:51:34:157] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   08 00 05 01                                         ....
    //[2023��02��09�� 22:51:34:194] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   09 00 05 01                                              ...
    //[2023��02��09�� 22:51:34:228] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   05 00 05 01                                         ....
    //[2023��02��09�� 22:51:34:262] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   06 00 05 01                                         ....
    //[2023��02��09�� 22:51:34:315] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   07 00 05 01                                         ....
    //[2023��02��09�� 22:51:34:354] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   08 00 05 01                                         ....
    //[2023��02��09�� 22:51:34:390] ����(subcmd-bb.c 4924): Unknown 0x60: 0x05
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  05 03 FF FF 00 00 00 00    ..`.............
    //( 00000010 )   09 00 05 01                                              ...
    rv = subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);

    if (pkt->data.flags && pkt->data.object_id != 0xFFFF) {
        if ((l->flags & LOBBY_TYPE_CHEATS_ENABLED) && src->options.switch_assist &&
            (src->last_switch_enabled_command.type == 0x05)) {
            DBG_LOG("[��������] �ظ�������һ������");
            subcmd_bb_switch_changed_pkt_t* gem = pkt;
            gem->data = src->last_switch_enabled_command;
            rv = subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)gem, 0);
        }
        src->last_switch_enabled_command = pkt->data;
    }

    return rv;
}

static int sub60_07_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_symbol_chat_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Don't send the message if they have the protection flag on. */
    if (src->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(src, __(src, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Don't send chats for a STFUed client. */
    if ((src->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 1);
}

static int sub60_07_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_symbol_chat_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Don't send the message if they have the protection flag on. */
    if (src->flags & CLIENT_FLAG_GC_PROTECT)
        return send_txt(src, __(src, "\tE\tC7�������¼��ſ��Խ��в���."));

    /* Don't send chats for a STFUed client. */
    if ((src->flags & CLIENT_FLAG_STFU))
        return 0;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 1);
}

/* XXXX: We need to handle the b0rked nature of the Gamecube packet for this one
   still (for more than just kill tracking). */
static int sub60_0A_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_mhit_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t mid, mid2, dmg;
    game_enemy_t* en;
    uint32_t flags;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_mhit_pkt_t), 0x03))
        return -2;

    /* Grab relevant information from the packet */
    mid2 = LE16(pkt->shdr.enemy_id);
    mid = LE16(pkt->enemy_id2);
    dmg = LE16(pkt->damage);
    flags = LE32(pkt->flags);

    /* Swap the flags on the packet if the user is on GC... Looks like Sega
       decided that it should be the opposite order as it is on DC/PC/BB. */
    if (src->version == CLIENT_VERSION_GC)
        flags = SWAP32(flags);

    /* Bail out now if we don't have any enemy data on the team. */
    if (!l->map_enemies || l->challenge || l->battle) {
        script_execute(ScriptActionEnemyHit, src, SCRIPT_ARG_PTR, src,
            SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        if (flags & 0x00000800)
            script_execute(ScriptActionEnemyKill, src, SCRIPT_ARG_PTR, src,
                SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        return send_lobby_mhit(l, src, mid, mid2, dmg, flags);
    }

    /* Make sure the enemy is in range. */
    if (mid > l->map_enemies->enemy_count) {
#ifdef DEBUG
        ERR_LOG("%s hit invalid enemy (%d -- max: "
            "%d)!\n"
            "Episode: %d, Floor: %d, Map: (%d, %d)", get_char_describe(src), mid,
            l->map_enemies->count, l->episode, c->cur_area,
            l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);

        if ((l->flags & LOBBY_FLAG_QUESTING))
            ERR_LOG("Quest ID: %d, Version: %d", l->qid, l->version);
#endif

        if (l->logfp) {
            fdebug(l->logfp, DBG_WARN, "%s hit invalid "
                "enemy (%d -- max: %d)!\n"
                "Episode: %d, Floor: %d, Map: (%d, %d)\n", get_player_describe(src), mid,
                l->map_enemies->enemy_count, l->episode, src->cur_area,
                l->maps[src->cur_area << 1], l->maps[(src->cur_area << 1) + 1]);

            if ((l->flags & LOBBY_FLAG_QUESTING))
                fdebug(l->logfp, DBG_WARN, "Quest ID: %d, Version: %d\n",
                    l->qid, l->version);
        }

        script_execute(ScriptActionEnemyHit, src, SCRIPT_ARG_PTR, src,
            SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        if (flags & 0x00000800)
            script_execute(ScriptActionEnemyKill, src, SCRIPT_ARG_PTR, src,
                SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_END);

        /* If server-side drops aren't on, then just send it on and hope for the
           best. We've probably got a bug somewhere on our end anyway... */
        if (!(l->flags & LOBBY_FLAG_SERVER_DROPS))
            return send_lobby_mhit(l, src, mid, mid2, dmg, flags);

        return -1;
    }

    /* Make sure it looks like they're in the right area for this... */
    /* XXXX: There are some issues still with Episode 2, so only spit this out
       for now on Episode 1. */
#ifdef DEBUG
    if (c->cur_area != l->map_enemies->enemies[mid].area && l->episode == 1 &&
        !(l->flags & LOBBY_FLAG_QUESTING)) {
        ERR_LOG("%s hit enemy in wrong area "
            "(%d -- max: %d)!\n Episode: %d, Area: %d, Enemy Area: %d "
            "Map: (%d, %d)", get_char_describe(src), mid, l->map_enemies->count,
            l->episode, c->cur_area, l->map_enemies->enemies[mid].area,
            l->maps[c->cur_area << 1], l->maps[(c->cur_area << 1) + 1]);
    }
#endif

    if (l->logfp && src->cur_area != l->map_enemies->enemies[mid].area &&
        !(l->flags & LOBBY_FLAG_QUESTING)) {
        fdebug(l->logfp, DBG_WARN, "%s hit enemy in wrong "
            "area (%d -- max: %d)!\n Episode: %d, Area: %d, Enemy Area: %d "
            "Map: (%d, %d)", get_player_describe(src), mid, l->map_enemies->enemy_count,
            l->episode, src->cur_area, l->map_enemies->enemies[mid].area,
            l->maps[src->cur_area << 1], l->maps[(src->cur_area << 1) + 1]);
    }

    /* Make sure the person's allowed to be on this floor in the first place. */
    if ((l->flags & LOBBY_FLAG_ONLY_ONE) && !(l->flags & LOBBY_FLAG_QUESTING)) {
        if (l->episode == 1) {
            switch (src->cur_area) {
            case 5:     /* Cave 3 */
            case 12:    /* De Rol Le */
                ERR_LOG("%s hit enemy in area "
                    "impossible to\nreach in a single-player team (%d)\n"
                    "Team Flags: %08" PRIx32 "", get_player_describe(src), src->cur_area, l->flags);
                break;
            }
        }
    }

    /* Save the hit, assuming the enemy isn't already dead. */
    en = &l->map_enemies->enemies[mid];
    if (!(en->clients_hit & 0x80)) {
        en->clients_hit |= (1 << src->client_id);
        en->last_client = src->client_id;

        script_execute(ScriptActionEnemyHit, src, SCRIPT_ARG_PTR, src,
            SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_UINT32, en->bp_entry,
            SCRIPT_ARG_UINT8, en->rt_index, SCRIPT_ARG_UINT8,
            en->clients_hit, SCRIPT_ARG_END);

        /* If the kill flag is set, mark it as dead and update the client's
           counter. */
        if (flags & 0x00000800) {
            en->clients_hit |= 0x80;

            script_execute(ScriptActionEnemyKill, src, SCRIPT_ARG_PTR, src,
                SCRIPT_ARG_UINT16, mid, SCRIPT_ARG_UINT32,
                en->bp_entry, SCRIPT_ARG_UINT8, en->rt_index,
                SCRIPT_ARG_UINT8, en->clients_hit, SCRIPT_ARG_END);

            if (en->bp_entry < 0x60 && !(l->flags & LOBBY_FLAG_HAS_NPC))
                ++src->enemy_kills[en->bp_entry];
        }
    }

    return send_lobby_mhit(l, src, mid, mid2, dmg, flags);
}

static int sub60_0A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_mhit_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t enemy_id2, enemy_id, dmg;
    game_enemy_t* en;
    uint32_t flags;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_mhit_pkt_t), 0x03))
        return -2;

    /* Make sure the enemy is in range. */
    enemy_id = LE16(pkt->shdr.enemy_id);
    enemy_id2 = LE16(pkt->enemy_id2);
    dmg = LE16(pkt->damage);
    flags = LE32(pkt->flags);

    /* Swap the flags on the packet if the user is on GC... Looks like Sega
       decided that it should be the opposite order as it is on DC/PC/BB. */
    if (src->version == CLIENT_VERSION_GC)
        flags = SWAP32(flags);

    if (enemy_id2 > l->map_enemies->enemy_count) {
#ifdef DEBUG
        ERR_LOG("%s ��������Ч�Ĺ��� (%d -- ��ͼ��������: "
            "%d)!", get_player_describe(src), enemy_id2, l->map_enemies->count);
#endif // DEBUG

        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Save the hit, assuming the enemy isn't already dead. */
    en = &l->map_enemies->enemies[enemy_id2];
    if (!(en->clients_hit & 0x80)) {
        en->clients_hit |= (1 << src->client_id);
        en->last_client = src->client_id;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_0B_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_bhit_pkt_t* pkt) {
    uint64_t now = get_ms_time();
    lobby_t* l = src->cur_lobby;

    /* ������Ҫ��ͼ��ս������սģʽ�д�����Щ����. */
    if (l->challenge || l->battle) {
        ERR_LOG("%s ��ģʽ�й���������!",
            get_player_describe(src));
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }
    
    if (!in_game(src))
        return -1;

    /* We only care about these if the AoE timer is set on the sender. */
    if (src->aoe_timer < now)
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    /* Check the type of the object that was hit. As the AoE timer can't be set
       if the objects aren't loaded, this shouldn't ever trip up... */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    /* Handle the object marked as hit, if appropriate. */
    handle_objhit_common(src, l, LE16(pkt->shdr.object_id));

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_0B_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_bhit_pkt_t* pkt) {
    uint64_t now = get_ms_time();
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

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

static int sub60_0C_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_add_or_remove_condition_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!pkt->condition_type)
        DBG_LOG("sub60_0C_bb 0x%zX", pkt->condition_type);

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_0C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_add_or_remove_condition_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!pkt->condition_type)
        DBG_LOG("sub60_0C_bb 0x%zX", pkt->condition_type);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_0D_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_add_or_remove_condition_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (pkt->condition_type) {
        if (src->game_data->err.has_error) {
            send_msg(src, BB_SCROLL_MSG_TYPE,
                "%s ����ָ��:0x%zX ��ָ��:0x%zX",
                __(src, "\tE\tC6���ݳ���,����ϵ����Ա����!"),
                src->game_data->err.error_cmd_type,
                src->game_data->err.error_subcmd_type
            );
            clean_client_err(&src->game_data->err);
        }

        if (l->flags & LOBBY_TYPE_GAME) {
            lobby_print_info2(src);
        }
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_0D_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_add_or_remove_condition_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (pkt->condition_type) {
        if (src->game_data->err.has_error) {
            send_msg(src, BB_SCROLL_MSG_TYPE,
                "%s ����ָ��:0x%zX ��ָ��:0x%zX",
                __(src, "\tE\tC6���ݳ���,����ϵ����Ա����!"),
                src->game_data->err.error_cmd_type,
                src->game_data->err.error_subcmd_type
            );
            clean_client_err(&src->game_data->err);
        }

        if (l->flags & LOBBY_TYPE_GAME) {
            lobby_print_info2(src);
        }
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_12_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_dragon_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int v = src->version, i;
    subcmd_dragon_act_t tr;

    if (!in_game(src))
        return -1;

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_dragon_act_t));
    tr.x.b = SWAP32(tr.x.b);
    tr.z.b = SWAP32(tr.z.b);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != src) {
            if (l->clients[i]->version == v) {
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
            }
            else {
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&tr);
            }
        }
    }

    return 0;
}

static int sub60_12_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_dragon_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int v = src->version, i;
    subcmd_bb_dragon_act_t tr;
    
    if (!in_game(src))
        return -1;

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

static int sub60_13_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_de_rolLe_boss_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t action = pkt->action, stage = pkt->stage;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_de_rolLe_boss_act_t), 0x02))
        return -2;

    send_txt(src, "%s\n����:0x%04X\n�׶�:0x%04X.", __(src, "\tE\tC6DR BOSS"), action, stage);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_14_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_de_rolLe_boss_sact_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t action = pkt->action, stage = pkt->stage;
    uint32_t unused = pkt->unused;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_de_rolLe_boss_sact_t), 0x03))
        return -2;

    send_txt(src, "%s\n����:0x%04X\n�׶�:0x%04X\n����:0x%08X.", __(src, "\tE\tC6DR BOSS2"),
        action, stage, unused);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_15_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_VolOptBossActions_6x15_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t unknown_a2 = pkt->unknown_a2, unknown_a3 = pkt->unknown_a3, unknown_a4 = pkt->unknown_a4, unknown_a5 = pkt->unknown_a5;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_VolOptBossActions_6x15_t), 0x03))
        return -2;

    send_txt(src, "%s\n����:0x%04X\n�׶�:0x%04X\n����1:0x%04X\n����2:0x%04X.", __(src, "\tE\tC6Vol Opt BOSS1"),
        unknown_a2, unknown_a3, unknown_a4, unknown_a5);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_16_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_VolOptBossActions_6x16_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_VolOptBossActions_6x16_t), 0x03))
        return -2;

    send_txt(src, "%s\n����:0x%04X\n�׶�:0x%04X\n����1:0x%04X\n����2:0x%04X.", __(src, "\tE\tC6Vol Opt BOSS2"),
        pkt->unknown_a2, pkt->unknown_a3, pkt->unknown_a4, pkt->unknown_a5);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_17_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_teleport_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_teleport_t), 0x05))
        return -2;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_18_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_dragon_special_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_dragon_special_act_t), 0x03))
        return -2;

    send_txt(src, "%s\n����1:%f\n����2:%f\n����1:0x%08X\n����2:0x%08X.", __(src, "\tE\tC6Dragon BOSS"),
        pkt->unknown_a1, pkt->unknown_a2, pkt->unknown_a1, pkt->unknown_a2);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_19_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_DarkFalzActions_6x19_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_DarkFalzActions_6x19_t), 0x04))
        return -2;

    send_txt(src, "%s\n����:0x%04X\n�׶�:0x%04X\n����1:0x%04X\n����2:0x%04X.", __(src, "\tE\tC6Dark Falz BOSS"),
        pkt->unknown_a2, pkt->unknown_a3, pkt->unknown_a4, pkt->unused);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_1B_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_create_npc_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_create_npc_t), 0x01))
        return -2;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_1C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_destory_npc_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_destory_npc_t), 0x01))
        return -2;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_1F_dc(ship_client_t* c, ship_client_t* dest, 
    subcmd_set_area_1F_t* pkt) {
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

        /* Clear the list of dropped items. */
        if (c->cur_area == 0) {
            memset(c->p2_drops, 0, sizeof(c->p2_drops));
            c->p2_drops_max = 0;
        }

        c->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(c, l);
    }

    return subcmd_send_lobby_dc(l, c, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_1F_bb(ship_client_t* src, ship_client_t* dest,
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

        // ����DC����
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

static int sub60_20_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_set_area_20_t* pkt) {
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

        /* Clear the list of dropped items. */
        if (src->cur_area == 0) {
            memset(src->p2_drops, 0, sizeof(src->p2_drops));
            src->p2_drops_max = 0;
        }

        src->cur_area = pkt->area;
        src->x = pkt->x;
        src->y = pkt->y;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(src, l);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_20_bb(ship_client_t* src, ship_client_t* dest,
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

        // ����DC����
        /* Clear the list of dropped items. */
        if (src->cur_area == 0) {
            memset(src->p2_drops, 0, sizeof(src->p2_drops));
            src->p2_drops_max = 0;
        }

        src->cur_area = pkt->area;
        src->x = pkt->x;
        src->y = pkt->y;
        src->z = pkt->z;

        //�������� �ͻ���֮�����λ��
        //DBG_LOG("�ͻ������� %d ��x:%f y:%f z:%f)", c->cur_area, c->x, c->y, c->z);
        //�ͻ������� 0 ��x:228.995331 y:0.000000 z:253.998901)

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_21_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_inter_level_warp_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        ERR_LOG("%s ���ʹ�������!",
            get_player_describe(src));
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        return -1;
    }

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        script_execute(ScriptActionChangeArea, src, SCRIPT_ARG_PTR, src,
            SCRIPT_ARG_INT, (int)pkt->area, SCRIPT_ARG_INT,
            src->cur_area, SCRIPT_ARG_END);
        src->cur_area = pkt->area;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(src, l);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_21_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_inter_level_warp_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Make sure the area is valid */
    if (pkt->area > 17) {
        ERR_LOG("%s ���ʹ�������!",
            get_player_describe(src)); 
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
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

static int sub60_22_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_set_player_visibility_6x22_6x23_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int i, rv;

    if (l->type == LOBBY_TYPE_LOBBY) {
        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != src &&
                subcmd_send_pos(src, l->clients[i])) {
                rv = -1;
                break;
            }
        }
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_22_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_set_player_visibility_6x22_6x23_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int i, rv;

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

static int sub60_23_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
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
    }

    rv = subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    return rv;
}

static int sub60_23_bb(ship_client_t* src, ship_client_t* dest,
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

        /* �������е���ҹ������ݷ������½���Ŀͻ��� */
        send_bb_guild_cmd(src, BB_GUILD_FULL_DATA);
        send_bb_guild_cmd(src, BB_GUILD_INITIALIZATION_DATA);
    }

    rv = subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    return rv;
}

static int sub60_24_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Ugh... For some reason, v1 really likes to send these at the start of
       quests. And by "really likes to send these", I mean that everybody sends
       one of these for themselves and everybody else in the team... That can
       cause some interesting problems if clients are out of sync at the
       beginning of a quest for any reason (like a PSOPC player playing with
       a v1 player might be, for instance). Thus, we have to ignore these sent
       by v1 players with other players' client IDs at the beginning of a
       quest. */
    if (src->version == CLIENT_VERSION_DCV1) {
        /* �����Լ��... */

        if (!check_pkt_size(src, pkt, 0x0018, 0x05))
            return -2;

        /* Oh look, misusing other portions of the client structure so that I
           don't have to make a new field... */
        if (src->autoreply_len && pkt->data[0] != src->client_id) {
            /* Silently drop the packet. */
            --src->autoreply_len;
            return 0;
        }
    }

    return subcmd_send_lobby_dc(l, src, pkt, 0);
}

static int sub60_24_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_set_pos_0x24_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_set_pos_0x24_t), 0x05))
        return -2;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->x = pkt->x;
        src->y = pkt->y;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
#ifdef DEBUG
        DBG_LOG("GC %u %d %d (0x%X 0x%X) X��:%f Y��:%f Z��:%f", c->guildcard,
            c->client_id, pkt->shdr.client_id, pkt->unk1, pkt->unused, pkt->x, pkt->y, pkt->z);
#endif // DEBUG
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_25_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_equip_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->item_id, equip_slot = pkt->equip_slot, equip_resault = 0;
    int i = 0;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_equip_t), 0x03))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
#ifdef DEBUG
        DBG_LOG("%s ID��һ��!",
            get_player_describe(src));
#endif // DEBUG
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

#ifdef DEBUG

    psocn_bb_char_t* character = get_client_char_bb(src);

    DBG_LOG("%s item_id 0x%08X equip_slot 0x%08X", get_player_describe(src), item_id, equip_slot);

#endif // DEBUG

    equip_resault = player_equip_item(src, item_id);
    /* �Ƿ������Ʒ������? */
    if (equip_resault) {
        ERR_LOG("%s װ����δ���ڵ���Ʒ����! ������:%d",
            get_player_describe(src), equip_resault);
        return -1;
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_26_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_unequip_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->item_id;
    int item_count, i = 0, isframe = 0;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_unequip_t), 0x03))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    inventory_t* inv = get_client_inv_bb(src);

    /* Find the item and remove the equip flag. */
    item_count = inv->item_count;

    i = find_iitem_index(inv, item_id);
    /* ����Ҳ�������Ʒ�����û��Ӵ�������. */
    if (i < 0) {
        ERR_LOG("%s ж����Чװ����Ʒ! ������ %d", get_player_describe(src), i);
        return i;
    }

    inv->iitems[i].flags &= LE32(0xFFFFFFF7);

    /* If its a frame, we have to make sure to unequip any units that
       may be equipped as well. */
    if (inv->iitems[i].data.datab[0] == ITEM_TYPE_GUARD &&
        inv->iitems[i].data.datab[1] == ITEM_SUBTYPE_FRAME) {
        isframe = 1;
    }

    /* Did we find something to equip? */
    if (i >= item_count) {
        ERR_LOG("%s ж����δ���ڵ���Ʒ����! ���� %d ID 0x%08X",
            get_player_describe(src), i, inv->iitems[i].data.item_id);
        print_item_data(&inv->iitems[i].data, src->version);
        return -2;
    }

    /* Clear any units if we unequipped a frame. */
    if (isframe) {
        for (i = 0; i < item_count; ++i) {
            if (inv->iitems[i].data.datab[0] == ITEM_TYPE_GUARD &&
                inv->iitems[i].data.datab[1] == ITEM_SUBTYPE_UNIT) {
                inv->iitems[i].flags &= LE32(0xFFFFFFF7);
            }
        }
    }

    /* Done, let everyone else know. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_27_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_use_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int num;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_use_item_t), 0x02))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
#ifdef DEBUG
        DBG_LOG("�Ƿ���NPC������Ʒ��");
#endif // DEBUG
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    if (!(src->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* Remove the item from the user's inventory */
    num = remove_iitem_v1(src->iitems, src->item_count, pkt->item_id, 1);
    if (num < 0)
        ERR_LOG("Couldn't remove item from inventory!\n");
    else
        src->item_count -= num;

send_pkt:
    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_27_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_use_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = LE32(pkt->item_id);
    errno_t err;

    pthread_mutex_lock(&src->mutex);

    if (!in_game(src)) {
        pthread_mutex_unlock(&src->mutex);
        return -1;
    }

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_use_item_t), 0x02)) {
        pthread_mutex_unlock(&src->mutex);
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
#ifdef DEBUG
        DBG_LOG("�Ƿ���NPC������Ʒ��");
#endif // DEBUG
        pthread_mutex_unlock(&src->mutex);
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    if ((err = player_use_item(src, item_id))) {
        ERR_LOG("%s ʹ����Ʒ��������! ������ %d", get_player_describe(src), err);
        pthread_mutex_unlock(&src->mutex);
        return -3;
    }

    subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    pthread_mutex_unlock(&src->mutex);
    return 0;
}

static int sub60_28_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_feed_mag_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src)) {
        return -1;
    }

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_feed_mag_t), 0x03)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    errno_t err = player_feed_mag(src, pkt->mag_item_id, pkt->fed_item_id);
    if (err) {
        ERR_LOG("%s ���ʹ�������! ������ %d",
            get_player_describe(src), err);
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_29_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_destroy_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int num;
    uint32_t i, item_id;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_destroy_item_t), 0x03))
        return -2;

    /* Ignore meseta */
    if (pkt->item_id != 0xFFFFFFFF) {
        /* Has the user recently dropped this item without picking it up? If so,
           they can't possibly have it in their inventory. */
        item_id = LE32(pkt->item_id);

        for (i = 0; i < src->p2_drops_max; ++i) {
            if (src->p2_drops[i] == item_id) {
                ERR_LOG("%s appears to be duping "
                    "item with id %" PRIu32 "!", get_player_describe(src), item_id);
            }
        }
    }

    if (!(src->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* Ignore meseta */
    if (pkt->item_id != 0xFFFFFFFF) {
        /* Remove the item from the user's inventory */
        num = remove_iitem_v1(src->iitems, src->item_count, pkt->item_id,
            LE32(pkt->amount));
        if (num < 0) {
            ERR_LOG("Couldn't remove item from inventory!\n");
        }
        else {
            src->item_count -= num;
        }
    }

send_pkt:
    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_29_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_destroy_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->item_id, amount = pkt->amount;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_destroy_item_t), 0x03))
        return -2;

    iitem_t item_data = remove_iitem(src, item_id, amount, src->version != CLIENT_VERSION_BB);
    if (item_data.data.datal[0] == 0 && item_data.data.data2l == 0) {
        ERR_LOG("%s ����ѵ���Ʒʧ��! ID 0x%08X ���� %u",
            get_player_describe(src), item_id, amount);
        return -1;
    }

    /* ���ݰ����, ��������Ϸ����. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_2A_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_drop_item_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if ((src->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("%s dropped item while shopping!",
            get_player_describe(src));
    }

    if (!check_pkt_size(src, pkt, sizeof(subcmd_drop_item_t), 0x06))
        return -2;

    /* Are we on Pioneer 2? If so, record the item they just dropped. */
    if (src->cur_area == 0) {
        if (src->p2_drops_max < 30) {
            src->p2_drops[src->p2_drops_max++] = LE32(pkt->item_id);
        }
        else {
            ERR_LOG("%s dropped too many items!"
                "This is possibly a bug in the server!", get_player_describe(src));
        }
    }

    /* Perhaps do more with this at some point when we do inventory tracking? */
    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_2A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_drop_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->item_id;
    uint16_t drop_amount = pkt->drop_amount, area = pkt->area;
    float x = pkt->x, y = pkt->y, z = pkt->z;
    int isframe = 0;

    if (!in_game(src)) {
        return -1;
    }

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if ((src->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("%s �������̵��е�����Ʒ!",
            get_player_describe(src));
    }

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_drop_item_t), 0x06)) {
        return -2;
    }

    inventory_t* inv = get_client_inv_bb(src);

    /* ����ұ����в�����Ʒ. */
    int index = find_iitem_index(inv, item_id);

    /* If the item isn't found, then punt the user from the ship. */
    if (index < 0) {
        ERR_LOG("%s �����˵���Ʒ ID 0x%04X �� ���ݰ� ID 0x%04X ����! ������ %d",
            get_player_describe(src), inv->iitems[index].data.item_id, item_id, index);
        return index;
    }

    iitem_t* drop_iitem = &inv->iitems[index];

    iitem_t iitem = remove_iitem(src, drop_iitem->data.item_id, drop_amount, src->version != CLIENT_VERSION_BB);
    if (iitem.data.datal[0] == 0 && iitem.data.data2l == 0) {
        ERR_LOG("%s ������Ʒʧ��!",
            get_player_describe(src));
        return -1;
    }

    /* We have the item... Add it to the lobby's inventory.
    �����������Ʒ��������ӵ������ı����� */
    if (!add_litem_locked(l, &iitem, (uint8_t)area, x, z)) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("�޷�����Ʒ������Ϸ���䱳��!");
        return -2;
    }

    /* ���ݰ����, ��������Ϸ����. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_2B_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_create_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    iitem_t item = { 0 };
    uint32_t v;
    int i;
    subcmd_create_item_t tr;

    if (!in_game(src))
        return -1;

    /* Buggy PSO version is buggy... */
    if (src->version == CLIENT_VERSION_DCV1 && pkt->shdr.size == 0x06)
        pkt->shdr.size = 0x07;

    if (!check_pkt_size(src, pkt, 0, 0x07))
        return -2;

    /* If we have multiple clients in the team, make sure that the client id in
       the packet matches the user sending the packet.
       Note: We don't do this in single player teams because NPCs do weird
       things if you change their equipment in quests. */
    if (l->num_clients != 1 && pkt->shdr.client_id != src->client_id)
        return -1;

    /* Outside of quests, we shouldn't be able to get these unless the shopping
       flag is set... Log any we get. */
    if (!(l->flags & LOBBY_FLAG_QUESTING) &&
        !(src->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("%s ������δ�����е������ȡ����Ʒ!", get_player_describe(src));
    }

    /* Run the bank action script, if any. */
    if (script_execute(ScriptActionBankAction, src, SCRIPT_ARG_PTR, src,
        SCRIPT_ARG_INT, 1, SCRIPT_ARG_UINT32, pkt->data.datal[0],
        SCRIPT_ARG_UINT32, pkt->data.datal[1], SCRIPT_ARG_UINT32,
        pkt->data.datal[2], SCRIPT_ARG_UINT32, pkt->data.data2l,
        SCRIPT_ARG_UINT32, pkt->data.item_id, SCRIPT_ARG_END) < 0) {
        return -1;
    }

    /* If we're in legit mode, we need to check the newly taken item. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) && l->limits_list) {
        switch (src->version) {
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

        default:
            return -1;
        }

        /* Fill in the item structure so we can check it. */
        memcpy(&item.data.datal[0], &pkt->data.datal[0], sizeof(uint32_t) * 5);

        if (!psocn_limits_check_item(l->limits_list, &item, v)) {
            DBG_LOG("%s Potentially non-legit item in legit mode:\n"
                "%08x %08x %08x %08x", get_player_describe(src), LE32(pkt->data.datal[0]),
                LE32(pkt->data.datal[1]), LE32(pkt->data.datal[2]),
                LE32(pkt->data.data2l));

            /* The item failed the check, so kick the user. */
            send_msg(src, MSG_BOX_TYPE, "%s\n\n%s\n%s",
                __(src, "\tE���ѱ��߳�������."),
                __(src, "ԭ��:"),
                __(src, "Attempt to remove a non-legit item from\n"
                    "the bank in a legit-mode game."));
            return -1;
        }
    }

    /* ������ǵ�������, either the game is not in legit mode, or the item is
       actually legit, so make a note of the ID, add it to the inventory and
       forward the packet on. */
    l->item_player_id[src->client_id] = (uint16_t)LE32(pkt->data.item_id);
    v = LE32(pkt->data.datal[0]);

    if (!(src->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    /* See if its a stackable item, since we have to treat them differently. */
    if (is_stackable(&pkt->data)) {
        /* Its stackable, so see if we have any in the inventory already */
        for (i = 0; i < src->item_count; ++i) {
            /* Found it, add what we're adding in */
            if (src->iitems[i].data.datal[0] == pkt->data.datal[0]) {
                src->iitems[i].data.datal[1] += pkt->data.datal[1];
                goto send_pkt;
            }
        }
    }

    memcpy(&src->iitems[src->item_count++].data.datal[0], &pkt->data.datal[0],
        sizeof(uint32_t) * 5);

send_pkt:
    /* If the item isn't a mag, or the client isn't Xbox or GC, then just
       send the packet away now. */
    if ((v & 0xff) != ITEM_TYPE_MAG ||
        (src->version != CLIENT_VERSION_XBOX && src->version != CLIENT_VERSION_GC)) {
        return subcmd_send_lobby_dc(src->cur_lobby, src, (subcmd_pkt_t*)pkt, 0);
    }
    else {
        /* If we have a mag and the user is on GC or Xbox, we have to swap the
           last dword when sending to the other of those two versions to make
           things work correctly in cross-play teams. */
        memcpy(&tr, pkt, sizeof(subcmd_create_item_t));
        tr.data.data2l = SWAP32(tr.data.data2l);

        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != src) {
                if (l->clients[i]->version == src->version) {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
                }
                else {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&tr);
                }
            }
        }

        return 0;
    }
}

static int sub60_2B_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_create_2B_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    iitem_t item = { 0 };
    uint32_t v;
    int i;
    subcmd_bb_create_2B_item_t tr;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_create_2B_item_t), 0x07))
        return -2;

    /* If we have multiple clients in the team, make sure that the client id in
       the packet matches the user sending the packet.
       Note: We don't do this in single player teams because NPCs do weird
       things if you change their equipment in quests. */
    if (l->num_clients != 1 && pkt->shdr.client_id != src->client_id)
        return -1;

    /* Outside of quests, we shouldn't be able to get these unless the shopping
       flag is set... Log any we get. */
    if (!(l->flags & LOBBY_FLAG_QUESTING) &&
        !(src->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("%s ������δ�����е������ȡ����Ʒ!", get_player_describe(src));
    }

    /* Run the bank action script, if any. */
    if (script_execute(ScriptActionBankAction, src, SCRIPT_ARG_PTR, src,
        SCRIPT_ARG_INT, 1, SCRIPT_ARG_UINT32, pkt->data.datal[0],
        SCRIPT_ARG_UINT32, pkt->data.datal[1], SCRIPT_ARG_UINT32,
        pkt->data.datal[2], SCRIPT_ARG_UINT32, pkt->data.data2l,
        SCRIPT_ARG_UINT32, pkt->data.item_id, SCRIPT_ARG_END) < 0) {
        return -1;
    }

    /* If we're in legit mode, we need to check the newly taken item. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) && l->limits_list) {
        switch (src->version) {
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

        case CLIENT_VERSION_BB:
            v = ITEM_VERSION_BB;
            break;

        default:
            return -1;
        }

        /* Fill in the item structure so we can check it. */
        memcpy(&item.data.datal[0], &pkt->data.datal[0], sizeof(uint32_t) * 5);

        if (!psocn_limits_check_item(l->limits_list, &item, v)) {
            DBG_LOG("%s Potentially non-legit item in legit mode:\n"
                "%08x %08x %08x %08x", get_player_describe(src), LE32(pkt->data.datal[0]),
                LE32(pkt->data.datal[1]), LE32(pkt->data.datal[2]),
                LE32(pkt->data.data2l));

            /* The item failed the check, so kick the user. */
            send_msg(src, MSG_BOX_TYPE, "%s\n\n%s\n%s",
                __(src, "\tE���ѱ��߳�������."),
                __(src, "ԭ��:"),
                __(src, "Attempt to remove a non-legit item from\n"
                    "the bank in a legit-mode game."));
            return -1;
        }
    }

    /* ������ǵ�������, either the game is not in legit mode, or the item is
       actually legit, so make a note of the ID, add it to the inventory and
       forward the packet on. */
    l->item_player_id[src->client_id] = (uint16_t)LE32(pkt->data.item_id);
    v = LE32(pkt->data.datal[0]);

    if (!(src->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    inventory_t* inv = get_client_inv_bb(src);

    /* See if its a stackable item, since we have to treat them differently. */
    if (is_stackable(&pkt->data)) {
        /* Its stackable, so see if we have any in the inventory already */
        for (i = 0; i < src->item_count; ++i) {
            /* Found it, add what we're adding in */
            if (inv->iitems[i].data.datal[0] == pkt->data.datal[0]) {
                inv->iitems[i].data.datal[1] += pkt->data.datal[1];
                goto send_pkt;
            }
        }
    }

    memcpy(&inv->iitems[inv->item_count++].data.datal[0], &pkt->data.datal[0],
        sizeof(uint32_t) * 5);

send_pkt:
    /* If the item isn't a mag, or the client isn't Xbox or GC, then just
       send the packet away now. */
    if ((v & 0xff) != ITEM_TYPE_MAG ||
        (src->version != CLIENT_VERSION_XBOX && src->version != CLIENT_VERSION_GC)) {
        return subcmd_send_lobby_bb(src->cur_lobby, src, (subcmd_bb_pkt_t*)pkt, 0);
    }
    else {
        /* If we have a mag and the user is on GC or Xbox, we have to swap the
           last dword when sending to the other of those two versions to make
           things work correctly in cross-play teams. */
        memcpy(&tr, pkt, sizeof(subcmd_create_item_t));
        tr.data.data2l = SWAP32(tr.data.data2l);

        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != src) {
                if (l->clients[i]->version == src->version) {
                    send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)pkt);
                }
                else {
                    send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)&tr);
                }
            }
        }

        return 0;
    }
}

static int sub60_2C_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_select_menu_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, 0, 0x05))
        return -2;

    /* Clear the list of dropped items. */
    if (pkt->unk == 0xFFFF && src->cur_area == 0) {
        memset(src->p2_drops, 0, sizeof(src->p2_drops));
        src->p2_drops_max = 0;
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_2C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_select_menu_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!check_pkt_size(src, pkt, 0, 0x05))
        return -2;

    /* Clear the list of dropped items. */
    if (pkt->unk == 0xFFFF && src->cur_area == 0) {
        memset(src->p2_drops, 0, sizeof(src->p2_drops));
        src->p2_drops_max = 0;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_2D_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_select_done_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, 0, 0x01))
        return -2;

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_2D_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_select_done_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!check_pkt_size(src, pkt, 0, 0x01))
        return -2;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_2F_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_hit_by_enemy_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t type = LE16(pkt->hdr.pkt_type);

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, 0, 0x03))
        return -2;

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, src, SUBCMD60_STAT_HPUP, 2000);
}

static int sub60_30_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_level_up_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, 0, 0x05))
        return -2;

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.client_id != src->client_id) {
        return -3;
    }

    /* Copy over the new data to the client's character structure... */
    src->pl->v1.character.disp.stats.atp = pkt->atp;
    src->pl->v1.character.disp.stats.mst = pkt->mst;
    src->pl->v1.character.disp.stats.evp = pkt->evp;
    src->pl->v1.character.disp.stats.hp = pkt->hp;
    src->pl->v1.character.disp.stats.dfp = pkt->dfp;
    src->pl->v1.character.disp.stats.ata = pkt->ata;
    src->pl->v1.character.disp.level = pkt->level;

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_31_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_use_medical_center_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;
//[2023��09��27�� 19:51:08:741] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x31
//
//[2023��09��27�� 19:51:08:749] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 08 00 31 01 01 00     `...1...
    //UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_31_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_use_medical_center_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    //UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_32_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_use_medical_center_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;
//[2023��09��27�� 19:51:08:767] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x32
//
//[2023��09��27�� 19:51:08:771] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 08 00 32 01 01 00     `...2...
    //UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_32_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_use_medical_center_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    //UNK_CSPD(pkt->type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_33_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_revive_player_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_revive_player_t), 0x02))
        return -2;

//[2023��08��31�� 22:00:13:769] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0x33 �汾 bb(5) �Ĵ���
//[2023��08��31�� 22:00:13:777] ����(subcmd_handle_60.c 7491): δ֪ 0x60 ָ��: 0x33
//( 00000000 )   10 00 60 00 00 00 00 00   33 02 00 00 01 00 00 00  ..`.....3.......
//[2023��08��31�� 22:01:07:912] ����(0480): �ͻ��� Sancaros(10000000) �Ͽ����� Blue Brust
//[2023��08��31�� 22:04:22:443] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0x33 �汾 bb(5) �Ĵ���
//[2023��08��31�� 22:04:22:452] ����(subcmd_handle_60.c 7491): δ֪ 0x60 ָ��: 0x33
//( 00000000 )   10 00 60 00 00 00 00 00   33 02 00 00 01 00 00 00  ..`.....3.......
//[2023��08��31�� 22:04:47:087] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0x33 �汾 bb(5) �Ĵ���
//[2023��08��31�� 22:04:47:096] ����(subcmd_handle_60.c 7491): δ֪ 0x60 ָ��: 0x33
//( 00000000 )   10 00 60 00 00 00 00 00   33 02 00 00 01 00 00 00  ..`.....3.......
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_37_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_photon_blast_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_photon_blast_t), 0x02))
        return -2;

    //[2023��07��15�� 20:40 : 25 : 447] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x37 �汾 5 �Ĵ���
    //[2023��07��15�� 20:40 : 25 : 463] ����(subcmd_handle_60.c 3591) : δ֪ 0x60 ָ�� : 0x37
    //(00000000)   10 00 60 00 00 00 00 00   37 02 00 00 64 00 00 00  ..`.....7...d...
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_39_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_photon_blast_ready_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_photon_blast_ready_t), 0x01))
        return -2;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_3A_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_game_client_leave_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:50:56:667] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x3A
//
//[2023��09��27�� 19:50:56:672] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 08 00 3A 01 01 00     `...:...
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_game_client_leave_t), 0x01))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("%s �Ƿ���NPC������Ʒ��", get_player_describe(src));
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    /* ����һ������֪ͨ������� ������뿪����Ϸ  TODO*/
    //print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_3A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_game_client_leave_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_game_client_leave_t), 0x01))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("%s �Ƿ���NPC������Ʒ��", get_player_describe(src));
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* ����һ������֪ͨ������� ������뿪����Ϸ  TODO*/
    //print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_3B_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_GAME) {
        //if (!l->challenge && !l->battle && !src->mode) {
        //    subcmd_send_bb_set_exp_rate(src, ship->cfg->globla_exp_mult);
        //    src->need_save_data = 1;
        //}
        //lobby_print_info2(src);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_3B_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (l->type == LOBBY_TYPE_GAME) {
        if (!l->challenge && !l->battle && !src->mode) {
            subcmd_send_bb_set_exp_rate(src, ship->cfg->globla_exp_mult);
            src->need_save_data = 1;
        }
        lobby_print_info2(src);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_3E_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_set_pos_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->w = pkt->w;
        src->x = pkt->x;
        src->y = pkt->y;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(src, l);
    }

    /* Clear this, in case we're at the lobby counter */
    src->last_info_req = 0;

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_3E_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_set_pos_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t area = pkt->area;
    float w = pkt->w, x = pkt->x, y = pkt->y, z = pkt->z;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->w = w;
        src->x = x;
        src->y = y;
        src->z = z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
    }

    /* Clear this, in case we're at the lobby counter */
    src->last_info_req = 0;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_3F_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_set_pos_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->w = pkt->w;
        src->x = pkt->x;
        src->y = pkt->y;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(src, l);
    }

    /* Clear this, in case we're at the lobby counter */
    src->last_info_req = 0;

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_3F_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_set_pos_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t area = pkt->area;
    float w = pkt->w, x = pkt->x, y = pkt->y, z = pkt->z;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->w = w;
        src->x = x;
        src->y = y;
        src->z = z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);
    }

    /* Clear this, in case we're at the lobby counter */
    src->last_info_req = 0;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_40_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_move_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->x = pkt->x;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(src, l);

        if (src->game_data->death) {
            src->game_data->death = 0;
        }
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_40_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_move_t* pkt) {
    lobby_t* l = src->cur_lobby;
    float x = pkt->x, z = pkt->z;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->x = x;
        src->z = z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);

        if (src->game_data->death) {
            src->game_data->death = 0;
        }
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_42_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_move_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->x = pkt->x;
        src->z = pkt->z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_qpos(src, l);

        if (src->game_data->death) {
            src->game_data->death = 0;
        }
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_42_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_move_t* pkt) {
    lobby_t* l = src->cur_lobby;
    float x = pkt->x, z = pkt->z;

    /* Save the new position and move along */
    if (src->client_id == pkt->shdr.client_id) {
        src->x = x;
        src->z = z;

        if ((l->flags & LOBBY_FLAG_QUESTING))
            update_bb_qpos(src, l);

        if (src->game_data->death) {
            src->game_data->death = 0;
        }
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_43_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_normal_attack_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:49:39:501] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x43
//
//[2023��09��27�� 19:49:39:507] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 43 02 01 00  C7 CB 00 00    `...C.......
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_normal_attack_t), 0x02))
        return -2;

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_43_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_normal_attack_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_normal_attack_t), 0x02))
        return -2;

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_44_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_normal_attack_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:49:46:183] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x44
//
//[2023��09��27�� 19:49:46:191] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 44 02 01 00  B6 4A 00 00    `...D....J..
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_normal_attack_t), 0x02))
        return -2;

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_44_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_normal_attack_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_normal_attack_t), 0x02))
        return -2;

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_45_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_normal_attack_t* pkt) {
    lobby_t* l = src->cur_lobby;
    
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_normal_attack_t), 0x02))
        return -2;

    /* Save the new area and move along */
    if (src->client_id == pkt->shdr.client_id) {
        if ((l->flags & LOBBY_TYPE_GAME))
            update_bb_qpos(src, l);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_46_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_objhit_phys_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t i;

    /* ������Ҫ��ͼ��ս������սģʽ�д�����Щ����. */
    if (l->challenge || l->battle) {
        ERR_LOG("%s hit object in mode!",
            get_player_describe(src));
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    if (!in_game(src))
        return -1;

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (LE16(pkt->hdr.pkt_len) != (sizeof(pkt->hdr) + (pkt->shdr.size << 2)) || pkt->shdr.size < 0x02) {
        ERR_LOG("%s sent bad objhit message!",
            get_player_describe(src));
        print_ascii_hex(errl, (unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    /* Handle each thing that was hit */
    for (i = 0; i < pkt->shdr.size - 2; ++i) {
        handle_objhit_common(src, l, LE16(pkt->objects[i].obj_id));
    }

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_46_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_objhit_phys_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t pkt_size = pkt->hdr.pkt_len;
    uint8_t size = pkt->shdr.size;
    uint32_t hit_count = pkt->hit_count;
    //uint8_t i;

    if (!in_game(src))
        return -1;

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt_size != (sizeof(bb_pkt_hdr_t) + (size << 2)) || size < 0x02) {
        ERR_LOG("%s �����𻵵���ͨ��������! %d %d hit_count %d",
            get_player_describe(src), pkt_size, (sizeof(bb_pkt_hdr_t) + (size << 2)), hit_count);
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        return -1;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (pkt->hdr.pkt_len == LE16(0x0010) || !l->map_objs || !l->map_enemies) {
        /* ����Ч��Ʒ���й��� */
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    ///* Handle each thing that was hit */
    //for (i = 0; i < pkt->shdr.size - 2; ++i)
    //    handle_bb_objhit_common(src, l, LE16(pkt->objects[i].obj_id));

    size_t allowed_count = min(pkt->shdr.size - 2, 11);
    if (hit_count > allowed_count) {
        ERR_LOG("%s �����𻵵���ͨ��������! %d %d hit_count %d",
            get_player_describe(src), pkt_size, (sizeof(bb_pkt_hdr_t) + (size << 2)), hit_count);
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_47_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_objhit_tech_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint8_t tech_level;

    /* ������Ҫ��ͼ��ս������սģʽ�д�����Щ����. */
    if (l->challenge || l->battle) {
        ERR_LOG("%s hit object in mode!",
            get_player_describe(src));
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    if (!in_game(src))
        return -1;

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (LE16(pkt->hdr.pkt_len) != (4 + (pkt->shdr.size << 2)) || pkt->shdr.size < 0x02) {
        ERR_LOG("%s sent bad objhit message!",
            get_player_describe(src));
        print_ascii_hex(errl, (unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    /* �����Լ��... Does the character have that level of technique? */
    tech_level = src->pl->v1.character.tech.all[pkt->tech];
    if (tech_level == 0xFF) {
        /* This might happen if the user learns a new tech in a team. Until we
           have real inventory tracking, we'll have to fudge this. Once we have
           real, full inventory tracking, this condition should probably
           disconnect the client */
        tech_level = pkt->level;
    }

    if (src->version >= CLIENT_VERSION_DCV2)
        tech_level += src->pl->v1.character.inv.iitems[pkt->tech].extension_data1;

    if (tech_level < pkt->level) {
        /* This might happen if the user learns a new tech in a team. Until we
           have real inventory tracking, we'll have to fudge this. Once we have
           real, full inventory tracking, this condition should probably
           disconnect the client */
        tech_level = pkt->level;
    }

    /* Check the type of the object that was hit. If there's no object data
       loaded here, we pretty much have to bail now. */
    if (!l->map_objs || !l->map_enemies)
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    /* See what technique was used... */
    switch (pkt->tech) {
        /* These work just like physical hits and can only hit one target, so
           handle them the simple way... */
    case TECHNIQUE_FOIE:
    case TECHNIQUE_ZONDE:
    case TECHNIQUE_GRANTS:
        if (pkt->shdr.size == 3)
            handle_objhit_common(src, l, LE16(pkt->objects[0].obj_id));
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

    default:
        ERR_LOG("%s used bad technique: %d",
            get_player_describe(src), (int)pkt->tech);
        return -1;
    }

    /* We're not doing any more interesting parsing with this one for now, so
       just send it along. */
    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_47_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_objhit_tech_t* pkt) {
    lobby_t* l = src->cur_lobby;

    pthread_mutex_lock(&src->mutex);
    if (!in_game(src)) {
        pthread_mutex_unlock(&src->mutex);
        return -1;
    }

    if (char_class_is_android(src->equip_flags) ||
        pkt->technique_number >= MAX_PLAYER_TECHNIQUES ||
        max_tech_level[pkt->technique_number].max_lvl[src->pl->bb.character.dress_data.ch_class] == 0
        ) {
        ERR_LOG("%s ְҵ %s �����𻵵� %s ������������!",
            get_player_describe(src), pso_class[src->pl->bb.character.dress_data.ch_class].cn_name,
            max_tech_level[pkt->technique_number].tech_name);
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        pthread_mutex_unlock(&src->mutex);
        return -1;
    }

    if (pkt->shdr.client_id != src->client_id) {
#ifdef DEBUG
        DBG_LOG("�Ƿ���NPC������Ʒ��");
#endif // DEBUG
        pthread_mutex_unlock(&src->mutex);
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    size_t allowed_count = min(pkt->shdr.size - 2, 10);

    if (pkt->target_count > allowed_count) {
        ERR_LOG("%s ְҵ %s �����𻵵� %s ������������!",
            get_player_describe(src), pso_class[src->pl->bb.character.dress_data.ch_class].cn_name,
            max_tech_level[pkt->technique_number].tech_name);
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        pthread_mutex_unlock(&src->mutex);
        return -1;
    }

    check_aoe_timer(src, pkt);
    subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    pthread_mutex_unlock(&src->mutex);
    return 0;
}

static int sub60_48_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_used_tech_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INFINITE_TP)) {
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, src, SUBCMD60_STAT_TPUP, 255);
}

static int sub60_48_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_used_tech_t* pkt) {
    lobby_t* l = src->cur_lobby;

    pthread_mutex_lock(&src->mutex);
    if (!in_game(src)) {
        pthread_mutex_unlock(&src->mutex);
        return -1;
    }

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_used_tech_t), 0x02)) {
        pthread_mutex_unlock(&src->mutex);
        return -2;
    }

    if (src->equip_flags & EQUIP_FLAGS_DROID ||
        pkt->technique_number >= MAX_PLAYER_TECHNIQUES ||
        max_tech_level[pkt->technique_number].max_lvl[src->pl->bb.character.dress_data.ch_class] == 0
        ) {
        ERR_LOG("%s �ͷ���Υ��ķ���!",
            get_player_describe(src));
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        pthread_mutex_unlock(&src->mutex);
        return -3;
    }
    
    if (pkt->shdr.client_id != src->client_id) {
#ifdef DEBUG
        DBG_LOG("�Ƿ���NPC������Ʒ��");
#endif // DEBUG
        pthread_mutex_unlock(&src->mutex);
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INFINITE_TP)) {
        pthread_mutex_unlock(&src->mutex);
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    send_lobby_mod_stat(l, src, SUBCMD60_STAT_TPUP, 255);
    pthread_mutex_unlock(&src->mutex);
    return 0;
}

static int sub60_49_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_subtract_PB_energy_6x49_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t entry_count = pkt->entry_count;
    uint8_t size = pkt->shdr.size;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_subtract_PB_energy_6x49_t), 0x03)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    size_t allowed_count = min(size - 3, 14);

    if (entry_count > allowed_count) {
        ERR_LOG("��Ч subtract PB energy ָ��");
    }

    //[2023��07��15�� 20:40:23:414] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x49 �汾 5 �Ĵ���
    //[2023��07��15�� 20:40:23:426] ����(subcmd_handle_60.c 3591): δ֪ 0x60 ָ��: 0x49
    //( 00000000 )   14 00 60 00 00 00 00 00   49 03 00 00 00 00 00 00  ..`.....I.......
    //( 00000010 )   64 00 00 00                                     d...
        /* This aught to do it... */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_4A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_defense_damage_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_defense_damage_t), 0x01))
        return -2;

    /* This aught to do it... */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_4B_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_take_damage_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, src, SUBCMD60_STAT_HPUP, 2000);
}

static int sub60_4B_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_take_damage_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (pkt->shdr.client_id != src->client_id) {
#ifdef DEBUG
        DBG_LOG("�Ƿ���NPC������Ʒ��");
#endif // DEBUG
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, src, SUBCMD60_STAT_HPUP, 2000);
}

static int sub60_4C_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_take_damage_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, src, SUBCMD60_STAT_HPUP, 2000);
}

static int sub60_4C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_take_damage_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;
    
    if (pkt->shdr.client_id != src->client_id) {
#ifdef DEBUG
        DBG_LOG("�Ƿ���NPC������Ʒ��");
#endif // DEBUG
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* If we're in legit mode or the flag isn't set, then don't do anything. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) ||
        !(src->flags & CLIENT_FLAG_INVULNERABLE)) {
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* This aught to do it... */
    subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    return send_lobby_mod_stat(l, src, SUBCMD60_STAT_HPUP, 2000);
}

static int sub60_4D_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_death_sync_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_death_sync_t), 0x02))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

//[2023��09��27�� 19:50:56:997] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x4D
//
//[2023��09��27�� 19:50:57:005] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 4D 02 01 00  03 00 00 00    `...M.......

    src->game_data->death = 1;

//    inventory_t* inv = get_client_inv_bb(src);
//
//#ifdef DEBUG
//
//    print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
//
//    for (size_t x = 0; x < inv->item_count; x++) {
//        print_iitem_data(&inv->iitems[x], x, src->version);
//    }
//
//#endif // DEBUG
//
//    int mag_index = find_equipped_mag(inv);
//    /* û���ҵ�MAG ֱ�ӷ��ͳ�ȥ */
//    if (mag_index) {
//        item_t* mag = &inv->iitems[mag_index].data;
//        mag->data2b[0] = max((mag->data2b[0] - pkt->flag), 0);
//    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_4D_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_death_sync_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_death_sync_t), 0x02))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

//[2023��08��24�� 19:10:08:279] ����(ship_packets.c 8302): GC 10000000 �������� quest118 (0 31)�汾 Blue Brust
//( 00000000 )   10 00 60 00 00 00 00 00   4D 02 00 00 03 00 00 00  ..`.....M.......

    src->game_data->death = 1;

    inventory_t* inv = get_client_inv_bb(src);

#ifdef DEBUG

    print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

    for (size_t x = 0; x < inv->item_count; x++) {
        print_iitem_data(&inv->iitems[x], x, src->version);
    }

#endif // DEBUG

    int mag_index = find_equipped_mag(inv);
    /* û���ҵ�MAG ֱ�ӷ��ͳ�ȥ */
    if (mag_index) {
        item_t* mag = &inv->iitems[mag_index].data;
        mag->data2b[0] = max((mag->data2b[0] - pkt->flag), 0);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_4E_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_cmd_4e_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:50:59:102] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x4E
//
//[2023��09��27�� 19:50:59:109] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 08 00 4E 01 01 00     `...N...
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_cmd_4e_t), 0x01))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_4E_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_cmd_4e_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_cmd_4e_t), 0x01))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_4F_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_player_saved_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    //if (pkt->shdr.client_id != src->client_id) {
    //    ERR_LOG("���� 0x60 ָ��: 0x%02X", pkt->hdr.pkt_type);
    //    UNK_CSPD(pkt->hdr.pkt_type, src->version, (uint8_t*)pkt);
    //    return -1;
    //}

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_50_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_switch_req_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:49:58:665] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x50
//
//[2023��09��27�� 19:49:58:673] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 50 02 01 00  DA 0C 00 00    `...P.......
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_switch_req_t), 0x02))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_50_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_switch_req_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_switch_req_t), 0x02))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_52_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We don't care about these in lobbies. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    /* Clear the list of dropped items. */
    if (src->cur_area == 0) {
        memset(src->p2_drops, 0, sizeof(src->p2_drops));
        src->p2_drops_max = 0;
    }

    /* Flip the shopping flag, since this packet is sent both for talking to the
       shop in the first place and when the client exits the shop. */
    src->flags ^= CLIENT_FLAG_SHOPPING;
    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_52_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_menu_req_t* pkt) {
    lobby_t* l = src->cur_lobby;

    /* We don't care about these in lobbies. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /* Clear the list of dropped items. */
    if (src->cur_area == 0) {
        memset(src->p2_drops, 0, sizeof(src->p2_drops));
        src->p2_drops_max = 0;
    }

    /* Flip the shopping flag, since this packet is sent both for talking to the
       shop in the first place and when the client exits the shop
       ��ת�����־����Ϊ�����ݰ��ڵ�һʱ��Ϳͻ��뿪�̵�ʱ���ᷢ�͸��̵�. */
    src->flags ^= CLIENT_FLAG_SHOPPING;
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_53_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x53_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Unknown_6x53_t), 0x01))
        return -2;

    //[2023��07��06�� 13:23:16:910] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x53 �汾 5 �Ĵ���
    //[2023��07��06�� 13:23:16:927] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x53
    //( 00000000 )   0C 00 60 00 00 00 00 00   53 01 00 00             ..`.....S...
    //[2023��07��06�� 13:23:25:808] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x53 �汾 5 �Ĵ���
    //[2023��07��06�� 13:23:25:832] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x53
    //( 00000000 )   0C 00 60 00 00 00 00 00   53 01 00 00             ..`.....S...
    //[2023��07��06�� 13:23:34:356] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x53 �汾 5 �Ĵ���
    //[2023��07��06�� 13:23:34:374] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x53
    //( 00000000 )   0C 00 60 00 00 00 00 00   53 01 00 00             ..`.....S...
    //[2023��07��06�� 13:23:40:484] ����(iitems.c 0428): δ�ӱ���������װ�������
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_55_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_map_warp_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_map_warp_t), 0x08))
        return -2;

//( 00000000 )   60 00 24 00 55 08 00 00   00 80 00 00 BA 7B 89 40  `.$.U....�..�{�@
//( 00000010 )   FF FF 7F 3F C5 89 4E C2   04 00 A0 41 00 00 00 00  ?ŉN?.�A....
//[2023��08��29�� 15:12:45:598] ����(subcmd_handle_60.c 3265): area = 0x8000
//[2023��08��29�� 15:12:45:608] ����(subcmd_handle_60.c 3269): area = 0x8000
//( 00000000 )   60 00 24 00 55 08 00 00   00 80 00 00 BA 7B 89 40  `.$.U....�..�{�@
//( 00000010 )   FF FF 7F 3F C5 89 4E C2   04 00 A0 41 00 00 00 00  ?ŉN?.�A....
//( 00000020 )   09 40 DD C4                                     .@��
//[2023��08��29�� 15:12:45:675] ����(subcmd_handle_60.c 3265): area = 0x8000
//[2023��08��29�� 15:12:45:683] ����(subcmd_handle_60.c 3269): area = 0x8000
//( 00000000 )   60 00 24 00 55 08 00 00   00 80 00 00 BA 7B 89 40  `.$.U....�..�{�@
//( 00000010 )   FF FF 7F 3F C5 89 4E C2   00 00 E0 36 00 00 00 00  ?ŉN?.?....
//[2023��08��29�� 15:12:45:753] ����(subcmd_handle_60.c 3265): area = 0x8000
//[2023��08��29�� 15:12:45:761] ����(subcmd_handle_60.c 3269): area = 0x8000
//( 00000000 )   60 00 24 00 55 08 00 00   00 80 00 00 BA 7B 89 40  `.$.U....�..�{�@
//( 00000010 )   FF FF 7F 3F C5 89 4E C2   00 00 E0 36 00 00 00 00  ?ŉN?.?....
//( 00000020 )   09 40 DD C4                                     .@��
//[2023��08��29�� 15:12:45:831] ����(subcmd_handle_60.c 3265): area = 0x8000
//[2023��08��29�� 15:12:45:841] ����(subcmd_handle_60.c 3269): area = 0x8000
    if (src->client_id == pkt->shdr.client_id) {

#ifdef DEBUG

        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

        switch (pkt->area)
        {
            /* �ܶ��� ʵ���� */
        case 0x8000:
            //l->govorlab = 1;
            DBG_LOG("�����ܶ�������ʶ��");
            break;

            /* EP1�ɴ� */
        case 0x0000:

            /* EP2�ɴ� */
        case 0xC000:

        default:
            //l->govorlab = 0;
            DBG_LOG("�뿪�ܶ�������ʶ��");
            break;
        }

#endif // DEBUG

    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_55_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_map_warp_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_map_warp_t), 0x08))
        return -2;

    if (src->client_id == pkt->shdr.client_id) {

        //DBG_LOG("area = 0x%04X", pkt->area);

#ifdef DEBUG

        switch (pkt->area)
        {
            /* �ܶ��� ʵ���� */
        case 0x8000:
            //l->govorlab = 1;
            DBG_LOG("�����ܶ�������ʶ��");
            break;

            /* EP1�ɴ� */
        case 0x0000:

            /* EP2�ɴ� */
        case 0xC000:

        default:
            //l->govorlab = 0;
            DBG_LOG("�뿪�ܶ�������ʶ��");
            break;
        }

#endif // DEBUG

    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_58_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_lobby_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_lobby_act_t), 0x02))
        return -2;

#ifdef DEBUG

    DBG_LOG("DC ����ID %d", pkt->act_id);

    // pkt->act_id ���������Ķ�ӦID

#endif // DEBUG

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_58_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_lobby_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_lobby_act_t), 0x02))
        return -2;

#ifdef DEBUG

    DBG_LOG("BB ����ID %d", pkt->act_id);

    // pkt->act_id ���������Ķ�ӦID

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_59_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pick_up_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:49:41:804] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x59
//
//[2023��09��27�� 19:49:41:811] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 10 00 59 03 01 00  01 00 01 00 73 01 01 06    `...Y.......s...
//[2023��09��27�� 19:49:52:191] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x59
//
//[2023��09��27�� 19:49:52:198] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 10 00 59 03 01 00  01 00 01 00 71 01 01 06    `...Y.......q...
//[2023��09��27�� 19:50:23:958] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x59
//
//[2023��09��27�� 19:50:23:964] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 10 00 59 03 01 00  01 00 01 00 7F 01 01 06    `...Y...........
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_pick_up_item_t), 0x03))
        return -2;

    ship_client_t* effective_c = l->clients[pkt->shdr.client_id];

    if (!effective_c) {
        return 0;
    }

    //if (l->flags & Lobby::Flag::ITEM_TRACKING_ENABLED) {
    //    auto item = l->remove_item(cmd.item_id);
    //    effective_c->game_data.player()->add_item(item);

    //    auto name = item.data.name(false);
    //    l->log.info("Player %hu picked up %08" PRIX32 " (%s)",
    //        cmd.header.client_id.load(), cmd.item_id.load(), name.c_str());
    //    if (c->options.debug) {
    //        string name = item.data.name(true);
    //        send_text_message_printf(c, "$C5PICK %08" PRIX32 "\n%s",
    //            cmd.item_id.load(), name.c_str());
    //    }
    //    effective_c->game_data.player()->print_inventory(stderr);
    //}

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_5A_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pick_up_item_req_t* pkt) {
    lobby_t* l = src->cur_lobby;

    ship_client_t* effective_c = l->clients[pkt->shdr.client_id];

    if (!effective_c) {
        return 0;
    }

    print_ascii_hex(dbgl, pkt, pkt->hdr.pkt_len);

    if (l->version == CLIENT_VERSION_BB) {
        if (!in_game(src))
            return -1;

        if (!check_pkt_size(src, pkt, sizeof(subcmd_pick_up_item_req_t), 0x03))
            return -2;

    //    if (!l->is_game() || (cmd.header.client_id != c->lobby_client_id)) {
    //        return;
    //    }

    //    if (!(l->flags & Lobby::Flag::ITEM_TRACKING_ENABLED)) {
    //        throw logic_error("item tracking not enabled in BB game");
    //    }

    //    auto item = l->remove_item(cmd.item_id);
    //    c->game_data.player()->add_item(item);

    //    auto name = item.data.name(false);
    //    l->log.info("Player %hu picked up %08" PRIX32 " (%s)",
    //        cmd.header.client_id.load(), cmd.item_id.load(), name.c_str());
    //    if (c->options.debug) {
    //        string name = item.data.name(true);
    //        send_text_message_printf(c, "$C5PICK/BB %08" PRIX32 "\n%s",
    //            cmd.item_id.load(), name.c_str());
    //    }
    //    c->game_data.player()->print_inventory(stderr);

    //    return send_pick_up_item(c, cmd.item_id, cmd.area);

    //}
    //else {
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_5D_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_drop_stack_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. Note that v1 is missing the stupid
       extra "two" field from the packet. */
    if (pkt->shdr.size != 0x0A && pkt->shdr.size != 0x09)
        return -1;

    /* If a shop menu is open, someone is probably doing something nefarious.
       Log it for now... */
    if ((src->flags & CLIENT_FLAG_SHOPPING)) {
        ERR_LOG("%s dropped stack while shopping!",
            get_player_describe(src));
    }

    /* Perhaps do more with this at some point when we do inventory tracking? */
    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_5E_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_buy_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t ic;
    int i;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_buy_t), 0x06))
        return -2;

    if (pkt->shdr.client_id != src->client_id)
        return -3;

    /* Make a note of the item ID, and add to the inventory */
    l->item_player_id[src->client_id] = LE32(pkt->data.item_id);

    if (!(src->flags & CLIENT_FLAG_TRACK_INVENTORY))
        goto send_pkt;

    ic = LE32(pkt->data.datal[0]);

    /* See if its a stackable item, since we have to treat them differently. */
    if (is_stackable(&pkt->data)) {
        /* Its stackable, so see if we have any in the inventory already */
        for (i = 0; i < src->item_count; ++i) {
            /* Found it, add what we're adding in */
            if (src->iitems[i].data.datal[0] == pkt->data.datal[0]) {
                src->iitems[i].data.datal[1] += pkt->data.datal[1];
                goto send_pkt;
            }
        }
    }

    memcpy(&src->iitems[src->item_count].data.datal[0], &pkt->data.datal[0],
        sizeof(uint32_t) * 4);
    src->iitems[src->item_count++].data.data2l = 0;

send_pkt:
    return subcmd_send_lobby_dc(src->cur_lobby, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_5F_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_itemgen_t* pkt) {
    lobby_t* l = src->cur_lobby;
    iitem_t item = { 0 };
    uint32_t v;
    int i;
    ship_client_t* c2;
    const char* name;
    subcmd_destroy_item_t dp;
    subcmd_itemgen_t tr;

    if (!in_game(src))
        return -1;

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. We accept two different sizes here
       0x0B for v2 and later, and 0x0A for v1. */
    if (pkt->shdr.size != 0x0B && pkt->shdr.size != 0x0A) {
        return -1;
    }

    /* If we're in legit mode, we need to check the item. */
    if ((l->flags & LOBBY_FLAG_LEGIT_MODE) && l->limits_list) {
        switch (src->version) {
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

        default:
            return -1;
        }

        /* Fill in the item structure so we can check it. */
        memcpy(&item.data.datal[0], &pkt->data.item.datal[0], 5 * sizeof(uint32_t));

        if (!psocn_limits_check_item(l->limits_list, &item, v)) {
            /* The item failed the check, deal with it. */
            DBG_LOG("Potentially non-legit item dropped in legit mode:\n"
                "%08x %08x %08x %08x %08x", LE32(pkt->data.item.datal[0]),
                LE32(pkt->data.item.datal[1]), LE32(pkt->data.item.datal[2]),
                LE32(pkt->data.item.data2l), LE32(pkt->data.item2));

            /* Grab the item name, if we can find it. */
            name = item_get_name(&item.data, v, 0);

            /* Fill in the destroy item packet. */
            memset(&dp, 0, sizeof(subcmd_destroy_item_t));
            dp.hdr.pkt_type = GAME_SUBCMD60_TYPE;
            dp.hdr.pkt_len = LE16(0x0010);
            dp.shdr.type = SUBCMD60_ITEM_GROUND_DESTROY;
            dp.shdr.size = 0x03;
            dp.item_id = pkt->data.item.item_id;

            /* Send out a warning message, followed by the drop, followed by a
               packet deleting the drop from the game (to prevent any desync) */
            for (i = 0; i < l->max_clients; ++i) {
                if ((c2 = l->clients[i])) {
                    if (name) {
                        send_txt(c2, "%s: %s",
                            __(c2, "\tE\tC7Potentially hacked drop\n"
                                "detected."), name);
                    }
                    else {
                        send_txt(c2, "%s",
                            __(c2, "\tE\tC7Potentially hacked drop\n"
                                "detected."));
                    }

                    /* Send out the drop item packet. This doesn't go to the
                       person who originated the drop (the team leader). */
                    if (src != c2) {
                        send_pkt_dc(c2, (dc_pkt_hdr_t*)pkt);
                    }

                    /* Send out the destroy drop packet. */
                    send_pkt_dc(c2, (dc_pkt_hdr_t*)&dp);
                }
            }

            /* We're done */
            return 0;
        }
    }

    /* If we end up here, then the item is legit... */
    v = LE32(pkt->data.item.datal[0]) & 0xff;

    /* If the item isn't a mag, or the client isn't Xbox or GC, then just
       send the packet away now. */
    if ((v & 0xff) != ITEM_TYPE_MAG ||
        (src->version != CLIENT_VERSION_XBOX && src->version != CLIENT_VERSION_GC)) {
        return subcmd_send_lobby_dc(src->cur_lobby, src, (subcmd_pkt_t*)pkt, 0);
    }
    else {
        /* If we have a mag and the user is on GC or Xbox, we have to swap the
           last dword when sending to the other of those two versions to make
           things work correctly in cross-play teams. */
        memcpy(&tr, pkt, sizeof(subcmd_itemgen_t));
        tr.data.item.data2l = SWAP32(tr.data.item.data2l);

        for (i = 0; i < l->max_clients; ++i) {
            if (l->clients[i] && l->clients[i] != src) {
                if (l->clients[i]->version == src->version) {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
                }
                else {
                    send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&tr);
                }
            }
        }

        return 0;
    }
}

static int sub60_61_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_levelup_req_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_levelup_req_t), 0xFF))
        return -2;

    //ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_63_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_destory_ground_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;
    iitem_t iitem_data = { 0 };
    int destory_count;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_destory_ground_item_t), 0x03))
        return -2;

    destory_count = remove_litem_locked(l, pkt->item_id, &iitem_data);

    if (destory_count < 0) {
        ERR_LOG("ɾ��������Ʒ����");
        return -1;
    }

    if (src->game_data->gm_debug)
        DBG_LOG("������Ʒ %08" PRIX32 " �ѱ��ݻ� (%s)",
            iitem_data.data.item_id, item_get_name(&iitem_data.data, src->version, 0));

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_66_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_use_star_atomizer_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_use_star_atomizer_t), 0x03))
        return -2;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_67_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_create_enemy_set_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:50:11:208] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x67
//
//[2023��09��27�� 19:50:11:214] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 14 00 67 04 8B 40  01 00 00 00 6F 00 00 00    `...g..@....o...
//(00000010) 01 00 00 00    ....
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_create_enemy_set_t), 0x04))
        return -2;

    /* ����֪ͨ������Ҵ�����������ʲô���� */

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_67_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_create_enemy_set_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_create_enemy_set_t), 0x04))
        return -2;

    /* ����֪ͨ������Ҵ�����������ʲô���� */

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_68_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pipe_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    /* See if the user is creating a pipe or destroying it. Destroying a pipe
       always matches the created pipe, but sets the area to 0. We could keep
       track of all of the pipe data, but that's probably overkill. For now,
       blindly accept any pipes where the area is 0. */
    if (pkt->area != 0) {
        /* Make sure the user is sending a pipe from the area he or she is
           currently in. */
        if (pkt->area != src->cur_area) {
            ERR_LOG("Attempt by %s to spawn pipe to area "
                "he/she is not in (in: %d, pipe: %d).", get_player_describe(src),
                src->cur_area, (int)pkt->area);
            return -1;
        }
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_68_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pipe_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_pipe_t), 0x07))
        return -2;

    /* See if the user is creating a pipe or destroying it. Destroying a pipe
       always matches the created pipe, but sets the area to 0. We could keep
       track of all of the pipe data, but that's probably overkill. For now,
       blindly accept any pipes where the area is 0
       �鿴�û��Ǵ����ܵ��������ٹܵ������ٹܵ������봴���Ĺܵ�ƥ�䣬
       ���Ὣ��������Ϊ0�����ǿ��Ը������йܵ����ݣ�
       �������̫�����ˡ�Ŀǰ��äĿ�������Ϊ0���κιܵ���. */
    if (pkt->area != 0) {
        /* Make sure the user is sending a pipe from the area he or she is
           currently in. */
        if (pkt->area != src->cur_area) {
            ERR_LOG("%s ���ԴӴ����Ŵ����������ڵ����� (��Ҳ㼶: %d, ���Ͳ㼶: %d).", get_player_describe(src),
                src->cur_area, (int)pkt->area);
            return -1;
        }
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_69_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    /* The only quests that allow NPCs to be loaded are those that require there
       to only be one player, so limit that here. Also, we only allow /npc in
       single-player teams, so that'll fall into line too. */
    if (l->num_clients > 1) {
        ERR_LOG("Attempt by %s to spawn NPC in multi-"
            "player team!", get_player_describe(src));
        return -1;
    }

    /* Either this is a legitimate request to spawn a quest NPC, or the player
       is doing something stupid like trying to NOL himself. We don't care if
       someone is stupid enough to NOL themselves, so send the packet on now. */
    return subcmd_send_lobby_dc(l, src, pkt, 0);
}

static int sub60_69_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    /* The only quests that allow NPCs to be loaded are those that require there
       to only be one player, so limit that here. Also, we only allow /npc in
       single-player teams, so that'll fall into line too. */
    if (l->num_clients > 1) {
        ERR_LOG("GC %s to spawn NPC in multi-"
            "player team!", get_player_describe(src));
        return -1;
    }

    /* Either this is a legitimate request to spawn a quest NPC, or the player
       is doing something stupid like trying to NOL himself. We don't care if
       someone is stupid enough to NOL themselves, so send the packet on now. */
    return subcmd_send_lobby_bb(l, src, pkt, 0);
}

static int sub60_6A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x6A_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Unknown_6x6A_t), 0x02))
        return -2;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_72_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_72_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_74_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_word_select_t* pkt) {
    /* Don't send the message if they have the protection flag on. */
    if (src->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(src, __(src, "\tE\tC7�������¼��ſ��Խ��в���."));
    }

    /* Don't send chats for a STFUed client. */
    if ((src->flags & CLIENT_FLAG_STFU)) {
        return 0;
    }

    switch (src->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
        return word_select_send_dc(src, pkt);

    case CLIENT_VERSION_PC:
        return word_select_send_pc(src, pkt);

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_BB:
    case CLIENT_VERSION_XBOX:
        return word_select_send_gc(src, pkt);
    }

    return 0;
}

static int sub60_74_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_word_select_t* pkt) {
    /* Don't send the message if they have the protection flag on. */
    if (src->flags & CLIENT_FLAG_GC_PROTECT) {
        return send_txt(src, __(src, "\tE\tC7�������¼��ſ��Խ��в���."));
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
        //print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

    return word_select_send_bb(src, pkt);
}

static int sub60_75_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_set_flag_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t flag_index = pkt->flag;
    uint16_t action = pkt->action;
    uint16_t difficulty = l->difficulty;
    int rv = 0;
//[2023��09��27�� 19:49:32:336] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x75
//
//[2023��09��27�� 19:49:32:342] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 75 02 09 8C  14 00 00 00    `...u.......
    if (!check_pkt_size(src, pkt, sizeof(subcmd_set_flag_t), 0x02))
        return -2;

    if (flag_index >= 0x400) {
        return 0;
    }

    DBG_LOG("%s ����SET_FLAGָ��! flag = 0x%02X action = 0x%02X episode = 0x%02X difficulty = 0x%02X",
        get_player_describe(src), flag_index, action, l->episode, difficulty);

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

    subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    if (src->version == CLIENT_VERSION_GC) {
        bool should_send_boss_drop_req = false;
        bool is_ep2 = (l->episode == GAME_TYPE_EPISODE_2);
        if ((l->episode == GAME_TYPE_EPISODE_1) && (src->cur_area == 0x0E)) {
            // On Normal, Dark Falz does not have a third phase, so send the drop
            // request after the end of the second phase. On all other difficulty
            // levels, send it after the third phase.
            if (((difficulty == 0) && (flag_index == 0x0035)) ||
                ((difficulty != 0) && (flag_index == 0x0037))) {
                should_send_boss_drop_req = true;
            }
        }
        else if (is_ep2 && (flag_index == 0x0057) && (src->cur_area == 0x0D)) {
            should_send_boss_drop_req = true;
        }

        if (should_send_boss_drop_req) {
            ship_client_t* c2 = l->clients[l->leader_id];
            if (c2) {
                subcmd_bitemreq_t req = { 0 };

                req.hdr.pkt_type = 0x62;
                req.hdr.pkt_len = sizeof(subcmd_bitemreq_t);
                req.hdr.flags = 0;
                req.shdr.type = 0x60;
                req.shdr.size = 0x06;
                req.shdr.object_id = 0x0000;

                req.area = (uint8_t)(src->cur_area);
                req.pt_index = (uint8_t)(is_ep2 ? 0x4E : 0x2F);
                req.request_id = 0x0B4F;
                req.x = is_ep2 ? -9999.0f : 10160.58984375f;
                req.z = 0.0f;
                req.unk2[0] = 2;
                req.unk2[1] = 0;
                //    req.
                //    {
                //        {0x60, 0x06, 0x0000},
                //        (uint8_t)(src->cur_area),
                //        (uint8_t)(is_ep2 ? 0x4E : 0x2F),
                //        0x0B4F,
                //        is_ep2 ? -9999.0f : 10160.58984375f,
                //        0.0f,
                //        2,
                //        0,
                //    },
                //    0xE0AEDC01,
                //};
                //send_command_t(c, 0x62, l->leader_id, req);
                return  l->dropfunc(c2, l, &req);
            }
        }
    }

    return 0;
}

static int sub60_75_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_set_flag_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t flag_index = pkt->flag;
    uint16_t action = pkt->action;
    uint16_t difficulty = pkt->difficulty;
    int rv = 0;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_set_flag_t), 0x03))
        return -2;

    if (flag_index >= 0x400) {
        return 0;
    }

#ifdef DEBUG

    DBG_LOG("%s ����SET_FLAGָ��! flag = 0x%02X action = 0x%02X episode = 0x%02X difficulty = 0x%02X",
        get_player_describe(src), flag_index, action, l->episode, difficulty);

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

static int sub60_76_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_killed_monster_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_killed_monster_t), 0x02)) {
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static inline int reg_sync_index(lobby_t* l, uint16_t regnum) {
    int i;

    if (!(l->q_flags & LOBBY_QFLAG_SYNC_REGS))
        return -1;

    for (i = 0; i < l->num_syncregs; ++i) {
        if (regnum == l->syncregs[i])
            return i;
    }

    return -1;
}

static int sub60_77_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_sync_reg_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t val = LE32(pkt->value);
    int done = 0, idx;
    uint32_t ctl;

    /* XXXX: Probably should do some checking here... */
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
    if ((idx = reg_sync_index(l, pkt->register_number)) != -1) {
        l->regvals[idx] = val;
    }

    if (!done)
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    return 0;
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

static int sub60_77_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_sync_reg_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t val = LE32(pkt->value);
    uint16_t register_number = pkt->register_number;
    int done = 0, idx;
    uint32_t ctl;

    /* We can't get these in lobbies without someone messing with something
   that they shouldn't be... Disconnect anyone that tries. */
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_sync_reg_t), 0x03)) {
        return -2;
    }

    /* TODO: Probably should do some checking here... */
    /* ���мĴ���ͬ���ű�����������ã�������ű����ط��㣬
    ��������Ѿ���ִ�����ͬ��. */
    if ((script_execute(ScriptActionQuestSyncRegister, src, SCRIPT_ARG_PTR, src,
        SCRIPT_ARG_PTR, l, SCRIPT_ARG_UINT8, register_number,
        SCRIPT_ARG_UINT32, val, SCRIPT_ARG_END))) {
        done = 1;
    }

    /* Does this quest use global flags? If so, then deal with them... */
    if ((l->q_flags & LOBBY_QFLAG_SHORT) && register_number == l->q_shortflag_reg &&
        !done) {
        /* Check the control bits for sensibility... */
        ctl = (val >> 29) & 0x07;

        /* Make sure the error or response bits aren't set. */
        if ((ctl & 0x06)) {
            DBG_LOG("����ʹ�÷Ƿ�ctlע���־Quest set flag register with illegal ctl!");
            send_sync_register(src, register_number, 0x8000FFFE);
        }
        /* Make sure we don't have anything with any reserved ctl bits set
           (unless a script has already handled the sync). */
        else if ((val & 0x17000000)) {
            DBG_LOG("����ʹ�üĴ���ctlע���־Quest set flag register with reserved ctl!");
            send_sync_register(src, register_number, 0x8000FFFE);
        }
        else if ((val & 0x08000000)) {
            /* Delete the flag... */
            DBG_LOG("ɾ������ע���־!");
            shipgate_send_qflag(&ship->sg, src, 1, ((val >> 16) & 0xFF),
                src->cur_lobby->qid, 0, QFLAG_DELETE_FLAG);
        }
        else {
            /* Send the request to the shipgate... */
            DBG_LOG("��������ע���־����բ! %d", ctl & 0x01);
            shipgate_send_qflag(&ship->sg, src, ctl & 0x01, (val >> 16) & 0xFF,
                src->cur_lobby->qid, val & 0xFFFF, 0);
        }

        done = 1;
    }

    /* Does this quest use server data calls? If so, deal with it... */
    if ((l->q_flags & LOBBY_QFLAG_DATA) && !done) {
        if (register_number == l->q_data_reg) {
            if (src->q_stack_top < CLIENT_MAX_QSTACK) {
                if (!(src->flags & CLIENT_FLAG_QSTACK_LOCK)) {
                    src->q_stack[src->q_stack_top++] = val;

                    /* Check if we've got everything we expected... */
                    if (src->q_stack_top >= 3 &&
                        src->q_stack_top == 3 + src->q_stack[1] + src->q_stack[2]) {
                        /* Call the function requested and reset the stack. */
                        ctl = quest_function_dispatch(src, l);

                        if (ctl != QUEST_FUNC_RET_NOT_YET) {
                            send_sync_register(src, register_number, ctl);
                            src->q_stack_top = 0;
                        }
                    }
                }
                else {
                    /* The stack is locked, ignore the write and report the
                       error. */
                    send_sync_register(src, register_number,
                        QUEST_FUNC_RET_STACK_LOCKED);
                }
            }
            else if (src->q_stack_top == CLIENT_MAX_QSTACK) {
                /* Eat the stack push and report an error. */
                send_sync_register(src, register_number,
                    QUEST_FUNC_RET_STACK_OVERFLOW);
            }

            done = 1;
        }
        else if (register_number == l->q_ctl_reg) {
            /* For now, the only reason we'll have one of these is to reset the
               stack. There might be other reasons later, but this will do, for
               the time being... */
            src->q_stack_top = 0;
            done = 1;
        }
    }

    /* Does this register have to be synced? */
    if ((idx = reg_sync_index_bb(l, register_number)) != -1) {
        l->regvals[idx] = val;
    }

    if (!done)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    return 0;
}

static int sub60_79_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_gogo_ball_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_gogo_ball_t), 0x06)) {
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_7A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x7A_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Unknown_6x7A_t), 0x01)) {
        return -2;
    }

#ifdef DEBUG

    //[2023��08��14�� 21:39 : 46 : 011] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x7A �汾 bb(5) �Ĵ���
    //[2023��08��14�� 21:39 : 46 : 021] ����(subcmd_handle_60.c 4842) : δ֪ 0x60 ָ�� : 0x7A
    //(00000000)   0C 00 60 00 00 00 00 00   7A 01 00 00             ..`.....z...
    //[2023��08��14�� 21:43 : 59 : 254] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x7A �汾 bb(5) �Ĵ���
    //[2023��08��14�� 21:43 : 59 : 265] ����(subcmd_handle_60.c 4842) : δ֪ 0x60 ָ�� : 0x7A
    //(00000000)   0C 00 60 00 00 00 00 00   7A 01 00 00             ..`.....z...
    //[2023��08��14�� 21:47 : 02 : 080] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x7A �汾 bb(5) �Ĵ���
    //[2023��08��14�� 21:47 : 02 : 089] ����(subcmd_handle_60.c 4842) : δ֪ 0x60 ָ�� : 0x7A
    //(00000000)   0C 00 60 00 00 00 00 00   7A 01 00 00             ..`.....z...
    //[2023��08��14�� 21:47 : 15 : 072] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x7A �汾 bb(5) �Ĵ���
    //[2023��08��14�� 21:47 : 15 : 082] ����(subcmd_handle_60.c 4842) : δ֪ 0x60 ָ�� : 0x7A
    //(00000000)   0C 00 60 00 00 00 00 00   7A 01 00 00             ..`.....z...

    DBG_LOG("ָ�� 0x%04X 0x%02X %s:%d ����ID %d ���� %d",
        pkt->hdr.pkt_type, pkt->shdr.type, get_player_describe(src), src->sec_data.slot, l->qid, src->cur_area);

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_7B_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x7B_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Unknown_6x7B_t), 0x01)) {
        return -2;
    }

#ifdef DEBUG

    //[2023��08��14�� 21:43 : 59 : 219] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x7B �汾 bb(5) �Ĵ���
    //[2023��08��14�� 21:43 : 59 : 229] ����(subcmd_handle_60.c 4842) : δ֪ 0x60 ָ�� : 0x7B
    //(00000000)   0C 00 60 00 00 00 00 00   7B 01 00 00             ..`.....{...
    //[2023��08��14�� 21:47 : 02 : 046] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x7B �汾 bb(5) �Ĵ���
    //[2023��08��14�� 21:47 : 02 : 055] ����(subcmd_handle_60.c 4842) : δ֪ 0x60 ָ�� : 0x7B
    //(00000000)   0C 00 60 00 00 00 00 00   7B 01 00 00             ..`.....{...
    //[2023��08��14�� 21:47 : 15 : 036] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x7B �汾 bb(5) �Ĵ���
    //[2023��08��14�� 21:47 : 15 : 048] ����(subcmd_handle_60.c 4842) : δ֪ 0x60 ָ�� : 0x7B
    //(00000000)   0C 00 60 00 00 00 00 00   7B 01 00 00             ..`.....{...
    //[2023��08��14�� 21:47 : 23 : 492] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x7B �汾 bb(5) �Ĵ���
    //[2023��08��14�� 21:47 : 23 : 503] ����(subcmd_handle_60.c 4842) : δ֪ 0x60 ָ�� : 0x7B
    //(00000000)   0C 00 60 00 00 00 00 00   7B 01 00 00             ..`.....{...

    //GC 42004146:2 ����ID 0 ���� 0

    DBG_LOG("ָ�� 0x%04X 0x%02X %s:%d ����ID %d ���� %d",
        pkt->hdr.pkt_type, pkt->shdr.type, get_player_describe(src), src->sec_data.slot, l->qid, src->cur_area);

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_7C_dc(ship_client_t* src, ship_client_t* dest, 
    subcmd_pkt_t* pkt) {
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
        pc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
        pc.hdr.pkt_len = LE16(0x00E4);

        pc.shdr.type = SUBCMD60_SET_C_GAME_MODE;
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
        dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
        dc.hdr.pkt_len = LE16(0x00AC);

        dc.shdr.type = SUBCMD60_SET_C_GAME_MODE;
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
        break;

    default:
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
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

int handle_bb_battle_mode(ship_client_t* src,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int ch, ch2;
    ship_client_t* lc = { 0 };

    if (!in_game(src))
        return -1;

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
                        memcpy(&lc->mode_pl->bb, lc->game_data->char_backup, sizeof(lc->mode_pl->bb));

                    if (lc->mode == 0x02) {
                        for (ch2 = 0; ch2 < lc->mode_pl->bb.inv.item_count; ch2++)
                        {
                            if (lc->mode_pl->bb.inv.iitems[ch2].data.datab[0] == 0x02)
                                lc->mode_pl->bb.inv.iitems[ch2].present = 0;
                        }
                        cleanup_bb_inv(lc->client_id, &lc->mode_pl->bb.inv);
                        lc->mode_pl->bb.disp.meseta = 0;
                    }
                    break;
                case 0x03:
                    // Wipe items and reset level.
                    for (ch2 = 0; ch2 < 30; ch2++)
                        lc->mode_pl->bb.inv.iitems[ch2].present = 0;
                    cleanup_bb_inv(lc->client_id, &lc->mode_pl->bb.inv);

                    uint8_t ch_class = lc->mode_pl->bb.dress_data.ch_class;

                    lc->mode_pl->bb.disp.level = 0;
                    lc->mode_pl->bb.disp.exp = 0;

                    lc->mode_pl->bb.disp.stats.atp = bb_char_stats.start_stats[ch_class].atp;
                    lc->mode_pl->bb.disp.stats.mst = bb_char_stats.start_stats[ch_class].mst;
                    lc->mode_pl->bb.disp.stats.evp = bb_char_stats.start_stats[ch_class].evp;
                    lc->mode_pl->bb.disp.stats.hp = bb_char_stats.start_stats[ch_class].hp;
                    lc->mode_pl->bb.disp.stats.dfp = bb_char_stats.start_stats[ch_class].dfp;
                    lc->mode_pl->bb.disp.stats.ata = bb_char_stats.start_stats[ch_class].ata;

                    /*if (l->battle_level > 1)
                        SkipToLevel(l->battle_level - 1, lc, 1);*/
                    lc->mode_pl->bb.disp.meseta = 0;
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
        pc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
        pc.hdr.pkt_len = LE16(0x00E4);

        pc.shdr.type = SUBCMD60_SET_C_GAME_MODE;
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
        dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
        dc.hdr.pkt_len = LE16(0x00AC);

        dc.shdr.type = SUBCMD60_SET_C_GAME_MODE;
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
        //print_ascii_hex(errl, (unsigned char*)&bb, LE16(pkt->hdr.pkt_len));
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

static int sub60_7C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (src->mode) {
        DBG_LOG("%s ������Ϸָ�� sub60_7C_bb !", get_player_describe(src));
    }

    if (l->battle)
        handle_bb_battle_mode(src, pkt);
    else if (l->challenge)
        handle_bb_challenge_mode_grave(src, pkt);

    return 0;
}

static int sub60_7D_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_sync_battle_mode_data_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_sync_battle_mode_data_t), 0x06)) {
        return -2;
    }

    send_msg(src, TEXT_MSG_TYPE, "%02X\n%02X\n%02X\n%02X\n%08X\n%08X\n%08X\n%08X"
        , pkt->unknown_a1
        , pkt->unused[0]
        , pkt->unused[1]
        , pkt->unused[2]
        , pkt->target_client_id
        , pkt->unknown_a2
        , pkt->unknown_a3[0]
        , pkt->unknown_a3[1]
    );

    //DBG_LOG("%s", get_player_describe(src));

    //print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

//[2023��07��12�� 20:08:18:088] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x7D �汾 5 �Ĵ���
//[2023��07��12�� 20:08:18:091] ����(subcmd_handle_60.c 3493): δ֪ 0x60 ָ��: 0x7D
//( 00000000 )   20 00 60 00 00 00 00 00   7D 06 00 00 04 00 FF FF   .`.....}.....
//( 00000010 )   01 00 00 00 FF FF FF FF   FF FF FF FF FF FF FF FF  ....
//[2023��07��12�� 20:08:18:113] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x7D �汾 5 �Ĵ���
//[2023��07��12�� 20:08:18:115] ����(subcmd_handle_60.c 3493): δ֪ 0x60 ָ��: 0x7D
//( 00000000 )   20 00 60 00 00 00 00 00   7D 06 00 00 04 00 FF FF   .`.....}.....
//( 00000010 )   00 00 00 00 FF FF FF FF   FF FF FF FF FF FF FF FF  ....
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_80_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_trigger_trap_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_trigger_trap_t), 0x02)) {
        return -2;
    }

    //[2023��07��06�� 13:21:25:848] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x80 �汾 5 �Ĵ���
    //[2023��07��06�� 13:21:25:865] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x80
    //( 00000000 )   10 00 60 00 00 00 00 00   80 02 FC 47 50 00 02 00  ..`.....�.�GP...

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_83_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_place_trap_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_place_trap_t), 0x02)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    //[2023��07��06�� 13:21:22:702] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x83 �汾 5 �Ĵ���
    //[2023��07��06�� 13:21:22:717] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x83
    //( 00000000 )   10 00 60 00 00 00 00 00   83 02 00 00 03 00 14 00  ..`.....?......

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_84_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_boss_act_ofp_u_sp_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_boss_act_ofp_u_sp_t), 0x04)) {
        return -2;
    }

#ifdef DEBUG
    //[2023��07��23�� 12:36 : 00 : 544] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:36 : 00 : 560] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 05 03 01 04  ..`..... ? ......
//(00000010)   00 02 02 00 00 00 C8 02                           ...... ?
//[2023��07��23�� 12:36 : 15 : 695] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:36 : 15 : 711] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 03 04 00 02  ..`..... ? ......
//(00000010)   05 01 03 00 02 00 C8 02                           ...... ?
//[2023��07��23�� 12:36 : 33 : 085] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:36 : 33 : 100] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 01 05 02 04  ..`..... ? ......
//(00000010)   00 03 04 00 02 00 C8 02                           ...... ?
//[2023��07��23�� 12:36 : 53 : 985] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:36 : 53 : 998] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 01 05 04 00  ..`..... ? ......
//(00000010)   03 02 05 00 01 00 C8 02                           ...... ?
//[2023��07��23�� 12:37 : 12 : 191] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:37 : 12 : 207] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 01 04 05 03  ..`..... ? ......
//(00000010)   00 02 05 00 00 00 C8 02                           ...... ?
//[2023��07��23�� 12:37 : 27 : 285] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:37 : 27 : 285] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 00 01 04 02  ..`..... ? ......
//(00000010)   05 03 05 00 01 00 C8 02                           ...... ?
//[2023��07��23�� 12:37 : 49 : 412] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:37 : 49 : 433] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 03 04 01 02  ..`..... ? ......
//(00000010)   05 00 05 00 00 00 C8 02                           ...... ?
//[2023��07��23�� 12:38 : 11 : 110] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:38 : 11 : 124] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 04 02 00 05  ..`..... ? ......
//(00000010)   01 00 05 00 00 00 C8 02                           ...... ?
//[2023��07��23�� 12:38 : 31 : 392] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:38 : 31 : 408] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 02 01 05 00  ..`..... ? ......
//(00000010)   04 00 05 00 00 00 C8 02                           ...... ?
//[2023��07��23�� 12:38 : 46 : 553] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:38 : 46 : 565] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 09 15 04 00 02 01  ..`..... ? ......
//(00000010)   05 00 05 00 00 00 C8 02                           ...... ?
//[2023��07��23�� 12:39 : 29 : 941] ����(subcmd_handle.c 0112) : subcmd_get_handler δ��ɶ� 0x60 0x85 �汾 bb(5) �Ĵ���
//[2023��07��23�� 12:39 : 29 : 956] ����(subcmd_handle_60.c 3862) : δ֪ 0x60 ָ�� : 0x85
//(00000000)   1C 00 60 00 00 00 00 00   85 05 00 00 00 00 1B 00  ..`..... ? ......
//(00000010)   10 00 20 00 EC E8 43 00   A7 73 00 00             ...��C.�s..
//[2023��08��20�� 17:53 : 59 : 609] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:53 : 59 : 624] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 01 04 02 03  ..`..... ? H.....
//(00000010)   00 05 02 00 00 00 EA 03                           ...... ?
//[2023��08��20�� 17:54 : 18 : 238] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:54 : 18 : 252] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 01 04 03 05  ..`..... ? H.....
//(00000010)   00 02 03 00 00 00 EA 03                           ...... ?
//[2023��08��20�� 17:54 : 35 : 411] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:54 : 35 : 426] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 02 05 00 01  ..`..... ? H.....
//(00000010)   03 04 04 00 01 00 EA 03                           ...... ?
//[2023��08��20�� 17:54 : 54 : 241] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:54 : 54 : 254] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 02 00 05 01  ..`..... ? H.....
//(00000010)   03 04 05 00 00 00 EA 03                           ...... ?
//[2023��08��20�� 17:55 : 12 : 135] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:55 : 12 : 148] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 05 04 01 02  ..`..... ? H.....
//(00000010)   00 03 05 00 00 00 EA 03                           ...... ?
//[2023��08��20�� 17:55 : 28 : 110] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:55 : 28 : 123] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 05 02 04 01  ..`..... ? H.....
//(00000010)   03 00 05 00 01 00 EA 03                           ...... ?
//[2023��08��20�� 17:55 : 43 : 457] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:55 : 43 : 470] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 05 04 00 02  ..`..... ? H.....
//(00000010)   01 03 05 00 00 00 EA 03                           ...... ?
//[2023��08��20�� 17:56 : 00 : 579] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:56 : 00 : 594] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 02 03 05 01  ..`..... ? H.....
//(00000010)   04 00 05 00 00 00 EA 03                           ...... ?
//[2023��08��20�� 17:56 : 21 : 243] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:56 : 21 : 255] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 01 04 03 00  ..`..... ? H.....
//(00000010)   05 00 05 00 02 00 EA 03                           ...... ?
//[2023��08��20�� 17:56 : 42 : 981] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:56 : 42 : 996] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 05 04 03 01  ..`..... ? H.....
//(00000010)   00 00 05 00 01 00 EA 03                           ...... ?
//[2023��08��20�� 17:57 : 07 : 216] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:57 : 07 : 229] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 00 03 01 04  ..`..... ? H.....
//(00000010)   05 00 05 00 01 00 EA 03                           ...... ?
//[2023��08��20�� 17:57 : 27 : 307] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:57 : 27 : 320] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 05 03 01 04  ..`..... ? H.....
//(00000010)   00 00 05 00 00 00 EA 03                           ...... ?
//[2023��08��20�� 17:57 : 40 : 893] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:57 : 40 : 904] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 00 03 01 05  ..`..... ? H.....
//(00000010)   04 00 05 00 01 00 EA 03                           ...... ?
//[2023��08��20�� 17:57 : 59 : 437] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:57 : 59 : 451] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 05 01 03 00  ..`..... ? H.....
//(00000010)   04 00 05 00 01 00 EA 03                           ...... ?
//[2023��08��20�� 17:58 : 30 : 969] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:58 : 30 : 986] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 00 03 01 04  ..`..... ? H.....
//(00000010)   04 00 04 00 01 00 EA 03                           ...... ?
//[2023��08��20�� 17:58 : 44 : 881] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:58 : 44 : 894] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 03 04 01 00  ..`..... ? H.....
//(00000010)   04 00 04 00 02 00 EA 03                           ...... ?
//[2023��08��20�� 17:58 : 58 : 509] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x84 �汾 bb(5) �Ĵ���
//[2023��08��20�� 17:58 : 58 : 522] ����(subcmd_handle_60.c 4906) : δ֪ 0x60 ָ�� : 0x84
//(00000000)   18 00 60 00 00 00 00 00   84 04 48 11 04 00 03 01  ..`..... ? H.....
//(00000010)   04 00 04 00 00 00 EA 03                           ...... ?
    DBG_LOG("ָ�� 0x%04X 0x%02X %s:%d ����ID %d ���� %d",
        pkt->hdr.pkt_type, pkt->shdr.type, get_player_describe(src), src->sec_data.slot, l->qid, src->cur_area);

    print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_85_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x85_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Unknown_6x85_t), 0x05)) {
        return -2;
    }

#ifdef DEBUG
[2023��09��06�� 21:54 : 52 : 356] ����(subcmd_handle.c 0113) : subcmd_get_handler δ��ɶ� 0x60 0x85 �汾 bb(5) �Ĵ���
[2023��09��06�� 21:54 : 52 : 363] ����(subcmd_handle_60.c 7550) : δ֪ 0x60 ָ�� : 0x85
(00000000)   1C 00 60 00 00 00 00 00   85 05 00 00 00 00 2D 00  ..`..... ? .... - .
(00000010)   16 00 26 00 EC E8 43 00   E5 74 00 00             .. & .��C.�t..
#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_86_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_HitDestructibleObject_6x86_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_HitDestructibleObject_6x86_t), 0x04)) {
        return -2;
    }

    //[2023��07��12�� 20:08:29:962] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x86 �汾 5 �Ĵ���
    //[2023��07��12�� 20:08:29:978] ����(subcmd_handle_60.c 3493): δ֪ 0x60 ָ��: 0x86
    //( 00000000 )   18 00 60 00 00 00 00 00   86 04 FB 40 00 00 00 00  ..`.....?�@....
    //( 00000010 )   FB 00 00 00 00 00 05 00                           ?......
    //[2023��07��12�� 20:08:32:368] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x86 �汾 5 �Ĵ���
    //[2023��07��12�� 20:08:32:386] ����(subcmd_handle_60.c 3493): δ֪ 0x60 ָ��: 0x86
    //( 00000000 )   18 00 60 00 00 00 00 00   86 04 FB 40 00 00 00 00  ..`.....?�@....
    //( 00000010 )   FB 00 00 00 00 00 04 00                           ?......

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_88_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_arrow_change_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_arrow_change_t), 0x01)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    src->arrow_color = pkt->shdr.client_id;
    return send_lobby_arrows(l);
}

static int sub60_89_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_player_died_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:50:56:968] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x89
//
//[2023��09��27�� 19:50:56:975] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 89 02 01 00  84 10 00 00    `...........
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_player_died_t), 0x02))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_89_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_player_died_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_player_died_t), 0x02)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_8A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_ch_game_select_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_ch_game_select_t), 0x02)) {
        return -2;
    }

    switch (pkt->mode)
    {
    case 0x00:
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        break;

    case 0x01:
        src->mode = pkt->mode;
    default:
        DBG_LOG("��սģʽ�����������");
        break;
    }

    DBG_LOG("pkt->mode %d src->mode %d", pkt->mode, src->mode);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_8D_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_set_technique_level_override_t* pkt) {
    lobby_t* l = src->cur_lobby;
//[2023��09��27�� 19:49:36:198] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x8D
//
//[2023��09��27�� 19:49:36:203] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 8D 02 01 00  00 00 00 00    `...........
//[2023��09��27�� 19:50:17:380] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x8D
//
//[2023��09��27�� 19:50:17:387] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 8D 02 01 00  00 00 00 00    `...........
//
//
//
//[2023��09��27�� 19:50:18:920] ���������� ����(subcmd_handle_60.c 6928): δ֪ DCV2 0x60 ָ��: 0x8D
//
//[2023��09��27�� 19:50:18:927] ���������� ����(f_logs.c 0088): ���ݰ�����:
//
//(00000000) 60 00 0C 00 8D 02 01 00  00 00 00 00    `...........
    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_set_technique_level_override_t), 0x02))
        return -2;

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
    }

    /*uint8_t tmp_level = pkt->level_upgrade;

    pkt->level_upgrade = tmp_level+100;*/

    print_ascii_hex(dbgl, pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
}

static int sub60_8D_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_set_technique_level_override_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_set_technique_level_override_t), 0x02)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    /*uint8_t tmp_level = pkt->level_upgrade;

    pkt->level_upgrade = tmp_level+100;*/

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_8F_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_battle_player_hit_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_battle_player_hit_t), 0x02)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        DBG_LOG("�Ƿ���NPC������Ʒ��");
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
    }

    DBG_LOG("CID %d ������ CID %d �˺� %d", pkt->shdr.client_id, pkt->target_client_id, pkt->damage);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_91_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x91_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Unknown_6x91_t), 0x05)) {
        return -2;
    }
    //[2023��07��29�� 09:04:37:017] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:37:032] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 3E 40 00 00 00 00  ..`.....?>@....
    //( 00000010 )   3E 00 00 00 00 00 02 00   00 00 00 02             >...........
    //[2023��07��29�� 09:04:37:618] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:37:635] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 3E 40 00 00 00 00  ..`.....?>@....
    //( 00000010 )   3E 00 00 00 00 00 01 00   00 00 00 02             >...........
    //[2023��07��29�� 09:04:38:050] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:38:064] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 3E 40 00 00 00 00  ..`.....?>@....
    //( 00000010 )   3E 00 00 00 01 00 00 00   96 00 01 02             >.......?..
    //[2023��07��29�� 09:04:41:117] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:41:131] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 33 40 00 00 00 00  ..`.....?3@....
    //( 00000010 )   33 00 00 00 00 00 05 00   00 00 00 02             3...........
    //[2023��07��29�� 09:04:41:584] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:41:600] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 33 40 00 00 00 00  ..`.....?3@....
    //( 00000010 )   33 00 00 00 00 00 04 00   00 00 00 02             3...........
    //[2023��07��29�� 09:04:42:017] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:42:030] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 33 40 00 00 00 00  ..`.....?3@....
    //( 00000010 )   33 00 00 00 00 00 03 00   00 00 00 02             3...........
    //[2023��07��29�� 09:04:42:917] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:42:930] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 33 40 00 00 00 00  ..`.....?3@....
    //( 00000010 )   33 00 00 00 00 00 02 00   00 00 00 02             3...........
    //[2023��07��29�� 09:04:43:383] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:43:397] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 33 40 00 00 00 00  ..`.....?3@....
    //( 00000010 )   33 00 00 00 00 00 01 00   00 00 00 02             3...........
    //[2023��07��29�� 09:04:43:818] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:43:831] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 33 40 00 00 00 00  ..`.....?3@....
    //( 00000010 )   33 00 00 00 01 00 00 00   2A 00 01 02             3.......*...
    //[2023��07��29�� 09:04:53:297] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:53:314] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 76 40 00 00 00 00  ..`.....?v@....
    //( 00000010 )   76 00 00 00 00 00 07 00   00 00 00 02             v...........
    //[2023��07��29�� 09:04:53:763] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:53:778] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 76 40 00 00 00 00  ..`.....?v@....
    //( 00000010 )   76 00 00 00 00 00 06 00   00 00 00 02             v...........
    //[2023��07��29�� 09:04:54:197] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:54:212] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 76 40 00 00 00 00  ..`.....?v@....
    //( 00000010 )   76 00 00 00 00 00 05 00   00 00 00 02             v...........
    //[2023��07��29�� 09:04:55:064] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:55:088] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 76 40 00 00 00 00  ..`.....?v@....
    //( 00000010 )   76 00 00 00 00 00 04 00   00 00 00 02             v...........
    //[2023��07��29�� 09:04:55:530] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:55:548] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 76 40 00 00 00 00  ..`.....?v@....
    //( 00000010 )   76 00 00 00 00 00 03 00   00 00 00 02             v...........
    //[2023��07��29�� 09:04:55:964] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:55:977] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 76 40 00 00 00 00  ..`.....?v@....
    //( 00000010 )   76 00 00 00 00 00 02 00   00 00 00 02             v...........
    //[2023��07��29�� 09:04:56:830] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:56:850] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 76 40 00 00 00 00  ..`.....?v@....
    //( 00000010 )   76 00 00 00 00 00 01 00   00 00 00 02             v...........
    //[2023��07��29�� 09:04:57:297] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:04:57:312] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 76 40 00 00 00 00  ..`.....?v@....
    //( 00000010 )   76 00 00 00 01 00 00 00   34 00 01 02             v.......4...
    //[2023��07��29�� 09:05:55:028] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:05:55:044] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 78 40 00 00 00 00  ..`.....?x@....
    //( 00000010 )   78 00 00 00 00 00 04 00   00 00 00 02             x...........
    //[2023��07��29�� 09:05:56:395] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:05:56:417] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 78 40 00 00 00 00  ..`.....?x@....
    //( 00000010 )   78 00 00 00 00 00 03 00   00 00 00 02             x...........
    //[2023��07��29�� 09:05:58:498] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:05:58:511] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 78 40 00 00 00 00  ..`.....?x@....
    //( 00000010 )   78 00 00 00 00 00 02 00   00 00 00 02             x...........
    //[2023��07��29�� 09:05:59:631] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:05:59:639] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 78 40 00 00 00 00  ..`.....?x@....
    //( 00000010 )   78 00 00 00 00 00 01 00   00 00 00 02             x...........
    //[2023��07��29�� 09:06:00:098] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:00:113] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 78 40 00 00 00 00  ..`.....?x@....
    //( 00000010 )   78 00 00 00 01 00 00 00   36 00 01 02             x.......6...
    //[2023��07��29�� 09:06:02:265] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:02:278] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 79 40 00 00 00 00  ..`.....?y@....
    //( 00000010 )   79 00 00 00 00 00 06 00   00 00 00 02             y...........
    //[2023��07��29�� 09:06:02:732] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:02:761] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 79 40 00 00 00 00  ..`.....?y@....
    //( 00000010 )   79 00 00 00 00 00 05 00   00 00 00 02             y...........
    //[2023��07��29�� 09:06:03:165] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:03:178] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 79 40 00 00 00 00  ..`.....?y@....
    //( 00000010 )   79 00 00 00 00 00 04 00   00 00 00 02             y...........
    //[2023��07��29�� 09:06:04:132] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:04:151] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 79 40 00 00 00 00  ..`.....?y@....
    //( 00000010 )   79 00 00 00 00 00 03 00   00 00 00 02             y...........
    //[2023��07��29�� 09:06:04:599] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:04:607] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 79 40 00 00 00 00  ..`.....?y@....
    //( 00000010 )   79 00 00 00 00 00 02 00   00 00 00 02             y...........
    //[2023��07��29�� 09:06:05:032] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:05:048] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 79 40 00 00 00 00  ..`.....?y@....
    //( 00000010 )   79 00 00 00 00 00 01 00   00 00 00 02             y...........
    //[2023��07��29�� 09:06:05:932] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:05:944] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 79 40 00 00 00 00  ..`.....?y@....
    //( 00000010 )   79 00 00 00 01 00 00 00   37 00 01 02             y.......7...
    //[2023��07��29�� 09:06:10:619] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:10:636] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 77 40 00 00 00 00  ..`.....?w@....
    //( 00000010 )   77 00 00 00 00 00 03 00   00 00 00 02             w...........
    //[2023��07��29�� 09:06:11:852] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:11:871] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 77 40 00 00 00 00  ..`.....?w@....
    //( 00000010 )   77 00 00 00 00 00 02 00   00 00 00 02             w...........
    //[2023��07��29�� 09:06:12:319] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:12:336] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 77 40 00 00 00 00  ..`.....?w@....
    //( 00000010 )   77 00 00 00 00 00 01 00   00 00 00 02             w...........
    //[2023��07��29�� 09:06:13:386] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:13:403] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 77 40 00 00 00 00  ..`.....?w@....
    //( 00000010 )   77 00 00 00 01 00 00 00   35 00 01 02             w.......5...
    //[2023��07��29�� 09:06:19:552] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:19:570] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 7A 40 00 00 00 00  ..`.....?z@....
    //( 00000010 )   7A 00 00 00 00 00 05 00   00 00 00 02             z...........
    //[2023��07��29�� 09:06:20:018] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:20:036] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 7A 40 00 00 00 00  ..`.....?z@....
    //( 00000010 )   7A 00 00 00 00 00 04 00   00 00 00 02             z...........
    //[2023��07��29�� 09:06:20:452] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:20:470] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 7A 40 00 00 00 00  ..`.....?z@....
    //( 00000010 )   7A 00 00 00 00 00 03 00   00 00 00 02             z...........
    //[2023��07��29�� 09:06:21:318] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:21:339] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 7A 40 00 00 00 00  ..`.....?z@....
    //( 00000010 )   7A 00 00 00 00 00 02 00   00 00 00 02             z...........
    //[2023��07��29�� 09:06:21:785] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0x91 �汾 bb(5) �Ĵ���
    //[2023��07��29�� 09:06:21:804] ����(subcmd_handle_60.c 4037): δ֪ 0x60 ָ��: 0x91
    //( 00000000 )   1C 00 60 00 00 00 00 00   91 05 7A 40 00 00 00 00  ..`.....?z@....
    //( 00000010 )   7A 00 00 00 00 00 01 00   00 00 00 02             z...........


#ifdef DEBUG

DBG_LOG("ָ�� 0x%04X 0x%02X %s:%d ����ID %d ���� %d",
    pkt->hdr.pkt_type, pkt->shdr.type, get_player_describe(src), src->sec_data.slot, l->qid, src->cur_area);

//print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_92_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x92_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Unknown_6x92_t), 0x03)) {
        return -2;
    }

#ifdef DEBUG

    //[2023��08��25�� 11:25:01:636] ����(subcmd_handle_60.c 5097): δ֪ 0x60 ָ��: 0x92
    //( 00000000 )   14 00 60 00 00 00 00 00   92 03 00 00 00 00 00 00  ..`.....?......
    //( 00000010 )   3A 6D 20 3E                                     :m >
    DBG_LOG("ָ�� 0x%04X (0x%02X 0x%04X) %s:%d ����ID %d ���� %d",
        pkt->hdr.pkt_type, pkt->shdr.type, pkt->shdr.unused, get_player_describe(src), src->sec_data.slot, l->qid, src->cur_area);
    DBG_LOG("����:0x%04X λ��:unk %f.",
        pkt->unknown_a1, pkt->unknown_a2);

    send_txt(src, "����:0x%04X\nλ��:unk %f.", pkt->unknown_a1, pkt->unknown_a2);

    //print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_93_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_timed_switch_activated_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_timed_switch_activated_t), 0x03)) {
        return -2;
    }

//[2023��08��13�� 11:12:35:173] ����(ship_packets.c 8277): GC 42004081 �������� quest143 �汾 Blue Brust   0 31
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AC 10 01 00 03 00  ..`.....??....
//( 00000010 )   00 FF 96 02                                     .?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 65 00  ..`.....??..e.
//( 00000010 )   01 00 00 00                                     ....
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 00 00  ..`.....??....
//( 00000010 )   01 00 00 00                                     ....
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 00 00  ..`.....??....
//( 00000010 )   01 00 00 00                                     ....
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 66 00  ..`.....??..f.
//( 00000010 )   01 54 AB 10                                     .T?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 00 00  ..`.....??....
//( 00000010 )   01 54 AB 10                                     .T?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 00 00  ..`.....??....
//( 00000010 )   01 54 AB 10                                     .T?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 67 00  ..`.....??..g.
//( 00000010 )   01 54 AB 10                                     .T?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 00 00  ..`.....??....
//( 00000010 )   01 54 AB 10                                     .T?
//( 00000000 )   14 00 60 00 00 00 00 00   93 03 AB 10 01 00 00 00  ..`.....??....
//( 00000010 )   01 54 AB 10                                     .T?
    //print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_97_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_ch_game_cancel_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_ch_game_cancel_t), 0x05)) {
        return -2;
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

static int sub60_9A_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_update_player_stat_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_update_player_stat_t), 0x02)) {
        return -2;
    }

    //[2023��07��06�� 12:31:43:857] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x9A �汾 5 �Ĵ���
    //[2023��07��06�� 12:31:43:869] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x9A
    //( 00000000 )   10 00 60 00 00 00 00 00   9A 02 00 00 00 00 03 0D  ..`.....?......

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_9B_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_battle_player_die_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_battle_player_die_t), 0x02)) {
        return -2;
    }

#ifdef DEBUG

    DBG_LOG("%s CID %d = %d Die %d", get_player_describe(src), src->client_id, pkt->shdr.client_id, pkt->die_count);

    //[2023��07��18�� 19:49 : 50 : 092] ����(subcmd_handle_60.c 3757) : δ֪ 0x60 ָ�� : 0x9B
    //(00000000)   10 00 60 00 00 00 00 00   9B 02 01 00 01 00 00 00  ..`..... ? ......

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_9C_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6x9C_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Unknown_6x9C_t), 0x02)) {
        return -2;
    }

#ifdef DEBUG

    //[2023��07��06�� 13:15:14:214] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x9C �汾 5 �Ĵ���
    //[2023��07��06�� 13:15:14:233] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x9C
    //( 00000000 )   10 00 60 00 00 00 00 00   9C 02 F0 10 10 00 00 00  ..`.....??....
    //[2023��07��06�� 13:15:14:306] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x9C �汾 5 �Ĵ���
    //[2023��07��06�� 13:15:14:322] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x9C
    //( 00000000 )   10 00 60 00 00 00 00 00   9C 02 EF 10 10 00 00 00  ..`.....??....
    //[2023��07��06�� 13:16:38:651] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x9C �汾 5 �Ĵ���
    //[2023��07��06�� 13:16:38:666] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x9C
    //( 00000000 )   10 00 60 00 00 00 00 00   9C 02 12 11 10 00 00 00  ..`.....?......
    //[2023��07��06�� 13:16:39:680] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x9C �汾 5 �Ĵ���
    //[2023��07��06�� 13:16:39:696] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x9C
    //( 00000000 )   10 00 60 00 00 00 00 00   9C 02 13 11 10 00 00 00  ..`.....?......
    //[2023��07��06�� 13:16:42:718] ����(subcmd_handle.c 0111): subcmd_get_handler δ��ɶ� 0x60 0x9C �汾 5 �Ĵ���
    //[2023��07��06�� 13:16:42:735] ����(subcmd_handle_60.c 3061): δ֪ 0x60 ָ��: 0x9C
    //( 00000000 )   10 00 60 00 00 00 00 00   9C 02 11 11 10 00 00 00  ..`.....?......


    DBG_LOG("ָ�� 0x%04X 0x%02X %s:%d ����ID %d ���� %d",
        pkt->hdr.pkt_type, pkt->shdr.type, get_player_describe(src), src->sec_data.slot, l->qid, src->cur_area);

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_9D_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_ch_game_failure_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_ch_game_failure_t), 0x02)) {
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_9F_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_GalGryphonActions_6x9F_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_GalGryphonActions_6x9F_t), 0xFF)) {
        return -2;
    }

    send_txt(src, "%s\n����:0x%04X\nλ��:x %f z %f.", __(src, "\tE\tC6GG BOSS"), pkt->unknown_a1, pkt->x, pkt->z);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_A0_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_GalGryphonActions_6xA0_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_GalGryphonActions_6xA0_t), 0x0A)) {
        return -2;
    }

    send_txt(src, "%s\n"
        "λ��:x:%f y:%f z:%f\n"
        "����:%d\n"
        "�׶�:0x%02X 0x%02X.",
        "δ֪:%d\n%d\n%d\n%d.",
        __(src, "\tE\tC6GG BOSS SP"),
        pkt->x, pkt->y, pkt->z,
        pkt->unknown_a1,
        pkt->unknown_a2, pkt->unknown_a3,
        pkt->unknown_a4[0],
        pkt->unknown_a4[1],
        pkt->unknown_a4[2],
        pkt->unknown_a4[3]
    );

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_A1_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_save_player_act_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_A3_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Episode2BossActions_6xA3_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_A4_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_OlgaFlowPhase1Actions_6xA4_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_A5_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_OlgaFlowPhase2Actions_6xA5_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_A8_dc(ship_client_t* src, ship_client_t* dest,
    subcmd_gol_dragon_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int v = src->version, i;
    subcmd_gol_dragon_act_t tr;

    if (!in_game(src))
        return -1;

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_gol_dragon_act_t));
    tr.x.b = SWAP32(tr.x.b);
    tr.z.b = SWAP32(tr.z.b);

    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != src) {
            if (l->clients[i]->version == v) {
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)pkt);
            }
            else {
                send_pkt_dc(l->clients[i], (dc_pkt_hdr_t*)&tr);
            }
        }
    }

    return 0;
}

static int sub60_A8_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_gol_dragon_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int v = src->version, i;
    subcmd_bb_gol_dragon_act_t tr;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_gol_dragon_act_t), 0x06)) {
        return -2;
    }

    if (v == CLIENT_VERSION_BB)
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_bb_gol_dragon_act_t));
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

static int sub60_A9_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_BarbaRayActions_6xA9_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_BarbaRayActions_6xA9_t), 0x02)) {
        return -2;
    }

//[2023��08��09�� 21:05:10:982] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:10:997] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 03 00 01 00  ..`.....??....
//[2023��08��09�� 21:05:11:044] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:11:058] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 08 00 02 00  ..`.....??....
//[2023��08��09�� 21:05:19:488] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:19:503] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0F 00 02 00  ..`.....??....
//[2023��08��09�� 21:05:20:917] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:20:932] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0F 00 03 00  ..`.....??....
//[2023��08��09�� 21:05:20:977] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:20:992] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 28 00 04 00  ..`.....??(...
//[2023��08��09�� 21:05:31:622] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:31:635] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 05 00  ..`.....??;...
//[2023��08��09�� 21:05:33:283] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:33:295] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 36 00 06 00  ..`.....??6...
//[2023��08��09�� 21:05:35:838] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:35:851] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 07 00  ..`.....??;...
//[2023��08��09�� 21:05:45:904] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:45:920] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 37 00 09 00  ..`.....??7...
//[2023��08��09�� 21:05:48:438] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:48:452] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3A 00 0A 00  ..`.....??:...
//[2023��08��09�� 21:05:50:116] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:50:127] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 29 00 0B 00  ..`.....??)...
//[2023��08��09�� 21:05:56:013] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:56:026] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 10 00 0B 00  ..`.....??....
//[2023��08��09�� 21:05:57:413] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:57:427] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 10 00 0C 00  ..`.....??....
//[2023��08��09�� 21:05:57:474] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:57:487] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 12 00 0D 00  ..`.....??....
//[2023��08��09�� 21:06:02:680] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:02:694] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 0E 00  ..`.....???...
//[2023��08��09�� 21:06:05:680] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:05:693] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 16 00 0F 00  ..`.....??....
//[2023��08��09�� 21:06:10:175] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:10:187] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3A 00 10 00  ..`.....??:...
//[2023��08��09�� 21:06:11:780] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:11:793] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 12 00 11 00  ..`.....??....
//[2023��08��09�� 21:06:17:047] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:17:058] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 12 00  ..`.....???...
//[2023��08��09�� 21:06:20:062] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:20:076] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 16 00 13 00  ..`.....??....
//[2023��08��09�� 21:06:24:557] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:24:570] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 14 00  ..`.....??;...
//[2023��08��09�� 21:06:26:162] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:26:175] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 28 00 15 00  ..`.....??(...
//[2023��08��09�� 21:06:36:905] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:36:918] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3A 00 16 00  ..`.....??:...
//[2023��08��09�� 21:06:38:600] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:38:613] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 36 00 17 00  ..`.....??6...
//[2023��08��09�� 21:06:41:121] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:41:133] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 18 00  ..`.....??;...
//[2023��08��09�� 21:06:42:755] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xAA �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:42:769] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xAA
//( 00000000 )   14 00 60 00 00 00 00 00   AA 03 86 16 44 00 19 00  ..`.....??D...
//( 00000010 )   00 00 00 00                                     ....
//[2023��08��09�� 21:06:51:272] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:51:287] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 39 00 1A 00  ..`.....??9...
//[2023��08��09�� 21:06:53:750] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:53:763] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 1B 00  ..`.....??;...
//[2023��08��09�� 21:06:55:384] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:06:55:398] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 29 00 1C 00  ..`.....??)...
//[2023��08��09�� 21:07:01:304] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:01:318] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0F 00 1D 00  ..`.....??....
//[2023��08��09�� 21:07:02:732] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:02:744] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 11 00 1E 00  ..`.....??....
//[2023��08��09�� 21:07:07:937] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:07:949] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 1F 00  ..`.....???...
//[2023��08��09�� 21:07:10:940] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:10:951] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 15 00 20 00  ..`.....??.. .
//[2023��08��09�� 21:07:15:435] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:15:448] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 21 00  ..`.....??;.!.
//[2023��08��09�� 21:07:17:046] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:17:058] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 11 00 22 00  ..`.....??..".
//[2023��08��09�� 21:07:22:316] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:22:328] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 23 00  ..`.....???.#.
//[2023��08��09�� 21:07:25:320] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:25:334] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 15 00 24 00  ..`.....??..$.
//[2023��08��09�� 21:07:29:786] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:29:802] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3A 00 25 00  ..`.....??:.%.
//[2023��08��09�� 21:07:31:419] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:31:433] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 11 00 26 00  ..`.....??..&.
//[2023��08��09�� 21:07:36:686] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:36:697] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 27 00  ..`.....???.'.
//[2023��08��09�� 21:07:39:689] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:39:705] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 15 00 28 00  ..`.....??..(.
//[2023��08��09�� 21:07:44:155] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:44:165] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 29 00  ..`.....??;.).
//[2023��08��09�� 21:07:45:823] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:45:839] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 04 00 2A 00  ..`.....??..*.
//[2023��08��09�� 21:07:56:129] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:56:145] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 10 00 2A 00  ..`.....??..*.
//[2023��08��09�� 21:07:57:529] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:07:57:541] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 28 00 2B 00  ..`.....??(.+.
//[2023��08��09�� 21:08:08:229] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:08:243] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 2B 00 2C 00  ..`.....??+.,.
//[2023��08��09�� 21:08:11:495] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:11:508] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3D 00 2D 00  ..`.....??=.-.
//[2023��08��09�� 21:08:13:134] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:13:149] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 29 00 2E 00  ..`.....??)...
//[2023��08��09�� 21:08:17:371] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:17:383] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3C 00 2F 00  ..`.....??<./.
//[2023��08��09�� 21:08:19:004] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:19:015] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 17 00 30 00  ..`.....??..0.
//[2023��08��09�� 21:08:20:866] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xAA �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:20:881] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xAA
//( 00000000 )   14 00 60 00 00 00 00 00   AA 03 86 16 18 00 31 00  ..`.....??..1.
//( 00000010 )   00 00 00 00                                     ....
//[2023��08��09�� 21:08:25:035] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xAA �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:25:050] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xAA
//( 00000000 )   14 00 60 00 00 00 00 00   AA 03 86 16 18 00 32 00  ..`.....??..2.
//( 00000010 )   00 00 00 00                                     ....
//[2023��08��09�� 21:08:29:176] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:29:189] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 19 00 33 00  ..`.....??..3.
//[2023��08��09�� 21:08:31:077] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:31:088] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0D 00 34 00  ..`.....??..4.
//[2023��08��09�� 21:08:42:543] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:42:556] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 28 00 35 00  ..`.....??(.5.
//[2023��08��09�� 21:08:46:910] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:46:922] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3C 00 36 00  ..`.....??<.6.
//[2023��08��09�� 21:08:48:571] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:48:588] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 32 00 37 00  ..`.....??2.7.
//[2023��08��09�� 21:09:07:602] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:07:616] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 37 00 39 00  ..`.....??7.9.
//[2023��08��09�� 21:09:10:203] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:10:219] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3C 00 3A 00  ..`.....??<.:.
//[2023��08��09�� 21:09:11:837] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:11:852] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3D 00 3B 00  ..`.....??=.;.
//[2023��08��09�� 21:09:13:442] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:13:456] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 29 00 3C 00  ..`.....??).<.
//[2023��08��09�� 21:09:19:406] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:19:422] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 10 00 3D 00  ..`.....??..=.
//[2023��08��09�� 21:09:20:745] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:20:758] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 12 00 3E 00  ..`.....??..>.
//[2023��08��09�� 21:09:25:978] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:25:995] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 3F 00  ..`.....???.?.
//[2023��08��09�� 21:09:27:994] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:28:006] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 16 00 40 00  ..`.....??..@.
//[2023��08��09�� 21:09:32:461] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:32:475] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3A 00 41 00  ..`.....??:.A.
//[2023��08��09�� 21:09:34:095] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:34:108] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 12 00 42 00  ..`.....??..B.
//[2023��08��09�� 21:09:39:367] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:39:381] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 43 00  ..`.....???.C.
//[2023��08��09�� 21:09:41:445] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:41:458] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 16 00 44 00  ..`.....??..D.
//[2023��08��09�� 21:09:45:850] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:45:862] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0A 00 45 00  ..`.....??..E.
//[2023��08��09�� 21:09:56:150] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:56:165] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0F 00 45 00  ..`.....??..E.
//[2023��08��09�� 21:09:57:607] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:57:621] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0F 00 46 00  ..`.....??..F.
//[2023��08��09�� 21:09:57:648] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:09:57:661] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 11 00 47 00  ..`.....??..G.
//[2023��08��09�� 21:10:02:817] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:02:829] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 48 00  ..`.....???.H.
//[2023��08��09�� 21:10:04:832] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:04:848] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 15 00 49 00  ..`.....??..I.
//[2023��08��09�� 21:10:09:299] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:09:312] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 4A 00  ..`.....??;.J.
//[2023��08��09�� 21:10:10:960] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:10:974] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 11 00 4B 00  ..`.....??..K.
//[2023��08��09�� 21:10:16:200] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:16:211] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3F 00 4C 00  ..`.....???.L.
//[2023��08��09�� 21:10:18:212] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:18:228] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 15 00 4D 00  ..`.....??..M.
//[2023��08��09�� 21:10:22:689] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:22:701] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 4E 00  ..`.....??;.N.
//[2023��08��09�� 21:10:24:322] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:24:336] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 05 00 4F 00  ..`.....??..O.
//[2023��08��09�� 21:10:28:709] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:28:724] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3D 00 50 00  ..`.....??=.P.
//[2023��08��09�� 21:10:30:309] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:30:319] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 17 00 51 00  ..`.....??..Q.
//[2023��08��09�� 21:10:40:455] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:40:469] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 19 00 54 00  ..`.....??..T.
//[2023��08��09�� 21:10:42:355] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:42:370] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3D 00 55 00  ..`.....??=.U.
//[2023��08��09�� 21:10:43:988] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:44:001] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0D 00 56 00  ..`.....??..V.
//[2023��08��09�� 21:10:55:505] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:55:516] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 08 00 02 00  ..`.....??....
//[2023��08��09�� 21:10:57:046] ����(block_bb.c 0322): PSOBB ����
//[2023��08��09�� 21:10:58:613] ����(block_bb.c 0381): Ĭ�ϵ���ģʽ
//[2023��08��09�� 21:11:03:935] ����(subcmd_send_bb.c 0610): GC 42004145 �ķ��侭�鱶������Ϊ 3000 ��
//[2023��08��09�� 21:11:03:982] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:03:994] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0F 00 02 00  ..`.....??....
//[2023��08��09�� 21:11:05:382] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:05:397] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 0F 00 03 00  ..`.....??....
//[2023��08��09�� 21:11:05:443] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:05:455] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 28 00 04 00  ..`.....??(...
//[2023��08��09�� 21:11:16:115] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:16:127] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 05 00  ..`.....??;...
//[2023��08��09�� 21:11:17:749] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:17:761] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 36 00 06 00  ..`.....??6...
//[2023��08��09�� 21:11:20:319] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:20:334] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3B 00 07 00  ..`.....??;...
//[2023��08��09�� 21:11:30:428] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:30:440] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 37 00 09 00  ..`.....??7...
//[2023��08��09�� 21:11:32:939] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:32:953] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 3A 00 0A 00  ..`.....??:...
//[2023��08��09�� 21:11:34:634] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:34:648] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 29 00 0B 00  ..`.....??)...
//[2023��08��09�� 21:11:40:546] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:40:561] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 10 00 0C 00  ..`.....??....
//[2023��08��09�� 21:11:41:342] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:41:355] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 1B 00 0C 00  ..`.....??....
//[2023��08��09�� 21:11:44:714] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:44:730] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 1C 00 0C 00  ..`.....??....
//[2023��08��09�� 21:11:54:186] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xA9 �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:54:198] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xA9
//( 00000000 )   10 00 60 00 00 00 00 00   A9 02 86 16 1D 00 0C 00  ..`.....??....

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_AA_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Episode2BossActions_6xAA_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_Episode2BossActions_6xAA_t), 0x03)) {
        return -2;
    }

//[2023��08��09�� 21:08:59:179] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xAA �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:08:59:191] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xAA
//( 00000000 )   14 00 60 00 00 00 00 00   AA 03 86 16 44 00 38 00  ..`.....??D.8.
//( 00000010 )   00 00 00 00                                     ....
//[2023��08��09�� 21:10:32:149] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xAA �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:32:162] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xAA
//( 00000000 )   14 00 60 00 00 00 00 00   AA 03 86 16 18 00 52 00  ..`.....??..R.
//( 00000010 )   00 00 00 00                                     ....
//[2023��08��09�� 21:10:36:288] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xAA �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:10:36:302] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xAA
//( 00000000 )   14 00 60 00 00 00 00 00   AA 03 86 16 18 00 53 00  ..`.....??..S.
//( 00000010 )   00 00 00 00                                     ....
//[2023��08��09�� 21:05:37:532] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xAA �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:05:37:543] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xAA
//( 00000000 )   14 00 60 00 00 00 00 00   AA 03 86 16 44 00 08 00  ..`.....??D...
//( 00000010 )   00 00 00 00                                     ....
//[2023��08��09�� 21:11:21:953] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xAA �汾 bb(5) �Ĵ���
//[2023��08��09�� 21:11:21:967] ����(subcmd_handle_60.c 4384): δ֪ 0x60 ָ��: 0xAA
//( 00000000 )   14 00 60 00 00 00 00 00   AA 03 86 16 44 00 08 00  ..`.....??D...
//( 00000010 )   00 00 00 00                                     ....

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_AB_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_create_lobby_chair_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_create_lobby_chair_t), 0x02)) {
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_AD_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_Unknown_6xAD_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

#ifdef DEBUG

    DBG_LOG("ָ�� 0x%04X 0x%02X %s:%d ����ID %d ���� %d",
        pkt->hdr.pkt_type, pkt->shdr.type, get_player_describe(src), src->sec_data.slot, l->qid, src->cur_area);

#endif // DEBUG

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_AF_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_turn_lobby_chair_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_turn_lobby_chair_t), 0x02)) {
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_B0_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_move_lobby_chair_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_move_lobby_chair_t), 0x02)) {
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_C0_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_sell_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->item_id, sell_amount = pkt->sell_amount;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_sell_item_t), 0x03)) {
        return -2;
    }

    psocn_bb_char_t* character = get_client_char_bb(src);

    iitem_t iitem = remove_iitem(src, item_id, sell_amount, src->version != CLIENT_VERSION_BB);
    if (iitem.data.datal[0] == 0 && iitem.data.data2l == 0) {
        ERR_LOG("%s:%d ���� %d �� ID 0x%08X ʧ��", 
            get_player_describe(src), src->sec_data.slot, sell_amount, item_id);
        return -1;
    }

    size_t orignal_price = price_for_item(&iitem.data);
    if (orignal_price <= 0) {
        ERR_LOG("%s:%d ���� %d �� ID 0x%04X �������� orignal_price %d",
            get_player_describe(src), src->sec_data.slot, sell_amount, item_id, orignal_price);
        return -2;
    }

    uint32_t sell_price = (orignal_price >> 3) * sell_amount;

    add_character_meseta(character, sell_price);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_C3_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_drop_split_stacked_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t item_id = pkt->item_id, amount = pkt->amount, area = pkt->area;
    float x = pkt->x, z = pkt->z;
    litem_t* lt;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_drop_split_stacked_item_t), 0x06)) {
        return -2;
    }

    iitem_t iitem = remove_iitem(src, item_id, amount, src->version != CLIENT_VERSION_BB);
    if (iitem.data.datal[0] == 0 && iitem.data.data2l == 0) {
        ERR_LOG("%s ����ѵ���Ʒʧ��! ID 0x%08X ���� %u",
            get_player_describe(src), item_id, amount);
        return -1;
    }

    iitem.data.item_id = generate_item_id(l, 0xFF);

    if (!add_iitem(src, iitem)) {
        ERR_LOG("%s ��Ʒ������ұ���ʧ��!",
            get_player_describe(src));
        return -1;
    }

    /* We have the item... Add it to the lobby's inventory. */
    lt = add_litem_locked(l, &iitem, area, x, z);
    if (!lt) {
        /* *Gulp* The lobby is probably toast... At least make sure this user is
           still (mostly) safe... */
        ERR_LOG("�޷�����Ʒ�������Ϸ����!");
        return -1;
    }

    return subcmd_send_lobby_drop_stack_bb(src, src->client_id, NULL, lt);
}

static int sub60_C4_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_sort_inv_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_sort_inv_t), 0x1F)) {
        return -2;
    }

    inventory_t* inv = get_client_inv_bb(src);

    sort_client_inv(inv);

    /* Nobody else really needs to care about this one... */
    return 0;
}

static int sub60_C5_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_medical_center_used_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_medical_center_used_t), 0x01)) {
        return -2;
    }

    psocn_bb_char_t* character = get_client_char_bb(src);

    subcmd_send_lobby_bb_delete_meseta(src, character, 10, false);

    /* Send it along to the rest of the lobby. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_C6_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_steal_exp_t* pkt) {
    lobby_t* l = src->cur_lobby;
    pmt_weapon_bb_t tmp_wp = { 0 };
    game_enemy_t* en;
    uint32_t exp_percent = 0;
    uint32_t exp_to_add;
    uint8_t special = 0;
    /* Make sure the enemy is in range. */
    uint16_t mid = LE16(pkt->shdr.enemy_id);
    uint32_t bp, exp_amount;
    int i;

    if (!in_game(src))
        return -1;

    psocn_bb_char_t* character = get_client_char_bb(src);

    mid = LE16(pkt->shdr.enemy_id);
    mid &= 0xFFF;

    if (mid < 0xB50) {
        for (i = 0; i < character->inv.item_count; i++) {
            if ((character->inv.iitems[i].flags & LE32(0x00000008)) &&
                (character->inv.iitems[i].data.datab[0] == ITEM_TYPE_WEAPON)) {
                if ((character->inv.iitems[i].data.datab[1] < 0x0A) &&
                    (character->inv.iitems[i].data.datab[2] < 0x05)) {
                    special = (character->inv.iitems[i].data.datab[4] & 0x1F);
                }
                else {
                    if ((character->inv.iitems[i].data.datab[1] < 0x0D) &&
                        (character->inv.iitems[i].data.datab[2] < 0x04))
                        special = (character->inv.iitems[i].data.datab[4] & 0x1F);
                    else {
                        if (pmt_lookup_weapon_bb(character->inv.iitems[i].data.datal[0], &tmp_wp)) {
                            ERR_LOG("%s װ���˲����ڵ���Ʒ����!",
                                get_player_describe(src));
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
                    if ((l->difficulty == GAME_TYPE_DIFFICULTY_ULTIMATE) && (src->equip_flags & EQUIP_FLAGS_DROID))
                        exp_percent += 30;
                    break;
                }

                break;
            }
        }

        if (exp_percent) {
            //DBG_LOG("������ԭֵ %02X %02X", mid, pkt->enemy_id2);
            mid &= 0xFFF;
            //DBG_LOG("��������ֵ %02X map_enemies->count %02X", mid, l->map_enemies->count);

            if (mid > l->map_enemies->enemy_count) {
                ERR_LOG("%s ɱ������Ч�ĵ��� (%d -- "
                    "��������: %d)!", get_player_describe(src), mid, l->map_enemies->enemy_count);
                return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
                //return -1;
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

            //�������鱶���������� exp_mult expboost 11.18
            exp_amount = (l->bb_params[bp].exp * exp_percent) / 100L;

            if (exp_amount > 80)  // Limit the amount of exp stolen to 80
                exp_amount = 80;

            if ((src->game_data->expboost) && (l->exp_mult > 0)) {
                exp_to_add = exp_amount;
                exp_amount = exp_to_add + (l->bb_params[bp].exp * l->exp_mult);
            }

            if (!exp_amount) {
                ERR_LOG("δ��ȡ��������ֵ %d bp %d ���� %d", exp_amount, bp, l->exp_mult);
                return client_give_exp(src, 1);
            }
#ifdef DEBUG

            DBG_LOG("exp_amount %d", exp_amount);

#endif // DEBUG

            return client_give_exp(src, exp_amount);
        }
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_C7_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_charge_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t meseta_amount = pkt->meseta_amount;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_charge_act_t), 0x02)) {
        return -2;
    }

    psocn_bb_char_t* character = get_client_char_bb(src);

    subcmd_send_lobby_bb_delete_meseta(src, character, meseta_amount, false);

    /* Send it along to the rest of the lobby. */
    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

/* �ӹ����ȡ�ľ��鱶��δ���е��� */
static int sub60_C8_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_req_exp_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint16_t mid;
    pmt_guard_bb_t tmp_guard = { 0 };
    uint32_t bp, exp_amount, eic = 0;
    game_enemy_t* en;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_req_exp_t), 0x03)) {
        return -2;
    }

    /* Make sure the enemy is in range. */
    mid = LE16(pkt->enemy_id2);
    mid &= 0xFFF;

    if (mid > l->map_enemies->enemy_count) {
        ERR_LOG("%s ɱ������Ч�ĵ��� (%d -- "
            "��������: %d)!", get_player_describe(src), mid, l->map_enemies->enemy_count);
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
        //return -1;
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
    exp_amount = l->bb_params[bp].exp;

    inventory_t* inv = get_client_inv_bb(src);

    for (int x = 0; x < inv->item_count; x++) {
        if (!(inv->iitems[x].flags & 0x00000008)) {
            continue;
        }
        if (inv->iitems[x].data.datab[0] != ITEM_TYPE_GUARD) {
            continue;
        }
        switch (inv->iitems[x].data.datab[1]) {
        case ITEM_SUBTYPE_FRAME:
            if (pmt_lookup_guard_bb(inv->iitems[x].data.datal[0], &tmp_guard)) {
                ERR_LOG("δ��PMT��ȡ�� 0x%04X ������!", inv->iitems[x].data.datal[0]);
                break;
            }
            eic += LE32(tmp_guard.eic);
            break;

        case ITEM_SUBTYPE_BARRIER:
            if (pmt_lookup_guard_bb(inv->iitems[x].data.datal[0], &tmp_guard)) {
                ERR_LOG("δ��PMT��ȡ�� 0x%04X ������!", inv->iitems[x].data.datal[0]);
                break;
            }
            eic += LE32(tmp_guard.eic);
            break;
        }
    }

    if (eic > 0) {
        exp_amount += eic;
        //DBG_LOG("������ֵ %d", exp_amount);
    }

    if ((src->game_data->expboost) && (l->exp_mult > 0))
        exp_amount = exp_amount * l->exp_mult;

    if (!exp_amount)
        ERR_LOG("δ��ȡ��������ֵ %d bp %d ���� %d", exp_amount, bp, l->exp_mult);

    // TODO �������乲���� �ֱ�������3����ҷ�����ֵ���ȵľ���ֵ
    if (!pkt->last_hitter) {
        exp_amount = (exp_amount * 80) / 100;
    }

    return client_give_exp(src, exp_amount);
}

static int sub60_CC_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_guild_ex_item_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t ex_item_id = pkt->ex_item_id, ex_amount = pkt->ex_amount;
    
    if (!in_game(src))
        return -1;

    if (ex_item_id == EMPTY_STRING) {
        DBG_LOG("���� 0x60 ָ��: 0x%02X", pkt->hdr.pkt_type);
        print_ascii_hex(dbgl, pkt, pkt->hdr.pkt_len);
        return -1;
    }

    inventory_t* inv = get_client_inv_bb(src);

    int i = find_iitem_index(inv, ex_item_id);

    /* ����Ҳ�������Ʒ�����û��Ӵ�������. */
    if (i < 0) {
        ERR_LOG("%s ���������ڵ�װ����Ʒ! ������ %d", get_player_describe(src), i);
        return i;
    }
    pmt_item_base_check_t item_base_check = get_item_definition_bb(inv->iitems[i].data.datal[0], inv->iitems[i].data.datal[1]);
    if (item_base_check.err) {
        return send_msg(src, MSG1_TYPE, "%s ������ %d", __(src, "\tE\tC4�����������Ʒ��Ч."), item_base_check.err);
    }

    iitem_t iitem = remove_iitem(src, pkt->ex_item_id, ex_amount, src->version != CLIENT_VERSION_BB);
    if (iitem.data.datal[0] == 0 && iitem.data.data2l == 0) {
        ERR_LOG("�޷�����ұ������Ƴ� ID 0x%08X ��Ʒ!", pkt->ex_item_id);
        return -1;
    }

    src->guild_points_personal_donation += item_base_check.base.team_points;
    src->bb_guild->data.guild_points_rest += item_base_check.base.team_points;

#ifdef DEBUG

    DBG_LOG("sub60_CC_bb %d", src->guild_points_personal_donation);

#endif // DEBUG
    shipgate_send_bb_guild_points(&ship->sg, src->guildcard, src->cur_block->b, item_base_check.base.team_points);

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_CF_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_start_battle_mode_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_start_battle_mode_t), 0x0D)) {
        return -2;
    }

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_D2_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_gallon_area_pkt_t* pkt) {
    lobby_t* l = src->cur_lobby;
    uint32_t quest_offset = pkt->quest_offset;
    uint32_t value = pkt->value;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_gallon_area_pkt_t), 0x03)) {
        return -2;
    }

    //[2023��02��09�� 21:50:51:617] ����(subcmd-bb.c 4858): Unknown 0x60: 0xD2
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  D2 03 FF FF 05 00 00 00    ..`.............
    //( 00000010 )   F3 AD 00 00                                         ....
    //[2023��02��09�� 21:50:51:653] ����(subcmd-bb.c 4858): Unknown 0x60: 0xD2
    //
    //( 00000000 )   14 00 60 00 00 00 00 00  D2 03 FF FF 05 00 00 00    ..`.............
    //( 00000010 )   9A D7 00 00                                         ....
    if (quest_offset < 23) {
        quest_offset *= 4;
        *(uint32_t*)&src->bb_pl->quest_data2[quest_offset] = value;
    }

    return send_pkt_bb(src, (bb_pkt_hdr_t*)pkt);
}

static int sub60_D7_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_item_exchange_pd_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_item_exchange_pd_t), 0x07)) {
        return -2;
    }

    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("%s ID��һ��!", get_player_describe(src));
        return -2;
    }

    //[2023��08��13�� 15:39:39:110] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0xD7 �汾 bb(5) �Ĵ���
    //[2023��08��13�� 15:39:39:121] ����(subcmd_handle_60.c 4737): δ֪ 0x60 ָ��: 0xD7
    //( 00000000 )   24 00 60 00 00 00 00 00   D7 07 00 00/ 01 02 4B 00  $.`.....?....K.
    //( 00000010 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
    //( 00000020 )   D0 00 C2 01                                     ??
//[2023��08��13�� 16:17:26:288] ����(ship_packets.c 8277): GC 42004064 �������� quest204 �汾 Blue Brust   0 31
//[2023��08��13�� 16:17:58:722] ��Ʒ(0263): ��Ʒ:(ID 8454144 / 00810000) ����΢��
//[2023��08��13�� 16:17:58:736] ��Ʒ(0268): ����: 03100000, 00630000, 00000000, 00000000
//( 00000000 )   24 00 60 00 00 00 00 00   D7 07 00 00 03 0E 07 00  $.`.....?......
//( 00000010 )   00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00  ................
//( 00000020 )   D0 00 C2 01                                     ??

    print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);

    iitem_t work_item = { 0 };
    for (size_t x = 0; x < ARRAYSIZE(gallons_shop_hopkins); x += 2) {
        if (pkt->add_item.datal[0] == gallons_shop_hopkins[x]) {
            work_item.data.datab[0] = ITEM_TYPE_TOOL;
            work_item.data.datab[1] = ITEM_SUBTYPE_PHOTON;
            work_item.data.datab[2] = 0x00;
            break;
        }
    }

    if (work_item.data.datab[0] != ITEM_TYPE_TOOL) {
        ERR_LOG("�һ�ʧ��, δ�ҵ���Ӧ��Ʒ");
        send_msg(src, BB_SCROLL_MSG_TYPE, __(src, "�һ�ʧ��,δ�ҵ���Ӧ�һ���Ʒ"));
        return -2;
    }

    inventory_t* inv = get_client_inv_bb(src);

    size_t work_item_id = find_iitem_stack_item_id(inv, &work_item);
    if (work_item_id == 0) {
        ERR_LOG("%s �һ�ʧ��, δ�ҵ���Ӧ��Ʒ", get_player_describe(src));
        send_msg(src, MSG_BOX_TYPE, "%s", __(src, "�һ�ʧ��, δ�ҵ���Ӧ��Ʒ"));
        send_bb_item_exchange_state(src, 0x00000001);
    }
    else {
        /* �ҵ�PD��Ʒ ��ɾ��ȫ������ */
        int index = find_iitem_index(inv, work_item_id);
        if (index < 0) {
            ERR_LOG("%s �һ���Ʒʧ��! ������ %d", get_player_describe(src), index);
            return index;
        }
        iitem_t* del_item = &inv->iitems[index];
        size_t max_count = get_item_amount(&del_item->data, 99);
        iitem_t pd = remove_iitem(src, del_item->data.item_id, max_count, src->version != CLIENT_VERSION_BB);
        if (item_not_identification_bb(pd.data.datal[0], pd.data.datal[1])) {
            ERR_LOG("ɾ��PD %d ID 0x%08X ʧ��", max_count, del_item->data.item_id);
            return -2;
        }

        rv = subcmd_send_lobby_bb_destroy_item(src, del_item->data.item_id, max_count);

        /* ����ұ�����Ӷһ���ɵ���Ʒ */
        iitem_t add_item = player_iitem_init(pkt->add_item);
        add_item.data.item_id = generate_item_id(l, src->client_id);
        add_item.data.datab[5] = get_item_amount(&add_item.data, 1);

        if (!(rv = add_iitem(src, add_item))) {
            ERR_LOG("%s �����ռ䲻��, �޷������Ʒ! ������ %d", get_player_describe(src), rv);
            return -3;
        }
        rv = subcmd_send_lobby_bb_create_inv_item(src, add_item.data, 1, true);

        /* �����������״̬ */
        rv = send_bb_confirm_update_quest_statistics(src, 1, pkt->function_id);

        rv = send_msg(src, MSG_BOX_TYPE, "%s", __(src, "�һ��ɹ�"));

        if (rv)
            return -3;
    }

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_D8_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_item_add_srank_weapon_special_t* pkt) {
    lobby_t* l = src->cur_lobby;
    int rv = 0;

    if (!in_game(src))
        return -1;

    if (pkt->shdr.client_id != src->client_id) {
        ERR_LOG("%s ID��һ��!", get_player_describe(src));
        return 0;
    }

    print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_D9_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_item_exchange_momoka_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

#ifdef DEBUG

    DBG_LOG("sub60_D9_bb ���ݰ���С 0x%04X SIZE 0x%04X", pkt->hdr.pkt_len, pkt->shdr.size);

#endif // DEBUG

    inventory_t* inv = get_client_inv_bb(src);

    iitem_t compare_item = { 0 };
    compare_item.data = pkt->compare_item;
    uint32_t compare_item_id = find_iitem_stack_item_id(inv, &compare_item);

    if (compare_item_id == 0) {

        ERR_LOG("%s û�жһ�����Ʒ!", get_player_describe(src));

        send_bb_item_exchange_state(src, 0x00000001);
    }
    else {

        iitem_t item = remove_iitem(src, compare_item_id, 1, src->version != CLIENT_VERSION_BB);
        if (item.data.datal[0] == 0 && item.data.data2l == 0) {
            ERR_LOG("�һ� %d ID 0x%04X ʧ��", 1, compare_item_id);
            return -1;
        }

        subcmd_send_bb_exchange_item_in_quest(src, compare_item_id, 1);

        subcmd_send_lobby_bb_destroy_item(src, compare_item_id, 1);

        iitem_t add_item = player_iitem_init(pkt->add_item);
        add_item.data.item_id = generate_item_id(l, src->client_id);
        add_item.data.datab[5] = get_item_amount(&add_item.data, 1);

        if (!add_iitem(src, add_item)) {
            ERR_LOG("%s �����ռ䲻��, �޷������Ʒ!", get_player_describe(src));
            return -1;
        }

        subcmd_send_lobby_bb_create_inv_item(src, add_item.data, get_item_amount(&add_item.data, 1), true);

        send_bb_item_exchange_state(src, 0x00000000);
    }

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_DC_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_boss_act_saint_million_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_boss_act_saint_million_t), 0x02)) {
        return -2;
    }

    send_txt(src, "%s\n����:0x%04X\n�׶�:0x%04X.", __(src, "\tE\tC6SM BOSS"), pkt->unknown_a1, pkt->unknown_a2);

    //[2023��07��28�� 21:42:58:852] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0xDC �汾 bb(5) �Ĵ���
    //[2023��07��28�� 21:42:58:871] ����(subcmd_handle_60.c 4003): δ֪ 0x60 ָ��: 0xDC
    //( 00000000 )   10 00 60 00 00 00 00 00   DC 02 FF FF 00 00 16 00  ..`.....?....
    //[2023��07��28�� 21:42:59:903] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0xDC �汾 bb(5) �Ĵ���
    //[2023��07��28�� 21:42:59:921] ����(subcmd_handle_60.c 4003): δ֪ 0x60 ָ��: 0xDC
    //( 00000000 )   10 00 60 00 00 00 00 00   DC 02 FF FF 00 00 11 00  ..`.....?....
    //[2023��07��28�� 21:43:09:525] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0xDC �汾 bb(5) �Ĵ���
    //[2023��07��28�� 21:43:09:540] ����(subcmd_handle_60.c 4003): δ֪ 0x60 ָ��: 0xDC
    //( 00000000 )   10 00 60 00 00 00 00 00   DC 02 FF FF 00 00 10 00  ..`.....?....
    //[2023��07��28�� 21:43:09:642] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0xDC �汾 bb(5) �Ĵ���
    //[2023��07��28�� 21:43:09:659] ����(subcmd_handle_60.c 4003): δ֪ 0x60 ָ��: 0xDC
    //( 00000000 )   10 00 60 00 00 00 00 00   DC 02 FF FF 00 00 18 00  ..`.....?....
    //[2023��07��28�� 21:43:12:886] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0xDC �汾 bb(5) �Ĵ���
    //[2023��07��28�� 21:43:12:904] ����(subcmd_handle_60.c 4003): δ֪ 0x60 ָ��: 0xDC
    //( 00000000 )   10 00 60 00 00 00 00 00   DC 02 FF FF 03 00 03 00  ..`.....?....
    //[2023��07��28�� 21:43:18:319] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0xDC �汾 bb(5) �Ĵ���
    //[2023��07��28�� 21:43:18:336] ����(subcmd_handle_60.c 4003): δ֪ 0x60 ָ��: 0xDC
    //( 00000000 )   10 00 60 00 00 00 00 00   DC 02 FF FF 04 00 03 00  ..`.....?....
    //[2023��07��28�� 21:43:21:497] ����(subcmd_handle.c 0112): subcmd_get_handler δ��ɶ� 0x60 0xDC �汾 bb(5) �Ĵ���
    //[2023��07��28�� 21:43:21:513] ����(subcmd_handle_60.c 4003): δ֪ 0x60 ָ��: 0xDC
    //( 00000000 )   10 00 60 00 00 00 00 00   DC 02 FF FF 00 00 11 00  ..`.....?....

    return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
}

static int sub60_DE_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_good_luck_act_t* pkt) {
    lobby_t* l = src->cur_lobby;
    sfmt_t* rng = &src->sfmt_rng;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_good_luck_act_t), 0x02)) {
        return -2;
    }

    inventory_t* inv = get_client_inv_bb(src);

    uint32_t ci = 0;
    item_t item = { 0 };

    size_t itemid = find_iitem_code_stack_item_id(inv, 0x00031003);
    if (!itemid) {
        ERR_LOG("%s ��ұ�����û�����ܵ��", get_player_describe(src));
        return send_bb_item_exchange_good_luck(src, 0x00000001, pkt->subcmd_code, pkt->flags);
    }
//               00 01 02 03 04 05 06 07   08 09 0A 0B 0C 0D 0E 0F
//( 00000000 )   10 00 60 00 00 00 00 00   DE 02 00 00 02 6F 5F 00  ..`.....?...o_.

        /* ɾ������Ʊ�� 10�� ���ǻ�ȫ��ɾ�� */
    iitem_t remove_item = remove_iitem(src, itemid, 1, src->version != CLIENT_VERSION_BB);
    if (remove_item.data.datal[0] == 0 && remove_item.data.data2l == 0) {
        ERR_LOG("%s �����𻵵�����", get_player_describe(src));
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        return -3;
    }
    subcmd_send_lobby_bb_destroy_item(src, itemid, 1);

    subcmd_send_bb_exchange_item_in_quest(src, itemid, 1);

    memset(&item, 0, sizeof(item_t));
    item.datal[0] = good_luck[sfmt_genrand_uint32(rng) % (sizeof(good_luck) >> 2)];
    item.item_id = generate_item_id(l, src->client_id);
    iitem_t add_item = player_iitem_init(item);
    if (!add_iitem(src, add_item)) {
        ERR_LOG("%s ��ȡ�һ���Ʒʧ��!", get_player_describe(src));
        return -5;
    }
    subcmd_send_lobby_bb_create_inv_item(src, add_item.data, stack_size(&add_item.data), true);

    print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
    send_bb_item_exchange_good_luck(src, 0x00000000, pkt->subcmd_code, pkt->flags);
    return send_msg(src, MSG_BOX_TYPE, "%s", __(src, "��Ʒ�һ��ɹ�"));
}

static int sub60_E1_bb(ship_client_t* src, ship_client_t* dest,
    subcmd_bb_gallons_plan_t* pkt) {
    lobby_t* l = src->cur_lobby;

    if (!in_game(src))
        return -1;

    if (!check_pkt_size(src, pkt, sizeof(subcmd_bb_gallons_plan_t), 0x03)) {
        return -2;
    }

    inventory_t* inv = get_client_inv_bb(src);

    iitem_t remove_item = { 0 };
    item_t item = { 0 };

    size_t pt_itemid = find_iitem_code_stack_item_id(inv, 0x00041003);
    if (!pt_itemid) {
        ERR_LOG("%s !pt_itemid ��ұ�����û�й��ӵ��", get_player_describe(src));
        send_msg(src, MSG1_TYPE, __(src, "\tE\tC4���ı�����û�й��ӵ����!"));
        send_bb_item_exchange_gallon_result(src, 0x00000001, pkt->subcmd_code, pkt->unknown_a2);
        return 0;
    }

//[2023��08��27�� 00:55:19:592] ����(ship_packets.c 8294): GC 10000001 �������� quest035 (0 31)�汾 Blue Brust
//[2023��08��27�� 00:56:42:059] ��Ʒ(0290): ��Ʒ:(ID 8454144 / 00810000) ���ӵ��
//[2023��08��27�� 00:56:42:061] ��Ʒ(0295): ����: 03100400, 00630000, 00000000, 00000000
//     ������
//( 00000000 )   14 00 60 00 00 00 00 00   E1 03 00 00 3C 3D 02 00  ..`.....?..<=..
//( 00000010 )   87 00 06 00                                     ?..
//     ������
//( 00000000 )   14 00 60 00 00 00 00 00   E1 03 00 00 3C 3D 03 00  ..`.....?..<=..
//( 00000010 )   7B 00 06 00                                     {...
//     ����ͨ��
//( 00000000 )   14 00 60 00 00 00 00 00   E1 03 00 00 3C 3D 01 00  ..`.....?..<=..
//( 00000010 )   79 00 06 00                                     y...

    memset(&item, 0, sizeof(item_t));

    switch (pkt->exchange_choice) {
    case 0x0001:
        /* ɾ������Ʊ�� 10�� ���ǻ�ȫ��ɾ�� */
        remove_item = remove_iitem(src, pt_itemid, 99, src->version != CLIENT_VERSION_BB);
        if (remove_item.data.datal[0] == 0 && remove_item.data.data2l == 0) {
            ERR_LOG("%s �����𻵵�����", get_player_describe(src));
            print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
            return -3;
        }
        // ����ͨ��
        item.datal[0] = 0x0000D500;
        break;

    case 0x0002:
        /* ɾ������Ʊ�� 15�� ���ǻ�ȫ��ɾ�� */
        remove_item = remove_iitem(src, pt_itemid, 99, src->version != CLIENT_VERSION_BB);
        if (remove_item.data.datal[0] == 0 && remove_item.data.data2l == 0) {
            ERR_LOG("%s �����𻵵�����", get_player_describe(src));
            print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
            return -3;
        }
        // ������
        item.datal[0] = 0x00070A00;
        break;

    case 0x0003:
        /* ɾ������Ʊ�� 20�� ���ǻ�ȫ��ɾ�� */
        remove_item = remove_iitem(src, pt_itemid, 99, src->version != CLIENT_VERSION_BB);
        if (remove_item.data.datal[0] == 0 && remove_item.data.data2l == 0) {
            ERR_LOG("%s �����𻵵�����", get_player_describe(src));
            print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
            return -3;
        }
        // ������
        item.datal[0] = 0x00570101;
        break;

    default:
        ERR_LOG("%s �����𻵵�����", get_player_describe(src));
        print_ascii_hex(errl, pkt, pkt->hdr.pkt_len);
        return -4;
    }

    subcmd_send_lobby_bb_destroy_item(src, pt_itemid, 99);

    subcmd_send_bb_exchange_item_in_quest(src, pt_itemid, 0x05 + (pkt->unknown_a2 * 5));

    item.item_id = generate_item_id(l, src->client_id);
    iitem_t add_item = player_iitem_init(item);
    if (!add_iitem(src, add_item)) {
        ERR_LOG("%s ��ȡ�һ���Ʒʧ��!", get_player_describe(src));
        return -5;
    }

    subcmd_send_lobby_bb_create_inv_item(src, add_item.data, stack_size(&add_item.data), true);

    // Gallon's Plan result
    //send_bb_item_exchange_gallon_result(src, 0x00000000, pkt->subcmd_code, pkt->unknown_a2);

    return send_bb_item_exchange_gallon_result(src, 0x00000000, pkt->subcmd_code, pkt->unknown_a2);
}

// ���庯��ָ������
subcmd_handle_func_t subcmd60_handler[] = {
    //cmd_type 00 - 0F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_SWITCH_CHANGED             , sub60_05_dc, sub60_05_dc, NULL,        sub60_05_dc, sub60_05_dc, sub60_05_bb },
    { SUBCMD60_SYMBOL_CHAT                , sub60_07_dc, sub60_07_dc, NULL,        sub60_07_dc, sub60_07_dc, sub60_07_bb },
    { SUBCMD60_HIT_MONSTER                , sub60_0A_dc, sub60_0A_dc, NULL,        sub60_0A_dc, sub60_0A_dc, sub60_0A_bb },
    { SUBCMD60_HIT_OBJ                    , sub60_0B_dc, sub60_0B_dc, NULL,        sub60_0B_dc, sub60_0B_dc, sub60_0B_bb },
    { SUBCMD60_CONDITION_ADD              , sub60_0C_dc, sub60_0C_dc, NULL,        sub60_0C_dc, sub60_0C_dc, sub60_0C_bb },
    { SUBCMD60_CONDITION_REMOVE           , sub60_0D_dc, sub60_0D_dc, NULL,        sub60_0D_dc, sub60_0D_dc, sub60_0D_bb },

    //cmd_type 10 - 1F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_BOSS_ACT_DRAGON            , sub60_12_dc, sub60_12_dc, NULL,        sub60_12_dc, sub60_12_dc, sub60_12_bb },
    { SUBCMD60_BOSS_ACT_DE_ROl_LE         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_13_bb },
    { SUBCMD60_BOSS_ACT_DE_ROl_LE2        , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_14_bb },
    { SUBCMD60_BOSS_ACT_VOL_OPT           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_15_bb },
    { SUBCMD60_BOSS_ACT_VOL_OPT2          , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_16_bb },
    { SUBCMD60_TELEPORT                   , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_17_bb },
    { SUBCMD60_UNKNOW_18                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_18_bb },
    { SUBCMD60_BOSS_ACT_DARK_FALZ         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_19_bb },
    { SUBCMD60_CREATE_NPC                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_1B_bb },
    { SUBCMD60_DESTORY_NPC                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_1C_bb },
    { SUBCMD60_SET_AREA_1F                , sub60_1F_dc, sub60_1F_dc, NULL,        sub60_1F_dc, sub60_1F_dc, sub60_1F_bb },

    //cmd_type 20 - 2F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_SET_AREA_20                , sub60_20_dc, sub60_20_dc, NULL,        sub60_20_dc, sub60_20_dc, sub60_20_bb },
    { SUBCMD60_INTER_LEVEL_WARP           , sub60_21_dc, sub60_21_dc, NULL,        sub60_21_dc, sub60_21_dc, sub60_21_bb },
    { SUBCMD60_LOAD_22                    , sub60_22_dc, sub60_22_dc, NULL,        sub60_22_dc, sub60_22_dc, sub60_22_bb },
    { SUBCMD60_FINISH_LOAD                , sub60_23_dc, sub60_23_dc, NULL,        sub60_23_dc, sub60_23_dc, sub60_23_bb },
    { SUBCMD60_SET_POS_24                 , sub60_24_dc, sub60_24_dc, NULL,        NULL,        NULL,        sub60_24_bb },
    { SUBCMD60_EQUIP                      , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_25_bb },
    { SUBCMD60_REMOVE_EQUIP               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_26_bb },
    { SUBCMD60_ITEM_USE                   , sub60_27_dc, sub60_27_dc, NULL,        sub60_27_dc, sub60_27_dc, sub60_27_bb },
    { SUBCMD60_FEED_MAG                   , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_28_bb },
    { SUBCMD60_ITEM_DELETE                , sub60_29_dc, sub60_29_dc, NULL,        sub60_29_dc, sub60_29_dc, sub60_29_bb },
    { SUBCMD60_ITEM_DROP                  , sub60_2A_dc, sub60_2A_dc, NULL,        sub60_2A_dc, sub60_2A_dc, sub60_2A_bb },
    { SUBCMD60_ITEM_TAKE                  , sub60_2B_dc, sub60_2B_dc, NULL,        sub60_2B_dc, sub60_2B_dc, sub60_2B_bb },
    { SUBCMD60_SELECT_MENU                , sub60_2C_dc, sub60_2C_dc, NULL,        sub60_2C_dc, sub60_2C_dc, sub60_2C_bb },
    { SUBCMD60_SELECT_DONE                , sub60_2D_dc, sub60_2D_dc, NULL,        sub60_2D_dc, sub60_2D_dc, sub60_2D_bb },
    { SUBCMD60_HIT_BY_ENEMY               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_2F_bb },

    //cmd_type 30 - 3F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_LEVEL_UP                   , sub60_30_dc, sub60_30_dc, NULL,        sub60_30_dc, sub60_30_dc, NULL        },
    { SUBCMD60_MEDIC_REQ                  , sub60_31_dc, sub60_31_dc, NULL,        sub60_31_dc, sub60_31_dc, sub60_31_bb },
    { SUBCMD60_MEDIC_DONE                 , sub60_32_dc, sub60_32_dc, NULL,        sub60_32_dc, sub60_32_dc, sub60_32_bb },
    { SUBCMD60_REVIVE_PLAYER              , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_33_bb },
    { SUBCMD60_PB_BLAST                   , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_37_bb },
    { SUBCMD60_PB_BLAST_READY             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_39_bb },
    { SUBCMD60_GAME_CLIENT_LEAVE          , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_3A_bb },
    { SUBCMD60_LOAD_3B                    , sub60_3B_dc, sub60_3B_dc, NULL,        sub60_3B_dc, sub60_3B_dc, sub60_3B_bb },
    { SUBCMD60_SET_POS_3E                 , sub60_3E_dc, sub60_3E_dc, NULL,        sub60_3E_dc, sub60_3E_dc, sub60_3E_bb },
    { SUBCMD60_SET_POS_3F                 , sub60_3F_dc, sub60_3F_dc, NULL,        sub60_3F_dc, sub60_3F_dc, sub60_3F_bb },

    //cmd_type 40 - 4F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_MOVE_SLOW                  , sub60_40_dc, sub60_40_dc, NULL,        sub60_40_dc, sub60_40_dc, sub60_40_bb },
    { SUBCMD60_MOVE_FAST                  , sub60_42_dc, sub60_42_dc, NULL,        sub60_42_dc, sub60_42_dc, sub60_42_bb },
    { SUBCMD60_ATTACK1                    , sub60_43_dc, sub60_43_dc, NULL,        sub60_43_dc, sub60_43_dc, sub60_43_bb },
    { SUBCMD60_ATTACK2                    , sub60_44_dc, sub60_44_dc, NULL,        sub60_44_dc, sub60_44_dc, sub60_44_bb },
    { SUBCMD60_ATTACK3                    , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_45_bb },
    { SUBCMD60_OBJHIT_PHYS                , sub60_46_dc, sub60_46_dc, NULL,        sub60_46_dc, sub60_46_dc, sub60_46_bb },
    { SUBCMD60_OBJHIT_TECH                , sub60_47_dc, sub60_47_dc, NULL,        sub60_47_dc, sub60_47_dc, sub60_47_bb },
    { SUBCMD60_USED_TECH                  , sub60_48_dc, sub60_48_dc, NULL,        sub60_48_dc, sub60_48_dc, sub60_48_bb },
    { SUBCMD60_SUBTRACT_PB_ENERGY         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_49_bb },
    { SUBCMD60_DEFENSE_DAMAGE             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_4A_bb },
    { SUBCMD60_TAKE_DAMAGE1               , sub60_4B_dc, sub60_4B_dc, NULL,        sub60_4B_dc, sub60_4B_dc, sub60_4B_bb },
    { SUBCMD60_TAKE_DAMAGE2               , sub60_4C_dc, sub60_4C_dc, NULL,        sub60_4C_dc, sub60_4C_dc, sub60_4C_bb },
    { SUBCMD60_DEATH_SYNC                 , sub60_4D_dc, sub60_4D_dc, NULL,        sub60_4D_dc, sub60_4D_dc, sub60_4D_bb },
    { SUBCMD60_UNKNOW_4E                  , sub60_4E_dc, sub60_4E_dc, NULL,        sub60_4E_dc, sub60_4E_dc, sub60_4E_bb },
    { SUBCMD60_PLAYER_SAVED               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_4F_bb },

    //cmd_type 50 - 5F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_SWITCH_REQ                 , sub60_50_dc, sub60_50_dc, NULL,        sub60_50_dc, sub60_50_dc, sub60_50_bb },
    { SUBCMD60_MENU_REQ                   , sub60_52_dc, sub60_52_dc, NULL,        sub60_52_dc, sub60_52_dc, sub60_52_bb },
    { SUBCMD60_UNKNOW_53                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_53_bb },
    { SUBCMD60_WARP_55                    , sub60_55_dc, sub60_55_dc, NULL,        sub60_55_dc, sub60_55_dc, sub60_55_bb },
    { SUBCMD60_LOBBY_ACT                  , sub60_58_dc, sub60_58_dc, NULL,        sub60_58_dc, sub60_58_dc, sub60_58_bb },
    { SUBCMD60_ITEM_DELETE_IN_MAP         , sub60_59_dc, sub60_59_dc, NULL,        sub60_59_dc, sub60_59_dc, NULL        },
    { SUBCMD60_PICK_UP_ITEM               , sub60_5A_dc, sub60_5A_dc, NULL,        sub60_5A_dc, sub60_5A_dc, NULL        },
    { SUBCMD60_DROP_STACK                 , sub60_5D_dc, sub60_5D_dc, NULL,        sub60_5D_dc, sub60_5D_dc, NULL        },
    { SUBCMD60_BUY                        , sub60_5E_dc, sub60_5E_dc, NULL,        sub60_5E_dc, sub60_5E_dc, NULL        },
    { SUBCMD60_ITEM_DROP_BOX_ENEMY        , sub60_5F_dc, sub60_5F_dc, NULL,        sub60_5F_dc, sub60_5F_dc, NULL        },

    //cmd_type 60 - 6F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_LEVEL_UP_REQ               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_61_bb },
    { SUBCMD60_ITEM_GROUND_DESTROY        , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_63_bb },
    { SUBCMD60_USE_STAR_ATOMIZER          , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_66_bb },
    { SUBCMD60_CREATE_ENEMY_SET           , sub60_67_dc, sub60_67_dc, NULL,        sub60_67_dc, sub60_67_dc, sub60_67_bb },
    { SUBCMD60_CREATE_PIPE                , sub60_68_dc, sub60_68_dc, NULL,        sub60_68_dc, sub60_68_dc, sub60_68_bb },
    { SUBCMD60_SPAWN_NPC                  , sub60_69_dc, sub60_69_dc, NULL,        sub60_69_dc, sub60_69_dc, sub60_69_bb },
    { SUBCMD60_UNKNOW_6A                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_6A_bb },

    //cmd_type 70 - 7F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_BURST_DONE                 , sub60_72_dc, sub60_72_dc, NULL,        sub60_72_dc, sub60_72_dc, sub60_72_bb },
    { SUBCMD60_WORD_SELECT                , sub60_74_dc, sub60_74_dc, NULL,        sub60_74_dc, sub60_74_dc, sub60_74_bb },
    { SUBCMD60_FLAG_SET                   , sub60_75_dc, sub60_75_dc, NULL,        sub60_75_dc, sub60_75_dc, sub60_75_bb },
    { SUBCMD60_KILL_MONSTER               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_76_bb },
    { SUBCMD60_SYNC_REG                   , sub60_77_dc, sub60_77_dc, NULL,        sub60_77_dc, sub60_77_dc, sub60_77_bb },
    { SUBCMD60_GOGO_BALL                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_79_bb },
    { SUBCMD60_UNKNOW_7A                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_7A_bb },
    { SUBCMD60_UNKNOW_7B                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_7B_bb },
    { SUBCMD60_SET_C_GAME_MODE            , sub60_7C_dc, sub60_7C_dc, NULL,        sub60_7C_dc, sub60_7C_dc, sub60_7C_bb },
    { SUBCMD60_SYNC_BATTLE_MODE_DATA      , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_7D_bb },

    //cmd_type 80 - 8F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_TRIGGER_TRAP               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_80_bb },
    { SUBCMD60_PLACE_TRAP                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_83_bb },
    { SUBCMD60_BOSS_ACT_OFP_U_SP          , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_84_bb },
    { SUBCMD60_UNKNOW_85                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_85_bb },
    { SUBCMD60_HIT_DESTRUCTIBLE_OBJECT    , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_86_bb },
    { SUBCMD60_ARROW_CHANGE               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_88_bb },
    { SUBCMD60_PLAYER_DIED                , sub60_89_dc, sub60_89_dc, NULL,        sub60_89_dc, sub60_89_dc, sub60_89_bb },
    { SUBCMD60_CH_GAME_SELECT             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_8A_bb },
    { SUBCMD60_OVERRIDE_TECH_LEVEL        , sub60_8D_dc, sub60_8D_dc, NULL,        sub60_8D_dc, sub60_8D_dc, sub60_8D_bb },
    { SUBCMD60_BATTLE_MODE_PLAYER_HIT     , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_8F_bb },

    //cmd_type 90 - 9F                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_UNKNOW_91                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_91_bb },
    { SUBCMD60_UNKNOW_92                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_92_bb },
    { SUBCMD60_TIMED_SWITCH_ACTIVATED     , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_93_bb },
    { SUBCMD60_CH_GAME_CANCEL             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_97_bb },
    { SUBCMD60_CHANGE_STAT                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_9A_bb },
    { SUBCMD60_BATTLE_MODE_PLAYER_DIE     , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_9B_bb },
    { SUBCMD60_UNKNOW_9C                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_9C_bb },
    { SUBCMD60_CH_GAME_FINISHED           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_9D_bb },
    { SUBCMD60_BOSS_ACT_GAL_GRYPHON       , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_9F_bb },

    //cmd_type A0 - AF                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_BOSS_ACT_GAL_GRYPHON_SP    , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_A0_bb },
    { SUBCMD60_PLAYER_ACT_SAVE            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_A1_bb },
    { SUBCMD60_BOSS_ACT_OFB               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_A3_bb },
    { SUBCMD60_BOSS_ACT_OFP_1             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_A4_bb },
    { SUBCMD60_BOSS_ACT_OFP_2             , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_A5_bb },
    { SUBCMD60_BOSS_ACT_GDRAGON           , sub60_A8_dc, sub60_A8_dc, NULL,        sub60_A8_dc, sub60_A8_dc, sub60_A8_bb },
    { SUBCMD60_BOSS_ACT_BARBA_RAY         , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_A9_bb },
    { SUBCMD60_BOSS_ACT_EP2               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_AA_bb },
    { SUBCMD60_CHAIR_CREATE               , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_AB_bb },
    { SUBCMD60_BOSS_ACT_OFP_2_SP          , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_AD_bb },
    { SUBCMD60_CHAIR_TURN                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_AF_bb },

    //cmd_type B0 - BF                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_CHAIR_MOVE                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_B0_bb },

    //cmd_type C0 - CF                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_ITEM_SELL                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C0_bb },
    { SUBCMD60_ITEM_DROP_SPLIT            , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C3_bb },
    { SUBCMD60_SORT_INV                   , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C4_bb },
    { SUBCMD60_MEDIC                      , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C5_bb },
    { SUBCMD60_STEAL_EXP                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C6_bb },
    { SUBCMD60_CHARGE_ACT                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C7_bb },
    { SUBCMD60_EXP_REQ                    , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_C8_bb },
    { SUBCMD60_ITEM_EXCHANGE_GUILD        , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_CC_bb },
    { SUBCMD60_START_BATTLE_MODE          , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_CF_bb },

    //cmd_type D0 - DF                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_GALLON_AREA                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_D2_bb },
    { SUBCMD60_ITEM_EXCHANGE_PD           , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_D7_bb },
    { SUBCMD60_SRANK_ATTR                 , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_D8_bb },
    { SUBCMD60_ITEM_EXCHANGE_MOMOKA       , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_D9_bb },
    { SUBCMD60_BOSS_ACT_SAINT_MILLION     , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_DC_bb },
    { SUBCMD60_GOOD_LUCK                  , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_DE_bb },

    //cmd_type E0 - EF                      DC           GC           EP3          XBOX         PC           BB
    { SUBCMD60_GALLON_PLAN                , NULL,        NULL,        NULL,        NULL,        NULL,        sub60_E1_bb },
};

/* ���� DC GC PC V1 V2 0x60 ���Կͻ��˵����ݰ�. */
int subcmd_handle_60(ship_client_t* src, subcmd_pkt_t* pkt) {
    __try {
        uint8_t type = pkt->type;
        lobby_t* l = src->cur_lobby;
        int rv, sent = 1, i = 0;
        ship_client_t* dest;
        uint16_t len = pkt->hdr.dc.pkt_len, hdr_type = pkt->hdr.dc.pkt_type;
        uint8_t dnum = pkt->hdr.dc.flags;

        /* The DC NTE must be treated specially, so deal with that elsewhere... */
        if (src->version == CLIENT_VERSION_DCV1 && (src->flags & CLIENT_FLAG_IS_NTE))
            return subcmd_dcnte_handle_bcast(src, pkt);

        /* ����ͻ��˲��ڴ������߶�������������ݰ�. */
        if (!l)
            return 0;

        pthread_mutex_lock(&l->mutex);

        /* ����Ŀ��ͻ���. */
        dest = l->clients[dnum];

#ifdef DEBUG

        DBG_LOG("0x%02X ָ��: 0x%02X", pkt->hdr.dc.pkt_type, type);
        DBG_LOG("c version %d", c->version);

        print_ascii_hex(errl, pkt, pkt->hdr.dc.pkt_len);

#endif // DEBUG

        l->subcmd_handle = subcmd_get_handler(hdr_type, type, src->version);

        /* If there's a burst going on in the lobby, delay most packets */
        if (l->flags & LOBBY_FLAG_BURSTING) {
            switch (type) {
            case SUBCMD60_SET_POS_3F://����ԾǨʱ���� 1
            case SUBCMD60_SET_AREA_1F://����ԾǨʱ���� 2
            case SUBCMD60_LOAD_3B://����ԾǨʱ���� 3
            case SUBCMD60_BURST_DONE:
                /* 0x7C ��սģʽ ���뷿����Ϸδ��ʼǰ����*/
            case SUBCMD60_SET_C_GAME_MODE:
                rv = l->subcmd_handle(src, dest, pkt);
                break;

            default:
                rv = lobby_enqueue_pkt(l, src, (dc_pkt_hdr_t*)pkt);
            }
        }
        else {
            //[2023��08��27�� 22:09:58:225] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0x07 �汾 gc(3) �Ĵ���
            //[2023��08��27�� 22:09:58:236] ����(subcmd_handle_dcgcpcv1v2.c 2088): δ֪ 0x60 ָ��: 0x07
            //( 00000000 )   60 00 48 00 07 11 C0 00   00 00 00 00 28 00 00 00  `.H...?....(...
            //( 00000010 )   FF FF 0D 00 FF FF FF FF   05 18 1D 00 05 28 1D 01  .......(..
            //( 00000020 )   36 20 2A 00 3C 00 32 00   FF 00 00 00 FF 00 00 00  6 *.<.2.......
            //( 00000030 )   FF 00 00 00 FF 00 00 02   FF 00 00 02 FF 00 00 02  ............
            //( 00000040 )   FF 00 00 02 FF 00 00 02                           ......
            //[2023��08��27�� 22:15:33:238] ����(subcmd_handle.c 0113): subcmd_get_handler δ��ɶ� 0x60 0x87 �汾 bb(5) �Ĵ���
            //[2023��08��27�� 22:15:33:249] ����(subcmd_handle_60.c 6180): δ֪ 0x60 ָ��: 0x87
            //( 00000000 )   10 00 60 00 00 00 00 00   87 02 00 00 CD CC CC 3E  ..`.....?..��?
            if (l->subcmd_handle == NULL) {
#ifdef BB_LOG_UNKNOWN_SUBS
                DBG_LOG("δ֪ %s 0x%02X ָ��: 0x%02X", client_type[src->version].ver_name, hdr_type, type);
                print_ascii_hex(dbgl, pkt, len);
#endif /* BB_LOG_UNKNOWN_SUBS */
                rv = subcmd_send_lobby_dc(l, src, (subcmd_pkt_t*)pkt, 0);
            }
            else {
                rv = l->subcmd_handle(src, dest, pkt);
            }
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

/* ����BB 0x60 ���ݰ�. */
int subcmd_bb_handle_60(ship_client_t* src, subcmd_bb_pkt_t* pkt) {
    __try {
        lobby_t* l = src->cur_lobby;
        ship_client_t* dest;
        uint16_t len = pkt->hdr.pkt_len;
        uint16_t hdr_type = pkt->hdr.pkt_type;
        uint8_t type = pkt->type;
        int rv = 0, sent = 1, i = 0;
        uint32_t dnum = LE32(pkt->hdr.flags);

        /* ����ͻ��˲��ڴ������߶�������������ݰ�. */
        if (!l)
            return 0;

#ifdef DEBUG_60

        DBG_LOG("��� 0x%02X ָ��: 0x%02X", hdr_type, type);

        if (src->mode)
            DBG_LOG("%s CH 0x%04X 60ָ��: 0x%02X", get_player_describe(src), hdr_type, type);

#endif // DEBUG_60

        pthread_mutex_lock(&l->mutex);

        /* ����Ŀ��ͻ���. */
        dest = l->clients[dnum];

        /* TODO Ŀ��ͻ��������ߣ������ٷ������ݰ�. 60��Ҫ̽���Ƿ��������ͻ��� �����ǵ����ĸ�����һ������ */
        //if (!dest) {
        //    //DBG_LOG("������ dest ��� 0x%02X ָ��: 0x%X", hdr_type, type);
        //    //pthread_mutex_unlock(&l->mutex);
        //    return 0;
        //}

        //subcmd_bb_60size_check(src, pkt);

        l->subcmd_handle = subcmd_get_handler(hdr_type, type, src->version);

        /* If there's a burst going on in the lobby, delay most packets */
        if (l->flags & LOBBY_FLAG_BURSTING) {
            switch (type) {
            case SUBCMD60_SET_POS_3F://����ԾǨʱ���� 1
            case SUBCMD60_SET_AREA_1F://����ԾǨʱ���� 2
            case SUBCMD60_LOAD_3B://����ԾǨʱ���� 3
            case SUBCMD60_BURST_DONE:
                /* 0x7C ��սģʽ ���뷿����Ϸδ��ʼǰ����*/
            case SUBCMD60_SET_C_GAME_MODE:
                rv = l->subcmd_handle(src, dest, pkt);
                break;

            default:
#ifdef DEBUG

                DBG_LOG("lobby_enqueue_pkt_bb 0x60 ָ��: 0x%02X", type);

#endif // DEBUG
                rv = lobby_enqueue_pkt_bb(l, src, (bb_pkt_hdr_t*)pkt);
            }
        }
        else {
            if (l->subcmd_handle == NULL) {
#ifdef BB_LOG_UNKNOWN_SUBS
                DBG_LOG("δ֪ %s 0x%02X ָ��: 0x%02X", client_type[src->version].ver_name, hdr_type, type);
                print_ascii_hex(dbgl, pkt, len);
#endif /* BB_LOG_UNKNOWN_SUBS */
                rv = subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)pkt, 0);
            }
            else {
                rv = l->subcmd_handle(src, dest, pkt);
            }
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
