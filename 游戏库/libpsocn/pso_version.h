/*
    梦幻之星中国 中国服务器
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

#include <inttypes.h>

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

static uint64_t CLIENT_CONFIG_MAGIC = LE64(0x492A890E82AC9839);

typedef enum game_ver{
    DC = 0,
    PC,
    PATCH,
    GC,
    XB,
    BB,
}game_ver_e;

typedef enum server_behavior{
    PC_CONSOLE_DETECT = 0,
    LOGIN_SERVER,
    LOBBY_SERVER,
    DATA_SERVER_BB,
    PATCH_SERVER_PC,
    PATCH_SERVER_BB,
    PROXY_SERVER,
}server_behavior_e;

enum Flag {
    // For patch server clients, client is Blue Burst rather than PC
    BB_PATCH = 0x0001,
    // After joining a lobby, client will no longer send D6 commands when they
    // close message boxes
    NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN = 0x0002,
    // Client has the above flag and has already joined a lobby, or is not GC
    NO_MESSAGE_BOX_CLOSE_CONFIRMATION = 0x0004,
    // Client is Episode 3, should be able to see CARD lobbies, and should only
    // be able to see/join games with the IS_EPISODE_3 flag
    EPISODE_3 = 0x0008,
    // Client is DC v1 (disables some features)
    DCV1 = 0x0010,
    // Client is loading into a game
    LOADING = 0x0020,
    // Client is loading a quest
    LOADING_QUEST = 0x0040,
    // Client is in the information menu (login server only)
    IN_INFORMATION_MENU = 0x0080,
    // Client is at the welcome message (login server only)
    AT_WELCOME_MESSAGE = 0x0100,
    // Client disconnects if it receives B2 (send_function_call)
    DOES_NOT_SUPPORT_SEND_FUNCTION_CALL = 0x0200,
    // Client has already received a 97 (enable saves) command, so don't show
    // the programs menu anymore
    SAVE_ENABLED = 0x0400,
    // Client requires doubly-encrypted code section in send_function_call
    ENCRYPTED_SEND_FUNCTION_CALL = 0x0800,
    // Client supports send_function_call but does not actually run the code
    SEND_FUNCTION_CALL_CHECKSUM_ONLY = 0x1000,
    // Client is GC Trial Edition, and therefore uses V2 encryption instead of
    // V3, and doesn't support some commands
    GC_TRIAL_EDITION = 0x2000,
};

uint16_t flags_for_version(game_ver_e version, int64_t sub_version);
const char* name_for_version(game_ver_e version);
game_ver_e version_for_name(const char* name);

const char* name_for_server_behavior(server_behavior_e behavior);
server_behavior_e server_behavior_for_name(const char* name);
