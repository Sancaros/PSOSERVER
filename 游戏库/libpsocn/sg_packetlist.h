/*
    梦幻之星中国 船闸服务器 数据包
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

#ifndef SHIPGATE_PACKETS_LIST
#define SHIPGATE_PACKETS_LIST

/* Size of the shipgate login packet. */
#define SHIPGATE_LOGINV0_SIZE       64

/* The requisite message for the msg field of the shipgate_login_pkt. */
static const char shipgate_login_msg[] =
"梦幻之星中国 船闸服务器 Copyright Sancaros";

#include <pso_opcodes_shipgate.h>

#endif /* !SHIPGATE_PACKETS_LIST */