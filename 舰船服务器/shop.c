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

#include <pso_character.h>

#include "shop.h"
#include "pmtdata.h"

//获取商店价格
uint32_t get_bb_shop_price(iitem_t* ci) {
    pmt_weapon_bb_t pmt_weapon = { 0 };
    pmt_guard_bb_t pmt_guard = { 0 };
    pmt_tool_bb_t pmt_tool = { 0 };
    uint32_t compare_item, ch;
    int32_t percent_add;
    uint32_t price = 10;
    uint8_t variation;
    float percent_calc;
    float price_calc;

    switch (ci->data.datab[0]) {
    case ITEM_TYPE_WEAPON: // Weapons 武器
        if (ci->data.datab[4] & 0x80)
            price = 1; // Untekked = 1 meseta 取消选中 = 1美赛塔
        else {
            if ((ci->data.datab[1] < 0x0D) && (ci->data.datab[2] < 0x05)) {
                if ((ci->data.datab[1] > 0x09) && (ci->data.datab[2] > 0x03)) // Canes, Rods, Wands become rare faster  拐杖、棍棒、魔杖越来越稀少 
                    break;

                if (pmt_lookup_weapon_bb(ci->data.datal[0], &pmt_weapon)) {
                    ERR_LOG("从PMT未获取到准确的数据!");
                    return -1;
                }

                price = pmt_weapon.atp_max + ci->data.datab[3];
                price *= price;
                price_calc = (float)price;
                switch (ci->data.datab[1]) {
                case 0x01:
                    price_calc /= 5.0f;
                    break;

                case 0x02:
                    price_calc /= 4.0f;
                    break;

                case 0x03:
                case 0x04:
                    price_calc *= 2.0f;
                    price_calc /= 3.0f;
                    break;

                case 0x05:
                    price_calc *= 4.0f;
                    price_calc /= 5.0f;
                    break;

                case 0x06:
                    price_calc *= 10.0f;
                    price_calc /= 21.0f;
                    break;

                case 0x07:
                    price_calc /= 3.0f;
                    break;

                case 0x08:
                    price_calc *= 25.0f;
                    break;

                case 0x09:
                    price_calc *= 10.0f;
                    price_calc /= 9.0f;
                    break;

                case 0x0A:
                    price_calc /= 2.0f;
                    break;

                case 0x0B:
                    price_calc *= 2.0f;
                    price_calc /= 5.0f;
                    break;

                case 0x0C:
                    price_calc *= 4.0f;
                    price_calc /= 3.0f;
                    break;
                }

                percent_add = 0;

                if (ci->data.datab[6])
                    percent_add += LE32(ci->data.datab[7]);

                if (ci->data.datab[8])
                    percent_add += LE32(ci->data.datab[9]);

                if (ci->data.datab[10])
                    percent_add += LE32(ci->data.datab[11]);

                if (percent_add != 0) {
                    percent_calc = price_calc;
                    percent_calc /= 300.0f;
                    percent_calc *= percent_add;
                    price_calc += percent_calc;
                }

                price_calc /= 8.0f;
                price = (int32_t)(price_calc);
                price += attrib[ci->data.datab[4]];
            }
        }
        break;

    case ITEM_TYPE_GUARD:
        switch (ci->data.datab[1]) {
        case ITEM_SUBTYPE_FRAME: // Armor 装备

            if (pmt_lookup_guard_bb(ci->data.datal[0], &pmt_guard)) {
                ERR_LOG("未从PMT获取到 0x%04X 的数据!", ci->data.datal[0]);
                return -3;
            }

            if (ci->data.datab[2] < 0x18) {
                // Calculate the amount to boost because of slots...
                if (ci->data.datab[5] > 4)
                    price = armor_prices[(ci->data.datab[2] * 5) + 4];
                else
                    price = armor_prices[(ci->data.datab[2] * 5) + ci->data.datab[5]];

                price -= armor_prices[(ci->data.datab[2] * 5)];

                if (ci->data.datab[6] > pmt_guard.dfp_range)
                    variation = 0;
                else
                    variation = ci->data.datab[6];

                if (ci->data.datab[8] <= pmt_guard.dfp_range)
                    variation += ci->data.datab[8];

                price += equip_prices[1][1][ci->data.datab[2]][variation];
            }
            break;
        case ITEM_SUBTYPE_BARRIER: // Shield 盾牌

            if (pmt_lookup_guard_bb(ci->data.datal[0], &pmt_guard)) {
                ERR_LOG("未从PMT获取到 0x%04X 的数据!", ci->data.datal[0]);
                return -3;
            }

            if (ci->data.datab[2] < 0x15) {
                if (ci->data.datab[6] > pmt_guard.dfp_range)
                    variation = 0;
                else
                    variation = ci->data.datab[6];

                if (ci->data.datab[8] <= pmt_guard.evp_range)
                    variation += ci->data.datab[8];

                price = equip_prices[1][2][ci->data.datab[2]][variation];
            }
            break;
        case ITEM_SUBTYPE_UNIT: // Units 插槽
            if (ci->data.datab[2] < 0x40)
                price = unit_prices[ci->data.datab[2]];
            break;
        }
        break;

    case ITEM_TYPE_TOOL:
        // Tool 工具
        if (ci->data.datab[1] == 0x02) { // Technique 魔法科技
            if (ci->data.datab[4] < 0x13)
                price = ((int32_t)(ci->data.datab[2] + 1) * tech_prices[ci->data.datab[4]]) / 100L;
        }
        else {
            compare_item = 0;
            memcpy(&compare_item, &ci->data.datab[0], 3);
            for (ch = 0; ch < (sizeof(tool_prices) / 4); ch += 2)
                if (compare_item == tool_prices[ch]) {
                    price = tool_prices[ch + 1];
                    break;
                }
        }
        break;
    }

    if (price < 0)
        price = 0;

    return price;
}

item_t create_bb_shop_item(uint8_t 难度, uint8_t 物品类型, struct mt19937_state* 随机因子) {
    static const uint8_t max_percentages[4] = { 20, 35, 45, 50 };
    static const uint8_t max_quantity[4] =  { 1,  1,  2,  2 };
    static const uint8_t base_tech_lvl[4] = { 1,  8, 15, 23 };
    static const uint8_t max_tech_lvl[4] =  { 8, 15, 23, 30 };
    static const uint8_t base_anti_lvl[4] = { 1,  2,  4,  5 };
    static const uint8_t max_anti_lvl[4] =  { 2,  4,  6,  7 };
    item_t item = { 0 };
    uint32_t tmp_value = 0;
    item.datab[0] = 物品类型;

    while (item.datab[0] == ITEM_TYPE_MAG) {
        item.datab[0] = mt19937_genrand_int32(随机因子) % 3;
    }

    /* 检索物品类型 */
    switch (item.datab[0]) {
    case ITEM_TYPE_WEAPON: // 武器
        item.datab[1] = (mt19937_genrand_int32(随机因子) % 12) + 1;

        if (item.datab[1] > 9) {
            item.datab[2] = 难度;
        }
        else
            item.datab[2] = (mt19937_genrand_int32(随机因子) & 1) + 难度;

        item.datab[3] = mt19937_genrand_int32(随机因子) % 11;
        item.datab[4] = mt19937_genrand_int32(随机因子) % 11;

        size_t num_percentages = 0;
        for (size_t x = 0; (x < 5) && (num_percentages < 3); x++) {
            if ((mt19937_genrand_int32(随机因子) % 4) == 1) {
                item.datab[(num_percentages * 2) + 6] = (uint8_t)x;
                item.datab[(num_percentages * 2) + 7] = mt19937_genrand_int32(随机因子) % (max_percentages[难度] + 1);
                num_percentages++;
            }
        }

        break;

    case ITEM_TYPE_GUARD: // 装甲
        item.datab[1] = 0;

        while (item.datab[1] == 0)
            item.datab[1] = mt19937_genrand_int32(随机因子) & 3;

        switch (item.datab[1]) {
        case ITEM_SUBTYPE_FRAME://护甲
            item.datab[2] = (mt19937_genrand_int32(随机因子) % 6) + (难度 * 6);
            item.datab[5] = mt19937_genrand_int32(随机因子) % 5;
            break;

        case ITEM_SUBTYPE_BARRIER://护盾 0 - 20 
            item.datab[2] = (mt19937_genrand_int32(随机因子) % 5) + (难度 * 5);//TODO 价格加个控制

            item.datab[6] = (mt19937_genrand_int32(随机因子) % 9) - 4;
            //if (tmp_value < 0)
            //    item.data_b[6] -= tmp_value;
            //else
            //    item.data_b[6] = tmp_value;

            item.datab[8] = mt19937_genrand_int32(随机因子) % 5;
            //if (tmp_value < 0)
            //    item.data_b[8] -= tmp_value;
            //else
            //    item.data_b[8] = tmp_value;

            //item.costb[2] = (mt19937_genrand_int32(随机因子) % 6) + (难度 * 5);//TODO 价格加个控制
            break;

        case ITEM_SUBTYPE_UNIT://插件 不生成带属性的 省的麻烦 TODO 以后再做更详细的
            item.datab[2] = (mt19937_genrand_int32(随机因子) % 2);

            //item.data_b[6] = (mt19937_genrand_int32(随机因子) % 3) - 2;

            //DBG_LOG("%02X %d %d", item.data_b[6], (mt19937_genrand_int32(随机因子) % 3) - 2, 难度);

            //if (item.data_b[6] > 1)
                //item.data_b[7] = 0xFF;

            //item.costb[2] = mt19937_genrand_int32(随机因子) % 0x3B;
            break;
        }
        break;

    case ITEM_TYPE_TOOL: // 药品工具
        item.datab[1] = mt19937_genrand_int32(随机因子) % 11;
        switch (item.datab[1]) {
        case ITEM_SUBTYPE_MATE:
        case ITEM_SUBTYPE_FLUID:
            switch (难度) {
            case GAME_TYPE_NORMAL:
                item.datab[2] = 0;
                break;

            case GAME_TYPE_EPISODE_1:
                item.datab[2] = mt19937_genrand_int32(随机因子) % 2;
                break;

            case GAME_TYPE_EPISODE_2:
                item.datab[2] = (mt19937_genrand_int32(随机因子) % 2) + 1;
                break;

            case GAME_TYPE_EPISODE_4:
                item.datab[2] = 2;
                break;
            }
            break;

        case ITEM_SUBTYPE_ANTI:
            item.datab[2] = mt19937_genrand_int32(随机因子) % 2;
            break;

        case ITEM_SUBTYPE_GRINDER:
            item.datab[2] = mt19937_genrand_int32(随机因子) % 3;
            break;

        //case ITEM_SUBTYPE_MATERIAL:
        //    item.datab[2] = mt19937_genrand_int32(随机因子) % 7;
        //    break;
        }

        switch (item.datab[1]) {
        case ITEM_SUBTYPE_DISK:
            item.datab[4] = mt19937_genrand_int32(随机因子) % 19;

            uint8_t ge_tech_level = mt19937_genrand_int32(随机因子) % max_tech_lvl[难度];
            uint8_t ge_anti_level = mt19937_genrand_int32(随机因子) % max_anti_lvl[难度];
            uint32_t random = mt19937_genrand_int32(随机因子) % 10;/* 0 - 9 */

            switch (item.datab[4]) {
            case 14:
            case 17:
                item.datab[2] = 0; // reverser & ryuker always level 1 这两个法术永远是1级
                break;

            case 16:

                if (random <= 1) {
                    // 生成一个介于基础值和最大技术等级之间的随机数
                    ge_anti_level = base_anti_lvl[难度] + mt19937_genrand_int32(随机因子) % (max_anti_lvl[难度] - base_anti_lvl[难度]);
                }
                else {
                    // 生成一个介于基础值和全局最大值之间的随机数
                    ge_anti_level = mt19937_genrand_int32(随机因子) % (max_anti_lvl[难度] - base_anti_lvl[0]);
                }

                item.datab[2] = ge_anti_level;
                break;

            default:
                if (random <= 1) {
                    // 生成一个介于基础值和最大技术等级之间的随机数
                    ge_tech_level = base_tech_lvl[难度] + mt19937_genrand_int32(随机因子) % (max_tech_lvl[难度] - base_tech_lvl[难度]);
                }
                else {
                    // 生成一个介于基础值和全局最大值之间的随机数
                    ge_tech_level = mt19937_genrand_int32(随机因子) % (max_tech_lvl[难度] - base_tech_lvl[0]);
                }

                item.datab[2] = ge_tech_level;
                break;
            }
            break;
        case ITEM_SUBTYPE_MATE:
        case ITEM_SUBTYPE_FLUID:
        case ITEM_SUBTYPE_SOL_ATOMIZER:
        case ITEM_SUBTYPE_MOON_ATOMIZER:
        case ITEM_SUBTYPE_STAR_ATOMIZER:
        case ITEM_SUBTYPE_ANTI:
        case ITEM_SUBTYPE_TELEPIPE:
        case ITEM_SUBTYPE_TRAP_VISION:
        case ITEM_SUBTYPE_PHOTON:
            item.datab[5] = mt19937_genrand_int32(随机因子) % (max_quantity[难度] + 1);
            break;
        }
    }

    return item;
}

double floor(double x) {
    int32_t integerPart = (int32_t)x;
    if (x >= 0.0 && x != (double)integerPart)
        return integerPart + 1;
    return integerPart;
}

size_t price_for_item(const item_t* item) {
    pmt_weapon_bb_t pmt_weapon = { 0 };
    pmt_guard_bb_t pmt_guard = { 0 };
    pmt_tool_bb_t pmt_tool = { 0 };
    errno_t err = 0;

    switch (item->datab[0]) {
    case ITEM_TYPE_WEAPON: {
        if (item->datab[4] & 0x80) {
            return 8;
        }
        if (is_item_rare(item)) {
            DBG_LOG("物品 0x%08X 是稀有物品", item->datal[0]);
            return 80;
        }

        float sale_divisor = pmt_lookup_sale_divisor_bb(item->datab[0], item->datab[1]);
        if (sale_divisor == 0.0) {
            ERR_LOG("物品 sale divisor 为0 0x%08X", item->datal[0]);
            return 0;
        }

        if (err = pmt_lookup_weapon_bb(item->datal[0], &pmt_weapon)) {
            ERR_LOG("从PMT未获取到准确的数据! 0x%08X 错误码 %d", item->datal[0], err);
            return -1;
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

        return special_stars_factor + (atp_factor * (bonus_factor / 100.0));
    }

    case ITEM_TYPE_GUARD: {
        if (is_item_rare(item)) {
            DBG_LOG("物品 0x%08X 是稀有物品", item->datal[0]);
            return 80;
        }

        if (item->datab[1] == ITEM_SUBTYPE_UNIT) { // Unit
            return get_item_adjusted_stars(item) * pmt_lookup_sale_divisor_bb(item->datab[0], 3);
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
            return -1;
        }

        double power_factor = pmt_guard.base_dfp + pmt_guard.base_evp + def_bonus + evp_bonus;
        double power_factor_floor = (int32_t)((power_factor * power_factor) / sale_divisor);
        return power_factor_floor + (70.0 * (double)(item->datab[5] + 1) * (double)(pmt_guard.level_req + 1));
    }

    case ITEM_TYPE_MAG:
        return (item->datab[2] + 1) * pmt_lookup_sale_divisor_bb(item->datab[0], item->datab[1]);

    case ITEM_TYPE_TOOL: {

        if (err = pmt_lookup_tools_bb(item->datal[0], &pmt_tool)) {
            ERR_LOG("从PMT未获取到准确的数据! 0x%08X 错误码 %d", item->datal[0], err);
            return -1;
        }

        return pmt_tool.cost * ((item->datab[1] == 2) ? (item->datab[2] + 1) : 1);
    }

    case ITEM_TYPE_MESETA:
        return item->data2l;

    default:
        ERR_LOG("无效物品, 价格为0 0x%08X", item->datal[0]);
        return 0;
    }
    ERR_LOG("不会吧 还能跑到这里吗？ 0x%08X", item->datal[0]);
}

/* 加载商店数据 */
int load_shop_data()
{
	uint32_t shop_checksum;
	uint32_t ch;
	FILE* fp;
	int rv = 0;
	if (file_log_console_show) {
		CONFIG_LOG("读取 Blue Burst 商店数据 %s ", shop_files[0]);
	}

	if (fopen_s(&fp, shop_files[0], "rb"))
	{
		ERR_LOG("Blue Burst 商店数据 %s 文件已缺失.", shop_files[0]);
		rv = -1;
		goto err;
	}

	if (fread(&shops[0], 1, 7000 * sizeof(shop_data_t), fp) != (7000 * sizeof(shop_data_t)))
	{
		ERR_LOG("Blue Burst 商店数据文件大小有误...");
		rv = -1;
		goto err;
	}

	fclose(fp);

	shop_checksum = psocn_crc32(&shops[0], 7000 * sizeof(shop_data_t));

	if (file_log_console_show) {
		CONFIG_LOG("读取 Blue Burst 商店数据2 %s", shop_files[1]);
	}

	if (fopen_s(&fp, shop_files[1], "rb"))
	{
		ERR_LOG("Blue Burst 商店数据2 %s 文件已缺失.", shop_files[1]);
		rv = -1;
		goto err;
	}

	fread(&equip_prices[0], 1, sizeof(equip_prices), fp);
	fclose(fp);

	// 基于玩家的等级设置商店基础物品...

	for (ch = 0; ch < MAX_PLAYER_LEVEL; ch++)
	{
		switch (ch / 20L)
		{
		case 0:	// Levels 1-20
			shopidx[ch] = 0;
			break;
		case 1: // Levels 21-40
			shopidx[ch] = 1000;
			break;
		case 2: // Levels 41-80
		case 3:
			shopidx[ch] = 2000;
			break;
		case 4: // Levels 81-120
		case 5:
			shopidx[ch] = 3000;
			break;
		case 6: // Levels 121-160
		case 7:
			shopidx[ch] = 4000;
			break;
		case 8: // Levels 161-180
			shopidx[ch] = 5000;
			break;
		default: // Levels 180+
			shopidx[ch] = 6000;
			break;
		}
	}

	return 0;
err:
	fclose(fp);
	return rv;
}