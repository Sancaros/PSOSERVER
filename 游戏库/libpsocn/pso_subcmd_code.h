/*
    梦幻之星中国 舰船服务器 副指令集
    版权 (C) 2022 Sancaros

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

/* Subcommand types we care about (0x62/0x6D). */
#define SUBCMD0x62_GUILDCARD                     0x06
#define SUBCMD0x62_PICK_UP                       0x5A    /* Sent to leader when picking up item */
#define SUBCMD0x62_ITEMREQ                       0x60    //SUBCMD0x62_MONSTER_DROP_ITEMREQ
#define SUBCMD0x62_UNKNOW_6A                     0x6A
#define SUBCMD0x62_BURST2                        0x6B
#define SUBCMD0x62_BURST3                        0x6C
#define SUBCMD0x62_BURST1                        0x6D
#define SUBCMD0x62_BURST4                        0x6E
#define SUBCMD0x62_BURST5                        0x6F
#define SUBCMD0x62_BURST_PLDATA                  0x70
#define SUBCMD0x62_BURST6                        0x71
#define SUBCMD0x62_BITEMREQ                      0xA2    /*SUBCMD0x62_BOX_DROP_ITEMREQ BB/GC - Request item drop from box */
#define SUBCMD0x62_TRADE                         0xA6    /* BB/GC - Player Trade function */
#define SUBCMD0x62_UNKNOW_A8                     0xA8 // Gol Dragon actions
#define SUBCMD0x62_UNKNOW_A9                     0xA9 // Barba Ray actions
#define SUBCMD0x62_UNKNOW_AA                     0xAA // Episode 2 boss actions
#define SUBCMD0x62_CHARACTER_INFO                0xAE
#define SUBCMD0x62_SHOP_REQ                      0xB5    /* Blue Burst - Request shop inventory */
#define SUBCMD0x62_SHOP_BUY                      0xB7    /* Blue Burst - Buy an item from the shop */
#define SUBCMD0x62_TEKKING                       0xB8    /* Blue Burst - Client is tekking a weapon */
#define SUBCMD0x62_TEKKED_RESULT                 0xB9    /* Blue Burst - Client is tekked a weapon and get the result */
#define SUBCMD0x62_TEKKED                        0xBA    /* Blue Burst - Client is tekked a weapon */
#define SUBCMD0x62_OPEN_BANK                     0xBB    /* Blue Burst - open the bank menu */
#define SUBCMD0x62_BANK_ACTION                   0xBD    /* Blue Burst - do something at the bank */
#define SUBCMD0x62_GUILD_INVITE1                 0xC1
#define SUBCMD0x62_GUILD_INVITE2                 0xC2
#define SUBCMD0x62_GUILD_MASTER_TRANS1           0xCD
#define SUBCMD0x62_GUILD_MASTER_TRANS2           0xCE
#define SUBCMD0x62_QUEST_ITEM_UNKNOW1            0xC9
#define SUBCMD0x62_QUEST_ITEM_RECEIVE            0xCA
#define SUBCMD0x62_BATTLE_CHAR_LEVEL_FIX         0xD0
#define SUBCMD0x62_CH_GRAVE_DATA                 0xD1
#define SUBCMD0x62_WARP_ITEM                     0xD6
#define SUBCMD0x62_QUEST_ONEPERSON_SET_ITEM      0xDF
#define SUBCMD0x62_QUEST_ONEPERSON_SET_BP        0xE0
#define SUBCMD0x62_GANBLING                      0xE2

///////////////////////////////////////////////////////////////////////////////
/* Subcommand types we might care about (0x60/0x6C). */
// 0x00: Invalid subcommand
// 0x01: Invalid subcommand

// 0x02: Unknown
// This subcommand is completely ignored (at least, by PSO GC).

// 0x03: Unknown (same handler as 02)
// This subcommand is completely ignored (at least, by PSO GC).

// 0x04: Unknown

#define SUBCMD_SWITCH_CHANGED      0x05 // 触发机关开启
#define SUBCMD_SYMBOL_CHAT         0x07

// 0x08: Invalid subcommand
// 0x09: Unknown

#define SUBCMD_HIT_MONSTER         0x0A
#define SUBCMD_HIT_OBJ             0x0B
#define SUBCMD_CONDITION_ADD       0x0C // Add condition (poison/slow/etc.)
#define SUBCMD_CONDITION_REMOVE    0x0D // Remove condition (poison/slow/etc.)

// 6x0E: Unknown
// 6x0F: Invalid subcommand

#define SUBCMD_DRAGON_ACT              0x12
#define SUBCMD0x60_ACTION_DE_ROl_LE    0x13
#define SUBCMD0x60_UNKNOW_14           0x14
#define SUBCMD0x60_ACTION_VOL_OPT      0x15
#define SUBCMD0x60_ACTION_VOL_OPT2     0x16
#define SUBCMD_TELEPORT                0x17 //SUBCMD0x60_TELEPORT_1
#define SUBCMD0x60_UNKNOW_18           0x18    /* Dragon special actions */
#define SUBCMD0x60_ACTION_DARK_FALZ    0x19

// 6x1A: Invalid subcommand

#define SUBCMD0x60_UNKNOW_1B    0x1B
#define SUBCMD0x60_UNKNOW_1C    0x1C

// 6x1D: Invalid subcommand
// 6x1E: Invalid subcommand

#define SUBCMD_SET_AREA         0x1F
#define SUBCMD_SET_AREA_20      0x20    /* Seems to match 0x1F */
#define SUBCMD_INTER_LEVEL_WARP 0x21   // Inter-level warp
#define SUBCMD_LOAD_22          0x22    /* Set player visibility Related to 0x21 and 0x23... */
#define SUBCMD_FINISH_LOAD      0x23    /* Finished loading to a map, maybe? */
#define SUBCMD_SET_POS_24       0x24    /* Used when starting a quest. */
#define SUBCMD_EQUIP            0x25
#define SUBCMD_REMOVE_EQUIP     0x26
#define SUBCMD_USE_ITEM         0x27
#define SUBCMD_FEED_MAG         0x28
#define SUBCMD_DELETE_ITEM      0x29    /* Selling, deposit in bank, etc */
#define SUBCMD_DROP_ITEM        0x2A    /* Drop full stack or non-stack item */
#define SUBCMD_TAKE_ITEM        0x2B    /* Create inventory item (e.g. from tekker or bank withdrawal)*/
#define SUBCMD_TALK_NPC         0x2C    /* Maybe this is talking to an NPC? */
#define SUBCMD_TALK_NPC_SIZE 0x01
#define SUBCMD_DONE_NPC         0x2D    /*SUBCMD_DONE_NPC Shows up when you're done with an NPC */
#define SUBCMD_DONE_NPC_SIZE 0x05
#define SUBCMD0x60_UNKNOW_2E    0x2E
#define SUBCMD0x60_UNKNOW_2F    0x2F
#define SUBCMD_LEVEL_UP         0x30
#define SUBCMD0x60_UNKNOW_MEDIC_31    0x31
#define SUBCMD0x60_UNKNOW_MEDIC_32    0x32
#define SUBCMD0x60_UNKNOW_33    0x33
#define SUBCMD0x60_UNKNOW_34    0x34

// 6x35: Invalid subcommand
#define SUBCMD0x60_UNKNOW_35    0x35
#define SUBCMD0x60_UNKNOW_36    0x36
#define SUBCMD0x60_UNKNOW_37    0x37
#define SUBCMD0x60_UNKNOW_38    0x38
#define SUBCMD0x60_UNKNOW_39    0x39
#define SUBCMD0x60_UNKNOW_3A    0x3A
#define SUBCMD_LOAD_3B      0x3B    /* Something with loading to a map... */
#define SUBCMD_LOAD_3B_SIZE 0x01

// 6x3C: Invalid subcommand
#define SUBCMD0x60_UNKNOW_3C    0x3C
// 6x3D: Invalid subcommand
#define SUBCMD0x60_UNKNOW_3D    0x3D

#define SUBCMD_SET_POS_3E   0x3E
#define SUBCMD_SET_POS_3F   0x3F
#define SUBCMD_MOVE_SLOW    0x40

// 6x41: Unknown
// This subcommand is completely ignored (at least, by PSO GC).
#define SUBCMD0x60_UNKNOW_41    0x41

#define SUBCMD_MOVE_FAST    0x42
#define SUBCMD0x60_ATTACK1    0x43
#define SUBCMD0x60_ATTACK2    0x44
#define SUBCMD0x60_ATTACK3    0x45
#define SUBCMD0x60_ATTACK_SIZE    0x10
#define SUBCMD_OBJHIT_PHYS  0x46
#define SUBCMD_OBJHIT_TECH  0x47
#define SUBCMD_USED_TECH    0x48
#define SUBCMD0x60_UNKNOW_49    0x49
#define SUBCMD0x60_TAKE_DAMAGE  0x4A //受到攻击并防御了
#define SUBCMD_TAKE_DAMAGE1 0x4B
#define SUBCMD_TAKE_DAMAGE2 0x4C
#define SUBCMD0x60_DEATH_SYNC   0x4D
#define SUBCMD0x60_UNKNOW_4E    0x4E
#define SUBCMD0x60_UNKNOW_4F    0x4F
#define SUBCMD0x60_REQ_SWITCH   0x50

// 6x51: Invalid subcommand
#define SUBCMD0x60_UNKNOW_51    0x51

#define SUBCMD0x60_MENU_REQ    0x52    /*SUBCMD_TALK_SHOP Talking to someone at a shop */
#define SUBCMD0x60_UNKNOW_53    0x53
#define SUBCMD0x60_UNKNOW_54    0x54
#define SUBCMD_WARP_55      0x55    /* 传送至总督府触发 */
#define SUBCMD0x60_UNKNOW_56    0x56
#define SUBCMD0x60_UNKNOW_57    0x57
#define SUBCMD_LOBBY_ACTION 0x58
#define SUBCMD_DEL_MAP_ITEM 0x59    /* Sent by leader when item picked up */
#define SUBCMD0x60_UNKNOW_5A    0x5A

// 6x5B: Invalid subcommand
#define SUBCMD0x60_UNKNOW_5B    0x5B

#define SUBCMD0x60_UNKNOW_5C    0x5C
#define SUBCMD_DROP_STACK       0x5D
#define SUBCMD_BUY              0x5E
#define SUBCMD_BOX_ENEMY_ITEM_DROP     0x5F // Drop item from box/enemy
#define SUBCMD_ENEMY_ITEM_DROP_REQ    0x60 // Request for item drop (handled by the server on BB)
#define SUBCMD_LEVEL_UP_REQ     0x61
#define SUBCMD0x60_UNKNOW_62    0x62
#define SUBCMD_DESTROY_ITEM     0x63    /* Sent when game inventory is full */
#define SUBCMD0x60_UNKNOW_64    0x64
#define SUBCMD0x60_UNKNOW_65    0x65
#define SUBCMD0x60_UNKNOW_66    0x66
#define SUBCMD0x60_CREATE_ENEMY_SET    0x67 // Create enemy set
#define SUBCMD_CREATE_PIPE      0x68
#define SUBCMD_SPAWN_NPC        0x69
#define SUBCMD0x60_UNKNOW_6A    0x6A
#define SUBCMD0x60_UNKNOW_6B    0x6B
#define SUBCMD0x60_UNKNOW_6C    0x6C
#define SUBCMD0x60_UNKNOW_6D    0x6D
#define SUBCMD0x60_UNKNOW_6E    0x6E
#define SUBCMD0x60_UNKNOW_6F    0x6F
#define SUBCMD0x60_UNKNOW_70    0x70
#define SUBCMD0x60_UNKNOW_71    0x71
#define SUBCMD_BURST_DONE   0x72
#define SUBCMD0x60_UNKNOW_73    0x73
#define SUBCMD_WORD_SELECT  0x74
#define SUBCMD0x60_SET_FLAG     0x75
#define SUBCMD_KILL_MONSTER 0x76    /* A monster was killed. */
#define SUBCMD_SYNC_REG     0x77    /* Sent when register is synced in quest */
#define SUBCMD0x60_UNKNOW_78    0x78
#define SUBCMD_GOGO_BALL    0x79
#define SUBCMD0x60_UNKNOW_7A    0x7A
#define SUBCMD0x60_UNKNOW_7B    0x7B
#define SUBCMD_CMODE_GRAVE      0x7C
#define SUBCMD0x60_UNKNOW_7D    0x7D
#define SUBCMD0x60_UNKNOW_7E    0x7E
#define SUBCMD0x60_UNKNOW_7F    0x7F
#define SUBCMD0x60_UNKNOW_80    0x80
#define SUBCMD0x60_UNKNOW_81    0x81
#define SUBCMD0x60_UNKNOW_82    0x82
#define SUBCMD0x60_UNKNOW_83    0x83
#define SUBCMD0x60_UNKNOW_84    0x84
#define SUBCMD0x60_UNKNOW_85    0x85
#define SUBCMD0x60_UNKNOW_86    0x86
#define SUBCMD0x60_UNKNOW_87    0x87
#define SUBCMD0x60_UNKNOW_88    0x88
#define SUBCMD0x60_UNKNOW_89    0x89
#define SUBCMD0x60_UNKNOW_8A    0x8A
#define SUBCMD0x60_UNKNOW_8B    0x8B
#define SUBCMD0x60_UNKNOW_8C    0x8C
#define SUBCMD_SET_TECH_LEVEL_OVERRIDE    0x8D //释放法术 TODO 未完成法术有效性分析
#define SUBCMD0x60_UNKNOW_8E    0x8E
#define SUBCMD0x60_UNKNOW_8F    0x8F
#define SUBCMD0x60_UNKNOW_90    0x90
#define SUBCMD0x60_UNKNOW_91    0x91
#define SUBCMD0x60_UNKNOW_92    0x92
#define SUBCMD_TIMED_SWITCH_ACTIVATED    0x93
#define SUBCMD_WARP             0x94
#define SUBCMD0x60_UNKNOW_95    0x95
#define SUBCMD0x60_UNKNOW_96    0x96
#define SUBCMD0x60_UNKNOW_97    0x97
#define SUBCMD0x60_UNKNOW_98    0x98
#define SUBCMD0x60_UNKNOW_99    0x99
#define SUBCMD_CHANGE_STAT      0x9A
#define SUBCMD0x60_UNKNOW_9B    0x9B
#define SUBCMD0x60_UNKNOW_9C    0x9C
#define SUBCMD0x60_UNKNOW_9D    0x9D
#define SUBCMD0x60_UNKNOW_9E    0x9E
#define SUBCMD_GAL_GRYPHON_ACT  0x9F    //Gal Gryphon actions(not valid on PC or Episode 3)
#define SUBCMD_GAL_GRYPHON_SACT 0xA0    /* Gal Gryphon special actions */
#define SUBCMD_GDRAGON_ACT      0xA8    /* Gol Dragon special actions */
#define SUBCMD0x60_UNKNOW_A9    0xA9 // Barba Ray actions
#define SUBCMD0x60_UNKNOW_AA    0xAA // Episode 2 boss actions
#define SUBCMD_LOBBY_CHAIR      0xAB // Create lobby chair
#define SUBCMD0x60_UNKNOW_AC    0xAC
#define SUBCMD0x60_UNKNOW_AD    0xAD // Olga Flow phase 2 subordinate boss actions
#define SUBCMD0x60_UNKNOW_AE    0xAE
#define SUBCMD_CHAIR_DIR        0xAF // 旋转椅子朝向
#define SUBCMD_CHAIR_MOVE       0xB0 // 移动椅子
#define SUBCMD0x60_UNKNOW_B5    0xB5 // BB shop request process_subcommand_open_shop_bb_or_unknown_ep3
#define SUBCMD_SHOP_INV         0xB6    /* Blue Burst - shop inventory */
#define SUBCMD0x60_UNKNOW_B7    0xB7 // process_subcommand_buy_shop_item_bb
#define SUBCMD0x60_UNKNOW_B8    0xB8 // process_subcommand_identify_item_bb
#define SUBCMD0x60_UNKNOW_B9    0xB9 // process_subcommand_unimplemented
#define SUBCMD0x60_UNKNOW_BA    0xBA // process_subcommand_accept_identify_item_bb
#define SUBCMD0x60_UNKNOW_BB    0xBB //process_subcommand_open_bank_bb
#define SUBCMD_BANK_INV     0xBC    /* Blue Burst - bank inventory */
#define SUBCMD0x60_UNKNOW_BD    0xBD //process_subcommand_bank_action_bb
#define SUBCMD_CREATE_ITEM  0xBE    /* Blue Burst - create new inventory item */
#define SUBCMD_JUKEBOX      0xBF    /* Episode III - Change background music */
#define SUBCMD_GIVE_EXP     0xBF    /* Blue Burst - give experience points */
#define SUBCMD0x60_SELL_ITEM    0xC0
#define SUBCMD_DROP_POS     0xC3    /* Blue Burst - Drop part of stack coords */
#define SUBCMD_SORT_INV     0xC4    /* Blue Burst - Sort inventory */
#define SUBCMD_MEDIC        0xC5    /* Blue Burst - Use the medical center */
#define SUBCMD0x60_STEAL_EXP    0xC6
#define SUBCMD0x60_CHARGE_ACT   0xC7
#define SUBCMD_REQ_EXP      0xC8    /* Blue Burst - Request Experience */
#define SUBCMD0x60_EX_ITEM_TEAM 0xCC
#define SUBCMD0x60_BATTLEMODE   0xCF
#define SUBCMD0x60_GALLON_AREA  0xD2
#define SUBCMD0x60_TRADE_DONE   0xD4
#define SUBCMD0x60_EX_ITEM      0xD5
#define SUBCMD0x60_PD_TREADE    0xD7
#define SUBCMD0x60_SRANK_ATTR   0xD8
#define SUBCMD0x60_EX_ITEM_MK   0xD9
#define SUBCMD0x60_PD_COMPARE   0xDA
#define SUBCMD0x60_UNKNOW_DB    0xDB
#define SUBCMD0x60_UNKNOW_DC    0xDC
#define SUBCMD_SET_EXP_RATE     0xDD
#define SUBCMD0x60_UNKNOW_DE    0xDE
#define SUBCMD0x60_GALLON_PLAN  0xE1


/* Subcommands that we might care about in the Dreamcast NTE (0x60) */
#define SUBCMD_DCNTE_SET_AREA       0x1D    /* 0x21 */
#define SUBCMD_DCNTE_FINISH_LOAD    0x1F    /* 0x23 */
#define SUBCMD_DCNTE_SET_POS        0x36    /* 0x3F */
#define SUBCMD_DCNTE_MOVE_SLOW      0x37    /* 0x40 */
#define SUBCMD_DCNTE_MOVE_FAST      0x39    /* 0x42 */
#define SUBCMD_DCNTE_TALK_SHOP      0x46    /* 0x52 */

/* The commands OK to send during bursting (0x62/0x6D). These are named for the
   order in which they're sent, hence why the names are out of order... */
//#define SUBCMD_BURST2       0x6B //SUBCMD0x62_BURST2
//#define SUBCMD_BURST3       0x6C //SUBCMD0x62_BURST3
//#define SUBCMD_BURST1       0x6D //SUBCMD0x62_BURST1
//#define SUBCMD_BURST4       0x6E //SUBCMD0x62_BURST4
//#define SUBCMD_BURST5       0x6F //SUBCMD0x62_BURST5
//#define SUBCMD_BURST_PLDATA       0x70 //SUBCMD0x62_BURST7 /* Was SUBCMD_BURST7 */
//#define SUBCMD_BURST6       0x71 //SUBCMD0x62_BURST6

   /* The commands OK to send during bursting (0x60) */
   /* 0x3B */
#define SUBCMD_UNK_7C       0x7C

/* Stats to use with Subcommand 0x9A (0x60) */
#define SUBCMD_STAT_HPDOWN  0
#define SUBCMD_STAT_TPDOWN  1
#define SUBCMD_STAT_MESDOWN 2
#define SUBCMD_STAT_HPUP    3
#define SUBCMD_STAT_TPUP    4

/* Actions that can be performed at the bank with Subcommand 0xBD (0x62) */
#define SUBCMD_BANK_ACT_DEPOSIT 0
#define SUBCMD_BANK_ACT_TAKE    1
#define SUBCMD_BANK_ACT_DONE    2
#define SUBCMD_BANK_ACT_CLOSE   3
