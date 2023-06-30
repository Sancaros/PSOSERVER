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

#ifndef SUBCMD_BB_HANDLE_62_H
#define SUBCMD_BB_HANDLE_62_H

// 定义函数指针的类型
typedef int (*subcmd62_handle_t)(ship_client_t* src, ship_client_t* dst, void* pkt);

typedef struct subcmd62_handle_func {
    int cmd_type;
    subcmd62_handle_t dc;
    subcmd62_handle_t gc;
    subcmd62_handle_t ep3;
    subcmd62_handle_t xb;
    subcmd62_handle_t pc;
    subcmd62_handle_t bb;
} subcmd62_handle_func_t[];

// 使用函数指针直接调用相应的处理函数
subcmd62_handle_t subcmd62_get_handler(int cmd_type, int version);

#endif /* !SUBCMD_BB_HANDLE_62_H */