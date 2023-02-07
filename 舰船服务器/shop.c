/*
    �λ�֮���й� ����������
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <f_logs.h>
#include <f_checksum.h>
#include <pso_cmd_code.h>

#include <pso_character.h>

#include "shop.h"
#include "pmtdata.h"

//��ȡ�̵�۸�
uint32_t get_bb_shop_price(iitem_t* ci) {
    pmt_weapon_bb_t tmp_wp = { 0 };
    pmt_guard_bb_t tmp_guard = { 0 };
    uint32_t compare_item, ch;
    int32_t percent_add, price;
    uint8_t variation;
    float percent_calc;
    float price_calc;

    price = 10;

    switch (ci->data.data_b[0]) {
    case ITEM_TYPE_WEAPON: // Weapons ����
        if (ci->data.data_b[4] & 0x80)
            price = 1; // Untekked = 1 meseta ȡ��ѡ�� = 1������
        else {
            if ((ci->data.data_b[1] < 0x0D) && (ci->data.data_b[2] < 0x05)) {
                if ((ci->data.data_b[1] > 0x09) && (ci->data.data_b[2] > 0x03)) // Canes, Rods, Wands become rare faster  ���ȡ�������ħ��Խ��Խϡ�� 
                    break;

                if (pmt_lookup_weapon_bb(ci->data.data_l[0], &tmp_wp)) {
                    ERR_LOG("��PMTδ��ȡ��׼ȷ������!");
                    return -1;
                }

                price = tmp_wp.atp_max + ci->data.data_b[3];
                price *= price;
                price_calc = (float)price;
                switch (ci->data.data_b[1]) {
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

                if (ci->data.data_b[6])
                    percent_add += (char)ci->data.data_b[7];

                if (ci->data.data_b[8])
                    percent_add += (char)ci->data.data_b[9];

                if (ci->data.data_b[10])
                    percent_add += (char)ci->data.data_b[11];

                if (percent_add != 0) {
                    percent_calc = price_calc;
                    percent_calc /= 300.0f;
                    percent_calc *= percent_add;
                    price_calc += percent_calc;
                }

                price_calc /= 8.0f;
                price = (int32_t)(price_calc);
                price += attrib[ci->data.data_b[4]];
            }
        }
        break;

    case ITEM_TYPE_GUARD:
        switch (ci->data.data_b[1]) {
        case ITEM_SUBTYPE_FRAME: // Armor װ��
            if (ci->data.data_b[2] < 0x18) {
                if (pmt_lookup_guard_bb(ci->data.data_l[0], &tmp_guard)) {
                    ERR_LOG("��PMTδ��ȡ��׼ȷ������!");
                    return -3;
                }

                // Calculate the amount to boost because of slots...
                if (ci->data.data_b[5] > 4)
                    price = armor_prices[(ci->data.data_b[2] * 5) + 4];
                else
                    price = armor_prices[(ci->data.data_b[2] * 5) + ci->data.data_b[5]];

                price -= armor_prices[(ci->data.data_b[2] * 5)];

                if (ci->data.data_b[6] > tmp_guard.dfp_range)
                    variation = 0;
                else
                    variation = ci->data.data_b[6];

                if (ci->data.data_b[8] <= tmp_guard.dfp_range)
                    variation += ci->data.data_b[8];

                price += equip_prices[1][1][ci->data.data_b[2]][variation];
            }
            break;
        case ITEM_SUBTYPE_BARRIER: // Shield ����
            if (ci->data.data_b[2] < 0x15) {
                if (pmt_lookup_guard_bb(ci->data.data_l[0], &tmp_guard)) {
                    ERR_LOG("��PMTδ��ȡ��׼ȷ������!");
                    return -3;
                }

                if (ci->data.data_b[6] > tmp_guard.dfp_range)
                    variation = 0;
                else
                    variation = ci->data.data_b[6];

                if (ci->data.data_b[8] <= tmp_guard.evp_range)
                    variation += ci->data.data_b[8];

                price = equip_prices[1][2][ci->data.data_b[2]][variation];
            }
            break;
        case ITEM_SUBTYPE_UNIT: // Units ���
            if (ci->data.data_b[2] < 0x40)
                price = unit_prices[ci->data.data_b[2]];
            break;
        }
        break;

    case ITEM_TYPE_TOOL:
        // Tool ����
        if (ci->data.data_b[1] == 0x02) { // Technique ħ���Ƽ�
            if (ci->data.data_b[4] < 0x13)
                price = ((int32_t)(ci->data.data_b[2] + 1) * tech_prices[ci->data.data_b[4]]) / 100L;
        }
        else {
            compare_item = 0;
            memcpy(&compare_item, &ci->data.data_b[0], 3);
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

    return (uint32_t)price;
}

sitem_t create_bb_shop_item(uint8_t �Ѷ�, uint8_t ��Ʒ����, struct mt19937_state* �������) {
    static const uint8_t max_percentages[4] = { 20, 35, 45, 50 };
    static const uint8_t max_quantity[4] = { 1,  1,  2,  2 };
    static const uint8_t max_tech_lvl[4] = { 8, 15, 23, 30 };
    static const uint8_t max_anti_lvl[4] = { 2,  4,  6,  7 };
    sitem_t item = { 0 };
    uint32_t tmp_value = 0;
    item.data_b[0] = ��Ʒ����;

    while (item.data_b[0] == ITEM_TYPE_MAG) {
        item.data_b[0] = mt19937_genrand_int32(�������) % 3;
    }

    /* ������Ʒ���� */
    switch (item.data_b[0]) {
    case ITEM_TYPE_WEAPON: // ����
        item.data_b[1] = (mt19937_genrand_int32(�������) % 12) + 1;

        if (item.data_b[1] > 9) {
            item.data_b[2] = �Ѷ�;
        }
        else
            item.data_b[2] = (mt19937_genrand_int32(�������) & 1) + �Ѷ�;

        item.data_b[3] = mt19937_genrand_int32(�������) % 11;
        item.data_b[4] = mt19937_genrand_int32(�������) % 11;

        size_t num_percentages = 0;
        for (size_t x = 0; (x < 5) && (num_percentages < 3); x++) {
            if ((mt19937_genrand_int32(�������) % 4) == 1) {
                item.data_b[(num_percentages * 2) + 6] = (uint8_t)x;
                item.data_b[(num_percentages * 2) + 7] = mt19937_genrand_int32(�������) % (max_percentages[�Ѷ�] + 1);
                num_percentages++;
            }
        }

        break;

    case ITEM_TYPE_GUARD: // װ��
        item.data_b[1] = 0;

        while (item.data_b[1] == 0)
            item.data_b[1] = mt19937_genrand_int32(�������) & 3;

        switch (item.data_b[1]) {
        case ITEM_SUBTYPE_FRAME://����
            item.data_b[2] = (mt19937_genrand_int32(�������) % 6) + (�Ѷ� * 6);
            item.data_b[5] = mt19937_genrand_int32(�������) % 5;
            break;

        case ITEM_SUBTYPE_BARRIER://���� 0 - 20 
            item.data_b[2] = (mt19937_genrand_int32(�������) % 5) + (�Ѷ� * 5);//TODO �۸�Ӹ�����

            item.data_b[6] = (mt19937_genrand_int32(�������) % 9) - 4;
            //if (tmp_value < 0)
            //    item.data_b[6] -= tmp_value;
            //else
            //    item.data_b[6] = tmp_value;

            item.data_b[8] = mt19937_genrand_int32(�������) % 5;
            //if (tmp_value < 0)
            //    item.data_b[8] -= tmp_value;
            //else
            //    item.data_b[8] = tmp_value;

            //item.costb[2] = (mt19937_genrand_int32(�������) % 6) + (�Ѷ� * 5);//TODO �۸�Ӹ�����
            break;

        case ITEM_SUBTYPE_UNIT://���
            item.data_b[2] = (mt19937_genrand_int32(�������) % 2) + 4;

            item.data_b[7] = (mt19937_genrand_int32(�������) % 5) - 4 + (�Ѷ� * 5);
            //if (tmp_value < 0)
            //    item.data_b[7] -= tmp_value;
            //else
            //    item.data_b[7] = tmp_value;

            //item.costb[2] = mt19937_genrand_int32(�������) % 0x3B;
            break;
        }
        break;

    case ITEM_TYPE_TOOL: // ҩƷ����
        item.data_b[1] = mt19937_genrand_int32(�������) % 12;
        switch (item.data_b[1]) {
        case ITEM_SUBTYPE_MATE:
        case ITEM_SUBTYPE_FLUID:
            switch (�Ѷ�) {
            case GAME_TYPE_NORMAL:
                item.data_b[2] = 0;
                break;

            case GAME_TYPE_EPISODE_1:
                item.data_b[2] = mt19937_genrand_int32(�������) % 2;
                break;

            case GAME_TYPE_EPISODE_2:
                item.data_b[2] = (mt19937_genrand_int32(�������) % 2) + 1;
                break;

            case GAME_TYPE_EPISODE_4:
                item.data_b[2] = 2;
                break;
            }
            break;

        case ITEM_SUBTYPE_ANTI:
            item.data_b[2] = mt19937_genrand_int32(�������) % 2;
            break;

        case ITEM_SUBTYPE_GRINDER:
            item.data_b[2] = mt19937_genrand_int32(�������) % 3;
            break;

        case ITEM_SUBTYPE_MATERIAL:
            item.data_b[2] = mt19937_genrand_int32(�������) % 7;
            break;
        }

        switch (item.data_b[1]) {
        case ITEM_SUBTYPE_DISK:
            item.data_b[4] = mt19937_genrand_int32(�������) % 19;
            switch (item.data_b[4]) {
            case 14:
            case 17:
                item.data_b[2] = 0; // reverser & ryuker always level 1 ������������Զ��1��
                break;
            case 16:
                item.data_b[2] = mt19937_genrand_int32(�������) % max_anti_lvl[�Ѷ�];
                break;
            default:
                item.data_b[2] = mt19937_genrand_int32(�������) % max_tech_lvl[�Ѷ�];
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
            item.data_b[5] = mt19937_genrand_int32(�������) % (max_quantity[�Ѷ�] + 1);
            break;
        }
    }

    return item;
}

/* �����̵����� */
int load_shop_data()
{
	uint32_t shop_checksum;
	uint32_t ch;
	FILE* fp;
	int rv = 0;
	if (file_log_console_show) {
		CONFIG_LOG("��ȡ Blue Burst �̵����� %s ", shop_files[0]);
	}

	if (fopen_s(&fp, shop_files[0], "rb"))
	{
		ERR_LOG("Blue Burst �̵����� %s �ļ���ȱʧ.", shop_files[0]);
		rv = -1;
		goto err;
	}

	if (fread(&shops[0], 1, 7000 * sizeof(shop_data_t), fp) != (7000 * sizeof(shop_data_t)))
	{
		ERR_LOG("Blue Burst �̵������ļ���С����...");
		rv = -1;
		goto err;
	}

	fclose(fp);

	shop_checksum = psocn_crc32(&shops[0], 7000 * sizeof(shop_data_t));

	if (file_log_console_show) {
		CONFIG_LOG("��ȡ Blue Burst �̵�����2 %s", shop_files[1]);
	}

	if (fopen_s(&fp, shop_files[1], "rb"))
	{
		ERR_LOG("Blue Burst �̵�����2 %s �ļ���ȱʧ.", shop_files[1]);
		rv = -1;
		goto err;
	}

	fread(&equip_prices[0], 1, sizeof(equip_prices), fp);
	fclose(fp);

	// ������ҵĵȼ������̵������Ʒ...

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