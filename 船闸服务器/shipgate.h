/*
    梦幻之星中国 船闸服务器
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

#ifndef SHIPGATE_H
#define SHIPGATE_H

#include <stdint.h>

#include <pso_player.h>

#include "ship.h"
#include "packets.h"
#include "scripts.h"

/* Minimum and maximum supported protocol ship<->shipgate protocol versions */
#define SHIPGATE_MINIMUM_PROTO_VER 12
#define SHIPGATE_MAXIMUM_PROTO_VER 19

#define SGATE_SERVER_VERSION VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

//#define VERSION_MAJOR 0;
//#define VERSION_MINOR 1;
//#define VERSION_MICRO 0;
//


/* Send a welcome packet to the given ship. */
int send_welcome(ship_t *c);

/* Forward a Dreamcast packet to the given ship, with additional metadata. */
int forward_dreamcast(ship_t *c, dc_pkt_hdr_t *pkt, uint32_t sender,
                      uint32_t gc, uint32_t block);

/* Forward a PC packet to the given ship like the above function. */
int forward_pc(ship_t *c, dc_pkt_hdr_t *pc, uint32_t sender, uint32_t gc,
               uint32_t block);

/* Forward a Blue Burs packet to the given ship, like the above. */
int forward_bb(ship_t *c, bb_pkt_hdr_t *bb, uint32_t sender, uint32_t gc,
               uint32_t block);

/* Send a ship up/down message to the given ship. */
int send_ship_status(ship_t *c, ship_t *o, uint16_t status);

/* Send a ping packet to a client. */
int send_ping(ship_t *c, int reply);

/* Send the ship a character data restore. */
int send_cdata(ship_t *c, uint32_t gc, uint32_t slot, void *cdata, int sz,
               uint32_t block);

/* Send a reply to a user login request. */
int send_usrloginreply(ship_t *c, uint32_t gc, uint32_t block, int good,
                       uint32_t p);

/* Send a client/game update packet. */
int send_counts(ship_t *c, uint32_t ship_id, uint16_t clients, uint16_t games);

/* Send an error packet to a ship */
int send_error(ship_t *c, uint16_t type, uint16_t flags, uint32_t err,
               const uint8_t *data, int data_sz);

/* Send a packet to tell a client that a friend has logged on or off */
int send_friend_message(ship_t *c, int on, uint32_t dest_gc,
                        uint32_t dest_block, uint32_t friend_gc,
                        uint32_t friend_block, uint32_t friend_ship,
                        const char *friend_name, const char *nickname);

/* Send a kick packet */
int send_kick(ship_t *c, uint32_t requester, uint32_t user, uint32_t block,
              const char *reason);

/* Send a portion of a user's friendlist to the user */
int send_friendlist(ship_t *c, uint32_t requester, uint32_t block,
                    int count, const friendlist_data_t *entries);

/* Send a global message packet to a ship */
int send_global_msg(ship_t *c, uint32_t requester, const char *text,
                    uint16_t len);

/* Begin an options packet */
void *user_options_begin(uint32_t guildcard, uint32_t block);

/* Append an option value to the options packet */
void *user_options_append(void *p, uint32_t opt, uint32_t len,
                          const uint8_t *data);

/* Finish off a user options packet and send it along */
int send_user_options(ship_t *c);

/* 发送客户端 Blue Burst 选项数据 */
int send_bb_opts(ship_t *c, uint32_t gc, uint32_t block,
                 psocn_bb_db_opts_t *opts, psocn_bb_db_guild_t* guild);
//
///* 发送客户端 Blue Burst 公会数据 */
//int send_bb_guild(ship_t *c, uint32_t gc, uint32_t block,
//                 psocn_bb_db_guild_t *guild);

/* Send a system-generated simple mail message. */
int send_simple_mail(ship_t *c, uint32_t gc, uint32_t block, uint32_t sender,
                     const char *name, const char *msg);

/* Send a script check packet. */
int send_script_check(ship_t *c, ship_script_t *scr);

/* Send a script to a ship. */
int send_script(ship_t *c, ship_script_t *scr);

/* Send a script setup packet to a ship. */
int send_sset(ship_t *c, uint32_t action, ship_script_t *scr);

/* Send a script data packet to a ship. */
int send_sdata(ship_t *c, uint32_t gc, uint32_t block, uint32_t event,
               const uint8_t *data, uint32_t len);

/* Send a quest flag response */
int send_qflag(ship_t *c, uint16_t type, uint32_t gc, uint32_t block,
               uint32_t fid, uint32_t qid, uint32_t value, uint32_t ctl);

/* Send a simple ship control request */
int send_sctl(ship_t *c, uint32_t ctl, uint32_t acc);

/* Send a shutdown/restart request */
int send_shutdown(ship_t *c, int restart, uint32_t acc, uint32_t when);

/* Begin a blocklist packet */
void user_blocklist_begin(uint32_t guildcard, uint32_t block);

/* Append a value to the blocklist packet */
void user_blocklist_append(uint32_t gc, uint32_t flags);

/* Finish off a user blocklist packet and send it along */
int send_user_blocklist(ship_t *c);

/* Send an error response to a user */
int send_user_error(ship_t *c, uint16_t pkt_type, uint32_t err_code,
                    uint32_t gc, uint32_t block, const char *message);

/* 发送BB职业最大法术数据至舰船 */
int send_max_tech_lvl_bb(ship_t* c, bb_max_tech_level_t* data);

/* 发送BB职业等级数据至舰船 */
int send_pl_lvl_data_bb(ship_t* c, bb_level_table_t* data);

#endif /* !SHIPGATE_H */
