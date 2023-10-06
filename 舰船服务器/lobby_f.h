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

#include "f_logs.h"
#include "clients.h"
#include "lobby.h"

#define LOBBY_BANK_DEPOSIT_ITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        BANK_DEPOSIT_LOG("---------区域 %d 物品ID 0x%08X 银行取物--------- ", area, item_id); \
        BANK_DEPOSIT_LOG("%s %s 在区域 %d 银行取物!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) \
            BANK_DEPOSIT_LOG("%s", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            BANK_DEPOSIT_LOG("物品: %s", get_item_describe(item, (c)->version));\
                BANK_DEPOSIT_LOG("编号: 0x%08X", item_id); \
                BANK_DEPOSIT_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                BANK_DEPOSIT_LOG("----------------------------------------------------"); \
            } else { \
                BANK_DEPOSIT_LOG("%s 存入无效物品", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
    } \
    else \
        ERR_LOG("%s 不在一个有效的房间内", get_player_describe(c)); \
} while (0)

#define LOBBY_BANK_TAKE_ITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        BANK_TAKE_LOG("---------区域 %d 物品ID 0x%08X 银行取物--------- ", area, item_id); \
        BANK_TAKE_LOG("%s %s 在区域 %d 银行取物!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) \
            BANK_TAKE_LOG("%s", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            BANK_TAKE_LOG("物品: %s", get_item_describe(item, (c)->version));\
                BANK_TAKE_LOG("编号: 0x%08X", item_id); \
                BANK_TAKE_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                BANK_TAKE_LOG("----------------------------------------------------"); \
            } else { \
                BANK_TAKE_LOG("%s 取出无效物品", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
    } \
    else \
        ERR_LOG("%s 不在一个有效的房间内", get_player_describe(c)); \
} while (0)

#define LOBBY_TEKKITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        TEKKS_LOG("---------区域 %d 物品ID 0x%08X 鉴定情况--------- ", area, item_id); \
        TEKKS_LOG("%s %s 在区域 %d 鉴定情况!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) \
            TEKKS_LOG("%s", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            TEKKS_LOG("物品: %s", get_item_describe(item, (c)->version));\
                TEKKS_LOG("编号: 0x%08X", item_id); \
                TEKKS_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                TEKKS_LOG("----------------------------------------------------"); \
            } else { \
                TEKKS_LOG("%s 鉴定无效物品", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
    } \
    else \
        ERR_LOG("%s 不在一个有效的房间内", get_player_describe(c)); \
} while (0)

#define LOBBY_PICKITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        PICKS_LOG("---------区域 %d 物品ID 0x%08X 拾取情况--------- ", area, item_id); \
        PICKS_LOG("%s %s 在区域 %d 拾取情况!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) \
            PICKS_LOG("%s", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            PICKS_LOG("物品: %s", get_item_describe(item, (c)->version));\
                PICKS_LOG("编号: 0x%08X", item_id); \
                PICKS_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                PICKS_LOG("----------------------------------------------------"); \
            } else { \
                PICKS_LOG("%s 拾取无效物品", get_player_describe(c)); \
                print_ascii_hex(pickl, item, PSOCN_STLENGTH_ITEM); \
            }\
    } \
    else \
        ERR_LOG("%s 不在一个有效的房间内", get_player_describe(c)); \
} while (0)

#define LOBBY_DROPITEM_LOG(c, item_id, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        DROPS_LOG("---------区域 %d 物品ID 0x%08X 掉落情况--------- ", area, item_id); \
        DROPS_LOG("%s %s 在区域 %d 掉落情况!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
        ); \
        if (l) \
            DROPS_LOG("%s", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            DROPS_LOG("物品: %s", get_item_describe(item, (c)->version));\
                DROPS_LOG("编号: 0x%08X", item_id); \
                DROPS_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                DROPS_LOG("----------------------------------------------------"); \
            } else {\
                DROPS_LOG("%s 掉落无效物品", get_player_describe(c)); \
                print_ascii_hex(dropl, item, PSOCN_STLENGTH_ITEM);\
            }\
    } \
    else \
        ERR_LOG("%s 不在一个有效的房间内", get_player_describe(c)); \
} while (0)

#define LOBBY_MOB_DROPITEM_LOG(c, mid, pt_index, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        MDROPS_LOG("---------区域 %d 第 %d 个怪物掉落情况--------- ", area, mid); \
        MDROPS_LOG("%s %s 在区域 %d 怪物掉落 (%d -- max:%d)!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , (c)->cur_area \
            , mid \
            , l->map_enemies->enemy_count \
        ); \
        if (l) \
            MDROPS_LOG("%s", get_lobby_enemy_pt_name_with_mid(l, pt_index, mid));\
            MDROPS_LOG("%s", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            MDROPS_LOG("物品: %s", get_item_describe(item, (c)->version));\
                MDROPS_LOG("编号: 0x%08X", (item)->item_id); \
                MDROPS_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                MDROPS_LOG("----------------------------------------------------"); \
            } else {\
                MDROPS_LOG("%s 掉落无效物品", get_lobby_enemy_pt_name_with_mid(l, pt_index, mid)); \
                print_ascii_hex(mdropl, item, PSOCN_STLENGTH_ITEM);\
            }\
    } \
    else \
        ERR_LOG("%s 不在一个有效的房间内", get_player_describe(c)); \
} while (0)

#define LOBBY_BOX_DROPITEM_LOG(c, request_id, pt_index, ignore_def, area, item) \
do { \
    lobby_t* l = (c)->cur_lobby; \
    if (l) { \
        BDROPS_LOG("---------区域 %d ID %d 箱子掉落情况--------- ", area, request_id); \
        BDROPS_LOG("%s %s 请求 %d 区域 %d 箱子掉落!" \
            , get_player_describe(c) \
            , get_section_describe(c, get_player_section(c), true) \
            , ignore_def\
            , (c)->cur_area \
            , request_id \
        ); \
        if (l) \
            BDROPS_LOG("%s", get_lobby_describe(l)); \
            if(!item_not_identification_bb((item)->datal[0], (item)->datal[1])){\
	            BDROPS_LOG("物品: %s", get_item_describe(item, (c)->version));\
                BDROPS_LOG("编号: 0x%08X", (item)->item_id); \
                BDROPS_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X", \
                            (item)->datab[0], (item)->datab[1], (item)->datab[2], (item)->datab[3], \
                            (item)->datab[4], (item)->datab[5], (item)->datab[6], (item)->datab[7], \
                            (item)->datab[8], (item)->datab[9], (item)->datab[10], (item)->datab[11], \
                            (item)->data2b[0], (item)->data2b[1], (item)->data2b[2], (item)->data2b[3]); \
                BDROPS_LOG("----------------------------------------------------"); \
            } else {\
                BDROPS_LOG("请求 %d 箱子掉落无效物品", ignore_def); \
                print_ascii_hex(bdropl, item, PSOCN_STLENGTH_ITEM);\
            }\
    } \
    else \
        ERR_LOG("%s 不在一个有效的房间内", get_player_describe(c)); \
} while (0)