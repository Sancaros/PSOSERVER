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
#include <pso_cmd_code.h>

#include <pso_character.h>

#include "shopdata.h"
#include "pmtdata.h"

//获取商店价格
uint32_t get_bb_shop_price(iitem_t* ci)
{
    uint32_t compare_item, ch;
    int32_t percent_add, price;
    uint8_t variation;
    float percent_calc;
    float price_calc;

    price = 10;

    switch (ci->data.data_b[0])
    {
    case 0x00: // Weapons 武器
        if (ci->data.data_b[4] & 0x80)
            price = 1; // Untekked = 1 meseta 取消选中 = 1美赛塔
        else
        {
            if ((ci->data.data_b[1] < 0x0D) && (ci->data.data_b[2] < 0x05))
            {
                if ((ci->data.data_b[1] > 0x09) && (ci->data.data_b[2] > 0x03)) // Canes, Rods, Wands become rare faster  拐杖、棍棒、魔杖越来越稀少 
                    break;
                price = pmt_weapon_bb[ci->data.data_b[1]][ci->data.data_b[2]]->atp_max + ci->data.data_b[3];
                price *= price;
                price_calc = (float)price;
                switch (ci->data.data_b[1])
                {
                case 0x01:
                    price_calc /= 5.0;
                    break;
                case 0x02:
                    price_calc /= 4.0;
                    break;
                case 0x03:
                case 0x04:
                    price_calc *= 2.0;
                    price_calc /= 3.0;
                    break;
                case 0x05:
                    price_calc *= 4.0;
                    price_calc /= 5.0;
                    break;
                case 0x06:
                    price_calc *= 10.0;
                    price_calc /= 21.0;
                    break;
                case 0x07:
                    price_calc /= 3.0;
                    break;
                case 0x08:
                    price_calc *= 25.0;
                    break;
                case 0x09:
                    price_calc *= 10.0;
                    price_calc /= 9.0;
                    break;
                case 0x0A:
                    price_calc /= 2.0;
                    break;
                case 0x0B:
                    price_calc *= 2.0;
                    price_calc /= 5.0;
                    break;
                case 0x0C:
                    price_calc *= 4.0;
                    price_calc /= 3.0;
                    break;
                }

                percent_add = 0;
                if (ci->data.data_b[6])
                    percent_add += (char)ci->data.data_b[7];
                if (ci->data.data_b[8])
                    percent_add += (char)ci->data.data_b[9];
                if (ci->data.data_b[10])
                    percent_add += (char)ci->data.data_b[11];

                if (percent_add != 0)
                {
                    percent_calc = price_calc;
                    percent_calc /= 300.0;
                    percent_calc *= percent_add;
                    price_calc += percent_calc;
                }
                price_calc /= 8.0;
                price = (int32_t)(price_calc);
                price += attrib[ci->data.data_b[4]];
            }
        }
        break;
    case 0x01:
        switch (ci->data.data_b[1])
        {
        case 0x01: // Armor 装备
            if (ci->data.data_b[2] < 0x18)
            {
                // Calculate the amount to boost because of slots...
                if (ci->data.data_b[5] > 4)
                    price = armor_prices[(ci->data.data_b[2] * 5) + 4];
                else
                    price = armor_prices[(ci->data.data_b[2] * 5) + ci->data.data_b[5]];
                price -= armor_prices[(ci->data.data_b[2] * 5)];
                if (ci->data.data_b[6] > pmt_armor_bb[ci->data.data_b[2]]->dfp_range)
                    variation = 0;
                else
                    variation = ci->data.data_b[6];
                if (ci->data.data_b[8] <= pmt_armor_bb[ci->data.data_b[2]]->dfp_range)
                    variation += ci->data.data_b[8];
                price += equip_prices[1][1][ci->data.data_b[2]][variation];
            }
            break;
        case 0x02: // Shield 盾牌
            if (ci->data.data_b[2] < 0x15)
            {
                if (ci->data.data_b[6] > pmt_shield_bb[ci->data.data_b[2]]->dfp_range)
                    variation = 0;
                else
                    variation = ci->data.data_b[6];
                if (ci->data.data_b[8] <= pmt_shield_bb[ci->data.data_b[2]]->evp_range)
                    variation += ci->data.data_b[8];
                price = equip_prices[1][2][ci->data.data_b[2]][variation];
            }
            break;
        case 0x03: // Units 插槽
            if (ci->data.data_b[2] < 0x40)
                price = unit_prices[ci->data.data_b[2]];
            break;
        }
        break;
    case 0x03:
        // Tool 工具
        if (ci->data.data_b[1] == 0x02) // Technique 魔法科技
        {
            if (ci->data.data_b[4] < 0x13)
                price = ((int32_t)(ci->data.data_b[2] + 1) * tech_prices[ci->data.data_b[4]]) / 100L;
        }
        else
        {
            compare_item = 0;
            memcpy(&compare_item, &ci->data.data_b[0], 3);
            for (ch = 0; ch < (sizeof(tool_prices) / 4); ch += 2)
                if (compare_item == tool_prices[ch])
                {
                    price = tool_prices[ch + 1];
                    break;
                }
        }
        break;
    }
    if (price < 0)
        price = 0;
    //ITEM_LOG("最终价格 = %u", price);
    return (uint32_t)price;
}

sitem_t create_bb_shop_item(uint8_t 难度, uint8_t 物品类型, struct mt19937_state* 随机因子) {
    static const uint8_t max_percentages[4] = { 20, 35, 45, 50 };
    static const uint8_t max_quantity[4] = { 1,  1,  2,  2 };
    static const uint8_t max_tech_level[4] = { 8, 15, 23, 30 };
    static const uint8_t max_anti_level[4] = { 2,  4,  6,  7 };
    sitem_t item = { 0 };
    item.data_b[0] = 物品类型;

    while (item.data_b[0] == 2) {
        item.data_b[0] = mt19937_genrand_int32(随机因子) % 3;
    }

    /* 检索物品类型 */
    switch (item.data_b[0]) {
    case 0x00: { // 武器
        item.data_b[1] = (mt19937_genrand_int32(随机因子) % 12) + 1;
        if (item.data_b[1] > 0x09) {
            item.data_b[2] = 难度;
        }
        else {
            item.data_b[2] = (mt19937_genrand_int32(随机因子) & 1) + 难度;
        }

        item.data_b[3] = mt19937_genrand_int32(随机因子) % 11;
        item.data_b[4] = mt19937_genrand_int32(随机因子) % 11;

        size_t num_percentages = 0;
        for (size_t x = 0; (x < 5) && (num_percentages < 3); x++) {
            if ((mt19937_genrand_int32(随机因子) % 4) == 1) {
                item.data_b[(num_percentages * 2) + 6] = (uint8_t)x;
                item.data_b[(num_percentages * 2) + 7] = mt19937_genrand_int32(随机因子) % (max_percentages[难度] + 1);
                num_percentages++;
            }
        }
        break;
    }
    case 0x01: // 装甲
        item.data_b[1] = 0;
        while (item.data_b[1] == 0) {
            item.data_b[1] = mt19937_genrand_int32(随机因子) & 3;
        }
        switch (item.data_b[1]) {
        case 0x01://护甲
            item.data_b[2] = (mt19937_genrand_int32(随机因子) % 6) + (难度 * 6);
            item.data_b[5] = mt19937_genrand_int32(随机因子) % 5;
            break;
        case 0x02://护盾
            item.costb[2] = (mt19937_genrand_int32(随机因子) % 6) + (难度 * 5);//TODO 价格加个控制
            item.data_b[6] = (mt19937_genrand_int32(随机因子) % 9) - 4;
            item.data_b[9] = (mt19937_genrand_int32(随机因子) % 9) - 4;
            break;
        case 0x03://插件
            item.costb[2] = mt19937_genrand_int32(随机因子) % 0x3B;
            item.data_b[7] = (mt19937_genrand_int32(随机因子) % 5) - 4;
            break;
        }
        break;
    case 0x03: // 药品工具
        item.data_b[1] = mt19937_genrand_int32(随机因子) % 12;
        switch (item.data_b[1]) {
        case 0x00:
        case 0x01:

            switch (难度) {
            case 0:
                item.data_b[2] = 0;
                break;

            case 1:
                item.data_b[2] = mt19937_genrand_int32(随机因子) % 2;
                break;

            case 2:
                item.data_b[2] = (mt19937_genrand_int32(随机因子) % 2) + 1;
                break;

            case 3:
                item.data_b[2] = 2;
                break;
            }

            break;

        case 0x06:
            item.data_b[2] = mt19937_genrand_int32(随机因子) % 2;
            break;

        case 0x0A:
            item.data_b[2] = mt19937_genrand_int32(随机因子) % 3;
            break;

        case 0x0B:
            item.data_b[2] = mt19937_genrand_int32(随机因子) % 7;
            break;
        }

        switch (item.data_b[1]) {
        case 0x02:
            item.data_b[4] = mt19937_genrand_int32(随机因子) % 19;
            switch (item.data_b[4]) {
            case 14:
            case 17:
                item.data_b[2] = 0; // reverser & ryuker always level 1 
                break;
            case 16:
                item.data_b[2] = mt19937_genrand_int32(随机因子) % max_anti_level[难度];
                break;
            default:
                item.data_b[2] = mt19937_genrand_int32(随机因子) % max_tech_level[难度];
                break;
            }
            break;
        case 0x00:
        case 0x01:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x10:
            item.data_b[5] = mt19937_genrand_int32(随机因子) % (max_quantity[难度] + 1);
            break;
        }
    }

    return item;
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