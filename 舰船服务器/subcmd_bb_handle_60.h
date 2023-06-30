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

#ifndef SUBCMD_BB_HANDLE_60_H
#define SUBCMD_BB_HANDLE_60_H

// 定义函数指针的类型
typedef int (*subcmd60_bb_handle_t)(ship_client_t*, void*);

extern subcmd60_bb_handle_t subcmd60_bb_handle[0x100];

#endif /* !SUBCMD_BB_HANDLE_60_H */