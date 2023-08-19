/*
    �λ�֮���й�

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

#ifndef PSOCN_CHARACTERS_H
#define PSOCN_CHARACTERS_H

#include <time.h>
#include <stdint.h>

#include "pso_struct_length.h"
#include "pso_struct_item.h"
#include "pso_struct_records.h"
#include "pso_struct_level_stats.h"
#include "pso_struct_techniques.h"

#define MAX_PLAYER_BANK_ITEMS           200
#define MAX_PLAYER_INV_ITEMS            30
#define MAX_PLAYER_TECHNIQUES           19
#define MAX_TRADE_ITEMS                 200

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

#pragma warning (disable:4200 4201)

typedef struct dress_flag {
    uint32_t guildcard;
    time_t flagtime;
} PACKED dress_flag_t;

/* ��ɫ��Ϣ���ݽṹ */
typedef struct psocn_disp_char {
    psocn_pl_stats_t stats;
    //TODO ����ʲô����
    union unkonw
    {
        struct unkonw1
        {

            uint8_t opt_flag1;
            uint8_t opt_flag2;
            uint8_t opt_flag3;
            uint8_t opt_flag4;
            uint8_t opt_flag5;
            uint8_t opt_flag6;
            uint8_t opt_flag7;
            uint8_t opt_flag8;
            uint8_t opt_flag9;
            uint8_t opt_flag10;
        };

        struct unkonw2
        {
            uint16_t unknown_a1;
            float unknown_a2;
            float unknown_a3;
        };

    };
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
} PACKED psocn_disp_char_t;

/* 10�ֽ� �ַ��� GC ���ո� 16 �ֽ� */
typedef struct psocn_guildcard_string {
    union guildcard_ {
        /* ���������ݿ��ȡ �ֽ� */
        struct {
            uint16_t str[5];
            uint16_t slot1;
            uint16_t slot2;
            uint16_t slot3;
        };

        char string[16];
    };
} PACKED psocn_guildcard_string_t;

typedef struct psocn_dress_data {
    psocn_guildcard_string_t guildcard_str;

    uint8_t unk1[8]; // 0x382-0x38F; 898 - 911 14 ����  // Same as E5 unknown2 ��E5ָ��� δ֪���� 2 һ��

    //���������ɫ
    uint8_t name_color_b; // ARGB8888
    uint8_t name_color_g;
    uint8_t name_color_r;
    uint8_t name_color_transparency;

    //Ƥ��ģ��
    uint16_t model;
    uint8_t unk3[10];
    uint32_t create_code;
    uint32_t name_color_checksum;

    uint8_t section;
    uint8_t ch_class;
    uint8_t v2flags;
    uint8_t version;
    uint32_t v1flags;
    uint16_t costume;
    uint16_t skin;
    uint16_t face;
    uint16_t head;
    uint16_t hair;
    uint16_t hair_r;
    uint16_t hair_g;
    uint16_t hair_b;
    //�����̬����
    float prop_x;
    float prop_y;
} PACKED psocn_dress_data_t;

/* BB �ͻ��� ������ƽṹ 24�ֽ� */
typedef struct psocn_bb_char_name {
    union bb_char_name {
        struct char_name_part_data {
            uint16_t name_tag;
            uint16_t name_tag2;
            uint16_t char_name[0x0A];
        };
        uint16_t all[0x0C];
    };
} PACKED psocn_bb_char_name_t;

/* ������֤��������Ԥ����ɫ���ݽṹ */
/* ����ָ��ͷ 124 �ֽ�*/
typedef struct psocn_bb_mini_char {
    uint32_t exp;
    uint32_t level;
    psocn_dress_data_t dress_data;
    psocn_bb_char_name_t name;
    uint8_t hw_info[0x08]; // 0x7C - 0x83
    uint32_t play_time;
} PACKED psocn_bb_mini_char_t;

/* ���ڷ��͸�������������ҵ����ݽṹ,��������������. */
/* BB ������ݽṹ 1244�ֽ� */
typedef struct psocn_bb_char {
    inventory_t inv;
    psocn_disp_char_t disp; //101
    psocn_dress_data_t dress_data;
    psocn_bb_char_name_t name;
    uint16_t padding;
    uint16_t unknown_a3; //4
    uint32_t play_time; //4
    uint8_t config[0xE8]; //232
    techniques_t tech; //20 /* Ĭ�� FF Ϊ��*/
} PACKED psocn_bb_char_t;

/* v1v2v3pc ������ݽṹ 1052�ֽ� */
typedef struct psocn_v1v2v3pc_char {
    inventory_t inv;
    psocn_disp_char_t disp;
    psocn_dress_data_t dress_data;
    uint8_t config[0x48];
    techniques_t tech;
} PACKED psocn_v1v2v3pc_char_t;

/* BB��λ�������ݽṹ 410 �ֽ�*/
typedef struct psocn_bb_key_config {
    uint8_t key_config[0x016C];           // 0114
    uint8_t joystick_config[0x0038];      // 0280
} PACKED bb_key_config_t;

/* BB�������ݽṹ TODO 2108�ֽ� �޷�����8���� ȱ8λ �ᵼ�������޷����� */
typedef struct psocn_bb_guild {
    uint32_t guild_owner_gc;               // ���ᴴʼ��
    uint32_t guild_id;                     // �������� 
    // ������Ϣ     8
    uint32_t guild_points_rank;
    uint32_t guild_points_rest;
    uint32_t guild_priv_level;             // ��Ա�ȼ�     4
    uint16_t guild_name[0x000E];           // ��������     28 guild_name_tag1 guild_name_tag2 guild_name[12]
    uint32_t guild_rank;                   // ��������     4
    uint8_t guild_flag[0x0800];            // ����ͼ��     2048
    // ���ά��     8�ֽ���
    union {
        struct guild_rewards {
            uint8_t guild_reward0;
            uint8_t guild_reward1;
            uint8_t guild_reward2;
            uint8_t guild_reward3;
            uint8_t guild_reward4;
            uint8_t guild_reward5;
            uint8_t guild_reward6;
            uint8_t guild_reward7;
        };
        uint8_t guild_reward[8];
    };
} PACKED bb_guild_t;

#define BB_GUILD_PRIV_LEVEL_MASTER 0x00000040
#define BB_GUILD_PRIV_LEVEL_ADMIN  0x00000030
#define BB_GUILD_PRIV_LEVEL_MEMBER 0x00000000

typedef struct psocn_bb_db_guild {
    bb_guild_t data;
} PACKED psocn_bb_db_guild_t;

// V3 GC ���� TODO 144
typedef struct psocn_v3_guild_card {
    uint32_t player_tag;
    uint32_t guildcard;
    char name[24];
    char guildcard_desc[108];
    uint8_t present; // 1 ��ʾ��GC����
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
} PACKED psocn_v3_guild_card_t;

// BB ��Ƭ����
// 19C0 - 19C7
typedef struct psocn_bb_guild_card {
    uint32_t guildcard;                 /* 4   32λ GC  */
    uint16_t name[24];                  /* 48  ������� */
    uint16_t guild_name[16];            /* 32  �������� */
    uint16_t guildcard_desc[88];        /* 176 ������� */
    uint8_t present;                    /* 1   ռλ�� 0x01 ��ʾ���� */
    uint8_t language;                   /* 1   ���� 0 -8 */
    uint8_t section;                    /* 1   ��ɫID */
    uint8_t char_class;                 /* 1   ����ְҵ */
} PACKED psocn_bb_guild_card_t;

/* BB ������ɫ���� 0x00E7 TODO �������ݰ�ͷ 8 �ֽ�*/
typedef struct psocn_bb_full_char {
    psocn_bb_char_t character;                                               // ������ݱ�               OK
    char guildcard_string[16];                                               // not saved
    uint32_t option_flags;                                                   // account
    uint8_t quest_data1[PSOCN_STLENGTH_BB_DB_QUEST_DATA1];                   // ����������ݱ�1          OK
    psocn_bank_t bank;                                                       // ����������ݱ�           OK
    psocn_bb_guild_card_t gc;                                                // ���GC���ݱ���         OK
    uint32_t unk2;                                                           // not saved
    uint8_t symbol_chats[PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS];                 // ѡ�����ݱ�
    uint8_t shortcuts[0x0A40];                                               // ѡ�����ݱ�
    uint16_t autoreply[0x00AC];                                              // ������ݱ�
    uint16_t infoboard[0x00AC];
    battle_records_t b_records;                                              // ��Ҷ�ս���ݱ�           TODO
    uint8_t unk3[4];
    bb_challenge_records_t c_records;                                        // �����ս���ݱ�           OK
    uint8_t tech_menu[PSOCN_STLENGTH_BB_DB_TECH_MENU];                       // ��ҷ��������ݱ�         OK
    uint8_t unk4[0x002C];                                                    // not saved
    uint8_t quest_data2[PSOCN_STLENGTH_BB_DB_QUEST_DATA2];                   // ����������ݱ�2
    uint8_t unk1[276];                                                       // 276 - 264 = 12
    bb_key_config_t key_cfg;                                                 // ѡ�����ݱ�               OK
    bb_guild_t guild_data;                                                   // GUILD���ݱ�              OK
} PACKED psocn_bb_full_char_t;

/* Ŀǰ�洢�����ݿ�Ľ�ɫ���ݽṹ. */
typedef struct psocn_bb_db_char {
    psocn_bb_char_t character;
    uint8_t quest_data1[PSOCN_STLENGTH_BB_DB_QUEST_DATA1];
    psocn_bank_t bank;
    uint16_t guildcard_desc[0x0058];//88
    uint16_t autoreply[0x00AC];//172
    uint16_t infoboard[0x00AC];//172
    bb_challenge_records_t c_records;
    uint8_t tech_menu[PSOCN_STLENGTH_BB_DB_TECH_MENU];
    uint8_t quest_data2[PSOCN_STLENGTH_BB_DB_QUEST_DATA2];
    battle_records_t b_records;
} PACKED psocn_bb_db_char_t;

typedef struct psocn_bb_default_char {
    psocn_bb_db_char_t char_class[MAX_PLAYER_CLASS_BB];
} PACKED psocn_bb_default_char_t;

typedef struct psocn_bb_mode_char {
    psocn_bb_char_t cdata[MAX_PLAYER_CLASS_BB][14];
} PACKED psocn_bb_mode_char_t;

// 11760 980

static int ds123a = sizeof(psocn_bb_mode_char_t);

// BB GC ���ݵ���ʵ���ļ� TODO 
typedef struct psocn_bb_guild_card_entry {
    psocn_bb_guild_card_t data;
    uint16_t comment[0x58];
    uint32_t padding;
} PACKED psocn_bb_guild_card_entry_t;

//BB GC �����ļ� TODO 
// 276 + 120 = 396 - 264 = 132     384 + 276 = 660 - 264 = 396
typedef struct bb_guildcard_data {
    uint8_t unk1[0x000C];//276 - 264 = 12
    psocn_bb_guild_card_t black_list[0x001E]; //30�� 264��С/��
    uint32_t black_gc_list[0x001E];//120
    psocn_bb_guild_card_entry_t entries[0x0069]; //105�� 444��С/��
    //uint8_t unk3[0x01BC];
} bb_guildcard_data_t; //54672

/* BB ����ѡ���ļ� TODO */
/* ������BB��ɫ�����з����������������,�洢�� ���ݿ� ��,
������ newserv �� .nsa �ļ�. */
typedef struct psocn_bb_db_opts {
    uint32_t black_gc_list[0x001E];//30 120�ֽ�             /* �������б� */
    bb_key_config_t key_cfg; //420                          /* ��λ�������� */
    uint32_t option_flags;
    uint8_t shortcuts[0x0A40];//2624
    uint8_t symbol_chats[PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS];//1248
    uint16_t guild_name[0x0010];//16
} PACKED psocn_bb_db_opts_t;

/* ģʽר�ý�ɫ���� inv ģʽ���� disp ģʽ��ɫ��ֵ dress_data ģʽר����� */
typedef union psocn_mode_char {
    psocn_v1v2v3pc_char_t nobb;
    psocn_bb_char_t bb;
    //inventory_t inv;
    //psocn_disp_char_t disp;
    //psocn_dress_data_t dress_data;
    //uint8_t techniques[0x14]; //20 /* Ĭ�� FF Ϊ��*/
} PACKED psocn_mode_char_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

// ��ɫ��ɫID����
const enum SectionIDs {
    SID_Viridia = 0, //����
    SID_Greennill, //����
    SID_Skyly, // ����
    SID_Bluefull, //����
    SID_Pinkal, //����
    SID_Purplenum, //�ۺ�
    SID_Redria, //���
    SID_Oran, //�Ȼ�
    SID_Yellowboze, //���
    SID_Whitill, //���
    SID_MAX //��ɫID�����ֵ
};

// ְҵ����
const enum Character_Classes {
    CLASS_HUMAR, //����������
    CLASS_HUNEWEARL, //������Ů����
    CLASS_HUCAST, //����������
    CLASS_RAMAR, //������ǹ��
    CLASS_RACAST, //��������ǹ��
    CLASS_RACASEAL, //������Ůǹ��
    CLASS_FOMARL, //����Ů��ʦ
    CLASS_FONEWM, //�������з�ʦ
    CLASS_FONEWEARL, //������Ů��ʦ
    CLASS_HUCASEAL, //������Ů����
    CLASS_FOMAR, //�����з�ʦ
    CLASS_RAMARL, //����Ůǹ��
    CLASS_MAX, // ���ְҵ����
    CLASS_FULL_CHAR = 12
};

typedef struct full_class {
    uint32_t class_code;
    char cn_name[24];
    char en_name[16];
    char class_file[64];
} full_class_t;

static full_class_t pso_class[] = {
    {CLASS_HUMAR,     "����������",    "HUmar",     "0_Ĭ��_����������.nsc"  },
    {CLASS_HUNEWEARL, "������Ů����",  "HUnewearl", "1_Ĭ��_������Ů����.nsc"},
    {CLASS_HUCAST,    "������������",  "HUcast",    "2_Ĭ��_������������.nsc"},
    {CLASS_RAMAR,     "������ǹ��",    "RAmar",     "3_Ĭ��_������ǹ��.nsc"  },
    {CLASS_RACAST,    "��������ǹ��",  "RAcast",    "4_Ĭ��_��������ǹ��.nsc"},
    {CLASS_RACASEAL,  "������Ůǹ��",  "RAcaseal",  "5_Ĭ��_������Ůǹ��.nsc"},
    {CLASS_FOMARL,    "����Ů��ʦ",    "FOmarl",    "6_Ĭ��_����Ů��ʦ.nsc"  },
    {CLASS_FONEWM,    "�������з�ʦ",  "FOnewm",    "7_Ĭ��_�������з�ʦ.nsc"},
    {CLASS_FONEWEARL, "������Ů��ʦ",  "FOnewearl", "8_Ĭ��_������Ů��ʦ.nsc"},
    {CLASS_HUCASEAL,  "������Ů����",  "HUcaseal",  "9_Ĭ��_������Ů����.nsc"},
    {CLASS_FOMAR,     "�����з�ʦ",    "FOmar",     "10_Ĭ��_�����з�ʦ.nsc" },
    {CLASS_RAMARL,    "����Ůǹ��",    "RAmarl",    "11_Ĭ��_����Ůǹ��.nsc" },
    {CLASS_FULL_CHAR, "������ɫ",      "Full_char", "Ĭ��_������ɫ.bin"      },
};

// ҩ�ﶨ��
const enum Character_Mats {
    ATP_MAT, //������ҩ
    MST_MAT, //����ҩ
    EVP_MAT, //����ҩ
    HP_MAT, //Ѫ��ҩ
    TP_MAT, //ħ��ҩ
    DFP_MAT, //����ҩ
    LCK_MAT, //����ҩ
    MATS_MAX // ҩ����
};
#endif /* !PSOCN_CHARACTERS_H */
