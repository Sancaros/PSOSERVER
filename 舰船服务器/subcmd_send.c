/*
    �λ�֮���й� ���������� ��ָ���
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

#include "subcmd_send.h"
#include "subcmd_size_table.h"

#include "handle_player_items.h"
#include "f_logs.h"
#include "utils.h"

// subcmd ֱ�ӷ���ָ�����ͻ���
/* ���͸�ָ�����ݰ������� ignore_check �Ƿ���Ա����뱻���Ե���� */
int subcmd_send_lobby_bb(lobby_t* l, ship_client_t* src, subcmd_bb_pkt_t* pkt, int ignore_check) {
    int i;

    if (!l) {
        ERR_LOG("GC %" PRIu32 " ����һ����Ч�Ĵ�����!",
            src->guildcard);
        return -1;
    }

    /* Send the packet to every connected client. */
    for (i = 0; i < l->max_clients; ++i) {
        if (!l->clients[i])
            continue;

        if (l->clients[i] == src)
            continue;

        /* If we're supposed to check the ignore list, and this client is on
           it, don't send the packet
           �������Ҫ�������б����Ҹÿͻ��������У��벻Ҫ�������ݰ�. */
        if (ignore_check && client_has_ignored(l->clients[i], src->guildcard))
            continue;

        if (l->clients[i]->version != CLIENT_VERSION_DCV1 ||
            !(l->clients[i]->flags & CLIENT_FLAG_IS_NTE))
            send_pkt_bb(l->clients[i], (bb_pkt_hdr_t*)pkt);
        else
            subcmd_translate_bb_to_nte(l->clients[i], pkt);
    }

    return 0;
}

/* 0x5D SUBCMD60_DROP_STACK BB ���˵���ѵ���Ʒ*/
int subcmd_send_drop_stack_bb(ship_client_t* src, uint16_t drop_src_id, litem_t* litem) {
    subcmd_drop_stack_t dc = { 0 };
    subcmd_bb_drop_stack_t bb = { 0 };

    /* ������ݲ�׼������.. */
    dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    dc.hdr.pkt_len = LE16(sizeof(subcmd_drop_stack_t));
    dc.hdr.flags = 0;

    dc.shdr.type = SUBCMD60_DROP_STACK;
    dc.shdr.size = 0x0A;
    dc.shdr.client_id = drop_src_id;

    bb.hdr.pkt_len = LE16(0x2C);
    bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    bb.hdr.flags = 0;

    bb.shdr.type = SUBCMD60_DROP_STACK;
    bb.shdr.size = 0x0A;
    bb.shdr.client_id = drop_src_id;

    dc.area = LE16(litem->area);
    bb.area = LE32(litem->area);
    bb.x = dc.x = litem->x;
    bb.z = dc.z = litem->z;

    bb.data = dc.data = litem->iitem.data;

    if (litem->iitem.data.datab[0] == ITEM_TYPE_MESETA)
        bb.data.data2l = dc.data.data2l = litem->iitem.data.data2l;

    if (is_stackable(&litem->iitem.data))
        bb.data.datab[5] = dc.data.datab[5] = litem->iitem.data.datab[5];

    bb.two = dc.two = LE32(0x00000002);

    if (src->version == CLIENT_VERSION_GC)
        bswap_data2_if_mag(&dc.data);

    switch (src->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        return send_pkt_dc(src, (dc_pkt_hdr_t*)&dc);

    case CLIENT_VERSION_BB:
        return send_pkt_bb(src, (bb_pkt_hdr_t*)&bb);

    default:
        return 0;
    }
}

/* 0x5D SUBCMD60_DROP_STACK DC ���˵���ѵ���Ʒ*/
int subcmd_send_drop_stack_dc(ship_client_t* src, 
    uint16_t drop_src_id, item_t item, uint8_t area, float x, float z) {
    subcmd_drop_stack_t dc = { 0 };
    subcmd_bb_drop_stack_t bb = { 0 };

    /* ������ݲ�׼������.. */
    dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    dc.hdr.pkt_len = LE16(sizeof(subcmd_drop_stack_t));
    dc.hdr.flags = 0;

    dc.shdr.type = SUBCMD60_DROP_STACK;
    dc.shdr.size = 0x0A;
    dc.shdr.client_id = drop_src_id;

    bb.hdr.pkt_len = LE16(0x2C);
    bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    bb.hdr.flags = 0;

    bb.shdr.type = SUBCMD60_DROP_STACK;
    bb.shdr.size = 0x0A;
    bb.shdr.client_id = drop_src_id;

    dc.area = LE16(area);
    bb.area = LE32(area);
    bb.x = dc.x = x;
    bb.z = dc.z = z;

    bb.data = dc.data = item;

    if (item.datab[0] == ITEM_TYPE_MESETA)
        bb.data.data2l = dc.data.data2l = item.data2l;

    if (is_stackable(&item))
        bb.data.datab[5] = dc.data.datab[5] = item.datab[5];

    bb.two = dc.two = LE32(0x00000002);

    if (src->version == CLIENT_VERSION_GC)
        bswap_data2_if_mag(&dc.data);

    switch (src->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        return send_pkt_dc(src, (dc_pkt_hdr_t*)&dc);

    case CLIENT_VERSION_BB:
        return send_pkt_bb(src, (bb_pkt_hdr_t*)&bb);

    default:
        return 0;
    }
}

/* 0x5D SUBCMD60_DROP_STACK BB ��������ѵ���Ʒ*/
int subcmd_send_lobby_drop_stack_bb(ship_client_t* src, uint16_t drop_src_id, ship_client_t* nosend, litem_t* litem) {
    lobby_t* l = src->cur_lobby;

    if (!l) {
        ERR_LOG("GC %" PRIu32 " ����һ����Ч�Ĵ�����!",
            src->guildcard);
        return -1;
    }

    subcmd_drop_stack_t dc = { 0 };
    subcmd_bb_drop_stack_t bb = { 0 };

    /* ������ݲ�׼������.. */
    dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    dc.hdr.pkt_len = LE16(sizeof(subcmd_drop_stack_t));
    dc.hdr.flags = 0;

    dc.shdr.type = SUBCMD60_DROP_STACK;
    dc.shdr.size = 0x0A;
    dc.shdr.client_id = drop_src_id;

    bb.hdr.pkt_len = LE16(0x2C);
    bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    bb.hdr.flags = 0;

    bb.shdr.type = SUBCMD60_DROP_STACK;
    bb.shdr.size = 0x0A;
    bb.shdr.client_id = drop_src_id;

    dc.area = LE16(litem->area);
    bb.area = LE32(litem->area);
    bb.x = dc.x = litem->x;
    bb.z = dc.z = litem->z;

    bb.data = dc.data = litem->iitem.data;

    if (litem->iitem.data.datab[0] == ITEM_TYPE_MESETA)
        bb.data.data2l = dc.data.data2l = litem->iitem.data.data2l;

    if (is_stackable(&litem->iitem.data))
        bb.data.datab[5] = dc.data.datab[5] = litem->iitem.data.datab[5];

    bb.two = dc.two = LE32(0x00000002);

    if (src->version == CLIENT_VERSION_GC)
        bswap_data2_if_mag(&dc.data);

    switch (src->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        return lobby_send_pkt_dc(l, nosend, (dc_pkt_hdr_t*)&dc, 0);

    case CLIENT_VERSION_BB:
        return lobby_send_pkt_bb(l, nosend, (bb_pkt_hdr_t*)&bb, 0);

    default:
        return 0;
    }
}

/* 0x5D SUBCMD60_DROP_STACK DC ��������ѵ���Ʒ*/
int subcmd_send_lobby_drop_stack_dc(ship_client_t* src, 
    uint16_t drop_src_id, ship_client_t* nosend, item_t item, uint8_t area, float x, float z) {
    lobby_t* l = src->cur_lobby;

    if (!l) {
        ERR_LOG("GC %" PRIu32 " ����һ����Ч�Ĵ�����!",
            src->guildcard);
        return -1;
    }

    subcmd_drop_stack_t dc = { 0 };
    subcmd_bb_drop_stack_t bb = { 0 };

    /* ������ݲ�׼������.. */
    dc.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    dc.hdr.pkt_len = LE16(sizeof(subcmd_drop_stack_t));
    dc.hdr.flags = 0;

    dc.shdr.type = SUBCMD60_DROP_STACK;
    dc.shdr.size = 0x0A;
    dc.shdr.client_id = drop_src_id;

    bb.hdr.pkt_len = LE16(0x2C);
    bb.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    bb.hdr.flags = 0;

    bb.shdr.type = SUBCMD60_DROP_STACK;
    bb.shdr.size = 0x0A;
    bb.shdr.client_id = drop_src_id;

    dc.area = LE16(area);
    bb.area = LE32(area);
    bb.x = dc.x = x;
    bb.z = dc.z = z;

    bb.data = dc.data = item;

    if (item.datab[0] == ITEM_TYPE_MESETA)
        bb.data.data2l = dc.data.data2l = item.data2l;

    if (is_stackable(&item))
        bb.data.datab[5] = dc.data.datab[5] = item.datab[5];

    bb.two = dc.two = LE32(0x00000002);

    if (src->version == CLIENT_VERSION_GC)
        bswap_data2_if_mag(&dc.data);

    switch (src->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        return lobby_send_pkt_dc(l, nosend, (dc_pkt_hdr_t*)&dc, 0);

    case CLIENT_VERSION_BB:
        return lobby_send_pkt_bb(l, nosend, (bb_pkt_hdr_t*)&bb, 0);

    default:
        return 0;
    }
}

/* 0x59 SUBCMD60_DEL_MAP_ITEM BB ʰȡ��Ʒ */
int subcmd_send_bb_del_map_item(ship_client_t* c, uint32_t area, uint32_t item_id) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_destroy_map_item_t pkt = { 0 };
    int pkt_size = sizeof(subcmd_bb_destroy_map_item_t);

    if (!l)
        return -1;

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    pkt.hdr.flags = 0;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD60_ITEM_DELETE_IN_MAP;
    pkt.shdr.size = 0x03;
    pkt.shdr.client_id = LE16(c->client_id);

    /* ���ʣ������ */
    pkt.client_id2 = LE16(c->client_id);
    pkt.area = area;
    pkt.item_id = item_id;

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
}

/* 0xBE SUBCMD60_CREATE_ITEM BB ���˻����Ʒ */
int subcmd_send_bb_create_inv_item(ship_client_t* src, item_t item, uint32_t amount) {
    subcmd_bb_create_item_t pkt = { 0 };

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(sizeof(subcmd_bb_create_item_t));
    pkt.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    pkt.hdr.flags = 0;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD60_ITEM_CREATE;
    pkt.shdr.size = 0x09;
    pkt.shdr.client_id = src->client_id;

    /* ���ʣ������ */
    memcpy(&pkt.item.datab[0], &item.datab[0], 12);
    pkt.item.item_id = item.item_id;

    if ((!is_stackable(&item)) || (item.datab[0] == ITEM_TYPE_MESETA))
        pkt.item.data2l = item.data2l;
    else
        pkt.item.datab[5] = (uint8_t)amount;

    /* ���һ��32λ�ֽڵĳ�ʼ��Ϊ0 δʹ�õ�*/
    pkt.unused2 = 0;

    return send_pkt_bb(src, (bb_pkt_hdr_t*)&pkt);
}

/* 0xBE SUBCMD60_CREATE_ITEM BB ���͸����������Ʒ ����SHOP���͵Ļ�ȡ */
int subcmd_send_lobby_bb_create_inv_item(ship_client_t* src, item_t item, uint32_t amount, bool send_to_src) {
    lobby_t* l = src->cur_lobby;
    subcmd_bb_create_item_t pkt = { 0 };

    if (!l)
        return -1;

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(sizeof(subcmd_bb_create_item_t));
    pkt.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    pkt.hdr.flags = 0;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD60_ITEM_CREATE;
    pkt.shdr.size = 0x09;
    pkt.shdr.client_id = src->client_id;

    /* ���ʣ������ */
    memcpy(&pkt.item.datab[0], &item.datab[0], 12);
    pkt.item.item_id = item.item_id;

    if ((!is_stackable(&item)) || (item.datab[0] == ITEM_TYPE_MESETA))
        pkt.item.data2l = item.data2l;
    else
        pkt.item.datab[5] = (uint8_t)amount;

    /* ���һ��32λ�ֽڵĳ�ʼ��Ϊ0 δʹ�õ�*/
    pkt.unused2 = 0;

    if (send_to_src)
        return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
    else
        return subcmd_send_lobby_bb(l, src, (subcmd_bb_pkt_t*)&pkt, 0);
}

/* 0xB9 SUBCMD62_TEKKED_RESULT BB ���˻�ü�����Ʒ */
int subcmd_send_bb_create_tekk_item(ship_client_t* src, item_t item) {
    subcmd_bb_tekk_identify_result_t pkt = { 0 };
    int pkt_size = sizeof(subcmd_bb_tekk_identify_result_t);

    if (src->version != CLIENT_VERSION_BB) {
        ERR_LOG("cannot send item identify result to non-BB client");
        return -1;
    }

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    pkt.hdr.flags = 0x00000000;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD62_TEKKED_RESULT;
    pkt.shdr.size = pkt_size / 4;
    pkt.shdr.client_id = src->client_id;

    /* ���ʣ������ */
    pkt.item = item;

    return send_pkt_bb(src, (bb_pkt_hdr_t*)&pkt);
}

/* 0x29 SUBCMD60_DELETE_ITEM BB ������Ʒ */
int subcmd_send_lobby_bb_destroy_item(ship_client_t* c, uint32_t item_id, uint32_t amt) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_destroy_item_t pkt = { 0 };
    int pkt_size = sizeof(subcmd_bb_destroy_item_t);

    if (!l)
        return -1;

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    pkt.hdr.flags = 0;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD60_ITEM_DELETE;
    pkt.shdr.size = (uint8_t)(pkt_size / 4);
    pkt.shdr.client_id = c->client_id;

    /* ���ʣ������ */
    pkt.item_id = item_id;
    pkt.amount = amt;

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
}

/* BB �ӿͻ����Ƴ������� */
int subcmd_send_lobby_bb_delete_meseta(ship_client_t* c, psocn_bb_char_t* character, uint32_t amount, bool drop) {
    lobby_t* l = c->cur_lobby;
    errno_t err = 0;

    if (err = remove_character_meseta(character, amount, c->version != CLIENT_VERSION_BB)) {
        ERR_LOG("���ӵ�е����������� %d < %d", character->disp.meseta, amount);
        return err;
    }

    if (drop) {
        iitem_t tmp_meseta = { 0 };
        tmp_meseta.data.datal[0] = LE32(Item_Meseta);
        tmp_meseta.data.datal[1] = tmp_meseta.data.datal[2] = 0;
        tmp_meseta.data.item_id = generate_item_id(l, c->client_id);
        tmp_meseta.data.data2l = amount;

        /* �������Ʒ... ���������뷿����Ʒ����. */
        litem_t* li_meseta = add_litem_locked(l, &tmp_meseta, c->drop_area, c->x, c->z);
        if (!li_meseta) {
            /* *����������ǿ����... ����ȷ�����û���Ȼ���󲿷֣���ȫ... */
            ERR_LOG("�޷�����Ʒ�������Ϸ����!");
            err = -1;
            return err;
        }

        /* �����������������ݰ�Ҫ����.����,����һ�����ݰ�,����ÿ������һ����Ʒ����.
        Ȼ��,����һ���ӿͻ��˵Ŀ����ɾ����Ʒ����.��һ�����뷢��ÿ����,
        �ڶ������뷢�����������������������������������. */
        if (err = subcmd_send_lobby_drop_stack_bb(c, c->client_id, NULL, li_meseta)) {
            ERR_LOG("��ҵ���������ʧ�� %d < %d", character->disp.meseta, amount);
            return err;
        }
    }

    if (!c->mode)
        c->pl->bb.character.disp.meseta = character->disp.meseta;

    return err;
}

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB ���������Ʒ */
int subcmd_send_lobby_bb_gm_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req) {
    subcmd_bb_itemgen_t gen = { 0 };
    int pkt_size = sizeof(subcmd_bb_itemgen_t);
    int r = LE16(req->entity_id);
    lobby_t* l = c->cur_lobby;

    /* ������ݲ�׼������. */
    gen.hdr.pkt_len = LE16(0x30);
    gen.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    gen.hdr.flags = 0;

    /* ��丱ָ������ */
    gen.shdr.type = SUBCMD60_ITEM_DROP_BOX_ENEMY;
    gen.shdr.size = 0x0B;
    gen.shdr.unused = 0x0000;

    /* ���ʣ������ */
    gen.data.area = req->area;
    gen.data.from_enemy = /*0x02*/req->pt_index != 0x30;
    gen.data.request_id = req->entity_id;
    gen.data.x = req->x;
    gen.data.z = req->z;
    gen.data.unk1 = LE32(0x00000010);

    gen.data.item.datal[0] = LE32(c->new_item.datal[0]);
    gen.data.item.datal[1] = LE32(c->new_item.datal[1]);
    gen.data.item.datal[2] = LE32(c->new_item.datal[2]);
    gen.data.item.data2l = LE32(c->new_item.data2l);
    gen.data.item2 = LE32(0x00000002);

    /* Obviously not "right", but it works though, so we'll go with it. */
    gen.data.item.item_id = LE32((r | 0x06010100));

    /* Send the packet to every client in the lobby. */
    subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&gen, 0);

    /* Clear this out. */
    clear_inv_item(&c->new_item);

    return 0;
}

int subcmd_send_bb_quest_itemreq(ship_client_t* c, subcmd_bb_itemreq_t* req, ship_client_t* dest) {
    uint32_t mid = LE16(req->entity_id);
    uint32_t pti = req->pt_index;
    lobby_t* l = c->cur_lobby;
    uint32_t qdrop = 0xFFFFFFFF;

    if (pti != 0x30 && l->mids)
        qdrop = quest_search_enemy_list(mid, l->mids, l->num_mids, 0);
    if (qdrop == 0xFFFFFFFF && l->mtypes)
        qdrop = quest_search_enemy_list(pti, l->mtypes, l->num_mtypes, 0);

    /* If we found something, the version matters here. Basically, we only care
       about the none option on DC/PC, as rares do not drop in quests. On GC,
       we have to block drops on all options other than free, since we have no
       control over the drop once we send it to the leader.
       ������Ƿ�����ʲô���汾���������Ҫ�������ϣ�����ֻ����DC/PC�ϵġ��ޡ�ѡ�
       ��Ϊϡ�����ֲ������������GC�ϣ����Ǳ�����ֹ������������ѡ���ϵĶ�����
       ��Ϊһ�����ǽ��������͸��쵼�ߣ����Ǿ��޷����ƶ�����*/
    if (qdrop != PSOCN_QUEST_ENDROP_FREE && qdrop != 0xFFFFFFFF) {
        switch (l->version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
        case CLIENT_VERSION_BB:
            if (qdrop == PSOCN_QUEST_ENDROP_NONE)
                return 0;
            break;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            return 0;
        }
    }

    /* If we haven't handled it, we're not supposed to, so send it on to the
       leader. */
    return send_pkt_bb(dest, (bb_pkt_hdr_t*)req);
}

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP ���������Ʒ */
int subcmd_send_bb_drop_item(ship_client_t* dest, subcmd_bb_itemreq_t* req, const iitem_t* item) {
    subcmd_bb_itemgen_t gen = { 0 };
    uint32_t tmp = LE32(req->unk1)/* & 0x0000FFFF*/;

    /* ������ݲ�׼������. */
    gen.hdr.pkt_len = LE16(0x0030);
    gen.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    gen.hdr.flags = 0;

    /* ��丱ָ������ */
    gen.shdr.type = SUBCMD60_ITEM_DROP_BOX_ENEMY;
    gen.shdr.size = 0x0B;
    gen.shdr.unused = 0x0000;

    /* ���ʣ������ */
    gen.data.area = req->area;
    gen.data.from_enemy = req->pt_index != 0x30;   /* Probably not right... but whatever. */
    gen.data.request_id = req->entity_id;
    gen.data.x = req->x;
    gen.data.z = req->z;
    gen.data.unk1 = LE32(tmp);       /* ??? */

    gen.data.item.datal[0] = LE32(item->data.datal[0]);
    gen.data.item.datal[1] = LE32(item->data.datal[1]);
    gen.data.item.datal[2] = LE32(item->data.datal[2]);
    gen.data.item.data2l = LE32(item->data.data2l);

    gen.data.item.item_id = LE32(item->data.item_id);

    /* Send the packet to every client in the lobby. */
    return send_pkt_bb(dest, (bb_pkt_hdr_t*)&gen);
}

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB ���������Ʒ */
int subcmd_send_lobby_bb_drop_item(ship_client_t* src, ship_client_t* nosend, subcmd_bb_itemreq_t* req, const iitem_t* item) {
    subcmd_bb_itemgen_t gen = { 0 };
    uint32_t tmp = LE32(req->unk1)/* & 0x0000FFFF*/;
    lobby_t* l = src->cur_lobby;

    if (!l) {
        ERR_LOG("����һ����Ч�Ĵ�����!");
        return -1;
    }

    for (int i = 0; i < l->max_clients; ++i) {
        if (l->clients[i] && l->clients[i] != nosend) {
            /* If we're supposed to check the ignore list, and this client is on
               it, don't send the packet. */
            if (client_has_ignored(l->clients[i], src->guildcard)) {
                continue;
            }

            subcmd_send_bb_drop_item(l->clients[i], req, item);
        }
    }

    return 0;
}

/* 0x5F SUBCMD60_BOX_ENEMY_ITEM_DROP BB ���������Ʒ */
int subcmd_send_lobby_bb_enemy_item_req(lobby_t* l, subcmd_bb_itemreq_t* req, const iitem_t* item) {
    subcmd_bb_itemgen_t gen = { 0 };
    uint32_t tmp = LE32(req->unk1)/* & 0x0000FFFF*/;

    if (!l)
        return -1;

    /* ������ݲ�׼������. */
    gen.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    gen.hdr.flags = 0;
    gen.hdr.pkt_len = LE16(0x0030);

    /* ��丱ָ������ */
    gen.shdr.type = SUBCMD60_ITEM_DROP_REQ_ENEMY;
    gen.shdr.size = 0x06;
    gen.shdr.unused = 0x0000;

    /* ���ʣ������ */
    gen.data.area = req->area;
    gen.data.from_enemy = req->pt_index != 0x30;   /* Probably not right... but whatever. */
    gen.data.request_id = req->entity_id;
    gen.data.x = req->x;
    gen.data.z = req->z;
    gen.data.unk1 = LE32(tmp);       /* ??? */

    gen.data.item.datal[0] = LE32(item->data.datal[0]);
    gen.data.item.datal[1] = LE32(item->data.datal[1]);
    gen.data.item.datal[2] = LE32(item->data.datal[2]);
    gen.data.item.data2l = LE32(item->data.data2l);

    gen.data.item.item_id = LE32(item->data.item_id);

    /* Send the packet to every client in the lobby. */
    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&gen, 0);
}

/* 0xBC SUBCMD60_BANK_INV BB ������� */
int subcmd_send_bb_bank(ship_client_t* src, psocn_bank_t* bank) {
    uint8_t* sendbuf = get_sendbuf();
    subcmd_bb_bank_inv_t* pkt = (subcmd_bb_bank_inv_t*)sendbuf;
    uint32_t num_items = LE32(bank->item_count);
    uint16_t size = sizeof(subcmd_bb_bank_inv_t) + num_items *
        PSOCN_STLENGTH_BITEM;
    block_t* b = src->cur_block;

    /* ������ݲ�׼������ */
    pkt->hdr.pkt_len = LE16(size);
    pkt->hdr.pkt_type = LE16(GAME_SUBCMD6C_TYPE);
    pkt->hdr.flags = 0;

    pkt->shdr.type = SUBCMD60_BANK_INV;
    pkt->shdr.size = 0;
    pkt->shdr.unused = 0x0000;

    pkt->size = LE32((num_items + 1) * PSOCN_STLENGTH_BITEM);
    //pkt->checksum = mt19937_genrand_int32(&b->rng); /* Client doesn't care */
    pkt->checksum = sfmt_genrand_uint32(&b->sfmt_rng); /* Client doesn't care */
    pkt->item_count = bank->item_count;
    pkt->meseta = bank->meseta;

    if (bank->item_count) {
        memcpy(&pkt->bitems, &bank->bitems[0], sizeof(bitem_t) * bank->item_count);
    }

    return send_pkt_bb(src, (bb_pkt_hdr_t*)pkt);
}

/* 0xBF SUBCMD60_GIVE_EXP BB ��һ�þ��� */
int subcmd_send_lobby_bb_exp(ship_client_t* dest, uint32_t exp_amount) {
    lobby_t* l = dest->cur_lobby;
    subcmd_bb_exp_t pkt = { 0 };
    int pkt_size = sizeof(subcmd_bb_exp_t);

    if (!l)
        return -1;

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = GAME_SUBCMD60_TYPE;
    pkt.hdr.flags = 0;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD60_GIVE_EXP;
    pkt.shdr.size = (pkt_size - 8) / 4;
    pkt.shdr.client_id = dest->client_id;

    /* ���ʣ������ */
    pkt.exp_amount = LE32(exp_amount);

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
}

/* 0xDD SUBCMD60_SET_EXP_RATE BB ������Ϸ���鱶�� */
int subcmd_send_bb_set_exp_rate(ship_client_t* c, uint32_t exp_rate) {
    lobby_t* l = c->cur_lobby;
    subcmd_bb_set_exp_rate_t pkt = { 0 };
    int pkt_size = sizeof(subcmd_bb_set_exp_rate_t);
    uint16_t exp_r = 0;
    int rv = 0;

    if (!l)
        return -1;

    if (c->game_data->expboost <= 0)
        c->game_data->expboost = 1;

    exp_r = exp_rate * c->game_data->expboost;

    if (exp_r <= 1)
        exp_r = 1;

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    pkt.hdr.flags = 0;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD60_SET_EXP_RATE;
    pkt.shdr.size = pkt_size / 4;
    pkt.shdr.params = LE16(exp_r);

    rv = send_pkt_bb(c, (bb_pkt_hdr_t*)&pkt);

    if (rv) {
        ERR_LOG("GC %" PRIu32 " ���÷��侭�� %u ����������!",
            c->guildcard, exp_r);
    }
    else {
        if (l->exp_mult < LE32(exp_r)) {
            l->exp_mult = LE32(exp_r);
#ifdef DEBUG
            DBG_LOG("GC %" PRIu32 " �ķ��侭�鱶������Ϊ %u ��", c->guildcard, l->exp_mult);
#endif // DEBUG
        }
    }

    if (l->exp_mult == 0)
        l->exp_mult = 1;

    return rv;
}

/* 0xDB SUBCMD60_EXCHANGE_ITEM_IN_QUEST BB �����жһ���Ʒ */
int subcmd_send_bb_exchange_item_in_quest(ship_client_t* c, uint32_t item_id, uint32_t amount) {
    subcmd_bb_item_exchange_in_quest_t pkt = { 0 };
    int pkt_size = sizeof(subcmd_bb_item_exchange_in_quest_t);

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    pkt.hdr.flags = 0;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD60_ITEM_EXCHANGE_IN_QUEST;
    pkt.shdr.size = pkt_size / 4;
    pkt.shdr.params = 0;

    pkt.unknown_a3 = 1;
    pkt.item_id = item_id;
    pkt.amount = amount;

    return send_pkt_bb(c, (bb_pkt_hdr_t*)&pkt);
}

/* 0x30 SUBCMD60_LEVEL_UP BB ���������ֵ�仯 */
int subcmd_send_lobby_bb_level(ship_client_t* dest) {
    lobby_t* l = dest->cur_lobby;
    subcmd_bb_level_up_t pkt = { 0 };
    int pkt_size = sizeof(subcmd_bb_level_up_t);
    int i;
    uint16_t base, mag;

    if (!l)
        return -1;

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = LE16(GAME_SUBCMD60_TYPE);
    pkt.hdr.flags = 0;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD60_LEVEL_UP;
    pkt.shdr.size = (pkt_size - 8) / 4;
    pkt.shdr.client_id = dest->client_id;

    psocn_bb_char_t* character = get_client_char_bb(dest);

    /* ��������������. ��Ϊ little-endian �ַ���. */
    pkt.atp = character->disp.stats.atp;
    pkt.mst = character->disp.stats.mst;
    pkt.evp = character->disp.stats.evp;
    pkt.hp = character->disp.stats.hp;
    pkt.dfp = character->disp.stats.dfp;
    pkt.ata = character->disp.stats.ata;
    pkt.level = character->disp.level;

    /* ����MAG����������. */
    for (i = 0; i < character->inv.item_count; ++i) {
        if ((character->inv.iitems[i].flags & LE32(0x00000008)) &&
            character->inv.iitems[i].data.datab[0] == ITEM_TYPE_MAG) {
            base = LE16(pkt.dfp);
            mag = LE16(character->inv.iitems[i].data.dataw[2]) / 100;
            pkt.dfp = LE16((base + mag));

            base = LE16(pkt.atp);
            mag = LE16(character->inv.iitems[i].data.dataw[3]) / 50;
            pkt.atp = LE16((base + mag));

            base = LE16(pkt.ata);
            mag = LE16(character->inv.iitems[i].data.dataw[4]) / 200;
            pkt.ata = LE16((base + mag));

            base = LE16(pkt.mst);
            mag = LE16(character->inv.iitems[i].data.dataw[5]) / 50;
            pkt.mst = LE16((base + mag));

            break;
        }
    }

    return subcmd_send_lobby_bb(l, NULL, (subcmd_bb_pkt_t*)&pkt, 0);
}

/* 0xB6 SUBCMD60_SHOP_INV BB ����ҷ��ͻ����嵥 */
int subcmd_bb_send_shop(ship_client_t* dest, uint8_t shop_type, uint8_t num_items, bool create) {
    lobby_t* l = dest->cur_lobby;
    subcmd_bb_shop_inv_t shop = { 0 };

    if (!l) {
        ERR_LOG("GC %" PRIu32 " �����κη�����!",
            dest->guildcard);
        return -1;
    }

    if (!create) {
        ERR_LOG("GC %" PRIu32 " �̵�˵����ݴ���! create %d shop_type %d num_items %d",
            dest->guildcard, create, shop_type, num_items);
        return send_bb_error_menu_list(dest);
    }

    if (num_items > ARRAY_SIZE(shop.items)) {
        ERR_LOG("GC %" PRIu32 " ��ȡ�̵���Ʒ��������! ���� %d > %d",
            dest->guildcard, num_items, ARRAY_SIZE(shop.items));
        return send_msg(dest, MSG1_TYPE, "%s", __(dest, "\tE\tC4�̵����ɴ���,��ȡ�̵���Ʒ��������,����ϵ����Ա����!"));
    }

    uint16_t len = LE16(16) + num_items * PSOCN_STLENGTH_ITEM;

    shop.hdr.pkt_len = LE16(len); //236 - 220 11 * 20 = 16 + num_items * PSOCN_STLENGTH_ITEM
    shop.hdr.pkt_type = LE16(GAME_SUBCMD6C_TYPE);
    shop.hdr.flags = 0;

    /* ��丱ָ������ */
    shop.shdr.type = SUBCMD60_SHOP_INV;
    shop.shdr.size = sizeof(dest->game_data->shop_items) / 4;
    shop.shdr.params = LE16(0x0000);

    /* ���ʣ������ */
    shop.shop_type = shop_type;
    shop.num_items = num_items;

    for (uint8_t i = 0; i < num_items; ++i) {
        clear_inv_item(&shop.items[i]);
        shop.items[i] = dest->game_data->shop_items[i];
    }

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)&shop);
}

/* 0xE3 SUBCMD62_COREN_ACT_RESULT BB ����ҷ��Ϳ��׶Ĳ���� */
int subcmd_bb_send_coren_reward(ship_client_t* dest, uint32_t flags, item_t item) {
    lobby_t* l = dest->cur_lobby;
    subcmd_bb_coren_act_result_t pkt = { 0 };
    int pkt_size = sizeof(subcmd_bb_coren_act_result_t);

    if (!l) {
        ERR_LOG("GC %" PRIu32 " �����κη�����!",
            dest->guildcard);
        return -1;
    }

    /* ������ݲ�׼������ */
    pkt.hdr.pkt_len = LE16(pkt_size);
    pkt.hdr.pkt_type = LE16(GAME_SUBCMD62_TYPE);
    pkt.hdr.flags = flags;

    /* ��丱ָ������ */
    pkt.shdr.type = SUBCMD62_COREN_ACT_RESULT;
    pkt.shdr.size = pkt_size / 4;
    pkt.shdr.client_id = dest->client_id;

    pkt.item_data = item;

    return send_pkt_bb(dest, (bb_pkt_hdr_t*)&pkt);
}

int subcmd_bb_60size_check(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    lobby_t* l = c->cur_lobby;
    int sent = 0;
    uint16_t size = pkt->hdr.pkt_len;
    uint16_t sizecheck = pkt->size;

    //if (type != SUBCMD60_MOVE_FAST && type != SUBCMD60_MOVE_SLOW) {
    //    switch (l->type) {

    //    case LOBBY_TYPE_LOBBY:
    //        DBG_LOG("BB������� GC %" PRIu32 " 60ָ��: 0x%02X", c->guildcard, type);
    //        break;

    //    case LOBBY_TYPE_GAME:
    //        if (l->flags & LOBBY_FLAG_QUESTING) {
    //            DBG_LOG("BB�������� GC %" PRIu32 " 60ָ��: 0x%02X", c->guildcard, type);
    //        }
    //        else
    //            DBG_LOG("BB������ͨ��Ϸ GC %" PRIu32 " 60ָ��: 0x%02X", c->guildcard, type);
    //        break;

    //    default:
    //        DBG_LOG("BB����ͨ�� GC %" PRIu32 " 60ָ��: 0x%02X", c->guildcard, type);
    //        break;
    //    }
    //}

    sizecheck *= 4;
    sizecheck += 8;

    if (!l)
        return -1;

    if (size != sizecheck)
    {
        DBG_LOG("�ͻ��˷��� 0x60 ָ�����ݰ� ��С��һ�� sizecheck != size.");
        DBG_LOG("ָ��: 0x%02X | ��С: %04X | Sizecheck: %04X", pkt->type,
            size, sizecheck);
        //pkt->size = ((size / 4) - 2);
    }

    if (l->type == LOBBY_TYPE_LOBBY)
    {
        type *= 2;

        if (type == SUBCMD62_GUILDCARD)
            sizecheck = sizeof(subcmd_bb_gcsend_t);
        else
            sizecheck = size_ct[type + 1] + 4;

        if ((size != sizecheck) && (sizecheck > 4))
            sent = 1;

        // No size check packet encountered while in Lobby_Room mode...
        if (sizecheck == 4) {
            //DBG_LOG("û��0x60ָ�� 0x%04X �Ĵ�С�����Ϣ", type);
            sent = 1;
        }
    }
    else
    {
        if (dont_send_60[(type * 2) + 1] == 1)
        {
            sent = 1;
            print_ascii_hex(dbgl, pkt, pkt->hdr.pkt_len);
        }
    }

    if ((pkt->data[1] != c->client_id) &&
        (size_ct[(type * 2) + 1] != 0x00) &&
        (type != SUBCMD60_SYMBOL_CHAT) &&
        (type != SUBCMD60_GOGO_BALL))
        sent = 1;

    if ((type == SUBCMD60_SYMBOL_CHAT) &&
        (pkt->data[1] != c->client_id))
        sent = 1;

    if (type == SUBCMD60_BURST_DONE)
        sent = 1;

    return sent;
}

int subcmd_bb_626Dsize_check(ship_client_t* c, subcmd_bb_pkt_t* pkt) {
    uint8_t type = pkt->type;
    int sent = 0;
    uint16_t size = pkt->hdr.pkt_len;
    uint16_t sizecheck = pkt->size;

    //DBG_LOG("BB���� GC %" PRIu32 " 62/6Dָ��: 0x%02X", c->guildcard, type);

    //print_ascii_hex(pkt, LE16(pkt->hdr.pkt_len));

    sizecheck *= 4;
    sizecheck += 8;

    if (size != sizecheck) {
        DBG_LOG("�ͻ��˷��� 0x6D62 ָ�����ݰ� ��С��һ��.");
        DBG_LOG("ָ��: 0x%02X | pkt_len: %04X | Sizecheck: %04X", type,
            size, sizecheck);
        sent = 1;
        //pkt->size = (size / 4);
    }

    if (type >= 0x04)
        sent = 1;

    return sent;
}