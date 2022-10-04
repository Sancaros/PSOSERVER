/*
    梦幻之星中国 静态库
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


#ifndef PACKETS_H_HAVE_HEADERS
#define PACKETS_H_HAVE_HEADERS

/* Always define the headers of packets, if they haven't already been taken
   care of. */

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
#define LE16(x) (((x >> 8) & 0xFF) | ((x & 0xFF) << 8))
#define LE32(x) (((x >> 24) & 0x00FF) | \
                 ((x >>  8) & 0xFF00) | \
                 ((x & 0xFF00) <<  8) | \
                 ((x & 0x00FF) << 24))
#define LE64(x) (((x >> 56) & 0x000000FF) | \
                 ((x >> 40) & 0x0000FF00) | \
                 ((x >> 24) & 0x00FF0000) | \
                 ((x >>  8) & 0xFF000000) | \
                 ((x & 0xFF000000) <<  8) | \
                 ((x & 0x00FF0000) << 24) | \
                 ((x & 0x0000FF00) << 40) | \
                 ((x & 0x000000FF) << 56))
#define BE64(x) x
#else
#define LE16(x) x
#define LE32(x) x
#define LE64(x) x
#define BE64(x) (((x >> 56) & 0x000000FF) | \
                 ((x >> 40) & 0x0000FF00) | \
                 ((x >> 24) & 0x00FF0000) | \
                 ((x >>  8) & 0xFF000000) | \
                 ((x & 0xFF000000) <<  8) | \
                 ((x & 0x00FF0000) << 24) | \
                 ((x & 0x0000FF00) << 40) | \
                 ((x & 0x000000FF) << 56))
#endif

#define SWAP32(x) (((x >> 24) & 0x00FF) | \
                   ((x >>  8) & 0xFF00) | \
                   ((x & 0xFF00) <<  8) | \
                   ((x & 0x00FF) << 24))

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

/* DC V3 GC XBOX客户端数据头 */
typedef struct dc_pkt_hdr {
    uint8_t pkt_type;
    uint8_t flags;
    uint16_t pkt_len;
} PACKED dc_pkt_hdr_t;

/* PC 客户端数据头 */
typedef struct pc_pkt_hdr {
    uint16_t pkt_len;
    uint8_t pkt_type;
    uint8_t flags;
} PACKED pc_pkt_hdr_t;

/* BB 客户端数据头 */
typedef struct bb_pkt_hdr {
    uint16_t pkt_len;
    uint16_t pkt_type;
    uint32_t flags;
} PACKED bb_pkt_hdr_t;

/* 客户端数据头联合结构 */
typedef union pkt_header {
    dc_pkt_hdr_t dc;
    dc_pkt_hdr_t gc;
    dc_pkt_hdr_t xb;
    pc_pkt_hdr_t pc;
    bb_pkt_hdr_t bb;
} pkt_header_t;

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

typedef struct bb_security {
    bb_pkt_hdr_t hdr;
    uint32_t err_code;
    uint32_t tag;
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

/* The welcome packet for setting up encryption keys */
typedef struct dc_welcome {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char copyright[0x40];
    uint32_t svect;
    uint32_t cvect;
    // As in 02, this field is not part of SEGA's implementation.
    char after_message[0xC0];
} PACKED dc_welcome_pkt;

typedef struct bb_welcome {
    bb_pkt_hdr_t hdr;
    char copyright[0x60];
    uint8_t svect[0x30];
    uint8_t cvect[0x30];
    // As in 02, this field is not part of SEGA's implementation.
    char after_message[0xC0];
} PACKED bb_welcome_pkt;

typedef struct bb_burst {
    bb_pkt_hdr_t hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED bb_burst_pkt;

typedef struct bb_done_burst {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
        bb_pkt_hdr_t bb;
    };
    uint8_t data[];
} PACKED bb_done_burst_pkt;

/* The menu selection packet that the client sends to us */
typedef struct dc_select {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED dc_select_pkt;

typedef struct bb_select {
    bb_pkt_hdr_t hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED bb_select_pkt;

/* Various login packets */
typedef struct dc_login_90 {
    dc_pkt_hdr_t hdr;
    char serial[8];
    uint8_t padding1[9];
    char access_key[8];
    uint8_t padding2[11];
} PACKED dc_login_90_pkt;

typedef struct dc_login_92_93 {
    dc_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t guildcard;
    uint32_t unk[3];
    uint8_t unused1;
    uint8_t language_code;
    uint16_t unused2;
    char serial[8];
    uint8_t padding1[9];
    char access_key[8];
    uint8_t padding2[9];
    char dc_id[8];
    uint8_t padding3[88];
    char name[16];
    uint8_t padding4[2];
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

typedef struct dcv2_login_9a {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint8_t unused[32];
    char serial[8];
    uint8_t padding1[8];
    char access_key[8];
    uint8_t padding2[10];
    uint8_t unk[7];
    uint8_t padding3[3];
    char dc_id[8];
    uint8_t padding4[88];
    uint8_t email[32];
    uint8_t padding5[16];
} PACKED dcv2_login_9a_pkt;

typedef struct gc_login_9c {
    dc_pkt_hdr_t hdr;
    uint8_t padding1[8];
    uint8_t version;
    uint8_t padding2[4];
    uint8_t language_code;
    uint8_t padding3[2];
    char serial[8];
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

typedef struct dcv2_login_9d {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t tag;
    uint32_t guildcard;
    uint8_t padding1[8];
    uint8_t version;
    uint8_t padding2[4];
    uint8_t language_code;
    uint8_t padding3[2];
    char v1_serial[8];
    uint8_t padding4[8];
    char v1_access_key[8];
    char padding5[8];
    char serial[8];
    uint8_t padding6[8];
    char access_key[8];
    uint8_t padding7[8];
    char dc_id[8];
    uint8_t padding8[88];
    uint16_t unk2;
    uint8_t padding9[14];
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

typedef struct gc_login_9e {
    dc_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t guildcard;
    uint8_t padding1[8];
    uint8_t version;
    uint8_t padding2[4];
    uint8_t language_code;
    uint8_t padding3[34];
    char serial[8];
    uint8_t padding4[8];
    char access_key[12];
    uint8_t padding5[4];
    char serial2[8];
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
    uint32_t tag;
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

typedef struct bb_login_93 {
    bb_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t guildcard;
    uint16_t version;
    uint8_t unused[6];
    uint32_t guild_id;
    char username[48];
    //uint8_t unused1[0x20];
    char password[48];
    //uint8_t unused2[8];
    //uint8_t unused2[0x28];
    uint32_t menu_id;
    uint32_t preferred_lobby_id;
    //uint32_t hwinfo[2];
    //uint8_t hwinfo[0x08];
    union VariableLengthSection {
        union ClientConfigFields {
            bb_client_config_pkt cfg;
            char version_string[40];
            uint32_t as_u32[10];
        };

        union ClientConfigFields old_clients;

        struct NewFormat {
            uint32_t hwinfo[2];
            union ClientConfigFields cfg;
        } new_clients;
    } var;
} PACKED bb_login_93_pkt;
//
//typedef struct bb_login_93 {
//    bb_pkt_hdr_t hdr;
//    uint32_t tag;
//    uint32_t guildcard;
//    uint16_t version;
//    uint8_t unk2[6];
//    uint32_t guild_id;
//    char username[16];
//    uint8_t unused1[32];
//    char password[16];
//    uint8_t unused2[40];
//    uint8_t hwinfo[8];
//    uint8_t security_data[40];
//} PACKED bb_login_93_pkt;

/* The first packet sent by the Dreamcast Network Trial Edition. Note that the
   serial number and access key are both 16-character strings with NUL
   terminators at the end. */
typedef struct dcnte_login_88 {
    dc_pkt_hdr_t hdr;
    char serial[16];
    uint8_t nul1;
    char access_key[16];
    uint8_t nul2;
} PACKED dcnte_login_88_pkt;

typedef struct dcnte_login_8a {
    dc_pkt_hdr_t hdr;
    char dc_id[8];
    uint8_t unk[8];
    char username[48];
    char password[48];
    char email[48];
} PACKED dcnte_login_8a_pkt;

/* This one is the "real" login packet from the DC NTE. If the client is still
   connected to the login server, you get more unused space at the end for some
   reason, but otherwise the packet is exactly the same between the ship and the
   login server. */
typedef struct dcnte_login_8b {
    dc_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t guildcard;
    uint8_t dc_id[8];
    uint8_t unk[8];
    char serial[16];
    uint8_t nul1;
    char access_key[16];
    uint8_t nul2;
    char username[48];
    char password[48];
    char char_name[16];
    uint8_t unused[2];
} PACKED dcnte_login_8b_pkt;

/* The packet to verify that a hunter's license has been procured. */
typedef struct login_gc_hlcheck {
    dc_pkt_hdr_t hdr;
    uint8_t padding1[32];
    char serial[8];
    uint8_t padding2[8];
    char access_key[12];
    uint8_t padding3[12];
    uint8_t version;
    uint8_t padding4[3];
    char serial2[8];
    uint8_t padding5[40];
    char access_key2[12];
    uint8_t padding6[36];
    char password[16];
    uint8_t padding7[32];
} PACKED gc_hlcheck_pkt;

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

/* The packet sent as a timestamp */
typedef struct dc_timestamp {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char timestamp[28];
} PACKED dc_timestamp_pkt;

typedef struct bb_timestamp {
    bb_pkt_hdr_t hdr;
    char timestamp[28];
} PACKED bb_timestamp_pkt;

/* The packet used for the information reply */
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
    uint16_t msg[];
} PACKED bb_info_reply_pkt;

typedef struct bb_info_reply_pkt bb_scroll_msg_pkt;

/* The ship list packet send to tell clients what blocks are up */
typedef struct dc_block_list {
    dc_pkt_hdr_t hdr;           /* The flags field says the entry count */
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;
        char name[0x12];
    } entries[0];
} PACKED dc_block_list_pkt;

typedef struct pc_block_list {
    pc_pkt_hdr_t hdr;           /* The flags field says the entry count */
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;
        uint16_t name[0x11];
    } entries[0];
} PACKED pc_block_list_pkt;

typedef struct bb_block_list {
    bb_pkt_hdr_t hdr;           /* The flags field says the entry count */
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;
        uint16_t name[0x11];
    } entries[0];
} PACKED bb_block_list_pkt;

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

#ifdef PLAYER_H

/* The packet sent by clients to send their character data */
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

/* The packet sent to clients when they join a lobby */
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

typedef struct dc_lobby_join {
    dc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1 */
    uint8_t lobby_num;
    uint16_t block_num;
    uint16_t event;
    uint32_t padding;
    struct {
        dc_player_hdr_t hdr;
        v1_player_t data;
    } entries[0];
} PACKED dc_lobby_join_pkt;

typedef struct pc_lobby_join {
    pc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1 */
    uint8_t lobby_num;
    uint16_t block_num;
    uint16_t event;
    uint32_t padding;
    struct {
        pc_player_hdr_t hdr;
        v1_player_t data;
    } entries[0];
} PACKED pc_lobby_join_pkt;

typedef struct xb_lobby_join {
    dc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1 */
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
        inventory_t inv;
        psocn_bb_char_t data;
    } entries[0];
} PACKED bb_lobby_join_pkt;

#endif

/* The packet sent to clients when someone leaves a lobby */
typedef struct dc_lobby_leave {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint16_t padding;
} PACKED dc_lobby_leave_pkt;

typedef struct bb_lobby_leave {
    bb_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint16_t padding;
} PACKED bb_lobby_leave_pkt;

/* The packet sent from/to clients for sending a normal chat */
typedef struct dc_chat {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t padding;
    uint32_t guildcard;
    char msg[0];
} PACKED dc_chat_pkt;

typedef struct bb_chat {
    bb_pkt_hdr_t hdr;
    uint32_t padding;
    uint32_t guildcard;
    uint16_t msg[];
} PACKED bb_chat_pkt;

/* The packet sent to search for a player */
typedef struct dc_guild_search {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t tag;
    uint32_t gc_search;
    uint32_t gc_target;
} PACKED dc_guild_search_pkt;

typedef struct bb_guild_search {
    bb_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t gc_search;
    uint32_t gc_target;
} PACKED bb_guild_search_pkt;

/* The packet sent to reply to a guild card search */
typedef struct dc_guild_reply {
    dc_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t gc_search;
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
    uint32_t tag;
    uint32_t gc_search;
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
    uint32_t tag;
    uint32_t gc_search;
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
    uint32_t tag;
    uint32_t gc_search;
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
    uint32_t tag;
    uint32_t gc_search;
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
    uint32_t tag;
    uint32_t gc_search;
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

/* The packet sent to send/deliver simple mail */
typedef struct dc_simple_mail {
    dc_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t gc_sender;
    char name[16];
    uint32_t gc_dest;
    char stuff[0x200]; /* Start = 0x20, end = 0xB0 */
} PACKED dc_simple_mail_pkt;

typedef struct pc_simple_mail {
    pc_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t gc_sender;
    uint16_t name[16];
    uint32_t gc_dest;
    char stuff[0x400]; /* Start = 0x30, end = 0x150 */
} PACKED pc_simple_mail_pkt;

typedef struct bb_simple_mail {
    bb_pkt_hdr_t hdr;
    uint32_t tag;
    uint32_t gc_sender;
    uint16_t name[16];
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
    uint32_t tag;
    uint32_t gc_sender;
    union mail_data {
        dc_mail_data_pkt dcmaildata;
        pc_mail_data_pkt pcmaildata;
        bb_mail_data_pkt bbmaildata;
    };
} PACKED simple_mail_pkt;

/* The packet sent by clients to create a game */
typedef struct dcnte_game_create {
    dc_pkt_hdr_t hdr;
    uint32_t unused[2];
    char name[16];
    char password[16];
} PACKED dcnte_game_create_pkt;

typedef struct dc_game_create {
    dc_pkt_hdr_t hdr;
    uint32_t unused[2];
    char name[16];
    char password[16];
    uint8_t difficulty;
    uint8_t battle;
    uint8_t challenge;
    uint8_t version;                    /* Set to 1 for v2 games, 0 otherwise */
} PACKED dc_game_create_pkt;

typedef struct pc_game_create {
    pc_pkt_hdr_t hdr;
    uint32_t unused[2];
    uint16_t name[16];
    uint16_t password[16];
    uint8_t difficulty;
    uint8_t battle;
    uint8_t challenge;
    uint8_t padding;
} PACKED pc_game_create_pkt;

typedef struct gc_game_create {
    dc_pkt_hdr_t hdr;
    uint32_t unused[2];
    char name[16];
    char password[16];
    uint8_t difficulty;
    uint8_t battle;
    uint8_t challenge;
    uint8_t episode;
} PACKED gc_game_create_pkt;

typedef struct bb_game_create {
    bb_pkt_hdr_t hdr;
    uint32_t unused[2];
    uint16_t name[16];
    uint16_t password[16];
    uint8_t difficulty;
    uint8_t battle;
    uint8_t challenge;
    uint8_t episode;
    uint8_t single_player;
    uint8_t padding[3];
} PACKED bb_game_create_pkt;

#ifdef PLAYER_H

/* The packet sent to clients to join a game */
typedef struct dcnte_game_join {
    dc_pkt_hdr_t hdr;
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;
    uint8_t unused;
    uint32_t maps[0x20];
    dc_player_hdr_t players[4];
} PACKED dcnte_game_join_pkt;

typedef struct dc_game_join {
    dc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    dc_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1. */
    uint8_t difficulty;
    uint8_t battle;
    uint8_t event;
    uint8_t section;
    uint8_t challenge;
    uint32_t rand_seed;
} PACKED dc_game_join_pkt;

typedef struct pc_game_join {
    pc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    pc_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1. */
    uint8_t difficulty;
    uint8_t battle;
    uint8_t event;
    uint8_t section;
    uint8_t challenge;
    uint32_t rand_seed;
} PACKED pc_game_join_pkt;

typedef struct gc_game_join {
    dc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    dc_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1. */
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

typedef struct xb_game_join {
    dc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    xbox_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1. */
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

typedef struct ep3_game_join {
    dc_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    dc_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1. */
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

typedef struct bb_game_join {
    bb_pkt_hdr_t hdr;
    uint32_t maps[0x20];
    bb_player_hdr_t players[4];
    uint8_t client_id;
    uint8_t leader_id;
    uint8_t one;                        /* Always 1. */
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

#endif

/* The packet sent to clients to give them the game select list */
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

typedef struct bb_game_list {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint8_t difficulty;
        uint8_t players;
        uint16_t name[16];
        uint8_t episode;
        uint8_t flags;
    } entries[0];
} PACKED bb_game_list_pkt;

/* The packet sent to display a large message to the user */
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
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
        bb_pkt_hdr_t bb;
    } hdr;
    char msg[];
} PACKED msg_box_pkt;

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

typedef struct bb_quest_list {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t name[32];
        uint16_t desc[122];
    } entries[0];
} PACKED bb_quest_list_pkt;

/* The packet sent to inform a client of a quest file that will be coming down
   the pipe */
typedef struct dc_quest_file {
    dc_pkt_hdr_t hdr;
    char name[32];
    uint8_t unused1[3];
    char filename[16];
    uint8_t unused2;
    uint32_t length;
} PACKED dc_quest_file_pkt;

typedef struct pc_quest_file {
    pc_pkt_hdr_t hdr;
    char name[32];
    uint16_t unused;
    uint16_t flags;
    char filename[16];
    uint32_t length;
} PACKED pc_quest_file_pkt;

typedef struct gc_quest_file {
    dc_pkt_hdr_t hdr;
    char name[32];
    uint16_t unused;
    uint16_t flags;
    char filename[16];
    uint32_t length;
} PACKED gc_quest_file_pkt;

typedef struct bb_quest_file {
    bb_pkt_hdr_t hdr;
    char unused1[32];
    uint16_t unused2;
    uint16_t flags;
    char filename[16];
    uint32_t length;
    char name[24];
} PACKED bb_quest_file_pkt;

/* The packet sent to actually send quest data */
typedef struct dc_quest_chunk {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    char filename[16];
    char data[1024];
    uint32_t length;
} PACKED dc_quest_chunk_pkt;

typedef struct bb_quest_chunk {
    bb_pkt_hdr_t hdr;
    char filename[16];
    char data[1024];
    uint32_t length;
} PACKED bb_quest_chunk_pkt;

/* The packet sent to update the list of arrows in a lobby */
typedef struct dc_arrow_list {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    struct {
        uint32_t tag;
        uint32_t guildcard;
        uint32_t arrow;
    } entries[0];
} PACKED dc_arrow_list_pkt;

typedef struct bb_arrow_list {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t tag;
        uint32_t guildcard;
        uint32_t arrow;
    } entries[0];
} PACKED bb_arrow_list_pkt;

/* The ship list packet sent to tell clients what ships are up */
typedef struct dc_ship_list {
    dc_pkt_hdr_t hdr;           /* The flags field says how many entries */
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;
        char name[0x12];
    } entries[0];
} PACKED dc_ship_list_pkt;

typedef struct pc_ship_list {
    pc_pkt_hdr_t hdr;           /* The flags field says how many entries */
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;
        uint16_t name[0x11];
    } entries[0];
} PACKED pc_ship_list_pkt;

typedef struct bb_ship_list {
    bb_pkt_hdr_t hdr;           /* The flags field says how many entries */
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;
        uint16_t name[0x11];
    } entries[0];
} PACKED bb_ship_list_pkt;

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

/* The packet used to ask for a GBA file */
typedef struct gc_gba_req {
    dc_pkt_hdr_t hdr;
    char filename[16];
} PACKED gc_gba_req_pkt;

/* The packet sent to clients to read the info board */
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

// D0 (C->S): Start trade sequence (V3/BB)
// The trade window sequence is a bit complicated. The normal flow is:
// - Clients sync trade state with 60xA6 commands (technically 62xA6)
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
//    le_uint16_t target_client_id;
//    le_uint16_t item_count;
//    // Note: PSO GC sends uninitialized data in the unused entries of this
//    // command. newserv parses and regenerates the item data when sending D3,
//    // which effectively erases the uninitialized data.
//    ItemData items[0x20];
//};
//[2022年08月24日 10:24 : 40 : 052] 调试(2111) : 舰仓：BB处理数据 指令 0x00D0 TRADE_0_TYPE 长度 652 字节 标志 0 GC 42004063
//(00000000)   8C 02 D0 00 00 00 00 00  00 00 01 00 00 06 00 00    ................
//(00000010)   00 00 00 00 00 00 00 00  00 00 21 00 00 00 00 00    ..........!.....
//(00000020)   00 00 00 00 00 00 00 00  00 00 9E 42 00 00 00 00    ...........B....
//(00000030)   00 00 00 00 00 00 00 00  00 00 20 44 00 00 F0 43    ..........D...C
//(00000040)   24 7D 2B 04 00 00 00 00  52 0E E2 60 3C 77 80 06    $} + .....R..`<w..
//(00000050)   10 C6 8E 23 00 00 E6 43  00 00 9E 42 00 00 00 00    ...#...C...B....
//(00000060)   00 00 E6 43 01 00 00 00  00 00 00 00 9C D7 41 7B    ...C..........A{
//(00000070)   FB CE 09 00 20 B1 7A 06  F0 04 E2 60 6A 05 E2 60    ..   ..z....`j..`
//(00000080)   40 BF AC 00 00 FE 7A 06  71 3D 0A 3F E1 7A 54 3F    @.....z.q = . ? .zT ?
//(00000090)   98 C0 8D 06 00 60 FE 7F  F0 C1 AF 07 00 00 7C 42    .....`........ | B
//(000000A0)   00 00 A2 42 00 00 7C 42  50 A2 23 04 00 00 00 00    ...B.. | BP.#.....
//(000000B0)   00 4D 8F 79 68 FA E8 03  96 1F E3 60 00 A8 EC 00.M.yh......`....
//(000000C0)   5C FA E8 03 20 1F E3 60  20 B1 7A 06 00 00 80 3F    \... ..`.z.... ?
//(000000D0)   29 42 A4 46 8F 01 80 BF  80 FA E8 03 BA 04 E2 60    )B.F...........`
//(000000E0)   40 BF AC 00 24 7D 2B 04  70 03 E2 60 38 FF E8 03    @...$ } + .p..`8...
//(000000F0)   C4 FA E8 03 25 03 E2 60  20 7D 2B 04 03 00 00 00    .... % ..` } + .....
//(00000100)   40 BF AC 00 9C 30 B7 32  00 00 80 3F 00 10 A4 46    @....0.2... ? ...F
//(00000110)   38 FF E8 03 24 7D 2B 04  00 00 00 00 74 07 00 00    8...$} + .....t...
//(00000120)   24 7D 2B 04 94 FA E8 03  D8 FC E8 03 28 28 E4 60    $} + .........((.`
//(00000130)   FF FF FF FF 21 5C 18 00  D6 0B 00 00 6C 73 35 4B    ....!\......ls5K
//(00000140)   BE EB 12 00 0C FB E8 03  92 59 02 00 21 E4 3B 02    .........Y..!.; .
//(00000150)   20 FB E8 03 01 00 00 00  0C FB E8 03 3C 00 00 00     ........... < ...
//(00000160)   A3 B7 D8 01 86 EE 8E B7  00 00 00 00 21 E4 3B 02    ............!.; .
//(00000170)   20 FB E8 03 07 00 00 00  A5 01 00 00 54 FB E8 03     ...........T...
//(00000180)   C6 CD B2 77 B0 FB E8 03  F4 FB E8 03 18 FC E8 03    ...w............
//(00000190)   E6 07 08 00 18 00 0A 00  18 00 27 00 C9 01 03 00    ..........'.....
//(000001A0)   86 EE 8E B7 A3 B7 D8 01  00 00 00 00 00 00 00 00    ................
//(000001B0)   00 00 00 00 BC FF FF FF  00 C0 DC F1 00 00 00 00    ................
//(000001C0)   00 00 00 00 94 FB E8 03  B7 FA 43 76 89 AB B2 77    ..........Cv...w
//(000001D0)   7C FB E8 03 7C FB E8 03  4B 43 43 76 0B 00 00 00 | ... | ...KCCv....
//(000001E0)   78 FB E8 03 70 4B F7 10  A8 13 0E 03 AC FB E8 03    x...pK..........
//(000001F0)   EF C5 85 00 C8 6B 71 00  55 79 71 00 40 BF F6 21    .....kq.Uyq.@..!
//(00000200)   3C 00 00 00 D8 FB E8 03  C0 FB E8 03 38 FF E8 03 < ...........8...
//(00000210)   B7 A2 85 00 B4 FB E8 03  F0 7F A6 01 C4 FB E8 03    ................
//(00000220)   C6 B4 85 00 09 00 00 00  AC C5 F4 21 C0 C4 F4 21    .... ......!...!
//(00000230)   E7 8B 05 63 00 00 00 00  F4 B4 85 00 01 00 00 00    ...c............
//(00000240)   A2 AD 83 00 D8 FB E8 03  27 00 00 00 18 00 00 00    ........'.......
//(00000250)   0A 00 00 00 18 00 00 00  09 00 00 00 7A 00 00 00    ........     ...z...
//(00000260)   A0 BE F6 21 40 BF F6 21  40 BF F6 21 A0 BE F6 21    ...!@..!@..!...!
//(00000270)   38 71 71 00 A0 BE F6 21  90 FB E8 03 D8 FC E8 03    8qq....!........
//(00000280)   5A 09 8F 00 FF FF FF FF  00 00 00 00                Z    ..........
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

/* The packet used to send C-Rank Data */
typedef struct dc_c_rank_update {
    dc_pkt_hdr_t hdr;
    struct {
        uint32_t client_id;
        union {
            uint8_t c_rank[0xB8];
            struct {
                uint32_t unk1;
                char string[0x0C];
                uint8_t unk2[0x24];
                uint16_t grave_unk4;
                uint16_t grave_deaths;
                uint32_t grave_coords_time[5];
                char grave_team[20];
                char grave_message[24];
                uint32_t times[9];
                uint32_t battle[7];
            };
        };
    } entries[0];
} PACKED dc_c_rank_update_pkt;

typedef struct pc_c_rank_update {
    pc_pkt_hdr_t hdr;
    struct {
        uint32_t client_id;
        union {
            uint8_t c_rank[0xF0];
            struct {
                uint32_t unk1;
                uint16_t string[0x0C];
                uint8_t unk2[0x24];
                uint16_t grave_unk4;
                uint16_t grave_deaths;
                uint32_t grave_coords_time[5];
                uint16_t grave_team[20];
                uint16_t grave_message[24];
                uint32_t times[9];
                uint32_t battle[7];
            };
        };
    } entries[0];
} PACKED pc_c_rank_update_pkt;

// 288
typedef struct gc_c_rank_update {
    dc_pkt_hdr_t hdr; // 4
    struct {
        uint32_t client_id; // 4
        union {
            uint8_t c_rank[0x0118]; // 280
            struct {
                uint32_t unk1;          /* Flip the words for dc/pc! */
                uint32_t times[9];
                uint32_t times_ep2[5];
                uint8_t unk2[0x24];     /* Probably corresponds to unk2 dc/pc */
                uint32_t grave_unk4;
                uint32_t grave_deaths;
                uint32_t grave_coords_time[5];
                char grave_team[20];
                char grave_message[48];
                uint8_t unk3[24];
                char string[12];
                uint8_t unk4[24];
                uint32_t battle[7];
            };
        };
    } entries[0];
} PACKED gc_c_rank_update_pkt;

typedef struct bb_c_rank_update {
    bb_pkt_hdr_t hdr;
    psocn_bb_c_rank_data_t entries[0];
} PACKED bb_c_rank_update_pkt;

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

/* The packet used to set a simple mail autoreply */
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

/* The packet used to write to the infoboard */
typedef autoreply_set_pkt gc_write_info_pkt;
typedef bb_autoreply_set_pkt bb_write_info_pkt;

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

#ifdef PSOCN_CHARACTERS_H

/* Blue Burst option configuration packet */
typedef struct bb_opt_config {
    bb_pkt_hdr_t hdr;
    uint8_t unk1[0x000C];                         //276 - 264 = 12
    psocn_bb_guild_card_t gc_data2;               //264大小
    bb_key_config_t key_cfg;
    bb_guild_t guild_data;
} PACKED bb_opt_config_pkt;

#endif

/* Blue Burst packet used to select a character. */
typedef struct bb_char_select {
    bb_pkt_hdr_t hdr;
    uint8_t slot;
    uint8_t padding1[3];
    uint8_t reason;
    uint8_t padding2[3];
} PACKED bb_char_select_pkt;

/* Blue Burst packet to acknowledge a character select. */
typedef struct bb_char_ack {
    bb_pkt_hdr_t hdr;
    uint8_t slot;
    uint8_t padding1[3];
    uint8_t code;
    uint8_t padding2[3];
} PACKED bb_char_ack_pkt;

/* Blue Burst packet to send the client's checksum */
typedef struct bb_checksum {
    bb_pkt_hdr_t hdr;
    uint32_t checksum;
    uint32_t padding;
} PACKED bb_checksum_pkt;

/* Blue Burst packet to acknowledge the client's checksum. */
typedef struct bb_checksum_ack {
    bb_pkt_hdr_t hdr;
    uint32_t ack;
} PACKED bb_checksum_ack_pkt;

/* Blue Burst packet that acts as a header for the client's guildcard data. */
typedef struct bb_guildcard_hdr {
    bb_pkt_hdr_t hdr;
    uint8_t one;
    uint8_t padding1[3];
    uint16_t len;
    uint8_t padding2[2];
    uint32_t checksum;
} PACKED bb_guildcard_hdr_pkt;

/* Blue Burst packet that requests guildcard data. */
typedef struct bb_guildcard_req {
    bb_pkt_hdr_t hdr;
    uint32_t unk;
    uint32_t chunk;
    uint32_t cont;
} PACKED bb_guildcard_req_pkt;
//(00000000)   08 00 E0 00 00 00 00 00                             ........
/* Blue Burst packet that requests option data. */
typedef struct bb_option_req {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_option_req_pkt;

/* Blue Burst packet for sending a chunk of guildcard data. */
typedef struct bb_guildcard_chunk {
    bb_pkt_hdr_t hdr;
    uint32_t unk;
    uint32_t chunk;
    uint8_t data[];
} PACKED bb_guildcard_chunk_pkt;

/* Blue Burst packet that's a header for the parameter files. */
typedef struct bb_param_hdr {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t size;
        uint32_t checksum;
        uint32_t offset;
        char filename[0x40];
    } entries[];
} PACKED bb_param_hdr_pkt;

/* Blue Burst packet for sending a chunk of the parameter files. */
typedef struct bb_param_chunk {
    bb_pkt_hdr_t hdr;
    uint32_t chunk;
    uint8_t data[];
} PACKED bb_param_chunk_pkt;

/* Blue Burst packet for setting flags (dressing room flag, for instance). */
typedef struct bb_setflag {
    bb_pkt_hdr_t hdr;
    uint32_t flags;
} PACKED bb_setflag_pkt;

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

/* Blue Burst packet for sending the full character data and options */
typedef struct bb_full_char {
    bb_pkt_hdr_t hdr;
    psocn_bb_full_char_t data;
} PACKED bb_full_char_pkt;

#endif /* PSOCN_CHARACTERS_H */

/* Blue Burst packet for updating options */
typedef struct bb_options_update {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_options_update_pkt;

/* Blue Burst 公会数据包 */
typedef struct bb_guild_pkt {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_pkt_pkt;

//////////////////////////////////////////////////////////////////////////
/* Blue Burst 公会创建数据 */
//               0x0020 + 8(数据头)     /0x0009 0x0042/  guild_name
//( 00000000 )   28 00 EA 01 00 00 00 00  09 00 42 00 31 00 32 00    (.......     .B.1.2. //16
// 
//( 00000010 )   33 00 00 00 64 01 00 00  C0 54 25 23 3C 9F 81 00    3...d....T%#<...     //32
//                          /
//( 00000020 )   78 E3 31 20 C4 9C 98 00                             x.1 ....             //40
//( 00000000 )   28 00 EA 01 00 00 00 00  09 00 42 00 33 00 33 00    (.......     .B.3.3.
//( 00000010 )   33 00 33 00 33 00 33 00  33 00 33 00 33 00 33 00    3.3.3.3.3.3.3.3.
//( 00000020 )   33 00 33 00 00 00 98 00                             3.3.....
typedef struct bb_guild_data {
    bb_pkt_hdr_t hdr;
    psocn_bb_db_guild_t guild;
} PACKED bb_guild_data_pkt;

typedef struct bb_guild_create {
    bb_pkt_hdr_t hdr;
    uint16_t guild_name[0x000E];
    uint32_t guildcard;
} PACKED bb_guild_create_pkt;

typedef struct bb_guild_unk_02EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_02EA_pkt;

typedef struct bb_guild_member_add {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_member_add_pkt;

typedef struct bb_guild_unk_04EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_04EA_pkt;

typedef struct bb_guild_member_remove {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_member_remove_pkt;

typedef struct bb_guild_unk_06EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_06EA_pkt;

typedef struct bb_guild_member_chat {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_member_chat_pkt;

typedef struct bb_guild_member_setting {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;
    uint32_t guild_id;
    uint16_t char_name[12];
    uint8_t data[];
} PACKED bb_guild_member_setting_pkt;

typedef struct bb_guild_unk_09EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_09EA_pkt;

typedef struct bb_guild_unk_0AEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_0AEA_pkt;

typedef struct bb_guild_unk_0BEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_0BEA_pkt;

typedef struct bb_guild_unk_0CEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_0CEA_pkt;

typedef struct bb_guild_unk_0DEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_0DEA_pkt;

typedef struct bb_guild_unk_0EEA {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;                    // 02B8         4
    uint32_t guild_id;                     // 02BC
    uint8_t guild_info[8];                 // 公会信息     8
    uint16_t guild_name[0x000E];           // 02CC
    uint16_t unk_flag;
    uint8_t guild_flag[0x0800];            // 公会图标
    uint8_t unk2;
    uint8_t unk3;
} PACKED bb_guild_unk_0EEA_pkt;

typedef struct bb_guild_member_flag_setting {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_member_flag_setting_pkt;

typedef struct bb_guild_dissolve_team {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_dissolve_team_pkt;

typedef struct bb_guild_member_promote {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_member_promote_pkt;

typedef struct bb_guild_unk_12EA {
    bb_pkt_hdr_t hdr;
    uint32_t unk;//TODO ??
    uint32_t guildcard;
    uint32_t guild_id;
    uint8_t guild_info[8];
    uint32_t guild_priv_level;
    uint16_t guild_name[0x000E];
    uint32_t guild_rank;
} PACKED bb_guild_unk_12EA_pkt;

typedef struct bb_guild_lobby_setting {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_lobby_setting_pkt;

typedef struct bb_guild_member_tittle {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_member_tittle_pkt;

typedef struct bb_guild_unk_15EA {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;
    uint32_t guild_id;
    uint8_t guild_info[8];
    uint32_t guild_priv_level;
    uint16_t guild_name[0x000E];
    uint32_t guild_rank;
    uint32_t guildcard2;
    uint32_t client_id;
    uint16_t char_name[12];
    uint8_t guild_flag[0x0800];
    uint32_t guild_rewards[2];
} PACKED bb_guild_unk_15EA_pkt;

typedef struct bb_guild_unk_16EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_16EA_pkt;

typedef struct bb_guild_unk_17EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_17EA_pkt;

typedef struct bb_guild_buy_privilege_and_point_info {
    bb_pkt_hdr_t hdr;
    //uint32_t guildcard;
    //uint32_t guild_id;
    //uint8_t guild_info[8];
    //uint32_t guild_priv_level;
    //uint32_t guild_rank;
    //uint32_t guildcard2;
    //uint32_t client_id;
    //uint16_t char_name[12];
    //uint32_t guild_rewards[2];
    uint8_t data[0x4C];
} PACKED bb_guild_buy_privilege_and_point_info_pkt;

typedef struct bb_guild_privilege_list {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_privilege_list_pkt;

typedef struct bb_guild_unk_1AEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_1AEA_pkt;

typedef struct bb_guild_unk_1BEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_1BEA_pkt;

typedef struct bb_guild_rank_list {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_rank_list_pkt;

typedef struct bb_guild_unk_1DEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_1DEA_pkt;

//////////////////////////////////////////////////////////////////////////
/* TODO 完成挑战模式BLOCK DF指令*/

typedef struct bb_challenge_01df {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_challenge_01df_pkt;

typedef struct bb_challenge_02df {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_challenge_02df_pkt;

typedef struct bb_challenge_03df {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_challenge_03df_pkt;

typedef struct bb_challenge_04df {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_challenge_04df_pkt;

typedef struct bb_challenge_05df {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_challenge_05df_pkt;

typedef struct bb_challenge_06df {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_challenge_06df_pkt;

/* Blue Burst packet for adding a Guildcard to the user's list */
typedef struct bb_guildcard_add {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;
    uint16_t name[24];
    uint16_t guild_name[16];
    uint16_t text[88];
    uint8_t one;
    uint8_t language;
    uint8_t section;
    uint8_t char_class;
} PACKED bb_guildcard_add_pkt;

typedef bb_guildcard_add_pkt bb_blacklist_add_pkt;
typedef bb_guildcard_add_pkt bb_guildcard_set_txt_pkt;

/* Blue Burst packet for deleting a Guildcard */
typedef struct bb_guildcard_del {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;
} PACKED bb_guildcard_del_pkt;

typedef bb_guildcard_del_pkt bb_blacklist_del_pkt;

/* Blue Burst packet for sorting Guildcards */
typedef struct bb_guildcard_sort {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard1;
    uint32_t guildcard2;
} PACKED bb_guildcard_sort_pkt;

/* Blue Burst packet for setting a comment on a Guildcard */
typedef struct bb_guildcard_comment {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;
    uint16_t text[88];
} PACKED bb_guildcard_comment_pkt;

/* Gamecube quest statistics packet (from Maximum Attack 2). */
typedef struct gc_quest_stats {
    dc_pkt_hdr_t hdr;
    uint32_t stats[10];
} PACKED gc_quest_stats_pkt;

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

/* Patch return packet. */
typedef struct patch_return {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t retval;
    uint32_t crc;
} PACKED patch_return_pkt;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* Parameters for the various packets. */
#include "packetlist.h"
#endif /* !PACKETS_H_HAVE_PACKETS */
#endif /* !PACKETS_H_HEADERS_ONLY */
