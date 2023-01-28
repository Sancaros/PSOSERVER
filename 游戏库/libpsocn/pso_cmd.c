/*
    �λ�֮���й� ������ ָ�
    ��Ȩ (C) 2022 Sancaros

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

#include "pso_cmd.h"

char tmp_cmd_name[256] = { 0 };

const char* get_name(cmd_map_st* cur, uint16_t cmd) {

    /* ������Ʒ�� */
    while (cur->cmd != NO_SUCH_CMD) {
        if (cur->cmd == cmd) {
            sprintf_s(tmp_cmd_name, _countof(tmp_cmd_name), "%s - %s", cur->name, cur->cnname);

            return &tmp_cmd_name[0];
        }

        ++cur;
    }

    /* δ�ҵ���Ʒ����... */
    return "δ�ҵ����ָ������";
}

/* ͨ������ԱȻ�ȡ��Ʒ���� */
const char* c_cmd_name(uint16_t cmd, int32_t version) {

    cmd_map_st* cur = &c_cmd_names[0];

    (void)version;

    //printf("�ͻ��˰汾Ϊ %d .ָ��: 0x%02X \n", version, cmd);

    /* δ�ҵ���Ʒ����... */
    return get_name(cur, cmd);
}

/* ͨ������ԱȻ�ȡ��Ʒ���� */
const char* c_subcmd_name(uint16_t cmd, int32_t version) {

    cmd_map_st* cur = &c_subcmd_names[0];

    (void)version;

    //printf("�ͻ��˰汾Ϊ %d .ָ��: 0x%02X \n", version, cmd);

    return get_name(cur, cmd);
}

/* ͨ������ԱȻ�ȡ��Ʒ���� */
const char* s_cmd_name(uint16_t cmd, int32_t version) {

    cmd_map_st* cur = &s_cmd_names[0];

    (void)version;

    //printf("�ͻ��˰汾Ϊ %d .ָ��: 0x%02X \n", version, cmd);

    return get_name(cur, cmd);
}