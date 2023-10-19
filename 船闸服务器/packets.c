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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <WinSock_Defines.h>

#include <psoconfig.h>
#include <f_logs.h>
#include <debug.h>
#include <f_checksum.h>
#include <f_iconv.h>
#include <database.h>

#include "version.h"
#include "shipgate.h"
#include "ship.h"
#include <pso_pack.h>

static uint8_t sendbuf[MAX_PACKET_BUFF];
bb_max_tech_level_t max_tech_level[MAX_PLAYER_TECHNIQUES];

/* The key for accessing our thread-specific send buffer. */
pthread_key_t sendbuf_key;

/* 获取 sendbuf 动态内存数据. */
uint8_t* get_sg_sendbuf(void) {
    //uint8_t* sendbuf = (uint8_t*)pthread_getspecific(sendbuf_key);

    ///* If we haven't initialized the sendbuf pointer yet for this thread, then
    //   we need to do that now. */
    //if (!sendbuf) {
    //    sendbuf = (uint8_t*)malloc(MAX_PACKET_BUFF);

    //    if (!sendbuf) {
    //        ERR_LOG("malloc");
    //        perror("malloc");
    //        return NULL;
    //    }

    //    memset(sendbuf, 0, MAX_PACKET_BUFF);

    //    if (pthread_setspecific(sendbuf_key, sendbuf)) {
    //        ERR_LOG("pthread_setspecific");
    //        free_safe(sendbuf);
    //        return NULL;
    //    }
    //}
    //uint8_t* sendbuf = (uint8_t*)malloc(MAX_PACKET_BUFF);

    ///* If we haven't initialized the sendbuf pointer yet for this thread, then
    //   we need to do that now. */
    //if (!sendbuf) {
    //    ERR_LOG("malloc");
    //    return NULL;
    //}

    memset(sendbuf, 0, MAX_PACKET_BUFF);

    return sendbuf;
}

#define MAX_BUFFER_SIZE 1024

// 发送消息
static ssize_t send_message(ship_t* c, const char* message, size_t message_len) {
    char compressed_msg[MAX_BUFFER_SIZE] = { 0 };
    uLongf compressed_len = MAX_BUFFER_SIZE;

    // 使用 zlib 进行数据压缩
    compress2((Bytef*)compressed_msg, &compressed_len, (const Bytef*)message, message_len, Z_BEST_COMPRESSION);

    // 发送压缩后的数据
    ssize_t ret = gnutls_record_send(c->session, compressed_msg, compressed_len);
    if (ret < 0) {
        perror("gnutls_record_send");
        return -1;
    }

    SGATE_LOG("Sent data: %d 字节", message_len);

    return ret;
}

static inline ssize_t ship_send(ship_t* c, const void* buffer, size_t len) {
    //int ret;
    //LOOP_CHECK(ret, gnutls_record_send(c->session, buffer, len));
    //return ret;
    return gnutls_record_send(c->session, buffer, len);
}

/* Send a raw packet away. */
static int send_raw(ship_t* c, int len, uint8_t* sendbuf) {
    __try {
        ssize_t rv, total = 0;

        if (sendbuf == NULL || isPacketEmpty(sendbuf, len) || len == 0 || len > MAX_PACKET_BUFF) {
            //ERR_LOG("空指针数据包或无效长度 %d 数据包.", len);
            print_ascii_hex(errl, sendbuf, len);
            return 0;
        }

        shipgate_hdr_t* pkt = (shipgate_hdr_t*)sendbuf;

        DATA_LOG("ship_t send_raw \ntype:0x%04X \nlen:0x%04X \nversion:0x%02X \nreserved:0x%02X \nflags:0x%04X"
            , ntohs(pkt->pkt_type), ntohs(pkt->pkt_len), pkt->version, pkt->reserved, pkt->flags);

        pthread_rwlock_wrlock(&c->rwlock);
        //pthread_mutex_lock(&c->pkt_mutex);

        /* Keep trying until the whole thing's sent. */
        if (!c->has_key || c->sock < 0 || !c->sendbuf_cur) {

            //DBG_LOG("send_raw");
            //print_ascii_hex(dbgl, sendbuf, len);

            while (total < len) {
                rv = ship_send(c, sendbuf + total, len - total);
                //TEST_LOG("向端口 %d 发送数据 %d 字节", c->sock, rv);

                /* 检测数据是否以发送完成? */
                if (rv == GNUTLS_E_AGAIN || rv == GNUTLS_E_INTERRUPTED) {
                    /* Try again. */
                    continue;
                }
                else if (rv < 0) {
                    pthread_rwlock_unlock(&c->rwlock);
                    //pthread_mutex_unlock(&c->pkt_mutex);
                    ERR_LOG("Gnutls *** 错误: %s", gnutls_strerror(rv));
                    ERR_LOG("Gnutls *** 发送损坏的数据长度(%d). 取消响应.", rv);
                    //free_safe(sendbuf);
                    return -1;
                }

                total += rv;
            }
        }

        pthread_rwlock_unlock(&c->rwlock);
        //pthread_mutex_unlock(&c->pkt_mutex);
        //free_safe(sendbuf);

        return 0;
    }

    __except (crash_handler(GetExceptionInformation())) {
        //pthread_mutex_unlock(&c->pkt_mutex);
        // 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

        CRASH_LOG("出现错误, 程序将退出.");
        (void)getchar();
        return -4;
    }
}

/* Encrypt a packet, and send it away. */
static int send_crypt(ship_t* c, int len, uint8_t* sendbuf) {
    /* Make sure its at least a header in length. */
    if (len < 8) {
        ERR_LOG(
            "send_crypt端口 %d 长度 = %d字节", c->sock, len);
        return -1;
    }

    return send_raw(c, len, sendbuf);
}

int forward_dreamcast(ship_t* c, dc_pkt_hdr_t* dc, uint32_t sender,
    uint32_t gc, uint32_t block) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_fw_9_pkt* pkt = (shipgate_fw_9_pkt*)sendbuf;
    int dc_len = LE16(dc->pkt_len);
    int full_len = sizeof(shipgate_fw_9_pkt) + dc_len;

    /* Round up the packet size, if needed. */
    if (full_len & 0x07)
        full_len = (full_len + 8) & 0xFFF8;

    /* Scrub the buffer */
    memset(pkt, 0, full_len);

    /* Fill in the shipgate header */
    pkt->hdr.pkt_len = htons(full_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_DC);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    /* Add the metadata */
    pkt->ship_id = htonl(sender);
    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);

    /* Copy in the packet, unchanged */
    if (safe_memcpy(pkt->pkt, (uint8_t*)dc, dc_len, pkt->pkt, pkt->pkt + dc_len)) {
#ifdef DEBUG
        DBG_LOG("安全复制成功！");
        DBG_LOG("After safe memcpy:");
        DBG_LOG("dest: ");
        for (int i = 0; i < dc_len; i++) {
            DBG_LOG("%d ", pkt->pkt[i]);
        }
#endif // DEBUG
    }
    else {
        ERR_LOG("安全复制失败!请检查数据包情况!");
    }
    //memcpy(pkt->pkt, dc, dc_len);

    /* 将数据包发送出去 */
    return send_crypt(c, full_len, sendbuf);
}

int forward_pc(ship_t* c, dc_pkt_hdr_t* pc, uint32_t sender, uint32_t gc,
    uint32_t block) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_fw_9_pkt* pkt = (shipgate_fw_9_pkt*)sendbuf;
    int pc_len = LE16(pc->pkt_len);
    int full_len = sizeof(shipgate_fw_9_pkt) + pc_len;

    /* Round up the packet size, if needed. */
    if (full_len & 0x07)
        full_len = (full_len + 8) & 0xFFF8;

    /* Scrub the buffer */
    memset(pkt, 0, full_len);

    /* Fill in the shipgate header */
    pkt->hdr.pkt_len = htons(full_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_PC);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    /* Add the metadata */
    pkt->ship_id = htonl(sender);
    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);

    /* Copy in the packet, unchanged */
    if (safe_memcpy(pkt->pkt, (uint8_t*)pc, pc_len, pkt->pkt, pkt->pkt + pc_len)) {
#ifdef DEBUG
        DBG_LOG("安全复制成功！");
        DBG_LOG("After safe memcpy:");
        DBG_LOG("dest: ");
        for (int i = 0; i < pc_len; i++) {
            DBG_LOG("%d ", pkt->pkt[i]);
        }
#endif // DEBUG
    }
    else {
        ERR_LOG("安全复制失败!请检查数据包情况!");
    }
    //memcpy(pkt->pkt, pc, pc_len);

    /* 将数据包发送出去 */
    return send_crypt(c, full_len, sendbuf);
}

int forward_bb(ship_t* c, bb_pkt_hdr_t* bb, uint32_t sender, uint32_t gc,
    uint32_t block) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_fw_9_pkt* pkt = (shipgate_fw_9_pkt*)sendbuf;
    int bb_len = LE16(bb->pkt_len);
    int full_len = sizeof(shipgate_fw_9_pkt) + bb_len;

    /* Round up the packet size, if needed. */
    if (full_len & 0x07)
        full_len = (full_len + 8) & 0xFFF8;

    /* Scrub the buffer */
    memset(pkt, 0, full_len);

    /* Fill in the shipgate header */
    pkt->hdr.pkt_len = htons(full_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_BB);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    /* Add the metadata */
    pkt->ship_id = htonl(sender);
    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);

    /* Copy in the packet, unchanged */
    if (safe_memcpy(pkt->pkt, (uint8_t*)bb, bb_len, pkt->pkt, pkt->pkt + bb_len)) {
#ifdef DEBUG
        DBG_LOG("安全复制成功！");
        DBG_LOG("After safe memcpy:");
        DBG_LOG("dest: ");
        for (int i = 0; i < bb_len; i++) {
            DBG_LOG("%d ", pkt->pkt[i]);
        }
#endif // DEBUG
    }
    else {
        ERR_LOG("安全复制失败!请检查数据包情况!");
    }
    //safe_memcpy(pkt->pkt, bb, bb_len);

    /* 将数据包发送出去 */
    return send_crypt(c, full_len, sendbuf);
}

/* Send a welcome packet to the given ship. */
int send_welcome(ship_t* c) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_login_pkt* pkt = (shipgate_login_pkt*)sendbuf;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_login_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_login_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_LOGIN);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    /* Fill in the required message */
    strcpy(pkt->msg, shipgate_login_msg);

    /* Fill in the version information */
    parse_version(&pkt->ver_major, &pkt->ver_minor, &pkt->ver_micro, SGATE_SERVER_VERSION);

    /* Fill in the nonces */
    memcpy(pkt->ship_nonce, c->ship_nonce, 4);
    memcpy(pkt->gate_nonce, c->gate_nonce, 4);

    /* 将数据包发送出去 */
    return send_raw(c, sizeof(shipgate_login_pkt), sendbuf);
}

int send_ship_status(ship_t* c, ship_t* o, uint16_t status) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_ship_status6_pkt* pkt = (shipgate_ship_status6_pkt*)sendbuf;

    /* If the ship hasn't finished logging in yet, don't send this. */
    if (o->ship_name[0] == 0)
        return 0;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_ship_status6_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_ship_status6_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_SSTATUS);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    /* Fill in the info */
    memcpy(pkt->name, o->ship_name, 12);
    pkt->ship_id = htonl(o->key_idx);
    memcpy(pkt->ship_host4, o->remote_host4, 32);
    memcpy(pkt->ship_host6, o->remote_host6, 128);
    pkt->ship_addr4 = o->remote_addr4;
    memcpy(pkt->ship_addr6, &o->remote_addr6, 16);
    pkt->ship_port = htons(o->port);
    pkt->status = htons(status);
    pkt->flags = htonl(o->flags);
    pkt->clients = htons(o->clients);
    pkt->games = htons(o->games);
    pkt->menu_code = htons(o->menu_code);
    pkt->ship_number = (uint8_t)o->ship_number;
    pkt->privileges = htonl(o->privileges);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_ship_status6_pkt), sendbuf);
}

/* Send a ping packet to a client. */
int send_ping(ship_t* c, int reply) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_ping_t* pkt = (shipgate_ping_t*)sendbuf;

    /* 填充数据头. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_ping_t));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_PING);
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    if (reply)
        pkt->hdr.flags = htons(SHDR_RESPONSE);
    else
        pkt->hdr.flags = 0;

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_ping_t), sendbuf);
}

/* Send the ship a character data restore. */
int send_cdata(ship_t* c, uint32_t gc, uint32_t slot, void* cdata, int sz,
    uint32_t block) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_char_data_pkt* pkt = (shipgate_char_data_pkt*)sendbuf;

    /* 填充数据头. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_char_data_pkt) + sz);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_CREQ);
    pkt->hdr.flags = htons(SHDR_RESPONSE);
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    /* Fill in the body. */
    pkt->guildcard = htonl(gc);
    pkt->slot = htonl(slot);
    pkt->block = htonl(block);
    memcpy(pkt->data, cdata, sz);

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_char_data_pkt) + sz, sendbuf);
}

/* Send the ship a character common bank data restore. */
int send_common_bank_data(ship_t* c, uint32_t gc, uint32_t slot, void* cbdata, int sz,
    uint32_t block) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_char_data_pkt* pkt = (shipgate_char_data_pkt*)sendbuf;

    /* 填充数据头. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_char_data_pkt) + sz);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_CREQ);
    pkt->hdr.flags = htons(SHDR_RESPONSE);
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    /* Fill in the body. */
    pkt->guildcard = htonl(gc);
    pkt->slot = htonl(slot);
    pkt->block = htonl(block);
    memcpy(pkt->data, cbdata, sz);

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_char_data_pkt) + sz, sendbuf);
}

/* Send a reply to a user login request. */
int send_usrloginreply(ship_t* c, uint32_t gc, uint32_t block, int good,
    uint32_t p) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_usrlogin_reply_pkt* pkt = (shipgate_usrlogin_reply_pkt*)sendbuf;
    uint16_t flags = good ? SHDR_RESPONSE : SHDR_FAILURE;

    /* Clear the packet first */
    memset(pkt, 0, sizeof(shipgate_usrlogin_reply_pkt));

    /* Fill in the response. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_usrlogin_reply_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_USRLOGIN);
    pkt->hdr.flags = htons(flags);
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);

    /* In protocol versions less than 18, priv was an 8 bit field. Since
       multibyte stuff is in network byte order, we have to shift to make that
       work. */
    if (c->proto_ver < 18)
        pkt->priv = htonl(p << 24);
    else
        pkt->priv = htonl(p);

    return send_crypt(c, sizeof(shipgate_usrlogin_reply_pkt), sendbuf);
}

/* Send a client/game update packet. */
int send_counts(ship_t* c, uint32_t ship_id, uint16_t clients, uint16_t games) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_cnt_pkt* pkt = (shipgate_cnt_pkt*)sendbuf;

    /* Clear the packet first */
    memset(pkt, 0, sizeof(shipgate_cnt_pkt));

    /* Fill in the response. */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_cnt_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_COUNT);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    pkt->clients = htons(clients);
    pkt->games = htons(games);
    pkt->ship_id = htonl(ship_id);

    return send_crypt(c, sizeof(shipgate_cnt_pkt), sendbuf);
}

/* Send an error packet to a ship */
int send_error(ship_t* c, uint16_t type, uint16_t flags, uint32_t err,
    const uint8_t* data, int data_sz, 
    uint32_t guildcard, uint32_t slot, uint32_t block, uint32_t target_gc, uint32_t reserved) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_error_pkt* pkt = (shipgate_error_pkt*)sendbuf;
    uint16_t sz;

    /* Make sure the data size is valid */
    if (data_sz > MAX_PACKET_BUFF - sizeof(shipgate_error_pkt))
        return -1;

    /* Clear the header of the packet */
    memset(pkt, 0, sizeof(shipgate_error_pkt));
    sz = sizeof(shipgate_error_pkt) + data_sz;

    /* Fill it in */
    pkt->hdr.pkt_len = htons(sz);
    pkt->hdr.pkt_type = htons(type);
    pkt->hdr.flags = htons(flags);
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;
    pkt->guildcard = htonl(guildcard);
    pkt->slot = htonl(slot);
    pkt->block = htonl(block);
    pkt->target_gc = htonl(target_gc);
    pkt->reserved = htonl(reserved);

    pkt->error_code = htonl(err);

    memcpy(pkt->data, data, data_sz);

    return send_crypt(c, sz, sendbuf);
}

/* Send a packet to tell a client that a friend has logged on or off */
int send_friend_message(ship_t* c, int on, uint32_t dest_gc,
    uint32_t dest_block, uint32_t friend_gc,
    uint32_t friend_block, uint32_t friend_ship,
    const char* friend_name, const char* nickname) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_friend_login_4_pkt* pkt = (shipgate_friend_login_4_pkt*)sendbuf;

    /* Clear the packet */
    memset(pkt, 0, sizeof(shipgate_friend_login_4_pkt));

    /* Fill it in */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_friend_login_4_pkt));
    pkt->hdr.pkt_type = htons((on ? SHDR_TYPE_FRLOGIN : SHDR_TYPE_FRLOGOUT));
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;
    pkt->dest_guildcard = htonl(dest_gc);
    pkt->dest_block = htonl(dest_block);
    pkt->friend_guildcard = htonl(friend_gc);
    pkt->friend_ship = htonl(friend_ship);
    pkt->friend_block = htonl(friend_block);
    strcpy(pkt->friend_name, friend_name);

    if (nickname) {
        strncpy(pkt->friend_nick, nickname, 32);
        pkt->friend_nick[31] = 0;
    }
    else {
        memset(pkt->friend_nick, 0, 32);
    }

    return send_crypt(c, sizeof(shipgate_friend_login_4_pkt), sendbuf);
}

/* Send a kick packet */
int send_kick(ship_t* c, uint32_t requester, uint32_t user, uint32_t block,
    const char* reason) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_kick_pkt* pkt = (shipgate_kick_pkt*)sendbuf;

    /* Scrub the buffer */
    memset(pkt, 0, sizeof(shipgate_kick_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_kick_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_KICK);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    pkt->requester = htonl(requester);
    pkt->guildcard = htonl(user);
    pkt->block = htonl(block);

    if (reason)
        strncpy(pkt->reason, reason, 64);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_kick_pkt), sendbuf);
}

/* Send a portion of a user's friendlist to the user */
int send_friendlist(ship_t* c, uint32_t requester, uint32_t block,
    int count, const friendlist_data_t* entries) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_friend_list_pkt* pkt = (shipgate_friend_list_pkt*)sendbuf;
    uint16_t len = sizeof(shipgate_friend_list_pkt) +
        sizeof(friendlist_data_t) * count;

    /* 填充数据头 */
    pkt->hdr.pkt_len = htons(len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_FRLIST);
    pkt->hdr.flags = htons(SHDR_RESPONSE);
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    pkt->requester = htonl(requester);
    pkt->block = htonl(block);

    /* Copy the friend data */
    memcpy(pkt->entries, entries, sizeof(friendlist_data_t) * count);

    /* 将数据包发送出去 */
    return send_crypt(c, len, sendbuf);
}

/* Send a global message packet to a ship */
int send_global_msg(ship_t* c, uint32_t requester, const char* text,
    uint16_t text_len) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_global_msg_pkt* pkt = (shipgate_global_msg_pkt*)sendbuf;
    uint16_t len = sizeof(shipgate_global_msg_pkt) + text_len;

    /* 填充数据头 */
    pkt->hdr.pkt_len = htons(len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_GLOBALMSG);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    pkt->requester = htonl(requester);
    pkt->reserved = 0;
    memcpy(pkt->text, text, len);

    /* 将数据包发送出去 */
    return send_crypt(c, len, sendbuf);
}

/* Begin an options packet */
void* user_options_begin(uint32_t guildcard, uint32_t block) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_user_opt_pkt* pkt = (shipgate_user_opt_pkt*)sendbuf;

    /* 填充数据头 */
    pkt->hdr.pkt_len = sizeof(shipgate_user_opt_pkt);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_USEROPT);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    pkt->guildcard = htonl(guildcard);
    pkt->block = htonl(block);
    pkt->count = 0;
    pkt->reserved = 0;

    /* Return the pointer to the end of the buffer */
    return &pkt->options[0];
}

/* Append an option value to the options packet */
void* user_options_append(void* p, uint32_t opt, uint32_t len,
    const uint8_t* data) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_user_opt_pkt* pkt = (shipgate_user_opt_pkt*)sendbuf;
    shipgate_user_opt_t* o = (shipgate_user_opt_t*)p;
    int padding = 8 - (len & 7);

    /* Add the new option in */
    o->option = htonl(opt);
    memcpy(o->data, data, len);

    /* Options must be a multiple of 8 bytes in length */
    while (padding--) {
        o->data[len++] = 0;
    }

    o->length = htonl(len + 8);

    /* Adjust the packet's information to account for the new option */
    pkt->hdr.pkt_len += len + 8;
    ++pkt->count;

    return (((uint8_t*)p) + len + 8);
}

/* Finish off a user options packet and send it along */
int send_user_options(ship_t* c) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_user_opt_pkt* pkt = (shipgate_user_opt_pkt*)sendbuf;
    uint16_t len = pkt->hdr.pkt_len;

    /* Make sure we have something to send, at least */
    if (!pkt->count)
        return 0;

    /* Swap that which we need to do */
    pkt->hdr.pkt_len = htons(len);
    pkt->count = htonl(pkt->count);

    /* 加密并发送 */
    return send_crypt(c, len, sendbuf);
}

/* 发送客户端 Blue Burst 选项数据 */
int send_bb_opts(ship_t* c, uint32_t gc, uint32_t block,
    psocn_bb_db_opts_t* opts, psocn_bb_db_guild_t* guild, 
    uint32_t guild_points_personal_donation, psocn_bank_t* common_bank) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_bb_opts_pkt* pkt = (shipgate_bb_opts_pkt*)sendbuf;

    memset(pkt, 0, sizeof(shipgate_bb_opts_pkt));

    /* 填充数据头 */
    pkt->hdr.pkt_len = htons(sizeof(shipgate_bb_opts_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_BBOPTS);
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;
    pkt->hdr.flags = htons(SHDR_RESPONSE);

    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);

    memcpy(&pkt->opts, opts, PSOCN_STLENGTH_BB_DB_OPTS);

    /* 填充公会数据 */
    memcpy(&pkt->guild, guild, PSOCN_STLENGTH_BB_GUILD);

    pkt->guild_points_personal_donation = guild_points_personal_donation;

    /* 填充公共仓库数据 */
    memcpy(&pkt->common_bank, common_bank, PSOCN_STLENGTH_BANK);

    /* 将数据包发送出去 */
    return send_crypt(c, sizeof(shipgate_bb_opts_pkt), sendbuf);
}

/* Send a system-generated simple mail message. */
int send_simple_mail(ship_t* c, uint32_t gc, uint32_t block, uint32_t sender,
    const char* name, const char* msg) {
    simple_mail_pkt pkt;
    size_t amt = strlen(name);

    /* Set up the mail. */
    memset(&pkt, 0, sizeof(simple_mail_pkt));
    pkt.dc.pkt_type = SIMPLE_MAIL_TYPE;
    pkt.dc.pkt_len = LE16(DC_SIMPLE_MAIL_LENGTH);
    pkt.player_tag = LE32(0x00010000);
    pkt.gc_sender = LE32(sender);

    /* Thank you GCC for this completely unnecessary warning that means I have
       to do this stupid song and dance to get rid of it (or tag the variable
       being used with a GCC-specific attribute) Basically, strncpy is totally
       useless now when you're dealing with things that need not be
       terminated. */
    if (amt > 16)
        amt = 16;

    //istrncpy(ic_gbk_to_utf8, pkt.name, name, amt);

    memcpy(pkt.dcmaildata.dcname, name, amt);

    pkt.dcmaildata.gc_dest = LE32(gc);

    //istrncpy(ic_gbk_to_utf8, pkt.stuff, msg, 0x90);
    strncpy(pkt.dcmaildata.stuff, msg, 0x90);

    return forward_dreamcast(c, (dc_pkt_hdr_t*)&pkt, c->key_idx, gc, block);
}

/* Send a chunk of scripting code to a ship. */
int send_script_chunk(ship_t* c, const char* local_fn, const char* remote_fn,
    uint8_t type, uint32_t file_len, uint32_t crc) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_schunk_pkt* pkt = (shipgate_schunk_pkt*)sendbuf;
    FILE* fp;

    /* Don't try to send these to a ship that won't know what to do with them */
    if (c->proto_ver < 16 || !(c->flags & LOGIN_FLAG_LUA))
        return 0;

    /* Make sure it isn't too large... */
    if (file_len > 32768) {
        ERR_LOG("发送的脚本文件 %s 数据过大",
            local_fn);
        return -1;
    }

    char luafile[32] = { 0 };
    sprintf_s(luafile, sizeof(luafile), "Scripts/%s", local_fn);
    errno_t err = fopen_s(&fp, luafile, "rb");

    if (err) {
        ERR_LOG("无法读取脚本文件 '%s'", local_fn);
        return -1;
    }
    else {
        /* 填充数据头 and such */
        memset(pkt, 0, sizeof(shipgate_schunk_pkt));
        pkt->hdr.pkt_len = htons(file_len + sizeof(shipgate_schunk_pkt));
        pkt->hdr.pkt_type = htons(SHDR_TYPE_SCHUNK);
        pkt->chunk_type = type;
        pkt->chunk_length = htonl((uint32_t)file_len);
        pkt->chunk_crc = htonl(crc);
        strncpy(pkt->filename, remote_fn, 32);

        /* Copy in the chunk */
        if (fread(pkt->chunk, 1, file_len, fp) != file_len) {
            fclose(fp);
            ERR_LOG("无法读取脚本文件 %s 数据", local_fn);
            return -1;
        }

        fclose(fp);
        /* 加密并发送 */
        return send_crypt(c, file_len + sizeof(shipgate_schunk_pkt), sendbuf);
    }
}

/* Send a packet to check if a particular script is in its current form on a
   ship. */
int send_script_check(ship_t* c, ship_script_t* scr) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_schunk_pkt* pkt = (shipgate_schunk_pkt*)sendbuf;
    uint8_t type = scr->module ? SCHUNK_TYPE_MODULE : SCHUNK_TYPE_SCRIPT;

    /* Don't try to send these to a ship that won't know what to do with them */
    if (c->proto_ver < 16 || !(c->flags & LOGIN_FLAG_LUA))
        return 0;

    /* Fill in the easy stuff */
    memset(pkt, 0, sizeof(shipgate_schunk_pkt));
    pkt->hdr.pkt_len = htons(sizeof(shipgate_schunk_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_SCHUNK);
    pkt->chunk_type = SCHUNK_CHECK | type;
    pkt->chunk_length = htonl(scr->len);
    strncpy(pkt->filename, scr->remote_fn, 32);
    pkt->chunk_crc = htonl(scr->crc);

    if (scr->event)
        pkt->action = htonl(scr->event);

    /* 加密并发送 */
    return send_crypt(c, sizeof(shipgate_schunk_pkt), sendbuf);
}

/* Send a packet to send a script to the a ship. */
int send_script(ship_t* c, ship_script_t* scr) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_schunk_pkt* pkt = (shipgate_schunk_pkt*)sendbuf;
    FILE* fp;
    uint16_t pkt_len;

    /* Don't try to send these to a ship that won't know what to do with them */
    if (c->proto_ver < 16 || !(c->flags & LOGIN_FLAG_LUA))
        return 0;

    SGATE_LOG("正在发送 %s 舰船脚本文件 '%s' (%s)", c->ship_name,
        scr->remote_fn, scr->local_fn);

    pkt_len = sizeof(shipgate_schunk_pkt) + scr->len;
    if (pkt_len & 0x07)
        pkt_len = (pkt_len + 8) & 0xFFF8;

    /* Fill in the easy stuff */
    memset(pkt, 0, pkt_len);
    pkt->hdr.pkt_len = htons(pkt_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_SCHUNK);
    pkt->chunk_type = scr->module ? SCHUNK_TYPE_MODULE : SCHUNK_TYPE_SCRIPT;
    pkt->chunk_length = htonl(scr->len);
    strncpy(pkt->filename, scr->remote_fn, 32);
    pkt->chunk_crc = htonl(scr->crc);

    if (scr->event)
        pkt->action = htonl(scr->event);

    char luafile[32] = { 0 };
    sprintf_s(luafile, sizeof(luafile), "Scripts/%s", scr->local_fn);
    /* Read the script file in... */
    errno_t err = fopen_s(&fp, luafile, "rb");

    if (err) {
        ERR_LOG("无法读取脚本文件 '%s'", scr->local_fn);
        return 0;
    }
    else {
        if (fread(pkt->chunk, 1, scr->len, fp) != scr->len) {
            fclose(fp);
            ERR_LOG("脚本文件 '%s' 长度发生变化?", scr->local_fn);
            return 0;
        }

        fclose(fp);
        /* 加密并发送 */
        return send_crypt(c, pkt_len, sendbuf);
    }

}

int send_sset(ship_t* c, uint32_t action, ship_script_t* scr) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_sset_pkt* pkt = (shipgate_sset_pkt*)sendbuf;

    /* Don't try to send these to a ship that won't know what to do with them */
    if (c->proto_ver < 16 || !(c->flags & LOGIN_FLAG_LUA))
        return 0;

    /* Make sure the requested operation makes sense... */
    if (scr && scr->module)
        return 0;

    /* Fill in the easy stuff */
    memset(pkt, 0, sizeof(shipgate_sset_pkt));
    pkt->hdr.pkt_len = htons(sizeof(shipgate_sset_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_SSET);
    pkt->action = htonl(action);

    if (scr)
        strncpy(pkt->filename, scr->remote_fn, 32);

    /* 加密并发送 */
    return send_crypt(c, sizeof(shipgate_sset_pkt), sendbuf);
}

/* Send a script data packet */
int send_sdata(ship_t* c, uint32_t gc, uint32_t block, uint32_t event,
    const uint8_t* data, uint32_t len) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_sdata_pkt* pkt = (shipgate_sdata_pkt*)sendbuf;
    uint16_t pkt_len;

    /* Don't try to send these to a ship that won't know what to do with them */
    if (c->proto_ver < 16 || !(c->flags & LOGIN_FLAG_LUA))
        return 0;

    /* Make sure the length is sane... */
    if (len > 32768) {
        ERR_LOG("舰船脚本数据包太大了!");
        return -1;
    }

    pkt_len = sizeof(shipgate_sdata_pkt) + len;
    if (pkt_len & 0x07)
        pkt_len = (pkt_len + 8) & 0xFFF8;

    /* 填充数据并准备发送.. */
    memset(pkt, 0, pkt_len);
    pkt->hdr.pkt_len = htons(pkt_len);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_SDATA);
    pkt->event_id = htonl(event);
    pkt->data_len = htonl(len);
    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);
    memcpy(pkt->data, data, len);

    /* 加密并发送. */
    return send_crypt(c, pkt_len, sendbuf);
}

/* Send a quest flag response */
int send_qflag(ship_t* c, uint16_t type, uint32_t gc, uint32_t block,
    uint32_t fid, uint32_t qid, uint32_t value, uint32_t ctl) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_qflag_pkt* pkt = (shipgate_qflag_pkt*)sendbuf;

    /* Don't try to send these to a ship that won't know what to do with them */
    if (c->proto_ver < 17)
        return 0;

    /* 填充数据并准备发送.. */
    memset(pkt, 0, sizeof(shipgate_qflag_pkt));
    pkt->hdr.pkt_len = htons(sizeof(shipgate_qflag_pkt));
    pkt->hdr.pkt_type = htons(type);
    pkt->hdr.flags = htons(SHDR_RESPONSE);
    pkt->guildcard = htonl(gc);
    pkt->block = htonl(block);
    pkt->flag_id = htonl((fid & 0xFFFF) | (ctl & 0xFFFF0000));
    pkt->quest_id = htonl(qid);
    pkt->flag_id_hi = htons(fid >> 16);
    pkt->value = htonl(value);

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_qflag_pkt), sendbuf);
}

/* Send a simple ship control request */
int send_sctl(ship_t* c, uint32_t ctl, uint32_t acc) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_shipctl_pkt* pkt = (shipgate_shipctl_pkt*)sendbuf;

    /* This packet doesn't exist until protocol 19. */
    if (c->proto_ver < 19)
        return 0;

    /* 填充数据并准备发送.. */
    memset(pkt, 0, sizeof(shipgate_shipctl_pkt));
    pkt->hdr.pkt_len = htons(sizeof(shipgate_shipctl_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_SHIP_CTL);

    pkt->ctl = htonl(ctl);
    pkt->acc = htonl(acc);

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_shipctl_pkt), sendbuf);
}

/* Send a shutdown/restart request */
int send_shutdown(ship_t* c, int restart, uint32_t acc, uint32_t when) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_sctl_shutdown_pkt* pkt = (shipgate_sctl_shutdown_pkt*)sendbuf;

    /* This packet doesn't exist until protocol 19. */
    if (c->proto_ver < 19)
        return 0;

    /* 填充数据并准备发送.. */
    memset(pkt, 0, sizeof(shipgate_sctl_shutdown_pkt));
    pkt->hdr.pkt_len = htons(sizeof(shipgate_sctl_shutdown_pkt));
    pkt->hdr.pkt_type = htons(SHDR_TYPE_SHIP_CTL);

    if (restart)
        pkt->ctl = htonl(SCTL_TYPE_RESTART);
    else
        pkt->ctl = htonl(SCTL_TYPE_SHUTDOWN);

    pkt->acc = htonl(acc);
    pkt->when = htonl(when);

    /* 加密并发送. */
    return send_crypt(c, sizeof(shipgate_sctl_shutdown_pkt), sendbuf);
}

/* Begin an blocklist packet */
void user_blocklist_begin(uint32_t guildcard, uint32_t block) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_user_blocklist_pkt* pkt = (shipgate_user_blocklist_pkt*)sendbuf;

    /* 填充数据头 */
    pkt->hdr.pkt_len = sizeof(shipgate_user_blocklist_pkt);
    pkt->hdr.pkt_type = htons(SHDR_TYPE_UBLOCKS);
    pkt->hdr.flags = 0;
    pkt->hdr.reserved = 0;
    pkt->hdr.version = 0;

    pkt->guildcard = htonl(guildcard);
    pkt->block = htonl(block);
    pkt->count = 0;
    pkt->reserved = 0;
}

/* Append a value to the blocklist packet */
void user_blocklist_append(uint32_t gc, uint32_t flags) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_user_blocklist_pkt* pkt = (shipgate_user_blocklist_pkt*)sendbuf;
    uint32_t i = pkt->count;

    /* Add the blocked user */
    pkt->entries[i].gc = htonl(gc);
    pkt->entries[i].flags = htonl(flags);

    /* Adjust the packet's information to account for the new option */
    pkt->hdr.pkt_len += 8;
    ++pkt->count;
}

/* Finish off a user blocklist packet and send it along */
int send_user_blocklist(ship_t* c) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_user_blocklist_pkt* pkt = (shipgate_user_blocklist_pkt*)sendbuf;
    uint16_t len = pkt->hdr.pkt_len;

    /* Make sure we have something to send, at least */
    if (!pkt->count)
        return 0;

    /* Make sure we don't try to send to a ship that won't know what to do with
       the packet. */
    if (c->proto_ver < 19)
        return 0;

    /* Swap that which we need to do */
    pkt->hdr.pkt_len = htons(len);
    pkt->count = htonl(pkt->count);

    /* 加密并发送 */
    return send_crypt(c, len, sendbuf);
}

int send_user_error(ship_t* c, uint16_t pkt_type, uint32_t err_code,
    uint32_t gc, uint32_t block, const char* message) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_user_err_pkt* pkt = (shipgate_user_err_pkt*)sendbuf;
    uint16_t len = (uint16_t)(message ? strlen(message) : 0);
    uint16_t fl = err_code != ERR_NO_ERROR ? SHDR_FAILURE : 0;

    if (c->proto_ver < 19)
        return 0;

    DBG_LOG("send_user_error");

    /* Round up the length to the next multiple of 8. */
    len += sizeof(shipgate_user_err_pkt);
    if (len & 7)
        len = (len + 7) & (~7);

    memset(pkt, 0, len);
    pkt->base.hdr.pkt_type = htons(pkt_type);
    pkt->base.hdr.pkt_len = htons(len);
    pkt->base.hdr.flags = htons(SHDR_RESPONSE | fl);

    pkt->gc = htonl(gc);
    pkt->block = htonl(block);
    if (message)
        strcpy(pkt->message, message);
    else
        strcpy(pkt->message, "NO MSG");

    return send_crypt(c, len, sendbuf);
}

int send_max_tech_lvl_bb(ship_t* c, bb_max_tech_level_t* data) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_max_tech_lvl_bb_pkt* pkt = (shipgate_max_tech_lvl_bb_pkt*)sendbuf;
    uint16_t len = sizeof(shipgate_max_tech_lvl_bb_pkt);

    /* Make sure we don't try to send to a ship that won't know what to do with
       the packet. */
    if (c->proto_ver < 19)
        return 0;

    /* Swap that which we need to do */
    pkt->hdr.pkt_type = htons(SHDR_TYPE_BBMAXTECH);
    pkt->hdr.pkt_len = htons(len);
    pkt->hdr.flags = SHDR_RESPONSE;

    memcpy(pkt->data, data, sizeof(bb_max_tech_level_t) * MAX_PLAYER_TECHNIQUES);

#ifdef DEBUG
    DBG_LOG("测试 法术 %d.%s 职业 %d 等级 %d", 1, pkt->data[1].tech_name, 1, pkt->data[1].max_lvl[1]);

#endif // DEBUG

    /* 加密并发送 */
    return send_crypt(c, len, sendbuf);
}

int send_pl_lvl_data_bb(ship_t* c, uint8_t* data, uint32_t compressed_size) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_pl_level_bb_pkt* pkt = (shipgate_pl_level_bb_pkt*)sendbuf;
    uint16_t len = (uint16_t)(compressed_size + 4 + sizeof(shipgate_hdr_t));

    if (!sendbuf) {
        ERR_LOG("申请动态内存失败");
        return -1;
    }

    /* Make sure we don't try to send to a ship that won't know what to do with
       the packet. */
    if (c->proto_ver < 19)
        return 0;

    if (len & 0x07)
        len = (len + 8) & 0xFFF8;

    /* Swap that which we need to do */
    pkt->hdr.pkt_type = htons(SHDR_TYPE_BBLVLDATA);
    pkt->hdr.pkt_len = htons(len);
    pkt->hdr.flags = SHDR_RESPONSE;

    pkt->compressed_size = htonl(compressed_size);

    memcpy(pkt->data, data, compressed_size);

    //DBG_LOG("测试 法术 %d.%s 职业 %d 等级 %d", 1, pkt->data[1].tech_name, 1, pkt->data[1].max_lvl[1]);

    /* 加密并发送 */
    return send_crypt(c, len, sendbuf);
}

int send_player_max_tech_level_table_bb(ship_t* c) {
    int i;

    //bb_max_tech_level_t* bb_max_tech_level = (bb_max_tech_level_t*)malloc(sizeof(bb_max_tech_level_t) * MAX_PLAYER_TECHNIQUES);

    //if (!bb_max_tech_level)
    //    return 0;

    if (read_player_max_tech_level_table_bb(max_tech_level)) {
        ERR_LOG("无法读取 Blue Burst 法术等级数据表");
        return -2;
    }

    i = send_max_tech_lvl_bb(c, max_tech_level);
    //free_safe(bb_max_tech_level);

    return i;
}

int send_player_level_table_bb(ship_t* c) {
    int i;
    Bytef* cmp_buf;
    uLong cmp_sz;
    int compressed = ~Z_OK;
    int compress_power = 9;
    size_t data_size = sizeof(bb_level_table_t);
    bb_level_table_t bb_level_tb = { 0 };

    if (read_player_level_table_bb(&bb_level_tb)) {
        ERR_LOG("无法读取 Blue Burst 等级数据表");
        return -2;
    }

    /* 压缩角色数据 */
    cmp_sz = compressBound((uLong)data_size);

    if ((cmp_buf = (Bytef*)malloc(cmp_sz))) {
        compressed = compress2(cmp_buf, &cmp_sz, (Bytef*)&bb_level_tb,
            (uLong)data_size, compress_power);
    }

    if (compressed == Z_OK && cmp_sz < data_size) {
#ifdef DEBUG
        DBG_LOG("压缩成功 原大小 %d 压缩后 %d ", data_size, cmp_sz);
#endif // DEBUG
    }
    else {
        DBG_LOG("压缩失败 原大小 %d 压缩后 %d ", data_size, cmp_sz);
        return -3;
    }

    i = send_pl_lvl_data_bb(c, (uint8_t*)cmp_buf, cmp_sz);

    return i;
}

int send_def_mode_char_data_bb(ship_t* c, uint8_t* data, uint32_t compressed_size) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_default_mode_char_data_bb_pkt* pkt = (shipgate_default_mode_char_data_bb_pkt*)sendbuf;
    uint16_t len = (uint16_t)(compressed_size + 4 + sizeof(shipgate_hdr_t));

    /* Make sure we don't try to send to a ship that won't know what to do with
       the packet. */
    if (c->proto_ver < 19)
        return 0;

    if (len & 0x07)
        len = (len + 8) & 0xFFF8;

    /* Swap that which we need to do */
    pkt->hdr.pkt_type = htons(SHDR_TYPE_BBDEFAULT_MODE_DATA);
    pkt->hdr.pkt_len = htons(len);
    pkt->hdr.flags = SHDR_RESPONSE;

    pkt->compressed_size = htonl(compressed_size);

    memcpy(pkt->data, data, compressed_size);

    /* 加密并发送 */
    return send_crypt(c, len, sendbuf);
}

int send_default_mode_char_data_bb(ship_t* c) {
    int i = 0, j = 0, len = 0;
    Bytef* cmp_buf;
    uLong cmp_sz;
    int compressed = ~Z_OK;
    int compress_power = 9;
    size_t data_size = sizeof(psocn_bb_mode_char_t);

    psocn_bb_mode_char_t* mode_chars = (psocn_bb_mode_char_t*)malloc(data_size);

    if (!mode_chars) {
        ERR_LOG("给 mode_chars 分配内存失败!");
        return -1;
    }

    if (db_get_character_default_mode(mode_chars)) {
        ERR_LOG("舰闸获取职业初始数据错误, 请检查函数错误");
        return -2;
    }

    /* 压缩角色数据 */
    cmp_sz = compressBound((uLong)data_size);

    if ((cmp_buf = (Bytef*)malloc(cmp_sz))) {
        compressed = compress2(cmp_buf, &cmp_sz, (Bytef*)mode_chars,
            (uLong)data_size, compress_power);
    }

    if (compressed == Z_OK && cmp_sz < data_size) {
#ifdef DEBUG
        DBG_LOG("压缩成功 原大小 %d 压缩后 %d ", data_size, cmp_sz);
#endif // DEBUG
    }
    else {
        DBG_LOG("压缩失败 原大小 %d 压缩后 %d ", data_size, cmp_sz);
        return -3;
    }

    i = send_def_mode_char_data_bb(c, (uint8_t*)cmp_buf, cmp_sz);

    free(cmp_buf);
    free_safe(mode_chars);

    return i;
}

int send_recive_data_complete(ship_t* c) {
    uint8_t* sendbuf = get_sg_sendbuf();
    shipgate_hdr_t* pkt = (shipgate_hdr_t*)sendbuf;
    uint16_t len = (uint16_t)(sizeof(shipgate_hdr_t));

    /* Make sure we don't try to send to a ship that won't know what to do with
       the packet. */
    if (c->proto_ver < 19)
        return 0;

    if (len & 0x07)
        len = (len + 8) & 0xFFF8;

    /* Swap that which we need to do */
    pkt->pkt_type = htons(SHDR_TYPE_COMPLETE_DATA);
    pkt->pkt_len = htons(len);
    pkt->flags = SHDR_RESPONSE;

    /* 加密并发送 */
    return send_crypt(c, len, sendbuf);
}

