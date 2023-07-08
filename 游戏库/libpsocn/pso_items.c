/*
    梦幻之星中国 舰船服务器 物品数据
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <f_logs.h>

#include "pso_items.h"
#include "pso_character.h"

const char* item_get_name_by_code(item_code_t code, int version) {
    item_map_t* cur = item_list;
    //TODO 未完成多语言物品名称

    (void)version;

    const char* unknow_name = "未识别物品名称";

    /* Take care of mags so that we'll match them properly... */
    if ((code & 0xFF) == 0x02) {
        code &= 0xFFFF;
    }

    /* Look through the list for the one we want */
    while (cur->code != Item_NoSuchItem) {
        if (cur->code == code) {
            return cur->name;
        }

        ++cur;
    }

    /* No item found... */
    return unknow_name;
}

const char* bbitem_get_name_by_code(bbitem_code_t code, int version) {
    bbitem_map_t* cur = bbitem_list_en;
    //TODO 未完成多语言物品名称
    (void)version;

    const char* unknow_name = "未识别物品名称";

    //TODO 未完成多语言物品名称
    int32_t languageCheck = 1;
    if (languageCheck) {
        cur = bbitem_list_cn;
    }

    /* Take care of mags so that we'll match them properly... */
    if ((code & 0xFF) == ITEM_TYPE_MAG) {
        code &= 0xFFFF;
    }

    /* Look through the list for the one we want */
    while (cur->code != Item_NoSuchItem) {
        if (cur->code == code) {
            return cur->name;
        }

        ++cur;
    }

    //ERR_LOG("物品ID不存在 %06X", cur->code);

    /* No item found... */
    return unknow_name;
}

/* 获取物品名称 */
const char* item_get_name(item_t* item, int version) {
    uint32_t code = item->data_b[0] | (item->data_b[1] << 8) |
        (item->data_b[2] << 16);

    /* 获取相关物品参数对比 */
    switch (item->data_b[0]) {
    case ITEM_TYPE_WEAPON:  /* 武器 */
        if (item->data_b[5]) {
            code = (item->data_b[5] << 8);
        }
        break;

    case ITEM_TYPE_GUARD:  /* 装甲 */
        if (item->data_b[1] != ITEM_SUBTYPE_UNIT && item->data_b[3]) {
            code = code | (item->data_b[3] << 16);
        }
        //printf("数据1: %02X 数据3: %02X\n", item->item_data1[1], item->item_data1[3]);
        break;

    case ITEM_TYPE_MAG:  /* 玛古 */
        if (item->data_b[1] == 0x00 && item->data_b[2] >= 0xC9) {
            code = 0x02 | (((item->data_b[2] - 0xC9) + 0x2C) << 8);
        }
        break;

    case ITEM_TYPE_TOOL: /* 物品 */
        if (code == 0x060D03 && item->data_b[3]) {
            code = 0x000E03 | ((item->data_b[3] - 1) << 16);
        }
        break;

    case ITEM_TYPE_MESETA: /* 美赛塔 */
        break;

    default:
        ERR_LOG("未找到物品类型");
        break;
    }

    if (version == 5)
        return bbitem_get_name_by_code((bbitem_code_t)code, version);
    else
        return item_get_name_by_code((item_code_t)code, version);
}

/* 打印物品数据 */
void print_item_data(item_t* item, int version) {
    ITEM_LOG("物品:(ID %d / %08X) %s",
        item->item_id, item->item_id, item_get_name(item, version));
    ITEM_LOG("数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
        item->data_b[0], item->data_b[1], item->data_b[2], item->data_b[3],
        item->data_b[4], item->data_b[5], item->data_b[6], item->data_b[7],
        item->data_b[8], item->data_b[9], item->data_b[10], item->data_b[11],
        item->data2_b[0], item->data2_b[1], item->data2_b[2], item->data2_b[3]);
}

/* 打印背包物品数据 */
void print_iitem_data(iitem_t* iitem, int item_index, int version) {
    ITEM_LOG("背包物品:(ID %d / %08X) %s",
        iitem->data.item_id, iitem->data.item_id, item_get_name(&iitem->data, version));
    ITEM_LOG(""
        "槽位 (%d) "
        "(%s) %04X "
        "鉴定 %d "
        "(%s) Flags %08X",
        item_index,
        ((iitem->present & LE32(0x0001)) ? "已占槽位" : "未占槽位"),
        iitem->present,
        iitem->tech,
        ((iitem->flags & LE32(0x00000008)) ? "已装备" : "未装备"),
        iitem->flags
    );
    ITEM_LOG("背包数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
        iitem->data.data_b[0], iitem->data.data_b[1], iitem->data.data_b[2], iitem->data.data_b[3],
        iitem->data.data_b[4], iitem->data.data_b[5], iitem->data.data_b[6], iitem->data.data_b[7],
        iitem->data.data_b[8], iitem->data.data_b[9], iitem->data.data_b[10], iitem->data.data_b[11],
        iitem->data.data2_b[0], iitem->data.data2_b[1], iitem->data.data2_b[2], iitem->data.data2_b[3]);
}

/* 打印银行物品数据 */
void print_bitem_data(bitem_t* bitem, int item_index, int version) {
    ITEM_LOG("银行物品:(ID %d / %08X) %s",
        bitem->data.item_id, bitem->data.item_id, item_get_name(&bitem->data, version));
    ITEM_LOG(""
        "槽位 (%d) "
        "(%s) %04X "
        "(%s) Flags %04X",
        item_index,
        ((bitem->amount & LE32(0x0001)) ? "堆叠" : "单独"),
        bitem->amount,
        ((bitem->show_flags & LE32(0x0001)) ? "显示" : "隐藏"),
        bitem->show_flags
    );
    ITEM_LOG("银行数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
        bitem->data.data_b[0], bitem->data.data_b[1], bitem->data.data_b[2], bitem->data.data_b[3],
        bitem->data.data_b[4], bitem->data.data_b[5], bitem->data.data_b[6], bitem->data.data_b[7],
        bitem->data.data_b[8], bitem->data.data_b[9], bitem->data.data_b[10], bitem->data.data_b[11],
        bitem->data.data2_b[0], bitem->data.data2_b[1], bitem->data.data2_b[2], bitem->data.data2_b[3]);
}

void print_biitem_data(void* data, int item_index, int version, int inv, int err) {
    char* inv_text = ((inv == 1) ? "背包" : "银行");
    char* err_text = ((err == 1) ? "错误" : "玩家");

    if (data) {
        if (inv) {
            iitem_t* iitem = (iitem_t*)data;

            ITEM_LOG("%s X %s物品:(ID %d / %08X) %s", err_text, inv_text,
                iitem->data.item_id, iitem->data.item_id, item_get_name(&iitem->data, version));

            ITEM_LOG(""
                "槽位 (%d) "
                "(%s) %04X "
                "鉴定 %d "
                "(%s) Flags %08X",
                item_index,
                ((iitem->present == 0x0001) ? "已占槽位" : "未占槽位"),
                iitem->present,
                iitem->tech,
                ((iitem->flags & LE32(0x00000008)) ? "已装备" : "未装备"),
                iitem->flags
            );

            ITEM_LOG("%s数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
                inv_text,
                iitem->data.data_b[0], iitem->data.data_b[1], iitem->data.data_b[2], iitem->data.data_b[3],
                iitem->data.data_b[4], iitem->data.data_b[5], iitem->data.data_b[6], iitem->data.data_b[7],
                iitem->data.data_b[8], iitem->data.data_b[9], iitem->data.data_b[10], iitem->data.data_b[11],
                iitem->data.data2_b[0], iitem->data.data2_b[1], iitem->data.data2_b[2], iitem->data.data2_b[3]);
        }
        else {
            bitem_t* bitem = (bitem_t*)data;

            ITEM_LOG("%s X %s物品:(ID %d / %08X) %s", err_text, inv_text,
                bitem->data.item_id, bitem->data.item_id, item_get_name(&bitem->data, version));

            ITEM_LOG(""
                "槽位 (%d) "
                "(%s) %04X "
                "(%s) Flags %04X",
                item_index,
                ((bitem->amount == 0x0001) ? "堆叠" : "单独"),
                bitem->amount,
                ((bitem->show_flags == 0x0001) ? "显示" : "隐藏"),
                bitem->show_flags
            );

            ITEM_LOG("%s数据: %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X, %02X%02X%02X%02X",
                inv_text,
                bitem->data.data_b[0], bitem->data.data_b[1], bitem->data.data_b[2], bitem->data.data_b[3],
                bitem->data.data_b[4], bitem->data.data_b[5], bitem->data.data_b[6], bitem->data.data_b[7],
                bitem->data.data_b[8], bitem->data.data_b[9], bitem->data.data_b[10], bitem->data.data_b[11],
                bitem->data.data2_b[0], bitem->data.data2_b[1], bitem->data.data2_b[2], bitem->data.data2_b[3]);
        }
    }
}
