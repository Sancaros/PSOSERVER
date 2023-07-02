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
#include "subcmd_handle_60.h"
#include "subcmd_handle_62.h"
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

// ���ͻ��˷�����Ϸ����ʱ, ���ô��ļ��еĺ���
// ָ�
// (60, 62, 6C, 6D, C9, CB).

// subcmd ֱ�ӷ���ָ�����ͻ���
/* ���͸�ָ�����ݰ������� ignore_check �Ƿ���Կͻ��˺��Ե���� c �Ƿ񲻷����Լ�*/
int subcmd_send_lobby_bb(lobby_t* l, ship_client_t* c, subcmd_bb_pkt_t* pkt, int ignore_check) {
    int i;

    /* Send the packet to every connected client. */
    for (i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != c) {
            /* If we're supposed to check the ignore list, and this client is on
               it, don't send the packet
               �������Ҫ�������б����Ҹÿͻ��������У��벻Ҫ�������ݰ�. */
            if (ignore_check && client_has_ignored(l->clients[i], c->guildcard)) {
                continue;
            }

            if (l->clients[i]->version != CLIENT_VERSION_DCV1 ||
                !(l->clients[i]->flags & CLIENT_FLAG_IS_NTE))
                send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)pkt);
            else
                subcmd_translate_bb_to_nte(l->clients[i], pkt);
        }
    }

    return 0;
}

static int handle_bb_cmd_check_size(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! size %02X",
            c->guildcard, pkt->type, size);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_cmd_check_game_size(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type != LOBBY_TYPE_GAME) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸ������ %s ָ��!",
            c->guildcard, c_cmd_name(pkt->hdr.pkt_type, 0));
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

static int handle_bb_item_req(ship_client_t* c, ship_client_t* d, subcmd_bb_itemreq_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0020) || pkt->shdr.size != 0x06) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! ���ݴ�С %02X",
            c->guildcard, pkt->shdr.type, pkt->shdr.size);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }
    
    if (c->new_item.data_l[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        rv = subcmd_send_bb_gm_itemreq(c, (subcmd_bb_itemreq_t*)pkt);
    }
    else if (l->dropfunc && (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
        rv = l->dropfunc(c, l, pkt);
    }
    else if ((l->num_mtypes || l->num_mids) &&
        (l->flags & LOBBY_FLAG_QUESTING)) {
        rv = subcmd_send_bb_quest_itemreq(c, (subcmd_bb_itemreq_t*)pkt, d);
    }
    else
        rv = send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);

    return rv;
}

static int handle_bb_bitem_req(ship_client_t* c, ship_client_t* d, subcmd_bb_bitemreq_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int rv = 0;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0030) || pkt->shdr.size != 0x0A) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����ָ�� 0x%02X! ���ݴ�С %02X",
            c->guildcard, pkt->shdr.type, pkt->shdr.size);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    if (c->new_item.data_l[0] && !(l->flags & LOBBY_FLAG_LEGIT_MODE)) {
        rv = subcmd_send_bb_gm_itemreq(c, (subcmd_bb_itemreq_t*)pkt);
    }
    else if (l->dropfunc && (l->flags & LOBBY_FLAG_SERVER_DROPS)) {
        rv = l->dropfunc(c, l, pkt);
    }
    else if ((l->num_mtypes || l->num_mids) &&
        (l->flags & LOBBY_FLAG_QUESTING)) {
        rv = subcmd_send_bb_quest_itemreq(c, (subcmd_bb_itemreq_t*)pkt, d);
    }
    else
        rv = send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);

    return rv;
}

static int handle_bb_62_check_game_loading(ship_client_t* c, subcmd_bb_pkt_t* pkt, int size) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����д�������Ϸ������ָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand matches with what we
       expect. Disconnect the client if not. */
    if (pkt->size != size)
        return -1;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_warp_item(ship_client_t* c, subcmd_bb_warp_item_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t iitem_count, found, i, stack;
    uint32_t wrap_id;
    iitem_t backup_item;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    memset(&backup_item, 0, sizeof(iitem_t));
    wrap_id = *(uint32_t*)&pkt->data[0x0C];

    for (i = 0; i < c->bb_pl->inv.item_count; i++) {
        if (c->bb_pl->inv.iitems[i].data.item_id == wrap_id) {
            memcpy(&backup_item, &c->bb_pl->inv.iitems[i], sizeof(iitem_t));
            break;
        }
    }

    if (backup_item.data.item_id) {
        stack = stack_size_for_item(backup_item.data);

        //if (!stack && pkt->item_amount > 1) {
        //    ERR_LOG("GC %" PRIu32 " banking multiple of "
        //        "a non-stackable item!", c->guildcard);
        //    return -1;
        //}

        iitem_count = c->bb_pl->inv.item_count;

        if ((found = item_remove_from_inv(c->bb_pl->inv.iitems, iitem_count,
            backup_item.data.item_id, 1)) < 1) {
            ERR_LOG("�޷�����ұ������Ƴ���Ʒ!");
            return -1;
        }

        if (found < 0 || found > 1) {
            ERR_LOG("GC %u Error removing item from inventory for "
                "banking!", c->guildcard);
            return -1;
        }

        c->bb_pl->inv.item_count = (iitem_count -= found);

        if (backup_item.data.data_b[0] == 0x02)
            backup_item.data.data2_b[2] |= 0x40; // Wrap a mag
        else
            backup_item.data.data_b[4] |= 0x40; // Wrap other

        /* ����Ʒ����������. */
        found = add_item_to_client(c, &backup_item);

        if (found == -1)
            return 0;
    }
    else {
        ERR_LOG("GC %" PRIu32 " ������ƷID %d ʧ��!",
            c->guildcard, wrap_id);
        return -1;
    }

    return 0;
}

static int handle_bb_quest_oneperson_set_ex_pc(ship_client_t* c, subcmd_bb_quest_oneperson_set_ex_pc_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint32_t iitem_count, found;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if ((l->oneperson) && (l->flags & LOBBY_FLAG_QUESTING) && (!l->drops_disabled)) {
        iitem_t tmp_iitem;
        iitem_count = c->bb_pl->inv.item_count;

        memset(&tmp_iitem, 0, sizeof(iitem_t));
        tmp_iitem.data.data_b[0] = 0x03;
        tmp_iitem.data.data_b[1] = 0x10;
        tmp_iitem.data.data_b[2] = 0x02;
        
        found = item_remove_from_inv(c->bb_pl->inv.iitems, iitem_count, 0, 1);

        if (found < 0 || found > 1) {
            ERR_LOG("GC %u Error handle_bb_quest_oneperson_set_ex_pc!", c->guildcard);
            return -1;
        }

        c->bb_pl->inv.item_count = (iitem_count -= found);
        c->pl->bb.inv.item_count = c->bb_pl->inv.item_count;

        if (!found) {
            l->drops_disabled = 1;
            return -1;
        }
    }

    return 0;
}

static int handle_bb_quest_reward_meseta(ship_client_t* c, subcmd_bb_quest_reward_meseta_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int meseta;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    if (pkt->hdr.pkt_len != LE16(0x0010) || pkt->shdr.size != 0x02) {
        ERR_LOG("GC %" PRIu32 " ���Ի�ȡ�������������������!",
            c->guildcard);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    meseta = pkt->amount;

    if (meseta < 0) {
        meseta = -meseta;

        if (meseta > (int)c->bb_pl->character.disp.meseta) {
            c->bb_pl->character.disp.meseta = 0;
        }
        else
            c->bb_pl->character.disp.meseta -= meseta;

        return 0;

    }
    else {
        iitem_t ii;
        memset(&ii, 0, sizeof(iitem_t));
        ii.data.data_b[0] = ITEM_TYPE_MESETA;
        ii.data.data2_l = meseta;
        ii.data.item_id = generate_item_id(l, 0xFF);

        if (add_item_to_client(c, &ii) == -1) {
            ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷������Ʒ!",
                c->guildcard);
            return -1;
        }

        return subcmd_send_lobby_bb_create_inv_item(c, ii.data, 0);
    }
}

static int handle_bb_quest_reward_item(ship_client_t* c, subcmd_bb_quest_reward_item_t* pkt) {
    lobby_t* l = c->cur_lobby;

    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));

    iitem_t ii;
    memset(&ii, 0, sizeof(iitem_t));
    ii.data = pkt->item_data;
    ii.data.item_id = generate_item_id(l, 0xFF);

    if (add_item_to_client(c, &ii) == -1) {
        ERR_LOG("GC %" PRIu32 " �����ռ䲻��, �޷������Ʒ!",
            c->guildcard);
        return -1;
    }

    return subcmd_send_lobby_bb_create_inv_item(c, ii.data, 0);
}

/* ����BB 0x62/0x6D ���ݰ�. */
int subcmd_bb_handle_one(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* dest;
    uint16_t len = pkt->hdr.pkt_len;
    uint16_t hdr_type = pkt->hdr.pkt_type;
    uint8_t type = pkt->type;
    uint8_t size = pkt->size;
    int rv = -1;
    uint32_t dnum = LE32(pkt->hdr.flags);

    /* Ignore these if the client isn't in a lobby. */
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

    DBG_LOG("�ͻ��� %d GC %u 0x%02X ָ��: 0x%02X", dnum, l->clients[dnum]->guildcard, hdr_type, type);

    //subcmd_bb_626Dsize_check(c, pkt);

    subcmd62_handle_t func = subcmd62_get_handler(type, c->version);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;
        switch (type) {
        case SUBCMD62_BURST5://0x62 6F //��������ԾǨ����ʱ���� 5
        case SUBCMD62_BURST6://0x62 71 //��������ԾǨ����ʱ���� 6
            send_bb_quest_data1(dest, &c->bb_pl->quest_data1);
            rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x62 ָ��: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {
    case SUBCMD62_GUILDCARD:
    case SUBCMD62_PICK_UP:
    case SUBCMD62_TRADE:
    case SUBCMD62_SHOP_REQ:
    case SUBCMD62_SHOP_BUY:
    case SUBCMD62_TEKKING:
    case SUBCMD62_TEKKED:
    case SUBCMD62_OPEN_BANK:
    case SUBCMD62_BANK_ACTION:
    case SUBCMD62_GUILD_INVITE1:
    case SUBCMD62_GUILD_INVITE2:
    case SUBCMD62_GUILD_MASTER_TRANS1:
    case SUBCMD62_GUILD_MASTER_TRANS2:
        rv = func(c, dest, pkt);
        break;

    case SUBCMD62_ITEMREQ:
    case SUBCMD62_BITEMREQ:
        rv = l->dropfunc(c, l, pkt);
        break;

    case SUBCMD62_WARP_ITEM:
        rv = handle_bb_warp_item(dest, (subcmd_bb_warp_item_t*)pkt);
        break;

    case SUBCMD62_QUEST_ONEPERSON_SET_EX_PC:
        rv = handle_bb_quest_oneperson_set_ex_pc(dest, (subcmd_bb_quest_oneperson_set_ex_pc_t*)pkt);
        break;

    case SUBCMD62_QUEST_REWARD_MESETA:
        rv = handle_bb_quest_reward_meseta(c, (subcmd_bb_quest_reward_meseta_t*)pkt);
        break;

    case SUBCMD62_QUEST_REWARD_ITEM:
        rv = handle_bb_quest_reward_item(c, (subcmd_bb_quest_reward_item_t*)pkt);
        break;
    }

#ifdef BB_LOG_UNKNOWN_SUBS
    if (func == NULL && rv == -1) {
        DBG_LOG("δ֪ 0x%02X ָ��: 0x%02X", hdr_type, type);
        //UNK_CSPD(type, c->version, pkt);
        rv = send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
    }
#endif /* BB_LOG_UNKNOWN_SUBS */

    pthread_mutex_unlock(&l->mutex);
    return rv;
}

int handle_bb_burst_pldata(ship_client_t* c, ship_client_t* d,
    subcmd_bb_burst_pldata_t* pkt) {
    lobby_t* l = c->cur_lobby;
    uint8_t ch_class = c->bb_pl->character.dress_data.ch_class;
    iitem_t* item;
    int i, rv = 0;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("GC %" PRIu32 " �ڴ����д��������е��������!", c->guildcard);
        return -1;
    }

    if ((c->version == CLIENT_VERSION_XBOX && d->version == CLIENT_VERSION_GC) ||
        (d->version == CLIENT_VERSION_XBOX && c->version == CLIENT_VERSION_GC)) {
        /* ɨ���沢�ڷ���֮ǰ�޸�����mag. */

        for (i = 0; i < pkt->inv.item_count; ++i) {
            item = &pkt->inv.iitems[i];

            /* �����Ŀ��mag,��ô���Ǳ��뽻�����ݵ����һ��dword.����,��ɫ��ͳ�����ݻ���һ���� */
            if (item->data.data_b[0] == ITEM_TYPE_MAG) {
                item->data.data2_l = SWAP32(item->data.data2_l);
            }
        }
    }
    else {
        //pkt->guildcard = c->guildcard;

        //// Check techniques...���ħ��,����ǻ����˾Ͳ�����ħ��
        //if (!(c->equip_flags & EQUIP_FLAGS_DROID)) {
        //    for (i = 0; i < BB_MAX_TECH_LEVEL; i++) {
        //        //if (pkt->techniques[i] == 0xFF)
        //            //pkt->techniques[i] = 0x00;

        //        if (pkt->techniques[i] > max_tech_level[i].max_lvl[ch_class])
        //            rv = -1;

        //        if (rv) {
        //            ERR_LOG("GC %u ������ Lv%d.%s ħ��! ����ֵ Lv%d.%s", pkt->guildcard,
        //                pkt->techniques[i], max_tech_level[i].tech_name, 
        //                max_tech_level[i].max_lvl[ch_class], max_tech_level[i].tech_name);

        //            /* �淶�����ݰ���ħ���ȼ� */
        //            //pkt->techniques[i] = max_tech_level[i].max_lvl[ch_class];
        //            rv = 0;
        //        }
        //    }
        //}

        ////for (i = 0; i < BB_MAX_TECH_LEVEL; i++) {
        ////    // ѧ�����м��� TODO �������׿���
        ////    //c->bb_pl->character.techniques[i] = max_tech_level[i].max_lvl[ch_class];
        ////    pkt->techniques[i] = c->bb_pl->character.techniques[i];
        ////}

        //// �����ҽ�ɫ�ṹ
        //pkt->dress_data = c->bb_pl->character.dress_data;

        //memcpy(&pkt->name[0], &c->bb_pl->character.name[0], sizeof(c->bb_pl->character.name));

        //// Prevent crashing with NPC skins... ��ֹNPCƤ������
        //if (c->bb_pl->character.dress_data.v2flags) {
        //    pkt->dress_data.v2flags = 0x00;
        //    pkt->dress_data.version = 0x00;
        //    pkt->dress_data.v1flags = LE32(0x00000000);
        //    pkt->dress_data.costume = LE16(0x0000);
        //    pkt->dress_data.skin = LE16(0x0000);
        //}

        ///* ������������ֵ */
        //pkt->stats.atp = c->bb_pl->character.disp.stats.atp;
        //pkt->stats.mst = c->bb_pl->character.disp.stats.mst;
        //pkt->stats.evp = c->bb_pl->character.disp.stats.evp;
        //pkt->stats.hp = c->bb_pl->character.disp.stats.hp;
        //pkt->stats.dfp = c->bb_pl->character.disp.stats.dfp;
        //pkt->stats.ata = c->bb_pl->character.disp.stats.ata;
        //pkt->stats.lck = c->bb_pl->character.disp.stats.lck;

        //for (i = 0; i < 10; i++)
        //    pkt->opt_flag[i] = c->bb_pl->character.disp.opt_flag[i];

        //pkt->level = c->bb_pl->character.disp.level;
        //pkt->exp = c->bb_pl->character.disp.exp;
        //pkt->meseta = c->bb_pl->character.disp.meseta;


        //// Could check inventory here �鿴����
        //pkt->inv.item_count = c->bb_pl->inv.item_count;

        //for (i = 0; i < MAX_PLAYER_INV_ITEMS; i++)
        //    memcpy(&pkt->inv.iitems[i], &c->bb_pl->inv.iitems[i], sizeof(iitem_t));

        //for (i = 0; i < sizeof(uint32_t); i++)
        //    memset(&pkt->unused[i], 0, sizeof(uint32_t));

        //pkt->shdr.size = pkt->hdr.pkt_len / 4;
    }

    //printf("%" PRIu32 " - %" PRIu32 "  %s \n", c->guildcard, pkt->guildcard, pkt->dress_data.guildcard_string);

    return send_pkt_bb(d, (bb_pkt_hdr_t*)pkt);
}

int subcmd_bb_handle_6D(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    lobby_t* l = c->cur_lobby;
    ship_client_t* dest;
    uint16_t len = pkt->hdr.pkt_len;
    uint16_t hdr_type = pkt->hdr.pkt_type;
    uint8_t type = pkt->type;
    uint8_t size = pkt->size;
    int rv = -1;
    uint32_t dnum = LE32(pkt->hdr.flags);

    /* Ignore these if the client isn't in a lobby. */
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

    DBG_LOG("�ͻ��� %d GC %u 0x%02X ָ��: 0x%02X", dnum, l->clients[dnum]->guildcard, hdr_type, type);

    //subcmd_bb_626Dsize_check(c, pkt);

    subcmd62_handle_t func = subcmd62_get_handler(type, c->version);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        rv = 0;
        switch (type) {
        case SUBCMD6D_BURST1://0x6D 6D //��������ԾǨ����ʱ���� 1
        case SUBCMD6D_BURST2://0x6D 6B //��������ԾǨ����ʱ���� 2
        case SUBCMD6D_BURST3://0x6D 6C //��������ԾǨ����ʱ���� 3
        case SUBCMD6D_BURST4://0x6D 6E //��������ԾǨ����ʱ���� 4
            if (l->flags & LOBBY_FLAG_QUESTING)
                rv = lobby_enqueue_burst_bb(l, c, (bb_pkt_hdr_t*)pkt);
            /* Fall through... */
            rv |= send_pkt_bb(dest, (bb_pkt_hdr_t*)pkt);
            break;

        case SUBCMD6D_BURST_PLDATA://0x6D 70 //��������ԾǨ����ʱ���� 7
            //rv = handle_bb_burst_pldata(c, dest, (subcmd_bb_burst_pldata_t*)pkt);
            rv = func(c, dest, pkt);
            break;

        default:
            DBG_LOG("lobby_enqueue_pkt_bb 0x62 ָ��: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    pthread_mutex_unlock(&l->mutex);
    return rv;
}


///////////////////////////////////////////////////////////////////////////////////////////////

int handle_bb_gol_dragon_act(ship_client_t* c, subcmd_bb_gol_dragon_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int v = c->version, i;
    subcmd_bb_gol_dragon_act_t tr;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        DBG_LOG("Guild card %" PRIu32 " reported Gol Dragon action in "
            "lobby!\n", c->guildcard);
        return -1;
    }

    if (v != CLIENT_VERSION_XBOX && v != CLIENT_VERSION_GC)
        return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    /* Make a version to send to the other version if the client is on GC or
       Xbox. */
    memcpy(&tr, pkt, sizeof(subcmd_bb_gol_dragon_act_t));
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

static int handle_bb_charge_act(ship_client_t* c, subcmd_bb_charge_act_t* pkt) {
    lobby_t* l = c->cur_lobby;
    int32_t meseta = *(int32_t*)pkt->mst;

    /* We can't get these in a lobby without someone messing with something that
       they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ����л�ȡ����!",
            c->guildcard);
        return -1;
    }

    if (meseta > 0)
    {
        if (c->bb_pl->character.disp.meseta >= (uint32_t)meseta)
            subcmd_send_bb_delete_meseta(c, meseta, 0);
        else
            subcmd_send_bb_delete_meseta(c, c->bb_pl->character.disp.meseta, 0);
    }
    else
    {
        meseta = -meseta;
        c->bb_pl->character.disp.meseta += (uint32_t)meseta;
        if (c->bb_pl->character.disp.meseta > 999999)
            c->bb_pl->character.disp.meseta = 999999;
    }

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_shop_inv(ship_client_t* c, subcmd_bb_shop_inv_t* pkt) {
    block_t* b = c->cur_block;
    lobby_t* l = c->cur_lobby;
 
    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    /* This aught to do it... */
    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_take_item(ship_client_t* c, subcmd_bb_take_item_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ����ָ��!",
            c->guildcard);
        return -1;
    }

    // BB should never send this command - inventory items should only be
    // created by the server in response to shop buy / bank withdraw / etc. reqs

    UNK_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);

    /* ���ݰ����, ��������Ϸ����. */
    return 0;
}

static int handle_bb_level_up(ship_client_t* c, subcmd_bb_level_up_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ�����ָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x05 || pkt->shdr.client_id != c->client_id) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    /* Copy over the new data to the client's character structure... */
    c->pl->bb.character.disp.stats.atp = pkt->atp;
    c->pl->bb.character.disp.stats.mst = pkt->mst;
    c->pl->bb.character.disp.stats.evp = pkt->evp;
    c->pl->bb.character.disp.stats.hp = pkt->hp;
    c->pl->bb.character.disp.stats.dfp = pkt->dfp;
    c->pl->bb.character.disp.stats.ata = pkt->ata;
    c->pl->bb.character.disp.level = pkt->level;

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

static int handle_bb_trade_done(ship_client_t* c, subcmd_bb_trade_t* pkt) {
    lobby_t* l = c->cur_lobby;

    /* We can't get these in lobbies without someone messing with something
       that they shouldn't be... Disconnect anyone that tries. */
    if (l->type == LOBBY_TYPE_LOBBY) {
        ERR_LOG("GC %" PRIu32 " �ڴ�����������Ϸ�����ָ��!",
            c->guildcard);
        return -1;
    }

    /* �����Լ��... Make sure the size of the subcommand and the client id
       match with what we expect. Disconnect the client if not. */
    if (pkt->shdr.size != 0x08) {
        ERR_LOG("GC %" PRIu32 " �����𻵵�����! 0x%02X",
            c->guildcard, pkt->shdr.type);
        ERR_CSPD(pkt->hdr.pkt_type, c->version, (uint8_t*)pkt);
        return -1;
    }

    return subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);
}

/* ����BB 0x60 ���ݰ�. */
int subcmd_bb_handle_bcast(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint16_t hdr_type = pkt->hdr.pkt_type;
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int rv = 0, sent = 1, i = 0;

    /* ����ͻ��˲��ڴ������߶�������������ݰ�. */
    if (!l)
        return 0;

#ifdef DEBUG_60

    DBG_LOG("��� 0x60 ָ��: 0x%02X", type);

#endif // DEBUG_60

    pthread_mutex_lock(&l->mutex);

    //subcmd62_bb_handle_t func = subcmd62_get_bb_handler(type);

    //if (func == NULL) {
    //    DBG_LOG("δ֪ 0x%02X ָ��: 0x%02X", hdr_type, type);
    //    display_packet(pkt, pkt->hdr.pkt_len);
    //}

    subcmd_bb_60size_check(c, pkt);

    /* If there's a burst going on in the lobby, delay most packets */
    if (l->flags & LOBBY_FLAG_BURSTING) {
        switch (type) {
        case SUBCMD60_SET_POS_3F://����ԾǨʱ���� 1

        case SUBCMD60_SET_AREA_1F://����ԾǨʱ���� 2

        case SUBCMD60_LOAD_3B://����ԾǨʱ���� 3
        case SUBCMD60_BURST_DONE:
            /* TODO ����������ںϽ����亯���� */

            /* 0x7C ��սģʽ ���뷿����Ϸδ��ʼǰ����*/
        case SUBCMD60_CMODE_GRAVE:
            rv = subcmd60_bb_handle[type](c, pkt);
            break;

        default:
            DBG_LOG("LOBBY_FLAG_BURSTING 0x60 ָ��: 0x%02X", type);
            rv = lobby_enqueue_pkt_bb(l, c, (bb_pkt_hdr_t*)pkt);
        }

        pthread_mutex_unlock(&l->mutex);
        return rv;
    }

    switch (type) {

        /* 0x05 �˺����������� */
    case SUBCMD60_SWITCH_CHANGED:

        /* 0x07 ������� */
    case SUBCMD60_SYMBOL_CHAT:

        /* 0x0A �˺����������� */
    case SUBCMD60_HIT_MONSTER:

        /* 0x0B �˺����������� */
    case SUBCMD60_HIT_OBJ:

        /* 0x0C �˺����������� */
    case SUBCMD60_CONDITION_ADD:

        /* 0x0D �˺����������� */
    case SUBCMD60_CONDITION_REMOVE:

        /* 0x12 EP1��BOSS�Ķ��� */
    case SUBCMD60_DRAGON_ACT:// Dragon actions

        /* 0x1F �˺����������� */
    case SUBCMD60_SET_AREA_1F:

        /* 0x20 �˺����������� */
    case SUBCMD60_SET_AREA_20:

        /* 0x21 ������һ�㴫�� */
    case SUBCMD60_INTER_LEVEL_WARP:

        /* 0x22 ������ҳ��� */
    case SUBCMD60_LOAD_22://subcmd_set_player_visibility_6x22_6x23_t

        /* 0x23 ������������������ */
    case SUBCMD60_FINISH_LOAD:

        /* 0x24 �˺����������� */
    case SUBCMD60_SET_POS_24:

        /* 0x25 �˺����������� */
    case SUBCMD60_EQUIP:

        /* 0x26 �˺����������� */
    case SUBCMD60_REMOVE_EQUIP:

        /* 0x27 ʹ����Ʒ */
    case SUBCMD60_USE_ITEM:

        /* 0x28 ι��MAG */
    case SUBCMD60_FEED_MAG:

        /* 0x29 �˺����������� */
    case SUBCMD60_DELETE_ITEM:

        /* 0x2A �˺����������� */
    case SUBCMD60_DROP_ITEM:

        /* 0x2C ��NPC��̸ */
    case SUBCMD60_TALK_NPC:

        /* 0x2D ��NPC��̸��� */
    case SUBCMD60_DONE_NPC:

        /* 0x2F �����˹��� */
    case SUBCMD60_HIT_BY_ENEMY:

        /* 0x31 ҽԺ�������� */
    case SUBCMD60_MEDIC_REQ:

        /* 0x32 ���ҽԺ���� */
    case SUBCMD60_MEDIC_DONE:

        /* 0x3A TODO δ֪ */
    case SUBCMD60_UNKNOW_3A:

        /* 0x3B TODO ��ͼ����ʱ���� */
    case SUBCMD60_LOAD_3B:

        /* 0x3E �˺����������� */
    case SUBCMD60_SET_POS_3E:
        /* 0x3F �˺����������� */
    case SUBCMD60_SET_POS_3F:

        /* 0x40 �˺����������� */
    case SUBCMD60_MOVE_SLOW:
        /* 0x42 �˺����������� */
    case SUBCMD60_MOVE_FAST:

        /* 0x43 �˺����������� */
    case SUBCMD60_ATTACK1:
        /* 0x44 �˺����������� */
    case SUBCMD60_ATTACK2:
        /* 0x45 �˺����������� */
    case SUBCMD60_ATTACK3:

        /* 0x46 �˺����������� */
    case SUBCMD60_OBJHIT_PHYS:
        /* 0x47 �˺����������� */
    case SUBCMD60_OBJHIT_TECH:

        /* 0x48 �˺����������� */
    case SUBCMD60_USED_TECH:

        /* 0x4A �˺����������� */
    case SUBCMD60_DEFENSE_DAMAGE:

        /* 0x4B �˺����������� */
    case SUBCMD60_TAKE_DAMAGE1:
        /* 0x4C �˺����������� */
    case SUBCMD60_TAKE_DAMAGE2:

        /* 0x4D SUBCMD60_DEATH_SYNC */
    case SUBCMD60_DEATH_SYNC:

        /* 0x4E δ֪ */
    case SUBCMD60_UNKNOW_4E:

        /* 0x4F SUBCMD60_PLAYER_SAVED */
    case SUBCMD60_PLAYER_SAVED:

        /* 0x50 ��Ϸ�������� */
    case SUBCMD60_SWITCH_REQ:

        /* 0x52 ��Ϸ�˵����� */
    case SUBCMD60_MENU_REQ:

        /* 0x55 ��ͼ���ͱ仯���� */
    case SUBCMD60_WARP_55:

        /* 0x58 �������� */
    case SUBCMD60_LOBBY_ACTION:

        /* 0x61 ����������� */
    case SUBCMD60_LEVEL_UP_REQ:

        /* 0x63 �ݻٵ�����Ʒ����������Ʒ����ʱ�� */
    case SUBCMD60_DESTROY_GROUND_ITEM:

        /* 0x67 �˺����������� */
    case SUBCMD60_CREATE_ENEMY_SET:

        /* 0x68 ���ɴ����� */
    case SUBCMD60_CREATE_PIPE:

        /* 0x69 ����NPC */
    case SUBCMD60_SPAWN_NPC:

        /* 0x6A TODO δ֪ */
    case SUBCMD60_UNKNOW_6A:

        /* 0x74 �˺����������� */
    case SUBCMD60_WORD_SELECT:

        /* 0x75 ���������� */
    case SUBCMD60_SET_FLAG:

        /* 0x76 �˺����������� */
    case SUBCMD60_KILL_MONSTER:

        /* 0x77 �˺����������� */
    case SUBCMD60_SYNC_REG:

        /* 0x79 ���������� */
    case SUBCMD60_GOGO_BALL:

        /* 0x88 ��Ҽ�ͷ���� */
    case SUBCMD60_ARROW_CHANGE:

        /* 0x89 ������� */
    case SUBCMD60_PLAYER_DIED:

        /* 0x8A ��սģʽ ����*/
    case SUBCMD60_UNKNOW_CH_8A:

        /* 0x8D �˺����������� */
    case SUBCMD60_SET_TECH_LEVEL_OVERRIDE:

        /* 0x93 ��սģʽ ����*/
    case SUBCMD60_TIMED_SWITCH_ACTIVATED:

        /* 0xA1 SUBCMD60_SAVE_PLAYER_ACT */
    case SUBCMD60_SAVE_PLAYER_ACT:

        /* 0xAB ����״̬ ����*/
    case SUBCMD60_CHAIR_CREATE:
        /* 0xAE ����״̬ ����*/
    case SUBCMD60_CHAIR_STATE:
        /* 0xAF ����״̬ ����*/
    case SUBCMD60_CHAIR_TURN:
        /* 0xB0 ����״̬ ����*/
    case SUBCMD60_CHAIR_MOVE:

        /* 0xC0 ������Ʒ */
    case SUBCMD60_SELL_ITEM:

        /* 0xC3 �˺����������� */
    case SUBCMD60_DROP_SPLIT_STACKED_ITEM:

        /* 0xC4 �˺����������� */
    case SUBCMD60_SORT_INV:

        /* 0xC5 �˺����������� */
    case SUBCMD60_MEDIC:

        /* 0xC6 ͵ȡ���� */
    case SUBCMD60_STEAL_EXP:
        /* 0xC8 �˺����������� */
    case SUBCMD60_EXP_REQ:

        /* 0xCC ����ת����Ʒ���� */
    case SUBCMD60_GUILD_EX_ITEM:

        /* 0xD2 �˺����������� */
    case SUBCMD60_GALLON_AREA:
        rv = subcmd60_bb_handle[type](c, pkt);
        break;


        /* 0xD9 Momoka��Ʒ���� */
    case SUBCMD60_EX_ITEM_MK:
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
        sent = 0;
        break;

    default:
#ifdef BB_LOG_UNKNOWN_SUBS
        //DBG_LOG("��δ��� 0x60: 0x%02X\n", type);
        //display_packet((uint8_t*)pkt, LE16(pkt->hdr.pkt_len));
        UNK_CSPD(type, c->version, (uint8_t*)pkt);
#endif /* BB_LOG_UNKNOWN_SUBS */
        sent = 0;
        break;

    }

    /* Broadcast anything we don't care to check anything about. */
    if (!sent)
        rv = subcmd_send_lobby_bb(l, c, (subcmd_bb_pkt_t*)pkt, 0);

    pthread_mutex_unlock(&l->mutex);
    return rv;
}