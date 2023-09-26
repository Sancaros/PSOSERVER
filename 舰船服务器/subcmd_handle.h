/*
    �λ�֮���й� ���������� 60ָ���
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

#ifndef SUBCMD_HANDLE_H
#define SUBCMD_HANDLE_H

// ���庯��ָ�������
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

// ʹ�ú���ָ��ֱ�ӵ�����Ӧ�Ĵ�����
subcmd_handle_t subcmd_get_handler(int cmd_type, int subcmd_type, int version);

/* �������Ƿ�����Ч����Ϸ������ */
bool in_game(ship_client_t* src);

/* �������Ƿ�����Ч�Ĵ����� */
bool in_lobby(ship_client_t* src);

/* ������ݰ��Ƿ���Ч */
bool check_pkt_size(ship_client_t* src, void* pkt, uint16_t len, uint8_t size);

/* ����newserv��汾���ݰ����麯�� */
char* prepend_command_header(int version, bool encryption_enabled, uint16_t cmd, uint32_t flag, const char* data);

#endif /* !SUBCMD_HANDLE_H */