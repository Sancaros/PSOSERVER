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

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdint.h>
#include <iconv.h>

#include "ship_packets.h"
#include <WS2tcpip.h>

#define BUG_REPORT_GC   1

int setup_server_address(psocn_ship_t* cfg, char* ship_host4, char* ship_host6, uint32_t ship_ip4, uint8_t* ship_ip6);

void print_packet(unsigned char* pkt, int len);
void print_packet2(const unsigned char *pkt, int len);
void fprint_packet(FILE *fp, const unsigned char *pkt, int len, int rec);

int dc_bug_report(ship_client_t *c, simple_mail_pkt *pkt);
int pc_bug_report(ship_client_t *c, simple_mail_pkt *pkt);
int bb_bug_report(ship_client_t *c, simple_mail_pkt *pkt);

int pkt_log_start(ship_client_t *i);
int pkt_log_stop(ship_client_t *i);

int team_log_start(lobby_t *i);
int team_log_stop(lobby_t *i);
int team_log_write(lobby_t *l, uint32_t msg_type, const char *fmt, ...);

void *xmalloc(size_t size);
const void *my_ntop(struct sockaddr_storage *addr, char str[INET6_ADDRSTRLEN]);
int my_pton(int family, const char *str, struct sockaddr_storage *addr);
int open_sock(int family, uint16_t port);

const char *skip_lang_code(const char *input);

void make_disp_data(ship_client_t *s, ship_client_t *d, void *buf);
void update_lobby_event(void);

/* Actually implemented in list.c, not utils.c. */
int send_player_list(ship_client_t *c, const char *params);

uint64_t get_ms_time(void);

void memcpy_str(void * restrict d, const char * restrict s, size_t sz);

// 抽奖函数
static inline int lottery_num(sfmt_t* rng, size_t items_count) {
	size_t i = 0;

	// 计算权重总和
	int totalWeight = 0;
	for (i = 0; i < items_count; i++) {
		if (i < 7) {  // 前7个物品占比70%
			totalWeight += 7;
		}
		else if (i < 17) {  // 接下来的10个物品占比20%
			totalWeight += 2;
		}
		else {  // 剩下的8个物品占比10%
			totalWeight += 1;
		}
	}

	// 生成随机数（范围：0 到 totalWeight-1）
	int randomNumber = sfmt_genrand_uint32(rng) % totalWeight;

	// 根据随机数确定中奖物品
	int winningItem = -1;
	int cumulativeWeight = 0;
	for (i = 0; i < items_count; i++) {
		if (i < 7) {
			cumulativeWeight += 7;
		}
		else if (i < 17) {
			cumulativeWeight += 2;
		}
		else {
			cumulativeWeight += 1;
		}
		if (randomNumber < cumulativeWeight) {
			winningItem = i;
			break;
		}
	}

	return winningItem;
}

/* Internationalization support */
#ifdef HAVE_LIBMINI18N
#include <mini18n-multi.h>
#include "clients.h"

extern mini18n_t langs[CLIENT_LANG_ALL];
#define __(c, s) mini18n_get(langs[c->language_code], s)
#else
#define __(c, s) s
#endif

void init_i18n(void);
void cleanup_i18n(void);

#endif /* !UTILS_H */
