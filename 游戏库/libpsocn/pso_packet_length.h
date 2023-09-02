/*
    梦幻之星中国 数据包长度表

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

#ifndef PSOCN_PACKET_LENGTH_H
#define PSOCN_PACKET_LENGTH_H

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

#define DC_PATCH_HEADER_LENGTH          0x0014
#define DC_PATCH_FOOTER_LENGTH          0x0018

#define BB_FULL_CHAR_LENGTH             0x39A0

#endif // !PSOCN_PACKET_LENGTH_H
