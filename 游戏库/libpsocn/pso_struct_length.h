/*
    梦幻之星中国 结构长度表

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

#ifndef PSOCN_STRUCT_LENGTH_H
#define PSOCN_STRUCT_LENGTH_H

/////////////////////////////////////////////////////////////////////////
/////

#define BB_CHARACTER_NAME_LENGTH            12 //完整的玩家名称结构长度 含tag        12双字节
#define BB_CHARACTER_CHAR_NAME_LENGTH       10 //定义双字节的玩家名称长度            10双字节
#define BB_CHARACTER_CHAR_NAME_WLENGTH      20 //用于单字节拷贝玩家名称不带tag的长度 20字节
#define BB_CHARACTER_CHAR_TAG_NAME_WLENGTH  24 //用于单字节拷贝玩家名称带tag的长度   24字节

#define PSOCN_STLENGTH_ITEM                 20
#define PSOCN_STLENGTH_IITEM                28
#define PSOCN_STLENGTH_INV                  844
#define PSOCN_STLENGTH_BITEM                24
#define PSOCN_STLENGTH_BANK                 4808

#define PSOCN_STLENGTH_DISP                 36

#define PSOCN_STLENGTH_GC_STR               16

#define PSOCN_STLENGTH_DRESS                80

#define PSOCN_STLENGTH_BB_MINI_CHAR         124
#define PSOCN_STLENGTH_BB_FULL_CHAR         14744
//#define PSOCN_STLENGTH_BB_DB_CHAR           7912
#define PSOCN_STLENGTH_BB_DB_CHAR           7884

#define PSOCN_STLENGTH_BB_CHAR              400
#define PSOCN_STLENGTH_BB_KEY_CONFIG        420
#define PSOCN_STLENGTH_BB_GUILD             2108
#define PSOCN_STLENGTH_BB_GC                264
#define PSOCN_STLENGTH_BB_GC_DATA           54672
#define PSOCN_STLENGTH_BB_DB_OPTS           4448
#define PSOCN_STLENGTH_BB_DB_QUEST_DATA1    520
#define PSOCN_STLENGTH_BB_DB_QUEST_DATA2    88
#define PSOCN_STLENGTH_BB_DB_TECH_MENU      40
#define PSOCN_STLENGTH_BB_DB_SYMBOL_CHATS   1248

/////////////////////////////////////////////////////////////////////////
/////

#define PSOCN_STLENGTH_BATTLE_RECORDS       24

#define PSOCN_STLENGTH_DC_RECORDS           188
#define PSOCN_STLENGTH_DC_RECORDS_DATA      184 /* 不含client_id */
#define PSOCN_STLENGTH_DC_CHALLENGE_RECORDS 160

#define PSOCN_STLENGTH_PC_RECORDS           244
#define PSOCN_STLENGTH_PC_RECORDS_DATA      240 /* 不含client_id */
#define PSOCN_STLENGTH_PC_CHALLENGE_RECORDS 216

#define PSOCN_STLENGTH_V3_RECORDS           284
#define PSOCN_STLENGTH_V3_RECORDS_DATA      280 /* 不含client_id */
#define PSOCN_STLENGTH_V3_CHALLENGE_RECORDS 256

#define PSOCN_STLENGTH_BB_RECORDS           348
#define PSOCN_STLENGTH_BB_RECORDS_DATA      344 /* 不含client_id */
#define PSOCN_STLENGTH_BB_CHALLENGE_RECORDS 320

/////////////////////////////////////////////////////////////////////////
/////
#define PC_CHARACTER_NAME_LENGTH            16






#endif // !PSOCN_STRUCT_LENGTH_H
