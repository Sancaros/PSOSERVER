/*
    梦幻之星中国 静态库
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


#ifndef PACKETS_H_HAVE_HEADERS
#define PACKETS_H_HAVE_HEADERS

#include <stdint.h>

#include "pso_player.h"

/* Always define the headers of packets, if they haven't already been taken
   care of. */

typedef union {
    float f;
    uint32_t b;
} bitfloat_t;

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

// 这个文件是 newserv 的客户端/服务器协议的权威参考。

// 对于不熟悉的人，le_uint 和 be_uint 类型（来自 phosg/Encoding.hh）与普通的 uint 类型相同，只是明确指定为小端或大端。
// parray 和 ptext 类型（来自 Text.hh）与标准数组相同，但具有各种安全和便利功能，
// 使我们不必使用易于出错的函数（如 memset/memcpy 和 strncpy）。

// 结构体命名类似于 [S|C|SC]CommandName[Versions]_Numbers
// S/C 表示谁发送了该命令（S = 服务器，C = 客户端，SC = 双方）
// 如果未指定版本，则格式对所有版本都相同。

// 版本标记如下：
// DCv1 = PSO Dreamcast v1
// DCv2 = PSO Dreamcast v2
// DC = DCv1 和 DCv2 都支持
// PC = PSO PC（v2）
// GC = PSO GC Episodes 1&2 和/或 Episode 3
// XB = PSO XBOX Episodes 1&2
// BB = PSO Blue Burst
// V3 = PSO GC 和 PSO XBOX（这些版本类似，共享许多格式）

// 对于可变长度的命令，通常在结构体末尾包含一个长度为零的数组，如果命令由 newserv 接收，则包含该数组，如果命令由 newserv 发送，则省略该数组。在后一种情况下，我们经常使用 StringWriter 来构造命令数据。

// 结构体按命令编号排序。长的 BB 命令按其低字节顺序排列；例如，命令 01EB 的位置是 EB。

// 文本转义代码

// 大多数文本字段允许使用不同的转义代码来改变解码、改变颜色或创建符号。这些转义代码总是以制表符字符（0x09，或 '\t'）开头。为了简洁起见，在 newserv 中我们通常用 $ 来表示它们，因为服务器会将玩家提供的文本中的大多数 $ 替换为 \t。这些转义代码包括：
// - 语言代码
// - - $E：将文本解释设置为英语
// - - $J：将文本解释设置为日语
// - 颜色代码
// - - $C0：黑色（000000）
// - - $C1：蓝色（0000FF）
// - - $C2：绿色（00FF00）
// - - $C3：青色（00FFFF）
// - - $C4：红色（FF0000）
// - - $C5：品红色（FF00FF）
// - - $C6：黄色（FFFF00）
// - - $C7：白色（FFFFFF）
// - - $C8：粉色（FF8080）
// - - $C9：紫罗兰色（8080FF）
// - - $CG：橙色脉动（FFE000 + 其他变暗色）
// - - $Ca：橙色（F5A052；仅第三集）
// - 特殊字符代码（仅适用于第三集）
// - - $B：破折号 + 小圆点符号
// - - $D：大圆点符号
// - - $F：女性符号
// - - $I：无限符号
// - - $M：男性符号
// - - $O：空心圆
// - - $R：实心圆
// - - $S：星形能力符号
// - - $X：交叉符号
// - - $d：向下箭头
// - - $l：向左箭头
// - - $r：向右箭头
// - - $u：向上箭头

/* DC V3 GC XBOX客户端数据头 4字节 */
typedef struct dc_pkt_hdr {
    uint8_t pkt_type;
    uint8_t flags;
    uint16_t pkt_len;
} PACKED dc_pkt_hdr_t;

/* PC 客户端数据头 4字节 */
typedef struct pc_pkt_hdr {
    uint16_t pkt_len;
    uint8_t pkt_type;
    uint8_t flags;
} PACKED pc_pkt_hdr_t;

/* BB 客户端数据头 8字节 */
typedef struct bb_pkt_hdr {
    uint16_t pkt_len;
    uint16_t pkt_type;
    uint32_t flags;
} PACKED bb_pkt_hdr_t;

/* DC V3 GC XBOX客户端错误数据头 4字节 */
typedef struct dc_err_pkt_hdr {
    dc_pkt_hdr_t hdr;
    uint8_t data[0];
} PACKED dc_err_pkt_hdr_t;

/* PC 客户端错误数据头 4字节 */
typedef struct pc_err_pkt_hdr {
    pc_pkt_hdr_t hdr;
    uint8_t data[0];
} PACKED pc_err_pkt_hdr_t;

/* BB 客户端错误数据头 8字节 */
typedef struct bb_err_pkt_hdr {
    bb_pkt_hdr_t hdr;
    uint8_t data[0];
} PACKED bb_err_pkt_hdr_t;

/* 客户端数据头联合结构 */
typedef union pkt_header {
    dc_pkt_hdr_t dc;
    pc_pkt_hdr_t pc;
    bb_pkt_hdr_t bb;
} pkt_header_t;

// Patch server commands

// The patch protocol is identical between PSO PC and PSO BB (the only versions
// on which it is used).

// A patch server session generally goes like this:
// Server: 02 (unencrypted)
// (all the following commands encrypted with PSO V2 encryption, even on BB)
// Client: 02
// Server: 04
// Client: 04
// If client's login information is wrong and server chooses to reject it:
//   Server: 15
//   Server disconnects
// Otherwise:
//   Server: 13 (if desired)
//   Server: 0B
//   Server: 09 (with directory name ".")
//   For each directory to be checked:
//     Server: 09
//     Server: (commands to check subdirectories - more 09/0A/0C)
//     For each file in the directory:
//       Server: 0C
//     Server: 0A
//   Server: 0D
//   For each 0C sent by the server earlier:
//     Client: 0F
//   Client: 10
//   If there are any files to be updated:
//     Server: 11
//     For each directory containing files to be updated:
//       Server: 09
//       Server: (commands to update subdirectories)
//       For each file to be updated in this directory:
//         Server: 06
//         Server: 07 (possibly multiple 07s if the file is large)
//         Server: 08
//       Server: 0A
//   Server: 12
//   Server disconnects

// 00: 无效或未解析指令
// 01: 无效或未解析指令
// 02 (服务器->客户端)：开始加密
// 客户端将使用一个 02 命令作出回应。
// 此命令后的所有命令将使用 PSO V2 加密方式进行加密。
// 如果在已加密会话期间发送此命令，客户端不会拒绝它；而是会重新初始化加密状态并以正常方式回应一个 02 命令。
// 下面的结构中的版权字段必须包含以下文本：
// "Patch Server. Copyright SonicTeam, LTD. 2001"

/* The packet sent to inform clients of their security data */
typedef struct dc_security {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t tag;
    uint32_t guildcard;
    uint8_t security_data[0];
} PACKED dc_security_pkt;

/* Client config packet as defined by newserv. */
//typedef struct bb_security_data {
//    uint32_t magic; // must be set to 0x48615467 archron 0xDEADBEEF syl
//    uint8_t slot; // status of client connecting on BB
//    uint8_t sel_char; // selected char
//    uint16_t flags; // just in case we lose them somehow between connections
//    uint16_t ports[4]; // used by shipgate clients
//    uint32_t unused[4];
//    uint32_t unused_bb_only[2];
//} PACKED bb_security_data_t;

typedef struct client_config {
    uint64_t magic;
    uint16_t flags;
    uint32_t proxy_destination_address;
    uint16_t proxy_destination_port;
    uint8_t unused[0x10];
} PACKED client_config_t;

typedef struct bb_client_config {
    client_config_t cfg;
    uint8_t slot;
    uint8_t sel_char;
    uint8_t unused[0x06];
} PACKED bb_client_config_pkt;

// 04 (S->C): Set guild card number and update client config ("security data")
// header.flag specifies an error code; the format described below is only used
// if this code is 0 (no error). Otherwise, the command has no arguments.
// Error codes (on GC):
//   01 = Line is busy (103)
//   02 = Already logged in (104)
//   03 = Incorrect password (106)
//   04 = Account suspended (107)
//   05 = Server down for maintenance (108)
//   06 = Incorrect password (127)
//   Any other nonzero value = Generic failure (101)
// The client config field in this command is ignored by pre-V3 clients as well
// as Episodes 1&2 Trial Edition. All other V3 clients save it as opaque data to
// be returned in a 9E or 9F command later. newserv sends the client config
// anyway to clients that ignore it.
// The client will respond with a 96 command, but only the first time it
// receives this command - for later 04 commands, the client will still update
// its client config but will not respond. Changing the security data at any
// time seems ok, but changing the guild card number of a client after it's
// initially set can confuse the client, and (on pre-V3 clients) possibly
// corrupt the character data. For this reason, newserv tries pretty hard to
// hide the remote guild card number when clients connect to the proxy server.
// BB clients have multiple client configs; this command sets the client config
// that is returned by the 9E and 9F commands, but does not affect the client
// config set by the E6 command (and returned in the 93 command). In most cases,
// E6 should be used for BB clients instead of 04.
typedef struct bb_security {
    bb_pkt_hdr_t hdr;
    uint32_t err_code;
    uint32_t player_tag;
    uint32_t guildcard;
    uint32_t guild_id;
    union {
        bb_client_config_pkt sec_data;
        uint8_t security_data[40];
    };
    uint32_t caps;
} PACKED bb_security_pkt;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

#endif /* !PACKETS_H_HAVE_HEADERS */

#ifndef PACKETS_H_HEADERS_ONLY
#ifndef PACKETS_H_HAVE_PACKETS
#define PACKETS_H_HAVE_PACKETS

/* Only do these if we really want them for the file we're compiling. */

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// 游戏指令 ////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// 00: 无效或未解析指令

// 01 (服务器->客户端)：大厅消息框
// 内部名称：RcvError
// 一个小的消息框出现在右下角，玩家必须按下一个键才能继续。消息的最大长度为 0x200 字节。
// 在 PSO 的内部，它被称为 RcvError，因为通常用于告诉玩家为什么他们无法做某事（例如加入一个已满的游戏）。
// 这个格式被多个命令共享；对于除了 06 (服务器->客户端) 之外的所有命令，guild_card_number 字段未使用，应该为 0。
struct SC_TextHeader_01_06_11_B0_EE {
    uint32_t unused;
    uint32_t guild_card_number;
    // Text immediately follows here (char[] on DC/V3, char16_t[] on PC/BB)
} PACKED;

typedef struct dc_msg01 {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t client_id;
    uint32_t guildcard;
    char msg[0];
} PACKED dc_msg01_pkt;

typedef struct bb_msg01 {
    bb_pkt_hdr_t hdr;
    uint32_t client_id;
    uint32_t guildcard;
    uint16_t msg[0];
} PACKED bb_msg01_pkt;

// 02 (服务器->客户端)：开始加密（不适用于 BB 版本）
// 内部名称：RcvPsoConnectV2
// 此命令应用于非初始会话（例如，在客户端已经选择了一个船只之后）。对于首次连接，应使用命令 17。
// 此命令后的所有命令将在 DC、PC 和 GC Episodes 1&2 Trial Edition 上使用 PSO V2 加密，或在其他 V3 版本上使用 PSO V3 加密。
// DCv1 客户端将以（加密的）93 命令作出回应。
// DCv2 和 PC 客户端将以（加密的）9A 或 9D 命令作出回应。
// V3 客户端将以（加密的）9A 或 9E 命令作出回应，除了 GC Episodes 1&2 Trial Edition，它的行为类似于 PC。
// 下面的结构中的版权字段必须包含以下文本：
// "DreamCast Lobby Server. Copyright SEGA Enterprises. 1999"
// （上述文本在所有使用此命令的版本上都是必需的，包括那些不运行在 DreamCast 上的版本。）

// 03 (客户端->服务器): 传统注册（非 BB 版本）
// 内部名称: SndRegist

struct S_ServerInitDefault_DC_PC_V3_02_17_91_9B {
    char copyright[0x40];
    uint32_t server_key; // Key for data sent by server
    uint32_t client_key; // Key for data sent by client
} PACKED;

struct S_ServerInitWithAfterMessage_DC_PC_V3_02_17_91_9B {
    struct S_ServerInitDefault_DC_PC_V3_02_17_91_9B basic_cmd;
    // 这个字段不是 SEGA 实现的一部分；客户端会忽略它。
    // newserv 在这里发送一条消息否认前面的版权声明。
    char after_message[];
} PACKED;

/* The welcome packet for setting up encryption keys */
typedef struct dc_welcome {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char copyright[0x40];
    uint32_t svect;
    uint32_t cvect;
    // 这个字段不是 SEGA 实现的一部分；客户端会忽略它。
    // newserv 在这里发送一条消息否认前面的版权声明。
    char after_message[0xC0];
} PACKED dc_welcome_pkt;

// 03 (服务器->客户端): 传统密码检查结果（非 BB 版本）
// header.flag 指示密码是否正确。如果 header.flag 为 0，
// 则删除保存在记忆卡中的密码（如果有），并断开客户端连接。
// 如果 header.flag 非零，则客户端将以 04 命令作出回应。有趣的是，
// 即使 DCv1 在其标准登录序列中也没有使用此命令，
// 因此这可能是一个非常早期开发的遗物。
// 没有其他参数

// 03 (服务器->客户端): 开始加密（BB 版本）
// 客户端将以（加密的）93 命令作出回应。
// 此命令后的所有命令将使用 PSO BB 加密。
// 下面的结构中的版权字段必须包含以下文本: 
// "Phantasy Star Online Blue Burst Game Server. Copyright 1999-2004 SONICTEAM."
typedef struct bb_welcome {
    bb_pkt_hdr_t hdr;
    char copyright[0x60];
    uint8_t server_key[0x30];
    uint8_t client_key[0x30];
    // 这个字段不是 SEGA 实现的一部分；客户端会忽略它。
    // newserv 在这里发送一条消息否认前面的版权声明。
    char after_message[0xC0];
} PACKED bb_welcome_pkt;

// 04 (服务器->客户端): 设置公会卡号并更新客户端配置（"安全数据"）
// 内部名称: RcvLogin
// header.flag 指定错误代码；以下描述的格式仅在该代码为0（无错误）时使用。否则，命令没有参数。
// 错误代码（对于游戏机卡）：
//   01 = 线路忙（103）
//   02 = 已登录（104）
//   03 = 密码错误（106）
//   04 = 帐户暂停（107）
//   05 = 服务器维护中（108）
//   06 = 密码错误（127）
//   其他非零值 = 一般故障（101）
// 此命令中的客户端配置字段被早期 V3 客户端以及第一和第二集试用版忽略。
// 所有其他 V3 客户端将其保存为不透明数据，稍后在9E或9F命令中返回。
// 不管客户端是否忽略它，newserv 都会发送客户端配置。
// 客户端将以96命令作出回应，但仅在首次收到此命令时才会回应——对于后续的04命令，
// 客户端仍会更新其客户端配置，但不会回应。随时更改安全数据似乎都没有问题，
// 但在将客户端的公会卡号设置后更改它可能会使客户端混乱，
// 并且（对于早期 V3 客户端）可能破坏角色数据。
// 因此，newserv 在客户端连接到代理服务器时尽力隐藏远程公会卡号。
// BB 客户端有多个客户端配置；此命令设置由9E和9F命令返回的客户端配置，
// 但不影响E6命令设置的客户端配置（并在93命令中返回）。
// 在大多数情况下，应该使用E6命令来设置 BB 客户端的客户端配置，而不是04命令。

//template <typename ClientConfigT>
//struct S_UpdateClientConfig {
//    // 注意：这里所称的 player_tag 实际上是三个字段：
//    // 两个 uint8_t，后跟一个 le_uint16_t。
//    // 目前不清楚这两个 uint8_t 字段的用途（它们似乎始终为零），
//    // 但 le_uint16_t 可能是一个布尔值，表示玩家是否存在（例如，在大厅数据结构中）。
//    // 出于历史和简单性的原因，newserv 将这三个字段合并为一个字段，
//    // 当玩家存在时，其取值为0x00010000，没有玩家存在时为零。
//    uint32_t player_tag = 0x00010000;
//    uint32_t guild_card_number = 0;
//    // ClientConfig 结构描述了 newserv 如何使用这个命令；
//    // 其他服务器可能不会以相同的格式使用接下来的 0x20 或 0x28 字节（或者可能根本不使用）。
//    // cfg 字段对客户端来说是不透明的；它将在下一个 9E 命令中原样返回内容（或者通过 9F 请求返回）。
//    ClientConfigT cfg;
//} PACKED;
//
//struct S_UpdateClientConfig_DC_PC_V3_04 : S_UpdateClientConfig<ClientConfig> {
//} PACKED;
//struct S_UpdateClientConfig_BB_04 : S_UpdateClientConfig<ClientConfigBB> {
//} PACKED;

// 05: 断开连接
// 内部名称: SndLogout
// 无参数
// 向客户端发送此命令将导致其断开连接。
// 相比于简单地关闭TCP连接，这样做没有任何优势。
// 当客户端准备断开连接时，将向服务器发送此命令，
// 但服务器在接收到此命令时不需要关闭连接（在某些情况下，客户端会在实际断开连接之前发送多个05命令）。
typedef struct bb_burst {
    bb_pkt_hdr_t hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED bb_burst_pkt;

// 06: 聊天
// 内部名称: RcvChat 和 SndChat
// 服务器->客户端的格式与 01 命令相同。消息的最大大小为 0x200 字节。
// 客户端->服务器的格式非常类似；我们在这个结构中包含了一个零长度的数组，以便更容易解析。
// 当由客户端发送时，text 字段仅包含消息内容。当由服务器发送时，text 字段包括原始玩家的名称，
// 后跟一个制表符，然后是消息内容。
// 在第三集战斗期间，入站的 06 命令的消息的第一个字节被解释方式与其他情况不同。
// 它应被视为一个位字段，其中低 4 位被用作指示谁能看到消息的掩码。
// 例如，如果设置了最低位（1），那么无论消息内容如何，在玩家 0 的屏幕上聊天消息将显示为“（悄悄话）”。
// 下一个位（2）将隐藏消息对玩家 1 不可见，以此类推。
// 这个字节的高 4 位似乎没有被使用，但通常非零且设置为值 4。
// （这可能是为了确保该字段始终是有效的 ASCII 字符，并且不会意外终止聊天字符串。）
// 在 newserv 中，我们称这个字节为 private_flags。
typedef struct dc_chat {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint16_t client_id;
    uint16_t unknow;
    //uint32_t client_id;
    uint32_t guildcard;
    char msg[0];
} PACKED dc_chat_pkt;

// 06: 聊天
// 内部名称: RcvChat 和 SndChat
// 服务器->客户端的格式与 01 命令相同。消息的最大大小为 0x200 字节。
// 客户端->服务器的格式非常类似；我们在这个结构中包含了一个零长度的数组，以便更容易解析。
// 当由客户端发送时，text 字段仅包含消息内容。当由服务器发送时，text 字段包括原始玩家的名称，
// 后跟一个制表符，然后是消息内容。
// 在第三集战斗期间，入站的 06 命令的消息的第一个字节被解释方式与其他情况不同。
// 它应被视为一个位字段，其中低 4 位被用作指示谁能看到消息的掩码。
// 例如，如果设置了最低位（1），那么无论消息内容如何，在玩家 0 的屏幕上聊天消息将显示为“（悄悄话）”。
// 下一个位（2）将隐藏消息对玩家 1 不可见，以此类推。
// 这个字节的高 4 位似乎没有被使用，但通常非零且设置为值 4。
// （这可能是为了确保该字段始终是有效的 ASCII 字符，并且不会意外终止聊天字符串。）
// 在 newserv 中，我们称这个字节为 private_flags。
// 16字节
typedef struct bb_chat {
    bb_pkt_hdr_t hdr;
    uint16_t client_id;
    uint16_t unknow;
    uint32_t guildcard;
    uint16_t msg[0];
} PACKED bb_chat_pkt;

// 07 (S->C): 选择飞船菜单
// 内部名称: RcvDirList
// 此命令触发了一个通用的阻塞式菜单，
// Sega（以及所有其他私服，似乎都是）在飞船选择菜单和区块选择菜单中使用了相同的形式。
// 有趣的是，在 PSO v1 和 v2 中出现了字符串“RcvBlockList”，
// 但它没有被使用，这意味着在某些时候可能有一个单独的命令用于发送区块列表，
// 但是它被废弃了。也许这个命令被用于 A1 命令，该命令在所有版本的 PSO（除了 DC NTE）中与 07 命令和 A0 命令相同。

/* 28字节 */
typedef struct dc_menu {
    uint32_t menu_id;
    uint32_t item_id;
    uint16_t flags;
    char name[0x12];
} PACKED dc_menu_t;

/* 44字节 */
typedef struct v3_menu {
    uint32_t menu_id;
    uint32_t item_id;
    uint16_t flags;
    uint16_t name[0x11];
} PACKED v3_menu_t;

// 命令是一个包含多个条目的列表；header.flag 是条目的数量。
// 第一个条目不包括在计数中，也不会出现在客户端上。
// 第一个条目的文本在客户端加入大厅时成为飞船的名称。
typedef struct dc_ship_list {
    dc_pkt_hdr_t hdr;           /* The flags field says how many entries */
    dc_menu_t entries[0];
} PACKED dc_ship_list_pkt;

// 07 (S->C): 选择飞船菜单
// 内部名称: RcvDirList
// 此命令触发了一个通用的阻塞式菜单，
// Sega（以及所有其他私服，似乎都是）在飞船选择菜单和区块选择菜单中使用了相同的形式。
// 有趣的是，在 PSO v1 和 v2 中出现了字符串“RcvBlockList”，
// 但它没有被使用，这意味着在某些时候可能有一个单独的命令用于发送区块列表，
// 但是它被废弃了。也许这个命令被用于 A1 命令，该命令在所有版本的 PSO（除了 DC NTE）中与 07 命令和 A0 命令相同。

// 命令是一个包含多个条目的列表；header.flag 是条目的数量。
// 第一个条目不包括在计数中，也不会出现在客户端上。
// 第一个条目的文本在客户端加入大厅时成为飞船的名称。
typedef struct pc_ship_list {
    pc_pkt_hdr_t hdr;           /* The flags field says how many entries */
    v3_menu_t entries[0];
} PACKED pc_ship_list_pkt;

// 07 (S->C): 选择飞船菜单
// 内部名称: RcvDirList
// 此命令触发了一个通用的阻塞式菜单，
// Sega（以及所有其他私服，似乎都是）在飞船选择菜单和区块选择菜单中使用了相同的形式。
// 有趣的是，在 PSO v1 和 v2 中出现了字符串“RcvBlockList”，
// 但它没有被使用，这意味着在某些时候可能有一个单独的命令用于发送区块列表，
// 但是它被废弃了。也许这个命令被用于 A1 命令，该命令在所有版本的 PSO（除了 DC NTE）中与 07 命令和 A0 命令相同。

// 命令是一个包含多个条目的列表；header.flag 是条目的数量。
// 第一个条目不包括在计数中，也不会出现在客户端上。
// 第一个条目的文本在客户端加入大厅时成为飞船的名称。
typedef struct bb_ship_list {
    bb_pkt_hdr_t hdr;           /* The flags field says how many entries */
    v3_menu_t entries[0];
} PACKED bb_ship_list_pkt;

// 08 (C->S): Request game list
// Internal name: SndGameList
// No arguments

// 08 (S->C): Game list
// Internal name: RcvGameList
// Client responds with 09 and 10 commands (or nothing if the player cancels).

// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client.
typedef struct dc_game_list {
    dc_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint8_t difficulty;
        uint8_t players;
        char name[16];
        uint8_t padding;
        uint8_t flags;
    } entries[0];
} PACKED dc_game_list_pkt;

// 08 (S->C): Game list
// Internal name: RcvGameList
// Client responds with 09 and 10 commands (or nothing if the player cancels).

// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client.
typedef struct pc_game_list {
    pc_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint8_t difficulty;
        uint8_t players;
        uint16_t name[16];
        uint8_t v2;
        uint8_t flags;
    } entries[0];
} PACKED pc_game_list_pkt;

// 08 (S->C): Game list
// Internal name: RcvGameList
// Client responds with 09 and 10 commands (or nothing if the player cancels).

// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client.
typedef struct bb_game_list {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        // 在第三集中，difficulty_tag 是 0x0A；
        // 在其他所有版本中，它是 difficulty + 0x22（因此，0x25 表示终极难度）。
        uint8_t difficulty;
        uint8_t players;
        uint16_t name[16];
        // The episode field is used differently by different versions:
        // - On DCv1, PC, and GC Episode 3, the value is ignored.
        // - On DCv2, 1 means v1 players can't join the game, and 0 means they can.
        // - On GC Ep1&2, 0x40 means Episode 1, and 0x41 means Episode 2.
        // - On BB, 0x40/0x41 mean Episodes 1/2 as on GC, and 0x43 means Episode 4.
        uint8_t episode;
        // Flags:
        // 02 = Locked (lock icon appears in menu; player is prompted for password if
        //      they choose this game)
        // 04 = In battle (Episode 3; a sword icon appears in menu)
        // 04 = Disabled (BB; used for solo games)
        // 10 = Is battle mode
        // 20 = Is challenge mode
        uint8_t flags;
    } entries[0];
} PACKED bb_game_list_pkt;

// 09 (C->S): Menu item info request
// Internal name: SndInfo
// Server will respond with an 11 command, or an A3 or A5 if the specified menu
// is the quest menu.
typedef struct dc_select {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED dc_select_pkt;

// 09 (C->S): Menu item info request
// Internal name: SndInfo
// Server will respond with an 11 command, or an A3 or A5 if the specified menu
// is the quest menu.
typedef struct bb_select {
    bb_pkt_hdr_t hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED bb_select_pkt;

// 0B: 无效或未解析指令

// 0C (C->S): Create game (DCv1)
// Same format as C1, but fields not supported by v1 (e.g. episode, v2 mode)
// are unused.
typedef struct dcnte_game_create {
    dc_pkt_hdr_t hdr;
    // menu_id and item_id are only used for the E7 (create spectator team) form
    // of this command
    uint32_t menu_id;
    uint32_t item_id;
    char name[16];
    char password[16];
} PACKED dcnte_game_create_pkt;

typedef struct dc_game_create {
    dc_pkt_hdr_t hdr;
    // menu_id and item_id are only used for the E7 (create spectator team) form
    // of this command
    uint32_t menu_id;
    uint32_t item_id;
    char name[16];
    char password[16];
    uint8_t difficulty;
    uint8_t battle;
    uint8_t challenge;
    uint8_t version;                    /* Set to 1 for v2 games, 0 otherwise */
} PACKED dc_game_create_pkt;

typedef struct pc_game_create {
    pc_pkt_hdr_t hdr;
    // menu_id and item_id are only used for the E7 (create spectator team) form
    // of this command
    uint32_t menu_id;
    uint32_t item_id;
    uint16_t name[16];
    uint16_t password[16];
    uint8_t difficulty;
    uint8_t battle;
    uint8_t challenge;
    uint8_t padding;
} PACKED pc_game_create_pkt;

typedef struct gc_game_create {
    dc_pkt_hdr_t hdr;
    // menu_id and item_id are only used for the E7 (create spectator team) form
    // of this command
    uint32_t menu_id;
    uint32_t item_id;
    char name[16];
    char password[16];
    uint8_t difficulty;
    uint8_t battle;
    uint8_t challenge;
    uint8_t episode;
} PACKED gc_game_create_pkt;

typedef struct bb_game_create {
    bb_pkt_hdr_t hdr;
    // menu_id and item_id are only used for the E7 (create spectator team) form
    // of this command
    uint32_t menu_id;
    uint32_t item_id;
    uint16_t name[0x10];
    uint16_t password[0x10];
    uint8_t difficulty;  // 0-3 (always 0 on Episode 3)
    uint8_t battle;      // 0 or 1 (always 0 on Episode 3)
    // Note: Episode 3 uses the challenge mode flag for view battle permissions.
    // 0 = view battle allowed; 1 = not allowed
    uint8_t challenge;   // 0 or 1
    // Note: According to the Sylverant wiki, in v2-land, the episode field has a
    // different meaning: if set to 0, the game can be joined by v1 and v2
    // players; if set to 1, it's v2-only.
    uint8_t episode;     // 1-4 on V3+ (3 on Episode 3); unused on DC/PC
    uint8_t single_player;
    uint8_t padding[3];
} PACKED bb_game_create_pkt;

// 0D: 无效或未解析指令

// 0E (S->C): Incomplete/legacy join game (non-BB)
// Internal name: RcvStartGame
// header.flag = number of valid entries in lobby_data

// This command appears to be a vestige of very early development; its
// second-phase handler is missing even in the earliest public prototype of PSO
// (DC NTE), and the command format is missing some important information
// necessary to start a game on any version.

// There is a failure mode in the 0E command handler that causes the thread
// receiving the command to loop infinitely doing nothing, effectively
// softlocking the game. This happens if the local player's Guild Card number
// doesn't match any of the lobby_data entries. (Notably, only the first
// (header.flag) entries are checked.)
// If the local player's Guild Card number does match one of the entries, the
// command does not softlock, but instead does nothing because the 0E
// second-phase handler is missing.

typedef struct UnknownA1 {
    uint32_t player_tag;
    uint32_t guild_card_number;
    uint8_t name[0x10];
} PACKED UnknownA1_t;

struct S_LegacyJoinGame_PC_0E {
    UnknownA1_t unknown_a1[4];
    uint8_t unknown_a3[0x20];
} PACKED;

typedef struct UnknownA0 {
    uint8_t unknown_a1[2];
    uint16_t unknown_a2;
    uint32_t unknown_a3;
} PACKED UnknownA0_t;

struct S_LegacyJoinGame_GC_0E {
    dc_player_hdr_t lobby_data[4];
    UnknownA0_t unknown_a0[8];
    uint32_t unknown_a1;
    uint8_t unknown_a2[0x20];
    uint8_t unknown_a3[4];
} PACKED;

typedef struct UnknownA1_XB {
    uint32_t player_tag;
    uint32_t guild_card_number;
    uint8_t name[0x18];
} PACKED UnknownA1_XB_t;

struct S_LegacyJoinGame_XB_0E {
    UnknownA1_XB_t unknown_a1[4];
    uint8_t unknown_a2[0x68];
} PACKED;

// 0F: 无效或未解析指令

// 10 (C->S): Menu selection
// Internal name: SndAction
// header.flag contains two flags: 02 specifies if a password is present, and 01
// specifies... something else. These two bits directly correspond to the two
// lowest bits in the flags field of the game menu: 02 specifies that the game
// is locked, but the function of 01 is unknown.
// Annoyingly, the no-arguments form of the command can have any flag value, so
// it doesn't suffice to check the flag value to know which format is being
// used!
//
//struct C_MenuSelection_10_Flag00 {
//  uint32_t menu_id = 0;
//  uint32_t item_id = 0;
//} PACKED;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag01 {
//  C_MenuSelection_10_Flag00 basic_cmd;
//  ptext<CharT, 0x10> unknown_a1;
//} PACKED;
//struct C_MenuSelection_DC_V3_10_Flag01 : C_MenuSelection_10_Flag01<char> {
//} PACKED;
//struct C_MenuSelection_PC_BB_10_Flag01 : C_MenuSelection_10_Flag01<char16_t> {
//} PACKED;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag02 {
//  C_MenuSelection_10_Flag00 basic_cmd;
//  ptext<CharT, 0x10> password;
//} PACKED;
//struct C_MenuSelection_DC_V3_10_Flag02 : C_MenuSelection_10_Flag02<char> {
//} PACKED;
//struct C_MenuSelection_PC_BB_10_Flag02 : C_MenuSelection_10_Flag02<char16_t> {
//} PACKED;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag03 {
//  C_MenuSelection_10_Flag00 basic_cmd;
//  ptext<CharT, 0x10> unknown_a1;
//  ptext<CharT, 0x10> password;
//} PACKED;
//struct C_MenuSelection_DC_V3_10_Flag03 : C_MenuSelection_10_Flag03<char> {
//} PACKED;
//struct C_MenuSelection_PC_BB_10_Flag03 : C_MenuSelection_10_Flag03<char16_t> {
//} PACKED;

// 11 (S->C): Ship info
// Internal name: RcvMessage
// Same format as 01 command. The text appears in a small box in the lower-left
// corner (on V3/BB) or lower-right corner of the screen.
typedef struct dc_info_reply {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t odd[2];
    char msg[];
} PACKED dc_info_reply_pkt;

typedef struct bb_info_reply {
    bb_pkt_hdr_t hdr;
    uint32_t unused[2];
    uint16_t msg[0];
} PACKED bb_info_reply_pkt;

// 12 (S->C): Valid but ignored (all versions)
// Internal name: RcvBaner
// This command's internal name is possibly a misspelling of "banner", which
// could be an early version of the 1A/D5 (large message box) commands, or of
// BB's 00EE (scrolling message) command; however, the existence of
// RcvBanerHead (16) seems to contradict this hypothesis since a text message
// would not require a separate header command. Even on DC NTE, this command
// does nothing, so this must have been scrapped very early in development.

// 13 (S->C): Write online quest file
// Internal name: RcvDownLoad
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A7 instead.
// All chunks except the last must have 0x400 data bytes. When downloading an
// online quest, the .bin and .dat chunks may be interleaved (although newserv
// currently sends them sequentially).

// header.flag = file chunk index (start offset / 0x400)
/* 1044 byts*/
typedef struct quest_chunk {
    char filename[16];
    char data[1024];
    uint32_t data_size;
} PACKED quest_chunk_t;

/* The packet sent to actually send quest data */
/* 1048 byts*/
typedef struct dc_quest_chunk {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    quest_chunk_t data;
} PACKED dc_quest_chunk_pkt;

/* 1052 byts*/
typedef struct bb_quest_chunk {
    bb_pkt_hdr_t hdr;
    quest_chunk_t data;
} PACKED bb_quest_chunk_pkt;

// 13 (C->S): Confirm file write (V3/BB)
// Client sends this in response to each 13 sent by the server. It appears these
// are only sent by V3 and BB - PSO DC and PC do not send these.

// header.flag = file chunk index (same as in the 13/A7 sent by the server)
typedef struct dc_write_quest_file_confirmation {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char filename[16];
} PACKED dc_write_quest_file_confirmation_pkt;

typedef struct bb_write_quest_file_confirmation {
    bb_pkt_hdr_t hdr;
    char filename[16];
} PACKED bb_write_quest_file_confirmation_pkt;

// 14 (S->C): Valid but ignored (all versions)
// Internal name: RcvUpLoad
// Based on its internal name, this command seems like the logical opposite of
// 13 (quest file download, named RcvDownLoad internally). However, even in DC
// NTE, this command does nothing, so it must have been scrapped very early in
// development. There is a SndUpLoad string in the DC versions, but the
// corresponding function was deleted.

// 15: 无效或未解析指令

// 16 (S->C): Valid but ignored (all versions)
// Internal name: RcvBanerHead
// It's not clear what this command was supposed to do, but it's likely related
// to 12 in some way. Like 12, this command does nothing, even on DC NTE.

// 17 (S->C): Start encryption at login server (except on BB)
// Internal name: RcvPsoRegistConnectV2
// Same format and usage as 02 command, but a different copyright string:
// "DreamCast Port Map. Copyright SEGA Enterprises. 1999"
// Unlike the 02 command, V3 clients will respond with a DB command when they
// receive a 17 command in any online session, with the exception of Episodes
// 1&2 trial edition (which responds with a 9A). DCv1 will respond with a 90. DC
// NTE will respond with an 8B. Other non-V3 clients will respond with a 9A or
// 9D.

// 18 (S->C): License verification result (PC/V3)
// Behaves exactly the same as 9A (S->C). No arguments except header.flag.

// 19 (S->C): Reconnect to different address
// Internal name: RcvPort
// Client will disconnect, and reconnect to the given address/port. Encryption
// will be disabled on the new connection; the server should send an appropriate
// command to enable it when the client connects.
// Note: PSO XB seems to ignore the address field, which makes sense given its
// networking architecture.

//struct S_Reconnect_19 : S_Reconnect<uint16_t> { } PACKED;

// Because PSO PC and some versions of PSO DC/GC use the same port but different
// protocols, we use a specially-crafted 19 command to send them to two
// different ports depending on the client version. I first saw this technique
// used by Schthack; I don't know if it was his original creation.
//struct S_ReconnectSplit_19 {
//    be_uint32_t pc_address;
//    uint16_t pc_port;
//    parray<uint8_t, 0x0F> unused1;
//    uint8_t gc_command = 0x19;
//    uint8_t gc_flag;
//    uint16_t gc_size = 0x97;
//    be_uint32_t gc_address;
//    uint16_t gc_port;
//    parray<uint8_t, 0xB0 - 0x23> unused2;
//} PACKED;

// 1A (S->C): Large message box
// Internal name: RcvText
// On V3, client will sometimes respond with a D6 command (see D6 for more
// information).
// Contents are plain text (char on DC/V3, char16_t on PC/BB). There must be at
// least one null character ('\0') before the end of the command data.
// There is a bug in V3 (and possibly all versions) where if this command is
// sent after the client has joined a lobby, the chat log window contents will
// appear in the message box, prepended to the message text from the command.
// The maximum length of the message is 0x400 bytes. This is the only
// difference between this command and the D5 command.

// 1B (S->C): Valid but ignored (all versions)
// Internal name: RcvBattleData
// This command does nothing in all PSO versions. There is a SndBattleData
// string in the DC versions, but the corresponding function was deleted.

// 1C (S->C): Valid but ignored (all versions)
// Internal name: RcvSystemFile
// This command does nothing in all PSO versions.

// 1D: Ping
// Internal name: RcvPing
// No arguments
// When sent to the client, the client will respond with a 1D command. Data sent
// by the server is ignored; the client always sends a 1D command with no data.

// 1E: Invalid command

// 1F (C->S): Request information menu
// Internal name: SndTextList
// No arguments
// This command is used in PSO DC and PC. It exists in V3 as well but is
// apparently unused.

// 1F (S->C): 信息菜单
// 内部名称: RcvTextList
// 与 07 命令相同的格式和用法，除了：
// - 菜单标题将显示为“信息”而不是“选择飞船”。
// - 在选择菜单项之前没有请求详细信息的方式（客户端不会发送 09 命令）。
// - 玩家可以按下一个按钮（例如GC上的B按钮）
// 关闭菜单而不选择任何内容，与飞船选择菜单不同。当这种情况发生时，客户端不会发送任何内容。

// 20: Invalid command

// 21: GameGuard control (old versions of BB)
// Format unknown

// 22: GameGuard check (BB)
// Command 0022 is a 16-byte challenge (sent in the data field) using the
// following structure.
typedef struct bb_gamecard_check_request {
    bb_pkt_hdr_t hdr; /* flags 0x00000000 Done 0x00000001 unDone*/
    uint32_t data[4];
} PACKED bb_gamecard_check_request_pkt;

// 22: GameGuard check (BB)
// Command 0122 uses a 4-byte challenge sent in the header.flag field instead.
// This version of the command has no other arguments.
typedef struct bb_gamecard_check_done {
    bb_pkt_hdr_t hdr; /* flags 0x00000000 Done 0x00000001 unDone*/
} PACKED bb_gamecard_check_done_pkt;

// 23 (S->C): Momoka Item Exchange result (BB)
// Sent in response to a 6xD9 command from the client.
// header.flag indicates if an item was exchanged: 0 means success, 1 means
// failure.

// 24 (S->C): 祝您好运的结果（BB）
// 在客户端发送的 6xDE 命令的响应中发送。
// header.flag 指示客户端的库存中是否有任何秘密彩票（从而可以参与）：0 表示成功，1 表示失败。
typedef struct bb_item_exchange_state {
    bb_pkt_hdr_t hdr; /* flags 0x00000000 Done 0x00000001 unDone*/
} PACKED bb_item_exchange_state_pkt;

typedef struct bb_item_exchange_good_luck {
    /* 0x00 - 0x07 */bb_pkt_hdr_t hdr;
    /* 0x08 - 0x09 */uint16_t subcmd_code;
    /* 0x0A - 0x0B */uint16_t flags;
    /* 0x0C - */ uint32_t items_res[8];
} PACKED bb_item_exchange_good_luck_pkt;

// 25 (S->C): Gallon's Plan result (BB)
// Sent in response to a 6xE1 command from the client.
typedef struct bb_item_exchange_gallon_result {
    bb_pkt_hdr_t hdr;
    uint16_t subcmd_code;
    uint8_t offset1;
    uint8_t value1;
    uint8_t offset2;
    uint8_t value2;
    uint16_t unused;
} PACKED bb_item_exchange_gallon_result_pkt;

// 26: 无效或未解析指令
// 27: 无效或未解析指令
// 28: 无效或未解析指令
// 29: 无效或未解析指令
// 2A: 无效或未解析指令
// 2B: 无效或未解析指令
// 2C: 无效或未解析指令
// 2D: 无效或未解析指令
// 2E: 无效或未解析指令
// 2F: 无效或未解析指令
// 30: 无效或未解析指令
// 31: 无效或未解析指令
// 32: 无效或未解析指令
// 33: 无效或未解析指令
// 34: 无效或未解析指令
// 35: 无效或未解析指令
// 36: 无效或未解析指令
// 37: 无效或未解析指令
// 38: 无效或未解析指令
// 39: 无效或未解析指令
// 3A: 无效或未解析指令
// 3B: 无效或未解析指令
// 3C: 无效或未解析指令
// 3D: 无效或未解析指令
// 3E: 无效或未解析指令
// 3F: 无效或未解析指令

// 40 (C->S): Guild card search
// Internal name: SndFindUser
// There is an unused command named SndFavorite in the DC versions of PSO,
// which may have been related to this command. SndFavorite seems to be
// completely unused; its sender function was optimized out of all known
// builds, leaving only its name string remaining.
// The server should respond with a 41 command if the target is online. If the
// target is not online, the server doesn't respond at all.
typedef struct dc_guild_search {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
} PACKED dc_guild_search_pkt;

// 40 (C->S): Guild card search
// Internal name: SndFindUser
// There is an unused command named SndFavorite in the DC versions of PSO,
// which may have been related to this command. SndFavorite seems to be
// completely unused; its sender function was optimized out of all known
// builds, leaving only its name string remaining.
// The server should respond with a 41 command if the target is online. If the
// target is not online, the server doesn't respond at all.
typedef struct bb_guild_search {
    bb_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
} PACKED bb_guild_search_pkt;

// 41 (S->C): Guild card search result
// Internal name: RcvUserAns
typedef struct dc_guild_reply {
    dc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
    uint32_t padding1;
    uint32_t ip;
    uint16_t port;
    uint16_t padding2;
    char location[0x44];
    uint32_t menu_id;
    uint32_t item_id;
    char padding3[0x3C];
    char name[0x20];
} PACKED dc_guild_reply_pkt;

typedef struct pc_guild_reply {
    pc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
    uint32_t padding1;
    uint32_t ip;
    uint16_t port;
    uint16_t padding2;
    uint16_t location[0x44];
    uint32_t menu_id;
    uint32_t item_id;
    uint8_t padding3[0x3C];
    uint16_t name[0x20];
} PACKED pc_guild_reply_pkt;

typedef struct bb_guild_reply {
    bb_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
    uint32_t padding1;
    uint32_t padding2;
    uint32_t ip;
    uint16_t port;
    uint16_t padding3;
    uint16_t location[0x44];
    uint32_t menu_id;
    uint32_t item_id;
    uint8_t padding4[0x3C];
    uint16_t name[0x20];
} PACKED bb_guild_reply_pkt;

/* IPv6 versions of the above two packets */
typedef struct dc_guild_reply6 {
    dc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
    uint32_t padding1;
    uint8_t ip[16];
    uint16_t port;
    uint16_t padding2;
    char location[0x44];
    uint32_t menu_id;
    uint32_t item_id;
    char padding3[0x3C];
    char name[0x20];
} PACKED dc_guild_reply6_pkt;

typedef struct pc_guild_reply6 {
    pc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
    uint32_t padding1;
    uint8_t ip[16];
    uint16_t port;
    uint16_t padding2;
    uint16_t location[0x44];
    uint32_t menu_id;
    uint32_t item_id;
    uint8_t padding3[0x3C];
    uint16_t name[0x20];
} PACKED pc_guild_reply6_pkt;

typedef struct bb_guild_reply6 {
    bb_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
    uint32_t padding1;
    uint32_t padding2;
    uint8_t ip[16];
    uint16_t port;
    uint16_t padding3;
    uint16_t location[0x44];
    uint32_t menu_id;
    uint32_t item_id;
    uint8_t padding4[0x3C];
    uint16_t name[0x20];
} PACKED bb_guild_reply6_pkt;

//// 42: 无效或未解析指令
//// 43: 无效或未解析指令

// 44 (S->C): Open file for download
// Internal name: RcvDownLoadHead
// Used for downloading online quests. The client will react to a 44 command if
// the filename ends in .bin or .dat.
// For download quests (to be saved to the memory card) and GBA games, the A6
// command is used instead. The client will react to A6 if the filename ends in
// .bin/.dat (quests), .pvr (textures), or .gba (GameBoy Advance games).
// It appears that the .gba handler for A6 was not deleted in PSO XB, even
// though it doesn't make sense for an XB client to receive such a file.

//struct S_OpenFile_DC_44_A6 {
//    ptext<char, 0x22> name; // Should begin with "PSO/"
//    // The type field is only used for download quests (A6); it is ignored for
//    // online quests (44). The following values are valid for A6:
//    //   0 = download quest (client expects .bin and .dat files)
//    //   1 = download quest (client expects .bin, .dat, and .pvr files)
//    //   2 = GBA game (GC only; client expects .gba file only)
//    //   3 = Episode 3 download quest (Ep3 only; client expects .bin file only)
//    // There is a bug in the type logic: an A6 command always overwrites the
//    // current download type even if the filename doesn't end in .bin, .dat, .pvr,
//    // or .gba. This may lead to a resource exhaustion bug if exploited carefully,
//    // but I haven't verified this. Generally the server should send all files for
//    // a given piece of content with the same type in each file's A6 command.
//    uint8_t type = 0;
//    ptext<char, 0x11> filename;
//    uint32_t file_size = 0;
//} PACKED;
//
//struct S_OpenFile_PC_V3_44_A6 {
//    ptext<char, 0x22> name; // Should begin with "PSO/"
//    le_uint16_t type = 0;
//    ptext<char, 0x10> filename;
//    uint32_t file_size = 0;
//} PACKED;
//
//// Curiously, PSO XB expects an extra 0x18 bytes at the end of this command, but
//// those extra bytes are unused, and the client does not fail if they're
//// omitted.
//struct S_OpenFile_XB_44_A6 : S_OpenFile_PC_V3_44_A6 {
//    parray<uint8_t, 0x18> unused2;
//} PACKED;
//
//struct S_OpenFile_BB_44_A6 {
//    parray<uint8_t, 0x22> unused;
//    le_uint16_t type = 0;
//    ptext<char, 0x10> filename;
//    uint32_t file_size = 0;
//    ptext<char, 0x18> name;
//} PACKED;


// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
/* The packet sent to inform a client of a quest file that will be coming down
   the pipe */
typedef struct dc_quest_file {
    dc_pkt_hdr_t hdr;
    char name[34];// Should begin with "PSO/"
  // The type field is only used for download quests (A6); it is ignored for
  // online quests (44). The following values are valid for A6:
  //   0 = download quest (client expects .bin and .dat files)
  //   1 = download quest (client expects .bin, .dat, and .pvr files)
  //   2 = GBA game (GC only; client expects .gba file only)
  //   3 = Episode 3 download quest (Ep3 only; client expects .bin file only)
  // There is a bug in the type logic: an A6 command always overwrites the
  // current download type even if the filename doesn't end in .bin, .dat, .pvr,
  // or .gba. This may lead to a resource exhaustion bug if exploited carefully,
  // but I haven't verified this. Generally the server should send all files for
  // a given piece of content with the same type in each file's A6 command.
    uint8_t type;
    char filename[17];
    uint32_t file_size;
} PACKED dc_quest_file_pkt;

// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
typedef struct pc_quest_file {
    pc_pkt_hdr_t hdr;
    char name[34]; // Should begin with "PSO/"
    uint16_t type;
    char filename[16];
    uint32_t file_size;
} PACKED pc_quest_file_pkt;

// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
typedef struct gc_quest_file {
    dc_pkt_hdr_t hdr;
    char name[34]; // Should begin with "PSO/"
    uint16_t type;
    char filename[16];
    uint32_t file_size;
} PACKED gc_quest_file_pkt;

// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
typedef struct xb_quest_file {
    dc_pkt_hdr_t hdr;
    char name[34]; // Should begin with "PSO/"
    uint16_t type;
    char filename[16];
    uint32_t file_size;
    uint8_t unused2[0x18];
} PACKED xb_quest_file_pkt;

// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
typedef struct bb_quest_file {
    bb_pkt_hdr_t hdr;
    char unused1[34];
    uint16_t type;
    char filename[16];
    uint32_t file_size;
    char name[24];
} PACKED bb_quest_file_pkt;

// 44 (C->S): Confirm open file (V3/BB)
// Client sends this in response to each 44 sent by the server.
// header.flag = quest number (sort of - seems like the client just echoes
// whatever the server sent in its header.flag field. Also quest numbers can be
// > 0xFF so the flag is essentially meaningless)
typedef struct dc_open_quest_file_confirmation {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char filename[0x10];
} PACKED dc_open_quest_file_confirmation_pkt;

typedef struct bb_open_quest_file_confirmation {
    bb_pkt_hdr_t hdr;
    char filename[0x10];
} PACKED bb_open_quest_file_confirmation_pkt;

// 45: 无效或未解析指令
// 46: 无效或未解析指令
// 47: 无效或未解析指令
// 48: 无效或未解析指令
// 49: 无效或未解析指令
// 4A: 无效或未解析指令
// 4B: 无效或未解析指令
// 4C: 无效或未解析指令
// 4D: 无效或未解析指令
// 4E: 无效或未解析指令
// 4F: 无效或未解析指令
// 50: 无效或未解析指令
// 51: 无效或未解析指令
// 52: 无效或未解析指令
// 53: 无效或未解析指令
// 54: 无效或未解析指令
// 55: 无效或未解析指令
// 56: 无效或未解析指令
// 57: 无效或未解析指令
// 58: 无效或未解析指令
// 59: 无效或未解析指令
// 5A: 无效或未解析指令
// 5B: 无效或未解析指令
// 5C: 无效或未解析指令
// 5D: 无效或未解析指令
// 5E: 无效或未解析指令
// 5F: 无效或未解析指令

// 60：广播命令
// 内部名称：SndPsoData
// 当客户端发送此命令时，服务器应将其转发给所有玩家
// 在同一个游戏/大厅，除了最初发送命令的玩家。
// 有关内容的详细信息，请参阅ReceiveSubcommands或下面的子命令索引。
// 此命令中的数据长度最多可达0x400字节。如果它更大，
// 客户端将表现出未定义的行为。

//61（C->S）：玩家数据
//内部名称：SndCharaDataV2（DCv1中的SndChara Data）
//有关此命令的格式，请参阅Player.hh中的PSOPlayerData结构。
//header.flag指定格式版本，该版本与
//与）游戏的主要版本相同。例如，格式版本为01
//DC v1上，PSO PC上02，PSO GC、XB和BB上03，第3集04。
//加入游戏后，客户端将库存项目ID按顺序分配为
//（0x00010000+（0x00200000*lobby_client_id）+x）。例如，玩家
//3的第8个项目的ID将变为0x00610007。上次游戏中的物品ID
//玩家将出现在他们的库存中。
//注意：如果客户端在收到此命令时正在游戏中，则
//客户发送的库存只包括在以下情况下不会消失的物品
//客户端崩溃！本质上，它反映了玩家的保存状态
//性格而非活动状态。

//struct PlayerRecordsEntry_DC {
//    /* 00 */ uint32_t client_id;
//    /* 04 */ dc_c_rank_records_t challenge;
//    /* A4 */ battle_records_t battle;
//    /* BC */
//} PACKED;
//
//struct PlayerRecordsEntry_PC {
//    /* 00 */ uint32_t client_id;
//    /* 04 */ pc_c_rank_records_t challenge;
//    /* DC */ battle_records_t battle;
//    /* F4 */
//} PACKED;
//
//struct PlayerRecordsEntry_V3 {
//    /* 0000 */ uint32_t client_id;
//    /* 0004 */ PlayerRecordsV3_Challenge<false> challenge;
//    /* 0104 */ battle_records_t battle;
//    /* 011C */
//} PACKED;
//
//struct PlayerRecordsEntry_BB {
//    /* 0000 */ uint32_t client_id;
//    /* 0004 */ PlayerRecordsBB_Challenge challenge;
//    /* 0144 */ battle_records_t battle;
//    /* 015C */
//} PACKED;

//struct C_CharacterData_DCv1_61_98 {
//    /* 0000 */ inventory_t inv;
//    /* 034C */ PlayerDispDataDCPCV3 disp;
//    /* 041C */
//} PACKED;
//
//struct C_CharacterData_DCv2_61_98 {
//    /* 0000 */ inventory_t inv;
//    /* 034C */ PlayerDispDataDCPCV3 disp;
//    /* 041C */ PlayerRecordsEntry_DC records;
//    /* 04D8 */ ChoiceSearchConfig<le_uint16_t> choice_search_config;
//    /* 04F0 */
//} PACKED;
//
//struct C_CharacterData_PC_61_98 {
//    /* 0000 */ inventory_t inv;
//    /* 034C */ PlayerDispDataDCPCV3 disp;
//    /* 041C */ PlayerRecordsEntry_PC records;
//    /* 0510 */ ChoiceSearchConfig<le_uint16_t> choice_search_config;
//    /* 0528 */ parray<uint32_t, 0x1E> blocked_senders;
//    /* 05A0 */ uint32_t auto_reply_enabled;
//    // The auto-reply message can be up to 0x200 characters. If it's shorter than
//    // that, the client truncates the command after the first null value (rounded
//    // up to the next 4-byte boundary).
//    /* 05A4 */ char16_t auto_reply[0];
//} PACKED;
//
//struct C_CharacterData_V3_61_98 {
//    /* 0000 */ inventory_t inv;
//    /* 034C */ PlayerDispDataDCPCV3 disp;
//    /* 041C */ PlayerRecordsEntry_V3 records;
//    /* 0538 */ ChoiceSearchConfig<le_uint16_t> choice_search_config;
//    /* 0550 */ ptext<char, 0xAC> info_board;
//    /* 05FC */ parray<uint32_t, 0x1E> blocked_senders;
//    /* 0674 */ uint32_t auto_reply_enabled;
//    // The auto-reply message can be up to 0x200 bytes. If it's shorter than that,
//    // the client truncates the command after the first zero byte (rounded up to
//    // the next 4-byte boundary).
//    /* 0678 */ char auto_reply[0];
//} PACKED;
//
//struct C_CharacterData_GC_Ep3_61_98 {
//    /* 0000 */ inventory_t inv;
//    /* 034C */ PlayerDispDataDCPCV3 disp;
//    /* 041C */ PlayerRecordsEntry_V3 records;
//    /* 0538 */ ChoiceSearchConfig<le_uint16_t> choice_search_config;
//    /* 0550 */ ptext<char, 0xAC> info_board;
//    /* 05FC */ parray<uint32_t, 0x1E> blocked_senders;
//    /* 0674 */ uint32_t auto_reply_enabled;
//    /* 0678 */ ptext<char, 0xAC> auto_reply;
//    /* 0724 */ Episode3::PlayerConfig ep3_config;
//    /* 2A74 */
//} PACKED;
//
//struct C_CharacterData_BB_61_98 {
//    /* 0000 */ inventory_t inv;
//    /* 034C */ PlayerDispDataBB disp;
//    /* 04DC */ PlayerRecordsEntry_BB records;
//    /* 0638 */ ChoiceSearchConfig<le_uint16_t> choice_search_config;
//    /* 0650 */ ptext<char16_t, 0xAC> info_board;
//    /* 07A8 */ parray<uint32_t, 0x1E> blocked_senders;
//    /* 0820 */ uint32_t auto_reply_enabled;
//    // Like on V3, the client truncates the command if the auto reply message is
//    // shorter than 0x200 bytes.
//    /* 082C */ char16_t auto_reply[0];
//} PACKED;

//62：目标命令
//内部名称：SndPsoData2
//当客户端发送此命令时，服务器应将其转发给玩家
//由同一游戏/大厅中的header.flag标识，即使该玩家是
//最初发送的玩家。
//有关内容的详细信息，请参阅ReceiveSubcommands或下面的子命令索引。
//此命令中的数据长度最多可达0x400字节。如果它更大，
//客户端将表现出未定义的行为。

// 63: 无效或未解析指令

#ifdef PLAYER_H

//64（S->C）：加入游戏
//内部名称：RcvStartGame3
//这将发送给加入的玩家；其他玩家得到65指令。
//注意（第3集除外）此命令不包括玩家的
//disp或库存数据。游戏中的客户端负责发送
//该数据在具有60/62/6C/6D命令的联接过程期间相互传递。
//奇怪的是，这个命令在内部被命名为RcvStartGame3，而0E则被命名为
//RcvStartGame。字符串RcvStartGame2出现在DC版本中，但它
//相关代码似乎已被删除-没有对该字符串的引用。
//基于命令0E和64之间的巨大差距，我们无法猜测
//命令号RcvStartGame2可能是。
typedef struct dcnte_game_join {
    dc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;
    uint8_t unused;
    uint32_t maps[0x20];
    dc_player_hdr_t players[4];
} PACKED dcnte_game_join_pkt;

// 64 (S->C): Join game
// This is sent to the joining player; the other players get a 65 instead.
// Note that (except on Episode 3) this command does not include the player's
// disp or inventory data. The clients in the game are responsible for sending
// that data to each other during the join process with 60/62/6C/6D commands.
// 68 (S->C): Add player to lobby
// Same format as 65 command, but used for lobbies instead of games.
// The command only includes the joining player's data.
typedef struct dc_game_join {
    dc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    dc_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1. */
    uint8_t difficulty;
    uint8_t battle;
    uint8_t event;
    uint8_t section;
    uint8_t challenge;
    uint32_t rand_seed;
} PACKED dc_game_join_pkt;

// 64 (S->C): Join game
// This is sent to the joining player; the other players get a 65 instead.
// Note that (except on Episode 3) this command does not include the player's
// disp or inventory data. The clients in the game are responsible for sending
// that data to each other during the join process with 60/62/6C/6D commands.
// 68 (S->C): Add player to lobby
// Same format as 65 command, but used for lobbies instead of games.
// The command only includes the joining player's data.
typedef struct pc_game_join {
    pc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    pc_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1. */
    uint8_t difficulty;
    uint8_t battle;
    uint8_t event;
    uint8_t section;
    uint8_t challenge;
    uint32_t rand_seed;
} PACKED pc_game_join_pkt;

// 64 (S->C): Join game
// This is sent to the joining player; the other players get a 65 instead.
// Note that (except on Episode 3) this command does not include the player's
// disp or inventory data. The clients in the game are responsible for sending
// that data to each other during the join process with 60/62/6C/6D commands.
// 68 (S->C): Add player to lobby
// Same format as 65 command, but used for lobbies instead of games.
// The command only includes the joining player's data.
typedef struct gc_game_join {
    dc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    dc_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1. */
    uint8_t difficulty;
    uint8_t battle;
    uint8_t event;
    uint8_t section;
    uint8_t challenge;
    uint32_t rand_seed;
    uint8_t episode;
    uint8_t one2;                       /* Always 1. */
    uint16_t padding;
} PACKED gc_game_join_pkt;

// 64 (S->C): Join game
// This is sent to the joining player; the other players get a 65 instead.
// Note that (except on Episode 3) this command does not include the player's
// disp or inventory data. The clients in the game are responsible for sending
// that data to each other during the join process with 60/62/6C/6D commands.
// 68 (S->C): Add player to lobby
// Same format as 65 command, but used for lobbies instead of games.
// The command only includes the joining player's data.
typedef struct xb_game_join {
    dc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    xbox_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1. */
    uint8_t difficulty;
    uint8_t battle;
    uint8_t event;
    uint8_t section;
    uint8_t challenge;
    uint32_t rand_seed;
    uint8_t episode;
    uint8_t one2;                       /* Always 1. */
    uint16_t padding;
} PACKED xb_game_join_pkt;

// 64 (S->C): Join game
// This is sent to the joining player; the other players get a 65 instead.
// Note that (except on Episode 3) this command does not include the player's
// disp or inventory data. The clients in the game are responsible for sending
// that data to each other during the join process with 60/62/6C/6D commands.
// 68 (S->C): Add player to lobby
// Same format as 65 command, but used for lobbies instead of games.
// The command only includes the joining player's data.
typedef struct ep3_game_join {
    dc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    dc_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1. */
    uint8_t difficulty;
    uint8_t battle;
    uint8_t event;
    uint8_t section;
    uint8_t challenge;
    uint32_t rand_seed;
    uint8_t episode;
    uint8_t one2;                       /* Always 1. */
    uint16_t padding;
    v1_player_t player_data[4];
} PACKED ep3_game_join_pkt;

// 64 (S->C): Join game
// This is sent to the joining player; the other players get a 65 instead.
// Note that (except on Episode 3) this command does not include the player's
// disp or inventory data. The clients in the game are responsible for sending
// that data to each other during the join process with 60/62/6C/6D commands.
// 68 (S->C): Add player to lobby
// Same format as 65 command, but used for lobbies instead of games.
// The command only includes the joining player's data.
typedef struct bb_game_join {
    bb_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    bb_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1. */
    uint8_t difficulty;
    uint8_t battle;
    uint8_t event;
    uint8_t section;
    uint8_t challenge;
    uint32_t rand_seed;
    uint8_t episode;
    uint8_t one2;                       /* Always 1. */
    uint8_t single_player;
    uint8_t unused;
} PACKED bb_game_join_pkt;

// 65 (S->C): Add player to game
// When a player joins an existing game, the joining player receives a 64
// command (described above), and the players already in the game receive a 65
// command containing only the joining player's data.
// Header flag = entry count (always 1 for 65 and 68; up to 0x0C for 67)
// 67 (S->C): Join lobby
// This is sent to the joining player; the other players receive a 68 instead.
// Same format as 65 command, but used for lobbies instead of games.
typedef struct dcnte_lobby_join {
    dc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint16_t padding;
    struct {
        dc_player_hdr_t hdr;
        v1_player_t data;
    } entries[0];
} PACKED dcnte_lobby_join_pkt;

// 65 (S->C): Add player to game
// Internal name: RcvBurstGame
// When a player joins an existing game, the joining player receives a 64
// command (described above), and the players already in the game receive a 65
// command containing only the joining player's data.

//typedef struct {
//    uint8_t client_id;
//    uint8_t leader_id;
//    uint8_t disable_udp;
//    uint8_t unused;
//} LobbyFlags_DCNTE;
//
//typedef struct {
//    uint8_t client_id;
//    uint8_t leader_id;
//    uint8_t disable_udp;
//    uint8_t lobby_number;
//    uint8_t block_number;
//    uint8_t unknown_a1;
//    uint8_t event;
//    uint8_t unknown_a2;
//    uint32_t unused;
//} LobbyFlags;
//
//typedef struct {
//    LobbyFlags lobby_flags;
//
//    struct {
//        PlayerLobbyDataDCGC lobby_data;
//        PlayerInventory inventory;
//        PlayerDispDataDCPCV3 disp;
//    } entry;
//
//    struct {
//        PlayerLobbyDataDCGC lobby_data;
//        PlayerInventory inventory;
//        PlayerDispDataDCPCV3 disp;
//    } entries[12];
//} S_JoinLobby_DCNTE_65_67_68;
//
//typedef struct {
//    LobbyFlags lobby_flags;
//
//    struct {
//        PlayerLobbyDataPC lobby_data;
//        PlayerInventory inventory;
//        PlayerDispDataDCPCV3 disp;
//    } entry;
//
//    struct {
//        PlayerLobbyDataPC lobby_data;
//        PlayerInventory inventory;
//        PlayerDispDataDCPCV3 disp;
//    } entries[12];
//} S_JoinLobby_PC_65_67_68;
//
//typedef struct {
//    LobbyFlags lobby_flags;
//
//    struct {
//        PlayerLobbyDataDCGC lobby_data;
//        PlayerInventory inventory;
//        PlayerDispDataDCPCV3 disp;
//    } entry;
//
//    struct {
//        PlayerLobbyDataDCGC lobby_data;
//        PlayerInventory inventory;
//        PlayerDispDataDCPCV3 disp;
//    } entries[12];
//} S_JoinLobby_DC_GC_65_67_68_Ep3_EB;
//
//typedef struct {
//    LobbyFlags lobby_flags;
//    uint32_t unknown_a4[6];
//
//    struct {
//        PlayerLobbyDataXB lobby_data;
//        PlayerInventory inventory;
//        PlayerDispDataDCPCV3 disp;
//    } entry;
//
//    struct {
//        PlayerLobbyDataXB lobby_data;
//        PlayerInventory inventory;
//        PlayerDispDataDCPCV3 disp;
//    } entries[12];
//} S_JoinLobby_XB_65_67_68;




// 67 (S->C): Join lobby
// This is sent to the joining player; the other players receive a 68 instead.
// Same format as 65 command, but used for lobbies instead of games.
typedef struct dc_lobby_join {
    dc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1 */
    uint8_t lobby_num;
    uint16_t block_num;
    uint16_t event;
    uint32_t padding;
    struct {
        dc_player_hdr_t hdr;
        v1_player_t data;
    } entries[0];
} PACKED dc_lobby_join_pkt;

// 65 (S->C): Add player to game
// When a player joins an existing game, the joining player receives a 64
// command (described above), and the players already in the game receive a 65
// command containing only the joining player's data.
// Header flag = entry count (always 1 for 65 and 68; up to 0x0C for 67)
// 67 (S->C): Join lobby
// This is sent to the joining player; the other players receive a 68 instead.
// Same format as 65 command, but used for lobbies instead of games.
typedef struct pc_lobby_join {
    pc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1 */
    uint8_t lobby_num;
    uint16_t block_num;
    uint16_t event;
    uint32_t padding;
    struct {
        pc_player_hdr_t hdr;
        v1_player_t data;
    } entries[0];
} PACKED pc_lobby_join_pkt;

// 65 (S->C): Add player to game
// When a player joins an existing game, the joining player receives a 64
// command (described above), and the players already in the game receive a 65
// command containing only the joining player's data.
// Header flag = entry count (always 1 for 65 and 68; up to 0x0C for 67)
// 67 (S->C): Join lobby
// This is sent to the joining player; the other players receive a 68 instead.
// Same format as 65 command, but used for lobbies instead of games.
typedef struct xb_lobby_join {
    dc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* Always 1 */
    uint8_t lobby_num;
    uint16_t block_num;
    uint16_t event;
    uint32_t padding;
    uint8_t unk[0x18];
    struct {
        xbox_player_hdr_t hdr;
        v1_player_t data;
    } entries[0];
} PACKED xb_lobby_join_pkt;

// 65 (S->C): Add player to game
// When a player joins an existing game, the joining player receives a 64
// command (described above), and the players already in the game receive a 65
// command containing only the joining player's data.
// Header flag = entry count (always 1 for 65 and 68; up to 0x0C for 67)
// 67 (S->C): Join lobby
// This is sent to the joining player; the other players receive a 68 instead.
// Same format as 65 command, but used for lobbies instead of games.
typedef struct bb_lobby_join {
    bb_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;                        /* 默认为 0x01 */
    uint8_t lobby_num;
    uint8_t block_num;
    uint8_t unknown_a1;
    uint8_t event;
    uint8_t unknown_a2;
    uint8_t unknown_a3[4];
    struct {
        bb_player_hdr_t hdr;
        //inventory_t inv;
        psocn_bb_char_t data;
    } entries[0];
} PACKED bb_lobby_join_pkt;

#endif

// 66 (S->C): Remove player from game
// Internal name: RcvExitGame
// This is sent to all players in a game except the leaving player.
// header.flag should be set to the leaving player ID (same as client_id).
// 69 (S->C): Remove player from lobby
// Same format as 66 command, but used for lobbies instead of games.
typedef struct dc_lobby_leave {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;
    uint8_t unused;
} PACKED dc_lobby_leave_pkt;

// 66 (S->C): Remove player from game
// This is sent to all players in a game except the leaving player.
// header.flag should be set to the leaving player ID (same as client_id).

// 67 (S->C): Join lobby
// Internal name: RcvStartLobby2
// This is sent to the joining player; the other players receive a 68 instead.
// Same format as 65 command, but used for lobbies instead of games.

// Curiously, this command is named RcvStartLobby2 internally, but there is no
// command named RcvStartLobby. The string "RcvStartLobby" does appear in the DC
// game executable, but it appears the relevant code was deleted.

// 68 (S->C): Add player to lobby
// Internal name: RcvBurstLobby
// Same format as 65 command, but used for lobbies instead of games.
// The command only includes the joining player's data.

// 69 (S->C): Remove player from lobby
// Internal name: RcvExitLobby
// Same format as 66 command, but used for lobbies instead of games.
typedef struct bb_lobby_leave {
    bb_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t disable_udp;
    uint8_t unused;
} PACKED bb_lobby_leave_pkt;

// 6A: Invalid command
// 6B: Invalid command

//6C：广播命令
//内部名称：RcvSoDataLong和SndPsoDataLong
//与60命令的格式和用法相同，但没有大小限制。

//6D：目标命令
//内部名称：RcvSoDataLong和SndPsoDataLong2
//与62命令的格式和用法相同，但没有大小限制。

// 6E: Invalid command

//6F（C->S）：设置游戏状态
//内部名称：SndBurstEnd
//此命令是在玩家完成加载后发送的，然后其他玩家可以
//加入游戏。在BB上，如果任务正在进行，则此命令将作为016F发送
//其他人不应该加入游戏。
typedef struct bb_done_burst {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
        bb_pkt_hdr_t bb;
    };
    uint8_t data[];
} PACKED bb_done_burst_pkt;

// 016F
typedef struct bb_done_quest_burst {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
        bb_pkt_hdr_t bb;
    };
    uint8_t data[];
} PACKED bb_done_quest_burst_pkt;

// 70: 无效或未解析指令
// 71: 无效或未解析指令
// 72: 无效或未解析指令
// 73: 无效或未解析指令
// 74: 无效或未解析指令
// 75: 无效或未解析指令
// 76: 无效或未解析指令
// 77: 无效或未解析指令
// 78: 无效或未解析指令
// 79: 无效或未解析指令
// 7A: 无效或未解析指令
// 7B: 无效或未解析指令
// 7C: 无效或未解析指令
// 7D: 无效或未解析指令
// 7E: 无效或未解析指令
// 7F: 无效或未解析指令

//80:有效但被忽略（所有版本）
//内部名称：RcvGenerateID和SndGenerateID
//此命令似乎用于为给定玩家设置下一个物品ID
//插槽。PSOV3及以后版本接受此命令，但完全忽略它。尤其是
//除了DC NTE之外，没有任何版本的PSO发送过此命令-很可能是
//用于实现某些项ID同步语义，这些语义后来更改为
//把领导者当作真理的源泉。
typedef struct C_GenerateID_DCNTE_80 {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t id;
    uint8_t unused1; // Always 0
    uint8_t unused2; // Always 0
    uint16_t unused3; // Always 0
    uint8_t unused4[4]; // Client sends uninitialized data here
} PACKED C_GenerateID_DCNTE_80_t;

struct S_GenerateID_DC_PC_V3_80 {
    bb_pkt_hdr_t bb;
    uint32_t client_id; // = 0
    uint32_t unused;
    uint32_t next_item_id;
} PACKED;

//81：简单邮件
//内部名称：RcvChatMessage和SndChatMessage
//两个方向的格式相同。服务器应转发命令
//to_guild_card_number的玩家，如果他们在线的话。如果不是
//在线时，服务器可能会将其存储以备稍后交付，并发送自动回复
//将消息返回给原始发件人，或者简单地丢弃该消息。
//在GC（可能还有其他版本）中，文本后面未使用的空间
//当客户端发送此命令时，包含未初始化的内存。newserv
//出于安全原因，在转发之前清除未初始化的数据。
typedef struct dc_simple_mail {
    dc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_sender;
    char from_name[16];
    uint32_t gc_dest;
    char stuff[0x200]; /* Start = 0x20, end = 0xB0 */
} PACKED dc_simple_mail_pkt;

typedef struct pc_simple_mail {
    pc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_sender;
    uint16_t from_name[16];
    uint32_t gc_dest;
    char stuff[0x400]; /* Start = 0x30, end = 0x150 */
} PACKED pc_simple_mail_pkt;

typedef struct bb_simple_mail {
    bb_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_sender;
    uint16_t from_name[16];
    uint32_t gc_dest;
    uint16_t timestamp[20];
    uint16_t message[0xAC];
    uint8_t unk2[0x2A0];
} PACKED bb_simple_mail_pkt;

typedef struct dc_mail_data {
    char dcname[0x10];
    uint32_t gc_dest;
    char stuff[0x200]; /* Start = 0x20, end = 0xB0 */
} PACKED dc_mail_data_pkt;

typedef struct pc_mail_data {
    uint16_t pcname[0x10];
    uint32_t gc_dest;
    char stuff[0x400]; /* Start = 0x30, end = 0x150 */
} PACKED pc_mail_data_pkt;

typedef struct bb_mail_data {
    uint16_t bbname[0x10];
    uint32_t gc_dest;
    uint16_t timestamp[0x14];
    uint16_t message[0xAC];
    uint8_t unk2[0x2A0];
} PACKED bb_mail_data_pkt;

typedef struct simple_mail {
    union hdr {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
        bb_pkt_hdr_t bb;
    };
    uint32_t player_tag;
    uint32_t gc_sender;
    union mail_data {
        dc_mail_data_pkt dcmaildata;
        pc_mail_data_pkt pcmaildata;
        bb_mail_data_pkt bbmaildata;
    };
} PACKED simple_mail_pkt;

// 82: 无效或未解析指令

//83（S->C）：大堂菜单
//内部名称：RcvRoomInfo
//奇怪的是，DC版本中有一个SndRoomInfo字符串。也许在
//早期（NTE之前）构建，客户端必须从
//服务器，SndRoomInfo是执行此操作的命令
//命令必须在DC NTE之前删除。
//此命令设置客户端用于大厅的菜单项ID
//传送机菜单。在DCv1上，客户端期望此处有10个条目；在所有其他
//除了第3集之外的版本，客户期望这里有15个项目；在第3集，
//客户希望这里有20件商品。发送更多或更少的项目不会
//更改客户端的大厅计数。如果发送的条目较少，则菜单
//某些大厅的项目ID将不会设置，客户端可能会发送
//如果玩家选择一个带有
//未设置的ID。在第三集中，CARD大厅是最后五个入口，甚至
//尽管它们出现在玩家屏幕上列表的顶部。
typedef struct dc_lobby_list {
    union {                     /* The flags field says the entry count */
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint32_t padding;
    } entries[0];
} PACKED dc_lobby_list_pkt;

typedef struct bb_lobby_list {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint32_t padding;
    } entries[0];
} PACKED bb_lobby_list_pkt;

//// Command is a list of these; header.flag is the entry count (10, 15 or 20)
//struct S_LobbyListEntry_83 {
//    le_uint32_t menu_id = 0;
//    le_uint32_t item_id = 0;
//    le_uint32_t unused = 0;
//} __packed__;

//84（C->S）：选择大厅
//内部名称：SndRoomChange

// 85: 无效或未解析指令
// 86: 无效或未解析指令
// 87: 无效或未解析指令

//88（C->S）：许可证检查（仅限DC NTE）
//服务器应该以88命令进行响应。
typedef struct dcnte_login_88 {
    dc_pkt_hdr_t hdr;
    char serial_number[17];
    char access_key[17];
} PACKED dcnte_login_88_pkt;

// 88 (S->C): License check result (DC NTE only)
// No arguments except header.flag.
// If header.flag is zero, client will respond with an 8A command. Otherwise, it
// will respond with an 8B command.

// 88 (S->C): Update lobby arrows (except DC NTE)
// If this command is sent while a client is joining a lobby, the client may
// ignore it. For this reason, the server should wait a few seconds after a
// client joins a lobby before sending an 88 command.
// This command is not supported on DC v1.
// The packet sent to update the list of arrows in a lobby
// Command is a list of these; header.flag is the entry count. There should
// be an update for every player in the lobby in this command, even if their
// arrow color isn't being changed.
typedef struct dc_arrow_list {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    struct {
        uint32_t player_tag;
        uint32_t guildcard;
        // Values for arrow_color:
        // any number not specified below (including 00) - none
        // 01 - red
        // 02 - blue
        // 03 - green
        // 04 - yellow
        // 05 - purple
        // 06 - cyan
        // 07 - orange
        // 08 - pink
        // 09 - white
        // 0A - white
        // 0B - white
        // 0C - black
        uint32_t arrow_color;
    } entries[0];
} PACKED dc_arrow_list_pkt;

typedef struct bb_arrow_list {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t player_tag;
        uint32_t guildcard;
        // Values for arrow_color:
        // any number not specified below (including 00) - none
        // 01 - red
        // 02 - blue
        // 03 - green
        // 04 - yellow
        // 05 - purple
        // 06 - cyan
        // 07 - orange
        // 08 - pink
        // 09 - white
        // 0A - white
        // 0B - white
        // 0C - black
        uint32_t arrow_color;
    } entries[0];
} PACKED bb_arrow_list_pkt;

// 89 (C->S): Set lobby arrow
// header.flag = arrow color number (see above); no other arguments.
// Server should send an 88 command to all players in the lobby.

// 8A (C->S): Connection information (DC NTE only)
// The server should respond with an 8A command.
//struct C_ConnectionInfo_DCNTE_8A {
//    ptext<char, 0x08> hardware_id;
//    uint32_t sub_version = 0x20;
//    uint32_t unknown_a1;
//    ptext<char, 0x30> username;
//    ptext<char, 0x30> password;
//    ptext<char, 0x30> email_address; // From Sylverant documentation
//} PACKED;
typedef struct dcnte_login_8a {
    dc_pkt_hdr_t hdr;
    char dc_id[8];
    uint8_t unk[8];
    char username[48];
    char password[48];
    char email[48];
} PACKED dcnte_login_8a_pkt;

// 8A (S->C): Connection information result (DC NTE only)
// header.flag is a success flag. If 0 is sent, the client shows an error
// message and disconnects. Otherwise, the client responds with an 8B command.

// 8A (C->S): Request lobby/game name (except DC NTE)
// No arguments

// 8A (S->C): Lobby/game name (except DC NTE)
// Contents is a string (char16_t on PC/BB, char on DC/V3) containing the lobby
// or game name. The client generally only sends this immediately after joining
// a game, but Sega's servers also replied to it if it was sent in a lobby. They
// would return a string like "LOBBY01" in that case even though this would
// never be used under normal circumstances.
// This one is the "real" login packet from the DC NTE. If the client is still
// connected to the login server, you get more unused space at the end for some
// reason, but otherwise the packet is exactly the same between the ship and the
// login server.
typedef struct dcnte_login_8b {
    dc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t guildcard;
    uint8_t dc_id[8];
    uint32_t sub_version;
    uint8_t is_extended;
    uint8_t language;
    uint8_t unused1[2];
    char serial[17];
    char access_key[17];
    char username[48];
    char password[48];
    char char_name[16];
    uint8_t unused[2];
} PACKED dcnte_login_8b_pkt;

// 8C: 无效或未解析指令

// 8D (S->C): Request player data (DC NTE only) DCNTE_CHAR_DATA_REQ_TYPE
// Behaves the same as 95 (S->C) on all other versions. DC NTE crashes if it
// receives 95, so this is used instead.

// 8E: 无效或未解析指令 DCNTE_SHIP_LIST_TYPE dc_ship_list_pkt

// 8F: 无效或未解析指令 DCNTE_BLOCK_LIST_REQ_TYPE dc_block_list_pkt

// 90 (C->S): V1 login (DC/PC/V3)
// This command is used during the DCv1 login sequence; a DCv1 client will
// respond to a 17 command with an (encrypted) 90. If a V3 client receives a 91
// command, however, it will also send a 90 in response, though the contents
// will be blank (all zeroes).
/* Various login packets */
typedef struct dc_login_90 {
    dc_pkt_hdr_t hdr;
    char serial_number[17];
    char access_key[17];
    uint8_t unused[2];
} PACKED dc_login_90_pkt;

// 90 (S->C): License verification result (V3)
// Behaves exactly the same as 9A (S->C). No arguments except header.flag.

// 91 (S->C): Start encryption at login server (legacy; non-BB only)
// Internal name: RcvPsoRegistConnect
// Same format and usage as 17 command, except the client will respond with a 90
// command. On versions that support it, this is strictly less useful than the
// 17 command. Curiously, this command appears to have been implemented after
// the 17 command since it's missing from the DC NTE version, but the 17 command
// is named RcvPsoRegistConnectV2 whereas 91 is simply RcvPsoRegistConnect. It's
// likely that after DC NTE, Sega simply changed the command numbers for this
// group of commands from 88-8F to 90-A1 (so DC NTE's 89 command became the 91
// command in all later versions).

// 92 (C->S): Register (DC)
//struct C_RegisterV1_DC_92 {
//    parray<uint8_t, 0x0C> unknown_a1;
//    uint8_t is_extended; // TODO: This is a guess
//    uint8_t language; // TODO: This is a guess; verify it
//    parray<uint8_t, 2> unknown_a3;
//    ptext<char, 0x10> hardware_id;
//    parray<uint8_t, 0x50> unused1;
//    ptext<char, 0x20> email; // According to Sylverant documentation
//    parray<uint8_t, 0x10> unused2;
//} PACKED;

// 92 (S->C): Register result (non-BB)
// Internal name: RcvPsoRegist
// Same format and usage as 9C (S->C) command.

// 93 (C->S): Log in (DCv1)
typedef struct dc_login_92_93 {
    dc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t guildcard;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    uint32_t sub_version;
    uint8_t is_extended;
    uint8_t language_code;
    uint16_t unused2;
    char serial_number[17];
    char access_key[17];
    // Note: The hardware_id field is likely shorter than this (only 8 bytes
    // appear to actually be used).
    char dc_id[48];
    char unknown_a3[48];
    char name[16];
    uint8_t padding2[2];
    uint8_t extra_data[0];
} PACKED dc_login_92_pkt;

typedef struct dc_login_92_93 dc_login_93_pkt;

typedef struct dc_login_93_meet {
    uint32_t lobby_menu;
    uint32_t lobby_id;
    uint8_t unk1[60];
    char meet_name[16];
    uint8_t unk2[16];
} PACKED dc_login_93_meet_ext;

// 93 (C->S): Log in (BB)
typedef struct bb_login_93 {
    bb_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t guildcard;
    uint16_t version;
    uint8_t unused[6];
    uint32_t guild_id;
    char username[48];
    char password[48];
    uint32_t menu_id;
    uint32_t preferred_lobby_id;
    union VariableLengthSection {
        union ClientConfigFields {
            bb_client_config_pkt cfg;
            char version_string[40];
            uint32_t as_u32[10];
        } PACKED;

        union ClientConfigFields old_clients;

        struct NewFormat {
            uint32_t hwinfo[2];
            union ClientConfigFields cfg;
        } PACKED new_clients;
    } PACKED var;
} PACKED bb_login_93_pkt;

// 94: 无效或未解析指令

// 95 (S->C): Request player data
// Internal name: RcvRecognition
// No arguments
// For some reason, some servers send high values in the header.flag field here.
// The header.flag field is completely unused by the client, however - sending
// zero works just fine. The original Sega servers had some uninitialized memory
// bugs, of which that may have been one, and other private servers may have
// just duplicated Sega's behavior verbatim.
// Client will respond with a 61 command.

// 96 (C->S): Character save information
// Internal name: SndSaveCountCheck
typedef struct dc_char_save_info_nobb_96 {
    dc_pkt_hdr_t hdr;
    // The creation timestamp is the number of seconds since 12:00AM on 1 January
    // 2000. Instead of computing this directly from the TBR (on PSO GC), the game
    // uses localtime(), then converts that to the desired timestamp. The leap
    // year correction in the latter phase of this computation seems incorrect; it
    // adds a day in 2002, 2006, etc. instead of 2004, 2008, etc. See
    // compute_psogc_timestamp in SaveFileFormats.cc for details.
    uint32_t creation_timestamp;
    // This field counts certain events on a per-character basis. One of the
    // relevant events is the act of sending a 96 command; another is the act of
    // receiving a 97 command (to which the client responds with a B1 command).
    // Presumably Sega's original implementation could keep track of this value
    // for each character and could therefore tell if a character had connected to
    // an unofficial server between connections to Sega's servers.
    uint32_t event_counter;
} PACKED dc_char_save_info_nobb_pkt;

typedef struct bb_char_save_info_V3_BB_96 {
    bb_pkt_hdr_t hdr;
    // The creation timestamp is the number of seconds since 12:00AM on 1 January
    // 2000. Instead of computing this directly from the TBR (on PSO GC), the game
    // uses localtime(), then converts that to the desired timestamp. The leap
    // year correction in the latter phase of this computation seems incorrect; it
    // adds a day in 2002, 2006, etc. instead of 2004, 2008, etc. See
    // compute_psogc_timestamp in SaveFileFormats.cc for details.
    uint32_t creation_timestamp;
    // This field counts certain events on a per-character basis. One of the
    // relevant events is the act of sending a 96 command; another is the act of
    // receiving a 97 command (to which the client responds with a B1 command).
    // Presumably Sega's original implementation could keep track of this value
    // for each character and could therefore tell if a character had connected to
    // an unofficial server between connections to Sega's servers.
    uint32_t event_counter;
} PACKED bb_char_save_info_V3_BB_96_pkt;

// 97 (S->C): Save to memory card
// No arguments
// Sending this command with header.flag == 0 will show a message saying that
// "character data was improperly saved", and will delete the character's items
// and challenge mode records. newserv (and all other unofficial servers) always
// send this command with flag == 1, which causes the client to save normally.
// Client will respond with a B1 command if header.flag is nonzero.

// 98 (C->S): Leave game
// Same format as 61 command.
// Client will send an 84 when it's ready to join a lobby.
// The packet sent by clients to send their character data
typedef struct dc_char_data {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    player_t data;
} PACKED dc_char_data_pkt;

typedef struct bb_char_data {
    bb_pkt_hdr_t hdr;
    player_t data;
} PACKED bb_char_data_pkt;

// 99 (C->S): Server time accepted
// No arguments

// 9A (C->S): Initial login (no password or client config)
// Not used on DCv1 - that version uses 90 instead.
typedef struct dcv2_login_9a {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char v1_serial_number[16];
    char v1_access_key[16];
    char serial_number[16];
    char access_key[16];
    uint32_t player_tag;
    uint32_t guildcard;
    uint32_t sub_version;

    union {
        struct s1_DCV2_9a
        {
            char dc_id[8];
            uint8_t padding4[88];
        } PACKED;
        struct s2_DCV2_9a
        {
            char serial_number2[0x30];
            char access_key2[0x30];
        } PACKED;
    } PACKED;

    uint8_t email[48];
} PACKED dcv2_login_9a_pkt;

// 9A (S->C): License verification result
// The result code is sent in the header.flag field. Result codes:
// 00 = license ok (don't save to memory card; client responds with 9D/9E)
// 01 = registration required (client responds with a 9C command)
// 02 = license ok (save to memory card; client responds with 9D/9E)
// For all of the below cases, the client doesn't respond and displays a message
// box describing the error. When the player presses a button, the client then
// disconnects.
// 03 = access key invalid (125) (client deletes saved license info)
// 04 = serial number invalid (126) (client deletes saved license info)
// 07 = invalid Hunter's License (117)
// 08 = Hunter's License expired (116)
// 0B = HL not registered under this serial number/access key (112)
// 0C = HL not registered under this serial number/access key (113)
// 0D = HL not registered under this serial number/access key (114)
// 0E = connection error (115)
// 0F = connection suspended (111)
// 10 = connection suspended (111)
// 11 = Hunter's License expired (116)
// 12 = invalid Hunter's License (117)
// 13 = servers under maintenance (118)
// Seems like most (all?) of the rest of the codes are "network error" (119).

// 9B (S->C): Secondary server init (non-BB)
// Behaves exactly the same as 17 (S->C).
// TODO: Check if this command exists on DC v1/v2.

// 9B (S->C): Secondary server init (BB)
// Format is the same as 03 (and the client uses the same encryption afterward).
// The only differences that 9B has from 03:
// - 9B does not work during the data-server phase (before the client has
//   reached the ship select menu), whereas 03 does.
// - For command 9B, the copyright string must be
//   "PSO NEW PM Server. Copyright 1999-2002 SONICTEAM.".
// - The client will respond with a command DB instead of a command 93.

// 9C (C->S): Register
// It appears PSO GC sends uninitialized data in the header.flag field here.
typedef struct gc_login_9c {
    dc_pkt_hdr_t hdr;
    uint8_t padding1[8];
    uint8_t version;
    uint8_t padding2[4];
    uint8_t language_code;
    uint8_t padding3[2];
    char serial_number[8];
    uint8_t padding4[40];
    char access_key[12];
    uint8_t padding5[36];
    char password[16];
    uint8_t padding6[32];
} gc_login_9c_pkt;

typedef struct xb_login_9c {
    dc_pkt_hdr_t hdr;
    uint8_t padding1[8];
    uint8_t version;
    uint8_t padding2[4];
    uint8_t language_code;
    uint8_t padding3[2];
    uint8_t xbl_gamertag[16];
    uint8_t padding4[32];
    uint8_t xbl_userid[16];
    uint8_t padding5[32];
    uint8_t xbox_pso[8];        /* Literally "xbox-pso" */
    uint8_t padding6[40];
} xb_login_9c_pkt;

// 9C (S->C): Register result
// On GC, the only possible error here seems to be wrong password (127) which is
// displayed if the header.flag field is zero. On DCv2/PC, the error text says
// something like "registration failed" instead. If header.flag is nonzero, the
// client proceeds with the login procedure by sending a 9D or 9E.

// 9D (C->S): Log in without client config (DCv2/PC/GC)
// Not used on DCv1 - that version uses 93 instead.
// Not used on most versions of V3 - the client sends 9E instead. The one
// type of PSO V3 that uses 9D is the Trial Edition of Episodes 1&2.
// The extended version of this command is sent if the client has not yet
// received an 04 (in which case the extended fields are blank) or if the client
// selected the Meet User option, in which case it specifies the requested lobby
// by its menu ID and item ID.
typedef struct dcv2_login_9d {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t player_tag;
    uint32_t guildcard;
    uint32_t unused1;
    uint32_t unused2;
    uint32_t sub_version;
    uint8_t is_extended; // If 1, structure has extended format
    uint8_t language_code; // 0 = JP, 1 = EN, 2 = DE, 3 = FR, 4 = ES
    uint8_t padding1[2];
    char v1_serial_number[16];
    char v1_access_key[16];
    char serial_number[16];
    char access_key[16];

    union {
        struct s1_DCV2_9d
        {
            char dc_id[8];
            uint8_t padding4[88];
        } PACKED;
        struct s2_DCV2_9d
        {
            char serial_number2[0x30];
            char access_key2[0x30];
        } PACKED;
    } PACKED;

    char name[16];
    uint8_t extra_data[0];
} PACKED dcv2_login_9d_pkt;

typedef struct dcv2_login_9d_meet {
    uint32_t lobby_menu;
    uint32_t lobby_id;
    uint8_t unk1[60];
    char meet_name[16];
    uint8_t unk2[16];
} PACKED dcv2_login_9d_meet_ext;

typedef struct pc_login_9d_meet {
    uint32_t lobby_menu;
    uint32_t lobby_id;
    uint8_t unk1[60];
    uint16_t meet_name[16];
    uint16_t unk2[16];
} PACKED pc_login_9d_meet_ext;

// 9E (C->S): Log in with client config (V3/BB)
// Not used on GC Episodes 1&2 Trial Edition.
// The extended version of this command is used in the same circumstances as
// when PSO PC uses the extended version of the 9D command.
// header.flag is 1 if the client has UDP disabled.
typedef struct gc_login_9e {
    dc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t guildcard;
    uint8_t padding1[8];
    uint8_t version;
    uint8_t padding2[4];
    uint8_t language_code;
    uint8_t padding3[34];
    char serial_number1[8];
    uint8_t padding4[8];
    char access_key1[12];
    uint8_t padding5[4];
    char serial_number2[8];
    uint8_t padding6[40];
    char access_key2[12];
    uint8_t padding7[36];
    char name[16];
    uint8_t padding8[32];
    uint8_t extra_data[0];
} PACKED gc_login_9e_pkt;

typedef struct gc_login_9e_meet {
    uint32_t lobby_menu;
    uint32_t lobby_id;
    uint8_t unk1[60];
    char meet_name[16];
    uint8_t unk2[16];
} PACKED gc_login_9e_meet_ext;

typedef struct xb_login_9e {
    dc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t guildcard;
    uint8_t padding1[8];
    uint8_t version;
    uint8_t padding2[4];
    uint8_t language_code;
    uint8_t padding3[34];
    uint8_t xbl_gamertag[16];
    uint8_t xbl_userid[16];
    uint8_t xbl_gamertag2[16];
    uint8_t padding6[32];
    uint8_t xbl_userid2[16];
    uint8_t padding7[32];
    char name[16];
    uint8_t padding8[32];
    xbox_ip_t xbl_ip;
    uint8_t unk[32];
    uint8_t extra_data[0];
} PACKED xb_login_9e_pkt;

typedef struct bb_login_9e {
    bb_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t guildcard;
    uint32_t sub_version;
    uint32_t unknown_a1;
    uint32_t unknown_a2;
    char v1_serial_number[16];
    char v1_access_key[16];
    char serial_number[16];
    char access_key[16];
    char username[48];
    char password[48];
    char guildcard_string[16];
    uint32_t unknown_a3[10];
    uint32_t menu_id;
    uint32_t lobby_id;
    uint8_t unknown_a4[0x3C];
    uint16_t player_name[0x20];
} PACKED bb_login_9e_pkt;

// 9F (S->C): Request client config / security data (V3/BB)
// This command is not valid on PSO GC Episodes 1&2 Trial Edition, nor any
// pre-V3 PSO versions.
// No arguments

// 9F (C->S): Client config / security data response (V3/BB)
// The data is opaque to the client, as described at the top of this file.
// If newserv ever sent a 9F command (it currently does not), the response
// format here would be ClientConfig (0x20 bytes) on V3, or ClientConfigBB (0x28
// bytes) on BB. However, on BB, this returns the client config that was set by
// a preceding 04 command, not the config set by a preceding E6 command.

// A0 (C->S): Change ship
// This structure is for documentation only; newserv ignores the arguments here.
// TODO: This structure is valid for GC clients; check if this command has the
// same arguments on other versions.

//struct C_ChangeShipOrBlock_A0_A1 {
//    uint32_t player_tag = 0x00010000;
//    uint32_t guild_card_number;
//    parray<uint8_t, 0x10> unused;
//} PACKED;

// A0 (S->C): Ship select menu
// Same as 07 command.

// A1 (C->S): Change block
// Same format as A0. As with A0, newserv ignores the arguments.

// A1 (S->C): Block select menu
// Same as 07 command.

// A2 (C->S): Request quest menu
// 这里只用到了flags来区分总督府和飞船任务

// A2 (S->C): Quest menu
// Client will respond with an 09, 10, or A9 command. For 09, the server should
// send the category or quest description via an A3 response; for 10, the server
// should send another quest menu (if a category was chosen), or send the quest
// data with 44/13 commands; for A9, the server does not need to respond.
//template <typename CharT, size_t ShortDescLength>
//struct S_QuestMenuEntry {
//    uint32_t menu_id;
//    uint32_t item_id;
//    ptext<CharT, 0x20> name;
//    ptext<CharT, ShortDescLength> short_description;
//} PACKED;
//struct S_QuestMenuEntry_PC_A2_A4 : S_QuestMenuEntry<char16_t, 0x70> { } PACKED;
//struct S_QuestMenuEntry_DC_GC_A2_A4 : S_QuestMenuEntry<char, 0x70> { } PACKED;
//struct S_QuestMenuEntry_XB_A2_A4 : S_QuestMenuEntry<char, 0x80> { } PACKED;
//struct S_QuestMenuEntry_BB_A2_A4 : S_QuestMenuEntry<char16_t, 0x7A> { } PACKED;

// A3 (S->C): Quest information
// Same format as 1A/D5 command (plain text)

// A4 (S->C): Download quest menu
// Same format as A2, but can be used when not in a game. The client responds
// similarly as for command A2 with the following differences:
// - Descriptions should be sent with the A5 command instead of A3.
// - If a quest is chosen, it should be sent with A6/A7 commands rather than
//   44/13, and it must be in a different encrypted format. The download quest
//   format is documented in create_download_quest_file in Quest.cc.
// - After the download is done, or if the player cancels the menu, the client
//   sends an A0 command instead of A9.

// A5 (S->C): Download quest information
// Same format as 1A/D5 command (plain text)

// A6: Open file for download
// Same format as 44.
// Used for download quests and GBA games. The client will react to this command
// if the filename ends in .bin/.dat (download quests), .gba (GameBoy Advance
// games), and, curiously, .pvr (textures). To my knowledge, the .pvr handler in
// this command has never been used.
// It also appears that the .gba handler was not deleted in PSO XB, even though
// it doesn't make sense for an XB client to receive such a file.
// For .bin files, the flags field should be zero. For .pvr files, the flags
// field should be 1. For .dat and .gba files, it seems the value in the flags
// field does not matter.
// Like the 44 command, the client->server form of this command is only used on
// V3 and BB.

// A7: Write download file
// Same format as 13.
// Like the 13 command, the client->server form of this command is only used on
// V3 and BB.

// A8: 无效或未解析指令

// A9 (C->S): Quest menu closed (canceled)
// No arguments
// This command is sent when the in-game quest menu (A2) is closed. When the
// download quest menu is closed, either by downloading a quest or canceling,
// the client sends A0 instead. The existence of the A0 response on the download
// case makes sense, because the client may not be in a lobby and the server may
// need to send another menu or redirect the client. But for the online quest
// menu, the client is already in a game and can move normally after canceling
// the quest menu, so it's not obvious why A9 is needed at all. newserv (and
// probably all other private servers) ignores it.
// Curiously, PSO GC sends uninitialized data in header.flag.

// AA (C->S): Send quest statistic (V3/BB)
// This command is generated when an opcode F92E is executed in a quest.
// The server should respond with an AB command.
// This command is likely never sent by PSO GC Episodes 1&2 Trial Edition,
// because the following command (AB) is definitely not valid on that version.
typedef struct dc_update_quest_stats {
    dc_pkt_hdr_t hdr;
    uint32_t quest_internal_id;
    uint16_t function_id1;
    uint16_t function_id2;
    uint32_t unknown_a2;
    uint32_t kill_count;
    uint32_t time_taken; // in seconds
    uint32_t unknown_a3[5];
} PACKED dc_update_quest_stats_pkt;

typedef struct bb_update_quest_stats {
    bb_pkt_hdr_t hdr;
    uint32_t quest_internal_id;
    uint16_t function_id1;
    uint16_t function_id2;
    uint32_t unknown_a2;
    uint32_t kill_count;
    uint32_t time_taken; // in seconds
    uint32_t unknown_a3[5];
} PACKED bb_update_quest_stats_pkt;

//struct C_SendQuestStatistic_V3_BB_AA {
//    le_uint16_t stat_id1 = 0;
//    le_uint16_t unused = 0;
//    le_uint16_t function_id1 = 0;
//    le_uint16_t function_id2 = 0;
//    parray<le_uint32_t, 8> params;
//} __packed__;

// AB (S->C): Confirm quest statistic (V3/BB)
// This command is not valid on PSO GC Episodes 1&2 Trial Edition.
// Upon receipt, the client starts a quest thread running the given function.
// Probably this is supposed to be one of the function IDs previously sent in
// the AA command, but the client does not check for this. The server can
// presumably use this command to call any function at any time during a quest.
typedef struct dc_confirm_update_quest_statistics {
    dc_pkt_hdr_t hdr;
    uint16_t function_id; // 0
    uint16_t unknown_a2; // Probably actually unused
} PACKED dc_confirm_update_quest_statistics_pkt;

typedef struct bb_confirm_update_quest_statistics {
    bb_pkt_hdr_t hdr;
    uint16_t function_id; // 0
    uint16_t unknown_a2; // Probably actually unused
} PACKED bb_confirm_update_quest_statistics_pkt;

// AC: Quest barrier (V3/BB)
// No arguments; header.flag must be 0 (or else the client disconnects)
// After a quest begins loading in a game (the server sends 44/13 commands to
// each player with the quest's data), each player will send an AC to the server
// when it has parsed the quest and is ready to start. When all players in a
// game have sent an AC to the server, the server should send them all an AC,
// which starts the quest for all players at (approximately) the same time.
// Sending this command to a GC client when it is not waiting to start a quest
// will cause it to crash.
// This command is not valid on PSO GC Episodes 1&2 Trial Edition.

// AD: 无效或未解析指令
// AE: 无效或未解析指令
// AF: 无效或未解析指令

// B0 (S->C): Text message
// Same format as 01 command.
// The message appears as an overlay on the right side of the screen. The player
// doesn't do anything to dismiss it; it will disappear after a few seconds.
// TODO: Check if this command exists on DC v1/v2.

// B1 (C->S): Request server time
// No arguments
// Server will respond with a B1 command.

// B1 (S->C): Server time
// Contents is a string like "%Y:%m:%d: %H:%M:%S.000" (the space is not a typo).
// For example: 2022:03:30: 15:36:42.000
// It seems the client ignores the date part and the milliseconds part; only the
// hour, minute, and second fields are actually used.
// This command can be sent even if it's not requested by the client (with B1).
// For example, some servers send this command every time a client joins a game.
// Client will respond with a 99 command.
typedef struct dc_timestamp {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char timestamp[28];
} PACKED dc_timestamp_pkt;

// B1 (C->S): Request server time
// No arguments
// Server will respond with a B1 command.

// B1 (S->C): Server time
// Contents is a string like "%Y:%m:%d: %H:%M:%S.000" (the space is not a typo).
// For example: 2022:03:30: 15:36:42.000
// It seems the client ignores the date part and the milliseconds part; only the
// hour, minute, and second fields are actually used.
// This command can be sent even if it's not requested by the client (with B1).
// For example, some servers send this command every time a client joins a game.
// Client will respond with a 99 command.
typedef struct bb_timestamp {
    bb_pkt_hdr_t hdr;
    char timestamp[28];
} PACKED bb_timestamp_pkt;

// B2 (S->C): Execute code and/or checksum memory
// Client will respond with a B3 command with the same header.flag value as was
// sent in the B2.
// On PSO PC, the code section (if included in the B2 command) is parsed and
// relocated, but is not actually executed, so the return_value field in the
// resulting B3 command is always 0. The checksum functionality does work on PSO
// PC, just like the other versions.
// This command doesn't work on some PSO GC versions, namely the later JP PSO
// Plus (v1.05), US PSO Plus (v1.02), or US/EU Episode 3. Sega presumably
// removed it after taking heat from Nintendo about enabling homebrew on the
// GameCube. On the earlier JP PSO Plus (v1.04) and JP Episode 3, this command
// is implemented as described here, with some additional compression and
// encryption steps added, similarly to how download quests are encoded. See
// send_function_call in SendCommands.cc for more details on how this works.

// newserv supports exploiting a bug in the USA version of Episode 3, which
// re-enables the use of this command on that version of the game. See
// system/ppc/Episode3USAQuestBufferOverflow.s for further details.

//struct S_ExecuteCode_B2 {
//    // If code_size == 0, no code is executed, but checksumming may still occur.
//    // In that case, this structure is the entire body of the command (no footer
//    // is sent).
//    uint32_t code_size; // Size of code (following this struct) and footer
//    uint32_t checksum_start; // May be null if size is zero
//    uint32_t checksum_size; // If zero, no checksum is computed
//    // The code immediately follows, ending with an S_ExecuteCode_Footer_B2
//} PACKED;

//template <typename LongT>
//struct S_ExecuteCode_Footer_B2 {
//    // Relocations is a list of words (uint16_t on DC/PC/XB/BB, be_uint16_t on
//    // GC) containing the number of doublewords (uint32_t) to skip for each
//    // relocation. The relocation pointer starts immediately after the
//    // checksum_size field in the header, and advances by the value of one
//    // relocation word (times 4) before each relocation. At each relocated
//    // doubleword, the address of the first byte of the code (after checksum_size)
//    // is added to the existing value.
//    // For example, if the code segment contains the following data (where R
//    // specifies doublewords to relocate):
//    //   RR RR RR RR ?? ?? ?? ?? ?? ?? ?? ?? RR RR RR RR
//    //   RR RR RR RR ?? ?? ?? ?? RR RR RR RR
//    // then the relocation words should be 0000, 0003, 0001, and 0002.
//    // If there is a small number of relocations, they may be placed in the unused
//    // fields of this structure to save space and/or confuse reverse engineers.
//    // The game never accesses the last 12 bytes of this structure unless
//    // relocations_offset points there, so those 12 bytes may also be omitted from
//    // the command entirely (without changing code_size - so code_size would
//    // technically extend beyond the end of the B2 command).
//    LongT relocations_offset; // Relative to code base (after checksum_size)
//    LongT num_relocations;
//    parray<LongT, 2> unused1;
//    // entrypoint_offset is doubly indirect - it points to a pointer to a 32-bit
//    // value that itself is the actual entrypoint. This is presumably done so the
//    // entrypoint can be optionally relocated.
//    LongT entrypoint_addr_offset; // Relative to code base (after checksum_size).
//    parray<LongT, 3> unused2;
//} PACKED;
//
//struct S_ExecuteCode_Footer_GC_B2 : S_ExecuteCode_Footer_B2<be_uint32_t> { } PACKED;
//struct S_ExecuteCode_Footer_DC_PC_XB_BB_B2
//    : S_ExecuteCode_Footer_B2<uint32_t> { } PACKED;

// B3 (C->S): Execute code and/or checksum memory result
// Not used on versions that don't support the B2 command (see above).
// Patch return packet.
typedef struct patch_return {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    // On DC, return_value has the value in r0 when the function returns.
    // On PC, return_value is always 0.
    // On GC, return_value has the value in r3 when the function returns.
    // On XB and BB, return_value has the value in eax when the function returns.
    // If code_size was 0 in the B2 command, return_value is always 0.
    uint32_t return_value;
    uint32_t checksum;
} PACKED patch_return_pkt;

// B4: 无效或未解析指令
// B5: 无效或未解析指令
// B6: 无效或未解析指令

// B7 (S->C): Rank update (Episode 3)

//struct S_RankUpdate_GC_Ep3_B7 {
//    uint32_t rank;
//    ptext<char, 0x0C> rank_text;
//    uint32_t meseta;
//    uint32_t max_meseta;
//    uint32_t jukebox_songs_unlocked;
//} PACKED;

// B7 (C->S): Confirm rank update (Episode 3)
// No arguments
// The client sends this after it receives a B7 from the server.

// B8 (S->C): Update card definitions (Episode 3)
// Contents is a single little-endian uint32_t specifying the size of the
// (PRS-compressed) data, followed immediately by the data. The maximum size of
// the compressed data is 0x9000 bytes, although the receive buffer size limit
// applies first in practice, which limits this to 0x7BF8 bytes. The maximum
// size of the decompressed data is 0x36EC0 bytes.
// Note: PSO BB accepts this command as well, but ignores it.

// B8 (C->S): Confirm updated card definitions (Episode 3)
// No arguments
// The client sends this after it receives a B8 from the server.

// B9 (S->C): Update media (Episode 3)
// This command is not valid on Episode 3 Trial Edition.

//struct S_UpdateMediaHeader_GC_Ep3_B9 {
//    // Valid values for the type field:
//    // 1: GVM file
//    // 2: Unknown; probably BML file
//    // 3: Unknown; probably BML file
//    // 4: Unknown; appears to be completely ignored
//    // Any other value: entire command is ignored
//    // For types 2 and 3, the game looks for various tokens in the decompressed
//    // data; specifically '****', 'GCAM', 'GJBM', 'GJTL', 'GLIM', 'GMDM', 'GSSM',
//    // 'NCAM', 'NJBM', 'NJCA', 'NLIM', 'NMDM', and 'NSSM'. Of these, 'GJTL',
//    // 'NMDM', and 'NSSM' are found in some of the game's existing BML files, but
//    // the others don't seem to be anywhere on the disc. 'NJBM' is found in
//    // psohistory_e.sfd, but not in any other files.
//    uint32_t type;
//    // Valid values for the type field (at least, when type is 1):
//    // 0: Unknown
//    // 1: Set lobby banner 1 (in front of where player 0 enters)
//    // 2: Set lobby banner 2 (left of banner 1)
//    // 3: Unknown
//    // 4: Set lobby banner 3 (left of banner 2; opposite where player 0 enters)
//    // 5: Unknown
//    // 6: Unknown
//    // Any other value: entire command is ignored
//    uint32_t which;
//    uint16_t size;
//    uint16_t unused;
//    // The PRS-compressed data immediately follows this header. The maximum size
//    // of the compressed data is 0x3800 bytes, and the data must decompress to
//    // fewer than 0x37000 bytes of output. The size field above contains the
//    // compressed size of this data (the decompressed size is not included
//    // anywhere in the command).
//} PACKED;

// B9 (C->S): Confirm received B9 (Episode 3)
// No arguments
// This command is not valid on Episode 3 Trial Edition.

// BA: Meseta transaction (Episode 3)
// This command is not valid on Episode 3 Trial Edition.
// header.flag specifies the transaction purpose. This has little meaning on the
// client. Specific known values:
// 00 = unknown
// 01 = unknown (S->C; request_token must match the last token sent by client)
// 02 = Spend meseta (at e.g. lobby jukebox or Pinz's shop) (C->S)
// 03 = Spend meseta response (S->C; request_token must match the last token
//      sent by client)
// 04 = unknown (S->C; request_token must match the last token sent by client)

//struct C_Meseta_GC_Ep3_BA {
//    uint32_t transaction_num;
//    uint32_t value;
//    uint32_t request_token;
//} PACKED;
//
//struct S_Meseta_GC_Ep3_BA {
//    uint32_t remaining_meseta;
//    uint32_t total_meseta_awarded;
//    uint32_t request_token; // Should match the token sent by the client
//} PACKED;


// BB (S->C): Tournament match information (Episode 3)
// This command is not valid on Episode 3 Trial Edition. Because of this, it
// must have been added fairly late in development, but it seems to be unused,
// perhaps because the E1/E3 commands are generally more useful... but the E1/E3
// commands exist in the Trial Edition! So why was this added? Was it just never
// finished? We may never know...
// header.flag is the number of valid match entries.

//struct S_TournamentMatchInformation_GC_Ep3_BB {
//    ptext<char, 0x20> tournament_name;
//    struct TeamEntry {
//        uint16_t win_count;
//        uint16_t is_active;
//        ptext<char, 0x20> name;
//    } PACKED;
//    parray<TeamEntry, 0x20> team_entries;
//    uint16_t num_teams;
//    uint16_t unknown_a3; // Probably actually unused
//    struct MatchEntry {
//        parray<char, 0x20> name;
//        uint8_t locked;
//        uint8_t count;
//        uint8_t max_count;
//        uint8_t unused;
//    } PACKED;
//    parray<MatchEntry, 0x40> match_entries;
//} PACKED;

// BC: 无效或未解析指令
// BD: 无效或未解析指令
// BE: 无效或未解析指令
// BF: 无效或未解析指令

// C0 (C->S): Request choice search options (DCv2 and later versions)
// Internal name: GetChoiceList
// No arguments
// Server should respond with a C0 command (described below).

// C0 (S->C): Choice search options (DCv2 and later versions)
// Internal name: RcvChoiceList

// Command is a list of these; header.flag is the entry count (incl. top-level).
// template <typename ItemIDT, typename CharT>
//struct S_ChoiceSearchEntry {
//    // Category IDs are nonzero; if the high byte of the ID is nonzero then the
//    // category can be set by the user at any time; otherwise it can't.
//    ItemIDT parent_category_id = 0; // 0 for top-level categories
//    ItemIDT category_id = 0;
//    ptext<CharT, 0x1C> text;
//} PACKED;

//struct S_ChoiceSearchEntry_DC_C0 : S_ChoiceSearchEntry<uint32_t, char> { } PACKED;
//struct S_ChoiceSearchEntry_V3_C0 : S_ChoiceSearchEntry<le_uint16_t, char> { } PACKED;
//struct S_ChoiceSearchEntry_PC_BB_C0 : S_ChoiceSearchEntry<le_uint16_t, char16_t> { } PACKED;

typedef struct dc_cs_entry {
    dc_pkt_hdr_t hdr;
    uint32_t parent_category_id;
    uint32_t category_id;
    char text[0x1C];
} PACKED dc_cs_entry_pkt;

typedef struct v3_cs_entry {
    dc_pkt_hdr_t hdr;
    uint16_t parent_category_id;
    uint16_t category_id;
    char text[0x1C];
} PACKED v3_cs_entry_pkt;

typedef struct pc_cs_entry {
    pc_pkt_hdr_t hdr;
    uint16_t parent_category_id;
    uint16_t category_id;
    uint16_t text[0x1C];
} PACKED pc_cs_entry_pkt;

typedef struct bb_cs_entry {
    bb_pkt_hdr_t hdr;
    uint16_t parent_category_id;
    uint16_t category_id;
    uint16_t text[0x1C];
} PACKED bb_cs_entry_pkt;

// Top-level categories are things like "Level", "Class", etc.
// Choices for each top-level category immediately follow the category, so
// a reasonable order of items is (for example):
//   00 00 11 01 "Preferred difficulty"
//   11 01 01 01 "Normal"
//   11 01 02 01 "Hard"
//   11 01 03 01 "Very Hard"
//   11 01 04 01 "Ultimate"
//   00 00 22 00 "Character class"
//   22 00 01 00 "HUmar"
//   22 00 02 00 "HUnewearl"
//   etc.

// C1 (C->S): Create game

//template <typename CharT>
//struct C_CreateGame {
//    // menu_id and item_id are only used for the E7 (create spectator team) form
//    // of this command
//    uint32_t menu_id;
//    uint32_t item_id;
//    ptext<CharT, 0x10> name;
//    ptext<CharT, 0x10> password;
//    uint8_t difficulty; // 0-3 (always 0 on Episode 3)
//    uint8_t battle_mode; // 0 or 1 (always 0 on Episode 3)
//    // Note: Episode 3 uses the challenge mode flag for view battle permissions.
//    // 0 = view battle allowed; 1 = not allowed
//    uint8_t challenge_mode; // 0 or 1
//    // Note: According to the Sylverant wiki, in v2-land, the episode field has a
//    // different meaning: if set to 0, the game can be joined by v1 and v2
//    // players; if set to 1, it's v2-only.
//    uint8_t episode; // 1-4 on V3+ (3 on Episode 3); unused on DC/PC
//} PACKED;
//struct C_CreateGame_DC_V3_0C_C1_Ep3_EC : C_CreateGame<char> { } PACKED;
//struct C_CreateGame_PC_C1 : C_CreateGame<char16_t> { } PACKED;
//
//struct C_CreateGame_BB_C1 : C_CreateGame<char16_t> {
//    uint8_t solo_mode;
//    parray<uint8_t, 3> unused2;
//} PACKED;

// C2 (C->S): Set choice search parameters
// Server does not respond.

//template <typename ItemIDT>
//struct C_ChoiceSearchSelections_C2_C3 {
//    uint16_t disabled; // 0 = enabled, 1 = disabled. Unused for command C3
//    uint16_t unused;
//    struct Entry {
//        ItemIDT parent_category_id;
//        ItemIDT category_id;
//    } PACKED;
//    Entry entries[0];
//} PACKED;
//
//struct C_ChoiceSearchSelections_DC_C2_C3 : C_ChoiceSearchSelections_C2_C3<uint32_t> { } PACKED;
//struct C_ChoiceSearchSelections_PC_V3_BB_C2_C3 : C_ChoiceSearchSelections_C2_C3<uint16_t> { } PACKED;

// C3 (C->S): Execute choice search
// Same format as C2. The disabled field is unused.
// Server should respond with a C4 command.

// C4 (S->C): Choice search results

// Command is a list of these; header.flag is the entry count
//struct S_ChoiceSearchResultEntry_V3_C4 {
//    uint32_t guild_card_number;
//    ptext<char, 0x10> name; // No language marker, as usual on V3
//    ptext<char, 0x20> info_string; // Usually something like "<class> Lvl <level>"
//    // Format is stricter here; this is "LOBBYNAME,BLOCKNUM,SHIPNAME"
//    // If target is in game, for example, "Game Name,BLOCK01,Alexandria"
//    // If target is in lobby, for example, "BLOCK01-1,BLOCK01,Alexandria"
//    ptext<char, 0x34> locator_string;
//    // Server IP and port for "meet user" option
//    uint32_t server_ip;
//    uint16_t server_port;
//    uint16_t unused1;
//    uint32_t menu_id;
//    uint32_t lobby_id; // These two are guesses
//    uint32_t game_id; // Zero if target is in a lobby rather than a game
//    parray<uint8_t, 0x58> unused2;
//} PACKED;

// C5 (S->C): Player records update (DCv2 and later versions)
// Internal name: RcvChallengeData
// Command is a list of PlayerRecordsEntry structures; header.flag specifies
// the entry count.
// The server sends this command when a player joins a lobby to update the
// challenge mode records of all the present players.
/* 舰船发送至客户端 C-Rank 数据包结构 */
typedef struct dc_records_update {
    dc_pkt_hdr_t hdr;
    psocn_dc_records_data_t entries[0];
} PACKED dc_records_update_pkt;

typedef struct pc_records_update {
    pc_pkt_hdr_t hdr;
    psocn_pc_records_data_t entries[0];
} PACKED pc_records_update_pkt;

typedef struct gc_records_update {
    dc_pkt_hdr_t hdr;
    psocn_v3_records_data_t entries[0];
} PACKED gc_records_update_pkt;

typedef struct bb_records_update {
    bb_pkt_hdr_t hdr;
    psocn_bb_records_data_t entries[0];
} PACKED bb_records_update_pkt;

// C6 (C->S): Set blocked senders list (V3/BB)
// The command always contains the same number of entries, even if the entries
// at the end are blank (zero).
/* The packet used to update the blocked senders list */
typedef struct gc_blacklist_update {
    union {
        dc_pkt_hdr_t gc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t list[30];
} PACKED gc_blacklist_update_pkt;

typedef struct bb_blacklist_update {
    bb_pkt_hdr_t hdr;
    uint32_t list[30];
} PACKED bb_blacklist_update_pkt;

//template <size_t Count>
//struct C_SetBlockedSenders_C6 {
//    parray<uint32_t, Count> blocked_senders;
//} PACKED;
//
//struct C_SetBlockedSenders_V3_C6 : C_SetBlockedSenders_C6<30> { } PACKED;
//struct C_SetBlockedSenders_BB_C6 : C_SetBlockedSenders_C6<28> { } PACKED;

// C7 (C->S): Enable simple mail auto-reply (V3/BB)
// Same format as 1A/D5 command (plain text).
// Server does not respond
// The packet used to set a simple mail autoreply
typedef struct autoreply_set {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char msg[];
} PACKED autoreply_set_pkt;

typedef struct bb_autoreply_set {
    bb_pkt_hdr_t hdr;
    uint16_t msg[];
} PACKED bb_autoreply_set_pkt;

// C8 (C->S): Disable simple mail auto-reply (V3/BB)
// No arguments
// Server does not respond

// C9 (C->S): Unknown (XB)
// No arguments except header.flag

// C9: Broadcast command (Episode 3)
// Same as 60, but should be forwarded only to Episode 3 clients.
// newserv uses this command for all server-generated events (in response to CA
// commands), except for map data requests. This differs from Sega's original
// implementation, which sent CA responses via 60 commands instead.

// CA (C->S): Server data request (Episode 3)
// The CA command format is the same as that of the 6xB3 commands, and the
// subsubcommands formats are shared as well. Unlike the 6x commands, the server
// is expected to respond to the command appropriately instead of forwarding it.
// Because the formats are shared, the 6xB3 commands are also referred to as CAx
// commands in the comments and structure names.
//CA（C->S）：服务器数据请求（第3集）
//CA命令格式与6xB3命令格式相同
//子命令格式也是共享的。与6x命令不同，服务器
//期望适当地响应该命令而不是转发该命令。
//因为格式是共享的，所以6xB3命令也被称为CAx
//注释和结构名称中的命令。

// CB: Broadcast command (Episode 3)
// Same as 60, but only send to Episode 3 clients.
// This command is identical to C9, except that CB is not valid on Episode 3
// Trial Edition (whereas C9 is valid).
// TODO: Presumably this command is meant to be forwarded from spectator teams
// to the primary team, whereas 6x and C9 commands are not. I haven't verified
// this or implemented this behavior yet though.

// CC (S->C): Confirm tournament entry (Episode 3)
// This command is not valid on Episode 3 Trial Edition.
// header.flag determines the client's registration state - 1 if the client is
// registered for the tournament, 0 if not.
// This command controls what's shown in the Check Tactics pane in the pause
// menu. If the client is registered (header.flag==1), the option is enabled and
// the bracket data in the command is shown there, and a third pane on the
// Status item shows the other information (tournament name, ship name, and
// start time). If the client is not registered, the Check Tactics option is
// disabled and the Status item has only two panes.

//struct S_ConfirmTournamentEntry_GC_Ep3_CC {
//    ptext<char, 0x40> tournament_name;
//    uint16_t num_teams;
//    uint16_t unknown_a1;
//    uint16_t unknown_a2;
//    uint16_t unknown_a3;
//    ptext<char, 0x20> server_name;
//    ptext<char, 0x20> start_time; // e.g. "15:09:30" or "13:03 PST"
//    struct TeamEntry {
//        uint16_t win_count;
//        uint16_t is_active;
//        ptext<char, 0x20> name;
//    } PACKED;
//    parray<TeamEntry, 0x20> team_entries;
//} PACKED;

// CD: 无效或未解析指令
// CE: 无效或未解析指令
// CF: 无效或未解析指令

// D0 (C->S): Start trade sequence (V3/BB)
// The trade window sequence is a bit complicated. The normal flow is:
// - Clients sync trade state with 6xA6 commands
// - When both have confirmed, one client (the initiator) sends a D0
// - Server sends a D1 to the non-initiator
// - Non-initiator sends a D0
// - Server sends a D1 to both clients
// - Both clients delete the sent items from their inventories (and send the
//   appropriate subcommand)
// - Both clients send a D2 (similarly to how AC works, the server should not
//   proceed until both D2s are received)
// - Server sends a D3 to both clients with each other's data from their D0s,
//   followed immediately by a D4 01 to both clients, which completes the trade
// - Both clients send the appropriate subcommand to create inventory items
// TODO: On BB, is the server responsible for sending the appropriate item
// subcommands?
// At any point if an error occurs, either client may send a D4 00, which
// cancels the entire sequence. The server should then send D4 00 to both
// clients.
// TODO: The server should presumably also send a D4 00 if either client
// disconnects during the sequence.
//交易窗口序列有点复杂。正常流量为：
//-客户端使用60xA6命令同步交易状态（技术上为62xA6）
//-当两者都已确认时，一个客户端（启动器）发送一个D0
//-服务器向非启动器发送D1
//-非发起方发送D0
//-服务器向两个客户端发送D1
//-两个客户从其库存中删除已发送的项目（并发送
//适当的子命令）
//-两个客户端都发送D2（与AC的工作方式类似，服务器不应
//继续，直到收到两个D2）
//-服务器向两个客户端发送D3，其中包含来自其D0的彼此数据，
//随后立即向两个客户发送D4 01，完成交易
//-两个客户端都发送相应的子命令以创建库存项目
//TODO:在BB上，服务器是否负责发送适当的项目
//子命令？
//在任何时候，如果发生错误，任何一个客户端都可以发送D4 00
//取消整个序列。然后，服务器应将D4.00发送给这两个服务器
//客户。
//struct SC_TradeItems_D0_D3 { // D0 when sent by client, D3 when sent by server
//    uint16_t target_client_id;
//    uint16_t item_count;
//    // Note: PSO GC sends uninitialized data in the unused entries of this
//    // command. newserv parses and regenerates the item data when sending D3,
//    // which effectively erases the uninitialized data.
//    parray<ItemData, 0x20> items;
//} PACKED;
typedef struct bb_trade_D0_D3 { // D0 when sent by client, D3 when sent by server
    bb_pkt_hdr_t hdr;
    //uint8_t type;
    //uint8_t size;
    uint16_t target_client_id;
    uint16_t item_count;
    // Note: PSO GC sends uninitialized data in the unused entries of this
    // command. newserv parses and regenerates the item data when sending D3,
    // which effectively erases the uninitialized data.
    //注意：PSO GC在未使用的条目中发送未初始化的数据
    // //命令。newserv在发送D3时解析并重新生成项目数据，
    //这有效地擦除了未初始化的数据。
    item_t items[0x20];
} PACKED bb_trade_D0_D3_pkt;

//typedef struct bb_trade_item {
//    uint8_t other_client_id;
//    uint8_t confirmed; // true if client has sent a D2 command
//    //std::vector<ItemData> items;
//    item_t items;
//} PACKED bb_trade_item_pkt;

// D1 (S->C): Advance trade state (V3/BB)
// No arguments
// See D0 description for usage information.

// D2 (C->S): Trade can proceed (V3/BB)
// No arguments
// See D0 description for usage information.

// D3 (S->C): Execute trade (V3/BB)
// Same format as D0. See D0 description for usage information.

// D4 (C->S): Trade failed (V3/BB)
// No arguments
// See D0 description for usage information.

// D4 (S->C): Trade complete (V3/BB)
// header.flag must be 0 (trade failed) or 1 (trade complete).
// See D0 description for usage information.

// D5: Large message box (V3/BB)
// Same as 1A command, except the maximum length of the message is 0x1000 bytes.
// The packet sent to display a large message to the user
typedef struct dc_msg_box {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char msg[];
} PACKED dc_msg_box_pkt;

typedef struct bb_msg_box {
    bb_pkt_hdr_t hdr;
    char msg[];
} PACKED bb_msg_box_pkt;

typedef struct msg_box {
    union hdr_t{
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
        bb_pkt_hdr_t bb;
    } hdr;
    char msg[];
} PACKED msg_box_pkt;

// D6 (C->S): Large message box closed (V3)
// No arguments
// DC, PC, and BB do not send this command at all. GC US v1.00 and v1.01 will
// send this command when any large message box (1A/D5) is closed; GC Plus and
// Episode 3 will send D6 only for large message boxes that occur before the
// client has joined a lobby. (After joining a lobby, large message boxes will
// still be displayed if sent by the server, but the client won't send a D6 when
// they are closed.) In some of these versions, there is a bug that sets an
// incorrect interaction mode when the message box is closed while the player is
// in the lobby; some servers (e.g. Schtserv) send a lobby welcome message
// anyway, along with an 01 (lobby message box) which properly sets the
// interaction mode when closed.

// D7 (C->S): Request GBA game file (V3)
// The server should send the requested file using A6/A7 commands.
// This command exists on XB as well, but it presumably is never sent by the
// client.
// The packet used to ask for a GBA file
typedef struct gc_gba_req {
    dc_pkt_hdr_t hdr;
    char filename[16];
} PACKED gc_gba_req_pkt;

// D7 (S->C): Unknown (V3/BB)
// No arguments
// This command is not valid on PSO GC Episodes 1&2 Trial Edition.
// On PSO V3, this command does... something. The command isn't *completely*
// ignored: it sets a global state variable, but it's not clear what that
// variable does. The variable is set to 0 when the client requests a GBA game
// (by sending a D7 command), and set to 1 when the client receives a D7
// command. The S->C D7 command may be used for declining a download or
// signaling an error of some sort.
// PSO BB accepts but completely ignores this command.

// D8 (C->S): Info board request (V3/BB)
// No arguments
// The server should respond with a D8 command (described below).
// The packet sent to clients to read the info board

// D8 (S->C): Info board contents (V3/BB)
// This command is not valid on PSO GC Episodes 1&2 Trial Edition.
// Command is a list of these; header.flag is the entry count. There should be
// one entry for each player in the current lobby/game.
typedef struct gc_read_info {
    dc_pkt_hdr_t hdr;
    struct {
        char name[0x10];
        char msg[0xAC];
    } entries[0];
} PACKED gc_read_info_pkt;

typedef struct bb_read_info {
    bb_pkt_hdr_t hdr;
    struct {
        uint16_t name[0x10];
        uint16_t msg[0xAC];
    } entries[0];
} PACKED bb_read_info_pkt;

// D9 (C->S): Write info board (V3/BB)
// Contents are plain text, like 1A/D5.
// Server does not respond
// The packet used to write to the infoboard
typedef autoreply_set_pkt gc_write_info_pkt;
typedef bb_autoreply_set_pkt bb_write_info_pkt;

// DA (S->C): Change lobby event (V3/BB)
// header.flag = new event number; no other arguments.
// This command is not valid on PSO GC Episodes 1&2 Trial Edition.

// DB (C->S): Verify license (V3/BB)
// Server should respond with a 9A command.
// The packet to verify that a hunter's license has been procured.
/* 224 字节 */
typedef struct login_v3_hlcheck {
    dc_pkt_hdr_t hdr;
    uint8_t padding1[32];
    char serial_number1[8]; /* 序列号变成了16进制 */
    uint8_t padding2[8];
    char access_key1[12];
    uint8_t padding3[12];
    uint32_t sub_version;
    char serial_number2[8];
    uint8_t padding4[40];
    char access_key2[12];
    uint8_t padding5[36];
    char password[48]; /* 数据包传递明文密码 */
} PACKED v3_hlcheck_pkt;

/* The same as above, but for xbox. */
typedef struct login_xb_hlcheck {
    dc_pkt_hdr_t hdr;
    uint8_t padding1[32];
    uint8_t xbl_gamertag[16];
    uint8_t xbl_userid[16];
    uint8_t padding2[8];
    uint8_t version;
    uint8_t padding3[3];
    uint8_t xbl_gamertag2[16];
    uint8_t padding4[32];
    uint8_t xbl_userid2[16];
    uint8_t padding5[32];
    uint8_t xbox_pso[8];        /* Literally "xbox-pso" */
    uint8_t padding6[40];
} PACKED xb_hlcheck_pkt;

// Note: This login pathway generally isn't used on BB (and isn't supported at
// all during the data server phase). All current servers use 03/93 instead.
//struct C_VerifyLicense_BB_DB {
//    // Note: These four fields are likely the same as those used in BB's 9E
//    ptext<char, 0x10> unknown_a3; // Always blank?
//    ptext<char, 0x10> unknown_a4; // == "?"
//    ptext<char, 0x10> unknown_a5; // Always blank?
//    ptext<char, 0x10> unknown_a6; // Always blank?
//    uint32_t sub_version;
//    ptext<char, 0x30> username;
//    ptext<char, 0x30> password;
//    ptext<char, 0x30> game_tag; // "psopc2"
//} PACKED;

// DC: Player menu state (Episode 3)
// No arguments. It seems the client expects the server to respond with another
// DC command, the contents and flag of which are ignored entirely - all it does
// is set a global flag on the client. This could be the mechanism for waiting
// until all players are at the counter, like how AC (quest barrier) works. I
// haven't spent any time investigating what this actually does; newserv just
// immediately and unconditionally responds to any DC from an Episode 3 client.

// DC: Guild card data (BB)
// Blue Burst packet that acts as a header for the client's guildcard data.
// 01DC
typedef struct bb_guildcard_hdr {
    bb_pkt_hdr_t hdr;
    uint32_t one; // 1
    uint32_t len; // 0x0000D590
    uint32_t checksum; // CRC32 of entire guild card file (0xD590 bytes)
} PACKED bb_guildcard_hdr_pkt;

// 02DC
/* Blue Burst packet for sending a chunk of guildcard data. */
typedef struct bb_guildcard_chunk {
    bb_pkt_hdr_t hdr;
    uint32_t unk;
    uint32_t chunk_index;
    uint8_t data[]; //0x6800 each chunk Command may be shorter if this is the last chunk
} PACKED bb_guildcard_chunk_pkt;

// 03DC
/* Blue Burst packet that requests guildcard data. */
typedef struct bb_guildcard_req {
    bb_pkt_hdr_t hdr;
    uint32_t unk;
    uint32_t chunk_index;
    uint32_t cont;
} PACKED bb_guildcard_req_pkt;

// DD (S->C): Send quest state to joining player (BB)
// When a player joins a game with a quest already in progress, the server
// should send this command to the leader. header.flag is the client ID that the
// leader should send quest state to; the leader will then send a series of
// target commands (62/6D) that the server can forward to the joining player.
// No other arguments
typedef struct bb_send_quest_state {
    // Unused entries are set to FFFF
    bb_pkt_hdr_t hdr;
    uint8_t data[0];
} PACKED bb_send_quest_state_pkt;

// DE (S->C): Rare monster list (BB)
typedef struct bb_rare_monster_list {
    bb_pkt_hdr_t hdr;
    uint16_t enemy_ids[0x10];
} PACKED bb_rare_monster_list_pkt;

// DF (C->S): Unknown (BB)
// This command has many subcommands. It's not clear what any of them do.
//////////////////////////////////////////////////////////////////////////
/* TODO 完成挑战模式BLOCK DF指令*/
typedef struct bb_challenge_01df {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1;
} PACKED bb_challenge_01df_pkt;

typedef struct bb_challenge_02df {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1;
} PACKED bb_challenge_02df_pkt;

typedef struct bb_challenge_03df {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1;
} PACKED bb_challenge_03df_pkt;

typedef struct bb_challenge_04df {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1;
} PACKED bb_challenge_04df_pkt;

typedef struct bb_challenge_05df {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1;
    uint16_t unknown_a2[0x0C];
} PACKED bb_challenge_05df_pkt;

typedef struct bb_challenge_06df {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1[3];
} PACKED bb_challenge_06df_pkt;

typedef struct bb_challenge_07df {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1; //0xFFFFFFFF
    uint32_t unknown_a2; // ALways 0
    uint32_t unknown_a3[5]; //应该是物品item_t
} PACKED bb_challenge_07df_pkt;

// E0 (S->C): Tournament list (Episode 3)
// The client will send 09 and 10 commands to inspect or enter a tournament. The
// server should respond to an 09 command with an E3 command; the server should
// respond to a 10 command with an E2 command.

// header.flag is the count of filled-in entries.
//struct S_TournamentList_GC_Ep3_E0 {
//    struct Entry {
//        uint32_t menu_id;
//        uint32_t item_id;
//        uint8_t unknown_a1;
//        uint8_t locked; // If nonzero, the lock icon appears in the menu
//        // Values for the state field:
//        // 00 = Preparing
//        // 01 = 1st Round
//        // 02 = 2nd Round
//        // 03 = 3rd Round
//        // 04 = Semifinals
//        // 05 = Entries no longer accepted
//        // 06 = Finals
//        // 07 = Preparing for Battle
//        // 08 = Battle in progress
//        // 09 = Preparing to view Battle
//        // 0A = Viewing a Battle
//        // Values beyond 0A don't appear to cause problems, but cause strings to
//        // appear that are obviously not intended to appear in the tournament list,
//        // like "View the board" and "Board: Write". (In fact, some of the strings
//        // listed above may be unintended for this menu as well.)
//        uint8_t state;
//        uint8_t unknown_a2;
//        uint32_t start_time; // In seconds since Unix epoch
//        ptext<char, 0x20> name;
//        uint16_t num_teams;
//        uint16_t max_teams;
//        uint16_t unknown_a3;
//        uint16_t unknown_a4;
//    } PACKED;
//    parray<Entry, 0x20> entries;
//} PACKED;

// E0 (C->S): Request team and key config (BB)
// Blue Burst packet that requests option data.
typedef struct bb_option_req {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_option_req_pkt;

// E1 (S->C): Game information (Episode 3)
// The header.flag argument determines which fields are valid (and which panes
// should be shown in the information window). The values are the same as for
// the E3 command, but each value only makes sense for one command. That is, 00,
// 01, and 04 should be used with the E1 command, while 02, 03, and 05 should be
// used with the E3 command. See the E3 command for descriptions of what each
// flag value means.

//struct S_GameInformation_GC_Ep3_E1 {
//    /* 0004 */ ptext<char, 0x20> game_name;
//    struct PlayerEntry {
//        ptext<char, 0x10> name; // From disp.name
//        ptext<char, 0x20> description; // Usually something like "FOmarl CLv30 J"
//    } PACKED;
//    /* 0024 */ parray<PlayerEntry, 4> player_entries;
//    /* 00E4 */ parray<uint8_t, 0x20> unknown_a3;
//    /* 0104 */ Episode3::Rules rules;
//    /* 0114 */ parray<uint8_t, 4> unknown_a4;
//    /* 0118 */ parray<PlayerEntry, 8> spectator_entries;
//} PACKED;

// E2 (C->S): Tournament control (Episode 3)
// No arguments (in any of its forms) except header.flag, which determines ths
// command's meaning. Specifically:
//   flag=00: request tournament list (server responds with E0)
//   flag=01: check tournament (server responds with E2)
//   flag=02: cancel tournament entry (server responds with CC)
//   flag=03: create tournament spectator team (server responds with E0)
//   flag=04: join tournament spectator team (server responds with E0)
// In case 02, the resulting CC command has header.flag = 0 to indicate the
// player is no longer registered.
// In cases 03 and 04, the client handles follow-ups differently from the 00
// case. In case 04, the client will send a 10 (to which the server responds
// with an E2), but when a choice is made from that menu, the client sends E6 01
// instead of 10. In case 03, the flow is similar, but the client sends E7
// instead of E6 01.
// newserv responds with a standard ship info box (11), but this seems not to be
// intended since it partially overlaps some windows.

// E2 (S->C): Tournament entry list (Episode 3)
// Client may send 09 commands if the player presses X. It's not clear what the
// server should respond with in this case.
// If the player selects an entry slot, client will respond with a long-form 10
// command (the Flag03 variant); in this case, unknown_a1 is the team name, and
// password is the team password. The server should respond to that with a CC
// command.

//struct S_TournamentEntryList_GC_Ep3_E2 {
//    uint16_t players_per_team;
//    uint16_t unused;
//    struct Entry {
//        uint32_t menu_id;
//        uint32_t item_id;
//        uint8_t unknown_a1;
//        // If locked is nonzero, a lock icon appears next to this team and the
//        // player is prompted for a password if they select this team.
//        uint8_t locked;
//        // State values:
//        // 00 = empty (team_name is ignored; entry is selectable)
//        // 01 = present, joinable (team_name renders in white)
//        // 02 = present, finalized (team_name renders in yellow)
//        // If state is any other value, the entry renders as if its state were 02,
//        // but cannot be selected at all (the menu cursor simply skips over it).
//        uint8_t state;
//        uint8_t unknown_a2;
//        ptext<char, 0x20> name;
//    } PACKED;
//    parray<Entry, 0x20> entries;
//} PACKED;

#ifdef PSOCN_CHARACTERS_H

// E2 (S->C): Team and key config (BB)
// See KeyAndTeamConfigBB in Player.hh for format
/* Blue Burst option configuration packet */
typedef struct bb_opt_config {
    bb_pkt_hdr_t hdr;
    uint8_t unk1[276];                            // 276 - 264 = 12
    bb_key_config_t key_cfg;
    bb_guild_t guild_data;
} PACKED bb_opt_config_pkt;

#endif

// E3 (S->C): Game or tournament info (Episode 3)
// The header.flag argument determines which fields are valid (and which panes
// should be shown in the information window). The values are:
// flag=00: Opponents pane only
// flag=01: Opponents and Rules panes
// flag=02: Rules and bracket panes (the bracket pane uses the tournament's name
//          as its title)
// flag=03: Opponents, Rules, and bracket panes
// flag=04: Spectators and Opponents pane
// flag=05: Spectators, Opponents, Rules, and bracket panes
// Sending other values in the header.flag field results in a blank info window
// with unintended strings appearing in the window title.
// Presumably the cases above would be used in different scenarios, probably:
// 00: When inspecting a non-tournament game with no battle in progress
// 01: When inspecting a non-tournament game with a battle in progress
// 02: When inspecting tournaments that have not yet started
// 03: When inspecting a tournament match
// 04: When inspecting a non-tournament spectator team
// 05: When inspecting a tournament spectator team
// The 00, 01, and 04 cases don't really make sense, because the E1 command is
// more appropriate for inspecting non-tournament games.

//struct S_TournamentGameDetails_GC_Ep3_E3 {
//    // These fields are used only if the Rules pane is shown
//    /* 0004/032C */ ptext<char, 0x20> name;
//    /* 0024/034C */ ptext<char, 0x20> map_name;
//    /* 0044/036C */ Episode3::Rules rules;
//
//    /* 0054/037C */ parray<uint8_t, 4> unknown_a1;
//
//    // This field is used only if the bracket pane is shown
//    struct BracketEntry {
//        uint16_t win_count;
//        uint16_t is_active;
//        ptext<char, 0x18> team_name;
//        parray<uint8_t, 8> unused;
//    } PACKED;
//    /* 0058/0380 */ parray<BracketEntry, 0x20> bracket_entries;
//
//    // This field is used only if the Opponents pane is shown. If players_per_team
//    // is 2, all fields are shown; if player_per_team is 1, team_name and
//    // players[1] is ignored (only players[0] is shown).
//    struct PlayerEntry {
//        ptext<char, 0x10> name;
//        ptext<char, 0x20> description; // Usually something like "RAmarl CLv24 E"
//    } PACKED;
//    struct TeamEntry {
//        ptext<char, 0x10> team_name;
//        parray<PlayerEntry, 2> players;
//    } PACKED;
//    /* 04D8/0800 */ parray<TeamEntry, 2> team_entries;
//
//    /* 05B8/08E0 */ uint16_t num_bracket_entries;
//    /* 05BA/08E2 */ uint16_t players_per_team;
//    /* 05BC/08E4 */ uint16_t unknown_a4;
//    /* 05BE/08E6 */ uint16_t num_spectators;
//    /* 05C0/08E8 */ parray<PlayerEntry, 8> spectator_entries;
//} PACKED;

// E3 (C->S): Player preview request (BB) BB_CHARACTER_SELECT_TYPE
// Blue Burst packet used to select a character.
typedef struct bb_char_select {
    bb_pkt_hdr_t hdr;
    uint32_t slot;
    uint32_t reason;
} PACKED bb_char_select_pkt;

// E4: CARD lobby battle table state (Episode 3)
// When client sends an E4, server should respond with another E4 (but these
// commands have different formats).
// When the client has received an E4 command in which all entries have state 0
// or 2, the client will stop the player from moving and show a message saying
// that the game will begin shortly. The server should send a 64 command shortly
// thereafter.

// header.flag = seated state (1 = present, 0 = leaving)
//struct C_CardBattleTableState_GC_Ep3_E4 {
//    uint16_t table_number;
//    uint16_t seat_number;
//} PACKED;

// header.flag = table number
//struct S_CardBattleTableState_GC_Ep3_E4 {
//    struct Entry {
//        // State values:
//        // 0 = no player present
//        // 1 = player present, not confirmed
//        // 2 = player present, confirmed
//        // 3 = player present, declined
//        uint16_t state;
//        uint16_t unknown_a1;
//        uint32_t guild_card_number;
//    } PACKED;
//    parray<Entry, 4> entries;
//} PACKED;

// E4 (S->C): Player choice or no player present (BB)
// Blue Burst packet to acknowledge a character select.
typedef struct bb_char_ack {
    bb_pkt_hdr_t hdr;
    uint32_t slot;
    uint32_t code; // 1 = approved 2 = no player present
} PACKED bb_char_ack_pkt;

// E5 (C->S): Confirm CARD lobby battle table choice (Episode 3)
// header.flag specifies whether the client answered "Yes" (1) or "No" (0).

//struct S_CardBattleTableConfirmation_GC_Ep3_E5 {
//    uint16_t table_number;
//    uint16_t seat_number;
//} PACKED;

// E5 (S->C): Player preview (BB)
// E5 (C->S): Create character (BB)
#ifdef PSOCN_CHARACTERS_H

/* Blue Burst packet for creating/updating a character as well as for the
   previews sent for the character select screen. */
typedef struct bb_char_preview {
    bb_pkt_hdr_t hdr;
    uint8_t slot;
    uint8_t sel_char; // selected char
    uint16_t flags; // just in case we lose them somehow between connections
    psocn_bb_mini_char_t data;
} PACKED bb_char_preview_pkt;

#endif /* PSOCN_CHARACTERS_H */

// E6 (C->S): Spectator team control (Episode 3)

// With header.flag == 0, this command has no arguments and is used for
// requesting the spectator team list. The server responds with an E6 command.

// With header.flag == 1, this command is used for joining a tournament
// spectator team. The following arguments are given in this form:

//struct C_JoinSpectatorTeam_GC_Ep3_E6_Flag01 {
//    uint32_t menu_id;
//    uint32_t item_id;
//} PACKED;

// E6 (S->C): Spectator team list (Episode 3)
// Same format as 08 command.

// E6 (S->C): Set guild card number and update client config (BB)
// BB clients have multiple client configs. This command sets the client config
// that is returned by the 93 commands, but does not affect the client config
// set by the 04 command (and returned in the 9E and 9F commands).

// E7 (C->S): Create spectator team (Episode 3)
// This command is used to create speectator teams for both tournaments and
// regular games. The server should be able to tell these cases apart by the
// menu and/or item ID.

//struct C_CreateSpectatorTeam_GC_Ep3_E7 {
//    uint32_t menu_id;
//    uint32_t item_id;
//    ptext<char, 0x10> name;
//    ptext<char, 0x10> password;
//    uint32_t unused;
//} PACKED;

// E7 (S->C): Unknown (Episode 3)
// Same format as E2 command.

#ifdef PSOCN_CHARACTERS_H

// E7: Save or load full player data (BB)
// See export_bb_player_data() in Player.cc for format.
// TODO: Verify full breakdown from send_E7 in BB disassembly.
/* Blue Burst packet for sending the full character data and options */
typedef struct bb_full_char {
    bb_pkt_hdr_t hdr;
    psocn_bb_full_char_t data;
} PACKED bb_full_char_pkt;

//struct SC_SyncCharacterSaveFile_BB_00E7 {
//    bb_pkt_hdr_t hdr;
//    /* 0000 */ PlayerInventory inventory; // From player data
//    /* 034C */ PlayerDispDataBB disp; // From player data
//    /* 04DC */ le_uint32_t unknown_a1;
//    /* 04E0 */ le_uint32_t creation_timestamp;
//    /* 04E4 */ le_uint32_t signature; // == 0xA205B064 (see SaveFileFormats.hh)
//    /* 04E8 */ le_uint32_t play_time_seconds;
//    /* 04EC */ le_uint32_t option_flags; // account
//    /* 04F0 */ parray<uint8_t, 0x0208> quest_data1; // player
//    /* 06F8 */ PlayerBank bank; // player
//    /* 19C0 */ GuildCardBB guild_card;
//    /* 1AC8 */ le_uint32_t unknown_a3;
//    /* 1ACC */ parray<uint8_t, 0x04E0> symbol_chats; // account
//    /* 1FAC */ parray<uint8_t, 0x0A40> shortcuts; // account
//    /* 29EC */ ptext<char16_t, 0x00AC> auto_reply; // player
//    /* 2B44 */ ptext<char16_t, 0x00AC> info_board; // player
//    /* 2C9C */ PlayerRecords_Battle<false> battle_records;
//    /* 2CB4 */ parray<uint8_t, 4> unknown_a4;
//    /* 2CB8 */ PlayerRecordsBB_Challenge challenge_records;
//    /* 2DF8 */ parray<uint8_t, 0x0028> tech_menu_config; // player
//    /* 2E20 */ parray<uint8_t, 0x002C> unknown_a6;
//    /* 2E4C */ parray<uint8_t, 0x0058> mode_quest_data; // player
//    /* 2EA4 */ KeyAndTeamConfigBB key_config; // account
//    /* 3994 */
//} PACKED;

#endif /* PSOCN_CHARACTERS_H */

// E8 (S->C): Join spectator team (Episode 3)
// header.flag = player count (including spectators)

//struct S_JoinSpectatorTeam_GC_Ep3_E8 {
//    parray<uint32_t, 0x20> variations; // 04-84; unused
//    struct PlayerEntry {
//        PlayerLobbyDataDCGC lobby_data; // 0x20 bytes
//        PlayerInventory inventory; // 0x34C bytes
//        PlayerDispDataDCPCV3 disp; // 0xD0 bytes
//    } PACKED; // 0x43C bytes
//    parray<PlayerEntry, 4> players; // 84-1174
//    uint8_t client_id;
//    uint8_t leader_id;
//    uint8_t disable_udp = 1;
//    uint8_t difficulty;
//    uint8_t battle_mode;
//    uint8_t event;
//    uint8_t section_id;
//    uint8_t challenge_mode;
//    uint32_t rare_seed;
//    uint8_t episode;
//    uint8_t unused2 = 1;
//    uint8_t solo_mode;
//    uint8_t unused3;
//    struct SpectatorEntry {
//        uint32_t player_tag;
//        uint32_t guild_card_number;
//        ptext<char, 0x20> name;
//        uint8_t present;
//        uint8_t unknown_a3;
//        uint16_t level;
//        parray<uint32_t, 2> unknown_a5;
//        parray<uint16_t, 2> unknown_a6;
//    } PACKED; // 0x38 bytes
//    // Somewhat misleadingly, this array also includes the players actually in the
//    // battle - they appear in the first positions. Presumably the first 4 are
//    // always for battlers, and the last 8 are always for spectators.
//    parray<SpectatorEntry, 12> entries; // 1184-1424
//    ptext<char, 0x20> spectator_team_name;
//    // This field doesn't appear to be actually used by the game, but some servers
//    // send it anyway (and the game presumably ignores it)
//    parray<PlayerEntry, 8> spectator_players;
//} PACKED;

// E8 (C->S): Guild card commands (BB)
typedef struct bb_guildcard {
    bb_pkt_hdr_t hdr;
    psocn_bb_guild_card_t gc_data;
} PACKED bb_guildcard_pkt;

// 01E8 (C->S): Check guild card file checksum
// This struct is for documentation purposes only; newserv ignores the contents
// of this command.
// Blue Burst packet to send the client's checksum
typedef struct bb_checksum {
    bb_pkt_hdr_t hdr;
    uint32_t checksum;
    uint32_t padding;
} PACKED bb_checksum_pkt;

// 02E8 (S->C): Accept/decline guild card file checksum
// If needs_update is nonzero, the client will request the guild card file by
// sending an 03E8 command. If needs_update is zero, the client will skip
// downloading the guild card file and send a 04EB command (requesting the
// stream file) instead.
// Blue Burst packet to acknowledge the client's checksum.
typedef struct bb_checksum_ack {
    bb_pkt_hdr_t hdr;
    //uint32_t ack;
    uint32_t needs_update;
    uint32_t unused;
} PACKED bb_checksum_ack_pkt;

// 03E8 (C->S): Request guild card file
// No arguments
// Server should send the guild card file data using DC commands.

// 04E8 (C->S): Add guild card
// Format is GuildCardBB (see Player.hh)
typedef bb_guildcard_pkt bb_guildcard_add_pkt;

// 05E8 (C->S): Delete guild card
// Blue Burst packet for deleting a Guildcard
typedef struct bb_guildcard_del {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;
} PACKED bb_guildcard_del_pkt;

// 06E8 (C->S): Update (overwrite) guild card
// Note: This command is also sent when the player writes a comment on their own
// guild card.
// Format is GuildCardBB (see Player.hh)
// Blue Burst packet for adding a Guildcard to the user's list
typedef bb_guildcard_pkt bb_guildcard_set_txt_pkt;

// 07E8 (C->S): Add blocked user
// Format is GuildCardBB (see Player.hh)
typedef bb_guildcard_pkt bb_blacklist_add_pkt;

// 08E8 (C->S): Delete blocked user
// Same format as 05E8.
typedef bb_guildcard_del_pkt bb_blacklist_del_pkt;

// 09E8 (C->S): Write comment
// Blue Burst packet for setting a comment on a Guildcard
typedef struct bb_guildcard_comment {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;
    uint16_t guildcard_desc[88];
} PACKED bb_guildcard_comment_pkt;

// 0AE8 (C->S): Set guild card position in list
// Blue Burst packet for sorting Guildcards
typedef struct bb_guildcard_sort {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard1;
    uint32_t guildcard2;
} PACKED bb_guildcard_sort_pkt;

//struct C_MoveGuildCard_BB_0AE8 {
//    bb_pkt_hdr_t hdr;
//    uint32_t guild_card_number;
//    uint32_t position;
//} PACKED;

// E9 (S->C): Remove player from spectator team (Episode 3)
// Same format as 66/69 commands. Like 69 (and unlike 66), the disable_udp field
// is unused in command E9. When a spectator leaves a spectator team, the
// primary players should receive a 6xB4x52 command to update their spectator
// counts.

// EA (S->C): Timed message box (Episode 3)
// The message appears in the upper half of the screen; the box is as wide as
// the 1A/D5 box but is vertically shorter. The box cannot be dismissed or
// interacted with by the player in any way; it disappears by itself after the
// given number of frames.
// header.flag appears to be relevant - the handler's behavior is different if
// it's 1 (vs. any other value). There don't seem to be any in-game behavioral
// differences though.

//struct S_TimedMessageBoxHeader_GC_Ep3_EA {
//    uint32_t duration; // In frames; 30 frames = 1 second
//    // Message data follows here (up to 0x1000 chars)
//} PACKED;

// EA: Team control (BB)

/* Blue Burst 公会数据包 */
typedef struct bb_guild_pkt {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_pkt_pkt;

/* TODO 解析公会会员的列表信息 暂未确定最后 8 位情况 */
typedef struct bb_guild_member_list {
    uint32_t member_index;
    uint32_t guild_priv_level;
    uint32_t guildcard_client;
    uint16_t char_name[BB_CHARACTER_NAME_LENGTH];
    uint8_t guild_reward[8];
} PACKED bb_guild_member_list_t;

//////////////////////////////////////////////////////////////////////////
/* Blue Burst 公会创建数据 */
typedef struct bb_guild_data {
    bb_pkt_hdr_t hdr;
    psocn_bb_db_guild_t guild;
    uint32_t padding;
} PACKED bb_guild_data_pkt;

typedef struct bb_guild_rv_data {
    bb_pkt_hdr_t hdr;
    uint8_t data[0];
} PACKED bb_guild_rv_data_pkt;

// 01EA (C->S): 创建公会
typedef struct bb_guild_create {
    bb_pkt_hdr_t hdr;
    uint16_t guild_name[0x000E];
    uint32_t guildcard;
} PACKED bb_guild_create_pkt;

// 02EA (S->C): 应该是某种公会功能完成后的响应
// This command behaves exactly like 1FEA.
typedef struct bb_guild_unk_02EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_02EA_pkt;

// 03EA (C->S): 新增会员
typedef struct bb_guild_member_add {
    bb_pkt_hdr_t hdr;
    uint32_t target_guildcard;
    uint8_t data[];
} PACKED bb_guild_member_add_pkt;

// 04EA (S->C): UNKNOW
typedef struct bb_guild_unk_04EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_04EA_pkt;

// 05EA (C->S): 移除公会会员
// Same format as 03EA.
typedef struct bb_guild_member_remove {
    bb_pkt_hdr_t hdr;
    uint32_t target_guildcard;
    uint8_t data[];
} PACKED bb_guild_member_remove_pkt;

// 06EA (S->C): UNKNOW
typedef struct bb_guild_unk_06EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_06EA_pkt;

// 07EA (C->S): 公会聊天
typedef struct bb_guild_member_chat {
    bb_pkt_hdr_t hdr;                                            /* 0x0000 8 */
    uint16_t name[BB_CHARACTER_NAME_LENGTH];                     /* 0x000C 24 */
    // It seems there are no real limits on the message length, other than the
    // overall command length limit of 0x7C00 bytes.
    uint8_t chat[];
} PACKED bb_guild_member_chat_pkt;

// 08EA (C->S): Team admin
// No arguments
typedef struct bb_guild_member_setting {
    bb_pkt_hdr_t hdr;
    uint32_t guild_member_amount;
    bb_guild_member_list_t entries[0];
} PACKED bb_guild_member_setting_pkt;

// 09EA (S->C): UNKNOW
typedef struct bb_guild_unk_09EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_09EA_pkt;

// 09EA (S->C): Unknown
struct S_Unknown_BB_09EA {
    bb_pkt_hdr_t hdr;
    uint32_t entry_count;
    uint8_t unknown_a2[4];
    struct Entry123 {
        // This is displayed as "<%04d> %s" % (value, message)
        uint32_t value;
        uint32_t color; // 0x10 or 0x20 = green, 0x30 = blue, 0x40 = red, anything else = white
        uint32_t unknown_a1;
        uint16_t message[0x10];
    } PACKED entries[0];// [entry_count] actually
} PACKED;

// 0AEA (S->C): UNKNOW
typedef struct bb_guild_unk_0AEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_0AEA_pkt;

// 0BEA (S->C): UNKNOW
typedef struct bb_guild_unk_0BEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_0BEA_pkt;

// 0CEA (S->C): UNKNOW
typedef struct bb_guild_unk_0CEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_0CEA_pkt;

// 0CEA (S->C): Unknown
struct S_Unknown_BB_0CEA {
    uint8_t unknown_a1[0x20];
    uint16_t unknown_a2[0];
} PACKED;

// 0DEA (C->S): Unknown
// No arguments
typedef struct bb_guild_invite_0DEA {
    bb_pkt_hdr_t hdr;
    uint32_t guild_id;
    uint32_t guildcard;
    uint8_t data[];
} PACKED bb_guild_invite_0DEA_pkt;

// 0EEA (S->C): Unknown
typedef struct bb_guild_unk_0EEA {
    bb_pkt_hdr_t hdr;                      // 0x00 - 0x07
    uint32_t guildcard;                    // 0x08 - 0x0B
    uint32_t guild_id;                     // 0x0C - 0x0F
    uint8_t guild_info[8];                 // 0x10 - 0x17
    uint16_t guild_name[0x000E];           // 0x18 - 0x1C
    uint16_t unk_flag;
    uint8_t guild_flag[0x0800];            // 公会图标
    uint8_t unk2;
    uint8_t unk3;
} PACKED bb_guild_unk_0EEA_pkt;

//不带函数头的数据包结构
typedef struct bb_guild_0EEA {
    uint32_t guildcard;                    // 02B8         4
    uint32_t guild_id;                     // 02BC
    uint8_t guild_info[8];                 // 公会信息     8
    uint16_t guild_name[0x000E];           // 02CC
    uint16_t unk_flag;
    uint8_t guild_flag[0x0800];            // 公会图标
    uint8_t panding;
} PACKED bb_guild_0EEA_pkt;

// 0FEA (C->S): 设置公会旗帜
typedef struct bb_guild_member_flag_setting {
    bb_pkt_hdr_t hdr;
    uint8_t guild_flag[0x0800];
} PACKED bb_guild_member_flag_setting_pkt;

// 10EA: 解散公会
// No arguments
typedef struct bb_guild_dissolve {
    bb_pkt_hdr_t hdr;
    //uint32_t guild_id;
    uint8_t data[];
} PACKED bb_guild_dissolve_pkt;

// 11EA (C->S): 提升会员权限
// TODO: header.flag is used for this command. Figure out what it's for.
typedef struct bb_guild_member_promote {
    bb_pkt_hdr_t hdr;
    uint32_t target_guildcard;
} PACKED bb_guild_member_promote_pkt;

// 12EA (S->C): Unknown
typedef struct bb_guild_unk_12EA {
    bb_pkt_hdr_t hdr;
    uint32_t unk; // Command is ignored unless this is 0
    uint32_t guildcard;
    uint32_t guild_id;
    uint8_t guild_info[8];
    uint32_t guild_priv_level;
    uint16_t guild_name[0x000E];
    uint32_t guild_rank;
} PACKED bb_guild_unk_12EA_pkt;

typedef struct bb_guild_lobby_client {
    uint32_t guild_owner_gc;
    uint32_t guild_id;
    uint32_t guild_points_rank;
    uint32_t guild_points_rest;
    uint32_t guild_priv_level;
    uint16_t guild_name[0x000E];
    uint32_t guild_rank;
    uint32_t client_guildcard;
    uint32_t client_id;
    uint16_t char_name[BB_CHARACTER_NAME_LENGTH];
    //uint8_t guild_reward[8];
    uint8_t guild_flag[0x0800];
} PACKED bb_guild_lobby_client_t;

// 13EA: 大厅玩家公会邀请
// header.flag specifies the number of entries.
typedef struct bb_guild_lobby_setting {
    bb_pkt_hdr_t hdr;
    bb_guild_lobby_client_t entries[0];
} PACKED bb_guild_lobby_setting_pkt;

// 14EA (C->S): 玩家头衔反馈
// No arguments. Client always sends 1 in the header.flag field.
typedef struct bb_guild_member_tittle {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_member_tittle_pkt;

// 15EA (S->C): BB 玩家整体公会数据 用于发送给其他玩家
typedef struct bb_full_guild_data {
    bb_pkt_hdr_t hdr;                              /* 0x0000 - 0x0007*/
    bb_guild_lobby_client_t entries;
} PACKED bb_full_guild_data_pkt;

// 16EA (S->C): UNKNOW
typedef struct bb_guild_unk_16EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_16EA_pkt;

// 17EA (S->C): 未完成
typedef struct bb_guild_unk_17EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_17EA_pkt;

// 18EA: 公会点数情报子列表
typedef struct bb_guild_point_info_list {
    bb_guild_member_list_t member_list;
    uint32_t guild_points_personal_donation;
} PACKED bb_guild_point_info_list_t;

// 18EA: 公会点数情报
// 客户端只发送8字节请求 (C->S)
// TODO: 服务器发送至客户端 S->C 
typedef struct bb_guild_buy_privilege_and_point_info {
    bb_pkt_hdr_t hdr;
    uint32_t guild_points_rank;
    uint8_t unk_data2[4];
    uint32_t guild_points_rest;
    uint32_t guild_member_amount;
    bb_guild_point_info_list_t entries[0];
} PACKED bb_guild_buy_privilege_and_point_info_pkt;

// 19EA: 公会排行榜 需要数据库读取支持
// 客户端只发送8字节请求 (C->S)
// TODO: 服务器发送至客户端 S->C
typedef struct bb_guild_privilege_list {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_privilege_list_pkt;

typedef struct bb_guild_special_item_list {
    uint16_t item_name[0x0C];
    uint16_t item_desc[0x60];
    uint32_t price;
} PACKED bb_guild_special_item_list_t;

// 1AEA: (S->C): 购买公会特典
typedef struct bb_guild_buy_special_item {
    bb_pkt_hdr_t hdr;
    uint32_t item_num;
    bb_guild_special_item_list_t entries[0];
} PACKED bb_guild_buy_special_item_pkt;

// 1BEA (C->S): 购买公会特典完成 扣除点数
// header.flag is used, but no other arguments
typedef struct bb_guild_unk_1BEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_1BEA_pkt;

// 1CEA (C->S): Ranking information
// No arguments
typedef struct bb_guild_rank_list {
    bb_pkt_hdr_t hdr;
    struct {//4+4+2+4+34 48
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;         // Should be 0x0F04 this value, apparently
        uint16_t name[0x11];
    } entries[0];
} PACKED bb_guild_rank_list_pkt;

// 1DEA (S->C): UNKNOW
typedef struct bb_guild_unk_1DEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_1DEA_pkt;

// 1EEA (C->S): Unknown
// header.flag is used, but it's unknown what the value means.
typedef struct bb_guild_unk_1EEA {
    bb_pkt_hdr_t hdr;
    uint16_t unknown_a1[0x10];
} PACKED bb_guild_unk_1EEA_pkt;

// 20EA (C->S): Unknown
// header.flag is used, but no other arguments
typedef struct bb_guild_unk_20EA {
    bb_pkt_hdr_t hdr;
} PACKED bb_guild_unk_20EA_pkt;

// EB (S->C): Add player to spectator team (Episode 3)
// Same format and usage as 65 and 68 commands, but sent to spectators in a
// spectator team.
// This command is used to add both primary players and spectators - if the
// client ID in .lobby_data is 0-3, it's a primary player, otherwise it's a
// spectator. (In the case of a primary player joining, the other primary
// players in the game receive a 65 command rather than an EB command to notify
// them of the joining player; in the case of a joining spectator, the primary
// players receive a 6xB4x52 instead.)

// 01EB (S->C): Send stream file index (BB)
// Command is a list of these; header.flag is the entry count.
// Blue Burst packet that's a header for the parameter files.
typedef struct bb_param_hdr {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t size;
        uint32_t checksum;
        uint32_t offset;
        char filename[0x40];
    } entries[];
} PACKED bb_param_hdr_pkt;

// 02EB (S->C): Send stream file chunk (BB)
// Blue Burst packet for sending a chunk of the parameter files.
typedef struct bb_param_chunk {
    bb_pkt_hdr_t hdr;
    uint32_t chunk_index;
    uint8_t data[]; // 0x6800
} PACKED bb_param_chunk_pkt;

// 03EB (C->S): Request a specific stream file chunk
// header.flag is the chunk index. Server should respond with a 02EB command.

// 04EB (C->S): Request stream file header
// No arguments
// Server should respond with a 01EB command.

// EC (C->S): Create game (Episode 3)
// Same format as C1; some fields are unused (e.g. episode, difficulty).

// EC (C->S): Leave character select (BB)
// Blue Burst packet for setting flags (dressing room flag, for instance).
typedef struct bb_setflag {
    bb_pkt_hdr_t hdr;
    // flags codes:
    // 0 = canceled
    // 1 = recreate character
    // 2 = dressing room
    uint32_t flags;
} PACKED bb_setflag_pkt;

// ED (S->C): Force leave lobby/game (Episode 3)
// No arguments
// This command forces the client out of the game or lobby they're currently in
// and sends them to the lobby. If the client is in a lobby (and not a game),
// the client sends a 98 in response as if they were in a game. Curiously, the
// client also sends a meseta transaction (BA) with a value of zero before
// sending an 84 to be added to a lobby. This is used when a spectator team is
// disbanded because the target game ends.

// ED (C->S): Update account data (BB)
// There are several subcommands (noted in the union below) that each update a
// specific kind of account data.
// TODO: Actually define these structures and don't just treat them as raw data
// Blue Burst packet for updating options
typedef struct bb_options_update {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_options_update_pkt;

// XXED ED数据包集合
typedef struct bb_options_config_update {
    bb_pkt_hdr_t hdr;
    union options {
        uint32_t option;
        uint8_t symbol_chats[0x4E0];
        uint8_t shortcuts[0xA40];
        uint8_t key_config[0x16C];
        uint8_t joystick_config[0x38];
        uint8_t tech_menu[PSOCN_STLENGTH_BB_DB_TECH_MENU];
        uint8_t customize[0xE8];
        bb_challenge_records_t c_records;
    };
} PACKED bb_options_config_update_pkt;

// EE: Trade cards (Episode 3)
// This command has different forms depending on the header.flag value; the flag
// values match the command numbers from the Episodes 1&2 trade window sequence.
// The sequence of events with the EE command also matches that of the Episodes
// 1&2 trade window; see the description of the D0 command above for details.

// EE D0 (C->S): Begin trade
//struct SC_TradeCards_GC_Ep3_EE_FlagD0_FlagD3 {
//    uint16_t target_client_id;
//    uint16_t entry_count;
//    struct Entry {
//        uint32_t card_type;
//        uint32_t count;
//    } PACKED;
//    parray<Entry, 4> entries;
//} PACKED;

// EE D1 (S->C): Advance trade state
//struct S_AdvanceCardTradeState_GC_Ep3_EE_FlagD1 {
//    uint32_t unused;
//} PACKED;

// EE D2 (C->S): Trade can proceed
// No arguments

// EE D3 (S->C): Execute trade
// Same format as EE D0

// EE D4 (C->S): Trade failed
// EE D4 (S->C): Trade complete

//struct S_CardTradeComplete_GC_Ep3_EE_FlagD4 {
//    uint32_t success; // 0 = failed, 1 = success, anything else = invalid
//} PACKED;

// EE (S->C): Scrolling message (BB)
// Same format as 01. The message appears at the top of the screen and slowly
// scrolls to the left.
typedef struct bb_info_reply_pkt bb_scroll_msg_pkt;

// EF (C->S): Join card auction (Episode 3)
// This command should be treated like AC (quest barrier); that is, when all
// players in the same game have sent an EF command, the server should send an
// EF back to them all at the same time to start the auction.

// EF (S->C): Start card auction (Episode 3)

//struct S_StartCardAuction_GC_Ep3_EF {
//    uint16_t points_available;
//    uint16_t unused;
//    struct Entry {
//        uint16_t card_id = 0xFFFF; // Must be < 0x02F1
//        uint16_t min_price; // Must be > 0 and < 100
//    } PACKED;
//    parray<Entry, 0x14> entries;
//} PACKED;

// EF (S->C): Set or disable shutdown command (BB)
// All variants of EF except 00EF cause the given Windows shell command to be
// run (via ShellExecuteA) just before the game exits normally. There can be at
// most one shutdown command at a time; a later EF command will overwrite the
// previous EF command's effects. The 00EF command deletes the previous shutdown
// command if any was present, causing no command to run when the game closes.
// There is no indication to the player when a shutdown command has been set.

// This command is likely just a vestigial debugging feature that Sega left in,
// but it presents a fairly obvious security risk. There is no way for the
// server to know whether an EF command it sent has actually executed on the
// client, so newserv's proxy unconditionally blocks this command.
struct S_SetShutdownCommand_BB_01EF {
    char command[0x200];
} PACKED;

// F0 (S->C): Unknown (BB)
typedef struct S_Unknown_BB_F0 {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1[7];
    uint32_t which; // Must be < 12
    uint16_t unknown_a2[0x10];
    uint32_t unknown_a3;
} PACKED S_Unknown_BB_F0_pkt;

// F1: 无效或未解析指令
// F2: 无效或未解析指令
// F3: 无效或未解析指令
// F4: 无效或未解析指令
// F5: 无效或未解析指令
// F6: 无效或未解析指令
// F7: 无效或未解析指令
// F8: 无效或未解析指令
// F9: 无效或未解析指令
// FA: 无效或未解析指令
// FB: 无效或未解析指令
// FC: 无效或未解析指令
// FD: 无效或未解析指令
// FE: 无效或未解析指令
// FF: 无效或未解析指令

/* The packet sent to redirect clients */
typedef struct dc_redirect {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t ip_addr;       /* Big-endian */
    uint16_t port;          /* Little-endian */
    uint8_t padding[2];
} PACKED dc_redirect_pkt;

typedef struct bb_redirect {
    bb_pkt_hdr_t hdr;
    uint32_t ip_addr;       /* Big-endian */
    uint16_t port;          /* Little-endian */
    uint8_t padding[2];
} PACKED bb_redirect_pkt;

typedef struct dc_redirect6 {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint8_t ip_addr[16];
    uint16_t port;          /* Little-endian */
    uint8_t padding[2];
} PACKED dc_redirect6_pkt;

typedef struct bb_redirect6 {
    bb_pkt_hdr_t hdr;
    uint8_t ip_addr[16];
    uint16_t port;          /* Little-endian */
    uint8_t padding[2];
} PACKED bb_redirect6_pkt;

/* The ship list packet send to tell clients what blocks are up */
typedef struct dc_block_list {
    dc_pkt_hdr_t hdr;           /* The flags field says the entry count */
    dc_menu_t entries[0];
} PACKED dc_block_list_pkt;

typedef struct pc_block_list {
    pc_pkt_hdr_t hdr;           /* The flags field says the entry count */
    v3_menu_t entries[0];
} PACKED pc_block_list_pkt;

typedef struct bb_block_list {
    bb_pkt_hdr_t hdr;           /* The flags field says the entry count */
    v3_menu_t entries[0];
} PACKED bb_block_list_pkt;

/* The packet used to send the quest list */
typedef struct dc_quest_list {
    dc_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        char name[32];
        char desc[112];
    } entries[0];
} PACKED dc_quest_list_pkt;

typedef struct pc_quest_list {
    pc_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t name[32];
        uint16_t desc[112];
    } entries[0];
} PACKED pc_quest_list_pkt;

typedef struct xb_quest_list {
    dc_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        char name[32];
        char desc[128];
    } entries[0];
} PACKED xb_quest_list_pkt;

typedef struct bb_quest {
    uint32_t menu_id;
    uint32_t item_id;
    uint16_t name[32];
    uint16_t desc[122];
} PACKED bb_quest_t;

typedef struct bb_quest_list {
    bb_pkt_hdr_t hdr;
    bb_quest_t entries[0];
} PACKED bb_quest_list_pkt;

/* The choice search options packet sent to tell clients what they can actually
   search on */
typedef struct dc_choice_search {
    dc_pkt_hdr_t hdr;
    struct {
        uint16_t menu_id;
        uint16_t item_id;
        char text[0x1C];
    } entries[0];
} PACKED dc_choice_search_pkt;

typedef struct pc_choice_search {
    pc_pkt_hdr_t hdr;
    struct {
        uint16_t menu_id;
        uint16_t item_id;
        uint16_t text[0x1C];
    } entries[0];
} PACKED pc_choice_search_pkt;

typedef struct bb_choice_search {
    bb_pkt_hdr_t hdr;
    struct {
        uint16_t menu_id;
        uint16_t item_id;
        uint8_t text[0x1C];
    } entries[0];
} PACKED bb_choice_search_pkt;

/* The packet sent to set up the user's choice search settings or to actually
   perform a choice search */
typedef struct dc_choice_set {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint8_t off;
    uint8_t padding[3];
    struct {
        uint16_t menu_id;
        uint16_t item_id;
    } entries[5];
} PACKED dc_choice_set_pkt;

/* The packet sent as a reply to a choice search */
typedef struct dc_choice_reply {
    dc_pkt_hdr_t hdr;
    struct {
        uint32_t guildcard;
        char name[0x10];
        char cl_lvl[0x20];
        char location[0x30];
        uint32_t padding;
        uint32_t ip;
        uint16_t port;
        uint16_t padding2;
        uint32_t menu_id;
        uint32_t item_id;
        uint8_t padding3[0x5C];
    } entries[0];
} PACKED dc_choice_reply_pkt;

typedef struct pc_choice_reply {
    pc_pkt_hdr_t hdr;
    struct {
        uint32_t guildcard;
        uint16_t name[0x10];
        uint16_t cl_lvl[0x20];
        uint16_t location[0x30];
        uint32_t padding;
        uint32_t ip;
        uint16_t port;
        uint16_t padding2;
        uint32_t menu_id;
        uint32_t item_id;
        uint8_t padding3[0x7C];
    } entries[0];
} PACKED pc_choice_reply_pkt;

/* The packet sent as a reply to a choice search in IPv6 mode */
typedef struct dc_choice_reply6 {
    dc_pkt_hdr_t hdr;
    struct {
        uint32_t guildcard;
        char name[0x10];
        char cl_lvl[0x20];
        char location[0x30];
        uint32_t padding;
        uint8_t ip[16];
        uint16_t port;
        uint16_t padding2;
        uint32_t menu_id;
        uint32_t item_id;
        uint8_t padding3[0x5C];
    } entries[0];
} PACKED dc_choice_reply6_pkt;

typedef struct pc_choice_reply6 {
    pc_pkt_hdr_t hdr;
    struct {
        uint32_t guildcard;
        uint16_t name[0x10];
        uint16_t cl_lvl[0x20];
        uint16_t location[0x30];
        uint32_t padding;
        uint8_t ip[16];
        uint16_t port;
        uint16_t padding2;
        uint32_t menu_id;
        uint32_t item_id;
        uint8_t padding3[0x7C];
    } entries[0];
} PACKED pc_choice_reply6_pkt;

/* The packet used in trading items */
typedef struct gc_trade {
    dc_pkt_hdr_t hdr;
    uint8_t who;
    uint8_t unk[];
} PACKED gc_trade_pkt;

typedef struct bb_trade {
    bb_pkt_hdr_t hdr;
    uint8_t who;
    uint8_t unk[];
} PACKED bb_trade_pkt;

/* The packet used to send the Episode 3 rank */
typedef struct ep3_rank_update {
    dc_pkt_hdr_t hdr;
    uint32_t rank;
    char rank_txt[12];
    uint32_t meseta;
    uint32_t max_meseta;
    uint32_t jukebox;
} ep3_rank_update_pkt;

/* The packet used to send the Episode 3 card list */
typedef struct ep3_card_update {
    dc_pkt_hdr_t hdr;
    uint32_t size;
    uint8_t data[];
} PACKED ep3_card_update_pkt;

/* The general format for 0xBA packets... */
typedef struct ep3_ba {
    dc_pkt_hdr_t hdr;
    uint32_t meseta;
    uint32_t total_meseta;
    uint16_t unused;                    /* Always 0? */
    uint16_t magic;
} PACKED ep3_ba_pkt;

/* The packet used to communiate various requests and such to the server from
   Episode 3 */
typedef struct ep3_server_data {
    dc_pkt_hdr_t hdr;
    uint8_t unk[4];
    uint8_t data[];
} PACKED ep3_server_data_pkt;

/* The packet used by Episode 3 clients to create a game */
typedef struct ep3_game_create {
    dc_pkt_hdr_t hdr;
    uint32_t unused[2];
    char name[16];
    char password[16];
    uint8_t unused2[2];
    uint8_t view_battle;
    uint8_t episode;
} PACKED ep3_game_create_pkt;

/* Patch packet (thanks KuromoriYu for a lot of this information!)
    The code_begin value is big endian on PSOGC, and little endian on PSODC */
typedef struct patch_send {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t entry_offset;  /* Pointer to offset_start in footer */
    uint32_t crc_start;
    uint32_t crc_length;
    uint32_t code_begin;    /* Pointer to the code, should usually be 4 */
    uint8_t code[];         /* Append the footer below after the code */
} PACKED patch_send_pkt;

/* Footer tacked onto the end of the patch packet. This is all big endian on
   PSOGC and little endian on PSODC.*/
typedef struct patch_send_footer {
    uint32_t offset_count;  /* Pointer to the below, relative to pkt start */
    uint32_t num_ptrs;      /* Number of pointers to relocate (at least 1) */
    uint32_t unk1;          /* 0 (padding?) */
    uint32_t unk2;          /* 0 (padding?) */
    uint32_t offset_start;  /* Set to 0 */
    uint16_t offset_entry;  /* Set to 0 */
    uint16_t offsets[];     /* Offsets to pointers to relocate within code */
} PACKED patch_send_footer;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* Parameters for the various packets. */
#include "pso_opcodes_block.h"
#endif /* !PACKETS_H_HAVE_PACKETS */
#endif /* !PACKETS_H_HEADERS_ONLY */
