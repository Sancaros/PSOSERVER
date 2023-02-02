/*
    梦幻之星中国 服务器 指令集
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pso_cmd_code.h"
#include "pso_subcmd_code.h"
#include "pso_sgcmd_code.h"

#define NO_SUCH_CMD 0xFFFFFFFF

#define BB_LOG_UNKNOWN_SUBS
#define LOG_UNKNOWN_SUBS

/* client conmand code to string mapping */
typedef struct cmd_map {
    uint32_t cmd;
    const char* name;
    const char* cnname;
} cmd_map_st;

static cmd_map_st c_subcmd_names[] = {
    { SUBCMD62_GUILDCARD, "SUBCMD62_GUILDCARD", "SUBCMD62_GUILDCARD" },
    { SUBCMD60_UNKNOW_88, "SUBCMD60_UNKNOW_88", "未知指令" },
};

static cmd_map_st c_cmd_names[] = {
    { MSG1_TYPE, "MSG1_TYPE", "消息指令" },
    { WELCOME_TYPE, "WELCOME_TYPE", "欢迎指令" },
    { BB_WELCOME_TYPE, "BB_WELCOME_TYPE", "欢迎指令" },
    { SECURITY_TYPE, "SECURITY_TYPE", "安全指令" },
    { BURSTING_TYPE, "BURSTING_TYPE", "跃迁指令" },
    { CHAT_TYPE, "CHAT_TYPE", "聊天指令" },
    { BLOCK_LIST_TYPE, "BLOCK_LIST_TYPE", "舰仓列表" },
    { GAME_LIST_TYPE, "GAME_LIST_TYPE", "游戏列表" },
    { INFO_REQUEST_TYPE, "INFO_REQUEST_TYPE", "信息请求指令" },
    { DC_GAME_CREATE_TYPE, "DC_GAME_CREATE_TYPE", "DC游戏创建" },
    { MENU_SELECT_TYPE, "MENU_SELECT_TYPE", "客户端至服务器指令" },
    { INFO_REPLY_TYPE, "INFO_REPLY_TYPE", "左下角弹窗信息框数据包" },
    { CLIENT_UNKNOW_12, "CLIENT_UNKNOW_12", "客户端至服务器指令" },
    { QUEST_CHUNK_TYPE, "QUEST_CHUNK_TYPE", "客户端至服务器指令" },
    { LOGIN_WELCOME_TYPE, "LOGIN_WELCOME_TYPE", "客户端至服务器指令" },
    { REDIRECT_TYPE, "REDIRECT_TYPE", "重定向数据包 用于连接" },
    { MSG_BOX_TYPE, "MSG_BOX_TYPE", "大信息窗口文本代码" },
    { PING_TYPE, "PING_TYPE", "Ping pong 心跳数据包" },
    { LOBBY_INFO_TYPE, "LOBBY_INFO_TYPE", "客户端至服务器指令" },
    { GUILD_SEARCH_TYPE, "GUILD_SEARCH_TYPE", "客户端至服务器指令" },
    { GUILD_REPLY_TYPE, "GUILD_REPLY_TYPE", "客户端至服务器指令" },
    { QUEST_FILE_TYPE, "QUEST_FILE_TYPE", "客户端至服务器指令" },
    { CLIENT_UNKNOW_4F, "CLIENT_UNKNOW_4F", "客户端至服务器指令" },
    { GAME_COMMAND0_TYPE, "GAME_COMMAND0_TYPE", "来自1.0 BB服务端" },
    { CHAR_DATA_TYPE, "CHAR_DATA_TYPE", "客户端至服务器指令" },
    { GAME_COMMAND2_TYPE, "GAME_COMMAND2_TYPE", "来自1.0 BB服务端" },
    { GAME_JOIN_TYPE, "GAME_JOIN_TYPE", "客户端至服务器指令" },
    { GAME_ADD_PLAYER_TYPE, "GAME_ADD_PLAYER_TYPE", "客户端至服务器指令" },
    { GAME_LEAVE_TYPE, "GAME_LEAVE_TYPE", "用户离开游戏房间数据包" },
    { LOBBY_JOIN_TYPE, "LOBBY_JOIN_TYPE", "用户进入大厅数据包" },
    { LOBBY_ADD_PLAYER_TYPE, "LOBBY_ADD_PLAYER_TYPE", "大厅新增用户数据包" },
    { LOBBY_LEAVE_TYPE, "LOBBY_LEAVE_TYPE", "用户离开大厅数据包" },
    { CLIENT_UNKNOW_6A, "CLIENT_UNKNOW_6A", "来自1.0 BB服务端" },
    { GAME_COMMANDC_TYPE, "GAME_COMMANDC_TYPE", "客户端至服务器指令" },
    { GAME_COMMANDD_TYPE, "GAME_COMMANDD_TYPE", "创建房间指令" },
    { DONE_BURSTING_TYPE, "DONE_BURSTING_TYPE", "客户端至服务器指令" },
    { DONE_BURSTING_TYPE01, "DONE_BURSTING_TYPE01", "完成跃迁" },
    { CLIENT_UNKNOW_77, "CLIENT_UNKNOW_77", "来自1.0 BB服务端" },
    { CLIENT_UNKNOW_81, "CLIENT_UNKNOW_81", "来自1.0 BB服务端" },
    { SIMPLE_MAIL_TYPE, "SIMPLE_MAIL_TYPE", "客户端至服务器指令" },
    { LOBBY_LIST_TYPE, "LOBBY_LIST_TYPE", "大厅房间信息板数据包" },
    { LOBBY_CHANGE_TYPE, "LOBBY_CHANGE_TYPE", "客户端至服务器指令" },
    { LOBBY_ARROW_LIST_TYPE, "LOBBY_ARROW_LIST_TYPE", "客户端至服务器指令" },
    { LOGIN_88_TYPE, "LOGIN_88_TYPE", "DC 网络测试版数据包" },
    { LOBBY_ARROW_CHANGE_TYPE, "LOBBY_ARROW_CHANGE_TYPE", "客户端至服务器指令" },
    { LOBBY_NAME_TYPE, "LOBBY_NAME_TYPE", "客户端至服务器指令" },
    { LOGIN_8A_TYPE, "LOGIN_8A_TYPE", "DC 网络测试版数据包" },
    { LOGIN_8B_TYPE, "LOGIN_8B_TYPE", "DC 网络测试版数据包" },
    { DCNTE_CHAR_DATA_REQ_TYPE, "DCNTE_CHAR_DATA_REQ_TYPE", "DC 网络测试版数据包" },
    { DCNTE_SHIP_LIST_TYPE, "DCNTE_SHIP_LIST_TYPE", "DC 网络测试版数据包" },
    { DCNTE_BLOCK_LIST_REQ_TYPE, "DCNTE_BLOCK_LIST_REQ_TYPE", "DC 网络测试版数据包" },
    { LOGIN_90_TYPE, "LOGIN_90_TYPE", "客户端至服务器指令" },
    { CLIENT_UNKNOW_91, "CLIENT_UNKNOW_91", "来自1.0 BB服务端" },
    { LOGIN_92_TYPE, "LOGIN_92_TYPE", "客户端至服务器指令" },
    { LOGIN_93_TYPE, "LOGIN_93_TYPE", "客户端至服务器指令" },
    { CHAR_DATA_REQUEST_TYPE, "CHAR_DATA_REQUEST_TYPE", "客户端至服务器指令" },
    { CHECKSUM_TYPE, "CHECKSUM_TYPE", "客户端至服务器指令" },
    { CHECKSUM_REPLY_TYPE, "CHECKSUM_REPLY_TYPE", "客户端至服务器指令" },
    { LEAVE_GAME_PL_DATA_TYPE, "LEAVE_GAME_PL_DATA_TYPE", "客户端至服务器指令" },
    { SHIP_LIST_REQ_TYPE, "SHIP_LIST_REQ_TYPE", "客户端至服务器指令" },
    { LOGIN_9A_TYPE, "LOGIN_9A_TYPE", "客户端至服务器指令" },
    { LOGIN_9C_TYPE, "LOGIN_9C_TYPE", "客户端至服务器指令" },
    { LOGIN_9D_TYPE, "LOGIN_9D_TYPE", "客户端至服务器指令" },
    { LOGIN_9E_TYPE, "LOGIN_9E_TYPE", "客户端至服务器指令" },
    { SHIP_LIST_TYPE, "SHIP_LIST_TYPE", "客户端至服务器指令" },
    { BLOCK_LIST_REQ_TYPE, "BLOCK_LIST_REQ_TYPE", "客户端至服务器指令" },
    { QUEST_LIST_TYPE, "QUEST_LIST_TYPE", "客户端至服务器指令" },
    { QUEST_INFO_TYPE, "QUEST_INFO_TYPE", "客户端至服务器指令" },
    { DL_QUEST_LIST_TYPE, "DL_QUEST_LIST_TYPE", "客户端至服务器指令" },
    { DL_QUEST_INFO_TYPE, "DL_QUEST_INFO_TYPE", "客户端至服务器指令" },
    { DL_QUEST_FILE_TYPE, "DL_QUEST_FILE_TYPE", "客户端至服务器指令" },
    { DL_QUEST_CHUNK_TYPE, "DL_QUEST_CHUNK_TYPE", "客户端至服务器指令" },
    { QUEST_END_LIST_TYPE, "QUEST_END_LIST_TYPE", "客户端至服务器指令" },
    { QUEST_STATS_TYPE, "QUEST_STATS_TYPE", "客户端至服务器指令" },
    { QUEST_LOAD_DONE_TYPE, "QUEST_LOAD_DONE_TYPE", "客户端至服务器指令" },
    { TEXT_MSG_TYPE, "TEXT_MSG_TYPE", "边栏信息窗数据包" },
    { TIMESTAMP_TYPE, "TIMESTAMP_TYPE", "当前时间戳" },
    { PATCH_TYPE, "PATCH_TYPE", "客户端至服务器指令" },
    { PATCH_RETURN_TYPE, "PATCH_RETURN_TYPE", "客户端至服务器指令" },
    { EP3_RANK_UPDATE_TYPE, "EP3_RANK_UPDATE_TYPE", "客户端至服务器指令" },
    { EP3_CARD_UPDATE_TYPE, "EP3_CARD_UPDATE_TYPE", "客户端至服务器指令" },
    { EP3_COMMAND_TYPE, "EP3_COMMAND_TYPE", "客户端至服务器指令" },
    { CHOICE_OPTION_TYPE, "CHOICE_OPTION_TYPE", "客户端至服务器指令" },
    { GAME_CREATE_TYPE, "GAME_CREATE_TYPE", "客户端至服务器指令" },
    { CHOICE_SETTING_TYPE, "CHOICE_SETTING_TYPE", "客户端至服务器指令" },
    { CHOICE_SEARCH_TYPE, "CHOICE_SEARCH_TYPE", "客户端至服务器指令" },
    { CHOICE_REPLY_TYPE, "CHOICE_REPLY_TYPE", "客户端至服务器指令" },
    { C_RANK_TYPE, "C_RANK_TYPE", "客户端至服务器指令" },
    { BLACKLIST_TYPE, "BLACKLIST_TYPE", "客户端至服务器指令" },
    { AUTOREPLY_SET_TYPE, "AUTOREPLY_SET_TYPE", "客户端至服务器指令" },
    { AUTOREPLY_CLEAR_TYPE, "AUTOREPLY_CLEAR_TYPE", "客户端至服务器指令" },
    { GAME_COMMAND_C9_TYPE, "GAME_COMMAND_C9_TYPE", "客户端至服务器指令" },
    { EP3_SERVER_DATA_TYPE, "EP3_SERVER_DATA_TYPE", "客户端至服务器指令" },
    { GAME_COMMAND_CB_TYPE, "GAME_COMMAND_CB_TYPE", "客户端至服务器指令" },
    { TRADE_0_TYPE, "TRADE_0_TYPE", "客户端至服务器指令" },
    { TRADE_1_TYPE, "TRADE_1_TYPE", "客户端至服务器指令" },
    { TRADE_2_TYPE, "TRADE_2_TYPE", "客户端至服务器指令" },
    { TRADE_3_TYPE, "TRADE_3_TYPE", "客户端至服务器指令" },
    { TRADE_4_TYPE, "TRADE_4_TYPE", "客户端至服务器指令" },
    { GC_MSG_BOX_TYPE, "GC_MSG_BOX_TYPE", "客户端至服务器指令" },
    { GC_MSG_BOX_CLOSED_TYPE, "GC_MSG_BOX_CLOSED_TYPE", "客户端至服务器指令" },
    { GC_GBA_FILE_REQ_TYPE, "GC_GBA_FILE_REQ_TYPE", "客户端至服务器指令" },
    { INFOBOARD_TYPE, "INFOBOARD_TYPE", "客户端至服务器指令" },
    { INFOBOARD_WRITE_TYPE, "INFOBOARD_WRITE_TYPE", "客户端至服务器指令" },
    { LOBBY_EVENT_TYPE, "LOBBY_EVENT_TYPE", "大厅事件改变数据包" },
    { GC_VERIFY_LICENSE_TYPE, "GC_VERIFY_LICENSE_TYPE", "客户端至服务器指令" },
    { EP3_MENU_CHANGE_TYPE, "EP3_MENU_CHANGE_TYPE", "客户端至服务器指令" },
    { BB_GUILDCARD_HEADER_TYPE, "BB_GUILDCARD_HEADER_TYPE", "客户端至服务器指令" },
    { BB_GUILDCARD_CHUNK_TYPE, "BB_GUILDCARD_CHUNK_TYPE", "客户端至服务器指令" },
    { BB_GUILDCARD_CHUNK_REQ_TYPE, "BB_GUILDCARD_CHUNK_REQ_TYPE", "客户端至服务器指令" },
    { BB_CHALLENGE_DF, "BB_CHALLENGE_DF", "挑战模式数据" },
    { BB_CHALLENGE_01DF, "BB_CHALLENGE_01DF", "挑战模式数据" },
    { BB_CHALLENGE_02DF, "BB_CHALLENGE_02DF", "挑战模式数据" },
    { BB_CHALLENGE_03DF, "BB_CHALLENGE_03DF", "挑战模式数据" },
    { BB_CHALLENGE_04DF, "BB_CHALLENGE_04DF", "挑战模式数据" },
    { BB_CHALLENGE_05DF, "BB_CHALLENGE_05DF", "挑战模式数据" },
    { BB_CHALLENGE_06DF, "BB_CHALLENGE_06DF", "挑战模式数据" },
    { BB_OPTION_REQUEST_TYPE, "BB_OPTION_REQUEST_TYPE", "客户端至服务器指令" },
    { BB_OPTION_CONFIG_TYPE, "BB_OPTION_CONFIG_TYPE", "客户端至服务器指令" },
    { BB_CHARACTER_SELECT_TYPE, "BB_CHARACTER_SELECT_TYPE", "客户端至服务器指令" },
    { BB_CHARACTER_ACK_TYPE, "BB_CHARACTER_ACK_TYPE", "客户端至服务器指令" },
    { BB_CHARACTER_UPDATE_TYPE, "BB_CHARACTER_UPDATE_TYPE", "客户端至服务器指令" },
    { BB_SECURITY_TYPE, "BB_SECURITY_TYPE", "BB 安全数据包" },
    { BB_FULL_CHARACTER_TYPE, "BB_FULL_CHARACTER_TYPE", "客户端至服务器指令" },
    { BB_GUILD_CHECKSUM_TYPE, "BB_GUILD_CHECKSUM_TYPE", "来自1.0 BB服务端 单指令 以下都是组合指令" },
    { BB_CHECKSUM_TYPE, "BB_CHECKSUM_TYPE", "客户端至服务器指令" },
    { BB_CHECKSUM_ACK_TYPE, "BB_CHECKSUM_ACK_TYPE", "客户端至服务器指令" },
    { BB_GUILD_REQUEST_TYPE, "BB_GUILD_REQUEST_TYPE", "客户端至服务器指令" },
    { BB_ADD_GUILDCARD_TYPE, "BB_ADD_GUILDCARD_TYPE", "客户端至服务器指令" },
    { BB_DEL_GUILDCARD_TYPE, "BB_DEL_GUILDCARD_TYPE", "客户端至服务器指令" },
    { BB_SET_GUILDCARD_TEXT_TYPE, "BB_SET_GUILDCARD_TEXT_TYPE", "客户端至服务器指令" },
    { BB_ADD_BLOCKED_USER_TYPE, "BB_ADD_BLOCKED_USER_TYPE", "客户端至服务器指令" },
    { BB_DEL_BLOCKED_USER_TYPE, "BB_DEL_BLOCKED_USER_TYPE", "客户端至服务器指令" },
    { BB_SET_GUILDCARD_COMMENT_TYPE, "BB_SET_GUILDCARD_COMMENT_TYPE", "客户端至服务器指令" },
    { BB_SORT_GUILDCARD_TYPE, "BB_SORT_GUILDCARD_TYPE", "客户端至服务器指令" },
    { BB_GUILD_COMMAND, "BB_GUILD_COMMAND", "0x00EA" },
    { BB_GUILD_CREATE, "BB_GUILD_CREATE", "0x01EA" },
    { BB_GUILD_UNK_02EA, "BB_GUILD_UNK_02EA", "0x02EA" },
    { BB_GUILD_MEMBER_ADD, "BB_GUILD_MEMBER_ADD", "0x03EA" },
    { BB_GUILD_UNK_04EA, "BB_GUILD_UNK_04EA", "0x04EA" },
    { BB_GUILD_MEMBER_REMOVE, "BB_GUILD_MEMBER_REMOVE", "0x05EA" },
    { BB_GUILD_UNK_06EA, "BB_GUILD_UNK_06EA", "0x06EA" },
    { BB_GUILD_CHAT, "BB_GUILD_CHAT", "0x07EA" },
    { BB_GUILD_MEMBER_SETTING, "BB_GUILD_MEMBER_SETTING", "0x08EA" },
    { BB_GUILD_UNK_09EA, "BB_GUILD_UNK_09EA", "0x09EA" },
    { BB_GUILD_UNK_0AEA, "BB_GUILD_UNK_0AEA", "0x0AEA" },
    { BB_GUILD_UNK_0BEA, "BB_GUILD_UNK_0BEA", "0x0BEA" },
    { BB_GUILD_UNK_0CEA, "BB_GUILD_UNK_0CEA", "0x0CEA" },
    { BB_GUILD_INVITE, "BB_GUILD_INVITE", "0x0DEA" },
    { BB_GUILD_UNK_0EEA, "BB_GUILD_UNK_0EEA", "0x0EEA" },
    { BB_GUILD_MEMBER_FLAG_SETTING, "BB_GUILD_MEMBER_FLAG_SETTING", "0x0FEA" },
    { BB_GUILD_DISSOLVE, "BB_GUILD_DISSOLVE", "0x10EA" },
    { BB_GUILD_MEMBER_PROMOTE, "BB_GUILD_MEMBER_PROMOTE", "0x11EA" },
    { BB_GUILD_UNK_12EA, "BB_GUILD_UNK_12EA", "0x12EA" },
    { BB_GUILD_LOBBY_SETTING, "BB_GUILD_LOBBY_SETTING", "0x13EA" },
    { BB_GUILD_MEMBER_TITLE, "BB_GUILD_MEMBER_TITLE", "0x14EA" },
    { BB_GUILD_FULL_DATA, "BB_GUILD_FULL_DATA", "0x15EA" },
    { BB_GUILD_UNK_16EA, "BB_GUILD_UNK_16EA", "0x16EA" },
    { BB_GUILD_UNK_17EA, "BB_GUILD_UNK_17EA", "0x17EA" },
    { BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO, "BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO", "0x18EA" },
    { BB_GUILD_PRIVILEGE_LIST, "BB_GUILD_PRIVILEGE_LIST", "0x19EA" },
    { BB_GUILD_UNK_1AEA, "BB_GUILD_UNK_1AEA", "0x1AEA" },
    { BB_GUILD_UNK_1BEA, "BB_GUILD_UNK_1BEA", "0x1BEA" },
    { BB_GUILD_RANKING_LIST, "BB_GUILD_RANKING_LIST", "0x1CEA" },
    { BB_PARAM_SEND_TYPE, "BB_PARAM_SEND_TYPE", "来自1.0 BB服务端 单指令 以下都是组合指令" },
    { BB_PARAM_HEADER_TYPE, "BB_PARAM_HEADER_TYPE", "客户端至服务器指令" },
    { BB_PARAM_CHUNK_TYPE, "BB_PARAM_CHUNK_TYPE", "客户端至服务器指令" },
    { BB_PARAM_CHUNK_REQ_TYPE, "BB_PARAM_CHUNK_REQ_TYPE", "客户端至服务器指令" },
    { BB_PARAM_HEADER_REQ_TYPE, "BB_PARAM_HEADER_REQ_TYPE", "客户端至服务器指令" },
    { EP3_GAME_CREATE_TYPE, "EP3_GAME_CREATE_TYPE", "客户端至服务器指令" },
    { BB_SETFLAG_TYPE, "BB_SETFLAG_TYPE", "客户端至服务器指令" },
    { BB_UPDATE_OPTION, "BB_UPDATE_OPTION", "来自1.0 BB服务端 单指令 以下都是组合指令" },
    { BB_UPDATE_OPTION_FLAGS, "BB_UPDATE_OPTION_FLAGS", "客户端至服务器指令" },
    { BB_UPDATE_SYMBOL_CHAT, "BB_UPDATE_SYMBOL_CHAT", "客户端至服务器指令" },
    { BB_UPDATE_SHORTCUTS, "BB_UPDATE_SHORTCUTS", "客户端至服务器指令" },
    { BB_UPDATE_KEY_CONFIG, "BB_UPDATE_KEY_CONFIG", "客户端至服务器指令" },
    { BB_UPDATE_PAD_CONFIG, "BB_UPDATE_PAD_CONFIG", "客户端至服务器指令" },
    { BB_UPDATE_TECH_MENU, "BB_UPDATE_TECH_MENU", "客户端至服务器指令" },
    { BB_UPDATE_CONFIG, "BB_UPDATE_CONFIG", "客户端至服务器指令" },
    { BB_UPDATE_C_MODE_CONFIG, "BB_UPDATE_C_MODE_CONFIG", "来自1.0 BB服务端" },
    { BB_SCROLL_MSG_TYPE, "BB_SCROLL_MSG_TYPE", "顶部公告数据包" },
    { DC_WELCOME_LENGTH, "DC_WELCOME_LENGTH", "客户端至服务器指令" },
    { BB_WELCOME_LENGTH, "BB_WELCOME_LENGTH", "客户端至服务器指令" },
    { BB_SECURITY_LENGTH, "BB_SECURITY_LENGTH", "客户端至服务器指令" },
    { DC_REDIRECT_LENGTH, "DC_REDIRECT_LENGTH", "客户端至服务器指令" },
    { BB_REDIRECT_LENGTH, "BB_REDIRECT_LENGTH", "客户端至服务器指令" },
    { DC_REDIRECT6_LENGTH, "DC_REDIRECT6_LENGTH", "客户端至服务器指令" },
    { BB_REDIRECT6_LENGTH, "BB_REDIRECT6_LENGTH", "客户端至服务器指令" },
    { DC_TIMESTAMP_LENGTH, "DC_TIMESTAMP_LENGTH", "客户端至服务器指令" },
    { BB_TIMESTAMP_LENGTH, "BB_TIMESTAMP_LENGTH", "客户端至服务器指令" },
    { DC_LOBBY_LIST_LENGTH, "DC_LOBBY_LIST_LENGTH", "客户端至服务器指令" },
    { EP3_LOBBY_LIST_LENGTH, "EP3_LOBBY_LIST_LENGTH", "客户端至服务器指令" },
    { BB_LOBBY_LIST_LENGTH, "BB_LOBBY_LIST_LENGTH", "客户端至服务器指令" },
    { DC_CHAR_DATA_LENGTH, "DC_CHAR_DATA_LENGTH", "客户端至服务器指令" },
    { DC_LOBBY_LEAVE_LENGTH, "DC_LOBBY_LEAVE_LENGTH", "客户端至服务器指令" },
    { BB_LOBBY_LEAVE_LENGTH, "BB_LOBBY_LEAVE_LENGTH", "客户端至服务器指令" },
    { DC_GUILD_REPLY_LENGTH, "DC_GUILD_REPLY_LENGTH", "客户端至服务器指令" },
    { PC_GUILD_REPLY_LENGTH, "PC_GUILD_REPLY_LENGTH", "客户端至服务器指令" },
    { BB_GUILD_REPLY_LENGTH, "BB_GUILD_REPLY_LENGTH", "客户端至服务器指令" },
    { DC_GUILD_REPLY6_LENGTH, "DC_GUILD_REPLY6_LENGTH", "客户端至服务器指令" },
    { PC_GUILD_REPLY6_LENGTH, "PC_GUILD_REPLY6_LENGTH", "客户端至服务器指令" },
    { BB_GUILD_REPLY6_LENGTH, "BB_GUILD_REPLY6_LENGTH", "客户端至服务器指令" },
    { DC_GAME_JOIN_LENGTH, "DC_GAME_JOIN_LENGTH", "客户端至服务器指令" },
    { GC_GAME_JOIN_LENGTH, "GC_GAME_JOIN_LENGTH", "客户端至服务器指令" },
    { EP3_GAME_JOIN_LENGTH, "EP3_GAME_JOIN_LENGTH", "客户端至服务器指令" },
    { DC_QUEST_INFO_LENGTH, "DC_QUEST_INFO_LENGTH", "客户端至服务器指令" },
    { PC_QUEST_INFO_LENGTH, "PC_QUEST_INFO_LENGTH", "客户端至服务器指令" },
    { BB_QUEST_INFO_LENGTH, "BB_QUEST_INFO_LENGTH", "客户端至服务器指令" },
    { DC_QUEST_FILE_LENGTH, "DC_QUEST_FILE_LENGTH", "客户端至服务器指令" },
    { BB_QUEST_FILE_LENGTH, "BB_QUEST_FILE_LENGTH", "客户端至服务器指令" },
    { DC_QUEST_CHUNK_LENGTH, "DC_QUEST_CHUNK_LENGTH", "客户端至服务器指令" },
    { BB_QUEST_CHUNK_LENGTH, "BB_QUEST_CHUNK_LENGTH", "客户端至服务器指令" },
    { DC_SIMPLE_MAIL_LENGTH, "DC_SIMPLE_MAIL_LENGTH", "客户端至服务器指令" },
    { PC_SIMPLE_MAIL_LENGTH, "PC_SIMPLE_MAIL_LENGTH", "客户端至服务器指令" },
    { BB_SIMPLE_MAIL_LENGTH, "BB_SIMPLE_MAIL_LENGTH", "客户端至服务器指令" },
    { BB_OPTION_CONFIG_LENGTH, "BB_OPTION_CONFIG_LENGTH", "客户端至服务器指令" },
    { BB_FULL_CHARACTER_LENGTH, "BB_FULL_CHARACTER_LENGTH", "客户端至服务器指令" },
    { BB_FULL_CHARACTER_LENGTH, "BB_FULL_CHARACTER_LENGTH", "客户端至服务器指令" },
    { DC_PATCH_HEADER_LENGTH, "DC_PATCH_HEADER_LENGTH", "客户端至服务器指令" },
    { DC_PATCH_FOOTER_LENGTH, "DC_PATCH_FOOTER_LENGTH", "客户端至服务器指令" },
    { NO_SUCH_CMD, "NO_SUCH_CMD", "指令不存在" }
};

static cmd_map_st s_cmd_names[] = {
    { SHDR_RESPONSE, "SHDR_RESPONSE", "Response to a request 32768" },
    { SHDR_FAILURE, "SHDR_FAILURE", "Failure to complete request 16384" },
    { SHDR_TYPE_DC, "SHDR_TYPE_DC", "A decrypted Dreamcast game packet 1" },
    { SHDR_TYPE_BB, "SHDR_TYPE_BB", "A decrypted Blue Burst game packet 2" },
    { SHDR_TYPE_PC, "SHDR_TYPE_PC", "A decrypted PCv2 game packet 3" },
    { SHDR_TYPE_GC, "SHDR_TYPE_GC", "A decrypted Gamecube game packet 4" },
    { SHDR_TYPE_EP3, "SHDR_TYPE_EP3", "A decrypted Episode 3 packet 5" },
    { SHDR_TYPE_XBOX, "SHDR_TYPE_XBOX", "A decrypted Xbox game packet 6" },
    { SHDR_TYPE_LOGIN, "SHDR_TYPE_LOGIN", "Shipgate hello packet 16" },
    { SHDR_TYPE_COUNT, "SHDR_TYPE_COUNT", "A Client/Game Count update 17" },
    { SHDR_TYPE_SSTATUS, "SHDR_TYPE_SSTATUS", "A Ship has come up or gone down 18" },
    { SHDR_TYPE_PING, "SHDR_TYPE_PING", "A Ping packet, enough said 19" },
    { SHDR_TYPE_CDATA, "SHDR_TYPE_CDATA", "Character data 20" },
    { SHDR_TYPE_CREQ, "SHDR_TYPE_CREQ", "Request saved character data 21" },
    { SHDR_TYPE_USRLOGIN, "SHDR_TYPE_USRLOGIN", "User login request 22" },
    { SHDR_TYPE_GCBAN, "SHDR_TYPE_GCBAN", "Guildcard ban 23" },
    { SHDR_TYPE_IPBAN, "SHDR_TYPE_IPBAN", "IP ban 24" },
    { SHDR_TYPE_BLKLOGIN, "SHDR_TYPE_BLKLOGIN", "User logs into a block 25" },
    { SHDR_TYPE_BLKLOGOUT, "SHDR_TYPE_BLKLOGOUT", "User logs off a block 26" },
    { SHDR_TYPE_FRLOGIN, "SHDR_TYPE_FRLOGIN", "A user's friend logs onto a block 27" },
    { SHDR_TYPE_FRLOGOUT, "SHDR_TYPE_FRLOGOUT", "A user's friend logs off a block 28" },
    { SHDR_TYPE_ADDFRIEND, "SHDR_TYPE_ADDFRIEND", "Add a friend to a user's list 29" },
    { SHDR_TYPE_DELFRIEND, "SHDR_TYPE_DELFRIEND", "Remove a friend from a user's list 30" },
    { SHDR_TYPE_LOBBYCHG, "SHDR_TYPE_LOBBYCHG", "A user changes lobbies 31" },
    { SHDR_TYPE_BCLIENTS, "SHDR_TYPE_BCLIENTS", "A bulk transfer of client info 32" },
    { SHDR_TYPE_KICK, "SHDR_TYPE_KICK", "A kick request 33" },
    { SHDR_TYPE_FRLIST, "SHDR_TYPE_FRLIST", "Friend list request/reply 34" },
    { SHDR_TYPE_GLOBALMSG, "SHDR_TYPE_GLOBALMSG", "A Global message packet 35" },
    { SHDR_TYPE_USEROPT, "SHDR_TYPE_USEROPT", "A user's options -- sent on login 36" },
    { SHDR_TYPE_LOGIN6, "SHDR_TYPE_LOGIN6", "A ship login (potentially IPv6) 37" },
    { SHDR_TYPE_BBOPTS, "SHDR_TYPE_BBOPTS", "A user's Blue Burst options 38" },
    { SHDR_TYPE_BBOPT_REQ, "SHDR_TYPE_BBOPT_REQ", "Request Blue Burst options 39" },
    { SHDR_TYPE_CBKUP, "SHDR_TYPE_CBKUP", "A character data backup packet 40" },
    { SHDR_TYPE_MKILL, "SHDR_TYPE_MKILL", "Monster kill update 41" },
    { SHDR_TYPE_TLOGIN, "SHDR_TYPE_TLOGIN", "Token-based login request 42" },
    { SHDR_TYPE_SCHUNK, "SHDR_TYPE_SCHUNK", "Script chunk 43" },
    { SHDR_TYPE_SDATA, "SHDR_TYPE_SDATA", "Script data 44" },
    { SHDR_TYPE_SSET, "SHDR_TYPE_SSET", "Script set 45" },
    { SHDR_TYPE_QFLAG_SET, "SHDR_TYPE_QFLAG_SET", "Set quest flag 46" },
    { SHDR_TYPE_QFLAG_GET, "SHDR_TYPE_QFLAG_GET", "Read quest flag 47" },
    { SHDR_TYPE_SHIP_CTL, "SHDR_TYPE_SHIP_CTL", "Ship control packet 48" },
    { SHDR_TYPE_UBLOCKS, "SHDR_TYPE_UBLOCKS", "User blocklist 49" },
    { SHDR_TYPE_UBL_ADD, "SHDR_TYPE_UBL_ADD", "User blocklist add 50" },
    { NO_SUCH_CMD, "NO_SUCH_CMD", "指令不存在" }
};

/* 通过代码对比获取指令名称 */
extern const char* c_cmd_name(uint16_t cmd, int32_t version);
extern const char* c_subcmd_name(uint16_t cmd, int32_t version);
extern const char* s_cmd_name(uint16_t cmd, int32_t version);
