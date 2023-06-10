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
#include "subcmd_send.h"
#include "shop.h"
#include "pmtdata.h"
#include "clients.h"
#include "ship_packets.h"
#include "utils.h"
#include "items.h"
#include "word_select.h"
#include "scripts.h"
#include "shipgate.h"
#include "quest_functions.h"
#include "mag_bb.h"
#include "subcmd-bb.c"

// 当客户端发送游戏命令时, 调用此文件中的函数
// 指令集
// (60, 62, 6C, 6D, C9, CB).

/* 原始 62 6D 指令函数 */
int subcmd_bb_handle_onev1(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* dest;
    uint8_t type = pkt->type;
    int rv = -1;
    uint32_t dnum = LE32(pkt->hdr.flags);

    subcmd_bb_626Dsize_check(c, pkt);

    /* 如果客户端不在大厅或者队伍中则忽略数据包. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    /* Find the destination. */
    dest = l->clients[dnum];

    /* The destination is now offline, don't bother sending it. */
    if (!dest) {
        pthread_mutex_unlock(&l->mutex);
        return 0;
    }

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;

        switch (type) {
        case SUBCMD62_BURST1://6D
        case SUBCMD62_BURST2://6B
        case SUBCMD62_BURST3://6C
        case SUBCMD62_BURST4://6E
            if (l->flags & LOBBY_FLAG_QUESTING)
                rv = lobby_enqueue_burst_bb(l, c, (bb_pkt_hdr_t*)pkt);
            /* Fall through... */

        case SUBCMD62_BURST5://6F
        case SUBCMD62_BURST6://71
            //case SUBCMD62_BURST_PLDATA://70
            rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            break;

        case SUBCMD62_BURST_PLDATA://70
            rv = handle_bb_burst_pldata(c, dest, (subcmd_bb_burst_pldata_t*)pkt);
            break;

        default:
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {
    case SUBCMD62_GUILDCARD:
        /* Make sure the recipient is not ignoring the sender... */
        if (client_has_ignored(dest, c->guildcard)) {
            rv = 0;
            break;
        }

        rv = handle_bb_gcsend(c, dest);
        break;

    case SUBCMD62_PICK_UP:
        rv = handle_bb_pick_up(c, (subcmd_bb_pick_up_t*)pkt);
        break;

    case SUBCMD62_SHOP_REQ:
        rv = handle_bb_shop_req(c, (subcmd_bb_shop_req_t*)pkt);
        break;

    case SUBCMD62_OPEN_BANK:
        rv = handle_bb_bank(c, (subcmd_bb_bank_open_t*)pkt);
        break;

    case SUBCMD62_BANK_ACTION:
        rv = handle_bb_bank_action(c, (subcmd_bb_bank_act_t*)pkt);
        break;

    case SUBCMD62_ITEMREQ:
    case SUBCMD62_BITEMREQ:
        /* Unlike earlier versions, we have to handle this here... */
        rv = l->dropfunc(c, l, pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x62 指令: 0x%02X", type);
        print_payload((unsigned char*)pkt, LE16(pkt->hdr.pkt_len));
#endif /* BB_LOG_UNKNOWN_SUBS */
        /* Forward the packet unchanged to the destination. */
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

/* 原始 60 指令函数 */
int subcmd_bb_handle_bcastv1(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv, sent = 1, i;

    /* 如果客户端不在大厅或者队伍中则忽略数据包. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    switch (type) {
    case SUBCMD60_SYMBOL_CHAT:
        rv = handle_bb_symbol_chat(c, (subcmd_bb_symbol_chat_t*)pkt);
        break;

    case SUBCMD60_HIT_MONSTER:
        rv = handle_bb_mhit(c, (subcmd_bb_mhit_pkt_t*)pkt);
        break;

    case SUBCMD60_SET_AREA_1F:
    case SUBCMD60_INTER_LEVEL_WARP:
        rv = handle_bb_set_area_1F(c, (subcmd_bb_set_area_1F_t*)pkt);
        break;

    case SUBCMD60_EQUIP:
        rv = handle_bb_equip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD60_REMOVE_EQUIP:
        rv = handle_bb_unequip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD60_DROP_ITEM:
        rv = handle_bb_drop_item(c, (subcmd_bb_drop_item_t*)pkt);
        break;

    case SUBCMD60_SET_POS_3E:
    case SUBCMD60_SET_POS_3F:
        rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
        break;

    case SUBCMD60_MOVE_SLOW:
    case SUBCMD60_MOVE_FAST:
        rv = handle_bb_move(c, (subcmd_bb_move_t*)pkt);
        break;

    case SUBCMD60_DELETE_ITEM:
        rv = handle_bb_destroy_item(c, (subcmd_bb_destroy_item_t*)pkt);
        break;

    case SUBCMD60_WORD_SELECT:
        rv = handle_bb_word_select(c, (subcmd_bb_word_select_t*)pkt);
        break;

    case SUBCMD60_DROP_POS:
        rv = handle_bb_drop_pos(c, (subcmd_bb_drop_pos_t*)pkt);
        break;

    case SUBCMD60_SORT_INV:
        rv = handle_bb_sort_inv(c, (subcmd_bb_sort_inv_t*)pkt);
        break;

    case SUBCMD60_MEDIC:
        rv = handle_bb_medic(c, (subcmd_bb_pkt_t*)pkt);
        break;

    case SUBCMD60_REQ_EXP:
        rv = handle_bb_req_exp(c, (subcmd_bb_req_exp_t*)pkt);
        break;

    case SUBCMD60_USED_TECH:
        rv = handle_bb_used_tech(c, (subcmd_bb_used_tech_t*)pkt);
        break;

    case SUBCMD60_TAKE_DAMAGE1:
    case SUBCMD60_TAKE_DAMAGE2:
        rv = handle_bb_take_damage(c, (subcmd_bb_take_damage_t*)pkt);
        break;

    case SUBCMD60_SPAWN_NPC:
        rv = handle_bb_spawn_npc(c, pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("Unknown 0x60: 0x%02X\n", type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
#endif /* BB_LOG_UNKNOWN_SUBS */
        sent = 0;
        break;

    case SUBCMD60_FINISH_LOAD:
        if (l->type == LOBBY_TYPE_LOBBY) {
            for (i = 0; i < l->max_clients; ++i) {
                if (l->clients[i] && l->clients[i] != c &&
                    subcmd_send_pos(c, l->clients[i])) {
                    rv = -1;
                    break;
                }
            }
        }

    case SUBCMD60_LOAD_22:
    case SUBCMD60_TALK_NPC:
    case SUBCMD60_DONE_NPC:
    case SUBCMD60_LOAD_3B:
    case SUBCMD60_MENU_REQ:
    case SUBCMD60_WARP_55:
    case SUBCMD60_LOBBY_ACTION:
    case SUBCMD60_GOGO_BALL:
    case SUBCMD60_CHAIR_CREATE:
    case SUBCMD60_CHAIR_TURN:
    case SUBCMD60_CHAIR_MOVE:
        sent = 0;
    }

    /* Broadcast anything we don't care to check anything about. */
    if (!sent)
        rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}


//弃用
static int handle_bb_chair_dir(ship_client_t* c, subcmd_bb_create_lobby_chair_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " 在游戏中触发了大厅房间指令!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02 || c->client_id != pkt->shdr.client_id) {
        ERR_LOG("GC %" PRIu32 " 发送损坏的数据! 0x%02X",
            c->guildcard, pkt->shdr.type);
        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

/* 处理BB 0x60 数据包. */
int subcmd_bb_handle_bcastv2(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0, sent = 1, i;

    /* Ignore these if the client isn't in a lobby. */
    if (!l)
        return 0;

    pthread_mutex_lock(&l->mutex);

    subcmd_bb_60size_check(c, pkt);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        switch (type) {
        case SUBCMD60_LOAD_3B:
        case SUBCMD60_BURST_DONE:
            /* TODO 将这个函数融合进房间函数中 */
            subcmd_send_bb_set_exp_rate(c, 3000);
            send_lobby_pkt(l, NULL, build_guild_full_data_pkt(c), 1);
            rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
            break;

        case SUBCMD60_SET_AREA_1F:
            rv = handle_bb_set_area_1F(c, (subcmd_bb_set_area_1F_t*)pkt);
            break;

        case SUBCMD60_SET_AREA_20:
            rv = handle_bb_set_area_20(c, (subcmd_bb_set_area_20_t*)pkt);
            break;

        case SUBCMD60_SET_POS_3F:
            rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
            break;

        case SUBCMD60_CMODE_GRAVE:
            rv = handle_bb_cmode_grave(c, pkt);
            break;

        default:
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {

    case SUBCMD60_SRANK_ATTR:
        sent = 0;
        break;

    case SUBCMD60_EX_ITEM_MK:
        sent = 0;
        break;

    case SUBCMD60_SWITCH_CHANGED:
        rv = handle_bb_switch_changed(c, (subcmd_bb_switch_changed_pkt_t*)pkt);
        break;

    case SUBCMD60_SYMBOL_CHAT: //间隔1秒可以正常发送
        if (time_check(c->subcmd_cooldown[type], 1))
            rv = handle_bb_symbol_chat(c, (subcmd_bb_symbol_chat_t*)pkt);
        else
            sent = 1;
        break;

    case SUBCMD60_HIT_MONSTER:
        rv = handle_bb_mhit(c, (subcmd_bb_mhit_pkt_t*)pkt);
        break;

    case SUBCMD60_HIT_OBJ:
        rv = handle_bb_objhit(c, (subcmd_bb_bhit_pkt_t*)pkt);
        break;

    case SUBCMD60_CONDITION_ADD:
        rv = handle_bb_condition(c, (subcmd_bb_add_or_remove_condition_t*)pkt);
        break;

    case SUBCMD60_CONDITION_REMOVE:
        rv = handle_bb_condition(c, (subcmd_bb_add_or_remove_condition_t*)pkt);
        break;

    case SUBCMD60_DRAGON_ACT:// Dragon actions
        rv = handle_bb_dragon_act(c, (subcmd_bb_dragon_act_t*)pkt);
        break;

    case SUBCMD60_ACTION_DE_ROl_LE:// De Rol Le actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_ACTION_DE_ROl_LE2:// De Rol Le actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_ACTION_VOL_OPT:// Vol Opt actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_ACTION_VOL_OPT2:// Vol Opt actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_TELEPORT:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_UNKNOW_18:// Dragon special actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_ACTION_DARK_FALZ:// Dark Falz actions
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_UNKNOW_1C:
        rv = handle_bb_cmd_check_game_size(c, pkt);
        break;

    case SUBCMD60_SET_AREA_1F:
        rv = handle_bb_set_area_1F(c, (subcmd_bb_set_area_1F_t*)pkt);
        break;

    case SUBCMD60_SET_AREA_20:
        rv = handle_bb_set_area_20(c, (subcmd_bb_set_area_20_t*)pkt);
        break;

    case SUBCMD60_INTER_LEVEL_WARP:
        rv = handle_bb_inter_level_warp(c, (subcmd_bb_inter_level_warp_t*)pkt);
        break;

    case SUBCMD60_LOAD_22://subcmd_set_player_visibility_6x22_6x23_t
        rv = handle_bb_load_22(c, (subcmd_bb_set_player_visibility_6x22_6x23_t*)pkt);
        break;

    case SUBCMD60_SET_POS_24:
        rv = handle_bb_set_pos_0x24(c, (subcmd_bb_set_pos_0x24_t*)pkt);
        break;

    case SUBCMD60_EQUIP:
        rv = handle_bb_equip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD60_REMOVE_EQUIP:
        rv = handle_bb_unequip(c, (subcmd_bb_equip_t*)pkt);
        break;

    case SUBCMD60_USE_ITEM:
        rv = handle_bb_use_item(c, (subcmd_bb_use_item_t*)pkt);
        break;

    case SUBCMD60_FEED_MAG:
        rv = handle_bb_feed_mag(c, (subcmd_bb_feed_mag_t*)pkt);
        break;

    case SUBCMD60_DELETE_ITEM:
        rv = handle_bb_destroy_item(c, (subcmd_bb_destroy_item_t*)pkt);
        break;

    case SUBCMD60_DROP_ITEM:
        rv = handle_bb_drop_item(c, (subcmd_bb_drop_item_t*)pkt);
        break;

    case SUBCMD60_TAKE_ITEM:
        rv = handle_bb_take_item(c, (subcmd_bb_take_item_t*)pkt);
        break;

    case SUBCMD60_TALK_NPC:
        rv = handle_bb_talk_npc(c, (subcmd_bb_talk_npc_t*)pkt);
        break;

    case SUBCMD60_DONE_NPC:
        rv = handle_bb_done_talk_npc(c, (subcmd_bb_end_talk_to_npc_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_2E:
        sent = 0;
        break;

    case SUBCMD60_HIT_BY_ENEMY:
        rv = handle_bb_hit_by_enemy(c, (subcmd_bb_hit_by_enemy_t*)pkt);
        break;

    case SUBCMD60_LEVEL_UP:
        rv = handle_bb_level_up(c, (subcmd_bb_level_up_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_MEDIC_31:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_UNKNOW_MEDIC_32:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_UNKNOW_33:
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_34:
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_35:
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_36:
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_37:
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_38:
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_39:
        sent = 0;
        break;

    case SUBCMD60_UNKNOW_3A:
        rv = handle_bb_cmd_3a(c, (subcmd_bb_cmd_3a_t*)pkt);
        break;

    case SUBCMD60_LOAD_3B:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_UNKNOW_3C:
    case SUBCMD60_UNKNOW_3D:
        sent = 0;
        break;

    case SUBCMD60_SET_POS_3E:
    case SUBCMD60_SET_POS_3F:
        rv = handle_bb_set_pos(c, (subcmd_bb_set_pos_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_41:
        sent = 0;
        break;

    case SUBCMD60_MOVE_SLOW:
    case SUBCMD60_MOVE_FAST:
        rv = handle_bb_move(c, (subcmd_bb_move_t*)pkt);
        break;

    case SUBCMD60_ATTACK1:
    case SUBCMD60_ATTACK2:
    case SUBCMD60_ATTACK3:
        rv = handle_bb_normal_attack(c, (subcmd_bb_natk_t*)pkt);
        break;

    case SUBCMD60_OBJHIT_PHYS:
        rv = handle_bb_objhit_phys(c, (subcmd_bb_objhit_phys_t*)pkt);
        break;

    case SUBCMD60_OBJHIT_TECH:
        rv = handle_bb_objhit_tech(c, (subcmd_bb_objhit_tech_t*)pkt);
        break;

    case SUBCMD60_USED_TECH:
        rv = handle_bb_used_tech(c, (subcmd_bb_used_tech_t*)pkt);
        break;

    case SUBCMD60_DEFENSE_DAMAGE:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x01);
        break;

    case SUBCMD60_DEATH_SYNC:
        rv = handle_bb_death_sync(c, (subcmd_bb_death_sync_t*)pkt);
        break;

    case SUBCMD60_UNKNOW_4E:
        rv = handle_bb_cmd_4e(c, (subcmd_bb_cmd_4e_t*)pkt);
        break;

    case SUBCMD60_REQ_SWITCH:
        rv = handle_bb_switch_req(c, (subcmd_bb_switch_req_t*)pkt);
        break;

    case SUBCMD60_WORD_SELECT:
        rv = handle_bb_word_select(c, (subcmd_bb_word_select_t*)pkt);
        break;

    case SUBCMD60_ARROW_CHANGE:
        rv = handle_bb_arrow_change(c, (subcmd_bb_arrow_change_t*)pkt);
        break;

    case SUBCMD60_PLAYER_DIED:
        rv = handle_bb_player_died(c, (subcmd_bb_player_died_t*)pkt);
        break;

    case SUBCMD60_SELL_ITEM:
        rv = handle_bb_sell_item(c, (subcmd_bb_sell_item_t*)pkt);
        break;

    case SUBCMD60_DROP_POS:
        rv = handle_bb_drop_pos(c, (subcmd_bb_drop_pos_t*)pkt);
        break;

    case SUBCMD60_SORT_INV:
        rv = handle_bb_sort_inv(c, (subcmd_bb_sort_inv_t*)pkt);
        break;

    case SUBCMD60_MEDIC:
        rv = handle_bb_medic(c, (subcmd_bb_pkt_t*)pkt);
        break;

    case SUBCMD60_STEAL_EXP:
        sent = 0;
        break;

    case SUBCMD60_CHARGE_ACT: // Charge action 充能动作
        rv = handle_bb_charge_act(c, (subcmd_bb_charge_act_t*)pkt);
        break;

    case SUBCMD60_REQ_EXP:
        rv = handle_bb_req_exp(c, (subcmd_bb_req_exp_t*)pkt);
        break;

    case SUBCMD60_EX_ITEM_TEAM:
        sent = 0;
        break;

    case SUBCMD60_BATTLEMODE:
        sent = 0;
        break;

    case SUBCMD60_GALLON_AREA:
        rv = handle_bb_gallon_area(c, (subcmd_bb_gallon_area_pkt_t*)pkt);
        break;

    case SUBCMD60_TAKE_DAMAGE1:
    case SUBCMD60_TAKE_DAMAGE2:
        rv = handle_bb_take_damage(c, (subcmd_bb_take_damage_t*)pkt);
        break;

    case SUBCMD60_MENU_REQ:
        rv = handle_bb_menu_req(c, (subcmd_bb_menu_req_t*)pkt);
        break;

    case SUBCMD60_CREATE_ENEMY_SET:
        rv = handle_bb_cmd_check_lobby_size(c, pkt, 0x04);
        break;

    case SUBCMD60_CREATE_PIPE:
        rv = handle_bb_create_pipe(c, (subcmd_bb_pipe_t*)pkt);
        break;

    case SUBCMD60_SPAWN_NPC:
        rv = handle_bb_spawn_npc(c, pkt);
        break;

    case SUBCMD60_UNKNOW_6A:
        rv = handle_bb_subcmd_6a(c, (subcmd_bb_Unknown_6x6A_t*)pkt);
        break;

    case SUBCMD60_SET_FLAG:
        rv = handle_bb_set_flag(c, (subcmd_bb_set_flag_t*)pkt);
        break;

    case SUBCMD60_KILL_MONSTER:
        rv = handle_bb_killed_monster(c, (subcmd_bb_killed_monster_t*)pkt);
        break;

    case SUBCMD60_SYNC_REG:
        rv = handle_bb_sync_reg(c, (subcmd_bb_sync_reg_t*)pkt);
        break;

    case SUBCMD60_CMODE_GRAVE:
        rv = handle_bb_cmode_grave(c, pkt);
        break;

        /*挑战模式 触发*/
    case SUBCMD60_UNKNOW_7D:
        sent = 0;
        //rv = handle_bb_cmd_check_game_size(c, pkt, 0x06);
        break;

        /*挑战模式 触发*/
    case SUBCMD60_UNKNOW_CH_8A:
        rv = handle_bb_Unknown_6x8A(c, (subcmd_bb_Unknown_6x8A_t*)pkt);
        break;

    case SUBCMD60_SET_TECH_LEVEL_OVERRIDE:
        rv = handle_bb_set_technique_level_override(c, (subcmd_bb_set_technique_level_override_t*)pkt);
        break;

        /*挑战模式 触发*/
    case SUBCMD60_TIMED_SWITCH_ACTIVATED:
        rv = handle_bb_timed_switch_activated(c, (subcmd_bb_timed_switch_activated_t*)pkt);
        break;

    case SUBCMD60_SHOP_INV:
        rv = handle_bb_shop_inv(c, (subcmd_bb_shop_inv_t*)pkt);
        break;

        // 0x53: Unknown (指令生效范围; 仅限游戏)
    case SUBCMD60_UNKNOW_53:
        rv = handle_bb_Unknown_6x53(c, (subcmd_bb_Unknown_6x53_t*)pkt);
        break;

    case SUBCMD60_WARP_55:
        rv = handle_bb_map_warp_55(c, (subcmd_bb_map_warp_t*)pkt);
        break;

    case SUBCMD60_LOBBY_ACTION:
        rv = handle_bb_lobby_act(c, (subcmd_bb_lobby_act_t*)pkt);
        break;

    case SUBCMD60_GOGO_BALL:
        rv = handle_bb_gogo_ball(c, (subcmd_bb_gogo_ball_t*)pkt);
        break;

    case SUBCMD60_GDRAGON_ACT:
        rv = handle_bb_gol_dragon_act(c, (subcmd_bb_gol_dragon_act_t*)pkt);
        break;

    case SUBCMD60_CHAIR_CREATE:
        rv = handle_bb_chair_dir(c, (subcmd_bb_create_lobby_chair_t*)pkt);
        break;

    case SUBCMD60_CHAIR_TURN:
        rv = handle_bb_chair_dir(c, (subcmd_bb_create_lobby_chair_t*)pkt);
        break;

    case SUBCMD60_CHAIR_MOVE:
        rv = handle_bb_chair_dir(c, (subcmd_bb_create_lobby_chair_t*)pkt);
        break;

    case SUBCMD60_TRADE_DONE:
        rv = handle_bb_trade_done(c, (subcmd_bb_trade_t*)pkt);
        break;

    case SUBCMD60_LEVEL_UP_REQ:
        rv = handle_bb_level_up_req(c, (subcmd_bb_levelup_req_t*)pkt);
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        DBG_LOG("未知 0x60 指令: 0x%02X", type);
#endif /* BB_LOG_UNKNOWN_SUBS */
        sent = 0;
        break;

    case SUBCMD60_FINISH_LOAD:
        if (l->type == LOBBY_TYPE_LOBBY) {
            for (i = 0; i < l->max_clients; ++i) {
                if (l->clients[i] && l->clients[i] != c &&
                    subcmd_send_pos(c, l->clients[i])) {
                    rv = -1;
                    break;
                }
            }
        }
    }

    /* Broadcast anything we don't care to check anything about. */
    if (!sent)
        rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}


//static int handle_bb_pick_up(ship_client_t* c, subcmd_bb_pick_up_t* pkt) {
//    lobby_t* l = c->cur_lobby;
//    int found;
//    iitem_t iitem_data;
//
//    /* We can't get these in a lobby without someone messing with something that
//       they shouldn't be... Disconnect anyone that tries. */
//    if (l->type == LOBBY_TYPE_LOBBY) {
//        ERR_LOG("GC %" PRIu32 " 在大厅中拾取了物品!",
//            c->guildcard);
//        return -1;
//    }
//
//    /* 合理性检查... Make sure the size of the subcommand and the client id
//       match with what we expect. Disconnect the client if not. */
//    if (pkt->hdr.pkt_len != LE16(0x0014) || pkt->shdr.size != 0x03 ||
//        pkt->shdr.client_id != c->client_id) {
//        ERR_LOG("GC %" PRIu32 " 发送错误的拾取数据!",
//            c->guildcard);
//        print_payload((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
//        return -1;
//    }
//
//    /* Try to remove the item from the lobby... */
//    found = lobby_remove_item_locked(l, pkt->item_id, &iitem_data);
//
//    if (found < 0)
//        return -1;
//    else if (found > 0)
//        /* Assume someone else already picked it up, and just ignore it... */
//        return 0;
//
//    if (add_item(c, iitem_data))
//        ERR_LOG("GC %" PRIu32 " 背包空间不足, 无法拾取!",
//            c->guildcard);
//
//    /* 给房间中客户端发包,消除这个物品. */
//    subcmd_send_bb_pick_item(c, iitem_data.data.item_id, pkt->area);
//
//    return subcmd_send_bb_destroy_map_item(c, pkt->area, iitem_data.data.item_id);
//}
