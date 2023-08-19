/*
    梦幻之星中国

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

/* 角色信息数据结构 */
typedef struct psocn_disp_char {
    psocn_pl_stats_t stats;
    //TODO 这是什么参数
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

/* 10字节 字符串 GC 含空格 16 字节 */
typedef struct psocn_guildcard_string {
    union guildcard_ {
        /* 仅用于数据库存取 分解 */
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

    uint8_t unk1[8]; // 0x382-0x38F; 898 - 911 14 整数  // Same as E5 unknown2 和E5指令的 未知函数 2 一样

    //玩家名称颜色
    uint8_t name_color_b; // ARGB8888
    uint8_t name_color_g;
    uint8_t name_color_r;
    uint8_t name_color_transparency;

    //皮肤模型
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
    //玩家体态数据
    float prop_x;
    float prop_y;
} PACKED psocn_dress_data_t;

/* BB 客户端 玩家名称结构 24字节 */
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

/* 用于认证服务器的预览角色数据结构 */
/* 不含指令头 124 字节*/
typedef struct psocn_bb_mini_char {
    uint32_t exp;
    uint32_t level;
    psocn_dress_data_t dress_data;
    psocn_bb_char_name_t name;
    uint8_t hw_info[0x08]; // 0x7C - 0x83
    uint32_t play_time;
} PACKED psocn_bb_mini_char_t;

/* 用于发送给大厅中其他玩家的数据结构,不包含背包数据. */
/* BB 玩家数据结构 1244字节 */
typedef struct psocn_bb_char {
    inventory_t inv;
    psocn_disp_char_t disp; //101
    psocn_dress_data_t dress_data;
    psocn_bb_char_name_t name;
    uint16_t padding;
    uint16_t unknown_a3; //4
    uint32_t play_time; //4
    uint8_t config[0xE8]; //232
    techniques_t tech; //20 /* 默认 FF 为空*/
} PACKED psocn_bb_char_t;

/* v1v2v3pc 玩家数据结构 1052字节 */
typedef struct psocn_v1v2v3pc_char {
    inventory_t inv;
    psocn_disp_char_t disp;
    psocn_dress_data_t dress_data;
    uint8_t config[0x48];
    techniques_t tech;
} PACKED psocn_v1v2v3pc_char_t;

/* BB键位设置数据结构 410 字节*/
typedef struct psocn_bb_key_config {
    uint8_t key_config[0x016C];           // 0114
    uint8_t joystick_config[0x0038];      // 0280
} PACKED bb_key_config_t;

/* BB公会数据结构 TODO 2108字节 无法整除8倍数 缺8位 会导致数据无法传输 */
typedef struct psocn_bb_guild {
    uint32_t guild_owner_gc;               // 公会创始人
    uint32_t guild_id;                     // 公会索引 
    // 公会信息     8
    uint32_t guild_points_rank;
    uint32_t guild_points_rest;
    uint32_t guild_priv_level;             // 会员等级     4
    uint16_t guild_name[0x000E];           // 公会名称     28 guild_name_tag1 guild_name_tag2 guild_name[12]
    uint32_t guild_rank;                   // 公会排行     4
    uint8_t guild_flag[0x0800];            // 公会图标     2048
    // 公会奖励     8种奖励
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

// V3 GC 数据 TODO 144
typedef struct psocn_v3_guild_card {
    uint32_t player_tag;
    uint32_t guildcard;
    char name[24];
    char guildcard_desc[108];
    uint8_t present; // 1 表示该GC存在
    uint8_t language;
    uint8_t section;
    uint8_t ch_class;
} PACKED psocn_v3_guild_card_t;

// BB 名片数据
// 19C0 - 19C7
typedef struct psocn_bb_guild_card {
    uint32_t guildcard;                 /* 4   32位 GC  */
    uint16_t name[24];                  /* 48  玩家名称 */
    uint16_t guild_name[16];            /* 32  公会名称 */
    uint16_t guildcard_desc[88];        /* 176 玩家描述 */
    uint8_t present;                    /* 1   占位符 0x01 表示存在 */
    uint8_t language;                   /* 1   语言 0 -8 */
    uint8_t section;                    /* 1   颜色ID */
    uint8_t char_class;                 /* 1   人物职业 */
} PACKED psocn_bb_guild_card_t;

/* BB 完整角色数据 0x00E7 TODO 不含数据包头 8 字节*/
typedef struct psocn_bb_full_char {
    psocn_bb_char_t character;                                               // 玩家数据表               OK
    char guildcard_string[16];                                               // not saved
    uint32_t option_flags;                                                   // account
    uint8_t quest_data1[PSOCN_STLENGTH_BB_DB_QUEST_DATA1];                   // 玩家任务数据表1          OK
    psocn_bank_t bank;                                                       // 玩家银行数据表           OK
    psocn_bb_guild_card_t gc;                                                // 玩家GC数据表部分         OK
    uint32_t unk2;                                                           // not saved
    uint8_t symbol_chats[PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS];                 // 选项数据表
    uint8_t shortcuts[0x0A40];                                               // 选项数据表
    uint16_t autoreply[0x00AC];                                              // 玩家数据表
    uint16_t infoboard[0x00AC];
    battle_records_t b_records;                                              // 玩家对战数据表           TODO
    uint8_t unk3[4];
    bb_challenge_records_t c_records;                                        // 玩家挑战数据表           OK
    uint8_t tech_menu[PSOCN_STLENGTH_BB_DB_TECH_MENU];                       // 玩家法术栏数据表         OK
    uint8_t unk4[0x002C];                                                    // not saved
    uint8_t quest_data2[PSOCN_STLENGTH_BB_DB_QUEST_DATA2];                   // 玩家任务数据表2
    uint8_t unk1[276];                                                       // 276 - 264 = 12
    bb_key_config_t key_cfg;                                                 // 选项数据表               OK
    bb_guild_t guild_data;                                                   // GUILD数据表              OK
} PACKED psocn_bb_full_char_t;

/* 目前存储于数据库的角色数据结构. */
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

// BB GC 数据单独实例文件 TODO 
typedef struct psocn_bb_guild_card_entry {
    psocn_bb_guild_card_t data;
    uint16_t comment[0x58];
    uint32_t padding;
} PACKED psocn_bb_guild_card_entry_t;

//BB GC 数据文件 TODO 
// 276 + 120 = 396 - 264 = 132     384 + 276 = 660 - 264 = 396
typedef struct bb_guildcard_data {
    uint8_t unk1[0x000C];//276 - 264 = 12
    psocn_bb_guild_card_t black_list[0x001E]; //30个 264大小/个
    uint32_t black_gc_list[0x001E];//120
    psocn_bb_guild_card_entry_t entries[0x0069]; //105个 444大小/个
    //uint8_t unk3[0x01BC];
} bb_guildcard_data_t; //54672

/* BB 数据选项文件 TODO */
/* 从完整BB角色数据中分离出来的设置数据,存储于 数据库 中,
来自于 newserv 的 .nsa 文件. */
typedef struct psocn_bb_db_opts {
    uint32_t black_gc_list[0x001E];//30 120字节             /* 黑名单列表 */
    bb_key_config_t key_cfg; //420                          /* 键位设置数据 */
    uint32_t option_flags;
    uint8_t shortcuts[0x0A40];//2624
    uint8_t symbol_chats[PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS];//1248
    uint16_t guild_name[0x0010];//16
} PACKED psocn_bb_db_opts_t;

/* 模式专用角色数据 inv 模式背包 disp 模式角色数值 dress_data 模式专用外观 */
typedef union psocn_mode_char {
    psocn_v1v2v3pc_char_t nobb;
    psocn_bb_char_t bb;
    //inventory_t inv;
    //psocn_disp_char_t disp;
    //psocn_dress_data_t dress_data;
    //uint8_t techniques[0x14]; //20 /* 默认 FF 为空*/
} PACKED psocn_mode_char_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

// 角色颜色ID定义
const enum SectionIDs {
    SID_Viridia = 0, //铬绿
    SID_Greennill, //翠绿
    SID_Skyly, // 天青
    SID_Bluefull, //纯蓝
    SID_Pinkal, //淡紫
    SID_Purplenum, //粉红
    SID_Redria, //真红
    SID_Oran, //橙黄
    SID_Yellowboze, //金黄
    SID_Whitill, //羽白
    SID_MAX //颜色ID最大数值
};

// 职业定义
const enum Character_Classes {
    CLASS_HUMAR, //人类男猎人
    CLASS_HUNEWEARL, //新人类女猎人
    CLASS_HUCAST, //机器男猎人
    CLASS_RAMAR, //人类男枪手
    CLASS_RACAST, //机器人男枪手
    CLASS_RACASEAL, //机器人女枪手
    CLASS_FOMARL, //人类女法师
    CLASS_FONEWM, //新人类男法师
    CLASS_FONEWEARL, //新人类女法师
    CLASS_HUCASEAL, //机器人女猎人
    CLASS_FOMAR, //人类男法师
    CLASS_RAMARL, //人类女枪手
    CLASS_MAX, // 最大职业数量
    CLASS_FULL_CHAR = 12
};

typedef struct full_class {
    uint32_t class_code;
    char cn_name[24];
    char en_name[16];
    char class_file[64];
} full_class_t;

static full_class_t pso_class[] = {
    {CLASS_HUMAR,     "人类男猎人",    "HUmar",     "0_默认_人类男猎人.nsc"  },
    {CLASS_HUNEWEARL, "新人类女猎人",  "HUnewearl", "1_默认_新人类女猎人.nsc"},
    {CLASS_HUCAST,    "机器人男猎人",  "HUcast",    "2_默认_机器人男猎人.nsc"},
    {CLASS_RAMAR,     "人类男枪手",    "RAmar",     "3_默认_人类男枪手.nsc"  },
    {CLASS_RACAST,    "机器人男枪手",  "RAcast",    "4_默认_机器人男枪手.nsc"},
    {CLASS_RACASEAL,  "机器人女枪手",  "RAcaseal",  "5_默认_机器人女枪手.nsc"},
    {CLASS_FOMARL,    "人类女法师",    "FOmarl",    "6_默认_人类女法师.nsc"  },
    {CLASS_FONEWM,    "新人类男法师",  "FOnewm",    "7_默认_新人类男法师.nsc"},
    {CLASS_FONEWEARL, "新人类女法师",  "FOnewearl", "8_默认_新人类女法师.nsc"},
    {CLASS_HUCASEAL,  "机器人女猎人",  "HUcaseal",  "9_默认_机器人女猎人.nsc"},
    {CLASS_FOMAR,     "人类男法师",    "FOmar",     "10_默认_人类男法师.nsc" },
    {CLASS_RAMARL,    "人类女枪手",    "RAmarl",    "11_默认_人类女枪手.nsc" },
    {CLASS_FULL_CHAR, "完整角色",      "Full_char", "默认_完整角色.bin"      },
};

// 药物定义
const enum Character_Mats {
    ATP_MAT, //攻击力药
    MST_MAT, //智力药
    EVP_MAT, //闪避药
    HP_MAT, //血量药
    TP_MAT, //魔力药
    DFP_MAT, //防御药
    LCK_MAT, //幸运药
    MATS_MAX // 药数量
};
#endif /* !PSOCN_CHARACTERS_H */
