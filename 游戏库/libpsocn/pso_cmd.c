/*
    梦幻之星中国 服务器 指令集
    版权 (C) 2022 Sancaros

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

    /* 检索物品表 */
    while (cur->cmd != NO_SUCH_CMD) {
        if (cur->cmd == cmd) {
            sprintf_s(tmp_cmd_name, _countof(tmp_cmd_name), "%s - %s", cur->name, cur->cnname);

            return &tmp_cmd_name[0];
        }

        ++cur;
    }

    /* 未找到物品数据... */
    return "未找到相关指令名称";
}

/* 通过代码对比获取物品名称 */
const char* c_cmd_name(uint16_t cmd, int32_t version) {

    cmd_map_st* cur = &c_cmd_names[0];

    (void)version;

    //printf("客户端版本为 %d .指令: 0x%02X \n", version, cmd);

    /* 未找到物品数据... */
    return get_name(cur, cmd);
}

/* 通过代码对比获取物品名称 */
const char* c_subcmd_name(uint16_t cmd, int32_t version) {

    cmd_map_st* cur = &c_subcmd_names[0];

    (void)version;

    //printf("客户端版本为 %d .指令: 0x%02X \n", version, cmd);

    return get_name(cur, cmd);
}

/* 通过代码对比获取物品名称 */
const char* s_cmd_name(uint16_t cmd, int32_t version) {

    cmd_map_st* cur = &s_cmd_names[0];

    (void)version;

    //printf("客户端版本为 %d .指令: 0x%02X \n", version, cmd);

    return get_name(cur, cmd);
}