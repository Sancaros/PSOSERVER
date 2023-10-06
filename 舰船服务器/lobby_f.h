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

#include "f_logs.h"
#include "clients.h"
#include "lobby.h"

#define LOBBY_PICKITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        PICKS_LOG("---------���� %d ��ƷID 0x%08X ʰȡ���--------- ", area, item_id); \
        PICKS_LOG("%s %s ������ %d ʰȡ���!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) \
            PICKS_LOG("%s", get_lobby_describe(l)); \
	        PICKS_LOG("��Ʒ: %s", get_item_describe(item, (c)->version));\
            PICKS_LOG("���: 0x%08X", item_id);\
            PICKS_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",\
                        (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3],\
                        (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7],\
                        (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11],\
                        (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]);\
            PICKS_LOG("----------------------------------------------------");\
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0)

#define LOBBY_DROPITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        DROPS_LOG("---------���� %d ��ƷID 0x%08X ʰȡ���--------- ", area, item_id); \
        DROPS_LOG("%s %s ������ %d ʰȡ���!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) \
            DROPS_LOG("%s", get_lobby_describe(l)); \
	        DROPS_LOG("��Ʒ: %s", get_item_describe(item, (c)->version));\
            DROPS_LOG("���: 0x%08X", item_id);\
            DROPS_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",\
                        (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3],\
                        (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7],\
                        (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11],\
                        (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]);\
            DROPS_LOG("----------------------------------------------------");\
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0)

#define LOBBY_MDROPITEM_LOG(c, mid, pt_index, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        DROPS_LOG("---------���� %d �� %d ������������--------- ", area, mid); \
        DROPS_LOG("%s %s ������ %d ������� (%d -- max:%d)!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
            , mid \
            , l->map_enemies->enemy_count \
        ); \
        if (l) \
            DROPS_LOG("%s", get_lobby_enemy_pt_name_with_mid(l, pt_index, mid));\
            DROPS_LOG("%s", get_lobby_describe(l)); \
	        DROPS_LOG("��Ʒ: %s", get_item_describe(item, (c)->version));\
            DROPS_LOG("���: 0x%08X", (item)->item_id);\
            DROPS_LOG("����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",\
                        (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3],\
                        (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7],\
                        (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11],\
                        (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]);\
            DROPS_LOG("----------------------------------------------------");\
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0)