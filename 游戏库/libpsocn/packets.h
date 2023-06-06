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
#define SWAP16(x) (((x >> 8) & 0xFF) | ((x & 0xFF) << 8))
#define LE32(x) x
#define SWAP32(x) (((x >> 24) & 0x00FF) | \
                   ((x >>  8) & 0xFF00) | \
                   ((x & 0xFF00) <<  8) | \
                   ((x & 0x00FF) << 24))
#define LE64(x) x
#define SWAP64(x) (((x >> 56) & 0x000000FF) | \
                 ((x >> 40) & 0x0000FF00) | \
                 ((x >> 24) & 0x00FF0000) | \
                 ((x >>  8) & 0xFF000000) | \
                 ((x & 0xFF000000) <<  8) | \
                 ((x & 0x00FF0000) << 24) | \
                 ((x & 0x0000FF00) << 40) | \
                 ((x & 0x000000FF) << 56))
#endif


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
    uint32_t client_id;
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
    uint32_t client_id;
    uint32_t guildcard;
    uint16_t msg[0];
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

// 0E (S->C): Incomplete/legacy join game (PC/V3)
// header.flag = number of valid entries in lobby_data

// It's fairly clear that this command was intended for joining games since its
// structure is similar to that of 64. Furthermore, 0E sets a flag on the client
// which is also set by commands 64, 67, and E8 (on Episode 3), which are all
// lobby and game join commands.

// There is a failure mode in the 0E command handlers on PC and V3 that causes
// the thread receiving the command to loop infinitely doing nothing,
// effectively softlocking the game. This happens if the local player's Guild
// Card number doesn't match any of the lobby_data entries. (Notably, only the
// first (header.flag) entries are checked.)
// If the local players' Guild Card number does match one of the entries, the
// command does not softlock, but instead does nothing (at least, on PC and V3)
// because the 0E second-phase handler is missing on the client.

// TODO: Check if this command exists on DC v1/v2.

typedef struct UnknownA1 {
    uint32_t player_tag;
    uint32_t guild_card_number;
    uint8_t unknown_a1[0x10];
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
    uint8_t unknown_a1[0x18];
} PACKED UnknownA1_XB_t;

struct S_LegacyJoinGame_XB_0E {
    UnknownA1_XB_t unknown_a1[4];
    uint8_t unknown_a2[0x68];
} PACKED;

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
//    uint32_t menu_id;
//    uint32_t item_id;
//} PACKED;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag01 {
//    C_MenuSelection_10_Flag00 basic_cmd;
//    ptext<CharT, 0x10> unknown_a1;
//} PACKED;
//struct C_MenuSelection_DC_V3_10_Flag01 : C_MenuSelection_10_Flag01<char> { } PACKED;
//struct C_MenuSelection_PC_BB_10_Flag01 : C_MenuSelection_10_Flag01<char16_t> { } PACKED;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag02 {
//    C_MenuSelection_10_Flag00 basic_cmd;
//    ptext<CharT, 0x10> password;
//} PACKED;
//struct C_MenuSelection_DC_V3_10_Flag02 : C_MenuSelection_10_Flag02<char> { } PACKED;
//struct C_MenuSelection_PC_BB_10_Flag02 : C_MenuSelection_10_Flag02<char16_t> { } PACKED;
//
//template <typename CharT>
//struct C_MenuSelection_10_Flag03 {
//    C_MenuSelection_10_Flag00 basic_cmd;
//    ptext<CharT, 0x10> unknown_a1;
//    ptext<CharT, 0x10> password;
//} PACKED;
//struct C_MenuSelection_DC_V3_10_Flag03 : C_MenuSelection_10_Flag03<char> { } PACKED;
//struct C_MenuSelection_PC_BB_10_Flag03 : C_MenuSelection_10_Flag03<char16_t> { } PACKED;

// 11 (S->C): Ship info
// Same format as 01 command.
// The packet used for the information reply
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
//    uint32_t data_size;
//} PACKED;

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

// 13 (C->S): Confirm file write (V3/BB)
// Client sends this in response to each 13 sent by the server. It appears these
// are only sent by V3 and BB - PSO DC and PC do not send these.

// header.flag = file chunk index (same as in the 13/A7 sent by the server)
//struct C_WriteFileConfirmation_V3_BB_13_A7 {
//    ptext<char, 0x10> filename;
//} PACKED;


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
//    parray<uint32_t, 4> data;
//} PACKED;

// Command 0122 uses a 4-byte challenge sent in the header.flag field instead.
// This version of the command has no other arguments.

// 23 (S->C): Unknown (BB)
// header.flag is used, but the command has no other arguments.

// 24 (S->C): Unknown (BB)
//struct S_Unknown_BB_24 {
//    uint16_t unknown_a1;
//    uint16_t unknown_a2;
//    parray<uint32_t, 8> values;
//} PACKED;

// 25 (S->C): Unknown (BB)
//struct S_Unknown_BB_25 {
//    bb_pkt_hdr_t hdr;
//    uint16_t unknown_a1;
//    uint8_t offset1;
//    uint8_t value1;
//    uint8_t offset2;
//    uint8_t value2;
//    uint16_t unused;
//} PACKED;

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
    uint32_t gc_searcher;
    uint32_t gc_target;
} PACKED dc_guild_search_pkt;

// 40 (C->S): Guild card search
// The server should respond with a 41 command if the target is online. If the
// target is not online, the server doesn't respond at all.
typedef struct bb_guild_search {
    bb_pkt_hdr_t hdr;
    uint32_t player_tag;
    uint32_t gc_searcher;
    uint32_t gc_target;
} PACKED bb_guild_search_pkt;

// 41 (S->C): Guild card search result
/* The packet sent to reply to a guild card search */
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

//// 42: Invalid command
//// 43: Invalid command

// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
//
//struct S_OpenFile_DC_44_A6 {
//    ptext<char, 0x20> name; // Should begin with "PSO/"
//    parray<uint8_t, 2> unused;
//    uint8_t flags;
//    ptext<char, 0x11> filename;
//    uint32_t file_size;
//} PACKED;
//
//struct S_OpenFile_PC_V3_44_A6 {
//    ptext<char, 0x20> name; // Should begin with "PSO/"
//    parray<uint8_t, 2> unused;
//    uint16_t flags; // 0 = download quest, 2 = online quest, 3 = Episode 3
//    ptext<char, 0x10> filename;
//    uint32_t file_size;
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
    char name[32];
    uint8_t unused1[3];
    char filename[16];
    uint8_t unused2;
    uint32_t length;
} PACKED dc_quest_file_pkt;

// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
typedef struct pc_quest_file {
    pc_pkt_hdr_t hdr;
    char name[32];
    uint16_t unused;
    uint16_t flags;
    char filename[16];
    uint32_t length;
} PACKED pc_quest_file_pkt;

// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
typedef struct gc_quest_file {
    dc_pkt_hdr_t hdr;
    char name[32];
    uint16_t unused;
    uint16_t flags;
    char filename[16];
    uint32_t length;
} PACKED gc_quest_file_pkt;

// 44 (S->C): Open file for download
// Used for downloading online quests. For download quests (to be saved to the
// memory card), use A6 instead.
// Unlike the A6 command, the client will react to a 44 command only if the
// filename ends in .bin or .dat.
typedef struct bb_quest_file {
    bb_pkt_hdr_t hdr;
    char unused1[32];
    uint16_t unused2;
    uint16_t flags;
    char filename[16];
    uint32_t length;
    char name[24];
} PACKED bb_quest_file_pkt;

// 44 (C->S): Confirm open file (V3/BB)
// Client sends this in response to each 44 sent by the server.

// header.flag = quest number (sort of - seems like the client just echoes
// whatever the server sent in its header.flag field. Also quest numbers can be
// > 0xFF so the flag is essentially meaningless)
//struct C_OpenFileConfirmation_44_A6 {
//    ptext<char, 0x10> filename;
//} PACKED;

// 45: Invalid command
// 46: Invalid command
// 47: Invalid command
// 48: Invalid command
// 49: Invalid command
// 4A: Invalid command
// 4B: Invalid command
// 4C: Invalid command
// 4D: Invalid command
// 4E: Invalid command
// 4F: Invalid command
// 50: Invalid command
// 51: Invalid command
// 52: Invalid command
// 53: Invalid command
// 54: Invalid command
// 55: Invalid command
// 56: Invalid command
// 57: Invalid command
// 58: Invalid command
// 59: Invalid command
// 5A: Invalid command
// 5B: Invalid command
// 5C: Invalid command
// 5D: Invalid command
// 5E: Invalid command
// 5F: Invalid command

// 60: Broadcast command
// When a client sends this command, the server should forward it to all players
// in the same game/lobby, except the player who originally sent the command.
// See ReceiveSubcommands or the subcommand index below for details on contents.
// The data in this command may be up to 0x400 bytes in length. If it's larger,
// the client will exhibit undefined behavior.

// 61 (C->S): Player data
// See the PSOPlayerData structs in Player.hh for this command's format.
// header.flag specifies the format version, which is related to (but not
// identical to) the game's major version. For example, the format version is 01
// on DC v1, 02 on PSO PC, 03 on PSO GC, XB, and BB, and 04 on Episode 3.
// Upon joining a game, the client assigns inventory item IDs sequentially as
// (0x00010000 + (0x00200000 * lobby_client_id) + x). So, for example, player
// 3's 8th item's ID would become 0x00610007. The item IDs from the last game
// the player was in will appear in their inventory in this command.
// Note: If the client is in a game at the time this command is received, the
// inventory sent by the client only includes items that would not disappear if
// the client crashes! Essentially, it reflects the saved state of the player's
// character rather than the live state.

// 62: Target command
// When a client sends this command, the server should forward it to the player
// identified by header.flag in the same game/lobby, even if that player is the
// player who originally sent it.
// See ReceiveSubcommands or the subcommand index below for details on contents.
// The data in this command may be up to 0x400 bytes in length. If it's larger,
// the client will exhibit undefined behavior.

// 63: Invalid command

#ifdef PLAYER_H

// 64 (S->C): Join game
// This is sent to the joining player; the other players get a 65 instead.
// Note that (except on Episode 3) this command does not include the player's
// disp or inventory data. The clients in the game are responsible for sending
// that data to each other during the join process with 60/62/6C/6D commands.
// 68 (S->C): Add player to lobby
// Same format as 65 command, but used for lobbies instead of games.
// The command only includes the joining player's data.
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
// When a player joins an existing game, the joining player receives a 64
// command (described above), and the players already in the game receive a 65
// command containing only the joining player's data.
// Header flag = entry count (always 1 for 65 and 68; up to 0x0C for 67)
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
        inventory_t inv;
        psocn_bb_char_t data;
    } entries[0];
} PACKED bb_lobby_join_pkt;

#endif

// 66 (S->C): Remove player from game
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
// 69 (S->C): Remove player from lobby
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

// 6C: Broadcast command
// Same format and usage as 60 command, but with no size limit.

// 6D: Target command
// Same format and usage as 62 command, but with no size limit.

// 6E: Invalid command

// 6F (C->S): Set game status
// This command is sent when a player is done loading and other players can then
// join the game. On BB, this command is sent as 016F if a quest is in progress
// and the game should not be joined by anyone else.
// 006F
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

// 70: Invalid command
// 71: Invalid command
// 72: Invalid command
// 73: Invalid command
// 74: Invalid command
// 75: Invalid command
// 76: Invalid command
// 77: Invalid command
// 78: Invalid command
// 79: Invalid command
// 7A: Invalid command
// 7B: Invalid command
// 7C: Invalid command
// 7D: Invalid command
// 7E: Invalid command
// 7F: Invalid command

// 80 (S->C): Ignored (PC/V3)
// TODO: Check if this command exists on DC v1/v2.
//struct S_Unknown_PC_V3_80 {
//    uint32_t which; // Expected to be in the range 00-0B... maybe client ID?
//    uint32_t unknown_a1; // Could be player_tag
//    uint32_t unknown_a2; // Could be guild_card_number
//} PACKED;

// 81: Simple mail
// Format is the same in both directions. The server should forward the command
// to the player with to_guild_card_number, if they are online. If they are not
// online, the server may store it for later delivery, send their auto-reply
// message back to the original sender, or simply drop the message.
// On GC (and probably other versions too) the unused space after the text
// contains uninitialized memory when the client sends this command. newserv
// clears the uninitialized data for security reasons before forwarding.
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

// 82: Invalid command

// 83 (S->C): Lobby menu
// This sets the menu item IDs that the client uses for the lobby teleport menu.
// The client expects 15 items here; sending more or fewer items does not change
// the lobby count on the client. If fewer entries are sent, the menu item IDs
// for some lobbies will not be set, and the client will likely send 84 commands
// that don't make sense if the player chooses one of lobbies with unset IDs.
// On Episode 3, the client expects 20 entries instead of 15. The CARD lobbies
// are the last five entries, even though they appear at the top of the list on
// the player's screen.
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

// 84 (C->S): Choose lobby same as bb_select_pkt

// 85: Invalid command
// 86: Invalid command
// 87: Invalid command

// 88 (C->S): License check (DC NTE only)
// The server should respond with an 88 command.
// The first packet sent by the Dreamcast Network Trial Edition. Note that the
// serial number and access key are both 16-character strings with NUL
// terminators at the end.
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

// 8C: Invalid command

// 8D (S->C): Request player data (DC NTE only) DCNTE_CHAR_DATA_REQ_TYPE
// Behaves the same as 95 (S->C) on all other versions. DC NTE crashes if it
// receives 95, so this is used instead.

// 8E: Invalid command DCNTE_SHIP_LIST_TYPE dc_ship_list_pkt

// 8F: Invalid command DCNTE_BLOCK_LIST_REQ_TYPE dc_block_list_pkt

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
// Same format and usage as 17 command, except the client will respond with a 90
// command. On versions that support it, this is strictly less useful than the
// 17 command.

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

// 94: Invalid command

// 95 (S->C): Request player data
// No arguments
// For some reason, some servers send high values in the header.flag field here.
// From what I can tell, that field appears to be completely unused by the
// client - sending zero works just fine. The original Sega servers had some
// uninitialized memory bugs, of which that may have been one, and other private
// servers may have just duplicated Sega's behavior verbatim.
// Client will respond with a 61 command.

// 96 (C->S): Character save information
// TODO: Check if this command exists on DC v1/v2.
typedef struct bb_char_save_info_V3_BB_96 {
    bb_pkt_hdr_t hdr;
    // This field appears to be a checksum or random stamp of some sort; it seems
    // to be unique and constant per character.
    uint32_t unknown_a1;
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

// A8: Invalid command

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

// AA (C->S): Update quest statistics (V3/BB)
// This command is used in Maximum Attack 2, but its format is unlikely to be
// specific to that quest. The structure here represents the only instance I've
// seen so far.
// The server should respond with an AB command.
// This command is likely never sent by PSO GC Episodes 1&2 Trial Edition,
// because the following command (AB) is definitely not valid on that version.
/* Gamecube quest statistics packet (from Maximum Attack 2). */
typedef struct dc_update_quest_stats {
    dc_pkt_hdr_t hdr;
    uint32_t quest_internal_id;
    uint16_t request_token;
    uint16_t unknown_a1;
    uint32_t unknown_a2;
    uint32_t kill_count;
    uint32_t time_taken; // in seconds
    uint32_t unknown_a3[5];
} PACKED dc_update_quest_stats_pkt;

typedef struct bb_update_quest_stats {
    bb_pkt_hdr_t hdr;
    uint32_t quest_internal_id;
    uint16_t request_token;
    uint16_t unknown_a1;
    uint32_t unknown_a2;
    uint32_t kill_count;
    uint32_t time_taken; // in seconds
    uint32_t unknown_a3[5];
} PACKED bb_update_quest_stats_pkt;

// AB (S->C): Confirm update quest statistics (V3/BB)
// This command is not valid on PSO GC Episodes 1&2 Trial Edition.
typedef struct dc_confirm_update_quest_statistics {
    dc_pkt_hdr_t hdr;
    uint16_t unknown_a1; // 0
    uint16_t unknown_a2; // Probably actually unused
    uint16_t request_token; // Should match token sent in AA command
    uint16_t unknown_a3; // Schtserv always sends 0xBFFF here
} PACKED dc_confirm_update_quest_statistics_pkt;

typedef struct bb_confirm_update_quest_statistics {
    bb_pkt_hdr_t hdr;
    uint16_t unknown_a1; // 0
    uint16_t unknown_a2; // Probably actually unused
    uint16_t request_token; // Should match token sent in AA command
    uint16_t unknown_a3; // Schtserv always sends 0xBFFF here
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

// AD: Invalid command
// AE: Invalid command
// AF: Invalid command

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

// B4: Invalid command
// B5: Invalid command
// B6: Invalid command

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

// BC: Invalid command
// BD: Invalid command
// BE: Invalid command
// BF: Invalid command

// C0 (C->S): Request choice search options
// No arguments
// Server should respond with a C0 command (described below).

// C0 (S->C): Choice search options

// Command is a list of these; header.flag is the entry count (incl. top-level).
// template <typename ItemIDT, typename CharT>
//struct S_ChoiceSearchEntry {
//    // Category IDs are nonzero; if the high byte of the ID is nonzero then the
//    // category can be set by the user at any time; otherwise it can't.
//    ItemIDT parent_category_id = 0; // 0 for top-level categories
//    ItemIDT category_id = 0;
//    ptext<CharT, 0x1C> text;
//} __packed__;

//struct S_ChoiceSearchEntry_DC_C0 : S_ChoiceSearchEntry<le_uint32_t, char> { } __packed__;
//struct S_ChoiceSearchEntry_V3_C0 : S_ChoiceSearchEntry<le_uint16_t, char> { } __packed__;
//struct S_ChoiceSearchEntry_PC_BB_C0 : S_ChoiceSearchEntry<le_uint16_t, char16_t> { } __packed__;

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

// C5 (S->C): Challenge rank update (V3/BB)
// header.flag = entry count
// The server sends this command when a player joins a lobby to update the
// challenge mode records of all the present players.
// Entry format is PlayerChallengeDataV3 or PlayerChallengeDataBB.
// newserv currently doesn't send this command at all because the V3 and
// BB formats aren't fully documented.
// TODO: Figure out where the text is in those formats, write appropriate
// conversion functions, and implement the command. Don't forget to overwrite
// the client_id field in each entry before sending.
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

// CD: Invalid command
// CE: Invalid command
// CF: Invalid command

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
    union {
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
// variable does. That variable is also set when a D7 is sent by the client, so
// it likely is related to GBA game loading in some way.
// PSO BB completely ignores this command.

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
//struct C_VerifyLicense_V3_DB {
//    ptext<char, 0x20> unused;
//    ptext<char, 0x10> serial_number; // On XB, this is the XBL gamertag
//    ptext<char, 0x10> access_key; // On XB, this is the XBL user ID
//    ptext<char, 0x08> unused2;
//    uint32_t sub_version;
//    ptext<char, 0x30> serial_number2; // On XB, this is the XBL gamertag
//    ptext<char, 0x30> access_key2; // On XB, this is the XBL user ID
//    ptext<char, 0x30> password; // On XB, this contains "xbox-pso"
//} PACKED;
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
    uint8_t unk1[0x000C];                         //276 - 264 = 12
    psocn_bb_guildcard_t gc_data2;               //264大小
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

// E7: Save or load full player data (BB)
// See export_bb_player_data() in Player.cc for format.
// TODO: Verify full breakdown from send_E7 in BB disassembly.
#ifdef PSOCN_CHARACTERS_H

/* Blue Burst packet for sending the full character data and options */
typedef struct bb_full_char {
    bb_pkt_hdr_t hdr;
    psocn_bb_full_char_t data;
} PACKED bb_full_char_pkt;

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
    psocn_bb_guildcard_t gc_data;
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
    uint32_t ack;
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
    uint16_t text[88];
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

//////////////////////////////////////////////////////////////////////////
/* Blue Burst 公会创建数据 */
typedef struct bb_guild_data {
    bb_pkt_hdr_t hdr;
    psocn_bb_db_guild_t guild;
} PACKED bb_guild_data_pkt;

typedef struct bb_guild_rv_data {
    bb_pkt_hdr_t hdr;
    uint8_t data[0];
} PACKED bb_guild_rv_data_pkt;

// 01EA (C->S): Create team
typedef struct bb_guild_create {
    bb_pkt_hdr_t hdr;
    uint16_t guild_name[0x000E];
    uint32_t guildcard;
} PACKED bb_guild_create_pkt;

// 02EA (S->C): UNKNOW
typedef struct bb_guild_unk_02EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_02EA_pkt;

// 03EA (C->S): Add team member
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

// 05EA (C->S): Remove team member
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

// 07EA (C->S): Team chat
typedef struct bb_guild_member_chat {
    bb_pkt_hdr_t hdr;                                            /* 0x0000 8 */
    //uint32_t guild_id;                                         /* 0x0008 4 */
    uint16_t name[BB_CHARACTER_NAME_LENGTH];                     /* 0x000C 24 */
    uint32_t guildcard;
    uint32_t guild_id;
    uint8_t chat[];
} PACKED bb_guild_member_chat_pkt;

// 08EA (C->S): Team admin
// No arguments
typedef struct bb_guild_member_setting {
    bb_pkt_hdr_t hdr;
    uint32_t amount;                           // 数量
    struct {
        uint32_t member_num;                   // 必须 1 起始
        uint32_t guild_priv_level;             // 会员等级     4
        uint32_t guildcard_client;
        uint16_t char_name[BB_CHARACTER_NAME_LENGTH]; //24
        uint32_t guild_rewards[2];             // 公会奖励 包含 更改皮肤  4 + 4
    } entries[0];
    //uint8_t data[];
} PACKED bb_guild_member_setting_pkt;

// 09EA (S->C): UNKNOW
typedef struct bb_guild_unk_09EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_09EA_pkt;

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

// 0FEA (C->S): Set team flag
typedef struct bb_guild_member_flag_setting {
    bb_pkt_hdr_t hdr;
    uint8_t guild_flag[0x0800];
} PACKED bb_guild_member_flag_setting_pkt;

// 10EA: Delete team
// No arguments
typedef struct bb_guild_dissolve {
    bb_pkt_hdr_t hdr;
    //uint32_t guild_id;
    uint8_t data[];
} PACKED bb_guild_dissolve_pkt;

// 11EA (C->S): Promote team member
// TODO: header.flag is used for this command. Figure out what it's for.
typedef struct bb_guild_member_promote {
    bb_pkt_hdr_t hdr;
    uint32_t target_guildcard;
} PACKED bb_guild_member_promote_pkt;

// 12EA (S->C): Unknown
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

// 13EA: Unknown
// No arguments
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

// 14EA (C->S): Unknown
// No arguments. Client always sends 1 in the header.flag field.
typedef struct bb_guild_member_tittle {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_member_tittle_pkt;

// 15EA (S->C): Unknown
typedef struct bb_guild_full_data {
    bb_pkt_hdr_t hdr;
    uint32_t guildcard;
    uint32_t guild_id;
    uint8_t guild_info[8];
    uint32_t guild_priv_level;
    uint16_t guild_name[0x000E];
    uint32_t guild_rank;
    uint32_t target_guildcard;
    uint32_t client_id;
    uint16_t char_name[BB_CHARACTER_NAME_LENGTH];
    uint8_t guild_flag[0x0800];
    uint32_t guild_rewards[2];
} PACKED bb_guild_full_data_pkt;

// 16EA (S->C): UNKNOW
typedef struct bb_guild_unk_16EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_16EA_pkt;

// 17EA (S->C): UNKNOW
typedef struct bb_guild_unk_17EA {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_unk_17EA_pkt;

// 18EA: Membership information
// No arguments (C->S)
// TODO: Document S->C format
typedef struct bb_guild_buy_privilege_and_point_info {
    bb_pkt_hdr_t hdr;
    uint8_t unk_data[0x000C];
    uint8_t guild_info[8];
    uint32_t guild_priv_level;
    uint32_t guildcard_client;
    uint16_t char_name[BB_CHARACTER_NAME_LENGTH];
} PACKED bb_guild_buy_privilege_and_point_info_pkt;

// 19EA: Privilege list
// No arguments (C->S)
// TODO: Document S->C format
typedef struct bb_guild_privilege_list {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_privilege_list_pkt;

// 1AEA: (S->C): Buy Guild Special Item
typedef struct bb_guild_buy_special_item {
    bb_pkt_hdr_t hdr;
    uint8_t data[];
} PACKED bb_guild_buy_special_item_pkt;

// 1BEA (C->S): Unknown
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

// 01ED
typedef struct bb_options_update_option {
    bb_pkt_hdr_t hdr;
    uint32_t option;
} PACKED bb_options_update_option_pkt;

// 02ED
typedef struct bb_options_update_symbol_chats {
    bb_pkt_hdr_t hdr;
    uint8_t symbol_chats[0x4E0];
} PACKED bb_options_update_symbol_chats_pkt;

// 03ED
typedef struct bb_options_update_chat_shortcuts {
    bb_pkt_hdr_t hdr;
    uint8_t chat_shortcuts[0xA40];
} PACKED bb_options_update_chat_shortcuts_pkt;

// 04ED
typedef struct bb_options_update_key_config {
    bb_pkt_hdr_t hdr;
    uint8_t key_config[0x16C];
} PACKED bb_options_update_key_config_pkt;

// 05ED
typedef struct bb_options_update_pad_config {
    bb_pkt_hdr_t hdr;
    uint8_t pad_config[0x38];
} PACKED bb_options_update_pad_config_pkt;

// 06ED
typedef struct bb_options_update_tech_menu {
    bb_pkt_hdr_t hdr;
    uint8_t tech_menu[0x28];
} PACKED bb_options_update_tech_menu_pkt;

// 07ED
typedef struct bb_options_update_customize {
    bb_pkt_hdr_t hdr;
    uint8_t customize[0xE8];
} PACKED bb_options_update_customize_pkt;

// 08ED
typedef struct bb_options_update_challenge_battle_config {
    bb_pkt_hdr_t hdr;
    uint8_t challenge_battle_config[0x140];
} PACKED bb_options_update_challenge_battle_config_pkt;

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

// EF (S->C): Unknown (BB)
// Has an unknown number of subcommands (00EF, 01EF, etc.)
// Contents are plain text (char).

// F0 (S->C): Unknown (BB)
typedef struct S_Unknown_BB_F0 {
    bb_pkt_hdr_t hdr;
    uint32_t unknown_a1[7];
    uint32_t which; // Must be < 12
    uint16_t unknown_a2[0x10];
    uint32_t unknown_a3;
} PACKED S_Unknown_BB_F0_pkt;

// F1: Invalid command
// F2: Invalid command
// F3: Invalid command
// F4: Invalid command
// F5: Invalid command
// F6: Invalid command
// F7: Invalid command
// F8: Invalid command
// F9: Invalid command
// FA: Invalid command
// FB: Invalid command
// FC: Invalid command
// FD: Invalid command
// FE: Invalid command
// FF: Invalid command

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
#include <pso_cmd_code.h>
#endif /* !PACKETS_H_HAVE_PACKETS */
#endif /* !PACKETS_H_HEADERS_ONLY */
