/*
    �λ�֮���й� �ṹ���ȱ�

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

#ifndef PSOCN_STRUCT_ITEM_H
#define PSOCN_STRUCT_ITEM_H

#include <stdint.h>

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

typedef struct item_data { // 0x14 bytes
    // ����һ��������Ʒ��ʽ�Ĳο�ע�ͣ����ڽ��Ͳ�ͬ������Ʒ�����ݸ�ʽ��
    // �����Ǹ�����Ʒ���ͼ�����Ӧ�����ݸ�ʽ��
    // ������Weapon��  ��00ZZZZGG SS00AABB AABBAABB 00000000
    // 
    // ���ߣ�Armor��   ��0101ZZ00 FFTTDDDD EEEE0000 00000000
    // 
    // ���ƣ�Shield��  ��0102ZZ00 FFTTDDDD EEEE0000 00000000
    // 
    // �����Unit��    ��0103ZZ00 FF0000RR RR000000 00000000
    // 
    // ��ţ�Mag��     ��02ZZLLWW HHHHIIII JJJJKKKK YYQQPPVV
    // 
    // ���ߣ�Tool��    ��03ZZZZFF 00CC0000 00000000 00000000
    // 
    // ÷������Meseta����04000000 00000000 00000000 MMMMMMMM
    // 
    // ��������ʽ�У�ÿ����д��ĸ����һ���ض������Ի���ֵ�ֶΡ�
    // ���磬
    // A ��ʾ�������ͣ����� S ����Ʒ�����ʾ�Զ������ƣ���
    // B ��ʾ�������������� S ����Ʒ�����ʾ�Զ������ƣ���
    // C ��ʾ�ѵ���С�����ڹ�������Ʒ����
    // D ��ʾ�����ӳɣ�
    // E ��ʾ���ܼӳɣ�
    // F ��ʾ��־λ��40 ��ʾ���ڣ����ڹ�������Ʒ���ԣ������Ʒ�ɶѵ�������ֶ�δʹ�ã���
    // G ��ʾ������ĥ�ȼ���
    // H ��ʾ��ŷ�������
    // I ��ʾ��Ź�������
    // J ��ʾ������ݶȣ�
    // K ��ʾ���������
    // L ��ʾ��ŵȼ���
    // M ��ʾ÷����������
    // P ��ʾ��ű�־λ��40 ��ʾ���ڣ�04 ��ʾӵ����� PB��02 ��ʾӵ���Ҳ� PB��01 ��ʾӵ������ PB����
    // Q ��ʾ���������
    // R ��ʾ��λ���η���С���򣩣�
    // S ��ʾ������־λ��80 ��ʾδ������40 ��ʾ���ڣ���
    // T ��ʾ���������
    // V ��ʾ�����ɫ��
    // W ��ʾ���ӱ�����
    // Y ��ʾ���ͬ����
    // Z ��ʾ��Ʒ ID��
    // ��ע�⣺PSO GC �汾�ڴ��������Ʒʱ�����ض� data2 �����ֽڽ�����byteswaps����
    // ��ʹ����Ʒʵ������С����
    // �⵼�����������汾�� PSO�������������汾�������ݡ�
    // ������Ҫ�ڽ��պͷ�������֮ǰ�ֶ��� data2 �����ֽڽ��������������⡣

    union {
        uint8_t data_b[12];//�ֽ�
        uint16_t data_w[6];//���ֽ�
        uint32_t data_l[3];//32λ��ֵ
    };

    uint32_t item_id;

    union {
        uint8_t data2_b[4];
        uint16_t data2_w[2];
        uint32_t data2_l;
    };
} PACKED item_t;

// PSO V2 stored some extra data in the character structs in a format that I'm
// sure Sega thought was very clever for backward compatibility, but for us is
// just plain annoying. Specifically, they used the third and fourth bytes of
// the InventoryItem struct to store some things not present in V1. The game
// stores arrays of bytes striped across these structures. In newserv, we call
// those fields extension_data. They contain:
//   items[0].extension_data1 through items[19].extension_data1:
//       Extended technique levels. The values in the v1_technique_levels array
//       only go up to 14 (tech level 15); if the player has a technique above
//       level 15, the corresponding extension_data1 field holds the remaining
//       levels (so a level 20 tech would have 14 in v1_technique_levels and 5
//       in the corresponding item's extension_data1 field).
//   items[0].extension_data2 through items[3].extension_data2:
//       The value known as unknown_a1 in the PSOGCCharacterFile::Character
//       struct. See SaveFileFormats.hh.
//   items[4].extension_data2 through items[7].extension_data2:
//       The timestamp when the character was last saved, in seconds since
//       January 1, 2000. Stored little-endian, so items[4] contains the LSB.
//   items[8].extension_data2 through items[12].extension_data2:
//       Number of power materials, mind materials, evade materials, def
//       materials, and luck materials (respectively) used by the player.
//   items[13].extension_data2 through items[15].extension_data2:
//       Unknown. These are not an array, but do appear to be related.

typedef struct psocn_iitem { // 0x1c bytes  
    uint16_t present; // 0x0001 = ��Ʒ��ʹ����, 0xFF00 = δʹ��
    uint16_t tech;  //�Ƿ����
    uint32_t flags;// 0x00000008 = ��װ��
    item_t data;
} PACKED iitem_t;

typedef struct psocn_inventory {
    uint8_t item_count;
    uint8_t hpmats_used;
    uint8_t tpmats_used;
    uint8_t language;
    iitem_t iitems[30];
} PACKED inventory_t;

typedef struct psocn_bitem { // 0x18 bytes
    item_t data;
    uint16_t amount;
    uint16_t show_flags; //�Ƿ���ʾ
} PACKED bitem_t;

typedef struct psocn_bank {
    uint32_t item_count;
    uint32_t meseta;
    bitem_t bitems[200];
} PACKED psocn_bank_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED


#endif // !PSOCN_STRUCT_ITEM_H