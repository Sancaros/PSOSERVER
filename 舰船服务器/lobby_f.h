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

#define LOBBY_GM_MAKE_ITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        char buff[512]; \
        int len = 0; \
        len += snprintf(buff + len, sizeof(buff) - len, "\n---------���� %d ��ƷID 0x%08X GMȨ������--------- \n", area, item_id); \
        len += snprintf(buff + len, sizeof(buff) - len, "%s %s ������ %d GMȨ������!\n" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) { \
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            len += snprintf(buff + len, sizeof(buff) - len, "��Ʒ: %s\n", get_item_describe(item, (c)->version)); \
                len += snprintf(buff + len, sizeof(buff) - len, "���: 0x%08X\n", item_id); \
                len += snprintf(buff + len, sizeof(buff) - len, "����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X\n", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                len += snprintf(buff + len, sizeof(buff) - len, "----------------------------------------------------\n"); \
            } else { \
                len += snprintf(buff + len, sizeof(buff) - len, "%s GMȨ��������Ч��Ʒ\n", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
        } \
        buff[sizeof(buff) - 1] = '\0'; \
        GM_LOG("%s", buff); \
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0)

#define LOBBY_BANK_DEPOSIT_ITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        char buff[512]; \
        int len = 0; \
        len += snprintf(buff + len, sizeof(buff) - len, "\n---------���� %d ��ƷID 0x%08X ���д���--------- \n", area, item_id); \
        len += snprintf(buff + len, sizeof(buff) - len, "%s %s ������ %d ���д���!\n" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) { \
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            len += snprintf(buff + len, sizeof(buff) - len, "��Ʒ: %s\n", get_item_describe(item, (c)->version)); \
                len += snprintf(buff + len, sizeof(buff) - len, "���: 0x%08X\n", item_id); \
                len += snprintf(buff + len, sizeof(buff) - len, "����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X\n", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                len += snprintf(buff + len, sizeof(buff) - len, "----------------------------------------------------\n"); \
            } else { \
                len += snprintf(buff + len, sizeof(buff) - len, "%s ������Ч��Ʒ\n", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
        } \
        buff[sizeof(buff) - 1] = '\0'; \
        BANK_DEPOSIT_LOG("%s", buff); \
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0)

#define LOBBY_BANK_TAKE_ITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        char buff[512]; \
        int len = 0; \
        len += snprintf(buff + len, sizeof(buff) - len, "\n---------���� %d ��ƷID 0x%08X ����ȡ��--------- \n", area, item_id); \
        len += snprintf(buff + len, sizeof(buff) - len, "%s %s ������ %d ����ȡ��!\n" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) { \
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            len += snprintf(buff + len, sizeof(buff) - len, "��Ʒ: %s\n", get_item_describe(item, (c)->version)); \
                len += snprintf(buff + len, sizeof(buff) - len, "���: 0x%08X\n", item_id); \
                len += snprintf(buff + len, sizeof(buff) - len, "����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X\n", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                len += snprintf(buff + len, sizeof(buff) - len, "----------------------------------------------------\n"); \
            } else { \
                len += snprintf(buff + len, sizeof(buff) - len, "%s ȡ����Ч��Ʒ\n", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
        } \
        buff[sizeof(buff) - 1] = '\0'; \
        BANK_TAKE_LOG("%s", buff); \
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0)

#define LOBBY_TEKKITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        char buff[512]; \
        int len = 0; \
        len += snprintf(buff + len, sizeof(buff) - len, "\n---------���� %d ��ƷID 0x%08X �������--------- \n", area, item_id); \
        len += snprintf(buff + len, sizeof(buff) - len, "%s %s ������ %d �������!\n" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) { \
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            len += snprintf(buff + len, sizeof(buff) - len, "��Ʒ: %s\n", get_item_describe(item, (c)->version)); \
                len += snprintf(buff + len, sizeof(buff) - len, "���: 0x%08X\n", item_id); \
                len += snprintf(buff + len, sizeof(buff) - len, "����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X\n", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                len += snprintf(buff + len, sizeof(buff) - len, "----------------------------------------------------\n"); \
            } else { \
                len += snprintf(buff + len, sizeof(buff) - len, "%s ������Ч��Ʒ\n", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
        } \
        buff[sizeof(buff) - 1] = '\0'; \
        TEKKS_LOG("%s", buff); \
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0)

#define LOBBY_PICKITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        char buff[512]; \
        int len = 0; \
        len += snprintf(buff + len, sizeof(buff) - len, "\n---------���� %d ��ƷID 0x%08X ʰȡ���--------- \n", area, item_id); \
        len += snprintf(buff + len, sizeof(buff) - len, "%s %s ������ %d ʰȡ���!\n" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) { \
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){ \
	            len += snprintf(buff + len, sizeof(buff) - len, "��Ʒ: %s\n", get_item_describe(item, (c)->version)); \
                len += snprintf(buff + len, sizeof(buff) - len, "���: 0x%08X\n", item_id); \
                len += snprintf(buff + len, sizeof(buff) - len, "����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X\n", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                len += snprintf(buff + len, sizeof(buff) - len, "----------------------------------------------------\n"); \
            } else { \
                len += snprintf(buff + len, sizeof(buff) - len, "%s ʰȡ��Ч��Ʒ\n", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
        } \
        buff[sizeof(buff) - 1] = '\0'; \
        PICKS_LOG("%s", buff); \
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0)

#define LOBBY_DROPITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        char buff[512]; \
        int len = 0; \
        len += snprintf(buff + len, sizeof(buff) - len, "\n---------���� %d ��ƷID 0x%08X �������--------- \n", area, item_id); \
        len += snprintf(buff + len, sizeof(buff) - len, "%s %s ������ %d �������!\n" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) { \
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_describe(l)); \
            if (!item_not_identification_bb((item)->datal[0], (item)->datal[1])) { \
                len += snprintf(buff + len, sizeof(buff) - len, "��Ʒ: %s\n", get_item_describe(item, (c)->version)); \
                len += snprintf(buff + len, sizeof(buff) - len, "���: 0x%08X\n", item_id); \
                len += snprintf(buff + len, sizeof(buff) - len, "����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X\n", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                len += snprintf(buff + len, sizeof(buff) - len, "----------------------------------------------------\n"); \
            } else { \
                len += snprintf(buff + len, sizeof(buff) - len, "%s ������Ч��Ʒ\n", get_player_describe(c)); \
                print_ascii_hex(dropl, item, PSOCN_STLENGTH_ITEM); \
            } \
        } \
        buff[sizeof(buff) - 1] = '\0'; \
        DROPS_LOG("%s", buff); \
    } \
    else { \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
    } \
} while (0)

#define LOBBY_MOB_DROPITEM_LOG(c, mid, pt_index, area, pt_area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        char buff[512]; \
        int len = 0; \
        len += snprintf(buff + len, sizeof(buff) - len, "\n---------���� %d(pt_area %d) �� %d ������(���:%d)�������--------- \n", area, pt_area, mid, l->map_enemies->enemy_count); \
        len += snprintf(buff + len, sizeof(buff) - len, "%s %s ������ %d �������\n" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) { \
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_enemy_pt_name_with_mid(l, pt_index, mid));\
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_describe(l)); \
            if (!item_not_identification_bb((item)->datal[0], (item)->datal[1])) { \
                len += snprintf(buff + len, sizeof(buff) - len, "��Ʒ: %s\n", get_item_describe(item, (c)->version)); \
                len += snprintf(buff + len, sizeof(buff) - len, "���: 0x%08X\n", (item)->item_id); \
                len += snprintf(buff + len, sizeof(buff) - len, "����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X\n", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                len += snprintf(buff + len, sizeof(buff) - len, "----------------------------------------------------\n"); \
            } else { \
                len += snprintf(buff + len, sizeof(buff) - len, "%s ������Ч��Ʒ\n", get_lobby_enemy_pt_name_with_mid(l, pt_index, mid)); \
                print_ascii_hex(mdropl, item, PSOCN_STLENGTH_ITEM);\
            } \
        } \
        buff[sizeof(buff) - 1] = '\0'; \
        MDROPS_LOG("%s", buff); \
    } \
    else \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
} while (0) \

#define LOBBY_BOX_DROPITEM_LOG(c, request_id, pt_index, ignore_def, area, pt_area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        char buff[512]; \
        int len = 0; \
        len += snprintf(buff + len, sizeof(buff) - len, "\n---------���� %d(pt_area %d) ID %d ���ӵ������--------- \n", area, pt_area, request_id); \
        len += snprintf(buff + len, sizeof(buff) - len, "%s %s ����Ԥ�� %d ������ %d ���ӵ���!\n" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , ignore_def\
            , (c)->cur_area \
        ); \
        if (l) { \
            len += snprintf(buff + len, sizeof(buff) - len, "%s\n", get_lobby_describe(l)); \
            if (!item_not_identification_bb((item)->datal[0], (item)->datal[1])) { \
                len += snprintf(buff + len, sizeof(buff) - len, "��Ʒ: %s\n", get_item_describe(item, (c)->version)); \
                len += snprintf(buff + len, sizeof(buff) - len, "���: 0x%08X\n", (item)->item_id); \
                len += snprintf(buff + len, sizeof(buff) - len, "����: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X\n", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                len += snprintf(buff + len, sizeof(buff) - len, "----------------------------------------------------\n"); \
            } else { \
                len += snprintf(buff + len, sizeof(buff) - len, "����Ԥ�� %d ���ӵ�����Ч��Ʒ\n", ignore_def); \
                print_ascii_hex(bdropl, item, PSOCN_STLENGTH_ITEM); \
            } \
        } \
        buff[sizeof(buff) - 1] = '\0'; \
        BDROPS_LOG("%s", buff); \
    } else { \
        ERR_LOG("%s ����һ����Ч�ķ�����", get_player_describe(c)); \
    } \
} while (0)
