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

/* 36�ֽ� ��ɫ��Ϣ���ݽṹ */
typedef struct psocn_disp_char {
    psocn_pl_stats_t stats;
    uint16_t unknown_a1;
    float unknown_a2;
    float unknown_a3;
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
} PACKED psocn_disp_char_t;

/* 108 �ֽ�*/
typedef struct psocn_dress_data {
    char gc_string[16];
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
        }PACKED;
        uint16_t all[0x0C];
    }PACKED;
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
    inventory_t inv;//844
    psocn_disp_char_t disp; //36
    psocn_dress_data_t dress_data;//80
    psocn_bb_char_name_t name;//24
    uint16_t padding;//2
    uint16_t unknown_a3; //2
    uint32_t play_time; //4
    uint8_t config[0xE8]; //232
    techniques_t technique_levels_v1; //20 /* Ĭ�� FF Ϊ�� ʵ������V1 �汾�ķ��� */
} PACKED psocn_bb_char_t;

/* v1v2v3pc ������ݽṹ 1052�ֽ� */
typedef struct psocn_v1v2v3pc_char {
    inventory_t inv;
    psocn_disp_char_t disp;
    psocn_dress_data_t dress_data;
    uint8_t config[0x48];
    techniques_t technique_levels_v1;
} PACKED psocn_v1v2v3pc_char_t;

/* BB��λ�������ݽṹ 410 �ֽ�*/
typedef struct psocn_bb_key_config {
    uint8_t keyboard_config[0x016C];// 0114
    uint8_t joystick_config[0x0038];// 0280
} PACKED bb_key_config_t;

#define BB_GUILD_PRIV_LEVEL_MASTER 0x00000040
#define BB_GUILD_PRIV_LEVEL_ADMIN  0x00000030
#define BB_GUILD_PRIV_LEVEL_MEMBER 0x00000000

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
        }PACKED;
        uint8_t guild_reward[8];
    }PACKED;
} PACKED bb_guild_t;

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

typedef struct psocn_bb_mode_quest_data {
    union {
        /* TODO ȷ��ÿһ�������Ӧ������ 0 ���� 1 */
        uint32_t part[PSOCN_STLENGTH_BB_DB_MODE_QUEST_DATA];
        uint8_t all[PSOCN_DATALENGTH_BB_DB_MODE_QUEST_DATA];
    }PACKED;
} PACKED psocn_bb_mode_quest_data_t;

/* BB ������ɫ���� 0x00E7 TODO �������ݰ�ͷ 8 �ֽ� ��С 14744*/
typedef struct psocn_bb_full_char {
    psocn_bb_char_t character;                                               // ������ݱ�               OK
    //char guildcard_string1[16];                                            // δ��ɱ���
    ////////////δ����
    /* 04DC */ uint32_t unknown_a1;
    /* 04E0 */ uint32_t creation_timestamp;
    /* 04E4 */ uint32_t signature; // == 0xA205B064 (see SaveFileFormats.hh)
    /* 04E8 */ uint32_t play_time_seconds;
    ////////////
    uint32_t option_flags;                                                   // account
    uint8_t quest_data1[PSOCN_STLENGTH_BB_DB_QUEST_DATA1];                   // ����������ݱ�1          OK
    psocn_bank_t bank;                                                       // ����������ݱ�           OK
    psocn_bb_guild_card_t gc;                                                // ���GC���ݱ���         OK
    uint32_t unk2;                                                           // δ��ɱ���
    uint8_t symbol_chats[PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS];                 // ѡ�����ݱ�
    uint8_t shortcuts[PSOCN_STLENGTH_BB_DB_SHORTCUTS];                       // ѡ�����ݱ�
    uint16_t autoreply[0x00AC];                                              // ������ݱ�
    uint16_t infoboard[0x00AC];
    battle_records_t b_records;                                              // ��Ҷ�ս���ݱ�           TODO
    uint8_t unk3[4];
    bb_challenge_records_t c_records;                                        // �����ս���ݱ�           OK
    uint8_t tech_menu[PSOCN_STLENGTH_BB_DB_TECH_MENU];                       // ��ҷ��������ݱ�         OK
    uint8_t unk4[0x002C];                                                    // δ��ɱ���
    psocn_bb_mode_quest_data_t mode_quest_data;                              // ����������ݱ�2
    uint8_t unk1[276];                                                       // 276 - 264 = 12
    bb_key_config_t key_cfg;                                                 // ѡ�����ݱ�               OK
    bb_guild_t guild_data;                                                   // GUILD���ݱ�              OK
} PACKED psocn_bb_full_char_t;

static int sdasd = sizeof(psocn_bb_full_char_t);

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
    psocn_bb_mode_quest_data_t mode_quest_data;
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
} PACKED psocn_mode_char_t;

typedef enum {
    TECH_DISK_MODE_ALLOW = 0,
    TECH_DISK_MODE_FORBID_ALL = 1,
    TECH_DISK_MODE_LIMIT_LEVEL = 2
} TechDiskMode;

typedef enum {
    WEAPON_AND_ARMOR_MODE_ALLOW = 0,
    WEAPON_AND_ARMOR_MODE_CLEAR_AND_ALLOW = 1,
    WEAPON_AND_ARMOR_MODE_FORBID_ALL = 2,
    WEAPON_AND_ARMOR_MODE_FORBID_RARES = 3
} WeaponAndArmorMode;

typedef enum {
    MAG_MODE_ALLOW = 0,
    MAG_MODE_FORBID_ALL = 1
} MagMode;

typedef enum {
    TOOL_MODE_ALLOW = 0,
    TOOL_MODE_CLEAR_AND_ALLOW = 1,
    TOOL_MODE_FORBID_ALL = 2
} ToolMode;

typedef enum {
    TRAP_MODE_DEFAULT = 0,
    TRAP_MODE_ALL_PLAYERS = 1
} TrapMode;

typedef enum {
    MESETA_MODE_ALLOW = 0,
    MESETA_MODE_FORBID_ALL = 1,
    MESETA_MODE_CLEAR_AND_ALLOW = 2
} MesetaMode;

typedef struct BattleRules {
    // Set by quest opcode F812, but values are remapped.
    //   F812 00 => FORBID_ALL
    //   F812 01 => ALLOW
    //   F812 02 => LIMIT_LEVEL
    /* 00 */ uint8_t tech_disk_mode;
    // Set by quest opcode F813, but values are remapped.
    //   F813 00 => FORBID_ALL
    //   F813 01 => ALLOW
    //   F813 02 => CLEAR_AND_ALLOW
    //   F813 03 => FORBID_RARES
    /* 01 */ uint8_t weapon_and_armor_mode;
    // Set by quest opcode F814, but values are remapped.
    //   F814 00 => FORBID_ALL
    //   F814 01 => ALLOW
    /* 02 */ uint8_t mag_mode;
    // Set by quest opcode F815, but values are remapped.
    //   F815 00 => FORBID_ALL
    //   F815 01 => ALLOW
    //   F815 02 => CLEAR_AND_ALLOW
    /* 03 */ uint8_t tool_mode;
    // Set by quest opcode F816. Values are not remapped.
    //   F816 00 => DEFAULT
    //   F816 01 => ALL_PLAYERS
    /* 04 */ uint8_t trap_mode;
    // Set by quest opcode F817. Value appears to be unused in all PSO versions.
    /* 05 */ uint8_t unused_F817;
    // Set by quest opcode F818, but values are remapped.
    //   F818 00 => 01
    //   F818 01 => 00
    //   F818 02 => 02
    // TODO: Define an enum class for this field.
    /* 06 */ uint8_t respawn_mode;
    // Set by quest opcode F819.
    /* 07 */ uint8_t replace_char;
    // Set by quest opcode F81A, but value is inverted.
    /* 08 */ uint8_t drop_weapon;
    // Set by quest opcode F81B.
    /* 09 */ uint8_t is_teams;
    // Set by quest opcode F852.
    /* 0A */ uint8_t hide_target_reticle;
    // Set by quest opcode F81E. Values are not remapped.
    //   F81E 00 => ALLOW
    //   F81E 01 => FORBID_ALL
    //   F81E 02 => CLEAR_AND_ALLOW
    /* 0B */ uint8_t meseta_mode;
    // Set by quest opcode F81D.
    /* 0C */ uint8_t death_level_up;
    // Set by quest opcode F851. The trap type is remapped:
    //   F851 00 XX => set count to XX for trap type 00
    //   F851 01 XX => set count to XX for trap type 02
    //   F851 02 XX => set count to XX for trap type 03
    //   F851 03 XX => set count to XX for trap type 01
    /* 0D */ uint8_t trap_counts[4];
    // Set by quest opcode F85E.
    /* 11 */ uint8_t enable_sonar;
    // Set by quest opcode F85F.
    /* 12 */ uint8_t sonar_count;
    // Set by quest opcode F89E.
    /* 13 */ uint8_t forbid_scape_dolls;
    // This value does not appear to be set by any quest opcode.
    /* 14 */ uint32_t unknown_a1;
    // Set by quest opcode F86F.
    /* 18 */ uint32_t lives;
    // Set by quest opcode F870.
    /* 1C */ uint32_t max_tech_level;
    // Set by quest opcode F871.
    /* 20 */ uint32_t char_level;
    // Set by quest opcode F872.
    /* 24 */ uint32_t time_limit;
    // Set by quest opcode F8A8.
    /* 28 */ uint16_t death_tech_level_up;
    /* 2A */ uint8_t unused[2];
    // Set by quest opcode F86B.
    /* 2C */ uint32_t box_drop_area;
    /* 30 */
} PACKED BattleRules_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

// ��ɫ��ɫID����
const enum SectionIDs {
    SID_Viridia = 0, //����
    SID_Greennill, //����
    SID_Skyly, //����
    SID_Bluefull, //����
    SID_Pinkal, //����
    SID_Purplenum, //�ۺ�
    SID_Redria, //���
    SID_Oran, //�Ȼ�
    SID_Yellowboze, //���
    SID_Whitill, //���
    SID_MAX //��ɫID�����ֵ
};

typedef struct section_ids_ {
    enum SectionIDs id;
    char cn_name[8];
    char en_name[12];
    char color_id[2];
} section_ids_t;

static section_ids_t section_ids[] = {
    {SID_Viridia,   "����",   "Viridia",   "0"},
    {SID_Greennill, "����",   "Greennill", "2"},
    {SID_Skyly,     "����",   "Skyly",     "3"},
    {SID_Bluefull,  "����",   "Bluefull",  "1"},
    {SID_Pinkal,    "����",   "Pinkal",    "9"},
    {SID_Purplenum, "�ۺ�",   "Purplenum", "8"},
    {SID_Redria,    "���",   "Redria",    "4"},
    {SID_Oran,      "�Ȼ�",   "Oran",      "a"},
    {SID_Yellowboze,"���",   "Yellowboze","6"},
    {SID_Whitill,   "���",   "Whitill",   "7"}
};

// ְҵ����
const enum Character_Classes {
    CLASS_HUMAR, //����������
    CLASS_HUNEWEARL, //������Ů����
    CLASS_HUCAST, //����������
    CLASS_RAMAR, //������ǹ��
    CLASS_RACAST, //��е��ǹ��
    CLASS_RACASEAL, //��еŮǹ��
    CLASS_FOMARL, //����Ůħ��ʦ
    CLASS_FONEWM, //��������ħ��ʦ
    CLASS_FONEWEARL, //������Ůħ��ʦ
    CLASS_HUCASEAL, //��еŮ����
    CLASS_FOMAR, //������ħ��ʦ
    CLASS_RAMARL, //����Ůǹ��
    CLASS_MAX, // ���ְҵ����
    CLASS_FULL_CHAR = 12
};

typedef struct full_class {
    uint32_t class_code;
    char cn_name[24];
    char en_name[16];
    char class_file[64];//unused
} full_class_t;

static full_class_t pso_class[] = {
    {CLASS_HUMAR,     "����������",    "HUmar",     "0_Ĭ��_����������.nsc"  },
    {CLASS_HUNEWEARL, "������Ů����",  "HUnewearl", "1_Ĭ��_������Ů����.nsc"},
    {CLASS_HUCAST,    "��е������",  "HUcast",    "2_Ĭ��_��е������.nsc"},
    {CLASS_RAMAR,     "������ǹ��",    "RAmar",     "3_Ĭ��_������ǹ��.nsc"  },
    {CLASS_RACAST,    "��е��ǹ��",  "RAcast",    "4_Ĭ��_��е��ǹ��.nsc"},
    {CLASS_RACASEAL,  "��еŮǹ��",  "RAcaseal",  "5_Ĭ��_��еŮǹ��.nsc"},
    {CLASS_FOMARL,    "����Ůħ��ʦ",    "FOmarl",    "6_Ĭ��_����Ůħ��ʦ.nsc"  },
    {CLASS_FONEWM,    "��������ħ��ʦ",  "FOnewm",    "7_Ĭ��_��������ħ��ʦ.nsc"},
    {CLASS_FONEWEARL, "������Ůħ��ʦ",  "FOnewearl", "8_Ĭ��_������Ůħ��ʦ.nsc"},
    {CLASS_HUCASEAL,  "��еŮ����",  "HUcaseal",  "9_Ĭ��_��еŮ����.nsc"},
    {CLASS_FOMAR,     "������ħ��ʦ",    "FOmar",     "10_Ĭ��_������ħ��ʦ.nsc" },
    {CLASS_RAMARL,    "����Ůǹ��",    "RAmarl",    "11_Ĭ��_����Ůǹ��.nsc" },
    {CLASS_FULL_CHAR, "������ɫ",      "Full_char", "Ĭ��_������ɫ.bin"      },
};

inline full_class_t* get_pso_class_describe(uint8_t ch_class) {
    if (ch_class < MAX_PLAYER_CLASS_BB)
        return &pso_class[ch_class];
    else
        return NULL;
}

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
