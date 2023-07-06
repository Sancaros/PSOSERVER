/*
    梦幻之星中国 舰船服务器 副指令集
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

/* Subcommand types we care about (0x62/0x6D). */
#define SUBCMD62_GUILDCARD                     0x06
#define SUBCMD62_PICK_UP                       0x5A    /* Sent to leader when picking up item */
#define SUBCMD62_ITEMREQ                       0x60    //SUBCMD62_MONSTER_DROP_ITEMREQ
#define SUBCMD62_UNKNOW_6A                     0x6A
/////////////////////////////////////////////////////////////////////////////////////
/* The commands OK to send during bursting (0x62/0x6D). These are named for the
   order in which they're sent, hence why the names are out of order... */
#define SUBCMD6D_BURST2                        0x6B
#define SUBCMD6D_BURST3                        0x6C
#define SUBCMD6D_BURST1                        0x6D
#define SUBCMD6D_BURST4                        0x6E
#define SUBCMD62_BURST5                        0x6F
#define SUBCMD6D_BURST_PLDATA                  0x70
#define SUBCMD62_BURST6                        0x71
/////////////////////////////////////////////////////////////////////////////////////
#define SUBCMD62_BITEMREQ                      0xA2    /*SUBCMD62_BOX_DROP_ITEMREQ BB/GC - Request item drop from box */
#define SUBCMD62_TRADE                         0xA6    /* BB/GC - Player Trade function */
#define SUBCMD62_UNKNOW_A8                     0xA8 // Gol Dragon actions
#define SUBCMD62_UNKNOW_A9                     0xA9 // Barba Ray actions
#define SUBCMD62_UNKNOW_AA                     0xAA // Episode 2 boss actions
#define SUBCMD62_CHARACTER_INFO                0xAE
#define SUBCMD62_SHOP_REQ                      0xB5    /* Blue Burst - Request shop inventory */
#define SUBCMD62_SHOP_BUY                      0xB7    /* Blue Burst - Buy an item from the shop */
#define SUBCMD62_TEKKING                       0xB8    /* Blue Burst - Client is tekking a weapon */
#define SUBCMD62_TEKKED_RESULT                 0xB9    /* Blue Burst - Client is tekked a weapon and get the result */
#define SUBCMD62_TEKKED                        0xBA    /* Blue Burst - Client is tekked a weapon */
#define SUBCMD62_OPEN_BANK                     0xBB    /* Blue Burst - open the bank menu */
/////////////////////////////////////////////////////////////////////////////////////
#define SUBCMD62_BANK_ACTION                   0xBD    /* Blue Burst - do something at the bank */
/* Actions that can be performed at the bank with Subcommand 0xBD (0x62) */
#define SUBCMD62_BANK_ACT_DEPOSIT              0
#define SUBCMD62_BANK_ACT_TAKE                 1
#define SUBCMD62_BANK_ACT_DONE                 2
#define SUBCMD62_BANK_ACT_CLOSE                3
/////////////////////////////////////////////////////////////////////////////////////
#define SUBCMD62_GUILD_INVITE1                 0xC1
#define SUBCMD62_GUILD_INVITE2                 0xC2
#define SUBCMD62_GUILD_MASTER_TRANS1           0xCD
#define SUBCMD62_GUILD_MASTER_TRANS2           0xCE
#define SUBCMD62_QUEST_REWARD_MESETA           0xC9
#define SUBCMD62_QUEST_REWARD_ITEM             0xCA
#define SUBCMD62_BATTLE_CHAR_LEVEL_FIX         0xD0
#define SUBCMD62_CH_GRAVE_DATA                 0xD1
#define SUBCMD62_WARP_ITEM                     0xD6
#define SUBCMD62_QUEST_ONEPERSON_SET_EX_PC     0xDF
#define SUBCMD62_QUEST_ONEPERSON_SET_BP        0xE0
#define SUBCMD62_GANBLING                      0xE2

///////////////////////////////////////////////////////////////////////////////
/* Subcommand types we might care about (0x60/0x6C). */
// 0x00: Invalid subcommand
// 0x01: Invalid subcommand

// 0x02: Unknown
// This subcommand is completely ignored (at least, by PSO GC).
#define SUBCMD60_UNKNOW_02                     0x02

// 0x03: Unknown (same handler as 02)
// This subcommand is completely ignored (at least, by PSO GC).
#define SUBCMD60_UNKNOW_03                     0x03

// 0x04: Unknown
#define SUBCMD60_UNKNOW_04                     0x04

#define SUBCMD60_SWITCH_CHANGED                0x05 // 触发机关开启
#define SUBCMD60_SYMBOL_CHAT                   0x07

// 0x08: Invalid subcommand

// 0x09: Unknown
#define SUBCMD60_UNKNOW_09                     0x09

#define SUBCMD60_HIT_MONSTER                   0x0A
#define SUBCMD60_HIT_OBJ                       0x0B
#define SUBCMD60_CONDITION_ADD                 0x0C // Add condition (poison/slow/etc.)
#define SUBCMD60_CONDITION_REMOVE              0x0D // Remove condition (poison/slow/etc.)

// 6x0E: Unknown
#define SUBCMD60_UNKNOW_0E                     0x0E

// 6x0F: Invalid subcommand

#define SUBCMD60_UNKNOW_10                     0x10
#define SUBCMD60_UNKNOW_11                     0x11
#define SUBCMD60_DRAGON_ACT                    0x12
#define SUBCMD60_ACTION_DE_ROl_LE              0x13
#define SUBCMD60_ACTION_DE_ROl_LE2             0x14
#define SUBCMD60_ACTION_VOL_OPT                0x15
#define SUBCMD60_ACTION_VOL_OPT2               0x16
#define SUBCMD60_TELEPORT                      0x17    //SUBCMD60_TELEPORT_1
#define SUBCMD60_UNKNOW_18                     0x18    /* Dragon special actions */
#define SUBCMD60_ACTION_DARK_FALZ              0x19

// 6x1A: Invalid subcommand

#define SUBCMD60_UNKNOW_1B                     0x1B
#define SUBCMD60_UNKNOW_1C                     0x1C

// 6x1D: Invalid subcommand
// 6x1E: Invalid subcommand

#define SUBCMD60_SET_AREA_1F                   0x1F
#define SUBCMD60_SET_AREA_20                   0x20    /* Seems to match 0x1F */
#define SUBCMD60_INTER_LEVEL_WARP              0x21    // Inter-level warp
#define SUBCMD60_LOAD_22                       0x22    /* Set player visibility Related to 0x21 and 0x23... */
#define SUBCMD60_FINISH_LOAD                   0x23    /* Finished loading to a map, maybe? */
#define SUBCMD60_SET_POS_24                    0x24    /* Used when starting a quest. */
#define SUBCMD60_EQUIP                         0x25
#define SUBCMD60_REMOVE_EQUIP                  0x26
#define SUBCMD60_USE_ITEM                      0x27
#define SUBCMD60_FEED_MAG                      0x28
#define SUBCMD60_DELETE_ITEM                   0x29    /* Selling, deposit in bank, etc */
#define SUBCMD60_DROP_ITEM                     0x2A    /* Drop full stack or non-stack item */
#define SUBCMD60_TAKE_ITEM                     0x2B    /* Create inventory item (e.g. from tekker or bank withdrawal)*/
#define SUBCMD60_SELECT_MENU                   0x2C    /* 选择菜单 */
#define SUBCMD60_SELECT_DONE                   0x2D    /* 选择完成 */
#define SUBCMD60_UNKNOW_2E                     0x2E
#define SUBCMD60_HIT_BY_ENEMY                  0x2F
#define SUBCMD60_LEVEL_UP                      0x30
#define SUBCMD60_MEDIC_REQ                     0x31
#define SUBCMD60_MEDIC_DONE                    0x32
#define SUBCMD60_UNKNOW_33                     0x33
#define SUBCMD60_UNKNOW_34                     0x34

// 6x35: Invalid subcommand
#define SUBCMD60_UNKNOW_35                     0x35
#define SUBCMD60_UNKNOW_36                     0x36
#define SUBCMD60_UNKNOW_37                     0x37
#define SUBCMD60_UNKNOW_38                     0x38
#define SUBCMD60_PB_BLAST_READY                0x39
#define SUBCMD60_GAME_CLIENT_LEAVE             0x3A

#define SUBCMD60_LOAD_3B                       0x3B    /* Something with loading to a map... */

// 6x3C: Invalid subcommand
#define SUBCMD60_UNKNOW_3C                     0x3C
// 6x3D: Invalid subcommand
#define SUBCMD60_UNKNOW_3D                     0x3D

#define SUBCMD60_SET_POS_3E                    0x3E
#define SUBCMD60_SET_POS_3F                    0x3F
#define SUBCMD60_MOVE_SLOW                     0x40

// 6x41: Unknown
// This subcommand is completely ignored (at least, by PSO GC).
#define SUBCMD60_UNKNOW_41                     0x41

#define SUBCMD60_MOVE_FAST                     0x42
#define SUBCMD60_ATTACK1                       0x43
#define SUBCMD60_ATTACK2                       0x44
#define SUBCMD60_ATTACK3                       0x45
#define SUBCMD60_OBJHIT_PHYS                   0x46
#define SUBCMD60_OBJHIT_TECH                   0x47
#define SUBCMD60_USED_TECH                     0x48
#define SUBCMD60_UNKNOW_49                     0x49
#define SUBCMD60_DEFENSE_DAMAGE                0x4A //受到攻击并防御了
#define SUBCMD60_TAKE_DAMAGE1                  0x4B
#define SUBCMD60_TAKE_DAMAGE2                  0x4C
#define SUBCMD60_DEATH_SYNC                    0x4D
#define SUBCMD60_UNKNOW_4E                     0x4E
#define SUBCMD60_PLAYER_SAVED                  0x4F /* 拯救玩家成功 含玩家client_id*/
#define SUBCMD60_SWITCH_REQ                    0x50

// 6x51: Invalid subcommand
#define SUBCMD60_UNKNOW_51                     0x51

#define SUBCMD60_MENU_REQ                      0x52    /*SUBCMD60_TALK_SHOP Talking to someone at a shop */
#define SUBCMD60_UNKNOW_53                     0x53
#define SUBCMD60_UNKNOW_54                     0x54
#define SUBCMD60_WARP_55                       0x55    /* 传送至总督府触发 */
#define SUBCMD60_UNKNOW_56                     0x56
#define SUBCMD60_UNKNOW_57                     0x57
#define SUBCMD60_LOBBY_ACTION                  0x58
#define SUBCMD60_DEL_MAP_ITEM                  0x59    /* Sent by leader when item picked up */
#define SUBCMD60_UNKNOW_5A                     0x5A

// 6x5B: Invalid subcommand
#define SUBCMD60_UNKNOW_5B                     0x5B

#define SUBCMD60_UNKNOW_5C                     0x5C
#define SUBCMD60_DROP_STACK                    0x5D
#define SUBCMD60_BUY                           0x5E
#define SUBCMD60_ITEM_DROP_BOX_ENEMY           0x5F // Drop item from box/enemy
#define SUBCMD60_ITEM_DROP_REQ_ENEMY           0x60 // Request for item drop (handled by the server on BB)
#define SUBCMD60_LEVEL_UP_REQ                  0x61
#define SUBCMD60_UNKNOW_62                     0x62
#define SUBCMD60_DESTROY_GROUND_ITEM           0x63    /* Sent when game inventory is full */
#define SUBCMD60_UNKNOW_64                     0x64
#define SUBCMD60_UNKNOW_65                     0x65
#define SUBCMD60_USE_STAR_ATOMIZER             0x66
#define SUBCMD60_CREATE_ENEMY_SET              0x67 // Create enemy set
#define SUBCMD60_CREATE_PIPE                   0x68
#define SUBCMD60_SPAWN_NPC                     0x69
#define SUBCMD60_UNKNOW_6A                     0x6A
#define SUBCMD60_UNKNOW_6B                     0x6B
#define SUBCMD60_UNKNOW_6C                     0x6C
#define SUBCMD60_UNKNOW_6D                     0x6D
#define SUBCMD60_UNKNOW_6E                     0x6E
#define SUBCMD60_QUEST_DATA1                   0x6F
#define SUBCMD60_UNKNOW_70                     0x70
#define SUBCMD60_UNKNOW_71                     0x71
#define SUBCMD60_BURST_DONE                    0x72
#define SUBCMD60_UNKNOW_73                     0x73
#define SUBCMD60_WORD_SELECT                   0x74
#define SUBCMD60_SET_FLAG                      0x75
#define SUBCMD60_KILL_MONSTER                  0x76    /* A monster was killed. */
#define SUBCMD60_SYNC_REG                      0x77    /* Sent when register is synced in quest */
#define SUBCMD60_UNKNOW_78                     0x78
#define SUBCMD60_GOGO_BALL                     0x79
#define SUBCMD60_UNKNOW_7A                     0x7A
#define SUBCMD60_UNKNOW_7B                     0x7B
#define SUBCMD60_GAME_MODE                     0x7C
#define SUBCMD60_UNKNOW_7D                     0x7D
#define SUBCMD60_UNKNOW_7E                     0x7E
#define SUBCMD60_UNKNOW_7F                     0x7F
#define SUBCMD60_TRIGGER_TRAP                  0x80
#define SUBCMD60_UNKNOW_81                     0x81
#define SUBCMD60_UNKNOW_82                     0x82
#define SUBCMD60_PLACE_TRAP                    0x83
#define SUBCMD60_UNKNOW_84                     0x84
#define SUBCMD60_UNKNOW_85                     0x85
#define SUBCMD60_UNKNOW_86                     0x86
#define SUBCMD60_UNKNOW_87                     0x87
#define SUBCMD60_ARROW_CHANGE                  0x88
#define SUBCMD60_PLAYER_DIED                   0x89
#define SUBCMD60_UNKNOW_CH_8A                  0x8A
#define SUBCMD60_UNKNOW_8B                     0x8B
#define SUBCMD60_UNKNOW_8C                     0x8C
#define SUBCMD60_OVERRIDE_TECH_LEVEL           0x8D //释放法术 TODO 未完成法术有效性分析
#define SUBCMD60_UNKNOW_8E                     0x8E
#define SUBCMD60_UNKNOW_8F                     0x8F
#define SUBCMD60_UNKNOW_90                     0x90
#define SUBCMD60_UNKNOW_91                     0x91
#define SUBCMD60_UNKNOW_92                     0x92
#define SUBCMD60_TIMED_SWITCH_ACTIVATED        0x93
#define SUBCMD60_WARP                          0x94
#define SUBCMD60_UNKNOW_95                     0x95
#define SUBCMD60_UNKNOW_96                     0x96
#define SUBCMD60_UNKNOW_97                     0x97
#define SUBCMD60_UNKNOW_98                     0x98
#define SUBCMD60_UNKNOW_99                     0x99
/////////////////////////////////////////////////////////////////////////////////////
#define SUBCMD60_CHANGE_STAT                   0x9A
/* Stats to use with Subcommand 0x9A (0x60) */
#define SUBCMD60_STAT_HPDOWN                   0
#define SUBCMD60_STAT_TPDOWN                   1
#define SUBCMD60_STAT_MESDOWN                  2
#define SUBCMD60_STAT_HPUP                     3
#define SUBCMD60_STAT_TPUP                     4
/////////////////////////////////////////////////////////////////////////////////////
#define SUBCMD60_UNKNOW_9B                     0x9B
#define SUBCMD60_UNKNOW_9C                     0x9C
#define SUBCMD60_UNKNOW_9D                     0x9D
#define SUBCMD60_UNKNOW_9E                     0x9E
#define SUBCMD60_GAL_GRYPHON_ACT               0x9F    //Gal Gryphon actions(not valid on PC or Episode 3)
#define SUBCMD60_GAL_GRYPHON_SACT              0xA0    /* Gal Gryphon special actions */
#define SUBCMD60_SAVE_PLAYER_ACT               0xA1    /* Save player actions */
#define SUBCMD60_GDRAGON_ACT                   0xA8    /* Gol Dragon special actions */
#define SUBCMD60_UNKNOW_A9                     0xA9   // Barba Ray actions
#define SUBCMD60_UNKNOW_AA                     0xAA   // Episode 2 boss actions
#define SUBCMD60_CHAIR_CREATE                  0xAB   // 生成大厅座椅
#define SUBCMD60_UNKNOW_AC                     0xAC
#define SUBCMD60_UNKNOW_AD                     0xAD   // Olga Flow phase 2 subordinate boss actions
#define SUBCMD62_CHAIR_STATE                   0xAE   // 向新进入大厅的玩家发送座椅状态
#define SUBCMD60_CHAIR_TURN                    0xAF   // 旋转椅子朝向
#define SUBCMD60_CHAIR_MOVE                    0xB0   // 移动椅子
#define SUBCMD60_UNKNOW_B5                     0xB5   // BB shop request process_subcommand_open_shop_bb_or_unknown_ep3
#define SUBCMD60_SHOP_INV                      0xB6   /* Blue Burst - shop inventory */
#define SUBCMD60_UNKNOW_B7                     0xB7   // process_subcommand_buy_shop_item_bb
#define SUBCMD60_UNKNOW_B8                     0xB8   // process_subcommand_identify_item_bb
#define SUBCMD60_UNKNOW_B9                     0xB9   // process_subcommand_unimplemented
#define SUBCMD60_UNKNOW_BA                     0xBA   // process_subcommand_accept_identify_item_bb
#define SUBCMD60_UNKNOW_BB                     0xBB   //process_subcommand_open_bank_bb
#define SUBCMD60_BANK_INV                      0xBC    /* Blue Burst - bank inventory */
#define SUBCMD60_UNKNOW_BD                     0xBD   //process_subcommand_bank_action_bb
#define SUBCMD60_CREATE_ITEM                   0xBE    /* Blue Burst - create new inventory item */
#define SUBCMD60_JUKEBOX                       0xBF    /* Episode III - Change background music */
#define SUBCMD60_GIVE_EXP                      0xBF    /* Blue Burst - give experience points */
#define SUBCMD60_SELL_ITEM                     0xC0
#define SUBCMD60_DROP_SPLIT_ITEM               0xC3    /* Blue Burst - Drop part of stack coords */
#define SUBCMD60_SORT_INV                      0xC4    /* Blue Burst - Sort inventory */
#define SUBCMD60_MEDIC                         0xC5    /* Blue Burst - Use the medical center */
#define SUBCMD60_STEAL_EXP                     0xC6
#define SUBCMD60_CHARGE_ACT                    0xC7
#define SUBCMD60_EXP_REQ                       0xC8    /* Blue Burst - Request Experience */
#define SUBCMD60_GUILD_EX_ITEM                 0xCC
#define SUBCMD60_BATTLEMODE                    0xCF
#define SUBCMD60_GALLON_AREA                   0xD2
#define SUBCMD60_TRADE_DONE                    0xD4
#define SUBCMD60_EX_ITEM                       0xD5
#define SUBCMD60_PD_TRADE                      0xD7
#define SUBCMD60_SRANK_ATTR                    0xD8   //为S级武器添加属性（尚未实现）  挑战模式
#define SUBCMD60_EX_ITEM_MK                    0xD9   // Momoka Item Exchange  Momoka物品交换 
#define SUBCMD60_PD_COMPARE                    0xDA
#define SUBCMD60_UNKNOW_DB                     0xDB
#define SUBCMD60_UNKNOW_DC                     0xDC
#define SUBCMD60_SET_EXP_RATE                  0xDD
#define SUBCMD60_UNKNOW_DE                     0xDE
#define SUBCMD60_GALLON_PLAN                   0xE1


/* Subcommands that we might care about in the Dreamcast NTE (0x60) */
#define SUBCMD60_DCNTE_SET_AREA       0x1D    /* 0x21 */
#define SUBCMD60_DCNTE_FINISH_LOAD    0x1F    /* 0x23 */
#define SUBCMD60_DCNTE_SET_POS        0x36    /* 0x3F */
#define SUBCMD60_DCNTE_MOVE_SLOW      0x37    /* 0x40 */
#define SUBCMD60_DCNTE_MOVE_FAST      0x39    /* 0x42 */
#define SUBCMD60_DCNTE_TALK_SHOP      0x46    /* 0x52 */

