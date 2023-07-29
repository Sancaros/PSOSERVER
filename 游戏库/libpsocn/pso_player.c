/*
    梦幻之星中国 玩家数据结构
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

#include "pso_player.h"
#include "f_logs.h"
#include <f_iconv.h>

/* Possible values for the version field of ship_client_t */
#define CLIENT_VERSION_DCV1     0
#define CLIENT_VERSION_DCV2     1
#define CLIENT_VERSION_PC       2
#define CLIENT_VERSION_GC       3
#define CLIENT_VERSION_EP3      4
#define CLIENT_VERSION_BB       5
#define CLIENT_VERSION_XBOX     6

int player_bb_name_cpy(psocn_bb_char_name_t* dst, psocn_bb_char_name_t* src) {
    size_t dst_name_len = sizeof(dst->char_name);
    size_t src_name_len = sizeof(src->char_name);

    dst->name_tag = src->name_tag;
    dst->name_tag2 = src->name_tag2;

    memcpy_s(&dst->char_name, dst_name_len, &src->char_name, src_name_len);

    if (!dst->char_name[0]) {
        ERR_LOG("玩家名称 %s 拷贝失败", src->char_name);
        return -1;
    }

    return 0;
}

char* get_player_name(player_t* pl, int version, bool raw) {

    memcpy(&player_name[0], 0, 24);

    switch (version)
    {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:

        return pl->v1.character.dress_data.guildcard_str.string;
        //if (raw)
        //    istrncpy16_raw(ic_utf8_to_gb18030, player_name,
        //        &pl->v1.character.dress_data.guildcard_str.string[0], 24, 16);
        //else
        //    istrncpy16_raw(ic_utf8_to_gb18030, player_name,
        //        &pl->v1.character.dress_data.guildcard_str.string[2], 24, 14);

    case CLIENT_VERSION_BB:
        if (raw)
            istrncpy16_raw(ic_utf16_to_gb18030, player_name,
                &pl->bb.character.name, 24, BB_CHARACTER_NAME_LENGTH);
        else
            istrncpy16_raw(ic_utf16_to_gb18030, player_name,
                &pl->bb.character.name.char_name[0], 24, BB_CHARACTER_CHAR_NAME_LENGTH);
    }

    if (player_name == NULL)
        ERR_LOG("GC %s 玩家名称为空", &pl->v1.character.dress_data.guildcard_str.string[2]);

    return player_name;
}
