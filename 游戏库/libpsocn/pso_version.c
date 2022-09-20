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

#include <stdio.h>
#include <string.h>

#include <f_logs.h>

#include "pso_version.h"

uint16_t flags_for_version(game_ver_e version, int64_t sub_version) {
    switch (sub_version) {
    case -1: // Initial check (before sub_version recognition)
        switch (version) {
        case DC:
            return NO_MESSAGE_BOX_CLOSE_CONFIRMATION;
        case GC:
        case XB:
            return 0;
        case PC:
            return NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
                SEND_FUNCTION_CALL_CHECKSUM_ONLY;
        case PATCH:
            return NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
                DOES_NOT_SUPPORT_SEND_FUNCTION_CALL;
        case BB:
            return NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
                SAVE_ENABLED;
        }
        break;

        // TODO: Which other sub_versions of DC v1 and v2 exist?
    case 0x21: // DCv1 US
        return DCV1 |
            NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
            DOES_NOT_SUPPORT_SEND_FUNCTION_CALL;

    case 0x26: // DCv2 US
        return NO_MESSAGE_BOX_CLOSE_CONFIRMATION;

    case 0x29: // PC
        return NO_MESSAGE_BOX_CLOSE_CONFIRMATION |
            SEND_FUNCTION_CALL_CHECKSUM_ONLY;

    case 0x30: // GC Ep1&2 JP v1.02, at least one version of PSO XB
    case 0x31: // GC Ep1&2 US v1.00, GC US v1.01, GC EU v1.00, GC JP v1.00
    case 0x34: // GC Ep1&2 JP v1.03
        return 0;
    case 0x32: // GC Ep1&2 EU 50Hz
    case 0x33: // GC Ep1&2 EU 60Hz
        return NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN;
    case 0x35: // GC Ep1&2 JP v1.04 (Plus)
        return NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
            ENCRYPTED_SEND_FUNCTION_CALL;
    case 0x36: // GC Ep1&2 US v1.02 (Plus)
    case 0x39: // GC Ep1&2 JP v1.05 (Plus)
        return NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
            DOES_NOT_SUPPORT_SEND_FUNCTION_CALL;

    case 0x40: // GC Ep3 trial
        return NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
            EPISODE_3;
    case 0x42: // GC Ep3 JP
        return NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
            EPISODE_3 |
            ENCRYPTED_SEND_FUNCTION_CALL;
    case 0x41: // GC Ep3 US
    case 0x43: // GC Ep3 EU
        return NO_MESSAGE_BOX_CLOSE_CONFIRMATION_AFTER_LOBBY_JOIN |
            EPISODE_3 |
            DOES_NOT_SUPPORT_SEND_FUNCTION_CALL;
    }
    ERR_LOG("未知 sub_version");
    return 0;
}

const char* name_for_version(game_ver_e version) {
    switch (version) {
    case GC:
        return "GC";
    case XB:
        return "XB";
    case PC:
        return "PC";
    case BB:
        return "BB";
    case DC:
        return "DC";
    case PATCH:
        return "Patch";
    default:
        return "Unknown";
    }
}

game_ver_e version_for_name(const char* name) {
    if (!strcmp(name, "DC") || !strcmp(name, "DreamCast")) {
        return DC;
    }
    else if (!strcmp(name, "PC")) {
        return PC;
    }
    else if (!strcmp(name, "GC") || !strcmp(name, "GameCube")) {
        return GC;
    }
    else if (!strcmp(name, "XB") || !strcmp(name, "Xbox")) {
        return XB;
    }
    else if (!strcmp(name, "BB") || !strcmp(name, "BlueBurst") ||
        !strcmp(name, "Blue Burst")) {
        return BB;
    }
    else if (!strcmp(name, "Patch")) {
        return PATCH;
    }
    else {
        ERR_LOG("无效版本名称");
        return 0xFF;
    }
}

const char* name_for_server_behavior(server_behavior_e behavior) {
    switch (behavior) {
    case PC_CONSOLE_DETECT:
        return "pc_console_detect";
    case LOGIN_SERVER:
        return "auth_server";
    case LOBBY_SERVER:
        return "lobby_server";
    case DATA_SERVER_BB:
        return "data_server_bb";
    case PATCH_SERVER_PC:
        return "patch_server_pc";
    case PATCH_SERVER_BB:
        return "patch_server_bb";
    case PROXY_SERVER:
        return "proxy_server";
    default:
        ERR_LOG("无效服务器行为");
        return NULL;
    }
}

server_behavior_e server_behavior_for_name(const char* name) {
    if (!strcmp(name, "pc_console_detect")) {
        return PC_CONSOLE_DETECT;
    }
    else if (!strcmp(name, "auth_server") || !strcmp(name, "auth")) {
        return LOGIN_SERVER;
    }
    else if (!strcmp(name, "lobby_server") || !strcmp(name, "lobby")) {
        return LOBBY_SERVER;
    }
    else if (!strcmp(name, "data_server_bb") || !strcmp(name, "data_server") || !strcmp(name, "data")) {
        return DATA_SERVER_BB;
    }
    else if (!strcmp(name, "patch_server_pc") || !strcmp(name, "patch_pc")) {
        return PATCH_SERVER_PC;
    }
    else if (!strcmp(name, "patch_server_bb") || !strcmp(name, "patch_bb")) {
        return PATCH_SERVER_BB;
    }
    else if (!strcmp(name, "proxy_server") || !strcmp(name, "proxy")) {
        return PROXY_SERVER;
    }
    else {
        ERR_LOG("服务器行为名称不正确");
        return 0xFF;
    }
}
