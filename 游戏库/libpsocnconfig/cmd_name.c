
#include "psoconfig.h"

/* ͨ������ԱȻ�ȡ��Ʒ���� */
const char* c_cmd_name(uint16_t cmd, int32_t version) {

    c_cmd_map_val_tbl cmd_code = (c_cmd_map_val_tbl)cmd;

    c_cmd_map_st* cur = &c_cmd_names[0];

    (void)version;

    /* ������Ʒ�� */
    while (cur->cmd != NoSuchcCmd) {
        if (cur->cmd == cmd_code) {
            return cur->name;
        }

        ++cur;
    }

    /* δ�ҵ���Ʒ����... */
    return "δ�ҵ����ָ������";
}

/* ͨ������ԱȻ�ȡ��Ʒ���� */
const char* s_cmd_name(uint16_t cmd, int32_t version) {

    s_cmd_map_val_tbl cmd_code = (s_cmd_map_val_tbl)cmd;

    s_cmd_map_st* cur = &s_cmd_names[0];

    (void)version;

    /* ������Ʒ�� */
    while (cur->cmd != NoSuchsCmd) {
        if (cur->cmd == cmd_code) {
            return cur->name;
        }

        ++cur;
    }

    /* δ�ҵ���Ʒ����... */
    return "δ�ҵ����ָ������";
}