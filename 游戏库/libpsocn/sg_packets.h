/*
    梦幻之星中国 船闸服务器 数据包
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

#ifndef SHIPGATE_PACKETS
#define SHIPGATE_PACKETS

#include <stdint.h>
#include <time.h>
#include <inttypes.h>

#include "pso_character.h"
#include "max_tech_level.h"

#ifndef _WIN32 
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* The header that is prepended to any packets sent to the shipgate (new version
   for protocol v10 and newer). 8字节 */
typedef struct shipgate_hdr {
    uint16_t pkt_len;
    uint16_t pkt_type;
    uint8_t version;
    uint8_t reserved;
    uint16_t flags;
} PACKED shipgate_hdr_t;

/* 舰船与舰闸之间相互ping数据包 */
typedef struct shipgate_ping {
    shipgate_hdr_t hdr;
    char host4[32];
    char host6[128];
} PACKED shipgate_ping_t;

/* General error packet. Individual packets can/should extend this base
   structure for more specific instances and to help match requests up with the
   error replies. */
typedef struct shipgate_error {
    shipgate_hdr_t hdr;
    uint32_t error_code;
    uint32_t reserved;
    uint8_t data[4096];
} PACKED shipgate_error_pkt;

/* Error packet in reply to character data send or character request */
typedef struct shipgate_cdata_err {
    shipgate_error_pkt base;
    uint32_t guildcard;
    uint32_t slot;
} PACKED shipgate_cdata_err_pkt;

/* Error packet in reply to character backup send or character backup request */
typedef struct shipgate_cbkup_err {
    shipgate_error_pkt base;
    uint32_t guildcard;
    uint32_t block;
} PACKED shipgate_cbkup_err_pkt;

/* Error packet in reply to gm login */
typedef struct shipgate_gm_err {
    shipgate_error_pkt base;
    uint32_t guildcard;
    uint32_t block;
} PACKED shipgate_gm_err_pkt;

/* Error packet in reply to ban */
typedef struct shipgate_ban_err {
    shipgate_error_pkt base;
    uint32_t req_gc;
    uint32_t target;
    uint32_t until;
    uint32_t reserved;
} PACKED shipgate_ban_err_pkt;

/* Error packet in reply to a block login */
typedef struct shipgate_blogin_err {
    shipgate_error_pkt base;
    uint32_t guildcard;
    uint32_t blocknum;
} PACKED shipgate_blogin_err_pkt;

/* Error packet in reply to a add/remove friend */
typedef struct shipgate_friend_err {
    shipgate_error_pkt base;
    uint32_t user_gc;
    uint32_t friend_gc;
} PACKED shipgate_friend_err_pkt;

/* Error packet in reply to a schunk */
typedef struct shipgate_schunk_err {
    shipgate_error_pkt base;
    uint8_t type;
    uint8_t reserved[3];
    char filename[32];
} PACKED shipgate_schunk_err_pkt;

/* Error packet in reply to a quest flag packet (either get or set) */
typedef struct shipgate_qflag_err {
    shipgate_error_pkt base;
    uint32_t guildcard;
    uint32_t block;
    uint32_t flag_id;
    uint32_t quest_id;
} PACKED shipgate_qflag_err_pkt;

/* Error packet in reply to a ship control packet */
typedef struct shipgate_sctl_err {
    shipgate_error_pkt base;
    uint32_t ctl;
    uint32_t acc;
    uint32_t reserved1;
    uint32_t reserved2;
} PACKED shipgate_sctl_err_pkt;

/* Error packet used in response to various user commands */
typedef struct shipgate_user_err {
    shipgate_error_pkt base;
    uint32_t gc;
    uint32_t block;
    char message[0];
} PACKED shipgate_user_err_pkt;

/* The request sent from the shipgate for a ship to identify itself. */
typedef struct shipgate_login {
    shipgate_hdr_t hdr;
    char msg[45];
    uint8_t ver_major;
    uint8_t ver_minor;
    uint8_t ver_micro;
    uint8_t gate_nonce[4];
    uint8_t ship_nonce[4];
} PACKED shipgate_login_pkt;

/* The reply to the login request from the shipgate (with IPv6 support).
   Note that IPv4 support is still required, as PSO itself does not actually
   support IPv6 (however, proxies can alleviate this problem a bit). */
typedef struct shipgate_login6_reply {
    shipgate_hdr_t hdr;
    uint32_t proto_ver;
    uint32_t flags;
    uint8_t name[12];
    char ship_host4[32];
    char ship_host6[128];
    uint32_t ship_addr4;                /* IPv4 address (required) */
    uint8_t ship_addr6[16];             /* IPv6 address (optional) */
    uint16_t ship_port;
    uint16_t reserved1;
    uint16_t clients;
    uint16_t games;
    uint16_t menu_code;
    uint8_t reserved2[2];
    uint32_t privileges;
} PACKED shipgate_login6_reply_pkt;

/* A update of the client/games count. */
typedef struct shipgate_cnt {
    shipgate_hdr_t hdr;
    uint16_t clients;
    uint16_t games;
    uint32_t ship_id;                   /* 0 for ship->gate */
} PACKED shipgate_cnt_pkt;

/* A forwarded player packet. */
typedef struct shipgate_fw_9 {
    shipgate_hdr_t hdr;
    uint32_t ship_id;
    uint32_t fw_flags;
    uint32_t guildcard;
    uint32_t block;
    uint8_t pkt[0];
} PACKED shipgate_fw_9_pkt;

/* A packet telling clients that a ship has started or dropped. */
typedef struct shipgate_ship_status {
    shipgate_hdr_t hdr;
    char name[12];
    uint32_t ship_id;
    uint32_t ship_addr;
    uint32_t int_addr;                  /* reserved for compatibility */
    uint16_t ship_port;
    uint16_t status;
    uint32_t flags;
    uint16_t clients;
    uint16_t games;
    uint16_t menu_code;
    uint8_t  ship_number;
    uint8_t  reserved;
} PACKED shipgate_ship_status_pkt;

/* Updated version of the above packet, supporting IPv6. */
typedef struct shipgate_ship_status6 {
    shipgate_hdr_t hdr;
    uint8_t name[12];
    uint32_t ship_id;
    uint32_t flags;
    char ship_host4[32];
    char ship_host6[128];
    uint32_t ship_addr4;                /* IPv4 address (required) */
    uint8_t ship_addr6[16];             /* IPv6 address (optional) */
    uint16_t ship_port;
    uint16_t status;
    uint16_t clients;
    uint16_t games;
    uint16_t menu_code;
    uint8_t  ship_number;
    uint8_t  reserved;
    uint32_t privileges;
} PACKED shipgate_ship_status6_pkt;

/* A packet sent to/from clients to save/restore character data. */
typedef struct shipgate_char_data {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t slot;
    uint32_t block;
    uint8_t data[];
} PACKED shipgate_char_data_pkt;

/* A packet sent to/from clients to save/restore common bank data. */
typedef struct shipgate_common_bank_data {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
    uint32_t common_bank_req;
    psocn_bank_t data;
} PACKED shipgate_common_bank_data_pkt;

typedef struct sg_char_bkup {
    uint32_t guildcard;
    uint8_t slot;
    uint32_t block;
    uint32_t c_version;
    uint8_t name[32];
} PACKED sg_char_bkup_pkt;

/* A packet sent from clients to save their character backup or to request that
   the gate send it back to them. */
typedef struct shipgate_char_bkup {
    shipgate_hdr_t hdr;
    sg_char_bkup_pkt game_info;
    //uint32_t guildcard;
    //uint8_t slot;
    //uint32_t block;
    //uint32_t c_version;
    //uint8_t name[32];
    uint8_t data[];
} PACKED shipgate_char_bkup_pkt;

/* A packet sent to request saved character data. */
typedef struct shipgate_char_req {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t slot;
} PACKED shipgate_char_req_pkt;

/* A packet sent to login a user. */
typedef struct shipgate_usrlogin_req {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
    char username[32];
    char password[32];
} PACKED shipgate_usrlogin_req_pkt;

/* A packet replying to a user login. */
typedef struct shipgate_usrlogin_reply {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
    uint32_t priv;
    uint8_t reserved[4];
} PACKED shipgate_usrlogin_reply_pkt;

/* A packet used to set a ban. */
typedef struct shipgate_ban_req {
    shipgate_hdr_t hdr;
    uint32_t req_gc;
    uint32_t target;
    uint32_t until;
    uint32_t reserved;
    char message[256];
} PACKED shipgate_ban_req_pkt;

/* Packet used to tell the shipgate that a user has logged into/off a block */
typedef struct shipgate_block_login {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t slot;
    uint32_t blocknum;
    char ch_name[32];
    uint8_t version;
} PACKED shipgate_block_login_pkt;

/* Packet to tell a ship that a client's friend has logged in/out */
typedef struct shipgate_friend_login {
    shipgate_hdr_t hdr;
    uint32_t dest_guildcard;
    uint32_t dest_block;
    uint32_t friend_guildcard;
    uint32_t friend_ship;
    uint32_t friend_block;
    uint32_t reserved;
    char friend_name[32];
    char friend_nick[32];
} PACKED shipgate_friend_login_pkt;

/* Updated version of above packet for protocol version 4 */
typedef struct shipgate_friend_login_4 {
    shipgate_hdr_t hdr;
    uint32_t dest_guildcard;
    uint32_t dest_block;
    uint32_t friend_guildcard;
    uint32_t friend_ship;
    uint32_t friend_block;
    uint32_t reserved;
    char friend_name[32];
    char friend_nick[32];
} PACKED shipgate_friend_login_4_pkt;

/* Packet to update a user's friendlist (used for either add or remove) */
typedef struct shipgate_friend_upd {
    shipgate_hdr_t hdr;
    uint32_t user_guildcard;
    uint32_t friend_guildcard;
} PACKED shipgate_friend_upd_pkt;

/* Packet to add a user to a friendlist (updated in protocol version 4) */
typedef struct shipgate_friend_add {
    shipgate_hdr_t hdr;
    uint32_t user_guildcard;
    uint32_t friend_guildcard;
    char friend_nick[32];
} PACKED shipgate_friend_add_pkt;

/* Packet to update a user's lobby in the shipgate's info */
typedef struct shipgate_lobby_change {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t lobby_id;
    char lobby_name[32];
} PACKED shipgate_lobby_change_pkt;

/* Packet to send a list of online clients (for when a ship reconnects to the
   shipgate) */
typedef struct shipgate_block_clients {
    shipgate_hdr_t hdr;
    uint32_t count;
    uint32_t block;
    struct {
        uint32_t guildcard;
        uint32_t lobby;
        uint32_t dlobby;
        uint32_t reserved;
        char ch_name[32];
        char lobby_name[32];
    } entries[0];
} PACKED shipgate_block_clients_pkt;

/* A kick request, sent to or from a ship */
typedef struct shipgate_kick_req {
    shipgate_hdr_t hdr;
    uint32_t requester;
    uint32_t reserved;
    uint32_t guildcard;
    uint32_t block;                     /* 0 for ship->shipgate */
    char reason[64];
} PACKED shipgate_kick_pkt;

/* This is used for storing the friendlist data for a friend list request. */
typedef struct friendlist_data {
    uint32_t guildcard;
    uint32_t ship;
    uint32_t block;
    uint32_t reserved;
    char name[32];
} PACKED friendlist_data_t;

/* Packet to send a portion of the user's friend list to the ship, including
   online/offline status. */
typedef struct shipgate_friend_list {
    shipgate_hdr_t hdr;
    uint32_t requester;
    uint32_t block;
    friendlist_data_t entries[];
} PACKED shipgate_friend_list_pkt;

/* Packet to request a portion of the friend list be sent */
typedef struct shipgate_friend_list_req {
    shipgate_hdr_t hdr;
    uint32_t requester;
    uint32_t block;
    uint32_t start;
    uint32_t reserved;
} PACKED shipgate_friend_list_req;

/* Packet to send a global message to all ships */
typedef struct shipgate_global_msg {
    shipgate_hdr_t hdr;
    uint32_t requester;
    uint32_t reserved;
    char text[];                        /* UTF-8, padded to 8-byte boundary */
} PACKED shipgate_global_msg_pkt;

/* An individual option for the options packet */
typedef struct shipgate_user_opt {
    uint32_t option;
    uint32_t length;
    uint8_t data[512];
} PACKED shipgate_user_opt_t;

/* Packet used to send a user's settings to a ship */
typedef struct shipgate_user_options {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
    uint32_t count;
    uint32_t reserved;
    shipgate_user_opt_t options[];
} PACKED shipgate_user_opt_pkt;

/* 请求 Blue Burst 选项数据包 */
typedef struct shipgate_bb_opts_req {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
} PACKED shipgate_bb_opts_req_pkt;

/* 发送 Blue Burst 选项数据包至客户端 */
typedef struct shipgate_bb_opts {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
    psocn_bb_db_opts_t opts;
    psocn_bb_db_guild_t guild;
    uint32_t guild_points_personal_donation;
    psocn_bank_t common_bank;
} PACKED shipgate_bb_opts_pkt;

/* Packet used to send an update to the user's monster kill counts.
   Version 1 adds a client version code where there used to be a reserved byte
   in the packet. */
typedef struct shipgate_mkill {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
    uint8_t episode;
    uint8_t difficulty;
    uint8_t version;
    uint8_t reserved;
    uint32_t counts[0x60];
} PACKED shipgate_mkill_pkt;

/* Packet used to send a script chunk to a ship. */
typedef struct shipgate_schunk {
    shipgate_hdr_t hdr;
    uint8_t chunk_type;
    uint8_t reserved[3];
    uint32_t chunk_length;
    uint32_t chunk_crc;
    uint32_t action;
    char filename[32];
    uint8_t chunk[];
} PACKED shipgate_schunk_pkt;

/* Packet used to communicate with a script running on the shipgate during a
   scripted event. */
typedef struct shipgate_sdata {
    shipgate_hdr_t hdr;
    uint32_t event_id;
    uint32_t data_len;
    uint32_t guildcard;
    uint32_t block;
    uint8_t episode;
    uint8_t difficulty;
    uint8_t version;
    uint8_t reserved;
    uint8_t data[];
} PACKED shipgate_sdata_pkt;

/* Packet used to set a script to respond to an event. */
typedef struct shipgate_sset {
    shipgate_hdr_t hdr;
    uint32_t action;
    uint32_t reserved;
    char filename[32];
} PACKED shipgate_sset_pkt;

/* Packet used to set a quest flag or to read one back. */
typedef struct shipgate_qflag {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
    uint32_t flag_id;
    uint32_t quest_id;
    uint16_t flag_id_hi;
    uint16_t reserved;
    uint32_t value;
} PACKED shipgate_qflag_pkt;

/* Packet used for ship control. */
typedef struct shipgate_shipctl {
    shipgate_hdr_t hdr;
    uint32_t ctl;
    uint32_t acc;
    uint32_t reserved1;
    uint32_t reserved2;
    uint8_t data[];
} PACKED shipgate_shipctl_pkt;

/* Packet used to shutdown or restart a ship. */
typedef struct shipgate_sctl_shutdown {
    shipgate_hdr_t hdr;
    uint32_t ctl;
    uint32_t acc;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t when;
    uint32_t reserved3;
} PACKED shipgate_sctl_shutdown_pkt;

/* Packet used to communicate system information from a ship. */
typedef struct shipgate_sctl_uname_reply {
    shipgate_hdr_t hdr;
    uint32_t ctl;
    uint32_t unused;
    uint32_t reserved1;
    uint32_t reserved2;
    uint8_t name[64];
    uint8_t node[64];
    uint8_t release[64];
    uint8_t version[64];
    uint8_t machine[64];
} PACKED shipgate_sctl_uname_reply_pkt;

/* Packet used to communicate fine-grained version information from a ship. */
typedef struct shipgate_sctl_ver_reply {
    shipgate_hdr_t hdr;
    uint32_t ctl;
    uint32_t unused;
    uint32_t reserved1;
    uint32_t reserved2;
    uint8_t ver_major;
    uint8_t ver_minor;
    uint8_t ver_micro;
    uint8_t flags;
    uint8_t commithash[20];
    uint64_t committime;
    uint8_t remoteref[];
} PACKED shipgate_sctl_ver_reply_pkt;

/* Packet used to send a user's blocklist to the ship */
typedef struct shipgate_user_blocklist {
    shipgate_hdr_t hdr;
    uint32_t guildcard;
    uint32_t block;
    uint32_t count;
    uint32_t reserved;
    struct {
        uint32_t gc;
        uint32_t flags;
    } entries[];
} PACKED shipgate_user_blocklist_pkt;

/* Packet used to add a player to a user's blocklist */
typedef struct shipgate_ubl_add {
    shipgate_hdr_t hdr;
    uint32_t requester;
    uint32_t block;
    uint32_t blocked_player;
    uint32_t flags;
    uint8_t blocked_name[32];
    uint8_t blocked_class;
    uint8_t reserved[7];
} PACKED shipgate_ubl_add_pkt;

/* 用于传输玩家职业最大法术数据表 */
typedef struct shipgate_max_tech_lvl_bb {
    shipgate_hdr_t hdr;
    bb_max_tech_level_t data[MAX_PLAYER_TECHNIQUES];
} PACKED shipgate_max_tech_lvl_bb_pkt;

/* 用于传输玩家职业等级数值数据表 */
typedef struct shipgate_pl_level_bb {
    shipgate_hdr_t hdr;
    bb_level_table_t data;
} PACKED shipgate_pl_level_bb_pkt;

/* 用于传输玩家职业等级数值数据表 */
typedef struct shipgate_default_mode_char_data_bb {
    shipgate_hdr_t hdr;
    uint32_t compressed_size;
    uint8_t data[];
} PACKED shipgate_default_mode_char_data_bb_pkt;

/* 用于查询玩家是否在线 */
typedef struct shipgate_check_plonline {
    shipgate_hdr_t hdr;
    uint32_t req_gc;
    uint32_t target_gc;
    uint32_t block;
    uint32_t flags;
} PACKED shipgate_check_plonline_pkt;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

#include "sg_packetlist.h"

#endif /* !SHIPGATE_PACKETS */