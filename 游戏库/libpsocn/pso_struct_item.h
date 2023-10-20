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

#define MAX_PLAYER_BANK_ITEMS           200
#define MAX_PLAYER_INV_ITEMS            30
#define MAX_PLAYER_TECHNIQUES           19
#define MAX_TRADE_ITEMS                 200
#define MAX_PLAYER_MESETA               999999

/* 20 �ֽ� */
typedef struct item_data {
    // ����һ��������Ʒ��ʽ�Ĳο�ע�ͣ����ڽ��Ͳ�ͬ������Ʒ�����ݸ�ʽ��
    // �����Ǹ�����Ʒ���ͼ�����Ӧ�����ݸ�ʽ��
    //                    0 1 2 3  4 5 6 7  8 9 A B  C D E F    
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
    // ��������Meseta����04000000 00000000 00000000 MMMMMMMM
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
    // M ��ʾ������������
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
        uint8_t datab[12];//���ֽ�
        uint16_t dataw[6];//���ֽ�
        uint32_t datal[3];//4�ֽ�
    }PACKED;
    uint32_t item_id;
    union {
        uint8_t data2b[4];
        uint16_t data2w[2];
        uint32_t data2l;
    }PACKED;
} PACKED item_t;

// ������Ʒ�����ṹ 20�ֽ�
typedef struct item_weapon {
    union data_l {
        struct data_b {
            //��Ʒ����
            uint8_t type;
            //������
            uint8_t subtype;
            //������Ʒ������
            uint8_t index;
            //��ĥ ���ֵò���� 35
            uint8_t polish;

            //special 0x00 - 0x28 / untekked 0x80 / presen 0x40 
            //���ӱ�ǩ ��ͬ��ǩֵ���
            uint8_t flags;
            //presen color type 0x00 - 0x0C 
            uint8_t subflags;
            // ����1 ����1�ٷֱ�
            // none native a beast machine dark hitt 0x00 - 0x05
            // 0x00 0x01   0x02    0x03    0x04 0x05
            uint8_t attrb1;
            uint8_t attrb1_add;

            // ����2 ����2�ٷֱ�
            // none native a beast machine dark hitt 0x00 - 0x05
            // 0x00 0x01   0x02    0x03    0x04 0x05
            uint8_t attrb2;
            uint8_t attrb2_add;
            // ����3 ����3�ٷֱ�
            // none native a beast machine dark hitt 0x00 - 0x05
            // 0x00 0x01   0x02    0x03    0x04 0x05
            uint8_t attrb3;
            uint8_t attrb3_add;
        }PACKED;
        uint32_t data_l[3];//4�ֽ�
        uint16_t data_w[6];//���ֽ�
    }PACKED;

    uint32_t item_id;              /* Set to 0xFFFFFFFF */

    /* �����ֽ�����δ�漰? */
    union data2_l {
        uint32_t amt;
        uint16_t data2_w[2];
        uint8_t data2_b[4];
    }PACKED;
} PACKED item_weapon_t;

/* ��Žṹ 20 �ֽ� */
typedef struct item_mag {
    uint8_t item_type; // "02" =P
    uint8_t mtype;
    uint8_t level;
    uint8_t photon_blasts;
    int16_t def;
    int16_t pow;
    int16_t dex;
    int16_t mind;
    uint32_t itemid;
    int8_t synchro;
    uint8_t IQ;
    uint8_t PBflags;
    uint8_t color;
} PACKED item_mag_t;

// BB ��������Ʒ�����ṹ 20�ֽ�
typedef struct item_mst {
    uint32_t type; //��Զ�� 0x00000004
    uint32_t unused1;
    uint32_t unused2;
    uint32_t item_id;              /* Set to 0xFFFFFFFF */
    uint32_t amt; //����������
} PACKED item_mst_t;

//PSO V2���ַ��ṹ�д洢��һЩ��������ݣ���ʽ����
//��Ȼ��������Ϊ�������Էǳ�����������������˵
//ֻ�Ǻܷ��ˡ�������˵������ʹ����
//InventoryItem�ṹ���洢V1�в����ڵ�һЩ��������Ϸ
//�洢����Щ�ṹ���������ֽ����顣��newserv�У����ǵ���
//��Щ�ֶ�extension_data�����ǰ�����
//items[0]��extension_data1��items[19]��extension_data1��
//��չ�ļ���ˮƽ��technique_levels_v1�����е�ֵ
//ֻ��14���������ȼ�15���������������ϼ���
//����15����Ӧ��extension_data1�ֶα���ʣ���
//�ȼ�����ˣ�һ��20���ļ�������technique_levels_v1��5����14��
//�ڶ�Ӧ��Ŀ��extension_data1�ֶ��У���
//items[0]��extension_data2��items[3]��extension_data2��
//PSOGCCharacterFile:��Character�ṹ�е�flags�ֶΣ�����
//SaveFileFormats.hh��ȡ��ϸ��Ϣ��
//items[4]��extension_data2��items[7]��extension_data2��
//�ϴα����ַ�ʱ��ʱ���������Ϊ��λ
//2000��1��1�ա��洢��Сendian������items[4]����LSB��
//items[8].exextension_data2��items[12].exextension _data2��
//�������ϡ�������ϡ���ܲ��ϡ�def������
//���Ϻ���ң��ֱ�ʹ�õ��������ϡ�
//items[13]��extension_data2��items[15]��extension_data2��
//δ֪����Щ����һ�����飬��������ȷʵ����صġ�
/* 28 �ֽ� */
typedef struct psocn_iitem {
    uint16_t present; // 0x0001 = ��Ʒ��ʹ����, 0xFF00 = δʹ��
  // See note above about these fields
    uint8_t extension_data1;  //�����ֽڴ洢����
    uint8_t extension_data2;  //�����ֽڴ洢��ҩ���� �ӱ����ĵڰ�λ��ʼ�洢
    uint32_t flags;// 0x00000008 = ��װ��
    item_t data;
} PACKED iitem_t;

/* 844 �ֽ� */
typedef struct psocn_inventory {
    uint8_t item_count;
    uint8_t hpmats_used;//HP ��ҩ����
    uint8_t tpmats_used;//TP ��ҩ����
    uint8_t language;
    iitem_t iitems[MAX_PLAYER_INV_ITEMS];
} PACKED inventory_t;

/* 24 �ֽ�*/
typedef struct psocn_bitem {
    item_t data;
    uint16_t amount;
    uint16_t show_flags; //�Ƿ���ʾ
} PACKED bitem_t;

/* 4808 �ֽ� */
typedef struct psocn_bank {
    uint32_t item_count;
    uint32_t meseta;
    bitem_t bitems[MAX_PLAYER_BANK_ITEMS];
} PACKED psocn_bank_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED


#endif // !PSOCN_STRUCT_ITEM_H