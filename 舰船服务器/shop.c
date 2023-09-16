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
#include <pso_opcodes_block.h>
#include <SFMT.h>

#include <pso_character.h>

#include "shop.h"
#include "pmtdata.h"

//��ȡ�̵�۸�
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
    case ITEM_TYPE_WEAPON: // Weapons ����
        if (ci->data.datab[4] & 0x80)
            price = 1; // Untekked = 1 meseta ȡ��ѡ�� = 1������
        else {
            if ((ci->data.datab[1] < 0x0D) && (ci->data.datab[2] < 0x05)) {
                if ((ci->data.datab[1] > 0x09) && (ci->data.datab[2] > 0x03)) // Canes, Rods, Wands become rare faster  ���ȡ�������ħ��Խ��Խϡ�� 
                    break;

                if (pmt_lookup_weapon_bb(ci->data.datal[0], &pmt_weapon)) {
                    ERR_LOG("��PMTδ��ȡ��׼ȷ������!");
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
        case ITEM_SUBTYPE_FRAME: // Armor װ��

            if (pmt_lookup_guard_bb(ci->data.datal[0], &pmt_guard)) {
                ERR_LOG("δ��PMT��ȡ�� 0x%04X ������!", ci->data.datal[0]);
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
        case ITEM_SUBTYPE_BARRIER: // Shield ����

            if (pmt_lookup_guard_bb(ci->data.datal[0], &pmt_guard)) {
                ERR_LOG("δ��PMT��ȡ�� 0x%04X ������!", ci->data.datal[0]);
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
        case ITEM_SUBTYPE_UNIT: // Units ���
            if (ci->data.datab[2] < 0x40)
                price = unit_prices[ci->data.datab[2]];
            break;
        }
        break;

    case ITEM_TYPE_TOOL:
        // Tool ����
        if (ci->data.datab[1] == ITEM_SUBTYPE_DISK) { // Technique ħ���Ƽ�
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

item_t create_bb_shop_tool_common_item(uint8_t �Ѷ�, uint8_t ��Ʒ����, uint8_t index) {
    static const uint8_t max_quantity[4] = { 1,  1,  1,  1 };
    item_t item = { 0 };
    item.datab[0] = ��Ʒ����;

    /* ������Ʒ���� */
    switch (item.datab[0]) {
    case ITEM_TYPE_TOOL: // ����

        item.datab[1] = index;

        switch (item.datab[1]) {
        case ITEM_SUBTYPE_MATE:
        case ITEM_SUBTYPE_FLUID:
            switch (�Ѷ�) {
            case GAME_TYPE_DIFFICULTY_NORMARL:
                item.datab[2] = 0;
                break;

            case GAME_TYPE_DIFFICULTY_HARD:
                item.datab[2] = 1;
                break;

            case GAME_TYPE_DIFFICULTY_VERY_HARD:
                item.datab[2] = 2;
                break;

            case GAME_TYPE_DIFFICULTY_ULTIMATE:
                item.datab[2] = 2;
                break;
            }
            item.datab[5] = max_quantity[�Ѷ�];
            break;
        }

        break;
    }
#ifdef DEBUG

    DBG_LOG("�Ѷ� %d item 0x%08X", �Ѷ�, item.datal[0]);

#endif // DEBUG

    return item;
}

item_t create_bb_shop_item(uint8_t �Ѷ�, uint8_t ��Ʒ����, sfmt_t* �������) {
    static const uint8_t max_percentages[4] = { 20, 35, 45, 50 };
    static const uint8_t max_quantity[4] = { 1,  1,  2,  2 };
    static const uint8_t max_tech_lvl[4] = { 4, 7, 10, 15 };
    static const uint8_t max_anti_lvl[4] = { 2,  4,  6,  7 };
    item_t item = { 0 };
    uint32_t tmp_value = 0;
    item.datab[0] = ��Ʒ����;
    errno_t err = 0;

    while (item.datab[0] == ITEM_TYPE_MAG) {
        item.datab[0] = sfmt_genrand_uint32(�������) % 3;
    }

    /* ������Ʒ���� */
    switch (item.datab[0]) {
    case ITEM_TYPE_WEAPON: // ����
        item.datab[1] = (sfmt_genrand_uint32(�������) % 12) + 1; /* 01 - 0C ��ͨ��Ʒ*/

        /* 9 ���¶��� 0/1 + �Ѷ� 9������ 0-3���Ѷȣ�����ID*/
        if (item.datab[1] > 9) {
            item.datab[2] = �Ѷ�;
        }
        else
            item.datab[2] = (sfmt_genrand_uint32(�������) & 1) + �Ѷ�;

        /* ��ĥֵ 0 - 10*/
        item.datab[3] = sfmt_genrand_uint32(�������) % 11;
        /* ���⹥�� 0 - 10 ����Ѷ� 0 - 3*/
        item.datab[4] = sfmt_genrand_uint32(�������) % 11 + �Ѷ�;
        /* datab[5] �����ﲻ�漰 ���� δ����*/

        /* ��������*/
        size_t num_percentages = 0;
        while (num_percentages < 3) {
            /*0-5 ������������*/
            for (size_t x = 0; x < 6; x++) {
                if ((sfmt_genrand_uint32(�������) % 4) == 1) {
                    /*+6 ��Ӧ���Բۣ�����ֱ�Ϊ 6 8 10�� +7��Ӧ��ֵ������ֱ�Ϊ �����1-20 1-35 1-45 1-50��*/
                    item.datab[(num_percentages * 2) + 6] = (uint8_t)x;
                    item.datab[(num_percentages * 2) + 7] = sfmt_genrand_uint32(�������) % (max_percentages[�Ѷ�] + 1);
                    num_percentages++;
                }
            }
        }

        break;

    case ITEM_TYPE_GUARD: // װ��
        pmt_guard_bb_t pmt_guard = { 0 };
        pmt_unit_bb_t pmt_unit = { 0 };
        item.datab[1] = 0;

        /* ������1��2 ��Ӧ���׻��߻���*/
        while (item.datab[1] == 0)
            item.datab[1] = sfmt_genrand_uint32(�������) & 3;

        switch (item.datab[1]) {
        case ITEM_SUBTYPE_FRAME://����
            /*������Ʒ������*/
            item.datab[2] = (sfmt_genrand_uint32(�������) % 6) + (�Ѷ� * 6);
            if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
                ERR_LOG("pmt_lookup_guard_bb ����������! ������ %d 0x%08X", err, item.datal[0]);
                break;
            }

            /*�����λ 0 - 4 */
            item.datab[5] = sfmt_genrand_uint32(�������) % 5;

            /* DFPֵ */
            tmp_value = sfmt_genrand_uint32(�������) % pmt_guard.dfp_range;
            if (tmp_value < 0)
                tmp_value = 0;
            item.datab[6] = tmp_value;

            /* EVPֵ */
            tmp_value = sfmt_genrand_uint32(�������) % pmt_guard.evp_range;
            if (tmp_value < 0)
                tmp_value = 0;
            item.datab[8] = tmp_value;
            break;

        case ITEM_SUBTYPE_BARRIER://���� 0 - 20 

            /*������Ʒ������*/
            item.datab[2] = (sfmt_genrand_uint32(�������) % 5) + (�Ѷ� * 5);
            if (err = pmt_lookup_guard_bb(item.datal[0], &pmt_guard)) {
                ERR_LOG("pmt_lookup_guard_bb ����������! ������ %d 0x%08X", err, item.datal[0]);
                break;
            }

            /* DFPֵ */
            tmp_value = sfmt_genrand_uint32(�������) % pmt_guard.dfp_range;
            if (tmp_value < 0)
                tmp_value = 0;
            item.datab[6] = tmp_value;

            /* EVPֵ */
            tmp_value = sfmt_genrand_uint32(�������) % pmt_guard.evp_range;
            if (tmp_value < 0)
                tmp_value = 0;
            item.datab[8] = tmp_value;
            break;

        case ITEM_SUBTYPE_UNIT://��� �����ɴ����Ե� ʡ���鷳 TODO �Ժ���������ϸ��
            item.datab[2] = (sfmt_genrand_uint32(�������) % 2);
            if (err = pmt_lookup_unit_bb(item.datal[0], &pmt_unit)) {
                ERR_LOG("pmt_lookup_unit_bb ����������! ������ %d 0x%08X", err, item.datal[0]);
                break;
            }

            //tmp_value = sfmt_genrand_uint32(�������) % pmt_unit.pm_range;
            //int randomIndex = 0;

            //item.dataw[3] = 0;

            //switch (tmp_value) {
            //case 1:
            //    randomIndex = sfmt_genrand_uint32(�������) % 3; // �������һ��0-2��Χ�ڵ�����

            //    switch (randomIndex) {
            //    case 0:
            //        item.dataw[3] = LE16(0xFFFF);
            //        break;

            //    case 1:
            //        item.dataw[3] = LE16(0x0000);
            //        break;

            //    case 2:
            //        item.dataw[3] = LE16(0x0001);
            //        break;
            //    }
            //    break;

            //case 2:
            //    randomIndex = sfmt_genrand_uint32(�������) % 5; // �������һ��0-4��Χ�ڵ�����

            //    switch (randomIndex) {
            //    case 0:
            //        item.dataw[3] = LE16(0xFFFE);
            //        break;

            //    case 1:
            //        item.dataw[3] = LE16(0xFFFF);
            //        break;

            //    case 2:
            //        item.dataw[3] = LE16(0x0000);
            //        break;

            //    case 3:
            //        item.dataw[3] = LE16(0x0001);
            //        break;

            //    case 4:
            //        item.dataw[3] = LE16(0x0002);
            //        break;
            //    }
            //    break;
            //}
            break;
        }
        break;

    case ITEM_TYPE_TOOL: // ҩƷ����
        item.datab[1] = sfmt_genrand_uint32(�������) % 9 + 2;
        switch (item.datab[1]) {
            //case ITEM_SUBTYPE_MATE:
            //case ITEM_SUBTYPE_FLUID:
            //    switch (�Ѷ�) {
            //    case GAME_TYPE_DIFFICULTY_NORMARL:
            //        item.datab[2] = 0;
            //        break;

            //    case GAME_TYPE_DIFFICULTY_HARD:
            //        item.datab[2] = sfmt_genrand_uint32(�������) % 2;
            //        break;

            //    case GAME_TYPE_DIFFICULTY_VERY_HARD:
            //        item.datab[2] = (sfmt_genrand_uint32(�������) % 2) + 1;
            //        break;

            //    case GAME_TYPE_DIFFICULTY_ULTIMATE:
            //        item.datab[2] = 2;
            //        break;
            //    }
            //    break;

        case ITEM_SUBTYPE_ANTI_TOOL:
            item.datab[2] = sfmt_genrand_uint32(�������) % 2;
            break;

        case ITEM_SUBTYPE_GRINDER:
            item.datab[2] = sfmt_genrand_uint32(�������) % 3;
            break;

            //case ITEM_SUBTYPE_MATERIAL:
            //    item.datab[2] = sfmt_genrand_uint32(�������) % 7;
            //    break;
        }

        switch (item.datab[1]) {
        case ITEM_SUBTYPE_DISK:
            item.datab[4] = sfmt_genrand_uint32(�������) % 19;
            switch (item.datab[4]) {
            case TECHNIQUE_RYUKER:
            case TECHNIQUE_REVERSER:
                item.datab[2] = 0; // reverser & ryuker always level 1 ������������Զ��1��
                break;
            case TECHNIQUE_ANTI:
                item.datab[2] = sfmt_genrand_uint32(�������) % max_anti_lvl[�Ѷ�];
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
                item.datab[2] = sfmt_genrand_uint32(�������) % max_tech_lvl[�Ѷ�];
                break;
            }
            break;
            //case ITEM_SUBTYPE_MATE:
            //case ITEM_SUBTYPE_FLUID:
        case ITEM_SUBTYPE_SOL_ATOMIZER:
        case ITEM_SUBTYPE_MOON_ATOMIZER:
        case ITEM_SUBTYPE_STAR_ATOMIZER:
        case ITEM_SUBTYPE_ANTI_TOOL:
        case ITEM_SUBTYPE_TELEPIPE:
        case ITEM_SUBTYPE_TRAP_VISION:
        case ITEM_SUBTYPE_PHOTON:
            item.datab[5] = sfmt_genrand_uint32(�������) % (max_quantity[�Ѷ�] + 1);
            break;
        }
    }

    return item;
}

item_t create_bb_shop_items(uint32_t �̵�����, uint8_t �Ѷ�, uint8_t ��Ʒ����, sfmt_t* �������) {
    item_t item = { 0 };

    switch (�̵�����) {
    case BB_SHOPTYPE_TOOL:// �����̵�
        if (��Ʒ���� < 2)
            item = create_bb_shop_tool_common_item(�Ѷ�, ITEM_TYPE_TOOL, ��Ʒ����);
        else
            item = create_bb_shop_item(�Ѷ�, ITEM_TYPE_TOOL, �������);
        break;

    case BB_SHOPTYPE_WEAPON:// �����̵�
        item = create_bb_shop_item(�Ѷ�, ITEM_TYPE_WEAPON, �������);
        break;

    case BB_SHOPTYPE_ARMOR:// װ���̵�
        item = create_bb_shop_item(�Ѷ�, ITEM_TYPE_GUARD, �������);
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
            DBG_LOG("��Ʒ 0x%08X ��ϡ����Ʒ", item->datal[0]);
#endif // DEBUG
            return 80;
        }

        float sale_divisor = pmt_lookup_sale_divisor_bb(item->datab[0], item->datab[1]);
        if (sale_divisor == 0.0) {
            ERR_LOG("��Ʒ sale divisor Ϊ0 0x%08X", item->datal[0]);
            return 0;
        }

        if (err = pmt_lookup_weapon_bb(item->datal[0], &pmt_weapon)) {
            ERR_LOG("��PMTδ��ȡ��׼ȷ������! 0x%08X ������ %d", item->datal[0], err);
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

            DBG_LOG("��Ʒ 0x%08X ��ϡ����Ʒ", item->datal[0]);

#endif // DEBUG

            return 80;
        }

        if (item->datab[1] == ITEM_SUBTYPE_UNIT) { // Unit
            price = (size_t)(get_item_adjusted_stars(item) * pmt_lookup_sale_divisor_bb(item->datab[0], 3));
            return price;
        }

        double sale_divisor = (double)pmt_lookup_sale_divisor_bb(item->datab[0], item->datab[1]);
        if (sale_divisor == 0.0) {
            ERR_LOG("��Ʒ sale divisor Ϊ0 0x%08X", item->datal[0]);
            return 0;
        }

        int16_t def_bonus = get_armor_or_shield_defense_bonus(item);
        int16_t evp_bonus = get_common_armor_evasion_bonus(item);

        if (err = pmt_lookup_guard_bb(item->datal[0], &pmt_guard)) {
            ERR_LOG("��PMTδ��ȡ��׼ȷ������! 0x%08X ������ %d", item->datal[0], err);
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
            ERR_LOG("��PMTδ��ȡ��׼ȷ������! 0x%08X ������ %d", item->datal[0], err);
            return err;
        }

        size_t price_num = 1;

        if (item->datab[1] == ITEM_SUBTYPE_DISK)
            if (item->datab[4] < 0x13)
                price_num = item->datab[2] + 1;

        price = pmt_tool.cost * price_num;
        if (price < 0) {
            ERR_LOG("pmt_lookup_tools_bb 0x%08X �ļ۸� %d Ϊ����", item->datal[0], pmt_tool.cost);
        }
        return price;
    }

    case ITEM_TYPE_MESETA:
        price = item->data2l;
        return price;

    default:
        ERR_LOG("��Ч��Ʒ, �۸�Ϊ0 0x%08X", item->datal[0]);
        return err;
    }
    ERR_LOG("����� �����ܵ������� 0x%08X", item->datal[0]);
}

/* �����̵����� */
int load_shop_data() {
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