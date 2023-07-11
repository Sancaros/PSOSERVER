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
subcmd_handle_t subcmd_get_handler_mode(int cmd_type, int subcmd_type, int version);

#endif /* !SUBCMD_HANDLE_H */