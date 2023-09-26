/*
    梦幻之星中国 舰船服务器 62指令处理
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

#include "subcmd_handle.h"

extern subcmd_handle_func_t subcmd60_handler[0x100];
extern subcmd_handle_func_t subcmdmode_handler[0x100];
extern subcmd_handle_func_t subcmd62_handler[0x100];
extern subcmd_handle_func_t subcmd6D_handler[0x100];

subcmd_handle_t subcmd_search_handler(
    subcmd_handle_func_t* handler,
    size_t max_src,
    int subcmd_type,
    int version) {

    for (size_t i = 0; i < max_src; i++) {
        if (handler[i].subcmd_type == subcmd_type) {

            //switch (version)
            //{
            //case CLIENT_VERSION_DCV1:
            //case CLIENT_VERSION_DCV2:
            //    return handler[i].dc;

            //case CLIENT_VERSION_PC:
            //    return handler[i].pc;

            //case CLIENT_VERSION_GC:
            //    return handler[i].gc;

            //case CLIENT_VERSION_EP3:
            //    return handler[i].ep3;

            //case CLIENT_VERSION_XBOX:
            //    return handler[i].xb;

            //case CLIENT_VERSION_BB:
            //    return handler[i].bb;
            //}

            switch (version)
            {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
            case CLIENT_VERSION_PC:
            case CLIENT_VERSION_GC:
            case CLIENT_VERSION_EP3:
            case CLIENT_VERSION_XBOX:
                return handler[i].dc;

            case CLIENT_VERSION_BB:
                return handler[i].bb;
            }
        }
    }

    return NULL;
}

// 使用函数指针直接调用相应的处理函数
subcmd_handle_t subcmd_get_handler(int cmd_type, int subcmd_type, int version) {
    __try {
        subcmd_handle_t func = { 0 };
        size_t count = 0;

        switch (cmd_type)
        {
        case GAME_SUBCMD60_TYPE:
            count = _countof(subcmd60_handler);
            func = subcmd_search_handler(subcmd60_handler, count, subcmd_type, version);
            break;

        case GAME_SUBCMD62_TYPE:
            count = _countof(subcmd62_handler);
            func = subcmd_search_handler(subcmd62_handler, count, subcmd_type, version);
            break;

        case GAME_SUBCMD6D_TYPE:
            count = _countof(subcmd6D_handler);
            func = subcmd_search_handler(subcmd6D_handler, count, subcmd_type, version);
            break;
        }

        if (!func)
            ERR_LOG("subcmd_get_handler 未完成对 "
                "0x%02X 0x%02X 版本 %s(%d) 的处理", cmd_type, subcmd_type, client_type[version].ver_name_file, version);

        return func;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return NULL;
    }
}
//
///* Possible values for the type parameter. */
//#define LOBBY_TYPE_LOBBY              0x00000001
//#define LOBBY_TYPE_GAME               0x00000002
//#define LOBBY_TYPE_EP3_GAME           0x00000004
//#define LOBBY_TYPE_PERSISTENT         0x00000008
//// Flags used only for games
//#define LOBBY_TYPE_CHEATS_ENABLED     0x00000100

/* 检测玩家是否在有效的游戏房间中 */
bool in_game(ship_client_t* src) {
    lobby_t* l = src->cur_lobby;

    if (!l) {
        ERR_LOG("%s 不在有效大厅中!", get_player_describe(src));
        return false;
    }

    if (src->version != CLIENT_VERSION_EP3) {

        if (l->type != LOBBY_TYPE_GAME) {
            ERR_LOG("%s 不在游戏房间中!", get_player_describe(src));
            ERR_LOG("当前房间信息:");
            ERR_LOG("章节: %d 难度: %d 区域: %d", l->episode, l->difficulty, src->cur_area);
            return false;
        }

    }
    else {

        if (l->type != LOBBY_TYPE_EP3_GAME) {
            ERR_LOG("%s 不在EP3游戏房间中!", get_player_describe(src));
            ERR_LOG("当前房间信息:");
            ERR_LOG("章节: %d 难度: %d 区域: %d", l->episode, l->difficulty, src->cur_area);
            return false;
        }
    }

    return true;
}

/* 检测玩家是否在有效的大厅中 */
bool in_lobby(ship_client_t* src) {
    lobby_t* l = src->cur_lobby;

    if (!l) {
        ERR_LOG("%s 不在有效大厅中!", get_player_describe(src));
        return false;
    }

    if (l->type != LOBBY_TYPE_LOBBY) {
        return false;
    }

    return true;
}

bool check_pkt_size(ship_client_t* src, void* pkt, uint16_t len, uint8_t size) {
    uint16_t pkt_type = 0;
    uint8_t pkt_subtype = 0, pkt_size = 0;
    size_t pkt_len = 0;

    switch (src->version)
    {
    case CLIENT_VERSION_BB:
        subcmd_bb_pkt_t* pkt_bb = (subcmd_bb_pkt_t*)pkt;
        pkt_type = pkt_bb->hdr.pkt_type, pkt_subtype = pkt_bb->type;
        pkt_len = pkt_bb->hdr.pkt_len;
        pkt_size = pkt_bb->size;
        break;

    case CLIENT_VERSION_PC:
        subcmd_pkt_t* pkt_pc = (subcmd_pkt_t*)pkt;
        pkt_type = pkt_pc->hdr.pc.pkt_type, pkt_subtype = pkt_pc->type;
        pkt_len = pkt_pc->hdr.pc.pkt_len;
        pkt_size = pkt_pc->size;
        break;


    default:
        subcmd_pkt_t* pkt_dc = (subcmd_pkt_t*)pkt;
        pkt_type = pkt_dc->hdr.dc.pkt_type, pkt_subtype = pkt_dc->type;
        pkt_len = pkt_dc->hdr.dc.pkt_len;
        pkt_size = pkt_dc->size;
        break;
    }

    if (pkt_len != LE16(len) && len != 0) {
        goto err_all;
    }

    if (pkt_size != size && size != 0xFF) {
        goto err_all;
    }

    return true;

err_all:
    ERR_LOG("%s 发送损坏的数据指令 0x%04X 0x%02X!", get_player_describe(src), pkt_type, pkt_subtype);
    print_ascii_hex(errl, pkt, pkt_len);
    return false;
}

char* prepend_command_header(
    int version,
    bool encryption_enabled,
    uint16_t cmd,
    uint32_t flag,
    const char* data) {
    void* ret = NULL;
    size_t data_size = strlen(data);

    switch (version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX: {
        dc_pkt_hdr_t header = { 0 };
        if (encryption_enabled) {
            header.pkt_len = (sizeof(header) + data_size + 3) & ~3;
        }
        else {
            header.pkt_len = (uint16_t)(sizeof(header) + data_size);
        }
        header.pkt_type = (uint8_t)cmd;
        header.flags = flag;

        ret = malloc(header.pkt_len);
        memcpy((uint8_t*)ret, &header, sizeof(header));
        memcpy((uint8_t*)ret + sizeof(dc_pkt_hdr_t), data, data_size);
        break;
    }
    case CLIENT_VERSION_PC: {
        pc_pkt_hdr_t header = { 0 };
        if (encryption_enabled) {
            header.pkt_len = (sizeof(header) + data_size + 3) & ~3;
        }
        else {
            header.pkt_len = (uint16_t)(sizeof(header) + data_size);
        }
        header.pkt_type = (uint8_t)cmd;
        header.flags = flag;

        ret = malloc(header.pkt_len);
        memcpy((uint8_t*)ret, &header, sizeof(header));
        memcpy((uint8_t*)ret + sizeof(pc_pkt_hdr_t), data, data_size);
        break;
    }
    case CLIENT_VERSION_BB: {
        bb_pkt_hdr_t header = { 0 };
        if (encryption_enabled) {
            header.pkt_len = (sizeof(header) + data_size + 3) & ~3;
        }
        else {
            header.pkt_len = (uint16_t)(sizeof(header) + data_size);
        }
        header.pkt_type  = cmd;
        header.flags = flag;

        ret = malloc(header.pkt_len);
        memcpy((uint8_t*)ret, &header, sizeof(header));
        memcpy((uint8_t*)ret + sizeof(bb_pkt_hdr_t), data, data_size);
        break;
    }
    default:
        ERR_LOG("prepend_command_header 未解析该版本数据");
    }

    return ret;
}