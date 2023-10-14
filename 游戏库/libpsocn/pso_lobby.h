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

#ifndef PSO_LOBBY_H
#define PSO_LOBBY_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t type;
    const char* name;
} LobbyTypeMapping;

typedef struct {
    const char* name;
    uint8_t value;
} NameToLobbyTypeMapping;

const char* name_for_lobby_type(uint8_t type);

uint8_t lobby_type_for_name(const char* name);

#endif // !PSO_LOBBY_H
