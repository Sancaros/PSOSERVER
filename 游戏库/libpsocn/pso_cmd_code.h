/*
    梦幻之星中国 服务器指令集
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

////////////////////////////////////////////////////////////////////////////////
// CommandServerInit: this function sends the command that initializes encryption

// strings needed for various functions
//const static char anti_copyright[] = "This server is in no way affiliated, sponsored, or supported by SEGA Enterprises or SONICTEAM. The preceding message exists only in order to remain compatible with programs that expect it.";
//const static char dc_port_map_copyright[] = "DreamCast Port Map. Copyright SEGA Enterprises. 1999";
//const static char dc_lobby_server_copyright[] = "DreamCast Lobby Server. Copyright SEGA Enterprises. 1999";
//const static char bb_game_server_copyright[] = "Phantasy Star Online Blue Burst Game Server. Copyright 1999-2004 SONICTEAM.";
//const static char bb_pm_server_copyright[] = "PSO NEW PM Server. Copyright 1999-2002 SONICTEAM.";
//const static char patch_server_copyright[] = "Patch Server. Copyright SonicTeam, LTD. 2001";

/*
#define MSG1_TYPE                       0x0001 //右下角弹窗信息数据包 ok
#define CHAT_TYPE                       0x0006 //聊天弹窗
#define BLOCK_LIST_TYPE                 0x0007 //大厅数据包 左上角信息窗
#define INFO_REPLY_TYPE                 0x0011 //左下角弹窗信息框数据包 实际无效
#define QUEST_CHUNK_TYPE                0x0013 //任务列表
#define MSG_BOX_TYPE                    0x001A //大信息窗口文本代码
#define LOBBY_LIST_TYPE                 0x0083 //大厅房间信息板数据包
#define SHIP_LIST_TYPE                  0x00A0 //舰船左上角信息窗
#define TEXT_MSG_TYPE                   0x00B0  //边栏信息窗数据包
#define GC_MSG_BOX_TYPE                 0x00D5 //GC BB XB信息窗
#define GC_MSG_BOX_CLOSED_TYPE          0x00D6 //GC 信息窗关闭
#define BB_SCROLL_MSG_TYPE              0x00EE //顶部公告数据包 ok
*/

/* 游戏房间指令定义 */
#define GAME_TYPE_NORMAL                             0x00
#define GAME_TYPE_EPISODE_1                          0x01
#define GAME_TYPE_EPISODE_2                          0x02
#define GAME_TYPE_EP3_EPISODE                        0x03
#define GAME_TYPE_EPISODE_4                          0x03

#define GAME_TYPE_MODE_NORMAL                        0x00
#define GAME_TYPE_MODE_BATTLE_BATTLERULE_1           0x01
#define GAME_TYPE_MODE_BATTLE_BATTLERULE_2           0x02
#define GAME_TYPE_MODE_BATTLE_BATTLERULE_3           0x03

#define GAME_TYPE_MODE_CHALLENGE_EP1                 0x01
#define GAME_TYPE_MODE_CHALLENGE_EP2                 0x02

#define GAME_TYPE_MODE_BATTLE                        0x03
#define GAME_TYPE_MODE_ONEPERSON                     0x04

#define GAME_TYPE_DIFFICULTY_NORMARL                 0x00
#define GAME_TYPE_DIFFICULTY_HARD                    0x01
#define GAME_TYPE_DIFFICULTY_VERY_HARD               0x02
#define GAME_TYPE_DIFFICULTY_ULTIMATE                0x03

/* Parameters for the various packets. */
#define MSG1_TYPE                       0x0001 //右下角弹窗信息数据包
#define WELCOME_TYPE                    0x0002 //"欢迎信息" 数据包
#define BB_WELCOME_TYPE                 0x0003 //Start Encryption Packet 开始加密数据包
#define SECURITY_TYPE                   0x0004
#define BURSTING_TYPE                   0x0005
#define CHAT_TYPE                       0x0006
#define BLOCK_LIST_TYPE                 0x0007 //大厅数据包
#define GAME_LIST_TYPE                  0x0008 //游戏房间数据包
#define INFO_REQUEST_TYPE               0x0009
#define DC_GAME_CREATE_TYPE             0x000C
#define MENU_SELECT_TYPE                0x0010
#define INFO_REPLY_TYPE                 0x0011 //左下角弹窗信息框数据包
#define CLIENT_UNKNOW_12                0x0012
#define QUEST_CHUNK_TYPE                0x0013
#define LOGIN_WELCOME_TYPE              0x0017
#define REDIRECT_TYPE                   0x0019 //重定向数据包 用于连接
#define MSG_BOX_TYPE                    0x001A //大信息窗口文本代码
#define PING_TYPE                       0x001D //Ping pong 心跳数据包
#define LOBBY_INFO_TYPE                 0x001F
#define GUILD_SEARCH_TYPE               0x0040
#define GUILD_REPLY_TYPE                0x0041
#define QUEST_FILE_TYPE                 0x0044
#define CLIENT_UNKNOW_4F                0x004F
#define GAME_COMMAND0_TYPE              0x0060  //来自1.0 BB服务端
#define CHAR_DATA_TYPE                  0x0061
#define GAME_COMMAND2_TYPE              0x0062  //来自1.0 BB服务端
#define GAME_JOIN_TYPE                  0x0064
#define GAME_ADD_PLAYER_TYPE            0x0065
#define GAME_LEAVE_TYPE                 0x0066 //用户离开游戏房间数据包
#define LOBBY_JOIN_TYPE                 0x0067 //用户进入大厅数据包
#define LOBBY_ADD_PLAYER_TYPE           0x0068 //大厅新增用户数据包
#define LOBBY_LEAVE_TYPE                0x0069 //用户离开大厅数据包
#define CLIENT_UNKNOW_6A                0x006A //来自1.0 BB服务端
#define GAME_COMMANDC_TYPE              0x006C
#define GAME_COMMANDD_TYPE              0x006D  //创建房间指令
#define DONE_BURSTING_TYPE              0x006F
//[2022年09月03日 08:59:54:819] 调试(block-bb.c 2195): 舰仓：BB处理数据 指令 0x016F 未找到相关指令名称 长度 8 字节 标志 0 GC 42004063
//( 00000000 )   08 00 6F 01 00 00 00 00                             ..o.....
#define DONE_BURSTING_TYPE01            0x016F
#define CLIENT_UNKNOW_77                0x0077  //来自1.0 BB服务端
#define CLIENT_UNKNOW_81                0x0080  //来自1.0 BB服务端
#define SIMPLE_MAIL_TYPE                0x0081
#define LOBBY_LIST_TYPE                 0x0083 //大厅房间信息板数据包
#define LOBBY_CHANGE_TYPE               0x0084
#define LOBBY_ARROW_LIST_TYPE           0x0088
#define LOGIN_88_TYPE                   0x0088  /* DC 网络测试版数据包 */
#define LOBBY_ARROW_CHANGE_TYPE         0x0089
#define LOBBY_NAME_TYPE                 0x008A
#define LOGIN_8A_TYPE                   0x008A  /* DC 网络测试版数据包 */
#define LOGIN_8B_TYPE                   0x008B  /* DC 网络测试版数据包 */
#define DCNTE_CHAR_DATA_REQ_TYPE        0x008D  /* DC 网络测试版数据包 */
#define DCNTE_SHIP_LIST_TYPE            0x008E  /* DC 网络测试版数据包 */
#define DCNTE_BLOCK_LIST_REQ_TYPE       0x008F  /* DC 网络测试版数据包 */
#define LOGIN_90_TYPE                   0x0090
#define CLIENT_UNKNOW_91                0x0091  //来自1.0 BB服务端
#define LOGIN_92_TYPE                   0x0092
#define LOGIN_93_TYPE                   0x0093
#define CHAR_DATA_REQUEST_TYPE          0x0095
#define CHECKSUM_TYPE                   0x0096
#define CHECKSUM_REPLY_TYPE             0x0097
#define LEAVE_GAME_PL_DATA_TYPE         0x0098
#define SHIP_LIST_REQ_TYPE              0x0099
#define EP3_UPDATE_REQ_TYPE             0x0099
#define LOGIN_9A_TYPE                   0x009A
#define LOGIN_9B_TYPE                   0x009B
#define LOGIN_9C_TYPE                   0x009C
#define LOGIN_9D_TYPE                   0x009D
#define LOGIN_9E_TYPE                   0x009E
#define SHIP_LIST_TYPE                  0x00A0
#define BLOCK_LIST_REQ_TYPE             0x00A1
#define QUEST_LIST_TYPE                 0x00A2
#define QUEST_INFO_TYPE                 0x00A3
#define DL_QUEST_LIST_TYPE              0x00A4
#define DL_QUEST_INFO_TYPE              0x00A5
#define DL_QUEST_FILE_TYPE              0x00A6
#define DL_QUEST_CHUNK_TYPE             0x00A7
#define QUEST_END_LIST_TYPE             0x00A9
#define QUEST_STATS_TYPE                0x00AA
#define QUEST_LOAD_DONE_TYPE            0x00AC
#define TEXT_MSG_TYPE                   0x00B0  //边栏信息窗数据包
#define TIMESTAMP_TYPE                  0x00B1  //当前时间戳
#define PATCH_TYPE                      0x00B2
#define PATCH_RETURN_TYPE               0x00B3
#define EP3_RANK_UPDATE_TYPE            0x00B7
#define EP3_CARD_UPDATE_TYPE            0x00B8
#define EP3_COMMAND_TYPE                0x00BA
#define CHOICE_OPTION_TYPE              0x00C0
#define GAME_CREATE_TYPE                0x00C1
#define CHOICE_SETTING_TYPE             0x00C2
#define CHOICE_SEARCH_TYPE              0x00C3
#define CHOICE_REPLY_TYPE               0x00C4
#define C_RANK_TYPE                     0x00C5
#define BLACKLIST_TYPE                  0x00C6
#define AUTOREPLY_SET_TYPE              0x00C7
#define AUTOREPLY_CLEAR_TYPE            0x00C8
#define GAME_COMMAND_C9_TYPE            0x00C9
#define EP3_SERVER_DATA_TYPE            0x00CA
#define GAME_COMMAND_CB_TYPE            0x00CB
#define CLIENT_UNKNOW_CC                0x00CC
#define TRADE_0_TYPE                    0x00D0
#define TRADE_1_TYPE                    0x00D1
#define TRADE_2_TYPE                    0x00D2
#define TRADE_3_TYPE                    0x00D3
#define TRADE_4_TYPE                    0x00D4
#define TRADE_5_TYPE                    0x00D5
#define GC_MSG_BOX_TYPE                 0x00D5 //BB XB GC 都支持
#define GC_MSG_BOX_CLOSED_TYPE          0x00D6
#define GC_GBA_FILE_REQ_TYPE            0x00D7
#define INFOBOARD_TYPE                  0x00D8
#define INFOBOARD_WRITE_TYPE            0x00D9
#define LOBBY_EVENT_TYPE                0x00DA  //大厅事件改变数据包
#define GC_VERIFY_LICENSE_TYPE          0x00DB
//////////////////////////////////////////////////
#define EP3_MENU_CHANGE_TYPE            0x00DC
#define BB_GUILDCARD_HEADER_TYPE        0x01DC
#define BB_GUILDCARD_CHUNK_TYPE         0x02DC
#define BB_GUILDCARD_CHUNK_REQ_TYPE     0x03DC
//////////////////////////////////////////////////
#define BB_SEND_QUEST_STATE             0x00DD
#define BB_RARE_MONSTER_LIST            0x00DE
//////////////////////////////////////////////////
#define BB_CHALLENGE_DF                 0x00DF  //来自1.0 BB服务端 挑战模式数据
#define BB_CHALLENGE_01DF               0x01DF  //来自1.0 BB服务端 挑战模式数据
#define BB_CHALLENGE_02DF               0x02DF  //来自1.0 BB服务端 挑战模式数据
#define BB_CHALLENGE_03DF               0x03DF  //来自1.0 BB服务端 挑战模式数据
#define BB_CHALLENGE_04DF               0x04DF  //来自1.0 BB服务端 挑战模式数据
#define BB_CHALLENGE_05DF               0x05DF  //来自1.0 BB服务端 挑战模式数据
#define BB_CHALLENGE_06DF               0x06DF  //来自1.0 BB服务端 挑战模式数据
#define BB_CHALLENGE_07DF               0x07DF  //来自1.0 BB服务端 挑战模式数据
//////////////////////////////////////////////////
#define BB_OPTION_REQUEST_TYPE          0x00E0
#define CLIENT_UNKNOW_E1                0x00E1  // E1 (S->C): Game information (Episode 3)
#define BB_OPTION_CONFIG_TYPE           0x00E2
#define BB_CHARACTER_SELECT_TYPE        0x00E3
#define BB_CHARACTER_ACK_TYPE           0x00E4
#define BB_CHARACTER_UPDATE_TYPE        0x00E5
#define BB_SECURITY_TYPE                0x00E6  //BB 安全数据包
#define BB_FULL_CHARACTER_TYPE          0x00E7
//////////////////////////////////////////////////确认客户端的预期校验和
#define BB_GUILD_CHECKSUM_TYPE          0x00E8  //来自1.0 BB服务端 单指令 以下都是组合指令
#define BB_CHECKSUM_TYPE                0x01E8
#define BB_CHECKSUM_ACK_TYPE            0x02E8
#define BB_GUILD_REQUEST_TYPE           0x03E8
#define BB_ADD_GUILDCARD_TYPE           0x04E8
#define BB_DEL_GUILDCARD_TYPE           0x05E8
#define BB_SET_GUILDCARD_TEXT_TYPE      0x06E8
#define BB_ADD_BLOCKED_USER_TYPE        0x07E8
#define BB_DEL_BLOCKED_USER_TYPE        0x08E8
#define BB_SET_GUILDCARD_COMMENT_TYPE   0x09E8
#define BB_SORT_GUILDCARD_TYPE          0x0AE8
//////////////////////////////////////////////////EA指令
#define BB_GUILD_COMMAND                0x00EA  //来自1.0 BB服务端 单指令 以下都是组合指令
#define BB_GUILD_CREATE                 0x01EA
#define BB_GUILD_UNK_02EA               0x02EA
#define BB_GUILD_MEMBER_ADD             0x03EA
#define BB_GUILD_UNK_04EA               0x04EA
#define BB_GUILD_MEMBER_REMOVE          0x05EA
#define BB_GUILD_UNK_06EA               0x06EA
#define BB_GUILD_CHAT                   0x07EA
#define BB_GUILD_MEMBER_SETTING         0x08EA
#define BB_GUILD_UNK_09EA               0x09EA
#define BB_GUILD_UNK_0AEA               0x0AEA
#define BB_GUILD_UNK_0BEA               0x0BEA
#define BB_GUILD_UNK_0CEA               0x0CEA
#define BB_GUILD_INVITE                 0x0DEA
#define BB_GUILD_UNK_0EEA               0x0EEA
#define BB_GUILD_MEMBER_FLAG_SETTING    0x0FEA
#define BB_GUILD_DISSOLVE               0x10EA
#define BB_GUILD_MEMBER_PROMOTE         0x11EA
#define BB_GUILD_UNK_12EA               0x12EA
#define BB_GUILD_LOBBY_SETTING          0x13EA
#define BB_GUILD_MEMBER_TITLE           0x14EA
#define BB_GUILD_FULL_DATA              0x15EA
#define BB_GUILD_UNK_16EA               0x16EA
#define BB_GUILD_UNK_17EA               0x17EA
#define BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO          0x18EA
#define BB_GUILD_PRIVILEGE_LIST         0x19EA
#define BB_GUILD_UNK_1AEA               0x1AEA
#define BB_GUILD_UNK_1BEA               0x1BEA
#define BB_GUILD_RANKING_LIST           0x1CEA
#define BB_GUILD_UNK_1DEA               0x1DEA
#define BB_GUILD_UNK_1EEA               0x1EEA
#define BB_GUILD_UNK_1FEA               0x1FEA
#define BB_GUILD_UNK_20EA               0x20EA
//////////////////////////////////////////////////
#define BB_PARAM_SEND_TYPE              0x00EB  //来自1.0 BB服务端 单指令 以下都是组合指令
#define BB_PARAM_HEADER_TYPE            0x01EB
#define BB_PARAM_CHUNK_TYPE             0x02EB
#define BB_PARAM_CHUNK_REQ_TYPE         0x03EB
#define BB_PARAM_HEADER_REQ_TYPE        0x04EB
//////////////////////////////////////////////////
#define EP3_GAME_CREATE_TYPE            0x00EC
#define BB_SETFLAG_TYPE                 0x00EC
//////////////////////////////////////////////////
#define BB_UPDATE_OPTION                0x00ED  //来自1.0 BB服务端 单指令 以下都是组合指令
#define BB_UPDATE_OPTION_FLAGS          0x01ED
#define BB_UPDATE_SYMBOL_CHAT           0x02ED
#define BB_UPDATE_SHORTCUTS             0x03ED
#define BB_UPDATE_KEY_CONFIG            0x04ED
#define BB_UPDATE_PAD_CONFIG            0x05ED
#define BB_UPDATE_TECH_MENU             0x06ED
#define BB_UPDATE_CONFIG                0x07ED
#define BB_UPDATE_C_MODE_CONFIG         0x08ED  //来自1.0 BB服务端
//////////////////////////////////////////////////
#define BB_SCROLL_MSG_TYPE              0x00EE  //顶部公告数据包
#define CLIENT_UNKNOW_F0                0x00F0

//////////////////////////////////////////////////数据包长度
#define DC_WELCOME_LENGTH               0x004C
#define BB_WELCOME_LENGTH               0x00C8
#define BB_SECURITY_LENGTH              0x0044
#define DC_REDIRECT_LENGTH              0x000C
#define BB_REDIRECT_LENGTH              0x0010
#define DC_REDIRECT6_LENGTH             0x0018
#define BB_REDIRECT6_LENGTH             0x001C
#define DC_TIMESTAMP_LENGTH             0x0020
#define BB_TIMESTAMP_LENGTH             0x0024
#define DC_LOBBY_LIST_LENGTH            0x00C4
#define EP3_LOBBY_LIST_LENGTH           0x0100
#define BB_LOBBY_LIST_LENGTH            0x00C8
#define DC_CHAR_DATA_LENGTH             0x0420
#define DC_LOBBY_LEAVE_LENGTH           0x0008
#define BB_LOBBY_LEAVE_LENGTH           0x000C
#define DC_GUILD_REPLY_LENGTH           0x00C4
#define PC_GUILD_REPLY_LENGTH           0x0128
#define BB_GUILD_REPLY_LENGTH           0x0130
#define DC_GUILD_REPLY6_LENGTH          0x00D0
#define PC_GUILD_REPLY6_LENGTH          0x0134
#define BB_GUILD_REPLY6_LENGTH          0x013C
#define DC_GAME_JOIN_LENGTH             0x0110
#define GC_GAME_JOIN_LENGTH             0x0114
#define EP3_GAME_JOIN_LENGTH            0x1184
#define DC_QUEST_INFO_LENGTH            0x0128
#define PC_QUEST_INFO_LENGTH            0x024C
#define BB_QUEST_INFO_LENGTH            0x0250
#define DC_QUEST_FILE_LENGTH            0x003C
#define BB_QUEST_FILE_LENGTH            0x0058
#define DC_QUEST_CHUNK_LENGTH           0x0418
#define BB_QUEST_CHUNK_LENGTH           0x041C
#define DC_SIMPLE_MAIL_LENGTH           0x0220
#define PC_SIMPLE_MAIL_LENGTH           0x0430
#define BB_SIMPLE_MAIL_LENGTH           0x045C
#define BB_OPTION_CONFIG_LENGTH         0x0AF8

//数据包大小 14748
#define BB_FULL_CHARACTER_LENGTH        0x399C

//数据包大小 14752
#define BB_FULL_CHARACTER_DATA_LENGTH   0x39A0

#define DC_PATCH_HEADER_LENGTH          0x0014
#define DC_PATCH_FOOTER_LENGTH          0x0018

/* Responses to login packets... */
/* DC 网络测试版数据包 - Responses to Packet 0x88. */
#define LOGIN_88_NEW_USER                   0
#define LOGIN_88_OK                         1

/* DC 网络测试版数据包 - Responses to Packet 0x8A. */
#define LOGIN_8A_REGISTER_OK                1

/* DCv1 - Responses to Packet 0x90. */
#define LOGIN_90_OK                         0
#define LOGIN_90_NEW_USER                   1
#define LOGIN_90_OK2                        2
#define LOGIN_90_BAD_SNAK                   3

/* DCv1 - Responses to Packet 0x92. */
#define LOGIN_92_BAD_SNAK                   0
#define LOGIN_92_OK                         1

/* DCv2/PC - Responses to Packet 0x9A. */
#define LOGIN_9A_OK                         0
#define LOGIN_9A_NEW_USER                   1
#define LOGIN_9A_OK2                        2
#define LOGIN_9A_BAD_ACCESS                 3
#define LOGIN_9A_BAD_SERIAL                 4
#define LOGIN_9A_ERROR                      5

/* DCv2/PC - Responses to Packet 0x9C. */
#define LOGIN_9CV2_REG_FAIL                 0
#define LOGIN_9CV2_OK                       1

/* Gamecube - Responses to Packet 0xDB. */
#define LOGIN_DB_OK                         0
#define LOGIN_DB_NEW_USER                   1
#define LOGIN_DB_OK2                        2
#define LOGIN_DB_BAD_ACCESS                 3
#define LOGIN_DB_BAD_SERIAL                 4
#define LOGIN_DB_NET_ERROR                  5   /* Also 6, 9, 10, 20-255. */
#define LOGIN_DB_NO_HL                      7   /* Also 18. */
#define LOGIN_DB_EXPIRED_HL                 8   /* Also 17. */
#define LOGIN_DB_BAD_HL                     11  /* Also 12, 13 - Diff errnos. */
#define LOGIN_DB_CONN_ERROR                 14
#define LOGIN_DB_SUSPENDED                  15  /* Also 16. */
#define LOGIN_DB_MAINTENANCE                19

/* Gamecube - Responses to Packet 0x9C. */
#define LOGIN_9CGC_BAD_PWD                  0
#define LOGIN_9CGC_OK                       1

/* Blue Burst - Responses to Packet 0x93. */
#define LOGIN_93BB_OK                       0
#define LOGIN_93BB_UNKNOWN_ERROR            1
#define LOGIN_93BB_BAD_USER_PWD             2
#define LOGIN_93BB_BAD_USER_PWD2            3
#define LOGIN_93BB_MAINTENANCE              4
#define LOGIN_93BB_ALREADY_ONLINE           5
#define LOGIN_93BB_BANNED                   6
#define LOGIN_93BB_BANNED2                  7
#define LOGIN_93BB_NO_USER_RECORD           8
#define LOGIN_93BB_PAY_UP                   9
#define LOGIN_93BB_LOCKED                   10  /* Improper shutdown */
#define LOGIN_93BB_BAD_VERSION              11
#define LOGIN_93BB_FORCED_DISCONNECT        12

/* Episode 3 - Types of 0xBA commands. */
#define EP3_COMMAND_LEAVE_TEAM              1
#define EP3_COMMAND_JUKEBOX_REQUEST         2
#define EP3_COMMAND_JUKEBOX_REPLY           3

/* Blue Burst - Character Acknowledgement codes. */
#define BB_CHAR_ACK_UPDATE                  0
#define BB_CHAR_ACK_SELECT                  1
#define BB_CHAR_ACK_NONEXISTANT             2
