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

extern subcmd_handle_func_t subcmd60_handler[0x100];
extern subcmd_handle_func_t subcmd62_handler[0x100];
extern subcmd_handle_func_t subcmd6D_handler[0x100];

subcmd_handle_t subcmd_search_handler(
    subcmd_handle_func_t* handler,
    size_t max_src,
    int subcmd_type,
    int version) {

    for (size_t i = 0; i < max_src; i++) {
        if (handler[i].subcmd_type == subcmd_type) {

            switch (version)
            {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
                return handler[i].dc;

            case CLIENT_VERSION_PC:
                return handler[i].pc;

            case CLIENT_VERSION_GC:
                return handler[i].gc;

            case CLIENT_VERSION_EP3:
                return handler[i].ep3;

            case CLIENT_VERSION_XBOX:
                return handler[i].xb;

            case CLIENT_VERSION_BB:
                return handler[i].bb;
            }
        }
    }

    return NULL;
}

// 使用函数指针直接调用相应的处理函数
subcmd_handle_t subcmd_get_handler(int cmd_type, int subcmd_type, int version) {
    subcmd_handle_t func = { 0 };
    size_t count = 0;

    switch (cmd_type)
    {
    case GAME_COMMAND0_TYPE:
        count = _countof(subcmd60_handler);
        func = subcmd_search_handler(subcmd60_handler, count, subcmd_type, version);
        break;

    case GAME_COMMAND2_TYPE:
        count = _countof(subcmd62_handler);
        func = subcmd_search_handler(subcmd62_handler, count, subcmd_type, version);
        break;

    case GAME_COMMANDD_TYPE:
        count = _countof(subcmd6D_handler);
        func = subcmd_search_handler(subcmd6D_handler, count, subcmd_type, version);
        break;
    }

    if(!func)
        ERR_LOG("subcmd_get_handler 未完成对 "
            "0x%02X 0x%02X 版本 %d 的处理", cmd_type, subcmd_type, version);

    return func;
}
