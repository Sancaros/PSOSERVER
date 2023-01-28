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

/* Subcommand types we care about (0x62/0x6D). */
#define SUBCMD_GUILDCARD    0x06
#define SUBCMD_PICK_UP      0x5A    /* Sent to leader when picking up item */
#define SUBCMD_ITEMREQ      0x60    //SUBCMD0x62_MONSTER_DROP_ITEMREQ
#define SUBCMD0x62_UNKNOW_6A    0x6A
#define SUBCMD0x62_BURST2    0x6B
#define SUBCMD0x62_BURST3    0x6C
#define SUBCMD0x62_BURST1    0x6D
#define SUBCMD0x62_BURST4    0x6E
#define SUBCMD0x62_BURST5    0x6F
#define SUBCMD0x62_BURST7    0x70
#define SUBCMD0x62_BURST6    0x71
#define SUBCMD_BITEMREQ     0xA2    /*SUBCMD0x62_BOX_DROP_ITEMREQ BB/GC - Request item drop from box */
#define SUBCMD0x62_TRADE     0xA6    /* BB/GC - Player Trade function */
#define SUBCMD0x62_UNKNOW_A8 0xA8 // Gol Dragon actions
#define SUBCMD0x62_UNKNOW_A9 0xA9 // Barba Ray actions
#define SUBCMD0x62_UNKNOW_AA 0xAA // Episode 2 boss actions
#define SUBCMD0x62_CHARACTER_INFO                0xAE
#define SUBCMD_SHOP_REQ      0xB5    /* Blue Burst - Request shop inventory */
#define SUBCMD_SHOP_BUY      0xB7    /* Blue Burst - Buy an item from the shop */
#define SUBCMD0x62_TEKKING                       0xB8    /* Blue Burst - Client is tekking a weapon */
#define SUBCMD0x62_TEKKED                        0xBA    /* Blue Burst - Client is tekked a weapon */
#define SUBCMD_OPEN_BANK    0xBB    /* Blue Burst - open the bank menu */
#define SUBCMD_BANK_ACTION  0xBD    /* Blue Burst - do something at the bank */
#define SUBCMD0x62_GUILD_INVITE1                 0xC1
#define SUBCMD0x62_GUILD_INVITE2                 0xC2
#define SUBCMD0x62_GUILD_MASTER_TRANS1           0xCD
#define SUBCMD0x62_GUILD_MASTER_TRANS2           0xCE
#define SUBCMD0x62_QUEST_ITEM_UNKNOW1            0xC9
#define SUBCMD0x62_QUEST_ITEM_RECEIVE            0xCA
#define SUBCMD0x62_BATTLE_CHAR_LEVEL_FIX         0xD0
#define SUBCMD0x62_CH_GRAVE_DATA                 0xD1
#define SUBCMD0x62_WARP_ITEM                     0xD6
#define SUBCMD0x62_QUEST_ONEPERSON_SET_ITEM      0xDF
#define SUBCMD0x62_QUEST_ONEPERSON_SET_BP        0xE0
#define SUBCMD0x62_GANBLING                      0xE2

///////////////////////////////////////////////////////////////////////////////
/* Subcommand types we might care about (0x60/0x6C). */
#define SUBCMD0x60_SWITCH_CHANGED    0x05 // 触发机关开启
#define SUBCMD_SYMBOL_CHAT  0x07
#define SUBCMD_HIT_MONSTER  0x0A
#define SUBCMD_HIT_OBJ      0x0B
#define SUBCMD_CONDITION_ADD       0x0C // Add condition (poison/slow/etc.)
#define SUBCMD_CONDITION_REMOVE    0x0D // Remove condition (poison/slow/etc.)
#define SUBCMD0x60_ACTION_DRAGON       0x12
#define SUBCMD0x60_ACTION_DE_ROl_LE    0x13
#define SUBCMD0x60_UNKNOW_14           0x14
#define SUBCMD0x60_ACTION_VOL_OPT      0x15
#define SUBCMD0x60_ACTION_VOL_OPT2     0x16
#define SUBCMD_TELEPORT     0x17 //SUBCMD0x60_TELEPORT_1
#define SUBCMD_DRAGON_ACT   0x18    /* Dragon special actions */
#define SUBCMD0x60_ACTION_DARK_FALZ    0x19
#define SUBCMD0x60_UNKNOW_1C    0x1C
#define SUBCMD_SET_AREA     0x1F
#define SUBCMD0x60_SET_AREA_20  0x20    /* Seems to match 0x1F */
#define SUBCMD_SET_AREA_21  0x21    /* Seems to match 0x1F */
#define SUBCMD_LOAD_22      0x22    /* Set player visibility Related to 0x21 and 0x23... */
#define SUBCMD_FINISH_LOAD  0x23    /* Finished loading to a map, maybe? */
#define SUBCMD_SET_POS_24   0x24    /* Used when starting a quest. */
#define SUBCMD_EQUIP        0x25
#define SUBCMD_REMOVE_EQUIP 0x26
#define SUBCMD_USE_ITEM     0x27
#define SUBCMD0x60_FEED_MAG     0x28
#define SUBCMD_DELETE_ITEM  0x29    /* Selling, deposit in bank, etc */
#define SUBCMD_DROP_ITEM    0x2A    /* Drop full stack or non-stack item */
#define SUBCMD_TAKE_ITEM    0x2B
#define SUBCMD_TALK_NPC     0x2C    /* Maybe this is talking to an NPC? */
#define SUBCMD_TALK_NPC_SIZE 0x01
#define SUBCMD_DONE_NPC     0x2D    /*SUBCMD_DONE_NPC Shows up when you're done with an NPC */
#define SUBCMD_DONE_NPC_SIZE 0x05
#define SUBCMD0x60_UNKNOW_2E    0x2E
#define SUBCMD0x60_UNKNOW_2F    0x2F
#define SUBCMD_LEVELUP      0x30
#define SUBCMD0x60_UNKNOW_MEDIC_31    0x31
#define SUBCMD0x60_UNKNOW_MEDIC_32    0x32
#define SUBCMD0x60_UNKNOW_33    0x33
#define SUBCMD0x60_UNKNOW_34    0x34
#define SUBCMD0x60_UNKNOW_35    0x35
#define SUBCMD0x60_UNKNOW_36    0x36
#define SUBCMD0x60_UNKNOW_37    0x37
#define SUBCMD0x60_UNKNOW_38    0x38
#define SUBCMD0x60_UNKNOW_39    0x39
#define SUBCMD0x60_UNKNOW_3A    0x3A
#define SUBCMD_LOAD_3B      0x3B    /* Something with loading to a map... */
#define SUBCMD_LOAD_3B_SIZE 0x01
#define SUBCMD0x60_UNKNOW_3C    0x3C
#define SUBCMD0x60_UNKNOW_3D    0x3D
#define SUBCMD_SET_POS_3E   0x3E
#define SUBCMD_SET_POS_3F   0x3F
#define SUBCMD_MOVE_SLOW    0x40
#define SUBCMD0x60_UNKNOW_41    0x41
#define SUBCMD_MOVE_FAST    0x42
#define SUBCMD0x60_ATTACK1    0x43
#define SUBCMD0x60_ATTACK2    0x44
#define SUBCMD0x60_ATTACK3    0x45
#define SUBCMD0x60_ATTACK_SIZE    0x10
#define SUBCMD_OBJHIT_PHYS  0x46
#define SUBCMD_OBJHIT_TECH  0x47
#define SUBCMD_USED_TECH    0x48
#define SUBCMD0x60_UNKNOW_49    0x49
#define SUBCMD0x60_TAKE_DAMAGE  0x4A //受到攻击并防御了
#define SUBCMD_TAKE_DAMAGE1 0x4B
#define SUBCMD_TAKE_DAMAGE2 0x4C
#define SUBCMD0x60_DEATH_SYNC   0x4D
#define SUBCMD0x60_UNKNOW_4E    0x4E
#define SUBCMD0x60_UNKNOW_4F    0x4F
#define SUBCMD0x60_REQ_SWITCH   0x50
#define SUBCMD0x60_UNKNOW_51    0x51
#define SUBCMD0x60_MENU_REQ    0x52    /*SUBCMD_TALK_SHOP Talking to someone at a shop */
#define SUBCMD0x60_UNKNOW_53    0x53
#define SUBCMD0x60_UNKNOW_54    0x54
#define SUBCMD_WARP_55      0x55    /* 传送至总督府触发 */
#define SUBCMD0x60_UNKNOW_56    0x56
#define SUBCMD0x60_UNKNOW_57    0x57
#define SUBCMD_LOBBY_ACTION 0x58
#define SUBCMD_DEL_MAP_ITEM 0x59    /* Sent by leader when item picked up */
#define SUBCMD0x60_UNKNOW_5A    0x5A
#define SUBCMD0x60_UNKNOW_5B    0x5B
#define SUBCMD0x60_UNKNOW_5C    0x5C
#define SUBCMD_DROP_STACK   0x5D
#define SUBCMD_BUY          0x5E
#define SUBCMD_ITEMDROP     0x5F
#define SUBCMD0x60_UNKNOW_60    0x60
#define SUBCMD0x60_LEVELUP    0x61
#define SUBCMD0x60_UNKNOW_62    0x62
#define SUBCMD_DESTROY_ITEM 0x63    /* Sent when game inventory is full */
#define SUBCMD0x60_UNKNOW_64    0x64
#define SUBCMD0x60_UNKNOW_65    0x65
#define SUBCMD0x60_UNKNOW_66    0x66
#define SUBCMD0x60_CREATE_ENEMY_SET    0x67 // Create enemy set
#define SUBCMD_CREATE_PIPE  0x68
#define SUBCMD_SPAWN_NPC    0x69
#define SUBCMD0x60_UNKNOW_6A    0x6A
#define SUBCMD0x60_UNKNOW_6B    0x6B
#define SUBCMD0x60_UNKNOW_6C    0x6C
#define SUBCMD0x60_UNKNOW_6D    0x6D
#define SUBCMD0x60_UNKNOW_6E    0x6E
#define SUBCMD0x60_UNKNOW_6F    0x6F
#define SUBCMD0x60_UNKNOW_70    0x70
#define SUBCMD0x60_UNKNOW_71    0x71
#define SUBCMD_BURST_DONE   0x72
#define SUBCMD0x60_UNKNOW_73    0x73
#define SUBCMD_WORD_SELECT  0x74
#define SUBCMD0x60_SET_FLAG     0x75
#define SUBCMD_KILL_MONSTER 0x76    /* A monster was killed. */
#define SUBCMD_SYNC_REG     0x77    /* Sent when register is synced in quest */
#define SUBCMD0x60_UNKNOW_78    0x78
#define SUBCMD_GOGO_BALL    0x79
#define SUBCMD0x60_UNKNOW_7A    0x7A
#define SUBCMD0x60_UNKNOW_7B    0x7B
#define SUBCMD_CMODE_GRAVE      0x7C
#define SUBCMD0x60_UNKNOW_7D    0x7D
#define SUBCMD0x60_UNKNOW_7E    0x7E
#define SUBCMD0x60_UNKNOW_7F    0x7F
#define SUBCMD0x60_UNKNOW_80    0x80
#define SUBCMD0x60_UNKNOW_81    0x81
#define SUBCMD0x60_UNKNOW_82    0x82
#define SUBCMD0x60_UNKNOW_83    0x83
#define SUBCMD0x60_UNKNOW_84    0x84
#define SUBCMD0x60_UNKNOW_85    0x85
#define SUBCMD0x60_UNKNOW_86    0x86
#define SUBCMD0x60_UNKNOW_87    0x87
#define SUBCMD0x60_UNKNOW_88    0x88
#define SUBCMD0x60_UNKNOW_89    0x89
#define SUBCMD0x60_UNKNOW_8A    0x8A
#define SUBCMD0x60_UNKNOW_8B    0x8B
#define SUBCMD0x60_UNKNOW_8C    0x8C
#define SUBCMD0x60_USE_TECH     0x8D //释放法术 TODO 未完成法术有效性分析
#define SUBCMD0x60_UNKNOW_8E    0x8E
#define SUBCMD0x60_UNKNOW_8F    0x8F
#define SUBCMD0x60_UNKNOW_90    0x90
#define SUBCMD0x60_UNKNOW_91    0x91
#define SUBCMD0x60_UNKNOW_92    0x92
#define SUBCMD0x60_UNKNOW_93    0x93
#define SUBCMD_WARP         0x94
#define SUBCMD0x60_UNKNOW_95    0x95
#define SUBCMD0x60_UNKNOW_96    0x96
#define SUBCMD0x60_UNKNOW_97    0x97
#define SUBCMD0x60_UNKNOW_98    0x98
#define SUBCMD0x60_UNKNOW_99    0x99
#define SUBCMD_CHANGE_STAT  0x9A
#define SUBCMD0x60_UNKNOW_9B    0x9B
#define SUBCMD0x60_UNKNOW_9C    0x9C
#define SUBCMD0x60_UNKNOW_9D    0x9D
#define SUBCMD0x60_UNKNOW_9E    0x9E
#define SUBCMD0x60_UNKNOW_9F    0x9F
#define SUBCMD_GDRAGON_ACT    0xA8    /* Gol Dragon special actions */
#define SUBCMD0x60_UNKNOW_A9    0xA9 // Barba Ray actions
#define SUBCMD0x60_UNKNOW_AA    0xAA // Episode 2 boss actions
#define SUBCMD_LOBBY_CHAIR  0xAB // Create lobby chair
#define SUBCMD0x60_UNKNOW_AC    0xAC
#define SUBCMD0x60_UNKNOW_AD    0xAD // Olga Flow phase 2 subordinate boss actions
#define SUBCMD0x60_UNKNOW_AE    0xAE
#define SUBCMD_CHAIR_DIR    0xAF // 旋转椅子朝向
#define SUBCMD_CHAIR_MOVE   0xB0 // 移动椅子
#define SUBCMD0x60_UNKNOW_B5    0xB5 // BB shop request process_subcommand_open_shop_bb_or_unknown_ep3
#define SUBCMD_SHOP_INV     0xB6    /* Blue Burst - shop inventory */
#define SUBCMD0x60_UNKNOW_B7    0xB7 // process_subcommand_buy_shop_item_bb
#define SUBCMD0x60_UNKNOW_B8    0xB8 // process_subcommand_identify_item_bb
#define SUBCMD0x60_UNKNOW_B9    0xB9 // process_subcommand_unimplemented
#define SUBCMD0x60_UNKNOW_BA    0xBA // process_subcommand_accept_identify_item_bb
#define SUBCMD0x60_UNKNOW_BB    0xBB //process_subcommand_open_bank_bb
#define SUBCMD_BANK_INV     0xBC    /* Blue Burst - bank inventory */
#define SUBCMD0x60_UNKNOW_BD    0xBD //process_subcommand_bank_action_bb
#define SUBCMD_CREATE_ITEM  0xBE    /* Blue Burst - create new inventory item */
#define SUBCMD_JUKEBOX      0xBF    /* Episode III - Change background music */
#define SUBCMD_GIVE_EXP     0xBF    /* Blue Burst - give experience points */
#define SUBCMD0x60_SELL_ITEM    0xC0
#define SUBCMD_DROP_POS     0xC3    /* Blue Burst - Drop part of stack coords */
#define SUBCMD_SORT_INV     0xC4    /* Blue Burst - Sort inventory */
#define SUBCMD_MEDIC        0xC5    /* Blue Burst - Use the medical center */
#define SUBCMD0x60_STEAL_EXP    0xC6
#define SUBCMD0x60_CHARGE_ACT   0xC7
#define SUBCMD_REQ_EXP      0xC8    /* Blue Burst - Request Experience */
#define SUBCMD0x60_EX_ITEM_TEAM 0xCC
#define SUBCMD0x60_BATTLEMODE   0xCF
#define SUBCMD0x60_GALLON_AREA  0xD2
#define SUBCMD0x60_TRADE_DONE   0xD4
#define SUBCMD0x60_EX_ITEM      0xD5
#define SUBCMD0x60_PD_TREADE    0xD7
#define SUBCMD0x60_SRANK_ATTR   0xD8
#define SUBCMD0x60_EX_ITEM_MK   0xD9
#define SUBCMD0x60_PD_COMPARE   0xDA
#define SUBCMD0x60_UNKNOW_DC    0xDC
#define SUBCMD0x60_UNKNOW_DE    0xDE
#define SUBCMD0x60_GALLON_PLAN  0xE1


/* Subcommands that we might care about in the Dreamcast NTE (0x60) */
#define SUBCMD_DCNTE_SET_AREA       0x1D    /* 0x21 */
#define SUBCMD_DCNTE_FINISH_LOAD    0x1F    /* 0x23 */
#define SUBCMD_DCNTE_SET_POS        0x36    /* 0x3F */
#define SUBCMD_DCNTE_MOVE_SLOW      0x37    /* 0x40 */
#define SUBCMD_DCNTE_MOVE_FAST      0x39    /* 0x42 */
#define SUBCMD_DCNTE_TALK_SHOP      0x46    /* 0x52 */

/* The commands OK to send during bursting (0x62/0x6D). These are named for the
   order in which they're sent, hence why the names are out of order... */
#define SUBCMD_BURST2       0x6B //SUBCMD0x62_BURST2
#define SUBCMD_BURST3       0x6C //SUBCMD0x62_BURST3
#define SUBCMD_BURST1       0x6D //SUBCMD0x62_BURST1
#define SUBCMD_BURST4       0x6E //SUBCMD0x62_BURST4
#define SUBCMD_BURST5       0x6F //SUBCMD0x62_BURST5
#define SUBCMD_BURST_PLDATA       0x70 //SUBCMD0x62_BURST7 /* Was SUBCMD_BURST7 */
#define SUBCMD_BURST6       0x71 //SUBCMD0x62_BURST6

/* The commands OK to send during bursting (0x60) */
/* 0x3B */
#define SUBCMD_UNK_7C       0x7C

/* Stats to use with Subcommand 0x9A (0x60) */
#define SUBCMD_STAT_HPDOWN  0
#define SUBCMD_STAT_TPDOWN  1
#define SUBCMD_STAT_MESDOWN 2
#define SUBCMD_STAT_HPUP    3
#define SUBCMD_STAT_TPUP    4

/* Actions that can be performed at the bank with Subcommand 0xBD (0x62) */
#define SUBCMD_BANK_ACT_DEPOSIT 0
#define SUBCMD_BANK_ACT_TAKE    1
#define SUBCMD_BANK_ACT_DONE    2
#define SUBCMD_BANK_ACT_CLOSE   3

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
