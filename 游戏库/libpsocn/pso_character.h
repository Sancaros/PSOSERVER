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

#define BB_CHARACTER_NAME_LENGTH 0x000C
#define PC_CHARACTER_NAME_LENGTH 0x0010

#define MAX_PLAYER_LEVEL                200
#define MAX_PLAYER_ITEMS                200
#define MAX_PLAYER_INV_ITEMS            30
#define MAX_PLAYER_CLASS_DC             9
#define MAX_PLAYER_CLASS_BB             12
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

typedef struct item_data { // 0x14 bytes
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

typedef struct psocn_iitem { // c bytes  
    uint16_t present;
    uint16_t tech;
    uint32_t flags;
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
    uint16_t flags;
} PACKED bitem_t;

typedef struct psocn_bank {
    uint32_t item_count;
    uint32_t meseta;
    bitem_t items[200];
} PACKED psocn_bank_t;

typedef struct psocn_pl_stats {
    uint16_t atp;//����
    uint16_t mst;//������
    uint16_t evp;//������
    uint16_t hp;//��ʼѪ��
    uint16_t dfp;//������
    uint16_t ata;
    uint16_t lck;//����
} PACKED psocn_pl_stats_t;

typedef struct psocn_lvl_stats {
    uint8_t atp;
    uint8_t mst;
    uint8_t evp;
    uint8_t hp;
    uint8_t dfp;
    uint8_t ata;
    uint8_t lck;
    uint8_t tp;
    uint32_t exp;
} PACKED psocn_lvl_stats_t;

/* Level-up information table from PlyLevelTbl.prs */
typedef struct bb_level_table {
    psocn_pl_stats_t start_stats[MAX_PLAYER_CLASS_BB];
    uint32_t start_stats_index[MAX_PLAYER_CLASS_BB]; /* Ӧ���ǵ� ��ʱ��֪ ���Ǻ������й�ϵ�� */
    psocn_lvl_stats_t levels[MAX_PLAYER_CLASS_BB][MAX_PLAYER_LEVEL];
} PACKED bb_level_table_t;

/* PSOv2 level-up information table from PlayerTable.prs */
typedef struct v2_level_table {
    psocn_lvl_stats_t levels[MAX_PLAYER_CLASS_DC][MAX_PLAYER_LEVEL];
} PACKED v2_level_table_t;

/* Player levelup data */
extern bb_level_table_t bb_char_stats;
extern v2_level_table_t v2_char_stats;

typedef struct psocn_dress_data {
    char guildcard_string[16];            /* GC�ŵ��ı���̬ */
    //uint32_t unk3[2];                   /* ������������ɫ�ṹһ�� */
    uint32_t dress_unk1;
    uint32_t dress_unk2;
    uint8_t name_color_b;
    uint8_t name_color_g;
    uint8_t name_color_r;
    uint8_t name_color_transparency;

    //Ƥ��ģ��
    uint16_t model;
    uint8_t dress_unk3[10];
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
    float prop_x;
    float prop_y;
    //uint16_t name[0x10];
} PACKED psocn_dress_data_t;

/* ��ɫ��Ϣ���ݽṹ */
typedef struct psocn_disp_char {
    psocn_pl_stats_t stats;
    uint8_t opt_flag[10];
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    psocn_dress_data_t dress_data;
} PACKED psocn_disp_char_t;

/* ������֤��������Ԥ����ɫ���ݽṹ */
typedef struct psocn_bb_mini_char {
    uint32_t exp;
    uint32_t level;
    psocn_dress_data_t dress_data;
    uint16_t name[0x0C];
    uint8_t hw_info[0x08]; // 0x7C - 0x83
    uint32_t play_time;
} PACKED psocn_bb_mini_char_t;

static int char_bb_minisize2 = sizeof(psocn_bb_mini_char_t);

/* ���ڷ��͸�������������ҵ����ݽṹ,��������������. */
typedef struct psocn_bb_char {
    psocn_disp_char_t disp; //101
    uint16_t name[BB_CHARACTER_NAME_LENGTH]; //24
    uint32_t play_time; //4
    uint32_t unknown_a3; //4
    uint8_t config[0xE8]; //232
    uint8_t techniques[0x14]; //20
} PACKED psocn_bb_char_t;

static int char_bb_size2 = sizeof(psocn_bb_char_t);

typedef struct psocn_v1v2v3pc_char {
    psocn_disp_char_t disp;
    uint8_t config[0x48];
    uint8_t techniques[0x14];
} PACKED psocn_v1v2v3pc_char_t;

static int char_v1v2v3pc_size = sizeof(psocn_v1v2v3pc_char_t);

/* BB��λ�������ݽṹ */
typedef struct psocn_bb_key_config {
    uint8_t key_config[0x016C];           // 0114
    uint8_t joystick_config[0x0038];      // 0280
} PACKED bb_key_config_t;

/* BB�������ݽṹ TODO*/
typedef struct psocn_bb_guild {
    uint32_t guildcard;                    // 02B8         4
    uint32_t guild_id;                     // 02BC         4 
    uint8_t guild_info[8];                 // ������Ϣ     8
    uint32_t guild_priv_level;             // ��Ա�ȼ�     4
    uint16_t guild_name[0x000E];           // 02CC         28
    uint32_t guild_rank;                   // ��������     4
    uint8_t guild_flag[0x0800];            // ����ͼ��     2048
    uint32_t guild_rewards[2];             // ���ά�� ���� ����Ƥ��  4 + 4
} PACKED bb_guild_t;

//   client_id  32       /        unk1  32       /          times[0]     /      times[1]
//0x00, 0x00, 0x06, 0x00, 0x03, 0x00, 0x01, 0x00, 0x07, 0x00, 0x04, 0x00, 0x02, 0x00, 0x08, 0x00,
//          times[2]     /        times[3]       /          times[4]     /      times[5]
//0x05, 0x00, 0x09, 0x00, 0x12, 0x00, 0x0F, 0x00, 0x10, 0x00, 0x11, 0x00, 0x0D, 0x00, 0x0A, 0x00,
//          times[6]     /        times[7]       /          times[8]     /    times_ep2[0] 32
//0x0B, 0x00, 0x0C, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//        times_ep2[1]   /      times_ep2[2]     /       times_ep2[3]    /    times_ep2[4]
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                             unk2[36]
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 16
// 
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 32
//          36           /         grave_unk4    /     grave_deaths      /     grave_deaths
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// grave_coords_time[0]  / grave_coords_time[1]  /  grave_coords_time[2] /  grave_coords_time[3]
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// grave_coords_time[4]  / grave_team[20]
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//                                               / grave_message[48]
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 8
// 
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 24
// 
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 40
//                                               / unk3[24]
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 8
// 
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 24
//  string[12]                                                           / unk4[24]
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 4
// 
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 20
//                       /      battle[0]        /      battle[1]        /      battle[2]        /       
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//      battle[3]        /      battle[4]        /      battle[5]        /      battle[6]        /
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
// 
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 16
//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  32

/* BB��սģʽ���ݽṹ TODO δ��� �ṹ��С348 ����344*/
typedef struct psocn_bb_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0x0158];
        //struct {
        //    uint32_t unk1;          /* Flip the words for dc/pc! */
        //    uint32_t times[9];
        //    uint32_t times_ep2[5];
        //    uint8_t unk2[0x24];     /* Probably corresponds to unk2 dc/pc */
        //    uint32_t grave_unk4;
        //    uint32_t grave_deaths;
        //    uint32_t grave_coords_time[5];
        //    char grave_team[20];
        //    char grave_message[48];
        //    uint8_t unk3[24];
        //    char string[12];
        //    uint8_t unk4[24];
        //    uint32_t battle[7];
        //} part;
    } c_rank;
} PACKED psocn_bb_c_rank_data_t;

static int bb_c_size = sizeof(psocn_bb_c_rank_data_t);

typedef struct psocn_v2_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0xB8];
        struct {
            uint32_t unk1;
            char string[0x0C];
            uint8_t unk2[0x24];
            uint16_t grave_unk4;
            uint16_t grave_deaths;
            uint32_t grave_coords_time[5];
            char grave_team[20];
            char grave_message[24];
            uint32_t times[9];
            uint32_t battle[7];
        } part;
    } c_rank;
} PACKED psocn_v2_c_rank_data_t;

static int v2_c_size = sizeof(psocn_v2_c_rank_data_t);

typedef struct psocn_pc_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0xF0];
        struct {
            uint32_t unk1;
            uint16_t string[0x0C];
            uint8_t unk2[0x24];
            uint16_t grave_unk4;
            uint16_t grave_deaths;
            uint32_t grave_coords_time[5];
            uint16_t grave_team[20];
            uint16_t grave_message[24];
            uint32_t times[9];
            uint32_t battle[7];
        } part;
    } c_rank;
} PACKED psocn_pc_c_rank_data_t;

static int pc_c_size = sizeof(psocn_pc_c_rank_data_t);

typedef struct psocn_v3_c_rank_data {
    uint32_t client_id;
    union {
        uint8_t all[0x0118];
        struct {
            uint32_t unk1;          /* Flip the words for dc/pc! */
            uint32_t times[9];
            uint32_t times_ep2[5];
            uint8_t unk2[0x24];     /* Probably corresponds to unk2 dc/pc */
            uint32_t grave_unk4;
            uint32_t grave_deaths;
            uint32_t grave_coords_time[5];
            char grave_team[20];
            char grave_message[48];
            uint8_t unk3[24];
            char string[12];
            uint8_t unk4[24];
            uint32_t battle[7];
        } part;
    } c_rank;
} PACKED psocn_v3_c_rank_data_t;

static int v3_c_size = sizeof(psocn_v3_c_rank_data_t);

// V3 GC ���� TODO 144
typedef struct psocn_v3_guild_card {
    uint32_t player_tag;
    uint32_t guildcard;
    char name[0x0018];
    char guildcard_desc[0x006C];
    uint8_t present; // 1 ��ʾ��GC����
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
} PACKED psocn_v3_guild_card_t;

// BB GC ���� TODO 264 + 120 = 384
typedef struct psocn_bb_guildcard {
    uint32_t guildcard;
    uint16_t name[0x0018]; //24 * 2 = 48
    uint16_t guild_name[0x0010]; // 32
    uint16_t guildcard_desc[0x0058];
    uint8_t present; // 1 ��ʾ��GC����
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
} PACKED psocn_bb_guildcard_t;

static int bb_c_gcsize = sizeof(psocn_bb_guildcard_t);

/* BB ������ɫ���� 0x00E7 TODO �������ݰ�ͷ 8 �ֽ�*/
typedef struct psocn_bb_full_char {
    inventory_t inv;                              // ������ݱ�
    psocn_bb_char_t character;                    // ������ݱ�

    uint8_t name3[0x0010];                        // not saved
    uint32_t option_flags;                        // account

    uint8_t quest_data1[0x0208];                  // ����������ݱ�1

    psocn_bank_t bank;                            // ����������ݱ�

    psocn_bb_guildcard_t gc_data;                 // ���GC���ݱ���

    uint32_t unk2;                                // not saved

    uint8_t symbol_chats[0x04E0];                 // ѡ�����ݱ�
    uint8_t shortcuts[0x0A40];                    // ѡ�����ݱ�

    uint16_t autoreply[0x00AC];                   // ������ݱ�
    uint16_t infoboard[0x00AC];                   // ������ݱ�

    uint8_t unk3[0x001C];                         // not saved

    uint8_t challenge_data[0x0140];               // �����ս���ݱ�
    uint8_t tech_menu[0x0028];                    // ��ҷ��������ݱ�

    uint8_t unk4[0x002C];                         // not saved

    uint8_t quest_data2[0x0058];                  // ����������ݱ�2

    //uint8_t unk_gc[0x114];
    uint8_t unk1[0x000C];                         // 276 - 264 = 12
    psocn_bb_guildcard_t gc_data2;                // 264��С

    bb_key_config_t key_cfg;                      // ѡ�����ݱ�
    bb_guild_t guild_data;                        // GUILD���ݱ�
} PACKED psocn_bb_full_char_t;

static int bb_c_fullsize = sizeof(psocn_bb_full_char_t);

/* Ŀǰ�洢�����ݿ�Ľ�ɫ���ݽṹ. */
typedef struct psocn_bb_db_char {
    inventory_t inv;
    psocn_bb_char_t character;
    uint8_t quest_data1[0x0208];
    psocn_bank_t bank;
    uint16_t guildcard_desc[0x0058];//88
    uint16_t autoreply[0x00AC];//172
    uint16_t infoboard[0x00AC];//172
    uint8_t challenge_data[0x0140];
    uint8_t tech_menu[0x0028];
    uint8_t quest_data2[0x0058];
} PACKED psocn_bb_db_char_t;

// BB GC ���ݵ���ʵ���ļ� TODO 
typedef struct psocn_bb_guild_card_entry {
    psocn_bb_guildcard_t data;
    uint16_t comment[0x58];
    uint32_t padding;
} PACKED psocn_bb_guild_card_entry_t;

//BB GC �����ļ� TODO 
// 276 + 120 = 396 - 264 = 132     384 + 276 = 660 - 264 = 396
typedef struct bb_guildcard_data {
    uint8_t unk1[0x000C];//276 - 264 = 12
    psocn_bb_guildcard_t black_list[0x001E]; //30�� 264��С/��
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
    uint8_t symbol_chats[0x04E0];//1248
    uint16_t guild_name[0x0010];//16
} PACKED psocn_bb_db_opts_t;

static int opt_size = sizeof(psocn_bb_db_opts_t);

/* BB ���������ļ� TODO */
typedef struct psocn_bb_db_guild {
    bb_guild_t guild_data;                  // account
} PACKED psocn_bb_db_guild_t;

static int guild_size = sizeof(psocn_bb_db_guild_t);

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

// ��ɫ��ɫID����
const enum SectionIDs {
    ID_Viridia = 0, //����
    ID_Greennill, //����
    ID_Skyly, // ����
    ID_Bluefull, //����
    ID_Pinkal, //����
    ID_Purplenum, //�ۺ�
    ID_Redria, //���
    ID_Oran, //�Ȼ�
    ID_Yellowboze, //���
    ID_Whitill, //���
    ID_MAX //��ɫID�����ֵ
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
    char cn_name[16];
    char en_name[16];
    char class_file[24];
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
