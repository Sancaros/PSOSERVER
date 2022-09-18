/*
    �λ�֮���й� ����������
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <f_logs.h>
#include <f_checksum.h>
#include <packetlist.h>

#include <pso_character.h>

#include "shopdata.h"
#include "pmtdata.h"

//��ȡ�̵�۸�
uint32_t get_bb_shop_price(iitem_t* ci)
{
    uint32_t compare_item, ch;
    int32_t percent_add, price;
    uint8_t variation;
    float percent_calc;
    float price_calc;

    price = 10;

    //���Լ����̵���Ʒ������
    //ITEM_LOG("����Ʒ�ֽ�����Ϊ:\r\n%02X%02X%02X%02X\r\n%02X%02X%02X%02X\r\n%02X%02X%02X%02X\r\n%02X%02X%02X%02X",
    /*ci->data_b[0], ci->data_b[1], ci->data_b[2], ci->data_b[3],
    ci->data_b[4], ci->data_b[5], ci->data_b[6], ci->data_b[7],
    ci->data_b[8], ci->data_b[9], ci->data_b[10], ci->data_b[11],
    ci->data2_b[0], ci->data2_b[1], ci->data2_b[2], ci->data2_b[3]);*/
    /*����Ʒ�ֽ�����Ϊ:
00060000
00000000
00000000*/
    switch (ci->data_b[0])
    {
    case 0x00: // Weapons ����
        if (ci->data_b[4] & 0x80)
            price = 1; // Untekked = 1 meseta ȡ��ѡ�� = 1������
        else
        {
            if ((ci->data_b[1] < 0x0D) && (ci->data_b[2] < 0x05))
            {
                if ((ci->data_b[1] > 0x09) && (ci->data_b[2] > 0x03)) // Canes, Rods, Wands become rare faster  ���ȡ�������ħ��Խ��Խϡ�� 
                    break;
                price = pmt_weapon_bb[ci->data_b[1]][ci->data_b[2]]->atp_max + ci->data_b[3];
                price *= price;
                price_calc = (float)price;
                switch (ci->data_b[1])
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
                if (ci->data_b[6])
                    percent_add += (char)ci->data_b[7];
                if (ci->data_b[8])
                    percent_add += (char)ci->data_b[9];
                if (ci->data_b[10])
                    percent_add += (char)ci->data_b[11];

                if (percent_add != 0)
                {
                    percent_calc = price_calc;
                    percent_calc /= 300.0;
                    percent_calc *= percent_add;
                    price_calc += percent_calc;
                }
                price_calc /= 8.0;
                price = (int32_t)(price_calc);
                price += attrib[ci->data_b[4]];
            }
        }
        break;
    case 0x01:
        switch (ci->data_b[1])
        {
        case 0x01: // Armor װ��
            if (ci->data_b[2] < 0x18)
            {
                // Calculate the amount to boost because of slots...
                if (ci->data_b[5] > 4)
                    price = armor_prices[(ci->data_b[2] * 5) + 4];
                else
                    price = armor_prices[(ci->data_b[2] * 5) + ci->data_b[5]];
                price -= armor_prices[(ci->data_b[2] * 5)];
                if (ci->data_b[6] > pmt_armor_bb[ci->data_b[2]]->dfp_range)
                    variation = 0;
                else
                    variation = ci->data_b[6];
                if (ci->data_b[8] <= pmt_armor_bb[ci->data_b[2]]->dfp_range)
                    variation += ci->data_b[8];
                price += equip_prices[1][1][ci->data_b[2]][variation];
            }
            break;
        case 0x02: // Shield ����
            if (ci->data_b[2] < 0x15)
            {
                if (ci->data_b[6] > pmt_shield_bb[ci->data_b[2]]->dfp_range)
                    variation = 0;
                else
                    variation = ci->data_b[6];
                if (ci->data_b[8] <= pmt_shield_bb[ci->data_b[2]]->evp_range)
                    variation += ci->data_b[8];
                price = equip_prices[1][2][ci->data_b[2]][variation];
            }
            break;
        case 0x03: // Units ���
            if (ci->data_b[2] < 0x40)
                price = unit_prices[ci->data_b[2]];
            break;
        }
        break;
    case 0x03:
        // Tool ����
        if (ci->data_b[1] == 0x02) // Technique ħ���Ƽ�
        {
            if (ci->data_b[4] < 0x13)
                price = ((int32_t)(ci->data_b[2] + 1) * tech_prices[ci->data_b[4]]) / 100L;
        }
        else
        {
            compare_item = 0;
            memcpy(&compare_item, &ci->data_b[0], 3);
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
    //ITEM_LOG("���ռ۸� = %u", price);
    return (uint32_t)price;
}

sitem_t create_bb_shop_item(uint8_t �Ѷ�, uint8_t ��Ʒ����) {
    static const uint8_t max_percentages[4] = { 20, 35, 45, 50 };
    static const uint8_t max_quantity[4] = { 1,  1,  2,  2 };
    static const uint8_t max_tech_level[4] = { 8, 15, 23, 30 };
    static const uint8_t max_anti_level[4] = { 2,  4,  6,  7 };

    sitem_t item = { 0 };
    item.data1b[0] = ��Ʒ����;
    while (item.data1b[0] == 2) {
        item.data1b[0] = rand() % 3;
    }
    //[2022��09��07�� 05:24:17:531] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ɨ���ǹ 00080106, 07000000, 00000000, 00000000
    //[2022��09��07�� 05:24:17:543] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: �⽣     00010009, 0A00030E, 040A0000, 00000000
    //[2022��09��07�� 05:24:17:556] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ذ��     00030004, 06000102, 00000000, 00000000
    //[2022��09��07�� 05:24:17:568] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ɢ��ǹ   00090006, 05000000, 00000000, 00000000
    //[2022��09��07�� 05:24:17:579] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ��ǹ     00060003, 0300000E, 010E0000, 00000000
    //[2022��09��07�� 05:24:17:593] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: �ѻ���ǹ 00070107, 04000111, 00000000, 00000000
    //[2022��09��07�� 05:24:17:605] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ����     000A0003, 08000006, 03090000, 00000000
    //[2022��09��07�� 05:24:17:616] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ħ��     000C0005, 0900040A, 00000000, 00000000
    //[2022��09��07�� 05:24:17:629] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ����     000B0009, 0A000201, 04090000, 00000000
    //[2022��09��07�� 05:24:17:641] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ���ܹ�ǹ 00060101, 01000003, 00000000, 00000000
    //( 00000000 )   DE 00 60 00 00 00 00 00  B6 2C 7F 03 01 0A 00 00    ..`......,......
    //( 00000010 )   00 08 01 06 07 00 00 00  00 00 00 00 13 00 00 00    ................
    //( 00000020 )   00 00 00 00 00 01 00 09  0A 00 03 0E 04 0A 00 00    .......      ........
    //( 00000030 )   14 00 00 00 00 00 00 00  00 03 00 04 06 00 01 02    ................
    //( 00000040 )   00 00 00 00 15 00 00 00  00 00 00 00 00 09 00 06    .............        ..
    //( 00000050 )   05 00 00 00 00 00 00 00  16 00 00 00 00 00 00 00    ................
    //( 00000060 )   00 06 00 03 03 00 00 0E  01 0E 00 00 17 00 00 00    ................
    //( 00000070 )   00 00 00 00 00 07 01 07  04 00 01 11 00 00 00 00    ................
    //( 00000080 )   18 00 00 00 00 00 00 00  00 0A 00 03 08 00 00 06    ................
    //( 00000090 )   03 09 00 00 19 00 00 00  00 00 00 00 00 0C 00 05    .    ..............
    //( 000000A0 )   09 00 04 0A 00 00 00 00  1A 00 00 00 00 00 00 00         ...............
    //( 000000B0 )   00 0B 00 09 0A 00 02 01  04 09 00 00 1B 00 00 00    ...  .....   ......
    //( 000000C0 )   00 00 00 00 00 06 01 01  01 00 00 03 00 00 00 00    ................
    //( 000000D0 )   1C 00 00 00 00 00 00 00  00 00 00 00 00 00          ..............
        /* ������Ʒ���� */
    switch (item.data1b[0]) {
    case 0x00: { // ����
        item.data1b[1] = (rand() % 12) + 1;
        if (item.data1b[1] > 0x09) {
            item.data1b[2] = �Ѷ�;
        }
        else {
            item.data1b[2] = (rand() & 1) + �Ѷ�;
        }

        item.data1b[3] = rand() % 11;
        item.data1b[4] = rand() % 11;

        int num_percentages = 0;
        for (uint8_t x = 0; (x < 5) && (num_percentages < 3); x++) {
            if ((rand() % 4) == 1) {
                item.data1b[(num_percentages * 2) + 6] = x;
                item.data1b[(num_percentages * 2) + 7] = rand() % (max_percentages[�Ѷ�] + 1);
                num_percentages++;
            }
        }
        break;
    }
             //[2022��09��07�� 05:23:14:228] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: Knight/Power 01030000, 000000FC, 00000000, 00001500
             //[2022��09��07�� 05:23:14:241] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: Soul Frame   01010400, 00030000, 00000000, 00000000
             //[2022��09��07�� 05:23:14:253] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: Barrier      01020000, 0000FE00, 00040000, 00000400
             //[2022��09��07�� 05:23:14:265] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: װ��         01010100, 00020000, 00000000, 00000000
             //[2022��09��07�� 05:23:14:279] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: Cross Armor  01010500, 00000000, 00000000, 00000000
             //[2022��09��07�� 05:23:14:291] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: Barrier      01020000, 0000FC00, 00FC0000, 00000300
             //[2022��09��07�� 05:23:14:303] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: Barrier      01020000, 00000000, 00030000, 00000300
             //[2022��09��07�� 05:23:14:315] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ����װ��     01010200, 00030000, 00000000, 00000000
             //[2022��09��07�� 05:23:14:327] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: Knight/Power 01030000, 000000FD, 00000000, 00001300
             //[2022��09��07�� 05:23:14:339] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: Knight/Power 01030000, 00000000, 00000000, 00002100
             //( 00000000 )   DE 00 60 00 00 00 00 00  B6 2C 7F 03 02 0A 00 00    ..`......,......
             //( 00000010 )   01 03 00 00 00 00 00 FC  00 00 00 00 00 00 00 00    ................
             //( 00000020 )   00 00 15 00 01 01 04 00  00 03 00 00 00 00 00 00    ................
             //( 00000030 )   01 00 00 00 00 00 00 00  01 02 00 00 00 00 FE 00    ................
             //( 00000040 )   00 04 00 00 02 00 00 00  00 00 04 00 01 01 01 00    ................
             //( 00000050 )   00 02 00 00 00 00 00 00  03 00 00 00 00 00 00 00    ................
             //( 00000060 )   01 01 05 00 00 00 00 00  00 00 00 00 04 00 00 00    ................
             //( 00000070 )   00 00 00 00 01 02 00 00  00 00 FC 00 00 FC 00 00    ................
             //( 00000080 )   05 00 00 00 00 00 03 00  01 02 00 00 00 00 00 00    ................
             //( 00000090 )   00 03 00 00 06 00 00 00  00 00 03 00 01 01 02 00    ................
             //( 000000A0 )   00 03 00 00 00 00 00 00  07 00 00 00 00 00 00 00    ................
             //( 000000B0 )   01 03 00 00 00 00 00 FD  00 00 00 00 08 00 00 00    ................
             //( 000000C0 )   00 00 13 00 01 03 00 00  00 00 00 00 00 00 00 00    ................
             //( 000000D0 )   09 00 00 00 00 00 21 00  00 00 00 00 00 00               .....!.......
             //( 00000000 )   10 00 62 00 00 00 00 00  B5 02 FF FF 00 00 00 00    ..b.............
    case 0x01: // װ��
        item.data1b[1] = 0;
        while (item.data1b[1] == 0) {
            item.data1b[1] = rand() & 3;
        }
        switch (item.data1b[1]) {
        case 0x01://����
            item.data1b[2] = (rand() % 6) + (�Ѷ� * 6);
            item.data1b[5] = rand() % 5;
            break;
        case 0x02://����
            item.costb[2] = (rand() % 6) + (�Ѷ� * 5);//TODO �۸�Ӹ�����
            item.data1b[6] = (rand() % 9) - 4;
            item.data1b[9] = (rand() % 9) - 4;
            break;
        case 0x03://���
            item.costb[2] = rand() % 0x3B;
            item.data1b[7] = (rand() % 5) - 4;
            break;
        }
        break;
        //[2022��09��07�� 05:23:23:995] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ������ҩ     030B0000, 00000000, 00000000, 00000000
        //[2022��09��07�� 05:23:24:007] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ������       03070000, 00000000, 00000000, 00000000
        //[2022��09��07�� 05:23:24:018] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: TP ҩ        030B0400, 00000000, 00000000, 00000000
        //[2022��09��07�� 05:23:24:030] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ����:Lv.2    03020100, 09000000, 00000000, 00000000
        //[2022��09��07�� 05:23:24:041] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ����̽����   03080000, 00010000, 00000000, 00000000
        //[2022��09��07�� 05:23:24:052] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ������       03070000, 00010000, 00000000, 00000000
        //[2022��09��07�� 05:23:24:064] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ��֮��       03040000, 00010000, 00000000, 00000000
        //[2022��09��07�� 05:23:24:075] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ����̽����   03080000, 00000000, 00000000, 00000000
        //[2022��09��07�� 05:23:24:086] ��Ʒ(1177): ����Ʒ�ֽ�����Ϊ: ������ż     03090000, 00000000, 00000000, 00000000
        //( 00000000 )   CA 00 60 00 00 00 00 00  B6 2C 7F 03 00 09 00 00    ..`......,...        ..
        //( 00000010 )   03 0B 00 00 00 00 00 00  00 00 00 00 0A 00 00 00    ................
        //( 00000020 )   00 00 00 00 03 07 00 00  00 00 00 00 00 00 00 00    ................
        //( 00000030 )   0B 00 00 00 00 00 00 00  03 0B 04 00 00 00 00 00    ................
        //( 00000040 )   00 00 00 00 0C 00 00 00  00 00 00 00 03 02 01 00    ................
        //( 00000050 )   09 00 00 00 00 00 00 00  0D 00 00 00 00 00 00 00         ...............
        //( 00000060 )   03 08 00 00 00 01 00 00  00 00 00 00 0E 00 00 00    ................
        //( 00000070 )   00 00 00 00 03 07 00 00  00 01 00 00 00 00 00 00    ................
        //( 00000080 )   0F 00 00 00 00 00 00 00  03 04 00 00 00 01 00 00    ................
        //( 00000090 )   00 00 00 00 10 00 00 00  00 00 00 00 03 08 00 00    ................
        //( 000000A0 )   00 00 00 00 00 00 00 00  11 00 00 00 00 00 00 00    ................
        //( 000000B0 )   03 09 00 00 00 00 00 00  00 00 00 00 12 00 00 00    .    ..............
        //( 000000C0 )   00 00 00 00 00 00 00 00  00 00                      ..........
        //( 00000000 )   10 00 62 00 00 00 00 00  B5 02 FF FF 01 00 00 00    ..b.............
    case 0x03: // ҩƷ����
        item.data1b[1] = rand() % 12;
        switch (item.data1b[1]) {
        case 0x00:
        case 0x01:
            if (�Ѷ� == 0) {
                item.data1b[2] = 0;
            }
            else if (�Ѷ� == 1) {
                item.data1b[2] = rand() % 2;
            }
            else if (�Ѷ� == 2) {
                item.data1b[2] = (rand() % 2) + 1;
            }
            else if (�Ѷ� == 3) {
                item.data1b[2] = 2;
            }
            break;

        case 0x06:
            item.data1b[2] = rand() % 2;
            break;

        case 0x0A:
            item.data1b[2] = rand() % 3;
            break;

        case 0x0B:
            item.data1b[2] = rand() % 7;
            break;
        }

        switch (item.data1b[1]) {
        case 0x02:
            item.data1b[4] = rand() % 19;
            switch (item.data1b[4]) {
            case 14:
            case 17:
                item.data1b[2] = 0; // reverser & ryuker always level 1 
                break;
            case 16:
                item.data1b[2] = rand() % max_anti_level[�Ѷ�];
                break;
            default:
                item.data1b[2] = rand() % max_tech_level[�Ѷ�];
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
            item.data1b[5] = rand() % (max_quantity[�Ѷ�] + 1);
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

	shop_checksum = calculate_checksum(&shops[0], 7000 * sizeof(shop_data_t));

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