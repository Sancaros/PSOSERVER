/*
    梦幻之星中国 大厅数据
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

#include "pso_lobby.h"

const LobbyTypeMapping lobby_type_to_name[] = {
    {0x00, "normal"},
    {0x0F, "inormal"},
    {0x10, "ipc"},
    {0x11, "iball"},
    {0x67, "cave2u"},
    {0xD4, "cave1"},
    {0xE9, "planet"},
    {0xEA, "clouds"},
    {0xED, "cave"},
    {0xEE, "jungle"},
    {0xEF, "forest2-2"},
    {0xF0, "forest2-1"},
    {0xF1, "windpower"},
    {0xF2, "overview"},
    {0xF3, "seaside"},
    {0xF4, "fons"},
    {0xF5, "dmorgue"},
    {0xF6, "caelum"},
    {0xF8, "cyber"},
    {0xF9, "boss1"},
    {0xFA, "boss2"},
    {0xFB, "dolor"},
    {0xFC, "dragon"},
    {0xFD, "derolle"},
    {0xFE, "volopt"},
    {0xFF, "darkfalz"}
};

const char* name_for_lobby_type(uint8_t type) {
    static const char* ret = "<未知大厅类型>";
    size_t mappings_count = sizeof(lobby_type_to_name) / sizeof(lobby_type_to_name[0]);

    for (size_t i = 0; i < mappings_count; i++) {
        if (lobby_type_to_name[i].type == type) {
            return lobby_type_to_name[i].name;
        }
    }

    return ret;
}

const NameToLobbyTypeMapping name_to_lobby_type[] = {
    {"normal", 0x00},
    {"inormal", 0x0F},
    {"ipc", 0x10},
    {"iball", 0x11},
    {"cave1", 0xD4},
    {"cave2u", 0x67},
    {"dragon", 0xFC},
    {"derolle", 0xFD},
    {"volopt", 0xFE},
    {"darkfalz", 0xFF},
    {"planet", 0xE9},
    {"clouds", 0xEA},
    {"cave", 0xED},
    {"jungle", 0xEE},
    {"forest2-2", 0xEF},
    {"forest2-1", 0xF0},
    {"windpower", 0xF1},
    {"overview", 0xF2},
    {"seaside", 0xF3},
    {"fons", 0xF4},
    {"dmorgue", 0xF5},
    {"caelum", 0xF6},
    {"cyber", 0xF8},
    {"boss1", 0xF9},
    {"boss2", 0xFA},
    {"dolor", 0xFB},
    {"ravum", 0xFC},
    {"sky", 0xFE},
    {"morgue", 0xFF},
};

uint8_t lobby_type_for_name(const char* name) {
    size_t name_to_lobby_type_count = sizeof(name_to_lobby_type) / sizeof(name_to_lobby_type[0]);

    for (size_t i = 0; i < name_to_lobby_type_count; i++) {
        if (strcmp(name, name_to_lobby_type[i].name) == 0) {
            return name_to_lobby_type[i].value;
        }
    }

    // Check if the name is a valid number
    char* endptr;
    unsigned long x = strtoul(name, &endptr, 0);
    if (*endptr == '\0') {
        if (x < name_to_lobby_type_count) {
            return name_to_lobby_type[x].value;
        }
    }

    return 0x80;
}