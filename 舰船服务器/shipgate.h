/*
    梦幻之星中国 舰船服务器
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

#include <time.h>
#include <inttypes.h>

#ifdef HAVE_SSIZE_T
#undef HAVE_SSIZE_T
#endif

#include <gnutls/gnutls.h>

#ifdef HAVE_SSIZE_T
#undef HAVE_SSIZE_T
#endif

#include <sg_packets.h>

/* Forward declarations. */
struct ship;
struct ship_client;
struct lobby;

#ifndef SHIP_DEFINED
#define SHIP_DEFINED
typedef struct ship ship_t;
#endif

#ifndef SHIP_CLIENT_DEFINED
#define SHIP_CLIENT_DEFINED
typedef struct ship_client ship_client_t;
#endif

#ifndef LOBBY_DEFINED
#define LOBBY_DEFINED
typedef struct lobby lobby_t;
#endif

/* Shipgate connection structure. */
struct shipgate_conn {
    int sock;
    int hdr_read;
    int has_key;

    time_t login_attempt;
    ship_t *ship;

    gnutls_session_t session;

    unsigned char *recvbuf;
    int recvbuf_cur;
    int recvbuf_size;
    shipgate_hdr_t pkt;

    unsigned char *sendbuf;
    int sendbuf_cur;
    int sendbuf_size;
    int sendbuf_start;
};

#ifndef SHIPGATE_CONN_DEFINED
#define SHIPGATE_CONN_DEFINED
typedef struct shipgate_conn shipgate_conn_t;
#endif

/* Attempt to connect to the shipgate. Returns < 0 on error, returns 0 on
   success. */
int shipgate_connect(ship_t *s, shipgate_conn_t *rv);

/* Reconnect to the shipgate if we are disconnected for some reason. */
int shipgate_reconnect(shipgate_conn_t *conn);

/* Clean up a shipgate connection. */
void shipgate_cleanup(shipgate_conn_t *c);

/* Read data from the shipgate. */
int shipgate_process_pkt(shipgate_conn_t *c);

/* Send any piled up data. */
int shipgate_send_pkts(shipgate_conn_t *c);

/* Send a newly opened ship's information to the shipgate. */
int shipgate_send_ship_info(shipgate_conn_t *c, ship_t *ship);

/* Send a client/game count update to the shipgate. */
int shipgate_send_cnt(shipgate_conn_t *c, uint16_t clients, uint16_t games);

/* Forward a Dreamcast packet to the shipgate. */
int shipgate_fw_dc(shipgate_conn_t *c, const void *dcp, uint32_t flags,
                   ship_client_t *req);

/* Forward a PC packet to the shipgate. */
int shipgate_fw_pc(shipgate_conn_t *c, const void *pcp, uint32_t flags,
                   ship_client_t *req);

/* Forward a Blue Burst packet to the shipgate. */
int shipgate_fw_bb(shipgate_conn_t *c, const void *bbp, uint32_t flags,
                   ship_client_t *req);

/* Send a ping packet to the server. */
int shipgate_send_ping(shipgate_conn_t *c, int reply);

/* Send the shipgate a character data save request. */
int shipgate_send_cdata(shipgate_conn_t *c, uint32_t gc, uint32_t slot,
                        const void *cdata, int len, uint32_t block);

/* Send the shipgate a request for character data. */
int shipgate_send_creq(shipgate_conn_t *c, uint32_t gc, uint32_t slot);

/* Send a user login request. */
int shipgate_send_usrlogin(shipgate_conn_t *c, uint32_t gc, uint32_t block,
                           const char *username, const char *password, int tok,
                           uint8_t ver);

/* Send a ban request. */
int shipgate_send_ban(shipgate_conn_t *c, uint16_t type, uint32_t requester,
                      uint32_t target, uint32_t until, const char *msg);

/* Send a friendlist update */
int shipgate_send_friend_del(shipgate_conn_t *c, uint32_t user,
                             uint32_t friend_gc);
int shipgate_send_friend_add(shipgate_conn_t *c, uint32_t user,
                             uint32_t friend_gc, const char *nick);

/* Send a block login/logout */
int shipgate_send_block_login(shipgate_conn_t *c, int on, uint32_t gc,
                              uint32_t block, const char *name);
int shipgate_send_block_login_bb(shipgate_conn_t *c, int on, uint32_t gc, uint8_t slot,
                                 uint32_t block, const uint16_t *name);

/* Send a lobby change packet */
int shipgate_send_lobby_chg(shipgate_conn_t *c, uint32_t user, uint32_t lobby,
                            const char *lobby_name);

/* Send a full client list */
int shipgate_send_clients(shipgate_conn_t *c);

/* Send a kick packet */
int shipgate_send_kick(shipgate_conn_t *c, uint32_t requester, uint32_t user,
                       const char *reason);

/* Send a friend list request packet */
int shipgate_send_frlist_req(shipgate_conn_t *c, uint32_t gc, uint32_t block,
                             uint32_t start);

/* Send a global message packet */
int shipgate_send_global_msg(shipgate_conn_t *c, uint32_t gc,
                             const char *text);

/* Send a user option update packet */
int shipgate_send_user_opt(shipgate_conn_t *c, uint32_t gc, uint32_t block,
                           uint32_t opt, uint32_t len, const uint8_t *data);

/* 请求 Blue Burst 客户端选项数据 */
int shipgate_send_bb_opt_req(shipgate_conn_t *c, uint32_t gc, uint32_t block);

/* 发送 Blue Burst 客户端选项数据至数据库 */
int shipgate_send_bb_opts(shipgate_conn_t *c, ship_client_t *cl);
//
///* 请求 Blue Burst 客户端公会数据 */
//int shipgate_send_bb_guild_req(shipgate_conn_t* c, uint32_t gc, uint32_t block);
//
///* 发送 Blue Burst 客户端公会数据至数据库 */
//int shipgate_send_bb_guild(shipgate_conn_t* c, ship_client_t* cl);

/* Send the shipgate a character data backup request. */
int shipgate_send_cbkup(shipgate_conn_t *c, sg_char_bkup_pkt* info, const void *cdata, int len);

/* Send the shipgate a request for character backup data. */
int shipgate_send_cbkup_req(shipgate_conn_t *c, sg_char_bkup_pkt* info);

/* Send a monster kill count update */
int shipgate_send_mkill(shipgate_conn_t *c, uint32_t gc, uint32_t block,
                        ship_client_t *cl, lobby_t *l);

/* Send a script data packet */
int shipgate_send_sdata(shipgate_conn_t *c, ship_client_t *sc, uint32_t event,
                        const uint8_t *data, uint32_t len);

/* Send a quest flag request or update */
int shipgate_send_qflag(shipgate_conn_t *c, ship_client_t *sc, int set,
                        uint32_t fid, uint32_t qid, uint32_t value,
                        uint32_t ctl);

#endif /* !SHIPGATE_H */
