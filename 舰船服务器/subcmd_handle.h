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

#ifndef SUBCMD_HANDLE_H
#define SUBCMD_HANDLE_H

// 定义函数指针的类型
typedef int (*subcmd_handle_t)(ship_client_t* src, ship_client_t* dst, void* pkt);

typedef struct subcmd_handle_func {
    int subcmd_type;
    subcmd_handle_t dc;
    subcmd_handle_t gc;
    subcmd_handle_t ep3;
    subcmd_handle_t xb;
    subcmd_handle_t pc;
    subcmd_handle_t bb;
} subcmd_handle_func_t;

// 使用函数指针直接调用相应的处理函数
subcmd_handle_t subcmd_get_handler(int cmd_type, int subcmd_type, int version);

/* 检测玩家是否在有效的游戏房间中 */
bool in_game(ship_client_t* src);

/* 检测玩家是否在有效的大厅中 */
bool in_lobby(ship_client_t* src);

/* 检测数据包是否有效 */
bool check_pkt_size(ship_client_t* src, void* pkt, uint16_t len, uint8_t size);

/* 来自newserv多版本数据包重组函数 */
char* prepend_command_header(int version, bool encryption_enabled, uint16_t cmd, uint32_t flag, const char* data);

#endif /* !SUBCMD_HANDLE_H */