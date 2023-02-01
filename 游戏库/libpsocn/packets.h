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

// Text escape codes

// Most text fields allow the use of various escape codes to change decoding,
// change color, or create symbols. These escape codes are always preceded by a
// tab character (0x09, or '\t'). For brevity, we generally refer to them with $
// instead in newserv, since the server substitutes most usage of $ in player-
// provided text with \t. The escape codes are:
// - Language codes
// - - $E: Set text interpretation to English
// - - $J: Set text interpretation to Japanese
// - Color codes
// - - $C0: Black (000000)
// - - $C1: Blue (0000FF)
// - - $C2: Green (00FF00)
// - - $C3: Cyan (00FFFF)
// - - $C4: Red (FF0000)
// - - $C5: Magenta (FF00FF)
// - - $C6: Yellow (FFFF00)
// - - $C7: White (FFFFFF)
// - - $C8: Pink (FF8080)
// - - $C9: Violet (8080FF)
// - - $CG: Orange pulse (FFE000 + darkenings thereof)
// - - $Ca: Orange (F5A052; Episode 3 only)
// - Special character codes (Ep3 only)
// - - $B: Dash + small bullet
// - - $D: Large bullet
// - - $F: Female symbol
// - - $I: Infinity
// - - $M: Male symbol
// - - $O: Open circle
// - - $R: Solid circle
// - - $S: Star-like ability symbol
// - - $X: Cross
// - - $d: Down arrow
// - - $l: Left arrow
// - - $r: Right arrow
// - - $u: Up arrow

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

/* 客户端数据头联合结构 */
typedef union pkt_header {
    dc_pkt_hdr_t dc;
    dc_pkt_hdr_t gc;
    dc_pkt_hdr_t xb;
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

// 00: Invalid command
// 01: Invalid command

// 02 (S->C): Start encryption
// Client will respond with an 02 command.
// All commands after this command will be encrypted with PSO V2 encryption.
// If this command is sent during an encrypted session, the client will not
// reject it; it will simply re-initialize its encryption state and respond with
// an 02 as normal.
// The copyright field in the below structure must contain the following text:
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

// Game server commands

// 00: Invalid command

// 01 (S->C): Lobby message box
// A small message box appears in lower-right corner, and the player must press
// a key to continue. The maximum length of the message is 0x200 bytes.
// This format is shared by multiple commands; for all of them except 06 (S->C),
// the guild_card_number field is unused and should be 0.

// 02 (S->C): Start encryption (except on BB)
// This command should be used for non-initial sessions (after the client has
// already selected a ship, for example). Command 17 should be used instead for
// the first connection.
// All commands after this command will be encrypted with PSO V2 encryption on
// DC, PC, and GC Episodes 1&2 Trial Edition, or PSO V3 encryption on other V3
// versions.
// DCv1 clients will respond with an (encrypted) 93 command.
// DCv2 and PC clients will respond with an (encrypted) 9A or 9D command.
// V3 clients will respond with an (encrypted) 9A or 9E command, except for GC
// Episodes 1&2 Trial Edition, which behaves like PC.
// The copyright field in the below structure must contain the following text:
// "DreamCast Lobby Server. Copyright SEGA Enterprises. 1999"
// (The above text is required on all versions that use this command, including
// those versions that don't run on the DreamCast.)

// 03 (C->S): Legacy login (non-BB)
// TODO: Check if this command exists on DC v1/v2.

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

// 03 (S->C): Legacy password check result (non-BB)
// header.flag specifies if the password was correct. If header.flag is 0, the
// password saved to the memory card (if any) is deleted and the client is
// disconnected. If header.flag is nonzero, the client responds with an 04
// command. Curiously, it looks like even DCv1 doesn't use this command in its
// standard login sequence, so this may be a relic from very early development.
// No other arguments

// 03 (S->C): Start encryption (BB)
// Client will respond with an (encrypted) 93 command.
// All commands after this command will be encrypted with PSO BB encryption.
// The copyright field in the below structure must contain the following text:
// "Phantasy Star Online Blue Burst Game Server. Copyright 1999-2004 SONICTEAM."
typedef struct bb_welcome {
    bb_pkt_hdr_t hdr;
    char copyright[0x60];
    uint8_t server_key[0x30];
    uint8_t client_key[0x30];
    // As in 02, this field is not part of SEGA's implementation.
    char after_message[0xC0];
} PACKED bb_welcome_pkt;

// 05: Disconnect
// No arguments
// Sending this command to a client will cause it to disconnect. There's no
// advantage to doing this over simply closing the TCP connection. Clients will
// send this command to the server when they are about to disconnect, but the
// server does not need to close the connection when it receives this command
// (and in some cases, the client will send multiple 05 commands before actually
// disconnecting).
typedef struct bb_burst {
    bb_pkt_hdr_t hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED bb_burst_pkt;

// 06: Chat
// Server->client format is same as 01 command. The maximum size of the message
// is 0x200 bytes.
// When sent by the client, the text field includes only the message. When sent
// by the server, the text field includes the origin player's name, followed by
// a tab character, followed by the message.
// During Episode 3 battles, the first byte of an inbound 06 command's message
// is interpreted differently. It should be treated as a bit field, with the low
// 4 bits intended as masks for who can see the message. If the low bit (1) is
// set, for example, then the chat message displays as " (whisper)" on player
// 0's screen regardless of the message contents. The next bit (2) hides the
// message from player 1, etc. The high 4 bits of this byte appear not to be
// used, but are often nonzero and set to the value 4. We call this byte
// private_flags in the places where newserv uses it.
// Client->server format is very similar; we include a zero-length array in this
// struct to make parsing easier.
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

// 06: Chat
// Server->client format is same as 01 command. The maximum size of the message
// is 0x200 bytes.
// When sent by the client, the text field includes only the message. When sent
// by the server, the text field includes the origin player's name, followed by
// a tab character, followed by the message.
// During Episode 3 battles, the first byte of an inbound 06 command's message
// is interpreted differently. It should be treated as a bit field, with the low
// 4 bits intended as masks for who can see the message. If the low bit (1) is
// set, for example, then the chat message displays as " (whisper)" on player
// 0's screen regardless of the message contents. The next bit (2) hides the
// message from player 1, etc. The high 4 bits of this byte appear not to be
// used, but are often nonzero and set to the value 4. We call this byte
// private_flags in the places where newserv uses it.
// Client->server format is very similar; we include a zero-length array in this
// struct to make parsing easier.
typedef struct bb_chat {
    bb_pkt_hdr_t hdr;
    uint32_t padding;
    uint32_t guildcard;
    uint16_t msg[];
} PACKED bb_chat_pkt;

// 07 (S->C): Ship select menu
// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client. The text of
// the first entry becomes the ship name when the client joins a lobby.
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

// 07 (S->C): Ship select menu
// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client. The text of
// the first entry becomes the ship name when the client joins a lobby.
typedef struct pc_ship_list {
    pc_pkt_hdr_t hdr;           /* The flags field says how many entries */
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;
        uint16_t name[0x11];
    } entries[0];
} PACKED pc_ship_list_pkt;

// 07 (S->C): Ship select menu
// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client. The text of
// the first entry becomes the ship name when the client joins a lobby.
typedef struct bb_ship_list {
    bb_pkt_hdr_t hdr;           /* The flags field says how many entries */
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        uint16_t flags;         // Should be 0x0F04 this value, apparently
        uint16_t name[0x11];
    } entries[0];
} PACKED bb_ship_list_pkt;

// 08 (C->S): Request game list
// No arguments
// 08 (S->C): Game list
// Client responds with 09 and 10 commands (or nothing if the player cancels).
// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client.
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

// 08 (C->S): Request game list
// No arguments
// 08 (S->C): Game list
// Client responds with 09 and 10 commands (or nothing if the player cancels).
// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client.
/* The packet sent to clients to give them the game select list */
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

// 08 (C->S): Request game list
// No arguments
// 08 (S->C): Game list
// Client responds with 09 and 10 commands (or nothing if the player cancels).
// Command is a list of these; header.flag is the entry count. The first entry
// is not included in the count and does not appear on the client.
/* The packet sent to clients to give them the game select list */
typedef struct bb_game_list {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t menu_id;
        uint32_t item_id;
        // difficulty_tag is 0x0A on Episode 3; on all other versions, it's
        // difficulty + 0x22 (so 0x25 means Ultimate, for example)
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
// Server will respond with an 11 command, or an A3 or A5 if the specified menu
// is the quest menu.
/* The menu selection packet that the client sends to us */
typedef struct dc_select {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED dc_select_pkt;

// 09 (C->S): Menu item info request
// Server will respond with an 11 command, or an A3 or A5 if the specified menu
// is the quest menu.
typedef struct bb_select {
    bb_pkt_hdr_t hdr;
    uint32_t menu_id;
    uint32_t item_id;
} PACKED bb_select_pkt;

// 0B: Invalid command

// 0C: Create game (DCv1)
// Same format as C1, but fields not supported by v1 (e.g. episode, v2 mode) are
// unused.
/* The packet sent by clients to create a game */
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

// 0D: Invalid command

// 0E (S->C): Unknown; possibly legacy join game (PC/V3)
// There is a failure mode in the command handlers on PC and V3 that causes the
// thread receiving the command to loop infinitely doing nothing, effectively
// softlocking the game.
// TODO: Check if this command exists on DC v1/v2.
struct S_Unknown_PC_0E {
    pc_pkt_hdr_t hdr;
    uint8_t unknown_a1[0x08];
    uint8_t unknown_a2[4][0x18];
    uint8_t unknown_a3[0x18];
} PACKED;

//struct S_Unknown_GC_0E {
//    PlayerLobbyDataDCGC lobby_data[4]; // This type is a guess
//    struct UnknownA0 {
//        parray<uint8_t, 2> unknown_a1;
//        le_uint16_t unknown_a2 = 0;
//        le_uint32_t unknown_a3 = 0;
//    } __packed__;
//    parray<UnknownA0, 8> unknown_a0;
//    le_uint32_t unknown_a1 = 0;
//    parray<uint8_t, 0x20> unknown_a2;
//    parray<uint8_t, 4> unknown_a3;
//} __packed__;
//
//struct S_Unknown_XB_0E {
//    parray<uint8_t, 0xE8> unknown_a1;
//} __packed__;

// 0F: Invalid command

// 10 (C->S): Menu selection
// header.flag contains two flags: 02 specifies if a password is present, and 01
// specifies... something else. These two bits directly correspond to the two
// lowest bits in the flags field of the game menu: 02 specifies that the game
// is locked, but the function of 01 is unknown.
// Annoyingly, the no-arguments form of the command can have any flag value, so
// it doesn't suffice to check the flag value to know which format is being
// used!
//
//struct C_MenuSelection_10_Flag00 {
//    le_uint32_t menu_id = 0;
//    le_uint32_t item_id = 0;
//} __packed__;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag01 {
//    C_MenuSelection_10_Flag00 basic_cmd;
//    ptext<CharT, 0x10> unknown_a1;
//} __packed__;
//struct C_MenuSelection_DC_V3_10_Flag01 : C_MenuSelection_10_Flag01<char> { } __packed__;
//struct C_MenuSelection_PC_BB_10_Flag01 : C_MenuSelection_10_Flag01<char16_t> { } __packed__;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag02 {
//    C_MenuSelection_10_Flag00 basic_cmd;
//    ptext<CharT, 0x10> password;
//} __packed__;
//struct C_MenuSelection_DC_V3_10_Flag02 : C_MenuSelection_10_Flag02<char> { } __packed__;
//struct C_MenuSelection_PC_BB_10_Flag02 : C_MenuSelection_10_Flag02<char16_t> { } __packed__;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag03 {
//    C_MenuSelection_10_Flag00 basic_cmd;
//    ptext<CharT, 0x10> unknown_a1;
//    ptext<CharT, 0x10> password;
//} __packed__;
//struct C_MenuSelection_DC_V3_10_Flag03 : C_MenuSelection_10_Flag03<char> { } __packed__;
//struct C_MenuSelection_PC_BB_10_Flag03 : C_MenuSelection_10_Flag03<char16_t> { } __packed__;

// 11 (S->C): Ship info
// Same format as 01 command.

// 12 (S->C): Valid but ignored (PC/V3/BB)
// TODO: Check if this command exists on DC v1/v2.

// 13 (S->C): Write online quest file
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A7 instead.
// All chunks except the last must have 0x400 data bytes. When downloading an
// online quest, the .bin and .dat chunks may be interleaved (although newserv
// currently sends them sequentially).

// header.flag = file chunk index (start offset / 0x400)
//struct S_WriteFile_13_A7 {
//    ptext<char, 0x10> filename;
//    parray<uint8_t, 0x400> data;
//    le_uint32_t data_size = 0;
//} __packed__;

// 13 (C->S): Confirm file write (V3/BB)
// Client sends this in response to each 13 sent by the server. It appears these
// are only sent by V3 and BB - PSO DC and PC do not send these.

// header.flag = file chunk index (same as in the 13/A7 sent by the server)
//struct C_WriteFileConfirmation_V3_BB_13_A7 {
//    ptext<char, 0x10> filename;
//} __packed__;


// 14 (S->C): Valid but ignored (PC/V3/BB)
// TODO: Check if this command exists on DC v1/v2.

// 15: Invalid command

// 16 (S->C): Valid but ignored (PC/V3/BB)
// TODO: Check if this command exists on DC v1/v2.

// 17 (S->C): Start encryption at login server (except on BB)
// Same format and usage as 02 command, but a different copyright string:
// "DreamCast Port Map. Copyright SEGA Enterprises. 1999"
// Unlike the 02 command, V3 clients will respond with a DB command when they
// receive a 17 command in any online session, with the exception of Episodes
// 1&2 trial edition (which responds with a 9A). DCv1 will respond with a 90.
// Other non-V3 clients will respond with a 9A or 9D.

// 18 (S->C): License verification result (PC/V3)
// Behaves exactly the same as 9A (S->C). No arguments except header.flag.
// TODO: Check if this command exists on DC v1/v2.

// 19 (S->C): Reconnect to different address
// Client will disconnect, and reconnect to the given address/port. Encryption
// will be disabled on the new connection; the server should send an appropriate
// command to enable it when the client connects.
// Note: PSO XB seems to ignore the address field, which makes sense given its
// networking architecture.

//struct S_Reconnect_19 : S_Reconnect<le_uint16_t> { } __packed__;

// Because PSO PC and some versions of PSO DC/GC use the same port but different
// protocols, we use a specially-crafted 19 command to send them to two
// different ports depending on the client version. I first saw this technique
// used by Schthack; I don't know if it was his original creation.
//struct S_ReconnectSplit_19 {
//    be_uint32_t pc_address = 0;
//    le_uint16_t pc_port = 0;
//    parray<uint8_t, 0x0F> unused1;
//    uint8_t gc_command = 0x19;
//    uint8_t gc_flag = 0;
//    le_uint16_t gc_size = 0x97;
//    be_uint32_t gc_address = 0;
//    le_uint16_t gc_port = 0;
//    parray<uint8_t, 0xB0 - 0x23> unused2;
//} __packed__;

// 1A (S->C): Large message box
// On V3, client will sometimes respond with a D6 command (see D6 for more
// information).
// Contents are plain text (char on DC/V3, char16_t on PC/BB). There must be at
// least one null character ('\0') before the end of the command data.
// There is a bug in V3 (and possibly all versions) where if this command is
// sent after the client has joined a lobby, the chat log window contents will
// appear in the message box, prepended to the message text from the command.
// The maximum length of the message is 0x400 bytes. This is the only difference
// between this command and the D5 command.

// 1B (S->C): Valid but ignored (PC/V3)
// TODO: Check if this command exists on DC v1/v2.

// 1C (S->C): Valid but ignored (PC/V3)
// TODO: Check if this command exists on DC v1/v2.

// 1D: Ping
// No arguments
// When sent to the client, the client will respond with a 1D command. Data sent
// by the server is ignored; the client always sends a 1D command with no data.

// 1E: Invalid command

// 1F (C->S): Request information menu
// No arguments
// This command is used in PSO DC and PC. It exists in V3 as well but is
// apparently unused.

// 1F (S->C): Information menu
// Same format and usage as 07 command, except:
// - The menu title will say "Information" instead of "Ship Select".
// - There is no way to request details before selecting a menu item (the client
//   will not send 09 commands).
// - The player can press a button (B on GC, for example) to close the menu
//   without selecting anything, unlike the ship select menu. The client does
//   not send anything when this happens.

// 20: Invalid command

// 21: GameGuard control (old versions of BB)
// Format unknown

// 22: GameGuard check (BB)

// Command 0022 is a 16-byte challenge (sent in the data field) using the
// following structure.

//struct SC_GameCardCheck_BB_0022 {
//    parray<le_uint32_t, 4> data;
//} __packed__;

// Command 0122 uses a 4-byte challenge sent in the header.flag field instead.
// This version of the command has no other arguments.

// 23 (S->C): Unknown (BB)
// header.flag is used, but the command has no other arguments.

// 24 (S->C): Unknown (BB)
//struct S_Unknown_BB_24 {
//    le_uint16_t unknown_a1 = 0;
//    le_uint16_t unknown_a2 = 0;
//    parray<le_uint32_t, 8> values;
//} __packed__;

// 25 (S->C): Unknown (BB)
//struct S_Unknown_BB_25 {
//    bb_pkt_hdr_t hdr;
//    le_uint16_t unknown_a1 = 0;
//    uint8_t offset1 = 0;
//    uint8_t value1 = 0;
//    uint8_t offset2 = 0;
//    uint8_t value2 = 0;
//    le_uint16_t unused = 0;
//} __packed__;

// 26: Invalid command
// 27: Invalid command
// 28: Invalid command
// 29: Invalid command
// 2A: Invalid command
// 2B: Invalid command
// 2C: Invalid command
// 2D: Invalid command
// 2E: Invalid command
// 2F: Invalid command
// 30: Invalid command
// 31: Invalid command
// 32: Invalid command
// 33: Invalid command
// 34: Invalid command
// 35: Invalid command
// 36: Invalid command
// 37: Invalid command
// 38: Invalid command
// 39: Invalid command
// 3A: Invalid command
// 3B: Invalid command
// 3C: Invalid command
// 3D: Invalid command
// 3E: Invalid command
// 3F: Invalid command

// 40 (C->S): Guild card search
// The server should respond with a 41 command if the target is online. If the
// target is not online, the server doesn't respond at all.
/* The packet sent to search for a player */
typedef struct dc_guild_search {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint32_t player_tag;
    uint32_t searcher_gc;
    uint32_t target_gc;
} PACKED dc_guild_search_pkt;

// 40 (C->S): Guild card search
// The server should respond with a 41 command if the target is online. If the
// target is not online, the server doesn't respond at all.
typedef struct bb_guild_search {
    bb_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t searcher_gc;
    uint32_t target_gc;
} PACKED bb_guild_search_pkt;

// 41 (S->C): Guild card search result
/* The packet sent to reply to a guild card search */
typedef struct dc_guild_reply {
    dc_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t searcher_gc;
    uint32_t target_gc;
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
    uint32_t searcher_gc;
    uint32_t target_gc;
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
    uint32_t searcher_gc;
    uint32_t target_gc;
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
    uint32_t searcher_gc;
    uint32_t target_gc;
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
    uint32_t searcher_gc;
    uint32_t target_gc;
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
    uint32_t searcher_gc;
    uint32_t target_gc;
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

//// 42: Invalid command
//// 43: Invalid command





typedef struct bb_done_burst {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
        bb_pkt_hdr_t bb;
    };
    uint8_t data[];
} PACKED bb_done_burst_pkt;

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

// B1 (C->S): Request server time
// No arguments
// Server will respond with a B1 command.
/* The packet sent as a timestamp */
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

// DD (S->C): Send quest state to joining player (BB)
// When a player joins a game with a quest already in progress, the server
// should send this command to the leader. header.flag is the client ID that the
// leader should send quest state to; the leader will then send a series of
// target commands (62/6D) that the server can forward to the joining player.
// No other arguments
typedef struct bb_send_quest_state {
    // Unused entries are set to FFFF
    uint8_t data[0];
} PACKED bb_send_quest_state_pkt;

// DE (S->C): Rare monster list (BB)
typedef struct bb_rare_monster_list {
    // Unused entries are set to FFFF
    uint16_t monster_ids[16];
} PACKED bb_rare_monster_list_pkt;

/* 舰船发送至客户端 C-Rank 数据包结构 */
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
    uint32_t target_guildcard;
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
    uint32_t guild_id;                     // 02BC
    uint8_t chat[];
} PACKED bb_guild_member_chat_pkt;

typedef struct bb_guild_member_setting {
    bb_pkt_hdr_t hdr;
    struct {
        uint32_t member_num;
        uint32_t guild_priv_level;             // 会员等级     4
        uint32_t guildcard_client;
        uint16_t char_name[BB_CHARACTER_NAME_LENGTH]; //24
        uint32_t guild_rewards[2];             // 公会奖励 包含 更改皮肤  4 + 4
    } entries[0];
    //uint8_t data[];
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

typedef struct bb_guild_invite_0DEA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_invite_0DEA_pkt;

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
    uint8_t guild_flag[0x0800];
} PACKED bb_guild_member_flag_setting_pkt;

typedef struct bb_guild_dissolve {
    bb_pkt_hdr_t hdr;
    //uint32_t guild_id;
    uint8_t data[];
} PACKED bb_guild_dissolve_pkt;

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
    struct {
        uint32_t guildcard;                    // 02B8         4
        uint32_t guild_id;                     // 02BC         4 
        uint8_t guild_info[8];                 // 公会信息     8
        uint32_t guild_priv_level;             // 会员等级     4
        uint16_t guild_name[0x000E];           // 02CC         28
        uint32_t guild_rank;                   // 公会排行     4
        uint32_t guildcard_client;
        uint32_t client_id;
        uint16_t char_name[BB_CHARACTER_NAME_LENGTH]; //24
        uint32_t guild_rewards[2];             // 公会奖励 包含 更改皮肤  4 + 4
        uint8_t guild_flag[0x0800];            // 公会图标     2048
    } entries[0];
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
    uint32_t guildcard_client;
    uint32_t client_id;
    uint16_t char_name[12];
    uint8_t guild_flag[0x0800];
    uint32_t guild_rewards[2];
} PACKED bb_guild_full_data_15EA_pkt;

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
    uint8_t unk_data[0x000C];
    uint8_t guild_info[8];
    uint32_t guild_priv_level;
    uint32_t guildcard_client;
    uint16_t char_name[BB_CHARACTER_NAME_LENGTH];
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
#include <pso_cmd_code.h>
#endif /* !PACKETS_H_HAVE_PACKETS */
#endif /* !PACKETS_H_HEADERS_ONLY */
