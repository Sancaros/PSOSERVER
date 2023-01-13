/*
    梦幻之星中国 设置文件.
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

#ifndef PSOCN_CONFIG_H
#define PSOCN_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "queue.h"

//#define SIZEOF_VOID_P 4
#define SOCKET_ERR(err, s) if(err==-1){perror(s);closesocket(err);return(-1);}
#define LOOP_CHECK(rval, cmd) do{rval = cmd;} while(rval == GNUTLS_E_AGAIN || rval == GNUTLS_E_INTERRUPTED);/* assert(rval >= 0)*/

#ifndef DATAROOTDIR
#define DATAROOTDIR "config\\"
#endif

#define PSOCN_DIRECTORY DATAROOTDIR ""

static const char psocn_directory[] = PSOCN_DIRECTORY;
static const char psocn_global_cfg[] = PSOCN_DIRECTORY "psocn_config_global.xml";
static const char psocn_database_cfg[] = PSOCN_DIRECTORY "psocn_config_database.xml";
static const char psocn_server_cfg[] = PSOCN_DIRECTORY "psocn_config_server.xml";
static const char psocn_patch_cfg[] = PSOCN_DIRECTORY "psocn_config_patch.xml";
static const char psocn_ship_cfg[] = PSOCN_DIRECTORY "psocn_config_ship.xml";

static const char* Welcome_Files[] = {
    "Motd\\Patch_Welcome.txt",
    "Motd\\Login_Welcome.txt",
};

typedef const enum c_cmd_map_val {
    msg1_type = 0x0001,//右下角弹窗信息数据包
    welcome_type = 0x0002,//"欢迎信息" 数据包
    bb_welcome_type = 0x0003,//Start Encryption Packet 开始加密数据包
    security_type = 0x0004,
    bursting_type = 0x0005,
    chat_type = 0x0006,
    block_list_type = 0x0007,//大厅数据包
    game_list_type = 0x0008,//游戏房间数据包
    info_request_type = 0x0009,
    dc_game_create_type = 0x000C,
    menu_select_type = 0x0010,
    info_reply_type = 0x0011,//左下角弹窗信息框数据包
    client_unknow_12 = 0x0012,
    quest_chunk_type = 0x0013,
    login_welcome_type = 0x0017,
    redirect_type = 0x0019,//重定向数据包 用于连接
    msg_box_type = 0x001A,//大信息窗口文本代码
    ping_type = 0x001D,//Ping pong 心跳数据包
    lobby_info_type = 0x001F,
    guild_search_type = 0x0040,
    guild_reply_type = 0x0041,
    quest_file_type = 0x0044,
    client_unknow_4f = 0x004F,
    game_command0_type = 0x0060,//来自1.0 BB服务端
    char_data_type = 0x0061,
    game_command2_type = 0x0062,//来自1.0 BB服务端
    game_join_type = 0x0064,
    game_add_player_type = 0x0065,
    game_leave_type = 0x0066,//用户离开游戏房间数据包
    lobby_join_type = 0x0067,//用户进入大厅数据包
    lobby_add_player_type = 0x0068,//大厅新增用户数据包
    lobby_leave_type = 0x0069,//用户离开大厅数据包
    client_unknow_6a = 0x006A,//来自1.0 BB服务端
    game_commandc_type = 0x006C,
    game_commandd_type = 0x006D,//创建房间指令
    done_bursting_type = 0x006F,
    done_bursting_type01 = 0x016F,
    client_unknow_77 = 0x0077,//来自1.0 BB服务端
    client_unknow_81 = 0x0080,//来自1.0 BB服务端
    simple_mail_type = 0x0081,
    lobby_list_type = 0x0083,//大厅房间信息板数据包
    lobby_change_type = 0x0084,
    lobby_arrow_list_type = 0x0088,
    login_88_type = 0x0088,/* DC 网络测试版数据包 */
    lobby_arrow_change_type = 0x0089,
    lobby_name_type = 0x008A,
    login_8a_type = 0x008A,/* DC 网络测试版数据包 */
    login_8b_type = 0x008B,/* DC 网络测试版数据包 */
    dcnte_char_data_req_type = 0x008D,/* DC 网络测试版数据包 */
    dcnte_ship_list_type = 0x008E,/* DC 网络测试版数据包 */
    dcnte_block_list_req_type = 0x008F,/* DC 网络测试版数据包 */
    login_90_type = 0x0090,
    client_unknow_91 = 0x0091,//来自1.0 BB服务端
    login_92_type = 0x0092,
    login_93_type = 0x0093,
    char_data_request_type = 0x0095,
    checksum_type = 0x0096,
    checksum_reply_type = 0x0097,
    leave_game_pl_data_type = 0x0098,
    ship_list_req_type = 0x0099,
    login_9a_type = 0x009A,
    login_9c_type = 0x009C,
    login_9d_type = 0x009D,
    login_9e_type = 0x009E,
    ship_list_type = 0x00A0,
    block_list_req_type = 0x00A1,
    quest_list_type = 0x00A2,
    quest_info_type = 0x00A3,
    dl_quest_list_type = 0x00A4,
    dl_quest_info_type = 0x00A5,
    dl_quest_file_type = 0x00A6,
    dl_quest_chunk_type = 0x00A7,
    quest_end_list_type = 0x00A9,
    quest_stats_type = 0x00AA,
    quest_load_done_type = 0x00AC,
    text_msg_type = 0x00B0,//边栏信息窗数据包
    timestamp_type = 0x00B1,//当前时间戳
    patch_type = 0x00B2,
    patch_return_type = 0x00B3,
    ep3_rank_update_type = 0x00B7,
    ep3_card_update_type = 0x00B8,
    ep3_command_type = 0x00BA,
    choice_option_type = 0x00C0,
    game_create_type = 0x00C1,
    choice_setting_type = 0x00C2,
    choice_search_type = 0x00C3,
    choice_reply_type = 0x00C4,
    c_rank_type = 0x00C5,
    blacklist_type = 0x00C6,
    autoreply_set_type = 0x00C7,
    autoreply_clear_type = 0x00C8,
    game_command_c9_type = 0x00C9,
    ep3_server_data_type = 0x00CA,
    game_command_cb_type = 0x00CB,
    trade_0_type = 0x00D0,
    trade_1_type = 0x00D1,
    trade_2_type = 0x00D2,
    trade_3_type = 0x00D3,
    trade_4_type = 0x00D4,
    trade_5_type = 0x00D5,
    gc_msg_box_type = 0x00D5,
    gc_msg_box_closed_type = 0x00D6,
    gc_gba_file_req_type = 0x00D7,
    infoboard_type = 0x00D8,
    infoboard_write_type = 0x00D9,
    lobby_event_type = 0x00DA,//大厅事件改变数据包
    gc_verify_license_type = 0x00DB,
    ep3_menu_change_type = 0x00DC,
    bb_guildcard_header_type = 0x01DC,
    bb_guildcard_chunk_type = 0x02DC,
    bb_guildcard_chunk_req_type = 0x03DC,
    bb_challenge_df = 0x00DF,
        bb_challenge_01df = 0x01DF,
        bb_challenge_02df = 0x02DF,
        bb_challenge_03df = 0x03DF,
        bb_challenge_04df = 0x04DF,
        bb_challenge_05df = 0x05DF,
        bb_challenge_06df = 0x06DF,
    bb_option_request_type = 0x00E0,
    bb_option_config_type = 0x00E2,
    bb_character_select_type = 0x00E3,
    bb_character_ack_type = 0x00E4,
    bb_character_update_type = 0x00E5,
    bb_security_type = 0x00E6,//BB 安全数据包
    bb_full_character_type = 0x00E7,
    bb_guild_checksum_type = 0x00E8,//来自1.0 BB服务端 单指令 以下都是组合指令
    /////////////////////////////////=//////,////确认客户端的预期校验和
    bb_checksum_type = 0x01E8,
    bb_checksum_ack_type = 0x02E8,
    bb_guild_request_type = 0x03E8,
    bb_add_guildcard_type = 0x04E8,
    bb_del_guildcard_type = 0x05E8,
    bb_set_guildcard_text_type = 0x06E8,
    bb_add_blocked_user_type = 0x07E8,
    bb_del_blocked_user_type = 0x08E8,
    bb_set_guildcard_comment_type = 0x09E8,
    bb_sort_guildcard_type = 0x0AE8,
    //////////////////////////////////////////////////EA指令
    bb_guild_command = 0x00EA,
    bb_guild_create = 0x01EA,
    bb_guild_unk_02ea = 0x02EA,
    bb_guild_member_add = 0x03EA,
    bb_guild_unk_04ea = 0x04EA,
    bb_guild_member_remove = 0x05EA,
    bb_guild_unk_06ea = 0x06EA,
    bb_guild_member_chat = 0x07EA,
    bb_guild_member_setting = 0x08EA,
    bb_guild_unk_09ea = 0x09EA,
    bb_guild_unk_0aea = 0x0AEA,
    bb_guild_unk_0bea = 0x0BEA,
    bb_guild_unk_0cea = 0x0CEA,
    bb_guild_unk_0dea = 0x0DEA,
    bb_guild_unk_0eea = 0x0EEA,
    bb_guild_member_flag_setting = 0x0FEA,
    bb_guild_dissolve = 0x10EA,
    bb_guild_member_promote = 0x11EA,
    bb_guild_unk_12ea = 0x12EA,
    bb_guild_lobby_setting = 0x13EA,
    bb_guild_member_title = 0x14EA,
    bb_guild_unk_15ea = 0x15EA,
    bb_guild_unk_16ea = 0x16EA,
    bb_guild_unk_17ea = 0x17EA,
    bb_guild_buy_privilege_and_point_info = 0x18EA,
    bb_guild_privilege_list = 0x19EA,
    bb_guild_unk_1aea = 0x1AEA,
    bb_guild_unk_1bea = 0x1BEA,
    bb_guild_ranking_list = 0x1CEA,
    //////////////////////////////////////////////////EB指令
    bb_param_send_type = 0x00EB,//来自1.0 BB服务端 单指令 以下都是组合指令
    bb_param_header_type = 0x01EB,
    bb_param_chunk_type = 0x02EB,
    bb_param_chunk_req_type = 0x03EB,
    bb_param_header_req_type = 0x04EB,
    //////////////////////////////////////////////////EC指令
    ep3_game_create_type = 0x00EC,
    bb_setflag_type = 0x00EC,
    //////////////////////////////////////////////////ED指令
    bb_update_option = 0x00ED,//来自1.0 BB服务端 单指令 以下都是组合指令
    bb_update_option_flags = 0x01ED,
    bb_update_symbol_chat = 0x02ED,
    bb_update_shortcuts = 0x03ED,
    bb_update_key_config = 0x04ED,
    bb_update_pad_config = 0x05ED,
    bb_update_tech_menu = 0x06ED,
    bb_update_config = 0x07ED,
    bb_update_config_mode = 0x08ED,//来自1.0 BB服务端
    //////////////////////////////////////////////////EE指令
    bb_scroll_msg_type = 0x00EE,//顶部公告数据包
    ///////////////////////////////////////////数据包长度
    dc_welcome_length = 0x004C,
    bb_welcome_length = 0x00C8,
    bb_security_length = 0x0044,
    dc_redirect_length = 0x000C,
    bb_redirect_length = 0x0010,
    dc_redirect6_length = 0x0018,
    bb_redirect6_length = 0x001C,
    dc_timestamp_length = 0x0020,
    bb_timestamp_length = 0x0024,
    dc_lobby_list_length = 0x00C4,
    ep3_lobby_list_length = 0x0100,
    bb_lobby_list_length = 0x00C8,
    dc_char_data_length = 0x0420,
    dc_lobby_leave_length = 0x0008,
    bb_lobby_leave_length = 0x000C,
    dc_guild_reply_length = 0x00C4,
    pc_guild_reply_length = 0x0128,
    bb_guild_reply_length = 0x0130,
    dc_guild_reply6_length = 0x00D0,
    pc_guild_reply6_length = 0x0134,
    bb_guild_reply6_length = 0x013C,
    dc_game_join_length = 0x0110,
    gc_game_join_length = 0x0114,
    ep3_game_join_length = 0x1184,
    dc_quest_info_length = 0x0128,
    pc_quest_info_length = 0x024C,
    bb_quest_info_length = 0x0250,
    dc_quest_file_length = 0x003C,
    bb_quest_file_length = 0x0058,
    dc_quest_chunk_length = 0x0418,
    bb_quest_chunk_length = 0x041C,
    dc_simple_mail_length = 0x0220,
    pc_simple_mail_length = 0x0430,
    bb_simple_mail_length = 0x045C,
    bb_option_config_length = 0x0AF8,
    //大小 14748
    //bb_full_character_length = 0x399C,
    //大小 14752
    bb_full_character_length = 0x39A0,
    dc_patch_header_length = 0x0014,
    dc_patch_footer_length = 0x0018,
    NoSuchcCmd = 0xFFFFFF

} c_cmd_map_val_tbl;

typedef const enum s_cmd_map_val {
    shdr_response = 0x8000, /*Response to a request 32768 */
    shdr_failure = 0x4000, /*Failure to complete request 16384 */
    shdr_type_dc = 0x0001, /*A decrypted Dreamcast game packet 1 */
    shdr_type_bb = 0x0002, /*A decrypted Blue Burst game packet 2 */
    shdr_type_pc = 0x0003, /*A decrypted PCv2 game packet 3 */
    shdr_type_gc = 0x0004, /*A decrypted Gamecube game packet 4 */
    shdr_type_ep3 = 0x0005, /*A decrypted Episode 3 packet 5 */
    shdr_type_xbox = 0x0006, /*A decrypted Xbox game packet 6 */
    shdr_type_login = 0x0010, /*Shipgate hello packet 16 */
    shdr_type_count = 0x0011, /*A Client/Game Count update 17 */
    shdr_type_sstatus = 0x0012, /*A Ship has come up or gone down 18 */
    shdr_type_ping = 0x0013, /*A Ping packet, enough said 19 */
    shdr_type_cdata = 0x0014, /*Character data 20 */
    shdr_type_creq = 0x0015, /*Request saved character data 21 */
    shdr_type_usrlogin = 0x0016, /*User login request 22 */
    shdr_type_gcban = 0x0017, /*Guildcard ban 23 */
    shdr_type_ipban = 0x0018, /*IP ban 24 */
    shdr_type_blklogin = 0x0019, /*User logs into a block 25 */
    shdr_type_blklogout = 0x001A, /*User logs off a block 26 */
    shdr_type_frlogin = 0x001B, /*A user's friend logs onto a block 27 */
    shdr_type_frlogout = 0x001C, /*A user's friend logs off a block 28 */
    shdr_type_addfriend = 0x001D, /*Add a friend to a user's list 29 */
    shdr_type_delfriend = 0x001E, /*Remove a friend from a user's list 30 */
    shdr_type_lobbychg = 0x001F, /*A user changes lobbies 31 */
    shdr_type_bclients = 0x0020, /*A bulk transfer of client info 32 */
    shdr_type_kick = 0x0021, /*A kick request 33 */
    shdr_type_frlist = 0x0022, /*Friend list request/reply 34 */
    shdr_type_globalmsg = 0x0023, /*A Global message packet 35 */
    shdr_type_useropt = 0x0024, /*A user's options -- sent on login 36 */
    shdr_type_login6 = 0x0025, /*A ship login (potentially IPv6) 37 */
    shdr_type_bbopts = 0x0026, /*A user's Blue Burst options 38 */
    shdr_type_bbopt_req = 0x0027, /*Request Blue Burst options 39 */
    shdr_type_cbkup = 0x0028, /*A character data backup packet 40 */
    shdr_type_mkill = 0x0029, /*Monster kill update 41 */
    shdr_type_tlogin = 0x002A, /*Token-based login request 42 */
    shdr_type_schunk = 0x002B, /*Script chunk 43 */
    shdr_type_sdata = 0x002C, /*Script data 44 */
    shdr_type_sset = 0x002D, /*Script set 45 */
    shdr_type_qflag_set = 0x002E, /*Set quest flag 46 */
    shdr_type_qflag_get = 0x002F, /*Read quest flag 47 */
    shdr_type_ship_ctl = 0x0030, /*Ship control packet 48 */
    shdr_type_ublocks = 0x0031, /*User blocklist 49 */
    shdr_type_ubl_add = 0x0032, /*User blocklist add 50 */
    shdr_type_bbguild = 0x0033, /*A user's Blue Burst guild data 51 */
    shdr_type_bbguild_req = 0x0034, /*Request Blue Burst guild data 52 */
    NoSuchsCmd = 0xFFFFFF
} s_cmd_map_val_tbl;

/* client conmand code to string mapping */
typedef struct c_cmd_map {
    c_cmd_map_val_tbl cmd;
    const char* name;
    const char* cnname;
} c_cmd_map_st;

/* client conmand code to string mapping */
typedef struct s_cmd_map {
    s_cmd_map_val_tbl cmd;
    const char* name;
    const char* cnname;
} s_cmd_map_st;

static c_cmd_map_st c_cmd_names[] = {
    { msg1_type, "MSG1_TYPE", "消息指令" },
    { welcome_type, "WELCOME_TYPE", "欢迎指令" },
    { bb_welcome_type, "BB_WELCOME_TYPE", "欢迎指令" },
    { security_type, "SECURITY_TYPE", "安全指令" },
    { bursting_type, "BURSTING_TYPE", "跃迁指令" },
    { chat_type, "CHAT_TYPE", "聊天指令" },
    { block_list_type, "BLOCK_LIST_TYPE", "舰仓列表" },
    { game_list_type, "GAME_LIST_TYPE", "游戏列表" },
    { info_request_type, "INFO_REQUEST_TYPE", "信息请求指令" },
    { dc_game_create_type, "DC_GAME_CREATE_TYPE", "DC游戏创建" },
    { menu_select_type, "MENU_SELECT_TYPE", "客户端至服务器指令" },
    { info_reply_type, "INFO_REPLY_TYPE", "左下角弹窗信息框数据包" },
    { client_unknow_12, "CLIENT_UNKNOW_12", "客户端至服务器指令" },
    { quest_chunk_type, "QUEST_CHUNK_TYPE", "客户端至服务器指令" },
    { login_welcome_type, "LOGIN_WELCOME_TYPE", "客户端至服务器指令" },
    { redirect_type, "REDIRECT_TYPE", "重定向数据包 用于连接" },
    { msg_box_type, "MSG_BOX_TYPE", "大信息窗口文本代码" },
    { ping_type, "PING_TYPE", "Ping pong 心跳数据包" },
    { lobby_info_type, "LOBBY_INFO_TYPE", "客户端至服务器指令" },
    { guild_search_type, "GUILD_SEARCH_TYPE", "客户端至服务器指令" },
    { guild_reply_type, "GUILD_REPLY_TYPE", "客户端至服务器指令" },
    { quest_file_type, "QUEST_FILE_TYPE", "客户端至服务器指令" },
    { client_unknow_4f, "CLIENT_UNKNOW_4F", "客户端至服务器指令" },
    { game_command0_type, "GAME_COMMAND0_TYPE", "来自1.0 BB服务端" },
    { char_data_type, "CHAR_DATA_TYPE", "客户端至服务器指令" },
    { game_command2_type, "GAME_COMMAND2_TYPE", "来自1.0 BB服务端" },
    { game_join_type, "GAME_JOIN_TYPE", "客户端至服务器指令" },
    { game_add_player_type, "GAME_ADD_PLAYER_TYPE", "客户端至服务器指令" },
    { game_leave_type, "GAME_LEAVE_TYPE", "用户离开游戏房间数据包" },
    { lobby_join_type, "LOBBY_JOIN_TYPE", "用户进入大厅数据包" },
    { lobby_add_player_type, "LOBBY_ADD_PLAYER_TYPE", "大厅新增用户数据包" },
    { lobby_leave_type, "LOBBY_LEAVE_TYPE", "用户离开大厅数据包" },
    { client_unknow_6a, "CLIENT_UNKNOW_6A", "来自1.0 BB服务端" },
    { game_commandc_type, "GAME_COMMANDC_TYPE", "客户端至服务器指令" },
    { game_commandd_type, "GAME_COMMANDD_TYPE", "创建房间指令" },
    { done_bursting_type, "DONE_BURSTING_TYPE", "正在跃迁" },
    { done_bursting_type01, "DONE_BURSTING_TYPE01", "完成跃迁" },
    { client_unknow_77, "CLIENT_UNKNOW_77", "来自1.0 BB服务端" },
    { client_unknow_81, "CLIENT_UNKNOW_81", "来自1.0 BB服务端" },
    { simple_mail_type, "SIMPLE_MAIL_TYPE", "客户端至服务器指令" },
    { lobby_list_type, "LOBBY_LIST_TYPE", "大厅房间信息板数据包" },
    { lobby_change_type, "LOBBY_CHANGE_TYPE", "客户端至服务器指令" },
    { lobby_arrow_list_type, "LOBBY_ARROW_LIST_TYPE", "客户端至服务器指令" },
    { login_88_type, "LOGIN_88_TYPE", "DC 网络测试版数据包" },
    { lobby_arrow_change_type, "LOBBY_ARROW_CHANGE_TYPE", "客户端至服务器指令" },
    { lobby_name_type, "LOBBY_NAME_TYPE", "客户端至服务器指令" },
    { login_8a_type, "LOGIN_8A_TYPE", "DC 网络测试版数据包" },
    { login_8b_type, "LOGIN_8B_TYPE", "DC 网络测试版数据包" },
    { dcnte_char_data_req_type, "DCNTE_CHAR_DATA_REQ_TYPE", "DC 网络测试版数据包" },
    { dcnte_ship_list_type, "DCNTE_SHIP_LIST_TYPE", "DC 网络测试版数据包" },
    { dcnte_block_list_req_type, "DCNTE_BLOCK_LIST_REQ_TYPE", "DC 网络测试版数据包" },
    { login_90_type, "LOGIN_90_TYPE", "客户端至服务器指令" },
    { client_unknow_91, "CLIENT_UNKNOW_91", "来自1.0 BB服务端" },
    { login_92_type, "LOGIN_92_TYPE", "客户端至服务器指令" },
    { login_93_type, "LOGIN_93_TYPE", "客户端至服务器指令" },
    { char_data_request_type, "CHAR_DATA_REQUEST_TYPE", "客户端至服务器指令" },
    { checksum_type, "CHECKSUM_TYPE", "客户端至服务器指令" },
    { checksum_reply_type, "CHECKSUM_REPLY_TYPE", "客户端至服务器指令" },
    { leave_game_pl_data_type, "LEAVE_GAME_PL_DATA_TYPE", "客户端至服务器指令" },
    { ship_list_req_type, "SHIP_LIST_REQ_TYPE", "客户端至服务器指令" },
    { login_9a_type, "LOGIN_9A_TYPE", "客户端至服务器指令" },
    { login_9c_type, "LOGIN_9C_TYPE", "客户端至服务器指令" },
    { login_9d_type, "LOGIN_9D_TYPE", "客户端至服务器指令" },
    { login_9e_type, "LOGIN_9E_TYPE", "客户端至服务器指令" },
    { ship_list_type, "SHIP_LIST_TYPE", "客户端至服务器指令" },
    { block_list_req_type, "BLOCK_LIST_REQ_TYPE", "客户端至服务器指令" },
    { quest_list_type, "QUEST_LIST_TYPE", "客户端至服务器指令" },
    { quest_info_type, "QUEST_INFO_TYPE", "客户端至服务器指令" },
    { dl_quest_list_type, "DL_QUEST_LIST_TYPE", "客户端至服务器指令" },
    { dl_quest_info_type, "DL_QUEST_INFO_TYPE", "客户端至服务器指令" },
    { dl_quest_file_type, "DL_QUEST_FILE_TYPE", "客户端至服务器指令" },
    { dl_quest_chunk_type, "DL_QUEST_CHUNK_TYPE", "客户端至服务器指令" },
    { quest_end_list_type, "QUEST_END_LIST_TYPE", "客户端至服务器指令" },
    { quest_stats_type, "QUEST_STATS_TYPE", "客户端至服务器指令" },
    { quest_load_done_type, "QUEST_LOAD_DONE_TYPE", "客户端至服务器指令" },
    { text_msg_type, "TEXT_MSG_TYPE", "边栏信息窗数据包" },
    { timestamp_type, "TIMESTAMP_TYPE", "当前时间戳" },
    { patch_type, "PATCH_TYPE", "客户端至服务器指令" },
    { patch_return_type, "PATCH_RETURN_TYPE", "客户端至服务器指令" },
    { ep3_rank_update_type, "EP3_RANK_UPDATE_TYPE", "客户端至服务器指令" },
    { ep3_card_update_type, "EP3_CARD_UPDATE_TYPE", "客户端至服务器指令" },
    { ep3_command_type, "EP3_COMMAND_TYPE", "客户端至服务器指令" },
    { choice_option_type, "CHOICE_OPTION_TYPE", "客户端至服务器指令" },
    { game_create_type, "GAME_CREATE_TYPE", "客户端至服务器指令" },
    { choice_setting_type, "CHOICE_SETTING_TYPE", "客户端至服务器指令" },
    { choice_search_type, "CHOICE_SEARCH_TYPE", "客户端至服务器指令" },
    { choice_reply_type, "CHOICE_REPLY_TYPE", "客户端至服务器指令" },
    { c_rank_type, "C_RANK_TYPE", "客户端至服务器指令" },
    { blacklist_type, "BLACKLIST_TYPE", "客户端至服务器指令" },
    { autoreply_set_type, "AUTOREPLY_SET_TYPE", "客户端至服务器指令" },
    { autoreply_clear_type, "AUTOREPLY_CLEAR_TYPE", "客户端至服务器指令" },
    { game_command_c9_type, "GAME_COMMAND_C9_TYPE", "客户端至服务器指令" },
    { ep3_server_data_type, "EP3_SERVER_DATA_TYPE", "客户端至服务器指令" },
    { game_command_cb_type, "GAME_COMMAND_CB_TYPE", "客户端至服务器指令" },
    { trade_0_type, "TRADE_0_TYPE", "交易指令" },
    { trade_1_type, "TRADE_1_TYPE", "交易指令" },
    { trade_2_type, "TRADE_2_TYPE", "交易指令" },
    { trade_3_type, "TRADE_3_TYPE", "交易指令" },
    { trade_4_type, "TRADE_4_TYPE", "交易指令" },
    { trade_5_type, "TRADE_5_TYPE", "交易指令" },
    { gc_msg_box_type, "GC_MSG_BOX_TYPE", "客户端至服务器指令" },
    { gc_msg_box_closed_type, "GC_MSG_BOX_CLOSED_TYPE", "客户端至服务器指令" },
    { gc_gba_file_req_type, "GC_GBA_FILE_REQ_TYPE", "客户端至服务器指令" },
    { infoboard_type, "INFOBOARD_TYPE", "客户端至服务器指令" },
    { infoboard_write_type, "INFOBOARD_WRITE_TYPE", "客户端至服务器指令" },
    { lobby_event_type, "LOBBY_EVENT_TYPE", "大厅事件改变数据包" },
    { gc_verify_license_type, "GC_VERIFY_LICENSE_TYPE", "客户端至服务器指令" },
    { ep3_menu_change_type, "EP3_MENU_CHANGE_TYPE", "客户端至服务器指令" },
    { bb_guildcard_header_type, "BB_GUILDCARD_HEADER_TYPE", "客户端至服务器指令" },
    { bb_guildcard_chunk_type, "BB_GUILDCARD_CHUNK_TYPE", "客户端至服务器指令" },
    { bb_guildcard_chunk_req_type, "BB_GUILDCARD_CHUNK_REQ_TYPE", "客户端至服务器指令" },
    { bb_challenge_df, "BB_CHALLENGE_DF", "挑战模式数据" },
    { bb_challenge_01df, "BB_CHALLENGE_01DF", "挑战模式数据" },
    { bb_challenge_02df, "BB_CHALLENGE_02DF", "挑战模式数据" },
    { bb_challenge_03df, "BB_CHALLENGE_03DF", "挑战模式数据" },
    { bb_challenge_04df, "BB_CHALLENGE_04DF", "挑战模式数据" },
    { bb_challenge_05df, "BB_CHALLENGE_05DF", "挑战模式数据" },
    { bb_challenge_06df, "BB_CHALLENGE_06DF", "挑战模式数据" },
    { bb_option_request_type, "BB_OPTION_REQUEST_TYPE", "客户端至服务器指令" },
    { bb_option_config_type, "BB_OPTION_CONFIG_TYPE", "客户端至服务器指令" },
    { bb_character_select_type, "BB_CHARACTER_SELECT_TYPE", "客户端至服务器指令" },
    { bb_character_ack_type, "BB_CHARACTER_ACK_TYPE", "客户端至服务器指令" },
    { bb_character_update_type, "BB_CHARACTER_UPDATE_TYPE", "客户端至服务器指令" },
    { bb_security_type, "BB_SECURITY_TYPE", "BB 安全数据包" },
    { bb_full_character_type, "BB_FULL_CHARACTER_TYPE", "客户端至服务器指令" },
    { bb_guild_checksum_type, "BB_GUILD_CHECKSUM_TYPE", "来自1.0 BB服务端 单指令 以下都是组合指令" },
    { bb_checksum_type, "BB_CHECKSUM_TYPE", "客户端至服务器指令" },
    { bb_checksum_ack_type, "BB_CHECKSUM_ACK_TYPE", "客户端至服务器指令" },
    { bb_guild_request_type, "BB_GUILD_REQUEST_TYPE", "客户端至服务器指令" },
    { bb_add_guildcard_type, "BB_ADD_GUILDCARD_TYPE", "客户端至服务器指令" },
    { bb_del_guildcard_type, "BB_DEL_GUILDCARD_TYPE", "客户端至服务器指令" },
    { bb_set_guildcard_text_type, "BB_SET_GUILDCARD_TEXT_TYPE", "客户端至服务器指令" },
    { bb_add_blocked_user_type, "BB_ADD_BLOCKED_USER_TYPE", "客户端至服务器指令" },
    { bb_del_blocked_user_type, "BB_DEL_BLOCKED_USER_TYPE", "客户端至服务器指令" },
    { bb_set_guildcard_comment_type, "BB_SET_GUILDCARD_COMMENT_TYPE", "客户端至服务器指令" },
    { bb_sort_guildcard_type, "BB_SORT_GUILDCARD_TYPE", "客户端至服务器指令" },
    { bb_guild_command, "BB_GUILD_COMMAND", "0x00EA" },
    { bb_guild_create, "BB_GUILD_CREATE", "0x01EA" },
    { bb_guild_unk_02ea, "BB_GUILD_UNK_02EA", "0x02EA" },
    { bb_guild_member_add, "BB_GUILD_MEMBER_ADD", "0x03EA" },
    { bb_guild_unk_04ea, "BB_GUILD_UNK_04EA", "0x04EA" },
    { bb_guild_member_remove, "BB_GUILD_MEMBER_REMOVE", "0x05EA" },
    { bb_guild_unk_06ea, "BB_GUILD_UNK_06EA", "0x06EA" },
    { bb_guild_member_chat, "BB_GUILD_CHAT", "0x07EA" },
    { bb_guild_member_setting, "BB_GUILD_MEMBER_SETTING", "0x08EA" },
    { bb_guild_unk_09ea, "BB_GUILD_UNK_09EA", "0x09EA" },
    { bb_guild_unk_0aea, "BB_GUILD_UNK_0AEA", "0x0AEA" },
    { bb_guild_unk_0bea, "BB_GUILD_UNK_0BEA", "0x0BEA" },
    { bb_guild_unk_0cea, "BB_GUILD_UNK_0CEA", "0x0CEA" },
    { bb_guild_unk_0dea, "BB_GUILD_UNK_0DEA", "0x0DEA" },
    { bb_guild_unk_0eea, "BB_GUILD_UNK_0EEA", "0x0EEA" },
    { bb_guild_member_flag_setting, "BB_GUILD_MEMBER_FLAG_SETTING", "0x0FEA" },
    { bb_guild_dissolve, "BB_GUILD_DISSOLVE", "0x10EA" },
    { bb_guild_member_promote, "BB_GUILD_MEMBER_PROMOTE", "0x11EA" },
    { bb_guild_unk_12ea, "BB_GUILD_UNK_12EA", "0x12EA" },
    { bb_guild_lobby_setting, "BB_GUILD_LOBBY_SETTING", "0x13EA" },
    { bb_guild_member_title, "BB_GUILD_MEMBER_TITLE", "0x14EA" },
    { bb_guild_unk_15ea, "BB_GUILD_UNK_15EA", "0x15EA" },
    { bb_guild_unk_16ea, "BB_GUILD_UNK_16EA", "0x16EA" },
    { bb_guild_unk_17ea, "BB_GUILD_UNK_17EA", "0x17EA" },
    { bb_guild_buy_privilege_and_point_info, "BB_GUILD_BUY_PRIVILEGE_AND_POINT_INFO", "0x18EA" },
    { bb_guild_privilege_list, "BB_GUILD_PRIVILEGE_LIST", "0x19EA" },
    { bb_guild_unk_1aea, "BB_GUILD_UNK_1AEA", "0x1AEA" },
    { bb_guild_unk_1bea, "BB_GUILD_UNK_1BEA", "0x1BEA" },
    { bb_guild_ranking_list, "BB_GUILD_RANKING_LIST", "0x1CEA" },
    { bb_param_send_type, "BB_PARAM_SEND_TYPE", "来自1.0 BB服务端 单指令 以下都是组合指令" },
    { bb_param_header_type, "BB_PARAM_HEADER_TYPE", "客户端至服务器指令" },
    { bb_param_chunk_type, "BB_PARAM_CHUNK_TYPE", "客户端至服务器指令" },
    { bb_param_chunk_req_type, "BB_PARAM_CHUNK_REQ_TYPE", "客户端至服务器指令" },
    { bb_param_header_req_type, "BB_PARAM_HEADER_REQ_TYPE", "客户端至服务器指令" },
    { ep3_game_create_type, "EP3_GAME_CREATE_TYPE", "客户端至服务器指令" },
    { bb_setflag_type, "BB_SETFLAG_TYPE", "客户端至服务器指令" },
    { bb_update_option, "BB_UPDATE_OPTION", "来自1.0 BB服务端 单指令 以下都是组合指令" },
    { bb_update_option_flags, "BB_UPDATE_OPTION_FLAGS", "客户端至服务器指令" },
    { bb_update_symbol_chat, "BB_UPDATE_SYMBOL_CHAT", "客户端至服务器指令" },
    { bb_update_shortcuts, "BB_UPDATE_SHORTCUTS", "客户端至服务器指令" },
    { bb_update_key_config, "BB_UPDATE_KEY_CONFIG", "客户端至服务器指令" },
    { bb_update_pad_config, "BB_UPDATE_PAD_CONFIG", "客户端至服务器指令" },
    { bb_update_tech_menu, "BB_UPDATE_TECH_MENU", "客户端至服务器指令" },
    { bb_update_config, "BB_UPDATE_CONFIG", "客户端至服务器指令" },
    { bb_update_config_mode, "BB_UPDATE_CONFIG_MODE", "来自1.0 BB服务端" },
    { bb_scroll_msg_type, "BB_SCROLL_MSG_TYPE", "顶部公告数据包" },
    { dc_welcome_length, "DC_WELCOME_LENGTH", "客户端至服务器指令" },
    { bb_welcome_length, "BB_WELCOME_LENGTH", "客户端至服务器指令" },
    { bb_security_length, "BB_SECURITY_LENGTH", "客户端至服务器指令" },
    { dc_redirect_length, "DC_REDIRECT_LENGTH", "客户端至服务器指令" },
    { bb_redirect_length, "BB_REDIRECT_LENGTH", "客户端至服务器指令" },
    { dc_redirect6_length, "DC_REDIRECT6_LENGTH", "客户端至服务器指令" },
    { bb_redirect6_length, "BB_REDIRECT6_LENGTH", "客户端至服务器指令" },
    { dc_timestamp_length, "DC_TIMESTAMP_LENGTH", "客户端至服务器指令" },
    { bb_timestamp_length, "BB_TIMESTAMP_LENGTH", "客户端至服务器指令" },
    { dc_lobby_list_length, "DC_LOBBY_LIST_LENGTH", "客户端至服务器指令" },
    { ep3_lobby_list_length, "EP3_LOBBY_LIST_LENGTH", "客户端至服务器指令" },
    { bb_lobby_list_length, "BB_LOBBY_LIST_LENGTH", "客户端至服务器指令" },
    { dc_char_data_length, "DC_CHAR_DATA_LENGTH", "客户端至服务器指令" },
    { dc_lobby_leave_length, "DC_LOBBY_LEAVE_LENGTH", "客户端至服务器指令" },
    { bb_lobby_leave_length, "BB_LOBBY_LEAVE_LENGTH", "客户端至服务器指令" },
    { dc_guild_reply_length, "DC_GUILD_REPLY_LENGTH", "客户端至服务器指令" },
    { pc_guild_reply_length, "PC_GUILD_REPLY_LENGTH", "客户端至服务器指令" },
    { bb_guild_reply_length, "BB_GUILD_REPLY_LENGTH", "客户端至服务器指令" },
    { dc_guild_reply6_length, "DC_GUILD_REPLY6_LENGTH", "客户端至服务器指令" },
    { pc_guild_reply6_length, "PC_GUILD_REPLY6_LENGTH", "客户端至服务器指令" },
    { bb_guild_reply6_length, "BB_GUILD_REPLY6_LENGTH", "客户端至服务器指令" },
    { dc_game_join_length, "DC_GAME_JOIN_LENGTH", "客户端至服务器指令" },
    { gc_game_join_length, "GC_GAME_JOIN_LENGTH", "客户端至服务器指令" },
    { ep3_game_join_length, "EP3_GAME_JOIN_LENGTH", "客户端至服务器指令" },
    { dc_quest_info_length, "DC_QUEST_INFO_LENGTH", "客户端至服务器指令" },
    { pc_quest_info_length, "PC_QUEST_INFO_LENGTH", "客户端至服务器指令" },
    { bb_quest_info_length, "BB_QUEST_INFO_LENGTH", "客户端至服务器指令" },
    { dc_quest_file_length, "DC_QUEST_FILE_LENGTH", "客户端至服务器指令" },
    { bb_quest_file_length, "BB_QUEST_FILE_LENGTH", "客户端至服务器指令" },
    { dc_quest_chunk_length, "DC_QUEST_CHUNK_LENGTH", "客户端至服务器指令" },
    { bb_quest_chunk_length, "BB_QUEST_CHUNK_LENGTH", "客户端至服务器指令" },
    { dc_simple_mail_length, "DC_SIMPLE_MAIL_LENGTH", "客户端至服务器指令" },
    { pc_simple_mail_length, "PC_SIMPLE_MAIL_LENGTH", "客户端至服务器指令" },
    { bb_simple_mail_length, "BB_SIMPLE_MAIL_LENGTH", "客户端至服务器指令" },
    { bb_option_config_length, "BB_OPTION_CONFIG_LENGTH", "客户端至服务器指令" },
    //{ 大小 14748, "大小 14748", "客户端至服务器指令" },
    //{ bb_full_character_length, "BB_FULL_CHARACTER_LENGTH", "客户端至服务器指令" },
    //{ 大小 14752, "大小 14752", "客户端至服务器指令" },
    { bb_full_character_length, "BB_FULL_CHARACTER_LENGTH", "客户端至服务器指令" },
    { dc_patch_header_length, "DC_PATCH_HEADER_LENGTH", "客户端至服务器指令" },
    { dc_patch_footer_length, "DC_PATCH_FOOTER_LENGTH", "客户端至服务器指令" },
    { NoSuchcCmd, "指令不存在" }
};

static s_cmd_map_st s_cmd_names[] = {
    { shdr_response, "SHDR_RESPONSE", "Response to a request 32768" },
    { shdr_failure, "SHDR_FAILURE", "Failure to complete request 16384" },
    { shdr_type_dc, "SHDR_TYPE_DC", "A decrypted Dreamcast game packet 1" },
    { shdr_type_bb, "SHDR_TYPE_BB", "A decrypted Blue Burst game packet 2" },
    { shdr_type_pc, "SHDR_TYPE_PC", "A decrypted PCv2 game packet 3" },
    { shdr_type_gc, "SHDR_TYPE_GC", "A decrypted Gamecube game packet 4" },
    { shdr_type_ep3, "SHDR_TYPE_EP3", "A decrypted Episode 3 packet 5" },
    { shdr_type_xbox, "SHDR_TYPE_XBOX", "A decrypted Xbox game packet 6" },
    { shdr_type_login, "SHDR_TYPE_LOGIN", "Shipgate hello packet 16" },
    { shdr_type_count, "SHDR_TYPE_COUNT", "A Client/Game Count update 17" },
    { shdr_type_sstatus, "SHDR_TYPE_SSTATUS", "A Ship has come up or gone down 18" },
    { shdr_type_ping, "SHDR_TYPE_PING", "A Ping packet, enough said 19" },
    { shdr_type_cdata, "SHDR_TYPE_CDATA", "Character data 20" },
    { shdr_type_creq, "SHDR_TYPE_CREQ", "Request saved character data 21" },
    { shdr_type_usrlogin, "SHDR_TYPE_USRLOGIN", "User login request 22" },
    { shdr_type_gcban, "SHDR_TYPE_GCBAN", "Guildcard ban 23" },
    { shdr_type_ipban, "SHDR_TYPE_IPBAN", "IP ban 24" },
    { shdr_type_blklogin, "SHDR_TYPE_BLKLOGIN", "User logs into a block 25" },
    { shdr_type_blklogout, "SHDR_TYPE_BLKLOGOUT", "User logs off a block 26" },
    { shdr_type_frlogin, "SHDR_TYPE_FRLOGIN", "A user's friend logs onto a block 27" },
    { shdr_type_frlogout, "SHDR_TYPE_FRLOGOUT", "A user's friend logs off a block 28" },
    { shdr_type_addfriend, "SHDR_TYPE_ADDFRIEND", "Add a friend to a user's list 29" },
    { shdr_type_delfriend, "SHDR_TYPE_DELFRIEND", "Remove a friend from a user's list 30" },
    { shdr_type_lobbychg, "SHDR_TYPE_LOBBYCHG", "A user changes lobbies 31" },
    { shdr_type_bclients, "SHDR_TYPE_BCLIENTS", "A bulk transfer of client info 32" },
    { shdr_type_kick, "SHDR_TYPE_KICK", "A kick request 33" },
    { shdr_type_frlist, "SHDR_TYPE_FRLIST", "Friend list request/reply 34" },
    { shdr_type_globalmsg, "SHDR_TYPE_GLOBALMSG", "A Global message packet 35" },
    { shdr_type_useropt, "SHDR_TYPE_USEROPT", "A user's options -- sent on login 36" },
    { shdr_type_login6, "SHDR_TYPE_LOGIN6", "A ship login (potentially IPv6) 37" },
    { shdr_type_bbopts, "SHDR_TYPE_BBOPTS", "A user's Blue Burst options 38" },
    { shdr_type_bbopt_req, "SHDR_TYPE_BBOPT_REQ", "Request Blue Burst options 39" },
    { shdr_type_cbkup, "SHDR_TYPE_CBKUP", "A character data backup packet 40" },
    { shdr_type_mkill, "SHDR_TYPE_MKILL", "Monster kill update 41" },
    { shdr_type_tlogin, "SHDR_TYPE_TLOGIN", "Token-based login request 42" },
    { shdr_type_schunk, "SHDR_TYPE_SCHUNK", "Script chunk 43" },
    { shdr_type_sdata, "SHDR_TYPE_SDATA", "Script data 44" },
    { shdr_type_sset, "SHDR_TYPE_SSET", "Script set 45" },
    { shdr_type_qflag_set, "SHDR_TYPE_QFLAG_SET", "Set quest flag 46" },
    { shdr_type_qflag_get, "SHDR_TYPE_QFLAG_GET", "Read quest flag 47" },
    { shdr_type_ship_ctl, "SHDR_TYPE_SHIP_CTL", "Ship control packet 48" },
    { shdr_type_ublocks, "SHDR_TYPE_UBLOCKS", "User blocklist 49" },
    { shdr_type_ubl_add, "SHDR_TYPE_UBL_ADD", "User blocklist add 50" },
    { shdr_type_bbguild, "SHDR_TYPE_BBGUILD", "A user's Blue Burst guild data 51" },
    { shdr_type_bbguild_req, "SHDR_TYPE_BBGUILD_REQ", "Request Blue Burst guild data 52" },
    { NoSuchsCmd, "指令不存在" }
};

/* Values for the shipgate flags portion of ships/proxies. */
#define SHIPGATE_FLAG_GMONLY    0x00000001
#define SHIPGATE_FLAG_NOV1      0x00000010
#define SHIPGATE_FLAG_NOV2      0x00000020
#define SHIPGATE_FLAG_NOPC      0x00000040
#define SHIPGATE_FLAG_NOEP12    0x00000080
#define SHIPGATE_FLAG_NOEP3     0x00000100
#define SHIPGATE_FLAG_NOBB      0x00000200
#define SHIPGATE_FLAG_NODCNTE   0x00000400
#define SHIPGATE_FLAG_NOPSOX    0x00000800
#define SHIPGATE_FLAG_NOPCNTE   0x00001000

/* The first few (V1, V2, PC) are only valid on the ship server, whereas the
   last couple (Ep3, BB) are only valid on the login server. GC works either
   place, but only some GC versions can see info files on the ship. */
#define PSOCN_INFO_V1       0x00000001
#define PSOCN_INFO_V2       0x00000002
#define PSOCN_INFO_PC       0x00000004
#define PSOCN_INFO_GC       0x00000008
#define PSOCN_INFO_EP3      0x00000010
#define PSOCN_INFO_BB       0x00000020
#define PSOCN_INFO_XBOX     0x00000040

   /* Languages that can be set for the info entries. */
#define PSOCN_INFO_JAPANESE 0x00000001
#define PSOCN_INFO_ENGLISH  0x00000002
#define PSOCN_INFO_GERMAN   0x00000004
#define PSOCN_INFO_FRENCH   0x00000008
#define PSOCN_INFO_SPANISH  0x00000010
#define PSOCN_INFO_CH_SIMP  0x00000020
#define PSOCN_INFO_CH_TRAD  0x00000040
#define PSOCN_INFO_KOREAN   0x00000080

/* Flags for the local_flags of a ship. */
#define PSOCN_SHIP_PMT_LIMITV2  0x00000001
#define PSOCN_SHIP_PMT_LIMITGC  0x00000002
#define PSOCN_SHIP_QUEST_RARES  0x00000004
#define PSOCN_SHIP_QUEST_SRARES 0x00000008
#define PSOCN_SHIP_PMT_LIMITBB  0x00000010

#define PSOCN_REG_DC            0x00000001
#define PSOCN_REG_DCNTE         0x00000002
#define PSOCN_REG_PC            0x00000004
#define PSOCN_REG_GC            0x00000008
#define PSOCN_REG_XBOX          0x00000010
#define PSOCN_REG_BB            0x00000020

typedef struct psocn_dbconfig {
    char* type;
    char* host;
    char* user;
    char* pass;
    char* db;
    uint16_t port;
    char* auto_reconnect;
    char* char_set;
    char* show_setting;
} psocn_dbconfig_t;

typedef struct psocn_srvconfig {
    char* host4;
    char* host6;
    uint32_t server_ip;
    uint8_t server_ip6[16];
} psocn_srvconfig_t;

typedef struct psocn_sgconfig {
    uint16_t shipgate_port;
    char* shipgate_cert;
    char* shipgate_key;
    char* shipgate_ca;
} psocn_sgconfig_t;

typedef struct psocn_info_file {
    char* desc;
    char* filename;
    uint32_t versions;
    uint32_t languages;
} psocn_info_file_t;

typedef struct psocn_limit_config {
    uint32_t id;
    char* name;
    char* filename;
    int enforce;
} psocn_limit_config_t;

typedef struct psocn_welcom_motd {
    char* web_host;
    uint16_t web_port;
    char* patch_welcom_file;
    char patch_welcom[4096];
    char* login_welcom_file;
    char login_welcom[4096];
} psocn_welcom_motd_t;

typedef struct psocn_config {
    psocn_srvconfig_t srvcfg;
    psocn_dbconfig_t dbcfg;
    psocn_sgconfig_t sgcfg;
    uint8_t registration_required;
    char* quests_dir;
    psocn_limit_config_t* limits;
    int limits_count;
    int limits_enforced;
    psocn_info_file_t* info_files;
    int info_file_count;
    psocn_welcom_motd_t w_motd;
    char* log_dir;
    char* log_prefix;
    char* patch_dir;
    char* sg_scripts_file;
    char* auth_scripts_file;
    char* socket_dir;
} psocn_config_t;

typedef struct psocn_event {
    uint8_t start_month;
    uint8_t start_day;
    uint8_t end_month;
    uint8_t end_day;
    uint8_t lobby_event;
    uint8_t game_event;
} psocn_event_t;

typedef struct psocn_shipcfg {
    char* shipgate_host;
    uint16_t shipgate_port;

    char* name;
    char* ship_cert;
    char* ship_key;
    uint32_t ship_key_idx;
    uint8_t ship_rc4key[128];
    char* ship_dir;
    char* shipgate_ca;
    char* gm_file;
    psocn_limit_config_t* limits;
    psocn_info_file_t* info_files;
    char* quests_file;
    char* quests_dir;
    char* bans_file;
    char* scripts_file;
    char* bb_param_dir;
    char* v2_param_dir;
    char* bb_map_dir;
    char* v2_map_dir;
    char* gc_map_dir;
    char* v2_ptdata_file;
    char* gc_ptdata_file;
    char* bb_ptdata_file;
    char* v2_pmtdata_file;
    char* gc_pmtdata_file;
    char* bb_pmtdata_file;
    char* v2_rtdata_file;
    char* gc_rtdata_file;
    char* bb_rtdata_file;
    psocn_event_t* events;
    char* smutdata_file;

    char* ship_host4;
    char* ship_host6;
    uint32_t ship_ip4;
    uint8_t ship_ip6[16];
    uint32_t shipgate_flags;
    uint32_t shipgate_proto_ver;
    uint32_t local_flags;

    uint16_t base_port;
    uint16_t menu_code;

    uint32_t blocks;
    int info_file_count;
    int event_count;
    int limits_count;
    int limits_default;
    uint32_t privileges;
} psocn_ship_t;

/* Patch server configuration structure. */
typedef struct patch_file_entry {
    char* filename;
    uint32_t size;
    uint32_t checksum;
    uint32_t client_checksum;

    struct patch_file_entry* next;
} patch_file_entry_t;

/* Patch file structure. */
typedef struct patch_file {
    TAILQ_ENTRY(patch_file) qentry;

    uint32_t flags;
    char* filename;

    patch_file_entry_t* entries;
} patch_file_t;

#define PATCH_FLAG_HAS_IF       0x00000001
#define PATCH_FLAG_HAS_ELSE     0x00000002
#define PATCH_FLAG_NO_IF        0x00000004

TAILQ_HEAD(file_queue, patch_file);

/* Client-side file structure. */
typedef struct patch_cfile {
    TAILQ_ENTRY(patch_cfile) qentry;

    patch_file_t* file;
    patch_file_entry_t* ent;
} patch_cfile_t;

TAILQ_HEAD(cfile_queue, patch_cfile);

typedef struct patch_config {
    psocn_srvconfig_t srvcfg;

    uint32_t disallow_pc;
    uint32_t disallow_bb;

    psocn_welcom_motd_t w_motd;
    uint16_t* pc_welcome;
    uint16_t* bb_welcome;
    uint16_t pc_welcome_size;
    uint16_t bb_welcome_size;

    char* pc_dir;
    char* bb_dir;

    struct file_queue pc_files;
    struct file_queue bb_files;
} patch_config_t;

char* int2ipstr(const int ip);
int32_t server_is_ip(char* server);
int check_ipaddr(char* IP);


/* 通过代码对比获取指令名称 */
extern const char* c_cmd_name(uint16_t cmd, int32_t version);
extern const char* s_cmd_name(uint16_t cmd, int32_t version);

/* In psoconfig_patch.c */
extern int patch_read_config(const char* fn, patch_config_t** cfg);
extern void patch_free_config(patch_config_t* cfg);

/* Read the configuration for the login server, shipgate, and patch server. */
extern int psocn_read_config(const char* f, psocn_config_t** cfg);

/* Clean up a configuration structure. */
extern void psocn_free_config(psocn_config_t* cfg);

/* 读取服务器地址设置. */
extern int psocn_read_srv_config(const char* f, psocn_srvconfig_t** cfg);

/* Clean up a configuration structure. */
extern void psocn_free_srv_config(psocn_srvconfig_t* cfg);

extern int psocn_read_db_config(const char* f, psocn_dbconfig_t** cfg);

/* Clean up a configuration structure. */
extern void psocn_free_db_config(psocn_dbconfig_t* cfg);

/* Read the ship configuration data. You are responsible for calling the
   function to clean up the configuration. */
extern int psocn_read_ship_config(const char* f, psocn_ship_t** cfg);

/* Clean up a ship configuration structure. */
extern void psocn_free_ship_config(psocn_ship_t* cfg);

extern int psocn_web_server_loadfile(const char* onlinefile, char* dest);

extern int psocn_web_server_getfile(void* HostName, int32_t port, char* file, const char* onlinefile);

#endif /* !PSOCN_CONFIG_H */
