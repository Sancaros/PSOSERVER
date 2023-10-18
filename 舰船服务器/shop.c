/*
    梦幻之星中国 舰船服务器
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
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <f_logs.h>
#include <f_checksum.h>
#include <pso_opcodes_block.h>
#include <SFMT.h>

#include <pso_character.h>

#include "shop.h"
#include "pmtdata.h"
#include "ptdata.h"

static const uint32_t stable_shop_common_tool_item_subtype[4][10] = {
    {
    BBItem_Monomate,
    BBItem_Monofluid,
    BBItem_Dimate,
    BBItem_Difluid,
    BBItem_Trimate,
    BBItem_Trifluid,
    BBItem_Sol_Atomizer,
    BBItem_Moon_Atomizer,
    BBItem_Star_Atomizer,
    BBItem_Scape_Doll
    },
    {
    BBItem_Monomate,
    BBItem_Monofluid,
    BBItem_Dimate,
    BBItem_Difluid,
    BBItem_Trimate,
    BBItem_Trifluid,
    BBItem_Sol_Atomizer,
    BBItem_Moon_Atomizer,
    BBItem_Star_Atomizer,
    BBItem_Scape_Doll
    },
    {
    BBItem_Monomate,
    BBItem_Monofluid,
    BBItem_Dimate,
    BBItem_Difluid,
    BBItem_Trimate,
    BBItem_Trifluid,
    BBItem_Sol_Atomizer,
    BBItem_Moon_Atomizer,
    BBItem_Star_Atomizer,
    BBItem_Scape_Doll
    },
    {
    BBItem_Monomate,
    BBItem_Monofluid,
    BBItem_Dimate,
    BBItem_Difluid,
    BBItem_Trimate,
    BBItem_Trifluid,
    BBItem_Sol_Atomizer,
    BBItem_Moon_Atomizer,
    BBItem_Star_Atomizer,
    BBItem_Scape_Doll
    }
};

item_t create_common_bb_shop_tool_item(uint8_t 难度, uint8_t index) {
    item_t item = { 0 };
    item.datal[0] = stable_shop_common_tool_item_subtype[难度][index];
    return item;
}

item_t create_common_bb_shop_item(uint8_t 难度, uint8_t 物品类型, sfmt_t* 随机因子) {
    static const uint8_t max_quantity[4] = { 1,  1,  1,  1 };
    static const uint8_t max_tech_lvl[4] = { 4,  7, 10, 15 };
    static const uint8_t max_anti_lvl[4] = { 2,  4,  6,  7 };
    item_t item = { 0 };
    uint8_t tmp_value = 0;
    item.datab[0] = 物品类型;
    errno_t err = 0;

    while (item.datab[0] == ITEM_TYPE_MAG) {
        item.datab[0] = sfmt_genrand_uint32(随机因子) % 3;
    }

    /* 检索物品类型 */
    switch (item.datab[0]) {
    case ITEM_TYPE_WEAPON: // 武器
        item.datab[1] = rand_int(随机因子, 12) + 1; /* 01 - 0C 普通物品*/

        /* 9 以下都是 0/1 + 难度 9以上则 0-3（难度）类型ID*/
        if (item.datab[1] > 9) {
            item.datab[2] = 难度;
        }
        else
            item.datab[2] = rand_int(随机因子, 2) + 难度;

        /* 打磨值 0 - 10*/
        if (sfmt_genrand_uint32(随机因子) % 2)
            item.datab[3] = sfmt_genrand_uint32(随机因子) % 8;

        /* 特殊攻击 0 - 10 配合难度 0 - 3*/
        if (sfmt_genrand_uint32(随机因子) % 2)
            item.datab[4] = sfmt_genrand_uint32(随机因子) % 11 + 难度;
        /* datab[5] 在这里不涉及 礼物 未鉴定*/

        /* 生成属性*/
        size_t num_percentages = 0;
        while (num_percentages < 3) {
            /*0-5 涵盖所有属性*/
            for (size_t x = 0; x < 6; x++) {
                if ((sfmt_genrand_uint32(随机因子) % 4) == 1) {
                    /*+6 对应属性槽（结果分别为 6 8 10） +7对应数值（结果分别为 随机数1-20 1-35 1-45 1-50）*/
                    item.datab[(num_percentages * 2) + 6] = (uint8_t)x;
                    tmp_value = /*sfmt_genrand_uint32(随机因子) % 6 +*/ weapon_bonus_values[sfmt_genrand_uint32(随机因子) % 21];/* 0 - 5 % 0 - 19*/

                    //if (tmp_value > 50)
                    //    tmp_value = 50;

                    //if (tmp_value < -50)
                    //    tmp_value = -50;

                    item.datab[(num_percentages * 2) + 7] = tmp_value;
                    num_percentages++;
                }
            }
        }

        break;

    case ITEM_TYPE_GUARD: // 装甲
        pmt_guard_bb_t pmt_guard = { 0 };
        pmt_unit_bb_t pmt_unit = { 0 };
        item.datab[1] = 0;

        /* 必须是1或2 对应护甲或者护盾*/
        while (item.datab[1] == 0)
            item.datab[1] = sfmt_genrand_uint32(随机因子) & 3;

        switch (item.datab[1]) {
        case ITEM_SUBTYPE_FRAME://护甲
            /*护甲物品子类型*/
            item.datab[2] = get_common_frame_subtype_range_for_difficult(难度, 0x06, 随机因子);
            if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
                ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                break;
            }

            /*随机槽位 0 - 4 */
            item.datab[5] = sfmt_genrand_uint32(随机因子) % 5;

            /* DFP值 */
            if (pmt_guard.dfp_range) {
                tmp_value = sfmt_genrand_uint32(随机因子) % (pmt_guard.dfp_range + 1);
                if (tmp_value < 0)
                    tmp_value = 0;
                item.datab[6] = tmp_value;
            }

            /* EVP值 */
            if (pmt_guard.evp_range) {
                tmp_value = sfmt_genrand_uint32(随机因子) % (pmt_guard.evp_range + 1);
                if (tmp_value < 0)
                    tmp_value = 0;
                item.datab[8] = tmp_value;
            }
            break;

        case ITEM_SUBTYPE_BARRIER://护盾 0 - 20 

            /*护盾物品子类型*/
            item.datab[2] = get_common_barrier_subtype_range_for_difficult(难度, 0x06, 随机因子);
            if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
                ERR_LOG("pmt_lookup_guard_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                break;
            }

            /* DFP值 */
            if (pmt_guard.dfp_range) {
                tmp_value = sfmt_genrand_uint32(随机因子) % (pmt_guard.dfp_range + 1);
                if (tmp_value < 0)
                    tmp_value = 0;
                item.datab[6] = tmp_value;
            }

            /* EVP值 */
            if (pmt_guard.evp_range) {
                tmp_value = sfmt_genrand_uint32(随机因子) % (pmt_guard.evp_range + 1);
                if (tmp_value < 0)
                    tmp_value = 0;
                item.datab[8] = tmp_value;
            }
            break;

        case ITEM_SUBTYPE_UNIT://插件
            item.datab[2] = common_unit_subtypes[(sfmt_genrand_uint32(随机因子) % 9)][(sfmt_genrand_uint32(随机因子) % 2)];
            if (err = pmt_lookup_unit_bb(item.datal[0], &pmt_unit)) {
                ERR_LOG("pmt_lookup_unit_bb 不存在数据! 错误码 %d 0x%08X", err, item.datal[0]);
                break;
            }
            tmp_value = sfmt_genrand_uint32(随机因子) % 5;
            item.datab[6] = unit_bonus_values[tmp_value][0];
            item.datab[7] = unit_bonus_values[tmp_value][1];
            break;
        }
        break;

    case ITEM_TYPE_TOOL: // 药品工具
        item.datab[1] = sfmt_genrand_uint32(随机因子) % 9 + 2;
        switch (item.datab[1]) {
        case ITEM_SUBTYPE_MATE:
        case ITEM_SUBTYPE_FLUID:
            switch (难度) {
            case GAME_TYPE_DIFFICULTY_NORMAL:
                item.datab[2] = 0;
                break;

            case GAME_TYPE_DIFFICULTY_HARD:
                item.datab[2] = sfmt_genrand_uint32(随机因子) % 2;
                break;

            case GAME_TYPE_DIFFICULTY_VERY_HARD:
                item.datab[2] = (sfmt_genrand_uint32(随机因子) % 2) + 1;
                break;

            case GAME_TYPE_DIFFICULTY_ULTIMATE:
                item.datab[2] = 2;
                break;
            }
            break;

        case ITEM_SUBTYPE_ANTI_TOOL:
            item.datab[2] = sfmt_genrand_uint32(随机因子) % 2;
            break;

        case ITEM_SUBTYPE_GRINDER:
            item.datab[2] = sfmt_genrand_uint32(随机因子) % 3;
            break;

            //case ITEM_SUBTYPE_MATERIAL:
            //    item.datab[2] = sfmt_genrand_uint32(随机因子) % 7;
            //    break;

        case ITEM_SUBTYPE_DISK:
            item.datab[4] = sfmt_genrand_uint32(随机因子) % 19;
            switch (item.datab[4]) {
            case TECHNIQUE_RYUKER:
            case TECHNIQUE_REVERSER:
                item.datab[2] = 0; // reverser & ryuker always level 1 这两个法术永远是1级
                break;
            case TECHNIQUE_ANTI:
                item.datab[2] = sfmt_genrand_uint32(随机因子) % max_anti_lvl[难度];
                break;
            case TECHNIQUE_FOIE:
            case TECHNIQUE_GIFOIE:
            case TECHNIQUE_RAFOIE:
            case TECHNIQUE_BARTA:
            case TECHNIQUE_GIBARTA:
            case TECHNIQUE_RABARTA:
            case TECHNIQUE_ZONDE:
            case TECHNIQUE_GIZONDE:
            case TECHNIQUE_RAZONDE:
            case TECHNIQUE_GRANTS:
            case TECHNIQUE_DEBAND:
            case TECHNIQUE_JELLEN:
            case TECHNIQUE_ZALURE:
            case TECHNIQUE_SHIFTA:
            case TECHNIQUE_RESTA:
            case TECHNIQUE_MEGID:
                item.datab[2] = sfmt_genrand_uint32(随机因子) % max_tech_lvl[难度];
                break;
            }
            break;
            //case ITEM_SUBTYPE_MATE:
            //case ITEM_SUBTYPE_FLUID:
        case ITEM_SUBTYPE_SOL_ATOMIZER:
        case ITEM_SUBTYPE_MOON_ATOMIZER:
        case ITEM_SUBTYPE_STAR_ATOMIZER:
        //case ITEM_SUBTYPE_ANTI_TOOL:
        case ITEM_SUBTYPE_TELEPIPE:
        case ITEM_SUBTYPE_TRAP_VISION:
        case ITEM_SUBTYPE_PHOTON:
            item.datab[5] = 1;
            break;
        }
    }

    return item;
}

item_t create_bb_shop_items(uint32_t player_level, 
    uint8_t random_shop, uint32_t 商店类型,
    uint8_t 难度, uint8_t 物品索引, 
    sfmt_t* 随机因子) {
    item_t item = { 0 };

    size_t table_index;
    if (难度 == GAME_TYPE_DIFFICULTY_ULTIMATE) {
        if (player_level < 11) {
            table_index = 2;
        }
        else if (player_level < 26) {
            table_index = 4;
        }
        else if (player_level < 43) {
            table_index = 4;
        }
        else if (player_level < 61) {
            table_index = 6;
        }
        else if (player_level < 100) {
            table_index = 6;
        }
        else if (player_level < 151) {
            table_index = 8;
        }
        else {
            table_index = 10;
        }
    }
    else {
        if (player_level < 11) {
            table_index = 2;
        }
        else if (player_level < 26) {
            table_index = 4;
        }
        else if (player_level < 43) {
            table_index = 4;
        }
        else if (player_level < 61) {
            table_index = 6;
        }
        else {
            table_index = 8;
        }
    }

    switch (商店类型) {
    case BB_SHOPTYPE_TOOL:// 工具商店
        if (物品索引 < table_index && !random_shop)
            item = create_common_bb_shop_tool_item(难度, 物品索引);
        else
            item = create_common_bb_shop_item(难度, ITEM_TYPE_TOOL, 随机因子);
        break;

    case BB_SHOPTYPE_WEAPON:// 武器商店
        item = create_common_bb_shop_item(难度, ITEM_TYPE_WEAPON, 随机因子);
        break;

    case BB_SHOPTYPE_ARMOR:// 装甲商店
        item = create_common_bb_shop_item(难度, ITEM_TYPE_GUARD, 随机因子);
        break;
    }

    return item;
}

size_t price_for_item(const item_t* item) {
    errno_t err = 0;
    size_t price = 0;

    switch (item->datab[0]) {
    case ITEM_TYPE_WEAPON: {
        pmt_weapon_bb_t pmt_weapon = { 0 };
        if (item->datab[4] & 0x80) {
            return 8;
        }
        if (is_item_rare(item)) {
#ifdef DEBUG
            DBG_LOG("物品 0x%08X 是稀有物品", item->datal[0]);
#endif // DEBUG
            return 80;
        }

        float sale_divisor = pmt_lookup_sale_divisor_bb(item->datab[0], item->datab[1]);
        if (sale_divisor == 0.0) {
            ERR_LOG("物品 sale divisor 为0 0x%08X", item->datal[0]);
            return 0;
        }

        if (err = pmt_lookup_weapon_bb(item->datal[0], &pmt_weapon)) {
            ERR_LOG("从PMT未获取到准确的数据! 0x%08X 错误码 %d", item->datal[0], err);
            return err;
        }

        //const auto& def = this->get_weapon(item->datab[1], item->datab[2]);
        double atp_max = pmt_weapon.atp_max + item->datab[3];
        double atp_factor = ((atp_max * atp_max) / sale_divisor);

        double bonus_factor = 0.0;
        for (size_t bonus_index = 0; bonus_index < 3; bonus_index++) {
            uint8_t bonus_type = item->datab[(2 * bonus_index) + 6];
            if ((bonus_type > 0) && (bonus_type < 6)) {
                bonus_factor += item->datab[(2 * bonus_index) + 7];
            }
            bonus_factor += 100.0;
        }

        size_t special_stars = get_special_stars(item->datab[4]);
        double special_stars_factor = 1000.0 * special_stars * special_stars;
        price = (size_t)(special_stars_factor + ((atp_factor * bonus_factor) / 100.0));
        return price;
    }

    case ITEM_TYPE_GUARD: {
        pmt_guard_bb_t pmt_guard = { 0 };
        if (is_item_rare(item)) {
#ifdef DEBUG
            DBG_LOG("物品 0x%08X 是稀有物品", item->datal[0]);
#endif // DEBUG
            return 80;
        }

        if (item->datab[1] == ITEM_SUBTYPE_UNIT) { // Unit
            price = (size_t)(get_item_adjusted_stars(item) * pmt_lookup_sale_divisor_bb(item->datab[0], 3));
            return price;
        }

        double sale_divisor = (double)pmt_lookup_sale_divisor_bb(item->datab[0], item->datab[1]);
        if (sale_divisor == 0.0) {
            ERR_LOG("物品 sale divisor 为0 0x%08X", item->datal[0]);
            return 0;
        }

        int16_t def_bonus = get_armor_or_shield_defense_bonus(item);
        int16_t evp_bonus = get_common_armor_evasion_bonus(item);

        if (err = pmt_lookup_guard_bb(item->datal[0], &pmt_guard)) {
            ERR_LOG("从PMT未获取到准确的数据! 0x%08X 错误码 %d", item->datal[0], err);
            return err;
        }

        double power_factor = pmt_guard.base_dfp + pmt_guard.base_evp + def_bonus + evp_bonus;
        double power_factor_floor = (int32_t)((power_factor * power_factor) / sale_divisor);
        price = (size_t)(power_factor_floor + (70.0 * (double)(item->datab[5] + 1) * (double)(pmt_guard.level_req + 1)));
        return price;
    }

    case ITEM_TYPE_MAG:
        price = (size_t)((item->datab[2] + 1) * pmt_lookup_sale_divisor_bb(item->datab[0], item->datab[1]));
        return price;

    case ITEM_TYPE_TOOL: {
        pmt_tool_bb_t pmt_tool = { 0 };
        if (err = pmt_lookup_tools_bb(item->datal[0], item->datal[1], &pmt_tool)) {
            ERR_LOG("从PMT未获取到准确的数据! 0x%08X 错误码 %d", item->datal[0], err);
            return err;
        }

        size_t price_num = 1;

        if (item->datab[1] == ITEM_SUBTYPE_DISK)
            if (item->datab[4] < 0x13)
                price_num = item->datab[2] + 1;

        price = pmt_tool.cost * price_num;
        if (price < 0) {
            ERR_LOG("pmt_lookup_tools_bb 0x%08X 的价格 %d 为负数", item->datal[0], pmt_tool.cost);
        }
        return price;
    }

    case ITEM_TYPE_MESETA:
        price = item->data2l;
        return price;

    default:
        ERR_LOG("无效物品, 价格为0 0x%08X", item->datal[0]);
        return err;
    }
    ERR_LOG("不会吧 还能跑到这里吗？ 0x%08X", item->datal[0]);
}

size_t get_shp_size(item_t* shop) {
    size_t sz = 0;
    while (item_not_identification_bb(shop[sz].datab[0], shop[sz].datab[1]) == 0) {
        sz++;
    }

    return sz;
}

void emplace_back_shop(item_t* shop, item_t newItem) {
    size_t sz = get_shp_size(shop);
    if (sz < BB_SHOP_SIZE) {
        shop[sz] = newItem;
    }
}

typedef struct random_table_spec {
    uint32_t offset;
    uint8_t entries_per_table;
    uint32_t datal;
} random_table_spec_t;

static random_table_spec_t random_common_recovery_table[6][10] = {
    {
    {0x00000000, 0, 0x00000003},
    },{
    {0x00000000, 0, 0x00000003},
    },{
    {0x00000000, 0, 0x00000003},
    },{
    {0x00000000, 0, 0x00000003},
    },{
    {0x00000000, 0, 0x00000003},
    },{
    {0x00000000, 0, 0x00000003},
    },
};

static const uint8_t tool_item_defs[][2] = {
    {0x00, 0x00},
    {0x00, 0x01},
    {0x00, 0x02},
    {0x01, 0x00},
    {0x01, 0x01},
    {0x01, 0x02},
    {0x06, 0x00},
    {0x06, 0x01},
    {0x03, 0x00},
    {0x04, 0x00},
    {0x05, 0x00},
    {0x07, 0x00},
    {0x08, 0x00},
    {0x09, 0x00},
    {0x0A, 0x00},
    {0xFF, 0xFF},
 };

void generate_common_tool_shop_recovery_items(
    item_t* shop, size_t player_level) {
    item_t item = { 0 };
    size_t table_index;
    if (player_level < 11) {
        table_index = 0;
    }
    else if (player_level < 26) {
        table_index = 1;
    }
    else if (player_level < 45) {
        table_index = 2;
    }
    else if (player_level < 61) {
        table_index = 3;
    }
    else if (player_level < 100) {
        table_index = 4;
    }
    else {
        table_index = 5;
    }

    random_table_spec_t* table = random_common_recovery_table[table_index];
    for (size_t z = 0; z < 10; z++) {
        //uint8_t type = table[z].unused[1];
        //if (type == 0x0F) {
        //    continue;
        //}
        item.datal[0] = table[z].datal;
        emplace_back_shop(shop, item);
    }
}

//item_t* generate_tool_shop_contents(size_t player_level, size_t nums) {
//    item_t* shop = (item_t*)malloc(sizeof(item_t) * nums); // 假设商店最多有 nums 个物品
//    if (!shop) {
//        ERR_LOG("生成物品失败 内存分配失败");
//        return NULL;
//    }
//
//    generate_common_tool_shop_recovery_items(shop, player_level);
//    generate_rare_tool_shop_recovery_items(shop, player_level);
//    generate_tool_shop_tech_disks(shop, player_level);
//
//    // 确定商店数组的大小
//    // 假设每个生成函数都会在 shop 数组中添加相应数量的物品，并返回物品数量
//    // 你可以根据实际情况修改这里的实现
//
//    qsort(shop, nums, sizeof(item_t), compare_for_sort);
//
//    return shop;
//}