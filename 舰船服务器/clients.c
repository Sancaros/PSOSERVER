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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <time.h>
#include <errno.h>
#include <math.h>
#include <WinSock_Defines.h>

//#include <mtwist.h>
#include <SFMT.h>
#include <encryption.h>
#include <f_logs.h>
#include <psomemory.h>

#include "ship.h"
#include "utils.h"
#include "clients.h"
#include "ship_packets.h"
#include "scripts.h"
#include "subcmd.h"
#include "subcmd_send.h"
#include "mapdata.h"
#include "handle_player_items.h"

#ifdef ENABLE_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif
#include "mag_bb.h"
#include <pso_items_coren_reward_list.h>

/* Player levelup data */
extern bb_level_table_t bb_char_stats;
extern v2_level_table_t v2_char_stats;
extern bb_max_tech_level_t max_tech_level[MAX_PLAYER_TECHNIQUES];

#ifdef UNUSED
#undef UNUSED
#endif

#define UNUSED __attribute__((unused))

const char* get_server_DayOfWeek_desc(void) {
    // 获取当前系统时间
    SYSTEMTIME time;
    GetLocalTime(&time);
    // 获取当前是星期几（星期天 = 0, 星期一 = 1, 星期二 = 2, ..., 星期六 = 6）
    // 将数字转换为对应的星期几文本
    const char* weekDays[] = { "星期天", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六" };
    return weekDays[time.wDayOfWeek];
}

/* The key for accessing our thread-specific receive buffer. */
pthread_key_t recvbuf_key;

/* The key for accessing our thread-specific send buffer. */
pthread_key_t sendbuf_key;

/* Destructor for the thread-specific receive buffer */
static void buf_dtor(void* rb) {
    free_safe(rb);
}

/* Initialize the clients system, allocating any thread specific keys */
int client_init(psocn_ship_t* cfg) {
    if (pthread_key_create(&recvbuf_key, &buf_dtor)) {
        perror("pthread_key_create");
        return -1;
    }

    if (pthread_key_create(&sendbuf_key, &buf_dtor)) {
        perror("pthread_key_create");
        return -1;
    }

    return 0;
}

/* Clean up the clients system. */
void client_shutdown(void) {
    pthread_key_delete(recvbuf_key);
    pthread_key_delete(sendbuf_key);
}

/* Create a new connection, storing it in the list of clients. */
ship_client_t* client_create_connection(int sock, int version, int type,
    struct client_queue* clients,
    ship_t* ship, block_t* block,
    struct sockaddr* ip, socklen_t size) {
    __try {
        uint32_t client_seed_dc, server_seed_dc;
        uint8_t client_seed_bb[48] = { 0 }, server_seed_bb[48] = { 0 };
        int i;
        pthread_mutexattr_t attr;

        ship_client_t* rv = (ship_client_t*)malloc(sizeof(ship_client_t));
        if (rv == NULL || !rv) {
            // 内存分配失败的处理逻辑...
            ERR_LOG("client_create_connection 分配新客户端内存错误");
            return NULL;
        }
        else {
            memset(rv, 0, sizeof(ship_client_t));  // 将分配的内存清零
            // 其他初始化操作...
        }

        /* Disable Nagle's algorithm */
        i = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (PCHAR)&i, sizeof(int)) < 0) {
            ERR_LOG("setsockopt");
        }

        /* For the DC versions, set up friendly receive buffers that should ensure
           that we don't try to negotiate window scaling.
           XXXX: Should we do this on GC too? It definitely shouldn't be needed on
                 PC/BB, and most likely not on Xbox either. */
        switch (version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
            i = 32767;
            if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (PCHAR)&i, sizeof(int)) < 0) {
                ERR_LOG("setsockopt");
            }
            break;
        }

        /* 当玩家进入舰仓时 初始化所有数据 */
        if (type == CLIENT_TYPE_BLOCK) {
            rv->game_data =
                (client_game_data_t*)malloc(sizeof(client_game_data_t));

            if (!rv->game_data) {
                ERR_LOG("malloc");
                free_safe(rv);
                closesocket(sock);
                return NULL;
            }

            memset(rv->game_data, 0, sizeof(client_game_data_t));

            rv->enemy_kills = (uint32_t*)malloc(sizeof(uint32_t) * 0x60);

            if (!rv->enemy_kills) {
                ERR_LOG("malloc");
                free_safe(rv->game_data);
                free_safe(rv);
                closesocket(sock);
                return NULL;
            }

            memset(rv->enemy_kills, 0, sizeof(uint32_t) * 0x60);

            rv->common_bank =
                (psocn_bank_t*)malloc(sizeof(psocn_bank_t));

            if (!rv->common_bank) {
                free_safe(rv->enemy_kills);
                free_safe(rv->game_data);
                free_safe(rv);
                closesocket(sock);
                return NULL;
            }

            memset(rv->common_bank, 0, sizeof(psocn_bank_t));

            rv->char_bank =
                (psocn_bank_t*)malloc(sizeof(psocn_bank_t));

            if (!rv->char_bank) {
                ERR_LOG("malloc");
                free_safe(rv->enemy_kills);
                free_safe(rv->game_data);
                free_safe(rv->common_bank);
                free_safe(rv);
                closesocket(sock);
                return NULL;
            }

            memset(rv->char_bank, 0, sizeof(psocn_bank_t));

            rv->pl = (player_t*)malloc(sizeof(player_t));

            if (!rv->pl) {
                ERR_LOG("malloc");
                free_safe(rv->enemy_kills);
                free_safe(rv->game_data);
                free_safe(rv->common_bank);
                free_safe(rv->char_bank);
                free_safe(rv);
                closesocket(sock);
                return NULL;
            }

            memset(rv->pl, 0, sizeof(player_t));

            rv->mode_pl =
                (psocn_mode_char_t*)malloc(sizeof(psocn_mode_char_t));

            if (!rv->mode_pl) {
                ERR_LOG("malloc");
                free_safe(rv->enemy_kills);
                free_safe(rv->game_data);
                free_safe(rv->common_bank);
                free_safe(rv->char_bank);
                free_safe(rv->pl);
                free_safe(rv);
                closesocket(sock);
                return NULL;
            }

            memset(rv->mode_pl, 0, sizeof(psocn_mode_char_t));

            rv->records =
                (record_data_t*)malloc(sizeof(record_data_t));

            if (!rv->records) {
                ERR_LOG("malloc");
                free_safe(rv->game_data);
                free_safe(rv->enemy_kills);
                free_safe(rv->common_bank);
                free_safe(rv->char_bank);
                free_safe(rv->pl);
                free_safe(rv->mode_pl);
                free_safe(rv);
                closesocket(sock);
                return NULL;
            }

            memset(rv->records, 0, sizeof(record_data_t));

            if (version == CLIENT_VERSION_BB) {
                rv->bb_pl =
                    (psocn_bb_db_char_t*)malloc(PSOCN_STLENGTH_BB_DB_CHAR);

                if (!rv->bb_pl) {
                    ERR_LOG("malloc");
                    free_safe(rv->game_data);
                    free_safe(rv->enemy_kills);
                    free_safe(rv->common_bank);
                    free_safe(rv->char_bank);
                    free_safe(rv->pl);
                    free_safe(rv->mode_pl);
                    free_safe(rv->records);
                    free_safe(rv);
                    closesocket(sock);
                    return NULL;
                }

                memset(rv->bb_pl, 0, PSOCN_STLENGTH_BB_DB_CHAR);

                rv->bb_opts =
                    (psocn_bb_db_opts_t*)malloc(PSOCN_STLENGTH_BB_DB_OPTS);

                if (!rv->bb_opts) {
                    ERR_LOG("malloc");
                    free_safe(rv->common_bank);
                    free_safe(rv->char_bank);
                    free_safe(rv->pl);
                    free_safe(rv->enemy_kills);
                    free_safe(rv->game_data);
                    free_safe(rv->mode_pl);
                    free_safe(rv->records);
                    free_safe(rv->bb_pl);
                    free_safe(rv);
                    closesocket(sock);
                    return NULL;
                }

                memset(rv->bb_opts, 0, PSOCN_STLENGTH_BB_DB_OPTS);

                rv->bb_guild =
                    (psocn_bb_db_guild_t*)malloc(PSOCN_STLENGTH_BB_GUILD);

                if (!rv->bb_guild) {
                    ERR_LOG("malloc");
                    free_safe(rv->common_bank);
                    free_safe(rv->char_bank);
                    free_safe(rv->pl);
                    free_safe(rv->enemy_kills);
                    free_safe(rv->game_data);
                    free_safe(rv->mode_pl);
                    free_safe(rv->records);
                    free_safe(rv->bb_pl);
                    free_safe(rv->bb_opts);
                    free_safe(rv);
                    closesocket(sock);
                    return NULL;
                }

                memset(rv->bb_guild, 0, PSOCN_STLENGTH_BB_GUILD);
            }
            else if (version == CLIENT_VERSION_XBOX) {

                rv->xbl_ip = (xbox_ip_t*)malloc(sizeof(xbox_ip_t));

                if (!rv->xbl_ip) {
                    ERR_LOG("malloc");
                    free_safe(rv->common_bank);
                    free_safe(rv->char_bank);
                    free_safe(rv->pl);
                    free_safe(rv->enemy_kills);
                    free_safe(rv->game_data);
                    free_safe(rv->mode_pl);
                    free_safe(rv->records);
                    free_safe(rv);
                    closesocket(sock);
                    return NULL;
                }

                memset(rv->xbl_ip, 0, sizeof(xbox_ip_t));
            }
        }

        /* 将基础参数存储进客户端的结构. */
        rv->sock = sock;
        rv->version = version;
        rv->cur_block = block;
        rv->arrow_color = 1;
        rv->last_message = rv->login_time = time(NULL);
        rv->hdr_size = 4;

        rv->isvip = false;
        rv->can_chat = true;
        rv->mode = 0;
        rv->need_save_data = false;
        rv->bank_type = false;

        /* Create the mutex */
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&rv->mutex, &attr);
        pthread_mutexattr_destroy(&attr);

        memcpy(&rv->ip_addr, ip, size);

        if (ip->sa_family == AF_INET6) {
            rv->flags |= CLIENT_FLAG_IPV6;
        }

        /* Make sure any packets sent early bail... */
        rv->ckey.type = 0xFF;
        rv->skey.type = 0xFF;

        /* TODO 客户端是否也要自带随机数 */
        uint32_t rng_seed = (uint32_t)(time(NULL) ^ sock);
        sfmt_init_gen_rand(&rv->sfmt_rng, rng_seed);
        sfmt_t* rng/* = &rv->sfmt_rng*/;

        if (type == CLIENT_TYPE_SHIP) {
            rv->flags |= CLIENT_FLAG_TYPE_SHIP;
            rng = &ship->sfmt_rng;
        }
        //else if (type == CLIENT_TYPE_BLOCK) {
        //    rng = &block->sfmt_rng;
        //}
        else {
            rng = &block->sfmt_rng;
            //rng = &rv->sfmt_rng;
        }

#ifdef ENABLE_LUA
        /* Initialize the script table */
        lua_newtable(ship->lstate);
        rv->script_ref = luaL_ref(ship->lstate, LUA_REGISTRYINDEX);
#endif

        switch (version) {
        case CLIENT_VERSION_DCV1:
        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
            /* Generate the encryption keys for the client and server. */
            client_seed_dc = sfmt_genrand_uint32(rng);
            server_seed_dc = sfmt_genrand_uint32(rng);

            CRYPT_CreateKeys(&rv->skey, &server_seed_dc, CRYPT_PC);
            CRYPT_CreateKeys(&rv->ckey, &client_seed_dc, CRYPT_PC);

            /* Send the client the welcome packet, or die trying. */
            if (send_dc_welcome(rv, server_seed_dc, client_seed_dc)) {
                goto err;
            }

            break;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_EP3:
        case CLIENT_VERSION_XBOX:
            /* Generate the encryption keys for the client and server. */
            client_seed_dc = sfmt_genrand_uint32(rng);
            server_seed_dc = sfmt_genrand_uint32(rng);

            CRYPT_CreateKeys(&rv->skey, &server_seed_dc, CRYPT_GAMECUBE);
            CRYPT_CreateKeys(&rv->ckey, &client_seed_dc, CRYPT_GAMECUBE);

            /* Send the client the welcome packet, or die trying. */
            if (send_dc_welcome(rv, server_seed_dc, client_seed_dc)) {
                goto err;
            }

            break;

        case CLIENT_VERSION_BB:
            /* Generate the encryption keys for the client and server. */
            for (i = 0; i < 48; i += 4) {
                client_seed_dc = sfmt_genrand_uint32(rng);
                server_seed_dc = sfmt_genrand_uint32(rng);

                client_seed_bb[i + 0] = (uint8_t)(client_seed_dc >> 0);
                client_seed_bb[i + 1] = (uint8_t)(client_seed_dc >> 8);
                client_seed_bb[i + 2] = (uint8_t)(client_seed_dc >> 16);
                client_seed_bb[i + 3] = (uint8_t)(client_seed_dc >> 24);
                server_seed_bb[i + 0] = (uint8_t)(server_seed_dc >> 0);
                server_seed_bb[i + 1] = (uint8_t)(server_seed_dc >> 8);
                server_seed_bb[i + 2] = (uint8_t)(server_seed_dc >> 16);
                server_seed_bb[i + 3] = (uint8_t)(server_seed_dc >> 24);
            }

            CRYPT_CreateKeys(&rv->skey, server_seed_bb, CRYPT_BLUEBURST);
            CRYPT_CreateKeys(&rv->ckey, client_seed_bb, CRYPT_BLUEBURST);
            rv->hdr_size = 8;

            /* Send the client the welcome packet, or die trying. */
            if (send_bb_welcome(rv, server_seed_bb, client_seed_bb)) {
                goto err;
            }

            break;
        }

        /* Insert it at the end of our list, and we're done. */
        if (type == CLIENT_TYPE_BLOCK) {
            pthread_rwlock_wrlock(&block->lock);
            TAILQ_INSERT_TAIL(clients, rv, qentry);
            ++block->num_clients;
            pthread_rwlock_unlock(&block->lock);
        }
        else {
            TAILQ_INSERT_TAIL(clients, rv, qentry);
        }

        /* 加载客户端其他函数 */
        rv->pkt_size = pkt_size;

        ship_inc_clients(ship);

        return rv;

    err:
#ifdef ENABLE_LUA
        /* Remove the table from the registry */
        luaL_unref(ship->lstate, LUA_REGISTRYINDEX, rv->script_ref);
#endif

        closesocket(sock);

        if (type == CLIENT_TYPE_BLOCK) {
            if (version == CLIENT_VERSION_XBOX) {
                free_safe(rv->xbl_ip);
            }

            free_safe(rv->enemy_kills);
            free_safe(rv->pl);
        }

        pthread_mutex_destroy(&rv->mutex);

        free_safe(rv);
        return NULL;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return NULL;
    }
}

void fix_bb_player_db_save_data(psocn_bb_db_char_t* character) {


}

/* 人物数据检测修正函数 */
bool check_bb_pl_data(ship_client_t* c) {
    if (isPacketEmpty(c->bb_pl->character.dress_data.gc_string, sizeof(c->bb_pl->character.dress_data.gc_string))) {
        ERR_LOG("%s 更新的数据有误 %s", get_player_describe(c), c->bb_pl->character.dress_data.gc_string);
        return false;
    }

    return true;
}

void client_send_bb_data(ship_client_t* c) {
    int i = 0;
    time_t now = time(NULL);

    uint32_t num_seconds = (uint32_t)now - (uint32_t)c->save_time;

    /* If the client was on Blue Burst, update their db character */
    if (c->version == CLIENT_VERSION_BB &&
        !(c->flags & CLIENT_FLAG_TYPE_SHIP) && check_bb_pl_data(c)) {

        c->need_save_data = false;

        /* 将游戏时间存储入人物数据 */
        c->bb_pl->character.play_time += (uint32_t)now - (uint32_t)c->login_time;

        c->save_time = now;

#ifdef DEBUG
        DBG_LOG("mode %d", c->mode);
        fix_client_inv(&c->bb_pl->character.inv);
        fix_equip_item(&c->bb_pl->character.inv);

#endif // DEBUG

        if (!c->mode) {
            /* 将玩家数据存入数据库 */
            fix_player_max_tech_level(&c->bb_pl->character);

            if (shipgate_send_cdata(&ship->sg, c->guildcard, c->sec_data.slot,
                c->bb_pl, PSOCN_STLENGTH_BB_DB_CHAR,
                c->cur_block->b)) {
                send_msg(c, BB_SCROLL_MSG_TYPE, "%s", __(c, "\tE\tC4存储数据失败~请联系管理员处理,并出大厅切换服务器保存数据."));
                //c->flags |= CLIENT_FLAG_DISCONNECTED;
            }
        }

        /* 将玩家选项数据存入数据库 */
        shipgate_send_bb_opts(&ship->sg, c);

#ifdef DEBUG
        DBG_LOG("%d 秒", num_seconds);
#endif // DEBUG
    }
}

/* Destroy a connection, closing the socket and removing it from the list. This
   must always be called with the appropriate lock held for the list! */
void client_destroy_connection(ship_client_t* c,
    struct client_queue* clients) {
    int i = 0;
    time_t now = time(NULL);
    char tstr[26];
    script_action_t action = ScriptActionClientShipLogout;

    if (!(c->flags & CLIENT_FLAG_TYPE_SHIP))
        action = ScriptActionClientBlockLogout;

    TAILQ_REMOVE(clients, c, qentry);

    client_send_bb_data(c);

    script_execute(action, c, SCRIPT_ARG_PTR, c, 0);

#ifdef ENABLE_LUA
    /* Remove the table from the registry */
    luaL_unref(ship->lstate, LUA_REGISTRYINDEX, c->script_ref);
#endif

    /* If the user was on a block, notify the shipgate */
    if (c->version != CLIENT_VERSION_BB && c->pl && c->pl->v1.character.dress_data.gc_string[0]) {
        shipgate_send_block_login(&ship->sg, 0, c->guildcard,
            c->cur_block->b, c->pl->v1.character.dress_data.gc_string);
    }
    else if (c->version == CLIENT_VERSION_BB && c->bb_pl) {
        shipgate_send_block_login_bb(&ship->sg, 0, c->guildcard, c->sec_data.slot,
            c->cur_block->b, (uint16_t*)&c->bb_pl->character.name);
    }

    ship_dec_clients(ship);

    ////TODO 与目前存在空房间的设定冲突 待适配
    ///* If the client has a lobby sitting around that was created but not added
    //   to the list of lobbies, destroy it */
    //if (c->create_lobby && c->create_lobby->lobby_create) {
    //    c->create_lobby->lobby_create = 0;
    //    lobby_destroy_noremove(c->create_lobby);
    //}

       /* If we were logging the user, closesocket the file */
    if (c->logfile) {
        ctime_s(tstr, sizeof(tstr), &now);
        tstr[strlen(tstr) - 1] = 0;
        fprintf(c->logfile, "[%s] 关闭连接\n", tstr);
        fclose(c->logfile);
    }

    if (c->sock >= 0) {
        closesocket(c->sock);
    }

    if (c->limits)
        release(c->limits);

    if (c->recvbuf) {
        free_safe(c->recvbuf);
    }

    if (c->sendbuf) {
        free_safe(c->sendbuf);
    }

    if (c->autoreply) {
        free_safe(c->autoreply);
    }

    if (c->enemy_kills) {
        free_safe(c->enemy_kills);
    }

    if (c->pl) {
        free_safe(c->pl);
    }

    if (c->game_data) {
        free_safe(c->game_data);
    }

    if (c->mode_pl) {
        free_safe(c->mode_pl);
    }

    if (c->records) {
        free_safe(c->records);
    }

    if (c->common_bank) {
        free_safe(c->common_bank);
    }

    if (c->char_bank) {
        free_safe(c->char_bank);
    }

    if (c->bb_pl) {
        free_safe(c->bb_pl);
    }

    if (c->bb_opts) {
        free_safe(c->bb_opts);
    }

    if (c->bb_guild) {
        free_safe(c->bb_guild);
    }

    if (c->next_maps) {
        free_safe(c->next_maps);
    }

    if (c->xbl_ip) {
        free_safe(c->xbl_ip);
    }

    pthread_mutex_destroy(&c->mutex);

    free_safe(c);
}

/* Read data from a client that is connected to any port. */
int client_process_pkt(ship_client_t* c) {

    __try {
        ssize_t sz;
        uint16_t pkt_sz;
        int rv = 0;
        unsigned char* rbp;
        void* tmp;
        uint8_t* recvbuf = get_recvbuf();
        const int hsz = client_type[c->version].hdr_size, hsm = 0x10000 - hsz;

        /* Make sure we got the recvbuf, otherwise, bail. */
        if (!recvbuf) {
            return -1;
        }

        /* If we've got anything buffered, copy it out to the main buffer to make
           the rest of this a bit easier. */
        if (c->recvbuf_cur) {
            memcpy(recvbuf, c->recvbuf, c->recvbuf_cur);
        }

        /* Attempt to read, and if we don't get anything, punt. */
        sz = recv(c->sock, recvbuf + c->recvbuf_cur, MAX_PACKET_BUFF - c->recvbuf_cur, 0);

        //TEST_LOG("客户端接收端口 %d 接收数据 = %d 字节 版本识别 = %d", c->sock, sz, c->version);

        if (sz <= 0) {
            //if(sz == SOCKET_ERROR) {
                //ERR_LOG("客户端接收数据错误: %s", strerror(errno));
            //}

            return -1;
        }

        sz += c->recvbuf_cur;
        c->recvbuf_cur = 0;
        rbp = recvbuf;

        /* As long as what we have is long enough, decrypt it. */
        while (sz >= hsz && rv == 0) {
            /* Decrypt the packet header so we know what exactly we're looking
               for, in terms of packet length. */
            if (!(c->flags & CLIENT_FLAG_HDR_READ)) {
                memcpy(&c->pkt, rbp, hsz);
                CRYPT_CryptData(&c->ckey, &c->pkt, hsz, 0);
                c->flags |= CLIENT_FLAG_HDR_READ;
            }

            /* Read the packet size to see how much we're expecting. */
            switch (c->version) {
            case CLIENT_VERSION_DCV1:
            case CLIENT_VERSION_DCV2:
            case CLIENT_VERSION_GC:
            case CLIENT_VERSION_EP3:
            case CLIENT_VERSION_XBOX:
                pkt_sz = LE16(c->pkt.dc.pkt_len);
                break;

            case CLIENT_VERSION_PC:
                pkt_sz = LE16(c->pkt.pc.pkt_len);
                break;

            case CLIENT_VERSION_BB:
                pkt_sz = LE16(c->pkt.bb.pkt_len);
                break;

            default:
                return -1;
            }

            /* A packet with a size less than that of it's header is obviously bad.
               Quite possibly, malicious. Boot the user and log it. */
            if (pkt_sz < hsz) {
                if (c->guildcard)
                    ERR_LOG("玩家 %" PRIu32 " 发送无效长度数据 0x%02X 0x%02X!", c->guildcard, pkt_sz, hsz);
                else
                    ERR_LOG("从未知客户端处接收到无效长度数据包.");

                return -1;
            }

            /* We'll always need a multiple of 8 or 4 (depending on the type of
               the client) bytes. */
            if (pkt_sz & (hsz - 1)) {
                pkt_sz = (pkt_sz & hsm) + hsz;
            }

            /* Do we have the whole packet? */
            if (sz >= (ssize_t)pkt_sz) {
                /* Yes, we do, decrypt it. */
                CRYPT_CryptData(&c->ckey, rbp + hsz, pkt_sz - hsz, 0);
                memcpy(rbp, &c->pkt, hsz);
                c->last_message = time(NULL);

                /* If we're logging the client, write into the log */
                if (c->logfile) {
                    fprint_packet(c->logfile, rbp, pkt_sz, 1);
                }

                /* Pass it onto the correct handler. */
                if (c->flags & CLIENT_FLAG_TYPE_SHIP) {
                    rv = ship_process_pkt(c, rbp);
                }
                else {
                    rv = block_process_pkt(c, rbp);
                }

                rbp += pkt_sz;
                sz -= pkt_sz;

                c->flags &= ~CLIENT_FLAG_HDR_READ;
            }
            else {
                //ERR_LOG("玩家 %" PRIu32 " 发送无效长度数据2!", c->guildcard);
                /* Nope, we're missing part, break out of the loop, and buffer
                   the remaining data.不，我们缺少一部分，中断循环，并缓冲剩余的数据 */
                break;
            }
        }

        /* If we've still got something left here, buffer it for the next pass. */
        if (sz && rv == 0) {
            /* Reallocate the recvbuf for the client if its too small. */
            if (c->recvbuf_size < sz) {
                tmp = realloc(c->recvbuf, sz);

                if (!tmp) {
                    perror("realloc");
                    return -1;
                }

                c->recvbuf = (unsigned char*)tmp;
                c->recvbuf_size = sz;
            }

            memcpy(c->recvbuf, rbp, sz);
            c->recvbuf_cur = sz;
        }
        else if (c->recvbuf) {
            /* Free the buffer, if we've got nothing in it. */
            free_safe(c->recvbuf);
            c->recvbuf = NULL;
            c->recvbuf_size = 0;
        }

        return rv;
    }

    __except (crash_handler(GetExceptionInformation())) {
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

/* Retrieve the thread-specific recvbuf for the current thread. */
uint8_t* get_recvbuf(void) {
    uint8_t* recvbuf = (uint8_t*)pthread_getspecific(recvbuf_key);

    /* If we haven't initialized the recvbuf pointer yet for this thread, then
       we need to do that now. */
    if (!recvbuf) {
        recvbuf = (uint8_t*)malloc(MAX_PACKET_BUFF);

        if (!recvbuf) {
            ERR_LOG("malloc");
            perror("malloc");
            return NULL;
        }

        if (pthread_setspecific(recvbuf_key, recvbuf)) {
            ERR_LOG("pthread_setspecific");
            perror("pthread_setspecific");
            free_safe(recvbuf);
            return NULL;
        }

        memset(recvbuf, 0, MAX_PACKET_BUFF);
    }

    return recvbuf;
}

/* Set up a simple mail autoreply. */
int client_set_autoreply(ship_client_t* c, void* buf, uint16_t len) {
    char* tmp;

    /* Make space for the new autoreply and copy it in. */
    if (!(tmp = malloc(len))) {
        ERR_LOG("分配动态内存给 autoreply (%" PRIu32
            "):\n%s", c->guildcard, strerror(errno));
        return -1;
    }

    memcpy(tmp, buf, len);

    switch (c->version) {
    case CLIENT_VERSION_PC:
        c->pl->pc.autoreply_enabled = LE32(1);
        c->autoreply_on = 1;
        break;

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        c->pl->v3.autoreply_enabled = LE32(1);
        c->autoreply_on = 1;
        break;

    case CLIENT_VERSION_BB:
        c->pl->bb.autoreply_enabled = LE32(1);
        c->autoreply_on = 1;
        memcpy(c->bb_pl->autoreply, buf, len);
        memset(((uint8_t*)c->bb_pl->autoreply) + len, 0, 0x158 - len);
        break;
    }

    /* Clean up and set the new autoreply in place */
    free_safe(c->autoreply);
    c->autoreply = tmp;
    c->autoreply_len = (int)len;

    return 0;
}

/* Disable the user's simple mail autoreply (if set). */
int client_disable_autoreply(ship_client_t* c) {
    switch (c->version) {
    case CLIENT_VERSION_PC:
        c->pl->pc.autoreply_enabled = 0;
        c->autoreply_on = 0;
        break;

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        c->pl->v3.autoreply_enabled = 0;
        c->autoreply_on = 0;
        break;

    case CLIENT_VERSION_BB:
        c->pl->bb.autoreply_enabled = 0;
        c->autoreply_on = 0;
        break;
    }

    return 0;
}

/* Check if a client has blacklisted someone. */
int client_has_blacklisted(ship_client_t* c, uint32_t gc) {
    uint32_t rgc = LE32(gc);
    int i;

    /* If the user doesn't have a blacklist, this is easy. */
    if (c->version < CLIENT_VERSION_PC)
        return 0;

    /* Look through each blacklist entry. */
    for (i = 0; i < 30; ++i) {
        if (c->blacklist[i] == rgc) {
            return 1;
        }
    }

    /* If we didn't find anything, then we're done. */
    return 0;
}

/* Check if a client has /ignore'd someone. */
int client_has_ignored(ship_client_t* c, uint32_t gc) {
    int i;

    /* Look through the ignore list... */
    for (i = 0; i < CLIENT_IGNORE_LIST_SIZE; ++i) {
        if (c->ignore_list[i] == gc) {
            return 1;
        }
    }

    /* We didn't find the person, so they're not being /ignore'd. */
    return 0;
}

/* Send a message to a client telling them that a friend has logged on/off */
void client_send_friendmsg(ship_client_t* c, int on, const char* fname,
    const char* ship, uint32_t block, const char* nick) {
    if (fname[0] != '\t') {
        send_txt(c, "%s%s %s\n%s %s\n%s 舰仓%02d",
            on ? __(c, "\tE\tC2") : __(c, "\tE\tC4"), nick,
            on ? __(c, "在线") : __(c, "离线"), __(c, "角色:"),
            fname, ship, (int)block);
    }
    else {
        send_txt(c, "%s%s %s\n%s %s\n%s 舰仓%02d",
            on ? __(c, "\tE\tC2") : __(c, "\tE\tC4"), nick,
            on ? __(c, "在线") : __(c, "离线"), __(c, "角色:"),
            fname + 2, ship, (int)block);
    }
}

static void give_stats(psocn_pl_stats_t* dest, psocn_lvl_stats_t* ent) {
    uint16_t tmp;

    tmp = LE16(dest->atp) + ent->atp;
    dest->atp = LE16(tmp);

    tmp = LE16(dest->mst) + ent->mst;
    dest->mst = LE16(tmp);

    tmp = LE16(dest->evp) + ent->evp;
    dest->evp = LE16(tmp);

    tmp = LE16(dest->hp) + ent->hp;
    dest->hp = LE16(tmp);

    tmp = LE16(dest->dfp) + ent->dfp;
    dest->dfp = LE16(tmp);

    tmp = LE16(dest->ata) + ent->ata;
    dest->ata = LE16(tmp);

    tmp = LE16(dest->lck) + ent->lck;
    dest->lck = LE16(tmp);
}

/* 给予Blue Burst客户端经验. */
int client_give_exp(ship_client_t* dest, uint32_t exp_amount) {
    uint32_t exp_total;
    psocn_lvl_stats_t* ent;
    int need_lvlup = 0;
    int cl;
    uint32_t level;

    if (dest->version != CLIENT_VERSION_BB || (!dest->bb_pl) && (!dest->mode_pl)) {
        ERR_LOG("%s 角色不存在或版本不为BB", get_player_describe(dest));
        return -1;
    }

    psocn_bb_char_t* character = get_client_char_bb(dest);

    /* No need if they've already maxed out. */
    if (character->disp.level >= 199)
        return 0;

    /* Add in the experience to their total so far. */
    exp_total = LE32(character->disp.exp);
    exp_total += exp_amount;
    character->disp.exp = LE32(exp_total);
    cl = character->dress_data.ch_class;
    level = LE32(character->disp.level);

    /* Send the packet telling them they've gotten experience. */
    if (subcmd_send_lobby_bb_exp(dest, exp_amount))
        return -1;

    /* See if they got any level ups. */
    do {
        /* 循环遍历升级经验表 要遍历到200级 */
        ent = &bb_char_stats.levels[cl][level + 1];

        if (exp_total >= ent->exp) {
            need_lvlup = 1;
            give_stats(&character->disp.stats, ent);
            ++level;
        }
    } while (exp_total >= ent->exp && level < (MAX_PLAYER_LEVEL - 1));

    /* If they got any level ups, send out the packet that says so. */
    if (need_lvlup) {
        character->disp.level = LE32(level);
        if (subcmd_send_lobby_bb_level(dest))
            return -1;
    }

    return 0;
}

/* 给予Blue Burst客户端等级提升. */
int client_give_level(ship_client_t* dest, uint32_t level_req) {
    uint32_t exp_total;
    psocn_lvl_stats_t* ent;
    int cl;
    uint32_t exp_gained;

    if (dest->version != CLIENT_VERSION_BB || (!dest->bb_pl) && (!dest->mode_pl) || level_req > 199)
        return -1;

    psocn_bb_char_t* character = get_client_char_bb(dest);

    /* No need if they've already at that level. */
    if (character->disp.level >= level_req)
        return 0;

    /* Grab the entry for that level... */
    cl = character->dress_data.ch_class;
    ent = &bb_char_stats.levels[cl][level_req];

    /* Add in the experience to their total so far. */
    exp_total = LE32(character->disp.exp);
    exp_gained = ent->exp - exp_total;
    character->disp.exp = LE32(ent->exp);

    /* Send the packet telling them they've gotten experience. */
    if (subcmd_send_lobby_bb_exp(dest, exp_gained))
        return -1;

    /* Send the level-up packet. */
    character->disp.level = LE32(level_req);
    if (subcmd_send_lobby_bb_level(dest))
        return -1;

    return 0;
}

static void give_stats_v2(psocn_disp_char_t* disp, psocn_lvl_stats_t* ent) {
    uint16_t tmp;

    tmp = LE16(disp->stats.atp) + ent->atp;
    disp->stats.atp = LE16(tmp);

    tmp = LE16(disp->stats.mst) + ent->mst;
    disp->stats.mst = LE16(tmp);

    tmp = LE16(disp->stats.evp) + ent->evp;
    disp->stats.evp = LE16(tmp);

    tmp = LE16(disp->stats.hp) + ent->hp;
    disp->stats.hp = LE16(tmp);

    tmp = LE16(disp->stats.dfp) + ent->dfp;
    disp->stats.dfp = LE16(tmp);

    tmp = LE16(disp->stats.ata) + ent->ata;
    disp->stats.ata = LE16(tmp);

    disp->exp = LE32(ent->exp);
}

/* 给予PSOv2客户端等级提升. */
int client_give_level_v2(ship_client_t* dest, uint32_t level_req) {
    psocn_lvl_stats_t* ent;
    int cl, i;

    if (dest->version != CLIENT_VERSION_DCV2 && dest->version != CLIENT_VERSION_PC)
        return -1;

    if (!dest->pl || level_req > (MAX_PLAYER_LEVEL - 1))
        return -1;

    /* No need if they've already at that level. */
    if (dest->pl->v1.character.disp.level >= level_req)
        return 0;

    /* Give all the stat boosts for the intervening levels... */
    cl = dest->pl->v1.character.dress_data.ch_class;

    for (i = dest->pl->v1.character.disp.level + 1; i <= (int)level_req; ++i) {
        ent = &v2_char_stats.levels[cl][i];
        give_stats_v2(&dest->pl->v1.character.disp, ent);
    }

    dest->pl->v1.character.disp.level = LE32(level_req);

    /* Reload them into the lobby. */
    send_lobby_join(dest, dest->cur_lobby);

    return 0;
}

static int check_char_v1(ship_client_t* src, player_t* pl) {
    bitfloat_t f1 = { 0 }, f2 = { 0 };

    /* Check some stuff that shouldn't ever change first... For these ones,
       we don't have to worry about byte ordering. */
    if (src->pl->v1.character.dress_data.model != pl->v1.character.dress_data.model)
        return -10;

    if (src->pl->v1.character.dress_data.section != pl->v1.character.dress_data.section)
        return -11;

    if (src->pl->v1.character.dress_data.ch_class != pl->v1.character.dress_data.ch_class)
        return -12;

    if (src->pl->v1.character.dress_data.costume != pl->v1.character.dress_data.costume)
        return -13;

    if (src->pl->v1.character.dress_data.skin != pl->v1.character.dress_data.skin)
        return -14;

    if (src->pl->v1.character.dress_data.face != pl->v1.character.dress_data.face)
        return -15;

    if (src->pl->v1.character.dress_data.head != pl->v1.character.dress_data.head)
        return -16;

    if (src->pl->v1.character.dress_data.hair != pl->v1.character.dress_data.hair)
        return -17;

    if (src->pl->v1.character.dress_data.hair_r != pl->v1.character.dress_data.hair_r)
        return -18;

    if (src->pl->v1.character.dress_data.hair_g != pl->v1.character.dress_data.hair_g)
        return -19;

    if (src->pl->v1.character.dress_data.hair_b != pl->v1.character.dress_data.hair_b)
        return -20;

    /* Floating point stuff... Ugh. Pay careful attention to these, just in case
       they're some special value like NaN or Inf (potentially because of byte
       ordering or whatnot). */
    f1.f = src->pl->v1.character.dress_data.prop_x;
    f2.f = pl->v1.character.dress_data.prop_x;
    if (f1.b != f2.b)
        return -21;

    f1.f = src->pl->v1.character.dress_data.prop_y;
    f2.f = pl->v1.character.dress_data.prop_y;
    if (f1.b != f2.b)
        return -22;

    if (memcmp(src->pl->v1.character.dress_data.gc_string, pl->v1.character.dress_data.gc_string, 16))
        return -23;

    /* Now make sure that nothing has decreased that should never decrease.
       Since these aren't equality comparisons, we have to deal with byte
       ordering here... The hp/tp materials count are 8-bits each, but
       everything else is multi-byte. */
    if (src->pl->v1.character.inv.hpmats_used > pl->v1.character.inv.hpmats_used)
        return -24;

    if (src->pl->v1.character.inv.tpmats_used > pl->v1.character.inv.tpmats_used)
        return -25;

    if (LE32(src->pl->v1.character.disp.exp) > LE32(pl->v1.character.disp.exp))
        return -26;

    /* Why is the level 32-bits?... */
    if (LE32(src->pl->v1.character.disp.level) > LE32(pl->v1.character.disp.level))
        return -27;

    /* Other stats omitted for now... */

    /* 如果我们到了这里, we've passed all the checks... */
    return 0;
}

static int check_char_v2(ship_client_t* src, player_t* pl) {
    return check_char_v1(src, pl);
}

static int check_char_pc(ship_client_t* src, player_t* pl) {
    return check_char_v1(src, pl);
}

static int check_char_gc(ship_client_t* src, player_t* pl) {
    return 0;
}

static int check_char_xbox(ship_client_t* src, player_t* pl) {
    return 0;
}

static int check_char_bb(ship_client_t* src, player_t* pl) {
    bitfloat_t f1 = { 0 }, f2 = { 0 };
    psocn_dress_data_t src_dress_data = src->pl->bb.character.dress_data;
    psocn_dress_data_t pkt_dress_data = pl->bb.character.dress_data;
    psocn_disp_char_t src_disp = src->pl->bb.character.disp;
    psocn_disp_char_t pkt_disp = pl->bb.character.disp;

    /* Check some stuff that shouldn't ever change first... For these ones,
       we don't have to worry about byte ordering. */
    if (src_dress_data.model != pkt_dress_data.model)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误model %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.model, pkt_dress_data.model);
        src_dress_data.model = pkt_dress_data.model;
        return -10;
    }

    if (src_dress_data.section != pkt_dress_data.section)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误section %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.section, pkt_dress_data.section);
        src_dress_data.section = pkt_dress_data.section;
        return -11;
    }

    if (src_dress_data.ch_class != pkt_dress_data.ch_class)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误ch_class %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.ch_class, pkt_dress_data.ch_class);
        src_dress_data.ch_class = pkt_dress_data.ch_class;
        return -12;
    }

    if (src_dress_data.costume != pkt_dress_data.costume)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误costume %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.costume, pkt_dress_data.costume);
        src_dress_data.costume = pkt_dress_data.costume;
        return -13;
    }

    if (src_dress_data.skin != pkt_dress_data.skin)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误skin %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.skin, pkt_dress_data.skin);
        src_dress_data.skin = pkt_dress_data.skin;
        return -14;
    }

    if (src_dress_data.face != pkt_dress_data.face)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误face %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.face, pkt_dress_data.face);
        src_dress_data.face = pkt_dress_data.face;
        return -15;
    }

    if (src_dress_data.head != pkt_dress_data.head)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误head %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.head, pkt_dress_data.head);
        src_dress_data.head = pkt_dress_data.head;
        return -16;
    }

    if (src_dress_data.hair != pkt_dress_data.hair)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误hair %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.hair, pkt_dress_data.hair);
        src_dress_data.hair = pkt_dress_data.hair;
        return -17;
    }

    if (src_dress_data.hair_r != pkt_dress_data.hair_r)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误hair_r %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.hair_r, pkt_dress_data.hair_r);
        src_dress_data.hair_r = pkt_dress_data.hair_r;
        return -18;
    }

    if (src_dress_data.hair_g != pkt_dress_data.hair_g)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误hair_g %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.hair_g, pkt_dress_data.hair_g);
        src_dress_data.hair_g = pkt_dress_data.hair_g;
        return -19;
    }

    if (src_dress_data.hair_b != pkt_dress_data.hair_b)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误hair_b %d %d", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.hair_b, pkt_dress_data.hair_b);
        src_dress_data.hair_b = pkt_dress_data.hair_b;
        return -20;
    }

    /* Floating point stuff... Ugh. Pay careful attention to these, just in case
       they're some special value like NaN or Inf (potentially because of byte
       ordering or whatnot). */
    f1.f = src_dress_data.prop_x;
    f2.f = pkt_dress_data.prop_x;
    if (f1.b != f2.b)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误b %u %u", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), f1.b, f2.b);
        f1.b = f2.b;
        return -21;
    }

    f1.f = src_dress_data.prop_y;
    f2.f = pkt_dress_data.prop_y;
    if (f1.b != f2.b)
    {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误b %u %u", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), f1.b, f2.b);
        f1.b = f2.b;
        return -22;
    }

    if (memcmp(src_dress_data.gc_string, pkt_dress_data.gc_string, 16)) {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误gc_string %s %s", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_dress_data.gc_string, pkt_dress_data.gc_string);
        return -23;
    }

    /* Now make sure that nothing has decreased that should never decrease.
       Since these aren't equality comparisons, we have to deal with byte
       ordering here... The hp/tp materials count are 8-bits each, but
       everything else is multi-byte. */
    if (src->pl->bb.character.inv.hpmats_used > pl->bb.character.inv.hpmats_used) {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误hpmats_used %u %u", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src->pl->bb.character.inv.hpmats_used, pl->bb.character.inv.hpmats_used);
        return -24;
    }

    if (src->pl->bb.character.inv.tpmats_used > pl->bb.character.inv.tpmats_used) {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误tpmats_used %u %u", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src->pl->bb.character.inv.tpmats_used, pl->bb.character.inv.tpmats_used);
        return -25;
    }

    if (LE32(src_disp.exp) > LE32(pkt_disp.exp)) {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误exp %u %u", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_disp.exp, pkt_disp.exp);
        src_disp.exp = pkt_disp.exp;
        return -26;
    }

    /* Why is the level 32-bits?... */
    if (LE32(src_disp.level) > LE32(pkt_disp.level)) {
        ERR_LOG("%s[舰仓%02d]: 角色数据检查失败 %s 错误level %u %u", ship->cfg->ship_name, src->cur_block->b,
            get_player_describe(src), src_disp.level, pkt_disp.level);
        src_disp.level = pkt_disp.level;
        return -27;
    }

    return 0;
}

int client_check_character(ship_client_t* c, player_t* pl, int ver) {

    /*DBG_LOG("%s[舰仓%02d]: client_check_character for GC %" PRIu32
        " 版本 = %d", ship->cfg->name, c->cur_block->b,
        c->guildcard, ver);*/

    switch (ver) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
        if (c->version == CLIENT_VERSION_DCV1) {
            if ((c->flags & CLIENT_FLAG_IS_NTE))
                /* XXXX */
                return 0;
            else
                return check_char_v1(c, pl);
        }
        else
            /* This shouldn't happen... */
            return -1;

    case CLIENT_VERSION_PC:
        if (c->version == CLIENT_VERSION_DCV2)
            return check_char_v2(c, pl);
        else if (c->version == CLIENT_VERSION_PC) {
            if (!(c->flags & CLIENT_FLAG_IS_NTE))
                return check_char_pc(c, pl);
            else
                /* XXXX */
                return 0;
        }
        else
            /* This shouldn't happen... */
            return -1;

    case CLIENT_VERSION_GC:
        if (c->version == CLIENT_VERSION_GC)
            return check_char_gc(c, pl);
        else if (c->version == CLIENT_VERSION_XBOX)
            return check_char_xbox(c, pl);
        else if (c->version == CLIENT_VERSION_EP3)
            /* XXXX */
            return 0;
        else
            /* This shouldn't happen... */
            return -1;

    case CLIENT_VERSION_EP3:
        if (c->version == CLIENT_VERSION_BB)
            /* XXXX */
            return 0;
        else
            /* This shouldn't happen... */
            return -1;

    case CLIENT_VERSION_BB:
        return check_char_bb(c, pl);

    default:
        ERR_LOG("角色数据检测: 未知版本 %d", ver);
    }

    /* XXXX */
    return 0;
}

int client_legit_check(ship_client_t* c, psocn_limits_t* limits) {
    uint32_t v;
    iitem_t* item;
    int j, irv;

    /* Figure out what version they're on. */
    switch (c->version) {
    case CLIENT_VERSION_DCV1:
        v = ITEM_VERSION_V1;
        break;

    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
        v = ITEM_VERSION_V2;
        break;

    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_XBOX: /* XXXX */
        v = ITEM_VERSION_GC;
        break;

    case CLIENT_VERSION_BB:
        v = ITEM_VERSION_BB; /* TODO */
    case CLIENT_VERSION_EP3:
    default:
        /* XXXX */
        return 0;
    }

    /* Make sure the player qualifies for legit mode... */
    for (j = 0; j < c->pl->v1.character.inv.item_count; ++j) {
        item = (iitem_t*)&c->pl->v1.character.inv.iitems[j];
        irv = psocn_limits_check_item(limits, item, v);

        if (!irv) {
            SHIPS_LOG("Potentially non-legit found in inventory (GC: %"
                PRIu32"):\n%08x %08x %08x %08x", c->guildcard,
                LE32(item->data.datal[0]), LE32(item->data.datal[1]),
                LE32(item->data.datal[2]), LE32(item->data.data2l));
            return -1;
        }
    }

    return 0;
}

psocn_bank_t* get_client_bank_bb(ship_client_t* src) {
#ifdef DEBUG

    if (src->bank_type) {
        DBG_LOG("%s 的银行模式为 %d", get_player_describe(src), src->bank_type);
    }

#endif // DEBUG

    return src->bank_type == false ? &src->bb_pl->bank : src->common_bank;
}

void print_quest_info(ship_client_t* src) {
    if (src->mode)
        DBG_LOG("%s 的游戏模式为 %d 任务编号 %d", get_player_describe(src), src->mode, src->cur_lobby->qid);
}

inventory_t* get_client_inv_bb(ship_client_t* src) {
    if (src->mode) {
        DBG_LOG("%s BB背包数据获取模式 %d 任务编号 %d", get_player_describe(src), src->mode, src->cur_lobby->qid);
    }
    return src->mode == 0 ? &src->bb_pl->character.inv : &src->mode_pl->bb.inv;
}

inventory_t* get_client_inv_nobb(ship_client_t* src) {
    if (src->mode) {
        DBG_LOG("%s NOBB背包数据获取模式 %d 任务编号 %d", get_player_describe(src), src->mode, src->cur_lobby->qid);
    }

    return src->mode == 0 ? &src->pl->v1.character.inv : &src->mode_pl->nobb.inv;
}

inventory_t* get_player_inv(ship_client_t* src) {
    if (src->version == CLIENT_VERSION_BB)
        return get_client_inv_bb(src);
    else
        return get_client_inv_nobb(src);
}

trade_inv_t* get_client_trade_inv_bb(ship_client_t* src) {
    if (src->mode) {
        DBG_LOG("%s BB交易背包数据获取模式 %d 任务编号 %d", get_player_describe(src), src->mode, src->cur_lobby->qid);
    }
    return &src->game_data->pending_item_trade;
}

psocn_bb_char_t* get_client_char_bb(ship_client_t* src) {
    if (src->mode) {
        DBG_LOG("%s BB角色数据获取模式 %d 任务编号 %d", get_player_describe(src), src->mode, src->cur_lobby->qid);
    }

    return src->mode == 0 ? &src->bb_pl->character : &src->mode_pl->bb;
}

psocn_v1v2v3pc_char_t* get_client_char_nobb(ship_client_t* src) {
    if (src->mode) {
        DBG_LOG("%s NOBB角色数据获取模式 %d 任务编号 %d", get_player_describe(src), src->mode, src->cur_lobby->qid);
    }

    return src->mode == 0 ? &src->pl->v1.character : &src->mode_pl->nobb;
}

ship_client_t* ge_target_client_by_id(lobby_t* l, uint32_t target_client_id) {
    int i = 0;

    for (i = 0; i < l->max_clients; ++i) {
        if (!l->clients[i])
            continue;

        if (l->clients[i]->client_id == target_client_id) {
            return l->clients[i];
        }
    }

    if (i >= l->max_clients)
        ERR_LOG("寻找的客户端ID %d 不存在", target_client_id);

    return NULL;
}

lobby_t* get_client_lobby(ship_client_t* src) {
    lobby_t* l = src->cur_lobby;
    if (!l) {
        ERR_LOG("%s 不在游戏或房间中", get_player_describe(src));
        return NULL;
    }
    return l;
}

char* get_player_describe(ship_client_t* src) {
    if (!src)
        return "玩家不存在";

    /* 初始化角色描述内存 */
    memset(char_des, 0, sizeof(char_des));

    sprintf(char_des, "%s(%d:%d:%s)"
        , get_player_name(src->pl, src->version, false)
        , src->guildcard
        , src->sec_data.slot
        , client_type[src->version].ver_name
    );

    return char_des;
}

char* get_lobby_leader_describe(lobby_t* l) {
    if (!l)
        return "房间不存在";

    ship_client_t* src = l->clients[l->leader_id];
    if (!src)
        return "房主不存在";

    /* 初始化角色描述内存 */
    memset(char_des, 0, sizeof(char_des));

    sprintf(char_des, "%s(%d:%d:%s)"
        , get_player_name(src->pl, src->version, false)
        , src->guildcard
        , src->sec_data.slot
        , client_type[src->version].ver_name
    );

    return char_des;
}

uint8_t get_player_msg_color_set(ship_client_t* src) {
    psocn_bb_char_t* character = get_client_char_bb(src);
    if (!character)
        return 0;

    return costume_mag_color_type[character->dress_data.ch_class][character->dress_data.costume];
}

uint8_t get_player_section(ship_client_t* src) {
    /* 初始化一个颜色ID数值 */
    uint8_t section_id = 0;

    if (!src)
        return section_id;

    switch (src->version) {
    case CLIENT_VERSION_DCV1:
    case CLIENT_VERSION_DCV2:
    case CLIENT_VERSION_PC:
    case CLIENT_VERSION_GC:
    case CLIENT_VERSION_EP3:
    case CLIENT_VERSION_XBOX:
        psocn_v1v2v3pc_char_t* v2character = get_client_char_nobb(src);
        section_id = v2character->dress_data.section;
        break;

    case CLIENT_VERSION_BB:
        psocn_bb_char_t* character = get_client_char_bb(src);
        section_id = character->dress_data.section;
        break;
    default:
        ERR_LOG("%s 版本 %s 颜色未获取成功", get_player_describe(src), client_type[0].ver_name);
        break;
    }

    return section_id;
}

char* get_section_describe(ship_client_t* src, uint8_t section, bool overwrite) {
    uint8_t section_id = 0, language_code = 0;

    if (src) {
        section_id = get_player_section(src);
        language_code = src->language_code;
    }

    if (overwrite)
        section_id = section;

    if (section_id > 9) {
        if (src)
            ERR_LOG("%s 的颜色ID %d 超出界限, 改为 0", get_player_describe(src), section_id);
        else
            ERR_LOG("颜色ID %d 超出界限, 改为 0", section_id);
        /* 如果超出界限 则恢复默认值 0 */
        section_id = 0;
    }

    if (language_code == 0)
        return section_ids[section_id].cn_name;
    else
        return section_ids[section_id].en_name;
}

uint8_t get_lobby_leader_section(lobby_t* l) {
    if (!l)
        return 0;

    ship_client_t* src = l->clients[l->leader_id];
    if (!src) {
        ERR_LOG("房间 %s 没有房主ID %d", l->name, l->leader_id);
        return 0;
    }

    return get_player_section(src);
}

uint8_t get_bb_max_tech_level(uint8_t ch_class, int tech) {
    if(ch_class < MAX_PLAYER_CLASS_BB)
        return max_tech_level[tech].max_lvl[ch_class];

    return 0;
}

void update_bb_mat_use(ship_client_t* src) {
    uint16_t atp_base = 0, mst_base = 0, evp_base = 0, hp_base = 0, dfp_base = 0, ata_base = 0;
    psocn_bb_char_t* character = get_client_char_bb(src);
    inventory_t* inv = &character->inv;
    uint8_t ch_class = character->dress_data.ch_class;
    psocn_pl_stats_t* startingData = &bb_char_stats.start_stats[ch_class];
    uint32_t level = character->disp.level;

    atp_base = startingData->atp;
    mst_base = startingData->mst;
    evp_base = startingData->evp;
    hp_base = startingData->hp;
    dfp_base = startingData->dfp;
    ata_base = startingData->ata;

    //DBG_LOG("startingData atp_base %u mst_base %u evp_base %u hp_base %u dfp_base %u ata_base %u", atp_base, mst_base, evp_base, hp_base, dfp_base, ata_base);

    for (size_t x = 0; x <= character->disp.level; x++) {
        atp_base += (uint16_t)bb_char_stats.levels[ch_class][x].atp;
        mst_base += (uint16_t)bb_char_stats.levels[ch_class][x].mst;
        evp_base += (uint16_t)bb_char_stats.levels[ch_class][x].evp;
        hp_base += (uint16_t)bb_char_stats.levels[ch_class][x].hp;
        dfp_base += (uint16_t)bb_char_stats.levels[ch_class][x].dfp;
        ata_base += (uint16_t)bb_char_stats.levels[ch_class][x].ata;
        //DBG_LOG("atp_base %u mst_base %u evp_base %u hp_base %u dfp_base %u ata_base %u", atp_base, mst_base, evp_base, hp_base, dfp_base, ata_base);
    }

    //DBG_LOG("atp_base %u mst_base %u evp_base %u hp_base %u dfp_base %u ata_base %u", atp_base, mst_base, evp_base, hp_base, dfp_base, ata_base);

    //set_material_usage(inv, MATERIAL_HP, (uint8_t)((character->disp.stats.hp - hp_base) / 2));
    //set_material_usage(inv, MATERIAL_TP, (character->disp.stats. - bb_char_stats.levels[ch_class][level].atp) / 2);
    set_material_usage(inv, MATERIAL_POWER, (uint8_t)((character->disp.stats.atp - atp_base) / 2));
    set_material_usage(inv, MATERIAL_MIND, (uint8_t)((character->disp.stats.mst - mst_base) / 2));
    set_material_usage(inv, MATERIAL_EVADE, (uint8_t)((character->disp.stats.evp - evp_base) / 2));
    set_material_usage(inv, MATERIAL_DEF, (uint8_t)((character->disp.stats.dfp - dfp_base) / 2));
    set_material_usage(inv, MATERIAL_LUCK, (uint8_t)((character->disp.stats.lck - 10) / 2));
}

void show_bb_player_info(ship_client_t* src) {
    psocn_bb_char_t* character = get_client_char_bb(src);
    inventory_t* inv = &character->inv;
    uint8_t ch_class = character->dress_data.ch_class;
    update_bb_mat_use(src);

    send_msg(src, MSG_BOX_TYPE,
        "-----------------------玩家信息-----------------------\n"
        "玩家名称: %s\n"
        "当前职业: %s\n"
        "当前等级: Lv%d 经验:%d 钱包:%d美赛塔\n"
        "背包数量: %d 语言: %u\n"
        "银行数量: 角色:%d 公共:%d\n"
        "嗑药情况: HP药:%u TP药:%u 攻药:%u 智药:%u 闪药:%u 防药:%u 运药:%u\n"
        "挑战模式: 详情未完成\n"
        , get_player_describe(src)
        , pso_class[ch_class].cn_name
        , character->disp.level + 1
        , character->disp.exp
        , character->disp.meseta
        , inv->item_count
        , inv->language
        , src->bb_pl->bank.item_count, src->common_bank->item_count
        , get_material_usage(inv, MATERIAL_HP)
        , get_material_usage(inv, MATERIAL_TP)
        , get_material_usage(inv, MATERIAL_POWER)
        , get_material_usage(inv, MATERIAL_MIND)
        , get_material_usage(inv, MATERIAL_EVADE)
        , get_material_usage(inv, MATERIAL_DEF)
        , get_material_usage(inv, MATERIAL_LUCK)
    );
}

void fix_player_max_tech_level(psocn_bb_char_t* character) {
    for (int i = 0; i < MAX_PLAYER_TECHNIQUES; i++) {
        if (character->technique_levels_v1.all[i] == TECHNIQUE_UNLEARN) {
            character->inv.iitems[i].extension_data1 = 0x00;
            character->technique_levels_v1.all[i] = TECHNIQUE_UNLEARN;
        }

        /* 顺序不能换 要先将余量给到ex1 再初始化为 0x0E */
        if (character->technique_levels_v1.all[i] > TECHNIQUE_V1_MAX_LEVEL) {
            character->inv.iitems[i].extension_data1 = character->technique_levels_v1.all[i] - TECHNIQUE_V1_MAX_LEVEL;
            character->technique_levels_v1.all[i] = TECHNIQUE_V1_MAX_LEVEL;
        }

        uint8_t player_tech_level = get_technique_level(&character->technique_levels_v1, &character->inv, i);
        if (player_tech_level == TECHNIQUE_UNLEARN)
            continue;

        uint8_t max_level = get_bb_max_tech_level(character->dress_data.ch_class, i);
        if (player_tech_level >= max_level) {
            /* 移除不合规的法术 */
#ifdef DEBUG
            DBG_LOG("%s:%s 法术 %s 等级 %d 高于 %d, 修正为 %d 级!"
                , character->dress_data.gc_string
                , pso_class[character->dress_data.ch_class].cn_name
                , get_technique_comment(i), show_technique_level(character, i)
                , get_bb_max_tech_level(character, i)
                , get_bb_max_tech_level(character, i)
            );
#endif // DEBUG

            set_technique_level(&character->technique_levels_v1, &character->inv, i, max_level);
        }
    }
}

void show_player_tech_info(ship_client_t* src) {
    psocn_bb_char_t* character = get_client_char_bb(src);
    uint8_t ch_class = character->dress_data.ch_class;
    char tmp_msg[4096] = { 0 };
    char data_str[100] = { 0 };

    if (char_class_is_android(src->equip_flags)) {
        send_msg(src, MSG_BOX_TYPE,
            "-----------------------玩家法术-----------------------\n"
            "玩家名称: %s\n"
            "当前职业: %s\n"
            "\tE\tCG你是个机器人啊~请自重~\n"
            , get_player_describe(src)
            , pso_class[ch_class].cn_name
        );
    }
    else {
        sprintf_s(tmp_msg, sizeof(tmp_msg),
            "-----------------------玩家法术-----------------------\n"
            "玩家名称: %s\n"
            "当前职业: %s\n"
            , get_player_describe(src)
            , pso_class[ch_class].cn_name
        );

        for (int i = 0; i < MAX_PLAYER_TECHNIQUES; ++i) {
            fix_player_max_tech_level(&src->bb_pl->character);

            if (get_bb_max_tech_level(src->bb_pl->character.dress_data.ch_class, i) == 0) {
                // 将要写入的数据格式化为字符串
                sprintf_s(data_str, sizeof(data_str),
                    "%s \tE\tC4无法学习\tE\tC7"
                    , get_technique_comment(i)
                );
            }
            else {
                uint8_t player_tech_level = get_technique_level(&character->technique_levels_v1, &character->inv, i);
                if (player_tech_level == TECHNIQUE_UNLEARN) {
                    sprintf_s(data_str, sizeof(data_str),
                        "%s \tE\tCG未学习\tE\tC7"
                        , get_technique_comment(i)
                    );
                }
                else {
                    // 将要写入的数据格式化为字符串
                    sprintf_s(data_str, sizeof(data_str),
                        "%s Lv%d"
                        , get_technique_comment(i)
                        , player_tech_level
                    );
                }
            }

            // 计算剩余可用空间
            size_t remaining_space = sizeof(tmp_msg) - strlen(tmp_msg);

            // 检查剩余空间是否足够
            if (strlen(data_str) + 1 > remaining_space) {  // +1 是为了考虑结尾的 null 字符
                break;  // 停止追加数据
            }

            // 每追加5个数据后插入换行符
            if ((i + 1) % 4 == 0) {
                strcat_s(data_str, sizeof(data_str), "\n");
            }
            else {
                // 在格式化后的数据后面追加 | 符号
                strcat_s(data_str, sizeof(data_str), " | ");
            }

            // 将格式化后的数据追加到 tmp_msg 字符数组中
            strcat_s(tmp_msg, sizeof(tmp_msg), data_str);
        }

        send_msg(src, MSG_BOX_TYPE, "%s", tmp_msg);
    }
}

void show_player_inv_info(ship_client_t* src) {
    inventory_t* inv = get_player_inv(src);
    char tmp_msg[4096] = { 0 };
    char data_str[100] = { 0 };

    sprintf_s(tmp_msg, sizeof(tmp_msg),
        "-----------------------玩家背包-----------------------\n"
        "玩家名称: %s\n"
        "背包数据: %d 件物品 %d 美赛塔\n"
        , get_player_describe(src)
        , inv->item_count, src->bb_pl->character.disp.meseta
    );

    for (int i = 0; i < inv->item_count; ++i) {
        // 将要写入的数据格式化为字符串
        sprintf_s(data_str, sizeof(data_str), "%d.%s", i
            , item_get_name(&inv->iitems[i].data, src->version, src->language_code)
        );

        // 计算剩余可用空间
        size_t remaining_space = sizeof(tmp_msg) - strlen(tmp_msg);

        // 检查剩余空间是否足够
        if (strlen(data_str) + 1 > remaining_space) {  // +1 是为了考虑结尾的 null 字符
            break;  // 停止追加数据
        }

        // 每追加5个数据后插入换行符
        if ((i + 1) % 5 == 0) {
            strcat_s(data_str, sizeof(data_str), "\n");
        }
        else {
            // 在格式化后的数据后面追加 | 符号
            strcat_s(data_str, sizeof(data_str), " | ");
        }

        // 将格式化后的数据追加到 tmp_msg 字符数组中
        strcat_s(tmp_msg, sizeof(tmp_msg), data_str);
    }

    send_msg(src, MSG_BOX_TYPE, "%s", tmp_msg);
}

void show_player_bank_info(ship_client_t* src) {
    psocn_bank_t* bank = get_client_bank_bb(src);
    char tmp_msg[4096] = { 0 };
    char data_str[100] = { 0 };

    sprintf_s(tmp_msg, sizeof(tmp_msg),
        "-----------------------玩家银行-----------------------\n"
        "玩家名称: %s\n"
        "银行数据: %d 件物品 %d 美赛塔\n"
        , get_player_describe(src)
        , bank->item_count, bank->meseta
    );

    for (size_t i = 0; i < bank->item_count; ++i) {
        // 将要写入的数据格式化为字符串
        sprintf_s(data_str, sizeof(data_str), "%d.%s", i
            , item_get_name(&bank->bitems[i].data, src->version, src->language_code)
        );

        // 计算剩余可用空间
        size_t remaining_space = sizeof(tmp_msg) - strlen(tmp_msg);

        // 检查剩余空间是否足够
        if (strlen(data_str) + 1 > remaining_space) {  // +1 是为了考虑结尾的 null 字符
            break;  // 停止追加数据
        }

        // 每追加5个数据后插入换行符
        if ((i + 1) % 5 == 0) {
            strcat_s(data_str, sizeof(data_str), "\n");
        }
        else {
            // 在格式化后的数据后面追加 | 符号
            strcat_s(data_str, sizeof(data_str), " | ");
        }

        // 将格式化后的数据追加到 tmp_msg 字符数组中
        strcat_s(tmp_msg, sizeof(tmp_msg), data_str);
    }

    send_msg(src, MSG_BOX_TYPE, "%s", tmp_msg);
}

void show_player_common_bank_info(ship_client_t* src) {
    psocn_bank_t* bank = src->common_bank;
    char tmp_msg[4096] = { 0 };
    char data_str[100] = { 0 };

    sprintf_s(tmp_msg, sizeof(tmp_msg),
        "-----------------------公共银行-----------------------\n"
        "玩家名称: %s\n"
        "银行数据: %d 件物品 %d 美赛塔\n"
        , get_player_describe(src)
        , bank->item_count, bank->meseta
    );

    for (size_t i = 0; i < bank->item_count; ++i) {
        // 将要写入的数据格式化为字符串
        sprintf_s(data_str, sizeof(data_str), "%d.%s", i
            , item_get_name(&bank->bitems[i].data, src->version, src->language_code)
        );

        // 计算剩余可用空间
        size_t remaining_space = sizeof(tmp_msg) - strlen(tmp_msg);

        // 检查剩余空间是否足够
        if (strlen(data_str) + 1 > remaining_space) {  // +1 是为了考虑结尾的 null 字符
            break;  // 停止追加数据
        }

        // 每追加5个数据后插入换行符
        if ((i + 1) % 5 == 0) {
            strcat_s(data_str, sizeof(data_str), "\n");
        }
        else {
            // 在格式化后的数据后面追加 | 符号
            strcat_s(data_str, sizeof(data_str), " | ");
        }

        // 将格式化后的数据追加到 tmp_msg 字符数组中
        strcat_s(tmp_msg, sizeof(tmp_msg), data_str);
    }

    send_msg(src, MSG_BOX_TYPE, "%s", tmp_msg);
}

void show_coren_reward_info(ship_client_t* src, uint32_t week, uint32_t index) {
    char tmp_msg[4096] = { 0 };
    char data_str[100] = { 0 };
    Coren_Reward_List_t reward_list = { 0 };
    reward_list.rewards = day_reward_list[week][index];
    size_t count = ARRAYSIZE(day_reward_list[week][index]);

    sprintf_s(tmp_msg, sizeof(tmp_msg),
        "-----------------------%s 科伦 \tE\tCG%d \tE\tC7奖励列表-----------------------\n"
        "数量: %d\n"
        , get_server_DayOfWeek_desc()
        , menu_choice_price[index]
        , count
    );

    for (size_t i = 0; i < count; ++i) {
        item_t tmp_i = { 0 };
        tmp_i.datal[0] = reward_list.rewards[i];
        // 将要写入的数据格式化为字符串
        sprintf_s(data_str, sizeof(data_str), "%d.%s", i + 1
            , item_get_name(&tmp_i, src->version, src->language_code)
        );

        // 计算剩余可用空间
        size_t remaining_space = sizeof(tmp_msg) - strlen(tmp_msg);

        // 检查剩余空间是否足够
        if (strlen(data_str) + 1 > remaining_space) {  // +1 是为了考虑结尾的 null 字符
            break;  // 停止追加数据
        }

        // 每追加5个数据后插入换行符
        if ((i + 1) % 5 == 0) {
            strcat_s(data_str, sizeof(data_str), "\n");
        }
        else {
            // 在格式化后的数据后面追加 | 符号
            strcat_s(data_str, sizeof(data_str), " | ");
        }

        // 将格式化后的数据追加到 tmp_msg 字符数组中
        strcat_s(tmp_msg, sizeof(tmp_msg), data_str);
    }

    send_msg(src, MSG_BOX_TYPE, "%s", tmp_msg);
}

void show_game_cmd_help_msg(ship_client_t* src) {
    lobby_t* l = src->cur_lobby;

    if (l) {
        if (l->type == LOBBY_TYPE_LOBBY) {
            send_msg(src, MSG_BOX_TYPE,
                "-----------------------房间指令菜单-----------------------\n"
                "/npc 召唤npc跟随作战, 任务和单人无效\n"
                "/ignore 忽略房间中某一位玩家\n"
                "/quit 快速断开游戏\n"
                //"/trackkill 杀敌数跟踪\n"
                "/bank 切换公共仓库\n"
                "/qr 非任务情况下返回先驱者2号\n"
                "/lb 快速返回舰仓\n"
                "/matuse 显示玩家嗑药情况\n"
                "/clean [inv/bank] 用于清理背包 银行数据 慎用！！！！\n"
            );
        }
    }

    send_txt(src, "%s", __(src, "\tE\tC4无法使用该指令."));
}

void show_lobby_cmd_help_msg(ship_client_t* src) {
    lobby_t* l = src->cur_lobby;

    if (l) {
        if (l->type == LOBBY_TYPE_LOBBY) {
            send_msg(src, MSG_BOX_TYPE,
                "-----------------------大厅指令菜单-----------------------\n"
                "/pl 玩家专属菜单,功能性菜单\n"
                //"/npcskin 切换为其他NPC皮肤 (范围0 - 5, 0为角色皮肤)\n"
                //"/save slot 保存某一个槽位角色的数据, slot范围 0 - 3\n"
                //"/restore slot 从备份数据中恢复角色数据, slot范围 0 - 3 慎用\n"
                //"/enablebk 当前已默认开启,开启自动备份功能1.\n"
                //"/disablebk 当前已默认开启,取消自动备份功能1.\n"
                "/noevent 取消个人节日设置,不会影响其他玩家.\n"
            );
        }
    }

    send_txt(src, "%s", __(src, "\tE\tC4无法使用该指令."));
}

void init_client_err(client_error_t* err, bool has_error, errno_t error_cmd_type, errno_t error_subcmd_type) {
    err->has_error = has_error;
    err->error_cmd_type = error_cmd_type;
    err->error_subcmd_type = error_subcmd_type;
}

void clean_client_err(client_error_t* err) {
    err->has_error = false;
    err->error_cmd_type = 0;
    err->error_subcmd_type = 0;
}

#ifdef ENABLE_LUA

static int client_guildcard_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushinteger(l, c->guildcard);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_isOnBlock_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushboolean(l, !(c->flags & CLIENT_FLAG_TYPE_SHIP));
    }
    else {
        lua_pushboolean(l, 0);
    }

    return 1;
}

static int client_disconnect_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        c->flags |= CLIENT_FLAG_DISCONNECTED;
    }

    return 0;
}

static int client_addr_lua(lua_State* l) {
    ship_client_t* c;
    char str[INET6_ADDRSTRLEN];

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        my_ntop(&c->ip_addr, str);
        lua_pushstring(l, str);
    }
    else {
        lua_pushliteral(l, "");
    }

    return 1;
}

static int client_version_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushinteger(l, c->version);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_clientID_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushinteger(l, c->client_id);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_privilege_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushinteger(l, c->privilege);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_send_lua(lua_State* l) {
    ship_client_t* c;
    const uint8_t* s;
    size_t len;
    uint16_t len2;

    if (lua_islightuserdata(l, 1) && lua_isstring(l, 2)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        s = (const uint8_t*)lua_tolstring(l, 2, &len);

        if (!s) {
            lua_pushinteger(l, -1);
            return 1;
        }

        /* Check it for sanity. */
        if (len < 4 || (len & 0x03)) {
            lua_pushinteger(l, -1);
            return 1;
        }

        len2 = s[2] | (s[3] << 8);
        if (len2 != len) {
            lua_pushinteger(l, -1);
            return 1;
        }

        if (send_pkt_dc(c, (const dc_pkt_hdr_t*)s)) {
            lua_pushinteger(l, -1);
            return 1;
        }

        lua_pushinteger(l, 0);
        return 1;
    }
    else {
        lua_pushinteger(l, -1);
        return 1;
    }
}

static int client_lobby_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);

        if (c->cur_lobby)
            lua_pushlightuserdata(l, c->cur_lobby);
        else
            lua_pushnil(l);
    }
    else {
        lua_pushnil(l);
    }

    return 1;
}

static int client_block_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);

        if (c->cur_block)
            lua_pushlightuserdata(l, c->cur_block);
        else
            lua_pushnil(l);
    }
    else {
        lua_pushnil(l);
    }

    return 1;
}

static int client_sendSData_lua(lua_State* l) {
    ship_client_t* c;
    uint32_t event;
    const uint8_t* s;
    size_t len;
    lua_Integer rv = -1;

    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2) && lua_isstring(l, 3)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        event = (uint32_t)lua_tointeger(l, 2);
        s = (const uint8_t*)lua_tolstring(l, 3, &len);

        rv = shipgate_send_sdata(&ship->sg, c, event, s, (uint32_t)len);
    }

    lua_pushinteger(l, rv);
    return 1;
}

static int client_sendMsg_lua(lua_State* l) {
    ship_client_t* c;
    const char* s;
    size_t len;

    if (lua_islightuserdata(l, 1) && lua_isstring(l, 2)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        s = (const char*)lua_tolstring(l, 2, &len);

        send_txt(c, "\tE\tC7%s", s);
    }

    lua_pushinteger(l, 0);
    return 1;
}

static int client_getTable_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_rawgeti(l, LUA_REGISTRYINDEX, c->script_ref);
    }
    else {
        lua_pushnil(l);
    }

    return 1;
}

static int client_area_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushinteger(l, c->cur_area);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_name_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);

        if (c->pl)
            lua_pushstring(l, c->pl->v1.character.dress_data.gc_string);
        else
            lua_pushnil(l);
    }
    else {
        lua_pushnil(l);
    }

    return 1;
}

static int client_flags_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushinteger(l, c->flags);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_level_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);

        if (c->pl)
            lua_pushinteger(l, (lua_Integer)(c->pl->v1.character.disp.level + 1));
        else
            lua_pushinteger(l, -1);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_sendMenu_lua(lua_State* l) {
    ship_client_t* c;
    gen_menu_entry_t* ents;
    lua_Integer count, i;
    int tp;
    const char* str;
    uint32_t menu_id;

    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2) &&
        lua_isinteger(l, 3) && lua_istable(l, 4) && lua_istable(l, 5)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        menu_id = (uint32_t)lua_tointeger(l, 2);
        count = lua_tointeger(l, 3);

        /* Make sure we've got a sane count for the menu. */
        if (count <= 0) {
            lua_pushinteger(l, -1);
            return 1;
        }

        ents = (gen_menu_entry_t*)malloc(sizeof(gen_menu_entry_t) * (size_t)count);
        if (!ents) {
            lua_pushinteger(l, -1);
            return 1;
        }

        /* Read each element from the tables passed in */
        for (i = 1; i <= count; ++i) {
            tp = lua_rawgeti(l, 4, i);
            if (tp != LUA_TNUMBER) {
                lua_pop(l, 1);
                free_safe(ents);
                lua_pushinteger(l, -1);
                return 1;
            }

            ents[i - 1].item_id = (uint32_t)lua_tointeger(l, -1);
            lua_pop(l, 1);

            tp = lua_rawgeti(l, 5, i);
            if (tp != LUA_TSTRING) {
                lua_pop(l, 1);
                free_safe(ents);
                lua_pushinteger(l, -1);
                return 1;
            }

            str = lua_tostring(l, -1);
            strncpy(ents[i - 1].text, str, 15);
            ents[i - 1].text[15] = 0;
            lua_pop(l, 1);
        }

        send_generic_menu(c, menu_id, (size_t)count, ents);
        lua_pushinteger(l, 0);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_dropItem_lua(lua_State* l) {
    ship_client_t* c;
    lobby_t* lb;
    //uint32_t item[4] = { 0, 0, 0, 0 };
    item_t item = { 0 };
    subcmd_drop_stack_t p2 = { 0 };

    /* We need at least the client itself and the first dword of the item */
    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lb = c->cur_lobby;
        item.datal[0] = (uint32_t)lua_tointeger(l, 2);

        /* Make sure we're in a team, not a regular lobby... */
        if (lb->type != LOBBY_TYPE_GAME) {
            lua_pushinteger(l, -1);
            return 1;
        }

        /* Check all the optional arguments */
        if (lua_isinteger(l, 3)) {
            item.datal[1] = (uint32_t)lua_tointeger(l, 3);

            if (lua_isinteger(l, 4)) {
                item.datal[2] = (uint32_t)lua_tointeger(l, 4);

                if (lua_isinteger(l, 5)) {
                    item.data2l = (uint32_t)lua_tointeger(l, 5);
                }
            }
        }

        /* Do some basic checks of the item... */
        if (is_stackable(&item) && !(item.datal[1] & 0x0000FF00)) {
            /* If the item is stackable and doesn't have a quantity, give one
               of it. */
            item.datal[1] |= (1 << 8);
        }

        /* Generate the packet to drop the item */
        p2.hdr.pkt_type = GAME_SUBCMD60_TYPE;
        p2.hdr.pkt_len = sizeof(subcmd_drop_stack_t);
        p2.hdr.flags = 0;
        p2.shdr.type = SUBCMD60_DROP_STACK;
        p2.shdr.size = 0x0A;
        p2.shdr.client_id = c->client_id;
        p2.area = LE16(c->cur_area);
        p2.unk = LE16(0);
        p2.x = c->x;
        p2.z = c->z;
        p2.data.datal[0] = LE32(item.datal[0]);
        p2.data.datal[1] = LE32(item.datal[1]);
        p2.data.datal[2] = LE32(item.datal[2]);
        p2.data.item_id = LE32(lb->item_lobby_id);
        p2.data.data2l = LE32(item.data2l);
        p2.two = LE32(0x00000002);
        ++lb->item_lobby_id;

        lobby_send_pkt_dc(lb, NULL, (dc_pkt_hdr_t*)&p2, 0);
        lua_pushinteger(l, 0);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_sendMsgBox_lua(lua_State* l) {
    ship_client_t* c;
    const char* s;
    size_t len;

    if (lua_islightuserdata(l, 1) && lua_isstring(l, 2)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        s = (const char*)lua_tolstring(l, 2, &len);

        send_msg(c, MSG1_TYPE, "\tE%s", s);
        lua_pushinteger(l, 0);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_numItems_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);

        /* Make sure we have character data first. */
        if (!c->pl) {
            lua_pushinteger(l, -1);
            return 1;
        }

        lua_pushinteger(l, c->pl->v1.character.inv.item_count);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_item_lua(lua_State* l) {
    ship_client_t* c;
    int index;

    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        index = (int)lua_tointeger(l, 2);

        /* Make sure we have character data first. */
        if (!c->pl) {
            lua_pushnil(l);
            return 1;
        }

        /* Make sure the index is sane */
        if (index < 0 || index >= c->pl->v1.character.inv.item_count) {
            lua_pushnil(l);
            return 1;
        }

        /* Create a table and put all 4 dwords of item data in it. */
        lua_newtable(l);
        lua_pushinteger(l, 1);
        lua_pushinteger(l, c->pl->v1.character.inv.iitems[index].data.datal[0]);
        lua_settable(l, -3);
        lua_pushinteger(l, 2);
        lua_pushinteger(l, c->pl->v1.character.inv.iitems[index].data.datal[1]);
        lua_settable(l, -3);
        lua_pushinteger(l, 2);
        lua_pushinteger(l, c->pl->v1.character.inv.iitems[index].data.datal[2]);
        lua_settable(l, -3);
        lua_pushinteger(l, 2);
        lua_pushinteger(l, c->pl->v1.character.inv.iitems[index].data.data2l);
        lua_settable(l, -3);
    }
    else {
        lua_pushnil(l);
    }

    return 1;
}

static int client_find_lua(lua_State* l) {
    ship_client_t* c;
    uint32_t gc;
    uint32_t i;

    if (lua_isinteger(l, 1)) {
        gc = (uint32_t)lua_tointeger(l, 1);

        for (i = 0; i < ship->cfg->blocks; ++i) {
            if ((c = block_find_client(ship->blocks[i], gc))) {
                lua_pushlightuserdata(l, c);
                return 1;
            }
        }

        lua_pushnil(l);
    }
    else {
        lua_pushnil(l);
    }

    return 1;
}

static int client_syncRegister_lua(lua_State* l) {
    ship_client_t* c;
    lua_Integer reg, value;

    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2) &&
        lua_isinteger(l, 3)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        reg = lua_tointeger(l, 2);
        value = lua_tointeger(l, 3);

        send_sync_register(c, (uint8_t)reg, (uint32_t)value);
        lua_pushinteger(l, 0);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static int client_hasItem_lua(lua_State* l) {
    ship_client_t* c;
    lua_Integer ic;
    uint32_t val;
    int i;
    iitem_t* item;

    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        ic = lua_tointeger(l, 2);

        for (i = 0; i < c->pl->v1.character.inv.item_count; ++i) {
            item = (iitem_t*)&c->pl->v1.character.inv.iitems[i];
            val = item->data.datal[0];

            /* Grab the real item type, if its a v2 item.
               Note: Gamecube uses this byte for wrapping paper design. */
            if (c->version < ITEM_VERSION_GC && item->data.datab[5])
                val = (item->data.datab[5] << 8);

            if ((val & 0x00FFFFFF) == ic) {
                lua_pushboolean(l, 1);
                return 1;
            }
        }
    }

    /* 如果我们到了这里, the user doesn't have the item requested, so return
       0 for false. */
    lua_pushboolean(l, 0);
    return 1;
}

static int client_legitCheck_lua(lua_State* l) {
    ship_client_t* c;
    int rv;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);

        rv = client_legit_check(c, ship->def_limits);
        lua_pushboolean(l, !rv);
        return 1;
    }

    lua_pushboolean(l, 1);
    return 1;
}

static int client_legitCheckItem_lua(lua_State* l) {
    ship_client_t* c;
    lua_Integer ic1, ic2, ic3, ic4, v;
    iitem_t item = { 0 };
    int rv;

    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2) &&
        lua_isinteger(l, 3) && lua_isinteger(l, 4) && lua_isinteger(l, 5)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        ic1 = lua_tointeger(l, 2);
        ic2 = lua_tointeger(l, 3);
        ic3 = lua_tointeger(l, 4);
        ic4 = lua_tointeger(l, 5);

        item.data.datal[0] = (uint32_t)ic1;
        item.data.datal[1] = (uint32_t)ic2;
        item.data.datal[2] = (uint32_t)ic3;
        item.data.data2l = (uint32_t)ic4;

        switch (c->version) {
        case CLIENT_VERSION_DCV1:
            v = ITEM_VERSION_V1;
            break;

        case CLIENT_VERSION_DCV2:
        case CLIENT_VERSION_PC:
            v = ITEM_VERSION_V2;
            break;

        case CLIENT_VERSION_GC:
        case CLIENT_VERSION_XBOX:
            v = ITEM_VERSION_GC;
            break;

        default:
            lua_pushboolean(l, 1);
            return 1;
        }

        rv = psocn_limits_check_item(ship->def_limits, &item, (uint32_t)v);

        if (!rv) {
            SHIPS_LOG("legitCheckItem failed for GC %" PRIu32 " with "
                "item %08" PRIx32 " %08" PRIx32 " %08" PRIx32 " %08" PRIx32
                "", c->guildcard, item.data.datal[0], item.data.datal[1],
                item.data.datal[2], item.data.data2l);
        }

        lua_pushboolean(l, !!rv);
        return 1;
    }

    lua_pushboolean(l, 1);
    return 1;
}

static int client_coords_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushnumber(l, c->x);
        lua_pushnumber(l, c->y);
        lua_pushnumber(l, c->z);

        return 3;
    }

    lua_pushnumber(l, 0);
    lua_pushnumber(l, 0);
    lua_pushnumber(l, 0);
    return 3;
}

static int client_distance_lua(lua_State* l) {
    ship_client_t* c, * c2;
    double x, y, z, d;

    if (lua_islightuserdata(l, 1) && lua_islightuserdata(l, 2)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        c2 = (ship_client_t*)lua_touserdata(l, 2);

        if (c->cur_lobby != c2->cur_lobby)
            goto err;

        if (c->cur_area != c2->cur_area)
            goto err;

        /* 计算欧氏距离. */
        x = c->x - c2->x;
        y = c->y - c2->y;
        z = c->z - c2->z;

        d = sqrt(x * x + y * y + z * z);
        lua_pushnumber(l, d);
        return 1;
    }

err:
    lua_pushnumber(l, -1);
    return 1;
}

static int client_language_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushinteger(l, c->language_code);
        return 1;
    }

    lua_pushinteger(l, -1);
    return 1;
}

static int client_qlang_lua(lua_State* l) {
    ship_client_t* c;

    if (lua_islightuserdata(l, 1)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        lua_pushinteger(l, c->q_lang);
        return 1;
    }

    lua_pushinteger(l, -1);
    return 1;
}

static int client_sendMenu2_lua(lua_State* l) {
    ship_client_t* c;
    gen_menu_entry_t* ents;
    lua_Integer count, i;
    int tp;
    const char* str;
    uint32_t menu_id;

    if (lua_islightuserdata(l, 1) && lua_isinteger(l, 2) &&
        lua_isinteger(l, 3) && lua_istable(l, 4)) {
        c = (ship_client_t*)lua_touserdata(l, 1);
        menu_id = (uint32_t)lua_tointeger(l, 2);
        count = lua_tointeger(l, 3);

        /* Make sure we've got a sane count for the menu. */
        if (count <= 0) {
            lua_pushinteger(l, -1);
            return 1;
        }

        ents = (gen_menu_entry_t*)malloc(sizeof(gen_menu_entry_t) * (size_t)count);
        if (!ents) {
            lua_pushinteger(l, -1);
            return 1;
        }

        /* Read each element from the tables passed in */
        for (i = 1; i <= count; ++i) {
            tp = lua_rawgeti(l, 4, i);
            if (tp != LUA_TTABLE) {
                lua_pop(l, 1);
                free_safe(ents);
                lua_pushinteger(l, -1);
                return 1;
            }

            tp = lua_rawgeti(l, -1, 1);
            if (tp != LUA_TNUMBER) {
                lua_pop(l, 2);
                free_safe(ents);
                lua_pushinteger(l, -1);
                return 1;
            }

            ents[i - 1].item_id = (uint32_t)lua_tointeger(l, -1);
            lua_pop(l, 1);

            tp = lua_rawgeti(l, -1, 2);
            if (tp != LUA_TSTRING) {
                lua_pop(l, 2);
                free_safe(ents);
                lua_pushinteger(l, -1);
                return 1;
            }

            str = lua_tostring(l, -1);
            strncpy(ents[i - 1].text, str, 15);
            ents[i - 1].text[15] = 0;
            lua_pop(l, 2);
        }

        tp = send_generic_menu(c, menu_id, (size_t)count, ents);
        free_safe(ents);
        lua_pushinteger(l, tp);
    }
    else {
        lua_pushinteger(l, -1);
    }

    return 1;
}

static const luaL_Reg clientlib[] = {
    { "guildcard", client_guildcard_lua },
    { "isOnBlock", client_isOnBlock_lua },
    { "disconnect", client_disconnect_lua },
    { "addr", client_addr_lua },
    { "version", client_version_lua },
    { "clientID", client_clientID_lua },
    { "privilege", client_privilege_lua },
    { "send", client_send_lua },
    { "lobby", client_lobby_lua },
    { "block", client_block_lua },
    { "sendScriptData", client_sendSData_lua },
    { "sendMsg", client_sendMsg_lua },
    { "getTable", client_getTable_lua },
    { "area", client_area_lua },
    { "name", client_name_lua },
    { "flags", client_flags_lua },
    { "level", client_level_lua },
    { "sendMenu", client_sendMenu_lua },
    { "dropItem", client_dropItem_lua },
    { "sendMsgBox", client_sendMsgBox_lua },
    { "numItems", client_numItems_lua },
    { "item", client_item_lua },
    { "find", client_find_lua },
    { "syncRegister", client_syncRegister_lua },
    { "hasItem", client_hasItem_lua },
    { "legitCheck", client_legitCheck_lua },
    { "legitCheckItem", client_legitCheckItem_lua },
    { "coords", client_coords_lua },
    { "distance", client_distance_lua },
    { "language", client_language_lua },
    { "qlang", client_qlang_lua },
    { "sendMenu2", client_sendMenu2_lua },
    { NULL, NULL }
};

int client_register_lua(lua_State* l) {
    luaL_newlib(l, clientlib);
    return 1;
}

#endif /* ENABLE_LUA */