/*
    梦幻之星中国 舰船服务器
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

#ifndef SUBCMD_H
#define SUBCMD_H

#include <pso_character.h>

#include "clients.h"

#include <pso_subcmd_code.h>

#ifdef PACKED
#undef PACKED
#endif

#ifndef _WIN32
#define PACKED __attribute__((packed))
#else
#define PACKED __declspec(align(1))
#pragma pack(push, 1) 
#endif

/* General format of a subcommand packet. */
typedef struct subcmd_pkt {
    union {
        dc_pkt_hdr_t dc;
        pc_pkt_hdr_t pc;
    } hdr;
    uint8_t type;
    uint8_t size;
    uint8_t data[0];
} PACKED subcmd_pkt_t;

typedef struct bb_subcmd_pkt {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t data[0];
} PACKED bb_subcmd_pkt_t;

typedef struct subcmd_bb_switch_changed {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t switch_id;
    uint8_t unk1[6];
    uint8_t area;
    uint8_t enabled;
} PACKED subcmd_bb_switch_changed_pkt_t;

/* Guild card send packet (Dreamcast). */
typedef struct subcmd_dc_gcsend {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;
    uint32_t tag;
    uint32_t guildcard;
    char name[24];
    char text[88];
    uint8_t unused2;
    uint8_t one;
    uint8_t language;
    uint8_t section;
    uint8_t char_class;
    uint8_t padding[3];
} PACKED subcmd_dc_gcsend_t;

/* Guild card send packet (PC). */
typedef struct subcmd_pc_gcsend {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;
    uint32_t tag;
    uint32_t guildcard;
    uint16_t name[24];
    uint16_t text[88];
    uint32_t padding;
    uint8_t one;
    uint8_t language;
    uint8_t section;
    uint8_t char_class;
} PACKED subcmd_pc_gcsend_t;

/* Guild card send packet (Gamecube). */
typedef struct subcmd_gc_gcsend {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;
    uint32_t tag;
    uint32_t guildcard;
    char name[24];
    char text[104];
    uint32_t padding;
    uint8_t one;
    uint8_t language;
    uint8_t section;
    uint8_t char_class;
} PACKED subcmd_gc_gcsend_t;

/* Guild card send packet (Xbox). */
typedef struct subcmd_xb_gcsend {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unk;                       /* 0x0D 0xFB */
    uint32_t tag;
    uint32_t guildcard;
    uint64_t xbl_userid;
    char name[24];
    char text[512];                     /* Why so long? */
    uint8_t one;
    uint8_t language;
    uint8_t section;
    uint8_t char_class;
} PACKED subcmd_xb_gcsend_t;

/* Guild card send packet (Blue Burst) */
typedef struct subcmd_bb_gc_send {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;
    uint32_t guildcard;
    uint16_t name[0x18];
    uint16_t guild_name[0x10];
    uint16_t text[0x58];
    uint8_t one;
    uint8_t language;
    uint8_t section;
    uint8_t char_class;
} PACKED subcmd_bb_gcsend_t;

/* Request drop from enemy (DC/PC/GC) or box (DC/PC) */
typedef struct subcmd_itemreq {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;
    uint8_t area;
    uint8_t pt_index;
    uint16_t req;
    float x;
    float y;
    uint32_t unk2[2];
} PACKED subcmd_itemreq_t;

/* Request drop from enemy (Blue Burst) */
typedef struct subcmd_bb_itemreq {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;
    uint8_t area;
    uint8_t pt_index;
    uint16_t req;
    float x;
    float y;
    uint32_t unk2[2];
} PACKED subcmd_bb_itemreq_t;

/* Request drop from box (GC) */
typedef struct subcmd_bitemreq {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t params1;                      /* 0x80 0x3F?*/
    uint8_t area;
    uint8_t pt_index;                   /* Always 0x30 */
    uint16_t req;
    float x;
    float y;
    uint32_t unk2[2];
    uint16_t unk3;
    uint16_t params2;                      /* 0x80 0x3F? */
    uint32_t unused[3];                 /* All zeroes? */
} PACKED subcmd_bitemreq_t;

/* Request drop from box (Blue Burst) */
typedef struct subcmd_bb_bitemreq {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t params1;                      /* 0x80 0x3F?*/
    uint8_t area;
    uint8_t pt_index;                   /* Always 0x30 */
    uint16_t req;
    float x;
    float y;
    uint32_t unk2[2];
    uint16_t unk3;
    uint16_t params2;                      /* 0x80 0x3F? */
    uint32_t unused[3];                 /* All zeroes? */
} PACKED subcmd_bb_bitemreq_t;

typedef struct subcmd_itemgen {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t params;
    uint8_t area;
    uint8_t what;
    uint16_t req;
    float x;
    float y;
    uint32_t unk1;
    item_t item;
    //uint32_t item[3];
    //uint32_t item_id;
    uint32_t item2;
} PACKED subcmd_itemgen_t;

typedef struct subcmd_bb_itemgen {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t params;
    uint8_t area;
    uint8_t what;
    uint16_t req;
    float x;
    float y;
    uint32_t unk1;
    item_t item;
    //uint32_t item[3];
    //uint32_t item_id;
    //uint32_t item2;
} PACKED subcmd_bb_itemgen_t;

typedef struct subcmd_levelup {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t atp;
    uint16_t mst;
    uint16_t evp;
    uint16_t hp;
    uint16_t dfp;
    uint16_t ata;
    uint32_t level;
} PACKED subcmd_levelup_t;

/* Packet used to take an item from the bank. */
typedef struct subcmd_take_item {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t data_l[3];
    uint32_t item_id;
    uint32_t data2_l;
    uint32_t unk;
} PACKED subcmd_take_item_t;

/* Packet used when a client takes damage. */
typedef struct subcmd_take_damage {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t unk1;
    uint16_t hp_rem;
    uint32_t unk2[2];
} PACKED subcmd_take_damage_t;

typedef struct subcmd_bb_take_damage {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t unk1;
    uint16_t hp_rem;
    uint32_t unk2[2];
} PACKED subcmd_bb_take_damage_t;

typedef struct subcmd_bb_menu_req {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t menu_id;
    uint16_t unk; //一直都是 00 00 难道和menu_id一起的？
    uint16_t unk2;
    uint16_t unk3;
} PACKED subcmd_bb_menu_req_t;

typedef struct subcmd_bb_pipe_pkt {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unk1;
    uint8_t client_id;
    uint8_t unused;
    uint8_t area_id;
    uint8_t unused2;
    uint32_t unk[5];
} PACKED subcmd_bb_pipe_pkt_t;

/* Packet used after a client uses a tech. */
typedef struct subcmd_used_tech {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t tech;
    uint8_t unused2;
    uint8_t level;
    uint8_t unused3;
} PACKED subcmd_used_tech_t;

typedef struct subcmd_bb_used_tech {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t tech;
    uint8_t unused2;
    uint8_t level;
    uint8_t unused3;
} PACKED subcmd_bb_used_tech_t;

/* Packet used when a client drops an item from their inventory */
typedef struct subcmd_drop_item {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t unk;
    uint16_t area;
    uint32_t item_id;
    float x;
    float y;
    float z;
} PACKED subcmd_drop_item_t;

typedef struct subcmd_bb_drop_item {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t unk;
    uint16_t area;
    uint32_t item_id;
    float x;
    float y;
    float z;
} PACKED subcmd_bb_drop_item_t;

/* Packet used to destroy an item on the map or to remove an item from a
   client's inventory */
typedef struct subcmd_destroy_item {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t item_id;
    uint32_t amount;
} PACKED subcmd_destroy_item_t;

typedef struct subcmd_bb_feed_mag {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t mag_id;
    uint8_t unk1;
    uint8_t isfeed; // 01
    uint8_t unused1;
    uint8_t item_id;
    uint8_t unk2;
    uint8_t isused; // 01
    uint8_t unused2;
} PACKED subcmd_bb_feed_mag_t;

typedef struct subcmd_bb_destroy_item {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t item_id;
    uint32_t amount;
} PACKED subcmd_bb_destroy_item_t;

typedef struct subcmd_pick_up {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t item_id;
    uint8_t area;
    uint8_t unused2[3];
} PACKED subcmd_pick_up_t;

typedef struct subcmd_bb_pick_up {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t item_id;
    uint8_t area;
    uint8_t unused2[3];
} PACKED subcmd_bb_pick_up_t;

typedef struct subcmd_bb_destroy_map_item {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t client_id2;
    uint8_t unused2;
    uint8_t area;
    uint8_t unused3;
    uint32_t item_id;
} PACKED subcmd_bb_destroy_map_item_t;

/* Packet used when dropping part of a stack of items */
typedef struct subcmd_drop_stack {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t area;
    uint16_t unk;
    float x;
    float z;
    uint32_t item[3];
    uint32_t item_id;
    uint32_t item2;
    uint32_t two;
} PACKED subcmd_drop_stack_t;

typedef struct subcmd_bb_drop_stack {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t area;
    float x;
    float z;
    uint32_t item[3];
    uint32_t item_id;
    uint32_t item2;
} PACKED subcmd_bb_drop_stack_t;

/* Packet used to update other people when a player warps to another area */
typedef struct subcmd_set_area {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t area;
    uint8_t unused2[3];
} PACKED subcmd_set_area_t;

typedef struct subcmd_bb_set_area {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t area;
    uint8_t unused2[3];
} PACKED subcmd_bb_set_area_t;

/* Packets used to set a user's position */
typedef struct subcmd_set_pos {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t unk;
    float w;
    float x;
    float y;
    float z;
} PACKED subcmd_set_pos_t;

typedef struct subcmd_bb_set_pos {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t unk;
    float w;
    float x;
    float y;
    float z;
} PACKED subcmd_bb_set_pos_t;

/* Packet used for moving around */
typedef struct subcmd_move {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    float x;
    float z;
    uint32_t unused2;   /* Not present in 0x42 */
} PACKED subcmd_move_t;

typedef struct subcmd_bb_move {
    bb_pkt_hdr_t hdr;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    float x;
    float z;
    uint32_t unused2;   /* Not present in 0x42 */
} PACKED subcmd_bb_move_t;

typedef struct subcmd_bb_natk {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    float x;
    float z;
    uint8_t unused2[2];   /* Not present in 0x42 */
} PACKED subcmd_bb_natk_t;

/* Packet used to teleport to a specified position */
typedef struct subcmd_teleport {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    float x;
    float y;
    float z;
    float w;
} PACKED subcmd_teleport_t;

/* Packet used when buying an item from the shop */
typedef struct subcmd_buy {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t item[3];
    uint32_t item_id;
    uint32_t meseta;
} PACKED subcmd_buy_t;

/* Packet used when an item has been used */
typedef struct subcmd_use_item {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t item_id;
} PACKED subcmd_use_item_t;

/* Packet used when an item has been used */
typedef struct subcmd_bb_use_item {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t item_id;
} PACKED subcmd_bb_use_item_t;

/* Packet used for word select */
typedef struct subcmd_word_select {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t client_id_gc;
    uint8_t num_words;
    uint8_t unused1;
    uint8_t ws_type;
    uint8_t unused2;
    uint16_t words[12];
} PACKED subcmd_word_select_t;

typedef struct subcmd_bb_word_select {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t client_id_gc;
    uint8_t num_words;
    uint8_t unused1;
    uint8_t ws_type;
    uint8_t unused2;
    uint16_t words[12];
} PACKED subcmd_bb_word_select_t;

typedef struct subcmd_bb_set_flag {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t unk1[0x02];
    uint8_t flag;
    uint8_t unused;
    uint8_t q1flag;
    uint8_t unk2[0x05];
} PACKED subcmd_bb_set_flag_t;

typedef struct subcmd_bb_killed_monster {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t unk1;
    uint8_t unk2;
    uint8_t flag;
    uint8_t unk3;
    uint8_t unk4;
    uint8_t unk5;
} PACKED subcmd_bb_killed_monster_t;

typedef struct subcmd_bb_player_died {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused1;
    uint8_t flag;
    uint8_t unk1;
    uint8_t unused2[2];
} PACKED subcmd_bb_player_died_t;

/* Packet used for grave data in C-Mode (Dreamcast) */
typedef struct subcmd_dc_grave {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t unused1;                    /* Unused? */
    uint8_t unused2;
    uint32_t client_id;

    //uint8_t unused3;
    //uint16_t unk0;

    uint32_t unk1;                      /* Matches with C-Data unk1 */
    char string[0x0C];                  /* Challenge Rank string */
    uint8_t unk2[0x24];                 /* Always blank? */
    uint16_t grave_unk4;
    uint16_t deaths;
    uint32_t coords_time[5];
    char team[20];
    char message[24];
    uint32_t times[9];
    uint32_t unk;
} PACKED subcmd_dc_grave_t;

/* Packet used for grave data in C-Mode (PC) */
typedef struct subcmd_pc_grave {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t unused1;                    /* Unused? */
    uint8_t unused2;
    uint32_t client_id;

    //uint8_t unused3;
    //uint16_t unk0;

    uint32_t unk1;                      /* Matches with C-Data unk1 */
    uint16_t string[0x0C];              /* Challenge Rank string */
    uint8_t unk2[0x24];                 /* Always blank? */
    uint16_t grave_unk4;
    uint16_t deaths;
    uint32_t coords_time[5];
    uint16_t team[20];
    uint16_t message[24];
    uint32_t times[9];
    uint32_t unk;
} PACKED subcmd_pc_grave_t;

/*挑战模式墓地数据 BB) */
//数据长度 = 0x0150; 336 = 8 + 1 + 1 + 1 + 1 + 4 + 320
typedef struct subcmd_bb_grave {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t unk1;
    uint8_t unk2;
    uint32_t client_id;
    uint8_t data[320];
} PACKED subcmd_bb_grave_t;

static int sdasd = sizeof(subcmd_bb_grave_t);

typedef struct subcmd_bb_use_tech {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t tech;
    uint8_t unused2;
    uint8_t level;
    uint8_t unused3;
} PACKED subcmd_bb_use_tech_t;

typedef struct subcmd_bb_death_sync {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unk1;
    uint8_t flag;
    uint8_t unk2[3];
} PACKED subcmd_bb_death_sync_t;

typedef struct subcmd_bb_cmd_3a {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unk1;
} PACKED subcmd_bb_cmd_3a_t;

typedef struct subcmd_bb_cmd_4e {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unk1;
} PACKED subcmd_bb_cmd_4e_t;

typedef struct subcmd_bb_switch_req {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unk1;
    uint8_t unk2;
    float x;
    float y;
    uint8_t unk3[2];
} PACKED subcmd_bb_switch_req_t;

typedef struct subcmd_bb_map_warp {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused[2];
    uint8_t flags; //80 为 总督府 00 为先驱者2号
    uint8_t unk[2];
    float x;
    float y;
    float z;
} PACKED subcmd_bb_map_warp_t;

typedef struct subcmd_bb_trade_menu {
    uint16_t menu_pos;
    uint16_t menu_id;
} PACKED subcmd_bb_trade_menu_t;

typedef struct subcmd_bb_trade_items {
    item_t titem[0x20][MAX_TRADE_ITEMS];
} PACKED subcmd_bb_trade_items_t;

/* 62处理交易数据包. (Blue Burst) */
typedef struct subcmd_bb_trade {
    bb_pkt_hdr_t hdr;                   /*18 00 62 00 00/01 00 00 00*/
    uint8_t type;                       /*A6*/
    uint8_t size;                       /*04*/
    uint16_t req_id;                    /*发起交易的人的序号为0000*/
    uint8_t trade_type;                 /*D0开始 D1 D2 D3 D4 D5交易取消*/
    uint8_t trade_stage;                /*00 02 04*/
    uint8_t unk2;                       /*71 00 AC F6 */
    uint8_t unk3;                       /*00 21*/
    subcmd_bb_trade_menu_t menu;
    //uint32_t menu_id;                   /*菜单序号*/
    uint32_t trade_num;                 /*目标序号为0001*/
    //uint16_t unused;                    /*一直为00 00*/
} PACKED subcmd_bb_trade_t;

/* 请求商店数据包. (Blue Burst) */
typedef struct subcmd_bb_shop_req {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unk;                       /* Always 0xFFFF? */
    uint8_t shop_type;
    uint8_t padding;
} PACKED subcmd_bb_shop_req_t;

/* Packet used for telling a client a shop's inventory. (Blue Burst) */
typedef struct subcmd_bb_shop_inv {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t params;  // 037F                 /* Set to 0 */
    uint8_t shop_type;
    uint8_t num_items;
    uint16_t unused2;                   /* Set to 0 */
    sitem_t items[0x14];
    //uint32_t unused3[4];                /* Set all to 0 */
} PACKED subcmd_bb_shop_inv_t;

/* Packet sent by the client to buy an item from the shop. (Blue Burst) */
typedef struct subcmd_bb_shop_buy {
    bb_pkt_hdr_t hdr;
    uint8_t type;//0x08
    uint8_t size;//0x09
    uint16_t unused1;//0x0A                   /* 0xFFFF (unused?) */
    uint32_t item_id;//0x0C                   /* 购买后生成的物品ID */
    uint8_t shop_type;//0x10                  /* 0 工具 1 武器 2 装甲*/
    uint8_t shop_index;//0x11                 /* 物品所处商店清单位置 */
    uint8_t num_bought;//0x12                 /* 购买数量 最高 10个 也就是 09*/
    uint8_t unknown_a1; // TODO: Probably actually unused; verify this
} PACKED subcmd_bb_shop_buy_t;

typedef struct subcmd_bb_sell_item {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint32_t item_id;
    uint32_t sell_num;
} PACKED subcmd_bb_sell_item_t;

typedef struct subcmd_bb_tekk_item {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    item_t item;
} PACKED subcmd_bb_tekk_item_t;

/* Packet sent by the client to notify that they're dropping part of a stack of
   items and tell where they're dropping them. (Blue Burst) */
typedef struct subcmd_bb_drop_pos {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t area;
    float x;
    float z;
    uint32_t item_id;
    uint32_t amount;
} PACKED subcmd_bb_drop_pos_t;

/* Packet sent to clients to let them know that an item got picked up off of
   the ground. (Blue Burst) */
typedef struct subcmd_bb_create_item {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    item_t item;
    //uint32_t item[3];
    //uint32_t item_id;
    //uint32_t item2;
    uint32_t unused2;
} PACKED subcmd_bb_create_item_t;

/* Packet sent by clients to open the bank menu. (Blue Burst) */
typedef struct subcmd_bb_bank_open {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;                    /* 0xFFFF */
    uint32_t unk;                       /* Maybe a checksum or somesuch? */
} PACKED subcmd_bb_bank_open_t;

/* Packet sent to clients to tell them what's in their bank. (Blue Burst) */
typedef struct subcmd_bb_bank_inv {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t unused[3];                  /* No size here, apparently. */
    uint32_t size;
    uint32_t checksum;
    uint32_t item_count;
    uint32_t meseta;
    bitem_t items[];
} PACKED subcmd_bb_bank_inv_t;

/* Packet sent by clients to do something at the bank. (Blue Burst) */
typedef struct subcmd_bb_bank_act {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;                    /* 0xFFFF */
    uint32_t item_id;
    uint32_t meseta_amount;
    uint8_t action;
    uint8_t item_amount;
    uint16_t unused2;                   /* 0xFFFF */
} PACKED subcmd_bb_bank_act_t;

/* Packet sent by clients to equip/unequip an item. */
typedef struct subcmd_bb_equip {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t item_id;
    uint32_t unk;
} PACKED subcmd_bb_equip_t;

/* Packet sent by clients to sort their inventory. (Blue Burst) */
typedef struct subcmd_bb_sort_inv {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unused;                    /* 0xFFFF */
    uint32_t item_ids[30];
} PACKED subcmd_bb_sort_inv_t;

/* Packet sent to clients to give them experience. (Blue Burst) */
typedef struct subcmd_bb_exp {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint32_t exp;
} PACKED subcmd_bb_exp_t;

/* Packet sent to clients regarding a level up. (Blue Burst)*/
typedef struct subcmd_bb_level {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t atp;
    uint16_t mst;
    uint16_t evp;
    uint16_t hp;
    uint16_t dfp;
    uint16_t ata;
    uint32_t level;
} PACKED subcmd_bb_level_t;

/* Packet sent by Blue Burst clients to request experience after killing an
   enemy. */
typedef struct subcmd_bb_req_exp {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t enemy_id2;
    uint16_t enemy_id;
    uint8_t client_id;
    uint8_t unused;
    uint8_t last_hitter;
    uint8_t unused2[3];
} PACKED subcmd_bb_req_exp_pkt_t;

typedef struct subcmd_bb_gallon_area {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unk1;                 /*FF FF*/
    uint16_t unk2;                 /*05 00 */
    uint8_t pos[4];
} PACKED subcmd_bb_gallon_area_pkt_t;

/* Packet sent by clients to say that a monster has been hit. */
typedef struct subcmd_mhit {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t enemy_id2;
    uint16_t enemy_id;
    uint16_t damage;
    uint32_t flags;
} PACKED subcmd_mhit_pkt_t;

typedef struct subcmd_bb_mhit {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t enemy_id2;
    uint16_t enemy_id;
    uint16_t damage;
    uint32_t flags;
} PACKED subcmd_bb_mhit_pkt_t;

/* Packet sent by clients to say that a box has been hit. */
typedef struct subcmd_bhit {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t box_id2;
    uint32_t unk;
    uint16_t box_id;
    uint16_t unk2;
} PACKED subcmd_bhit_pkt_t;

typedef struct subcmd_bb_bhit {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t box_id2;
    uint32_t unk;
    uint16_t box_id;
    uint16_t unk2;
} PACKED subcmd_bb_bhit_pkt_t;

/* Packet sent by clients to say that they killed a monster. Unfortunately, this
   doesn't always get sent (for instance for the Darvants during a Falz fight),
   thus its not actually used in Sylverant for anything. */
typedef struct subcmd_mkill {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t enemy_id;
    uint8_t client_id;
    uint8_t unused;
    uint16_t unk;
} PACKED subcmd_mkill_pkt_t;

/* Packet sent to create a pipe. Most of this, I haven't bothered looking too
   much at... */
typedef struct subcmd_pipe {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unk1;
    uint8_t client_id;
    uint8_t unused;
    uint8_t area_id;
    uint8_t unused2;
    uint32_t unk[5];                    /* Location is in here. */
} PACKED subcmd_pipe_pkt_t;

/* Packet sent when an object is hit by a technique. */
typedef struct subcmd_objhit_tech {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t tech;
    uint8_t unused2;
    uint8_t level;
    uint8_t hit_count;
    struct {
        uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
        uint16_t unused;
    } objects[];
} PACKED subcmd_objhit_tech_t;

/* Packet sent when an object is hit by a technique. */
typedef struct subcmd_bb_objhit_tech {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t tech;
    uint8_t unused2;
    uint8_t level;
    uint8_t hit_count;
    struct {
        uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
        uint16_t unused;
    } objects[];
} PACKED subcmd_bb_objhit_tech_t;

/* Packet sent when an object is hit by a physical attack. */
typedef struct subcmd_objhit_phys {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t hit_count;
    uint8_t unused2[3];
    struct {
        uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
        uint16_t unused;
    } objects[];
} PACKED subcmd_objhit_phys_t;

/* Packet sent when an object is hit by a physical attack. */
typedef struct subcmd_bb_objhit_phys {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint8_t hit_count;
    uint8_t unused2[3];
    struct {
        uint16_t obj_id;    /* 0xtiii -> i: id, t: type (4 object, 1 monster) */
        uint16_t unused;
    } objects[];
} PACKED subcmd_bb_objhit_phys_t;

/* Packet sent in response to a quest register sync (sync_register, sync_let,
   or sync_leti in qedit terminology). */
typedef struct subcmd_sync_reg {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unk1;          /* Probably unused junk. */
    uint8_t reg_num;
    uint8_t unused;
    uint16_t unk2;          /* Probably unused junk again. */
    uint32_t value;
} PACKED subcmd_sync_reg_t;

/* Packet sent in response to a quest register sync (sync_register, sync_let,
   or sync_leti in qedit terminology). */
typedef struct subcmd_bb_sync_reg {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t unk1;          /* Probably unused junk. */
    uint8_t reg_num;
    uint8_t unused;
    uint16_t unk2;          /* Probably unused junk again. */
    uint32_t value;
} PACKED subcmd_bb_sync_reg_t;

/* Packet sent when talking to an NPC on Pioneer 2 (and other purposes). */
typedef struct subcmd_talk_npc {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t unk;           /* Always 0xFFFF for NPCs? */
    uint16_t zero;          /* Always 0? */
    float x;
    float z;
    uint32_t unused2;       /* Always zero? */
} PACKED subcmd_talk_npc_t;

/* Packet sent when talking to an NPC on Pioneer 2 (and other purposes). */
typedef struct subcmd_bb_talk_npc {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t unused;
    uint16_t unk;           /* Always 0xFFFF for NPCs? */
    uint16_t zero;          /* Always 0? */
    float x;
    float z;
    uint32_t unused2;       /* Always zero? */
} PACKED subcmd_bb_talk_npc_t;

typedef struct subcmd_bb_chair_dir {
    bb_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint8_t client_id;
    uint8_t flag;
    uint8_t unk1[2];
    uint8_t unk2[2];
} PACKED subcmd_bb_chair_dir_t;

/* Packet used by the Dragon boss to deal with its actions. */
typedef struct subcmd_dragon_act {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t enemy_id;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
} PACKED subcmd_dragon_act_t;

/* Packet used by the Gol Dragon boss to deal with its actions. */
typedef struct subcmd_gol_dragon_act {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t size;
    uint16_t enemy_id;
    uint32_t unk[2];
    bitfloat_t x;
    bitfloat_t z;
    uint32_t unk2;
} PACKED subcmd_gol_dragon_act_t;

/* Packet used to communicate current state of players in-game while a new
   player is bursting. */
typedef struct subcmd_burst_pldata {
    dc_pkt_hdr_t hdr;
    uint8_t type;
    uint8_t unused1;
    uint16_t unused2;
    uint32_t size_minus_4;  /* ??? */
    uint32_t unk1[2];
    float x;
    float y;
    float z;
    uint8_t unk2[0x54];
    uint32_t tag;
    uint32_t guildcard;
    uint8_t unk3[0x44];
    uint8_t techs[20];
    char name[16];
    uint32_t c_unk1[2];
    uint32_t name_color;
    uint8_t model;
    uint8_t c_unused[15];
    uint32_t name_color_checksum;
    uint8_t section;
    uint8_t ch_class;
    uint8_t v2flags;
    uint8_t version;
    uint32_t v1flags;
    uint16_t costume;
    uint16_t skin;
    uint16_t face;
    uint16_t head;
    uint16_t hair;
    uint16_t hair_r;
    uint16_t hair_g;
    uint16_t hair_b;
    float prop_x;
    float prop_y;
    uint16_t atp;
    uint16_t mst;
    uint16_t evp;
    uint16_t hp;
    uint16_t dfp;
    uint16_t ata;
    uint16_t lck;
    uint16_t c_unk2;
    uint32_t c_unk3[2];
    uint32_t level;
    uint32_t exp;
    uint32_t meseta;
    inventory_t inv;
    uint32_t zero;          /* Unused? */
    /* Xbox has a little bit more at the end than other versions... Probably
       safe to just ignore it. */
} PACKED subcmd_burst_pldata_t;

#ifndef _WIN32
#else
#pragma pack()
#endif

#undef PACKED

/* Handle a 0x62/0x6D packet. */
int subcmd_handle_one(ship_client_t *c, subcmd_pkt_t *pkt);
int subcmd_bb_handle_one(ship_client_t *c, bb_subcmd_pkt_t *pkt);

/* Handle a 0x60 packet. */
int subcmd_handle_bcast(ship_client_t *c, subcmd_pkt_t *pkt);
int subcmd_bb_handle_bcast(ship_client_t *c, bb_subcmd_pkt_t *pkt);
int subcmd_dcnte_handle_bcast(ship_client_t *c, subcmd_pkt_t *pkt);

/* Handle an 0xC9/0xCB packet from Episode 3. */
int subcmd_handle_ep3_bcast(ship_client_t *c, subcmd_pkt_t *pkt);

int subcmd_send_lobby_item(lobby_t *l, subcmd_itemreq_t *req,
                           const uint32_t item[4]);
int subcmd_send_bb_lobby_item(lobby_t *l, subcmd_bb_itemreq_t *req,
                              const iitem_t *item);

int subcmd_send_bb_exp(ship_client_t *c, uint32_t exp);
int subcmd_send_bb_level(ship_client_t *c);

int subcmd_send_pos(ship_client_t *dst, ship_client_t *src);

/* Send a broadcast subcommand to the whole lobby. */
int subcmd_send_lobby_dc(lobby_t *l, ship_client_t *c, subcmd_pkt_t *pkt,
                         int igcheck);
int subcmd_send_lobby_bb(lobby_t *l, ship_client_t *c, bb_subcmd_pkt_t *pkt,
                         int igcheck);
int subcmd_send_lobby_dcnte(lobby_t *l, ship_client_t *c, subcmd_pkt_t *pkt,
                            int igcheck);

/* Stuff dealing with the Dreamcast Network Trial edition */
int subcmd_translate_dc_to_nte(ship_client_t *c, subcmd_pkt_t *pkt);
int subcmd_translate_nte_to_dc(ship_client_t *c, subcmd_pkt_t *pkt);
int subcmd_translate_bb_to_nte(ship_client_t *c, bb_subcmd_pkt_t *pkt);

#endif /* !SUBCMD_H */
