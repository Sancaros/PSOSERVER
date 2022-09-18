
#include "psoconfig.h"

/* 通过代码对比获取物品名称 */
const char* c_cmd_name(uint16_t cmd, int32_t version) {

    c_cmd_map_val_tbl cmd_code = (c_cmd_map_val_tbl)cmd;

    c_cmd_map_st* cur = &c_cmd_names[0];

    (void)version;

    /* 检索物品表 */
    while (cur->cmd != NoSuchcCmd) {
        if (cur->cmd == cmd_code) {
            return cur->name;
        }

        ++cur;
    }

    /* 未找到物品数据... */
    return "未找到相关指令名称";
}

/* 通过代码对比获取物品名称 */
const char* s_cmd_name(uint16_t cmd, int32_t version) {

    s_cmd_map_val_tbl cmd_code = (s_cmd_map_val_tbl)cmd;

    s_cmd_map_st* cur = &s_cmd_names[0];

    (void)version;

    /* 检索物品表 */
    while (cur->cmd != NoSuchsCmd) {
        if (cur->cmd == cmd_code) {
            return cur->name;
        }

        ++cur;
    }

    /* 未找到物品数据... */
    return "未找到相关指令名称";
}