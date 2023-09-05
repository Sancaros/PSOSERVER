/*
    梦幻之星中国 认证服务器
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

#ifndef LOGIN_PACKETS_H
#define LOGIN_PACKETS_H

#include <stdint.h>
#include <WinSock_Defines.h>

#include <encryption.h>
#include <quest.h>

#include "auth.h"
#include <pso_player.h>
#include "packets.h" //登录数据结构
#include "pso_menu.h"

typedef u_long in_addr_t;

/* This must be placed into the copyright field in the BB welcome packet. */
const static char login_bb_game_server_copyright[] =
    "Phantasy Star Online Blue Burst Game Server. Copyright 1999-2004 SONICTEAM.";

/* This must be placed into the pm copyright field in the BB welcome packet. */
const static char login_bb_pm_server_copyright[] =
    "PSO NEW PM Server. Copyright 1999-2002 SONICTEAM.";

/* This must be placed into the copyright field in the DC welcome packet. */
const static char login_dc_welcome_copyright[] =
    "DreamCast Port Map. Copyright SEGA Enterprises. 1999";

const static char anti_copyright[] = "This server is in no way affiliated, sponsored, "
"or supported by SEGA Enterprises or SONICTEAM. "
"The preceding message exists only in order to remain compatible with programs that expect it.";

/* Send a Dreamcast Welcome packet to the given client. */
int send_dc_welcome(login_client_t *c, uint32_t svect, uint32_t cvect);

/* Send a Blue Burst Welcome packet to the given client. */
int send_bb_welcome(login_client_t* c, const uint8_t svect[48],
                    const uint8_t cvect[48], uint8_t flags);

/* Send a large message packet to the given client. */
int send_large_msg(login_client_t *c, const char* fmt, ...);

/* Send a scrolling message packet to the given client. */
int send_scroll_msg(login_client_t *c, const char* fmt, ...);

/* Send the Dreamcast security packet to the given client. */
int send_dc_security(login_client_t *c, uint32_t gc, const void *data,
                     int data_len);

/* Send the Blue Burst security packet to the given client. */
int send_bb_security(login_client_t *c, uint32_t gc, uint32_t err,
                     uint32_t guild_id, const void *data, int data_len);

/* Send a redirect packet to the given client. */
int send_redirect(login_client_t *c, in_addr_t ip, uint16_t port);

#ifdef PSOCN_ENABLE_IPV6
/* Send a redirect packet (IPv6) to the given client. */
int send_redirect6(login_client_t *c, const uint8_t ip[16], uint16_t port);
#endif

/* Send a packet to clients connecting on the Gamecube port to sort out any PC
   clients that might end up there. This must be sent before encryption is set
   up! */
int send_selective_redirect(login_client_t *c);

/* Send a timestamp packet to the given client. */
int send_timestamp(login_client_t *c);

/* Send the initial menu to clients, with the options of "Ship Select" and
   "Download". */
int send_initial_menu(login_client_t *c);

/* Send the list of ships to the client. */
int send_ship_list(login_client_t *c, uint16_t menu_code);

/* Send a information reply packet to the client. */
int send_info_reply(login_client_t *c, const char* fmt, ...);

/* Send a simple (header-only) packet to the client. */
int send_simple(login_client_t *c, int type, int flags);

/* Send the quest list to a client. */
int send_quest_list(login_client_t *c, psocn_quest_category_t *l);

/* Send a quest to a client. */
int send_quest(login_client_t *c, psocn_quest_t *q);

/* Send an Episode 3 rank update to a client. */
int send_ep3_rank_update(login_client_t *c);

/* Send the Episode 3 card list to a client. */
int send_ep3_card_update(login_client_t *c);

/* Send a Blue Burst option reply to the client. */
int send_bb_option_reply(login_client_t *c, bb_key_config_t keys, bb_guild_t guild_data);

/* Send a Blue Burst character acknowledgement to the client. */
int send_bb_char_ack(login_client_t *c, uint8_t slot, uint8_t code);

/* Send a Blue Burst checksum acknowledgement to the client. */
int send_bb_checksum_ack(login_client_t *c, uint32_t ack);

/* Send a Blue Burst guildcard header packet. */
int send_bb_guild_header(login_client_t *c, uint32_t checksum);

/* Send a Blue Burst guildcard chunk packet. */
int send_bb_guildcard_chunk(login_client_t *c, uint32_t chunk_index);

/* Send a prepared Blue Burst packet. */
int send_bb_pkt(login_client_t *c, bb_pkt_hdr_t *hdr);

/* 发送角色更衣室数据 用于更新角色 */
int send_bb_char_dressing_room(psocn_bb_char_t *c, psocn_bb_mini_char_t *mc);

/* Send a Blue Burst character preview packet. */
int send_bb_char_preview(login_client_t *c, const psocn_bb_mini_char_t *mc,
                         uint8_t slot);

/* Send the content of the "Information" menu. */
int send_info_list(login_client_t *c);

/* Send a message box packet to the user. */
int send_message_box(login_client_t *c, const char *fmt, ...);

/* Send a message box containing an information file entry. */
int send_info_file(login_client_t *c, uint32_t entry);

/* Send a text message to the client (i.e, for stuff related to commands). */
int send_msg(login_client_t* c, uint16_t type, const char* fmt, ...);

/* Send the GM operations menu to the user. */
int send_gm_menu(login_client_t *c);

/* Send the message of the day to the given client. */
int send_motd(login_client_t *c);

/* Send the long description of a quest to the given client. */
int send_quest_description(login_client_t *c, psocn_quest_t *q);

/* Detect what version of PSO the client is on. */
int send_dc_version_detect(login_client_t *c);
int send_gc_version_detect(login_client_t *c);

/* Send a single patch to a client. */
int send_single_patch_dc(login_client_t *c, const patchset_t *p);
int send_single_patch_gc(login_client_t *c, const patchset_t *p);

/* Send the menu of all available patches to the client. */
int send_patch_menu(login_client_t *c);

/* Send a request for the client to CRC some part of its memory space. */
int send_crc_check(login_client_t *c, uint32_t start, uint32_t count);

int send_disconnect(login_client_t* c, uint32_t flags);

#endif /* !LOGIN_PACKETS_H */
